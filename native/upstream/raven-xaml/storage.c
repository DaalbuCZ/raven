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
#include "windows_storage.h"

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))
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
static BOOL ensure_local_state(WCHAR *path /* MAX_PATH */)
{
	WCHAR directory[MAX_PATH];
	DWORD attributes;
	if (!executable_directory(directory))
		return FALSE;
	if ((UINT32)lstrlenW(directory) + 1 + (UINT32)lstrlenW(local_state_name) + 1 >= MAX_PATH)
		return FALSE;
	lstrcpyW(path, directory);
	lstrcatW(path, L"\\");
	lstrcatW(path, local_state_name);
	attributes = GetFileAttributesW(path);
	if (attributes != INVALID_FILE_ATTRIBUTES)
		return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	if (!CreateDirectoryW(path, NULL))
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

/* The StorageFile: a name, the full path and the extension, bound to nothing but the file system itself. */
#define FILE_NAME_MAX 96

struct file_object {
	file_i iface;
	item_i item;
	LONG refs;
	WCHAR name[FILE_NAME_MAX];
	WCHAR path[MAX_PATH];
	WCHAR type[FILE_NAME_MAX]; /* the extension with the dot, empty when the name carries none */
};

static HRESULT WINAPI file_query(file_i *iface, REFIID iid, void **out);
static ULONG WINAPI file_addref(file_i *iface);
static ULONG WINAPI file_release(file_i *iface);

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
	return E_NOTIMPL;
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
	return E_NOTIMPL;
}

static HRESULT WINAPI file_open_transacted_write_async(file_i *iface,
	__FIAsyncOperation_1_Windows__CStorage__CStorageStreamTransaction **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI file_copy_overload_default(file_i *iface, folder_i *folder, file_operation_i **operation)
{
	(void)iface;
	(void)folder;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
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
	return E_NOTIMPL;
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
	return E_NOTIMPL;
}

static HRESULT WINAPI file_copy_and_replace_async(file_i *iface, file_i *file, async_action_i **operation)
{
	(void)iface;
	(void)file;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI file_move_overload_default(file_i *iface, folder_i *folder, async_action_i **operation)
{
	(void)iface;
	(void)folder;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
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
	return E_NOTIMPL;
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
	return E_NOTIMPL;
}

static HRESULT WINAPI file_move_and_replace_async(file_i *iface, file_i *file, async_action_i **operation)
{
	(void)iface;
	(void)file;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
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
	if (!DeleteFileW(self->path)) {
		result = last_error();
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
	return E_NOTIMPL;
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
	return E_NOTIMPL;
}

static HRESULT WINAPI file_item_get_basic_properties(item_i *iface,
	__FIAsyncOperation_1_Windows__CStorage__CFileProperties__CBasicProperties **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
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
	return E_NOTIMPL;
}

static HRESULT WINAPI file_item_get_date_created(item_i *iface, __x_ABI_CWindows_CFoundation_CDateTime *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	ZeroMemory(value, sizeof *value);
	return E_NOTIMPL;
}

static HRESULT WINAPI file_item_is_of_type(item_i *iface, __x_ABI_CWindows_CStorage_CStorageItemTypes type,
	boolean *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	(void)type;
	return E_NOTIMPL;
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
static HRESULT file_create(const WCHAR *path, file_i **out)
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
		return E_INVALIDARG;
	if (!(self = ALLOC(sizeof *self)))
		return E_OUTOFMEMORY;
	self->iface.lpVtbl = (void *)&file_table;
	self->item.lpVtbl = (void *)&file_item_table;
	self->refs = 1;
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
		return E_INVALIDARG;
	if (*length >= FILE_NAME_MAX)
		return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
	for (i = 0; i < *length; ++i)
		if (text[i] == L'\\' || text[i] == L'/' || text[i] == L':')
			return E_INVALIDARG;
	if ((*length == 1 && text[0] == L'.') || (*length == 2 && text[0] == L'.' && text[1] == L'.'))
		return E_INVALIDARG;
	return S_OK;
}

/* The StorageFolder: the LocalState directory, with the name and path it is bound to. */
struct folder_object {
	folder_i iface;
	item_i item;
	LONG refs;
	WCHAR name[FILE_NAME_MAX];
	WCHAR path[MAX_PATH];
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
	return E_NOTIMPL;
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
	return E_NOTIMPL;
}

static HRESULT WINAPI folder_item_delete(item_i *iface, __x_ABI_CWindows_CStorage_CStorageDeleteOption option,
	async_action_i **operation)
{
	(void)iface;
	(void)option;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
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
	return E_NOTIMPL;
}

static HRESULT WINAPI folder_item_get_attributes(item_i *iface, __x_ABI_CWindows_CStorage_CFileAttributes *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	return E_NOTIMPL;
}

static HRESULT WINAPI folder_item_get_date_created(item_i *iface, __x_ABI_CWindows_CFoundation_CDateTime *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	ZeroMemory(value, sizeof *value);
	return E_NOTIMPL;
}

static HRESULT WINAPI folder_item_is_of_type(item_i *iface, __x_ABI_CWindows_CStorage_CStorageItemTypes type,
	boolean *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	(void)type;
	return E_NOTIMPL;
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
		return E_NOTIMPL;
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
	result = file_create(path, &file);
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
	return E_NOTIMPL;
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
	return E_NOTIMPL;
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
	result = file_create(path, &file);
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
	return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_item_async(folder_i *iface, HSTRING name,
	__FIAsyncOperation_1_Windows__CStorage__CIStorageItem **operation)
{
	(void)iface;
	(void)name;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_files_async(folder_i *iface,
	__FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFile **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_folders_async(folder_i *iface,
	__FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CStorageFolder **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_items_async(folder_i *iface,
	__FIAsyncOperation_1___FIVectorView_1_Windows__CStorage__CIStorageItem **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
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

/* The ApplicationData: one per process, the way Package.Current is. The LocalFolder is part of the singleton and
 * lives as long as the process; its directory is resolved and created the first time a guest asks for it. */
struct application_data_object {
	application_data_i iface;
	LONG refs;
	struct folder_object local_folder;
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
	return E_NOTIMPL;
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
	return E_NOTIMPL;
}

static HRESULT WINAPI application_data_clear_all_async(application_data_i *iface, async_action_i **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI application_data_clear_async(application_data_i *iface,
	__x_ABI_CWindows_CStorage_CApplicationDataLocality locality, async_action_i **operation)
{
	(void)iface;
	(void)locality;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
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
	return E_NOTIMPL;
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
		if (ensure_local_state(path)) {
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
	return E_NOTIMPL;
}

static HRESULT WINAPI application_data_get_temporary_folder(application_data_i *iface, folder_i **value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI application_data_add_data_changed(application_data_i *iface,
	__FITypedEventHandler_2_Windows__CStorage__CApplicationData_IInspectable *handler, EventRegistrationToken *token)
{
	(void)iface;
	(void)handler;
	if (!token)
		return E_POINTER;
	return E_NOTIMPL;
}

static HRESULT WINAPI application_data_remove_data_changed(application_data_i *iface, EventRegistrationToken token)
{
	(void)iface;
	(void)token;
	return E_NOTIMPL;
}

static HRESULT WINAPI application_data_signal_data_changed(application_data_i *iface)
{
	(void)iface;
	return E_NOTIMPL;
}

static HRESULT WINAPI application_data_get_roaming_storage_quota(application_data_i *iface, UINT64 *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	return E_NOTIMPL;
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
	{(void *)&application_data_table},
	0, /* the singleton is never freed; the count follows the references the guests hold */
	{{(void *)&folder_table}, {(void *)&folder_item_table}, 0, L"", L""},
	0,
	S_OK,
};

static HRESULT WINAPI application_data_query(application_data_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CStorage_CIApplicationData) || equal_iid(iid, &IID_IAgileObject))
		*out = iface;
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
	return E_NOTIMPL;
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
	return E_NOTIMPL;
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
	return E_NOTIMPL;
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
	return E_NOTIMPL;
}

static HRESULT WINAPI pathio_read_lines_async(pathio_statics_i *iface, HSTRING absolute_path,
	__FIAsyncOperation_1___FIVectorView_1_HSTRING **operation)
{
	(void)iface;
	(void)absolute_path;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
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
	return E_NOTIMPL;
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
	return E_NOTIMPL;
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
	return E_NOTIMPL;
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
	return E_NOTIMPL;
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
	return E_NOTIMPL;
}

static HRESULT WINAPI pathio_read_buffer_async(pathio_statics_i *iface, HSTRING absolute_path,
	__FIAsyncOperation_1_Windows__CStorage__CStreams__CIBuffer **operation)
{
	(void)iface;
	(void)absolute_path;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
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
	return E_NOTIMPL;
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
	return E_NOTIMPL;
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

/* The catalog routes Windows.Storage.ApplicationData and Windows.Storage.PathIO here. */
HRESULT raven_storage_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (length == ARRAYSIZE(application_data_class_name) - 1 &&
		CompareStringOrdinal(text, (int)length, application_data_class_name, -1, FALSE) == CSTR_EQUAL)
		return application_factory_query(&IID_IActivationFactory, (void **)out);
	if (length == ARRAYSIZE(pathio_class_name) - 1 &&
		CompareStringOrdinal(text, (int)length, pathio_class_name, -1, FALSE) == CSTR_EQUAL)
		return pathio_factory_query(&IID_IActivationFactory, (void **)out);
	return CLASS_E_CLASSNOTAVAILABLE;
}
