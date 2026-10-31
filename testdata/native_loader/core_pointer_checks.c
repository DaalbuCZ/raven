#define INITGUID
#define CONST_VTABLE
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_UI_Input
#define WIDL_using_Windows_Devices_Input
#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_System
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <windows.ui.core.h>
#include <windows.ui.input.h>
#include <windows.devices.input.h>

typedef __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs handler_interface;
typedef __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgsVtbl handler_vtable;
enum {ENTERED, EXITED, MOVED, PRESSED, RELEASED, WHEEL, CAPTURE_LOST, POINTER_EVENTS};
struct pointer_handler { const handler_vtable *lpVtbl; LONG refs; int event; };
static struct pointer_state {
	ICoreWindow *window;
	DWORD thread;
	struct pointer_handler handlers[POINTER_EVENTS];
	EventRegistrationToken tokens[POINTER_EVENTS];
	int counts[POINTER_EVENTS], failure;
	BOOL fail_move;
	Point position;
	UINT32 frame;
	UINT64 timestamp;
	VirtualKeyModifiers modifiers;
	PointerUpdateKind update;
	INT32 wheel;
	boolean left, right, middle, x1, x2, horizontal, contact;
	IPointerEventArgs *retained_args;
	IPointerPoint *retained_point;
	IPointerPointProperties *retained_properties;
	IPointerDevice *device;
} state;
#define CHECK(condition) do { if (!(condition)) { state.failure = __LINE__; return E_FAIL; } } while (0)

static HRESULT WINAPI handler_query(handler_interface *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid,
		&IID___FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct pointer_handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI handler_addref(handler_interface *iface) { return InterlockedIncrement(&((struct pointer_handler *)iface)->refs); }
static ULONG WINAPI handler_release(handler_interface *iface) { return InterlockedDecrement(&((struct pointer_handler *)iface)->refs); }
static HRESULT WINAPI handler_invoke(handler_interface *iface, ICoreWindow *window, IPointerEventArgs *args)
{
	int event = ((struct pointer_handler *)iface)->event;
	CHECK(window == state.window && GetCurrentThreadId() == state.thread);
	++state.counts[event];
	IPointerPoint *point = NULL;
	IPointerPointProperties *properties = NULL;
	CHECK(IPointerEventArgs_get_CurrentPoint(args, &point) == S_OK && point);
	CHECK(IPointerPoint_get_Properties(point, &properties) == S_OK && properties);
	CHECK(IPointerPoint_get_Position(point, &state.position) == S_OK);
	Point raw;
	CHECK(IPointerPoint_get_RawPosition(point, &raw) == S_OK && raw.X == state.position.X && raw.Y == state.position.Y);
	UINT32 id = 0;
	CHECK(IPointerPoint_get_pointer_id(point, &id) == S_OK && id == 1);
	CHECK(IPointerPoint_get_FrameId(point, &state.frame) == S_OK && state.frame);
	CHECK(IPointerPoint_get_Timestamp(point, &state.timestamp) == S_OK && state.timestamp && state.timestamp <= GetTickCount64() * 1000 + 1000);
	CHECK(IPointerPoint_get_IsInContact(point, &state.contact) == S_OK);
	CHECK(IPointerEventArgs_get_KeyModifiers(args, &state.modifiers) == S_OK);
	CHECK(IPointerPointProperties_get_IsLeftButtonPressed(properties, &state.left) == S_OK);
	CHECK(IPointerPointProperties_get_IsRightButtonPressed(properties, &state.right) == S_OK);
	CHECK(IPointerPointProperties_get_IsMiddleButtonPressed(properties, &state.middle) == S_OK);
	CHECK(IPointerPointProperties_get_IsXButton1Pressed(properties, &state.x1) == S_OK);
	CHECK(IPointerPointProperties_get_IsXButton2Pressed(properties, &state.x2) == S_OK);
	CHECK(IPointerPointProperties_get_MouseWheelDelta(properties, &state.wheel) == S_OK);
	CHECK(IPointerPointProperties_get_IsHorizontalMouseWheel(properties, &state.horizontal) == S_OK);
	CHECK(IPointerPointProperties_get_PointerUpdateKind(properties, &state.update) == S_OK);
	CHECK(state.contact == !!(state.left || state.right || state.middle || state.x1 || state.x2));
	FLOAT pressure;
	boolean primary, in_range;
	CHECK(IPointerPointProperties_get_Pressure(properties, &pressure) == S_OK && pressure == 0.5f);
	CHECK(IPointerPointProperties_get_IsPrimary(properties, &primary) == S_OK && primary);
	CHECK(IPointerPointProperties_get_IsInRange(properties, &in_range) == S_OK && in_range);
	IPointerDevice *device = NULL;
	CHECK(IPointerPoint_get_PointerDevice(point, &device) == S_OK && device);
	PointerDeviceType type;
	CHECK(IPointerDevice_get_PointerDeviceType(device, &type) == S_OK && type == PointerDeviceType_Mouse);
	if (!state.device) { state.device = device; IPointerDevice_AddRef(device); }
	CHECK(device == state.device);
	IPointerDevice_Release(device);
	ICoreWindowEventArgs *handled = NULL;
	CHECK(IPointerEventArgs_QueryInterface(args, &IID_ICoreWindowEventArgs, (void **)&handled) == S_OK);
	CHECK(ICoreWindowEventArgs_put_Handled(handled, TRUE) == S_OK);
	ICoreWindowEventArgs_Release(handled);
	if (event == PRESSED && !state.retained_point) {
		state.retained_args = args; IPointerEventArgs_AddRef(args);
		state.retained_point = point; IPointerPoint_AddRef(point);
		state.retained_properties = properties; IPointerPointProperties_AddRef(properties);
		/* GetIntermediatePoints stays E_NOTIMPL: the desktop input has no history buffer yet (the args snapshot carries
		 * exactly one point, the CurrentPoint), and the slot lives in the component's desktop window dispatch. */
	}
	IPointerPointProperties_Release(properties);
	IPointerPoint_Release(point);
	return state.fail_move && event == MOVED ? E_ACCESSDENIED : S_OK;
}
static const handler_vtable handler_table = {handler_query, handler_addref, handler_release, handler_invoke};

/* Cursor objects. "Windows.UI.Core.CoreCursor" is cataloged to the component, so the factory resolves the way the
 * projection resolves any runtime class: through RoGetActivationFactory. */
static int cursor_checks(ICoreWindow *window)
{
	HMODULE combase = LoadLibraryW(L"combase.dll");
	HRESULT (WINAPI *ro_factory)(HSTRING, REFIID, void **) = combase ?
		(void *)GetProcAddress(combase, "RoGetActivationFactory") : NULL;
	if (!ro_factory) return __LINE__;
	HSTRING name = NULL;
	static const WCHAR text[] = L"Windows.UI.Core.CoreCursor";
	if (WindowsCreateString(text, ARRAYSIZE(text) - 1, &name) != S_OK) return __LINE__;
	ICoreCursorFactory *builder = NULL;
	if (ro_factory(name, &IID___x_ABI_CWindows_CUI_CCore_CICoreCursorFactory, (void **)&builder) != S_OK || !builder)
		return __LINE__;
	IActivationFactory *activation = NULL;
	if (((IUnknown *)builder)->lpVtbl->QueryInterface((IUnknown *)builder, &IID_IActivationFactory,
		(void **)&activation) != S_OK || !activation) return __LINE__;
	/* RoActivateInstance of the class answers the default cursor: the arrow. */
	ICoreCursor *activated = (void *)1;
	if (activation->lpVtbl->ActivateInstance(activation, (IInspectable **)&activated) != S_OK || !activated) return __LINE__;
	CoreCursorType type = CoreCursorType_Custom;
	if (activated->lpVtbl->get_Type(activated, &type) != S_OK || type != CoreCursorType_Arrow) return __LINE__;
	activated->lpVtbl->Release(activated);
	/* The factory builds a cursor of the requested type; an out-of-range type is rejected. */
	ICoreCursor *hand = NULL;
	if (builder->lpVtbl->CreateCursor(builder, CoreCursorType_Hand, 0, &hand) != S_OK || !hand) return __LINE__;
	if (hand->lpVtbl->get_Type(hand, &type) != S_OK || type != CoreCursorType_Hand) return __LINE__;
	ICoreCursor *bad = (void *)1;
	if (builder->lpVtbl->CreateCursor(builder, (CoreCursorType)0x40, 0, &bad) != E_INVALIDARG || bad) return __LINE__;
	/* The window's cursor: the platform default until an application puts one; the put stores the object itself and a
	 * NULL put restores the default. */
	ICoreCursor *current = NULL;
	if (ICoreWindow_get_PointerCursor(window, &current) != S_OK || !current ||
		ICoreWindow_get_PointerCursor(window, NULL) != E_POINTER) return __LINE__;
	if (current->lpVtbl->get_Type(current, &type) != S_OK || type != CoreCursorType_Arrow) return __LINE__;
	ICoreCursor_Release(current);
	if (ICoreWindow_put_PointerCursor(window, hand) != S_OK) return __LINE__;
	ICoreCursor *stored = NULL;
	if (ICoreWindow_get_PointerCursor(window, &stored) != S_OK || stored != hand) return __LINE__; /* the same object */
	ICoreCursor_Release(stored);
	if (ICoreWindow_put_PointerCursor(window, NULL) != S_OK) return __LINE__;
	if (ICoreWindow_get_PointerCursor(window, &stored) != S_OK || !stored) return __LINE__;
	if (stored->lpVtbl->get_Type(stored, &type) != S_OK || type != CoreCursorType_Arrow) return __LINE__;
	ICoreCursor_Release(stored);
	ICoreCursor_Release(hand);
	builder->lpVtbl->Release(builder);
	activation->lpVtbl->Release(activation);
	FreeLibrary(combase);
	WindowsDeleteString(name);
	return 0;
}

static DWORD WINAPI message_clock_worker(void *context)
{
	if (GetMessageTime() != 0) return 1;
	if (!PostThreadMessageW(GetCurrentThreadId(), WM_APP, 0, 0)) return 2;
	MSG message;
	if (!PeekMessageW(&message, NULL, WM_APP, WM_APP, PM_REMOVE)) return 3;
	Sleep(20);
	if ((UINT32)GetMessageTime() != message.time) return 4;
	return 0;
}

int raven_pointer_checks(ICoreWindow *window, ICoreDispatcher *dispatcher, HWND hwnd)
{
	state.window = window;
	state.thread = GetCurrentThreadId();
	int cursor_result = cursor_checks(window);
	if (cursor_result) return cursor_result;
	for (int i = 0; i < POINTER_EVENTS; ++i) {
		state.handlers[i].lpVtbl = &handler_table;
		state.handlers[i].refs = 1;
		state.handlers[i].event = i;
	}
	if (ICoreWindow_add_PointerEntered(window, (void *)&state.handlers[ENTERED], &state.tokens[ENTERED]) != S_OK) return __LINE__;
	if (ICoreWindow_add_PointerExited(window, (void *)&state.handlers[EXITED], &state.tokens[EXITED]) != S_OK) return __LINE__;
	if (ICoreWindow_add_PointerMoved(window, (void *)&state.handlers[MOVED], &state.tokens[MOVED]) != S_OK) return __LINE__;
	if (ICoreWindow_add_PointerPressed(window, (void *)&state.handlers[PRESSED], &state.tokens[PRESSED]) != S_OK) return __LINE__;
	if (ICoreWindow_add_PointerReleased(window, (void *)&state.handlers[RELEASED], &state.tokens[RELEASED]) != S_OK) return __LINE__;
	if (ICoreWindow_add_PointerWheelChanged(window, (void *)&state.handlers[WHEEL], &state.tokens[WHEEL]) != S_OK) return __LINE__;
	if (ICoreWindow_add_PointerCaptureLost(window, (void *)&state.handlers[CAPTURE_LOST], &state.tokens[CAPTURE_LOST]) != S_OK) return __LINE__;

	/* Message time belongs to retrieval, including non-removing peeks, not the eventual dispatch. */
	if (!PostMessageW(hwnd, WM_MOUSEMOVE, MK_CONTROL | MK_SHIFT, MAKELPARAM(-5, 7))) return __LINE__;
	MSG message;
	if (!PeekMessageW(&message, hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_NOREMOVE)) return __LINE__;
	Sleep(25);
	HANDLE clock = CreateThread(NULL, 0, message_clock_worker, NULL, 0, NULL);
	DWORD code;
	if (!clock || WaitForSingleObject(clock, 5000) != WAIT_OBJECT_0 || !GetExitCodeThread(clock, &code) || code) return __LINE__;
	CloseHandle(clock);
	if ((UINT32)GetMessageTime() != message.time ||
		ICoreDispatcher_ProcessEvents(dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) != S_OK ||
		state.counts[ENTERED] != 1 || state.counts[MOVED] != 1 || state.position.X != -5 || state.position.Y != 7 ||
		(UINT32)(state.timestamp / 1000) != message.time || GetTickCount64() - state.timestamp / 1000 < 15 ||
		(state.modifiers & (VirtualKeyModifiers_Control | VirtualKeyModifiers_Shift)) !=
			(VirtualKeyModifiers_Control | VirtualKeyModifiers_Shift)) return __LINE__;
	static const UINT down[] = {WM_LBUTTONDOWN, WM_RBUTTONDOWN, WM_MBUTTONDOWN, WM_XBUTTONDOWN, WM_XBUTTONDOWN};
	static const UINT up[] = {WM_LBUTTONUP, WM_RBUTTONUP, WM_MBUTTONUP, WM_XBUTTONUP, WM_XBUTTONUP};
	static const UINT flags[] = {MK_LBUTTON, MK_RBUTTON, MK_MBUTTON, MK_XBUTTON1, MK_XBUTTON2};
	for (int button = 0; button < 5; ++button) {
		WPARAM keys = MAKEWPARAM(flags[button], button == 3 ? XBUTTON1 : button == 4 ? XBUTTON2 : 0);
		SendMessageW(hwnd, down[button], keys, MAKELPARAM(-12, 34));
		if (!state.contact || state.update != (PointerUpdateKind)(1 + button * 2)) return __LINE__;
		keys = MAKEWPARAM(0, button == 3 ? XBUTTON1 : button == 4 ? XBUTTON2 : 0);
		SendMessageW(hwnd, up[button], keys, MAKELPARAM(25, 36));
		if (state.contact || state.update != (PointerUpdateKind)(2 + button * 2)) return __LINE__;
	}
	if (state.counts[PRESSED] != 5 || state.counts[RELEASED] != 5) return __LINE__;
	POINT screen = {20, -10};
	if (!ClientToScreen(hwnd, &screen)) return __LINE__;
	SendMessageW(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(0, 120), MAKELPARAM(screen.x, screen.y));
	if (state.wheel != 120 || state.horizontal || state.position.X != 20 || state.position.Y != -10) return __LINE__;
	SendMessageW(hwnd, WM_MOUSEHWHEEL, MAKEWPARAM(MK_CONTROL, -240), MAKELPARAM(screen.x, screen.y));
	if (state.wheel != -240 || !state.horizontal || state.counts[WHEEL] != 2 || state.update != PointerUpdateKind_Other) return __LINE__;
	if (!SetCursorPos(screen.x, screen.y)) return __LINE__;
	Point current;
	if (ICoreWindow_get_PointerPosition(window, &current) != S_OK || current.X != 20 || current.Y != -10) return __LINE__;
	if (ICoreWindow_SetPointerCapture(window) != S_OK || GetCapture() != hwnd ||
		ICoreWindow_ReleasePointerCapture(window) != S_OK || GetCapture() || state.counts[CAPTURE_LOST] != 1) return __LINE__;
	WNDCLASSW definition = {0};
	definition.hInstance = GetModuleHandleW(NULL);
	definition.lpfnWndProc = DefWindowProcW;
	definition.lpszClassName = L"Raven pointer capture test";
	if (!RegisterClassW(&definition)) return __LINE__;
	HWND other = CreateWindowExW(0, definition.lpszClassName, L"", 0, 0, 0, 0, 0,
		HWND_MESSAGE, NULL, definition.hInstance, NULL);
	if (!other || ICoreWindow_SetPointerCapture(window) != S_OK) return __LINE__;
	SetCapture(other);
	if (GetCapture() != other || state.counts[CAPTURE_LOST] != 2 ||
		ICoreWindow_ReleasePointerCapture(window) != S_OK || GetCapture() != other) return __LINE__;
	ReleaseCapture();
	DestroyWindow(other);
	UnregisterClassW(definition.lpszClassName, definition.hInstance);
	SendMessageW(hwnd, WM_MOUSELEAVE, 0, 0);
	SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(2, 3));
	if (state.counts[EXITED] != 1 || state.counts[ENTERED] != 2) return __LINE__;
	int moves = state.counts[MOVED];
	if (ICoreWindow_put_IsInputEnabled(window, FALSE) != S_OK) return __LINE__;
	SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(9, 8));
	if (state.counts[MOVED] != moves || ICoreWindow_put_IsInputEnabled(window, TRUE) != S_OK) return __LINE__;
	SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(9, 8));
	if (state.counts[MOVED] != moves + 1 || state.counts[ENTERED] != 3) return __LINE__;
	state.fail_move = TRUE;
	if (!PostMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(6, 7)) ||
		ICoreDispatcher_ProcessEvents(dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) != E_ACCESSDENIED) return __LINE__;
	state.fail_move = FALSE;
	return state.failure;
}

int raven_pointer_finish(void)
{
	if (state.failure) return state.failure;
	if (!state.retained_point || !state.retained_properties || !state.retained_args || !state.device) return __LINE__;
	Point position;
	boolean left;
	if (IPointerPoint_get_Position(state.retained_point, &position) != S_OK || position.X != -12 || position.Y != 34 ||
		IPointerPointProperties_get_IsLeftButtonPressed(state.retained_properties, &left) != S_OK || !left) return __LINE__;
	if (IPointerEventArgs_Release(state.retained_args) != 0 || IPointerPoint_Release(state.retained_point) != 1 ||
		IPointerPointProperties_Release(state.retained_properties) != 0 || IPointerDevice_Release(state.device) != 0) return __LINE__;
	for (int i = 0; i < POINTER_EVENTS; ++i)
		if (state.handlers[i].refs != 1) return __LINE__;
	return 0;
}
