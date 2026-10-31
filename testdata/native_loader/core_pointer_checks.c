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
/* Matches native/upstream/raven-xaml/pointer.h; the sample layout crosses the export boundary. */
struct raven_mouse_sample {
	__x_ABI_CWindows_CFoundation_CPoint position;
	UINT32 buttons, frame;
	UINT64 timestamp;
	__x_ABI_CWindows_CSystem_CVirtualKeyModifiers modifiers;
	__x_ABI_CWindows_CUI_CInput_CPointerUpdateKind update;
	INT32 wheel;
	boolean horizontal;
	__x_ABI_CWindows_CDevices_CInput_CPointerDeviceType device;
	UINT32 pointer_id;
	boolean in_contact, primary, in_range;
};
typedef HRESULT (*history_create_function)(const struct raven_mouse_sample *, UINT32,
	__FIVector_1_Windows__CUI__CInput__CPointerPoint **);
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
	boolean left, right, middle, x1, x2, horizontal, contact, primary, in_range;
	/* What the next event is expected to carry: the mouse checks run with the defaults
	 * (id 1, Mouse, primary, in range) and the touch checks point them at the contact. */
	UINT32 expect_id;
	PointerDeviceType expect_type;
	boolean expect_primary, expect_in_range;
	IPointerEventArgs *retained_args;
	IPointerPoint *retained_point;
	IPointerPointProperties *retained_properties;
	__FIVector_1_Windows__CUI__CInput__CPointerPoint *retained_history;
	IPointerEventArgs *retained_touch_args;
	__FIVector_1_Windows__CUI__CInput__CPointerPoint *retained_touch_history;
	IPointerDevice *device;
	IPointerDevice *touch_device;
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
	CHECK(IPointerPoint_get_pointer_id(point, &id) == S_OK && id == state.expect_id);
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
	CHECK(IPointerPointProperties_get_IsPrimary(properties, &primary) == S_OK && primary == state.expect_primary);
	CHECK(IPointerPointProperties_get_IsInRange(properties, &in_range) == S_OK && in_range == state.expect_in_range);
	state.primary = primary;
	state.in_range = in_range;
	IPointerDevice *device = NULL;
	CHECK(IPointerPoint_get_PointerDevice(point, &device) == S_OK && device);
	PointerDeviceType type;
	CHECK(IPointerDevice_get_PointerDeviceType(device, &type) == S_OK && type == state.expect_type);
	/* The device object must be the stable catalog identity for its type: the mouse's
	 * across every mouse event, the touch entry's across every touch event. */
	if (state.expect_type == PointerDeviceType_Mouse) {
		if (!state.device) { state.device = device; IPointerDevice_AddRef(device); }
		CHECK(device == state.device);
	} else if (state.expect_type == PointerDeviceType_Touch) {
		if (!state.touch_device) { state.touch_device = device; IPointerDevice_AddRef(device); }
		CHECK(device == state.touch_device);
	}
	IPointerDevice_Release(device);
	ICoreWindowEventArgs *handled = NULL;
	CHECK(IPointerEventArgs_QueryInterface(args, &IID_ICoreWindowEventArgs, (void **)&handled) == S_OK);
	CHECK(ICoreWindowEventArgs_put_Handled(handled, TRUE) == S_OK);
	ICoreWindowEventArgs_Release(handled);
	if (event == PRESSED && !state.retained_point) {
		state.retained_args = args; IPointerEventArgs_AddRef(args);
		state.retained_point = point; IPointerPoint_AddRef(point);
		state.retained_properties = properties; IPointerPointProperties_AddRef(properties);
		/* Intermediate points precede CurrentPoint: the samples retained before this press, oldest
		 * first. The first press follows at least one move, so the history is non-empty and every
		 * element's FrameId is strictly older than the current point's. */
		__FIVector_1_Windows__CUI__CInput__CPointerPoint *history = NULL;
		CHECK(__x_ABI_CWindows_CUI_CCore_CIPointerEventArgs_GetIntermediatePoints(args, &history) == S_OK && history);
		UINT32 count = 0;
		CHECK(__FIVector_1_Windows__CUI__CInput__CPointerPoint_get_Size(history, &count) == S_OK && count >= 1);
		UINT32 previous = state.frame;
		for (UINT32 i = 0; i < count; ++i) {
			IPointerPoint *element = NULL;
			CHECK(__FIVector_1_Windows__CUI__CInput__CPointerPoint_GetAt(history, i, &element) == S_OK && element);
			UINT32 frame = 0;
			CHECK(IPointerPoint_get_FrameId(element, &frame) == S_OK && frame && frame < previous);
			previous = frame;
			IPointerPoint_Release(element);
		}
		IPointerPoint *beyond = (void *)1;
		CHECK(__FIVector_1_Windows__CUI__CInput__CPointerPoint_GetAt(history, count, &beyond) == E_BOUNDS && !beyond);
		state.retained_history = history; /* released in raven_pointer_finish */
	}
	/* The touch press's snapshot: ring entries are samples regardless of device, so the
	 * contact's own previous sample (its in-contact update) must round-trip with the
	 * touch device's identity and the id the message carried. */
	if (event == PRESSED && state.expect_type == PointerDeviceType_Touch && !state.retained_touch_args) {
		state.retained_touch_args = args; IPointerEventArgs_AddRef(args);
		__FIVector_1_Windows__CUI__CInput__CPointerPoint *history = NULL;
		CHECK(__x_ABI_CWindows_CUI_CCore_CIPointerEventArgs_GetIntermediatePoints(args, &history) == S_OK && history);
		UINT32 count = 0;
		CHECK(__FIVector_1_Windows__CUI__CInput__CPointerPoint_get_Size(history, &count) == S_OK && count >= 1);
		IPointerPoint *element = NULL;
		CHECK(__FIVector_1_Windows__CUI__CInput__CPointerPoint_GetAt(history, count - 1, &element) == S_OK && element);
		UINT32 element_id = 0;
		CHECK(IPointerPoint_get_pointer_id(element, &element_id) == S_OK && element_id == state.expect_id);
		IPointerDevice *element_device = NULL;
		CHECK(IPointerPoint_get_PointerDevice(element, &element_device) == S_OK && element_device == state.touch_device);
		IPointerDevice_Release(element_device);
		IPointerPoint_Release(element);
		state.retained_touch_history = history; /* released in raven_pointer_finish */
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

/* The engine-internal history builder, reached through GetProcAddress the way style_client reaches
 * raven_style_add_setter: the vector owns one reference per element, so the fixture balances by
 * releasing each fetched point once and the vector last. */
static int history_checks(void)
{
	HMODULE component = GetModuleHandleW(L"raven-xaml.dll");
	if (!component) return __LINE__;
	history_create_function create_history =
		(history_create_function)GetProcAddress(component, "raven_pointer_history_create");
	if (!create_history) return __LINE__;
	struct raven_mouse_sample samples[3];
	memset(samples, 0, sizeof samples);
	for (UINT32 i = 0; i < 3; ++i) {
		samples[i].position.X = 10.0f * (FLOAT)i;
		samples[i].position.Y = 20.0f * (FLOAT)i + 1.0f;
		samples[i].buttons = MK_LBUTTON;
		samples[i].frame = 100 + i;
		samples[i].timestamp = 1000 + i;
		samples[i].modifiers = VirtualKeyModifiers_Control;
		samples[i].update = PointerUpdateKind_Other;
	}
	__FIVector_1_Windows__CUI__CInput__CPointerPoint *history = NULL;
	if (create_history(samples, 3, &history) != S_OK || !history) return __LINE__;
	UINT32 count = 0;
	if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_get_Size(history, &count) != S_OK || count != 3) return __LINE__;
	IPointerPoint *points[3] = {NULL, NULL, NULL};
	for (UINT32 i = 0; i < count; ++i) {
		if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_GetAt(history, i, &points[i]) != S_OK || !points[i]) return __LINE__;
		Point position;
		UINT32 frame = 0;
		UINT64 timestamp = 0;
		if (IPointerPoint_get_Position(points[i], &position) != S_OK ||
			position.X != samples[i].position.X || position.Y != samples[i].position.Y) return __LINE__;
		if (IPointerPoint_get_FrameId(points[i], &frame) != S_OK || frame != samples[i].frame) return __LINE__;
		if (IPointerPoint_get_Timestamp(points[i], &timestamp) != S_OK || timestamp != samples[i].timestamp) return __LINE__;
	}
	IPointerPoint *beyond = (void *)1;
	if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_GetAt(history, count, &beyond) != E_BOUNDS || beyond) return __LINE__;
	BOOLEAN found = FALSE;
	UINT32 index = 0;
	if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_IndexOf(history, points[1], &index, &found) != S_OK ||
		!found || index != 1) return __LINE__;
	/* An element of another snapshot is a different object: IndexOf answers not-found. */
	__FIVector_1_Windows__CUI__CInput__CPointerPoint *other = NULL;
	if (create_history(&samples[0], 1, &other) != S_OK || !other) return __LINE__;
	IPointerPoint *foreign = NULL;
	if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_GetAt(other, 0, &foreign) != S_OK || !foreign) return __LINE__;
	if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_IndexOf(history, foreign, &index, &found) != S_OK ||
		found || index) return __LINE__;
	IPointerPoint_Release(foreign);
	IPointerPoint *many[3] = {NULL, NULL, NULL};
	UINT32 got = 0;
	if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_GetMany(history, 1, 3, many, &got) != S_OK || got != 2 ||
		many[0] != points[1] || many[1] != points[2]) return __LINE__;
	for (UINT32 i = 0; i < got; ++i)
		if (IPointerPoint_Release(many[i]) != 2) return __LINE__; /* the vector and GetAt each hold one */
	/* An empty snapshot is a valid vector, not a null out. */
	__FIVector_1_Windows__CUI__CInput__CPointerPoint *empty = NULL;
	if (create_history(NULL, 0, &empty) != S_OK || !empty) return __LINE__;
	UINT32 empty_count = 7;
	if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_get_Size(empty, &empty_count) != S_OK || empty_count) return __LINE__;
	IPointerPoint *nothing = (void *)1;
	if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_GetAt(empty, 0, &nothing) != E_BOUNDS || nothing) return __LINE__;
	if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_Release(empty) != 0) return __LINE__;
	if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_Release(other) != 0) return __LINE__;
	for (UINT32 i = 0; i < count; ++i)
		if (IPointerPoint_Release(points[i]) != 1) return __LINE__;
	if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_Release(history) != 0) return __LINE__;
	return 0;
}

/* The PointerDevice statics, reached through GetProcAddress the way the history builder is: the
 * class-name catalog entry and the RoGetActivationFactory chain stay with the integrator. */
typedef HRESULT (*statics_create_function)(__x_ABI_CWindows_CDevices_CInput_CIPointerDeviceStatics **);
static int device_checks(void)
{
	HMODULE component = GetModuleHandleW(L"raven-xaml.dll");
	if (!component) return __LINE__;
	statics_create_function create_statics =
		(statics_create_function)GetProcAddress(component, "raven_pointer_device_statics");
	if (!create_statics) return __LINE__;
	IPointerDeviceStatics *statics = NULL;
	if (create_statics(&statics) != S_OK || !statics) return __LINE__;
	__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice *view = NULL;
	if (IPointerDeviceStatics_GetPointerDevices(statics, &view) != S_OK || !view) return __LINE__;
	UINT32 count = 0;
	if (__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice_get_Size(view, &count) != S_OK || count < 1) return __LINE__;
	IPointerDevice *mouse = NULL;
	UINT32 mouse_index = 0;
	for (UINT32 i = 0; i < count; ++i) {
		IPointerDevice *device = NULL;
		if (__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice_GetAt(view, i, &device) != S_OK || !device) return __LINE__;
		PointerDeviceType type;
		if (IPointerDevice_get_PointerDeviceType(device, &type) != S_OK) return __LINE__;
		if (type == PointerDeviceType_Mouse && !mouse) {
			mouse = device;
			mouse_index = i;
			continue;
		}
		IPointerDevice_Release(device);
	}
	if (!mouse) return __LINE__;
	/* The view answers the same object identity the event path already verified. */
	if (mouse != state.device) return __LINE__;
	/* Documented mouse values: never integrated, one contact, an extent and its screen projection. */
	boolean integrated = TRUE;
	UINT32 contacts = 0;
	Rect physical, screen;
	if (IPointerDevice_get_IsIntegrated(mouse, &integrated) != S_OK || integrated) return __LINE__;
	if (IPointerDevice_get_MaxContacts(mouse, &contacts) != S_OK || contacts != 1) return __LINE__;
	if (IPointerDevice_get_PhysicalDeviceRect(mouse, &physical) != S_OK || physical.Width <= 0 || physical.Height <= 0) return __LINE__;
	if (IPointerDevice_get_ScreenRect(mouse, &screen) != S_OK || screen.Width <= 0 || screen.Height <= 0) return __LINE__;
	/* GetPointerDevice(1) resolves the mouse the events report; another id is an unknown pointer. */
	IPointerDevice *by_id = NULL;
	if (IPointerDeviceStatics_GetPointerDevice(statics, 1, &by_id) != S_OK || by_id != state.device) return __LINE__;
	if (IPointerDevice_Release(by_id) != 3) return __LINE__; /* state.device, the view, the fetched mouse */
	IPointerDevice *unknown = (void *)1;
	if (IPointerDeviceStatics_GetPointerDevice(statics, 2, &unknown) != E_INVALIDARG || unknown) return __LINE__;
	/* View bounds: one past the end answers E_BOUNDS; GetMany clips and reports what fit. */
	IPointerDevice *beyond = (void *)1;
	if (__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice_GetAt(view, count, &beyond) != E_BOUNDS || beyond) return __LINE__;
	BOOLEAN found = FALSE;
	UINT32 index = 0;
	if (__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice_IndexOf(view, mouse, &index, &found) != S_OK ||
		!found || index != mouse_index) return __LINE__;
	IPointerDevice *many[8] = {0};
	UINT32 got = 0;
	if (__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice_GetMany(view, 0, 8, many, &got) != S_OK ||
		got != (count < 8 ? count : 8)) return __LINE__;
	for (UINT32 i = 0; i < got; ++i) {
		IPointerDevice *element = NULL;
		if (__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice_GetAt(view, i, &element) != S_OK ||
			element != many[i]) return __LINE__;
		IPointerDevice_Release(element);
	}
	UINT32 drained = 0;
	if (__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice_GetMany(view, count, 8, many, &drained) != S_OK || drained) return __LINE__;
	if (__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice_GetMany(view, count + 1, 8, many, &drained) != E_BOUNDS || drained) return __LINE__;
	/* A second view is another snapshot of the same stable catalog: the mouse keeps its identity. */
	__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice *again = NULL;
	if (IPointerDeviceStatics_GetPointerDevices(statics, &again) != S_OK || !again) return __LINE__;
	IPointerDevice *same = NULL;
	if (__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice_GetAt(again, mouse_index, &same) != S_OK ||
		same != mouse) return __LINE__;
	IPointerDevice_Release(same);
	for (UINT32 i = 0; i < got; ++i)
		/* Every catalog entry is held by the catalog and both views; the mouse also by
		 * state.device and the fetched pointer. */
		if (IPointerDevice_Release(many[i]) != (many[i] == mouse ? 4 : 3)) return __LINE__;
	if (IPointerDevice_Release(mouse) != 3) return __LINE__; /* state.device and the two views remain */
	if (__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice_Release(view) != 0) return __LINE__;
	if (__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice_Release(again) != 0) return __LINE__;
	/* The class answers RoGetActivationFactory like any cataloged runtime class: the statics QI off the
	 * activation factory resolves the same singleton the export handed out. */
	HMODULE combase = LoadLibraryW(L"combase.dll");
	HRESULT (WINAPI *ro_factory)(HSTRING, REFIID, void **) = combase ?
		(void *)GetProcAddress(combase, "RoGetActivationFactory") : NULL;
	if (!ro_factory) return __LINE__;
	HSTRING device_name = NULL;
	static const WCHAR device_text[] = L"Windows.Devices.Input.PointerDevice";
	if (WindowsCreateString(device_text, ARRAYSIZE(device_text) - 1, &device_name) != S_OK) return __LINE__;
	IActivationFactory *device_activation = NULL;
	if (ro_factory(device_name, &IID___x_ABI_CWindows_CDevices_CInput_CIPointerDeviceStatics,
		(void **)&device_activation) != S_OK || !device_activation) return __LINE__;
	if (device_activation != (IActivationFactory *)statics) return __LINE__; /* the same singleton, seen through the chain */
	WindowsDeleteString(device_name);
	if (IActivationFactory_Release(device_activation) != 1) return __LINE__; /* the export's create ref remains */
	if (IPointerDeviceStatics_Release(statics) != 0) return __LINE__;
	return 0;
}

/* Touch delivery over the WM_POINTER source: the messages are posted straight at the
 * window's HWND — the emulated desktop cannot generate them from real input (no digitizer
 * in its enumeration, SendInput rejects touch injection), but the decode they drive is the
 * same code path a host-generated message would take. */
static int touch_checks(ICoreWindow *window, HWND hwnd, ICoreDispatcher *dispatcher)
{
	(void)window;
	int entered = state.counts[ENTERED], exited = state.counts[EXITED];
	int moved = state.counts[MOVED], pressed = state.counts[PRESSED], released = state.counts[RELEASED];
	state.expect_id = 5;
	state.expect_type = PointerDeviceType_Touch;
	/* Hover begins: in range, not in contact, the first pointer in range so primary. */
	SendMessageW(hwnd, WM_POINTERENTER, MAKEWPARAM(5, POINTER_MESSAGE_FLAG_INRANGE), MAKELPARAM(10, 20));
	if (state.counts[ENTERED] != entered + 1 || state.contact || !state.primary ||
		!state.in_range || state.position.X != 10 || state.position.Y != 20 ||
		state.update != PointerUpdateKind_Other) return __LINE__;
	/* The contact: in range and in contact, still the primary pointer. */
	SendMessageW(hwnd, WM_POINTERUPDATE, MAKEWPARAM(5, POINTER_MESSAGE_FLAG_INRANGE | POINTER_MESSAGE_FLAG_INCONTACT),
		MAKELPARAM(15, 25));
	if (state.counts[MOVED] != moved + 1 || !state.contact || !state.left || !state.primary ||
		state.position.X != 15 || state.position.Y != 25) return __LINE__;
	SendMessageW(hwnd, WM_POINTERDOWN, MAKEWPARAM(5, POINTER_MESSAGE_FLAG_NEW | POINTER_MESSAGE_FLAG_INRANGE |
		POINTER_MESSAGE_FLAG_INCONTACT | POINTER_MESSAGE_FLAG_PRIMARY), MAKELPARAM(20, 30));
	if (state.counts[PRESSED] != pressed + 1 || !state.contact || state.update != PointerUpdateKind_LeftButtonPressed)
		return __LINE__;
	/* A second simultaneous contact: its own id, never the primary one. */
	state.expect_id = 6;
	state.expect_primary = FALSE;
	SendMessageW(hwnd, WM_POINTERDOWN, MAKEWPARAM(6, POINTER_MESSAGE_FLAG_NEW | POINTER_MESSAGE_FLAG_INRANGE |
		POINTER_MESSAGE_FLAG_INCONTACT), MAKELPARAM(40, 50));
	if (state.counts[PRESSED] != pressed + 2 || !state.contact || state.primary) return __LINE__;
	state.expect_id = 6;
	SendMessageW(hwnd, WM_POINTERUPDATE, MAKEWPARAM(6, POINTER_MESSAGE_FLAG_INRANGE | POINTER_MESSAGE_FLAG_INCONTACT),
		MAKELPARAM(42, 52));
	if (state.position.X != 42 || state.position.Y != 52 || state.primary) return __LINE__;
	/* The contacts lift: out of contact, still in range until the leave. */
	SendMessageW(hwnd, WM_POINTERUP, MAKEWPARAM(6, POINTER_MESSAGE_FLAG_INRANGE), MAKELPARAM(43, 53));
	if (state.counts[RELEASED] != released + 1 || state.contact || state.left ||
		state.update != PointerUpdateKind_LeftButtonReleased) return __LINE__;
	state.expect_id = 5;
	/* Id 5 is the first contact still in range, so it is primary again. */
	state.expect_primary = TRUE;
	SendMessageW(hwnd, WM_POINTERUP, MAKEWPARAM(5, POINTER_MESSAGE_FLAG_INRANGE), MAKELPARAM(22, 32));
	if (state.counts[RELEASED] != released + 2 || state.contact) return __LINE__;
	/* The hover ends and the contact table drains; a leaving id drops out of the
	 * table before the primary answer is computed. */
	state.expect_in_range = FALSE;
	state.expect_primary = FALSE;
	SendMessageW(hwnd, WM_POINTERLEAVE, MAKEWPARAM(5, 0), MAKELPARAM(22, 32));
	if (state.counts[EXITED] != exited + 1 || state.in_range) return __LINE__;
	state.expect_id = 6;
	SendMessageW(hwnd, WM_POINTERLEAVE, MAKEWPARAM(6, 0), MAKELPARAM(43, 53));
	if (state.counts[EXITED] != exited + 2) return __LINE__;
	/* The touch device is the statics catalog's touch entry: the view, the ids the
	 * messages carried, and the event path all answer one object. */
	HMODULE component = GetModuleHandleW(L"raven-xaml.dll");
	if (!component) return __LINE__;
	statics_create_function create_statics =
		(statics_create_function)GetProcAddress(component, "raven_pointer_device_statics");
	if (!create_statics) return __LINE__;
	IPointerDeviceStatics *statics = NULL;
	if (create_statics(&statics) != S_OK || !statics) return __LINE__;
	__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice *view = NULL;
	if (IPointerDeviceStatics_GetPointerDevices(statics, &view) != S_OK || !view) return __LINE__;
	UINT32 count = 0;
	if (__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice_get_Size(view, &count) != S_OK || count < 2)
		return __LINE__;
	boolean found_touch = FALSE;
	for (UINT32 i = 0; i < count; ++i) {
		IPointerDevice *device = NULL;
		if (__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice_GetAt(view, i, &device) != S_OK || !device)
			return __LINE__;
		PointerDeviceType type;
		if (IPointerDevice_get_PointerDeviceType(device, &type) != S_OK) return __LINE__;
		if (type == PointerDeviceType_Touch) {
			if (device != state.touch_device) return __LINE__;
			found_touch = TRUE;
			/* No host source reports the stand-in digitizer's facts. */
			boolean integrated = FALSE;
			UINT32 contacts = 9;
			Rect rect;
			if (IPointerDevice_get_IsIntegrated(device, &integrated) != E_NOTIMPL) return __LINE__;
			if (IPointerDevice_get_MaxContacts(device, &contacts) != E_NOTIMPL) return __LINE__;
			if (IPointerDevice_get_PhysicalDeviceRect(device, &rect) != E_NOTIMPL) return __LINE__;
		}
		IPointerDevice_Release(device);
	}
	if (!found_touch) return __LINE__;
	if (__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice_Release(view) != 0) return __LINE__;
	/* The ids the engine delivered resolve to the same touch entry; one it never
	 * delivered is still an unknown pointer. */
	IPointerDevice *by_id = NULL;
	if (IPointerDeviceStatics_GetPointerDevice(statics, 5, &by_id) != S_OK || by_id != state.touch_device)
		return __LINE__;
	IPointerDevice_Release(by_id);
	by_id = NULL;
	if (IPointerDeviceStatics_GetPointerDevice(statics, 6, &by_id) != S_OK || by_id != state.touch_device)
		return __LINE__;
	IPointerDevice_Release(by_id);
	by_id = (void *)1;
	if (IPointerDeviceStatics_GetPointerDevice(statics, 7, &by_id) != E_INVALIDARG || by_id) return __LINE__;
	if (IPointerDeviceStatics_Release(statics) != 0) return __LINE__;
	/* The mouse checks must still pass untouched after the touch traffic. */
	state.expect_id = 1;
	state.expect_type = PointerDeviceType_Mouse;
	state.expect_primary = TRUE;
	state.expect_in_range = TRUE;
	if (!PostMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(4, 5))) return __LINE__;
	if (ICoreDispatcher_ProcessEvents(dispatcher, CoreProcessEventsOption_ProcessAllIfPresent) != S_OK) return __LINE__;
	if (state.counts[MOVED] != moved + 3 || state.position.X != 4 || state.position.Y != 5) return __LINE__;
	return 0;
}

int raven_pointer_checks(ICoreWindow *window, ICoreDispatcher *dispatcher, HWND hwnd)
{
	state.window = window;
	state.thread = GetCurrentThreadId();
	state.expect_id = 1;
	state.expect_type = PointerDeviceType_Mouse;
	state.expect_primary = TRUE;
	state.expect_in_range = TRUE;
	int cursor_result = cursor_checks(window);
	if (cursor_result) return cursor_result;
	int history_result = history_checks();
	if (history_result) return history_result;
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
	int device_result = device_checks();
	if (device_result) return device_result;
	int touch_result = touch_checks(window, hwnd, dispatcher);
	if (touch_result) return touch_result;
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
	/* The history snapshot was taken at press time and must still answer it, though hundreds of
	 * later samples have overwritten the window ring since. */
	if (!state.retained_history) return __LINE__;
	UINT32 history_count = 0;
	if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_get_Size(state.retained_history, &history_count) != S_OK ||
		history_count < 1) return __LINE__;
	IPointerPoint *oldest = NULL;
	if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_GetAt(state.retained_history, 0, &oldest) != S_OK || !oldest) return __LINE__;
	UINT32 oldest_frame = 0, current_frame = 0;
	if (IPointerPoint_get_FrameId(oldest, &oldest_frame) != S_OK) {
		IPointerPoint_Release(oldest);
		return __LINE__;
	}
	IPointerPoint_Release(oldest);
	if (IPointerPoint_get_FrameId(state.retained_point, &current_frame) != S_OK || oldest_frame >= current_frame) return __LINE__;
	if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_Release(state.retained_history) != 1) return __LINE__;
	if (IPointerEventArgs_Release(state.retained_args) != 0 || IPointerPoint_Release(state.retained_point) != 1 ||
		IPointerPointProperties_Release(state.retained_properties) != 0 || IPointerDevice_Release(state.device) != 0) return __LINE__;
	/* The touch press's snapshot answers the same way after everything above: the args
	 * hold the history, the history holds the contact's samples with their own device. */
	if (!state.retained_touch_args || !state.retained_touch_history || !state.touch_device) return __LINE__;
	if (__FIVector_1_Windows__CUI__CInput__CPointerPoint_Release(state.retained_touch_history) != 1) return __LINE__;
	if (IPointerEventArgs_Release(state.retained_touch_args) != 0) return __LINE__;
	/* Unlike the static mouse (the catalog carries it without a reference), the touch
	 * entry is owned by the catalog, which keeps its reference for the process. */
	if (IPointerDevice_Release(state.touch_device) != 1) return __LINE__;
	for (int i = 0; i < POINTER_EVENTS; ++i)
		if (state.handlers[i].refs != 1) return __LINE__;
	return 0;
}
