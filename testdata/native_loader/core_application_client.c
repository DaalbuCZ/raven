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
	__x_ABI_CWindows_CFoundation_CCollections_CIPropertySet *properties = (void *)1;
	if (ICoreApplication_get_Properties(app, &properties) != E_NOTIMPL || properties) return 13;
	void *missing = (void *)1;
	if (ICoreApplication_QueryInterface(app, &IID_IFrameworkView, &missing) != E_NOINTERFACE || missing) return 14;
	HMODULE component = GetModuleHandleW(L"raven-xaml.dll");
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
