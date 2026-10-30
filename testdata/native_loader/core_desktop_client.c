#define INITGUID
#define CONST_VTABLE
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_System
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <windows.ui.core.h>
#include "../../native/upstream/raven-xaml/corewindow_interop.h"

int raven_pointer_checks(ICoreWindow *, ICoreDispatcher *, HWND);
int raven_pointer_finish(void);

enum {KEY_DOWN, KEY_UP, CHARACTER, SIZE_CHANGED, VISIBLE, ACTIVATED, INPUT_ENABLED, CLOSED, EVENT_COUNT};
struct state;
struct handler {
	const void *table;
	struct state *state;
	LONG refs;
	int event;
};
struct state {
	ICoreWindow *window;
	ICoreDispatcher *dispatcher;
	raven_core_window_interop *interop;
	struct handler handlers[EVENT_COUNT];
	EventRegistrationToken tokens[EVENT_COUNT];
	DWORD thread;
	int failure, counts[EVENT_COUNT];
	BOOL fail_key, fail_close;
	Size size;
	boolean visible, enabled;
	CoreWindowActivationState activation;
	VirtualKey key;
	CorePhysicalKeyStatus key_status;
	UINT32 characters[16];
	IKeyEventArgs *retained_key;
	ICoreWindowEventArgs *retained_closed;
};
#define REQUIRE(condition) do { if (!(condition)) { state->failure = __LINE__; return E_FAIL; } } while (0)
static HRESULT check_event(struct state *state, ICoreWindow *sender)
{
	REQUIRE(sender == state->window && GetCurrentThreadId() == state->thread);
	return S_OK;
}
static HRESULT WINAPI invoke_key(void *iface, ICoreWindow *sender, IKeyEventArgs *args)
{
	struct handler *handler = iface;
	struct state *state = handler->state;
	if (FAILED(check_event(state, sender))) return E_FAIL;
	++state->counts[handler->event];
	REQUIRE(IKeyEventArgs_get_VirtualKey(args, &state->key) == S_OK);
	REQUIRE(IKeyEventArgs_get_KeyStatus(args, &state->key_status) == S_OK);
	ICoreWindowEventArgs *handled = NULL;
	REQUIRE(IKeyEventArgs_QueryInterface(args, &IID_ICoreWindowEventArgs, (void **)&handled) == S_OK);
	REQUIRE(ICoreWindowEventArgs_put_Handled(handled, TRUE) == S_OK);
	boolean value = FALSE;
	REQUIRE(ICoreWindowEventArgs_get_Handled(handled, &value) == S_OK && value);
	ICoreWindowEventArgs_Release(handled);
	REQUIRE(state->interop->lpVtbl->put_MessageHandled(state->interop, (boolean)1) == S_OK);
	if (handler->event == KEY_DOWN && !state->retained_key) {
		state->retained_key = args;
		IKeyEventArgs_AddRef(args);
		/* Revoke/re-register while the source holds a dispatch snapshot. */
		REQUIRE(ICoreWindow_remove_KeyDown(sender, state->tokens[KEY_DOWN]) == S_OK);
		REQUIRE(ICoreWindow_add_KeyDown(sender, (void *)handler, &state->tokens[KEY_DOWN]) == S_OK);
	}
	return state->fail_key ? E_ACCESSDENIED : S_OK;
}
static HRESULT WINAPI invoke_character(void *iface, ICoreWindow *sender, ICharacterReceivedEventArgs *args)
{
	struct state *state = ((struct handler *)iface)->state;
	if (FAILED(check_event(state, sender))) return E_FAIL;
	int index = state->counts[CHARACTER]++;
	REQUIRE(index < 16 && ICharacterReceivedEventArgs_get_KeyCode(args, &state->characters[index]) == S_OK);
	return S_OK;
}
static HRESULT WINAPI invoke_size(void *iface, ICoreWindow *sender, IWindowSizeChangedEventArgs *args)
{
	struct state *state = ((struct handler *)iface)->state;
	if (FAILED(check_event(state, sender))) return E_FAIL;
	++state->counts[SIZE_CHANGED];
	REQUIRE(IWindowSizeChangedEventArgs_get_Size(args, &state->size) == S_OK);
	return S_OK;
}
static HRESULT WINAPI invoke_visible(void *iface, ICoreWindow *sender, IVisibilityChangedEventArgs *args)
{
	struct state *state = ((struct handler *)iface)->state;
	if (FAILED(check_event(state, sender))) return E_FAIL;
	++state->counts[VISIBLE];
	REQUIRE(IVisibilityChangedEventArgs_get_Visible(args, &state->visible) == S_OK);
	return S_OK;
}
static HRESULT WINAPI invoke_activated(void *iface, ICoreWindow *sender, IWindowActivatedEventArgs *args)
{
	struct state *state = ((struct handler *)iface)->state;
	if (FAILED(check_event(state, sender))) return E_FAIL;
	++state->counts[ACTIVATED];
	REQUIRE(IWindowActivatedEventArgs_get_WindowActivationState(args, &state->activation) == S_OK);
	return S_OK;
}
static HRESULT WINAPI invoke_input(void *iface, ICoreWindow *sender, IInputEnabledEventArgs *args)
{
	struct state *state = ((struct handler *)iface)->state;
	if (FAILED(check_event(state, sender))) return E_FAIL;
	++state->counts[INPUT_ENABLED];
	REQUIRE(IInputEnabledEventArgs_get_InputEnabled(args, &state->enabled) == S_OK);
	return S_OK;
}
static HRESULT WINAPI invoke_closed(void *iface, ICoreWindow *sender, ICoreWindowEventArgs *args)
{
	struct state *state = ((struct handler *)iface)->state;
	if (FAILED(check_event(state, sender))) return E_FAIL;
	++state->counts[CLOSED];
	REQUIRE(!state->retained_closed);
	state->retained_closed = args;
	ICoreWindowEventArgs_AddRef(args);
	return state->fail_close ? E_ACCESSDENIED : S_OK;
}

static HRESULT WINAPI key_query(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CKeyEventArgs *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID___FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CKeyEventArgs)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI key_addref(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CKeyEventArgs *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI key_release(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CKeyEventArgs *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI key_invoke(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CKeyEventArgs *iface, ICoreWindow *sender, IKeyEventArgs *args) { return invoke_key(iface, sender, args); }
static const __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CKeyEventArgsVtbl key_table = { key_query, key_addref, key_release, key_invoke };

static HRESULT WINAPI character_query(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCharacterReceivedEventArgs *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID___FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCharacterReceivedEventArgs)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI character_addref(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCharacterReceivedEventArgs *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI character_release(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCharacterReceivedEventArgs *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI character_invoke(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCharacterReceivedEventArgs *iface, ICoreWindow *sender, ICharacterReceivedEventArgs *args) { return invoke_character(iface, sender, args); }
static const __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCharacterReceivedEventArgsVtbl character_table = { character_query, character_addref, character_release, character_invoke };

static HRESULT WINAPI size_query(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowSizeChangedEventArgs *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID___FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowSizeChangedEventArgs)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI size_addref(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowSizeChangedEventArgs *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI size_release(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowSizeChangedEventArgs *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI size_invoke(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowSizeChangedEventArgs *iface, ICoreWindow *sender, IWindowSizeChangedEventArgs *args) { return invoke_size(iface, sender, args); }
static const __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowSizeChangedEventArgsVtbl size_table = { size_query, size_addref, size_release, size_invoke };

static HRESULT WINAPI visible_query(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CVisibilityChangedEventArgs *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID___FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CVisibilityChangedEventArgs)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI visible_addref(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CVisibilityChangedEventArgs *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI visible_release(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CVisibilityChangedEventArgs *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI visible_invoke(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CVisibilityChangedEventArgs *iface, ICoreWindow *sender, IVisibilityChangedEventArgs *args) { return invoke_visible(iface, sender, args); }
static const __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CVisibilityChangedEventArgsVtbl visible_table = { visible_query, visible_addref, visible_release, visible_invoke };

static HRESULT WINAPI activated_query(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowActivatedEventArgs *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID___FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowActivatedEventArgs)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI activated_addref(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowActivatedEventArgs *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI activated_release(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowActivatedEventArgs *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI activated_invoke(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowActivatedEventArgs *iface, ICoreWindow *sender, IWindowActivatedEventArgs *args) { return invoke_activated(iface, sender, args); }
static const __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowActivatedEventArgsVtbl activated_table = { activated_query, activated_addref, activated_release, activated_invoke };

static HRESULT WINAPI input_query(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CInputEnabledEventArgs *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID___FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CInputEnabledEventArgs)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI input_addref(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CInputEnabledEventArgs *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI input_release(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CInputEnabledEventArgs *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI input_invoke(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CInputEnabledEventArgs *iface, ICoreWindow *sender, IInputEnabledEventArgs *args) { return invoke_input(iface, sender, args); }
static const __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CInputEnabledEventArgsVtbl input_table = { input_query, input_addref, input_release, input_invoke };

static HRESULT WINAPI closed_query(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCoreWindowEventArgs *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID___FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCoreWindowEventArgs)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI closed_addref(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCoreWindowEventArgs *iface) { return InterlockedIncrement(&((struct handler *)iface)->refs); }
static ULONG WINAPI closed_release(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCoreWindowEventArgs *iface) { return InterlockedDecrement(&((struct handler *)iface)->refs); }
static HRESULT WINAPI closed_invoke(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCoreWindowEventArgs *iface, ICoreWindow *sender, ICoreWindowEventArgs *args) { return invoke_closed(iface, sender, args); }
static const __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCoreWindowEventArgsVtbl closed_table = { closed_query, closed_addref, closed_release, closed_invoke };

static DWORD WINAPI wrong_thread(void *context)
{
	struct state *state = context;
	HWND hwnd = (void *)1;
	Rect bounds;
	if (state->interop->lpVtbl->get_WindowHandle(state->interop, &hwnd) != RPC_E_WRONG_THREAD || hwnd ||
		ICoreWindow_get_Bounds(state->window, &bounds) != RPC_E_WRONG_THREAD) return 1;
	return 0;
}

int raven_entry(void)
{
	typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	factory_function factory = runtime ? (void *)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	HRESULT (WINAPI *initialize)(int) = runtime ? (void *)GetProcAddress(runtime, "RoInitialize") : NULL;
	if (!factory || !initialize || initialize(1) != S_OK) return 1;
	HSTRING name = NULL;
	static const WCHAR text[] = L"Windows.UI.Core.CoreWindow";
	if (WindowsCreateString(text, ARRAYSIZE(text) - 1, &name) != S_OK) return 2;
	ICoreWindowStatic *statics = NULL;
	if (factory(name, &IID_ICoreWindowStatic, (void **)&statics) != S_OK) return 3;
	WindowsDeleteString(name);
	struct state value = {0};
	struct state *state = &value;
	state->thread = GetCurrentThreadId();
	if (ICoreWindowStatic_GetForCurrentThread(statics, &state->window) != S_OK ||
		ICoreWindow_get_Dispatcher(state->window, &state->dispatcher) != S_OK ||
		ICoreWindow_QueryInterface(state->window, &raven_core_window_interop_iid, (void **)&state->interop) != S_OK) return 4;
	IUnknown *identity = NULL;
	if (state->interop->lpVtbl->QueryInterface(state->interop, &IID_IUnknown, (void **)&identity) != S_OK || identity != (void *)state->window) return 5;
	IUnknown_Release(identity);
	HWND hwnd = NULL;
	if (state->interop->lpVtbl->get_WindowHandle(state->interop, &hwnd) != S_OK || !hwnd || !IsWindow(hwnd)) return 6;
	boolean visible = TRUE;
	Rect bounds;
	if (ICoreWindow_get_Visible(state->window, &visible) != S_OK || visible ||
		ICoreWindow_get_Bounds(state->window, &bounds) != S_OK || bounds.Width != 800 || bounds.Height != 600) return 7;
	ICoreCursor *cursor = (void *)1;
	if (ICoreWindow_get_PointerCursor(state->window, &cursor) != E_NOTIMPL || cursor ||
		ICoreWindow_get_PointerCursor(state->window, NULL) != E_POINTER) return 26;
	HANDLE other = CreateThread(NULL, 0, wrong_thread, state, 0, NULL);
	DWORD code;
	if (!other || WaitForSingleObject(other, 5000) != WAIT_OBJECT_0 || !GetExitCodeThread(other, &code) || code) return 8;
	CloseHandle(other);
	const void *tables[] = {&key_table, &key_table, &character_table, &size_table, &visible_table, &activated_table, &input_table, &closed_table};
	for (int i = 0; i < EVENT_COUNT; ++i) {
		state->handlers[i].table = tables[i];
		state->handlers[i].state = state;
		state->handlers[i].refs = 1;
		state->handlers[i].event = i;
	}
	if (ICoreWindow_add_KeyDown(state->window, (void *)&state->handlers[KEY_DOWN], &state->tokens[KEY_DOWN]) != S_OK) return 9;
	if (ICoreWindow_add_KeyUp(state->window, (void *)&state->handlers[KEY_UP], &state->tokens[KEY_UP]) != S_OK) return 9;
	if (ICoreWindow_add_CharacterReceived(state->window, (void *)&state->handlers[CHARACTER], &state->tokens[CHARACTER]) != S_OK) return 9;
	if (ICoreWindow_add_SizeChanged(state->window, (void *)&state->handlers[SIZE_CHANGED], &state->tokens[SIZE_CHANGED]) != S_OK) return 9;
	if (ICoreWindow_add_VisibilityChanged(state->window, (void *)&state->handlers[VISIBLE], &state->tokens[VISIBLE]) != S_OK) return 9;
	if (ICoreWindow_add_Activated(state->window, (void *)&state->handlers[ACTIVATED], &state->tokens[ACTIVATED]) != S_OK) return 9;
	if (ICoreWindow_add_InputEnabled(state->window, (void *)&state->handlers[INPUT_ENABLED], &state->tokens[INPUT_ENABLED]) != S_OK) return 9;
	if (ICoreWindow_add_Closed(state->window, (void *)&state->handlers[CLOSED], &state->tokens[CLOSED]) != S_OK) return 9;

	if (!SetWindowPos(hwnd, NULL, 0, 0, 640, 480, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE) ||
		!state->counts[SIZE_CHANGED] || ICoreWindow_get_Bounds(state->window, &bounds) != S_OK ||
		bounds.Width != state->size.Width || bounds.Height != state->size.Height) return 10;
	LPARAM down = (LPARAM)(3U | (0x1eU << 16) | (1U << 24) | (1U << 29) | (1U << 30));
	SendMessageW(hwnd, WM_KEYDOWN, 'A', down);
	if (state->counts[KEY_DOWN] != 1 || state->key != VirtualKey_A || state->key_status.RepeatCount != 3 ||
		state->key_status.ScanCode != 0x1e || !state->key_status.IsExtendedKey || !state->key_status.IsMenuKeyDown ||
		!state->key_status.WasKeyDown || state->key_status.IsKeyReleased) return 11;
	if (!PostMessageW(hwnd, WM_KEYUP, 'A', (LPARAM)(ULONG_PTR)(1U | (0x1eU << 16) | (1U << 30) | (1U << 31))) ||
		ICoreDispatcher_ProcessEvents(state->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) != S_OK ||
		state->counts[KEY_UP] != 1 || !state->key_status.IsKeyReleased) return 12;
	SendMessageW(hwnd, WM_CHAR, 0xe9, 1);
	SendMessageW(hwnd, WM_CHAR, 0xd83d, 1);
	SendMessageW(hwnd, WM_CHAR, 0xde80, 1);
	SendMessageW(hwnd, WM_CHAR, 0xd800, 1);
	SendMessageW(hwnd, WM_CHAR, 'x', 1);
	if (state->counts[CHARACTER] != 4 || state->characters[0] != 0xe9 || state->characters[1] != 0x1f680 ||
		state->characters[2] != 0xfffd || state->characters[3] != 'x') return 13;
	if (ICoreWindow_put_IsInputEnabled(state->window, FALSE) != S_OK || state->enabled || state->counts[INPUT_ENABLED] != 1) return 14;
	SendMessageW(hwnd, WM_KEYDOWN, 'A', down);
	if (state->counts[KEY_DOWN] != 1 || ICoreWindow_put_IsInputEnabled(state->window, TRUE) != S_OK ||
		!state->enabled || state->counts[INPUT_ENABLED] != 2) return 15;
	state->fail_key = TRUE;
	if (!PostMessageW(hwnd, WM_KEYDOWN, 'B', 1) ||
		ICoreDispatcher_ProcessEvents(state->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) != E_ACCESSDENIED) return 16;
	state->fail_key = FALSE;
	if (ICoreDispatcher_ProcessEvents(state->dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) != S_OK) return 17;
	int pointer_result = raven_pointer_checks(state->window, state->dispatcher, hwnd);
	if (pointer_result) return pointer_result;
	WCHAR mode[4];
	BOOL display = GetEnvironmentVariableW(L"RAVEN_TEST_CORE_DESKTOP", mode, ARRAYSIZE(mode)) != 0;
	if (display) {
		SetWindowTextW(hwnd, L"Raven CoreWindow fixture");
		if (ICoreWindow_Activate(state->window) != S_OK || ICoreWindow_get_Visible(state->window, &visible) != S_OK || !visible ||
			!state->counts[VISIBLE] || !state->visible || !state->counts[ACTIVATED]) return 18;
		HDC dc = GetDC(hwnd);
		HBRUSH brush = CreateSolidBrush(RGB(0, 255, 0));
		RECT client;
		if (!dc || !brush || !GetClientRect(hwnd, &client) || !FillRect(dc, &client, brush)) return 19;
		DeleteObject(brush);
		ReleaseDC(hwnd, dc);
		if (ICoreDispatcher_ProcessEvents(state->dispatcher, CoreProcessEventsOption_ProcessUntilQuit) != S_OK) return 20;
	} else {
		state->fail_close = TRUE;
		if (!PostMessageW(hwnd, WM_CLOSE, 0, 0) ||
			ICoreDispatcher_ProcessEvents(state->dispatcher, CoreProcessEventsOption_ProcessUntilQuit) != E_ACCESSDENIED) return 21;
	}
	if (state->failure) return state->failure;
	pointer_result = raven_pointer_finish();
	if (pointer_result) return pointer_result;
	if (state->counts[CLOSED] != 1 || IsWindow(hwnd) || ICoreWindow_Close(state->window) != S_OK ||
		state->counts[CLOSED] != 1 || !state->retained_key || !state->retained_closed) return 22;
	HWND closed = (void *)1;
	if (state->interop->lpVtbl->get_WindowHandle(state->interop, &closed) != (HRESULT)0x80000013 || closed) return 23;
	CorePhysicalKeyStatus retained;
	if (IKeyEventArgs_get_KeyStatus(state->retained_key, &retained) != S_OK || retained.RepeatCount != 3) return 24;
	IKeyEventArgs_Release(state->retained_key);
	ICoreWindowEventArgs_Release(state->retained_closed);
	for (int i = 0; i < EVENT_COUNT; ++i)
		if (state->handlers[i].refs != 1) return 25;
	state->interop->lpVtbl->Release(state->interop);
	ICoreDispatcher_Release(state->dispatcher);
	ICoreWindow_Release(state->window);
	ICoreWindowStatic_Release(statics);
	return 0;
}
