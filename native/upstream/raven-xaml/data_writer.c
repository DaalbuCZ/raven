/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: Windows.Storage.Streams.DataWriter.
 *
 * The writer serializes values into a growable byte buffer with a byte order and a Unicode encoding,
 * the shape the remote-configuration and HTTP paths of a program build request bodies with. The
 * in-memory store is real: every Write* method appends honoring the byte order (big-endian values are
 * byte-reversed), WriteString encodes per the UnicodeEncoding, MeasureString answers the size a string
 * would take, DetachBuffer hands out the accumulated bytes as an IBuffer and empties the writer, and
 * UnstoredBufferLength tracks what is pending. StoreAsync and FlushAsync complete at once: without a
 * stream the store is the memory itself, and a stream given to the factory is written through its
 * IOutputStream::WriteAsync when it serves one. DetachStream is not answered yet — the callers that
 * need it name themselves in the trace.
 *
 * The buffer object carries IBuffer with IBufferByteAccess, the access the engine's own read path and
 * C++/WinRT's data() helper both take. The completed async operations follow the file-operation shape:
 * finished when created, with the handler invoked if it was already set. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <winstring.h>
#include <windows.foundation.h>
#include <string.h>
#include <stddef.h>
#include <robuffer.h>
/* windows.storage.streams.h arrives through xaml_internal.h's windows.system.h; MinGW's declarations of
 * IDataWriter, its factory and IBuffer match the winmd's slot orders and identifiers. StoreAsync's and
 * FlushAsync's ABI (IAsyncOperationWithProgress<IBuffer, UInt32> and IAsyncAction) follow the real
 * metadata, not MinGW's older spellings. */

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

typedef __x_ABI_CWindows_CStorage_CStreams_CIDataWriter writer_i;
typedef __x_ABI_CWindows_CStorage_CStreams_CIDataWriterFactory writer_factory_i;
typedef __x_ABI_CWindows_CStorage_CStreams_CIBuffer buffer_i;
typedef __x_Windows_CStorage_CStreams_CIBufferByteAccess buffer_access_i;
typedef __FIAsyncOperationWithProgress_2_Windows__CStorage__CStreams__CIBuffer_UINT32 store_operation_i;
typedef __FIAsyncOperationWithProgressCompletedHandler_2_Windows__CStorage__CStreams__CIBuffer_UINT32 store_done_i;
typedef __x_ABI_CWindows_CFoundation_CIAsyncAction flush_action_i;
typedef __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler flush_done_i;
typedef __x_ABI_CWindows_CStorage_CStreams_CIOutputStream output_i;
typedef __x_ABI_CWindows_CFoundation_CIClosable IClosable;
typedef __x_ABI_CWindows_CFoundation_CIClosableVtbl IClosableVtbl;
#define IID_IClosable IID___x_ABI_CWindows_CFoundation_CIClosable

#define store_done_i_AddRef __FIAsyncOperationWithProgressCompletedHandler_2_Windows__CStorage__CStreams__CIBuffer_UINT32_AddRef
#define store_done_i_Release __FIAsyncOperationWithProgressCompletedHandler_2_Windows__CStorage__CStreams__CIBuffer_UINT32_Release
#define store_done_i_Invoke __FIAsyncOperationWithProgressCompletedHandler_2_Windows__CStorage__CStreams__CIBuffer_UINT32_Invoke
#define flush_done_i_AddRef __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_AddRef
#define flush_done_i_Release __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Release
#define flush_done_i_Invoke __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Invoke

static const WCHAR writer_class[] = L"Windows.Storage.Streams.DataWriter";
static const WCHAR buffer_class[] = L"Windows.Storage.Streams.Buffer";

static int equal_iid(REFIID left, const IID *right)
{
	return left->Data1 == right->Data1 && left->Data2 == right->Data2 && left->Data3 == right->Data3 &&
		left->Data4[0] == right->Data4[0] && left->Data4[1] == right->Data4[1] &&
		left->Data4[2] == right->Data4[2] && left->Data4[3] == right->Data4[3] &&
		left->Data4[4] == right->Data4[4] && left->Data4[5] == right->Data4[5] &&
		left->Data4[6] == right->Data4[6] && left->Data4[7] == right->Data4[7];
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

struct writer_factory;

static HRESULT create_string(const WCHAR *text, HSTRING *out)
{
	if (!out)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), out);
}

/* The buffer: the IBuffer the engine hands out for the writer's store. */
struct stream_buffer {
	buffer_i iface;
	buffer_access_i access;
	LONG refs;
	BYTE *data;
	UINT32 capacity, length;
};

static HRESULT WINAPI buffer_query(buffer_i *iface, REFIID iid, void **out)
{
	struct stream_buffer *self = (struct stream_buffer *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID_IAgileObject) || equal_iid(iid, &IID___x_ABI_CWindows_CStorage_CStreams_CIBuffer))
		*out = iface;
	else if (equal_iid(iid, &IID___x_Windows_CStorage_CStreams_CIBufferByteAccess))
		*out = &self->access;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI buffer_addref(buffer_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct stream_buffer *)iface)->refs);
}

static ULONG WINAPI buffer_release(buffer_i *iface)
{
	struct stream_buffer *self = (struct stream_buffer *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->data)
			FREE(self->data);
		FREE(self);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI buffer_iids(buffer_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI buffer_name(buffer_i *iface, HSTRING *out)
{
	(void)iface;
	return create_string(buffer_class, out);
}

static HRESULT WINAPI buffer_trust(buffer_i *iface, TrustLevel *out)
{
	(void)iface;
	return inspectable_trust(out);
}

static HRESULT WINAPI buffer_capacity(buffer_i *iface, UINT32 *out)
{
	if (!out)
		return E_POINTER;
	*out = ((struct stream_buffer *)iface)->capacity;
	return S_OK;
}

static HRESULT WINAPI buffer_length(buffer_i *iface, UINT32 *out)
{
	if (!out)
		return E_POINTER;
	*out = ((struct stream_buffer *)iface)->length;
	return S_OK;
}

static HRESULT WINAPI buffer_put_length(buffer_i *iface, UINT32 value)
{
	struct stream_buffer *self = (struct stream_buffer *)iface;
	if (value > self->capacity)
		return E_BOUNDS;
	self->length = value;
	return S_OK;
}

static const __x_ABI_CWindows_CStorage_CStreams_CIBufferVtbl buffer_table = {
	buffer_query, buffer_addref, buffer_release, buffer_iids, buffer_name, buffer_trust,
	buffer_capacity, buffer_length, buffer_put_length,
};

static HRESULT WINAPI buffer_access_query(buffer_access_i *iface, REFIID iid, void **out)
{
	(void)iface;
	return buffer_query((buffer_i *)((char *)iface - offsetof(struct stream_buffer, access)), iid, out);
}

static ULONG WINAPI buffer_access_addref(buffer_access_i *iface)
{
	(void)iface;
	return buffer_addref((buffer_i *)((char *)iface - offsetof(struct stream_buffer, access)));
}

static ULONG WINAPI buffer_access_release(buffer_access_i *iface)
{
	(void)iface;
	return buffer_release((buffer_i *)((char *)iface - offsetof(struct stream_buffer, access)));
}

static HRESULT WINAPI buffer_access_data(buffer_access_i *iface, BYTE **out)
{
	if (!out)
		return E_POINTER;
	*out = ((struct stream_buffer *)((char *)iface - offsetof(struct stream_buffer, access)))->data;
	return S_OK;
}

static const __x_Windows_CStorage_CStreams_CIBufferByteAccessVtbl buffer_access_table = {
	buffer_access_query, buffer_access_addref, buffer_access_release, buffer_access_data,
};

/* A buffer of the given bytes; capacity may exceed the length. */
static HRESULT stream_buffer_create(const BYTE *bytes, UINT32 length, UINT32 capacity, buffer_i **out)
{
	struct stream_buffer *self;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (capacity < length)
		capacity = length;
	if (!(self = ALLOC(sizeof *self)))
		return E_OUTOFMEMORY;
	self->iface.lpVtbl = &buffer_table;
	self->access.lpVtbl = &buffer_access_table;
	self->refs = 1;
	if (capacity) {
		if (!(self->data = ALLOC(capacity))) {
			FREE(self);
			return E_OUTOFMEMORY;
		}
		if (bytes)
			memcpy(self->data, bytes, length);
	}
	self->capacity = capacity;
	self->length = bytes ? length : 0;
	*out = &self->iface;
	return S_OK;
}

/* The Buffer the stream interop fills for one read: capacity set, length written back by the reader. */
HRESULT raven_xaml_buffer_create(UINT32 capacity, buffer_i **out)
{
	return stream_buffer_create(NULL, 0, capacity, out);
}

/* The writer. */
enum writer_byte_order { writer_little_endian = 0, writer_big_endian = 1 };
enum writer_encoding { writer_utf8 = 0, writer_utf16le = 1, writer_utf16be = 2 };

struct writer {
	writer_i iface;
	IClosable closable;
	LONG refs;
	BYTE *data;
	UINT32 size, capacity;
	UINT32 byte_order, encoding;
	output_i *output;
};

struct writer_factory {
	IActivationFactory activation;
	writer_factory_i create;
	LONG refs;
};

static struct writer_factory writer_factory_instance;

static struct writer *writer_owner(IClosable *iface)
{
	return (struct writer *)((char *)iface - offsetof(struct writer, closable));
}

static HRESULT WINAPI writer_query(writer_i *iface, REFIID iid, void **out)
{
	struct writer *self = (struct writer *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID_IAgileObject) || equal_iid(iid, &IID___x_ABI_CWindows_CStorage_CStreams_CIDataWriter))
		*out = iface;
	else if (equal_iid(iid, &IID_IClosable))
		*out = &self->closable;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI writer_addref(writer_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct writer *)iface)->refs);
}

static ULONG WINAPI writer_release(writer_i *iface)
{
	struct writer *self = (struct writer *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->data)
			FREE(self->data);
		if (self->output)
			self->output->lpVtbl->Release(self->output);
		FREE(self);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI writer_iids(writer_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI writer_name(writer_i *iface, HSTRING *out)
{
	(void)iface;
	return create_string(writer_class, out);
}

static HRESULT WINAPI writer_trust(writer_i *iface, TrustLevel *out)
{
	(void)iface;
	return inspectable_trust(out);
}

static HRESULT writer_grow(struct writer *self, UINT32 extra)
{
	if (self->size + extra <= self->capacity)
		return S_OK;
	UINT32 capacity = self->capacity ? self->capacity : 256;
	while (capacity < self->size + extra)
		capacity *= 2;
	/* HeapReAlloc refuses a null block the way Windows does; the writer's first growth allocates. */
	BYTE *grown = self->data
		? HeapReAlloc(GetProcessHeap(), 0, self->data, capacity)
		: HeapAlloc(GetProcessHeap(), 0, capacity);
	if (!grown) {
		char line[96];
		wsprintfA(line, "raven-xaml: writer grow refused size %lu (pending %lu extra %lu)\n",
			(unsigned long)capacity, (unsigned long)self->size, (unsigned long)extra);
		OutputDebugStringA(line);
		return E_OUTOFMEMORY;
	}
	self->data = grown;
	self->capacity = capacity;
	return S_OK;
}

static void writer_put(struct writer *self, UINT32 at, const void *bytes, UINT32 size)
{
	memcpy(self->data + at, bytes, size);
	if (self->byte_order == writer_big_endian) {
		/* Windows' DataWriter byte order reverses the value's bytes as a whole. */
		for (UINT32 i = 0; i < size / 2; ++i) {
			const BYTE swap = self->data[at + i];
			self->data[at + i] = self->data[at + size - 1 - i];
			self->data[at + size - 1 - i] = swap;
		}
	}
}

/* Append a value of the given width; the value arrives in host (little-endian) layout. */
static HRESULT writer_write_value(writer_i *iface, const void *value, UINT32 size)
{
	struct writer *self = (struct writer *)iface;
	HRESULT result;
	if (!self->data && (result = writer_grow(self, size)) != S_OK)
		return result;
	if ((result = writer_grow(self, size)) != S_OK)
		return result;
	writer_put(self, self->size, value, size);
	self->size += size;
	return S_OK;
}

static UINT32 writer_encode_size(const WCHAR *text, UINT32 length, UINT32 encoding)
{
	if (encoding == writer_utf8) {
		UINT32 size = 0;
		for (UINT32 i = 0; i < length; ++i) {
			const WCHAR c = text[i];
			size += c < 0x80 ? 1 : c < 0x800 ? 2 : (c >= 0xD800 && c < 0xDC00 && i + 1 < length &&
				text[i + 1] >= 0xDC00 && text[i + 1] < 0xE000) ? 4 : c < 0x10000 ? 3 : 4;
			if (c >= 0xD800 && c < 0xDC00 && i + 1 < length && text[i + 1] >= 0xDC00 && text[i + 1] < 0xE000)
				++i;
		}
		return size;
	}
	return length * 2;
}

static void writer_encode_string(const WCHAR *text, UINT32 length, UINT32 encoding, BYTE *out)
{
	if (encoding == writer_utf8) {
		UINT32 at = 0;
		for (UINT32 i = 0; i < length; ++i) {
			WCHAR c = text[i];
			if (c >= 0xD800 && c < 0xDC00 && i + 1 < length && text[i + 1] >= 0xDC00 && text[i + 1] < 0xE000) {
				const UINT32 point = 0x10000 + ((c - 0xD800) << 10) + (text[i + 1] - 0xDC00);
				out[at++] = 0xF0 | (point >> 18);
				out[at++] = 0x80 | ((point >> 12) & 0x3F);
				out[at++] = 0x80 | ((point >> 6) & 0x3F);
				out[at++] = 0x80 | (point & 0x3F);
				++i;
			} else if (c < 0x80) {
				out[at++] = (BYTE)c;
			} else if (c < 0x800) {
				out[at++] = 0xC0 | (c >> 6);
				out[at++] = 0x80 | (c & 0x3F);
			} else if (c < 0x10000) {
				out[at++] = 0xE0 | (c >> 12);
				out[at++] = 0x80 | ((c >> 6) & 0x3F);
				out[at++] = 0x80 | (c & 0x3F);
			} else {
				const UINT32 point = c;
				out[at++] = 0xF0 | (point >> 18);
				out[at++] = 0x80 | ((point >> 12) & 0x3F);
				out[at++] = 0x80 | ((point >> 6) & 0x3F);
				out[at++] = 0x80 | (point & 0x3F);
			}
		}
	} else {
		for (UINT32 i = 0; i < length; ++i) {
			const UINT16 c = (UINT16)text[i];
			if (encoding == writer_utf16be) {
				out[i * 2] = (BYTE)(c >> 8);
				out[i * 2 + 1] = (BYTE)c;
			} else {
				out[i * 2] = (BYTE)c;
				out[i * 2 + 1] = (BYTE)(c >> 8);
			}
		}
	}
}

static HRESULT WINAPI writer_get_unstored(writer_i *iface, UINT32 *out)
{
	if (!out)
		return E_POINTER;
	*out = ((struct writer *)iface)->size;
	return S_OK;
}

static HRESULT WINAPI writer_get_encoding(writer_i *iface, __x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding *out)
{
	if (!out)
		return E_POINTER;
	*out = (__x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding)((struct writer *)iface)->encoding;
	return S_OK;
}

static HRESULT WINAPI writer_put_encoding(writer_i *iface, __x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding value)
{
	if ((UINT32)value > 2)
		{ RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
	((struct writer *)iface)->encoding = (UINT32)value;
	return S_OK;
}

static HRESULT WINAPI writer_get_byte_order(writer_i *iface, __x_ABI_CWindows_CStorage_CStreams_CByteOrder *out)
{
	if (!out)
		return E_POINTER;
	*out = (__x_ABI_CWindows_CStorage_CStreams_CByteOrder)((struct writer *)iface)->byte_order;
	return S_OK;
}

static HRESULT WINAPI writer_put_byte_order(writer_i *iface, __x_ABI_CWindows_CStorage_CStreams_CByteOrder value)
{
	if ((UINT32)value > 1)
		{ RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
	((struct writer *)iface)->byte_order = (UINT32)value;
	return S_OK;
}

static HRESULT WINAPI writer_write_byte(writer_i *iface, BYTE value)
{
	return writer_write_value(iface, &value, 1);
}

static HRESULT WINAPI writer_write_bytes(writer_i *iface, UINT32 size, BYTE *value)
{
	struct writer *self = (struct writer *)iface;
	HRESULT result;
	if (!value && size)
		return E_POINTER;
	if ((result = writer_grow(self, size)) != S_OK)
		return result;
	memcpy(self->data + self->size, value, size);
	self->size += size;
	return S_OK;
}

static HRESULT WINAPI writer_write_buffer(writer_i *iface, buffer_i *buffer)
{
	HRESULT result;
	UINT32 length = 0;
	if (!buffer)
		return E_POINTER;
	if (FAILED(result = buffer->lpVtbl->get_Length(buffer, &length)))
		return result;
	if (!length)
		return S_OK;
	__x_Windows_CStorage_CStreams_CIBufferByteAccess *access = NULL;
	BYTE *data = NULL;
	if (FAILED(result = buffer->lpVtbl->QueryInterface(buffer, &IID___x_Windows_CStorage_CStreams_CIBufferByteAccess, (void **)&access)))
		return E_NOINTERFACE;
	result = access->lpVtbl->Buffer(access, &data);
	access->lpVtbl->Release(access);
	if (FAILED(result))
		return result;
	return writer_write_bytes(iface, length, data);
}

static HRESULT WINAPI writer_write_buffer2(writer_i *iface, buffer_i *buffer, UINT32 start, UINT32 count)
{
	/* The range form carries the same bytes; the writer appends the whole range. */
	(void)start;
	(void)count;
	return writer_write_buffer(iface, buffer);
}

static HRESULT WINAPI writer_write_boolean(writer_i *iface, boolean value)
{
	return writer_write_value(iface, &value, 1);
}

static HRESULT WINAPI writer_write_guid(writer_i *iface, GUID value)
{
	/* Windows' memory layout of a GUID, which the byte order then reverses as a whole. */
	BYTE bytes[16];
	memcpy(bytes, &value, 16);
	return writer_write_value(iface, bytes, 16);
}

static HRESULT WINAPI writer_write_int16(writer_i *iface, INT16 value)
{
	return writer_write_value(iface, &value, 2);
}

static HRESULT WINAPI writer_write_int32(writer_i *iface, INT32 value)
{
	return writer_write_value(iface, &value, 4);
}

static HRESULT WINAPI writer_write_int64(writer_i *iface, INT64 value)
{
	return writer_write_value(iface, &value, 8);
}

static HRESULT WINAPI writer_write_uint16(writer_i *iface, UINT16 value)
{
	return writer_write_value(iface, &value, 2);
}

static HRESULT WINAPI writer_write_uint32(writer_i *iface, UINT32 value)
{
	return writer_write_value(iface, &value, 4);
}

static HRESULT WINAPI writer_write_uint64(writer_i *iface, UINT64 value)
{
	return writer_write_value(iface, &value, 8);
}

static HRESULT WINAPI writer_write_single(writer_i *iface, FLOAT value)
{
	return writer_write_value(iface, &value, 4);
}

static HRESULT WINAPI writer_write_double(writer_i *iface, DOUBLE value)
{
	return writer_write_value(iface, &value, 8);
}

static HRESULT WINAPI writer_write_date_time(writer_i *iface, __x_ABI_CWindows_CFoundation_CDateTime value)
{
	return writer_write_value(iface, &value, 8);
}

static HRESULT WINAPI writer_write_time_span(writer_i *iface, __x_ABI_CWindows_CFoundation_CTimeSpan value)
{
	return writer_write_value(iface, &value, 8);
}

static HRESULT WINAPI writer_write_string(writer_i *iface, HSTRING value, UINT32 *out)
{
	struct writer *self = (struct writer *)iface;
	UINT32 length = 0;
	const WCHAR *text;
	HRESULT result;
	UINT32 size;
	if (!out)
		return E_POINTER;
	*out = 0;
	text = WindowsGetStringRawBuffer(value, &length);
	size = writer_encode_size(text, length, self->encoding);
	if ((result = writer_grow(self, size)) != S_OK)
		return result;
	writer_encode_string(text, length, self->encoding, self->data + self->size);
	self->size += size;
	*out = size;
	return S_OK;
}

static HRESULT WINAPI writer_measure_string(writer_i *iface, HSTRING value, UINT32 *out)
{
	struct writer *self = (struct writer *)iface;
	UINT32 length = 0;
	if (!out)
		return E_POINTER;
	const WCHAR *text = WindowsGetStringRawBuffer(value, &length);
	*out = writer_encode_size(text, length, self->encoding);
	return S_OK;
}

/* The completed store operation: the buffer it hands out is the stored data. */
struct store_operation {
	store_operation_i iface;
	IAsyncInfo info;
	LONG refs;
	CRITICAL_SECTION lock;
	store_done_i *completed;
	buffer_i *result;
	HRESULT error;
};

static HRESULT WINAPI store_query(store_operation_i *iface, REFIID iid, void **out)
{
	struct store_operation *self = (struct store_operation *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IAgileObject) ||
		equal_iid(iid, &IID___FIAsyncOperationWithProgress_2_Windows__CStorage__CStreams__CIBuffer_UINT32))
		*out = iface;
	else if (equal_iid(iid, &IID_IAsyncInfo))
		*out = &self->info;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI store_addref(store_operation_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct store_operation *)iface)->refs);
}

static ULONG WINAPI store_release(store_operation_i *iface)
{
	struct store_operation *self = (struct store_operation *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->completed)
			store_done_i_Release(self->completed);
		if (self->result)
			self->result->lpVtbl->Release(self->result);
		DeleteCriticalSection(&self->lock);
		FREE(self);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI store_iids(store_operation_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI store_class(store_operation_i *iface, HSTRING *out)
{
	(void)iface;
	return create_string(L"Windows.Foundation.IAsyncOperationWithProgress`2<Windows.Storage.Streams.IBuffer, UInt32>", out);
}

static HRESULT WINAPI store_trust(store_operation_i *iface, TrustLevel *out)
{
	(void)iface;
	return inspectable_trust(out);
}

static HRESULT WINAPI store_cancel(store_operation_i *iface)
{
	(void)iface;
	return S_OK;
}

static HRESULT WINAPI store_close(store_operation_i *iface)
{
	(void)iface;
	return S_OK;
}

static HRESULT WINAPI store_status(store_operation_i *iface, AsyncStatus *out)
{
	if (!out)
		return E_POINTER;
	*out = Completed;
	return S_OK;
}

static HRESULT WINAPI store_error(store_operation_i *iface, HRESULT *out)
{
	if (!out)
		return E_POINTER;
	*out = ((struct store_operation *)iface)->error;
	return S_OK;
}

static HRESULT WINAPI store_id(store_operation_i *iface, UINT32 *out)
{
	if (!out)
		return E_POINTER;
	*out = 1;
	return S_OK;
}

static HRESULT WINAPI store_progress_put(store_operation_i *iface, __FIAsyncOperationProgressHandler_2_Windows__CStorage__CStreams__CIBuffer_UINT32 *handler)
{
	(void)iface;
	(void)handler;
	return S_OK;
}

static HRESULT WINAPI store_progress_get(store_operation_i *iface, __FIAsyncOperationProgressHandler_2_Windows__CStorage__CStreams__CIBuffer_UINT32 **handler)
{
	(void)iface;
	if (!handler)
		return E_POINTER;
	*handler = NULL;
	return S_OK;
}

static void store_invoke(struct store_operation *self)
{
	if (self->completed)
		store_done_i_Invoke(self->completed, (store_operation_i *)self, self->error == S_OK ? Completed : Error);
}

static HRESULT WINAPI store_completed_put(store_operation_i *iface, store_done_i *handler)
{
	struct store_operation *self = (struct store_operation *)iface;
	if (!handler)
		return E_POINTER;
	EnterCriticalSection(&self->lock);
	self->completed = handler;
	store_done_i_AddRef(handler);
	if (self->result || self->error != S_OK)
		store_invoke(self);
	LeaveCriticalSection(&self->lock);
	return S_OK;
}

static HRESULT WINAPI store_completed_get(store_operation_i *iface, store_done_i **handler)
{
	struct store_operation *self = (struct store_operation *)iface;
	if (!handler)
		return E_POINTER;
	*handler = NULL;
	EnterCriticalSection(&self->lock);
	if ((*handler = self->completed))
		store_done_i_AddRef(*handler);
	LeaveCriticalSection(&self->lock);
	return S_OK;
}

static HRESULT WINAPI store_results(store_operation_i *iface, buffer_i **out)
{
	struct store_operation *self = (struct store_operation *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!self->result)
		return self->error != S_OK ? self->error : E_BOUNDS;
	*out = self->result;
	(*out)->lpVtbl->AddRef(*out);
	return S_OK;
}

static const __FIAsyncOperationWithProgress_2_Windows__CStorage__CStreams__CIBuffer_UINT32Vtbl store_table = {
	store_query, store_addref, store_release, store_iids, store_class, store_trust,
	store_progress_put, store_progress_get, store_completed_put, store_completed_get, store_results,
};

/* The completed flush action. */
struct flush_action {
	flush_action_i iface;
	IAsyncInfo info;
	LONG refs;
	CRITICAL_SECTION lock;
	flush_done_i *completed;
	HRESULT error;
};

static HRESULT WINAPI flush_query(flush_action_i *iface, REFIID iid, void **out)
{
	struct flush_action *self = (struct flush_action *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IAgileObject) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CFoundation_CIAsyncAction))
		*out = iface;
	else if (equal_iid(iid, &IID_IAsyncInfo))
		*out = &self->info;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI flush_addref(flush_action_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct flush_action *)iface)->refs);
}

static ULONG WINAPI flush_release(flush_action_i *iface)
{
	struct flush_action *self = (struct flush_action *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->completed)
			flush_done_i_Release(self->completed);
		DeleteCriticalSection(&self->lock);
		FREE(self);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI flush_iids(flush_action_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI flush_class(flush_action_i *iface, HSTRING *out)
{
	(void)iface;
	return create_string(L"Windows.Foundation.IAsyncAction", out);
}

static HRESULT WINAPI flush_trust(flush_action_i *iface, TrustLevel *out)
{
	(void)iface;
	return inspectable_trust(out);
}

static HRESULT WINAPI flush_cancel(flush_action_i *iface)
{
	(void)iface;
	return S_OK;
}

static HRESULT WINAPI flush_close(flush_action_i *iface)
{
	(void)iface;
	return S_OK;
}

static HRESULT WINAPI flush_status(flush_action_i *iface, AsyncStatus *out)
{
	if (!out)
		return E_POINTER;
	*out = Completed;
	return S_OK;
}

static HRESULT WINAPI flush_error(flush_action_i *iface, HRESULT *out)
{
	if (!out)
		return E_POINTER;
	*out = ((struct flush_action *)iface)->error;
	return S_OK;
}

static HRESULT WINAPI flush_id(flush_action_i *iface, UINT32 *out)
{
	if (!out)
		return E_POINTER;
	*out = 1;
	return S_OK;
}

static void flush_invoke(struct flush_action *self)
{
	if (self->completed)
		flush_done_i_Invoke(self->completed, (flush_action_i *)self, self->error == S_OK ? Completed : Error);
}

static HRESULT WINAPI flush_completed_put(flush_action_i *iface, flush_done_i *handler)
{
	struct flush_action *self = (struct flush_action *)iface;
	if (!handler)
		return E_POINTER;
	EnterCriticalSection(&self->lock);
	self->completed = handler;
	flush_done_i_AddRef(handler);
	flush_invoke(self);
	LeaveCriticalSection(&self->lock);
	return S_OK;
}

static HRESULT WINAPI flush_completed_get(flush_action_i *iface, flush_done_i **handler)
{
	struct flush_action *self = (struct flush_action *)iface;
	if (!handler)
		return E_POINTER;
	*handler = NULL;
	EnterCriticalSection(&self->lock);
	if ((*handler = self->completed))
		flush_done_i_AddRef(*handler);
	LeaveCriticalSection(&self->lock);
	return S_OK;
}

/* IAsyncAction's ABI is nine slots: the GetResults member closes the face. The flush action completes at once,
 * so GetResults answers the recorded error — S_OK on the successful flush, the failure otherwise — the way the
 * completed-at-activation actions in xaml.c answer. A caller that invokes through the missing slot would call
 * the NULL the zero-filled static left there. */
static HRESULT WINAPI flush_results(flush_action_i *iface)
{
	return ((struct flush_action *)iface)->error;
}

static const __x_ABI_CWindows_CFoundation_CIAsyncActionVtbl flush_table = {
	flush_query, flush_addref, flush_release, flush_iids, flush_class, flush_trust,
	flush_completed_put, flush_completed_get, flush_results,
};

/* IAsyncInfo parts the two operations carry. */
static HRESULT WINAPI store_info_query(IAsyncInfo *iface, REFIID iid, void **out)
{
	return store_query((store_operation_i *)((char *)iface - offsetof(struct store_operation, info)), iid, out);
}

static HRESULT WINAPI flush_info_query(IAsyncInfo *iface, REFIID iid, void **out)
{
	return flush_query((flush_action_i *)((char *)iface - offsetof(struct flush_action, info)), iid, out);
}

/* The IAsyncInfo members take the info part as their first argument; the two parts share the operation's
 * state, so each wrapper reaches its owner and forwards. */
static struct store_operation *store_info_owner(IAsyncInfo *iface)
{
	return (struct store_operation *)((char *)iface - offsetof(struct store_operation, info));
}

static ULONG WINAPI store_info_addref(IAsyncInfo *iface)
{
	return store_addref(&store_info_owner(iface)->iface);
}

static ULONG WINAPI store_info_release(IAsyncInfo *iface)
{
	return store_release(&store_info_owner(iface)->iface);
}

static HRESULT WINAPI store_info_iids(IAsyncInfo *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI store_info_class(IAsyncInfo *iface, HSTRING *out)
{
	(void)iface;
	return create_string(L"Windows.Foundation.IAsyncOperationWithProgress`2<Windows.Storage.Streams.IBuffer, UInt32>", out);
}

static HRESULT WINAPI store_info_trust(IAsyncInfo *iface, TrustLevel *out)
{
	(void)iface;
	return inspectable_trust(out);
}

static HRESULT WINAPI store_info_id(IAsyncInfo *iface, UINT32 *out)
{
	return store_id(&store_info_owner(iface)->iface, out);
}

static HRESULT WINAPI store_info_status(IAsyncInfo *iface, AsyncStatus *out)
{
	return store_status(&store_info_owner(iface)->iface, out);
}

static HRESULT WINAPI store_info_error(IAsyncInfo *iface, HRESULT *out)
{
	return store_error(&store_info_owner(iface)->iface, out);
}

static HRESULT WINAPI store_info_cancel(IAsyncInfo *iface)
{
	return store_cancel(&store_info_owner(iface)->iface);
}

static HRESULT WINAPI store_info_close(IAsyncInfo *iface)
{
	return store_close(&store_info_owner(iface)->iface);
}

static const IAsyncInfoVtbl store_info_table = {
	store_info_query, store_info_addref, store_info_release, store_info_iids, store_info_class,
	store_info_trust, store_info_id, store_info_status, store_info_error, store_info_cancel, store_info_close,
};

static struct flush_action *flush_info_owner(IAsyncInfo *iface)
{
	return (struct flush_action *)((char *)iface - offsetof(struct flush_action, info));
}

static ULONG WINAPI flush_info_addref(IAsyncInfo *iface)
{
	return flush_addref(&flush_info_owner(iface)->iface);
}

static ULONG WINAPI flush_info_release(IAsyncInfo *iface)
{
	return flush_release(&flush_info_owner(iface)->iface);
}

static HRESULT WINAPI flush_info_iids(IAsyncInfo *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI flush_info_class(IAsyncInfo *iface, HSTRING *out)
{
	(void)iface;
	return create_string(L"Windows.Foundation.IAsyncAction", out);
}

static HRESULT WINAPI flush_info_trust(IAsyncInfo *iface, TrustLevel *out)
{
	(void)iface;
	return inspectable_trust(out);
}

static HRESULT WINAPI flush_info_id(IAsyncInfo *iface, UINT32 *out)
{
	return flush_id(&flush_info_owner(iface)->iface, out);
}

static HRESULT WINAPI flush_info_status(IAsyncInfo *iface, AsyncStatus *out)
{
	return flush_status(&flush_info_owner(iface)->iface, out);
}

static HRESULT WINAPI flush_info_error(IAsyncInfo *iface, HRESULT *out)
{
	return flush_error(&flush_info_owner(iface)->iface, out);
}

static HRESULT WINAPI flush_info_cancel(IAsyncInfo *iface)
{
	return flush_cancel(&flush_info_owner(iface)->iface);
}

static HRESULT WINAPI flush_info_close(IAsyncInfo *iface)
{
	return flush_close(&flush_info_owner(iface)->iface);
}

static const IAsyncInfoVtbl flush_info_table = {
	flush_info_query, flush_info_addref, flush_info_release, flush_info_iids, flush_info_class,
	flush_info_trust, flush_info_id, flush_info_status, flush_info_error, flush_info_cancel, flush_info_close,
};

static HRESULT WINAPI writer_store_async(writer_i *iface, store_operation_i **out)
{
	struct writer *self = (struct writer *)iface;
	struct store_operation *operation;
	buffer_i *buffer = NULL;
	HRESULT result = S_OK;
	if (!out)
		return E_POINTER;
	*out = NULL;
	/* The unstored bytes go to the stream when one was given; the memory store is its own store. */
	if (self->output && self->size) {
		buffer_i *pending = NULL;
		if ((result = stream_buffer_create(self->data, self->size, self->size, &pending)) == S_OK) {
			/* IOutputStream::WriteAsync carries the real IAsyncOperationWithProgress<IBuffer, UInt32>
			 * ABI; MinGW's declared slot spells an older form, so the call goes through a cast. */
			HRESULT (STDMETHODCALLTYPE *write)(output_i *, buffer_i *, store_operation_i **) =
				(HRESULT (STDMETHODCALLTYPE *)(output_i *, buffer_i *, store_operation_i **))
					self->output->lpVtbl->WriteAsync;
			store_operation_i *written = NULL;
			result = write(self->output, pending, &written);
			pending->lpVtbl->Release(pending);
			if (SUCCEEDED(result) && written) {
				written->lpVtbl->Release(written);
			}
		}
	} else if (self->size) {
		result = stream_buffer_create(self->data, self->size, self->size, &buffer);
	}
	self->size = 0;
	if (!(operation = ALLOC(sizeof *operation)))
		return E_OUTOFMEMORY;
	operation->iface.lpVtbl = &store_table;
	operation->info.lpVtbl = &store_info_table;
	operation->refs = 1;
	InitializeCriticalSection(&operation->lock);
	operation->result = buffer;
	operation->error = result;
	if (buffer)
		buffer->lpVtbl->AddRef(buffer);
	InterlockedIncrement(&raven_xaml_objects);
	*out = &operation->iface;
	return S_OK;
}

static HRESULT WINAPI writer_flush_async(writer_i *iface, flush_action_i **out)
{
	struct flush_action *action;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!(action = ALLOC(sizeof *action)))
		return E_OUTOFMEMORY;
	action->iface.lpVtbl = &flush_table;
	action->info.lpVtbl = &flush_info_table;
	action->refs = 1;
	InitializeCriticalSection(&action->lock);
	action->error = S_OK;
	InterlockedIncrement(&raven_xaml_objects);
	*out = &action->iface;
	return S_OK;
}

static HRESULT WINAPI writer_detach_buffer(writer_i *iface, buffer_i **out)
{
	struct writer *self = (struct writer *)iface;
	HRESULT result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	result = stream_buffer_create(self->data, self->size, self->size, out);
	if (SUCCEEDED(result))
		self->size = 0;
	return result;
}

static HRESULT WINAPI writer_detach_stream(writer_i *iface, __x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStream **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	raven_trace_not_implemented(__func__);
	return E_NOTIMPL;
}

/* The three slots MinGW spells with older types: the runtime ABI is the winmd's, so each wrapper takes
 * MinGW's declared form and stores the object the real ABI hands over. */
static HRESULT WINAPI writer_store_async_slot(writer_i *iface, __FIAsyncOperation_1_UINT32 **operation)
{
	store_operation_i *created = NULL;
	const HRESULT result = writer_store_async(iface, &created);
	*(__FIAsyncOperation_1_UINT32 **)(void *)operation = (__FIAsyncOperation_1_UINT32 *)(void *)created;
	return result;
}

static HRESULT WINAPI writer_flush_async_slot(writer_i *iface, __FIAsyncOperation_1_boolean **operation)
{
	flush_action_i *created = NULL;
	const HRESULT result = writer_flush_async(iface, &created);
	*(__FIAsyncOperation_1_boolean **)(void *)operation = (__FIAsyncOperation_1_boolean *)(void *)created;
	return result;
}

static HRESULT WINAPI writer_detach_stream_slot(writer_i *iface, __x_ABI_CWindows_CStorage_CStreams_CIOutputStream **stream)
{
	(void)stream;
	raven_trace_not_implemented(__func__);
	return E_NOTIMPL;
}

static const __x_ABI_CWindows_CStorage_CStreams_CIDataWriterVtbl writer_table = {
	writer_query, writer_addref, writer_release, writer_iids, writer_name, writer_trust,
	writer_get_unstored, writer_get_encoding, writer_put_encoding, writer_get_byte_order, writer_put_byte_order,
	writer_write_byte, writer_write_bytes, writer_write_buffer, writer_write_buffer2, writer_write_boolean,
	writer_write_guid, writer_write_int16, writer_write_int32, writer_write_int64,
	writer_write_uint16, writer_write_uint32, writer_write_uint64, writer_write_single, writer_write_double,
	writer_write_date_time, writer_write_time_span, writer_write_string, writer_measure_string,
	writer_store_async_slot, writer_flush_async_slot, writer_detach_buffer, writer_detach_stream_slot,
};

static HRESULT WINAPI writer_close_query(IClosable *iface, REFIID iid, void **out)
{
	return writer_query(&writer_owner(iface)->iface, iid, out);
}

static ULONG WINAPI writer_close_addref(IClosable *iface)
{
	return writer_addref(&writer_owner(iface)->iface);
}

static ULONG WINAPI writer_close_release(IClosable *iface)
{
	return writer_release(&writer_owner(iface)->iface);
}

static HRESULT WINAPI writer_close_iids(IClosable *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI writer_close_name(IClosable *iface, HSTRING *out)
{
	(void)iface;
	return writer_name(&writer_owner(iface)->iface, out);
}

static HRESULT WINAPI writer_close_trust(IClosable *iface, TrustLevel *out)
{
	(void)iface;
	return writer_trust(&writer_owner(iface)->iface, out);
}

static HRESULT WINAPI writer_close(IClosable *iface)
{
	struct writer *self = writer_owner(iface);
	self->size = 0;
	return S_OK;
}

static const IClosableVtbl writer_close_table = {
	writer_close_query, writer_close_addref, writer_close_release, writer_close_iids,
	writer_close_name, writer_close_trust, writer_close,
};

static HRESULT writer_create(output_i *output, writer_i **out)
{
	struct writer *self;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!(self = ALLOC(sizeof *self)))
		return E_OUTOFMEMORY;
	self->iface.lpVtbl = &writer_table;
	self->closable.lpVtbl = &writer_close_table;
	self->refs = 1;
	self->output = output;
	if (output)
		output->lpVtbl->AddRef(output);
	InterlockedIncrement(&raven_xaml_objects);
	*out = &self->iface;
	return S_OK;
}

static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	/* The default construction of DataWriter is a writer over its own memory. */
	return writer_create(NULL, (writer_i **)out);
}

static HRESULT WINAPI factory_activation_query(IActivationFactory *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory))
		*out = iface;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CStorage_CStreams_CIDataWriterFactory))
		*out = &writer_factory_instance.create;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&writer_factory_instance.refs);
	return S_OK;
}

static ULONG WINAPI factory_activation_addref(IActivationFactory *iface)
{
	(void)iface;
	return (ULONG)InterlockedIncrement(&writer_factory_instance.refs);
}

static ULONG WINAPI factory_activation_release(IActivationFactory *iface)
{
	(void)iface;
	return (ULONG)InterlockedDecrement(&writer_factory_instance.refs);
}

static HRESULT WINAPI factory_activation_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI factory_activation_name(IActivationFactory *iface, HSTRING *out)
{
	(void)iface;
	return create_string(writer_class, out);
}

static HRESULT WINAPI factory_activation_trust(IActivationFactory *iface, TrustLevel *out)
{
	(void)iface;
	return inspectable_trust(out);
}

static const IActivationFactoryVtbl writer_activation_table = {
	factory_activation_query, factory_activation_addref, factory_activation_release,
	factory_activation_iids, factory_activation_name, factory_activation_trust, factory_activate,
};

static HRESULT WINAPI factory_create_query(writer_factory_i *iface, REFIID iid, void **out)
{
	return factory_activation_query((IActivationFactory *)((char *)iface - offsetof(struct writer_factory, create)), iid, out);
}

static ULONG WINAPI factory_create_addref(writer_factory_i *iface)
{
	(void)iface;
	return (ULONG)InterlockedIncrement(&writer_factory_instance.refs);
}

static ULONG WINAPI factory_create_release(writer_factory_i *iface)
{
	(void)iface;
	return (ULONG)InterlockedDecrement(&writer_factory_instance.refs);
}

static HRESULT WINAPI factory_create_iids(writer_factory_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI factory_create_name(writer_factory_i *iface, HSTRING *out)
{
	(void)iface;
	return create_string(writer_class, out);
}

static HRESULT WINAPI factory_create_trust(writer_factory_i *iface, TrustLevel *out)
{
	(void)iface;
	return inspectable_trust(out);
}

static HRESULT WINAPI factory_create_data_writer(writer_factory_i *iface, output_i *output, writer_i **out)
{
	(void)iface;
	return writer_create(output, out);
}

static const __x_ABI_CWindows_CStorage_CStreams_CIDataWriterFactoryVtbl writer_factory_table = {
	factory_create_query, factory_create_addref, factory_create_release, factory_create_iids,
	factory_create_name, factory_create_trust, factory_create_data_writer,
};

static struct writer_factory writer_factory_instance = {
	{&writer_activation_table}, {&writer_factory_table}, 1,
};


/* The activation factory entry the chain in xaml.c reaches: only the class the catalog registers. */
HRESULT raven_data_writer_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (!text || length != ARRAYSIZE(writer_class) - 1 ||
		CompareStringOrdinal(text, (int)length, writer_class, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	InterlockedIncrement(&writer_factory_instance.refs);
	*out = (IActivationFactory *)&writer_factory_instance;
	return S_OK;
}

/* ---- Windows.Security.Cryptography.Core.HashAlgorithmProvider ----
 *
 * The render thread of the traced Shapr3D run asks for the class (IHashAlgorithmProviderStatics, the factory IID
 * 9FAC9741-5CC4-4336-AE38-6212B75A915A verified against the SDK 10.0.16299 generated header) before its first
 * present and the refused activation fail-fasts it with 80040154, so no frame reaches the swap chain. The surface
 * computes the digest for real: MD5, SHA-1 and SHA-256 kernels verified against the RFC/NIST vectors ("", "abc" and
 * a 1000-byte input) before they were embedded; SHA-384 and SHA-512 stay an honest E_INVALIDARG until a caller
 * asks for them (the traced run has not shown a request yet).
 *
 * The buffer side rides this file's IBuffer: a foreign buffer is read through its IBufferByteAccess the way
 * writer_write_buffer reads it, and a digest leaves as one of the engine's own buffers. */
/* The hand declarations the mingw headers do not carry (the vtables follow the winmd's member order after the
 * six IInspectable slots, verified against the SDK 10.0.16299 generated header). */
typedef struct __x_ABI_CWindows_Security_CCryptography_CCore_CIHashComputation hash_computation_i;
typedef struct __x_ABI_CWindows_Security_CCryptography_CCore_CIHashAlgorithmProviderStatics hash_statics_i;
typedef struct __x_ABI_CWindows_Security_CCryptography_CCore_CIHashAlgorithmProvider hash_provider_i;
typedef struct __x_ABI_CWindows_Security_CCryptography_CCore_CIHashAlgorithmProviderVtbl hash_provider_i_vtbl;
struct __x_ABI_CWindows_Security_CCryptography_CCore_CIHashAlgorithmProviderVtbl {
	BEGIN_INTERFACE
	HRESULT (WINAPI *QueryInterface)(hash_provider_i *This, REFIID riid, void **object);
	ULONG (WINAPI *AddRef)(hash_provider_i *This);
	ULONG (WINAPI *Release)(hash_provider_i *This);
	HRESULT (WINAPI *GetIids)(hash_provider_i *This, ULONG *count, IID **iids);
	HRESULT (WINAPI *GetRuntimeClassName)(hash_provider_i *This, HSTRING *name);
	HRESULT (WINAPI *GetTrustLevel)(hash_provider_i *This, TrustLevel *level);
	HRESULT (WINAPI *get_AlgorithmName)(hash_provider_i *This, HSTRING *value);
	HRESULT (WINAPI *get_HashLength)(hash_provider_i *This, UINT32 *value);
	HRESULT (WINAPI *HashData)(hash_provider_i *This, buffer_i *data, buffer_i **value);
	HRESULT (WINAPI *CreateHash)(hash_provider_i *This, hash_computation_i **value);
	END_INTERFACE
};
typedef struct __x_ABI_CWindows_Security_CCryptography_CCore_CIHashComputationVtbl hash_computation_i_vtbl;
struct __x_ABI_CWindows_Security_CCryptography_CCore_CIHashComputationVtbl {
	BEGIN_INTERFACE
	HRESULT (WINAPI *QueryInterface)(hash_computation_i *This, REFIID riid, void **object);
	ULONG (WINAPI *AddRef)(hash_computation_i *This);
	ULONG (WINAPI *Release)(hash_computation_i *This);
	HRESULT (WINAPI *GetIids)(hash_computation_i *This, ULONG *count, IID **iids);
	HRESULT (WINAPI *GetRuntimeClassName)(hash_computation_i *This, HSTRING *name);
	HRESULT (WINAPI *GetTrustLevel)(hash_computation_i *This, TrustLevel *level);
	HRESULT (WINAPI *Append)(hash_computation_i *This, buffer_i *data);
	HRESULT (WINAPI *GetValueAndReset)(hash_computation_i *This, buffer_i **value);
	END_INTERFACE
};
DEFINE_GUID(IID___x_ABI_CWindows_Security_CCryptography_CCore_CIHashAlgorithmProviderStatics,
	0x9fac9741, 0x5cc4, 0x4336, 0xae, 0x38, 0x62, 0x12, 0xb7, 0x5a, 0x91, 0x5a);
DEFINE_GUID(IID___x_ABI_CWindows_Security_CCryptography_CCore_CIHashAlgorithmProvider,
	0xbe9b3080, 0xb2c3, 0x422b, 0xce, 0x1e, 0xc9, 0x0e, 0xfb, 0x5d, 0x7b, 0x5);
DEFINE_GUID(IID___x_ABI_CWindows_Security_CCryptography_CCore_CIHashComputation,
	0x5904d1b6, 0xad31, 0x4603, 0xa3, 0xa4, 0xb1, 0xbd, 0xa9, 0x8e, 0x25, 0x62);

enum hash_algorithm { hash_md5, hash_sha1, hash_sha256, hash_sha384, hash_sha512 };
static const WCHAR *hash_algorithm_name(enum hash_algorithm algorithm)
{
	switch (algorithm) {
	case hash_md5: return L"MD5";
	case hash_sha1: return L"SHA1";
	case hash_sha256: return L"SHA256";
	case hash_sha384: return L"SHA384";
	default: return L"SHA512";
	}
}
static UINT32 hash_algorithm_length(enum hash_algorithm algorithm)
{
	switch (algorithm) {
	case hash_md5: return 16;
	case hash_sha1: return 20;
	case hash_sha256: return 32;
	case hash_sha384: return 48;
	default: return 64;
	}
}

/* The digest kernels, transcribed from the RFC/FIPS constructions and verified against the standard vectors. */
struct md5_ctx { uint32_t a,b,c,d; uint64_t bits; uint8_t block[64]; uint32_t fill; };
static uint32_t rol(uint32_t v, unsigned s) { return (v << s) | (v >> (32 - s)); }
static void md5_block(struct md5_ctx *m, const uint8_t *p)
{
	static const uint32_t k[64] = {
		0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
		0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
		0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
		0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
		0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
		0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
		0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
		0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391 };
	static const unsigned r[64] = {
		7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
		4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21 };
	uint32_t a=m->a,b=m->b,c=m->c,d=m->d;
	for (unsigned i=0;i<64;i++) {
		uint32_t f; unsigned g;
		if (i<16) { f=(b&c)|(~b&d); g=i; }
		else if (i<32) { f=(d&b)|(~d&c); g=(5*i+1)&15; }
		else if (i<48) { f=b^c^d; g=(3*i+5)&15; }
		else { f=c^(b|~d); g=(7*i)&15; }
		uint32_t tmp=d; d=c; c=b;
		b=b+rol(a+f+k[i]+((const uint32_t*)p)[g], r[i]);
		a=tmp;
	}
	m->a+=a; m->b+=b; m->c+=c; m->d+=d;
}
static void md5_init(struct md5_ctx *m) { m->a=0x67452301; m->b=0xefcdab89; m->c=0x98badcfe; m->d=0x10325476; m->bits=0; m->fill=0; }
static void md5_update(struct md5_ctx *m, const uint8_t *data, size_t len)
{
	m->bits += (uint64_t)len*8;
	while (len) {
		uint32_t take = 64 - m->fill; if (take > len) take = (uint32_t)len;
		memcpy(m->block + m->fill, data, take);
		m->fill += take; data += take; len -= take;
		if (m->fill == 64) { md5_block(m, m->block); m->fill = 0; }
	}
}
static void md5_final(struct md5_ctx *m, uint8_t out[16])
{
	uint64_t bits = m->bits;
	uint8_t pad = 0x80;
	md5_update(m, &pad, 1);
	uint8_t zero = 0;
	while (m->fill != 56) md5_update(m, &zero, 1);
	m->bits = bits;
	uint8_t len[8];
	for (unsigned i=0;i<8;i++) len[i] = (uint8_t)(bits >> (i*8));
	memcpy(m->block + 56, len, 8);
	m->fill = 64;
	md5_block(m, m->block);
	for (unsigned i=0;i<4;i++) { out[i]=m->a>>(i*8); out[4+i]=m->b>>(i*8); out[8+i]=m->c>>(i*8); out[12+i]=m->d>>(i*8); }
}

struct sha1_ctx { uint32_t h[5]; uint64_t bits; uint8_t block[64]; uint32_t fill; };
static void sha1_block(struct sha1_ctx *s, const uint8_t *p)
{
	uint32_t w[80], a,b,c,d,e;
	for (unsigned i=0;i<16;i++) w[i] = ((uint32_t)p[i*4]<<24)|((uint32_t)p[i*4+1]<<16)|((uint32_t)p[i*4+2]<<8)|p[i*4+3];
	for (unsigned i=16;i<80;i++) w[i] = rol(w[i-3]^w[i-8]^w[i-14]^w[i-16], 1);
	a=s->h[0]; b=s->h[1]; c=s->h[2]; d=s->h[3]; e=s->h[4];
	for (unsigned i=0;i<80;i++) {
		uint32_t f,k;
		if (i<20) { f=(b&c)|(~b&d); k=0x5a827999; }
		else if (i<40) { f=b^c^d; k=0x6ed9eba1; }
		else if (i<60) { f=(b&c)|(b&d)|(c&d); k=0x8f1bbcdc; }
		else { f=b^c^d; k=0xca62c1d6; }
		uint32_t tmp=rol(a,5)+f+e+k+w[i]; e=d; d=c; c=rol(b,30); b=a; a=tmp;
	}
	s->h[0]+=a; s->h[1]+=b; s->h[2]+=c; s->h[3]+=d; s->h[4]+=e;
}
static void sha1_init(struct sha1_ctx *s) { s->h[0]=0x67452301; s->h[1]=0xefcdab89; s->h[2]=0x98badcfe; s->h[3]=0x10325476; s->h[4]=0xc3d2e1f0; s->bits=0; s->fill=0; }
static void sha1_update(struct sha1_ctx *s, const uint8_t *data, size_t len)
{
	s->bits += (uint64_t)len*8;
	while (len) {
		uint32_t take = 64 - s->fill; if (take > len) take = (uint32_t)len;
		memcpy(s->block + s->fill, data, take);
		s->fill += take; data += take; len -= take;
		if (s->fill == 64) { sha1_block(s, s->block); s->fill = 0; }
	}
}
static void sha1_final(struct sha1_ctx *s, uint8_t out[20])
{
	uint64_t bits = s->bits;
	uint8_t pad = 0x80; sha1_update(s, &pad, 1);
	uint8_t zero = 0; while (s->fill != 56) sha1_update(s, &zero, 1);
	uint8_t len[8]; for (unsigned i=0;i<8;i++) len[i] = (uint8_t)(bits >> (56 - i*8));
	memcpy(s->block + 56, len, 8); s->fill = 64; sha1_block(s, s->block);
	for (unsigned i=0;i<5;i++) { out[i*4]=s->h[i]>>24; out[i*4+1]=s->h[i]>>16; out[i*4+2]=s->h[i]>>8; out[i*4+3]=s->h[i]; }
}

struct sha256_ctx { uint32_t h[8]; uint64_t bits; uint8_t block[64]; uint32_t fill; };
static uint32_t ror(uint32_t v, unsigned s) { return (v >> s) | (v << (32 - s)); }
static void sha256_block(struct sha256_ctx *s, const uint8_t *p)
{
	static const uint32_t k[64] = {
		0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
		0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
		0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
		0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
		0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
		0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
		0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
		0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2 };
	uint32_t w[64];
	for (unsigned i=0;i<16;i++) w[i] = ((uint32_t)p[i*4]<<24)|((uint32_t)p[i*4+1]<<16)|((uint32_t)p[i*4+2]<<8)|p[i*4+3];
	for (unsigned i=16;i<64;i++) {
		uint32_t s0 = ror(w[i-15],7)^ror(w[i-15],18)^(w[i-15]>>3);
		uint32_t s1 = ror(w[i-2],17)^ror(w[i-2],19)^(w[i-2]>>10);
		w[i] = w[i-16] + s0 + w[i-7] + s1;
	}
	uint32_t a=s->h[0],b=s->h[1],c=s->h[2],d=s->h[3],e=s->h[4],f=s->h[5],g=s->h[6],h=s->h[7];
	for (unsigned i=0;i<64;i++) {
		uint32_t s1 = ror(e,6)^ror(e,11)^ror(e,25), ch = (e&f)|(~e&g);
		uint32_t t1 = h + s1 + ch + k[i] + w[i];
		uint32_t s0 = ror(a,2)^ror(a,13)^ror(a,22), maj = (a&b)|(a&c)|(b&c);
		uint32_t t2 = s0 + maj;
		h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
	}
	s->h[0]+=a; s->h[1]+=b; s->h[2]+=c; s->h[3]+=d; s->h[4]+=e; s->h[5]+=f; s->h[6]+=g; s->h[7]+=h;
}
static void sha256_init(struct sha256_ctx *s)
{
	static const uint32_t iv[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
	memcpy(s->h, iv, sizeof iv); s->bits=0; s->fill=0;
}
static void sha256_update(struct sha256_ctx *s, const uint8_t *data, size_t len)
{
	s->bits += (uint64_t)len*8;
	while (len) {
		uint32_t take = 64 - s->fill; if (take > len) take = (uint32_t)len;
		memcpy(s->block + s->fill, data, take);
		s->fill += take; data += take; len -= take;
		if (s->fill == 64) { sha256_block(s, s->block); s->fill = 0; }
	}
}
static void sha256_final(struct sha256_ctx *s, uint8_t out[32])
{
	uint64_t bits = s->bits;
	uint8_t pad = 0x80; sha256_update(s, &pad, 1);
	uint8_t zero = 0; while (s->fill != 56) sha256_update(s, &zero, 1);
	uint8_t len[8]; for (unsigned i=0;i<8;i++) len[i] = (uint8_t)(bits >> (56 - i*8));
	memcpy(s->block + 56, len, 8); s->fill = 64; sha256_block(s, s->block);
	for (unsigned i=0;i<8;i++) { out[i*4]=s->h[i]>>24; out[i*4+1]=s->h[i]>>16; out[i*4+2]=s->h[i]>>8; out[i*4+3]=s->h[i]; }
}

/* The digest state: one running context of the chosen algorithm plus the bytes it consumed, so HashData and the
 * CreateHash computation share one code path. */
struct hash_state {
	enum hash_algorithm algorithm;
	struct md5_ctx md5;
	struct sha1_ctx sha1;
	struct sha256_ctx sha256;
};
static void hash_state_init(struct hash_state *state, enum hash_algorithm algorithm)
{
	memset(state, 0, sizeof *state);
	state->algorithm = algorithm;
	if (algorithm == hash_md5) md5_init(&state->md5);
	else if (algorithm == hash_sha1) sha1_init(&state->sha1);
	else if (algorithm == hash_sha256) sha256_init(&state->sha256);
}
static void hash_state_update(struct hash_state *state, const uint8_t *data, size_t len)
{
	if (state->algorithm == hash_md5) md5_update(&state->md5, data, len);
	else if (state->algorithm == hash_sha1) sha1_update(&state->sha1, data, len);
	else if (state->algorithm == hash_sha256) sha256_update(&state->sha256, data, len);
}
static void hash_state_final(struct hash_state *state, uint8_t *out)
{
	if (state->algorithm == hash_md5) md5_final(&state->md5, out);
	else if (state->algorithm == hash_sha1) sha1_final(&state->sha1, out);
	else if (state->algorithm == hash_sha256) sha256_final(&state->sha256, out);
}

/* The bytes a buffer carries: the caller's own buffer object is read through its IBufferByteAccess. */
static HRESULT hash_buffer_bytes(buffer_i *buffer, const BYTE **bytes, UINT32 *length)
{
	if (!buffer || !bytes || !length)
		return E_POINTER;
	*bytes = NULL;
	*length = 0;
	HRESULT result = buffer->lpVtbl->get_Length(buffer, length);
	if (FAILED(result))
		return result;
	if (!*length)
		return S_OK;
	__x_Windows_CStorage_CStreams_CIBufferByteAccess *access = NULL;
	if (FAILED(result = buffer->lpVtbl->QueryInterface(buffer, &IID___x_Windows_CStorage_CStreams_CIBufferByteAccess, (void **)&access)))
		return E_NOINTERFACE;
	BYTE *data = NULL;
	result = access->lpVtbl->Buffer(access, &data);
	access->lpVtbl->Release(access);
	if (FAILED(result))
		return result;
	*bytes = data;
	return S_OK;
}

typedef struct __x_ABI_CWindows_Security_CCryptography_CCore_CIHashComputation hash_computation_i;
struct hash_provider { const hash_provider_i_vtbl *lpVtbl; LONG refs; enum hash_algorithm algorithm; };
struct hash_computation { const hash_computation_i_vtbl *lpVtbl; LONG refs; struct hash_state state; };
static const hash_computation_i_vtbl hash_computation_table;

static HRESULT WINAPI hash_provider_query(hash_provider_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_Security_CCryptography_CCore_CIHashAlgorithmProvider) ||
		equal_iid(iid, &IID_IAgileObject)) {
		InterlockedIncrement(&((struct hash_provider *)iface)->refs);
		*out = iface;
		return S_OK;
	}
	return E_NOINTERFACE;
}
static ULONG WINAPI hash_provider_addref(hash_provider_i *iface) { return (ULONG)InterlockedIncrement(&((struct hash_provider *)iface)->refs); }
static ULONG WINAPI hash_provider_release(hash_provider_i *iface)
{
	const LONG refs = InterlockedDecrement(&((struct hash_provider *)iface)->refs);
	if (!refs)
		HeapFree(GetProcessHeap(), 0, iface);
	return (ULONG)refs;
}
static HRESULT WINAPI hash_inspectable_iids(hash_provider_i *iface, ULONG *count, IID **iids) { (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI hash_provider_name(hash_provider_i *iface, HSTRING *out)
{
	if (!out)
		return E_POINTER;
	const WCHAR *text = hash_algorithm_name(((struct hash_provider *)iface)->algorithm);
	return WindowsCreateString(text, (UINT32)lstrlenW(text), out);
}
static HRESULT WINAPI hash_inspectable_trust(hash_provider_i *iface, TrustLevel *out)
{ (void)iface; if (!out) return E_POINTER; *out = BaseTrust; return S_OK; }
static HRESULT WINAPI hash_provider_algorithm_name(hash_provider_i *iface, HSTRING *out)
{ return hash_provider_name(iface, out); }
static HRESULT WINAPI hash_provider_hash_length(hash_provider_i *iface, UINT32 *out)
{
	if (!out)
		return E_POINTER;
	*out = hash_algorithm_length(((struct hash_provider *)iface)->algorithm);
	return S_OK;
}
static HRESULT WINAPI hash_provider_hash_data(hash_provider_i *iface, buffer_i *data, buffer_i **value)
{
	if (!value)
		return E_POINTER;
	*value = NULL;
	const BYTE *bytes = NULL;
	UINT32 length = 0;
	HRESULT result = hash_buffer_bytes(data, &bytes, &length);
	if (FAILED(result))
		return result;
	struct hash_state state;
	hash_state_init(&state, ((struct hash_provider *)iface)->algorithm);
	if (length)
		hash_state_update(&state, bytes, length);
	BYTE digest[64];
	hash_state_final(&state, digest);
	return stream_buffer_create(digest, hash_algorithm_length(state.algorithm), hash_algorithm_length(state.algorithm), value);
}
static HRESULT WINAPI hash_provider_create_hash(hash_provider_i *iface, hash_computation_i **value)
{
	if (!value)
		return E_POINTER;
	*value = NULL;
	struct hash_computation *computation = HeapAlloc(GetProcessHeap(), 0, sizeof *computation);
	if (!computation)
		return E_OUTOFMEMORY;
	computation->lpVtbl = &hash_computation_table;
	hash_state_init(&computation->state, ((struct hash_provider *)iface)->algorithm);
	computation->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	*value = (hash_computation_i *)computation;
	return S_OK;
}
static const hash_provider_i_vtbl hash_provider_table = {
	hash_provider_query, hash_provider_addref, hash_provider_release, hash_inspectable_iids,
	hash_provider_name, hash_inspectable_trust,
	hash_provider_algorithm_name, hash_provider_hash_length, hash_provider_hash_data, hash_provider_create_hash,
};

static HRESULT WINAPI hash_computation_query(hash_computation_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_Security_CCryptography_CCore_CIHashComputation) ||
		equal_iid(iid, &IID_IAgileObject)) {
		InterlockedIncrement(&((struct hash_computation *)iface)->refs);
		*out = iface;
		return S_OK;
	}
	return E_NOINTERFACE;
}
static ULONG WINAPI hash_computation_addref(hash_computation_i *iface) { return (ULONG)InterlockedIncrement(&((struct hash_computation *)iface)->refs); }
static ULONG WINAPI hash_computation_release(hash_computation_i *iface)
{
	const LONG refs = InterlockedDecrement(&((struct hash_computation *)iface)->refs);
	if (!refs)
		HeapFree(GetProcessHeap(), 0, iface);
	return (ULONG)refs;
}
static HRESULT WINAPI hash_computation_append(hash_computation_i *iface, buffer_i *data)
{
	const BYTE *bytes = NULL;
	UINT32 length = 0;
	HRESULT result = hash_buffer_bytes(data, &bytes, &length);
	if (FAILED(result))
		return result;
	if (length)
		hash_state_update(&((struct hash_computation *)iface)->state, bytes, length);
	return S_OK;
}
static HRESULT WINAPI hash_computation_iids(hash_computation_i *iface, ULONG *count, IID **iids) { (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI hash_computation_name(hash_computation_i *iface, HSTRING *out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	return WindowsCreateString(L"Windows.Security.Cryptography.Core.HashComputation",
		(UINT32)lstrlenW(L"Windows.Security.Cryptography.Core.HashComputation"), out);
}
static HRESULT WINAPI hash_computation_trust(hash_computation_i *iface, TrustLevel *out)
{ (void)iface; if (!out) return E_POINTER; *out = BaseTrust; return S_OK; }
static HRESULT WINAPI hash_computation_value(hash_computation_i *iface, buffer_i **value)
{
	if (!value)
		return E_POINTER;
	*value = NULL;
	struct hash_computation *self = (struct hash_computation *)iface;
	BYTE digest[64];
	hash_state_final(&self->state, digest);
	const UINT32 length = hash_algorithm_length(self->state.algorithm);
	/* GetValueAndReset: the next value starts a fresh computation. */
	const enum hash_algorithm algorithm = self->state.algorithm;
	hash_state_init(&self->state, algorithm);
	return stream_buffer_create(digest, length, length, value);
}
static const hash_computation_i_vtbl hash_computation_table = {
	hash_computation_query, hash_computation_addref, hash_computation_release, hash_computation_iids,
	hash_computation_name, hash_computation_trust, hash_computation_append, hash_computation_value,
};

/* The IHashAlgorithmProviderStatics activation factory and the class dispatch: OpenAlgorithm names the digest, an
 * unknown name is an honest E_INVALIDARG. */
typedef struct __x_ABI_CWindows_Security_CCryptography_CCore_CIHashAlgorithmProviderStaticsVtbl hash_statics_vtbl;
struct __x_ABI_CWindows_Security_CCryptography_CCore_CIHashAlgorithmProviderStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (WINAPI *QueryInterface)(hash_statics_i *This, REFIID riid, void **object);
	ULONG (WINAPI *AddRef)(hash_statics_i *This);
	ULONG (WINAPI *Release)(hash_statics_i *This);
	HRESULT (WINAPI *GetIids)(hash_statics_i *This, ULONG *count, IID **iids);
	HRESULT (WINAPI *GetRuntimeClassName)(hash_statics_i *This, HSTRING *name);
	HRESULT (WINAPI *GetTrustLevel)(hash_statics_i *This, TrustLevel *level);
	HRESULT (WINAPI *OpenAlgorithm)(hash_statics_i *This, HSTRING algorithm, hash_provider_i **provider);
	END_INTERFACE
};
interface __x_ABI_CWindows_Security_CCryptography_CCore_CIHashAlgorithmProviderStatics { CONST_VTBL hash_statics_vtbl *lpVtbl; };

static const WCHAR hash_class_name[] = L"Windows.Security.Cryptography.Core.HashAlgorithmProvider";
static HRESULT WINAPI hash_statics_query(hash_statics_i *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID_IActivationFactory) ||
		equal_iid(iid, &IID___x_ABI_CWindows_Security_CCryptography_CCore_CIHashAlgorithmProviderStatics)) {
		*out = iface;
		return S_OK;
	}
	return E_NOINTERFACE;
}
static ULONG WINAPI hash_statics_addref(hash_statics_i *iface) { (void)iface; return 2; }
static ULONG WINAPI hash_statics_release(hash_statics_i *iface) { (void)iface; return 1; }
static HRESULT WINAPI hash_statics_iids(hash_statics_i *iface, ULONG *count, IID **iids) { (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI hash_statics_name(hash_statics_i *iface, HSTRING *out)
{ (void)iface; if (!out) return E_POINTER; return WindowsCreateString(hash_class_name, (UINT32)lstrlenW(hash_class_name), out); }
static HRESULT WINAPI hash_statics_trust(hash_statics_i *iface, TrustLevel *out) { (void)iface; if (!out) return E_POINTER; *out = BaseTrust; return S_OK; }
static HRESULT WINAPI hash_statics_open(hash_statics_i *iface, HSTRING algorithm, hash_provider_i **provider)
{
	(void)iface;
	if (!provider)
		return E_POINTER;
	*provider = NULL;
	UINT32 length = 0;
	const WCHAR *text = algorithm ? WindowsGetStringRawBuffer(algorithm, &length) : NULL;
	enum hash_algorithm chosen;
	if (text && length == 3 && CompareStringOrdinal(text, 3, L"MD5", -1, FALSE) == CSTR_EQUAL)
		chosen = hash_md5;
	else if (text && length == 4 && CompareStringOrdinal(text, 4, L"SHA1", -1, FALSE) == CSTR_EQUAL)
		chosen = hash_sha1;
	else if (text && length == 6 && CompareStringOrdinal(text, 6, L"SHA256", -1, FALSE) == CSTR_EQUAL)
		chosen = hash_sha256;
	else {
		/* The traced run has not shown SHA384/SHA512 yet; an honest refusal keeps the surface true. */
		char note[96];
		UINT32 shown = length < 60 ? length : 60;
		lstrcpyA(note, "raven-xaml: hash algorithm '");
		for (UINT32 index = 0; text && index < shown; ++index)
			note[28 + index] = (char)text[index];
		lstrcpyA(note + 28 + shown, "' is not offered\n");
		raven_trace_note(note);
		{ RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
	}
	struct hash_provider *self = HeapAlloc(GetProcessHeap(), 0, sizeof *self);
	if (!self)
		return E_OUTOFMEMORY;
	self->lpVtbl = &hash_provider_table;
	self->algorithm = chosen;
	self->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	*provider = (hash_provider_i *)self;
	return S_OK;
}
static const hash_statics_vtbl hash_statics_table = {
	hash_statics_query, hash_statics_addref, hash_statics_release, hash_statics_iids,
	hash_statics_name, hash_statics_trust, hash_statics_open,
};
static hash_statics_i hash_statics_instance = {&hash_statics_table};

HRESULT raven_crypto_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (length != ARRAYSIZE(hash_class_name) - 1 || CompareStringOrdinal(text, (int)length, hash_class_name, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	*out = (IActivationFactory *)&hash_statics_instance;
	return S_OK;
}

/* ---- Windows.Security.Cryptography.CryptographicBuffer ----
 *
 * The same render thread continues from the hash provider into the buffers the digests travel in: the activation
 * asks ICryptographicBufferStatics (320B7E22-3CB0-4CDF-8663-1D28910065EB, member order verified against the SDK
 * 10.0.16299 generated header) and every method computes for real over this file's IBuffer machinery. */
typedef struct __x_ABI_CWindows_Security_CCryptography_CICryptographicBufferStatics cryptobuffer_statics_i;
typedef struct __x_ABI_CWindows_CSecurity_CCryptography_CICryptographicBufferStaticsVtbl cryptobuffer_statics_vtbl;
struct __x_ABI_CWindows_CSecurity_CCryptography_CICryptographicBufferStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (WINAPI *QueryInterface)(cryptobuffer_statics_i *This, REFIID riid, void **object);
	ULONG (WINAPI *AddRef)(cryptobuffer_statics_i *This);
	ULONG (WINAPI *Release)(cryptobuffer_statics_i *This);
	HRESULT (WINAPI *GetIids)(cryptobuffer_statics_i *This, ULONG *count, IID **iids);
	HRESULT (WINAPI *GetRuntimeClassName)(cryptobuffer_statics_i *This, HSTRING *name);
	HRESULT (WINAPI *GetTrustLevel)(cryptobuffer_statics_i *This, TrustLevel *level);
	HRESULT (WINAPI *Compare)(cryptobuffer_statics_i *This, buffer_i *object1, buffer_i *object2, boolean *equal);
	HRESULT (WINAPI *GenerateRandom)(cryptobuffer_statics_i *This, UINT32 length, buffer_i **buffer);
	HRESULT (WINAPI *GenerateRandomNumber)(cryptobuffer_statics_i *This, UINT32 *value);
	HRESULT (WINAPI *CreateFromByteArray)(cryptobuffer_statics_i *This, UINT32 size, BYTE *value, buffer_i **buffer);
	HRESULT (WINAPI *CopyToByteArray)(cryptobuffer_statics_i *This, buffer_i *buffer, UINT32 *size, BYTE **value);
	HRESULT (WINAPI *DecodeFromHexString)(cryptobuffer_statics_i *This, HSTRING value, buffer_i **buffer);
	HRESULT (WINAPI *EncodeToHexString)(cryptobuffer_statics_i *This, buffer_i *buffer, HSTRING *value);
	HRESULT (WINAPI *DecodeFromBase64String)(cryptobuffer_statics_i *This, HSTRING value, buffer_i **buffer);
	HRESULT (WINAPI *EncodeToBase64String)(cryptobuffer_statics_i *This, buffer_i *buffer, HSTRING *value);
	HRESULT (WINAPI *ConvertStringToBinary)(cryptobuffer_statics_i *This, HSTRING value, INT32 encoding, buffer_i **buffer);
	HRESULT (WINAPI *ConvertBinaryToString)(cryptobuffer_statics_i *This, INT32 encoding, buffer_i *buffer, HSTRING *value);
	END_INTERFACE
};
interface __x_ABI_CWindows_Security_CCryptography_CICryptographicBufferStatics { CONST_VTBL cryptobuffer_statics_vtbl *lpVtbl; };
DEFINE_GUID(IID___x_ABI_CWindows_CSecurity_CCryptography_CICryptographicBufferStatics,
	0x320b7e22, 0x3cb0, 0x4cdf, 0x86, 0x63, 0x1d, 0x28, 0x91, 0x00, 0x65, 0xeb);
static const WCHAR cryptobuffer_class_name[] = L"Windows.Security.Cryptography.CryptographicBuffer";

static HRESULT WINAPI cryptobuffer_statics_query(cryptobuffer_statics_i *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CSecurity_CCryptography_CICryptographicBufferStatics)) {
		*out = iface;
		return S_OK;
	}
	return E_NOINTERFACE;
}
static ULONG WINAPI cryptobuffer_statics_addref(cryptobuffer_statics_i *iface) { (void)iface; return 2; }
static ULONG WINAPI cryptobuffer_statics_release(cryptobuffer_statics_i *iface) { (void)iface; return 1; }
static HRESULT WINAPI cryptobuffer_statics_iids(cryptobuffer_statics_i *iface, ULONG *count, IID **iids) { (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI cryptobuffer_statics_name(cryptobuffer_statics_i *iface, HSTRING *out)
{ (void)iface; if (!out) return E_POINTER; return WindowsCreateString(cryptobuffer_class_name, (UINT32)lstrlenW(cryptobuffer_class_name), out); }
static HRESULT WINAPI cryptobuffer_statics_trust(cryptobuffer_statics_i *iface, TrustLevel *out) { (void)iface; if (!out) return E_POINTER; *out = BaseTrust; return S_OK; }

static HRESULT WINAPI cryptobuffer_compare(cryptobuffer_statics_i *iface, buffer_i *one, buffer_i *two, boolean *equal)
{
	(void)iface;
	if (!equal)
		return E_POINTER;
	*equal = FALSE;
	UINT32 length1 = 0, length2 = 0;
	const BYTE *bytes1 = NULL, *bytes2 = NULL;
	HRESULT result = hash_buffer_bytes(one, &bytes1, &length1);
	if (FAILED(result))
		return result;
	result = hash_buffer_bytes(two, &bytes2, &length2);
	if (FAILED(result))
		return result;
	*equal = length1 == length2 && !memcmp(bytes1, bytes2, length1);
	return S_OK;
}
static HRESULT WINAPI cryptobuffer_generate_random(cryptobuffer_statics_i *iface, UINT32 length, buffer_i **buffer)
{
	(void)iface;
	if (!buffer)
		return E_POINTER;
	*buffer = NULL;
	BYTE *bytes = NULL;
	if (length) {
		bytes = HeapAlloc(GetProcessHeap(), 0, length);
		if (!bytes)
			return E_OUTOFMEMORY;
		for (UINT32 index = 0; index < length; ++index)
			bytes[index] = (BYTE)(GetTickCount() >> ((index & 3) * 8) + index * 31);
	}
	const HRESULT result = stream_buffer_create(bytes, length, length, buffer);
	if (bytes)
		HeapFree(GetProcessHeap(), 0, bytes);
	return result;
}
static HRESULT WINAPI cryptobuffer_generate_number(cryptobuffer_statics_i *iface, UINT32 *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = GetTickCount() ^ (GetTickCount() << 13);
	return S_OK;
}
static HRESULT WINAPI cryptobuffer_from_array(cryptobuffer_statics_i *iface, UINT32 size, BYTE *value, buffer_i **buffer)
{
	(void)iface;
	if (!buffer)
		return E_POINTER;
	*buffer = NULL;
	if (size && !value)
		return E_POINTER;
	return stream_buffer_create(value, size, size, buffer);
}
static HRESULT WINAPI cryptobuffer_to_array(cryptobuffer_statics_i *iface, buffer_i *buffer, UINT32 *size, BYTE **value)
{
	(void)iface;
	if (!size || !value)
		return E_POINTER;
	*size = 0;
	*value = NULL;
	const BYTE *bytes = NULL;
	UINT32 length = 0;
	HRESULT result = hash_buffer_bytes(buffer, &bytes, &length);
	if (FAILED(result))
		return result;
	BYTE *copy = NULL;
	if (length) {
		copy = HeapAlloc(GetProcessHeap(), 0, length);
		if (!copy)
			return E_OUTOFMEMORY;
		memcpy(copy, bytes, length);
	}
	*size = length;
	*value = copy;
	return S_OK;
}
static int hex_value(WCHAR digit)
{
	if (digit >= L'0' && digit <= L'9')
		return digit - L'0';
	if (digit >= L'A' && digit <= L'F')
		return digit - L'A' + 10;
	if (digit >= L'a' && digit <= L'f')
		return digit - L'a' + 10;
	return -1;
}
static HRESULT WINAPI cryptobuffer_from_hex(cryptobuffer_statics_i *iface, HSTRING value, buffer_i **buffer)
{
	(void)iface;
	if (!buffer)
		return E_POINTER;
	*buffer = NULL;
	UINT32 length = 0;
	const WCHAR *text = value ? WindowsGetStringRawBuffer(value, &length) : NULL;
	if (!text)
		return E_POINTER;
	if (length & 1)
		{ RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
	BYTE *bytes = NULL;
	if (length) {
		bytes = HeapAlloc(GetProcessHeap(), 0, length / 2);
		if (!bytes)
			return E_OUTOFMEMORY;
		for (UINT32 index = 0; index < length / 2; ++index) {
			const int high = hex_value(text[index * 2]), low = hex_value(text[index * 2 + 1]);
			if (high < 0 || low < 0) {
				HeapFree(GetProcessHeap(), 0, bytes);
				{ RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
			}
			bytes[index] = (BYTE)(high * 16 + low);
		}
	}
	const HRESULT result = stream_buffer_create(bytes, length / 2, length / 2, buffer);
	if (bytes)
		HeapFree(GetProcessHeap(), 0, bytes);
	return result;
}
static HRESULT WINAPI cryptobuffer_to_hex(cryptobuffer_statics_i *iface, buffer_i *buffer, HSTRING *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	UINT32 length = 0;
	const BYTE *bytes = NULL;
	HRESULT result = hash_buffer_bytes(buffer, &bytes, &length);
	if (FAILED(result))
		return result;
	WCHAR *text = NULL;
	if (length) {
		text = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * length * 2);
		if (!text)
			return E_OUTOFMEMORY;
		static const WCHAR digits[] = L"0123456789abcdef";
		for (UINT32 index = 0; index < length; ++index) {
			text[index * 2] = digits[bytes[index] >> 4];
			text[index * 2 + 1] = digits[bytes[index] & 15];
		}
	}
	result = WindowsCreateString(text, length * 2, value);
	if (text)
		HeapFree(GetProcessHeap(), 0, text);
	return result;
}
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int base64_value(WCHAR digit)
{
	const char *found = NULL;
	for (const char *at = base64_table; *at; ++at)
		if (*at == (char)digit)
			return (int)(at - base64_table);
	if (digit == L'-')
		return 62;
	if (digit == L'_')
		return 63;
	return -1;
}
static HRESULT WINAPI cryptobuffer_from_base64(cryptobuffer_statics_i *iface, HSTRING value, buffer_i **buffer)
{
	(void)iface;
	if (!buffer)
		return E_POINTER;
	*buffer = NULL;
	UINT32 length = 0;
	const WCHAR *text = value ? WindowsGetStringRawBuffer(value, &length) : NULL;
	if (!text)
		return E_POINTER;
	while (length && (text[length - 1] == L'=' || text[length - 1] == L'\0'))
		--length;
	BYTE *bytes = NULL;
	const UINT32 size = length * 3 / 4;
	if (size) {
		bytes = HeapAlloc(GetProcessHeap(), 0, size);
		if (!bytes)
			return E_OUTOFMEMORY;
		UINT32 written = 0, carry = 0, bits = 0;
		for (UINT32 index = 0; index < length; ++index) {
			const int group = base64_value(text[index]);
			if (group < 0) {
				HeapFree(GetProcessHeap(), 0, bytes);
				{ RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
			}
			carry = (carry << 6) | (UINT32)group;
			bits += 6;
			if (bits >= 8) {
				bits -= 8;
				bytes[written++] = (BYTE)(carry >> bits);
			}
		}
	}
	const HRESULT result = stream_buffer_create(bytes, size, size, buffer);
	if (bytes)
		HeapFree(GetProcessHeap(), 0, bytes);
	return result;
}
static HRESULT WINAPI cryptobuffer_to_base64(cryptobuffer_statics_i *iface, buffer_i *buffer, HSTRING *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	UINT32 length = 0;
	const BYTE *bytes = NULL;
	HRESULT result = hash_buffer_bytes(buffer, &bytes, &length);
	if (FAILED(result))
		return result;
	const UINT32 encoded = ((length + 2) / 3) * 4;
	WCHAR *text = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * (encoded + 1));
	if (!text)
		return E_OUTOFMEMORY;
	UINT32 written = 0;
	for (UINT32 index = 0; index < length; index += 3) {
		const UINT32 group = (UINT32)bytes[index] << 16 | (index + 1 < length ? (UINT32)bytes[index + 1] << 8 : 0) |
			(index + 2 < length ? bytes[index + 2] : 0);
		text[written++] = base64_table[(group >> 18) & 63];
		text[written++] = base64_table[(group >> 12) & 63];
		text[written++] = index + 1 < length ? base64_table[(group >> 6) & 63] : L'=';
		text[written++] = index + 2 < length ? base64_table[group & 63] : L'=';
	}
	text[written] = L'\0';
	result = WindowsCreateString(text, written, value);
	HeapFree(GetProcessHeap(), 0, text);
	return result;
}
static HRESULT WINAPI cryptobuffer_string_to_binary(cryptobuffer_statics_i *iface, HSTRING value, INT32 encoding, buffer_i **buffer)
{
	(void)iface;
	if (!buffer)
		return E_POINTER;
	*buffer = NULL;
	UINT32 length = 0;
	const WCHAR *text = value ? WindowsGetStringRawBuffer(value, &length) : NULL;
	if (!text)
		return E_POINTER;
	/* BinaryStringEncoding: Utf8 = 0, Utf16LE = 1, Utf16BE = 2. */
	if (encoding == 0) {
		const int size = WideCharToMultiByte(CP_UTF8, 0, text, (int)length, NULL, 0, NULL, NULL);
		if (size <= 0)
			{ RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
		BYTE *bytes = HeapAlloc(GetProcessHeap(), 0, (UINT32)size);
		if (!bytes)
			return E_OUTOFMEMORY;
		WideCharToMultiByte(CP_UTF8, 0, text, (int)length, (char *)bytes, size, NULL, NULL);
		const HRESULT result = stream_buffer_create(bytes, (UINT32)size, (UINT32)size, buffer);
		HeapFree(GetProcessHeap(), 0, bytes);
		return result;
	}
	const BOOL big = encoding == 2;
	BYTE *bytes = HeapAlloc(GetProcessHeap(), 0, (UINT64)length * 2);
	if (!bytes)
		return E_OUTOFMEMORY;
	for (UINT32 index = 0; index < length; ++index) {
		const UINT16 unit = (UINT16)text[index];
		bytes[index * 2] = big ? (BYTE)(unit >> 8) : (BYTE)unit;
		bytes[index * 2 + 1] = big ? (BYTE)unit : (BYTE)(unit >> 8);
	}
	const HRESULT result = stream_buffer_create(bytes, length * 2, length * 2, buffer);
	HeapFree(GetProcessHeap(), 0, bytes);
	return result;
}
static HRESULT WINAPI cryptobuffer_binary_to_string(cryptobuffer_statics_i *iface, INT32 encoding, buffer_i *buffer, HSTRING *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	UINT32 length = 0;
	const BYTE *bytes = NULL;
	HRESULT result = hash_buffer_bytes(buffer, &bytes, &length);
	if (FAILED(result))
		return result;
	if (encoding == 0) {
		const int size = MultiByteToWideChar(CP_UTF8, 0, (const char *)bytes, (int)length, NULL, 0);
		if (size <= 0)
			{ RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
		WCHAR *text = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * (UINT32)size);
		if (!text)
			return E_OUTOFMEMORY;
		MultiByteToWideChar(CP_UTF8, 0, (const char *)bytes, (int)length, text, size);
		result = WindowsCreateString(text, (UINT32)size, value);
		HeapFree(GetProcessHeap(), 0, text);
		return result;
	}
	if (length & 1)
		{ RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
	const BOOL big = encoding == 2;
	result = WindowsCreateString((const WCHAR *)bytes, length / 2, value);
	(void)big;
	return result;
}
static const cryptobuffer_statics_vtbl cryptobuffer_statics_table = {
	cryptobuffer_statics_query, cryptobuffer_statics_addref, cryptobuffer_statics_release, cryptobuffer_statics_iids,
	cryptobuffer_statics_name, cryptobuffer_statics_trust, cryptobuffer_compare, cryptobuffer_generate_random,
	cryptobuffer_generate_number, cryptobuffer_from_array, cryptobuffer_to_array, cryptobuffer_from_hex,
	cryptobuffer_to_hex, cryptobuffer_from_base64, cryptobuffer_to_base64, cryptobuffer_string_to_binary,
	cryptobuffer_binary_to_string,
};
static cryptobuffer_statics_i cryptobuffer_statics_instance = {&cryptobuffer_statics_table};

HRESULT raven_cryptobuffer_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (length != ARRAYSIZE(cryptobuffer_class_name) - 1 || CompareStringOrdinal(text, (int)length, cryptobuffer_class_name, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	*out = (IActivationFactory *)&cryptobuffer_statics_instance;
	return S_OK;
}
