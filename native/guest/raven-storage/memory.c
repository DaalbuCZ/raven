/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
#include "docfile.h"
#include <string.h>

/* The bytes under a storage: a file, or global memory. Streams on global memory live here too;
 * clones of one stream share the block and each keep their own position. */

typedef struct file_bytes {
	ILockBytes ILockBytes_iface;
	LONG refs;
	HANDLE file;
} file_bytes;

static HRESULT status_of_last_error(void)
{
	DWORD error = GetLastError();

	switch (error) {
	case ERROR_FILE_NOT_FOUND:
		return STG_E_FILENOTFOUND;
	case ERROR_PATH_NOT_FOUND:
		return STG_E_PATHNOTFOUND;
	case ERROR_ACCESS_DENIED:
		return STG_E_ACCESSDENIED;
	case ERROR_SHARING_VIOLATION:
		return STG_E_SHAREVIOLATION;
	case ERROR_LOCK_VIOLATION:
		return STG_E_LOCKVIOLATION;
	case ERROR_FILE_EXISTS:
	case ERROR_ALREADY_EXISTS:
		return STG_E_FILEALREADYEXISTS;
	case ERROR_INVALID_NAME:
		return STG_E_INVALIDNAME;
	case ERROR_DISK_FULL:
		return STG_E_MEDIUMFULL;
	default:
		return HRESULT_FROM_WIN32(error);
	}
}

static file_bytes *file_bytes_of(ILockBytes *iface)
{
	return CONTAINING_RECORD(iface, file_bytes, ILockBytes_iface);
}

static HRESULT WINAPI file_bytes_QueryInterface(ILockBytes *iface, REFIID riid, void **out)
{
	if (!out)
		return E_INVALIDARG;
	if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ILockBytes)) {
		*out = iface;
		ILockBytes_AddRef(iface);
		return S_OK;
	}
	*out = NULL;
	return E_NOINTERFACE;
}

static ULONG WINAPI file_bytes_AddRef(ILockBytes *iface)
{
	return InterlockedIncrement(&file_bytes_of(iface)->refs);
}

static ULONG WINAPI file_bytes_Release(ILockBytes *iface)
{
	file_bytes *object = file_bytes_of(iface);
	ULONG refs = InterlockedDecrement(&object->refs);

	if (!refs) {
		CloseHandle(object->file);
		mem_free(object);
	}
	return refs;
}

static BOOL move_to(HANDLE file, ULARGE_INTEGER offset)
{
	LARGE_INTEGER at;

	at.QuadPart = (LONGLONG)offset.QuadPart;
	return SetFilePointerEx(file, at, NULL, FILE_BEGIN);
}

static HRESULT WINAPI file_bytes_ReadAt(ILockBytes *iface, ULARGE_INTEGER offset, void *buffer, ULONG count, ULONG *read)
{
	file_bytes *object = file_bytes_of(iface);
	ULONG done = 0;

	if (read)
		*read = 0;
	if (!move_to(object->file, offset) || !ReadFile(object->file, buffer, count, &done, NULL))
		return status_of_last_error();
	if (read)
		*read = done;
	return S_OK;
}

static HRESULT WINAPI file_bytes_WriteAt(ILockBytes *iface, ULARGE_INTEGER offset, const void *buffer, ULONG count, ULONG *written)
{
	file_bytes *object = file_bytes_of(iface);
	ULONG done = 0;

	if (written)
		*written = 0;
	if (!move_to(object->file, offset) || !WriteFile(object->file, buffer, count, &done, NULL))
		return status_of_last_error();
	if (written)
		*written = done;
	return S_OK;
}

static HRESULT WINAPI file_bytes_Flush(ILockBytes *iface)
{
	return FlushFileBuffers(file_bytes_of(iface)->file) ? S_OK : status_of_last_error();
}

static HRESULT WINAPI file_bytes_SetSize(ILockBytes *iface, ULARGE_INTEGER size)
{
	file_bytes *object = file_bytes_of(iface);

	if (!move_to(object->file, size) || !SetEndOfFile(object->file))
		return status_of_last_error();
	return S_OK;
}

static HRESULT WINAPI file_bytes_LockRegion(ILockBytes *iface, ULARGE_INTEGER offset, ULARGE_INTEGER count, DWORD type)
{
	return STG_E_INVALIDFUNCTION;
}

static HRESULT WINAPI file_bytes_UnlockRegion(ILockBytes *iface, ULARGE_INTEGER offset, ULARGE_INTEGER count, DWORD type)
{
	return STG_E_INVALIDFUNCTION;
}

static HRESULT WINAPI file_bytes_Stat(ILockBytes *iface, STATSTG *stat, DWORD flag)
{
	file_bytes *object = file_bytes_of(iface);
	LARGE_INTEGER size;

	memset(stat, 0, sizeof(*stat));
	stat->type = STGTY_LOCKBYTES;
	if (!GetFileSizeEx(object->file, &size))
		return status_of_last_error();
	stat->cbSize.QuadPart = (ULONGLONG)size.QuadPart;
	return S_OK;
}

static const ILockBytesVtbl file_bytes_vtable = {
	file_bytes_QueryInterface, file_bytes_AddRef, file_bytes_Release, file_bytes_ReadAt, file_bytes_WriteAt,
	file_bytes_Flush, file_bytes_SetSize, file_bytes_LockRegion, file_bytes_UnlockRegion, file_bytes_Stat,
};

HRESULT file_bytes_open(const WCHAR *path, DWORD mode, BOOL create, ILockBytes **result)
{
	DWORD access = GENERIC_READ, share, disposition = OPEN_EXISTING;
	file_bytes *object;
	HANDLE file;

	if ((mode & 3) != STGM_READ)
		access |= GENERIC_WRITE;
	switch (mode & 0x70) {
	case STGM_SHARE_EXCLUSIVE:
		share = 0;
		break;
	case STGM_SHARE_DENY_WRITE:
		share = FILE_SHARE_READ;
		break;
	case STGM_SHARE_DENY_READ:
		share = FILE_SHARE_WRITE;
		break;
	default:
		share = FILE_SHARE_READ | FILE_SHARE_WRITE;
		break;
	}
	if (create)
		disposition = mode & STGM_CREATE ? CREATE_ALWAYS : CREATE_NEW;
	file = CreateFileW(path, access, share, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return status_of_last_error();
	object = mem_alloc(sizeof(*object));
	if (!object) {
		CloseHandle(file);
		return E_OUTOFMEMORY;
	}
	object->ILockBytes_iface.lpVtbl = &file_bytes_vtable;
	object->refs = 1;
	object->file = file;
	*result = &object->ILockBytes_iface;
	return S_OK;
}

/* Global memory. The bytes live in one HGLOBAL that clones share; every touch of the block's fields — the handle the
 * block stands for, the logical size, the memory itself — runs under the block's host-side spin lock, the way ole32's
 * lock bytes hold the block while a stream op runs. A write that must grow calls GlobalReAlloc inside the section:
 * while a caller holds a GlobalLock of its own on the handle, the realloc fails (a locked object does not move), and
 * the failure comes back as the medium-full error a Windows stream answers in that spot. This cannot use the guest
 * critical-section API: acquiring it from a storage callback would re-enter native callback accounting. */

struct shared_block {
	LONG refs;
	volatile LONG lock;
	HGLOBAL memory;
	BOOL free_on_release;
	ULONGLONG size;
};

static void block_lock(struct shared_block *block)
{
	while (__atomic_exchange_n(&block->lock, 1, __ATOMIC_ACQUIRE))
		while (__atomic_load_n(&block->lock, __ATOMIC_RELAXED))
			;
}

static void block_unlock(struct shared_block *block)
{
	__atomic_store_n(&block->lock, 0, __ATOMIC_RELEASE);
}

static HRESULT block_create(HGLOBAL memory, BOOL free_on_release, struct shared_block **result)
{
	struct shared_block *block = mem_alloc(sizeof(*block));

	if (!block)
		return E_OUTOFMEMORY;
	block->lock = 0;
	if (memory) {
		block->size = GlobalSize(memory);
	} else {
		memory = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD | GMEM_ZEROINIT, 0);
		if (!memory) {
			mem_free(block);
			return E_OUTOFMEMORY;
		}
	}
	block->refs = 1;
	block->memory = memory;
	block->free_on_release = free_on_release;
	*result = block;
	return S_OK;
}

static void block_release(struct shared_block *block)
{
	if (InterlockedDecrement(&block->refs))
		return;
	if (block->free_on_release)
		GlobalFree(block->memory);
	mem_free(block);
}

/* The handle the block stands for now; a resize of a fixed caller block replaces it, so the read goes through the section. */
static HGLOBAL block_memory(struct shared_block *block)
{
	HGLOBAL memory;
	block_lock(block);
	memory = block->memory;
	block_unlock(block);
	return memory;
}

static ULONGLONG block_logical_size(struct shared_block *block)
{
	ULONGLONG size;
	block_lock(block);
	size = block->size;
	block_unlock(block);
	return size;
}

/* The resize proper, called with the section held; block_resize is the callers' wrapper around it. */
static HRESULT block_resize_locked(struct shared_block *block, ULONGLONG size)
{
	HGLOBAL grown;

	if (size > (SIZE_T)-1)
		return STG_E_MEDIUMFULL;
	if (size > GlobalSize(block->memory) || !size) {
		grown = GlobalReAlloc(block->memory, (SIZE_T)size, GMEM_MOVEABLE | GMEM_ZEROINIT);
		if (!grown)
			/* A locked object does not move: the growth fails, as the same ask on Windows does. */
			return STG_E_MEDIUMFULL;
		block->memory = grown;
	}
	if (size > block->size) {
		BYTE *data = GlobalLock(block->memory);
		if (data) {
			memset(data + block->size, 0, (SIZE_T)(size - block->size));
			GlobalUnlock(block->memory);
		}
	}
	block->size = size;
	return S_OK;
}

static HRESULT block_resize(struct shared_block *block, ULONGLONG size)
{
	HRESULT status;

	block_lock(block);
	status = block_resize_locked(block, size);
	block_unlock(block);
	return status;
}

static ULONG block_read(struct shared_block *block, ULONGLONG offset, void *buffer, ULONG count)
{
	BYTE *data;

	block_lock(block);
	if (offset >= block->size)
		count = 0;
	else if (count > block->size - offset)
		count = (ULONG)(block->size - offset);
	if (count) {
		data = GlobalLock(block->memory);
		if (!data)
			count = 0;
		else {
			memcpy(buffer, data + offset, count);
			GlobalUnlock(block->memory);
		}
	}
	block_unlock(block);
	return count;
}

static HRESULT block_write(struct shared_block *block, ULONGLONG offset, const void *buffer, ULONG count, ULONG *written)
{
	BYTE *data;

	*written = 0;
	if (!count)
		return S_OK;
	block_lock(block);
	if (offset + count > block->size) {
		/* The growth stays inside the section: the resize's realloc and this memcpy cannot interleave with another
		   writer's, and a locked block answers the medium-full error before anything is written. */
		HRESULT grown = block_resize_locked(block, offset + count);
		if (FAILED(grown)) {
			block_unlock(block);
			return grown;
		}
	}
	data = GlobalLock(block->memory);
	if (!data) {
		block_unlock(block);
		return STG_E_MEDIUMFULL;
	}
	memcpy(data + offset, buffer, count);
	GlobalUnlock(block->memory);
	*written = count;
	block_unlock(block);
	return S_OK;
}

typedef struct memory_bytes {
	ILockBytes ILockBytes_iface;
	LONG refs;
	struct shared_block *block;
} memory_bytes;

static memory_bytes *memory_bytes_of(ILockBytes *iface)
{
	return CONTAINING_RECORD(iface, memory_bytes, ILockBytes_iface);
}

static ULONG WINAPI memory_bytes_AddRef(ILockBytes *iface)
{
	return InterlockedIncrement(&memory_bytes_of(iface)->refs);
}

static ULONG WINAPI memory_bytes_Release(ILockBytes *iface)
{
	memory_bytes *object = memory_bytes_of(iface);
	ULONG refs = InterlockedDecrement(&object->refs);

	if (!refs) {
		block_release(object->block);
		mem_free(object);
	}
	return refs;
}

static HRESULT WINAPI memory_bytes_ReadAt(ILockBytes *iface, ULARGE_INTEGER offset, void *buffer, ULONG count, ULONG *read)
{
	ULONG done = block_read(memory_bytes_of(iface)->block, offset.QuadPart, buffer, count);

	if (read)
		*read = done;
	return S_OK;
}

static HRESULT WINAPI memory_bytes_WriteAt(ILockBytes *iface, ULARGE_INTEGER offset, const void *buffer, ULONG count, ULONG *written)
{
	ULONG done = 0;
	HRESULT status = block_write(memory_bytes_of(iface)->block, offset.QuadPart, buffer, count, &done);

	if (written)
		*written = done;
	return status;
}

static HRESULT WINAPI memory_bytes_Flush(ILockBytes *iface)
{
	return S_OK;
}

static HRESULT WINAPI memory_bytes_SetSize(ILockBytes *iface, ULARGE_INTEGER size)
{
	return block_resize(memory_bytes_of(iface)->block, size.QuadPart);
}

static HRESULT WINAPI memory_bytes_Stat(ILockBytes *iface, STATSTG *stat, DWORD flag)
{
	memset(stat, 0, sizeof(*stat));
	stat->type = STGTY_LOCKBYTES;
	stat->cbSize.QuadPart = block_logical_size(memory_bytes_of(iface)->block);
	return S_OK;
}

static const ILockBytesVtbl memory_bytes_vtable = {
	file_bytes_QueryInterface, memory_bytes_AddRef, memory_bytes_Release, memory_bytes_ReadAt, memory_bytes_WriteAt,
	memory_bytes_Flush, memory_bytes_SetSize, file_bytes_LockRegion, file_bytes_UnlockRegion, memory_bytes_Stat,
};

HRESULT WINAPI CreateILockBytesOnHGlobal(HGLOBAL memory, BOOL free_on_release, ILockBytes **out)
{
	memory_bytes *object;
	HRESULT status;

	if (!out)
		return E_POINTER;
	*out = NULL;
	object = mem_alloc(sizeof(*object));
	if (!object)
		return E_OUTOFMEMORY;
	status = block_create(memory, free_on_release, &object->block);
	if (FAILED(status)) {
		mem_free(object);
		return status;
	}
	object->ILockBytes_iface.lpVtbl = &memory_bytes_vtable;
	object->refs = 1;
	*out = &object->ILockBytes_iface;
	return S_OK;
}

HRESULT WINAPI GetHGlobalFromILockBytes(ILockBytes *bytes, HGLOBAL *memory)
{
	if (!memory)
		return E_INVALIDARG;
	*memory = NULL;
	if (!bytes || bytes->lpVtbl != &memory_bytes_vtable)
		return E_INVALIDARG;
	*memory = block_memory(memory_bytes_of(bytes)->block);
	return S_OK;
}

typedef struct memory_stream {
	IStream IStream_iface;
	LONG refs;
	volatile LONG position_lock;
	struct shared_block *block;
	ULONGLONG position;
} memory_stream;

static void stream_position_lock(memory_stream *object)
{
	while (__atomic_exchange_n(&object->position_lock, 1, __ATOMIC_ACQUIRE))
		while (__atomic_load_n(&object->position_lock, __ATOMIC_RELAXED))
			;
}

static void stream_position_unlock(memory_stream *object)
{
	__atomic_store_n(&object->position_lock, 0, __ATOMIC_RELEASE);
}

static const IStreamVtbl memory_stream_vtable;

static memory_stream *memory_stream_of(IStream *iface)
{
	return CONTAINING_RECORD(iface, memory_stream, IStream_iface);
}

static HRESULT WINAPI memory_stream_QueryInterface(IStream *iface, REFIID riid, void **out)
{
	if (!out)
		return E_POINTER;
	if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ISequentialStream) || IsEqualIID(riid, &IID_IStream)) {
		*out = iface;
		IStream_AddRef(iface);
		return S_OK;
	}
	*out = NULL;
	return E_NOINTERFACE;
}

static ULONG WINAPI memory_stream_AddRef(IStream *iface)
{
	return InterlockedIncrement(&memory_stream_of(iface)->refs);
}

static ULONG WINAPI memory_stream_Release(IStream *iface)
{
	memory_stream *object = memory_stream_of(iface);
	ULONG refs = InterlockedDecrement(&object->refs);

	if (!refs) {
		block_release(object->block);
		mem_free(object);
	}
	return refs;
}

static HRESULT WINAPI memory_stream_Read(IStream *iface, void *buffer, ULONG count, ULONG *read)
{
	memory_stream *object = memory_stream_of(iface);
	ULONG done;

	stream_position_lock(object);
	done = block_read(object->block, object->position, buffer, count);

	object->position += done;
	stream_position_unlock(object);
	if (read)
		*read = done;
	return S_OK;
}

static HRESULT WINAPI memory_stream_Write(IStream *iface, const void *buffer, ULONG count, ULONG *written)
{
	memory_stream *object = memory_stream_of(iface);
	ULONG done = 0;
	HRESULT status;

	if (written)
		*written = 0;
	if (!buffer)
		return STG_E_INVALIDPOINTER;
	stream_position_lock(object);
	status = block_write(object->block, object->position, buffer, count, &done);
	object->position += done;
	stream_position_unlock(object);
	if (written)
		*written = done;
	return status;
}

static HRESULT WINAPI memory_stream_Seek(IStream *iface, LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER *position)
{
	memory_stream *object = memory_stream_of(iface);
	LONGLONG base;
	HRESULT status = S_OK;

	stream_position_lock(object);
	switch (origin) {
	case STREAM_SEEK_SET:
		base = 0;
		break;
	case STREAM_SEEK_CUR:
		base = (LONGLONG)object->position;
		break;
	case STREAM_SEEK_END:
		base = (LONGLONG)block_logical_size(object->block);
		break;
	default:
		status = STG_E_SEEKERROR;
		base = (LONGLONG)object->position;
		move.QuadPart = 0;
		break;
	}
	if (SUCCEEDED(status) && base + move.QuadPart < 0) {
		status = STG_E_SEEKERROR;
		base = (LONGLONG)object->position;
		move.QuadPart = 0;
	}
	object->position = (ULONGLONG)(base + move.QuadPart);
	if (position)
		position->QuadPart = object->position;
	stream_position_unlock(object);
	return status;
}

static HRESULT WINAPI memory_stream_SetSize(IStream *iface, ULARGE_INTEGER size)
{
	memory_stream *object = memory_stream_of(iface);

	if (size.QuadPart == block_logical_size(object->block))
		return S_OK;
	return block_resize(object->block, size.QuadPart);
}

static HRESULT WINAPI memory_stream_CopyTo(IStream *iface, IStream *target, ULARGE_INTEGER count, ULARGE_INTEGER *read, ULARGE_INTEGER *written)
{
	ULONGLONG left = count.QuadPart, total_read = 0, total_written = 0;
	BYTE *block;
	HRESULT status = S_OK;

	if (!target)
		return STG_E_INVALIDPOINTER;
	block = mem_alloc(65536);
	if (!block)
		return E_OUTOFMEMORY;
	while (left) {
		ULONG got = 0, put = 0;
		status = IStream_Read(iface, block, left > 65536 ? 65536 : (ULONG)left, &got);
		if (FAILED(status) || !got)
			break;
		total_read += got;
		status = IStream_Write(target, block, got, &put);
		total_written += put;
		if (FAILED(status) || put != got)
			break;
		left -= got;
	}
	mem_free(block);
	if (read)
		read->QuadPart = total_read;
	if (written)
		written->QuadPart = total_written;
	return status;
}

static HRESULT WINAPI memory_stream_Commit(IStream *iface, DWORD flags)
{
	return S_OK;
}

static HRESULT WINAPI memory_stream_Revert(IStream *iface)
{
	return S_OK;
}

static HRESULT WINAPI memory_stream_LockRegion(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER count, DWORD type)
{
	return STG_E_INVALIDFUNCTION;
}

static HRESULT WINAPI memory_stream_Stat(IStream *iface, STATSTG *stat, DWORD flag)
{
	if (!stat)
		return STG_E_INVALIDPOINTER;
	memset(stat, 0, sizeof(*stat));
	stat->type = STGTY_STREAM;
	stat->cbSize.QuadPart = block_logical_size(memory_stream_of(iface)->block);
	return S_OK;
}

static HRESULT memory_stream_new(struct shared_block *block, IStream **out)
{
	memory_stream *object = mem_alloc(sizeof(*object));

	if (!object)
		return E_OUTOFMEMORY;
	object->IStream_iface.lpVtbl = &memory_stream_vtable;
	object->refs = 1;
	object->position_lock = 0;
	object->block = block;
	*out = &object->IStream_iface;
	return S_OK;
}

static HRESULT WINAPI memory_stream_Clone(IStream *iface, IStream **out)
{
	memory_stream *object = memory_stream_of(iface);
	HRESULT status;

	if (!out)
		return E_INVALIDARG;
	/* The reference the clone will hold is taken before anything is handed out: a concurrent release of the last
	 * other reference must not free the block while a clone of it is being born, which would leave the new stream
	 * over freed memory and the handle's registration gone at the next realloc. */
	InterlockedIncrement(&object->block->refs);
	status = memory_stream_new(object->block, out);
	if (FAILED(status)) {
		block_release(object->block);
		return status;
	}
	stream_position_lock(object);
	memory_stream_of(*out)->position = object->position;
	stream_position_unlock(object);
	return S_OK;
}

static const IStreamVtbl memory_stream_vtable = {
	memory_stream_QueryInterface, memory_stream_AddRef, memory_stream_Release, memory_stream_Read, memory_stream_Write,
	memory_stream_Seek, memory_stream_SetSize, memory_stream_CopyTo, memory_stream_Commit, memory_stream_Revert,
	memory_stream_LockRegion, memory_stream_LockRegion, memory_stream_Stat, memory_stream_Clone,
};

HRESULT WINAPI CreateStreamOnHGlobal(HGLOBAL memory, BOOL free_on_release, IStream **out)
{
	struct shared_block *block;
	HRESULT status;

	if (!out)
		return E_INVALIDARG;
	*out = NULL;
	status = block_create(memory, free_on_release, &block);
	if (FAILED(status))
		return status;
	status = memory_stream_new(block, out);
	if (FAILED(status))
		block_release(block);
	return status;
}

HRESULT WINAPI GetHGlobalFromStream(IStream *stream, HGLOBAL *memory)
{
	if (!memory)
		return E_INVALIDARG;
	*memory = NULL;
	if (!stream || stream->lpVtbl != &memory_stream_vtable)
		return E_INVALIDARG;
	*memory = block_memory(memory_stream_of(stream)->block);
	return S_OK;
}
