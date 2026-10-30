/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Single-view CoreApplication startup on the caller's UI thread. Interface layouts are supplied by mingw's public WinRT headers. */
#define CONST_VTABLE
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define WIDL_using_Windows_ApplicationModel_Core
#define WIDL_using_Windows_ApplicationModel_Activation
#define WIDL_using_Windows_UI_Core
#include <winstring.h>
#include <appmodel.h>
#include <windows.applicationmodel.core.h>
#include <windows.applicationmodel.activation.h>
#include "xaml_internal.h"
#include "events.h"

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))
#define E_ILLEGAL_METHOD_CALL ((HRESULT)0x8000000e)
#define RO_E_CLOSED ((HRESULT)0x80000013)

typedef __FITypedEventHandler_2_Windows__CApplicationModel__CCore__CCoreApplicationView_Windows__CApplicationModel__CActivation__CIActivatedEventArgs activated_handler;
typedef __x_ABI_CWindows_CFoundation_CIAsyncAction async_action;
typedef __FIEventHandler_1_IInspectable exiting_handler;

static const WCHAR application_name[] = L"Windows.ApplicationModel.Core.CoreApplication";
static const WCHAR view_name[] = L"Windows.ApplicationModel.Core.CoreApplicationView";
static const WCHAR launch_name[] = L"Windows.ApplicationModel.Activation.LaunchActivatedEventArgs";
static DWORD session_tls = TLS_OUT_OF_INDEXES;
static LONG running;
static CRITICAL_SECTION application_lock;
static DWORD application_thread;

static HRESULT class_string(const WCHAR *text, HSTRING *out)
{
	if (!out) return E_POINTER;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), out);
}
static HRESULT no_iids(ULONG *count, IID **iids)
{
	if (!count || !iids) return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}
static HRESULT base_trust(TrustLevel *out)
{
	if (!out) return E_POINTER;
	*out = BaseTrust;
	return S_OK;
}

static struct raven_event_list exiting;

struct application_view {
	ICoreApplicationView iface;
	LONG refs;
	ICoreWindow *window;
	struct raven_event_list activated;
};
static HRESULT WINAPI view_query(ICoreApplicationView *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) && !IsEqualIID(iid, &IID_ICoreApplicationView))
		return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct application_view *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI view_addref(ICoreApplicationView *iface)
{
	return InterlockedIncrement(&((struct application_view *)iface)->refs);
}
static ULONG WINAPI view_release(ICoreApplicationView *iface)
{
	struct application_view *self = (void *)iface;
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		raven_event_close(&self->activated);
		DeleteCriticalSection(&self->activated.lock);
		ICoreWindow_Release(self->window);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return refs;
}
static HRESULT WINAPI view_iids(ICoreApplicationView *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI view_class(ICoreApplicationView *iface, HSTRING *out) { return class_string(view_name, out); }
static HRESULT WINAPI view_trust(ICoreApplicationView *iface, TrustLevel *out) { return base_trust(out); }
static HRESULT WINAPI view_window(ICoreApplicationView *iface, ICoreWindow **out)
{
	if (!out) return E_POINTER;
	*out = ((struct application_view *)iface)->window;
	ICoreWindow_AddRef(*out);
	return S_OK;
}
static HRESULT WINAPI view_add_activated(ICoreApplicationView *iface, activated_handler *handler, EventRegistrationToken *token)
{
	return raven_event_add(&((struct application_view *)iface)->activated, (IUnknown *)handler, token);
}
static HRESULT WINAPI view_remove_activated(ICoreApplicationView *iface, EventRegistrationToken token)
{
	return raven_event_remove(&((struct application_view *)iface)->activated, token);
}
static HRESULT WINAPI view_main(ICoreApplicationView *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = TRUE;
	return S_OK;
}
static HRESULT WINAPI view_hosted(ICoreApplicationView *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = FALSE;
	return S_OK;
}
static const ICoreApplicationViewVtbl view_table = {
	view_query, view_addref, view_release, view_iids, view_class, view_trust,
	view_window, view_add_activated, view_remove_activated, view_main, view_hosted
};

/* mingw lacks ILaunchActivatedEventArgs. This ABI declaration follows Microsoft's published generated metadata:
 * https://github.com/microsoft/windows-rs/blob/master/crates/libs/windows/src/Windows/ApplicationModel/Activation/mod.rs
 * (ILaunchActivatedEventArgs_Vtbl: IInspectable, Arguments, TileId; IID fbc93e26-a14a-4b4f-82b0-33bed920af52). */
static const IID launch_iid = {0xfbc93e26, 0xa14a, 0x4b4f, {0x82,0xb0,0x33,0xbe,0xd9,0x20,0xaf,0x52}};
struct launch_interface;
struct launch_vtable {
	HRESULT (WINAPI *QueryInterface)(struct launch_interface *, REFIID, void **);
	ULONG (WINAPI *AddRef)(struct launch_interface *);
	ULONG (WINAPI *Release)(struct launch_interface *);
	HRESULT (WINAPI *GetIids)(struct launch_interface *, ULONG *, IID **);
	HRESULT (WINAPI *GetRuntimeClassName)(struct launch_interface *, HSTRING *);
	HRESULT (WINAPI *GetTrustLevel)(struct launch_interface *, TrustLevel *);
	HRESULT (WINAPI *get_Arguments)(struct launch_interface *, HSTRING *);
	HRESULT (WINAPI *get_TileId)(struct launch_interface *, HSTRING *);
};
struct launch_interface { const struct launch_vtable *lpVtbl; };
struct launch_args {
	IActivatedEventArgs iface;
	LONG refs;
	struct launch_interface launch;
};
static HRESULT WINAPI args_query(IActivatedEventArgs *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID_IActivatedEventArgs))
		*out = iface;
	else if (IsEqualIID(iid, &launch_iid))
		*out = &((struct launch_args *)iface)->launch;
	else return E_NOINTERFACE;
	InterlockedIncrement(&((struct launch_args *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI args_addref(IActivatedEventArgs *iface) { return InterlockedIncrement(&((struct launch_args *)iface)->refs); }
static ULONG WINAPI args_release(IActivatedEventArgs *iface)
{
	struct launch_args *self = (void *)iface;
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return refs;
}
static HRESULT WINAPI args_iids(IActivatedEventArgs *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI args_class(IActivatedEventArgs *iface, HSTRING *out) { return class_string(launch_name, out); }
static HRESULT WINAPI args_trust(IActivatedEventArgs *iface, TrustLevel *out) { return base_trust(out); }
static HRESULT WINAPI args_kind(IActivatedEventArgs *iface, ActivationKind *out)
{
	if (!out) return E_POINTER;
	*out = ActivationKind_Launch;
	return S_OK;
}
static HRESULT WINAPI args_state(IActivatedEventArgs *iface, ApplicationExecutionState *out)
{
	if (!out) return E_POINTER;
	*out = ApplicationExecutionState_NotRunning;
	return S_OK;
}
static HRESULT WINAPI args_splash(IActivatedEventArgs *iface, ISplashScreen **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	return S_OK;
}
static const IActivatedEventArgsVtbl args_table = {
	args_query, args_addref, args_release, args_iids, args_class, args_trust, args_kind, args_state, args_splash
};
static struct launch_args *launch_owner(struct launch_interface *iface) { return CONTAINING_RECORD(iface, struct launch_args, launch); }
static HRESULT WINAPI launch_query(struct launch_interface *iface, REFIID iid, void **out) { return args_query(&launch_owner(iface)->iface, iid, out); }
static ULONG WINAPI launch_addref(struct launch_interface *iface) { return args_addref(&launch_owner(iface)->iface); }
static ULONG WINAPI launch_release(struct launch_interface *iface) { return args_release(&launch_owner(iface)->iface); }
static HRESULT WINAPI launch_iids(struct launch_interface *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI launch_class(struct launch_interface *iface, HSTRING *out) { return class_string(launch_name, out); }
static HRESULT WINAPI launch_trust(struct launch_interface *iface, TrustLevel *out) { return base_trust(out); }
static HRESULT WINAPI launch_arguments(struct launch_interface *iface, HSTRING *out)
{
	/* shortcut: direct framework-view launch has no package activation arguments; use manifest activation data in stage 5. */
	return class_string(L"", out);
}
static HRESULT WINAPI launch_tile(struct launch_interface *iface, HSTRING *out) { return class_string(L"", out); }
static const struct launch_vtable launch_table = {
	launch_query, launch_addref, launch_release, launch_iids, launch_class, launch_trust, launch_arguments, launch_tile
};

struct launch_dispatch {
	IDispatchedHandler iface;
	LONG refs;
	struct application_view *view;
};
static HRESULT WINAPI dispatch_query(IDispatchedHandler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IDispatchedHandler)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct launch_dispatch *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI dispatch_addref(IDispatchedHandler *iface) { return InterlockedIncrement(&((struct launch_dispatch *)iface)->refs); }
static ULONG WINAPI dispatch_release(IDispatchedHandler *iface)
{
	struct launch_dispatch *self = (void *)iface;
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		view_release(&self->view->iface);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return refs;
}
static HRESULT WINAPI dispatch_invoke(IDispatchedHandler *iface)
{
	struct launch_dispatch *self = (void *)iface;
	IUnknown **handlers;
	ULONG count;
	HRESULT result = raven_event_snapshot(&self->view->activated, &handlers, &count);
	if (FAILED(result)) return result;
	struct launch_args *args = ALLOC(sizeof *args);
	if (!args) result = E_OUTOFMEMORY;
	else {
		args->iface.lpVtbl = &args_table;
		args->refs = 1;
		args->launch.lpVtbl = &launch_table;
		InterlockedIncrement(&raven_xaml_objects);
	}
	for (ULONG i = 0; i < count; ++i) {
		if (SUCCEEDED(result))
			result = ((activated_handler *)handlers[i])->lpVtbl->Invoke((activated_handler *)handlers[i], &self->view->iface, &args->iface);
		IUnknown_Release(handlers[i]);
	}
	FREE(handlers);
	if (args) args_release(&args->iface);
	return result;
}
static const IDispatchedHandlerVtbl dispatch_table = {dispatch_query, dispatch_addref, dispatch_release, dispatch_invoke};

struct session {
	struct application_view *view;
	IFrameworkViewSource *source;
	IFrameworkView *framework;
	async_action *activation;
};
static void dispose_session(struct session *session)
{
	TlsSetValue(session_tls, NULL);
	if (session->view) {
		ICoreWindow_Close(session->view->window);
		raven_event_close(&session->view->activated);
	}
	if (session->activation) session->activation->lpVtbl->Release(session->activation);
	if (session->framework) IFrameworkView_Release(session->framework);
	if (session->source) IFrameworkViewSource_Release(session->source);
	if (session->view) view_release(&session->view->iface);
	FREE(session);
	EnterCriticalSection(&application_lock);
	application_thread = 0;
	LeaveCriticalSection(&application_lock);
	InterlockedExchange(&running, 0);
}

static LONG factory_refs = 1;
static IActivationFactory activation_factory;
static ICoreApplication application;
static ICoreApplicationExit application_exit;
static HRESULT factory_query(REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID_IActivationFactory))
		*out = &activation_factory;
	else if (IsEqualIID(iid, &IID_ICoreApplication)) *out = &application;
	else if (IsEqualIID(iid, &IID_ICoreApplicationExit)) *out = &application_exit;
	else return E_NOINTERFACE;
	InterlockedIncrement(&factory_refs);
	return S_OK;
}

static HRESULT WINAPI activation_query(IActivationFactory *iface, REFIID iid, void **out) { return factory_query(iid, out); }
static ULONG WINAPI activation_addref(IActivationFactory *iface) { return InterlockedIncrement(&factory_refs); }
static ULONG WINAPI activation_release(IActivationFactory *iface) { return InterlockedDecrement(&factory_refs); }
static HRESULT WINAPI activation_iids(IActivationFactory *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI activation_class(IActivationFactory *iface, HSTRING *out) { return class_string(application_name, out); }
static HRESULT WINAPI activation_trust(IActivationFactory *iface, TrustLevel *out) { return base_trust(out); }

static HRESULT WINAPI app_query(ICoreApplication *iface, REFIID iid, void **out) { return factory_query(iid, out); }
static ULONG WINAPI app_addref(ICoreApplication *iface) { return InterlockedIncrement(&factory_refs); }
static ULONG WINAPI app_release(ICoreApplication *iface) { return InterlockedDecrement(&factory_refs); }
static HRESULT WINAPI app_iids(ICoreApplication *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI app_class(ICoreApplication *iface, HSTRING *out) { return class_string(application_name, out); }
static HRESULT WINAPI app_trust(ICoreApplication *iface, TrustLevel *out) { return base_trust(out); }

static HRESULT WINAPI exit_query(ICoreApplicationExit *iface, REFIID iid, void **out) { return factory_query(iid, out); }
static ULONG WINAPI exit_addref(ICoreApplicationExit *iface) { return InterlockedIncrement(&factory_refs); }
static ULONG WINAPI exit_release(ICoreApplicationExit *iface) { return InterlockedDecrement(&factory_refs); }
static HRESULT WINAPI exit_iids(ICoreApplicationExit *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI exit_class(ICoreApplicationExit *iface, HSTRING *out) { return class_string(application_name, out); }
static HRESULT WINAPI exit_trust(ICoreApplicationExit *iface, TrustLevel *out) { return base_trust(out); }

static HRESULT WINAPI activation_instance(IActivationFactory *iface, IInspectable **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	return E_ILLEGAL_METHOD_CALL;
}
static HRESULT WINAPI app_id(ICoreApplication *iface, HSTRING *out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	UINT32 length = 0;
	LONG error = GetCurrentApplicationUserModelId(&length, NULL);
	if (error != ERROR_INSUFFICIENT_BUFFER) return HRESULT_FROM_WIN32(error);
	WCHAR *text = ALLOC((SIZE_T)length * sizeof *text);
	if (!text) return E_OUTOFMEMORY;
	error = GetCurrentApplicationUserModelId(&length, text);
	HRESULT result = error ? HRESULT_FROM_WIN32(error) : class_string(text, out);
	FREE(text);
	return result;
}
static HRESULT WINAPI app_add_suspending(ICoreApplication *iface,
	__FIEventHandler_1_Windows__CApplicationModel__CSuspendingEventArgs *handler, EventRegistrationToken *token)
{
	if (!token || !handler) return E_POINTER;
	token->value = 0;
	return E_NOTIMPL;
}
static HRESULT WINAPI app_remove_suspending(ICoreApplication *iface, EventRegistrationToken token) { return E_NOTIMPL; }
static HRESULT WINAPI app_add_resuming(ICoreApplication *iface, exiting_handler *handler, EventRegistrationToken *token)
{
	if (!token || !handler) return E_POINTER;
	token->value = 0;
	return E_NOTIMPL;
}
static HRESULT WINAPI app_remove_resuming(ICoreApplication *iface, EventRegistrationToken token) { return E_NOTIMPL; }
static HRESULT WINAPI app_properties(ICoreApplication *iface, __x_ABI_CWindows_CFoundation_CCollections_CIPropertySet **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	return E_NOTIMPL;
}
static HRESULT WINAPI app_current(ICoreApplication *iface, ICoreApplicationView **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	struct session *session = session_tls == TLS_OUT_OF_INDEXES ? NULL : TlsGetValue(session_tls);
	if (!session || !session->view) return E_ILLEGAL_METHOD_CALL;
	*out = &session->view->iface;
	view_addref(*out);
	return S_OK;
}
static HRESULT queue_activation(struct session *session)
{
	struct launch_dispatch *dispatch = ALLOC(sizeof *dispatch);
	if (!dispatch) return E_OUTOFMEMORY;
	dispatch->iface.lpVtbl = &dispatch_table;
	dispatch->refs = 1;
	dispatch->view = session->view;
	view_addref(&session->view->iface);
	InterlockedIncrement(&raven_xaml_objects);
	ICoreDispatcher *dispatcher = NULL;
	HRESULT result = ICoreWindow_get_Dispatcher(session->view->window, &dispatcher);
	if (SUCCEEDED(result)) {
		result = ICoreDispatcher_RunAsync(dispatcher, CoreDispatcherPriority_High, &dispatch->iface, &session->activation);
		ICoreDispatcher_Release(dispatcher);
	}
	dispatch_release(&dispatch->iface);
	return result;
}
static HRESULT notify_exiting(void)
{
	IUnknown **handlers;
	ULONG count;
	HRESULT result = raven_event_snapshot(&exiting, &handlers, &count);
	if (FAILED(result)) return result;
	for (ULONG i = 0; i < count; ++i) {
		if (SUCCEEDED(result))
			result = ((exiting_handler *)handlers[i])->lpVtbl->Invoke((exiting_handler *)handlers[i], (IInspectable *)&activation_factory, NULL);
		IUnknown_Release(handlers[i]);
	}
	FREE(handlers);
	return result;
}
static HRESULT WINAPI app_run(ICoreApplication *iface, IFrameworkViewSource *source)
{
	if (!source) return E_POINTER;
	if (session_tls == TLS_OUT_OF_INDEXES) return E_OUTOFMEMORY;
	if (InterlockedCompareExchange(&running, 1, 0)) return E_ILLEGAL_METHOD_CALL;
	struct session *session = ALLOC(sizeof *session);
	if (!session) {
		InterlockedExchange(&running, 0);
		return E_OUTOFMEMORY;
	}
	IFrameworkViewSource_AddRef(source);
	session->source = source;
	struct application_view *view = ALLOC(sizeof *view);
	HRESULT result = view ? raven_current_window(&view->window) : E_OUTOFMEMORY;
	if (FAILED(result)) {
		FREE(view);
		dispose_session(session);
		return result;
	}
	view->iface.lpVtbl = &view_table;
	view->refs = 1;
	InitializeCriticalSection(&view->activated.lock);
	InterlockedIncrement(&raven_xaml_objects);
	session->view = view;
	if (!TlsSetValue(session_tls, session)) {
		dispose_session(session);
		return E_OUTOFMEMORY;
	}
	MSG message;
	PeekMessageW(&message, NULL, 0, 0, PM_NOREMOVE);
	EnterCriticalSection(&application_lock);
	application_thread = GetCurrentThreadId();
	LeaveCriticalSection(&application_lock);
	BOOL initialized = FALSE;
	result = IFrameworkViewSource_CreateView(source, &session->framework);
	if (SUCCEEDED(result) && !session->framework) result = E_UNEXPECTED;
	if (SUCCEEDED(result)) {
		result = IFrameworkView_Initialize(session->framework, &view->iface);
		initialized = SUCCEEDED(result);
	}
	if (SUCCEEDED(result)) result = IFrameworkView_SetWindow(session->framework, view->window);
	/* shortcut: a directly supplied view source has no manifest entry-point string; package activation supplies it in stage 5. */
	if (SUCCEEDED(result)) result = IFrameworkView_Load(session->framework, NULL);
	if (SUCCEEDED(result)) result = queue_activation(session);
	if (SUCCEEDED(result)) result = IFrameworkView_Run(session->framework);
	ICoreWindow_Close(view->window);
	if (session->activation) {
		HRESULT activated = session->activation->lpVtbl->GetResults(session->activation);
		if (SUCCEEDED(result) && FAILED(activated)) result = activated;
	}
	if (initialized) {
		HRESULT cleanup = IFrameworkView_Uninitialize(session->framework);
		if (SUCCEEDED(result)) result = cleanup;
		cleanup = notify_exiting();
		if (SUCCEEDED(result)) result = cleanup;
	}
	dispose_session(session);
	return result;
}
static HRESULT WINAPI app_run_factories(ICoreApplication *iface, __x_ABI_CWindows_CFoundation_CIGetActivationFactory *factory)
{
	return factory ? E_NOTIMPL : E_POINTER;
}
static HRESULT WINAPI exit_application(ICoreApplicationExit *iface)
{
	EnterCriticalSection(&application_lock);
	DWORD thread = application_thread;
	LeaveCriticalSection(&application_lock);
	if (!thread) return E_ILLEGAL_METHOD_CALL;
	return PostThreadMessageW(thread, WM_QUIT, 0, 0) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}
static HRESULT WINAPI exit_add(ICoreApplicationExit *iface, exiting_handler *handler, EventRegistrationToken *token)
{
	return raven_event_add(&exiting, (IUnknown *)handler, token);
}
static HRESULT WINAPI exit_remove(ICoreApplicationExit *iface, EventRegistrationToken token)
{
	return raven_event_remove(&exiting, token);
}
static const IActivationFactoryVtbl activation_table = {
	activation_query, activation_addref, activation_release, activation_iids, activation_class, activation_trust, activation_instance
};
static const ICoreApplicationVtbl application_table = {
	app_query, app_addref, app_release, app_iids, app_class, app_trust, app_id,
	app_add_suspending, app_remove_suspending, app_add_resuming, app_remove_resuming, app_properties,
	app_current, app_run, app_run_factories
};
static const ICoreApplicationExitVtbl exit_table = {
	exit_query, exit_addref, exit_release, exit_iids, exit_class, exit_trust, exit_application, exit_add, exit_remove
};
static IActivationFactory activation_factory = {&activation_table};
static ICoreApplication application = {&application_table};
static ICoreApplicationExit application_exit = {&exit_table};

HRESULT raven_application_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length;
	const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	if (length != ARRAYSIZE(application_name) - 1 ||
		CompareStringOrdinal(text, (int)length, application_name, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	return factory_query(&IID_IActivationFactory, (void **)out);
}
BOOL raven_application_notify(DWORD reason, void *reserved)
{
	if (reason == DLL_PROCESS_ATTACH) {
		session_tls = TlsAlloc();
		if (session_tls == TLS_OUT_OF_INDEXES) return FALSE;
		InitializeCriticalSection(&application_lock);
		InitializeCriticalSection(&exiting.lock);
	} else if (reason == DLL_THREAD_DETACH || (reason == DLL_PROCESS_DETACH && !reserved)) {
		struct session *session = session_tls == TLS_OUT_OF_INDEXES ? NULL : TlsGetValue(session_tls);
		if (session) dispose_session(session);
	}
	if (reason == DLL_PROCESS_DETACH && session_tls != TLS_OUT_OF_INDEXES) {
		if (!reserved) raven_event_close(&exiting);
		DeleteCriticalSection(&exiting.lock);
		DeleteCriticalSection(&application_lock);
		TlsFree(session_tls);
		session_tls = TLS_OUT_OF_INDEXES;
	}
	return TRUE;
}
