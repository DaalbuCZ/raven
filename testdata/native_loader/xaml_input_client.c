/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The input fixture: pointer events route from the desktop messages through the CoreWindow to the element tree, and the
 * pressed, released and wheel events bubble. A compiled page hosts a full-window blue grid (the grandparent) carrying a
 * 300x300 red grid (the parent) that carries a 100x100 green grid (the child) that carries a 200x50 button; the client
 * stores PointerPressed/Released/WheelChanged handlers on all three grids through the public IUIElement event slots (each
 * appending its id — child 1, parent 2, grandparent 3 — to a shared call-order log), the button's Click (IButtonBase) and
 * enter/exit handlers on the outer grids, injects mouse messages on the interop HWND, and asserts observable state: a press,
 * release or wheel on the child runs the child's handler first, then the parent's, then the grandparent's; a button press
 * clicks exactly once even though the grids above it hold PointerPressed handlers; a pressed element that no longer hovers
 * receives its release alone; and entered/exited never bubble — a parent's entered handler fires once per hover entry, never
 * again as a bubble of the child's delivery. State, not pixels. */
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

/* The element-side handlers: one that fails (the routing must deliver past it) and ones that count. The counting handlers
 * also append their id to the shared call-order log the bubble checks compare after each dispatch; id zero logs nothing. */
struct element_state {
	const element_pointer_table *lpVtbl;
	LONG refs;
	int *count;
	HRESULT result;
	int id;
};

/* The shared call-order log: reset before each injected event, compared after it. */
static int order_log[32];
static int order_count;

static int order_is(const int *expected, int count)
{
	if (order_count != count)
		return 0;
	for (int index = 0; index < count; ++index)
		if (order_log[index] != expected[index])
			return 0;
	return 1;
}

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
	if (handler->id && order_count < (int)(sizeof order_log / sizeof *order_log))
		order_log[order_count++] = handler->id;
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

/* A page whose content is a full-window blue grid carrying a 300x300 red grid that carries a 100x100 green grid that carries
 * a button sized by its own Width and Height. The blue grid must paint (a non-zero background) for its descendants to render
 * — and so to be recorded into the hit-test geometry — at every level of the nesting. */
static size_t build_input_document(unsigned char *doc)
{
	size_t at = begin_document(doc);
	at = create_type(doc, at, 525);                       /* Page */
	at = set_property(doc, at, 999);                      /* Content */
	at = create_type(doc, at, 442);                       /* blue grid (grandparent, fills the window) */
	at = set_string(doc, at, 647, "Blue");                /* Background */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 442);                       /* red grid (parent) */
	at = set_string(doc, at, 647, "Red");                 /* Background */
	at = set_float(doc, at, 409, 300.0f);                 /* Width */
	at = set_float(doc, at, 393, 300.0f);                 /* Height */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 442);                       /* green grid (child) */
	at = set_string(doc, at, 647, "Green");               /* Background */
	at = set_float(doc, at, 409, 100.0f);                 /* Width */
	at = set_float(doc, at, 393, 100.0f);                 /* Height */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 540);                       /* Button */
	at = set_float(doc, at, 409, 200.0f);                 /* Width */
	at = set_float(doc, at, 393, 50.0f);                  /* Height */
	doc[at++] = 0x02;                                     /* pop the button */
	doc[at++] = 0x02;                                     /* pop the green grid */
	doc[at++] = 0x02;                                     /* pop the red grid */
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

	/* Reach the tree: the page's grandparent grid, the parent grid inside it, the child grid inside that, and the button. */
	content_control1 *page_content = NULL;
	if (page->lpVtbl->QueryInterface(page, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
		(void **)&page_content) != S_OK || !page_content) return 12;
	IInspectable *outer_object = NULL;
	if (page_content->lpVtbl->get_Content(page_content, (uint64_t)(uintptr_t)&outer_object) != S_OK || !outer_object) return 13;
	page_content->lpVtbl->Release(page_content);
	panel1 *outer_panel = NULL;
	if (outer_object->lpVtbl->QueryInterface(outer_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
		(void **)&outer_panel) != S_OK || !outer_panel) return 14;
	void *outer_children = NULL;
	if (outer_panel->lpVtbl->get_Children(outer_panel, (uint64_t)(uintptr_t)&outer_children) != S_OK || !outer_children) return 15;
	const struct children_vector_vtbl *outer_children_vtbl = ((struct children_vector *)outer_children)->lpVtbl;
	IInspectable *middle_object = NULL;
	if (outer_children_vtbl->GetAt(outer_children, 0, &middle_object) != S_OK || !middle_object) return 16;
	if (outer_children_vtbl->Release(outer_children) != 1) return 17;
	outer_panel->lpVtbl->Release(outer_panel);
	panel1 *middle_panel = NULL;
	if (middle_object->lpVtbl->QueryInterface(middle_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
		(void **)&middle_panel) != S_OK || !middle_panel) return 18;
	void *middle_children = NULL;
	if (middle_panel->lpVtbl->get_Children(middle_panel, (uint64_t)(uintptr_t)&middle_children) != S_OK || !middle_children)
		return 19;
	const struct children_vector_vtbl *middle_children_vtbl = ((struct children_vector *)middle_children)->lpVtbl;
	IInspectable *child_object = NULL;
	if (middle_children_vtbl->GetAt(middle_children, 0, &child_object) != S_OK || !child_object) return 20;
	if (middle_children_vtbl->Release(middle_children) != 1) return 21;
	middle_panel->lpVtbl->Release(middle_panel);
	panel1 *child_panel = NULL;
	if (child_object->lpVtbl->QueryInterface(child_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
		(void **)&child_panel) != S_OK || !child_panel) return 22;
	void *child_children = NULL;
	if (child_panel->lpVtbl->get_Children(child_panel, (uint64_t)(uintptr_t)&child_children) != S_OK || !child_children) return 23;
	const struct children_vector_vtbl *child_children_vtbl = ((struct children_vector *)child_children)->lpVtbl;
	IInspectable *button_object = NULL;
	if (child_children_vtbl->GetAt(child_children, 0, &button_object) != S_OK || !button_object) return 24;
	if (child_children_vtbl->Release(child_children) != 1) return 25;
	child_panel->lpVtbl->Release(child_panel);
	uielement1 *button_ui = NULL;
	if (button_object->lpVtbl->QueryInterface(button_object, &IID__x_ABI_CWindows_CUI_CXaml_CIUIElement,
		(void **)&button_ui) != S_OK || !button_ui) return 26;
	uielement1 *outer_ui = NULL;
	if (outer_object->lpVtbl->QueryInterface(outer_object, &IID__x_ABI_CWindows_CUI_CXaml_CIUIElement,
		(void **)&outer_ui) != S_OK || !outer_ui) return 27;
	uielement1 *middle_ui = NULL;
	if (middle_object->lpVtbl->QueryInterface(middle_object, &IID__x_ABI_CWindows_CUI_CXaml_CIUIElement,
		(void **)&middle_ui) != S_OK || !middle_ui) return 28;
	uielement1 *child_ui = NULL;
	if (child_object->lpVtbl->QueryInterface(child_object, &IID__x_ABI_CWindows_CUI_CXaml_CIUIElement,
		(void **)&child_ui) != S_OK || !child_ui) return 29;
	IUnknown *button_identity = NULL;
	if (button_object->lpVtbl->QueryInterface(button_object, &IID_IUnknown, (void **)&button_identity) != S_OK || !button_identity)
		return 30;

	/* The subscriptions: enter/exit on the outer grids (one failing handler first), the pressed/released/wheel handlers on
	 * all three grids through the public IUIElement slots (ids 1, 2, 3 in the call-order log), the button's Click, and the
	 * CoreWindow's own moves. */
	int entered_count = 0, failed_count = 0, middle_entered_count = 0, exited_count = 0, click_count = 0, core_moves = 0;
	int child_pressed_count = 0, middle_pressed_count = 0, outer_pressed_count = 0;
	int child_released_count = 0, middle_released_count = 0, outer_released_count = 0;
	int child_wheel_count = 0, middle_wheel_count = 0, outer_wheel_count = 0;
	struct element_state failing_entered = {&element_table, 1, &failed_count, E_ACCESSDENIED, 0};
	struct element_state counting_entered = {&element_table, 1, &entered_count, S_OK, 0};
	struct element_state middle_entered = {&element_table, 1, &middle_entered_count, S_OK, 0};
	struct element_state counting_exited = {&element_table, 1, &exited_count, S_OK, 0};
	struct element_state child_pressed = {&element_table, 1, &child_pressed_count, S_OK, 1};
	struct element_state middle_pressed = {&element_table, 1, &middle_pressed_count, S_OK, 2};
	struct element_state outer_pressed = {&element_table, 1, &outer_pressed_count, S_OK, 3};
	struct element_state child_released = {&element_table, 1, &child_released_count, S_OK, 1};
	struct element_state middle_released = {&element_table, 1, &middle_released_count, S_OK, 2};
	struct element_state outer_released = {&element_table, 1, &outer_released_count, S_OK, 3};
	struct element_state child_wheel = {&element_table, 1, &child_wheel_count, S_OK, 1};
	struct element_state middle_wheel = {&element_table, 1, &middle_wheel_count, S_OK, 2};
	struct element_state outer_wheel = {&element_table, 1, &outer_wheel_count, S_OK, 3};
	struct click_state click_handler = {&click_table_value, 1, &click_count, button_identity};
	struct core_state core_handler = {&core_table_value, 1, &core_moves};
	EventRegistrationToken fail_token = {0}, entered_token = {0}, middle_entered_token = {0}, exited_token = {0};
	EventRegistrationToken child_pressed_token = {0}, middle_pressed_token = {0}, outer_pressed_token = {0};
	EventRegistrationToken child_released_token = {0}, middle_released_token = {0}, outer_released_token = {0};
	EventRegistrationToken child_wheel_token = {0}, middle_wheel_token = {0}, outer_wheel_token = {0};
	EventRegistrationToken click_token = {0}, core_token = {0};
	if (outer_ui->lpVtbl->add_PointerEntered(outer_ui, (uint64_t)(uintptr_t)&failing_entered,
		(uint64_t)(uintptr_t)&fail_token) != S_OK || !fail_token.value) return 31;
	if (outer_ui->lpVtbl->add_PointerEntered(outer_ui, (uint64_t)(uintptr_t)&counting_entered,
		(uint64_t)(uintptr_t)&entered_token) != S_OK || !entered_token.value) return 32;
	if (middle_ui->lpVtbl->add_PointerEntered(middle_ui, (uint64_t)(uintptr_t)&middle_entered,
		(uint64_t)(uintptr_t)&middle_entered_token) != S_OK || !middle_entered_token.value) return 33;
	if (outer_ui->lpVtbl->add_PointerExited(outer_ui, (uint64_t)(uintptr_t)&counting_exited,
		(uint64_t)(uintptr_t)&exited_token) != S_OK || !exited_token.value) return 34;
	if (child_ui->lpVtbl->add_PointerPressed(child_ui, (uint64_t)(uintptr_t)&child_pressed,
		(uint64_t)(uintptr_t)&child_pressed_token) != S_OK || !child_pressed_token.value) return 35;
	if (middle_ui->lpVtbl->add_PointerPressed(middle_ui, (uint64_t)(uintptr_t)&middle_pressed,
		(uint64_t)(uintptr_t)&middle_pressed_token) != S_OK || !middle_pressed_token.value) return 36;
	if (outer_ui->lpVtbl->add_PointerPressed(outer_ui, (uint64_t)(uintptr_t)&outer_pressed,
		(uint64_t)(uintptr_t)&outer_pressed_token) != S_OK || !outer_pressed_token.value) return 37;
	if (child_ui->lpVtbl->add_PointerReleased(child_ui, (uint64_t)(uintptr_t)&child_released,
		(uint64_t)(uintptr_t)&child_released_token) != S_OK || !child_released_token.value) return 38;
	if (middle_ui->lpVtbl->add_PointerReleased(middle_ui, (uint64_t)(uintptr_t)&middle_released,
		(uint64_t)(uintptr_t)&middle_released_token) != S_OK || !middle_released_token.value) return 39;
	if (outer_ui->lpVtbl->add_PointerReleased(outer_ui, (uint64_t)(uintptr_t)&outer_released,
		(uint64_t)(uintptr_t)&outer_released_token) != S_OK || !outer_released_token.value) return 40;
	if (child_ui->lpVtbl->add_PointerWheelChanged(child_ui, (uint64_t)(uintptr_t)&child_wheel,
		(uint64_t)(uintptr_t)&child_wheel_token) != S_OK || !child_wheel_token.value) return 41;
	if (middle_ui->lpVtbl->add_PointerWheelChanged(middle_ui, (uint64_t)(uintptr_t)&middle_wheel,
		(uint64_t)(uintptr_t)&middle_wheel_token) != S_OK || !middle_wheel_token.value) return 42;
	if (outer_ui->lpVtbl->add_PointerWheelChanged(outer_ui, (uint64_t)(uintptr_t)&outer_wheel,
		(uint64_t)(uintptr_t)&outer_wheel_token) != S_OK || !outer_wheel_token.value) return 43;
	buttonbase1 *button_base = NULL;
	if (button_object->lpVtbl->QueryInterface(button_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase,
		(void **)&button_base) != S_OK || !button_base) return 44;
	if (button_base->lpVtbl->add_Click(button_base, (uint64_t)(uintptr_t)&click_handler,
		(uint64_t)(uintptr_t)&click_token) != S_OK || !click_token.value) return 45;
	if (ICoreWindow_add_PointerMoved(window, (void *)&core_handler, &core_token) != S_OK || !core_token.value) return 46;

	/* An element is hit-testable until the application says otherwise; the flag round-trips. */
	boolean visible = FALSE;
	if (outer_ui->lpVtbl->get_IsHitTestVisible(outer_ui, (uint64_t)(uintptr_t)&visible) != S_OK || !visible) return 47;
	if (outer_ui->lpVtbl->put_IsHitTestVisible(outer_ui, (uint64_t)FALSE) != S_OK) return 48;
	if (outer_ui->lpVtbl->get_IsHitTestVisible(outer_ui, (uint64_t)(uintptr_t)&visible) != S_OK || visible) return 49;
	if (outer_ui->lpVtbl->put_IsHitTestVisible(outer_ui, (uint64_t)TRUE) != S_OK ||
		outer_ui->lpVtbl->get_IsHitTestVisible(outer_ui, (uint64_t)(uintptr_t)&visible) != S_OK || !visible) return 50;
	if (button_ui->lpVtbl->get_IsHitTestVisible(button_ui, (uint64_t)(uintptr_t)&visible) != S_OK || !visible) return 51;
	enum raven_xaml_click_mode mode = raven_xaml_click_hover;
	if (button_base->lpVtbl->get_ClickMode(button_base, (uint64_t)(uintptr_t)&mode) != S_OK ||
		mode != raven_xaml_click_release) return 52;

	/* A move onto the child's own background: the child, its parent and its grandparent all enter — the flat hover set — and
	 * each entered handler fires exactly once. Entered never bubbles: the parent's and grandparent's deliveries are their
	 * own hover entries, not a second run triggered by the child's delivery. */
	SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(75, 75));
	if (entered_count != 1 || failed_count != 1 || middle_entered_count != 1 || exited_count != 0 || core_moves != 1) return 53;

	/* A release with no pressed element routes like a press: the child is the topmost element under the pointer, and the
	 * release bubbles through the parent, then the grandparent. */
	order_count = 0;
	SendMessageW(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(75, 75));
	{
		static const int expected[] = {1, 2, 3};
		if (!order_is(expected, 3)) return 54;
	}
	if (child_released_count != 1 || middle_released_count != 1 || outer_released_count != 1) return 55;

	/* A wheel over the child bubbles the same way: child first, then parent, then grandparent. The wheel message carries
	 * screen coordinates, so the point goes through ClientToScreen first. */
	order_count = 0;
	POINT wheel_point = {75, 75};
	ClientToScreen(hwnd, &wheel_point);
	SendMessageW(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(0, 120), MAKELPARAM(wheel_point.x, wheel_point.y));
	{
		static const int expected[] = {1, 2, 3};
		if (!order_is(expected, 3)) return 56;
	}
	if (child_wheel_count != 1 || middle_wheel_count != 1 || outer_wheel_count != 1) return 57;

	/* A move onto the button, then a press on it: the button is the target and its own behavior tracks the press, while the
	 * press bubbles through the child, the parent and the grandparent — each grid's pressed handler runs once, in order. */
	SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(25, 10));
	if (core_moves != 2) return 58;
	order_count = 0;
	SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(25, 10));
	{
		static const int expected[] = {1, 2, 3};
		if (!order_is(expected, 3)) return 59;
	}
	if (child_pressed_count != 1 || middle_pressed_count != 1 || outer_pressed_count != 1) return 60;
	boolean pressed = FALSE;
	if (button_base->lpVtbl->get_IsPressed(button_base, (uint64_t)(uintptr_t)&pressed) != S_OK || !pressed) return 61;

	/* The release: the pressed element (the button) still hovers, so the release bubbles through its chain read from the
	 * hover set — child, parent, grandparent — and the Click behavior raises exactly once despite the grids' pressed
	 * handlers having run with the press. */
	order_count = 0;
	SendMessageW(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(25, 10));
	{
		static const int expected[] = {1, 2, 3};
		if (!order_is(expected, 3)) return 62;
	}
	if (click_count != 1) return 63;
	if (child_released_count != 2 || middle_released_count != 2 || outer_released_count != 2) return 64;
	if (button_base->lpVtbl->get_IsPressed(button_base, (uint64_t)(uintptr_t)&pressed) != S_OK || pressed) return 65;

	/* Back over the child's own background, a press there: the child is the target this time and the same chain runs. */
	SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(75, 75));
	order_count = 0;
	SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(75, 75));
	{
		static const int expected[] = {1, 2, 3};
		if (!order_is(expected, 3)) return 66;
	}
	if (child_pressed_count != 2 || middle_pressed_count != 2 || outer_pressed_count != 2) return 67;

	/* Out of the window: every hovered element exits, the outer grid among them, and the hover set clears. */
	SendMessageW(hwnd, WM_MOUSELEAVE, 0, 0);
	if (exited_count != 1) return 68;

	/* The release lands while the pressed child is no longer hovered: the child receives it alone — the flat geometry has
	 * no tree to walk, so nothing bubbles for a pressed element outside the hover set. */
	order_count = 0;
	SendMessageW(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(400, 300));
	{
		static const int expected[] = {1};
		if (!order_is(expected, 1)) return 69;
	}
	if (child_released_count != 3 || middle_released_count != 2 || outer_released_count != 2 || click_count != 1) return 70;

	/* Back in: the child, parent and grandparent enter again, each entered handler firing exactly once per entry — the
	 * child's entered delivery did not bubble into a second parent run. */
	SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(75, 75));
	if (entered_count != 2 || failed_count != 2 || middle_entered_count != 2 || core_moves != 4) return 71;

	/* Revocation removes exactly what add handed out; after it the routing delivers nothing to the elements. */
	if (outer_ui->lpVtbl->remove_PointerEntered(outer_ui, (uint64_t)fail_token.value) != S_OK) return 72;
	if (outer_ui->lpVtbl->remove_PointerEntered(outer_ui, (uint64_t)entered_token.value) != S_OK) return 73;
	if (middle_ui->lpVtbl->remove_PointerEntered(middle_ui, (uint64_t)middle_entered_token.value) != S_OK) return 74;
	if (outer_ui->lpVtbl->remove_PointerExited(outer_ui, (uint64_t)exited_token.value) != S_OK) return 75;
	if (child_ui->lpVtbl->remove_PointerPressed(child_ui, (uint64_t)child_pressed_token.value) != S_OK) return 76;
	if (middle_ui->lpVtbl->remove_PointerPressed(middle_ui, (uint64_t)middle_pressed_token.value) != S_OK) return 77;
	if (outer_ui->lpVtbl->remove_PointerPressed(outer_ui, (uint64_t)outer_pressed_token.value) != S_OK) return 78;
	if (child_ui->lpVtbl->remove_PointerReleased(child_ui, (uint64_t)child_released_token.value) != S_OK) return 79;
	if (middle_ui->lpVtbl->remove_PointerReleased(middle_ui, (uint64_t)middle_released_token.value) != S_OK) return 80;
	if (outer_ui->lpVtbl->remove_PointerReleased(outer_ui, (uint64_t)outer_released_token.value) != S_OK) return 81;
	if (child_ui->lpVtbl->remove_PointerWheelChanged(child_ui, (uint64_t)child_wheel_token.value) != S_OK) return 82;
	if (middle_ui->lpVtbl->remove_PointerWheelChanged(middle_ui, (uint64_t)middle_wheel_token.value) != S_OK) return 83;
	if (outer_ui->lpVtbl->remove_PointerWheelChanged(outer_ui, (uint64_t)outer_wheel_token.value) != S_OK) return 84;
	if (button_base->lpVtbl->remove_Click(button_base, (uint64_t)click_token.value) != S_OK) return 85;
	SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(500, 400));
	POINT late_wheel = {500, 400};
	ClientToScreen(hwnd, &late_wheel);
	SendMessageW(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(0, 120), MAKELPARAM(late_wheel.x, late_wheel.y));
	if (entered_count != 2 || failed_count != 2 || middle_entered_count != 2 || exited_count != 1 || core_moves != 5 ||
		child_pressed_count != 2 || middle_pressed_count != 2 || outer_pressed_count != 2 ||
		child_released_count != 3 || middle_released_count != 2 || outer_released_count != 2 ||
		child_wheel_count != 1 || middle_wheel_count != 1 || outer_wheel_count != 1 || click_count != 1) return 86;

	/* Closing the window unsubscribes the routing; the tree below survives the close intact. */
	if (ICoreWindow_Close(window) != S_OK) return 87;

	button_base->lpVtbl->Release(button_base);
	if (failing_entered.refs != 1 || counting_entered.refs != 1 || middle_entered.refs != 1 || counting_exited.refs != 1 ||
		child_pressed.refs != 1 || middle_pressed.refs != 1 || outer_pressed.refs != 1 ||
		child_released.refs != 1 || middle_released.refs != 1 || outer_released.refs != 1 ||
		child_wheel.refs != 1 || middle_wheel.refs != 1 || outer_wheel.refs != 1 ||
		click_handler.refs != 1 || core_handler.refs != 1) return 88;
	IUnknown_Release(button_identity);
	button_ui->lpVtbl->Release(button_ui);
	child_ui->lpVtbl->Release(child_ui);
	middle_ui->lpVtbl->Release(middle_ui);
	outer_ui->lpVtbl->Release(outer_ui);
	button_object->lpVtbl->Release(button_object);
	child_object->lpVtbl->Release(child_object);
	middle_object->lpVtbl->Release(middle_object);
	outer_object->lpVtbl->Release(outer_object);
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
