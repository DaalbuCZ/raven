/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.Storage subset of the Windows Runtime.
 *
 * The local state is real. ApplicationData.Current's LocalFolder is the LocalState directory beside the running
 * executable — the same resolution package.c applies to the manifest and mrt_manager.c to resources.pri — created on
 * demand, and every file the subset hands out is a real file on the emulated filesystem (CreateFileW/ReadFile/
 * WriteFile). PathIO takes absolute paths for the same reason: they are read and written where they name.
 *
 * The async methods run to completion inside the call that returns them — there is no thread hop, the CPU-fallback
 * style of the composition presentation. An operation is terminal by the time it escapes, GetResults answers at once,
 * and a Completed handler assigned after completion is invoked immediately, the way the dispatcher actions complete
 * their already-finished work (dispatcher_queue.c's shutdown action carries the same shape). A method that cannot
 * produce its result reports it through the operation's Error status, never by failing the call itself: GetFileAsync
 * over a name the directory does not have completes with HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), the answer Windows
 * documents for a missing item, which GetResults propagates.
 *
 * CreateFileAsync honours the CreationCollisionOption the winmd numbers: ReplaceExisting (1) truncates an existing
 * file, FailIfExists (2) fails with ERROR_FILE_EXISTS, and GenerateUniqueName (0) and OpenIfExists (3) answer
 * E_NOTIMPL rather than a made-up name. Text is UTF-8 without a byte order mark, the default encoding of FileIO and
 * PathIO, and the only one this subset serves. DeleteAsync deletes for both StorageDeleteOption values: the emulated
 * filesystem has no recycle bin to send the file to. Everything outside the listed surface answers E_NOTIMPL at the
 * exact signature its vtable slot declares. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <winstring.h>
#include <stddef.h>
#include <windows.storage.h>
#include <windows.storage.streams.h>
#include <robuffer.h>
#include "windows_storage.h"

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))
static void trace_not_implemented(const char *function)
{
	WCHAR message[128];
	UINT32 at = 0;
	static const WCHAR head[] = L"raven-storage not implemented: ";
	for (const WCHAR *scan = head; *scan && at < ARRAYSIZE(message) - 1; ++scan)
		message[at++] = *scan;
	for (const char *scan = function; *scan && at < ARRAYSIZE(message) - 1; ++scan)
		message[at++] = (WCHAR)*scan;
	message[at] = 0;
	if (GetEnvironmentVariableW(L"RAVEN_TRACE_DEBUG", NULL, 0))
		OutputDebugStringW(message);
}

#ifndef E_ILLEGAL_METHOD_CALL
#define E_ILLEGAL_METHOD_CALL ((HRESULT)0x8000000e)
#endif
#ifndef E_ILLEGAL_DELEGATE_ASSIGNMENT
#define E_ILLEGAL_DELEGATE_ASSIGNMENT ((HRESULT)0x80000018)
#endif
#ifndef RO_E_CLOSED
#define RO_E_CLOSED ((HRESULT)0x80000013)
#endif

typedef __x_ABI_CWindows_CStorage_CIApplicationData application_data_i;
typedef __x_ABI_CWindows_CStorage_CIApplicationDataStatics application_data_statics_i;
typedef __x_ABI_CWindows_CStorage_CIStorageFolder folder_i;
typedef __x_ABI_CWindows_CStorage_CIStorageFile file_i;
typedef __x_ABI_CWindows_CStorage_CIStorageItem item_i;
typedef __x_ABI_CWindows_CStorage_CIPathIOStatics pathio_statics_i;
typedef __FIAsyncOperation_1_Windows__CStorage__CStorageFile file_operation_i;
typedef __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile file_completed_i;
typedef __FIAsyncOperation_1_HSTRING text_operation_i;
typedef __FIAsyncOperationCompletedHandler_1_HSTRING text_completed_i;
typedef __x_ABI_CWindows_CFoundation_CIAsyncAction async_action_i;
typedef __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler action_completed_i;

static const WCHAR application_data_class_name[] = L"Windows.Storage.ApplicationData";
static const WCHAR pathio_class_name[] = L"Windows.Storage.PathIO";
static const WCHAR folder_class_name[] = L"Windows.Storage.StorageFolder";
static const WCHAR file_class_name[] = L"Windows.Storage.StorageFile";
static const WCHAR local_state_name[] = L"LocalState";
/* The parameterized names spell the signature the way the projected type system prints them; each async type
carries its own beside its vtable. */
static const WCHAR application_data_statics_class_name[] = L"Windows.Storage.IApplicationDataStatics";
static const WCHAR pathio_statics_class_name[] = L"Windows.Storage.IPathIOStatics";

/* A text file larger than this answers E_OUTOFMEMORY rather than exhausting the commit the guest was granted; the
 * configuration-sized files the API serves stay far below it. */
#define STORAGE_TEXT_MAX (16 * 1024 * 1024)

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
	*out = NULL;
	if (!text)
		text = L"";
	return WindowsCreateString(text, (UINT32)lstrlenW(text), out);
}

static HRESULT last_error(void)
{
	DWORD error = GetLastError();
	return HRESULT_FROM_WIN32(error ? error : ERROR_GEN_FAILURE);
}

/* The directory the running image lives in; the local state is beside it, as the manifest and the PRI are. */
static BOOL executable_directory(WCHAR *path /* MAX_PATH */)
{
	WCHAR directory[MAX_PATH];
	DWORD length = GetModuleFileNameW(NULL, directory, MAX_PATH);
	UINT32 cut = 0;
	UINT32 i;
	if (!length || length >= MAX_PATH)
		return FALSE;
	for (i = 0; i < length; ++i)
		if (directory[i] == L'\\' || directory[i] == L'/')
			cut = i;
	directory[cut] = 0;
	lstrcpyW(path, directory);
	return TRUE;
}

/* The LocalState directory: created on demand, verified afterwards, so get_LocalFolder answers a directory that
 * exists by the time the call returns. */
static BOOL ensure_application_folder(WCHAR *path /* MAX_PATH */, const WCHAR *folder_name)
{
	WCHAR directory[MAX_PATH];
	DWORD attributes;
	if (!executable_directory(directory))
		return FALSE;
	if ((UINT32)lstrlenW(directory) + 1 + (UINT32)lstrlenW(folder_name) + 1 >= MAX_PATH)
		return FALSE;
	lstrcpyW(path, directory);
	lstrcatW(path, L"\\");
	lstrcatW(path, folder_name);
	attributes = GetFileAttributesW(path);
	if (attributes != INVALID_FILE_ATTRIBUTES)
		return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	if (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
		return FALSE;
	attributes = GetFileAttributesW(path);
	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

#include "storage_settings.inc"

/* The async core
 *
 * Every operation and action of the subset shares the shape the dispatcher actions carry: the status, the error, the
 * assigned completed handler and the IAsyncInfo identity, one lock over the transitions, the handler invoked outside
 * the lock. The terminal state is always reached before the object escapes its creating call, so nothing is ever in
 * flight: put_Completed after completion invokes the handler immediately and Cancel has nothing to cancel. */
struct async_core {
	LONG refs;
	CRITICAL_SECTION lock;
	AsyncStatus status;
	HRESULT error;
	UINT32 id;
	BOOL closed, assigned;
	void *completed;
};

static LONG next_async_id;

/* The lifetime and the IAsyncInfo part, identical for every instantiation. The type supplies its _free function and
 * its _type_name before the macro expands, and the completed handler type every slot releases and invokes. */
#define STORAGE_ASYNC_LIFETIME(name, iface_type, own_iid, handler_type) \
	static HRESULT WINAPI name##_query(iface_type *iface, REFIID iid, void **out) \
	{ \
		struct name *self = (struct name *)iface; \
		if (!out) \
			return E_POINTER; \
		*out = NULL; \
		if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || \
			equal_iid(iid, own_iid) || equal_iid(iid, &IID_IAgileObject)) \
			*out = iface; \
		else if (equal_iid(iid, &IID_IAsyncInfo)) \
			*out = &self->info; \
		else \
			return E_NOINTERFACE; \
		InterlockedIncrement(&self->core.refs); \
		return S_OK; \
	} \
	static ULONG WINAPI name##_addref(iface_type *iface) \
	{ \
		return (ULONG)InterlockedIncrement(&((struct name *)iface)->core.refs); \
	} \
	static ULONG WINAPI name##_release(iface_type *iface) \
	{ \
		struct name *self = (struct name *)iface; \
		const LONG refs = InterlockedDecrement(&self->core.refs); \
		if (!refs) { \
			if (self->core.completed) \
				handler_type##_Release((handler_type *)self->core.completed); \
			name##_free(self); \
			DeleteCriticalSection(&self->core.lock); \
			FREE(self); \
			InterlockedDecrement(&raven_xaml_objects); \
		} \
		return (ULONG)refs; \
	} \
	static HRESULT WINAPI name##_iids(iface_type *iface, ULONG *count, IID **iids) \
	{ \
		(void)iface; \
		return inspectable_iids(count, iids); \
	} \
	static HRESULT WINAPI name##_class(iface_type *iface, HSTRING *out) \
	{ \
		(void)iface; \
		return create_string(name##_type_name, out); \
	} \
	static HRESULT WINAPI name##_trust(iface_type *iface, TrustLevel *level) \
	{ \
		(void)iface; \
		return inspectable_trust(level); \
	} \
	static struct name *name##_owner(IAsyncInfo *info) \
	{ \
		return CONTAINING_RECORD(info, struct name, info); \
	} \
	static HRESULT WINAPI name##_info_query(IAsyncInfo *info, REFIID iid, void **out) \
	{ \
		return name##_query(&name##_owner(info)->iface, iid, out); \
	} \
	static ULONG WINAPI name##_info_addref(IAsyncInfo *info) \
	{ \
		return name##_addref(&name##_owner(info)->iface); \
	} \
	static ULONG WINAPI name##_info_release(IAsyncInfo *info) \
	{ \
		return name##_release(&name##_owner(info)->iface); \
	} \
	static HRESULT WINAPI name##_info_iids(IAsyncInfo *info, ULONG *count, IID **iids) \
	{ \
		(void)info; \
		return inspectable_iids(count, iids); \
	} \
	static HRESULT WINAPI name##_info_class(IAsyncInfo *info, HSTRING *out) \
	{ \
		(void)info; \
		return create_string(name##_type_name, out); \
	} \
	static HRESULT WINAPI name##_info_trust(IAsyncInfo *info, TrustLevel *level) \
	{ \
		(void)info; \
		return inspectable_trust(level); \
	} \
	static HRESULT WINAPI name##_info_id(IAsyncInfo *info, UINT32 *out) \
	{ \
		struct name *self = name##_owner(info); \
		HRESULT result; \
		if (!out) \
			return E_POINTER; \
		EnterCriticalSection(&self->core.lock); \
		result = self->core.closed ? RO_E_CLOSED : S_OK; \
		*out = self->core.id; \
		LeaveCriticalSection(&self->core.lock); \
		return result; \
	} \
	static HRESULT WINAPI name##_info_status(IAsyncInfo *info, AsyncStatus *out) \
	{ \
		struct name *self = name##_owner(info); \
		HRESULT result; \
		if (!out) \
			return E_POINTER; \
		EnterCriticalSection(&self->core.lock); \
		result = self->core.closed ? RO_E_CLOSED : S_OK; \
		*out = self->core.status; \
		LeaveCriticalSection(&self->core.lock); \
		return result; \
	} \
	static HRESULT WINAPI name##_info_error(IAsyncInfo *info, HRESULT *out) \
	{ \
		struct name *self = name##_owner(info); \
		HRESULT result; \
		if (!out) \
			return E_POINTER; \
		EnterCriticalSection(&self->core.lock); \
		result = self->core.closed ? RO_E_CLOSED : S_OK; \
		*out = self->core.error; \
		LeaveCriticalSection(&self->core.lock); \
		return result; \
	} \
	static HRESULT WINAPI name##_info_cancel(IAsyncInfo *info) \
	{ \
		/* terminal before it ever escaped; there is nothing in flight to cancel */ \
		(void)info; \
		return S_OK; \
	} \
	static HRESULT WINAPI name##_info_close(IAsyncInfo *info) \
	{ \
		struct name *self = name##_owner(info); \
		void *handler = NULL; \
		HRESULT result = S_OK; \
		EnterCriticalSection(&self->core.lock); \
		if (self->core.status == Started) result = E_ILLEGAL_METHOD_CALL; \
		else { \
			self->core.closed = TRUE; \
			handler = self->core.completed; \
			self->core.completed = NULL; \
		} \
		LeaveCriticalSection(&self->core.lock); \
		if (handler) \
			handler_type##_Release((handler_type *)handler); \
		return result; \
	} \
	static const IAsyncInfoVtbl name##_info_table = { \
		name##_info_query, name##_info_addref, name##_info_release, name##_info_iids, name##_info_class, \
		name##_info_trust, name##_info_id, name##_info_status, name##_info_error, name##_info_cancel, \
		name##_info_close, \
	};

#define STORAGE_ASYNC_COMPLETED(name, iface_type, handler_type) \
	static HRESULT WINAPI name##_put_completed(iface_type *iface, handler_type *handler) \
	{ \
		struct name *self = (struct name *)iface; \
		HRESULT result = S_OK; \
		AsyncStatus status; \
		EnterCriticalSection(&self->core.lock); \
		status = self->core.status; \
		if (self->core.closed) result = RO_E_CLOSED; \
		else if (self->core.assigned) result = E_ILLEGAL_DELEGATE_ASSIGNMENT; \
		else { \
			self->core.assigned = TRUE; \
			self->core.completed = handler; \
			if (handler) { \
				handler_type##_AddRef(handler); \
				if (status != Started) handler_type##_AddRef(handler); \
			} \
		} \
		LeaveCriticalSection(&self->core.lock); \
		if (SUCCEEDED(result) && handler && status != Started) { \
			handler_type##_Invoke(handler, iface, status); \
			handler_type##_Release(handler); \
		} \
		return result; \
	} \
	static HRESULT WINAPI name##_get_completed(iface_type *iface, handler_type **handler) \
	{ \
		struct name *self = (struct name *)iface; \
		HRESULT result = S_OK; \
		if (!handler) \
			return E_POINTER; \
		*handler = NULL; \
		EnterCriticalSection(&self->core.lock); \
		if (self->core.closed) result = RO_E_CLOSED; \
		else if ((*handler = (handler_type *)self->core.completed)) handler_type##_AddRef(*handler); \
		LeaveCriticalSection(&self->core.lock); \
		return result; \
	}

#define STORAGE_ASYNC_FINISH(name, iface_type, handler_type) \
	static void name##_finish(iface_type *iface, HRESULT error, AsyncStatus status) \
	{ \
		struct name *self = (struct name *)iface; \
		handler_type *handler = NULL; \
		EnterCriticalSection(&self->core.lock); \
		if (self->core.status == Started) { \
			self->core.status = status; \
			self->core.error = error; \
			handler = (handler_type *)self->core.completed; \
			if (handler) handler_type##_AddRef(handler); \
		} \
		LeaveCriticalSection(&self->core.lock); \
		if (handler) { \
			handler_type##_Invoke(handler, iface, status); \
			handler_type##_Release(handler); \
		} \
	}

/* The instantiation's constructor: emitted after the vtable it fills the iface with. */
#define STORAGE_ASYNC_NEW(name, iface_type) \
	static struct name *name##_new(void) \
	{ \
		struct name *self = ALLOC(sizeof *self); \
		if (!self) \
			return NULL; \
		self->iface.lpVtbl = (void *)&name##_table; \
		self->info.lpVtbl = (void *)&name##_info_table; \
		self->core.refs = 1; \
		self->core.status = Started; \
		self->core.error = S_OK; \
		self->core.id = (UINT32)InterlockedIncrement(&next_async_id); \
		InitializeCriticalSection(&self->core.lock); \
		InterlockedIncrement(&raven_xaml_objects); \
		return self; \
	}

/* The operation that produces a StorageFile: CreateFileAsync and GetFileAsync return it. */
struct file_operation {
	file_operation_i iface;
	IAsyncInfo info;
	struct async_core core;
	file_i *result;
};

static void file_operation_free(struct file_operation *self)
{
	if (self->result)
		__x_ABI_CWindows_CStorage_CIStorageFile_Release(self->result);
}

static const WCHAR file_operation_type_name[] = L"Windows.Foundation.IAsyncOperation`1<Windows.Storage.StorageFile>";

STORAGE_ASYNC_LIFETIME(file_operation, file_operation_i, &IID___FIAsyncOperation_1_Windows__CStorage__CStorageFile,
	__FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile)
STORAGE_ASYNC_COMPLETED(file_operation, file_operation_i, __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile)
STORAGE_ASYNC_FINISH(file_operation, file_operation_i, __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStorageFile)

static HRESULT WINAPI file_operation_results(file_operation_i *iface, __x_ABI_CWindows_CStorage_CIStorageFile **results)
{
	struct file_operation *self = (struct file_operation *)iface;
	HRESULT result;
	if (!results)
		return E_POINTER;
	*results = NULL;
	EnterCriticalSection(&self->core.lock);
	result = self->core.closed ? RO_E_CLOSED : self->core.status == Started ? E_ILLEGAL_METHOD_CALL : self->core.error;
	LeaveCriticalSection(&self->core.lock);
	if (SUCCEEDED(result) && self->result) {
		/* the operation keeps its result alive until the guest releases it */
		*results = self->result;
		__x_ABI_CWindows_CStorage_CIStorageFile_AddRef(*results);
	}
	return result;
}

static const __FIAsyncOperation_1_Windows__CStorage__CStorageFileVtbl file_operation_table = {
	.QueryInterface = file_operation_query,
	.AddRef = file_operation_addref,
	.Release = file_operation_release,
	.GetIids = file_operation_iids,
	.GetRuntimeClassName = file_operation_class,
	.GetTrustLevel = file_operation_trust,
	.put_Completed = file_operation_put_completed,
	.get_Completed = file_operation_get_completed,
	.GetResults = file_operation_results,
};

STORAGE_ASYNC_NEW(file_operation, file_operation_i)

/* The operation that produces a text value: PathIO.ReadTextAsync returns it. */
struct text_operation {
	text_operation_i iface;
	IAsyncInfo info;
	struct async_core core;
	HSTRING result;
};

static void text_operation_free(struct text_operation *self)
{
	if (self->result)
		WindowsDeleteString(self->result);
}

static const WCHAR text_operation_type_name[] = L"Windows.Foundation.IAsyncOperation`1<String>";

STORAGE_ASYNC_LIFETIME(text_operation, text_operation_i, &IID___FIAsyncOperation_1_HSTRING,
	__FIAsyncOperationCompletedHandler_1_HSTRING)
STORAGE_ASYNC_COMPLETED(text_operation, text_operation_i, __FIAsyncOperationCompletedHandler_1_HSTRING)
STORAGE_ASYNC_FINISH(text_operation, text_operation_i, __FIAsyncOperationCompletedHandler_1_HSTRING)

static HRESULT WINAPI text_operation_results(text_operation_i *iface, HSTRING *results)
{
	struct text_operation *self = (struct text_operation *)iface;
	HRESULT result;
	if (!results)
		return E_POINTER;
	*results = NULL;
	EnterCriticalSection(&self->core.lock);
	result = self->core.closed ? RO_E_CLOSED : self->core.status == Started ? E_ILLEGAL_METHOD_CALL : self->core.error;
	LeaveCriticalSection(&self->core.lock);
	if (SUCCEEDED(result) && self->result)
		WindowsDuplicateString(self->result, results);
	return result;
}

static const __FIAsyncOperation_1_HSTRINGVtbl text_operation_table = {
	.QueryInterface = text_operation_query,
	.AddRef = text_operation_addref,
	.Release = text_operation_release,
	.GetIids = text_operation_iids,
	.GetRuntimeClassName = text_operation_class,
	.GetTrustLevel = text_operation_trust,
	.put_Completed = text_operation_put_completed,
	.get_Completed = text_operation_get_completed,
	.GetResults = text_operation_results,
};

STORAGE_ASYNC_NEW(text_operation, text_operation_i)

/* The action: StorageFile.DeleteAsync returns it. IAsyncAction carries no result; GetResults answers the error the
 * dispatcher actions give, which is how the shape spells "the work is done and this is how it went". */
struct storage_action {
	async_action_i iface;
	IAsyncInfo info;
	struct async_core core;
};

static void storage_action_free(struct storage_action *self)
{
	(void)self;
}

static const WCHAR storage_action_type_name[] = L"Windows.Foundation.IAsyncAction";

STORAGE_ASYNC_LIFETIME(storage_action, async_action_i, &IID___x_ABI_CWindows_CFoundation_CIAsyncAction,
	__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler)
STORAGE_ASYNC_COMPLETED(storage_action, async_action_i, __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler)
STORAGE_ASYNC_FINISH(storage_action, async_action_i, __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler)

static HRESULT WINAPI storage_action_results(async_action_i *iface)
{
	struct storage_action *self = (struct storage_action *)iface;
	HRESULT result;
	EnterCriticalSection(&self->core.lock);
	result = self->core.closed ? RO_E_CLOSED : self->core.status == Started ? E_ILLEGAL_METHOD_CALL : self->core.error;
	LeaveCriticalSection(&self->core.lock);
	return result;
}

static const __x_ABI_CWindows_CFoundation_CIAsyncActionVtbl storage_action_table = {
	.QueryInterface = storage_action_query,
	.AddRef = storage_action_addref,
	.Release = storage_action_release,
	.GetIids = storage_action_iids,
	.GetRuntimeClassName = storage_action_class,
	.GetTrustLevel = storage_action_trust,
	.put_Completed = storage_action_put_completed,
	.get_Completed = storage_action_get_completed,
	.GetResults = storage_action_results,
};

STORAGE_ASYNC_NEW(storage_action, async_action_i)

typedef __x_ABI_CWindows_CStorage_CStreams_CIInputStream input_i;
typedef __FIAsyncOperation_1_Windows__CStorage__CStreams__CIInputStream input_operation_i;
typedef __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStreams__CIInputStream input_done_i;
typedef __FIAsyncOperationWithProgress_2_Windows__CStorage__CStreams__CIBuffer_UINT32 read_operation_i;
typedef __FIAsyncOperationWithProgressCompletedHandler_2_Windows__CStorage__CStreams__CIBuffer_UINT32 read_done_i;
typedef __FIAsyncOperationProgressHandler_2_Windows__CStorage__CStreams__CIBuffer_UINT32 progress_i;
typedef __x_ABI_CWindows_CStorage_CStreams_CIBuffer buffer_i;
typedef __x_ABI_CWindows_CFoundation_CIClosable IClosable;
typedef __x_ABI_CWindows_CFoundation_CIClosableVtbl IClosableVtbl;
#define IID_IClosable IID___x_ABI_CWindows_CFoundation_CIClosable
#define input_done_i_AddRef __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStreams__CIInputStream_AddRef
#define input_done_i_Release __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStreams__CIInputStream_Release
#define input_done_i_Invoke __FIAsyncOperationCompletedHandler_1_Windows__CStorage__CStreams__CIInputStream_Invoke
#define read_done_i_AddRef __FIAsyncOperationWithProgressCompletedHandler_2_Windows__CStorage__CStreams__CIBuffer_UINT32_AddRef
#define read_done_i_Release __FIAsyncOperationWithProgressCompletedHandler_2_Windows__CStorage__CStreams__CIBuffer_UINT32_Release
#define read_done_i_Invoke __FIAsyncOperationWithProgressCompletedHandler_2_Windows__CStorage__CStreams__CIBuffer_UINT32_Invoke

struct input_operation { input_operation_i iface; IAsyncInfo info; struct async_core core; input_i *result; };
static const WCHAR input_operation_type_name[]=L"Windows.Foundation.IAsyncOperation`1<Windows.Storage.Streams.IInputStream>";
static void input_operation_free(struct input_operation *self) { if (self->result) self->result->lpVtbl->Release(self->result); }
STORAGE_ASYNC_LIFETIME(input_operation,input_operation_i,&IID___FIAsyncOperation_1_Windows__CStorage__CStreams__CIInputStream,input_done_i)
STORAGE_ASYNC_COMPLETED(input_operation,input_operation_i,input_done_i)
STORAGE_ASYNC_FINISH(input_operation,input_operation_i,input_done_i)
static HRESULT WINAPI input_operation_results(input_operation_i *iface,input_i **out)
{
	struct input_operation *self=(void *)iface;
	if (!out) return E_POINTER; *out=NULL;
	if (self->core.closed) return RO_E_CLOSED;
	if (self->core.status==Started) return E_ILLEGAL_METHOD_CALL;
	if (FAILED(self->core.error)) return self->core.error;
	if ((*out=self->result)) (*out)->lpVtbl->AddRef(*out);
	return S_OK;
}
static const __FIAsyncOperation_1_Windows__CStorage__CStreams__CIInputStreamVtbl input_operation_table={input_operation_query,input_operation_addref,input_operation_release,input_operation_iids,input_operation_class,input_operation_trust,input_operation_put_completed,input_operation_get_completed,input_operation_results};
STORAGE_ASYNC_NEW(input_operation,input_operation_i)

struct read_operation { read_operation_i iface; IAsyncInfo info; struct async_core core; buffer_i *result; progress_i *progress; };
static const WCHAR read_operation_type_name[]=L"Windows.Foundation.IAsyncOperationWithProgress`2<Windows.Storage.Streams.IBuffer, UInt32>";
static void read_operation_free(struct read_operation *self)
{
	if (self->result) self->result->lpVtbl->Release(self->result);
	if (self->progress) self->progress->lpVtbl->Release(self->progress);
}
STORAGE_ASYNC_LIFETIME(read_operation,read_operation_i,&IID___FIAsyncOperationWithProgress_2_Windows__CStorage__CStreams__CIBuffer_UINT32,read_done_i)
STORAGE_ASYNC_COMPLETED(read_operation,read_operation_i,read_done_i)
STORAGE_ASYNC_FINISH(read_operation,read_operation_i,read_done_i)
static HRESULT WINAPI read_progress_put(read_operation_i *iface,progress_i *handler)
{
	struct read_operation *self=(void *)iface;
	if (handler) handler->lpVtbl->AddRef(handler);
	if (self->progress) self->progress->lpVtbl->Release(self->progress);
	self->progress=handler; return S_OK;
}
static HRESULT WINAPI read_progress_get(read_operation_i *iface,progress_i **out)
{
	struct read_operation *self=(void *)iface;
	if (!out) return E_POINTER;
	if ((*out=self->progress)) (*out)->lpVtbl->AddRef(*out);
	return S_OK;
}
static HRESULT WINAPI read_operation_results(read_operation_i *iface,buffer_i **out)
{
	struct read_operation *self=(void *)iface;
	if (!out) return E_POINTER; *out=NULL;
	if (self->core.closed) return RO_E_CLOSED;
	if (self->core.status==Started) return E_ILLEGAL_METHOD_CALL;
	if (FAILED(self->core.error)) return self->core.error;
	if ((*out=self->result)) (*out)->lpVtbl->AddRef(*out);
	return S_OK;
}
static const __FIAsyncOperationWithProgress_2_Windows__CStorage__CStreams__CIBuffer_UINT32Vtbl read_operation_table={read_operation_query,read_operation_addref,read_operation_release,read_operation_iids,read_operation_class,read_operation_trust,read_progress_put,read_progress_get,read_operation_put_completed,read_operation_get_completed,read_operation_results};
STORAGE_ASYNC_NEW(read_operation,read_operation_i)

struct file_input { input_i iface; IClosable closable; LONG refs; HANDLE handle; };
static HRESULT WINAPI input_query(input_i *iface,REFIID iid,void **out)
{
	struct file_input *self=(void *)iface;
	if (!out) return E_POINTER; *out=NULL;
	if (equal_iid(iid,&IID_IUnknown) || equal_iid(iid,&IID_IInspectable) || equal_iid(iid,&IID_IAgileObject) || equal_iid(iid,&IID___x_ABI_CWindows_CStorage_CStreams_CIInputStream)) *out=iface;
	else if (equal_iid(iid,&IID_IClosable)) *out=&self->closable;
	else return E_NOINTERFACE;
	InterlockedIncrement(&self->refs); return S_OK;
}
static ULONG WINAPI input_addref(input_i *iface) { return InterlockedIncrement(&((struct file_input *)iface)->refs); }
static ULONG WINAPI input_release(input_i *iface)
{
	struct file_input *self=(void *)iface; LONG refs=InterlockedDecrement(&self->refs);
	if (!refs) { if (self->handle!=INVALID_HANDLE_VALUE) CloseHandle(self->handle); FREE(self); InterlockedDecrement(&raven_xaml_objects); }
	return refs;
}
static HRESULT WINAPI input_iids(input_i *iface,ULONG *count,IID **out) { (void)iface; return inspectable_iids(count,out); }
static HRESULT WINAPI input_name(input_i *iface,HSTRING *out) { (void)iface; return create_string(L"Windows.Storage.Streams.IInputStream",out); }
static HRESULT WINAPI input_trust(input_i *iface,TrustLevel *out) { (void)iface; return inspectable_trust(out); }
static HRESULT WINAPI input_read(input_i *iface,buffer_i *buffer,UINT32 count,__x_ABI_CWindows_CStorage_CStreams_CInputStreamOptions options,read_operation_i **out)
{
	struct file_input *self=(void *)iface; struct read_operation *operation; __x_Windows_CStorage_CStreams_CIBufferByteAccess *access=NULL;
	UINT32 capacity=0; BYTE *bytes=NULL; DWORD taken=0; HRESULT result;
	(void)options;
	if (!out) return E_POINTER; *out=NULL;
	if (!buffer) { raven_trace_note("raven-storage: null buffer\n"); return E_INVALIDARG; }
	if (self->handle==INVALID_HANDLE_VALUE) return RO_E_CLOSED;
	result=buffer->lpVtbl->get_Capacity(buffer,&capacity);
	if (FAILED(result)) return result;
	if (count>capacity) { raven_trace_note("raven-storage: count over capacity\n"); return E_INVALIDARG; }
	result=buffer->lpVtbl->QueryInterface(buffer,&IID___x_Windows_CStorage_CStreams_CIBufferByteAccess,(void **)&access);
	if (FAILED(result)) return result;
	result=access->lpVtbl->Buffer(access,&bytes);
	if (SUCCEEDED(result) && !ReadFile(self->handle,bytes,count,&taken,NULL)) result=last_error();
	access->lpVtbl->Release(access);
	if (SUCCEEDED(result)) result=buffer->lpVtbl->put_Length(buffer,taken);
	if (!(operation=read_operation_new())) return E_OUTOFMEMORY;
	if (SUCCEEDED(result)) { operation->result=buffer; buffer->lpVtbl->AddRef(buffer); }
	read_operation_finish(&operation->iface,result,SUCCEEDED(result)?Completed:Error);
	*out=&operation->iface; return S_OK;
}
static const __x_ABI_CWindows_CStorage_CStreams_CIInputStreamVtbl input_table={input_query,input_addref,input_release,input_iids,input_name,input_trust,input_read};
static struct file_input *input_owner(IClosable *iface) { return CONTAINING_RECORD(iface,struct file_input,closable); }
static HRESULT WINAPI input_close_query(IClosable *iface,REFIID iid,void **out) { return input_query(&input_owner(iface)->iface,iid,out); }
static ULONG WINAPI input_close_addref(IClosable *iface) { return input_addref(&input_owner(iface)->iface); }
static ULONG WINAPI input_close_release(IClosable *iface) { return input_release(&input_owner(iface)->iface); }
static HRESULT WINAPI input_close_iids(IClosable *iface,ULONG *count,IID **out) { (void)iface; return inspectable_iids(count,out); }
static HRESULT WINAPI input_close_name(IClosable *iface,HSTRING *out) { (void)iface; return create_string(L"Windows.Storage.Streams.IInputStream",out); }
static HRESULT WINAPI input_close_trust(IClosable *iface,TrustLevel *out) { (void)iface; return inspectable_trust(out); }
static HRESULT WINAPI input_close(IClosable *iface) { struct file_input *self=input_owner(iface); if (self->handle!=INVALID_HANDLE_VALUE) { CloseHandle(self->handle); self->handle=INVALID_HANDLE_VALUE; } return S_OK; }
static const IClosableVtbl input_close_table={input_close_query,input_close_addref,input_close_release,input_close_iids,input_close_name,input_close_trust,input_close};

/* The StorageFile: a name, the full path and the extension, bound to nothing but the file system itself. */
#define FILE_NAME_MAX 96

struct file_object {
	file_i iface;
	item_i item;
	__x_ABI_CWindows_CStorage_CStreams_CIInputStreamReference input_reference;
	LONG refs;
	WCHAR name[FILE_NAME_MAX];
	WCHAR path[MAX_PATH];
	WCHAR type[FILE_NAME_MAX]; /* the extension with the dot, empty when the name carries none */
	BOOL read_only;
};

static HRESULT WINAPI file_query(file_i *iface, REFIID iid, void **out);
static ULONG WINAPI file_addref(file_i *iface);
static ULONG WINAPI file_release(file_i *iface);

typedef __x_ABI_CWindows_CStorage_CStreams_CIInputStreamReference input_reference_i;
static struct file_object *file_reference_owner(input_reference_i *iface) { return CONTAINING_RECORD(iface,struct file_object,input_reference); }
static HRESULT WINAPI file_reference_query(input_reference_i *iface,REFIID iid,void **out) { return file_query(&file_reference_owner(iface)->iface,iid,out); }
static ULONG WINAPI file_reference_addref(input_reference_i *iface) { return file_addref(&file_reference_owner(iface)->iface); }
static ULONG WINAPI file_reference_release(input_reference_i *iface) { return file_release(&file_reference_owner(iface)->iface); }
static HRESULT WINAPI file_reference_iids(input_reference_i *iface,ULONG *count,IID **out) { (void)iface; return inspectable_iids(count,out); }
static HRESULT WINAPI file_reference_name(input_reference_i *iface,HSTRING *out) { (void)iface; return create_string(file_class_name,out); }
static HRESULT WINAPI file_reference_trust(input_reference_i *iface,TrustLevel *out) { (void)iface; return inspectable_trust(out); }
static HRESULT WINAPI file_reference_open(input_reference_i *iface,input_operation_i **out)
{
	struct file_object *file=file_reference_owner(iface); struct input_operation *operation; struct file_input *stream;
	HANDLE handle; HRESULT result;
	if (!out) return E_POINTER; *out=NULL;
	handle=CreateFileW(file->path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	result=handle==INVALID_HANDLE_VALUE ? last_error() : S_OK;
	if (!(operation=input_operation_new())) { if (handle!=INVALID_HANDLE_VALUE) CloseHandle(handle); return E_OUTOFMEMORY; }
	if (SUCCEEDED(result)) {
		stream=ALLOC(sizeof *stream);
		if (!stream) { CloseHandle(handle); result=E_OUTOFMEMORY; }
		else { stream->iface.lpVtbl=&input_table; stream->closable.lpVtbl=&input_close_table; stream->refs=1; stream->handle=handle; operation->result=&stream->iface; InterlockedIncrement(&raven_xaml_objects); }
	}
	input_operation_finish(&operation->iface,result,SUCCEEDED(result)?Completed:Error);
	*out=&operation->iface; return S_OK;
}
static const __x_ABI_CWindows_CStorage_CStreams_CIInputStreamReferenceVtbl file_reference_table={file_reference_query,file_reference_addref,file_reference_release,file_reference_iids,file_reference_name,file_reference_trust,file_reference_open};

/* The item part forwards its lifetime to the object, found the way the dispatcher's IAsyncInfo finds its action. */
static struct file_object *file_item_owner(item_i *iface)
{
	return CONTAINING_RECORD(iface, struct file_object, item);
}

static HRESULT WINAPI file_item_query(item_i *iface, REFIID iid, void **out)
{
	return file_query(&file_item_owner(iface)->iface, iid, out);
}

static ULONG WINAPI file_item_addref(item_i *iface)
{
	return file_addref(&file_item_owner(iface)->iface);
}

static ULONG WINAPI file_item_release(item_i *iface)
{
	return file_release(&file_item_owner(iface)->iface);
}

static HRESULT WINAPI file_iids(file_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI file_class(file_i *iface, HSTRING *out)
{
	(void)iface;
	return create_string(file_class_name, out);
}

static HRESULT WINAPI file_trust(file_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI file_item_iids(item_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI file_item_class(item_i *iface, HSTRING *out)
{
	(void)iface;
	return create_string(file_class_name, out);
}

static HRESULT WINAPI file_item_trust(item_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI file_get_file_type(file_i *iface, HSTRING *value)
{
	struct file_object *self = (struct file_object *)iface;
	return create_string(self->type, value);
}

/* The content type needs the mime table; no observed startup path has asked for it yet. */
static HRESULT WINAPI file_get_content_type(file_i *iface, HSTRING *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

/* The random access stream is the storage streams stage's work; the file itself is already openable through PathIO. */
static HRESULT WINAPI file_open_async(file_i *iface, __x_ABI_CWindows_CStorage_CFileAccessMode mode,
	__FIAsyncOperation_1_Windows__CStorage__CStreams__CIRandomAccessStream **operation)
{
	(void)iface;
	(void)mode;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI file_open_transacted_write_async(file_i *iface,
	__FIAsyncOperation_1_Windows__CStorage__CStorageStreamTransaction **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI file_copy_overload_default(file_i *iface, folder_i *folder, file_operation_i **operation)
{
	(void)iface;
	(void)folder;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI file_copy_overload_options(file_i *iface, folder_i *folder, HSTRING name,
	file_operation_i **operation)
{
	(void)iface;
	(void)folder;
	(void)name;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI file_copy_overload(file_i *iface, folder_i *folder, HSTRING name,
	__x_ABI_CWindows_CStorage_CNameCollisionOption option, file_operation_i **operation)
{
	(void)iface;
	(void)folder;
	(void)name;
	(void)option;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI file_copy_and_replace_async(file_i *iface, file_i *file, async_action_i **operation)
{
	(void)iface;
	(void)file;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI file_move_overload_default(file_i *iface, folder_i *folder, async_action_i **operation)
{
	(void)iface;
	(void)folder;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI file_move_overload_options(file_i *iface, folder_i *folder, HSTRING name,
	async_action_i **operation)
{
	(void)iface;
	(void)folder;
	(void)name;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI file_move_overload(file_i *iface, folder_i *folder, HSTRING name,
	__x_ABI_CWindows_CStorage_CNameCollisionOption option, async_action_i **operation)
{
	(void)iface;
	(void)folder;
	(void)name;
	(void)option;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI file_move_and_replace_async(file_i *iface, file_i *file, async_action_i **operation)
{
	(void)iface;
	(void)file;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

/* The delete: real, for both option values — the emulated filesystem has no recycle bin, so PermanentDelete and the
 * default send the file to the same place. The work runs inside the call; the action only reports it. */
static HRESULT WINAPI file_delete_with_option(file_i *iface, __x_ABI_CWindows_CStorage_CStorageDeleteOption option,
	async_action_i **operation)
{
	struct file_object *self = (struct file_object *)iface;
	struct storage_action *action;
	HRESULT result = S_OK;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	(void)option;
	if (!(action = storage_action_new()))
		return E_OUTOFMEMORY;
	if (self->read_only || !DeleteFileW(self->path)) {
		result = self->read_only ? E_ACCESSDENIED : last_error();
		storage_action_finish(&action->iface, result, Error);
	} else {
		storage_action_finish(&action->iface, S_OK, Completed);
	}
	*operation = &action->iface;
	return S_OK;
}

static HRESULT WINAPI file_item_delete_overload(item_i *iface, async_action_i **operation)
{
	return file_delete_with_option(&file_item_owner(iface)->iface, StorageDeleteOption_Default, operation);
}

static HRESULT WINAPI file_item_delete(item_i *iface, __x_ABI_CWindows_CStorage_CStorageDeleteOption option,
	async_action_i **operation)
{
	return file_delete_with_option(&file_item_owner(iface)->iface, option, operation);
}

static HRESULT WINAPI file_item_rename_overload(item_i *iface, HSTRING name, async_action_i **operation)
{
	(void)iface;
	(void)name;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI file_item_rename(item_i *iface, HSTRING name,
	__x_ABI_CWindows_CStorage_CNameCollisionOption option, async_action_i **operation)
{
	(void)iface;
	(void)name;
	(void)option;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI file_item_get_basic_properties(item_i *iface,
	__FIAsyncOperation_1_Windows__CStorage__CFileProperties__CBasicProperties **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

/* The item interface is the second part of the object, so the owner comes from the containing record, not the cast. */
static HRESULT WINAPI file_item_get_name(item_i *iface, HSTRING *value)
{
	struct file_object *self = file_item_owner(iface);
	return create_string(self->name, value);
}

static HRESULT WINAPI file_item_get_path(item_i *iface, HSTRING *value)
{
	struct file_object *self = file_item_owner(iface);
	return create_string(self->path, value);
}

static HRESULT WINAPI file_item_get_attributes(item_i *iface, __x_ABI_CWindows_CStorage_CFileAttributes *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI file_item_get_date_created(item_i *iface, __x_ABI_CWindows_CFoundation_CDateTime *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	ZeroMemory(value, sizeof *value);
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI file_item_is_of_type(item_i *iface, __x_ABI_CWindows_CStorage_CStorageItemTypes type,
	boolean *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	(void)type;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static const __x_ABI_CWindows_CStorage_CIStorageFileVtbl file_table = {
	.QueryInterface = file_query,
	.AddRef = file_addref,
	.Release = file_release,
	.GetIids = file_iids,
	.GetRuntimeClassName = file_class,
	.GetTrustLevel = file_trust,
	.get_FileType = file_get_file_type,
	.get_ContentType = file_get_content_type,
	.OpenAsync = file_open_async,
	.OpenTransactedWriteAsync = file_open_transacted_write_async,
	.CopyOverloadDefaultNameAndOptions = file_copy_overload_default,
	.CopyOverloadDefaultOptions = file_copy_overload_options,
	.CopyOverload = file_copy_overload,
	.CopyAndReplaceAsync = file_copy_and_replace_async,
	.MoveOverloadDefaultNameAndOptions = file_move_overload_default,
	.MoveOverloadDefaultOptions = file_move_overload_options,
	.MoveOverload = file_move_overload,
	.MoveAndReplaceAsync = file_move_and_replace_async,
};

static const __x_ABI_CWindows_CStorage_CIStorageItemVtbl file_item_table = {
	.QueryInterface = file_item_query,
	.AddRef = file_item_addref,
	.Release = file_item_release,
	.GetIids = file_item_iids,
	.GetRuntimeClassName = file_item_class,
	.GetTrustLevel = file_item_trust,
	.RenameAsyncOverloadDefaultOptions = file_item_rename_overload,
	.RenameAsync = file_item_rename,
	.DeleteAsyncOverloadDefaultOptions = file_item_delete_overload,
	.DeleteAsync = file_item_delete,
	.GetBasicPropertiesAsync = file_item_get_basic_properties,
	.get_Name = file_item_get_name,
	.get_Path = file_item_get_path,
	.get_Attributes = file_item_get_attributes,
	.get_DateCreated = file_item_get_date_created,
	.IsOfType = file_item_is_of_type,
};

static HRESULT WINAPI file_query(file_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CStorage_CIStorageFile) || equal_iid(iid, &IID_IAgileObject))
		*out = iface;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CStorage_CIStorageItem))
		*out = &((struct file_object *)iface)->item;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CStorage_CStreams_CIInputStreamReference))
		*out = &((struct file_object *)iface)->input_reference;
	else
		return E_NOINTERFACE;
	file_addref(iface);
	return S_OK;
}

static ULONG WINAPI file_addref(file_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct file_object *)iface)->refs);
}

static ULONG WINAPI file_release(file_i *iface)
{
	struct file_object *self = (struct file_object *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

/* A file object for a path under the folder: the name is the last path component, the type the extension from the
 * last dot in the name, dot included, as the documented FileType answers for a file without one too. */
static HRESULT file_create(const WCHAR *path, BOOL read_only, file_i **out)
{
	struct file_object *self;
	const WCHAR *name_start = path;
	const WCHAR *dot = NULL;
	const WCHAR *walk;
	if (!out)
		return E_POINTER;
	*out = NULL;
	for (walk = path; *walk; ++walk) {
		if (*walk == L'\\' || *walk == L'/') {
			name_start = walk + 1;
			dot = NULL;
		} else if (*walk == L'.')
			dot = walk;
	}
	if ((SIZE_T)(walk - name_start) >= FILE_NAME_MAX || (SIZE_T)(dot ? walk - dot : 0) >= FILE_NAME_MAX)
		{ raven_trace_note("raven-storage: E_INVALIDARG at storage.c:1118\n"); return E_INVALIDARG; }
	if (!(self = ALLOC(sizeof *self)))
		return E_OUTOFMEMORY;
	self->iface.lpVtbl = (void *)&file_table;
	self->item.lpVtbl = (void *)&file_item_table;
	self->input_reference.lpVtbl = &file_reference_table;
	self->refs = 1;
	self->read_only = read_only;
	lstrcpyW(self->name, name_start);
	lstrcpyW(self->path, path);
	if (dot)
		lstrcpyW(self->type, dot);
	InterlockedIncrement(&raven_xaml_objects);
	*out = &self->iface;
	return S_OK;
}

/* A completed operation for a file: the result of work that already ran, or the error it failed with. */
static HRESULT file_operation_completed(file_i *file, HRESULT error, file_operation_i **out)
{
	struct file_operation *operation;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!(operation = file_operation_new()))
		return E_OUTOFMEMORY;
	if (SUCCEEDED(error)) {
		operation->result = file;
		__x_ABI_CWindows_CStorage_CIStorageFile_AddRef(file);
		file_operation_finish(&operation->iface, S_OK, Completed);
	} else {
		file_operation_finish(&operation->iface, error, Error);
	}
	*out = &operation->iface;
	return S_OK;
}

/* The name a CreateFileAsync or GetFileAsync accepts: a single component, neither empty nor a relative step, that
 * fits the folder's path. Windows rejects a wider set of characters; the ones that would change what the call names
 * are the ones refused here. */
static HRESULT validate_item_name(HSTRING name, UINT32 *length)
{
	const WCHAR *text = WindowsGetStringRawBuffer(name, length);
	UINT32 i;
	if (!*length)
		{ raven_trace_note("raven-storage: E_INVALIDARG at storage.c:1163\n"); return E_INVALIDARG; }
	if (*length >= FILE_NAME_MAX)
		return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
	for (i = 0; i < *length; ++i)
		if (text[i] == L'\\' || text[i] == L'/' || text[i] == L':')
			{ raven_trace_note("raven-storage: E_INVALIDARG at storage.c:1168\n"); return E_INVALIDARG; }
	if ((*length == 1 && text[0] == L'.') || (*length == 2 && text[0] == L'.' && text[1] == L'.'))
		{ raven_trace_note("raven-storage: E_INVALIDARG at storage.c:1170\n"); return E_INVALIDARG; }
	return S_OK;
}

/* The StorageFolder: the LocalState directory, with the name and path it is bound to. */
struct folder_object {
	folder_i iface;
	item_i item;
	LONG refs;
	WCHAR name[FILE_NAME_MAX];
	WCHAR path[MAX_PATH];
	BOOL read_only;
};

static HRESULT WINAPI folder_query(folder_i *iface, REFIID iid, void **out);
static ULONG WINAPI folder_addref(folder_i *iface);
static ULONG WINAPI folder_release(folder_i *iface);

static struct folder_object *folder_item_owner(item_i *iface)
{
	return CONTAINING_RECORD(iface, struct folder_object, item);
}

static HRESULT WINAPI folder_item_query(item_i *iface, REFIID iid, void **out)
{
	return folder_query(&folder_item_owner(iface)->iface, iid, out);
}

static ULONG WINAPI folder_item_addref(item_i *iface)
{
	return folder_addref(&folder_item_owner(iface)->iface);
}

static ULONG WINAPI folder_item_release(item_i *iface)
{
	return folder_release(&folder_item_owner(iface)->iface);
}

static HRESULT WINAPI folder_iids(folder_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI folder_class(folder_i *iface, HSTRING *out)
{
	(void)iface;
	return create_string(folder_class_name, out);
}

static HRESULT WINAPI folder_trust(folder_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI folder_item_iids(item_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI folder_item_class(item_i *iface, HSTRING *out)
{
	(void)iface;
	return create_string(folder_class_name, out);
}

static HRESULT WINAPI folder_item_trust(item_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

/* As with the file, the item part sits after the folder part, so the owner comes from the containing record. */
static HRESULT WINAPI folder_item_get_name(item_i *iface, HSTRING *value)
{
	struct folder_object *self = folder_item_owner(iface);
	return create_string(self->name, value);
}

static HRESULT WINAPI folder_item_get_path(item_i *iface, HSTRING *value)
{
	struct folder_object *self = folder_item_owner(iface);
	return create_string(self->path, value);
}

/* A folder's rename, delete and properties are outside the subset; only the file's delete is real. */
static HRESULT WINAPI folder_item_rename_overload(item_i *iface, HSTRING name, async_action_i **operation)
{
	(void)iface;
	(void)name;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI folder_item_rename(item_i *iface, HSTRING name,
	__x_ABI_CWindows_CStorage_CNameCollisionOption option, async_action_i **operation)
{
	(void)iface;
	(void)name;
	(void)option;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI folder_item_delete(item_i *iface, __x_ABI_CWindows_CStorage_CStorageDeleteOption option,
	async_action_i **operation)
{
	(void)iface;
	(void)option;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI folder_item_delete_overload(item_i *iface, async_action_i **operation)
{
	return folder_item_delete(iface, StorageDeleteOption_Default, operation);
}

static HRESULT WINAPI folder_item_get_basic_properties(item_i *iface,
	__FIAsyncOperation_1_Windows__CStorage__CFileProperties__CBasicProperties **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI folder_item_get_attributes(item_i *iface, __x_ABI_CWindows_CStorage_CFileAttributes *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI folder_item_get_date_created(item_i *iface, __x_ABI_CWindows_CFoundation_CDateTime *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	ZeroMemory(value, sizeof *value);
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI folder_item_is_of_type(item_i *iface, __x_ABI_CWindows_CStorage_CStorageItemTypes type,
	boolean *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	(void)type;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

/* CreateFileAsync: an empty file, created or replaced per the collision option, and the operation that already
 * carries it. The work runs inside the call, so the returned operation is terminal. */
static HRESULT WINAPI folder_create_file(folder_i *iface, HSTRING name,
	__x_ABI_CWindows_CStorage_CCreationCollisionOption option, file_operation_i **operation)
{
	struct folder_object *self = (struct folder_object *)iface;
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	WCHAR path[MAX_PATH];
	DWORD disposition;
	HANDLE handle;
	file_i *file = NULL;
	HRESULT result;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	result = validate_item_name(name, &length);
	if (FAILED(result))
		return result;
	if (self->read_only) return file_operation_completed(NULL, E_ACCESSDENIED, operation);
	if ((UINT32)lstrlenW(self->path) + 1 + length + 1 >= MAX_PATH)
		return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
	switch (option) {
	case CreationCollisionOption_ReplaceExisting:
		disposition = CREATE_ALWAYS;
		break;
	case CreationCollisionOption_FailIfExists:
		disposition = CREATE_NEW;
		break;
	default:
		/* GenerateUniqueName and OpenIfExists would invent or reopen; neither is the subset's to promise */
		{ trace_not_implemented(__func__); return E_NOTIMPL; }
	}
	lstrcpyW(path, self->path);
	lstrcatW(path, L"\\");
	lstrcatW(path, text);
	handle = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
		/* the async contract: the call succeeds and the operation carries the failure, the way GetFileAsync does */
		result = last_error();
		return file_operation_completed(NULL, result, operation);
	}
	CloseHandle(handle);
	result = file_create(path, self->read_only, &file);
	if (FAILED(result))
		return result;
	result = file_operation_completed(file, S_OK, operation);
	__x_ABI_CWindows_CStorage_CIStorageFile_Release(file);
	return result;
}

/* the documented default of the single-argument overload: creating fails on an existing file */
static HRESULT WINAPI folder_create_file_overload(folder_i *iface, HSTRING name, file_operation_i **operation)
{
	return folder_create_file(iface, name, CreationCollisionOption_FailIfExists, operation);
}

static HRESULT WINAPI folder_create_folder_overload(folder_i *iface, HSTRING name,
	__FIAsyncOperation_1_Windows__CStorage__CStorageFolder **operation)
{
	(void)iface;
	(void)name;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI folder_create_folder(folder_i *iface, HSTRING name,
	__x_ABI_CWindows_CStorage_CCreationCollisionOption option,
	__FIAsyncOperation_1_Windows__CStorage__CStorageFolder **operation)
{
	(void)iface;
	(void)name;
	(void)option;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

/* GetFileAsync: the operation completes with the file when the directory has it, and with
 * HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) — the answer Windows gives for a missing item — when it does not. A
 * directory of that name is no file. */
static HRESULT WINAPI folder_get_file_async(folder_i *iface, HSTRING name, file_operation_i **operation)
{
	struct folder_object *self = (struct folder_object *)iface;
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	WCHAR path[MAX_PATH];
	DWORD attributes;
	file_i *file = NULL;
	HRESULT result;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	result = validate_item_name(name, &length);
	if (FAILED(result))
		return result;
	if ((UINT32)lstrlenW(self->path) + 1 + length + 1 >= MAX_PATH)
		return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
	lstrcpyW(path, self->path);
	lstrcatW(path, L"\\");
	lstrcatW(path, text);
	attributes = GetFileAttributesW(path);
	if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY))
		return file_operation_completed(NULL, last_error(), operation);
	result = file_create(path, self->read_only, &file);
	if (FAILED(result))
		return result;
	result = file_operation_completed(file, S_OK, operation);
	__x_ABI_CWindows_CStorage_CIStorageFile_Release(file);
	return result;
}

static HRESULT WINAPI folder_get_folder_async(folder_i *iface, HSTRING name,
	__FIAsyncOperation_1_Windows__CStorage__CStorageFolder **operation)
{
	(void)iface;
	(void)name;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI folder_get_item_async(folder_i *iface, HSTRING name,
	__FIAsyncOperation_1_Windows__CStorage__CIStorageItem **operation)
{
	(void)iface;
	(void)name;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI folder_get_files_async(folder_i *iface,
	__FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI folder_get_folders_async(folder_i *iface,
	__FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFolder **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI folder_get_items_async(folder_i *iface,
	__FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CIStorageItem **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static const __x_ABI_CWindows_CStorage_CIStorageFolderVtbl folder_table = {
	.QueryInterface = folder_query,
	.AddRef = folder_addref,
	.Release = folder_release,
	.GetIids = folder_iids,
	.GetRuntimeClassName = folder_class,
	.GetTrustLevel = folder_trust,
	.CreateFileAsyncOverloadDefaultOptions = folder_create_file_overload,
	.CreateFileAsync = folder_create_file,
	.CreateFolderAsyncOverloadDefaultOptions = folder_create_folder_overload,
	.CreateFolderAsync = folder_create_folder,
	.GetFileAsync = folder_get_file_async,
	.GetFolderAsync = folder_get_folder_async,
	.GetItemAsync = folder_get_item_async,
	.GetFilesAsyncOverloadDefaultOptionsStartAndCount = folder_get_files_async,
	.GetFoldersAsyncOverloadDefaultOptionsStartAndCount = folder_get_folders_async,
	.GetItemsAsyncOverloadDefaultStartAndCount = folder_get_items_async,
};

static const __x_ABI_CWindows_CStorage_CIStorageItemVtbl folder_item_table = {
	.QueryInterface = folder_item_query,
	.AddRef = folder_item_addref,
	.Release = folder_item_release,
	.GetIids = folder_item_iids,
	.GetRuntimeClassName = folder_item_class,
	.GetTrustLevel = folder_item_trust,
	.RenameAsyncOverloadDefaultOptions = folder_item_rename_overload,
	.RenameAsync = folder_item_rename,
	.DeleteAsyncOverloadDefaultOptions = folder_item_delete_overload,
	.DeleteAsync = folder_item_delete,
	.GetBasicPropertiesAsync = folder_item_get_basic_properties,
	.get_Name = folder_item_get_name,
	.get_Path = folder_item_get_path,
	.get_Attributes = folder_item_get_attributes,
	.get_DateCreated = folder_item_get_date_created,
	.IsOfType = folder_item_is_of_type,
};

static HRESULT WINAPI folder_query(folder_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CStorage_CIStorageFolder) || equal_iid(iid, &IID_IAgileObject))
		*out = iface;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CStorage_CIStorageItem))
		*out = &((struct folder_object *)iface)->item;
	else
		return E_NOINTERFACE;
	folder_addref(iface);
	return S_OK;
}

static ULONG WINAPI folder_addref(folder_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct folder_object *)iface)->refs);
}

/* The folder belongs to the application data singleton and lives as long as the process, the way the package
 * singleton does; the count follows the guests' references and the object is never freed. */
static ULONG WINAPI folder_release(folder_i *iface)
{
	return (ULONG)InterlockedDecrement(&((struct folder_object *)iface)->refs);
}

/* The package's installed folder shares the folder implementation but is distinct from writable local state. */
HRESULT raven_storage_installed_folder(const WCHAR *path, folder_i **out)
{
	static struct folder_object folder = { .iface = {(void *)&folder_table}, .item = {(void *)&folder_item_table}, .read_only = TRUE };
	static LONG state;
	static HRESULT result;
	if (!path || !out) return E_POINTER;
	*out = NULL;
	if (InterlockedCompareExchange(&state, 1, 0) == 0) {
		const WCHAR *name = path;
		for (const WCHAR *p = path; *p; ++p) if (*p == L'\\' || *p == L'/') name = p + 1;
		DWORD attributes = GetFileAttributesW(path);
		if (attributes == INVALID_FILE_ATTRIBUTES) result = last_error();
		else if (!(attributes & FILE_ATTRIBUTE_DIRECTORY)) result = HRESULT_FROM_WIN32(ERROR_DIRECTORY);
		else if (lstrlenW(path) >= MAX_PATH || lstrlenW(name) >= FILE_NAME_MAX) result = E_INVALIDARG;
		else { lstrcpyW(folder.path, path); lstrcpyW(folder.name, name); result = S_OK; }
		InterlockedExchange(&state, 2);
	} else while (InterlockedCompareExchange(&state, 2, 2) != 2) Sleep(0);
	if (FAILED(result)) return result;
	folder_addref(&folder.iface);
	*out = &folder.iface;
	return S_OK;
}

/* The ApplicationData: one per process, the way Package.Current is. The LocalFolder is part of the singleton and
 * lives as long as the process; its directory is resolved and created the first time a guest asks for it. */
struct application_data_object {
	application_data_i iface;
	__x_ABI_CWindows_CStorage_CIApplicationData2 cache_iface;
	LONG refs;
	struct folder_object local_folder;
	struct folder_object temporary_folder;
	struct folder_object cache_folder;
	LONG temporary_state;
	HRESULT temporary_error;
	LONG cache_state;
	HRESULT cache_error;
	/* the once-per-process resolution: 0 idle, 1 running, 2 settled (the error stays for the later askers) */
	LONG local_state;
	HRESULT local_error;
};

static HRESULT WINAPI application_data_query(application_data_i *iface, REFIID iid, void **out);
static ULONG WINAPI application_data_addref(application_data_i *iface);
static ULONG WINAPI application_data_release(application_data_i *iface);

static HRESULT WINAPI application_data_get_version(application_data_i *iface, UINT32 *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI application_data_set_version_async(application_data_i *iface, UINT32 version,
	__x_ABI_CWindows_CStorage_CIApplicationDataSetVersionHandler *handler, async_action_i **operation)
{
	(void)iface;
	(void)version;
	(void)handler;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI application_data_clear_all_async(application_data_i *iface, async_action_i **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI application_data_clear_async(application_data_i *iface,
	__x_ABI_CWindows_CStorage_CApplicationDataLocality locality, async_action_i **operation)
{
	(void)iface;
	(void)locality;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI application_data_get_local_settings(application_data_i *iface,
	__x_ABI_CWindows_CStorage_CIApplicationDataContainer **value)
{
	(void)iface;
	return storage_local_settings(value);
}

static HRESULT WINAPI application_data_get_roaming_settings(application_data_i *iface,
	__x_ABI_CWindows_CStorage_CIApplicationDataContainer **value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

/* The LocalState directory beside the image, created on demand. The folder answers from the singleton, so repeated
 * asks hand out the same object. */
static HRESULT WINAPI application_data_get_local_folder(application_data_i *iface, folder_i **value)
{
	struct application_data_object *self = (struct application_data_object *)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (InterlockedCompareExchange(&self->local_state, 1, 0) == 0) {
		WCHAR path[MAX_PATH];
		if (ensure_application_folder(path, local_state_name)) {
			lstrcpyW(self->local_folder.name, local_state_name);
			lstrcpyW(self->local_folder.path, path);
			self->local_error = S_OK;
		} else {
			/* the failure stays: a directory that cannot be made will not be made by asking again */
			self->local_error = last_error();
		}
		InterlockedExchange(&self->local_state, 2);
	} else {
		while (InterlockedCompareExchange(&self->local_state, 2, 2) != 2)
			Sleep(0);
	}
	if (FAILED(self->local_error))
		return self->local_error;
	folder_addref(&self->local_folder.iface);
	*value = &self->local_folder.iface;
	return S_OK;
}

static HRESULT WINAPI application_data_get_roaming_folder(application_data_i *iface, folder_i **value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

/* The Temp directory of the storage root, created on demand and answered from the singleton. */
static HRESULT WINAPI application_data_get_temporary_folder(application_data_i *iface, folder_i **value)
{
	struct application_data_object *self = (struct application_data_object *)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (InterlockedCompareExchange(&self->temporary_state, 1, 0) == 0) {
		WCHAR path[MAX_PATH];
		if (ensure_application_folder(path, L"Temp")) {
			lstrcpyW(self->temporary_folder.name, L"Temp");
			lstrcpyW(self->temporary_folder.path, path);
			self->temporary_error = S_OK;
		} else {
			/* the failure stays: a directory that cannot be made will not be made by asking again */
			self->temporary_error = last_error();
		}
		InterlockedExchange(&self->temporary_state, 2);
	} else {
		while (InterlockedCompareExchange(&self->temporary_state, 2, 2) != 2)
			Sleep(0);
	}
	if (FAILED(self->temporary_error))
		return self->temporary_error;
	folder_addref(&self->temporary_folder.iface);
	*value = &self->temporary_folder.iface;
	return S_OK;
}

static HRESULT WINAPI application_data_add_data_changed(application_data_i *iface,
	__FITypedEventHandler_2_Windows__CStorage__CApplicationData_IInspectable *handler, EventRegistrationToken *token)
{
	(void)iface;
	(void)handler;
	if (!token)
		return E_POINTER;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI application_data_remove_data_changed(application_data_i *iface, EventRegistrationToken token)
{
	(void)iface;
	(void)token;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI application_data_signal_data_changed(application_data_i *iface)
{
	(void)iface;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI application_data_get_roaming_storage_quota(application_data_i *iface, UINT64 *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI application_data_iids(application_data_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI application_data_class(application_data_i *iface, HSTRING *out)
{
	(void)iface;
	return create_string(application_data_class_name, out);
}

static HRESULT WINAPI application_data_trust(application_data_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static struct application_data_object *cache_owner(__x_ABI_CWindows_CStorage_CIApplicationData2 *iface)
{
	return CONTAINING_RECORD(iface, struct application_data_object, cache_iface);
}
static HRESULT WINAPI cache_query(__x_ABI_CWindows_CStorage_CIApplicationData2 *iface, REFIID iid, void **out)
{ return application_data_query(&cache_owner(iface)->iface, iid, out); }
static ULONG WINAPI cache_addref(__x_ABI_CWindows_CStorage_CIApplicationData2 *iface)
{ return application_data_addref(&cache_owner(iface)->iface); }
static ULONG WINAPI cache_release(__x_ABI_CWindows_CStorage_CIApplicationData2 *iface)
{ return application_data_release(&cache_owner(iface)->iface); }
static HRESULT WINAPI cache_iids(__x_ABI_CWindows_CStorage_CIApplicationData2 *iface, ULONG *count, IID **iids)
{ return application_data_iids(&cache_owner(iface)->iface, count, iids); }
static HRESULT WINAPI cache_class(__x_ABI_CWindows_CStorage_CIApplicationData2 *iface, HSTRING *out)
{ return application_data_class(&cache_owner(iface)->iface, out); }
static HRESULT WINAPI cache_trust(__x_ABI_CWindows_CStorage_CIApplicationData2 *iface, TrustLevel *level)
{ return application_data_trust(&cache_owner(iface)->iface, level); }
static HRESULT WINAPI cache_folder(__x_ABI_CWindows_CStorage_CIApplicationData2 *iface, folder_i **out)
{
	struct application_data_object *self = cache_owner(iface);
	if (!out) return E_POINTER;
	*out = NULL;
	if (InterlockedCompareExchange(&self->cache_state, 1, 0) == 0) {
		WCHAR path[MAX_PATH];
		if (ensure_application_folder(path, L"LocalCache")) {
			lstrcpyW(self->cache_folder.name, L"LocalCache");
			lstrcpyW(self->cache_folder.path, path);
			self->cache_error = S_OK;
		} else self->cache_error = last_error();
		InterlockedExchange(&self->cache_state, 2);
	} else while (InterlockedCompareExchange(&self->cache_state, 2, 2) != 2) Sleep(0);
	if (FAILED(self->cache_error)) return self->cache_error;
	folder_addref(&self->cache_folder.iface);
	*out = &self->cache_folder.iface;
	return S_OK;
}
static const __x_ABI_CWindows_CStorage_CIApplicationData2Vtbl cache_table = {
	cache_query, cache_addref, cache_release, cache_iids, cache_class, cache_trust, cache_folder
};

static const __x_ABI_CWindows_CStorage_CIApplicationDataVtbl application_data_table = {
	.QueryInterface = application_data_query,
	.AddRef = application_data_addref,
	.Release = application_data_release,
	.GetIids = application_data_iids,
	.GetRuntimeClassName = application_data_class,
	.GetTrustLevel = application_data_trust,
	.get_Version = application_data_get_version,
	.SetVersionAsync = application_data_set_version_async,
	.ClearAllAsync = application_data_clear_all_async,
	.ClearAsync = application_data_clear_async,
	.get_LocalSettings = application_data_get_local_settings,
	.get_RoamingSettings = application_data_get_roaming_settings,
	.get_LocalFolder = application_data_get_local_folder,
	.get_RoamingFolder = application_data_get_roaming_folder,
	.get_TemporaryFolder = application_data_get_temporary_folder,
	.add_DataChanged = application_data_add_data_changed,
	.remove_DataChanged = application_data_remove_data_changed,
	.SignalDataChanged = application_data_signal_data_changed,
	.get_RoamingStorageQuota = application_data_get_roaming_storage_quota,
};

static struct application_data_object application_data_instance = {
	.iface = {(void *)&application_data_table},
	.cache_iface = {(void *)&cache_table},
	.local_folder = {{(void *)&folder_table}, {(void *)&folder_item_table}, 0, L"", L""},
	.temporary_folder = {{(void *)&folder_table}, {(void *)&folder_item_table}, 0, L"", L""},
	.cache_folder = {{(void *)&folder_table}, {(void *)&folder_item_table}, 0, L"", L""},
};

static HRESULT WINAPI application_data_query(application_data_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CStorage_CIApplicationData) || equal_iid(iid, &IID_IAgileObject))
		*out = iface;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CStorage_CIApplicationData2))
		*out = &((struct application_data_object *)iface)->cache_iface;
	else
		return E_NOINTERFACE;
	application_data_addref(iface);
	return S_OK;
}

static ULONG WINAPI application_data_addref(application_data_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct application_data_object *)iface)->refs);
}

static ULONG WINAPI application_data_release(application_data_i *iface)
{
	return (ULONG)InterlockedDecrement(&((struct application_data_object *)iface)->refs);
}

/* PathIO: the statics only, over absolute paths. */
static HRESULT pathio_write_text(HSTRING absolute_path, HSTRING contents)
{
	const WCHAR *path = WindowsGetStringRawBuffer(absolute_path, NULL);
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(contents, &length);
	char *buffer = NULL;
	int bytes = 0;
	HANDLE file;
	DWORD written = 0;
	BOOL wrote;
	HRESULT result = S_OK;
	if (length) {
		bytes = WideCharToMultiByte(CP_UTF8, 0, text, (int)length, NULL, 0, NULL, NULL);
		if (!bytes)
			return last_error();
		if (!(buffer = ALLOC((SIZE_T)bytes)))
			return E_OUTOFMEMORY;
		if (WideCharToMultiByte(CP_UTF8, 0, text, (int)length, buffer, bytes, NULL, NULL) != bytes) {
			FREE(buffer);
			return last_error();
		}
	}
	file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		FREE(buffer);
		return last_error();
	}
	wrote = WriteFile(file, buffer, (DWORD)bytes, &written, NULL);
	if (!wrote || written != (DWORD)bytes)
		result = last_error();
	CloseHandle(file);
	FREE(buffer);
	return result;
}

static HRESULT pathio_read_text(HSTRING absolute_path, HSTRING *out)
{
	const WCHAR *path = WindowsGetStringRawBuffer(absolute_path, NULL);
	LARGE_INTEGER size;
	BYTE *buffer;
	DWORD read = 0;
	int wide;
	WCHAR *text;
	HRESULT result;
	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (file == INVALID_HANDLE_VALUE)
		return last_error();
	if (!GetFileSizeEx(file, &size)) {
		result = last_error();
		CloseHandle(file);
		return result;
	}
	if (size.QuadPart > STORAGE_TEXT_MAX) {
		CloseHandle(file);
		return E_OUTOFMEMORY;
	}
	if (!(buffer = ALLOC((SIZE_T)size.QuadPart + 1))) {
		CloseHandle(file);
		return E_OUTOFMEMORY;
	}
	if (size.QuadPart && (!ReadFile(file, buffer, (DWORD)size.QuadPart, &read, NULL) || !read)) {
		result = last_error();
		CloseHandle(file);
		FREE(buffer);
		return result;
	}
	CloseHandle(file);
	/* a UTF-8 byte order mark precedes the text Windows writes with an explicit encoding; the decoder takes it off */
	if (read >= 3 && buffer[0] == 0xef && buffer[1] == 0xbb && buffer[2] == 0xbf) {
		read -= 3;
		memmove(buffer, buffer + 3, read);
	}
	if (!read) {
		FREE(buffer);
		return create_string(L"", out);
	}
	wide = MultiByteToWideChar(CP_UTF8, 0, (const char *)buffer, (int)read, NULL, 0);
	if (!wide) {
		result = last_error();
		FREE(buffer);
		return result;
	}
	if (!(text = ALLOC((SIZE_T)wide * sizeof(WCHAR)))) {
		FREE(buffer);
		return E_OUTOFMEMORY;
	}
	MultiByteToWideChar(CP_UTF8, 0, (const char *)buffer, (int)read, text, wide);
	FREE(buffer);
	result = WindowsCreateString(text, (UINT32)wide, out);
	FREE(text);
	return result;
}

static HRESULT WINAPI pathio_read_text_async(pathio_statics_i *iface, HSTRING absolute_path,
	text_operation_i **operation)
{
	struct text_operation *text;
	HRESULT result;
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	if (!(text = text_operation_new()))
		return E_OUTOFMEMORY;
	result = pathio_read_text(absolute_path, &text->result);
	if (SUCCEEDED(result))
		text_operation_finish(&text->iface, S_OK, Completed);
	else {
		text->result = NULL;
		text_operation_finish(&text->iface, result, Error);
	}
	*operation = &text->iface;
	return S_OK;
}

static HRESULT WINAPI pathio_write_text_async(pathio_statics_i *iface, HSTRING absolute_path, HSTRING contents,
	async_action_i **operation)
{
	struct storage_action *action;
	HRESULT result;
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	if (!(action = storage_action_new()))
		return E_OUTOFMEMORY;
	result = pathio_write_text(absolute_path, contents);
	storage_action_finish(&action->iface, result, SUCCEEDED(result) ? Completed : Error);
	*operation = &action->iface;
	return S_OK;
}

/* The rest of the statics: line and buffer forms answer E_NOTIMPL at their exact signatures, the buffer one carrying
 * the length and pointer the array ABI crosses. */
static HRESULT WINAPI pathio_write_text_async2(pathio_statics_i *iface, HSTRING absolutePath, HSTRING contents,
	__x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding encoding, async_action_i **operation)
{
	(void)iface;
	(void)absolutePath;
	(void)contents;
	(void)encoding;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI pathio_read_text_async2(pathio_statics_i *iface, HSTRING absolute_path,
	__x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding encoding, text_operation_i **operation)
{
	(void)iface;
	(void)absolute_path;
	(void)encoding;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI pathio_append_text_async(pathio_statics_i *iface, HSTRING absolute_path, HSTRING contents,
	async_action_i **operation)
{
	(void)iface;
	(void)absolute_path;
	(void)contents;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI pathio_append_text_async2(pathio_statics_i *iface, HSTRING absolute_path, HSTRING contents,
	__x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding encoding, async_action_i **operation)
{
	(void)iface;
	(void)absolute_path;
	(void)contents;
	(void)encoding;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI pathio_read_lines_async(pathio_statics_i *iface, HSTRING absolute_path,
	__FIAsyncOperation_1___FIVectorView_1_HSTRING **operation)
{
	(void)iface;
	(void)absolute_path;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI pathio_read_lines_async2(pathio_statics_i *iface, HSTRING absolute_path,
	__x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding encoding,
	__FIAsyncOperation_1___FIVectorView_1_HSTRING **operation)
{
	(void)iface;
	(void)absolute_path;
	(void)encoding;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI pathio_write_lines_async(pathio_statics_i *iface, HSTRING absolute_path,
	__FIIterable_1_HSTRING *lines, async_action_i **operation)
{
	(void)iface;
	(void)absolute_path;
	(void)lines;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI pathio_write_lines_async2(pathio_statics_i *iface, HSTRING absolute_path,
	__FIIterable_1_HSTRING *lines, __x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding encoding,
	async_action_i **operation)
{
	(void)iface;
	(void)absolute_path;
	(void)lines;
	(void)encoding;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI pathio_append_lines_async(pathio_statics_i *iface, HSTRING absolute_path,
	__FIIterable_1_HSTRING *lines, async_action_i **operation)
{
	(void)iface;
	(void)absolute_path;
	(void)lines;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI pathio_append_lines_async2(pathio_statics_i *iface, HSTRING absolute_path,
	__FIIterable_1_HSTRING *lines, __x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding encoding,
	async_action_i **operation)
{
	(void)iface;
	(void)absolute_path;
	(void)lines;
	(void)encoding;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI pathio_read_buffer_async(pathio_statics_i *iface, HSTRING absolute_path,
	__FIAsyncOperation_1_Windows__CStorage__CStreams__CIBuffer **operation)
{
	(void)iface;
	(void)absolute_path;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI pathio_write_buffer_async(pathio_statics_i *iface, HSTRING absolute_path,
	__x_ABI_CWindows_CStorage_CStreams_CIBuffer *buffer, async_action_i **operation)
{
	(void)iface;
	(void)absolute_path;
	(void)buffer;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI pathio_write_bytes_async(pathio_statics_i *iface, HSTRING absolute_path, UINT32 value_count,
	const BYTE *value, async_action_i **operation)
{
	(void)iface;
	(void)absolute_path;
	(void)value_count;
	(void)value;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	{ trace_not_implemented(__func__); return E_NOTIMPL; }
}


/* The class factories: each class answers its statics beside IActivationFactory, and neither class has a public
 * constructor — the application data is asked for, the path statics are only asked of. The two factories share one
 * refcount, the way the dispatcher queue's factories do, and neither is ever freed. */
static LONG factory_refs = 1;
static IActivationFactory application_activation, pathio_activation;
static application_data_statics_i application_statics;
static pathio_statics_i pathio_statics;

static HRESULT application_factory_query(REFIID iid, void **out);
static HRESULT pathio_factory_query(REFIID iid, void **out);

static HRESULT factory_query_for(IActivationFactory *factory, IUnknown *statics, REFIID statics_iid, REFIID iid,
	void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID_IActivationFactory))
		*out = factory;
	else if (equal_iid(iid, statics_iid))
		*out = statics;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&factory_refs);
	return S_OK;
}

static HRESULT application_factory_query(REFIID iid, void **out)
{
	return factory_query_for(&application_activation, (IUnknown *)&application_statics,
		&IID___x_ABI_CWindows_CStorage_CIApplicationDataStatics, iid, out);
}

static HRESULT pathio_factory_query(REFIID iid, void **out)
{
	return factory_query_for(&pathio_activation, (IUnknown *)&pathio_statics,
		&IID__x_ABI_CWindows_CStorage_CIPathIOStatics, iid, out);
}

static HRESULT WINAPI factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
	return iface == &pathio_activation ? pathio_factory_query(iid, out) : application_factory_query(iid, out);
}

static ULONG WINAPI factory_addref(IActivationFactory *iface)
{
	(void)iface;
	return (ULONG)InterlockedIncrement(&factory_refs);
}

static ULONG WINAPI factory_release(IActivationFactory *iface)
{
	(void)iface;
	return (ULONG)InterlockedDecrement(&factory_refs);
}

static HRESULT WINAPI factory_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI factory_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

/* The class has no public constructor: the application data of a process is asked for, never constructed by name. */
static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return E_ILLEGAL_METHOD_CALL;
}

static HRESULT WINAPI application_factory_class(IActivationFactory *iface, HSTRING *out)
{
	(void)iface;
	return create_string(application_data_class_name, out);
}

static HRESULT WINAPI pathio_factory_class(IActivationFactory *iface, HSTRING *out)
{
	(void)iface;
	return create_string(pathio_class_name, out);
}

static const IActivationFactoryVtbl application_factory_table = {
	.QueryInterface = factory_query,
	.AddRef = factory_addref,
	.Release = factory_release,
	.GetIids = factory_iids,
	.GetRuntimeClassName = application_factory_class,
	.GetTrustLevel = factory_trust,
	.ActivateInstance = factory_activate,
};

static const IActivationFactoryVtbl pathio_factory_table = {
	.QueryInterface = factory_query,
	.AddRef = factory_addref,
	.Release = factory_release,
	.GetIids = factory_iids,
	.GetRuntimeClassName = pathio_factory_class,
	.GetTrustLevel = factory_trust,
	.ActivateInstance = factory_activate,
};

/* The statics forward their lifetime to the factory, whose activation part answers for both. */
static HRESULT WINAPI application_statics_query(application_data_statics_i *iface, REFIID iid, void **out)
{
	(void)iface;
	return application_factory_query(iid, out);
}

static ULONG WINAPI application_statics_addref(application_data_statics_i *iface)
{
	(void)iface;
	return (ULONG)InterlockedIncrement(&factory_refs);
}

static ULONG WINAPI application_statics_release(application_data_statics_i *iface)
{
	(void)iface;
	return (ULONG)InterlockedDecrement(&factory_refs);
}

static HRESULT WINAPI application_statics_iids(application_data_statics_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI application_statics_class(application_data_statics_i *iface, HSTRING *out)
{
	(void)iface;
	return create_string(application_data_statics_class_name, out);
}

static HRESULT WINAPI application_statics_trust(application_data_statics_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI application_statics_get_current(application_data_statics_i *iface, application_data_i **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	application_data_addref(&application_data_instance.iface);
	*out = &application_data_instance.iface;
	return S_OK;
}

static const __x_ABI_CWindows_CStorage_CIApplicationDataStaticsVtbl application_statics_table = {
	.QueryInterface = application_statics_query,
	.AddRef = application_statics_addref,
	.Release = application_statics_release,
	.GetIids = application_statics_iids,
	.GetRuntimeClassName = application_statics_class,
	.GetTrustLevel = application_statics_trust,
	.get_Current = application_statics_get_current,
};

static HRESULT WINAPI pathio_statics_query(pathio_statics_i *iface, REFIID iid, void **out)
{
	(void)iface;
	return pathio_factory_query(iid, out);
}

static ULONG WINAPI pathio_statics_addref(pathio_statics_i *iface)
{
	(void)iface;
	return (ULONG)InterlockedIncrement(&factory_refs);
}

static ULONG WINAPI pathio_statics_release(pathio_statics_i *iface)
{
	(void)iface;
	return (ULONG)InterlockedDecrement(&factory_refs);
}

static HRESULT WINAPI pathio_statics_iids(pathio_statics_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI pathio_statics_class(pathio_statics_i *iface, HSTRING *out)
{
	(void)iface;
	return create_string(pathio_statics_class_name, out);
}

static HRESULT WINAPI pathio_statics_trust(pathio_statics_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static const __x_ABI_CWindows_CStorage_CIPathIOStaticsVtbl pathio_statics_table = {
	.QueryInterface = pathio_statics_query,
	.AddRef = pathio_statics_addref,
	.Release = pathio_statics_release,
	.GetIids = pathio_statics_iids,
	.GetRuntimeClassName = pathio_statics_class,
	.GetTrustLevel = pathio_statics_trust,
	.ReadTextAsync = pathio_read_text_async,
	.ReadTextAsync2 = pathio_read_text_async2,
	.WriteTextAsync = pathio_write_text_async,
	.WriteTextAsync2 = pathio_write_text_async2,
	.AppendTextAsync = pathio_append_text_async,
	.AppendTextAsync2 = pathio_append_text_async2,
	.ReadLinesAsync = pathio_read_lines_async,
	.ReadLinesAsync2 = pathio_read_lines_async2,
	.WriteLinesAsync = pathio_write_lines_async,
	.WriteLinesAsync2 = pathio_write_lines_async2,
	.AppendLinesAsync = pathio_append_lines_async,
	.AppendLinesAsync2 = pathio_append_lines_async2,
	.ReadBufferAsync = pathio_read_buffer_async,
	.WriteBufferAsync = pathio_write_buffer_async,
	.WriteBytesAsync = pathio_write_bytes_async,
};

static IActivationFactory application_activation = {(void *)&application_factory_table};
static application_data_statics_i application_statics = {(void *)&application_statics_table};
static IActivationFactory pathio_activation = {(void *)&pathio_factory_table};
static pathio_statics_i pathio_statics = {(void *)&pathio_statics_table};

/* StorageFile lookup uses the same file object and completed operation as folder lookup. */
BOOL raven_package_root(WCHAR *path, UINT32 capacity);
static const IID file_statics_iid = {0x5984c710,0xdaf2,0x43c8,{0x8b,0xb4,0xa4,0xd3,0xea,0xcf,0xd0,0x3f}};
static IActivationFactory file_activation;
struct file_statics_vtbl {
	HRESULT (WINAPI *query)(void *,REFIID,void **);
	ULONG (WINAPI *addref)(void *), (WINAPI *release)(void *);
	HRESULT (WINAPI *iids)(void *,ULONG *,IID **), (WINAPI *name)(void *,HSTRING *), (WINAPI *trust)(void *,TrustLevel *);
	HRESULT (WINAPI *path)(void *,HSTRING,file_operation_i **);
	HRESULT (WINAPI *uri)(void *,__x_ABI_CWindows_CFoundation_CIUriRuntimeClass *,file_operation_i **);
	HRESULT (WINAPI *stream1)(void *,void *,void *,void *,file_operation_i **);
	HRESULT (WINAPI *stream2)(void *,void *,void *,void *,file_operation_i **);
	HRESULT (WINAPI *stream3)(void *,void *,void *,void *,file_operation_i **);
	HRESULT (WINAPI *stream4)(void *,void *,void *,void *,file_operation_i **);
};
struct file_statics_object { const struct file_statics_vtbl *lpVtbl; };
static struct file_statics_object file_statics;
static HRESULT WINAPI file_static_query(void *self,REFIID iid,void **out)
{
	(void)self;
	if (!out) return E_POINTER;
	*out=NULL;
	if (equal_iid(iid,&IID_IUnknown) || equal_iid(iid,&IID_IInspectable) || equal_iid(iid,&IID_IActivationFactory) || equal_iid(iid,&IID_IAgileObject)) *out=&file_activation;
	else if (equal_iid(iid,&file_statics_iid)) *out=&file_statics;
	else return E_NOINTERFACE;
	InterlockedIncrement(&factory_refs);
	return S_OK;
}
static ULONG WINAPI file_static_addref(void *self) { (void)self; return InterlockedIncrement(&factory_refs); }
static ULONG WINAPI file_static_release(void *self) { (void)self; return InterlockedDecrement(&factory_refs); }
static HRESULT WINAPI file_static_iids(void *self,ULONG *count,IID **out) { (void)self; return inspectable_iids(count,out); }
static HRESULT WINAPI file_static_name(void *self,HSTRING *out) { (void)self; return create_string(file_class_name,out); }
static HRESULT WINAPI file_static_trust(void *self,TrustLevel *out) { (void)self; return inspectable_trust(out); }
static HRESULT WINAPI file_static_activate(IActivationFactory *self,IInspectable **out) { (void)self; if (!out) return E_POINTER; *out=NULL; return E_NOTIMPL; }
static HRESULT file_from_path(const WCHAR *path, BOOL read_only, file_operation_i **out)
{
	file_i *file=NULL; DWORD attrs; HRESULT result;
	if (!out) return E_POINTER;
	*out=NULL;
	if (!path || !*path || lstrlenW(path)>=MAX_PATH) { raven_trace_note("raven-storage: path empty or too long\n"); return E_INVALIDARG; }
	attrs=GetFileAttributesW(path);
	if (attrs==INVALID_FILE_ATTRIBUTES) return file_operation_completed(NULL,last_error(),out);
	if (attrs & FILE_ATTRIBUTE_DIRECTORY) return file_operation_completed(NULL,HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),out);
	result=file_create(path,read_only,&file);
	if (FAILED(result)) return result;
	result=file_operation_completed(file,S_OK,out);
	__x_ABI_CWindows_CStorage_CIStorageFile_Release(file);
	return result;
}
static HRESULT WINAPI file_static_path(void *self,HSTRING path,file_operation_i **out)
{
	(void)self;
	return file_from_path(WindowsGetStringRawBuffer(path,NULL),FALSE,out);
}
static HRESULT WINAPI file_static_uri(void *self,__x_ABI_CWindows_CFoundation_CIUriRuntimeClass *uri,file_operation_i **out)
{
	HSTRING value=NULL; WCHAR path[MAX_PATH]; const WCHAR *text,*rest; HRESULT result;
	(void)self;
	if (!uri || !out) return E_POINTER;
	*out=NULL;
	result=uri->lpVtbl->get_AbsoluteUri(uri,&value);
	if (FAILED(result)) return result;
	text=WindowsGetStringRawBuffer(value,NULL);
	if (!text || CompareStringOrdinal(text,11,L"ms-appx:///",11,TRUE)!=CSTR_EQUAL) { WindowsDeleteString(value); return E_NOTIMPL; }
	rest=text+11;
	/* Keep package resources inside the package; escaped paths need a full URI decoder. */
	for (const WCHAR *scan=rest; *scan; ++scan)
		if (*scan==L'%' || *scan==L':' || (*scan==L'.' && scan[1]==L'.')) { WindowsDeleteString(value); raven_trace_note("raven-storage: uri path escapes the package\n"); return E_INVALIDARG; }
	if (!raven_package_root(path,MAX_PATH)) { WindowsDeleteString(value); raven_trace_note("raven-storage: no package root\n"); return E_INVALIDARG; }
	if (lstrlenW(path)+1+lstrlenW(rest)>=MAX_PATH) { WindowsDeleteString(value); return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE); }
	lstrcatW(path,L"\\"); lstrcatW(path,rest);
	result=file_from_path(path,TRUE,out);
	WindowsDeleteString(value);
	return result;
}
static HRESULT WINAPI file_static_stream(void *self,void *a,void *b,void *c,file_operation_i **out)
{ (void)self; (void)a; (void)b; (void)c; if (!out) return E_POINTER; *out=NULL; return E_NOTIMPL; }
static const struct file_statics_vtbl file_statics_table={file_static_query,file_static_addref,file_static_release,file_static_iids,file_static_name,file_static_trust,file_static_path,file_static_uri,file_static_stream,file_static_stream,file_static_stream,file_static_stream};
static struct file_statics_object file_statics={&file_statics_table};
static const IActivationFactoryVtbl file_activation_table={(void *)file_static_query,(void *)file_static_addref,(void *)file_static_release,(void *)file_static_iids,(void *)file_static_name,(void *)file_static_trust,file_static_activate};
static IActivationFactory file_activation={&file_activation_table};

/* The catalog routes Windows.Storage.ApplicationData and Windows.Storage.PathIO here. */
HRESULT raven_storage_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (length == ARRAYSIZE(file_class_name) - 1 &&
		CompareStringOrdinal(text, (int)length, file_class_name, -1, FALSE) == CSTR_EQUAL)
		return file_static_query(&file_activation,&IID_IActivationFactory,(void **)out);
	if (length == ARRAYSIZE(application_data_class_name) - 1 &&
		CompareStringOrdinal(text, (int)length, application_data_class_name, -1, FALSE) == CSTR_EQUAL)
		return application_factory_query(&IID_IActivationFactory, (void **)out);
	if (length == ARRAYSIZE(pathio_class_name) - 1 &&
		CompareStringOrdinal(text, (int)length, pathio_class_name, -1, FALSE) == CSTR_EQUAL)
		return pathio_factory_query(&IID_IActivationFactory, (void **)out);
	return CLASS_E_CLASSNOTAVAILABLE;
}

/* The absolute path behind a storage file this component created. The xml_dom module
 * uses it to read and write the files a document load or save names; the identity check
 * is the file object's own vtable, so guests' file implementations answer E_NOINTERFACE. */
HRESULT raven_storage_file_path(__x_ABI_CWindows_CStorage_CIStorageFile *file, WCHAR *path, UINT32 capacity)
{
	struct file_object *self;
	if (!file || !path)
		return E_POINTER;
	if (((file_i *)file)->lpVtbl != &file_table)
		return E_NOINTERFACE;
	self = (struct file_object *)file;
	if ((UINT32)lstrlenW(self->path) + 1 > capacity)
		return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	lstrcpyW(path, self->path);
	return S_OK;
}
