#define WIN32_LEAN_AND_MEAN
#define CINTERFACE
#define COBJMACROS
#define INITGUID
#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <activation.h>
#include <robuffer.h>
#include <windows.foundation.h>
#include <windows.storage.h>
#include <windows.storage.streams.h>
#include <stdint.h>

/* The C-mode names the widl headers give these interfaces, aliased to the spellings the fixture reads. */
typedef __x_ABI_CWindows_CStorage_CStreams_CIDataWriter IDataWriter;
typedef __x_ABI_CWindows_CStorage_CStreams_CIDataWriterFactory IDataWriterFactory;
typedef __x_ABI_CWindows_CStorage_CStreams_CIBuffer IBuffer;
#include <asyncinfo.h>
typedef __x_Windows_CStorage_CStreams_CIBufferByteAccess IBufferByteAccess;
#define IID_IDataWriter IID___x_ABI_CWindows_CStorage_CStreams_CIDataWriter
#define IID_IDataWriterFactory IID___x_ABI_CWindows_CStorage_CStreams_CIDataWriterFactory
#define IID_IBuffer IID___x_ABI_CWindows_CStorage_CStreams_CIBuffer

/* DataWriter, the surface the remote-configuration and HTTP paths build request bodies with: activation
 * through the catalog and its factory, the value writes honoring the byte order, the Unicode encodings,
 * MeasureString, UnstoredBufferLength, the detached buffer's bytes through IBufferByteAccess, and the
 * completed StoreAsync/FlushAsync. */
typedef HRESULT (WINAPI *activation_function)(HSTRING, REFIID, void **);

static int same_text(HSTRING value, const WCHAR *expected)
{
	UINT32 length = 0;
	const WCHAR *text = expected ? WindowsGetStringRawBuffer(value, &length) : NULL;
	return (INT)length == (expected ? (INT)lstrlenW(expected) : 0) &&
		(!expected || CompareStringOrdinal(text, (int)length, expected, -1, FALSE) == CSTR_EQUAL);
}

static HRESULT make_string(const WCHAR *text, HSTRING *out)
{
	return WindowsCreateString(text, (UINT32)lstrlenW(text), out);
}

static HRESULT writer_of(IInspectable *instance, IDataWriter **out)
{
	return instance->lpVtbl->QueryInterface(instance, &IID___x_ABI_CWindows_CStorage_CStreams_CIDataWriter, (void **)out);
}

static int bytes_of(IBuffer *buffer, const BYTE *expected, UINT32 length)
{
	IBufferByteAccess *access = NULL;
	BYTE *data = NULL;
	UINT32 buffer_length = 0;
	int same = 0;
	if (buffer->lpVtbl->get_Length(buffer, &buffer_length) != S_OK)
		return 3001;
	if (buffer_length != length)
		return 3002;
	if (buffer->lpVtbl->QueryInterface(buffer, &IID___x_Windows_CStorage_CStreams_CIBufferByteAccess, (void **)&access) != S_OK)
		return 3003;
	if (access->lpVtbl->Buffer(access, &data) != S_OK)
		return 3004;
	if (data) {
		UINT32 index;
		for (index = 0; index < length; ++index)
			if (data[index] != expected[index])
				break;
		same = index == length;
		if (!same)
			return (int)(1000 + buffer_length);
	}
	access->lpVtbl->Release(access);
	return same;
}

static IAsyncInfo *info_of(IUnknown *operation)
{
	IAsyncInfo *info = NULL;
	operation->lpVtbl->QueryInterface(operation, &IID_IAsyncInfo, (void **)&info);
	return info;
}

int raven_entry(void)
{
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	activation_function activate_factory = runtime ? (activation_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	if (!activate_factory)
		return 1;
	HSTRING key = NULL, text = NULL;
	IActivationFactory *factory = NULL;
	IInspectable *instance = NULL;
	IDataWriter *writer = NULL;
	IDataWriterFactory *writer_factory = NULL;
	HRESULT result;
	UINT32 size = 0, unstored = 0;
	IBuffer *buffer = NULL;
	IUnknown *operation = NULL;
	IAsyncInfo *info = NULL;
	AsyncStatus status;
	HSTRING empty = NULL;

	/* The catalog carries the class; its factory activates an in-memory writer. */
	if (make_string(L"Windows.Storage.Streams.DataWriter", &key) != S_OK)
		return 2;
	if (activate_factory(key, &IID_IActivationFactory, (void **)&factory) != S_OK || !factory)
		return 3;
	if (factory->lpVtbl->QueryInterface(factory, &IID_IDataWriterFactory, (void **)&writer_factory) != S_OK || !writer_factory)
		return 5;
	if (factory->lpVtbl->ActivateInstance(factory, &instance) != S_OK || !instance)
		return 4;
	{
		IDataWriter *from_instance = NULL;
		if (instance->lpVtbl->QueryInterface(instance, &IID_IDataWriter, (void **)&from_instance) != S_OK || !from_instance)
			return 5;
		from_instance->lpVtbl->Release(from_instance);
	}
	if (WindowsCreateString(L"", 0, &empty) != S_OK)
		return 6;
	/* The factory creates a writer over no stream, and refuses a null out. */
	if (writer_factory->lpVtbl->CreateDataWriter(writer_factory, NULL, NULL) != E_POINTER)
		return 7;
	if (writer_factory->lpVtbl->CreateDataWriter(writer_factory, NULL, (IDataWriter **)&writer) != S_OK || !writer)
		return 8;
	/* The activated instance is a writer too. */
	{
		IDataWriter *from_instance = NULL;
		if (writer_of(instance, &from_instance) != S_OK || !from_instance)
			return 9;
		from_instance->lpVtbl->Release(from_instance);
	}
	if (factory->lpVtbl->ActivateInstance(factory, NULL) != E_POINTER)
		return 10;

	/* A default writer writes little-endian UTF-8; the unstored length tracks every write. */
	if (writer->lpVtbl->get_UnicodeEncoding(writer, (__x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding *)&size) != S_OK || size != 0)
		return 11;
	if (writer->lpVtbl->get_ByteOrder(writer, (__x_ABI_CWindows_CStorage_CStreams_CByteOrder *)&size) != S_OK || size != 0)
		return 12;
	if (writer->lpVtbl->WriteByte(writer, 0xAB) != S_OK)
		return 13;
	if (writer->lpVtbl->get_UnstoredBufferLength(writer, &unstored) != S_OK || unstored != 1)
		return 14;
	if (writer->lpVtbl->WriteInt32(writer, 0x12345678) != S_OK)
		return 15;
	if (writer->lpVtbl->get_UnstoredBufferLength(writer, &unstored) != S_OK || unstored != 5)
		return 16;
	if (make_string(L"hi", &text) != S_OK)
		return 17;
	if (writer->lpVtbl->MeasureString(writer, text, &size) != S_OK || size != 2)
		return 100 + size;
	if (writer->lpVtbl->WriteString(writer, text, &size) != S_OK || size != 2)
		return 19;
	if (writer->lpVtbl->get_UnstoredBufferLength(writer, &unstored) != S_OK || unstored != 7)
		return 20;

	/* The detached buffer carries the bytes written so far and empties the writer. */
	if (writer->lpVtbl->DetachBuffer(writer, &buffer) != S_OK || !buffer)
		return 21;
	if (!bytes_of(buffer, (const BYTE *)"\xAB\x78\x56\x34\x12\x68\x69", 7))
		return 22;
	IBufferByteAccess *access = NULL;
	BYTE *data = NULL;
	if (buffer->lpVtbl->QueryInterface(buffer, &IID___x_Windows_CStorage_CStreams_CIBufferByteAccess, (void **)&access) != S_OK)
		return 23;
	if (access->lpVtbl->Buffer(access, &data) != S_OK || !data)
		return 24;
	access->lpVtbl->Release(access);
	buffer->lpVtbl->Release(buffer);
	if (writer->lpVtbl->get_UnstoredBufferLength(writer, &unstored) != S_OK || unstored != 0)
		return 25;

	/* Big-endian values come back byte-reversed, and the encodings size their strings. */
	if (writer->lpVtbl->put_ByteOrder(writer, (__x_ABI_CWindows_CStorage_CStreams_CByteOrder)1) != S_OK)
		return 26;
	if (writer->lpVtbl->WriteInt32(writer, 0x12345678) != S_OK)
		return 27;
	if (writer->lpVtbl->put_UnicodeEncoding(writer, (__x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding)1) != S_OK)
		return 28;
	if (writer->lpVtbl->MeasureString(writer, text, &size) != S_OK || size != 4)
		return 29;
	if (writer->lpVtbl->WriteString(writer, text, &size) != S_OK || size != 4)
		return 30;
	if (writer->lpVtbl->DetachBuffer(writer, &buffer) != S_OK)
		return 31;
	if (!bytes_of(buffer, (const BYTE *)"\x78\x56\x34\x12\x68\x00\x69\x00", 8))
		return 32;
	buffer->lpVtbl->Release(buffer);

	/* StoreAsync on the memory store completes with the stored buffer; FlushAsync completes too. */
	if (writer->lpVtbl->WriteBytes(writer, 3, (BYTE *)"abc") != S_OK)
		return 33;
	{
		__FIAsyncOperationWithProgress_2_Windows__CStorage__CStreams__CIBuffer_UINT32 *typed_operation = NULL;
		if (writer->lpVtbl->StoreAsync(writer, (__FIAsyncOperation_1_UINT32 **)&typed_operation) != S_OK || !typed_operation)
			return 34;
		operation = (IUnknown *)typed_operation;
	}
	if ((info = info_of(operation)) == NULL)
		return 35;
	if (info->lpVtbl->get_Status(info, &status) != S_OK || status != Completed)
		return 36;
	{
		__FIAsyncOperationWithProgress_2_Windows__CStorage__CStreams__CIBuffer_UINT32 *typed =
			(__FIAsyncOperationWithProgress_2_Windows__CStorage__CStreams__CIBuffer_UINT32 *)operation;
		if (typed->lpVtbl->GetResults(typed, &buffer) != S_OK || !buffer)
			return 37;
		if (!bytes_of(buffer, (const BYTE *)"abc", 3))
			return 38;
		buffer->lpVtbl->Release(buffer);
	}
	info->lpVtbl->Release(info);
	operation->lpVtbl->Release(operation);
	operation = NULL;
	{
		__FIAsyncOperation_1_boolean *typed_action = NULL;
		if (writer->lpVtbl->FlushAsync(writer, &typed_action) != S_OK || !typed_action)
			return 39;
		/* The runtime ABI hands back an IAsyncAction; the slot spells MinGW's older form. */
		operation = (IUnknown *)typed_action;
	}
	if ((info = info_of(operation)) == NULL)
		return 40;
	if (info->lpVtbl->get_Status(info, &status) != S_OK || status != Completed)
		return 41;
	info->lpVtbl->Release(info);
	operation->lpVtbl->Release(operation);
	operation = NULL;

	/* WriteBuffer copies an IBuffer's bytes; the writer refuses an out-of-range encoding. */
	if (writer->lpVtbl->WriteString(writer, text, &size) != S_OK || size != 4)
		return 42;
	if (writer->lpVtbl->DetachBuffer(writer, &buffer) != S_OK)
		return 43;
	if (writer->lpVtbl->WriteBuffer(writer, buffer) != S_OK)
		return 44;
	if (writer->lpVtbl->get_UnstoredBufferLength(writer, &unstored) != S_OK || unstored != 4)
		return 45;
	if (writer->lpVtbl->put_UnicodeEncoding(writer, (__x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding)5) != E_INVALIDARG)
		return 46;
	if (writer->lpVtbl->put_ByteOrder(writer, (__x_ABI_CWindows_CStorage_CStreams_CByteOrder)7) != E_INVALIDARG)
		return 47;
	buffer->lpVtbl->Release(buffer);
	buffer = NULL;

	WindowsDeleteString(empty);
	WindowsDeleteString(text);
	writer->lpVtbl->Release(writer);
	writer_factory->lpVtbl->Release(writer_factory);
	instance->lpVtbl->Release(instance);
	factory->lpVtbl->Release(factory);
	WindowsDeleteString(key);
	return 0;
}
