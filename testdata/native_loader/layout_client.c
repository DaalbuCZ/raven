/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The layout fixture: a compiled page hosts a full-size blue grid whose column definitions are 100 and 200 and whose row
 * definitions are 100 and 100, and a red child grid placed in cell (1, 1) by the attached Grid.Row and Grid.Column of the
 * compiled document. The definitions themselves are created client-side through the public API — the classes are activated by
 * name, given their fixed lengths, and appended through the IGrid collections — and the client verifies the presented pixels
 * through the same emulated GDI surface. A definition widened after hosting re-presents the tree. A third page checks the
 * attached Grid.RowSpan and Grid.ColumnSpan: a child spanning tracks fills the merged region its own track opens, a
 * column-span-only child stays one row tall, and a child without slot properties keeps cell (0, 0). */
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_UI_Composition
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
#include "../../native/upstream/raven-xaml/windows_ui_xaml_layout.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_window.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);
typedef HRESULT (WINAPI *load_function)(const void *, uint32_t, IInspectable **);

typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIGrid grid1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition row_definition1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition column_definition1;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindow window1;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindowStatics window_statics;

/* The GUIDs of IVector`1 over the two definition classes, the SHA-1 of their signatures over the parameterized-type namespace,
 * validated the way the children vector was: against mingw's precomputed IVector<HSTRING> and the derived IVector<UIElement>. */
DEFINE_GUID(IID___FIVector_1_RowDefinition, 0x2a17524a, 0x48ef, 0x579b, 0x9e, 0x63, 0xe1, 0xce, 0x52, 0xad, 0xca, 0xe2);
DEFINE_GUID(IID___FIVector_1_ColumnDefinition, 0xcaf7c66c, 0x1ccb, 0x5ada, 0xaf, 0xce, 0xfc, 0x62, 0xd2, 0xdb, 0x2c, 0x66);

/* The IVector`1 ABI has no mingw header, so the slots the fixture drives are declared locally in metadata order after the six
 * IInspectable methods. */
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

/* A page whose content is a blue grid holding a red child grid placed in cell (1, 1). The constant nodes of the attached
 * properties are float constants because the loader's integer dispatch does not reach the element layer yet. */
static size_t build_layout_document(unsigned char *doc)
{
	size_t at = begin_document(doc);
	at = create_type(doc, at, 525);                       /* Page */
	at = set_property(doc, at, 999);                      /* Content */
	at = create_type(doc, at, 442);                       /* blue grid */
	at = set_string(doc, at, 647, "Blue");                /* Background */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 442);                       /* red cell grid */
	at = set_string(doc, at, 647, "Red");                 /* Background */
	at = set_float(doc, at, 860, 1.0f);                   /* Grid.Row */
	at = set_float(doc, at, 857, 1.0f);                   /* Grid.Column */
	doc[at++] = 0x02;                                     /* pop the red grid */
	doc[at++] = 0x02;                                     /* pop the blue grid */
	const UINT32 nodes = (UINT32)(at - 168);
	put32(doc, 8, nodes);
	put32(doc, 164, nodes);
	return at;
}

/* A single grid whose children carry the attached row/column properties; the definitions come from the client through the
 * public collection, so one document serves the star and the auto rounds with different definition sets. */
static size_t build_measure_document(unsigned char *doc)
{
	size_t at = begin_document(doc);
	at = create_type(doc, at, 525);                       /* Page */
	at = set_property(doc, at, 999);                      /* Content */
	at = create_type(doc, at, 442);                       /* gray grid */
	at = set_string(doc, at, 647, "Gray");                /* Background */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 442);                       /* red grid, cell (0,0) */
	at = set_string(doc, at, 647, "Red");                 /* Background */
	at = set_float(doc, at, 860, 0.0f);                   /* Grid.Row */
	at = set_float(doc, at, 857, 0.0f);                   /* Grid.Column */
	doc[at++] = 0x02;                                     /* pop */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 442);                       /* blue grid, cell (0,2) */
	at = set_string(doc, at, 647, "Blue");                /* Background */
	at = set_float(doc, at, 860, 0.0f);                   /* Grid.Row */
	at = set_float(doc, at, 857, 2.0f);                   /* Grid.Column */
	doc[at++] = 0x02;                                     /* pop */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 442);                       /* green grid, cell (0,1), fixed width */
	at = set_string(doc, at, 647, "Green");               /* Background */
	at = set_float(doc, at, 860, 0.0f);                   /* Grid.Row */
	at = set_float(doc, at, 857, 1.0f);                   /* Grid.Column */
	at = set_float(doc, at, 409, 60.0f);                  /* Width */
	doc[at++] = 0x02;                                     /* pop */
	doc[at++] = 0x02;                                     /* pop the gray grid */
	const UINT32 nodes = (UINT32)(at - 168);
	put32(doc, 8, nodes);
	put32(doc, 164, nodes);
	return at;
}

/* The span page: a gray grid whose children are a blue grid without slot properties (cell (0, 0), one track), a green grid
 * spanning two columns from column 1 of row 0 (the column-span-only case), and a red grid spanning two columns and two rows
 * from cell (1, 1). The definitions come from the client through the public collections — three 100-pixel columns and three
 * 100-pixel rows, so the red child's merged region reaches x 300, y 300 and the row span covers two real tracks. */
static size_t build_span_document(unsigned char *doc)
{
	size_t at = begin_document(doc);
	at = create_type(doc, at, 525);                       /* Page */
	at = set_property(doc, at, 999);                      /* Content */
	at = create_type(doc, at, 442);                       /* gray grid */
	at = set_string(doc, at, 647, "Gray");                /* Background */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 442);                       /* blue grid, cell (0, 0), no slot properties */
	at = set_string(doc, at, 647, "Blue");                /* Background */
	doc[at++] = 0x02;                                     /* pop */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 442);                       /* green grid, row 0 column 1, two columns */
	at = set_string(doc, at, 647, "Green");               /* Background */
	at = set_float(doc, at, 860, 0.0f);                   /* Grid.Row */
	at = set_float(doc, at, 857, 1.0f);                   /* Grid.Column */
	at = set_float(doc, at, 859, 2.0f);                   /* Grid.ColumnSpan */
	doc[at++] = 0x02;                                     /* pop */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 442);                       /* red grid, cell (1, 1), two columns and two rows */
	at = set_string(doc, at, 647, "Red");                 /* Background */
	at = set_float(doc, at, 860, 1.0f);                   /* Grid.Row */
	at = set_float(doc, at, 857, 1.0f);                   /* Grid.Column */
	at = set_float(doc, at, 862, 2.0f);                   /* Grid.RowSpan */
	at = set_float(doc, at, 859, 2.0f);                   /* Grid.ColumnSpan */
	doc[at++] = 0x02;                                     /* pop */
	doc[at++] = 0x02;                                     /* pop the gray grid */
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
	const size_t size = build_layout_document(document);
	if (load(document, (uint32_t)size, &page) != S_OK || !page) return 7;

	/* The root grid is the page's content; the fixture gives it its definitions through the IGrid collections. */
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
	void *row_collection = NULL, *column_collection = NULL;
	if (grid->lpVtbl->get_RowDefinitions(grid, (uint64_t)(uintptr_t)&row_collection) != S_OK || !row_collection) return 11;
	if (grid->lpVtbl->get_ColumnDefinitions(grid, (uint64_t)(uintptr_t)&column_collection) != S_OK || !column_collection) return 12;
	struct fixture_vector *rows = NULL, *columns = NULL;
	if (((IInspectable *)row_collection)->lpVtbl->QueryInterface((IInspectable *)row_collection,
		&IID___FIVector_1_RowDefinition, (void **)&rows) != S_OK || !rows) return 13;
	if (((IInspectable *)column_collection)->lpVtbl->QueryInterface((IInspectable *)column_collection,
		&IID___FIVector_1_ColumnDefinition, (void **)&columns) != S_OK || !columns) return 14;

	static const WCHAR row_name[] = L"Windows.UI.Xaml.Controls.RowDefinition";
	static const WCHAR column_name[] = L"Windows.UI.Xaml.Controls.ColumnDefinition";
	const double row_lengths[] = {100.0, 100.0};
	const double column_lengths[] = {100.0, 200.0};
	for (UINT32 index = 0; index < 2; ++index) {
		HSTRING key = NULL;
		IActivationFactory *activation = NULL;
		IInspectable *object = NULL;
		if (WindowsCreateString(row_name, ARRAYSIZE(row_name) - 1, &key) != S_OK) return 22;
		if (RoGetActivationFactory(key, &IID_IActivationFactory, (void **)&activation) != S_OK || !activation) return 23;
		if (activation->lpVtbl->ActivateInstance(activation, &object) != S_OK || !object) return 24;
		row_definition1 *definition = NULL;
		if (object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition,
			(void **)&definition) != S_OK || !definition) return 25;
		__x_ABI_CWindows_CUI_CXaml_CGridLength length = {row_lengths[index], GridUnitType_Pixel};
		if (definition->lpVtbl->put_Height(definition, length) != S_OK) return 26;
		if (rows->lpVtbl->Append(rows, object) != S_OK) return 27;
		definition->lpVtbl->Release(definition);
		object->lpVtbl->Release(object);
		activation->lpVtbl->Release(activation);
		WindowsDeleteString(key);
	}
	for (UINT32 index = 0; index < 2; ++index) {
		HSTRING key = NULL;
		IActivationFactory *activation = NULL;
		IInspectable *object = NULL;
		if (WindowsCreateString(column_name, ARRAYSIZE(column_name) - 1, &key) != S_OK) return 28;
		if (RoGetActivationFactory(key, &IID_IActivationFactory, (void **)&activation) != S_OK || !activation) return 29;
		if (activation->lpVtbl->ActivateInstance(activation, &object) != S_OK || !object) return 30;
		column_definition1 *definition = NULL;
		if (object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition,
			(void **)&definition) != S_OK || !definition) return 31;
		__x_ABI_CWindows_CUI_CXaml_CGridLength length = {column_lengths[index], GridUnitType_Pixel};
		if (definition->lpVtbl->put_Width(definition, length) != S_OK) return 32;
		if (columns->lpVtbl->Append(columns, object) != S_OK) return 33;
		definition->lpVtbl->Release(definition);
		object->lpVtbl->Release(object);
		activation->lpVtbl->Release(activation);
		WindowsDeleteString(key);
	}

	/* The collections carry what was appended and the lengths round-trip. */
	UINT32 count = 0;
	if (rows->lpVtbl->get_Size(rows, &count) != S_OK || count != 2) return 34;
	if (columns->lpVtbl->get_Size(columns, &count) != S_OK || count != 2) return 35;
	IInspectable *second_column = NULL;
	if (columns->lpVtbl->GetAt(columns, 1, &second_column) != S_OK || !second_column) return 36;
	column_definition1 *column = NULL;
	if (second_column->lpVtbl->QueryInterface(second_column, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition,
		(void **)&column) != S_OK || !column) return 37;
	__x_ABI_CWindows_CUI_CXaml_CGridLength width = {0, GridUnitType_Auto};
	if (column->lpVtbl->get_Width(column, &width) != S_OK || width.Value != 200.0 || width.GridUnitType != GridUnitType_Pixel)
		return 38;

	/* Hosting the page on the window renders it into the cells. */
	HSTRING window_key = NULL;
	static const WCHAR window_name[] = L"Windows.UI.Xaml.Window";
	if (WindowsCreateString(window_name, ARRAYSIZE(window_name) - 1, &window_key) != S_OK) return 39;
	IActivationFactory *window_activation = NULL;
	if (RoGetActivationFactory(window_key, &IID_IActivationFactory, (void **)&window_activation) != S_OK || !window_activation)
		return 40;
	window_statics *window_current = NULL;
	if (window_activation->lpVtbl->QueryInterface(window_activation, &IID__x_ABI_CWindows_CUI_CXaml_CIWindowStatics,
		(void **)&window_current) != S_OK || !window_current) return 41;
	window1 *xaml_window = NULL;
	if (window_current->lpVtbl->get_Current(window_current, (uint64_t)(uintptr_t)&xaml_window) != S_OK || !xaml_window) return 42;
	if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)page) != S_OK) return 43;

	/* The root grid fills the window; the red child sits in the cell at x 100..300, y 100..200. */
	if (color_at(hwnd, 50, 50, RGB(0, 0, 255))) return 44;
	if (color_at(hwnd, 250, 50, RGB(0, 0, 255))) return 45;
	if (color_at(hwnd, 50, 150, RGB(0, 0, 255))) return 46;
	if (color_at(hwnd, 150, 150, RGB(255, 0, 0))) return 47;
	if (color_at(hwnd, 350, 150, RGB(0, 0, 255))) return 48;
	if (color_at(hwnd, 400, 300, RGB(0, 0, 255))) return 49;

	/* Widening a definition through the real slot re-presents: the cell now reaches x 400. */
	width.Value = 300.0;
	if (column->lpVtbl->put_Width(column, width) != S_OK) return 50;
	if (color_at(hwnd, 150, 150, RGB(255, 0, 0))) return 51;
	if (color_at(hwnd, 350, 150, RGB(255, 0, 0))) return 52;
	if (color_at(hwnd, 450, 150, RGB(0, 0, 255))) return 53;

	column->lpVtbl->Release(column);
	second_column->lpVtbl->Release(second_column);

	/* Measure and arrange: a second page whose grid carries star, fixed and auto columns. The star columns share what the
	 * fixed column leaves on the 800-wide window (350 each), the fixed column takes its length, and re-hosting the page
	 * with Auto columns sizes the first track from the fixed width its content carries and gives the rest to the star. */
	{
		static unsigned char measure_document[1024];
		IInspectable *measure_page = NULL;
		if (load(measure_document, (uint32_t)build_measure_document(measure_document), &measure_page) != S_OK || !measure_page)
			return 60;
		IInspectable *measure_grid_object = NULL;
		{
			__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *content_control = NULL;
			if (measure_page->lpVtbl->QueryInterface(measure_page, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
				(void **)&content_control) != S_OK || !content_control) return 61;
			content_control->lpVtbl->get_Content(content_control, (uint64_t)(uintptr_t)&measure_grid_object);
			content_control->lpVtbl->Release(content_control);
		}
		if (!measure_grid_object) return 610;
		__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid *measure_grid = NULL;
		if (measure_grid_object->lpVtbl->QueryInterface(measure_grid_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid,
			(void **)&measure_grid) != S_OK || !measure_grid) return 611;
		void *measure_row_collection = NULL, *measure_column_collection = NULL;
		if (measure_grid->lpVtbl->get_RowDefinitions(measure_grid, (uint64_t)(uintptr_t)&measure_row_collection) != S_OK ||
			!measure_row_collection) return 62;
		if (measure_grid->lpVtbl->get_ColumnDefinitions(measure_grid, (uint64_t)(uintptr_t)&measure_column_collection) != S_OK ||
			!measure_column_collection) return 63;
		struct fixture_vector *measure_rows = NULL, *measure_columns = NULL;
		if (((IInspectable *)measure_row_collection)->lpVtbl->QueryInterface((IInspectable *)measure_row_collection,
			&IID___FIVector_1_RowDefinition, (void **)&measure_rows) != S_OK || !measure_rows) return 64;
		if (((IInspectable *)measure_column_collection)->lpVtbl->QueryInterface((IInspectable *)measure_column_collection,
			&IID___FIVector_1_ColumnDefinition, (void **)&measure_columns) != S_OK || !measure_columns) return 65;
		__x_ABI_CWindows_CUI_CXaml_CGridLength star_length = {1.0, GridUnitType_Star};
		__x_ABI_CWindows_CUI_CXaml_CGridLength fixed_length = {100.0, GridUnitType_Pixel};
		static const WCHAR definition_name[] = L"Windows.UI.Xaml.Controls.ColumnDefinition";
		for (UINT32 index = 0; index < 3; ++index) {
			HSTRING key = NULL;
			IActivationFactory *activation = NULL;
			IInspectable *object = NULL;
			if (WindowsCreateString(definition_name, ARRAYSIZE(definition_name) - 1, &key) != S_OK) return 66;
			if (RoGetActivationFactory(key, &IID_IActivationFactory, (void **)&activation) != S_OK || !activation) return 67;
			if (activation->lpVtbl->ActivateInstance(activation, &object) != S_OK || !object) return 68;
			column_definition1 *definition = NULL;
			if (object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition,
				(void **)&definition) != S_OK || !definition) return 69;
			if (definition->lpVtbl->put_Width(definition, index == 1 ? fixed_length : star_length) != S_OK) return 70;
			if (measure_columns->lpVtbl->Append(measure_columns, object) != S_OK) return 71;
			definition->lpVtbl->Release(definition);
			object->lpVtbl->Release(object);
			activation->lpVtbl->Release(activation);
			WindowsDeleteString(key);
		}
		if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)measure_page) != S_OK) return 72;
		/* Columns 1*, 100, 1* on the 800-wide window: red 0..350, green 350..410, blue 450..800. */
		if (color_at(hwnd, 100, 50, RGB(255, 0, 0))) return 73;
		if (color_at(hwnd, 349, 50, RGB(255, 0, 0))) {
			HDC probe = GetDC(hwnd);
			const COLORREF pixel = probe ? GetPixel(probe, 349, 50) : 0;
			if (probe) ReleaseDC(hwnd, probe);
			return 7400 + (int)(pixel & 0xff) + ((int)(pixel >> 8) & 0xff) * 256;
		}
		if (color_at(hwnd, 351, 50, RGB(0, 128, 0))) return 75;
		if (color_at(hwnd, 405, 50, RGB(0, 128, 0))) return 76;
		if (color_at(hwnd, 411, 50, RGB(128, 128, 128))) return 761;
		if (color_at(hwnd, 449, 50, RGB(128, 128, 128))) return 77;
		if (color_at(hwnd, 451, 50, RGB(0, 0, 255))) return 78;
		if (color_at(hwnd, 799, 50, RGB(0, 0, 255))) return 79;
		/* The same page hosted with Auto, 100, 1*: the first column takes the fixed width its red child carries... the red
		 * child carries none, so the Auto column measures zero; the green child keeps its 60 within the 100 column, and the
		 * star takes everything the fixed column leaves. */
		if (measure_columns->lpVtbl->Clear(measure_columns) != S_OK) return 80;
		for (UINT32 index = 0; index < 3; ++index) {
			HSTRING key = NULL;
			IActivationFactory *activation = NULL;
			IInspectable *object = NULL;
			if (WindowsCreateString(definition_name, ARRAYSIZE(definition_name) - 1, &key) != S_OK) return 81;
			if (RoGetActivationFactory(key, &IID_IActivationFactory, (void **)&activation) != S_OK || !activation) return 82;
			if (activation->lpVtbl->ActivateInstance(activation, &object) != S_OK || !object) return 83;
			column_definition1 *definition = NULL;
			if (object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition,
				(void **)&definition) != S_OK || !definition) return 84;
			__x_ABI_CWindows_CUI_CXaml_CGridLength length = {0, GridUnitType_Auto};
			if (index == 1)
				length = fixed_length;
			else if (index == 2)
				length = star_length;
			if (definition->lpVtbl->put_Width(definition, length) != S_OK) return 85;
			if (measure_columns->lpVtbl->Append(measure_columns, object) != S_OK) return 86;
			definition->lpVtbl->Release(definition);
			object->lpVtbl->Release(object);
			activation->lpVtbl->Release(activation);
			WindowsDeleteString(key);
		}
		if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)measure_page) != S_OK) return 87;
		/* The Auto column measures zero (its red child carries no width), so the columns collapse: the green child fills
		 * 0..60 from its own Width, the fixed column follows at once, and the star takes the rest. */
		if (color_at(hwnd, 25, 50, RGB(0, 128, 0))) return 88;
		if (color_at(hwnd, 61, 50, RGB(128, 128, 128))) return 89;
		if (color_at(hwnd, 101, 50, RGB(0, 0, 255))) return 90;
		if (color_at(hwnd, 250, 50, RGB(0, 0, 255))) return 91;
		if (color_at(hwnd, 799, 50, RGB(0, 0, 255))) return 92;
		measure_columns->lpVtbl->Release(measure_columns);
		measure_rows->lpVtbl->Release(measure_rows);
		((IInspectable *)measure_column_collection)->lpVtbl->Release((IInspectable *)measure_column_collection);
		((IInspectable *)measure_row_collection)->lpVtbl->Release((IInspectable *)measure_row_collection);
		measure_grid->lpVtbl->Release(measure_grid);
		measure_grid_object->lpVtbl->Release(measure_grid_object);
		measure_page->lpVtbl->Release(measure_page);
	}

	/* Spans: a third page whose grid carries three 100-pixel columns and rows. The green child spans two columns from
	 * column 1 of row 0 and the red child spans two columns and two rows from cell (1, 1); the third row is what lets the
	 * red child's merged region reach y 300 and its row span cover two real tracks. */
	{
		static unsigned char span_document[1024];
		IInspectable *span_page = NULL;
		if (load(span_document, (uint32_t)build_span_document(span_document), &span_page) != S_OK || !span_page) return 93;
		IInspectable *span_grid_object = NULL;
		{
			__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *content_control = NULL;
			if (span_page->lpVtbl->QueryInterface(span_page, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
				(void **)&content_control) != S_OK || !content_control) return 94;
			content_control->lpVtbl->get_Content(content_control, (uint64_t)(uintptr_t)&span_grid_object);
			content_control->lpVtbl->Release(content_control);
		}
		if (!span_grid_object) return 95;
		__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid *span_grid = NULL;
		if (span_grid_object->lpVtbl->QueryInterface(span_grid_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid,
			(void **)&span_grid) != S_OK || !span_grid) return 96;
		void *span_row_collection = NULL, *span_column_collection = NULL;
		if (span_grid->lpVtbl->get_RowDefinitions(span_grid, (uint64_t)(uintptr_t)&span_row_collection) != S_OK ||
			!span_row_collection) return 97;
		if (span_grid->lpVtbl->get_ColumnDefinitions(span_grid, (uint64_t)(uintptr_t)&span_column_collection) != S_OK ||
			!span_column_collection) return 98;
		struct fixture_vector *span_rows = NULL, *span_columns = NULL;
		if (((IInspectable *)span_row_collection)->lpVtbl->QueryInterface((IInspectable *)span_row_collection,
			&IID___FIVector_1_RowDefinition, (void **)&span_rows) != S_OK || !span_rows) return 99;
		if (((IInspectable *)span_column_collection)->lpVtbl->QueryInterface((IInspectable *)span_column_collection,
			&IID___FIVector_1_ColumnDefinition, (void **)&span_columns) != S_OK || !span_columns) return 100;
		static const WCHAR definition_name[] = L"Windows.UI.Xaml.Controls.ColumnDefinition";
		__x_ABI_CWindows_CUI_CXaml_CGridLength track_length = {100.0, GridUnitType_Pixel};
		for (UINT32 index = 0; index < 3; ++index) {
			HSTRING key = NULL;
			IActivationFactory *activation = NULL;
			IInspectable *object = NULL;
			if (WindowsCreateString(definition_name, ARRAYSIZE(definition_name) - 1, &key) != S_OK) return 101;
			if (RoGetActivationFactory(key, &IID_IActivationFactory, (void **)&activation) != S_OK || !activation) return 102;
			if (activation->lpVtbl->ActivateInstance(activation, &object) != S_OK || !object) return 103;
			column_definition1 *definition = NULL;
			if (object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition,
				(void **)&definition) != S_OK || !definition) return 104;
			if (definition->lpVtbl->put_Width(definition, track_length) != S_OK) return 105;
			if (span_columns->lpVtbl->Append(span_columns, object) != S_OK) return 106;
			definition->lpVtbl->Release(definition);
			object->lpVtbl->Release(object);
			activation->lpVtbl->Release(activation);
			WindowsDeleteString(key);
		}
		static const WCHAR row_definition_name[] = L"Windows.UI.Xaml.Controls.RowDefinition";
		for (UINT32 index = 0; index < 3; ++index) {
			HSTRING key = NULL;
			IActivationFactory *activation = NULL;
			IInspectable *object = NULL;
			if (WindowsCreateString(row_definition_name, ARRAYSIZE(row_definition_name) - 1, &key) != S_OK) return 107;
			if (RoGetActivationFactory(key, &IID_IActivationFactory, (void **)&activation) != S_OK || !activation) return 108;
			if (activation->lpVtbl->ActivateInstance(activation, &object) != S_OK || !object) return 109;
			row_definition1 *definition = NULL;
			if (object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition,
				(void **)&definition) != S_OK || !definition) return 110;
			if (definition->lpVtbl->put_Height(definition, track_length) != S_OK) return 111;
			if (span_rows->lpVtbl->Append(span_rows, object) != S_OK) return 112;
			definition->lpVtbl->Release(definition);
			object->lpVtbl->Release(object);
			activation->lpVtbl->Release(activation);
			WindowsDeleteString(key);
		}
		if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)span_page) != S_OK) return 113;
		/* The blue child fills cell (0, 0) alone; the green child merges columns 1-2 of row 0; the red child merges
		 * columns 1-2 of rows 1-2 and fills the merged region to x 300, y 300. */
		if (color_at(hwnd, 50, 50, RGB(0, 0, 255))) return 114;
		if (color_at(hwnd, 99, 99, RGB(0, 0, 255))) return 115;       /* the blue cell ends at x 100, y 100 */
		if (color_at(hwnd, 101, 99, RGB(0, 128, 0))) return 116;      /* the green span beside it */
		if (color_at(hwnd, 99, 101, RGB(128, 128, 128))) return 117;  /* nothing below the blue cell */
		if (color_at(hwnd, 150, 50, RGB(0, 128, 0))) return 118;
		if (color_at(hwnd, 250, 50, RGB(0, 128, 0))) return 119;
		if (color_at(hwnd, 299, 50, RGB(0, 128, 0))) return 120;      /* the span reaches x 300 */
		if (color_at(hwnd, 301, 50, RGB(128, 128, 128))) return 121;  /* past the spanned columns */
		if (color_at(hwnd, 150, 99, RGB(0, 128, 0))) return 122;      /* green's absent row span is one track */
		if (color_at(hwnd, 150, 101, RGB(255, 0, 0))) return 123;     /* red begins at y 100 */
		if (color_at(hwnd, 150, 150, RGB(255, 0, 0))) return 124;
		if (color_at(hwnd, 250, 150, RGB(255, 0, 0))) return 125;
		if (color_at(hwnd, 150, 250, RGB(255, 0, 0))) return 126;     /* the second spanned row */
		if (color_at(hwnd, 250, 250, RGB(255, 0, 0))) return 127;
		if (color_at(hwnd, 299, 299, RGB(255, 0, 0))) return 128;     /* the merged region reaches x 300, y 300 */
		if (color_at(hwnd, 301, 150, RGB(128, 128, 128))) return 129; /* past the red child's columns */
		if (color_at(hwnd, 150, 301, RGB(128, 128, 128))) return 130; /* past the red child's rows */
		if (color_at(hwnd, 50, 250, RGB(128, 128, 128))) return 131;  /* column 0 below the blue cell stays background */
		if (color_at(hwnd, 400, 300, RGB(128, 128, 128))) return 132; /* beyond all the tracks */
		span_columns->lpVtbl->Release(span_columns);
		span_rows->lpVtbl->Release(span_rows);
		((IInspectable *)span_column_collection)->lpVtbl->Release((IInspectable *)span_column_collection);
		((IInspectable *)span_row_collection)->lpVtbl->Release((IInspectable *)span_row_collection);
		span_grid->lpVtbl->Release(span_grid);
		span_grid_object->lpVtbl->Release(span_grid_object);
		span_page->lpVtbl->Release(span_page);
	}

	xaml_window->lpVtbl->Release(xaml_window);
	window_current->lpVtbl->Release(window_current);
	window_activation->lpVtbl->Release(window_activation);
	WindowsDeleteString(window_key);
	columns->lpVtbl->Release(columns);
	rows->lpVtbl->Release(rows);
	((IInspectable *)column_collection)->lpVtbl->Release((IInspectable *)column_collection);
	((IInspectable *)row_collection)->lpVtbl->Release((IInspectable *)row_collection);
	grid->lpVtbl->Release(grid);
	root->lpVtbl->Release(root);
	page->lpVtbl->Release(page);
	ICoreWindow_Release(window);
	__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_Release(statics);
	WindowsDeleteString(core_key);
	interop->lpVtbl->Release(interop);
	return 0;
}
