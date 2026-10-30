#define INITGUID
#define CONST_VTABLE
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_Foundation
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <windows.ui.core.h>

/* The RunIdleAsync fixture: an idle callback runs only after queued normal work and reports IsDispatcherIdle,
 * the returned action completes, cancels and fails like every other dispatcher action, a re-queueing idle
 * callback interleaves with normal work a bounded number of times, and the pump still waits for cross-thread
 * idle work instead of spinning. Close completes pending and new idle actions without invoking callbacks. */
#define E_ILLEGAL_METHOD_CALL ((HRESULT)0x8000000e)
struct handler { const void *table; LONG refs; struct client *client; int value; };
struct client {
	ICoreWindowStatic *windows;
	ICoreWindow *window;
	ICoreDispatcher *dispatcher;
	IAsyncAction *worker_action;
	struct handler runs[3], idle, done;
	DWORD thread;
	int used, requeued, requeue, fail_idle, completed_count, failure, sequence[48];
	BOOL is_idle[48];
	AsyncStatus completed_status;
	IAsyncAction *expect_action;
	HRESULT result;
};
#define CHECK(condition) do { if (!(condition)) { self->failure = __LINE__; return E_FAIL; } } while (0)
static HRESULT record(struct client *self, int value, BOOL is_idle)
{
	CHECK(GetCurrentThreadId() == self->thread && self->used < 48);
	self->sequence[self->used] = value;
	self->is_idle[self->used] = is_idle;
	++self->used;
	return S_OK;
}
static HRESULT run_invoke(void *iface)
{
	struct handler *handler = iface;
	return record(handler->client, handler->value, FALSE);
}
static HRESULT idle_invoke(void *iface, IIdleDispatchedHandlerArgs *args)
{
	struct handler *handler = iface;
	struct client *self = handler->client;
	boolean is_idle = FALSE;
	CHECK(args && IIdleDispatchedHandlerArgs_get_IsDispatcherIdle(args, &is_idle) == S_OK);
	CHECK(record(self, handler->value, is_idle ? TRUE : FALSE) == S_OK);
	if (self->fail_idle) return E_ACCESSDENIED;
	if (self->requeue && self->requeued < 3) {
		/* Post the normal work first, then re-queue the idle callback, so the normal item still dispatches
		 * first and the re-queued callback reads IsDispatcherIdle true. */
		IAsyncAction *action = NULL;
		self->runs[2].value = 30 + self->requeued;
		if (ICoreDispatcher_RunAsync(self->dispatcher, CoreDispatcherPriority_Normal, (void *)&self->runs[2], &action) != S_OK || !action) return E_FAIL;
		IAsyncAction_Release(action);
		if (ICoreDispatcher_RunIdleAsync(self->dispatcher, (void *)&self->idle, &action) != S_OK || !action) return E_FAIL;
		IAsyncAction_Release(action);
		++self->requeued;
	}
	return S_OK;
}
static HRESULT done_invoke(void *iface, IAsyncAction *sender, AsyncStatus status)
{
	struct handler *handler = iface;
	struct client *self = handler->client;
	CHECK(GetCurrentThreadId() == self->thread && sender && sender == self->expect_action);
	++self->completed_count;
	self->completed_status = status;
	return S_OK;
}

static HRESULT WINAPI run_query(IDispatchedHandler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI run_addref(IDispatchedHandler *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI run_release(IDispatchedHandler *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI run_invoke_abi(IDispatchedHandler *iface) { return run_invoke(iface); }
static const IDispatchedHandlerVtbl run_table = {run_query, run_addref, run_release, run_invoke_abi};

static HRESULT WINAPI idle_query(IIdleDispatchedHandler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI idle_addref(IIdleDispatchedHandler *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI idle_release(IIdleDispatchedHandler *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI idle_invoke_abi(IIdleDispatchedHandler *iface, IIdleDispatchedHandlerArgs *args) { return idle_invoke(iface, args); }
static const IIdleDispatchedHandlerVtbl idle_table = {idle_query, idle_addref, idle_release, idle_invoke_abi};

static HRESULT WINAPI done_query(IAsyncActionCompletedHandler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI done_addref(IAsyncActionCompletedHandler *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI done_release(IAsyncActionCompletedHandler *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI done_invoke_abi(IAsyncActionCompletedHandler *iface, IAsyncAction *sender, AsyncStatus status) { return done_invoke(iface, sender, status); }
static const IAsyncActionCompletedHandlerVtbl done_table = {done_query, done_addref, done_release, done_invoke_abi};

static IAsyncInfo *action_info(IAsyncAction *action)
{
	IAsyncInfo *info = NULL;
	return IAsyncAction_QueryInterface(action, &IID_IAsyncInfo, (void **)&info) == S_OK ? info : NULL;
}
static int check_action(struct client *self, IAsyncAction *action, AsyncStatus want_status, HRESULT want_error)
{
	IAsyncInfo *info = action_info(action);
	AsyncStatus status = Started;
	HRESULT error = S_OK;
	(void)self;
	if (!info) return __LINE__;
	if (IAsyncInfo_get_Status(info, &status) != S_OK || status != want_status ||
		IAsyncInfo_get_ErrorCode(info, &error) != S_OK || error != want_error) {
		IAsyncInfo_Release(info);
		return __LINE__;
	}
	IAsyncInfo_Release(info);
	return IAsyncAction_GetResults(action) == want_error ? 0 : __LINE__;
}
static HRESULT cancel_action(IAsyncAction *action)
{
	IAsyncInfo *info = action_info(action);
	HRESULT result = E_FAIL;
	if (!info) return E_FAIL;
	result = IAsyncInfo_Cancel(info);
	IAsyncInfo_Release(info);
	return result;
}
/* ProcessOneAndAllPending blocks until something dispatches, so this loop proves the pump waits and wakes. */
static int wait_used(struct client *self, int count, DWORD timeout_ms)
{
	UINT64 start = GetTickCount64();
	while (self->used < count) {
		HRESULT result = ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessOneAndAllPending);
		if (FAILED(result)) return (int)result;
		if (self->failure) return self->failure;
		if (GetTickCount64() - start > timeout_ms) return __LINE__;
	}
	return 0;
}
static DWORD WINAPI idle_worker(void *context)
{
	struct client *self = context;
	boolean access = TRUE;
	if (ICoreDispatcher_get_HasThreadAccess(self->dispatcher, &access) != S_OK || access) return 1;
	Sleep(80);
	if (ICoreDispatcher_RunIdleAsync(self->dispatcher, (void *)&self->idle, &self->worker_action) != S_OK || !self->worker_action) return 2;
	return 0;
}

static HRESULT run_checks(struct client *self)
{
	IAsyncAction *actions[3] = {NULL, NULL, NULL};
	IAsyncInfo *info = NULL;
	AsyncStatus status = Started;
	UINT64 started;
	int used;

	/* An idle callback queued on a quiet dispatcher runs with IsDispatcherIdle true and completes its action. */
	self->idle.value = 1;
	CHECK(ICoreDispatcher_RunIdleAsync(self->dispatcher, (void *)&self->idle, &actions[0]) == S_OK && actions[0]);
	CHECK((info = action_info(actions[0])) != NULL);
	CHECK(IAsyncInfo_get_Status(info, &status) == S_OK && status == Started);
	CHECK(IAsyncAction_GetResults(actions[0]) == E_ILLEGAL_METHOD_CALL);
	IAsyncInfo_Release(info);
	self->expect_action = actions[0];
	CHECK(IAsyncAction_put_Completed(actions[0], (void *)&self->done) == S_OK);
	CHECK(ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) == S_OK);
	CHECK(self->used == 1 && self->sequence[0] == 1 && self->is_idle[0]);
	CHECK(check_action(self, actions[0], Completed, S_OK) == 0);
	CHECK(self->completed_count == 1 && self->completed_status == Completed);
	CHECK(cancel_action(actions[0]) == S_OK);
	CHECK(check_action(self, actions[0], Completed, S_OK) == 0);
	IAsyncAction_Release(actions[0]);
	self->completed_count = 0;
	self->used = 0;

	/* A normal callback queued while the idle item is pending runs first, and the idle callback reports
	 * IsDispatcherIdle false because work arrived after it was queued. */
	self->idle.value = 2;
	CHECK(ICoreDispatcher_RunIdleAsync(self->dispatcher, (void *)&self->idle, &actions[0]) == S_OK && actions[0]);
	self->runs[0].value = 3;
	CHECK(ICoreDispatcher_RunAsync(self->dispatcher, CoreDispatcherPriority_Normal, (void *)&self->runs[0], &actions[1]) == S_OK && actions[1]);
	CHECK(ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) == S_OK);
	CHECK(self->used == 2 && self->sequence[0] == 3 && !self->is_idle[0] && self->sequence[1] == 2 && !self->is_idle[1]);
	CHECK(check_action(self, actions[0], Completed, S_OK) == 0);
	IAsyncAction_Release(actions[0]);
	IAsyncAction_Release(actions[1]);
	self->used = 0;

	/* Priority and FIFO order are unchanged: High, then the Idle-priority RunAsync sibling, then RunIdleAsync —
	 * and idle-priority sibling work does not make the dispatcher busy. */
	self->runs[0].value = 4;
	CHECK(ICoreDispatcher_RunAsync(self->dispatcher, CoreDispatcherPriority_High, (void *)&self->runs[0], &actions[0]) == S_OK && actions[0]);
	self->runs[1].value = 5;
	CHECK(ICoreDispatcher_RunAsync(self->dispatcher, CoreDispatcherPriority_Idle, (void *)&self->runs[1], &actions[1]) == S_OK && actions[1]);
	self->idle.value = 6;
	CHECK(ICoreDispatcher_RunIdleAsync(self->dispatcher, (void *)&self->idle, &actions[2]) == S_OK && actions[2]);
	CHECK(ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) == S_OK);
	CHECK(self->used == 3 && self->sequence[0] == 4 && self->sequence[1] == 5 && self->sequence[2] == 6 && self->is_idle[2]);
	IAsyncAction_Release(actions[0]);
	IAsyncAction_Release(actions[1]);
	IAsyncAction_Release(actions[2]);
	self->used = 0;

	/* Cancelling before the callback runs prevents it and completes the action as Canceled. */
	self->idle.value = 7;
	CHECK(ICoreDispatcher_RunIdleAsync(self->dispatcher, (void *)&self->idle, &actions[0]) == S_OK && actions[0]);
	self->expect_action = actions[0];
	CHECK(IAsyncAction_put_Completed(actions[0], (void *)&self->done) == S_OK);
	CHECK(cancel_action(actions[0]) == S_OK);
	CHECK(check_action(self, actions[0], Canceled, HRESULT_FROM_WIN32(ERROR_CANCELLED)) == 0);
	CHECK(self->completed_count == 1 && self->completed_status == Canceled);
	CHECK(ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) == S_OK);
	CHECK(self->used == 0);
	IAsyncAction_Release(actions[0]);
	self->completed_count = 0;

	/* A failing handler still completes the action, with Error status, and leaves the queue working. */
	self->fail_idle = TRUE;
	self->idle.value = 8;
	CHECK(ICoreDispatcher_RunIdleAsync(self->dispatcher, (void *)&self->idle, &actions[0]) == S_OK && actions[0]);
	self->expect_action = actions[0];
	CHECK(IAsyncAction_put_Completed(actions[0], (void *)&self->done) == S_OK);
	CHECK(ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) == S_OK);
	CHECK(self->used == 1 && self->sequence[0] == 8);
	CHECK(check_action(self, actions[0], Error, E_ACCESSDENIED) == 0);
	CHECK(self->completed_count == 1 && self->completed_status == Error);
	self->fail_idle = FALSE;
	self->runs[0].value = 9;
	CHECK(ICoreDispatcher_RunAsync(self->dispatcher, CoreDispatcherPriority_Normal, (void *)&self->runs[0], &actions[1]) == S_OK && actions[1]);
	CHECK(ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) == S_OK);
	CHECK(self->used == 2 && self->sequence[1] == 9);
	IAsyncAction_Release(actions[0]);
	IAsyncAction_Release(actions[1]);
	self->completed_count = 0;
	self->used = 0;

	/* The pump waits for cross-thread idle work instead of spinning: the worker posts after 80 ms and the blocked
	 * ProcessEvents returns only once the item dispatched. */
	self->idle.value = 10;
	{
		HANDLE worker = CreateThread(NULL, 0, idle_worker, self, 0, NULL);
		DWORD code = 0;
		CHECK(worker);
		started = GetTickCount64();
		CHECK(wait_used(self, 1, 3000) == 0);
		CHECK(GetTickCount64() - started >= 60 && self->sequence[0] == 10 && self->is_idle[0]);
		CHECK(WaitForSingleObject(worker, 5000) == WAIT_OBJECT_0 && GetExitCodeThread(worker, &code) && !code);
		CloseHandle(worker);
	}
	CHECK(check_action(self, self->worker_action, Completed, S_OK) == 0);
	IAsyncAction_Release(self->worker_action);
	self->worker_action = NULL;
	self->used = 0;

	/* A self-re-queueing idle callback interleaves with normal work a bounded number of times and then drains:
	 * idle, normal, idle, normal, idle, normal, idle. */
	self->requeue = TRUE;
	self->idle.value = 11;
	CHECK(ICoreDispatcher_RunIdleAsync(self->dispatcher, (void *)&self->idle, &actions[0]) == S_OK && actions[0]);
	CHECK(ICoreDispatcher_ProcessEvents(self->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) == S_OK);
	CHECK(self->used == 7 && self->requeued == 3);
	CHECK(self->sequence[0] == 11 && self->is_idle[0] && self->sequence[1] == 30 && !self->is_idle[1] &&
		self->sequence[2] == 11 && self->is_idle[2] && self->sequence[3] == 31 && !self->is_idle[3] &&
		self->sequence[4] == 11 && self->is_idle[4] && self->sequence[5] == 32 && !self->is_idle[5] &&
		self->sequence[6] == 11 && self->is_idle[6]);
	IAsyncAction_Release(actions[0]);
	self->requeue = FALSE;
	self->requeued = 0;
	self->used = 0;

	/* Window close completes pending and new idle actions without invoking their callbacks, like RunAsync. */
	self->runs[0].value = 40;
	CHECK(ICoreDispatcher_RunAsync(self->dispatcher, CoreDispatcherPriority_Normal, (void *)&self->runs[0], &actions[0]) == S_OK && actions[0]);
	self->idle.value = 41;
	CHECK(ICoreDispatcher_RunIdleAsync(self->dispatcher, (void *)&self->idle, &actions[1]) == S_OK && actions[1]);
	self->expect_action = actions[1];
	CHECK(IAsyncAction_put_Completed(actions[1], (void *)&self->done) == S_OK);
	used = self->used;
	CHECK(ICoreWindow_Close(self->window) == S_OK);
	CHECK(self->used == used && self->completed_count == 1 && self->completed_status == Completed);
	CHECK(check_action(self, actions[0], Completed, S_OK) == 0);
	CHECK(check_action(self, actions[1], Completed, S_OK) == 0);
	self->idle.value = 42;
	CHECK(ICoreDispatcher_RunIdleAsync(self->dispatcher, (void *)&self->idle, &actions[2]) == S_OK && actions[2]);
	self->expect_action = actions[2];
	CHECK(IAsyncAction_put_Completed(actions[2], (void *)&self->done) == S_OK);
	CHECK(self->used == used && self->completed_count == 2 && self->completed_status == Completed);
	CHECK(check_action(self, actions[2], Completed, S_OK) == 0);
	IAsyncAction_Release(actions[0]);
	IAsyncAction_Release(actions[1]);
	IAsyncAction_Release(actions[2]);
	return S_OK;
}

static DWORD WINAPI run_ui(void *context)
{
	struct client *self = context;
	self->thread = GetCurrentThreadId();
	if (ICoreWindowStatic_GetForCurrentThread(self->windows, &self->window) != S_OK || !self->window) return 1;
	if (ICoreWindow_get_Dispatcher(self->window, &self->dispatcher) != S_OK || !self->dispatcher) return 2;
	self->result = run_checks(self);
	return self->failure ? self->failure : FAILED(self->result) ? 3 : 0;
}
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);
int raven_entry(void)
{
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	factory_function factory = runtime ? (void *)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	HRESULT (WINAPI *initialize)(int) = runtime ? (void *)GetProcAddress(runtime, "RoInitialize") : NULL;
	HRESULT (WINAPI *unload)(void);
	HSTRING key = NULL;
	ICoreWindowStatic *windows = NULL;
	static const WCHAR window_name[] = L"Windows.UI.Core.CoreWindow";
	struct client state = {0};
	HANDLE thread;
	DWORD code = 0;
	if (!factory || !initialize || initialize(1) != S_OK) return 1;
	if (WindowsCreateString(window_name, ARRAYSIZE(window_name) - 1, &key) != S_OK ||
		factory(key, &IID_ICoreWindowStatic, (void **)&windows) != S_OK) return 2;
	WindowsDeleteString(key);
	unload = (void *)GetProcAddress(GetModuleHandleW(L"raven-xaml.dll"), "DllCanUnloadNow");
	if (!unload || unload() != S_OK) return 3;
	state.windows = windows;
	state.result = E_PENDING;
	for (int i = 0; i < 3; ++i) state.runs[i] = (struct handler){&run_table, 1, &state, 0};
	state.idle = (struct handler){&idle_table, 1, &state, 0};
	state.done = (struct handler){&done_table, 1, &state, 0};
	thread = CreateThread(NULL, 0, run_ui, &state, 0, NULL);
	if (!thread || WaitForSingleObject(thread, 20000) != WAIT_OBJECT_0 || !GetExitCodeThread(thread, &code)) return 4;
	CloseHandle(thread);
	if (state.failure) return state.failure;
	if (code) return 1000 + code;
	if (state.result != S_OK) return 5;
	ICoreDispatcher_Release(state.dispatcher);
	ICoreWindow_Release(state.window);
	if (state.runs[0].refs != 1 || state.runs[1].refs != 1 || state.runs[2].refs != 1 ||
		state.idle.refs != 1 || state.done.refs != 1) return 6;
	if (unload() != S_OK) return 7;
	ICoreWindowStatic_Release(windows);
	return 0;
}
