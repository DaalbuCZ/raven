/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The rendering fixture: a compiled page hosts a blue grid with a red square over it, the XAML window presents it through the
 * composition stage, and the client verifies the pixels through the same emulated GDI surface. Covers the hosting of window
 * content, the background and size properties of the element classes, and the re-render when a background changes through the
 * real Panel.Background brush slot. */
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
#include <windows.ui.composition.h>
#include <windows.foundation.h>
#include "../../native/upstream/raven-xaml/corewindow_interop.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_controls.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_elements.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_window.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);
typedef HRESULT (WINAPI *load_function)(const void *, uint32_t, IInspectable **);

typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPanel panel1;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindow window1;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindowStatics window_statics;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositor ICompositor;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush IColorBrush;
typedef __x_ABI_CWindows_CUI_CColor Color;

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

/* A page whose content is a blue grid holding a red 100x100 square and an unrendered text block. */
static size_t build_render_document(unsigned char *doc)
{
	size_t at = begin_document(doc);
	at = create_type(doc, at, 525);                       /* Page */
	at = set_property(doc, at, 999);                      /* Content */
	at = create_type(doc, at, 442);                       /* blue grid */
	at = set_string(doc, at, 647, "Blue");                /* Background */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 442);                       /* red square */
	at = set_string(doc, at, 647, "Red");                 /* Background */
	at = set_float(doc, at, 409, 100.0f);                 /* Width */
	at = set_float(doc, at, 393, 100.0f);                 /* Height */
	doc[at++] = 0x02;                                     /* pop the square */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 396);                       /* TextBlock */
	at = set_string(doc, at, 800, "Hello");               /* Text */
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
	const size_t size = build_render_document(document);
	if (load(document, (uint32_t)size, &page) != S_OK || !page) return 7;

	/* Hosting the page on the window renders it. */
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

	if (color_at(hwnd, 5, 5, RGB(255, 0, 0))) return 12;
	if (color_at(hwnd, 400, 300, RGB(0, 0, 255))) return 13;
	if (color_at(hwnd, 150, 150, RGB(0, 0, 255))) return 14;

	/* The real brush slot re-renders: the composition brush the fixture builds becomes the square's background. */
	HSTRING compositor_key = NULL;
	static const WCHAR compositor_name[] = L"Windows.UI.Composition.Compositor";
	if (WindowsCreateString(compositor_name, ARRAYSIZE(compositor_name) - 1, &compositor_key) != S_OK) return 23;
	IInspectable *compositor_object = NULL;
	if (RoActivateInstance(compositor_key, &compositor_object) != S_OK || !compositor_object) return 15;
	ICompositor *compositor = NULL;
	if (compositor_object->lpVtbl->QueryInterface(compositor_object,
		&IID___x_ABI_CWindows_CUI_CComposition_CICompositor, (void **)&compositor) != S_OK || !compositor) return 16;
	IColorBrush *brush = NULL;
	Color green;
	green.A = 255;
	green.R = 0;
	green.G = 255;
	green.B = 0;
	if (compositor->lpVtbl->CreateColorBrushWithColor(compositor, green, &brush) != S_OK || !brush) return 17;
	/* The blue grid is the page's content; its IPanel takes the real brush and the tree re-renders. */
	IInspectable *blue_grid = NULL;
	{
		__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *content_control = NULL;
		if (page->lpVtbl->QueryInterface(page, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
			(void **)&content_control) != S_OK || !content_control) return 18;
		content_control->lpVtbl->get_Content(content_control, (uint64_t)(uintptr_t)&blue_grid);
		content_control->lpVtbl->Release(content_control);
	}
	if (!blue_grid) return 19;
	panel1 *blue_panel = NULL;
	if (blue_grid->lpVtbl->QueryInterface(blue_grid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
		(void **)&blue_panel) != S_OK || !blue_panel) return 24;
	if (blue_panel->lpVtbl->put_Background(blue_panel, (uint64_t)(uintptr_t)brush) != S_OK) return 26;
	if (color_at(hwnd, 400, 300, RGB(0, 255, 0))) return 27;
	if (color_at(hwnd, 5, 5, RGB(255, 0, 0))) return 28;

	/* A native resize re-presents the rendered tree into the enlarged surface. */
	HDC dc = GetDC(hwnd);
	HBRUSH fill = CreateSolidBrush(RGB(255, 255, 255));
	RECT client;
	if (!dc || !fill || !GetClientRect(hwnd, &client) || !FillRect(dc, &client, fill)) return 29;
	DeleteObject(fill);
	ReleaseDC(hwnd, dc);
	if (!SetWindowPos(hwnd, NULL, 0, 0, 900, 700, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE)) return 30;
	if (color_at(hwnd, 5, 5, RGB(255, 0, 0))) return 31;
	if (color_at(hwnd, 400, 300, RGB(0, 255, 0))) return 32;

	blue_panel->lpVtbl->Release(blue_panel);
	blue_grid->lpVtbl->Release(blue_grid);
	brush->lpVtbl->Release(brush);
	compositor->lpVtbl->Release(compositor);
	compositor_object->lpVtbl->Release(compositor_object);
	WindowsDeleteString(compositor_key);
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
