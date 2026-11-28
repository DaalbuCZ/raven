/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* CreateStreamOverRandomAccessStream, the shcore interop that hands a classic IStream consumer a WinRT stream: the IStream face
 * wraps the random-access stream's reads, so programs that work in IStream terms (Direct2D's CreateSvgDocument among them) can
 * take what the Windows Runtime hands them. The reads go through the stream's own IInputStream face, one ReadAsync per read with
 * the answer copied out; a Seek remembers the position and the next read re-derives the input stream there, as the forward-only
 * face demands. Writes answer as the traced gap they are until an IOutputStream consumer needs them. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <winstring.h>
#include <objidl.h>
#include <windows.storage.streams.h>
#include <robuffer.h>

#ifndef RO_E_CLOSED
#define RO_E_CLOSED ((HRESULT)0x80000013)
#endif

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

static int equal_iid(REFIID left, const IID *right)
{
	return left->Data1 == right->Data1 && left->Data2 == right->Data2 && left->Data3 == right->Data3 &&
		left->Data4[0] == right->Data4[0] && left->Data4[1] == right->Data4[1] &&
		left->Data4[2] == right->Data4[2] && left->Data4[3] == right->Data4[3] &&
		left->Data4[4] == right->Data4[4] && left->Data4[5] == right->Data4[5] &&
		left->Data4[6] == right->Data4[6] && left->Data4[7] == right->Data4[7];
}

static HRESULT create_string(const WCHAR *text, HSTRING *out)
{
	if (!out)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), out);
}

static HRESULT inspectable_iids(ULONG *count, IID **iids)
{
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}

static HRESULT inspectable_trust(TrustLevel *level)
{
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

/* The buffer a read fills, from the DataWriter component's Buffer: capacity set, length written back after the read. */
HRESULT raven_xaml_buffer_create(UINT32 capacity, __x_ABI_CWindows_CStorage_CStreams_CIBuffer **out);

struct ras_over {
	IStream iface;
	LONG refs;
	__x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStream *ras;
	UINT64 position;
};

static struct ras_over *ras_owner(IStream *iface) { return (struct ras_over *)iface; }

static HRESULT WINAPI over_query(IStream *iface, REFIID iid, void **out)
{
	struct ras_over *self = ras_owner(iface);
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_ISequentialStream) || equal_iid(iid, &IID_IStream))
		*out = iface;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI over_addref(IStream *iface)
{
	return (ULONG)InterlockedIncrement(&ras_owner(iface)->refs);
}

static ULONG WINAPI over_release(IStream *iface)
{
	struct ras_over *self = ras_owner(iface);
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->ras)
			self->ras->lpVtbl->Release(self->ras);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

/* The stream's own size, as the stat and the end-origin seek both read it. */
static HRESULT over_size(struct ras_over *self, UINT64 *size)
{
	if (!self->ras->lpVtbl->get_Size)
		return E_NOTIMPL;
	return self->ras->lpVtbl->get_Size(self->ras, size);
}

/* One read: a fresh IInputStream at the remembered position (the face is forward-only), one ReadAsync into a buffer of the
   caller's capacity, the answer copied out and the buffer released. The operation completes as it is made, so the results
   come back at once. */
static HRESULT WINAPI over_read(IStream *iface, void *buffer, ULONG size, ULONG *done)
{
	struct ras_over *self = ras_owner(iface);
	__x_ABI_CWindows_CStorage_CStreams_CIInputStream *input = NULL;
	__FIAsyncOperationWithProgress_2_Windows__CStorage__CStreams__CIBuffer_UINT32 *operation = NULL;
	__x_ABI_CWindows_CStorage_CStreams_CIBuffer *filled = NULL;
	__x_ABI_CWindows_CStorage_CStreams_CIBuffer *read_buffer = NULL;
	HRESULT result;
	if (done)
		*done = 0;
	if (!self->ras)
		return RO_E_CLOSED;
	if (!buffer || (size && !done))
		return E_POINTER;
	if (!size)
		return S_OK;
	result = self->ras->lpVtbl->GetInputStreamAt(self->ras, self->position, &input);
	if (FAILED(result))
		return result;
	result = raven_xaml_buffer_create(size, &read_buffer);
	if (SUCCEEDED(result))
		result = input->lpVtbl->ReadAsync(input, read_buffer, size, InputStreamOptions_None, &operation);
	if (SUCCEEDED(result) && operation)
		result = operation->lpVtbl->GetResults(operation, &filled);
	if (SUCCEEDED(result) && filled) {
		__x_Windows_CStorage_CStreams_CIBufferByteAccess *access = NULL;
		result = filled->lpVtbl->QueryInterface(filled, &IID___x_Windows_CStorage_CStreams_CIBufferByteAccess, (void **)&access);
		if (SUCCEEDED(result)) {
			BYTE *bytes = NULL;
			UINT32 length = 0;
			result = access->lpVtbl->Buffer(access, &bytes);
			if (SUCCEEDED(result))
				result = filled->lpVtbl->get_Length(filled, &length);
			if (SUCCEEDED(result) && bytes) {
				if (length > size)
					length = size;
				memcpy(buffer, bytes, length);
				if (done)
					*done = length;
				self->position += length;
			}
			access->lpVtbl->Release(access);
		}
		filled->lpVtbl->Release(filled);
	}
	if (read_buffer)
		read_buffer->lpVtbl->Release(read_buffer);
	if (operation)
		operation->lpVtbl->Release(operation);
	input->lpVtbl->Release(input);
	return result;
}

static HRESULT WINAPI over_write(IStream *iface, const void *buffer, ULONG size, ULONG *written)
{
	(void)iface; (void)buffer;
	if (written)
		*written = 0;
	raven_trace_not_implemented("CreateStreamOverRandomAccessStream_Write");
	return E_NOTIMPL;
}

static HRESULT WINAPI over_seek(IStream *iface, LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER *new_position)
{
	struct ras_over *self = ras_owner(iface);
	UINT64 base;
	HRESULT result = S_OK;
	if (!self->ras)
		return RO_E_CLOSED;
	switch (origin) {
	case STREAM_SEEK_SET:
		base = 0;
		break;
	case STREAM_SEEK_CUR:
		base = self->position;
		break;
	case STREAM_SEEK_END:
		result = over_size(self, &base);
		if (FAILED(result))
			return result;
		break;
	default:
		return STG_E_INVALIDFUNCTION;
	}
	/* A seek past either end is a bounds error, as the random-access face's Seek answers. */
	if (move.QuadPart < 0 && (UINT64)(-move.QuadPart) > base)
		return E_BOUNDS;
	self->position = (UINT64)((LONGLONG)base + move.QuadPart);
	if (new_position)
		new_position->QuadPart = self->position;
	return S_OK;
}

static HRESULT WINAPI over_set_size(IStream *iface, ULARGE_INTEGER size)
{
	struct ras_over *self = ras_owner(iface);
	HRESULT result;
	if (!self->ras)
		return RO_E_CLOSED;
	if (!self->ras->lpVtbl->put_Size)
		return E_NOTIMPL;
	result = self->ras->lpVtbl->put_Size(self->ras, size.QuadPart);
	if (result == E_NOTIMPL)
		raven_trace_not_implemented("CreateStreamOverRandomAccessStream_SetSize");
	return result;
}

static HRESULT WINAPI over_copy_to(IStream *iface, IStream *target, ULARGE_INTEGER size, ULARGE_INTEGER *read, ULARGE_INTEGER *written)
{
	struct ras_over *self = ras_owner(iface);
	BYTE stage[8192];
	ULARGE_INTEGER left = size, moved = {0, 0}, took = {0, 0};
	HRESULT result = S_OK;
	if (!target)
		return E_POINTER;
	if (read)
		read->QuadPart = 0;
	if (written)
		written->QuadPart = 0;
	while (left.QuadPart && SUCCEEDED(result)) {
		ULONG chunk = left.QuadPart < sizeof stage ? (ULONG)left.QuadPart : sizeof stage, got = 0, put = 0;
		result = over_read(iface, stage, chunk, &got);
		if (FAILED(result) || !got)
			break;
		result = target->lpVtbl->Write(target, stage, got, &put);
		if (FAILED(result))
			break;
		moved.QuadPart += got;
		took.QuadPart += put;
		left.QuadPart -= got;
	}
	if (read)
		*read = moved;
	if (written)
		*written = took;
	if (SUCCEEDED(result) && left.QuadPart)
		result = S_FALSE; /* the stream ended before the requested span */
	return result;
}

static HRESULT WINAPI over_commit(IStream *iface, DWORD flags) { (void)iface; (void)flags; return S_OK; }
static HRESULT WINAPI over_revert(IStream *iface) { (void)iface; return S_OK; }
static HRESULT WINAPI over_lock(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD lock) { (void)iface; (void)offset; (void)size; (void)lock; return S_OK; }
static HRESULT WINAPI over_unlock(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD lock) { (void)iface; (void)offset; (void)size; (void)lock; return S_OK; }

static HRESULT WINAPI over_stat(IStream *iface, STATSTG *stat, DWORD flags)
{
	struct ras_over *self = ras_owner(iface);
	UINT64 size = 0;
	HRESULT result;
	if (!stat)
		return STG_E_INVALIDPOINTER;
	memset(stat, 0, sizeof *stat);
	if (!self->ras)
		return RO_E_CLOSED;
	result = over_size(self, &size);
	if (FAILED(result))
		return result;
	stat->type = STGTY_STREAM;
	stat->cbSize.QuadPart = size;
	stat->grfMode = STGM_READ;
	/* The name costs an allocation Windows only pays when the caller asks for it, and the caller releases it with the task allocator. */
	if ((flags & STATFLAG_NONAME))
		return S_OK;
	{
		static const WCHAR name[] = L"RandomAccessStreamOverStream";
		if (!(stat->pwcsName = CoTaskMemAlloc(sizeof name)))
			return STG_E_INSUFFICIENTMEMORY;
		memcpy(stat->pwcsName, name, sizeof name);
	}
	return S_OK;
}

static HRESULT WINAPI over_clone(IStream *iface, IStream **out)
{
	struct ras_over *self = ras_owner(iface);
	struct ras_over *clone;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!self->ras)
		return RO_E_CLOSED;
	if (!(clone = ALLOC(sizeof *clone)))
		return E_OUTOFMEMORY;
	clone->iface.lpVtbl = iface->lpVtbl;
	clone->refs = 1;
	clone->ras = self->ras;
	clone->position = self->position;
	clone->ras->lpVtbl->AddRef(clone->ras);
	InterlockedIncrement(&raven_xaml_objects);
	*out = &clone->iface;
	return S_OK;
}

static const IStreamVtbl over_table = {
	over_query, over_addref, over_release,
	over_read, over_write, over_seek, over_set_size, over_copy_to,
	over_commit, over_revert, over_lock, over_unlock, over_stat, over_clone,
};

/* The runtime class the shcore interop builds: an IStream over the random-access stream the caller hands in. */
__declspec(dllexport) HRESULT WINAPI CreateStreamOverRandomAccessStream(IUnknown *random_access_stream, REFIID iid, void **out)
{
	struct ras_over *self;
	__x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStream *ras = NULL;
	HRESULT result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!random_access_stream)
		{ RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
	result = random_access_stream->lpVtbl->QueryInterface(random_access_stream,
		&IID___x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStream, (void **)&ras);
	if (FAILED(result))
		{ RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
	if (!(self = ALLOC(sizeof *self))) {
		ras->lpVtbl->Release(ras);
		return E_OUTOFMEMORY;
	}
	self->iface.lpVtbl = &over_table;
	self->refs = 1;
	self->ras = ras;
	self->position = 0;
	InterlockedIncrement(&raven_xaml_objects);
	result = over_query(&self->iface, iid, out);
	over_release(&self->iface);
	return result;
}
