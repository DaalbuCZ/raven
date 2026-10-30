#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <windows.ui.core.h>
#include <windows.foundation.h>

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);

/* The dispatcher handler and the completion handler are ordinary COM objects in this client. */
struct handler_object {
	const __x_ABI_CWindows_CUI_CCore_CIDispatchedHandlerVtbl *lpVtbl;
	LONG refs, calls;
	DWORD thread;
	HRESULT result;
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *close_window;
};

struct completion_object {
	const __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandlerVtbl *lpVtbl;
	LONG refs, calls;
	HANDLE signal;
	AsyncStatus expected;
};

static HRESULT WINAPI handler_query(__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID___x_ABI_CWindows_CUI_CCore_CIDispatchedHandler)) {
		*out = iface;
		InterlockedIncrement(&((struct handler_object *)iface)->refs);
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI handler_addref(__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler *iface)
{
	return (ULONG)InterlockedIncrement(&((struct handler_object *)iface)->refs);
}

static ULONG WINAPI handler_release(__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler *iface)
{
	return (ULONG)InterlockedDecrement(&((struct handler_object *)iface)->refs);
}

static HRESULT WINAPI handler_invoke(__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler *iface)
{
	struct handler_object *self = (struct handler_object *)iface;
	self->thread = GetCurrentThreadId();
	InterlockedIncrement(&self->calls);
	if (self->close_window)
		return __x_ABI_CWindows_CUI_CCore_CICoreWindow_Close(self->close_window);
	return self->result;
}

static const __x_ABI_CWindows_CUI_CCore_CIDispatchedHandlerVtbl handler_table = {
	handler_query, handler_addref, handler_release, handler_invoke};

static HRESULT WINAPI completion_query(__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID___x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler)) {
		*out = iface;
		InterlockedIncrement(&((struct completion_object *)iface)->refs);
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI completion_addref(__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler *iface)
{
	return (ULONG)InterlockedIncrement(&((struct completion_object *)iface)->refs);
}

static ULONG WINAPI completion_release(__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler *iface)
{
	return (ULONG)InterlockedDecrement(&((struct completion_object *)iface)->refs);
}

static HRESULT WINAPI completion_invoke(__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler *iface,
	__x_ABI_CWindows_CFoundation_CIAsyncAction *action, AsyncStatus status)
{
	(void)action;
	struct completion_object *self = (struct completion_object *)iface;
	if (status != self->expected) return E_FAIL;
	InterlockedIncrement(&self->calls);
	if (self->signal) SetEvent(self->signal);
	return S_OK;
}

static const __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandlerVtbl completion_table = {
	completion_query, completion_addref, completion_release, completion_invoke};

struct ui_worker {
	__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic *statics;
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *window;
	__x_ABI_CWindows_CUI_CCore_CICoreDispatcher *dispatcher;
	HANDLE ready, first_pumped, message_received;
	HWND native_window;
	DWORD message_thread, timer_thread;
	HRESULT result;
};

static LRESULT CALLBACK pump_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct ui_worker *worker = (void *)GetWindowLongPtrW(window, GWLP_USERDATA);
	if (message == WM_TIMER && worker) {
		KillTimer(window, wparam);
		worker->timer_thread = GetCurrentThreadId();
		SetEvent(worker->message_received);
		return 0;
	}
	if (message == WM_APP && worker) {
		worker->message_thread = GetCurrentThreadId();
		SetEvent(worker->message_received);
		return 0;
	}
	return DefWindowProcW(window, message, wparam, lparam);
}

static DWORD WINAPI pump_worker(void *context)
{
	struct ui_worker *worker = context;
	worker->result = __x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_GetForCurrentThread(worker->statics, &worker->window);
	if (SUCCEEDED(worker->result))
		worker->result = __x_ABI_CWindows_CUI_CCore_CICoreWindow_get_Dispatcher(worker->window, &worker->dispatcher);
	WNDCLASSW class = {0};
	class.lpfnWndProc = pump_window_proc;
	class.hInstance = GetModuleHandleW(NULL);
	class.lpszClassName = L"RavenCorePumpTest";
	if (!RegisterClassW(&class)) worker->result = E_FAIL;
	worker->native_window = CreateWindowExW(0, class.lpszClassName, L"", 0, 0, 0, 0, 0,
		HWND_MESSAGE, NULL, class.hInstance, NULL);
	if (!worker->native_window) worker->result = E_FAIL;
	SetWindowLongPtrW(worker->native_window, GWLP_USERDATA, (LONG_PTR)worker);
	SetEvent(worker->ready);
	if (FAILED(worker->result)) return 1;
	worker->result = __x_ABI_CWindows_CUI_CCore_CICoreDispatcher_ProcessEvents(worker->dispatcher,
		CoreProcessEventsOption_ProcessOneAndAllPending);
	SetEvent(worker->first_pumped);
	if (SUCCEEDED(worker->result))
		worker->result = __x_ABI_CWindows_CUI_CCore_CICoreDispatcher_ProcessEvents(worker->dispatcher,
			CoreProcessEventsOption_ProcessUntilQuit);
	DestroyWindow(worker->native_window);
	UnregisterClassW(class.lpszClassName, class.hInstance);
	return FAILED(worker->result);
}

struct lifetime_worker {
	__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic *statics;
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *window;
	__x_ABI_CWindows_CUI_CCore_CICoreDispatcher *dispatcher;
	BOOL explicit_exit;
};

static DWORD WINAPI lifetime_thread(void *context)
{
	struct lifetime_worker *worker = context;
	if (__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_GetForCurrentThread(worker->statics, &worker->window) != S_OK) return 1;
	if (__x_ABI_CWindows_CUI_CCore_CICoreWindow_get_Dispatcher(worker->window, &worker->dispatcher) != S_OK) return 2;
	if (worker->explicit_exit) ExitThread(0);
	return 0;
}

static int thread_lifetime(__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic *statics)
{
	HMODULE component = GetModuleHandleW(L"raven-xaml.dll");
	HRESULT (WINAPI *can_unload)(void) = component ? (void *)GetProcAddress(component, "DllCanUnloadNow") : NULL;
	if (!can_unload || can_unload() != S_OK) return 80;
	for (int mode = 0; mode < 2; ++mode) {
		struct lifetime_worker worker = {statics, NULL, NULL, mode};
		HANDLE thread = CreateThread(NULL, 0, lifetime_thread, &worker, 0, NULL);
		DWORD code;
		if (!thread || WaitForSingleObject(thread, 5000) != WAIT_OBJECT_0 || !GetExitCodeThread(thread, &code) || code) return 81;
		if (can_unload() != S_FALSE) return 82;
		__x_ABI_CWindows_CUI_CCore_CICoreDispatcher *closed_dispatcher = (void *)1;
		if (__x_ABI_CWindows_CUI_CCore_CICoreWindow_get_Dispatcher(worker.window, &closed_dispatcher) != (HRESULT)0x80000013 || closed_dispatcher) return 83;
		__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_Release(worker.dispatcher);
		__x_ABI_CWindows_CUI_CCore_CICoreWindow_Release(worker.window);
		if (can_unload() != S_OK) return 84;
		CloseHandle(thread);
	}
	return 0;
}

static int cross_thread_dispatch(__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic *statics,
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *main_window, BOOL quit_message)
{
	struct ui_worker worker = {0};
	worker.statics = statics;
	worker.ready = CreateEventW(NULL, TRUE, FALSE, NULL);
	worker.first_pumped = CreateEventW(NULL, TRUE, FALSE, NULL);
	worker.message_received = CreateEventW(NULL, FALSE, FALSE, NULL);
	HANDLE completed = CreateEventW(NULL, FALSE, FALSE, NULL);
	DWORD thread_id, exit_code;
	if (!worker.ready || !worker.first_pumped || !worker.message_received || !completed) return 30;
	HANDLE thread = CreateThread(NULL, 0, pump_worker, &worker, 0, &thread_id);
	if (!thread || WaitForSingleObject(worker.ready, 5000) != WAIT_OBJECT_0 || FAILED(worker.result)) return 31;
	if (worker.window == main_window || WaitForSingleObject(worker.first_pumped, 20) != WAIT_TIMEOUT ||
		WaitForSingleObject(thread, 0) != WAIT_TIMEOUT) return 32;
	boolean access = TRUE;
	if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_get_HasThreadAccess(worker.dispatcher, &access) != S_OK || access) return 33;
	if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_ProcessEvents(worker.dispatcher,
		CoreProcessEventsOption_ProcessAllIfPresent) != RPC_E_WRONG_THREAD) return 34;
	/* Wake ProcessOneAndAllPending solely with a desktop message, before posting any WinRT work. */
	if (!PostMessageW(worker.native_window, WM_APP, 0, 0) ||
		WaitForSingleObject(worker.message_received, 5000) != WAIT_OBJECT_0 || worker.message_thread != thread_id ||
		WaitForSingleObject(worker.first_pumped, 5000) != WAIT_OBJECT_0) return 48;
	/* The persistent pump must wake for desktop input too. */
	if (!PostMessageW(worker.native_window, WM_APP, 0, 0) ||
		WaitForSingleObject(worker.message_received, 5000) != WAIT_OBJECT_0) return 49;
	if (!SetTimer(worker.native_window, 1, 20, NULL) ||
		WaitForSingleObject(worker.message_received, 5000) != WAIT_OBJECT_0 || worker.timer_thread != thread_id) return 85;
	/* Register concurrently with completion, including cases where the UI thread wins the race. */
	struct handler_object handlers[128] = {0};
	struct completion_object completions[128] = {0};
	for (int i = 0; i < 128; ++i) {
		handlers[i].lpVtbl = &handler_table;
		handlers[i].refs = 1;
		completions[i].lpVtbl = &completion_table;
		completions[i].refs = 1;
		completions[i].signal = completed;
		completions[i].expected = Completed;
		__x_ABI_CWindows_CFoundation_CIAsyncAction *action = NULL;
		if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_RunAsync(worker.dispatcher, CoreDispatcherPriority_Normal,
			(__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler *)&handlers[i], &action) != S_OK) return 35;
		if (__x_ABI_CWindows_CFoundation_CIAsyncAction_put_Completed(action,
			(__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler *)&completions[i]) != S_OK) return 36;
		if (WaitForSingleObject(completed, 5000) != WAIT_OBJECT_0) return 37;
		if (i == 0 && WaitForSingleObject(worker.first_pumped, 5000) != WAIT_OBJECT_0) return 46;
		if (completions[i].calls != 1 || handlers[i].calls != 1 || handlers[i].thread != thread_id) return 38;
		if (__x_ABI_CWindows_CFoundation_CIAsyncAction_GetResults(action) != S_OK) return 39;
		/* Completion keeps the handler until the action is released. */
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler *saved = NULL;
		if (__x_ABI_CWindows_CFoundation_CIAsyncAction_get_Completed(action, &saved) != S_OK || saved != (void *)&completions[i]) return 40;
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Release(saved);
		__x_ABI_CWindows_CFoundation_CIAsyncAction_Release(action);
	}
	struct handler_object quit = {&handler_table, 1, 0, 0, S_OK, worker.window};
	__x_ABI_CWindows_CFoundation_CIAsyncAction *action = NULL;
	if (quit_message) {
		if (!PostThreadMessageW(thread_id, WM_QUIT, 0, 0)) return 86;
	} else if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_RunAsync(worker.dispatcher, CoreDispatcherPriority_Normal,
		(__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler *)&quit, &action) != S_OK) return 41;
	if (WaitForSingleObject(thread, 5000) != WAIT_OBJECT_0 || !GetExitCodeThread(thread, &exit_code) || exit_code) return 42;
	if (quit.calls != (quit_message ? 0 : 1)) return 43;
	if (action) {
		if (quit.thread != thread_id || __x_ABI_CWindows_CFoundation_CIAsyncAction_GetResults(action) != S_OK) return 43;
		__x_ABI_CWindows_CFoundation_CIAsyncAction_Release(action);
	}
	/* A shutting-down dispatcher completes RunAsync without executing its callback. */
	if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_RunAsync(worker.dispatcher, CoreDispatcherPriority_Normal,
		(__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler *)&quit, &action) != S_OK) return 44;
	if (quit.calls != (quit_message ? 0 : 1) || __x_ABI_CWindows_CFoundation_CIAsyncAction_GetResults(action) != S_OK) return 45;
	__x_ABI_CWindows_CFoundation_CIAsyncAction_Release(action);
	__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_Release(worker.dispatcher);
	__x_ABI_CWindows_CUI_CCore_CICoreWindow_Release(worker.window);
	for (int i = 0; i < 128; ++i)
		if (handlers[i].refs != 1 || completions[i].refs != 1) return 47;
	CloseHandle(thread);
	CloseHandle(completed);
	CloseHandle(worker.first_pumped);
	CloseHandle(worker.message_received);
	CloseHandle(worker.ready);
	return 0;
}

static int queue_contract(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window,
	__x_ABI_CWindows_CUI_CCore_CICoreDispatcher *dispatcher)
{
	struct handler_object handlers[4] = {0};
	__x_ABI_CWindows_CFoundation_CIAsyncAction *actions[4] = {0};
	for (int i = 0; i < 4; ++i) {
		handlers[i].lpVtbl = &handler_table;
		handlers[i].refs = 1;
		if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_RunAsync(dispatcher, CoreDispatcherPriority_Idle + i,
			(__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler *)&handlers[i], &actions[i]) != S_OK) return 70;
	}
	for (int i = 3; i >= 0; --i) {
		if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_ProcessEvents(dispatcher,
			CoreProcessEventsOption_ProcessOneIfPresent) != S_OK || handlers[i].calls != 1) return 71;
		for (int pending = 0; pending < i; ++pending)
			if (handlers[pending].calls) return 72;
		__x_ABI_CWindows_CFoundation_CIAsyncAction_Release(actions[i]);
	}
	__x_ABI_CWindows_CFoundation_CIAsyncAction *action = (void *)1;
	if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_RunAsync(dispatcher, 99,
		(__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler *)&handlers[0], &action) != E_INVALIDARG || action) return 73;
	if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_RunAsync(dispatcher, CoreDispatcherPriority_Normal,
		NULL, &action) != E_POINTER || action) return 74;
	if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_RunAsync(dispatcher, CoreDispatcherPriority_Normal,
		(__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler *)&handlers[0], &action) != S_OK) return 75;
	if (__x_ABI_CWindows_CUI_CCore_CICoreWindow_Close(window) != S_OK || handlers[0].calls != 1 ||
		__x_ABI_CWindows_CFoundation_CIAsyncAction_GetResults(action) != S_OK) return 76;
	__x_ABI_CWindows_CFoundation_CIAsyncAction_Release(action);
	for (int i = 0; i < 4; ++i)
		if (handlers[i].refs != 1) return 77;
	return 0;
}

static int async_contract(__x_ABI_CWindows_CUI_CCore_CICoreDispatcher *dispatcher)
{
	UINT32 previous_id = 0;
	for (int mode = 0; mode < 3; ++mode) {
		struct handler_object handler = {&handler_table, 1, 0, 0, mode == 1 ? E_ACCESSDENIED : S_OK};
		struct completion_object completion = {&completion_table, 1, 0, NULL, mode == 2 ? Canceled : mode == 1 ? Error : Completed};
		__x_ABI_CWindows_CFoundation_CIAsyncAction *action = NULL;
		IAsyncInfo *info = NULL;
		if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_RunAsync(dispatcher, CoreDispatcherPriority_Normal,
			(__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler *)&handler, &action) != S_OK) return 50;
		if (__x_ABI_CWindows_CFoundation_CIAsyncAction_QueryInterface(action, &IID_IAsyncInfo, (void **)&info) != S_OK) return 51;
		IUnknown *identity = NULL;
		if (IAsyncInfo_QueryInterface(info, &IID_IUnknown, (void **)&identity) != S_OK || identity != (void *)action) return 52;
		IUnknown_Release(identity);
		UINT32 id = 0;
		AsyncStatus status;
		HRESULT error;
		if (IAsyncInfo_get_Id(info, &id) != S_OK || !id || id == previous_id) return 53;
		previous_id = id;
		if (IAsyncInfo_get_Status(info, &status) != S_OK || status != Started) return 54;
		if (IAsyncInfo_Close(info) != (HRESULT)0x8000000e ||
			__x_ABI_CWindows_CFoundation_CIAsyncAction_GetResults(action) != (HRESULT)0x8000000e) return 55;
		if (mode == 2 && IAsyncInfo_Cancel(info) != S_OK) return 56;
		if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_ProcessEvents(dispatcher,
			CoreProcessEventsOption_ProcessAllIfPresent) != S_OK) return 57;
		if (IAsyncInfo_get_Status(info, &status) != S_OK || status != completion.expected) return 58;
		HRESULT expected_error = mode == 2 ? HRESULT_FROM_WIN32(ERROR_CANCELLED) : handler.result;
		if (IAsyncInfo_get_ErrorCode(info, &error) != S_OK || error != expected_error ||
			__x_ABI_CWindows_CFoundation_CIAsyncAction_GetResults(action) != expected_error) return 59;
		if (handler.calls != (mode == 2 ? 0 : 1)) return 60;
		/* Late registration must deliver exactly once, with the actual terminal status. */
		if (__x_ABI_CWindows_CFoundation_CIAsyncAction_put_Completed(action,
			(__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler *)&completion) != S_OK || completion.calls != 1) return 61;
		if (__x_ABI_CWindows_CFoundation_CIAsyncAction_put_Completed(action,
			(__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler *)&completion) != (HRESULT)0x80000018) return 62;
		if (IAsyncInfo_Close(info) != S_OK || IAsyncInfo_get_Status(info, &status) != (HRESULT)0x80000013 ||
			__x_ABI_CWindows_CFoundation_CIAsyncAction_GetResults(action) != (HRESULT)0x80000013) return 63;
		IAsyncInfo_Release(info);
		__x_ABI_CWindows_CFoundation_CIAsyncAction_Release(action);
		if (handler.refs != 1 || completion.refs != 1) return 64;
	}
	if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_ProcessEvents(dispatcher, 99) != E_INVALIDARG) return 65;
	return 0;
}

/* The window of the calling thread, its dispatcher and a queued handler: the same window is answered twice, the dispatcher
 * reports the thread that owns it, the handler runs when events are processed, and the action completes after it. The string
 * functions are imported from runtimeobject.dll, as a Windows Runtime program imports them. */
int raven_entry(void)
{
	static const WCHAR name[] = L"Windows.UI.Core.CoreWindow";
	static const WCHAR missing[] = L"Windows.Raven.Missing";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function RoGetActivationFactory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	if (!RoInitialize || !RoGetActivationFactory) return 20;
	if (FAILED(RoInitialize(1))) return 1;
	HSTRING class_key = NULL;
	if (WindowsCreateString(name, ARRAYSIZE(name) - 1, &class_key) != S_OK || !class_key) return 21;
	__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic *statics = NULL;
	if (RoGetActivationFactory(class_key, &IID___x_ABI_CWindows_CUI_CCore_CICoreWindowStatic, (void **)&statics) != S_OK || !statics)
		return 2;
	int lifetime_result = thread_lifetime(statics);
	if (lifetime_result) return lifetime_result;
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *first = NULL, *second = NULL;
	if (__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_GetForCurrentThread(statics, &first) != S_OK || !first) return 3;
	if (__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_GetForCurrentThread(statics, &second) != S_OK || second != first) return 4;
	HSTRING runtime_name = NULL;
	UINT32 length = 0;
	if (__x_ABI_CWindows_CUI_CCore_CICoreWindow_GetRuntimeClassName(first, &runtime_name) != S_OK) return 5;
	const WCHAR *text = WindowsGetStringRawBuffer(runtime_name, &length);
	if (length != ARRAYSIZE(name) - 1 || CompareStringOrdinal(text, (int)length, name, -1, FALSE) != CSTR_EQUAL) return 6;
	WindowsDeleteString(runtime_name);
	__x_ABI_CWindows_CUI_CCore_CICoreDispatcher *dispatcher = NULL;
	if (__x_ABI_CWindows_CUI_CCore_CICoreWindow_get_Dispatcher(first, &dispatcher) != S_OK || !dispatcher) return 7;
	boolean access = FALSE;
	if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_get_HasThreadAccess(dispatcher, &access) != S_OK || !access) return 8;
	struct handler_object handler = {&handler_table, 1, 0};
	struct completion_object completion = {&completion_table, 1, 0, NULL, Completed};
	__x_ABI_CWindows_CFoundation_CIAsyncAction *action = NULL;
	if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_RunAsync(dispatcher, CoreDispatcherPriority_Normal,
		(__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler *)&handler, &action) != S_OK || !action) return 9;
	if (handler.calls) return 10;
	if (__x_ABI_CWindows_CFoundation_CIAsyncAction_put_Completed(action,
		(__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler *)&completion) != S_OK) return 11;
	if (__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_ProcessEvents(dispatcher,
		CoreProcessEventsOption_ProcessAllIfPresent) != S_OK) return 12;
	if (handler.calls != 1 || completion.calls != 1) return 13;
	if (__x_ABI_CWindows_CFoundation_CIAsyncAction_GetResults(action) != S_OK) return 14;
	__x_ABI_CWindows_CFoundation_CIAsyncAction_Release(action);
	int result = async_contract(dispatcher);
	if (result) return result;
	for (int mode = 0; mode < 2; ++mode) {
		result = cross_thread_dispatch(statics, first, mode);
		if (result) return result;
	}
	result = queue_contract(first, dispatcher);
	if (result) return result;
	__x_ABI_CWindows_CUI_CCore_CICoreDispatcher_Release(dispatcher);
	__x_ABI_CWindows_CUI_CCore_CICoreWindow_Release(second);
	__x_ABI_CWindows_CUI_CCore_CICoreWindow_Release(first);
	__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_Release(statics);
	HSTRING missing_key = NULL;
	if (WindowsCreateString(missing, ARRAYSIZE(missing) - 1, &missing_key) != S_OK) return 22;
	IInspectable *object = (void *)1;
	if (RoGetActivationFactory(missing_key, &IID_IActivationFactory, (void **)&object) != REGDB_E_CLASSNOTREG || object)
		return 15;
	WindowsDeleteString(missing_key);
	WindowsDeleteString(class_key);
	return 0;
}
