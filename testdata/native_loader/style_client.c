/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The style fixture: activates Windows.UI.Xaml.Style through the runtime catalog, adds a Background setter carrying a
 * colour-name string through the engine-internal helper, and verifies that putting the style on a hosted grid whose own
 * background is unset paints it red — read back through the same emulated GDI surface the rendering fixture uses. A second
 * grid without the style keeps its own background, put_Style(NULL) restores the unset state, the style round-trips through
 * a ResourceDictionary under a string key and still applies, TargetType round-trips the interop TypeName and the Setters
 * Size reflects the stored pair. Freestanding C like the other guests: hand-rolled COM, no CRT beyond the freestanding one
 * the build links. */
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <roapi.h>
#include <activation.h>
#include <windows.foundation.h>
#include <windows.ui.core.h>
#include <windows.ui.composition.h>
#include "../../native/upstream/raven-xaml/corewindow_interop.h"
#include "windows_ui_xaml_dictionary.h"
#include "windows_ui_xaml_elements.h"
#include "windows_ui_xaml_style.h"
#include "windows_ui_xaml_window.h"

typedef __x_ABI_CWindows_CUI_CXaml_CIStyle style_i;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindow window_i;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindowStatics window_statics_i;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPanel panel_i;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositor compositor_i;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush brush_i;
typedef __x_ABI_CWindows_CUI_CColor color_s;
typedef __FIMap_2_IInspectable_IInspectable map_i;
typedef __FIReference_1_HSTRING boxed_string_i;
typedef __FIReference_1_HSTRINGVtbl boxed_string_i_vtbl;
typedef __FIVector_1_Windows__CUI__CXaml__CSetterBase setter_vector_i;
typedef __x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName type_name_s;

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);

/* The engine-internal exports, reached through GetProcAddress the way the loader fixtures reach RavenXamlLoad; the
 * signatures follow xaml_styles.h. */
typedef HRESULT (*add_setter_function)(IInspectable *, const WCHAR *, UINT32, IInspectable *);
typedef HRESULT (*put_style_function)(IInspectable *, IInspectable *);
typedef HRESULT (*get_style_function)(IInspectable *, IInspectable **);

static int same_text(HSTRING value, const WCHAR *expected)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(value, &length);
	return length == lstrlenW(expected) && CompareStringOrdinal(text, (int)length, expected, -1, FALSE) == CSTR_EQUAL;
}

static int same_unknown(void *left, void *right)
{
	IUnknown *one = NULL, *other = NULL;
	const int same = left && right &&
		SUCCEEDED(((IUnknown *)left)->lpVtbl->QueryInterface((IUnknown *)left, &IID_IUnknown, (void **)&one)) && one &&
		SUCCEEDED(((IUnknown *)right)->lpVtbl->QueryInterface((IUnknown *)right, &IID_IUnknown, (void **)&other)) &&
		other && one == other;
	if (one)
		one->lpVtbl->Release(one);
	if (other)
		other->lpVtbl->Release(other);
	return same;
}

/* A boxed string: what a WinRT caller passes as a dictionary key or a setter value. The inspectable part carries the
 * identity, the box part answers IReference<HSTRING>, and QI reaches one from the other as COM identity requires — the
 * same object the resource-dictionary fixture builds. */
struct fixture_box;
struct fixture_box_part {
	const boxed_string_i_vtbl *lpVtbl;
	struct fixture_box *owner;
};
struct fixture_box {
	const IInspectableVtbl *lpVtbl;
	struct fixture_box_part box;
	LONG refs;
	HSTRING value;
};

static ULONG fixture_box_addref(struct fixture_box *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG fixture_box_release(struct fixture_box *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->value)
			WindowsDeleteString(self->value);
		HeapFree(GetProcessHeap(), 0, self);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI box_query(IInspectable *iface, REFIID iid, void **object)
{
	struct fixture_box *self = (struct fixture_box *)iface;
	if (!object)
		return E_POINTER;
	*object = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable)) {
		*object = iface;
	} else if (IsEqualIID(iid, &IID___FIReference_1_HSTRING)) {
		*object = &self->box;
	} else
		return E_NOINTERFACE;
	fixture_box_addref(self);
	return S_OK;
}

static HRESULT WINAPI box_part_query(boxed_string_i *iface, REFIID iid, void **object)
{
	struct fixture_box *self = ((struct fixture_box_part *)iface)->owner;
	if (!object)
		return E_POINTER;
	*object = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable)) {
		*object = self;
	} else if (IsEqualIID(iid, &IID___FIReference_1_HSTRING)) {
		*object = iface;
	} else
		return E_NOINTERFACE;
	fixture_box_addref(self);
	return S_OK;
}

static ULONG WINAPI box_addref(IInspectable *iface) { return fixture_box_addref((struct fixture_box *)iface); }
static ULONG WINAPI box_release(IInspectable *iface) { return fixture_box_release((struct fixture_box *)iface); }
static ULONG WINAPI box_part_addref(boxed_string_i *iface)
{
	return fixture_box_addref(((struct fixture_box_part *)iface)->owner);
}
static ULONG WINAPI box_part_release(boxed_string_i *iface)
{
	return fixture_box_release(((struct fixture_box_part *)iface)->owner);
}

static HRESULT WINAPI box_iids(IInspectable *iface, ULONG *count, IID **iids)
{ (void)iface; if (!count || !iids) return E_POINTER; *count = 0; *iids = NULL; return S_OK; }
static HRESULT WINAPI box_part_iids(boxed_string_i *iface, ULONG *count, IID **iids)
{ (void)iface; if (!count || !iids) return E_POINTER; *count = 0; *iids = NULL; return S_OK; }
static HRESULT WINAPI box_trust(IInspectable *iface, TrustLevel *level)
{ (void)iface; if (!level) return E_POINTER; *level = BaseTrust; return S_OK; }
static HRESULT WINAPI box_part_trust(boxed_string_i *iface, TrustLevel *level)
{ (void)iface; if (!level) return E_POINTER; *level = BaseTrust; return S_OK; }

static HRESULT WINAPI box_name(IInspectable *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Raven.Fixture.BoxedString";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, ARRAYSIZE(text) - 1, name);
}

static HRESULT WINAPI box_part_name(boxed_string_i *iface, HSTRING *name)
{
	return box_name((IInspectable *)((struct fixture_box_part *)iface)->owner, name);
}

static HRESULT WINAPI box_part_value(boxed_string_i *iface, HSTRING *value)
{
	struct fixture_box *self = ((struct fixture_box_part *)iface)->owner;
	if (!value)
		return E_POINTER;
	*value = NULL;
	/* the caller deletes what it receives, so the stored string is handed out duplicated */
	return WindowsDuplicateString(self->value, value);
}

static const IInspectableVtbl box_inspectable_table = {
	box_query, box_addref, box_release, box_iids, box_name, box_trust,
};

static const boxed_string_i_vtbl box_part_table = {
	box_part_query, box_part_addref, box_part_release, box_part_iids, box_part_name, box_part_trust,
	box_part_value,
};

static struct fixture_box *make_box(const WCHAR *text)
{
	struct fixture_box *self = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *self);
	if (!self)
		return NULL;
	self->lpVtbl = &box_inspectable_table;
	self->box.lpVtbl = &box_part_table;
	self->box.owner = self;
	self->refs = 1;
	if (WindowsCreateString(text, (UINT32)lstrlenW(text), &self->value) != S_OK) {
		HeapFree(GetProcessHeap(), 0, self);
		return NULL;
	}
	return self;
}

static HRESULT activate(const WCHAR *name, IInspectable **out)
{
	HSTRING key = NULL;
	IActivationFactory *factory = NULL;
	HRESULT result;
	if (WindowsCreateString(name, (UINT32)lstrlenW(name), &key) != S_OK)
		return E_FAIL;
	result = RoGetActivationFactory(key, &IID_IActivationFactory, (void **)&factory);
	WindowsDeleteString(key);
	if (FAILED(result))
		return result;
	result = factory->lpVtbl->ActivateInstance(factory, out);
	factory->lpVtbl->Release(factory);
	return result;
}

static int color_at(HWND hwnd, int x, int y, COLORREF expected)
{
	HDC dc = GetDC(hwnd);
	if (!dc)
		return 1;
	const COLORREF pixel = GetPixel(dc, x, y);
	ReleaseDC(hwnd, dc);
	return pixel == expected ? 0 : 1;
}

/* One runtime object per check; the codes are unique so a failure names its step. The two grids take turns as the hosted
 * root, the way the rendering fixture swaps its content: the styled grid first paints nothing (its own background is
 * unset), red under the style, nothing again after put_Style(NULL), red once more through the dictionary round trip; the
 * plain grid paints its own blue before any style exists and again at the end. */
int raven_entry(void)
{
	static const WCHAR style_name[] = L"Windows.UI.Xaml.Style";
	static const WCHAR grid_name[] = L"Windows.UI.Xaml.Controls.Grid";
	static const WCHAR dictionary_name[] = L"Windows.UI.Xaml.ResourceDictionary";
	static const WCHAR core_name[] = L"Windows.UI.Core.CoreWindow";
	static const WCHAR window_name[] = L"Windows.UI.Xaml.Window";
	static const WCHAR compositor_name[] = L"Windows.UI.Composition.Compositor";
	static const WCHAR grid_type[] = L"Windows.UI.Xaml.Controls.Grid";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function initialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function get_factory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	HSTRING core_key = NULL, window_key = NULL, compositor_key = NULL;
	__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic *core_statics = NULL;
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *core = NULL;
	raven_core_window_interop *interop = NULL;
	HWND hwnd = NULL;
	IActivationFactory *window_activation = NULL;
	window_statics_i *window_statics = NULL;
	window_i *xaml_window = NULL;
	IInspectable *grid_plain = NULL, *grid_styled = NULL, *style = NULL, *found = NULL, *compositor_object = NULL;
	style_i *style_iface = NULL;
	panel_i *plain_panel = NULL;
	compositor_i *compositor = NULL;
	brush_i *brush = NULL;
	map_i *map = NULL;
	IInspectable *dictionary = NULL;
	HMODULE component = NULL;
	add_setter_function add_setter = NULL;
	put_style_function put_style = NULL;
	get_style_function get_style = NULL;
	struct fixture_box *red = NULL, *key = NULL, *probe = NULL;
	if (!initialize || !get_factory)
		return 1;
	if (initialize(1) != S_OK)
		return 2;
	/* the interop window and its surface */
	if (WindowsCreateString(core_name, ARRAYSIZE(core_name) - 1, &core_key) != S_OK)
		return 3;
	if (get_factory(core_key, &IID___x_ABI_CWindows_CUI_CCore_CICoreWindowStatic, (void **)&core_statics) != S_OK ||
		!core_statics)
		return 4;
	if (__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_GetForCurrentThread(core_statics, &core) != S_OK || !core)
		return 5;
	if (core->lpVtbl->QueryInterface(core, &raven_core_window_interop_iid, (void **)&interop) != S_OK || !interop)
		return 6;
	if (interop->lpVtbl->get_WindowHandle(interop, &hwnd) != S_OK || !hwnd)
		return 7;
	core->lpVtbl->Activate(core);
	/* the XAML window the fixture hosts its grids on */
	if (WindowsCreateString(window_name, ARRAYSIZE(window_name) - 1, &window_key) != S_OK)
		return 8;
	if (get_factory(window_key, &IID_IActivationFactory, (void **)&window_activation) != S_OK || !window_activation)
		return 9;
	if (window_activation->lpVtbl->QueryInterface(window_activation, &IID__x_ABI_CWindows_CUI_CXaml_CIWindowStatics,
		(void **)&window_statics) != S_OK || !window_statics)
		return 10;
	if (window_statics->lpVtbl->get_Current(window_statics, (uint64_t)(uintptr_t)&xaml_window) != S_OK || !xaml_window)
		return 11;
	/* the engine-internal exports */
	component = LoadLibraryW(L"raven-xaml.dll");
	add_setter = component ? (add_setter_function)GetProcAddress(component, "raven_style_add_setter") : NULL;
	put_style = component ? (put_style_function)GetProcAddress(component, "raven_xaml_element_put_style") : NULL;
	get_style = component ? (get_style_function)GetProcAddress(component, "raven_xaml_element_get_style") : NULL;
	if (!add_setter || !put_style || !get_style)
		return 12;
	/* the plain grid: its own blue background through the real brush slot, set before any style exists */
	if (activate(grid_name, &grid_plain) != S_OK || !grid_plain)
		return 13;
	if (WindowsCreateString(compositor_name, ARRAYSIZE(compositor_name) - 1, &compositor_key) != S_OK)
		return 14;
	if (RoActivateInstance(compositor_key, &compositor_object) != S_OK || !compositor_object)
		return 15;
	if (compositor_object->lpVtbl->QueryInterface(compositor_object,
		&IID___x_ABI_CWindows_CUI_CComposition_CICompositor, (void **)&compositor) != S_OK || !compositor)
		return 16;
	{
		color_s blue;
		blue.A = 255;
		blue.R = 0;
		blue.G = 0;
		blue.B = 255;
		if (compositor->lpVtbl->CreateColorBrushWithColor(compositor, blue, &brush) != S_OK || !brush)
			return 17;
	}
	if (grid_plain->lpVtbl->QueryInterface(grid_plain, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
		(void **)&plain_panel) != S_OK || !plain_panel)
		return 18;
	if (plain_panel->lpVtbl->put_Background(plain_panel, (uint64_t)(uintptr_t)brush) != S_OK)
		return 19;
	if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)grid_plain) != S_OK)
		return 20;
	/* the unstyled grid paints its own background */
	if (color_at(hwnd, 5, 5, RGB(0, 0, 255)))
		return 21;
	if (color_at(hwnd, 300, 200, RGB(0, 0, 255)))
		return 22;
	/* the style: the fresh instance is not sealed and TargetType round-trips the interop TypeName */
	if (activate(style_name, &style) != S_OK || !style)
		return 23;
	if (style->lpVtbl->QueryInterface(style, &IID__x_ABI_CWindows_CUI_CXaml_CIStyle, (void **)&style_iface) != S_OK ||
		!style_iface)
		return 24;
	{
		type_name_s put, read;
		boolean sealed_flag = TRUE;
		if (WindowsCreateString(grid_type, ARRAYSIZE(grid_type) - 1, &put.Name) != S_OK)
			return 25;
		put.Kind = TypeKind_Metadata;
		read.Name = NULL;
		read.Kind = TypeKind_Primitive;
		if (style_iface->lpVtbl->put_TargetType(style_iface, put) != S_OK)
			return 26;
		if (style_iface->lpVtbl->get_TargetType(style_iface, &read) != S_OK || !read.Name)
			return 27;
		if (!same_text(read.Name, grid_type) || read.Kind != TypeKind_Metadata)
			return 28;
		WindowsDeleteString(read.Name);
		WindowsDeleteString(put.Name);
		if (style_iface->lpVtbl->get_IsSealed(style_iface, &sealed_flag) != S_OK || sealed_flag)
			return 29;
	}
	/* the Background setter carries a colour-name string the way the compiled markup spells one */
	if (!(red = make_box(L"Red")))
		return 30;
	if (add_setter(style, L"Background", (UINT32)lstrlenW(L"Background"), (IInspectable *)red) != S_OK)
		return 31;
	/* the Setters collection reflects the stored pair */
	{
		void *setters = NULL;
		setter_vector_i *vector = NULL;
		UINT32 size = 0;
		if (style_iface->lpVtbl->get_Setters(style_iface, (uint64_t)(uintptr_t)&setters) != S_OK || !setters)
			return 32;
		vector = (setter_vector_i *)setters;
		if (vector->lpVtbl->get_Size(vector, &size) != S_OK || size != 1)
			return 33;
		vector->lpVtbl->Release(vector);
	}
	/* the styled grid: its own background is unset, so hosting it first paints nothing */
	if (activate(grid_name, &grid_styled) != S_OK || !grid_styled)
		return 34;
	if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)grid_styled) != S_OK)
		return 35;
	if (color_at(hwnd, 5, 5, RGB(0, 0, 0)))
		return 36;
	/* putting the style on the hosted grid re-renders it red */
	if (put_style(grid_styled, style) != S_OK)
		return 37;
	{
		IInspectable *roundtrip = NULL;
		if (get_style(grid_styled, &roundtrip) != S_OK || !roundtrip)
			return 38;
		if (!same_unknown(roundtrip, style))
			return 39;
		roundtrip->lpVtbl->Release(roundtrip);
	}
	if (color_at(hwnd, 5, 5, RGB(255, 0, 0)))
		return 40;
	if (color_at(hwnd, 300, 200, RGB(255, 0, 0)))
		return 41;
	/* put_Style(NULL) restores the unset state */
	if (put_style(grid_styled, NULL) != S_OK)
		return 42;
	if (color_at(hwnd, 5, 5, RGB(0, 0, 0)))
		return 43;
	/* the style round-trips through a ResourceDictionary under a string key and still applies */
	if (activate(dictionary_name, &dictionary) != S_OK || !dictionary)
		return 44;
	if (dictionary->lpVtbl->QueryInterface(dictionary, &IID___FIMap_2_IInspectable_IInspectable, (void **)&map) != S_OK ||
		!map)
		return 45;
	if (!(key = make_box(L"style.key")) || !(probe = make_box(L"style.key")))
		return 46;
	{
		boolean replaced = TRUE;
		if (map->lpVtbl->Insert(map, (IInspectable *)key, style, &replaced) != S_OK || replaced)
			return 47;
	}
	if (map->lpVtbl->Lookup(map, (IInspectable *)probe, &found) != S_OK || !found)
		return 48;
	if (!same_unknown(found, style))
		return 49;
	/* the looked-up style applies to the element the same way */
	if (put_style(grid_styled, found) != S_OK)
		return 50;
	if (color_at(hwnd, 5, 5, RGB(255, 0, 0)))
		return 51;
	/* the plain grid kept its own background the whole time: hosting it again paints blue, unstyled */
	if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)grid_plain) != S_OK)
		return 52;
	if (color_at(hwnd, 5, 5, RGB(0, 0, 255)))
		return 53;
	if (color_at(hwnd, 300, 200, RGB(0, 0, 255)))
		return 54;
	/* release everything: the window's own reference goes with Close, and clearing the content detaches the hosted tree
	 * from the render and input roots. The component cannot answer DllCanUnloadNow with S_OK after a render — the
	 * composition target keeps the presented visual tree until the process ends, which is why the rendering fixture never
	 * checks it — so unload stays untested here and every style object above releases through the refs it owns. */
	found->lpVtbl->Release(found);
	found = NULL;
	if (put_style(grid_styled, NULL) != S_OK)
		return 55;
	if (xaml_window->lpVtbl->put_Content(xaml_window, 0) != S_OK)
		return 56;
	xaml_window->lpVtbl->Close(xaml_window);
	if (plain_panel)
		plain_panel->lpVtbl->Release(plain_panel);
	if (brush)
		brush->lpVtbl->Release(brush);
	if (compositor)
		compositor->lpVtbl->Release(compositor);
	if (compositor_object)
		compositor_object->lpVtbl->Release(compositor_object);
	if (map)
		map->lpVtbl->Release(map);
	if (dictionary)
		dictionary->lpVtbl->Release(dictionary);
	fixture_box_release(red);
	fixture_box_release(key);
	fixture_box_release(probe);
	if (style_iface)
		style_iface->lpVtbl->Release(style_iface);
	if (style)
		style->lpVtbl->Release(style);
	if (grid_styled)
		grid_styled->lpVtbl->Release(grid_styled);
	if (grid_plain)
		grid_plain->lpVtbl->Release(grid_plain);
	xaml_window->lpVtbl->Release(xaml_window);
	window_statics->lpVtbl->Release(window_statics);
	window_activation->lpVtbl->Release(window_activation);
	WindowsDeleteString(window_key);
	WindowsDeleteString(compositor_key);
	interop->lpVtbl->Release(interop);
	core->lpVtbl->Release(core);
	__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_Release(core_statics);
	WindowsDeleteString(core_key);
	return 0;
}
