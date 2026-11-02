/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.UI.Xaml.Application class of the Windows Runtime.
 *
 * The class is what a XAML application derives from. This file provides its activation: the factory a program asks for by name,
 * the statics of the class, the composable factory and an application object with the three interfaces the type carries. The
 * implements application composition, current and startup, resource storage, and lifecycle event registration. Suspend and
 * resume notifications are not delivered by the desktop host yet; other engine-dependent members remain unimplemented.
 *
 * The object is not a rendering surface: composition and the engine stages plug into the same factory later. */
#define INITGUID
#define CONST_VTABLE
#define WIDL_using_Windows_UI_Core
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include "windows_ui_xaml_application.h"
#include "windows_ui_xaml_application_overrides.h"
#include "windows_ui_xaml_window_created.h"
#include "events.h"
#include <windows.foundation.h>

#define E_ILLEGAL_METHOD_CALL ((HRESULT)0x8000000e)
#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

typedef __x_ABI_CWindows_CUI_CXaml_CIApplication xaml_application1;
typedef __x_ABI_CWindows_CUI_CXaml_CIApplication2 xaml_application2;
typedef __x_ABI_CWindows_CUI_CXaml_CIApplication3 xaml_application3;
typedef __x_ABI_CWindows_CUI_CXaml_CIApplicationStatics xaml_application_statics;
typedef __x_ABI_CWindows_CUI_CXaml_CIApplicationFactory xaml_application_factory;

static const WCHAR class_name[] = L"Windows.UI.Xaml.Application";
/* The application the runtime last created; Application.Current returns it. */
static void *current_application;
static DWORD startup_thread;

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

static HRESULT runtime_class_name(HSTRING *name)
{
	if (!name)
		return E_POINTER;
	return WindowsCreateString(class_name, (UINT32)lstrlenW(class_name), name);
}

static int equal_iid(REFIID left, const IID *right)
{
	return left->Data1 == right->Data1 && left->Data2 == right->Data2 && left->Data3 == right->Data3 &&
		left->Data4[0] == right->Data4[0] && left->Data4[1] == right->Data4[1] &&
		left->Data4[2] == right->Data4[2] && left->Data4[3] == right->Data4[3] &&
		left->Data4[4] == right->Data4[4] && left->Data4[5] == right->Data4[5] &&
		left->Data4[6] == right->Data4[6] && left->Data4[7] == right->Data4[7];
}

/* Method slots the engine does not implement yet answer E_NOTIMPL. Each stub carries the exact signature of its slot, so a call
 * never depends on an argument count the metadata did not give. */
#define STUB0(iface, name) static HRESULT WINAPI name(iface *This) { (void)This;return E_NOTIMPL; }
#define STUB1(iface, name) static HRESULT WINAPI name(iface *This, uint64_t a1) { (void)This; (void)a1;return E_NOTIMPL; }
#define STUB2(iface, name) \
	static HRESULT WINAPI name(iface *This, uint64_t a1, uint64_t a2) { (void)This; (void)a1; (void)a2;return E_NOTIMPL; }
#define STUB3(iface, name) \
	static HRESULT WINAPI name(iface *This, uint64_t a1, uint64_t a2, uint64_t a3) \
	{ (void)This; (void)a1; (void)a2; (void)a3;return E_NOTIMPL; }

struct application;

struct application1_part { const __x_ABI_CWindows_CUI_CXaml_CIApplicationVtbl *lpVtbl; struct application *owner; };
struct application2_part { const __x_ABI_CWindows_CUI_CXaml_CIApplication2Vtbl *lpVtbl; struct application *owner; };
struct application3_part { const __x_ABI_CWindows_CUI_CXaml_CIApplication3Vtbl *lpVtbl; struct application *owner; };
struct application_inner_part { const IInspectableVtbl *lpVtbl; struct application *owner; };
struct application_overrides_part { const __x_ABI_CWindows_CUI_CXaml_CIApplicationOverridesVtbl *lpVtbl; struct application *owner; };

struct application {
	struct application1_part first;
	struct application2_part second;
	struct application3_part third;
	struct application_inner_part inner;
	struct application_overrides_part overrides;
	LONG refs;
	IInspectable *outer;
	LONG exited;
	INT32 requested_theme;
	struct raven_event_list suspending, resuming;
	/* The ResourceDictionary the application assigns through put_Resources; a reference the application object owns. */
	IInspectable *resources;
};

struct application_factory;

struct factory_activation { const IActivationFactoryVtbl *lpVtbl; struct application_factory *owner; };
struct factory_statics {
	const __x_ABI_CWindows_CUI_CXaml_CIApplicationStaticsVtbl *lpVtbl;
	struct application_factory *owner;
};
struct factory_create {
	const __x_ABI_CWindows_CUI_CXaml_CIApplicationFactoryVtbl *lpVtbl;
	struct application_factory *owner;
};

struct application_factory {
	struct factory_activation activation;
	struct factory_statics statics;
	struct factory_create create;
	LONG refs;
};

/* Resources stores the ResourceDictionary the application assigns; the engine reads it once the style stage applies resources.
 * A put re-renders like every other property store, even though nothing reaches the tree yet. */
/* Application.Resources is never null on the real platform: an application that never assigns it still has a dictionary to
 * read, and a compiled App.xaml fills that same dictionary. The first read creates it; the application owns the reference
 * and every later read hands out the same object. */
static HRESULT WINAPI application1_get_Resources(xaml_application1 *iface, IInspectable **out)
{
	struct application *self = ((struct application1_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	if (!self->resources) {
		IInspectable *created = NULL;
		const HRESULT made = raven_xaml_create_stable(371, &created);
		if (FAILED(made) || !created)
			return FAILED(made) ? made : E_OUTOFMEMORY;
		self->resources = created;
	}
	*out = self->resources;
	(*out)->lpVtbl->AddRef(*out);
	return S_OK;
}

static HRESULT WINAPI application1_put_Resources(xaml_application1 *iface, IInspectable *value)
{
	struct application *self = ((struct application1_part *)iface)->owner;
	IInspectable *dictionary = value;
	if (dictionary)
		dictionary->lpVtbl->AddRef(dictionary);
	if (self->resources)
		self->resources->lpVtbl->Release(self->resources);
	self->resources = dictionary;
	raven_render_changed();
	return S_OK;
}
STUB1(xaml_application1, application1_get_DebugSettings)
static HRESULT WINAPI application1_get_RequestedTheme(xaml_application1 *iface, INT32 *out)
{
	if (!out) return E_POINTER;
	*out = ((struct application1_part *)iface)->owner->requested_theme;
	return S_OK;
}
static HRESULT WINAPI application1_put_RequestedTheme(xaml_application1 *iface, INT32 value)
{
	if (value < 0 || value > 1) return E_INVALIDARG;
	/* shortcut: resources do not yet select a theme or reject changes after startup; upgrade with theme rendering. */
	((struct application1_part *)iface)->owner->requested_theme = (INT32)value;
	return S_OK;
}
STUB2(xaml_application1, application1_add_UnhandledException)
STUB1(xaml_application1, application1_remove_UnhandledException)
#define APPLICATION_EVENT(NAME, FIELD) \
	static HRESULT WINAPI application1_add_##NAME(xaml_application1 *iface, uint64_t handler, uint64_t token) \
	{ \
		return raven_event_add(&((struct application1_part *)iface)->owner->FIELD, (IUnknown *)(uintptr_t)handler, \
			(EventRegistrationToken *)(uintptr_t)token); \
	} \
	static HRESULT WINAPI application1_remove_##NAME(xaml_application1 *iface, uint64_t token) \
	{ \
		EventRegistrationToken value; \
		value.value = (INT64)token; \
		return raven_event_remove(&((struct application1_part *)iface)->owner->FIELD, value); \
	}
APPLICATION_EVENT(Suspending, suspending)
APPLICATION_EVENT(Resuming, resuming)
STUB1(xaml_application2, application2_get_FocusVisualKind)
STUB1(xaml_application2, application2_put_FocusVisualKind)
STUB1(xaml_application2, application2_get_RequiresPointerMode)
STUB1(xaml_application2, application2_put_RequiresPointerMode)
STUB2(xaml_application2, application2_add_LeavingBackground)
STUB1(xaml_application2, application2_remove_LeavingBackground)
STUB2(xaml_application2, application2_add_EnteredBackground)
STUB1(xaml_application2, application2_remove_EnteredBackground)
STUB1(xaml_application3, application3_get_HighContrastAdjustment)
STUB1(xaml_application3, application3_put_HighContrastAdjustment)
static HRESULT load_component(IInspectable *component, IInspectable *locator)
{
	typedef __x_ABI_CWindows_CFoundation_CIUriRuntimeClass uri_i;
	static const WCHAR prefix[] = L"ms-appx:///";
	static const WCHAR files[] = L"/Files";
	WCHAR path[1024];
	uri_i *uri = NULL;
	HSTRING absolute = NULL;
	const BYTE *blob = NULL;
	UINT32 length = 0, path_length, bytes = 0;
	HRESULT result;
	if (!component || !locator) return E_POINTER;
	result = IInspectable_QueryInterface(locator, &IID___x_ABI_CWindows_CFoundation_CIUriRuntimeClass, (void **)&uri);
	if (FAILED(result)) return result;
	result = uri->lpVtbl->get_AbsoluteUri(uri, &absolute);
	uri->lpVtbl->Release(uri);
	if (FAILED(result)) return result;
	const WCHAR *text = WindowsGetStringRawBuffer(absolute, &length);
	if (length < ARRAYSIZE(prefix) - 1 + 5 || length + ARRAYSIZE(files) >= ARRAYSIZE(path) ||
		CompareStringOrdinal(text, ARRAYSIZE(prefix) - 1, prefix, -1, TRUE) != CSTR_EQUAL ||
		CompareStringOrdinal(text + length - 5, 5, L".xaml", -1, TRUE) != CSTR_EQUAL) {
		result = E_NOTIMPL;
		goto done;
	}
	/* Shortcut: only local package XAML without escaping or URI qualifiers is resolved here. */
	for (UINT32 i = ARRAYSIZE(prefix) - 1; i < length; ++i)
		if (!text[i] || text[i] == L'?' || text[i] == L'#' || text[i] == L'%' || text[i] == L'\\') {
			result = E_NOTIMPL;
			goto done;
		}
	memcpy(path, files, (ARRAYSIZE(files) - 1) * sizeof(WCHAR));
	path_length = ARRAYSIZE(files) - 1 + length - (ARRAYSIZE(prefix) - 2);
	memcpy(path + ARRAYSIZE(files) - 1, text + ARRAYSIZE(prefix) - 2,
		(length - (ARRAYSIZE(prefix) - 2) - 5) * sizeof(WCHAR));
	path_length -= 1; /* .xaml becomes .xbf */
	memcpy(path + path_length - 4, L".xbf", 4 * sizeof(WCHAR));
	result = raven_pri_get_blob(path, path_length, &blob, &bytes);
	if (SUCCEEDED(result)) result = raven_xaml_load_into(blob, bytes, component);
done:
	WindowsDeleteString(absolute);
	return result;
}
static HRESULT WINAPI statics_LoadComponent(xaml_application_statics *iface, IInspectable *component, IInspectable *locator)
{ return load_component(component, locator); }
static HRESULT WINAPI statics_LoadComponent2(xaml_application_statics *iface, IInspectable *component,
	IInspectable *locator, INT32 location)
{
	if (location != 0) return location == 1 ? E_NOTIMPL : E_INVALIDARG;
	return load_component(component, locator);
}

/* The shared reference count and release of an application object, from any of its three interface sub-objects. */
static ULONG application_addref(struct application *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG application_release(struct application *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		raven_event_close(&self->suspending);
		raven_event_close(&self->resuming);
		DeleteCriticalSection(&self->suspending.lock);
		DeleteCriticalSection(&self->resuming.lock);
		if (self->resources)
			IInspectable_Release(self->resources);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT application_inner_query(struct application *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable)) {
		*out = self->outer ? (void *)&self->inner : (void *)&self->first;
		application_addref(self);
		return S_OK;
	} else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication))
		*out = &self->first;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication2))
		*out = &self->second;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication3))
		*out = &self->third;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides))
		*out = &self->overrides;
	else
		return E_NOINTERFACE;
	if (self->outer)
		IInspectable_AddRef(self->outer);
	else
		application_addref(self);
	return S_OK;
}

static HRESULT application_query(struct application *self, REFIID iid, void **out)
{
	if (self->outer)
		return IInspectable_QueryInterface(self->outer, iid, out);
	return application_inner_query(self, iid, out);
}

static HRESULT WINAPI application1_query(xaml_application1 *iface, REFIID iid, void **out)
{
	return application_query(((struct application1_part *)iface)->owner, iid, out);
}

static HRESULT WINAPI application2_query(xaml_application2 *iface, REFIID iid, void **out)
{
	return application_query(((struct application2_part *)iface)->owner, iid, out);
}

static HRESULT WINAPI application3_query(xaml_application3 *iface, REFIID iid, void **out)
{
	return application_query(((struct application3_part *)iface)->owner, iid, out);
}

static ULONG application_interface_addref(struct application *self) { return self->outer ? IInspectable_AddRef(self->outer) : application_addref(self); }
static ULONG application_interface_release(struct application *self) { return self->outer ? IInspectable_Release(self->outer) : application_release(self); }
static ULONG WINAPI application1_addref(xaml_application1 *iface) { return application_interface_addref(((struct application1_part *)iface)->owner); }
static ULONG WINAPI application2_addref(xaml_application2 *iface) { return application_interface_addref(((struct application2_part *)iface)->owner); }
static ULONG WINAPI application3_addref(xaml_application3 *iface) { return application_interface_addref(((struct application3_part *)iface)->owner); }
static ULONG WINAPI application1_release(xaml_application1 *iface) { return application_interface_release(((struct application1_part *)iface)->owner); }
static ULONG WINAPI application2_release(xaml_application2 *iface) { return application_interface_release(((struct application2_part *)iface)->owner); }
static ULONG WINAPI application3_release(xaml_application3 *iface) { return application_interface_release(((struct application3_part *)iface)->owner); }

static HRESULT WINAPI application1_iids(xaml_application1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI application1_trust(xaml_application1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI application1_class_name(xaml_application1 *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name);
}

static HRESULT WINAPI application2_iids(xaml_application2 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI application2_trust(xaml_application2 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI application2_class_name(xaml_application2 *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name);
}

static HRESULT WINAPI application3_iids(xaml_application3 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI application3_trust(xaml_application3 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI application3_class_name(xaml_application3 *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name);
}

static HRESULT WINAPI application1_exit(xaml_application1 *iface)
{
	InterlockedExchange(&((struct application1_part *)iface)->owner->exited, 1);
	DWORD thread = (DWORD)InterlockedCompareExchange((LONG *)&startup_thread, 0, 0);
	if (thread && !PostThreadMessageW(thread, WM_QUIT, 0, 0)) return HRESULT_FROM_WIN32(GetLastError());
	return S_OK;
}

static const __x_ABI_CWindows_CUI_CXaml_CIApplicationVtbl application1_table = {
	.QueryInterface = application1_query,
	.AddRef = application1_addref,
	.Release = application1_release,
	.GetIids = application1_iids,
	.GetRuntimeClassName = application1_class_name,
	.GetTrustLevel = application1_trust,
	.get_Resources = application1_get_Resources,
	.put_Resources = application1_put_Resources,
	.get_DebugSettings = application1_get_DebugSettings,
	.get_RequestedTheme = application1_get_RequestedTheme,
	.put_RequestedTheme = application1_put_RequestedTheme,
	.add_UnhandledException = application1_add_UnhandledException,
	.remove_UnhandledException = application1_remove_UnhandledException,
	.add_Suspending = application1_add_Suspending,
	.remove_Suspending = application1_remove_Suspending,
	.add_Resuming = application1_add_Resuming,
	.remove_Resuming = application1_remove_Resuming,
	.Exit = application1_exit,
};

static const __x_ABI_CWindows_CUI_CXaml_CIApplication2Vtbl application2_table = {
	.QueryInterface = application2_query,
	.AddRef = application2_addref,
	.Release = application2_release,
	.GetIids = application2_iids,
	.GetRuntimeClassName = application2_class_name,
	.GetTrustLevel = application2_trust,
	.get_FocusVisualKind = application2_get_FocusVisualKind,
	.put_FocusVisualKind = application2_put_FocusVisualKind,
	.get_RequiresPointerMode = application2_get_RequiresPointerMode,
	.put_RequiresPointerMode = application2_put_RequiresPointerMode,
	.add_LeavingBackground = application2_add_LeavingBackground,
	.remove_LeavingBackground = application2_remove_LeavingBackground,
	.add_EnteredBackground = application2_add_EnteredBackground,
	.remove_EnteredBackground = application2_remove_EnteredBackground,
};

static const __x_ABI_CWindows_CUI_CXaml_CIApplication3Vtbl application3_table = {
	.QueryInterface = application3_query,
	.AddRef = application3_addref,
	.Release = application3_release,
	.GetIids = application3_iids,
	.GetRuntimeClassName = application3_class_name,
	.GetTrustLevel = application3_trust,
	.get_HighContrastAdjustment = application3_get_HighContrastAdjustment,
	.put_HighContrastAdjustment = application3_put_HighContrastAdjustment,
};

static HRESULT WINAPI inner_query(IInspectable *iface, REFIID iid, void **out)
{
	return application_inner_query(((struct application_inner_part *)iface)->owner, iid, out);
}
static ULONG WINAPI inner_addref(IInspectable *iface) { return application_addref(((struct application_inner_part *)iface)->owner); }
static ULONG WINAPI inner_release(IInspectable *iface) { return application_release(((struct application_inner_part *)iface)->owner); }
static HRESULT WINAPI inner_iids(IInspectable *iface, ULONG *count, IID **iids) { (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI inner_name(IInspectable *iface, HSTRING *name) { (void)iface; return runtime_class_name(name); }
static HRESULT WINAPI inner_trust(IInspectable *iface, TrustLevel *level) { (void)iface; return inspectable_trust(level); }
static const IInspectableVtbl inner_table = {
	.QueryInterface = inner_query, .AddRef = inner_addref, .Release = inner_release,
	.GetIids = inner_iids, .GetRuntimeClassName = inner_name, .GetTrustLevel = inner_trust,
};

typedef __x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides overrides_i;
static HRESULT WINAPI base_overrides_query(overrides_i *iface, REFIID iid, void **out)
{ return application_query(((struct application_overrides_part *)iface)->owner, iid, out); }
static ULONG WINAPI base_overrides_addref(overrides_i *iface)
{ return application_interface_addref(((struct application_overrides_part *)iface)->owner); }
static ULONG WINAPI base_overrides_release(overrides_i *iface)
{ return application_interface_release(((struct application_overrides_part *)iface)->owner); }
static HRESULT WINAPI base_overrides_iids(overrides_i *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI base_overrides_name(overrides_i *iface, HSTRING *name)
{ (void)iface; return runtime_class_name(name); }
static HRESULT WINAPI base_overrides_trust(overrides_i *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static HRESULT WINAPI base_overrides_event(overrides_i *iface, uintptr_t args)
{ (void)iface; (void)args; return S_OK; }
static const __x_ABI_CWindows_CUI_CXaml_CIApplicationOverridesVtbl base_overrides_table = {
	base_overrides_query, base_overrides_addref, base_overrides_release, base_overrides_iids, base_overrides_name,
	base_overrides_trust, base_overrides_event, base_overrides_event, base_overrides_event, base_overrides_event,
	base_overrides_event, base_overrides_event, base_overrides_event, base_overrides_event, base_overrides_event
};

static struct application *create_application(IInspectable *outer)
{
	struct application *application = ALLOC(sizeof *application);
	if (!application)
		return NULL;
	application->first.lpVtbl = &application1_table;
	application->first.owner = application;
	application->second.lpVtbl = &application2_table;
	application->second.owner = application;
	application->third.lpVtbl = &application3_table;
	application->third.owner = application;
	application->inner.lpVtbl = &inner_table;
	application->inner.owner = application;
	application->overrides.lpVtbl = &base_overrides_table;
	application->overrides.owner = application;
	application->refs = 1;
	application->outer = outer;
	InitializeCriticalSection(&application->suspending.lock);
	InitializeCriticalSection(&application->resuming.lock);
	InterlockedIncrement(&raven_xaml_objects);
	return application;
}

typedef __x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParams application_params;

struct params {
	const __x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParamsVtbl *lpVtbl;
	LONG refs;
};

static HRESULT WINAPI params_query(application_params *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParams))
		return E_NOINTERFACE;
	InterlockedIncrement(&((struct params *)iface)->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI params_addref(application_params *iface)
{
	return (ULONG)InterlockedIncrement(&((struct params *)iface)->refs);
}

static ULONG WINAPI params_release(application_params *iface)
{
	struct params *self = (struct params *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI params_iids(application_params *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI params_name(application_params *iface, HSTRING *name)
{
	static const WCHAR params_name[] = L"Windows.UI.Xaml.ApplicationInitializationCallbackParams";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(params_name, (UINT32)lstrlenW(params_name), name);
}

static HRESULT WINAPI params_trust(application_params *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static const __x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParamsVtbl params_table = {
	.QueryInterface = params_query,
	.AddRef = params_addref,
	.Release = params_release,
	.GetIids = params_iids,
	.GetRuntimeClassName = params_name,
	.GetTrustLevel = params_trust,
};

static struct params *create_params(void)
{
	struct params *parameters = ALLOC(sizeof *parameters);
	if (!parameters)
		return NULL;
	parameters->lpVtbl = &params_table;
	parameters->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	return parameters;
}

typedef HRESULT (STDMETHODCALLTYPE *initialization_invoke)(void *self, IInspectable *parameters);

typedef __x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgs window_created_i;
typedef __x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides application_overrides_i;
struct window_created_args {
	window_created_i iface;
	LONG refs;
	IInspectable *window;
	struct raven_weak_source weak;
};
static HRESULT WINAPI window_args_query(window_created_i *iface, REFIID iid, void **out)
{
	struct window_created_args *self = (void *)iface;
	if (!out) return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IWeakReferenceSource)) *out = &self->weak;
	else if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgs)) *out = iface;
	else return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}
static ULONG WINAPI window_args_addref(window_created_i *iface)
{ return InterlockedIncrement(&((struct window_created_args *)iface)->refs); }
static ULONG WINAPI window_args_release(window_created_i *iface)
{
	struct window_created_args *self = (void *)iface;
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		raven_weak_source_disconnect(&self->weak);
		IInspectable_Release(self->window);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return refs;
}
static HRESULT WINAPI window_args_iids(window_created_i *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI window_args_name(window_created_i *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Windows.UI.Xaml.WindowCreatedEventArgs";
	(void)iface;
	if (!name) return E_POINTER;
	return WindowsCreateString(text, ARRAYSIZE(text) - 1, name);
}
static HRESULT WINAPI window_args_trust(window_created_i *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static HRESULT WINAPI window_args_window(window_created_i *iface, uintptr_t out)
{
	struct window_created_args *self = (void *)iface;
	if (!out) return E_POINTER;
	IInspectable_AddRef(self->window);
	*(IInspectable **)(uintptr_t)out = self->window;
	return S_OK;
}
static const __x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgsVtbl window_args_table = {
	window_args_query, window_args_addref, window_args_release, window_args_iids, window_args_name,
	window_args_trust, window_args_window
};

static HRESULT start_application(void)
{
	IInspectable *application = NULL, *window = NULL, *launch = NULL;
	application_overrides_i *overrides = NULL;
	ICoreWindow *core = NULL;
	ICoreDispatcher *dispatcher = NULL;
	struct window_created_args *args = NULL;
	HRESULT result = raven_xaml_application_current(&application);
	if (FAILED(result) || !application) return result;
	if (!((struct application1_part *)application)->owner->outer) { result = S_OK; goto done; }
	result = IInspectable_QueryInterface(application, &IID__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides, (void **)&overrides);
	if (FAILED(result)) goto done;
	result = raven_xaml_window_current(&window);
	if (FAILED(result)) goto done;
	args = ALLOC(sizeof *args);
	if (!args) { result = E_OUTOFMEMORY; goto done; }
	args->iface.lpVtbl = &window_args_table;
	args->refs = 1;
	args->window = window;
	IInspectable_AddRef(window);
	raven_weak_source_init(&args->weak, args, &args->refs, (raven_weak_query_fn)window_args_query);
	InterlockedIncrement(&raven_xaml_objects);
	result = overrides->lpVtbl->OnWindowCreated(overrides, (uintptr_t)&args->iface);
	if (SUCCEEDED(result)) result = raven_launch_arguments_create(&launch);
	if (SUCCEEDED(result)) result = overrides->lpVtbl->OnLaunched(overrides, (uintptr_t)launch);
	if (SUCCEEDED(result)) result = raven_current_window(&core);
	if (SUCCEEDED(result)) result = ICoreWindow_get_Dispatcher(core, &dispatcher);
	if (SUCCEEDED(result)) result = ICoreDispatcher_ProcessEvents(dispatcher, CoreProcessEventsOption_ProcessUntilQuit);
done:
	if (dispatcher) ICoreDispatcher_Release(dispatcher);
	if (core) ICoreWindow_Release(core);
	if (launch) IInspectable_Release(launch);
	if (args) window_args_release(&args->iface);
	if (window) IInspectable_Release(window);
	if (overrides) overrides->lpVtbl->Release(overrides);
	IInspectable_Release(application);
	return result;
}

/* The callback creates the application before activation and the dispatcher loop. */
static HRESULT WINAPI statics_Start(xaml_application_statics *iface, uint64_t callback)
{
	struct params *parameters;
	initialization_invoke invoke;
	void *vtable;
	HRESULT result;
	(void)iface;

	if (!callback)
		return E_POINTER;
	if (!(parameters = create_params()))
		return E_OUTOFMEMORY;
	/* A delegate is an IUnknown whose fourth slot is Invoke: the object's first word is its vtable. */
	vtable = *(void **)(uintptr_t)callback;
	invoke = (initialization_invoke)((void **)vtable)[3];
	if (InterlockedCompareExchange((LONG *)&startup_thread, (LONG)GetCurrentThreadId(), 0)) {
		params_release((application_params *)parameters);
		return E_ILLEGAL_METHOD_CALL;
	}
	result = invoke((void *)(uintptr_t)callback, (IInspectable *)parameters);
	params_release((application_params *)parameters);
	if (SUCCEEDED(result)) result = start_application();
	InterlockedExchange((LONG *)&startup_thread, 0);
	return result;
}

static void set_current(void *object)
{
	void *previous;
	if (object)
		((IUnknown *)object)->lpVtbl->AddRef((IUnknown *)object);
	previous = InterlockedExchangePointer(&current_application, object);
	if (previous)
		((IUnknown *)previous)->lpVtbl->Release((IUnknown *)previous);
}

HRESULT raven_xaml_application_current(IInspectable **out)
{
	void *current;
	if (!out)
		return E_POINTER;
	*out = NULL;
	current = InterlockedCompareExchangePointer(&current_application, NULL, NULL);
	if (!current)
		return S_OK;
	((IUnknown *)current)->lpVtbl->AddRef((IUnknown *)current);
	*out = (IInspectable *)current;
	return S_OK;
}

static HRESULT WINAPI statics_get_Current(xaml_application_statics *iface, uint64_t out)
{
	void *current;
	(void)iface;

	if (!out)
		return E_POINTER;
	current = InterlockedCompareExchangePointer(&current_application, NULL, NULL);
	if (!current) {
		*(void **)(uintptr_t)out = NULL;
		return S_OK;
	}
	((IUnknown *)current)->lpVtbl->AddRef((IUnknown *)current);
	*(void **)(uintptr_t)out = current;
	return S_OK;
}

static HRESULT factory_query_parts(struct application_factory *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory))
		*out = &self->activation;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics))
		*out = &self->statics;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIApplicationFactory))
		*out = &self->create;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static HRESULT WINAPI factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
	return factory_query_parts(((struct factory_activation *)iface)->owner, iid, out);
}

static HRESULT WINAPI statics_query(xaml_application_statics *iface, REFIID iid, void **out)
{
	return factory_query_parts(((struct factory_statics *)iface)->owner, iid, out);
}

static HRESULT WINAPI create_query(xaml_application_factory *iface, REFIID iid, void **out)
{
	return factory_query_parts(((struct factory_create *)iface)->owner, iid, out);
}

static ULONG WINAPI factory_addref(IActivationFactory *iface)
{
	return (ULONG)InterlockedIncrement(&((struct factory_activation *)iface)->owner->refs);
}

static ULONG WINAPI factory_release(IActivationFactory *iface)
{
	return (ULONG)InterlockedDecrement(&((struct factory_activation *)iface)->owner->refs);
}

static ULONG WINAPI statics_addref(xaml_application_statics *iface)
{
	return (ULONG)InterlockedIncrement(&((struct factory_statics *)iface)->owner->refs);
}

static ULONG WINAPI statics_release(xaml_application_statics *iface)
{
	return (ULONG)InterlockedDecrement(&((struct factory_statics *)iface)->owner->refs);
}

static ULONG WINAPI create_addref(xaml_application_factory *iface)
{
	return (ULONG)InterlockedIncrement(&((struct factory_create *)iface)->owner->refs);
}

static ULONG WINAPI create_release(xaml_application_factory *iface)
{
	return (ULONG)InterlockedDecrement(&((struct factory_create *)iface)->owner->refs);
}

static HRESULT WINAPI factory_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI factory_class_name(IActivationFactory *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name);
}

static HRESULT WINAPI factory_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI statics_iids(xaml_application_statics *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI statics_class_name(xaml_application_statics *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name);
}

static HRESULT WINAPI statics_trust(xaml_application_statics *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI create_iids(xaml_application_factory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI create_class_name(xaml_application_factory *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name);
}

static HRESULT WINAPI create_trust(xaml_application_factory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out)
{
	struct application *application;
	(void)iface;

	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!(application = create_application(NULL)))
		return E_OUTOFMEMORY;
	*out = (IInspectable *)&application->first;
	set_current(&application->first);
	return S_OK;
}

/* CreateInstance returns the base object through inner when composing with an outer application. */
static HRESULT WINAPI factory_create(xaml_application_factory *iface, uint64_t outer, uint64_t inner, uint64_t instance)
{
	struct application *application;
	(void)iface;

	if (!instance)
		return E_POINTER;
	*(void **)(uintptr_t)instance = NULL;
	if (outer && !inner)
		return E_POINTER;
	if (inner)
		*(void **)(uintptr_t)inner = NULL;
	if (!(application = create_application((IInspectable *)(uintptr_t)outer)))
		return E_OUTOFMEMORY;
	if (outer) {
		*(void **)(uintptr_t)inner = &application->inner;
		IInspectable_AddRef(application->outer);
	}
	*(void **)(uintptr_t)instance = &application->first;
	set_current(&application->first);
	return S_OK;
}

static const IActivationFactoryVtbl activation_table = {
	.QueryInterface = factory_query,
	.AddRef = factory_addref,
	.Release = factory_release,
	.GetIids = factory_iids,
	.GetRuntimeClassName = factory_class_name,
	.GetTrustLevel = factory_trust,
	.ActivateInstance = factory_activate,
};

static const __x_ABI_CWindows_CUI_CXaml_CIApplicationStaticsVtbl statics_table = {
	.QueryInterface = statics_query,
	.AddRef = statics_addref,
	.Release = statics_release,
	.GetIids = statics_iids,
	.GetRuntimeClassName = statics_class_name,
	.GetTrustLevel = statics_trust,
	.get_Current = statics_get_Current,
	.Start = statics_Start,
	.LoadComponent = statics_LoadComponent,
	.LoadComponent2 = statics_LoadComponent2,
};

static const __x_ABI_CWindows_CUI_CXaml_CIApplicationFactoryVtbl create_table = {
	.QueryInterface = create_query,
	.AddRef = create_addref,
	.Release = create_release,
	.GetIids = create_iids,
	.GetRuntimeClassName = create_class_name,
	.GetTrustLevel = create_trust,
	.CreateInstance = factory_create,
};

static struct application_factory factory_instance = {
	{&activation_table, &factory_instance}, {&statics_table, &factory_instance}, {&create_table, &factory_instance}, 1};

HRESULT raven_xaml_application_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (length != ARRAYSIZE(class_name) - 1 || CompareStringOrdinal(text, (int)length, class_name, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	InterlockedIncrement(&factory_instance.refs);
	*out = (IActivationFactory *)&factory_instance.activation;
	return S_OK;
}
