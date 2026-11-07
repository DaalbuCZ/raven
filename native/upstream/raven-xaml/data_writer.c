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
		return E_INVALIDARG;
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
		return E_INVALIDARG;
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

static const __x_ABI_CWindows_CFoundation_CIAsyncActionVtbl flush_table = {
	flush_query, flush_addref, flush_release, flush_iids, flush_class, flush_trust,
	flush_completed_put, flush_completed_get,
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

struct writer_factory {
	IActivationFactory activation;
	writer_factory_i create;
	LONG refs;
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
