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
#ifndef THREAD_SYNCHRONIZE
#define THREAD_SYNCHRONIZE (0x00100000L)
#endif

typedef __FITypedEventHandler_2_Windows__CSystem__CDispatcherQueue_Windows__CSystem__CDispatcherQueueShutdownStartingEventArgs starting_handler;
typedef __FITypedEventHandler_2_Windows__CSystem__CDispatcherQueue_Windows__CSystem__CDispatcherQueueShutdownStartingEventArgsVtbl starting_vtable;
typedef __FITypedEventHandler_2_Windows__CSystem__CDispatcherQueue_IInspectable completed_handler;
typedef __FITypedEventHandler_2_Windows__CSystem__CDispatcherQueue_IInspectableVtbl completed_vtable;
struct client;
struct handler { const void *table; LONG refs; struct client *client; int value; };
struct client {
	IDispatcherQueueStatics *queue_statics;
	IDispatcherQueueController *controller;
	IDispatcherQueue *queue;
	IDispatcherQueue2 *access;
	IDispatcherQueueTimer *timer;
	IDispatcherQueueShutdownStartingEventArgs *args;
	IAsyncAction *shutdown;
	struct handler enqueue[4], last, starting, completed, action_done;
	DWORD controller_thread;
	HANDLE ran;
	int mode, failure, used, sequence[16], shutdown_events, action_calls;
};
#define CHECK(condition) do { if (!(condition)) { self->failure = __LINE__; return E_FAIL; } } while (0)
static HRESULT record(struct client *self, int value)
{
	DWORD thread = GetCurrentThreadId();
	if (!self->controller_thread) self->controller_thread = thread;
	CHECK(thread == self->controller_thread && self->used < 16);
	self->sequence[self->used++] = value;
	return S_OK;
}
static HRESULT enqueue_invoke(void *iface)
{
	struct handler *handler = iface;
	return record(handler->client, handler->value);
}
static HRESULT last_invoke(void *iface)
{
	struct client *self = ((struct handler *)iface)->client;
	/* The final operation enqueued from ShutdownStarting still dispatches before the loop exits. */
	return record(self, 5);
}
static HRESULT starting_invoke(void *iface, IDispatcherQueue *sender, IDispatcherQueueShutdownStartingEventArgs *args)
{
	struct client *self = ((struct handler *)iface)->client;
	CHECK(GetCurrentThreadId() == self->controller_thread && sender == self->queue && args && !self->shutdown_events);
	self->shutdown_events = 1;
	self->args = args;
	IDispatcherQueueShutdownStartingEventArgs_AddRef(args);
	IDeferral *deferral = (void *)1;
	CHECK(IDispatcherQueueShutdownStartingEventArgs_GetDeferral(args, &deferral) == E_NOTIMPL && !deferral);
	boolean accepted = FALSE;
	CHECK(IDispatcherQueue_TryEnqueue(sender, (void *)&self->last, &accepted) == S_OK && accepted);
	/* A failing handler must not stall the shutdown; the drain and the completed event still follow. */
	return self->mode == 1 ? E_ACCESSDENIED : S_OK;
}
static HRESULT completed_invoke(void *iface, IDispatcherQueue *sender, IInspectable *args)
{
	struct client *self = ((struct handler *)iface)->client;
	CHECK(GetCurrentThreadId() == self->controller_thread && sender == self->queue && !args && self->shutdown_events == 1);
	self->shutdown_events = 2;
	return S_OK;
}
static HRESULT action_done_invoke(void *iface, IAsyncAction *action, AsyncStatus status)
{
	struct client *self = ((struct handler *)iface)->client;
	CHECK(action == self->shutdown && status == Completed && !self->action_calls);
	self->action_calls = 1;
	/* ShutdownQueueAsync joins the dedicated thread, which has exited by the time this handler runs. */
	HANDLE thread = OpenThread(THREAD_SYNCHRONIZE, FALSE, self->controller_thread);
	CHECK(thread && WaitForSingleObject(thread, 0) == WAIT_OBJECT_0);
	CloseHandle(thread);
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

static HRESULT WINAPI last_query(IDispatcherQueueHandler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI last_addref(IDispatcherQueueHandler *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI last_release(IDispatcherQueueHandler *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI last_invoke_abi(IDispatcherQueueHandler *iface) { return last_invoke(iface); }
static const IDispatcherQueueHandlerVtbl last_table = {last_query, last_addref, last_release, last_invoke_abi};

static HRESULT WINAPI ping_query(IDispatcherQueueHandler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI ping_addref(IDispatcherQueueHandler *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI ping_release(IDispatcherQueueHandler *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI ping_invoke_abi(IDispatcherQueueHandler *iface)
{
	struct client *self = ((struct handler *)iface)->client;
	/* The dedicated thread's own queue is the controller's queue, and it has thread access there. */
	IDispatcherQueue *current = (void *)1;
	boolean access = FALSE;
	if (IDispatcherQueueStatics_GetForCurrentThread(self->queue_statics, &current) != S_OK || current != self->queue) return E_FAIL;
	IDispatcherQueue_Release(current);
	if (IDispatcherQueue2_get_HasThreadAccess(self->access, &access) != S_OK || !access) return E_FAIL;
	SetEvent(self->ran);
	return S_OK;
}
static const IDispatcherQueueHandlerVtbl ping_table = {ping_query, ping_addref, ping_release, ping_invoke_abi};

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

static HRESULT WINAPI action_done_query(IAsyncActionCompletedHandler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI action_done_addref(IAsyncActionCompletedHandler *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI action_done_release(IAsyncActionCompletedHandler *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI action_done_invoke_abi(IAsyncActionCompletedHandler *iface, IAsyncAction *action, AsyncStatus status) { return action_done_invoke(iface, action, status); }
static const IAsyncActionCompletedHandlerVtbl action_done_table = {action_done_query, action_done_addref, action_done_release, action_done_invoke_abi};

static int check_queue_post_shutdown(struct client *self)
{
	boolean accepted = TRUE;
	/* The queue behaves like the closed CoreWindow-owned queues after shutdown. */
	if (IDispatcherQueue_TryEnqueue(self->queue, (void *)&self->enqueue[0], &accepted) != S_OK || accepted) return 61;
	if (IDispatcherQueue_TryEnqueueWithPriority(self->queue, DispatcherQueuePriority_Normal, (void *)&self->enqueue[0], &accepted) != S_OK || accepted) return 62;
	IDispatcherQueueTimer *missing = (void *)1;
	if (IDispatcherQueue_CreateTimer(self->queue, &missing) != (HRESULT)0x80000013 || missing) return 63;
	boolean running = TRUE;
	if (IDispatcherQueueTimer_get_IsRunning(self->timer, &running) != S_OK || running) return 64;
	if (IDispatcherQueueTimer_Start(self->timer) != (HRESULT)0x80000013) return 65;
	EventRegistrationToken token;
	if (IDispatcherQueueTimer_add_Tick(self->timer, (void *)&self->enqueue[0], &token) != (HRESULT)0x80000013) return 66;
	boolean access = TRUE;
	if (IDispatcherQueue2_get_HasThreadAccess(self->access, &access) != S_OK || access) return 67;
	IDispatcherQueue *again = NULL;
	if (IDispatcherQueueController_get_DispatcherQueue(self->controller, &again) != S_OK || !again) return 68;
	IDispatcherQueue_Release(again);
	return 0;
}

static int run_checks(struct client *self)
{
	boolean accepted = TRUE, access = TRUE;
	IDispatcherQueue *absent = (void *)1;
	/* The main thread owns no CoreWindow, so the controller's queue is not the current one here. */
	if (IDispatcherQueueStatics_GetForCurrentThread(self->queue_statics, &absent) != S_OK || absent) return 1;
	HSTRING name = NULL;
	static const WCHAR controller_name[] = L"Windows.System.DispatcherQueueController";
	if (IDispatcherQueueController_GetRuntimeClassName(self->controller, &name) != S_OK) return 2;
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	if (length != ARRAYSIZE(controller_name) - 1 || CompareStringOrdinal(text, (int)length, controller_name, -1, FALSE) != CSTR_EQUAL) return 3;
	WindowsDeleteString(name);
	if (IDispatcherQueueController_get_DispatcherQueue(self->controller, &self->queue) != S_OK || !self->queue) return 4;
	IDispatcherQueue *again = NULL;
	if (IDispatcherQueueController_get_DispatcherQueue(self->controller, &again) != S_OK || again != self->queue) return 5;
	IDispatcherQueue_Release(again);
	if (IDispatcherQueue_QueryInterface(self->queue, &IID_IDispatcherQueue2, (void **)&self->access) != S_OK ||
		IDispatcherQueue2_get_HasThreadAccess(self->access, &access) != S_OK || access) return 6;
	if (IDispatcherQueue_CreateTimer(self->queue, &self->timer) != S_OK) return 7;
	EventRegistrationToken token;
	if (IDispatcherQueue_add_ShutdownStarting(self->queue, (void *)&self->starting, &token) != S_OK ||
		IDispatcherQueue_add_ShutdownCompleted(self->queue, (void *)&self->completed, &token) != S_OK) return 8;
	/* Cross-thread work executes on the dedicated thread, in priority order. */
	for (int i = 0; i < 3; ++i) {
		DispatcherQueuePriority priority = i == 2 ? DispatcherQueuePriority_High : DispatcherQueuePriority_Normal;
		if (IDispatcherQueue_TryEnqueueWithPriority(self->queue, priority, (void *)&self->enqueue[i], &accepted) != S_OK || !accepted) return 9;
	}
	if (IDispatcherQueue_TryEnqueueWithPriority(self->queue, DispatcherQueuePriority_Low, (void *)&self->enqueue[3], &accepted) != S_OK || !accepted) return 10;
	if (WaitForSingleObject(self->ran, 5000) != WAIT_OBJECT_0 || !self->controller_thread) return 11;
	if (self->used != 3 || self->sequence[0] != 1 || self->sequence[1] != 2 || self->sequence[2] != 3) return 12;
	HMODULE component = GetModuleHandleW(L"raven-xaml.dll");
	HRESULT (WINAPI *unload)(void) = component ? (void *)GetProcAddress(component, "DllCanUnloadNow") : NULL;
	if (!unload || unload() != S_FALSE) return 13;
	if (IDispatcherQueueController_ShutdownQueueAsync(self->controller, &self->shutdown) != S_OK || !self->shutdown) return 14;
	if (IAsyncAction_put_Completed(self->shutdown, (void *)&self->action_done) != S_OK || self->action_calls != 1) return 15;
	if (IAsyncAction_put_Completed(self->shutdown, (void *)&self->action_done) != (HRESULT)0x80000018) return 16;
	IAsyncInfo *info = NULL;
	if (IAsyncAction_QueryInterface(self->shutdown, &IID_IAsyncInfo, (void **)&info) != S_OK) return 17;
	AsyncStatus status;
	UINT32 id = 0;
	HRESULT error = E_FAIL;
	if (IAsyncInfo_get_Status(info, &status) != S_OK || status != Completed ||
		IAsyncInfo_get_ErrorCode(info, &error) != S_OK || error != S_OK ||
		IAsyncInfo_get_Id(info, &id) != S_OK || !id ||
		IAsyncAction_GetResults(self->shutdown) != S_OK) return 18;
	if (IAsyncInfo_Close(info) != S_OK || IAsyncInfo_get_Status(info, &status) != (HRESULT)0x80000013) return 19;
	IAsyncInfo_Release(info);
	int queue_result = check_queue_post_shutdown(self);
	if (queue_result) return queue_result;
	if (self->shutdown_events != 2 || self->used != 4 || self->sequence[3] != 5) return 20;
	IAsyncAction *repeat = (void *)1;
	if (IDispatcherQueueController_ShutdownQueueAsync(self->controller, &repeat) != (HRESULT)0x8000000e || repeat) return 21;
	if (unload() != S_FALSE) return 22;
	return 0;
}

int raven_entry(void)
{
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	HRESULT (WINAPI *initialize)(int) = runtime ? (void *)GetProcAddress(runtime, "RoInitialize") : NULL;
	HRESULT (WINAPI *factory)(HSTRING, REFIID, void **) = runtime ? (void *)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	if (!initialize || !factory || initialize(1) != S_OK) return 1;
	static const WCHAR controller_class[] = L"Windows.System.DispatcherQueueController";
	static const WCHAR queue_class[] = L"Windows.System.DispatcherQueue";
	HSTRING key = NULL;
	struct client states[2] = {{0}};
	IDispatcherQueueControllerStatics *controller_statics = NULL;
	IDispatcherQueueStatics *queue_statics = NULL;
	if (WindowsCreateString(controller_class, ARRAYSIZE(controller_class) - 1, &key) != S_OK ||
		factory(key, &IID_IDispatcherQueueControllerStatics, (void **)&controller_statics) != S_OK || !controller_statics) return 3;
	WindowsDeleteString(key);
	if (WindowsCreateString(queue_class, ARRAYSIZE(queue_class) - 1, &key) != S_OK ||
		factory(key, &IID_IDispatcherQueueStatics, (void **)&queue_statics) != S_OK || !queue_statics) return 4;
	WindowsDeleteString(key);
	HMODULE component = GetModuleHandleW(L"raven-xaml.dll");
	HRESULT (WINAPI *unload)(void) = component ? (void *)GetProcAddress(component, "DllCanUnloadNow") : NULL;
	if (!unload || unload() != S_OK) return 2;
	/* Each class answers only its own statics. */
	IUnknown *wrong = (void *)1;
	if (IUnknown_QueryInterface(controller_statics, &IID_IDispatcherQueueStatics, (void **)&wrong) != E_NOINTERFACE || wrong) return 5;
	wrong = (void *)1;
	if (IUnknown_QueryInterface(queue_statics, &IID_IDispatcherQueueControllerStatics, (void **)&wrong) != E_NOINTERFACE || wrong) return 6;
	for (int mode = 0; mode < 2; ++mode) {
		struct client *self = &states[mode];
		self->queue_statics = queue_statics;
		self->mode = mode;
		self->ran = CreateEventW(NULL, FALSE, FALSE, NULL);
		int values[] = {2, 3, 1};
		for (int i = 0; i < 3; ++i) self->enqueue[i] = (struct handler){&enqueue_table, 1, self, values[i]};
		self->enqueue[3] = (struct handler){&ping_table, 1, self, 0};
		self->last = (struct handler){&last_table, 1, self, 0};
		self->starting = (struct handler){&starting_table, 1, self, 0};
		self->completed = (struct handler){&completed_table, 1, self, 0};
		self->action_done = (struct handler){&action_done_table, 1, self, 0};
		if (!self->ran) return 7;
		if (IDispatcherQueueControllerStatics_CreateOnDedicatedThread(controller_statics, &self->controller) != S_OK || !self->controller) return 8;
		int result = run_checks(self);
		if (self->failure) return self->failure;
		if (result) return 100 + result;
		if (unload() != S_FALSE) return 9;
		IDispatcherQueueShutdownStartingEventArgs_Release(self->args);
		IDispatcherQueueTimer_Release(self->timer);
		IDispatcherQueue2_Release(self->access);
		IDispatcherQueue_Release(self->queue);
		IAsyncAction_Release(self->shutdown);
		IDispatcherQueueController_Release(self->controller);
		CloseHandle(self->ran);
		for (int i = 0; i < 4; ++i) if (self->enqueue[i].refs != 1) return 10;
		if (self->last.refs != 1 || self->starting.refs != 1 || self->completed.refs != 1 || self->action_done.refs != 1) return 11;
		if (unload() != S_OK) return 12;
	}
	IDispatcherQueueStatics_Release(queue_statics);
	IDispatcherQueueControllerStatics_Release(controller_statics);
	if (unload() != S_OK) return 13;
	return 0;
}
