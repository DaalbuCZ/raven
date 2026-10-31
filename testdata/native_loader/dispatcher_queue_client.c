#define INITGUID
#define CONST_VTABLE
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define WIDL_using_Windows_System
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_Foundation
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <windows.system.h>
#include <windows.ui.core.h>

typedef __FITypedEventHandler_2_Windows__CSystem__CDispatcherQueueTimer_IInspectable tick_handler;
typedef __FITypedEventHandler_2_Windows__CSystem__CDispatcherQueueTimer_IInspectableVtbl tick_vtable;
typedef __FITypedEventHandler_2_Windows__CSystem__CDispatcherQueue_Windows__CSystem__CDispatcherQueueShutdownStartingEventArgs starting_handler;
typedef __FITypedEventHandler_2_Windows__CSystem__CDispatcherQueue_Windows__CSystem__CDispatcherQueueShutdownStartingEventArgsVtbl starting_vtable;
typedef __FITypedEventHandler_2_Windows__CSystem__CDispatcherQueue_IInspectable completed_handler;
typedef __FITypedEventHandler_2_Windows__CSystem__CDispatcherQueue_IInspectableVtbl completed_vtable;
struct client;
struct handler { const void *table; LONG refs; struct client *client; int value; };
struct client {
	ICoreWindowStatic *windows;
	IDispatcherQueueStatics *statics;
	ICoreWindow *window;
	ICoreDispatcher *dispatcher;
	IDispatcherQueue *queue;
	IDispatcherQueue2 *access;
	IDispatcherQueueTimer *timer;
	IDispatcherQueueShutdownStartingEventArgs *shutdown_args;
	struct handler enqueue[4], idle, tick, starting, completed;
	DWORD thread;
	HWND hwnd;
	int mode, phase, failure, ticks, shutdown, used, sequence[32];
	BOOL fail_tick, fail_callback;
	UINT64 earliest;
	HRESULT result;
};
#define CHECK(condition) do { if (!(condition)) { self->failure = __LINE__; return E_FAIL; } } while (0)
static HRESULT record(struct client *self, int value)
{
	CHECK(GetCurrentThreadId() == self->thread && self->used < 32);
	self->sequence[self->used++] = value;
	return S_OK;
}
static HRESULT enqueue_invoke(void *iface)
{
	struct handler *handler = iface;
	struct client *self = handler->client;
	HRESULT result = record(self, handler->value);
	return self->fail_callback ? E_ACCESSDENIED : result;
}
static HRESULT tick_invoke(void *iface, IDispatcherQueueTimer *sender, IInspectable *args)
{
	struct client *self = ((struct handler *)iface)->client;
	CHECK(sender == self->timer && !args && GetCurrentThreadId() == self->thread && GetTickCount64() >= self->earliest);
	++self->ticks;
	if (self->phase == 0) {
		boolean running = TRUE;
		CHECK(IDispatcherQueueTimer_get_IsRunning(sender, &running) == S_OK && !running);
		CHECK(record(self, 5) == S_OK);
	}
	if (self->phase == 1 && self->ticks == 3) CHECK(IDispatcherQueueTimer_Stop(sender) == S_OK);
	if (self->phase == 2) CHECK(record(self, 7) == S_OK);
	return self->fail_tick ? E_ACCESSDENIED : S_OK;
}
static HRESULT starting_invoke(void *iface, IDispatcherQueue *sender, IDispatcherQueueShutdownStartingEventArgs *args)
{
	struct client *self = ((struct handler *)iface)->client;
	CHECK(GetCurrentThreadId() == self->thread && sender == self->queue && args && !self->shutdown);
	self->shutdown = 1;
	self->shutdown_args = args;
	IDispatcherQueueShutdownStartingEventArgs_AddRef(args);
	/* The immediate CoreWindow-owned close cannot wait for a deferral; only the controller's pump honors them. */
	IDeferral *deferral = (void *)1;
	CHECK(IDispatcherQueueShutdownStartingEventArgs_GetDeferral(args, &deferral) == E_NOTIMPL && !deferral);
	boolean accepted = TRUE;
	CHECK(IDispatcherQueue_TryEnqueue(sender, (void *)&self->enqueue[0], &accepted) == S_OK && !accepted);
	return self->mode == 1 ? E_ACCESSDENIED : S_OK;
}
static HRESULT completed_invoke(void *iface, IDispatcherQueue *sender, IInspectable *args)
{
	struct client *self = ((struct handler *)iface)->client;
	CHECK(GetCurrentThreadId() == self->thread && sender == self->queue && !args && self->shutdown == 1);
	self->shutdown = 2;
	boolean running = TRUE;
	CHECK(IDispatcherQueueTimer_get_IsRunning(self->timer, &running) == S_OK && !running);
	return S_OK;
}

static HRESULT WINAPI enqueue_query(IDispatcherQueueHandler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI enqueue_addref(IDispatcherQueueHandler *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI enqueue_release(IDispatcherQueueHandler *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI enqueue_invoke_abi(IDispatcherQueueHandler *iface) { return enqueue_invoke(iface); }
static const IDispatcherQueueHandlerVtbl enqueue_table = {enqueue_query, enqueue_addref, enqueue_release, enqueue_invoke_abi};

static HRESULT WINAPI idle_query(IDispatchedHandler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI idle_addref(IDispatchedHandler *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI idle_release(IDispatchedHandler *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI idle_invoke_abi(IDispatchedHandler *iface) { return enqueue_invoke(iface); }
static const IDispatchedHandlerVtbl idle_table = {idle_query, idle_addref, idle_release, idle_invoke_abi};

static HRESULT WINAPI tick_query(tick_handler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI tick_addref(tick_handler *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI tick_release(tick_handler *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI tick_invoke_abi(tick_handler *iface, IDispatcherQueueTimer *sender, IInspectable *args) { return tick_invoke(iface, sender, args); }
static const tick_vtable tick_table = {tick_query, tick_addref, tick_release, tick_invoke_abi};

static HRESULT WINAPI starting_query(starting_handler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI starting_addref(starting_handler *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI starting_release(starting_handler *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI starting_invoke_abi(starting_handler *iface, IDispatcherQueue *sender, IDispatcherQueueShutdownStartingEventArgs *args) { return starting_invoke(iface, sender, args); }
static const starting_vtable starting_table = {starting_query, starting_addref, starting_release, starting_invoke_abi};

static HRESULT WINAPI completed_query(completed_handler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI completed_addref(completed_handler *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI completed_release(completed_handler *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI completed_invoke_abi(completed_handler *iface, IDispatcherQueue *sender, IInspectable *args) { return completed_invoke(iface, sender, args); }
static const completed_vtable completed_table = {completed_query, completed_addref, completed_release, completed_invoke_abi};

static DWORD WINAPI post_worker(void *context)
{
	struct client *self = context;
	boolean access = TRUE, accepted = FALSE;
	IDispatcherQueue *absent = (void *)1;
	if (IDispatcherQueue2_get_HasThreadAccess(self->access, &access) != S_OK || access ||
		IDispatcherQueueStatics_GetForCurrentThread(self->statics, &absent) != S_OK || absent) return 1;
	if (IDispatcherQueue_TryEnqueue(self->queue, (void *)&self->enqueue[3], &accepted) != S_OK || !accepted) return 2;
	TimeSpan interval = {250000};
	if (IDispatcherQueueTimer_put_Interval(self->timer, interval) != S_OK ||
		IDispatcherQueueTimer_put_IsRepeating(self->timer, FALSE) != S_OK || IDispatcherQueueTimer_Start(self->timer) != S_OK) return 3;
	return 0;
}
static int wait_ticks(struct client *self, int count)
{
	while (self->ticks < count) {
		HRESULT result = ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessOneAndAllPending);
		if (FAILED(result)) return (int)result;
		if (self->failure) return self->failure;
	}
	return 0;
}
static int pending_timer(struct client *self)
{
	MSG message;
	if (!PeekMessageW(&message, self->hwnd, WM_TIMER, WM_TIMER, PM_REMOVE)) return __LINE__;
	DispatchMessageW(&message);
	return 0;
}
static HRESULT run_checks(struct client *self)
{
	boolean value = TRUE;
	TimeSpan interval;
	CHECK(IDispatcherQueueTimer_get_Interval(self->timer, &interval) == S_OK && interval.Duration == 0);
	CHECK(IDispatcherQueueTimer_get_IsRepeating(self->timer, &value) == S_OK && value);
	CHECK(IDispatcherQueueTimer_get_IsRunning(self->timer, &value) == S_OK && !value);
	TimeSpan negative = {-1};
	CHECK(IDispatcherQueueTimer_put_Interval(self->timer, negative) == E_INVALIDARG);
	TimeSpan maximum = {0x7fffffffffffffffLL};
	CHECK(IDispatcherQueueTimer_put_Interval(self->timer, maximum) == S_OK &&
		IDispatcherQueueTimer_Start(self->timer) == S_OK && IDispatcherQueueTimer_Stop(self->timer) == S_OK);
	boolean accepted = TRUE;
	CHECK(IDispatcherQueue_TryEnqueueWithPriority(self->queue, 7, (void *)&self->enqueue[0], &accepted) == E_INVALIDARG && !accepted);
	CHECK(IDispatcherQueue_TryEnqueue(self->queue, NULL, &accepted) == E_POINTER && !accepted);
	EventRegistrationToken token;
	CHECK(IDispatcherQueueTimer_add_Tick(self->timer, (void *)&self->tick, &token) == S_OK);
	CHECK(IDispatcherQueue_add_ShutdownStarting(self->queue, (void *)&self->starting, &token) == S_OK);
	CHECK(IDispatcherQueue_add_ShutdownCompleted(self->queue, (void *)&self->completed, &token) == S_OK);
	if (self->mode == 2) {
		interval.Duration = 10000;
		CHECK(IDispatcherQueueTimer_put_Interval(self->timer, interval) == S_OK && IDispatcherQueueTimer_Start(self->timer) == S_OK);
		ExitThread(0);
	}
	self->phase = 0;
	interval.Duration = 200000;
	CHECK(IDispatcherQueueTimer_put_Interval(self->timer, interval) == S_OK &&
		IDispatcherQueueTimer_put_IsRepeating(self->timer, FALSE) == S_OK && IDispatcherQueueTimer_Start(self->timer) == S_OK);
	Sleep(35);
	for (int i = 0; i < 3; ++i) {
		static const DispatcherQueuePriority priorities[] = {DispatcherQueuePriority_Low, DispatcherQueuePriority_Normal, DispatcherQueuePriority_High};
		CHECK(IDispatcherQueue_TryEnqueueWithPriority(self->queue, priorities[i], (void *)&self->enqueue[i], &accepted) == S_OK && accepted);
	}
	IAsyncAction *action = NULL;
	CHECK(ICoreDispatcher_RunAsync(self->dispatcher, CoreDispatcherPriority_Idle, (void *)&self->idle, &action) == S_OK);
	CHECK(ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) == S_OK);
	CHECK(self->used == 5 && self->ticks == 1);
	for (int i = 0; i < 5; ++i) CHECK(self->sequence[i] == i + 1);
	IAsyncAction_Release(action);
	self->phase = 1; self->ticks = 0;
	interval.Duration = 150000;
	CHECK(IDispatcherQueueTimer_put_Interval(self->timer, interval) == S_OK &&
		IDispatcherQueueTimer_put_IsRepeating(self->timer, TRUE) == S_OK && IDispatcherQueueTimer_Start(self->timer) == S_OK);
	CHECK(wait_ticks(self, 3) == 0 && IDispatcherQueueTimer_get_IsRunning(self->timer, &value) == S_OK && !value);
	self->phase = 2; self->ticks = self->used = 0;
	HANDLE worker = CreateThread(NULL, 0, post_worker, self, 0, NULL);
	DWORD code;
	CHECK(worker && WaitForSingleObject(worker, 5000) == WAIT_OBJECT_0 && GetExitCodeThread(worker, &code) && !code);
	CloseHandle(worker);
	CHECK(wait_ticks(self, 1) == 0 && self->used == 2 && self->sequence[0] == 6 && self->sequence[1] == 7);
	self->fail_callback = TRUE;
	CHECK(IDispatcherQueue_TryEnqueue(self->queue, (void *)&self->enqueue[0], &accepted) == S_OK && accepted);
	CHECK(ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) == E_ACCESSDENIED);
	self->fail_callback = FALSE;
	self->phase = 3; self->ticks = 0;
	interval.Duration = 200000;
	CHECK(IDispatcherQueueTimer_put_Interval(self->timer, interval) == S_OK && IDispatcherQueueTimer_Start(self->timer) == S_OK);
	Sleep(35);
	CHECK(pending_timer(self) == 0 && IDispatcherQueueTimer_Stop(self->timer) == S_OK);
	CHECK(ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) == S_OK && !self->ticks);
	self->phase = 4;
	CHECK(IDispatcherQueueTimer_Start(self->timer) == S_OK);
	Sleep(35);
	CHECK(pending_timer(self) == 0);
	interval.Duration = 1000000;
	self->earliest = GetTickCount64() + 100;
	CHECK(IDispatcherQueueTimer_put_Interval(self->timer, interval) == S_OK);
	CHECK(ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) == S_OK && !self->ticks);
	CHECK(wait_ticks(self, 1) == 0);
	self->earliest = 0;
	self->phase = 5; self->ticks = 0;
	interval.Duration = 300000;
	CHECK(IDispatcherQueueTimer_put_Interval(self->timer, interval) == S_OK &&
		IDispatcherQueueTimer_put_IsRepeating(self->timer, TRUE) == S_OK && IDispatcherQueueTimer_Start(self->timer) == S_OK);
	Sleep(45);
	CHECK(pending_timer(self) == 0 && IDispatcherQueueTimer_put_IsRepeating(self->timer, FALSE) == S_OK);
	CHECK(ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) == S_OK && !self->ticks);
	CHECK(wait_ticks(self, 1) == 0);
	Sleep(45);
	CHECK(ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) == S_OK && self->ticks == 1);
	self->phase = 6; self->ticks = 0;
	interval.Duration = 1000000;
	CHECK(IDispatcherQueueTimer_put_Interval(self->timer, interval) == S_OK && IDispatcherQueueTimer_Start(self->timer) == S_OK);
	Sleep(60);
	self->earliest = GetTickCount64() + 100;
	CHECK(IDispatcherQueueTimer_Start(self->timer) == S_OK);
	Sleep(50);
	CHECK(ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) == S_OK && !self->ticks);
	CHECK(wait_ticks(self, 1) == 0);
	self->earliest = 0;
	self->phase = 7; self->ticks = 0; self->fail_tick = TRUE;
	interval.Duration = 100000;
	CHECK(IDispatcherQueueTimer_put_Interval(self->timer, interval) == S_OK && IDispatcherQueueTimer_Start(self->timer) == S_OK);
	CHECK(wait_ticks(self, 1) == (int)E_ACCESSDENIED && self->ticks == 1);
	self->fail_tick = FALSE;
	CHECK(IDispatcherQueueTimer_get_IsRunning(self->timer, &value) == S_OK && !value);
	/* Close with an active timer and ordinary work still pending. */
	CHECK(IDispatcherQueueTimer_put_IsRepeating(self->timer, TRUE) == S_OK && IDispatcherQueueTimer_Start(self->timer) == S_OK);
	CHECK(IDispatcherQueue_TryEnqueue(self->queue, (void *)&self->enqueue[0], &accepted) == S_OK && accepted);
	int used = self->used;
	HRESULT close = ICoreWindow_Close(self->window);
	CHECK(close == (self->mode == 1 ? E_ACCESSDENIED : S_OK) && self->shutdown == 2 && self->used == used);
	CHECK(IDispatcherQueue_TryEnqueue(self->queue, (void *)&self->enqueue[0], &accepted) == S_OK && !accepted);
	CHECK(IDispatcherQueueTimer_Start(self->timer) == (HRESULT)0x80000013);
	IDispatcherQueueTimer *missing = (void *)1;
	CHECK(IDispatcherQueue_CreateTimer(self->queue, &missing) == (HRESULT)0x80000013 && !missing);
	return S_OK;
}

#include "../../native/upstream/raven-xaml/corewindow_interop.h"
static DWORD WINAPI run_ui(void *context)
{
	struct client *self = context;
	self->thread = GetCurrentThreadId();
	IDispatcherQueue *absent = (void *)1;
	if (IDispatcherQueueStatics_GetForCurrentThread(self->statics, &absent) != S_OK || absent) return 1;
	if (ICoreWindowStatic_GetForCurrentThread(self->windows, &self->window) != S_OK ||
		ICoreWindow_get_Dispatcher(self->window, &self->dispatcher) != S_OK ||
		IDispatcherQueueStatics_GetForCurrentThread(self->statics, &self->queue) != S_OK || !self->queue) return 2;
	IDispatcherQueue *again = NULL;
	if (IDispatcherQueueStatics_GetForCurrentThread(self->statics, &again) != S_OK || again != self->queue) return 3;
	IDispatcherQueue_Release(again);
	boolean access = FALSE;
	if (IDispatcherQueue_QueryInterface(self->queue, &IID_IDispatcherQueue2, (void **)&self->access) != S_OK ||
		IDispatcherQueue2_get_HasThreadAccess(self->access, &access) != S_OK || !access) return 4;
	raven_core_window_interop *interop = NULL;
	if (ICoreWindow_QueryInterface(self->window, &raven_core_window_interop_iid, (void **)&interop) != S_OK ||
		interop->lpVtbl->get_WindowHandle(interop, &self->hwnd) != S_OK) return 5;
	interop->lpVtbl->Release(interop);
	if (IDispatcherQueue_CreateTimer(self->queue, &self->timer) != S_OK) return 6;
	self->result = run_checks(self);
	if (IDispatcherQueueStatics_GetForCurrentThread(self->statics, &absent) != S_OK || absent) return 7;
	return self->failure ? self->failure : FAILED(self->result) ? 8 : 0;
}
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);
int raven_entry(void)
{
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	factory_function factory = runtime ? (void *)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	HRESULT (WINAPI *initialize)(int) = runtime ? (void *)GetProcAddress(runtime, "RoInitialize") : NULL;
	if (!factory || !initialize || initialize(1) != S_OK) return 1;
	HSTRING key = NULL;
	ICoreWindowStatic *windows = NULL;
	IDispatcherQueueStatics *statics = NULL;
	static const WCHAR window_name[] = L"Windows.UI.Core.CoreWindow";
	static const WCHAR queue_name[] = L"Windows.System.DispatcherQueue";
	if (WindowsCreateString(window_name, ARRAYSIZE(window_name)-1, &key) != S_OK ||
		factory(key, &IID_ICoreWindowStatic, (void **)&windows) != S_OK) return 2;
	WindowsDeleteString(key);
	if (WindowsCreateString(queue_name, ARRAYSIZE(queue_name)-1, &key) != S_OK ||
		factory(key, &IID_IDispatcherQueueStatics, (void **)&statics) != S_OK) return 3;
	WindowsDeleteString(key);
	HRESULT (WINAPI *unload)(void) = (void *)GetProcAddress(GetModuleHandleW(L"raven-xaml.dll"), "DllCanUnloadNow");
	if (!unload || unload() != S_OK) return 4;
	for (int mode = 0; mode < 3; ++mode) {
		struct client state = {0};
		state.windows = windows; state.statics = statics; state.mode = mode; state.result = E_PENDING;
		int values[] = {3, 2, 1, 6};
		for (int i = 0; i < 4; ++i) state.enqueue[i] = (struct handler){&enqueue_table, 1, &state, values[i]};
		state.idle = (struct handler){&idle_table, 1, &state, 4};
		state.tick = (struct handler){&tick_table, 1, &state, 0};
		state.starting = (struct handler){&starting_table, 1, &state, 0};
		state.completed = (struct handler){&completed_table, 1, &state, 0};
		HANDLE thread = CreateThread(NULL, 0, run_ui, &state, 0, NULL);
		DWORD code = 0;
		if (!thread || WaitForSingleObject(thread, 10000) != WAIT_OBJECT_0 || !GetExitCodeThread(thread, &code)) return 5;
		CloseHandle(thread);
		if (state.failure) return state.failure;
		if (code) return 1000 + code;
		if (state.result != (mode == 2 ? E_PENDING : S_OK) || state.shutdown != 2) return 6;
		boolean running = TRUE, access = TRUE;
		if (IDispatcherQueueTimer_get_IsRunning(state.timer, &running) != S_OK || running ||
			IDispatcherQueue2_get_HasThreadAccess(state.access, &access) != S_OK || access) return 7;
		if (!state.shutdown_args) return 8;
		IDispatcherQueueShutdownStartingEventArgs_Release(state.shutdown_args);
		IDispatcherQueueTimer_Release(state.timer);
		IDispatcherQueue2_Release(state.access);
		IDispatcherQueue_Release(state.queue);
		ICoreDispatcher_Release(state.dispatcher);
		ICoreWindow_Release(state.window);
		for (int i = 0; i < 4; ++i) if (state.enqueue[i].refs != 1) return 9;
		if (state.idle.refs != 1 || state.tick.refs != 1 || state.starting.refs != 1 || state.completed.refs != 1 || unload() != S_OK) return 10;
	}
	IDispatcherQueueStatics_Release(statics);
	ICoreWindowStatic_Release(windows);
	return 0;
}
