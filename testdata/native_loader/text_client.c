/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The text fixture: a compiled page hosts a blue grid whose two fixed columns carry a TextBlock with text in the first and a
 * textless one in the second, the XAML window presents it through the composition stage, and the client verifies the pixels
 * through the same emulated GDI surface. The fallback clears the window to black, so the block carries an explicit white
 * Foreground over the blue background — the XAML default foreground is black and would be invisible here. Covers the text
 * painting, the Text and Foreground mutations through the real slots, the FontSize round-trip, and that a block without text
 * paints nothing. The block does not influence its own size yet — the measure stage will — so the definitions place it. */
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_UI_Composition
#define WIDL_using_Windows_Foundation
#include <windows.h>
#include <winstring.h>
#include <string.h>
#include <activation.h>
#include <roapi.h>
#include <windows.ui.core.h>
#include <windows.ui.composition.h>
#include <windows.foundation.h>
#include "../../native/upstream/raven-xaml/corewindow_interop.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_controls.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_elements.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_layout.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_window.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);
typedef HRESULT (WINAPI *load_function)(const void *, uint32_t, IInspectable **);

typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPanel panel1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIGrid grid1;

/* IVector`1<ColumnDefinition>, from the parameterized-type rule over the two winmd GUIDs (see docs/uwp-xaml.md). */
DEFINE_GUID(IID___FIVector_1_ColumnDefinition, 0xcaf7c66c, 0x1ccb, 0x5ada, 0xaf, 0xce, 0xfc, 0x62, 0xd2, 0xdb, 0x2c, 0x66);
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock textblock1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition column_definition1;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindow window1;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindowStatics window_statics;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositor ICompositor;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush IColorBrush;
typedef __x_ABI_CWindows_CUI_CColor Color;

/* The IVector<UIElement> the panel's Children and the IGrid's ColumnDefinitions return: the parameterized ABI has no mingw
 * header, so the slots the fixture reads are declared locally in metadata order after the six IInspectable methods. */
struct fixture_vector_vtbl {
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

struct fixture_vector {
	const struct fixture_vector_vtbl *lpVtbl;
};

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
	UINT32 bits = 0;
	const unsigned char *bytes = (const unsigned char *)&value;
	for (int index = 0; index < 4; ++index)
		bits |= (UINT32)bytes[index] << (index * 8);
	put32(doc, at, bits);
	return at + 4;
}

/* A page whose content is a blue grid with two fixed 200-pixel columns: a TextBlock with Text, FontSize and an explicit white
 * Foreground in the first, and a textless TextBlock with the same foreground in the second. */
static size_t build_text_document(unsigned char *doc)
{
	size_t at = begin_document(doc);
	at = create_type(doc, at, 525);                       /* Page */
	at = set_property(doc, at, 999);                      /* Content */
	at = create_type(doc, at, 442);                       /* blue grid */
	at = set_string(doc, at, 647, "Blue");                /* Background */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 396);                       /* text block with text */
	at = set_float(doc, at, 784, 24.0f);                  /* FontSize */
	at = set_string(doc, at, 788, "White");               /* Foreground */
	at = set_string(doc, at, 800, "Hello");               /* Text */
	doc[at++] = 0x02;                                     /* pop the text block */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 396);                       /* textless text block */
	at = set_float(doc, at, 857, 1.0f);                   /* Grid.Column */
	at = set_string(doc, at, 788, "White");               /* Foreground */
	doc[at++] = 0x02;                                     /* pop the text block */
	doc[at++] = 0x02;                                     /* pop the blue grid */
	const UINT32 nodes = (UINT32)(at - 168);
	put32(doc, 8, nodes);
	put32(doc, 164, nodes);
	return at;
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

/* The glyph pixels are deterministic: the CPU fallback rasterizes monochrome coverage, so a foreground leaves pixels of
 * exactly its colour where the strokes are. A pixel counts as the foreground when every channel stays within 55 of it, which
 * the background and the black clear colour never do. */
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

static UINT count_miscolored(HWND hwnd, int x, int y, int width, int height, COLORREF expected)
{
	HDC dc = GetDC(hwnd);
	UINT found = 0;
	if (!dc)
		return 0;
	for (int row = 0; row < height; ++row)
		for (int column = 0; column < width; ++column)
			if (GetPixel(dc, x + column, y + row) != expected)
				++found;
	ReleaseDC(hwnd, dc);
	return found;
}

/* The generated ABI carries a float as its IEEE-754 bit pattern in the low 32 bits of the slot word. */
static uint64_t word_of_float(float value)
{
	uint64_t word = 0;
	memcpy(&word, &value, sizeof value);
	return word;
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
	if (__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_GetForCurrentThread(statics, &window) != S_OK || !window) return 3;
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
	const size_t size = build_text_document(document);
	if (load(document, (uint32_t)size, &page) != S_OK || !page) return 7;

	/* The root grid is the page's content; the fixture gives it its two fixed columns through the IGrid collection. */
	IInspectable *root = NULL;
	{
		__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *content_control = NULL;
		if (page->lpVtbl->QueryInterface(page, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
			(void **)&content_control) != S_OK || !content_control) return 8;
		content_control->lpVtbl->get_Content(content_control, (uint64_t)(uintptr_t)&root);
		content_control->lpVtbl->Release(content_control);
	}
	if (!root) return 9;
	grid1 *grid = NULL;
	if (root->lpVtbl->QueryInterface(root, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, (void **)&grid) != S_OK || !grid)
		return 10;
	void *column_collection = NULL;
	if (grid->lpVtbl->get_ColumnDefinitions(grid, (uint64_t)(uintptr_t)&column_collection) != S_OK || !column_collection)
		return 11;
	struct fixture_vector *columns = (struct fixture_vector *)column_collection;
	static const WCHAR column_name[] = L"Windows.UI.Xaml.Controls.ColumnDefinition";
	for (UINT32 index = 0; index < 2; ++index) {
		HSTRING key = NULL;
		IActivationFactory *activation = NULL;
		IInspectable *object = NULL;
		if (WindowsCreateString(column_name, ARRAYSIZE(column_name) - 1, &key) != S_OK) return 22;
		if (RoGetActivationFactory(key, &IID_IActivationFactory, (void **)&activation) != S_OK || !activation) return 23;
		if (activation->lpVtbl->ActivateInstance(activation, &object) != S_OK || !object) return 24;
		column_definition1 *definition = NULL;
		if (object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition,
			(void **)&definition) != S_OK || !definition) return 25;
		__x_ABI_CWindows_CUI_CXaml_CGridLength length = {200.0, GridUnitType_Pixel};
		if (definition->lpVtbl->put_Width(definition, length) != S_OK) return 26;
		if (columns->lpVtbl->Append(columns, object) != S_OK) return 27;
		definition->lpVtbl->Release(definition);
		object->lpVtbl->Release(object);
		activation->lpVtbl->Release(activation);
		WindowsDeleteString(key);
	}

	/* Hosting the page on the window renders it. */
	HSTRING window_key = NULL;
	static const WCHAR window_name[] = L"Windows.UI.Xaml.Window";
	if (WindowsCreateString(window_name, ARRAYSIZE(window_name) - 1, &window_key) != S_OK) return 28;
	IActivationFactory *window_activation = NULL;
	if (RoGetActivationFactory(window_key, &IID_IActivationFactory, (void **)&window_activation) != S_OK || !window_activation)
		return 29;
	window_statics *window_current = NULL;
	if (window_activation->lpVtbl->QueryInterface(window_activation, &IID__x_ABI_CWindows_CUI_CXaml_CIWindowStatics,
		(void **)&window_current) != S_OK || !window_current) return 30;
	window1 *xaml_window = NULL;
	if (window_current->lpVtbl->get_Current(window_current, (uint64_t)(uintptr_t)&xaml_window) != S_OK || !xaml_window) return 31;
	if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)page) != S_OK) return 32;

	/* The grid fills the window and the text block draws at the top-left of its cell: the glyphs land in the first
	 * column's corner, the second column stays background. */
	static const COLORREF background = RGB(0, 0, 255);
	static const COLORREF white = RGB(255, 255, 255);
	if (count_miscolored(hwnd, 150, 150, 8, 8, background)) return 33;
	if (count_miscolored(hwnd, 450, 300, 8, 8, background)) return 34;
	if (!count_foreground(hwnd, 0, 0, 60, 30, white)) return 35;
	if (count_miscolored(hwnd, 210, 0, 50, 30, background)) return 36;

	/* The first text block answers for its state: the loader's FontSize reached it and the round-trip works through the
	 * real slots. */
	panel1 *grid_panel = NULL;
	if (root->lpVtbl->QueryInterface(root, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel, (void **)&grid_panel) != S_OK ||
		!grid_panel) return 37;
	void *children_collection = NULL;
	if (grid_panel->lpVtbl->get_Children(grid_panel, (uint64_t)(uintptr_t)&children_collection) != S_OK ||
		!children_collection) return 38;
	struct fixture_vector *children = (struct fixture_vector *)children_collection;
	UINT32 child_count = 0;
	if (children->lpVtbl->get_Size(children, &child_count) != S_OK || child_count != 2) return 39;
	IInspectable *text_object = NULL;
	if (children->lpVtbl->GetAt(children, 0, &text_object) != S_OK || !text_object) return 40;
	textblock1 *textblock = NULL;
	if (text_object->lpVtbl->QueryInterface(text_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
		(void **)&textblock) != S_OK || !textblock) return 41;
	float font_size = 0;
	if (textblock->lpVtbl->get_FontSize(textblock, (uint64_t)(uintptr_t)&font_size) != S_OK || font_size != 24.0f) return 42;
	if (textblock->lpVtbl->put_FontSize(textblock, word_of_float(30.0f)) != S_OK) return 43;
	if (textblock->lpVtbl->get_FontSize(textblock, (uint64_t)(uintptr_t)&font_size) != S_OK || font_size != 30.0f) return 44;
	/* The larger glyphs re-render into the same corner. */
	if (!count_foreground(hwnd, 0, 0, 60, 30, white)) return 45;
	if (textblock->lpVtbl->put_FontSize(textblock, word_of_float(24.0f)) != S_OK) return 46;

	/* Changing Text through the real slot re-renders, and a block without text paints nothing. */
	HSTRING world = NULL;
	static const WCHAR world_text[] = L"World";
	if (WindowsCreateString(world_text, ARRAYSIZE(world_text) - 1, &world) != S_OK) return 47;
	if (textblock->lpVtbl->put_Text(textblock, (uint64_t)(uintptr_t)world) != S_OK) return 48;
	if (!count_foreground(hwnd, 0, 0, 60, 30, white)) return 49;
	if (textblock->lpVtbl->put_Text(textblock, 0) != S_OK) return 50;
	if (count_miscolored(hwnd, 0, 0, 60, 30, background)) return 51;
	HSTRING hello = NULL;
	static const WCHAR hello_text[] = L"Hello";
	if (WindowsCreateString(hello_text, ARRAYSIZE(hello_text) - 1, &hello) != S_OK) return 52;
	if (textblock->lpVtbl->put_Text(textblock, (uint64_t)(uintptr_t)hello) != S_OK) return 53;
	if (!count_foreground(hwnd, 0, 0, 60, 30, white)) return 54;
	WindowsDeleteString(world);
	WindowsDeleteString(hello);

	/* Foreground takes the composition colour brush the way Panel.Background does; the glyphs come back in the new colour. */
	HSTRING compositor_key = NULL;
	static const WCHAR compositor_name[] = L"Windows.UI.Composition.Compositor";
	if (WindowsCreateString(compositor_name, ARRAYSIZE(compositor_name) - 1, &compositor_key) != S_OK) return 55;
	IInspectable *compositor_object = NULL;
	if (RoActivateInstance(compositor_key, &compositor_object) != S_OK || !compositor_object) return 56;
	ICompositor *compositor = NULL;
	if (compositor_object->lpVtbl->QueryInterface(compositor_object,
		&IID___x_ABI_CWindows_CUI_CComposition_CICompositor, (void **)&compositor) != S_OK || !compositor) return 57;
	IColorBrush *brush = NULL;
	Color lime;
	lime.A = 255;
	lime.R = 0;
	lime.G = 255;
	lime.B = 0;
	if (compositor->lpVtbl->CreateColorBrushWithColor(compositor, lime, &brush) != S_OK || !brush) return 58;
	static const COLORREF lime_color = RGB(0, 255, 0);
	if (textblock->lpVtbl->put_Foreground(textblock, (uint64_t)(uintptr_t)brush) != S_OK) return 59;
	if (!count_foreground(hwnd, 0, 0, 60, 30, lime_color)) return 60;

	brush->lpVtbl->Release(brush);
	compositor->lpVtbl->Release(compositor);
	compositor_object->lpVtbl->Release(compositor_object);
	WindowsDeleteString(compositor_key);
	textblock->lpVtbl->Release(textblock);
	text_object->lpVtbl->Release(text_object);
	children->lpVtbl->Release(children);
	grid_panel->lpVtbl->Release(grid_panel);
	columns->lpVtbl->Release(columns);

	/* Measure and arrange: a second page whose grid carries an Auto column and a fixed one. The Auto column takes the
	 * measured run of the text block it holds, so the red column that follows starts at the text extent. The exact extent
	 * depends on the face the host font service picks, so the fixture asserts bounds instead of a pixel. */
	{
		static unsigned char extent_document[1024];
		IInspectable *extent_page = NULL;
		size_t at = begin_document(extent_document);
		at = create_type(extent_document, at, 525);           /* Page */
		at = set_property(extent_document, at, 999);          /* Content */
		at = create_type(extent_document, at, 442);           /* blue grid */
		at = set_string(extent_document, at, 647, "Blue");    /* Background */
		at = set_property(extent_document, at, 648);          /* Children */
		at = create_type(extent_document, at, 396);           /* the measured text block */
		at = set_float(extent_document, at, 784, 24.0f);      /* FontSize */
		at = set_string(extent_document, at, 788, "White");   /* Foreground */
		at = set_string(extent_document, at, 800, "Hello");   /* Text */
		extent_document[at++] = 0x02;                         /* pop the text block */
		at = set_property(extent_document, at, 648);          /* Children */
		at = create_type(extent_document, at, 442);           /* red grid, cell (0,1) */
		at = set_string(extent_document, at, 647, "Red");     /* Background */
		at = set_float(extent_document, at, 857, 1.0f);       /* Grid.Column */
		extent_document[at++] = 0x02;                         /* pop the red grid */
		extent_document[at++] = 0x02;                         /* pop the blue grid */
		const UINT32 extent_nodes = (UINT32)(at - 168);
		put32(extent_document, 8, extent_nodes);
		put32(extent_document, 164, extent_nodes);
		if (load(extent_document, (uint32_t)at, &extent_page) != S_OK || !extent_page) return 70;
		IInspectable *extent_grid_object = NULL;
		{
			__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *content_control = NULL;
			if (extent_page->lpVtbl->QueryInterface(extent_page, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
				(void **)&content_control) != S_OK || !content_control) return 71;
			content_control->lpVtbl->get_Content(content_control, (uint64_t)(uintptr_t)&extent_grid_object);
			content_control->lpVtbl->Release(content_control);
		}
		if (!extent_grid_object) return 72;
		__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid *extent_grid = NULL;
		if (extent_grid_object->lpVtbl->QueryInterface(extent_grid_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid,
			(void **)&extent_grid) != S_OK || !extent_grid) return 73;
		void *extent_column_collection = NULL;
		if (extent_grid->lpVtbl->get_ColumnDefinitions(extent_grid, (uint64_t)(uintptr_t)&extent_column_collection) != S_OK ||
			!extent_column_collection) return 74;
		struct fixture_vector *extent_columns = NULL;
		if (((IInspectable *)extent_column_collection)->lpVtbl->QueryInterface((IInspectable *)extent_column_collection,
			&IID___FIVector_1_ColumnDefinition, (void **)&extent_columns) != S_OK || !extent_columns) return 75;
		HSTRING column_key = NULL;
		static const WCHAR column_name[] = L"Windows.UI.Xaml.Controls.ColumnDefinition";
		if (WindowsCreateString(column_name, ARRAYSIZE(column_name) - 1, &column_key) != S_OK) return 76;
		IActivationFactory *definition_activation = NULL;
		if (RoGetActivationFactory(column_key, &IID_IActivationFactory, (void **)&definition_activation) != S_OK ||
			!definition_activation) return 77;
		for (UINT32 index = 0; index < 2; ++index) {
			IInspectable *object = NULL;
			if (definition_activation->lpVtbl->ActivateInstance(definition_activation, &object) != S_OK || !object) return 78;
			__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition *definition = NULL;
			if (object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition,
				(void **)&definition) != S_OK || !definition) return 79;
			__x_ABI_CWindows_CUI_CXaml_CGridLength length = {0, GridUnitType_Auto};
			if (index == 1)
				length = (__x_ABI_CWindows_CUI_CXaml_CGridLength){100.0, GridUnitType_Pixel};
			if (definition->lpVtbl->put_Width(definition, length) != S_OK) return 80;
			if (extent_columns->lpVtbl->Append(extent_columns, object) != S_OK) return 81;
			definition->lpVtbl->Release(definition);
			object->lpVtbl->Release(object);
		}
		definition_activation->lpVtbl->Release(definition_activation);
		WindowsDeleteString(column_key);
		if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)extent_page) != S_OK) return 82;
		/* The red column starts at the measured run: "Hello" at 24 DIP is wider than 20 and narrower than 250, the red
		 * column is 100 wide, and everything past it is the blue background. */
		HDC scan = GetDC(hwnd);
		int red_at = -1;
		for (int x = 0; x < 800 && red_at < 0; ++x) {
			const COLORREF pixel = scan ? GetPixel(scan, x, 50) : 0;
			if ((pixel & 0xffffff) == RGB(255, 0, 0))
				red_at = x;
		}
		if (scan)
			ReleaseDC(hwnd, scan);
		if (red_at < 20 || red_at > 250) return 83;
		if (color_at(hwnd, red_at + 50, 50, RGB(255, 0, 0))) return 84;
		if (color_at(hwnd, red_at + 150, 50, RGB(0, 0, 255))) return 85;
		if (!count_foreground(hwnd, 0, 0, red_at < 60 ? red_at : 60, 30, white)) return 86;
		extent_columns->lpVtbl->Release(extent_columns);
		((IInspectable *)extent_column_collection)->lpVtbl->Release((IInspectable *)extent_column_collection);
		definition_activation = NULL;
		extent_grid->lpVtbl->Release(extent_grid);
		extent_grid_object->lpVtbl->Release(extent_grid_object);
		extent_page->lpVtbl->Release(extent_page);
	}
	grid->lpVtbl->Release(grid);
	root->lpVtbl->Release(root);
	xaml_window->lpVtbl->Release(xaml_window);
	window_current->lpVtbl->Release(window_current);
	window_activation->lpVtbl->Release(window_activation);
	WindowsDeleteString(window_key);
	page->lpVtbl->Release(page);
	ICoreWindow_Release(window);
	__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_Release(statics);
	WindowsDeleteString(core_key);
	interop->lpVtbl->Release(interop);
	return 0;
}
