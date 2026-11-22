/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.System.Threading.ThreadPool class of the Windows Runtime.
 *
 * The statics of the thread pool: every RunAsync runs its work item handler once on a thread of its
 * own, and the action it returns follows the shared async shape (IAsyncAction + IAsyncInfo, the
 * completion handler raised after the handler returns, as the system completes a work item). The
 * priorities and options are accepted and carried no further: this pool has one kind of thread. The
 * caller this serves is Win2D's async machinery (AsyncOperation::StartThreadPoolDelegate), which
 * schedules document and image work the application awaits. */
#define INITGUID
#define CONST_VTABLE
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define WIDL_using_Windows_System
#define WIDL_using_Windows_System_Threading
#define WIDL_using_Windows_Foundation
#include <windows.h>
#include <winstring.h>
#include "xaml_internal.h"
#include "windows.foundation.h"
#include "windows.system.threading.h"

typedef __x_ABI_CWindows_CFoundation_CIAsyncAction async_action;
typedef __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler action_completed;
typedef __x_ABI_CWindows_CSystem_CThreading_CIWorkItemHandler work_handler;
typedef __x_ABI_CWindows_CSystem_CThreading_CIThreadPoolStatics pool_statics;

static const WCHAR class_name[] = L"Windows.System.Threading.ThreadPool";
static const WCHAR action_name[] = L"Windows.Foundation.IAsyncAction";

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))
#define RO_E_CLOSED ((HRESULT)0x80000013)
#define E_ILLEGAL_METHOD_CALL ((HRESULT)0x8000000e)
#define E_ILLEGAL_DELEGATE_ASSIGNMENT ((HRESULT)0x80000018)

static HRESULT no_iids(ULONG *count, IID **iids)
{
	if (!count || !iids) return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}

static HRESULT class_string(const WCHAR *name, HSTRING *out)
{
	if (!out) return E_POINTER;
	return WindowsCreateString(name, (UINT32)lstrlenW(name), out);
}

static HRESULT trust(TrustLevel *out)
{
	if (!out) return E_POINTER;
	*out = BaseTrust;
	return S_OK;
}

/* ---- the action: the shared async-action shape, completed when the work item returns ---- */
struct pool_action {
	async_action iface;
	IAsyncInfo info;
	LONG refs;
	CRITICAL_SECTION lock;
	AsyncStatus status;
	HRESULT error;
	UINT32 id;
	BOOL closed, assigned;
	action_completed *completed;
};
static ULONG WINAPI action_addref(async_action *iface);
static void action_finish(async_action *iface, HRESULT result, AsyncStatus status)
{
	struct pool_action *self = (void *)iface;
	action_completed *handler = NULL;
	EnterCriticalSection(&self->lock);
	if (self->status == Started) {
		self->status = status;
		self->error = result;
		handler = self->completed;
		if (handler) __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_AddRef(handler);
	}
	LeaveCriticalSection(&self->lock);
	if (handler) {
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Invoke(handler, iface, status);
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Release(handler);
	}
}
static HRESULT WINAPI action_query(async_action *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IInspectable) ||
		IsEqualGUID(iid, &IID___x_ABI_CWindows_CFoundation_CIAsyncAction)) *out = iface;
	else if (IsEqualGUID(iid, &IID_IAsyncInfo)) *out = &((struct pool_action *)iface)->info;
	else return E_NOINTERFACE;
	action_addref(iface);
	return S_OK;
}
static ULONG WINAPI action_addref(async_action *iface) { return InterlockedIncrement(&((struct pool_action *)iface)->refs); }
static ULONG WINAPI action_release(async_action *iface)
{
	struct pool_action *self = (void *)iface;
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->completed) __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Release(self->completed);
		DeleteCriticalSection(&self->lock);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return refs;
}
static HRESULT WINAPI action_iids(async_action *iface, ULONG *count, IID **iids) { (void)iface; return no_iids(count, iids); }
static HRESULT WINAPI action_class(async_action *iface, HSTRING *out) { (void)iface; return class_string(action_name, out); }
static HRESULT WINAPI action_trust(async_action *iface, TrustLevel *out) { (void)iface; return trust(out); }
static HRESULT WINAPI action_put_completed(async_action *iface, action_completed *handler)
{
	struct pool_action *self = (void *)iface;
	HRESULT result = S_OK;
	AsyncStatus status;
	EnterCriticalSection(&self->lock);
	status = self->status;
	if (self->closed) result = RO_E_CLOSED;
	else if (self->assigned) result = E_ILLEGAL_DELEGATE_ASSIGNMENT;
	else {
		self->assigned = TRUE;
		self->completed = handler;
		if (handler) {
			__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_AddRef(handler);
			if (status != Started) __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_AddRef(handler);
		}
	}
	LeaveCriticalSection(&self->lock);
	if (SUCCEEDED(result) && handler && status != Started) {
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Invoke(handler, iface, status);
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Release(handler);
	}
	return result;
}
static HRESULT WINAPI action_get_completed(async_action *iface, action_completed **handler)
{
	struct pool_action *self = (void *)iface;
	HRESULT result = S_OK;
	if (!handler) return E_POINTER;
	*handler = NULL;
	EnterCriticalSection(&self->lock);
	if (self->closed) result = RO_E_CLOSED;
	else if ((*handler = self->completed)) __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_AddRef(*handler);
	LeaveCriticalSection(&self->lock);
	return result;
}
static HRESULT WINAPI action_results(async_action *iface)
{
	struct pool_action *self = (void *)iface;
	HRESULT result;
	EnterCriticalSection(&self->lock);
	result = self->closed ? RO_E_CLOSED : self->status == Started ? E_ILLEGAL_METHOD_CALL : self->error;
	LeaveCriticalSection(&self->lock);
	return result;
}
static struct pool_action *info_owner(IAsyncInfo *iface) { return CONTAINING_RECORD(iface, struct pool_action, info); }
static HRESULT WINAPI info_query(IAsyncInfo *iface, REFIID iid, void **out) { return action_query(&info_owner(iface)->iface, iid, out); }
static ULONG WINAPI info_addref(IAsyncInfo *iface) { return action_addref(&info_owner(iface)->iface); }
static ULONG WINAPI info_release(IAsyncInfo *iface) { return action_release(&info_owner(iface)->iface); }
static HRESULT WINAPI info_iids(IAsyncInfo *iface, ULONG *count, IID **iids) { (void)iface; return no_iids(count, iids); }
static HRESULT WINAPI info_class(IAsyncInfo *iface, HSTRING *out) { (void)iface; return class_string(action_name, out); }
static HRESULT WINAPI info_trust(IAsyncInfo *iface, TrustLevel *out) { (void)iface; return trust(out); }
static HRESULT WINAPI info_id(IAsyncInfo *iface, UINT32 *out)
{
	if (!out) return E_POINTER;
	struct pool_action *self = info_owner(iface);
	EnterCriticalSection(&self->lock);
	HRESULT result = self->closed ? RO_E_CLOSED : S_OK;
	*out = self->id;
	LeaveCriticalSection(&self->lock);
	return result;
}
static HRESULT WINAPI info_status(IAsyncInfo *iface, AsyncStatus *out)
{
	if (!out) return E_POINTER;
	struct pool_action *self = info_owner(iface);
	EnterCriticalSection(&self->lock);
	HRESULT result = self->closed ? RO_E_CLOSED : S_OK;
	*out = self->status;
	LeaveCriticalSection(&self->lock);
	return result;
}
static HRESULT WINAPI info_error(IAsyncInfo *iface, HRESULT *out)
{
	if (!out) return E_POINTER;
	struct pool_action *self = info_owner(iface);
	EnterCriticalSection(&self->lock);
	HRESULT result = self->closed ? RO_E_CLOSED : S_OK;
	*out = self->error;
	LeaveCriticalSection(&self->lock);
	return result;
}
static HRESULT WINAPI info_cancel(IAsyncInfo *iface)
{
	/* A running work item is not cancelled; the action still completes when the handler returns. */
	(void)iface;
	return S_OK;
}
static HRESULT WINAPI info_close(IAsyncInfo *iface)
{
	struct pool_action *self = info_owner(iface);
	action_completed *handler = NULL;
	HRESULT result = S_OK;
	EnterCriticalSection(&self->lock);
	if (self->status == Started) result = E_ILLEGAL_METHOD_CALL;
	else {
		self->closed = TRUE;
		handler = self->completed;
		self->completed = NULL;
	}
	LeaveCriticalSection(&self->lock);
	if (handler) __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Release(handler);
	return result;
}
static const IAsyncInfoVtbl pool_info_table = {
	info_query, info_addref, info_release, info_iids, info_class, info_trust, info_id, info_status, info_error, info_cancel, info_close
};
static const __x_ABI_CWindows_CFoundation_CIAsyncActionVtbl pool_action_table = {
	action_query, action_addref, action_release, action_iids, action_class, action_trust,
	action_put_completed, action_get_completed, action_results
};
static struct pool_action *new_pool_action(void)
{
	static LONG next_id;
	struct pool_action *action = ALLOC(sizeof *action);
	if (!action) return NULL;
	action->iface.lpVtbl = &pool_action_table;
	action->info.lpVtbl = &pool_info_table;
	action->refs = 1;
	action->status = Started;
	action->id = (UINT32)InterlockedIncrement(&next_id);
	InitializeCriticalSection(&action->lock);
	InterlockedIncrement(&raven_xaml_objects);
	return action;
}

/* ---- the work item: one handler, one thread, the action it completes ---- */
struct pool_work {
	work_handler *handler;
	async_action *action;
	UINT32 id;
};

static DWORD WINAPI work_thread(void *context)
{
	struct pool_work work = *(struct pool_work *)context;
	FREE(context);
	{
		char note[64];
		wsprintfA(note, "raven-xaml: thread pool: work item %u starts\n", (unsigned)work.id);
		raven_trace_note(note);
	}
	__x_ABI_CWindows_CSystem_CThreading_CIWorkItemHandler_Invoke(work.handler, work.action);
	action_finish(work.action, S_OK, Completed);
	__x_ABI_CWindows_CSystem_CThreading_CIWorkItemHandler_Release(work.handler);
	action_release(work.action);
	return 0;
}

static HRESULT pool_run(work_handler *handler, async_action **operation)
{
	struct pool_action *action;
	struct pool_work *work;
	HANDLE thread;
	if (!operation) return E_POINTER;
	*operation = NULL;
	if (!handler) return E_INVALIDARG;
	if (!(action = new_pool_action())) return E_OUTOFMEMORY;
	if (!(work = ALLOC(sizeof *work))) {
		action_release(&action->iface);
		return E_OUTOFMEMORY;
	}
	work->handler = handler;
	__x_ABI_CWindows_CSystem_CThreading_CIWorkItemHandler_AddRef(handler);
	work->action = &action->iface;
	work->id = action->id;
	action_addref(&action->iface); /* the work thread's own reference: the caller may drop its action at once */
	if (!(thread = CreateThread(NULL, 0, work_thread, work, 0, NULL))) {
		HRESULT error = HRESULT_FROM_WIN32(GetLastError());
		FREE(work);
		__x_ABI_CWindows_CSystem_CThreading_CIWorkItemHandler_Release(handler);
		action_finish(&action->iface, error, Error);
		action_release(&action->iface);
		action_release(&action->iface);
		return error;
	}
	CloseHandle(thread);
	*operation = &action->iface;
	return S_OK;
}

/* ---- the factory: the activation parts of the static class ---- */
typedef __x_ABI_CWindows_CSystem_CThreading_CIThreadPoolStaticsVtbl pool_statics_vtbl;
struct factory;
struct part_activation { const IActivationFactoryVtbl *lpVtbl; struct factory *owner; };
struct part_statics { const pool_statics_vtbl *lpVtbl; struct factory *owner; };
struct factory {
	struct part_activation activation;
	struct part_statics statics;
	LONG refs;
};

static HRESULT factory_query(struct factory *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IInspectable) || IsEqualGUID(iid, &IID_IActivationFactory))
		*out = &self->activation;
	else if (IsEqualGUID(iid, &IID___x_ABI_CWindows_CSystem_CThreading_CIThreadPoolStatics))
		*out = &self->statics;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

#define PART_BOILERPLATE(part, iface) \
	static HRESULT WINAPI part##_query(iface *This, REFIID iid, void **out) \
	{ return factory_query(((struct part *)This)->owner, iid, out); } \
	static ULONG WINAPI part##_addref(iface *This) \
	{ return (ULONG)InterlockedIncrement(&((struct part *)This)->owner->refs); } \
	static ULONG WINAPI part##_release(iface *This) \
	{ return (ULONG)InterlockedDecrement(&((struct part *)This)->owner->refs); } \
	static HRESULT WINAPI part##_iids(iface *This, ULONG *count, IID **iids) \
	{ (void)This; return no_iids(count, iids); } \
	static HRESULT WINAPI part##_name(iface *This, HSTRING *name) \
	{ (void)This; return class_string(class_name, name); } \
	static HRESULT WINAPI part##_trust(iface *This, TrustLevel *level) \
	{ (void)This; return trust(level); }

PART_BOILERPLATE(part_activation, IActivationFactory)
PART_BOILERPLATE(part_statics, pool_statics)

static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	raven_trace_note("raven-xaml: thread pool: ActivateInstance of a class that only has statics\n");
	return E_ILLEGAL_METHOD_CALL;
}

static const IActivationFactoryVtbl activation_table = {
	part_activation_query, part_activation_addref, part_activation_release, part_activation_iids, part_activation_name, part_activation_trust,
	factory_activate
};

static HRESULT WINAPI statics_RunAsync(pool_statics *iface, work_handler *handler, async_action **operation)
{
	(void)iface;
	raven_trace_note("raven-xaml: thread pool: RunAsync\n");
	return pool_run(handler, operation);
}

static HRESULT WINAPI statics_RunWithPriorityAsync(pool_statics *iface, work_handler *handler,
	__x_ABI_CWindows_CSystem_CThreading_CWorkItemPriority priority, async_action **operation)
{
	(void)iface;
	(void)priority;
	raven_trace_note("raven-xaml: thread pool: RunWithPriorityAsync\n");
	return pool_run(handler, operation);
}

static HRESULT WINAPI statics_RunWithPriorityAndOptionsAsync(pool_statics *iface, work_handler *handler,
	__x_ABI_CWindows_CSystem_CThreading_CWorkItemPriority priority, __x_ABI_CWindows_CSystem_CThreading_CWorkItemOptions options,
	async_action **operation)
{
	(void)iface;
	(void)priority;
	(void)options;
	raven_trace_note("raven-xaml: thread pool: RunWithPriorityAndOptionsAsync\n");
	return pool_run(handler, operation);
}

static const pool_statics_vtbl statics_table = {
	part_statics_query, part_statics_addref, part_statics_release, part_statics_iids, part_statics_name, part_statics_trust,
	statics_RunAsync, statics_RunWithPriorityAsync, statics_RunWithPriorityAndOptionsAsync
};

static struct factory pool_factory_instance = {
	{ &activation_table, NULL },
	{ &statics_table, NULL },
	0
};

HRESULT raven_thread_pool_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	if (length != ARRAYSIZE(class_name) - 1 ||
		CompareStringOrdinal(text, (int)length, class_name, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	pool_factory_instance.activation.owner = &pool_factory_instance;
	pool_factory_instance.statics.owner = &pool_factory_instance;
	InterlockedIncrement(&pool_factory_instance.refs);
	*out = (IActivationFactory *)&pool_factory_instance.activation;
	return S_OK;
}
