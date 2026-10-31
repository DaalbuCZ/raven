/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The style fixture: activates Windows.UI.Xaml.Style through the runtime catalog, adds setters carrying colour-name
 * strings and boxed numbers through the engine-internal helper, and verifies that putting a style on a hosted element
 * APPLIES the setters to the element's own storage — a Background "Red" setter paints a hosted grid red through the same
 * named-colour path the compiled markup uses, a FontSize setter lands in the text block's real font size (read back
 * through the public getter), and the pixel follows the setter the way it follows a direct put. The application is
 * atomic: a wrong-kind setter value (a number where a colour is wanted) fails put_Style with E_INVALIDARG and the element
 * keeps its previous state. put_Style(NULL) clears the reference but the engine re-applies on put and does not revert, so
 * the applied values stay. BasedOn applies the base chain's setters before the style's own, so a derived style overriding
 * only FontSize over a base that sets Background takes both, the derived FontSize winning. Read back through the same
 * emulated GDI surface the rendering fixture uses; the text re-measure asserts bounds instead of pixels, the way the text
 * fixture does, because the face the host font service picks decides the exact extent. A second grid without the style
 * keeps its own background, the style round-trips through a ResourceDictionary under a string key and still applies,
 * TargetType round-trips the interop TypeName and the Setters Size reflects the stored pairs. Freestanding C like the
 * other guests: hand-rolled COM, no CRT beyond the freestanding one the build links. */
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
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock textblock_i;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositor compositor_i;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush brush_i;
typedef __x_ABI_CWindows_CUI_CColor color_s;
typedef __FIMap_2_IInspectable_IInspectable map_i;
typedef __FIReference_1_HSTRING boxed_string_i;
typedef __FIReference_1_HSTRINGVtbl boxed_string_i_vtbl;
typedef __FIReference_1_DOUBLE boxed_number_i;
typedef __FIReference_1_DOUBLEVtbl boxed_number_i_vtbl;
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

/* A boxed string: what a WinRT caller passes as a dictionary key or a colour-name setter value. The inspectable part
 * carries the identity, the box part answers IReference<HSTRING>, and QI reaches one from the other as COM identity
 * requires — the same object the resource-dictionary fixture builds. */
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

/* A boxed number: what a WinRT caller passes for the numeric setter values (FontSize, Width, Height). The same shape as
 * the string box over IReference<double>. */
struct fixture_number;
struct fixture_number_part {
	const boxed_number_i_vtbl *lpVtbl;
	struct fixture_number *owner;
};
struct fixture_number {
	const IInspectableVtbl *lpVtbl;
	struct fixture_number_part box;
	LONG refs;
	double value;
};

static ULONG fixture_number_addref(struct fixture_number *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG fixture_number_release(struct fixture_number *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs)
		HeapFree(GetProcessHeap(), 0, self);
	return (ULONG)refs;
}

static HRESULT WINAPI number_query(IInspectable *iface, REFIID iid, void **object)
{
	struct fixture_number *self = (struct fixture_number *)iface;
	if (!object)
		return E_POINTER;
	*object = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable)) {
		*object = iface;
	} else if (IsEqualIID(iid, &IID___FIReference_1_DOUBLE)) {
		*object = &self->box;
	} else
		return E_NOINTERFACE;
	fixture_number_addref(self);
	return S_OK;
}

static HRESULT WINAPI number_part_query(boxed_number_i *iface, REFIID iid, void **object)
{
	struct fixture_number *self = ((struct fixture_number_part *)iface)->owner;
	if (!object)
		return E_POINTER;
	*object = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable)) {
		*object = self;
	} else if (IsEqualIID(iid, &IID___FIReference_1_DOUBLE)) {
		*object = iface;
	} else
		return E_NOINTERFACE;
	fixture_number_addref(self);
	return S_OK;
}

static ULONG WINAPI number_addref(IInspectable *iface) { return fixture_number_addref((struct fixture_number *)iface); }
static ULONG WINAPI number_release(IInspectable *iface) { return fixture_number_release((struct fixture_number *)iface); }
static ULONG WINAPI number_part_addref(boxed_number_i *iface)
{
	return fixture_number_addref(((struct fixture_number_part *)iface)->owner);
}
static ULONG WINAPI number_part_release(boxed_number_i *iface)
{
	return fixture_number_release(((struct fixture_number_part *)iface)->owner);
}

static HRESULT WINAPI number_iids(IInspectable *iface, ULONG *count, IID **iids)
{ (void)iface; if (!count || !iids) return E_POINTER; *count = 0; *iids = NULL; return S_OK; }
static HRESULT WINAPI number_part_iids(boxed_number_i *iface, ULONG *count, IID **iids)
{ (void)iface; if (!count || !iids) return E_POINTER; *count = 0; *iids = NULL; return S_OK; }
static HRESULT WINAPI number_trust(IInspectable *iface, TrustLevel *level)
{ (void)iface; if (!level) return E_POINTER; *level = BaseTrust; return S_OK; }
static HRESULT WINAPI number_part_trust(boxed_number_i *iface, TrustLevel *level)
{ (void)iface; if (!level) return E_POINTER; *level = BaseTrust; return S_OK; }

static HRESULT WINAPI number_name(IInspectable *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Raven.Fixture.BoxedNumber";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, ARRAYSIZE(text) - 1, name);
}

static HRESULT WINAPI number_part_name(boxed_number_i *iface, HSTRING *name)
{
	return number_name((IInspectable *)((struct fixture_number_part *)iface)->owner, name);
}

static HRESULT WINAPI number_part_value(boxed_number_i *iface, double *value)
{
	struct fixture_number *self = ((struct fixture_number_part *)iface)->owner;
	if (!value)
		return E_POINTER;
	*value = self->value;
	return S_OK;
}

static const IInspectableVtbl number_inspectable_table = {
	number_query, number_addref, number_release, number_iids, number_name, number_trust,
};

static const boxed_number_i_vtbl number_part_table = {
	number_part_query, number_part_addref, number_part_release, number_part_iids, number_part_name, number_part_trust,
	number_part_value,
};

static struct fixture_number *make_number(double value)
{
	struct fixture_number *self = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *self);
	if (!self)
		return NULL;
	self->lpVtbl = &number_inspectable_table;
	self->box.lpVtbl = &number_part_table;
	self->box.owner = self;
	self->refs = 1;
	self->value = value;
	return self;
}

/* The parameterized IVector`1<UIElement> a panel's Children hands out: the GUID is the derivation the element layer
 * documents (SHA-1 of the signature over the parameterized-type namespace), the vtable the twelve IVector`1 slots after
 * the IInspectable ones. The engine's children vector answers this IID, so the fixture can append a child the way the
 * loader does. */
DEFINE_GUID(IID___FIVector_1_UIElement, 0x393d6210, 0xfa21, 0x573a, 0xa2, 0x38, 0xf0, 0xe8, 0x73, 0x3b, 0x3c, 0x94);

typedef struct fixture_children_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(void *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(void *This);
	ULONG (STDMETHODCALLTYPE *Release)(void *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(void *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(void *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(void *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetAt)(void *This, UINT32 index, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(void *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *GetView)(void *This, void **value);
	HRESULT (STDMETHODCALLTYPE *IndexOf)(void *This, IInspectable *element, UINT32 *index, boolean *found);
	HRESULT (STDMETHODCALLTYPE *SetAt)(void *This, UINT32 index, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *InsertAt)(void *This, UINT32 index, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *RemoveAt)(void *This, UINT32 index);
	HRESULT (STDMETHODCALLTYPE *Append)(void *This, IInspectable *value);
} fixture_children_vtbl;

/* The object the QI hands back: the vtable sits behind its lpVtbl, the way every COM interface here does. */
struct fixture_children {
	const fixture_children_vtbl *lpVtbl;
};

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

static int channel_delta(int left, int right)
{
	return left > right ? left - right : right - left;
}

/* The glyph pixels are deterministic the way the text fixture describes: the CPU fallback rasterizes monochrome coverage,
 * so a foreground leaves pixels of exactly its colour where the strokes are. A pixel counts as the foreground when every
 * channel stays within 55 of it, which the blue background never does. */
static int near_color(COLORREF pixel, COLORREF color)
{
	return channel_delta(GetRValue(pixel), GetRValue(color)) <= 55 &&
		channel_delta(GetGValue(pixel), GetGValue(color)) <= 55 &&
		channel_delta(GetBValue(pixel), GetBValue(color)) <= 55;
}

static UINT count_foreground(HWND hwnd, int x, int y, int width, int height, COLORREF color)
{
	HDC dc = GetDC(hwnd);
	UINT found = 0;
	if (!dc)
		return 0;
	for (int row = 0; row < height; ++row)
		for (int column = 0; column < width; ++column)
			if (near_color(GetPixel(dc, x + column, y + row), color))
				++found;
	ReleaseDC(hwnd, dc);
	return found;
}

/* One runtime object per check; the codes are unique so a failure names its step. The styled grid takes turns as the
 * hosted root, the way the rendering fixture swaps its content: it paints nothing (its own background is unset), red
 * under the first style, still red after put_Style(NULL) — the engine re-applies on put and does not revert — red again
 * through the dictionary round trip, blue under the replacement style (whose FontSize setter lands on the text block
 * child instead), red once more through the BasedOn chain, and it stays red through the wrong-kind rejections. The plain
 * grid paints its own blue before any style exists and again at the end. */
int raven_entry(void)
{
	static const WCHAR style_name[] = L"Windows.UI.Xaml.Style";
	static const WCHAR grid_name[] = L"Windows.UI.Xaml.Controls.Grid";
	static const WCHAR textblock_name[] = L"Windows.UI.Xaml.Controls.TextBlock";
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
	IInspectable *style2 = NULL, *textblock = NULL, *base_style = NULL, *derived_style = NULL;
	IInspectable *bad_style = NULL, *bad_style2 = NULL, *skip_style = NULL, *roundtrip = NULL;
	style_i *style_iface = NULL, *derived_iface = NULL;
	textblock_i *text_iface = NULL;
	panel_i *plain_panel = NULL, *styled_panel = NULL;
	compositor_i *compositor = NULL;
	brush_i *brush = NULL;
	map_i *map = NULL;
	IInspectable *dictionary = NULL;
	void *children = NULL;
	struct fixture_children *child_vector = NULL;
	HMODULE component = NULL;
	add_setter_function add_setter = NULL;
	put_style_function put_style = NULL;
	get_style_function get_style = NULL;
	struct fixture_box *red = NULL, *key = NULL, *probe = NULL, *blue = NULL, *red2 = NULL, *blue2 = NULL;
	struct fixture_number *size32 = NULL, *size14 = NULL, *seven = NULL, *three = NULL;
	UINT count14 = 0, count32 = 0;
	float font_size = 0;
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
	/* putting the style on the hosted grid applies the setter to the grid's own storage and re-renders it red */
	if (put_style(grid_styled, style) != S_OK)
		return 37;
	{
		if (get_style(grid_styled, &roundtrip) != S_OK || !roundtrip)
			return 38;
		if (!same_unknown(roundtrip, style))
			return 39;
		roundtrip->lpVtbl->Release(roundtrip);
		roundtrip = NULL;
	}
	if (color_at(hwnd, 5, 5, RGB(255, 0, 0)))
		return 40;
	if (color_at(hwnd, 300, 200, RGB(255, 0, 0)))
		return 41;
	/* put_Style(NULL) clears the reference; the engine re-applies on put and does not revert, so the applied red stays */
	if (put_style(grid_styled, NULL) != S_OK)
		return 42;
	if (color_at(hwnd, 5, 5, RGB(255, 0, 0)))
		return 43;
	if (get_style(grid_styled, &roundtrip) != S_OK || roundtrip)
		return 44;
	/* the style round-trips through a ResourceDictionary under a string key and still applies */
	if (activate(dictionary_name, &dictionary) != S_OK || !dictionary)
		return 45;
	if (dictionary->lpVtbl->QueryInterface(dictionary, &IID___FIMap_2_IInspectable_IInspectable, (void **)&map) != S_OK ||
		!map)
		return 46;
	if (!(key = make_box(L"style.key")) || !(probe = make_box(L"style.key")))
		return 47;
	{
		boolean replaced = TRUE;
		if (map->lpVtbl->Insert(map, (IInspectable *)key, style, &replaced) != S_OK || replaced)
			return 48;
	}
	if (map->lpVtbl->Lookup(map, (IInspectable *)probe, &found) != S_OK || !found)
		return 49;
	if (!same_unknown(found, style))
		return 50;
	/* the looked-up style applies to the element the same way */
	if (put_style(grid_styled, found) != S_OK)
		return 51;
	if (color_at(hwnd, 5, 5, RGB(255, 0, 0)))
		return 52;
	/* the replacement style carries Background "Blue" AND FontSize 32: the grid takes the Background, and once a text
	 * block child exists the same style's FontSize reaches the block's real font size */
	if (activate(style_name, &style2) != S_OK || !style2)
		return 53;
	if (!(blue = make_box(L"Blue")))
		return 54;
	if (add_setter(style2, L"Background", (UINT32)lstrlenW(L"Background"), (IInspectable *)blue) != S_OK)
		return 55;
	if (!(size32 = make_number(32.0)))
		return 56;
	if (add_setter(style2, L"FontSize", (UINT32)lstrlenW(L"FontSize"), (IInspectable *)size32) != S_OK)
		return 57;
	/* the text block child: its own Text through the real slot, appended to the hosted grid the way the loader does */
	if (activate(textblock_name, &textblock) != S_OK || !textblock)
		return 58;
	if (textblock->lpVtbl->QueryInterface(textblock, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
		(void **)&text_iface) != S_OK || !text_iface)
		return 59;
	{
		HSTRING text_value = NULL;
		static const WCHAR text_w[] = L"Hello";
		if (WindowsCreateString(text_w, ARRAYSIZE(text_w) - 1, &text_value) != S_OK)
			return 60;
		if (text_iface->lpVtbl->put_Text(text_iface, (uint64_t)(uintptr_t)text_value) != S_OK) {
			WindowsDeleteString(text_value);
			return 60;
		}
		WindowsDeleteString(text_value);
	}
	if (grid_styled->lpVtbl->QueryInterface(grid_styled, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
		(void **)&styled_panel) != S_OK || !styled_panel)
		return 61;
	if (styled_panel->lpVtbl->get_Children(styled_panel, (uint64_t)(uintptr_t)&children) != S_OK || !children)
		return 62;
	if (((IInspectable *)children)->lpVtbl->QueryInterface((IInspectable *)children, &IID___FIVector_1_UIElement,
		(void **)&child_vector) != S_OK || !child_vector)
		return 63;
	if (child_vector->lpVtbl->Append(child_vector, textblock) != S_OK)
		return 64;
	/* the get_Children reference is dropped here; the QI'd vector's own reference releases at the cleanup */
	((IInspectable *)children)->lpVtbl->Release((IInspectable *)children);
	children = NULL;
	/* the replacement style on the grid: Background blue (the FontSize setter has no grid storage and is skipped) */
	if (put_style(grid_styled, style2) != S_OK)
		return 65;
	/* the pixel checks stay clear of the text block's run at the grid origin */
	if (color_at(hwnd, 300, 200, RGB(0, 0, 255)))
		return 66;
	if (color_at(hwnd, 600, 500, RGB(0, 0, 255)))
		return 67;
	/* the child's text renders at its default size over the blue */
	count14 = count_foreground(hwnd, 0, 0, 220, 40, RGB(0, 0, 0));
	if (!count14)
		return 68;
	/* the same style on the text block: FontSize 32 lands in the real font size, and the re-render re-measures the run */
	if (put_style(textblock, style2) != S_OK)
		return 69;
	if (text_iface->lpVtbl->get_FontSize(text_iface, (uint64_t)(uintptr_t)&font_size) != S_OK || font_size != 32.0f)
		return 70;
	/* bounded, the way the text fixture asserts its extent: the face the host font service picks decides the exact run,
	 * so the re-measure is a growth assertion, not a pixel */
	count32 = count_foreground(hwnd, 0, 0, 220, 40, RGB(0, 0, 0));
	if (count32 <= count14)
		return 71;
	/* BasedOn: a base that sets Background "Red" and FontSize 14, a derived style overriding only the FontSize with 32.
	 * Both apply — the base's Background reaches the grid through the chain — and the derived FontSize wins. */
	if (activate(style_name, &base_style) != S_OK || !base_style)
		return 72;
	if (!(red2 = make_box(L"Red")))
		return 73;
	if (add_setter(base_style, L"Background", (UINT32)lstrlenW(L"Background"), (IInspectable *)red2) != S_OK)
		return 74;
	if (!(size14 = make_number(14.0)))
		return 75;
	if (add_setter(base_style, L"FontSize", (UINT32)lstrlenW(L"FontSize"), (IInspectable *)size14) != S_OK)
		return 76;
	if (activate(style_name, &derived_style) != S_OK || !derived_style)
		return 77;
	if (derived_style->lpVtbl->QueryInterface(derived_style, &IID__x_ABI_CWindows_CUI_CXaml_CIStyle,
		(void **)&derived_iface) != S_OK || !derived_iface)
		return 78;
	if (derived_iface->lpVtbl->put_BasedOn(derived_iface, (uint64_t)(uintptr_t)base_style) != S_OK)
		return 79;
	if (add_setter(derived_style, L"FontSize", (UINT32)lstrlenW(L"FontSize"), (IInspectable *)size32) != S_OK)
		return 80;
	/* the derived style on the grid: the base's Background applies through the chain */
	if (put_style(grid_styled, derived_style) != S_OK)
		return 81;
	if (color_at(hwnd, 300, 200, RGB(255, 0, 0)))
		return 82;
	/* the derived style on the text block: the base's 14 applies first, the derived 32 lands after and wins */
	if (put_style(textblock, derived_style) != S_OK)
		return 83;
	if (text_iface->lpVtbl->get_FontSize(text_iface, (uint64_t)(uintptr_t)&font_size) != S_OK || font_size != 32.0f)
		return 84;
	/* a wrong-kind value fails the application honestly: a NUMBER where a colour is wanted makes put_Style answer
	 * E_INVALIDARG, and nothing lands — the pixel keeps the style it had and get_Style still answers it */
	if (activate(style_name, &bad_style) != S_OK || !bad_style)
		return 85;
	if (!(seven = make_number(7.0)))
		return 86;
	if (add_setter(bad_style, L"Background", (UINT32)lstrlenW(L"Background"), (IInspectable *)seven) != S_OK)
		return 87;
	if (put_style(grid_styled, bad_style) != E_INVALIDARG)
		return 88;
	if (color_at(hwnd, 300, 200, RGB(255, 0, 0)))
		return 89;
	if (get_style(grid_styled, &roundtrip) != S_OK || !roundtrip)
		return 90;
	if (!same_unknown(roundtrip, derived_style))
		return 91;
	roundtrip->lpVtbl->Release(roundtrip);
	roundtrip = NULL;
	/* the mirror case: a STRING where a number is wanted fails the text block's put the same way */
	if (activate(style_name, &bad_style2) != S_OK || !bad_style2)
		return 92;
	if (!(blue2 = make_box(L"Blue")))
		return 93;
	if (add_setter(bad_style2, L"FontSize", (UINT32)lstrlenW(L"FontSize"), (IInspectable *)blue2) != S_OK)
		return 94;
	if (put_style(textblock, bad_style2) != E_INVALIDARG)
		return 95;
	if (text_iface->lpVtbl->get_FontSize(text_iface, (uint64_t)(uintptr_t)&font_size) != S_OK || font_size != 32.0f)
		return 96;
	/* a property the element has no storage for is skipped, not failed: a style of only such setters applies with no
	 * effect on the pixels */
	if (activate(style_name, &skip_style) != S_OK || !skip_style)
		return 97;
	if (!(three = make_number(3.0)))
		return 98;
	if (add_setter(skip_style, L"Opacity", (UINT32)lstrlenW(L"Opacity"), (IInspectable *)three) != S_OK)
		return 99;
	if (put_style(grid_styled, skip_style) != S_OK)
		return 100;
	if (color_at(hwnd, 300, 200, RGB(255, 0, 0)))
		return 101;
	/* the plain grid kept its own background the whole time: hosting it again paints blue, unstyled */
	if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)grid_plain) != S_OK)
		return 102;
	if (color_at(hwnd, 5, 5, RGB(0, 0, 255)))
		return 103;
	if (color_at(hwnd, 300, 200, RGB(0, 0, 255)))
		return 104;
	/* release everything: the window's own reference goes with Close, and clearing the content detaches the hosted tree
	 * from the render and input roots. The component cannot answer DllCanUnloadNow with S_OK after a render — the
	 * composition target keeps the presented visual tree until the process ends, which is why the rendering fixture never
	 * checks it — so unload stays untested here and every style object above releases through the refs it owns. */
	if (child_vector)
		((IInspectable *)child_vector)->lpVtbl->Release((IInspectable *)child_vector);
	if (xaml_window->lpVtbl->put_Content(xaml_window, 0) != S_OK)
		return 105;
	xaml_window->lpVtbl->Close(xaml_window);
	if (text_iface)
		text_iface->lpVtbl->Release(text_iface);
	if (styled_panel)
		styled_panel->lpVtbl->Release(styled_panel);
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
	fixture_box_release(blue);
	fixture_box_release(red2);
	fixture_box_release(blue2);
	fixture_number_release(size32);
	fixture_number_release(size14);
	fixture_number_release(seven);
	fixture_number_release(three);
	if (derived_iface)
		derived_iface->lpVtbl->Release(derived_iface);
	if (style_iface)
		style_iface->lpVtbl->Release(style_iface);
	if (style)
		style->lpVtbl->Release(style);
	if (style2)
		style2->lpVtbl->Release(style2);
	if (base_style)
		base_style->lpVtbl->Release(base_style);
	if (derived_style)
		derived_style->lpVtbl->Release(derived_style);
	if (bad_style)
		bad_style->lpVtbl->Release(bad_style);
	if (bad_style2)
		bad_style2->lpVtbl->Release(bad_style2);
	if (skip_style)
		skip_style->lpVtbl->Release(skip_style);
	if (found)
		found->lpVtbl->Release(found);
	if (textblock)
		textblock->lpVtbl->Release(textblock);
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
