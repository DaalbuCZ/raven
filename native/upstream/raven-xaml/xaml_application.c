/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.UI.Xaml.Application class of the Windows Runtime.
 *
 * The class is what a XAML application derives from. This file provides its activation: the factory a program asks for by name,
 * the statics of the class, the composable factory and an application object with the three interfaces the type carries. The
 * members that need the XAML engine (resources, theme, events, startup) answer E_NOTIMPL for now; what is established here is the
 * exact ABI the .NET Native type loader binds to, taken from Windows.UI.Xaml.winmd by scripts/generate-winrt-header.py.
 *
 * The object is not a rendering surface: composition and the engine stages plug into the same factory later. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include "windows_ui_xaml_application.h"

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

struct application {
	struct application1_part first;
	struct application2_part second;
	struct application3_part third;
	LONG refs;
	IInspectable *outer;
	LONG exited;
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
static HRESULT WINAPI application1_get_Resources(xaml_application1 *iface, uint64_t out)
{
	struct application *self = ((struct application1_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = self->resources;
	if (self->resources)
		self->resources->lpVtbl->AddRef(self->resources);
	return S_OK;
}

static HRESULT WINAPI application1_put_Resources(xaml_application1 *iface, uint64_t value)
{
	struct application *self = ((struct application1_part *)iface)->owner;
	IInspectable *dictionary = (IInspectable *)(uintptr_t)value;
	if (dictionary)
		dictionary->lpVtbl->AddRef(dictionary);
	if (self->resources)
		self->resources->lpVtbl->Release(self->resources);
	self->resources = dictionary;
	raven_render_changed();
	return S_OK;
}
STUB1(xaml_application1, application1_get_DebugSettings)
STUB1(xaml_application1, application1_get_RequestedTheme)
STUB1(xaml_application1, application1_put_RequestedTheme)
STUB2(xaml_application1, application1_add_UnhandledException)
STUB1(xaml_application1, application1_remove_UnhandledException)
STUB2(xaml_application1, application1_add_Suspending)
STUB1(xaml_application1, application1_remove_Suspending)
STUB2(xaml_application1, application1_add_Resuming)
STUB1(xaml_application1, application1_remove_Resuming)
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
STUB2(xaml_application_statics, statics_LoadComponent)
STUB3(xaml_application_statics, statics_LoadComponent2)

/* The shared reference count and release of an application object, from any of its three interface sub-objects. */
static ULONG application_addref(struct application *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG application_release(struct application *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->outer)
			IInspectable_Release(self->outer);
		if (self->resources)
			IInspectable_Release(self->resources);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT application_query(struct application *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication))
		*out = &self->first;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication2))
		*out = &self->second;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication3))
		*out = &self->third;
	else if (self->outer) {
		/* Aggregation: the interfaces the base object does not carry belong to the composed application the runtime built
		 * around it. The engine reaches the app's IXamlMetadataProvider this way. */
		return ((IInspectable *)self->outer)->lpVtbl->QueryInterface(self->outer, iid, out);
	} else
		return E_NOINTERFACE;
	application_addref(self);
	return S_OK;
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

static ULONG WINAPI application1_addref(xaml_application1 *iface) { return application_addref(((struct application1_part *)iface)->owner); }
static ULONG WINAPI application2_addref(xaml_application2 *iface) { return application_addref(((struct application2_part *)iface)->owner); }
static ULONG WINAPI application3_addref(xaml_application3 *iface) { return application_addref(((struct application3_part *)iface)->owner); }
static ULONG WINAPI application1_release(xaml_application1 *iface) { return application_release(((struct application1_part *)iface)->owner); }
static ULONG WINAPI application2_release(xaml_application2 *iface) { return application_release(((struct application2_part *)iface)->owner); }
static ULONG WINAPI application3_release(xaml_application3 *iface) { return application_release(((struct application3_part *)iface)->owner); }

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
	application->refs = 1;
	application->outer = outer;
	if (outer)
		IInspectable_AddRef(outer);
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

/* Application.Start gives the callback the initialization parameters and lets it create the application. The XAML engine would
 * then run the view loop; here the callback's result is returned to the runtime. */
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
	result = invoke((void *)(uintptr_t)callback, (IInspectable *)parameters);
	params_release((application_params *)parameters);
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
	if (inner)
		*(void **)(uintptr_t)inner = NULL;
	if (!(application = create_application((IInspectable *)(uintptr_t)outer)))
		return E_OUTOFMEMORY;
	/* shortcut: inner shares the base object's identity and lifetime, implement full aggregation when identity matters. */
	if (outer && inner) {
		application_addref(application);
		*(void **)(uintptr_t)inner = &application->first;
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
