/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The input fixture: pointer events route from the desktop messages through the CoreWindow to the element tree. A compiled
 * page hosts a full-size blue grid carrying a 200x50 button; the client subscribes the button's Click (IButtonBase) and the
 * grid's PointerEntered/Exited (IUIElement), injects mouse messages on the interop HWND with client coordinates, and asserts
 * observable state: the enter/exit transitions of the grid, IsPressed across the press, exactly one Click for a press and
 * release on the button, none for a press and release off it, a failing handler not stopping the others, and the CoreWindow's
 * own pointer events still firing alongside the routing. State, not pixels. */
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CONST_VTABLE
#define CINTERFACE
#define COBJMACROS
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_Foundation
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <roapi.h>
#include <windows.ui.core.h>
#include <windows.foundation.h>
#include "../../native/upstream/raven-xaml/corewindow_interop.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_controls.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_elements.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_window.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_uielement.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_buttonbase.h"
#include "../../native/upstream/raven-xaml/xaml_input.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);
typedef HRESULT (WINAPI *load_function)(const void *, uint32_t, IInspectable **);
typedef HRESULT (WINAPI *unload_function)(void);

typedef __x_ABI_CWindows_CUI_CXaml_CIUIElement uielement1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPanel panel1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl content_control1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase buttonbase1;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindow window1;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindowStatics window_statics;

/* The typed handlers the element events cross the ABI as; the engine stores and invokes them through IUnknown. */
typedef __FITypedEventHandler_2_Windows__CUI__CXaml__CUIElement_Windows__CUI__CXaml__CInput__CPointerRoutedEventArgs element_pointer_handler;
typedef __FITypedEventHandler_2_Windows__CUI__CXaml__CUIElement_Windows__CUI__CXaml__CInput__CPointerRoutedEventArgsVtbl element_pointer_table;
typedef __FITypedEventHandler_2_Windows__CObject_Windows__CUI__CXaml__CRoutedEventArgs click_handler;
typedef __FITypedEventHandler_2_Windows__CObject_Windows__CUI__CXaml__CRoutedEventArgsVtbl click_table;
typedef __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs core_pointer_handler;
typedef __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgsVtbl core_pointer_table;

/* The IVector<UIElement> the panel's Children returns: the parameterized ABI has no mingw header, so the slots the fixture
 * reads (GetAt) are declared by hand, exactly as the engine's own collection lays them out. */
struct children_vector {
	const void *lpVtbl;
};
struct children_vector_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(void *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(void *This);
	ULONG (STDMETHODCALLTYPE *Release)(void *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(void *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(void *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(void *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetAt)(void *This, UINT32 index, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(void *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *GetView)(void *This, void **value);
	HRESULT (STDMETHODCALLTYPE *IndexOf)(void *This, IInspectable *element, UINT32 *index, boolean *value);
	HRESULT (STDMETHODCALLTYPE *SetAt)(void *This, UINT32 index, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *InsertAt)(void *This, UINT32 index, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *RemoveAt)(void *This, UINT32 index);
	HRESULT (STDMETHODCALLTYPE *Append)(void *This, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *RemoveAtEnd)(void *This);
	HRESULT (STDMETHODCALLTYPE *Clear)(void *This);
	HRESULT (STDMETHODCALLTYPE *GetMany)(void *This, UINT32 start_index, UINT32 items_size, IInspectable **items, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *ReplaceAll)(void *This, UINT32 count, IInspectable **items);
};

/* The element-side handlers: one that fails (the routing must deliver past it) and ones that count. */
struct element_state {
	const element_pointer_table *lpVtbl;
	LONG refs;
	int *count;
	HRESULT result;
};

static HRESULT WINAPI element_query(element_pointer_handler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct element_state *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI element_addref(element_pointer_handler *iface) { return InterlockedIncrement(&((struct element_state *)iface)->refs); }
static ULONG WINAPI element_release(element_pointer_handler *iface) { return InterlockedDecrement(&((struct element_state *)iface)->refs); }
static HRESULT WINAPI element_invoke(element_pointer_handler *iface, uielement1 *sender,
	__x_ABI_CWindows_CUI_CXaml_CInput_CIPointerRoutedEventArgs *args)
{
	struct element_state *handler = (struct element_state *)iface;
	(void)sender;
	(void)args;
	++*handler->count;
	return handler->result;
}
static const element_pointer_table element_table = {element_query, element_addref, element_release, element_invoke};

/* The Click handler: counts the raises and checks the sender is the button (one COM identity with the IUIElement held here). */
struct click_state {
	const click_table *lpVtbl;
	LONG refs;
	int *count;
	IUnknown *identity;
};
static HRESULT WINAPI click_query(click_handler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct click_state *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI click_addref(click_handler *iface) { return InterlockedIncrement(&((struct click_state *)iface)->refs); }
static ULONG WINAPI click_release(click_handler *iface) { return InterlockedDecrement(&((struct click_state *)iface)->refs); }
static HRESULT WINAPI click_invoke(click_handler *iface, IInspectable *sender, __x_ABI_CWindows_CUI_CXaml_CRoutedEventArgs *args)
{
	struct click_state *handler = (struct click_state *)iface;
	IUnknown *sender_identity = NULL;
	(void)args;
	if (FAILED(sender->lpVtbl->QueryInterface(sender, &IID_IUnknown, (void **)&sender_identity))) return E_FAIL;
	const int same = sender_identity == handler->identity;
	IUnknown_Release(sender_identity);
	if (!same) return E_FAIL;
	++*handler->count;
	return S_OK;
}
static const click_table click_table_value = {click_query, click_addref, click_release, click_invoke};

/* The CoreWindow's own pointer events keep reaching application handlers alongside the routing. */
struct core_state {
	const core_pointer_table *lpVtbl;
	LONG refs;
	int *count;
};
static HRESULT WINAPI core_query(core_pointer_handler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) &&
		!IsEqualIID(iid, &IID___FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs))
		return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct core_state *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI core_addref(core_pointer_handler *iface) { return InterlockedIncrement(&((struct core_state *)iface)->refs); }
static ULONG WINAPI core_release(core_pointer_handler *iface) { return InterlockedDecrement(&((struct core_state *)iface)->refs); }
static HRESULT WINAPI core_invoke(core_pointer_handler *iface, ICoreWindow *window, IPointerEventArgs *args)
{
	(void)window;
	(void)args;
	++*((struct core_state *)iface)->count;
	return S_OK;
}
static const core_pointer_table core_table_value = {core_query, core_addref, core_release, core_invoke};

static void put16(unsigned char *doc, size_t at, UINT16 value)
{
	doc[at] = (unsigned char)(value & 0xff);
	doc[at + 1] = (unsigned char)(value >> 8);
}

static void put32(unsigned char *doc, size_t at, UINT32 value)
{
	doc[at] = (unsigned char)value;
	doc[at + 1] = (unsigned char)(value >> 8);
	doc[at + 2] = (unsigned char)(value >> 16);
	doc[at + 3] = (unsigned char)(value >> 24);
}

static void put64(unsigned char *doc, size_t at, UINT64 value)
{
	for (int index = 0; index < 8; ++index)
		doc[at + index] = (unsigned char)(value >> (index * 8));
}

static void put_wide(unsigned char *doc, size_t at, const char *text)
{
	UINT32 length = 0;
	while (text[length])
		++length;
	put32(doc, at, length);
	at += 4;
	for (UINT32 index = 0; index < length; ++index) {
		doc[at++] = (unsigned char)text[index];
		doc[at++] = 0;
	}
}

/* The empty metadata section every fixture shares, with the substream table for one node stream. */
static size_t begin_document(unsigned char *doc)
{
	for (size_t index = 0; index < 1024; ++index)
		doc[index] = 0;
	doc[0] = 'X'; doc[1] = 'B'; doc[2] = 'F';
	put32(doc, 4, 144);
	put32(doc, 12, 2);
	put32(doc, 16, 1);
	put64(doc, 20, 0x78);
	put64(doc, 28, 0x7c);
	put64(doc, 36, 0x80);
	put64(doc, 44, 0x84);
	put64(doc, 52, 0x88);
	put64(doc, 60, 0x8c);
	put32(doc, 156, 1);
	put32(doc, 160, 0);
	return 168;
}

static size_t create_type(unsigned char *doc, size_t at, UINT16 stable)
{
	doc[at++] = 0x14;
	put16(doc, at, (UINT16)(stable | 0x8000));
	return at + 2;
}

static size_t set_property(unsigned char *doc, size_t at, UINT16 stable)
{
	doc[at++] = 0x07;
	put16(doc, at, (UINT16)(stable | 0x8000));
	return at + 2;
}

static size_t set_string(unsigned char *doc, size_t at, UINT16 stable, const char *text)
{
	doc[at++] = 0x1a;
	put16(doc, at, (UINT16)(stable | 0x8000));
	at += 2;
	doc[at++] = 0x09;
	UINT32 length = 0;
	while (text[length])
		++length;
	put32(doc, at, length);
	at += 4;
	for (UINT32 index = 0; index < length; ++index) {
		doc[at++] = (unsigned char)text[index];
		doc[at++] = 0;
	}
	return at;
}

static size_t set_float(unsigned char *doc, size_t at, UINT16 stable, float value)
{
	doc[at++] = 0x1a;
	put16(doc, at, (UINT16)(stable | 0x8000));
	at += 2;
	doc[at++] = 0x03;
	put32(doc, at, (UINT32)0);
	UINT32 bits = 0;
	const unsigned char *bytes = (const unsigned char *)&value;
	for (int index = 0; index < 4; ++index)
		bits |= (UINT32)bytes[index] << (index * 8);
	put32(doc, at, bits);
	return at + 4;
}

/* A page whose content is a blue grid carrying a button sized by its own Width and Height. */
static size_t build_input_document(unsigned char *doc)
{
	size_t at = begin_document(doc);
	at = create_type(doc, at, 525);                       /* Page */
	at = set_property(doc, at, 999);                      /* Content */
	at = create_type(doc, at, 442);                       /* blue grid */
	at = set_string(doc, at, 647, "Blue");                /* Background */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 540);                       /* Button */
	at = set_float(doc, at, 409, 200.0f);                 /* Width */
	at = set_float(doc, at, 393, 50.0f);                  /* Height */
	doc[at++] = 0x02;                                     /* pop the button */
	doc[at++] = 0x02;                                     /* pop the blue grid */
	const UINT32 nodes = (UINT32)(at - 168);
	put32(doc, 8, nodes);
	put32(doc, 164, nodes);
	return at;
}

int raven_entry(void)
{
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function RoGetActivationFactory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	if (!RoInitialize || !RoGetActivationFactory) return 20;
	if (FAILED(RoInitialize(1))) return 1;

	HSTRING core_key = NULL;
	static const WCHAR core_name[] = L"Windows.UI.Core.CoreWindow";
	if (WindowsCreateString(core_name, ARRAYSIZE(core_name) - 1, &core_key) != S_OK) return 21;
	__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic *statics = NULL;
	if (RoGetActivationFactory(core_key, &IID___x_ABI_CWindows_CUI_CCore_CICoreWindowStatic, (void **)&statics) != S_OK ||
		!statics) return 2;
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *window = NULL;
	if (ICoreWindowStatic_GetForCurrentThread(statics, &window) != S_OK || !window) return 3;
	raven_core_window_interop *interop = NULL;
	if (window->lpVtbl->QueryInterface(window, &raven_core_window_interop_iid, (void **)&interop) != S_OK || !interop) return 4;
	HWND hwnd = NULL;
	if (interop->lpVtbl->get_WindowHandle(interop, &hwnd) != S_OK || !hwnd) return 5;
	ICoreWindow_Activate(window);

	/* The compiled page the fixture hosts. */
	HMODULE component = LoadLibraryW(L"raven-xaml.dll");
	load_function load = component ? (load_function)GetProcAddress(component, "RavenXamlLoad") : NULL;
	if (!load) return 6;
	static unsigned char document[1024];
	IInspectable *page = NULL;
	const size_t size = build_input_document(document);
	if (load(document, (uint32_t)size, &page) != S_OK || !page) return 7;

	/* Hosting the page subscribes the routing to the window's pointer events. */
	HSTRING window_key = NULL;
	static const WCHAR window_name[] = L"Windows.UI.Xaml.Window";
	if (WindowsCreateString(window_name, ARRAYSIZE(window_name) - 1, &window_key) != S_OK) return 22;
	IActivationFactory *window_activation = NULL;
	if (RoGetActivationFactory(window_key, &IID_IActivationFactory, (void **)&window_activation) != S_OK || !window_activation)
		return 8;
	window_statics *window_current = NULL;
	if (window_activation->lpVtbl->QueryInterface(window_activation, &IID__x_ABI_CWindows_CUI_CXaml_CIWindowStatics,
		(void **)&window_current) != S_OK || !window_current) return 9;
	window1 *xaml_window = NULL;
	if (window_current->lpVtbl->get_Current(window_current, (uint64_t)(uintptr_t)&xaml_window) != S_OK || !xaml_window) return 10;
	if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)page) != S_OK) return 11;

	/* Reach the tree: the page's grid and the button it hosts. */
	content_control1 *page_content = NULL;
	if (page->lpVtbl->QueryInterface(page, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
		(void **)&page_content) != S_OK || !page_content) return 12;
	IInspectable *grid_object = NULL;
	if (page_content->lpVtbl->get_Content(page_content, (uint64_t)(uintptr_t)&grid_object) != S_OK || !grid_object) return 13;
	page_content->lpVtbl->Release(page_content);
	panel1 *grid_panel = NULL;
	if (grid_object->lpVtbl->QueryInterface(grid_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
		(void **)&grid_panel) != S_OK || !grid_panel) return 14;
	void *children = NULL;
	if (grid_panel->lpVtbl->get_Children(grid_panel, (uint64_t)(uintptr_t)&children) != S_OK || !children) return 15;
	const struct children_vector_vtbl *children_vtbl = ((struct children_vector *)children)->lpVtbl;
	IInspectable *button_object = NULL;
	if (children_vtbl->GetAt(children, 0, &button_object) != S_OK || !button_object) return 16;
	if (children_vtbl->Release(children) != 1) return 17;
	grid_panel->lpVtbl->Release(grid_panel);
	uielement1 *button_ui = NULL;
	if (button_object->lpVtbl->QueryInterface(button_object, &IID__x_ABI_CWindows_CUI_CXaml_CIUIElement,
		(void **)&button_ui) != S_OK || !button_ui) return 18;
	uielement1 *grid_ui = NULL;
	if (grid_object->lpVtbl->QueryInterface(grid_object, &IID__x_ABI_CWindows_CUI_CXaml_CIUIElement,
		(void **)&grid_ui) != S_OK || !grid_ui) return 19;
	IUnknown *button_identity = NULL;
	if (button_object->lpVtbl->QueryInterface(button_object, &IID_IUnknown, (void **)&button_identity) != S_OK || !button_identity)
		return 23;

	/* The subscriptions: the grid's enter and exit (one failing handler first), the button's Click, the CoreWindow's own moves. */
	int entered_count = 0, failed_count = 0, exited_count = 0, click_count = 0, core_moves = 0;
	struct element_state failing_entered = {&element_table, 1, &failed_count, E_ACCESSDENIED};
	struct element_state counting_entered = {&element_table, 1, &entered_count, S_OK};
	struct element_state counting_exited = {&element_table, 1, &exited_count, S_OK};
	struct click_state click_handler = {&click_table_value, 1, &click_count, button_identity};
	struct core_state core_handler = {&core_table_value, 1, &core_moves};
	EventRegistrationToken fail_token = {0}, entered_token = {0}, exited_token = {0}, click_token = {0}, core_token = {0};
	if (grid_ui->lpVtbl->add_PointerEntered(grid_ui, (uint64_t)(uintptr_t)&failing_entered,
		(uint64_t)(uintptr_t)&fail_token) != S_OK || !fail_token.value) return 24;
	if (grid_ui->lpVtbl->add_PointerEntered(grid_ui, (uint64_t)(uintptr_t)&counting_entered,
		(uint64_t)(uintptr_t)&entered_token) != S_OK || !entered_token.value) return 25;
	if (grid_ui->lpVtbl->add_PointerExited(grid_ui, (uint64_t)(uintptr_t)&counting_exited,
		(uint64_t)(uintptr_t)&exited_token) != S_OK || !exited_token.value) return 26;
	buttonbase1 *button_base = NULL;
	if (button_object->lpVtbl->QueryInterface(button_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase,
		(void **)&button_base) != S_OK || !button_base) return 27;
	if (button_base->lpVtbl->add_Click(button_base, (uint64_t)(uintptr_t)&click_handler,
		(uint64_t)(uintptr_t)&click_token) != S_OK || !click_token.value) return 28;
	if (ICoreWindow_add_PointerMoved(window, (void *)&core_handler, &core_token) != S_OK || !core_token.value) return 29;

	/* An element is hit-testable until the application says otherwise; the flag round-trips. */
	boolean visible = FALSE;
	if (grid_ui->lpVtbl->get_IsHitTestVisible(grid_ui, (uint64_t)(uintptr_t)&visible) != S_OK || !visible) return 30;
	if (grid_ui->lpVtbl->put_IsHitTestVisible(grid_ui, (uint64_t)FALSE) != S_OK) return 31;
	if (grid_ui->lpVtbl->get_IsHitTestVisible(grid_ui, (uint64_t)(uintptr_t)&visible) != S_OK || visible) return 32;
	if (grid_ui->lpVtbl->put_IsHitTestVisible(grid_ui, (uint64_t)TRUE) != S_OK ||
		grid_ui->lpVtbl->get_IsHitTestVisible(grid_ui, (uint64_t)(uintptr_t)&visible) != S_OK || !visible) return 33;
	if (button_ui->lpVtbl->get_IsHitTestVisible(button_ui, (uint64_t)(uintptr_t)&visible) != S_OK || !visible) return 34;
	enum raven_xaml_click_mode mode = raven_xaml_click_hover;
	if (button_base->lpVtbl->get_ClickMode(button_base, (uint64_t)(uintptr_t)&mode) != S_OK ||
		mode != raven_xaml_click_release) return 35;

	/* A move into the button: the grid enters (through the failing handler first), and the CoreWindow's own move still fires. */
	SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(100, 25));
	if (entered_count != 1 || failed_count != 1 || exited_count != 0 || core_moves != 1) return 36;

	/* Press and release on the button: IsPressed crosses the press, Click raises exactly once. */
	SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(100, 25));
	boolean pressed = FALSE;
	if (button_base->lpVtbl->get_IsPressed(button_base, (uint64_t)(uintptr_t)&pressed) != S_OK || !pressed) return 37;
	SendMessageW(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(100, 25));
	if (click_count != 1) return 38;
	if (button_base->lpVtbl->get_IsPressed(button_base, (uint64_t)(uintptr_t)&pressed) != S_OK || pressed) return 39;

	/* Out of the window: every hovered element exits, the grid among them. */
	SendMessageW(hwnd, WM_MOUSELEAVE, 0, 0);
	if (exited_count != 1) return 40;

	/* Back over the grid background, then a press and release off the button: the grid is pressed, the button is not, and no
	 * Click fires. The grid stays hovered over its own background, so no further exit. */
	SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(400, 300));
	if (entered_count != 2 || failed_count != 2 || core_moves != 2 || exited_count != 1) return 41;
	SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(400, 300));
	SendMessageW(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(400, 300));
	if (click_count != 1) return 42;
	if (button_base->lpVtbl->get_IsPressed(button_base, (uint64_t)(uintptr_t)&pressed) != S_OK || pressed) return 43;

	/* The failing handler never stopped the others; revocation removes exactly what add handed out. */
	if (grid_ui->lpVtbl->remove_PointerEntered(grid_ui, (uint64_t)fail_token.value) != S_OK) return 44;
	if (grid_ui->lpVtbl->remove_PointerEntered(grid_ui, (uint64_t)entered_token.value) != S_OK) return 45;
	if (grid_ui->lpVtbl->remove_PointerExited(grid_ui, (uint64_t)exited_token.value) != S_OK) return 46;
	if (button_base->lpVtbl->remove_Click(button_base, (uint64_t)click_token.value) != S_OK) return 47;
	SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(500, 400));
	if (entered_count != 2 || failed_count != 2 || core_moves != 3) return 48;

	/* Closing the window unsubscribes the routing; the tree below survives the close intact. */
	if (ICoreWindow_Close(window) != S_OK) return 49;

	button_base->lpVtbl->Release(button_base);
	if (failing_entered.refs != 1 || counting_entered.refs != 1 || counting_exited.refs != 1 ||
		click_handler.refs != 1 || core_handler.refs != 1) return 50;
	IUnknown_Release(button_identity);
	button_ui->lpVtbl->Release(button_ui);
	grid_ui->lpVtbl->Release(grid_ui);
	button_object->lpVtbl->Release(button_object);
	grid_object->lpVtbl->Release(grid_object);
	xaml_window->lpVtbl->Release(xaml_window);
	window_current->lpVtbl->Release(window_current);
	window_activation->lpVtbl->Release(window_activation);
	WindowsDeleteString(window_key);
	page->lpVtbl->Release(page);
	ICoreWindow_Release(window);
	ICoreWindowStatic_Release(statics);
	WindowsDeleteString(core_key);
	interop->lpVtbl->Release(interop);
	return 0;
}
