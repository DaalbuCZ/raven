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
#include <windows.foundation.h>
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

/* engine-internal, package.c: the package-relative application id the manifest beside the image declares, or nothing
 * when the process carries no manifest identity */
BOOL raven_package_application_id(const WCHAR **out);
/* engine-internal, package.c: the runtime class name the manifest's Applications/Application@EntryPoint declares, or
 * nothing when the process carries no manifest identity or the manifest declares no EntryPoint */
BOOL raven_package_application_entry_point(const WCHAR **out);

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
	/* .NET Native takes a weak reference on every WinRT object it projects and treats its absence as a hard error; the
	 * shared helper (xaml_internal.h) embeds the source part here and the view disconnects it on release. */
	struct raven_weak_source weak;
};
static HRESULT WINAPI view_query(ICoreApplicationView *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	/* The view is thread-affine — it wraps the calling UI thread's CoreWindow and its events fire on that thread — so the
	 * IAgileObject marker stays refused; IMarshal is not implemented anywhere in the component (xaml_internal.h). */
	if (IsEqualIID(iid, &IID_IWeakReferenceSource)) {
		*out = &((struct application_view *)iface)->weak;
		InterlockedIncrement(&((struct application_view *)iface)->refs);
		return S_OK;
	}
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
		raven_weak_source_disconnect(&self->weak);
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
static const IID prelaunch_iid = {0x0c44717b, 0x19f7, 0x48d6, {0xb0,0x46,0xcf,0x22,0x82,0x6e,0xaa,0x74}};
struct prelaunch_interface;
struct prelaunch_vtable {
	HRESULT (WINAPI *QueryInterface)(struct prelaunch_interface *, REFIID, void **);
	ULONG (WINAPI *AddRef)(struct prelaunch_interface *);
	ULONG (WINAPI *Release)(struct prelaunch_interface *);
	HRESULT (WINAPI *GetIids)(struct prelaunch_interface *, ULONG *, IID **);
	HRESULT (WINAPI *GetRuntimeClassName)(struct prelaunch_interface *, HSTRING *);
	HRESULT (WINAPI *GetTrustLevel)(struct prelaunch_interface *, TrustLevel *);
	HRESULT (WINAPI *get_PrelaunchActivated)(struct prelaunch_interface *, boolean *);
};
struct prelaunch_interface { const struct prelaunch_vtable *lpVtbl; };
struct launch_args {
	IActivatedEventArgs iface;
	LONG refs;
	struct launch_interface launch;
	struct prelaunch_interface prelaunch;
	struct raven_weak_source weak;
};
static HRESULT WINAPI args_query(IActivatedEventArgs *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID_IActivatedEventArgs))
		*out = iface;
	else if (IsEqualIID(iid, &launch_iid))
		*out = &((struct launch_args *)iface)->launch;
	else if (IsEqualIID(iid, &prelaunch_iid))
		*out = &((struct launch_args *)iface)->prelaunch;
	else if (IsEqualIID(iid, &IID_IWeakReferenceSource))
		*out = &((struct launch_args *)iface)->weak;
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
		raven_weak_source_disconnect(&self->weak);
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
	/* Launch arguments are the activation contract's argument string (a tile or protocol activation carries one); a
	 * framework-view launch over CoreApplication.Run receives none, so the answer stays empty. The manifest's
	 * Applications/Application@EntryPoint is not this string: it travels to the view through IFrameworkView::Load. */
	return class_string(L"", out);
}
static HRESULT WINAPI launch_tile(struct launch_interface *iface, HSTRING *out) { return class_string(L"", out); }
static const struct launch_vtable launch_table = {
	launch_query, launch_addref, launch_release, launch_iids, launch_class, launch_trust, launch_arguments, launch_tile
};

static struct launch_args *prelaunch_owner(struct prelaunch_interface *iface)
{ return CONTAINING_RECORD(iface, struct launch_args, prelaunch); }
static HRESULT WINAPI prelaunch_query(struct prelaunch_interface *iface, REFIID iid, void **out)
{ return args_query(&prelaunch_owner(iface)->iface, iid, out); }
static ULONG WINAPI prelaunch_addref(struct prelaunch_interface *iface) { return args_addref(&prelaunch_owner(iface)->iface); }
static ULONG WINAPI prelaunch_release(struct prelaunch_interface *iface) { return args_release(&prelaunch_owner(iface)->iface); }
static HRESULT WINAPI prelaunch_iids(struct prelaunch_interface *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI prelaunch_class(struct prelaunch_interface *iface, HSTRING *out) { return class_string(launch_name, out); }
static HRESULT WINAPI prelaunch_trust(struct prelaunch_interface *iface, TrustLevel *out) { return base_trust(out); }
static HRESULT WINAPI prelaunch_get(struct prelaunch_interface *iface, boolean *out)
{ if (!out) return E_POINTER; *out = FALSE; return S_OK; }
static const struct prelaunch_vtable prelaunch_table = {
	prelaunch_query, prelaunch_addref, prelaunch_release, prelaunch_iids, prelaunch_class, prelaunch_trust, prelaunch_get
};

HRESULT raven_launch_arguments_create(IInspectable **out)
{
	struct launch_args *args;
	if (!out) return E_POINTER;
	*out = NULL;
	args = ALLOC(sizeof *args);
	if (!args) return E_OUTOFMEMORY;
	args->iface.lpVtbl = &args_table;
	args->refs = 1;
	args->launch.lpVtbl = &launch_table;
	args->prelaunch.lpVtbl = &prelaunch_table;
	raven_weak_source_init(&args->weak, args, &args->refs, (raven_weak_query_fn)args_query);
	InterlockedIncrement(&raven_xaml_objects);
	*out = (IInspectable *)&args->launch;
	return S_OK;
}

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
		args->prelaunch.lpVtbl = &prelaunch_table;
		raven_weak_source_init(&args->weak, args, &args->refs, (raven_weak_query_fn)args_query);
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
/* the weak source the runtime takes on the singleton; the target is created at DLL attach and never disconnects, because
 * CoreApplication lives for the process the way its factory does */
static struct raven_weak_source core_application_weak;
static HRESULT factory_query(REFIID iid, void **out);
static HRESULT WINAPI core_application_weak_query(void *owner, REFIID iid, void **out)
{
	(void)owner;
	return factory_query(iid, out);
}
static HRESULT factory_query(REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID_IActivationFactory))
		*out = &activation_factory;
	else if (IsEqualIID(iid, &IID_ICoreApplication)) *out = &application;
	else if (IsEqualIID(iid, &IID_ICoreApplicationExit)) *out = &application_exit;
	else if (IsEqualIID(iid, &IID_IWeakReferenceSource)) *out = &core_application_weak;
	else if (IsEqualIID(iid, &IID_IAgileObject))
		/* Agile: the singleton is stateless over interlocked/lock-guarded startup state (Run synchronizes through
		 * application_lock and atomics), it outlives every reference, so its interfaces are callable from any
		 * apartment. IMarshal stays unimplemented (xaml_internal.h). */
		*out = &application;
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

/* The property set. CoreApplication.Properties hands out one process-wide instance and the class activation creates
 * independent ones; every instance owns its entries, its refcount and its own critical section, so two activated
 * results share nothing. The class is claimed: GetRuntimeClassName answers the runtime class name on every part. */
static const WCHAR property_set_name[] = L"Windows.Foundation.Collections.PropertySet";

struct property_entry {
	HSTRING key;
	IInspectable *value;
};
struct property_set_object {
	__x_ABI_CWindows_CFoundation_CCollections_CIPropertySet base;
	__FIMap_2_HSTRING_IInspectable map;
	__FIMapView_2_HSTRING_IInspectable view;
	LONG refs;
	BOOL tracked;	/* activation-created instances count against DllCanUnloadNow; the Properties instance does not */
	CRITICAL_SECTION lock;
	struct property_entry *entries;
	UINT32 count;
	UINT32 capacity;
	/* the weak source .NET Native takes on every projected WinRT object (xaml_internal.h) */
	struct raven_weak_source weak;
};
static struct property_set_object *property_set_properties;

static HRESULT WINAPI property_base_query(__x_ABI_CWindows_CFoundation_CCollections_CIPropertySet *iface, REFIID iid, void **out);
static ULONG WINAPI property_base_addref(__x_ABI_CWindows_CFoundation_CCollections_CIPropertySet *iface);
static ULONG WINAPI property_base_release(__x_ABI_CWindows_CFoundation_CCollections_CIPropertySet *iface);
static HRESULT WINAPI property_base_iids(__x_ABI_CWindows_CFoundation_CCollections_CIPropertySet *iface, ULONG *count, IID **iids);
static HRESULT WINAPI property_base_class(__x_ABI_CWindows_CFoundation_CCollections_CIPropertySet *iface, HSTRING *out);
static HRESULT WINAPI property_base_trust(__x_ABI_CWindows_CFoundation_CCollections_CIPropertySet *iface, TrustLevel *out);
static HRESULT WINAPI property_map_query(__FIMap_2_HSTRING_IInspectable *iface, REFIID iid, void **out);
static ULONG WINAPI property_map_addref(__FIMap_2_HSTRING_IInspectable *iface);
static ULONG WINAPI property_map_release(__FIMap_2_HSTRING_IInspectable *iface);
static HRESULT WINAPI property_map_iids(__FIMap_2_HSTRING_IInspectable *iface, ULONG *count, IID **iids);
static HRESULT WINAPI property_map_class(__FIMap_2_HSTRING_IInspectable *iface, HSTRING *out);
static HRESULT WINAPI property_map_trust(__FIMap_2_HSTRING_IInspectable *iface, TrustLevel *out);
static HRESULT WINAPI property_view_query(__FIMapView_2_HSTRING_IInspectable *iface, REFIID iid, void **out);
static ULONG WINAPI property_view_addref(__FIMapView_2_HSTRING_IInspectable *iface);
static ULONG WINAPI property_view_release(__FIMapView_2_HSTRING_IInspectable *iface);
static HRESULT WINAPI property_view_iids(__FIMapView_2_HSTRING_IInspectable *iface, ULONG *count, IID **iids);
static HRESULT WINAPI property_view_class(__FIMapView_2_HSTRING_IInspectable *iface, HSTRING *out);
static HRESULT WINAPI property_view_trust(__FIMapView_2_HSTRING_IInspectable *iface, TrustLevel *out);

static const __x_ABI_CWindows_CFoundation_CCollections_CIPropertySetVtbl property_base_table = {
	property_base_query, property_base_addref, property_base_release,
	property_base_iids, property_base_class, property_base_trust
};
static const __FIMap_2_HSTRING_IInspectableVtbl property_map_table;
static const __FIMapView_2_HSTRING_IInspectableVtbl property_view_table;

static ULONG property_addref(struct property_set_object *self)
{
	return InterlockedIncrement(&self->refs);
}
static void property_entry_dispose(struct property_entry *entry);
/* WINAPI so the weak-reference helper can reach it through its function-pointer type on x86 (the two conventions differ
 * there); the parts call it directly as before. */
static HRESULT WINAPI property_query(struct property_set_object *self, REFIID iid, void **out);
static ULONG property_release(struct property_set_object *self)
{
	ULONG remaining = InterlockedDecrement(&self->refs);
	if (!remaining) {
		raven_weak_source_disconnect(&self->weak);
		for (UINT32 i = 0; i < self->count; ++i)
			property_entry_dispose(&self->entries[i]);
		if (self->entries) FREE(self->entries);
		DeleteCriticalSection(&self->lock);
		if (self->tracked) InterlockedDecrement(&raven_xaml_objects);
		FREE(self);
	}
	return remaining;
}
static HRESULT WINAPI property_query(struct property_set_object *self, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IWeakReferenceSource)) {
		*out = &self->weak;
	} else if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) ||
		IsEqualIID(iid, &IID___x_ABI_CWindows_CFoundation_CCollections_CIPropertySet))
		*out = &self->base;
	else if (IsEqualIID(iid, &IID___FIMap_2_HSTRING_IInspectable))
		*out = &self->map;
	else if (IsEqualIID(iid, &IID___FIMapView_2_HSTRING_IInspectable))
		*out = &self->view;
	else if (IsEqualIID(iid, &IID_IAgileObject))
		/* Agile: every member guards the entry array with the instance's critical section, the reference count is
		 * interlocked and no field is read outside the lock, so the set is genuinely free-threaded. IMarshal stays
		 * unimplemented (xaml_internal.h). */
		*out = &self->base;
	else
		return E_NOINTERFACE;
	property_addref(self);
	return S_OK;
}

/* The three parts forward their lifetime to the object, whose base their member offset reaches. */
#define PROPERTY_PART_LIFETIME(part, iface_type, member) \
	static HRESULT WINAPI part##_query(iface_type *iface, REFIID iid, void **out) \
	{ return property_query(CONTAINING_RECORD(iface, struct property_set_object, member), iid, out); } \
	static ULONG WINAPI part##_addref(iface_type *iface) \
	{ return property_addref(CONTAINING_RECORD(iface, struct property_set_object, member)); } \
	static ULONG WINAPI part##_release(iface_type *iface) \
	{ return property_release(CONTAINING_RECORD(iface, struct property_set_object, member)); } \
	static HRESULT WINAPI part##_iids(iface_type *iface, ULONG *count, IID **iids) \
	{ (void)iface; return no_iids(count, iids); } \
	static HRESULT WINAPI part##_class(iface_type *iface, HSTRING *out) \
	{ (void)iface; return class_string(property_set_name, out); } \
	static HRESULT WINAPI part##_trust(iface_type *iface, TrustLevel *out) \
	{ (void)iface; return base_trust(out); }

PROPERTY_PART_LIFETIME(property_base, __x_ABI_CWindows_CFoundation_CCollections_CIPropertySet, base)
PROPERTY_PART_LIFETIME(property_map, __FIMap_2_HSTRING_IInspectable, map)
PROPERTY_PART_LIFETIME(property_view, __FIMapView_2_HSTRING_IInspectable, view)

/* String keys compare by content; a NULL HSTRING is the empty key the way every HSTRING argument reads. */
static BOOL property_key_equal(HSTRING left, HSTRING right)
{
	INT order = 0;
	if (left == right) return TRUE;
	if (!left || !right) return FALSE;
	if (FAILED(WindowsCompareStringOrdinal(left, right, &order))) return FALSE;
	return order == 0;
}
static struct property_entry *property_find(struct property_set_object *self, HSTRING key)
{
	for (UINT32 i = 0; i < self->count; ++i)
		if (property_key_equal(self->entries[i].key, key))
			return &self->entries[i];
	return NULL;
}
static void property_entry_dispose(struct property_entry *entry)
{
	if (entry->key) WindowsDeleteString(entry->key);
	if (entry->value) entry->value->lpVtbl->Release(entry->value);
}

static HRESULT property_lookup_impl(struct property_set_object *self, HSTRING key, IInspectable **value)
{
	struct property_entry *entry;
	HRESULT result;
	if (!value) return E_POINTER;
	*value = NULL;
	EnterCriticalSection(&self->lock);
	entry = property_find(self, key);
	if (entry && entry->value) {
		*value = entry->value;
		(*value)->lpVtbl->AddRef(*value);
	}
	result = entry ? S_OK : E_BOUNDS;
	LeaveCriticalSection(&self->lock);
	return result;
}
static HRESULT property_size_impl(struct property_set_object *self, UINT32 *value)
{
	if (!value) return E_POINTER;
	EnterCriticalSection(&self->lock);
	*value = self->count;
	LeaveCriticalSection(&self->lock);
	return S_OK;
}
static HRESULT property_has_key_impl(struct property_set_object *self, HSTRING key, boolean *found)
{
	if (!found) return E_POINTER;
	EnterCriticalSection(&self->lock);
	*found = property_find(self, key) ? TRUE : FALSE;
	LeaveCriticalSection(&self->lock);
	return S_OK;
}

static HRESULT WINAPI property_map_lookup(__FIMap_2_HSTRING_IInspectable *iface, HSTRING key, IInspectable **value)
{
	return property_lookup_impl(CONTAINING_RECORD(iface, struct property_set_object, map), key, value);
}
static HRESULT WINAPI property_map_size(__FIMap_2_HSTRING_IInspectable *iface, UINT32 *value)
{
	return property_size_impl(CONTAINING_RECORD(iface, struct property_set_object, map), value);
}
static HRESULT WINAPI property_map_has_key(__FIMap_2_HSTRING_IInspectable *iface, HSTRING key, boolean *found)
{
	return property_has_key_impl(CONTAINING_RECORD(iface, struct property_set_object, map), key, found);
}
static HRESULT WINAPI property_map_get_view(__FIMap_2_HSTRING_IInspectable *iface, __FIMapView_2_HSTRING_IInspectable **view)
{
	struct property_set_object *self = CONTAINING_RECORD(iface, struct property_set_object, map);
	if (!view) return E_POINTER;
	/* the view reads the same storage, so it sees later mutations the way the live map does */
	*view = &self->view;
	property_addref(self);
	return S_OK;
}
static HRESULT WINAPI property_map_insert(__FIMap_2_HSTRING_IInspectable *iface, HSTRING key, IInspectable *value,
	boolean *replaced)
{
	struct property_set_object *self = CONTAINING_RECORD(iface, struct property_set_object, map);
	struct property_entry *entry;
	HRESULT result = S_OK;
	if (replaced) *replaced = FALSE;
	EnterCriticalSection(&self->lock);
	entry = property_find(self, key);
	if (entry) {
		if (replaced) *replaced = TRUE;
		if (value) value->lpVtbl->AddRef(value);
		if (entry->value) entry->value->lpVtbl->Release(entry->value);
		entry->value = value;
	} else {
		if (self->count == self->capacity) {
			const UINT32 capacity = self->capacity ? self->capacity * 2 : 4;
			struct property_entry *entries = ALLOC((SIZE_T)capacity * sizeof *entries);
			if (!entries) {
				result = E_OUTOFMEMORY;
			} else {
				if (self->entries) {
					memcpy(entries, self->entries, (SIZE_T)self->count * sizeof *entries);
					FREE(self->entries);
				}
				self->entries = entries;
				self->capacity = capacity;
			}
		}
		if (SUCCEEDED(result)) {
			UINT32 length = 0;
			const WCHAR *text = key ? WindowsGetStringRawBuffer(key, &length) : NULL;
			HSTRING stored = NULL;
			/* the map owns its keys: an empty key normalizes to the NULL handle WindowsCreateString answers */
			result = WindowsCreateString(text, length, &stored);
			if (SUCCEEDED(result)) {
				self->entries[self->count].key = stored;
				self->entries[self->count].value = value;
				if (value) value->lpVtbl->AddRef(value);
				++self->count;
			}
		}
	}
	LeaveCriticalSection(&self->lock);
	return result;
}
static HRESULT WINAPI property_map_remove(__FIMap_2_HSTRING_IInspectable *iface, HSTRING key)
{
	struct property_set_object *self = CONTAINING_RECORD(iface, struct property_set_object, map);
	HRESULT result = E_BOUNDS;
	EnterCriticalSection(&self->lock);
	for (UINT32 i = 0; i < self->count; ++i) {
		if (!property_key_equal(self->entries[i].key, key)) continue;
		property_entry_dispose(&self->entries[i]);
		for (UINT32 rest = i; rest + 1 < self->count; ++rest)
			self->entries[rest] = self->entries[rest + 1];
		--self->count;
		result = S_OK;
		break;
	}
	LeaveCriticalSection(&self->lock);
	return result;
}
static HRESULT WINAPI property_map_clear(__FIMap_2_HSTRING_IInspectable *iface)
{
	struct property_set_object *self = CONTAINING_RECORD(iface, struct property_set_object, map);
	EnterCriticalSection(&self->lock);
	for (UINT32 i = 0; i < self->count; ++i)
		property_entry_dispose(&self->entries[i]);
	self->count = 0;
	LeaveCriticalSection(&self->lock);
	return S_OK;
}

static HRESULT WINAPI property_view_lookup(__FIMapView_2_HSTRING_IInspectable *iface, HSTRING key, IInspectable **value)
{
	return property_lookup_impl(CONTAINING_RECORD(iface, struct property_set_object, view), key, value);
}
static HRESULT WINAPI property_view_size(__FIMapView_2_HSTRING_IInspectable *iface, UINT32 *value)
{
	return property_size_impl(CONTAINING_RECORD(iface, struct property_set_object, view), value);
}
static HRESULT WINAPI property_view_has_key(__FIMapView_2_HSTRING_IInspectable *iface, HSTRING key, boolean *found)
{
	return property_has_key_impl(CONTAINING_RECORD(iface, struct property_set_object, view), key, found);
}
static HRESULT WINAPI property_view_split(__FIMapView_2_HSTRING_IInspectable *iface,
	__FIMapView_2_HSTRING_IInspectable **first, __FIMapView_2_HSTRING_IInspectable **second)
{
	(void)iface;
	if (first) *first = NULL;
	if (second) *second = NULL;
	return E_NOTIMPL;
}

static const __FIMap_2_HSTRING_IInspectableVtbl property_map_table = {
	property_map_query, property_map_addref, property_map_release,
	property_map_iids, property_map_class, property_map_trust,
	property_map_lookup, property_map_size, property_map_has_key,
	property_map_get_view, property_map_insert, property_map_remove, property_map_clear
};
static const __FIMapView_2_HSTRING_IInspectableVtbl property_view_table = {
	property_view_query, property_view_addref, property_view_release,
	property_view_iids, property_view_class, property_view_trust,
	property_view_lookup, property_view_size, property_view_has_key, property_view_split
};

/* Every instance — the process-wide Properties one and each activation result — starts with the one reference its
 * creator holds; tracked instances balance DllCanUnloadNow, which the Properties instance must never block. */
static HRESULT property_set_create(struct property_set_object **out, BOOL tracked)
{
	struct property_set_object *self = ALLOC(sizeof *self);
	if (!self) return E_OUTOFMEMORY;
	self->base.lpVtbl = &property_base_table;
	self->map.lpVtbl = &property_map_table;
	self->view.lpVtbl = &property_view_table;
	self->refs = 1;
	self->tracked = tracked;
	self->entries = NULL;
	self->count = 0;
	self->capacity = 0;
	InitializeCriticalSection(&self->lock);
	raven_weak_source_init(&self->weak, self, &self->refs, (raven_weak_query_fn)property_query);
	if (tracked) InterlockedIncrement(&raven_xaml_objects);
	*out = self;
	return S_OK;
}

/* The class factory. PropertySet is activatable: ActivateInstance really creates, and two results share nothing. */
static LONG property_factory_refs;
static IActivationFactory property_factory;
static HRESULT WINAPI property_factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) ||
		IsEqualIID(iid, &IID_IActivationFactory))
		*out = &property_factory;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&property_factory_refs);
	return S_OK;
}
static ULONG WINAPI property_factory_addref(IActivationFactory *iface) { (void)iface; return InterlockedIncrement(&property_factory_refs); }
static ULONG WINAPI property_factory_release(IActivationFactory *iface) { (void)iface; return InterlockedDecrement(&property_factory_refs); }
static HRESULT WINAPI property_factory_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{ (void)iface; return no_iids(count, iids); }
static HRESULT WINAPI property_factory_class(IActivationFactory *iface, HSTRING *out)
{ (void)iface; return class_string(property_set_name, out); }
static HRESULT WINAPI property_factory_trust(IActivationFactory *iface, TrustLevel *out)
{ (void)iface; return base_trust(out); }
static HRESULT WINAPI property_factory_activate(IActivationFactory *iface, IInspectable **out)
{
	struct property_set_object *self;
	HRESULT result;
	(void)iface;
	if (!out) return E_POINTER;
	*out = NULL;
	result = property_set_create(&self, TRUE);
	if (FAILED(result)) return result;
	*out = (IInspectable *)&self->base;
	return S_OK;
}
static const IActivationFactoryVtbl property_factory_table = {
	property_factory_query, property_factory_addref, property_factory_release,
	property_factory_iids, property_factory_class, property_factory_trust, property_factory_activate
};
static IActivationFactory property_factory = {&property_factory_table};

/* The activation-chain entry, exported so a guest reaches the factory before the class's catalog entry lands. */
__declspec(dllexport) HRESULT raven_property_set_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out) return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (!text || length != ARRAYSIZE(property_set_name) - 1 ||
		CompareStringOrdinal(text, (int)length, property_set_name, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	InterlockedIncrement(&property_factory_refs);
	*out = &property_factory;
	return S_OK;
}

static HRESULT WINAPI app_properties(ICoreApplication *iface, __x_ABI_CWindows_CFoundation_CCollections_CIPropertySet **out)
{
	(void)iface;
	if (!out) return E_POINTER;
	*out = &property_set_properties->base;
	property_addref(property_set_properties);
	return S_OK;
}

static HRESULT WINAPI app_id(ICoreApplication *iface, HSTRING *out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	/* with package identity the Id is the package-relative application id the manifest declares */
	const WCHAR *praid = NULL;
	if (raven_package_application_id(&praid)) return class_string(praid, out);
	/* no package identity: forward the host's answer */
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
	raven_weak_source_init(&view->weak, view, &view->refs, (raven_weak_query_fn)view_query);
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
	/* Manifest-driven activation data: a packaged activation carries the manifest's Applications/Application@EntryPoint
	 * into Load, and the emulated desktop supplies it from the manifest beside the image the same way. A process without
	 * a manifest entry point loads with no string, which the view-source case already answered. The EntryPoint class
	 * itself is the guest application's own XAML Application subclass: the component cannot serve it by activation (the
	 * object comes from the guest runtime through Application.Start), so the name travels only as this argument. */
	HSTRING entry_point = NULL;
	if (SUCCEEDED(result)) {
		const WCHAR *entry = NULL;
		if (raven_package_application_entry_point(&entry))
			result = class_string(entry, &entry_point);
	}
	if (SUCCEEDED(result)) result = IFrameworkView_Load(session->framework, entry_point);
	if (entry_point) WindowsDeleteString(entry_point);
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
	/* RunWithActivationFactories is the packaged/COM-launch startup entry: the activation host delivers the launch
	 * arguments, then asks the callback's GetActivationFactory for the application's entry-point class while the
	 * method serves requests on the UI thread until shutdown. The emulated desktop honestly supports none of that
	 * chain: the harness is an ordinary Win32 process, no COM surrogate or packaged launch delivers activation
	 * arguments, so no host exists to pull factories through the callback and no part of the flow is probeable
	 * here. The view-source path Run drives cannot be entered from this route either: Run needs an
	 * IFrameworkViewSource this route's caller never supplies, and the real flow obtains the view through the
	 * activation the desktop cannot deliver. The only class name the engine could honestly ask the callback for is
	 * the manifest's Applications/Application@EntryPoint, which a process without a manifest identity does not
	 * carry (identity is read once per process, so the fixture cannot supply one without losing the no-package Id
	 * branch it already covers). What the ABI documents and the fixture can probe stays: a NULL callback is the
	 * projection's ArgumentNullException and answers E_POINTER like Run's NULL source, and a call while Run owns
	 * the UI thread is the same rejected concurrent startup E_ILLEGAL_METHOD_CALL that a nested Run gets. The run
	 * itself waits for a packaged guest and answers E_NOTIMPL without retaining the callback. */
	if (!factory) return E_POINTER;
	if (InterlockedCompareExchange(&running, 0, 0)) return E_ILLEGAL_METHOD_CALL;
	return E_NOTIMPL;
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
		raven_weak_source_init(&core_application_weak, &application, &factory_refs, core_application_weak_query);
		/* the process-wide Properties instance is created once here and holds its own reference for the engine's life */
		if (FAILED(property_set_create(&property_set_properties, FALSE))) {
			DeleteCriticalSection(&exiting.lock);
			DeleteCriticalSection(&application_lock);
			TlsFree(session_tls);
			session_tls = TLS_OUT_OF_INDEXES;
			return FALSE;
		}
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
