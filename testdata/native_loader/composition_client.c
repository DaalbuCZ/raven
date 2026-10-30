/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The composition fixture: a client builds a visual tree on the thread's CoreWindow, the compositor presents it into the
 * desktop surface, and the client reads the pixels back through the same GDI surface Raven emulates. Covers the target for the
 * current view, the container and sprite visuals, solid-colour brushes, the visibility, offset and brush mutations, and the
 * re-presentation after a native window resize. */
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_UI_Composition
#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Numerics
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <roapi.h>
#include <windows.ui.core.h>
#include <windows.ui.composition.h>
#include <windows.foundation.h>
#include <windows.foundation.numerics.h>
#include "../../native/upstream/raven-xaml/corewindow_interop.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);

typedef __x_ABI_CWindows_CUI_CComposition_CICompositor ICompositor;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionObject ICompositionObject;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionTarget ICompositionTarget;
typedef __x_ABI_CWindows_CUI_CComposition_CIContainerVisual IContainerVisual;
typedef __x_ABI_CWindows_CUI_CComposition_CISpriteVisual ISpriteVisual;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionBrush ICompositionBrush;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush IColorBrush;
typedef __x_ABI_CWindows_CUI_CComposition_CIVisual IVisual;
typedef __x_ABI_CWindows_CUI_CComposition_CIVisualCollection IVisualCollection;
typedef __x_ABI_CWindows_CUI_CColor Color;
typedef __x_ABI_CWindows_CFoundation_CNumerics_CVector2 Vector2;
typedef __x_ABI_CWindows_CFoundation_CNumerics_CVector3 Vector3;

static Color color_of(BYTE red, BYTE green, BYTE blue)
{
	Color value;
	value.A = 255;
	value.R = red;
	value.G = green;
	value.B = blue;
	return value;
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

static IColorBrush *create_brush(ICompositor *compositor, Color color)
{
	IColorBrush *brush = NULL;
	if (compositor->lpVtbl->CreateColorBrushWithColor(compositor, color, &brush) != S_OK || !brush)
		return NULL;
	return brush;
}

static ISpriteVisual *create_sprite(ICompositor *compositor, Color color, float x, float y, float width, float height)
{
	ISpriteVisual *sprite = NULL;
	if (compositor->lpVtbl->CreateSpriteVisual(compositor, &sprite) != S_OK || !sprite)
		return NULL;
	IVisual *visual = NULL;
	if (sprite->lpVtbl->QueryInterface(sprite, &IID___x_ABI_CWindows_CUI_CComposition_CIVisual, (void **)&visual) != S_OK ||
		!visual) {
		sprite->lpVtbl->Release(sprite);
		return NULL;
	}
	IColorBrush *brush = create_brush(compositor, color);
	if (!brush) {
		visual->lpVtbl->Release(visual);
		sprite->lpVtbl->Release(sprite);
		return NULL;
	}
	Vector2 size;
	size.X = width;
	size.Y = height;
	Vector3 offset;
	offset.X = x;
	offset.Y = y;
	offset.Z = 0;
	if (sprite->lpVtbl->put_Brush(sprite, (ICompositionBrush *)brush) != S_OK ||
		visual->lpVtbl->put_Size(visual, size) != S_OK || visual->lpVtbl->put_Offset(visual, offset) != S_OK) {
		brush->lpVtbl->Release(brush);
		visual->lpVtbl->Release(visual);
		sprite->lpVtbl->Release(sprite);
		return NULL;
	}
	brush->lpVtbl->Release(brush);
	visual->lpVtbl->Release(visual);
	return sprite;
}

static int append_child(IContainerVisual *container, ISpriteVisual *sprite)
{
	IVisualCollection *children = NULL;
	IVisual *visual = NULL;
	if (container->lpVtbl->get_Children(container, &children) != S_OK || !children) return 1;
	if (sprite->lpVtbl->QueryInterface(sprite, &IID___x_ABI_CWindows_CUI_CComposition_CIVisual, (void **)&visual) != S_OK ||
		!visual) {
		children->lpVtbl->Release(children);
		return 1;
	}
	const HRESULT result = children->lpVtbl->InsertAtTop(children, visual);
	visual->lpVtbl->Release(visual);
	children->lpVtbl->Release(children);
	return result == S_OK ? 0 : 1;
}

int raven_entry(void)
{
	static const WCHAR compositor_name[] = L"Windows.UI.Composition.Compositor";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function RoGetActivationFactory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	if (!RoInitialize || !RoGetActivationFactory) return 20;
	if (FAILED(RoInitialize(1))) return 1;

	static const WCHAR core_name[] = L"Windows.UI.Core.CoreWindow";
	HSTRING core_key = NULL;
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
	/* Showing the window needs a desktop the host may not provide headless; the emulated surface exists either way. */
	ICoreWindow_Activate(window);

	/* The compositor and its object identity through ICompositionObject. */
	HSTRING compositor_key = NULL;
	if (WindowsCreateString(compositor_name, ARRAYSIZE(compositor_name) - 1, &compositor_key) != S_OK) return 22;
	IInspectable *compositor_object = NULL;
	if (RoActivateInstance(compositor_key, &compositor_object) != S_OK || !compositor_object) return 7;
	ICompositor *compositor = NULL;
	if (compositor_object->lpVtbl->QueryInterface(compositor_object,
		&IID___x_ABI_CWindows_CUI_CComposition_CICompositor, (void **)&compositor) != S_OK || !compositor) return 8;
	ICompositionObject *object = NULL;
	if (compositor_object->lpVtbl->QueryInterface(compositor_object,
		&IID___x_ABI_CWindows_CUI_CComposition_CICompositionObject, (void **)&object) != S_OK || !object) return 9;
	ICompositor *through_object = NULL;
	if (object->lpVtbl->get_Compositor(object, &through_object) != S_OK || !through_object) return 10;
	IUnknown *compositor_unknown = NULL, *through_unknown = NULL;
	if (compositor->lpVtbl->QueryInterface(compositor, &IID_IUnknown, (void **)&compositor_unknown) != S_OK) return 11;
	if (through_object->lpVtbl->QueryInterface(through_object, &IID_IUnknown, (void **)&through_unknown) != S_OK) return 12;
	if (compositor_unknown != through_unknown) return 13;
	through_unknown->lpVtbl->Release(through_unknown);
	compositor_unknown->lpVtbl->Release(compositor_unknown);
	through_object->lpVtbl->Release(through_object);
	object->lpVtbl->Release(object);

	/* The target for the current view binds the thread's CoreWindow. */
	ICompositionTarget *target = NULL;
	if (compositor->lpVtbl->CreateTargetForCurrentView(compositor, &target) != S_OK || !target) return 14;

	/* The tree: a background the size of the client, a red square, a blue square above it. */
	IContainerVisual *root = NULL;
	if (compositor->lpVtbl->CreateContainerVisual(compositor, &root) != S_OK || !root) return 15;
	__x_ABI_CWindows_CFoundation_CRect bounds;
	if (ICoreWindow_get_Bounds(window, &bounds) != S_OK) return 16;
	ISpriteVisual *background = create_sprite(compositor, color_of(255, 255, 255), 0, 0, bounds.Width, bounds.Height);
	if (!background) return 17;
	ISpriteVisual *red = create_sprite(compositor, color_of(255, 0, 0), 0, 0, 50, 50);
	if (!red) return 18;
	ISpriteVisual *blue = create_sprite(compositor, color_of(0, 0, 255), 60, 10, 50, 50);
	if (!blue) return 19;
	if (append_child(root, (ISpriteVisual *)background)) return 23;
	if (append_child(root, red)) return 24;
	if (append_child(root, blue)) return 25;
	IVisual *root_visual = NULL;
	if (root->lpVtbl->QueryInterface(root, &IID___x_ABI_CWindows_CUI_CComposition_CIVisual, (void **)&root_visual) != S_OK ||
		!root_visual) return 261;
	if (target->lpVtbl->put_Root(target, root_visual) != S_OK) return 26;
	root_visual->lpVtbl->Release(root_visual);

	/* Presentation lands in the window surface: the background shows between the squares, red covers its rectangle, and the
	 * blue square drawn above it covers its own. */
	if (color_at(hwnd, 60, 60, RGB(255, 255, 255))) return 27;
	if (color_at(hwnd, 10, 10, RGB(255, 0, 0))) return 28;
	if (color_at(hwnd, 70, 20, RGB(0, 0, 255))) return 29;

	/* Moving a visual re-presents; the old place falls back to what is under it. */
	IVisual *blue_visual = NULL;
	if (blue->lpVtbl->QueryInterface(blue, &IID___x_ABI_CWindows_CUI_CComposition_CIVisual, (void **)&blue_visual) != S_OK ||
		!blue_visual) return 301;
	Vector3 moved;
	moved.X = 10;
	moved.Y = 60;
	moved.Z = 0;
	if (blue_visual->lpVtbl->put_Offset(blue_visual, moved) != S_OK) return 30;
	if (color_at(hwnd, 70, 20, RGB(255, 255, 255))) return 31;
	if (color_at(hwnd, 20, 70, RGB(0, 0, 255))) return 32;

	/* Visibility and brush mutations present too. */
	if (blue_visual->lpVtbl->put_IsVisible(blue_visual, FALSE) != S_OK) return 33;
	if (color_at(hwnd, 20, 70, RGB(255, 255, 255))) return 34;
	if (blue_visual->lpVtbl->put_IsVisible(blue_visual, TRUE) != S_OK) return 35;
	if (color_at(hwnd, 20, 70, RGB(0, 0, 255))) return 36;
	IColorBrush *green = create_brush(compositor, color_of(0, 255, 0));
	if (!green) return 37;
	if (red->lpVtbl->put_Brush(red, (ICompositionBrush *)green) != S_OK) return 38;
	if (color_at(hwnd, 10, 10, RGB(0, 255, 0))) return 39;
	green->lpVtbl->Release(green);
	blue_visual->lpVtbl->Release(blue_visual);

	/* A native resize re-presents the tree into the enlarged surface: the client paints the whole window red first, so only
	 * a fresh presentation can restore the tree everywhere and clear the newly exposed area. */
	HDC dc = GetDC(hwnd);
	HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));
	RECT client;
	if (!dc || !brush || !GetClientRect(hwnd, &client) || !FillRect(dc, &client, brush)) return 40;
	DeleteObject(brush);
	ReleaseDC(hwnd, dc);
	if (!SetWindowPos(hwnd, NULL, 0, 0, 900, 700, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE)) return 41;
	if (color_at(hwnd, 10, 10, RGB(0, 255, 0))) return 42;
	if (color_at(hwnd, 850, 650, RGB(0, 0, 0))) return 43;

	blue->lpVtbl->Release(blue);
	red->lpVtbl->Release(red);
	background->lpVtbl->Release(background);
	root->lpVtbl->Release(root);
	target->lpVtbl->Release(target);
	compositor->lpVtbl->Release(compositor);
	compositor_object->lpVtbl->Release(compositor_object);
	WindowsDeleteString(compositor_key);
	interop->lpVtbl->Release(interop);
	ICoreWindow_Release(window);
	__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_Release(statics);
	WindowsDeleteString(core_key);
	return 0;
}
