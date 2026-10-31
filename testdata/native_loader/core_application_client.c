#define INITGUID
#define CONST_VTABLE
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define WIDL_using_Windows_ApplicationModel_Core
#define WIDL_using_Windows_ApplicationModel_Activation
#define WIDL_using_Windows_UI_Core
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <windows.applicationmodel.core.h>
#include <windows.applicationmodel.activation.h>

typedef __FITypedEventHandler_2_Windows__CApplicationModel__CCore__CCoreApplicationView_Windows__CApplicationModel__CActivation__CIActivatedEventArgs activated_handler;
typedef __FITypedEventHandler_2_Windows__CApplicationModel__CCore__CCoreApplicationView_Windows__CApplicationModel__CActivation__CIActivatedEventArgsVtbl activated_vtable;
typedef __FIEventHandler_1_IInspectable exiting_handler;
typedef __FIEventHandler_1_IInspectableVtbl exiting_vtable;
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);

enum {CREATE = 1, INITIALIZE = 2, SET_WINDOW = 4, LOAD = 8, RUN = 16, ACTIVATED = 32, DISPATCH = 64, UNINITIALIZE = 128, EXITING = 256};
struct client {
	IFrameworkViewSource source;
	IFrameworkView framework;
	activated_handler activated;
	IDispatchedHandler dispatched;
	exiting_handler exiting;
	LONG refs;
	ICoreApplication *app;
	ICoreApplicationExit *exit;
	ICoreApplicationView *view;
	ICoreWindow *window;
	ICoreDispatcher *dispatcher;
	IActivatedEventArgs *args;
	EventRegistrationToken activation_token, exit_token;
	HANDLE activated_event;
	DWORD ui_thread;
	int mode, stages, failure;
	HRESULT result;
};
#define REQUIRE(condition) do { if (!(condition)) { self->failure = __LINE__; return E_FAIL; } } while (0)

static HRESULT client_query(struct client *self, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID_IFrameworkViewSource))
		*out = &self->source;
	else if (IsEqualIID(iid, &IID_IFrameworkView)) *out = &self->framework;
	else if (IsEqualIID(iid, &IID_IDispatchedHandler)) *out = &self->dispatched;
	else return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}
static HRESULT no_iids(ULONG *count, IID **iids)
{
	if (!count || !iids) return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}

static HRESULT WINAPI source_query(IFrameworkViewSource *iface, REFIID iid, void **out)
{
	return client_query(CONTAINING_RECORD(iface, struct client, source), iid, out);
}
static ULONG WINAPI source_addref(IFrameworkViewSource *iface)
{
	return InterlockedIncrement(&CONTAINING_RECORD(iface, struct client, source)->refs);
}
static ULONG WINAPI source_release(IFrameworkViewSource *iface)
{
	return InterlockedDecrement(&CONTAINING_RECORD(iface, struct client, source)->refs);
}
static HRESULT WINAPI source_iids(IFrameworkViewSource *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI source_class(IFrameworkViewSource *iface, HSTRING *out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	return S_OK;
}
static HRESULT WINAPI source_trust(IFrameworkViewSource *iface, TrustLevel *out)
{
	if (!out) return E_POINTER;
	*out = BaseTrust;
	return S_OK;
}

static HRESULT WINAPI framework_query(IFrameworkView *iface, REFIID iid, void **out)
{
	return client_query(CONTAINING_RECORD(iface, struct client, framework), iid, out);
}
static ULONG WINAPI framework_addref(IFrameworkView *iface)
{
	return InterlockedIncrement(&CONTAINING_RECORD(iface, struct client, framework)->refs);
}
static ULONG WINAPI framework_release(IFrameworkView *iface)
{
	return InterlockedDecrement(&CONTAINING_RECORD(iface, struct client, framework)->refs);
}
static HRESULT WINAPI framework_iids(IFrameworkView *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI framework_class(IFrameworkView *iface, HSTRING *out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	return S_OK;
}
static HRESULT WINAPI framework_trust(IFrameworkView *iface, TrustLevel *out)
{
	if (!out) return E_POINTER;
	*out = BaseTrust;
	return S_OK;
}

static HRESULT WINAPI activated_query(activated_handler *iface, REFIID iid, void **out)
{
	return client_query(CONTAINING_RECORD(iface, struct client, activated), iid, out);
}
static ULONG WINAPI activated_addref(activated_handler *iface)
{
	return InterlockedIncrement(&CONTAINING_RECORD(iface, struct client, activated)->refs);
}
static ULONG WINAPI activated_release(activated_handler *iface)
{
	return InterlockedDecrement(&CONTAINING_RECORD(iface, struct client, activated)->refs);
}

static HRESULT WINAPI dispatched_query(IDispatchedHandler *iface, REFIID iid, void **out)
{
	return client_query(CONTAINING_RECORD(iface, struct client, dispatched), iid, out);
}
static ULONG WINAPI dispatched_addref(IDispatchedHandler *iface)
{
	return InterlockedIncrement(&CONTAINING_RECORD(iface, struct client, dispatched)->refs);
}
static ULONG WINAPI dispatched_release(IDispatchedHandler *iface)
{
	return InterlockedDecrement(&CONTAINING_RECORD(iface, struct client, dispatched)->refs);
}

static HRESULT WINAPI exiting_query(exiting_handler *iface, REFIID iid, void **out)
{
	return client_query(CONTAINING_RECORD(iface, struct client, exiting), iid, out);
}
static ULONG WINAPI exiting_addref(exiting_handler *iface)
{
	return InterlockedIncrement(&CONTAINING_RECORD(iface, struct client, exiting)->refs);
}
static ULONG WINAPI exiting_release(exiting_handler *iface)
{
	return InterlockedDecrement(&CONTAINING_RECORD(iface, struct client, exiting)->refs);
}

static HRESULT WINAPI source_create(IFrameworkViewSource *iface, IFrameworkView **out)
{
	struct client *self = CONTAINING_RECORD(iface, struct client, source);
	REQUIRE(!self->stages && out);
	self->stages |= CREATE;
	*out = NULL;
	if (self->mode == 1) return E_ACCESSDENIED;
	if (self->mode == 10) return S_OK;
	*out = &self->framework;
	framework_addref(*out);
	return S_OK;
}
static HRESULT WINAPI framework_initialize(IFrameworkView *iface, ICoreApplicationView *view)
{
	struct client *self = CONTAINING_RECORD(iface, struct client, framework);
	REQUIRE(self->stages == CREATE && view);
	self->stages |= INITIALIZE;
	self->view = view;
	ICoreApplicationView_AddRef(view);
	ICoreApplicationView *current = NULL;
	REQUIRE(ICoreApplication_GetCurrentView(self->app, &current) == S_OK && current == view);
	ICoreApplicationView_Release(current);
	boolean value = FALSE;
	REQUIRE(ICoreApplicationView_get_IsMain(view, &value) == S_OK && value);
	REQUIRE(ICoreApplicationView_get_IsHosted(view, &value) == S_OK && !value);
	REQUIRE(ICoreApplication_Run(self->app, &self->source) == (HRESULT)0x8000000e);
	EventRegistrationToken removed;
	REQUIRE(ICoreApplicationView_add_Activated(view, &self->activated, &removed) == S_OK && removed.value);
	REQUIRE(ICoreApplicationView_add_Activated(view, &self->activated, &self->activation_token) == S_OK &&
		self->activation_token.value && self->activation_token.value != removed.value);
	REQUIRE(ICoreApplicationView_remove_Activated(view, removed) == S_OK);
	REQUIRE(ICoreApplicationExit_add_Exiting(self->exit, &self->exiting, &self->exit_token) == S_OK);
	return self->mode == 2 ? E_ACCESSDENIED : S_OK;
}
static HRESULT WINAPI framework_window(IFrameworkView *iface, ICoreWindow *window)
{
	struct client *self = CONTAINING_RECORD(iface, struct client, framework);
	REQUIRE(self->stages == (CREATE | INITIALIZE) && window);
	self->stages |= SET_WINDOW;
	ICoreWindow *view_window = NULL;
	REQUIRE(ICoreApplicationView_get_CoreWindow(self->view, &view_window) == S_OK && view_window == window);
	ICoreWindow_Release(view_window);
	self->window = window;
	ICoreWindow_AddRef(window);
	REQUIRE(ICoreWindow_get_Dispatcher(window, &self->dispatcher) == S_OK && self->dispatcher);
	return self->mode == 3 ? E_ACCESSDENIED : S_OK;
}
static HRESULT WINAPI framework_load(IFrameworkView *iface, HSTRING entry)
{
	struct client *self = CONTAINING_RECORD(iface, struct client, framework);
	REQUIRE(self->stages == (CREATE | INITIALIZE | SET_WINDOW));
	self->stages |= LOAD;
	/* Direct view-source startup has no manifest-supplied entry point. */
	REQUIRE(WindowsGetStringLen(entry) == 0);
	return self->mode == 4 ? E_ACCESSDENIED : S_OK;
}
static HRESULT WINAPI framework_run(IFrameworkView *iface)
{
	struct client *self = CONTAINING_RECORD(iface, struct client, framework);
	REQUIRE(self->stages == (CREATE | INITIALIZE | SET_WINDOW | LOAD));
	self->stages |= RUN;
	if (self->mode == 6) return E_ACCESSDENIED;
	if (self->mode == 9) ExitThread(0);
	REQUIRE(ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessUntilQuit) == S_OK);
	return S_OK;
}
static HRESULT WINAPI framework_uninitialize(IFrameworkView *iface)
{
	struct client *self = CONTAINING_RECORD(iface, struct client, framework);
	REQUIRE((self->stages & INITIALIZE) && !(self->stages & UNINITIALIZE));
	self->stages |= UNINITIALIZE;
	REQUIRE(ICoreApplicationView_remove_Activated(self->view, self->activation_token) == S_OK);
	return self->mode == 7 ? E_ACCESSDENIED : S_OK;
}
static HRESULT WINAPI activated_invoke(activated_handler *iface, ICoreApplicationView *sender, IActivatedEventArgs *args)
{
	struct client *self = CONTAINING_RECORD(iface, struct client, activated);
	REQUIRE(self->stages == (CREATE | INITIALIZE | SET_WINDOW | LOAD | RUN));
	self->stages |= ACTIVATED;
	REQUIRE(GetCurrentThreadId() == self->ui_thread && sender == self->view && args);
	ActivationKind kind;
	ApplicationExecutionState previous;
	REQUIRE(IActivatedEventArgs_get_Kind(args, &kind) == S_OK && kind == ActivationKind_Launch);
	REQUIRE(IActivatedEventArgs_get_PreviousExecutionState(args, &previous) == S_OK && previous == ApplicationExecutionState_NotRunning);
	ISplashScreen *splash = (void *)1;
	REQUIRE(IActivatedEventArgs_get_SplashScreen(args, &splash) == S_OK && !splash);
	/* Independently query the published launch IID and call its two ABI slots. */
	static const IID launch = {0xfbc93e26,0xa14a,0x4b4f,{0x82,0xb0,0x33,0xbe,0xd9,0x20,0xaf,0x52}};
	IUnknown *launch_args = NULL, *identity = NULL;
	REQUIRE(IActivatedEventArgs_QueryInterface(args, &launch, (void **)&launch_args) == S_OK && launch_args);
	REQUIRE(IUnknown_QueryInterface(launch_args, &IID_IUnknown, (void **)&identity) == S_OK && identity == (void *)args);
	IUnknown_Release(identity);
	void **slots = *(void ***)launch_args;
	for (int slot = 6; slot < 8; ++slot) {
		HRESULT (WINAPI *get_string)(void *, HSTRING *) = (void *)slots[slot];
		HSTRING text = NULL;
		REQUIRE(get_string(launch_args, &text) == S_OK && WindowsGetStringLen(text) == 0);
		WindowsDeleteString(text);
	}
	IUnknown_Release(launch_args);
	self->args = args;
	IActivatedEventArgs_AddRef(args);
	REQUIRE(ICoreApplicationView_remove_Activated(sender, self->activation_token) == S_OK);
	/* A registration added by this callback must not join the current event dispatch. */
	REQUIRE(ICoreApplicationView_add_Activated(sender, &self->activated, &self->activation_token) == S_OK);
	if (self->mode == 8) {
		SetEvent(self->activated_event);
		return S_OK;
	}
	if (self->mode == 5) {
		REQUIRE(ICoreApplicationExit_Exit(self->exit) == S_OK);
		return E_ACCESSDENIED;
	}
	__x_ABI_CWindows_CFoundation_CIAsyncAction *action = NULL;
	REQUIRE(ICoreDispatcher_RunAsync(self->dispatcher, CoreDispatcherPriority_Normal, &self->dispatched, &action) == S_OK && action);
	action->lpVtbl->Release(action);
	return S_OK;
}
static HRESULT WINAPI dispatched_invoke(IDispatchedHandler *iface)
{
	struct client *self = CONTAINING_RECORD(iface, struct client, dispatched);
	REQUIRE(self->stages == (CREATE | INITIALIZE | SET_WINDOW | LOAD | RUN | ACTIVATED));
	self->stages |= DISPATCH;
	REQUIRE(GetCurrentThreadId() == self->ui_thread);
	return ICoreApplicationExit_Exit(self->exit);
}
static HRESULT WINAPI exiting_invoke(exiting_handler *iface, IInspectable *sender, IInspectable *args)
{
	struct client *self = CONTAINING_RECORD(iface, struct client, exiting);
	REQUIRE((self->stages & UNINITIALIZE) && !(self->stages & EXITING) && sender && !args);
	self->stages |= EXITING;
	REQUIRE(GetCurrentThreadId() == self->ui_thread);
	return ICoreApplicationExit_remove_Exiting(self->exit, self->exit_token);
}
static const IFrameworkViewSourceVtbl source_table = {
	source_query, source_addref, source_release, source_iids, source_class, source_trust, source_create
};
static const IFrameworkViewVtbl framework_table = {
	framework_query, framework_addref, framework_release, framework_iids, framework_class, framework_trust,
	framework_initialize, framework_window, framework_load, framework_run, framework_uninitialize
};
static const activated_vtable activated_table = {activated_query, activated_addref, activated_release, activated_invoke};
static const IDispatchedHandlerVtbl dispatched_table = {dispatched_query, dispatched_addref, dispatched_release, dispatched_invoke};
static const exiting_vtable exiting_table = {exiting_query, exiting_addref, exiting_release, exiting_invoke};

static DWORD WINAPI run_application(void *context)
{
	struct client *self = context;
	self->ui_thread = GetCurrentThreadId();
	self->result = ICoreApplication_Run(self->app, &self->source);
	ICoreApplicationView *view = (void *)1;
	if (ICoreApplication_GetCurrentView(self->app, &view) != (HRESULT)0x8000000e || view) return 1;
	return 0;
}

int raven_entry(void)
{
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	factory_function factory = runtime ? (void *)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	HRESULT (WINAPI *initialize)(int) = runtime ? (void *)GetProcAddress(runtime, "RoInitialize") : NULL;
	if (!factory || !initialize || initialize(1) != S_OK) return 1;
	static const WCHAR name[] = L"Windows.ApplicationModel.Core.CoreApplication";
	HSTRING key = NULL;
	if (WindowsCreateString(name, ARRAYSIZE(name) - 1, &key) != S_OK) return 2;
	ICoreApplication *app = NULL;
	if (factory(key, &IID_ICoreApplication, (void **)&app) != S_OK || !app) return 3;
	WindowsDeleteString(key);
	ICoreApplicationExit *exit = NULL;
	IActivationFactory *activation = NULL;
	if (ICoreApplication_QueryInterface(app, &IID_ICoreApplicationExit, (void **)&exit) != S_OK ||
		ICoreApplication_QueryInterface(app, &IID_IActivationFactory, (void **)&activation) != S_OK) return 4;
	IUnknown *a = NULL, *b = NULL;
	ICoreApplication_QueryInterface(app, &IID_IUnknown, (void **)&a);
	ICoreApplicationExit_QueryInterface(exit, &IID_IUnknown, (void **)&b);
	if (!a || a != b) return 5;
	IUnknown_Release(a);
	IUnknown_Release(b);
	IInspectable *instance = (void *)1;
	if (IActivationFactory_ActivateInstance(activation, &instance) != (HRESULT)0x8000000e || instance) return 6;
	IActivationFactory_Release(activation);
	ICoreApplicationView *absent = (void *)1;
	if (ICoreApplication_GetCurrentView(app, &absent) != (HRESULT)0x8000000e || absent ||
		ICoreApplication_Run(app, NULL) != E_POINTER || ICoreApplicationExit_Exit(exit) != (HRESULT)0x8000000e) return 7;
	HSTRING id = (void *)1;
	if (ICoreApplication_get_Id(app, &id) != HRESULT_FROM_WIN32(15700) || id) return 12;
	/* Properties: the process-wide property set, handed out as the same object and honest through its
	 * IMap<HSTRING, IInspectable> shape. */
	__x_ABI_CWindows_CFoundation_CCollections_CIPropertySet *properties = (void *)1, *properties_again = NULL;
	if (ICoreApplication_get_Properties(app, &properties) != S_OK || !properties) return 13;
	if (ICoreApplication_get_Properties(app, &properties_again) != S_OK || properties_again != properties) return 13;
	/* the engine holds its own reference on the process-wide instance, so two guest references balance to two */
	if (properties_again->lpVtbl->Release(properties_again) != 2) return 13;
	__FIMap_2_HSTRING_IInspectable *map = NULL;
	if (properties->lpVtbl->QueryInterface(properties, &IID___FIMap_2_HSTRING_IInspectable, (void **)&map) != S_OK ||
		!map) return 60;
	UINT32 size = 1;
	if (map->lpVtbl->get_Size(map, &size) != S_OK || size) return 61;
	HSTRING property_key = NULL, property_key_again = NULL, property_other = NULL;
	if (WindowsCreateString(L"One", 3, &property_key) != S_OK ||
		WindowsCreateString(L"One", 3, &property_key_again) != S_OK ||
		WindowsCreateString(L"Two", 3, &property_other) != S_OK) return 62;
	boolean replaced = TRUE;
	if (map->lpVtbl->Insert(map, property_key, (IInspectable *)app, &replaced) != S_OK || replaced) return 63;
	if (map->lpVtbl->get_Size(map, &size) != S_OK || size != 1) return 64;
	IInspectable *value = (void *)1;
	if (map->lpVtbl->Lookup(map, property_key, &value) != S_OK || value != (IInspectable *)app) return 65;
	value->lpVtbl->Release(value);
	boolean found = FALSE;
	if (map->lpVtbl->HasKey(map, property_key, &found) != S_OK || !found ||
		map->lpVtbl->HasKey(map, property_other, &found) != S_OK || found) return 66;
	/* string keys compare by content: a different HSTRING with the same text replaces */
	replaced = FALSE;
	if (map->lpVtbl->Insert(map, property_key_again, NULL, &replaced) != S_OK || !replaced) return 67;
	if (map->lpVtbl->Lookup(map, property_key, &value) != S_OK || value) return 68;
	if (map->lpVtbl->Insert(map, property_other, (IInspectable *)app, &replaced) != S_OK || replaced) return 69;
	if (map->lpVtbl->get_Size(map, &size) != S_OK || size != 2) return 70;
	__FIMapView_2_HSTRING_IInspectable *view = NULL;
	if (map->lpVtbl->GetView(map, &view) != S_OK || !view) return 71;
	if (view->lpVtbl->get_Size(view, &size) != S_OK || size != 2 ||
		view->lpVtbl->HasKey(view, property_other, &found) != S_OK || !found ||
		view->lpVtbl->Lookup(view, property_key, &value) != S_OK || value) return 72;
	view->lpVtbl->Release(view);
	if (map->lpVtbl->Remove(map, property_key) != S_OK || map->lpVtbl->Remove(map, property_key) != E_BOUNDS ||
		map->lpVtbl->Lookup(map, property_key, &value) != E_BOUNDS) return 73;
	if (map->lpVtbl->get_Size(map, &size) != S_OK || size != 1) return 74;
	if (map->lpVtbl->Clear(map) != S_OK || map->lpVtbl->get_Size(map, &size) != S_OK || size ||
		map->lpVtbl->HasKey(map, property_other, &found) != S_OK || found) return 75;
	/* a NULL HSTRING is the empty key the way every HSTRING argument reads */
	replaced = TRUE;
	if (map->lpVtbl->Insert(map, NULL, (IInspectable *)app, &replaced) != S_OK || replaced ||
		map->lpVtbl->Lookup(map, NULL, &value) != S_OK || value != (IInspectable *)app ||
		map->lpVtbl->Remove(map, NULL) != S_OK || map->lpVtbl->get_Size(map, &size) != S_OK || size) return 76;
	WindowsDeleteString(property_key);
	WindowsDeleteString(property_key_again);
	WindowsDeleteString(property_other);
	map->lpVtbl->Release(map);
	/* the class name the properties object claims, once the class is activatable */
	HSTRING properties_class = NULL;
	if (((IInspectable *)properties)->lpVtbl->GetRuntimeClassName((IInspectable *)properties, &properties_class) != S_OK ||
		!properties_class) return 86;
	{
		UINT32 length = 0;
		const WCHAR *text = WindowsGetStringRawBuffer(properties_class, &length);
		static const WCHAR claimed[] = L"Windows.Foundation.Collections.PropertySet";
		if (length != ARRAYSIZE(claimed) - 1 ||
			CompareStringOrdinal(text, (int)length, claimed, -1, FALSE) != CSTR_EQUAL) return 87;
	}
	WindowsDeleteString(properties_class);
	properties->lpVtbl->Release(properties);
	/* PropertySet activation: the factory creates independent instances. The class routes through
	 * RoGetActivationFactory once its catalog entry lands; until then the engine's exported factory serves the
	 * same object, and this check runs against whichever route is wired (route 1 = the chain, 0 = the export). */
	HMODULE component_properties = GetModuleHandleW(L"raven-xaml.dll");
	typedef HRESULT (WINAPI *property_factory_function)(HSTRING, IActivationFactory **);
	property_factory_function property_factory = component_properties ?
		(property_factory_function)GetProcAddress(component_properties, "raven_property_set_factory") : NULL;
	if (!property_factory) return 90;
	static const WCHAR property_set_class[] = L"Windows.Foundation.Collections.PropertySet";
	HSTRING class_name = NULL;
	if (WindowsCreateString(property_set_class, ARRAYSIZE(property_set_class) - 1, &class_name) != S_OK) return 91;
	IActivationFactory *property_factory_export = NULL;
	if (property_factory(class_name, &property_factory_export) != S_OK || !property_factory_export) return 92;
	IActivationFactory *property_factory_chain = NULL;
	HRESULT chained = factory(class_name, &IID_IActivationFactory, (void **)&property_factory_chain);
	int route = 1;
	if (chained == S_OK) {
		/* the chain resolves through the same DllGetActivationFactory, so it must answer the same factory */
		if (property_factory_chain != property_factory_export) return 93;
		IActivationFactory_Release(property_factory_export);
	} else {
		if (chained != (HRESULT)0x80040154 || property_factory_chain) return 94;
		route = 0;
		property_factory_chain = property_factory_export;
	}
	(void)route;
	IInspectable *first = NULL, *second = NULL;
	if (IActivationFactory_ActivateInstance(property_factory_chain, &first) != S_OK || !first) return 95;
	if (IActivationFactory_ActivateInstance(property_factory_chain, &second) != S_OK || !second) return 95;
	if (first == second) return 96;
	/* the process-wide Properties object is neither of the activated instances and still the same across gets */
	__x_ABI_CWindows_CFoundation_CCollections_CIPropertySet *properties_third = (void *)1, *properties_fourth = NULL;
	if (ICoreApplication_get_Properties(app, &properties_third) != S_OK || !properties_third ||
		ICoreApplication_get_Properties(app, &properties_fourth) != S_OK || properties_fourth != properties_third ||
		properties_third == (__x_ABI_CWindows_CFoundation_CCollections_CIPropertySet *)first ||
		properties_third == (__x_ABI_CWindows_CFoundation_CCollections_CIPropertySet *)second) return 97;
	properties_fourth->lpVtbl->Release(properties_fourth);
	properties_third->lpVtbl->Release(properties_third);
	/* each instance answers the IPropertySet and the IMap shapes */
	__x_ABI_CWindows_CFoundation_CCollections_CIPropertySet *set_first = NULL, *set_second = NULL;
	__FIMap_2_HSTRING_IInspectable *map_first = NULL, *map_second = NULL;
	if (first->lpVtbl->QueryInterface(first, &IID___x_ABI_CWindows_CFoundation_CCollections_CIPropertySet, (void **)&set_first) != S_OK ||
		!set_first ||
		second->lpVtbl->QueryInterface(second, &IID___x_ABI_CWindows_CFoundation_CCollections_CIPropertySet, (void **)&set_second) != S_OK ||
		!set_second ||
		set_first->lpVtbl->QueryInterface(set_first, &IID___FIMap_2_HSTRING_IInspectable, (void **)&map_first) != S_OK ||
		!map_first ||
		set_second->lpVtbl->QueryInterface(set_second, &IID___FIMap_2_HSTRING_IInspectable, (void **)&map_second) != S_OK ||
		!map_second) return 98;
	HSTRING instance_key = NULL;
	if (WindowsCreateString(L"Fixture.Instance", 16, &instance_key) != S_OK) return 99;
	boolean instance_replaced = TRUE;
	if (map_first->lpVtbl->Insert(map_first, instance_key, (IInspectable *)app, &instance_replaced) != S_OK ||
		instance_replaced) return 100;
	UINT32 first_size = 0, second_size = 1;
	if (map_first->lpVtbl->get_Size(map_first, &first_size) != S_OK || first_size != 1 ||
		map_second->lpVtbl->get_Size(map_second, &second_size) != S_OK || second_size) return 100;
	IInspectable *instance_value = (void *)1;
	if (map_second->lpVtbl->Lookup(map_second, instance_key, &instance_value) != E_BOUNDS || instance_value) return 101;
	instance_replaced = TRUE;
	if (map_second->lpVtbl->Insert(map_second, instance_key, (IInspectable *)exit, &instance_replaced) != S_OK ||
		instance_replaced) return 101;
	if (map_second->lpVtbl->Lookup(map_second, instance_key, &instance_value) != S_OK ||
		instance_value != (IInspectable *)exit) return 101;
	instance_value->lpVtbl->Release(instance_value);
	instance_value = (void *)1;
	if (map_first->lpVtbl->Lookup(map_first, instance_key, &instance_value) != S_OK ||
		instance_value != (IInspectable *)app) return 101;
	instance_value->lpVtbl->Release(instance_value);
	/* the view of one instance sees only its own entries */
	__FIMapView_2_HSTRING_IInspectable *view_second = NULL;
	if (map_second->lpVtbl->GetView(map_second, &view_second) != S_OK || !view_second) return 102;
	boolean instance_found = FALSE;
	if (view_second->lpVtbl->get_Size(view_second, &second_size) != S_OK || second_size != 1 ||
		view_second->lpVtbl->HasKey(view_second, instance_key, &instance_found) != S_OK || !instance_found) return 102;
	view_second->lpVtbl->Release(view_second);
	/* GetRuntimeClassName answers the claimed runtime class name on the activated instances */
	for (int which = 0; which < 2; ++which) {
		IInspectable *instance = which ? second : first;
		HSTRING instance_class = NULL;
		if (instance->lpVtbl->GetRuntimeClassName(instance, &instance_class) != S_OK || !instance_class) return 103;
		UINT32 length = 0;
		const WCHAR *text = WindowsGetStringRawBuffer(instance_class, &length);
		if (length != ARRAYSIZE(property_set_class) - 1 ||
			CompareStringOrdinal(text, (int)length, property_set_class, -1, FALSE) != CSTR_EQUAL) return 103;
		WindowsDeleteString(instance_class);
	}
	/* the releases balance to zero per instance: the entries and the critical section go with the last reference */
	if (map_second->lpVtbl->Release(map_second) != 2) return 104;
	if (map_first->lpVtbl->Release(map_first) != 2) return 104;
	if (set_second->lpVtbl->Release(set_second) != 1) return 104;
	if (set_first->lpVtbl->Release(set_first) != 1) return 104;
	if (second->lpVtbl->Release(second) != 0) return 105;
	if (first->lpVtbl->Release(first) != 0) return 105;
	IActivationFactory_Release(property_factory_chain);
	WindowsDeleteString(instance_key);
	WindowsDeleteString(class_name);
	void *missing = (void *)1;
	if (ICoreApplication_QueryInterface(app, &IID_IFrameworkView, &missing) != E_NOINTERFACE || missing) return 14;
	HMODULE component = GetModuleHandleW(L"raven-xaml.dll");
	/* The manifest scanner's Application/Id reader, through the engine-internal seam: this harness runs without a
	 * package layout (the identity is read once per process, so both branches cannot run here), so the reader is
	 * checked on manifests from memory, parsed by the same scanner the manifest on disk goes through. */
	typedef HRESULT (WINAPI *manifest_id_function)(const BYTE *, UINT32, HSTRING *);
	manifest_id_function manifest_id = component ?
		(manifest_id_function)GetProcAddress(component, "raven_package_manifest_application_id") : NULL;
	if (!manifest_id) return 80;
	static const char with_application[] =
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
		"<Package xmlns=\"http://schemas.microsoft.com/appx/manifest/foundation/windows10\">"
		"<Identity Name=\"Raven.Fixture\" Publisher=\"CN=Raven\" Version=\"1.2.3.4\"/>"
		"<Applications><Application Id=\"Raven.Fixture.App\" EntryPoint=\"Raven.Fixture.App\">"
		"<uap:VisualElements DisplayName=\"Raven\"/></Application></Applications>"
		"</Package>";
	HSTRING praid = (void *)1;
	if (manifest_id((const BYTE *)with_application, (UINT32)sizeof with_application - 1, &praid) != S_OK || !praid)
		return 81;
	{
		UINT32 length = 0;
		const WCHAR *text = WindowsGetStringRawBuffer(praid, &length);
		if (length != 17 || CompareStringOrdinal(text, (int)length, L"Raven.Fixture.App", -1, FALSE) != CSTR_EQUAL)
			return 82;
	}
	WindowsDeleteString(praid);
	static const char without_application[] =
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
		"<Package xmlns=\"http://schemas.microsoft.com/appx/manifest/foundation/windows10\">"
		"<Identity Name=\"Raven.Fixture\" Publisher=\"CN=Raven\" Version=\"1.2.3.4\"/>"
		"</Package>";
	praid = (void *)1;
	if (manifest_id((const BYTE *)without_application, (UINT32)sizeof without_application - 1, &praid) != S_OK ||
		WindowsGetStringLen(praid)) return 83;
	WindowsDeleteString(praid);
	/* The manifest scanner's Applications/Application@EntryPoint reader, through the same seam shape: this is the
	 * runtime class name a packaged activation carries into IFrameworkView::Load. The harness runs without a package
	 * layout, so the in-process reader answers nothing here and Load receives no string (checked in framework_load). */
	typedef HRESULT (WINAPI *manifest_entry_function)(const BYTE *, UINT32, HSTRING *);
	manifest_entry_function manifest_entry = component ?
		(manifest_entry_function)GetProcAddress(component, "raven_package_manifest_application_entry_point") : NULL;
	if (!manifest_entry) return 84;
	HSTRING entry = (void *)1;
	if (manifest_entry((const BYTE *)with_application, (UINT32)sizeof with_application - 1, &entry) != S_OK || !entry)
		return 85;
	{
		UINT32 length = 0;
		const WCHAR *text = WindowsGetStringRawBuffer(entry, &length);
		if (length != 17 || CompareStringOrdinal(text, (int)length, L"Raven.Fixture.App", -1, FALSE) != CSTR_EQUAL)
			return 85;
	}
	WindowsDeleteString(entry);
	entry = (void *)1;
	if (manifest_entry((const BYTE *)without_application, (UINT32)sizeof without_application - 1, &entry) != S_OK ||
		WindowsGetStringLen(entry)) return 88;
	WindowsDeleteString(entry);
	HRESULT (WINAPI *can_unload)(void) = component ? (void *)GetProcAddress(component, "DllCanUnloadNow") : NULL;
	if (!can_unload || can_unload() != S_OK) return 8;
	static const int expected[] = {511, 1, 3, 391, 399, 447, 415, 511, 447, 31, 1};
	for (int mode = 0; mode < 11; ++mode) {
		struct client self = {0};
		self.source.lpVtbl = &source_table;
		self.framework.lpVtbl = &framework_table;
		self.activated.lpVtbl = &activated_table;
		self.dispatched.lpVtbl = &dispatched_table;
		self.exiting.lpVtbl = &exiting_table;
		self.refs = 1;
		self.app = app;
		self.exit = exit;
		self.mode = mode;
		self.result = E_PENDING;
		self.activated_event = CreateEventW(NULL, TRUE, FALSE, NULL);
		HANDLE thread = CreateThread(NULL, 0, run_application, &self, 0, NULL);
		if (!self.activated_event || !thread) return 9;
		if (mode == 8) {
			if (WaitForSingleObject(self.activated_event, 5000) != WAIT_OBJECT_0 ||
				ICoreApplication_GetCurrentView(app, &absent) != (HRESULT)0x8000000e || absent ||
				ICoreApplication_Run(app, &self.source) != (HRESULT)0x8000000e || ICoreApplicationExit_Exit(exit) != S_OK) return 10;
		}
		DWORD code;
		if (WaitForSingleObject(thread, 5000) != WAIT_OBJECT_0 || !GetExitCodeThread(thread, &code) || code) return 11;
		if (self.failure) return self.failure;
		HRESULT expected_result = mode == 9 ? E_PENDING : mode == 10 ? E_UNEXPECTED :
			(mode >= 1 && mode <= 7) ? E_ACCESSDENIED : S_OK;
		if (self.result != expected_result || self.stages != expected[mode]) return 20 + mode;
		ICoreApplicationExit_remove_Exiting(exit, self.exit_token);
		if (self.args) IActivatedEventArgs_Release(self.args);
		if (self.dispatcher) ICoreDispatcher_Release(self.dispatcher);
		if (self.window) ICoreWindow_Release(self.window);
		if (self.view) ICoreApplicationView_Release(self.view);
		if (self.refs != 1 || can_unload() != S_OK) return 40 + mode;
		CloseHandle(thread);
		CloseHandle(self.activated_event);
	}
	ICoreApplicationExit_Release(exit);
	ICoreApplication_Release(app);
	return 0;
}
