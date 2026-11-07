/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The storage fixture. It activates the two Windows.Storage classes the catalog routes to raven-xaml.dll and walks the
   honest slice: ApplicationData.Current's LocalFolder bound to the LocalState directory beside this executable, a file
   created through it and deleted again, the file name looked up and missed, and PathIO's text round trip over absolute
   paths. The async methods of the subset complete before the call returns, so every Completed handler here runs inside
   the call that assigned it. */
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <windows.storage.h>
#include <windows.storage.streams.h>
#include "windows_storage.h"

typedef __x_ABI_CWindows_CStorage_CIApplicationDataStatics appdata_statics_i;
typedef __x_ABI_CWindows_CStorage_CIApplicationData appdata_i;
typedef __x_ABI_CWindows_CStorage_CIApplicationDataContainer container_i;
typedef __x_ABI_CWindows_CStorage_CIStorageFolder folder_i;
typedef __x_ABI_CWindows_CStorage_CIStorageItem item_i;
typedef __x_ABI_CWindows_CStorage_CIStorageFile file_i;
typedef __FIAsyncOperation_1_Windows__CStorage__CStorageFile file_op_i;
struct file_statics_table { void *base[6]; HRESULT (WINAPI *get_path)(void *,HSTRING,file_op_i **); };
struct file_statics { const struct file_statics_table *lpVtbl; };
typedef __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile file_done_i;
typedef __FIAsyncOperation_1_HSTRING text_op_i;
typedef __x_ABI_CWindows_CFoundation_CIAsyncAction action_i;
typedef __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler action_done_i;
typedef __x_ABI_CWindows_CStorage_CIPathIOStatics pathio_statics_i;

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);

#ifdef _WIN64
static const WCHAR fixture_arch[] = L"x64";
#else
static const WCHAR fixture_arch[] = L"x86";
#endif

/* the file the folder round trip carries and the one PathIO writes; both carry the width, so the x64 and x86 runs of
   the fixture can share one LocalState directory without meeting each other's files */
static WCHAR fixture_file_name[64];
static WCHAR fixture_missing_name[64];
static WCHAR fixture_text_name[64];
static const WCHAR fixture_text[] = L"Hello, Raven storage fixture!";
static const char fixture_bytes[] = "Hello, Raven storage fixture!"; /* the UTF-8 spelling of the text above */

static int same_text(HSTRING value, const WCHAR *expected)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(value, &length);
	return (int)length == lstrlenW(expected) && CompareStringOrdinal(text, (int)length, expected, -1, FALSE) == CSTR_EQUAL;
}

static BOOL text_ends_with(HSTRING value, const WCHAR *suffix)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(value, &length);
	SIZE_T suffix_length = lstrlenW(suffix);
	return length >= suffix_length &&
		CompareStringOrdinal(text + length - suffix_length, (int)suffix_length, suffix, -1, FALSE) == CSTR_EQUAL;
}

/* The Completed handler the operation invokes: the invocation count and the last status, for the fixture to check. */
struct file_done {
	file_done_i iface;
	LONG refs;
	ULONG calls;
	AsyncStatus status;
};

static HRESULT WINAPI file_done_query(file_done_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) &&
		!IsEqualIID(iid, &IID___FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile))
		return E_NOINTERFACE;
	InterlockedIncrement(&((struct file_done *)iface)->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI file_done_addref(file_done_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct file_done *)iface)->refs);
}

static ULONG WINAPI file_done_release(file_done_i *iface)
{
	return (ULONG)InterlockedDecrement(&((struct file_done *)iface)->refs);
}

static HRESULT WINAPI file_done_invoke(file_done_i *iface, file_op_i *operation, AsyncStatus status)
{
	struct file_done *self = (struct file_done *)iface;
	(void)operation;
	self->calls++;
	self->status = status;
	return S_OK;
}

static const __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFileVtbl file_done_table = {
	file_done_query, file_done_addref, file_done_release, file_done_invoke,
};

static struct file_done *file_done_new(void)
{
	struct file_done *done = (struct file_done *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *done);
	if (done) {
		done->iface.lpVtbl = (void *)&file_done_table;
		done->refs = 1;
	}
	return done;
}

/* The action's handler, the same shape over IAsyncAction. */
struct action_done {
	action_done_i iface;
	LONG refs;
	ULONG calls;
	AsyncStatus status;
};

static HRESULT WINAPI action_done_query(action_done_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) &&
		!IsEqualIID(iid, &IID___x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler))
		return E_NOINTERFACE;
	InterlockedIncrement(&((struct action_done *)iface)->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI action_done_addref(action_done_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct action_done *)iface)->refs);
}

static ULONG WINAPI action_done_release(action_done_i *iface)
{
	return (ULONG)InterlockedDecrement(&((struct action_done *)iface)->refs);
}

static HRESULT WINAPI action_done_invoke(action_done_i *iface, action_i *action, AsyncStatus status)
{
	struct action_done *self = (struct action_done *)iface;
	(void)action;
	self->calls++;
	self->status = status;
	return S_OK;
}

static const __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandlerVtbl action_done_table = {
	action_done_query, action_done_addref, action_done_release, action_done_invoke,
};

static struct action_done *action_done_new(void)
{
	struct action_done *done = (struct action_done *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *done);
	if (done) {
		done->iface.lpVtbl = (void *)&action_done_table;
		done->refs = 1;
	}
	return done;
}

static BOOL file_exists(const WCHAR *path)
{
	return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
}

/* The check the storage APIs cannot do for themselves: the bytes PathIO wrote are on the disk where the path names. */
static BOOL raw_bytes_equal(const WCHAR *path, const char *expected, DWORD length)
{
	BYTE buffer[128];
	DWORD read = 0;
	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	BOOL same;
	if (file == INVALID_HANDLE_VALUE)
		return FALSE;
	if (!ReadFile(file, buffer, sizeof buffer, &read, NULL)) {
		CloseHandle(file);
		return FALSE;
	}
	CloseHandle(file);
	same = read == length && !memcmp(buffer, expected, length);
	return same;
}

int raven_entry(void)
{
	static const WCHAR appdata_name[] = L"Windows.Storage.ApplicationData";
	static const WCHAR pathio_name[] = L"Windows.Storage.PathIO";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function RoGetActivationFactory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	HSTRING appdata_key = NULL, pathio_key = NULL, text = NULL, name = NULL, stored = NULL;
	IActivationFactory *activation = NULL, *pathio_activation = NULL;
	appdata_statics_i *statics = NULL;
	appdata_i *current = NULL, *current_again = NULL;
	folder_i *folder = NULL, *folder_again = NULL, *roaming = NULL;
	container_i *settings = NULL;
	item_i *item = NULL, *file_item = NULL, *looked_up_item = NULL;
	file_op_i *operation = NULL, *lookup = NULL, *missing = NULL, *collide = NULL, *scratch = NULL;
	text_op_i *text_operation = NULL;
	action_i *write_action = NULL, *delete_action = NULL;
	pathio_statics_i *pathio = NULL;
	file_i *file = NULL, *looked_up = NULL;
	IAsyncInfo *info = NULL;
	struct file_done *file_handler = NULL;
	struct action_done *write_handler = NULL, *delete_handler = NULL;
	AsyncStatus status = Started;
	HRESULT result = S_OK;
	WCHAR path[MAX_PATH];
	UINT32 length = 0;
	const WCHAR *text_buffer;
	SIZE_T keep;
	SIZE_T dir_keep;
	HMODULE component;
	HRESULT (WINAPI *can_unload)(void);
	{
		HMODULE (WINAPI *load_packaged)(const WCHAR *, DWORD) = (void *)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadPackagedLibrary");
		if (!load_packaged || load_packaged(L"anything.dll", 0) || GetLastError() != 15700) return 83;
	}
	if (!RoInitialize || !RoGetActivationFactory) return 20;
	if (FAILED(RoInitialize(1))) return 1;
	lstrcpyW(fixture_file_name, L"raven-storage-fixture-");
	lstrcatW(fixture_file_name, fixture_arch);
	lstrcatW(fixture_file_name, L".txt");
	lstrcpyW(fixture_missing_name, L"raven-storage-missing-");
	lstrcatW(fixture_missing_name, fixture_arch);
	lstrcatW(fixture_missing_name, L".txt");
	lstrcpyW(fixture_text_name, L"raven-storage-text-");
	lstrcatW(fixture_text_name, fixture_arch);
	lstrcatW(fixture_text_name, L".txt");
	if (WindowsCreateString(appdata_name, ARRAYSIZE(appdata_name) - 1, &appdata_key) != S_OK || !appdata_key) return 21;
	if (RoGetActivationFactory(appdata_key, &IID_IActivationFactory, (void **)&activation) != S_OK || !activation) return 2;
	if (activation->lpVtbl->QueryInterface(activation, &IID___x_ABI_CWindows_CStorage_CIApplicationDataStatics,
		(void **)&statics) != S_OK || !statics) return 3;
	/* ApplicationData.Current is the process's one application data: two asks answer the same object */
	if (statics->lpVtbl->get_Current(statics, &current) != S_OK || !current) return 4;
	if (statics->lpVtbl->get_Current(statics, &current_again) != S_OK || current_again != current) return 5;
	{
		HSTRING class_name = NULL;
		if (current->lpVtbl->GetRuntimeClassName(current, &class_name) != S_OK ||
			!same_text(class_name, L"Windows.Storage.ApplicationData")) return 6;
		WindowsDeleteString(class_name);
	}
	{
		__x_ABI_CWindows_CStorage_CIApplicationData2 *data2 = NULL;
		folder_i *cache = NULL, *again = NULL;
		item_i *cache_item = NULL;
		IUnknown *identity = NULL;
		HSTRING cache_path = NULL;
		if (current->lpVtbl->QueryInterface(current, &IID___x_ABI_CWindows_CStorage_CIApplicationData2, (void **)&data2) != S_OK || !data2 ||
			data2->lpVtbl->QueryInterface(data2, &IID_IUnknown, (void **)&identity) != S_OK || identity != (IUnknown *)current ||
			data2->lpVtbl->get_LocalCacheFolder(data2, &cache) != S_OK || !cache ||
			data2->lpVtbl->get_LocalCacheFolder(data2, &again) != S_OK || again != cache ||
			cache->lpVtbl->QueryInterface(cache, &IID___x_ABI_CWindows_CStorage_CIStorageItem, (void **)&cache_item) != S_OK || !cache_item ||
			cache_item->lpVtbl->get_Path(cache_item, &cache_path) != S_OK || !text_ends_with(cache_path, L"\\LocalCache")) return 81;
		const WCHAR *cache_text = WindowsGetStringRawBuffer(cache_path, NULL);
		if (!(GetFileAttributesW(cache_text) & FILE_ATTRIBUTE_DIRECTORY) || GetFileAttributesW(cache_text) == INVALID_FILE_ATTRIBUTES) return 82;
		WindowsDeleteString(cache_path);
		cache_item->lpVtbl->Release(cache_item);
		again->lpVtbl->Release(again);
		cache->lpVtbl->Release(cache);
		identity->lpVtbl->Release(identity);
		data2->lpVtbl->Release(data2);
	}
	/* the LocalFolder is the LocalState directory beside this executable, created on demand; Name and Path answer
	   through the IStorageItem the folder also is */
	if (current->lpVtbl->get_LocalFolder(current, &folder) != S_OK || !folder) return 7;
	if (folder->lpVtbl->QueryInterface(folder, &IID___x_ABI_CWindows_CStorage_CIStorageItem, (void **)&item) != S_OK ||
		!item) return 8;
	if (item->lpVtbl->get_Name(item, &text) != S_OK || !same_text(text, L"LocalState")) return 8;
	WindowsDeleteString(text);
	text = NULL;
	if (item->lpVtbl->get_Path(item, &text) != S_OK || !text_ends_with(text, L"\\LocalState")) return 9;
	length = 0;
	text_buffer = WindowsGetStringRawBuffer(text, &length);
	if (!length || length >= MAX_PATH) return 9;
	memcpy(path, text_buffer, (SIZE_T)length * sizeof(WCHAR));
	path[length] = 0;
	/* the directory path the later file names hang off; the file sections below extend and restore it */
	dir_keep = length;
	WindowsDeleteString(text);
	text = NULL;
	/* the directory exists by the time the call returns */
	{
		DWORD attributes = GetFileAttributesW(path);
		if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY)) return 10;
	}
	if (current->lpVtbl->get_LocalFolder(current, &folder_again) != S_OK || folder_again != folder) return 11;
	if (item->lpVtbl->get_Path(item, &text) != S_OK || !same_text(text, path)) return 12;
	WindowsDeleteString(text);
	text = NULL;
	/* LocalSettings became a real container: the same singleton answers again, roaming stays outside the subset */
	if (current->lpVtbl->get_LocalSettings(current, &settings) != S_OK || !settings) return 15;
	{
		container_i *settings_again = NULL;
		if (current->lpVtbl->get_LocalSettings(current, &settings_again) != S_OK || settings_again != settings) return 15;
		settings_again->lpVtbl->Release(settings_again);
	}
	settings->lpVtbl->Release(settings);
	settings = NULL;
	if (current->lpVtbl->get_RoamingFolder(current, &roaming) != E_NOTIMPL || roaming) return 15;
	/* CreateFileAsync over a fresh name: an empty file on disk, the operation already Completed, the handler already run */
	if (!(file_handler = file_done_new())) return 16;
	if (WindowsCreateString(fixture_file_name, lstrlenW(fixture_file_name), &name) != S_OK || !name) return 17;
	keep = lstrlenW(path);
	if (keep + 1 + lstrlenW(fixture_file_name) + 1 >= MAX_PATH) return 17;
	lstrcpyW(path + keep, L"\\");
	lstrcpyW(path + keep + 1, fixture_file_name);
	if (folder->lpVtbl->CreateFileAsync(folder, name, CreationCollisionOption_ReplaceExisting, &operation) != S_OK ||
		!operation) return 18;
	if (__FIAsyncOperation_1_Windows__CStorage__CStorageFile_QueryInterface(operation, &IID_IAsyncInfo, (void **)&info) != S_OK || !info) return 18;
	if (IAsyncInfo_get_Status(info, &status) != S_OK || status != Completed) return 19;
	IAsyncInfo_Release(info);
	info = NULL;
	if (__FIAsyncOperation_1_Windows__CStorage__CStorageFile_put_Completed(operation, &file_handler->iface) != S_OK ||
		file_handler->calls != 1 || file_handler->status != Completed) return 20;
	if (__FIAsyncOperation_1_Windows__CStorage__CStorageFile_GetResults(operation, &file) != S_OK || !file) return 21;
	/* Name and Path answer through the IStorageItem the file also is, FileType through IStorageFile */
	if (file->lpVtbl->QueryInterface(file, &IID___x_ABI_CWindows_CStorage_CIStorageItem, (void **)&file_item) != S_OK ||
		!file_item) return 21;
	if (file_item->lpVtbl->get_Name(file_item, &text) != S_OK || !same_text(text, fixture_file_name)) return 22;
	WindowsDeleteString(text);
	text = NULL;
	if (file->lpVtbl->get_FileType(file, &text) != S_OK || !same_text(text, L".txt")) return 23;
	WindowsDeleteString(text);
	text = NULL;
	if (file_item->lpVtbl->get_Path(file_item, &text) != S_OK || !same_text(text, path)) return 24;
	WindowsDeleteString(text);
	text = NULL;
	if (!file_exists(path)) return 25;
	{
		__x_ABI_CWindows_CStorage_CStreams_CIInputStreamReference *reference=NULL;
		__FIAsyncOperation_1_Windows__CStorage__CStreams__CIInputStream *opening=NULL;
		__x_ABI_CWindows_CStorage_CStreams_CIInputStream *stream=NULL;
		if (file->lpVtbl->QueryInterface(file,&IID___x_ABI_CWindows_CStorage_CStreams_CIInputStreamReference,(void **)&reference)!=S_OK || !reference ||
			reference->lpVtbl->OpenSequentialReadAsync(reference,&opening)!=S_OK || !opening ||
			opening->lpVtbl->GetResults(opening,&stream)!=S_OK || !stream) return 92;
		stream->lpVtbl->Release(stream); opening->lpVtbl->Release(opening); reference->lpVtbl->Release(reference);
	}
	{
		static const IID iid={0x5984c710,0xdaf2,0x43c8,{0x8b,0xb4,0xa4,0xd3,0xea,0xcf,0xd0,0x3f}};
		static const WCHAR class_name[]=L"Windows.Storage.StorageFile";
		struct file_statics *statics=NULL; HSTRING key=NULL, absolute=NULL; file_op_i *lookup=NULL; file_i *found=NULL;
		if (WindowsCreateString(class_name,ARRAYSIZE(class_name)-1,&key)!=S_OK ||
			RoGetActivationFactory(key,&iid,(void **)&statics)!=S_OK || !statics ||
			WindowsCreateString(path,lstrlenW(path),&absolute)!=S_OK ||
			statics->lpVtbl->get_path(statics,absolute,&lookup)!=S_OK || !lookup ||
			lookup->lpVtbl->GetResults(lookup,&found)!=S_OK || !found) return 91;
		found->lpVtbl->Release(found); lookup->lpVtbl->Release(lookup);
		((IUnknown *)statics)->lpVtbl->Release((IUnknown *)statics);
		WindowsDeleteString(absolute); WindowsDeleteString(key);
	}
	/* the same name under FailIfExists: the call succeeds, the operation completes with the documented error */
	if (folder->lpVtbl->CreateFileAsync(folder, name, CreationCollisionOption_FailIfExists, &collide) != S_OK ||
		!collide) return 26;
	if (__FIAsyncOperation_1_Windows__CStorage__CStorageFile_QueryInterface(collide, &IID_IAsyncInfo, (void **)&info) != S_OK || !info) return 26;
	if (IAsyncInfo_get_Status(info, &status) != S_OK || status != Error) return 27;
	if (IAsyncInfo_get_ErrorCode(info, &result) != S_OK ||
		result != (HRESULT)0x80070050UL /* HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) */) return 28;
	IAsyncInfo_Release(info);
	info = NULL;
	/* GenerateUniqueName is not the subset's to promise; the failing call hands out nothing to release */
	if (folder->lpVtbl->CreateFileAsync(folder, name, CreationCollisionOption_GenerateUniqueName, &scratch) !=
		E_NOTIMPL || scratch) return 29;
	/* GetFileAsync round-trips a name the directory has and misses one it does not */
	if (folder->lpVtbl->GetFileAsync(folder, name, &lookup) != S_OK || !lookup) return 30;
	if (__FIAsyncOperation_1_Windows__CStorage__CStorageFile_QueryInterface(lookup, &IID_IAsyncInfo, (void **)&info) != S_OK || !info) return 30;
	if (IAsyncInfo_get_Status(info, &status) != S_OK || status != Completed) return 31;
	IAsyncInfo_Release(info);
	info = NULL;
	if (__FIAsyncOperation_1_Windows__CStorage__CStorageFile_GetResults(lookup, &looked_up) != S_OK || !looked_up)
		return 32;
	if (looked_up->lpVtbl->QueryInterface(looked_up, &IID___x_ABI_CWindows_CStorage_CIStorageItem,
		(void **)&looked_up_item) != S_OK || !looked_up_item) return 32;
	if (looked_up_item->lpVtbl->get_Name(looked_up_item, &text) != S_OK || !same_text(text, fixture_file_name))
		return 33;
	WindowsDeleteString(text);
	text = NULL;
	if (WindowsCreateString(fixture_missing_name, lstrlenW(fixture_missing_name), &name) != S_OK || !name) return 34;
	if (folder->lpVtbl->GetFileAsync(folder, name, &missing) != S_OK || !missing) return 35;
	if (__FIAsyncOperation_1_Windows__CStorage__CStorageFile_QueryInterface(missing, &IID_IAsyncInfo, (void **)&info) != S_OK || !info) return 35;
	if (IAsyncInfo_get_Status(info, &status) != S_OK || status != Error) return 36;
	if (IAsyncInfo_get_ErrorCode(info, &result) != S_OK ||
		result != (HRESULT)0x80070002UL /* HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) */) return 37;
	IAsyncInfo_Release(info);
	info = NULL;
	/* PathIO over absolute paths: the write lands byte for byte, the read gives the text back */
	if (WindowsCreateString(pathio_name, ARRAYSIZE(pathio_name) - 1, &pathio_key) != S_OK || !pathio_key) return 38;
	if (RoGetActivationFactory(pathio_key, &IID_IActivationFactory, (void **)&pathio_activation) != S_OK ||
		!pathio_activation) return 39;
	if (pathio_activation->lpVtbl->QueryInterface(pathio_activation, &IID__x_ABI_CWindows_CStorage_CIPathIOStatics,
		(void **)&pathio) != S_OK || !pathio) return 40;
	if (WindowsCreateString(fixture_text, lstrlenW(fixture_text), &text) != S_OK || !text) return 41;
	/* the create section left the file path behind; the text file hangs off the directory again */
	path[dir_keep] = 0;
	keep = lstrlenW(path);
	lstrcpyW(path + keep, L"\\");
	lstrcpyW(path + keep + 1, fixture_text_name);
	if (WindowsCreateString(path, lstrlenW(path), &name) != S_OK || !name) return 41;
	if (!(write_handler = action_done_new())) return 42;
	if (pathio->lpVtbl->WriteTextAsync(pathio, name, text, &write_action) != S_OK || !write_action) return 43;
	if (write_action->lpVtbl->QueryInterface(write_action, &IID_IAsyncInfo, (void **)&info) != S_OK || !info) return 43;
	if (IAsyncInfo_get_Status(info, &status) != S_OK || status != Completed) return 44;
	IAsyncInfo_Release(info);
	info = NULL;
	if (__x_ABI_CWindows_CFoundation_CIAsyncAction_put_Completed(write_action, &write_handler->iface) != S_OK ||
		write_handler->calls != 1 || write_handler->status != Completed) return 45;
	if (!raw_bytes_equal(path, fixture_bytes, (DWORD)(ARRAYSIZE(fixture_bytes) - 1))) return 46;
	if (pathio->lpVtbl->ReadTextAsync(pathio, name, &text_operation) != S_OK || !text_operation) return 47;
	if (__FIAsyncOperation_1_HSTRING_QueryInterface(text_operation, &IID_IAsyncInfo, (void **)&info) != S_OK || !info) return 47;
	if (IAsyncInfo_get_Status(info, &status) != S_OK || status != Completed) return 48;
	IAsyncInfo_Release(info);
	info = NULL;
	if (__FIAsyncOperation_1_HSTRING_GetResults(text_operation, &stored) != S_OK || !stored) return 49;
	if (!same_text(stored, fixture_text)) return 50;
	WindowsDeleteString(stored);
	stored = NULL;
	/* DeleteAsync, through the item the file also is, removes the file the folder created */
	if (!(delete_handler = action_done_new())) return 51;
	if (file_item->lpVtbl->DeleteAsyncOverloadDefaultOptions(file_item, &delete_action) != S_OK || !delete_action)
		return 52;
	if (delete_action->lpVtbl->QueryInterface(delete_action, &IID_IAsyncInfo, (void **)&info) != S_OK || !info)
		return 52;
	if (IAsyncInfo_get_Status(info, &status) != S_OK || status != Completed) return 53;
	IAsyncInfo_Release(info);
	info = NULL;
	if (__x_ABI_CWindows_CFoundation_CIAsyncAction_put_Completed(delete_action, &delete_handler->iface) != S_OK ||
		delete_handler->calls != 1 || delete_handler->status != Completed) return 54;
	keep = lstrlenW(path) - lstrlenW(fixture_text_name);
	lstrcpyW(path + keep, fixture_file_name);
	if (file_exists(path)) return 55;
	__FIAsyncOperation_1_HSTRING_Release(text_operation);
	__x_ABI_CWindows_CFoundation_CIAsyncAction_Release(write_action);
	action_done_release(&write_handler->iface);
	__x_ABI_CWindows_CFoundation_CIAsyncAction_Release(delete_action);
	action_done_release(&delete_handler->iface);
	item->lpVtbl->Release(item);
	file_item->lpVtbl->Release(file_item);
	looked_up_item->lpVtbl->Release(looked_up_item);
	__x_ABI_CWindows_CStorage_CIStorageFile_Release(looked_up);
	__FIAsyncOperation_1_Windows__CStorage__CStorageFile_Release(lookup);
	__FIAsyncOperation_1_Windows__CStorage__CStorageFile_Release(missing);
	__FIAsyncOperation_1_Windows__CStorage__CStorageFile_Release(collide);
	__x_ABI_CWindows_CStorage_CIStorageFile_Release(file);
	__FIAsyncOperation_1_Windows__CStorage__CStorageFile_Release(operation);
	file_done_release(&file_handler->iface);
	folder_again->lpVtbl->Release(folder_again);
	folder->lpVtbl->Release(folder);
	current_again->lpVtbl->Release(current_again);
	current->lpVtbl->Release(current);
	statics->lpVtbl->Release(statics);
	activation->lpVtbl->Release(activation);
	pathio->lpVtbl->Release(pathio);
	pathio_activation->lpVtbl->Release(pathio_activation);
	WindowsDeleteString(pathio_key);
	WindowsDeleteString(appdata_key);
	WindowsDeleteString(name);
	WindowsDeleteString(text);
	/* the component keeps no object behind: after the last reference drops, the module unloads clean */
	component = GetModuleHandleW(L"raven-xaml.dll");
	can_unload = component ? (HRESULT (WINAPI *)(void))GetProcAddress(component, "DllCanUnloadNow") : NULL;
	if (can_unload && can_unload() != S_OK) return 56;
	return 0;
}
