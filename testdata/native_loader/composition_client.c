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
#include "../../native/upstream/raven-xaml/swapchain_panel.h"

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

/* These are reference-count probes, not rendered swapchains: the storage hook only uses
 * IUnknown lifetime methods. No test below claims a texture was created or displayed. */
struct chain_probe { IUnknown iface; LONG refs; };
static HRESULT WINAPI chain_query(IUnknown *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct chain_probe *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI chain_addref(IUnknown *iface)
{ return InterlockedIncrement(&((struct chain_probe *)iface)->refs); }
static ULONG WINAPI chain_release(IUnknown *iface)
{ return InterlockedDecrement(&((struct chain_probe *)iface)->refs); }
static IUnknownVtbl chain_table = { chain_query, chain_addref, chain_release };
typedef HRESULT (*panel_factory_fn)(IActivationFactory **);
typedef HRESULT (*panel_store_fn)(raven_swapchain_panel *, IDXGISwapChain *);
typedef HRESULT (*panel_get_fn)(raven_swapchain_panel *, IDXGISwapChain **);
struct panel_worker {
	raven_swapchain_panel *panel;
	raven_swapchain_native *native;
	panel_store_fn store;
	panel_get_fn get;
};
static DWORD WINAPI panel_wrong_thread(void *context)
{
	struct panel_worker *worker = context;
	IDXGISwapChain *chain = (void *)(UINT_PTR)1;
	if (worker->get(worker->panel, &chain) != RPC_E_WRONG_THREAD || chain) return __LINE__;
	if (worker->store(worker->panel, NULL) != RPC_E_WRONG_THREAD) return __LINE__;
	if (worker->native->lpVtbl->SetSwapChain(worker->native, NULL) != RPC_E_WRONG_THREAD) return __LINE__;
	return 0;
}
static int check_swapchain_panel(void)
{
	HMODULE library = LoadLibraryW(L"raven-xaml.dll");
	panel_factory_fn factory = library ? (panel_factory_fn)GetProcAddress(library, "raven_xaml_swapchain_panel_factory") : NULL;
	panel_store_fn store = library ? (panel_store_fn)GetProcAddress(library, "raven_xaml_swapchain_panel_store") : NULL;
	panel_get_fn get = library ? (panel_get_fn)GetProcAddress(library, "raven_xaml_swapchain_panel_get") : NULL;
	if (!factory || !store || !get) return __LINE__;
	if (factory(NULL) != E_POINTER) return __LINE__;
	IActivationFactory *activation = NULL;
	if (factory(&activation) != S_OK || !activation) return __LINE__;
	if (activation->lpVtbl->ActivateInstance(activation, NULL) != E_POINTER) return __LINE__;
	IInspectable *object = NULL;
	if (activation->lpVtbl->ActivateInstance(activation, &object) != S_OK || !object) return __LINE__;
	raven_swapchain_panel *panel = NULL;
	if (object->lpVtbl->QueryInterface(object, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CISwapChainPanel,
		(void **)&panel) != S_OK || !panel) return __LINE__;
	raven_swapchain_native *native = NULL;
	if (panel->lpVtbl->QueryInterface(panel, &IID_ISwapChainPanelNative, (void **)&native) != S_OK || !native) return __LINE__;
	IUnknown *identity = NULL;
	if (native->lpVtbl->QueryInterface(native, &IID_IUnknown, (void **)&identity) != S_OK ||
		identity != (IUnknown *)object) return __LINE__;
	identity->lpVtbl->Release(identity);
	HSTRING name = NULL;
	if (panel->lpVtbl->GetRuntimeClassName(panel, &name) != S_OK ||
		lstrcmpW(WindowsGetStringRawBuffer(name, NULL), L"Windows.UI.Xaml.Controls.SwapChainPanel")) return __LINE__;
	WindowsDeleteString(name);
	FLOAT scale = 17;
	if (panel->lpVtbl->get_CompositionScaleX(panel, NULL) != E_POINTER) return __LINE__;
	if (panel->lpVtbl->get_CompositionScaleX(panel, &scale) != E_NOTIMPL || scale) return __LINE__;
	scale = 17;
	if (panel->lpVtbl->get_CompositionScaleY(panel, &scale) != E_NOTIMPL || scale) return __LINE__;
	EventRegistrationToken token; token.value = 17;
	if (panel->lpVtbl->add_CompositionScaleChanged(panel, NULL, &token) != E_NOTIMPL || token.value) return __LINE__;
	if (panel->lpVtbl->remove_CompositionScaleChanged(panel, token) != E_NOTIMPL) return __LINE__;
	__x_ABI_CWindows_CUI_CCore_CICoreInputSourceBase *input = (void *)(UINT_PTR)1;
	if (panel->lpVtbl->CreateCoreIndependentInputSource(panel, 0, &input) != E_NOTIMPL || input) return __LINE__;
	IDXGISwapChain *value = (void *)(UINT_PTR)1;
	if (get(panel, NULL) != E_POINTER || get(panel, &value) != S_OK || value) return __LINE__;
	struct chain_probe first = {{&chain_table}, 1}, second = {{&chain_table}, 1};
	if (native->lpVtbl->SetSwapChain(native, (IDXGISwapChain *)&first) != E_NOTIMPL || first.refs != 1) return __LINE__;
	if (store(panel, (IDXGISwapChain *)&first) != S_OK || first.refs != 2) return __LINE__;
	if (store(panel, (IDXGISwapChain *)&first) != S_OK || first.refs != 2) return __LINE__;
	if (get(panel, &value) != S_OK || value != (IDXGISwapChain *)&first || first.refs != 3) return __LINE__;
	value->lpVtbl->Release(value);
	if (store(panel, (IDXGISwapChain *)&second) != S_OK || first.refs != 1 || second.refs != 2) return __LINE__;
	if (native->lpVtbl->SetSwapChain(native, (IDXGISwapChain *)&first) != E_NOTIMPL || first.refs != 1) return __LINE__;
	if (get(panel, &value) != S_OK || value != (IDXGISwapChain *)&second) return __LINE__;
	value->lpVtbl->Release(value);
	struct panel_worker worker = {panel, native, store, get};
	HANDLE thread = CreateThread(NULL, 0, panel_wrong_thread, &worker, 0, NULL);
	DWORD result = 1;
	if (!thread || WaitForSingleObject(thread, 5000) != WAIT_OBJECT_0 ||
		!GetExitCodeThread(thread, &result) || result) return __LINE__;
	CloseHandle(thread);
	if (native->lpVtbl->SetSwapChain(native, NULL) != S_OK || second.refs != 1) return __LINE__;
	if (store(panel, (IDXGISwapChain *)&first) != S_OK) return __LINE__;
	object->lpVtbl->Release(object);
	panel->lpVtbl->Release(panel);
	if (native->lpVtbl->Release(native) != 0 || first.refs != 1) return __LINE__;
	activation->lpVtbl->Release(activation);
	/* Keep the component loaded while later fixture code uses its CoreWindow class. */
	return 0;
}

int raven_entry(void)
{
	static const WCHAR compositor_name[] = L"Windows.UI.Composition.Compositor";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function RoGetActivationFactory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	if (!RoInitialize || !RoGetActivationFactory) return __LINE__;
	if (FAILED(RoInitialize(1))) return __LINE__;

	static const WCHAR core_name[] = L"Windows.UI.Core.CoreWindow";
	HSTRING core_key = NULL;
	if (WindowsCreateString(core_name, ARRAYSIZE(core_name) - 1, &core_key) != S_OK) return __LINE__;
	__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic *statics = NULL;
	if (RoGetActivationFactory(core_key, &IID___x_ABI_CWindows_CUI_CCore_CICoreWindowStatic, (void **)&statics) != S_OK ||
		!statics) return __LINE__;
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *window = NULL;
	if (__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_GetForCurrentThread(statics, &window) != S_OK || !window) return __LINE__;
	raven_core_window_interop *interop = NULL;
	if (window->lpVtbl->QueryInterface(window, &raven_core_window_interop_iid, (void **)&interop) != S_OK || !interop) return __LINE__;
	HWND hwnd = NULL;
	if (interop->lpVtbl->get_WindowHandle(interop, &hwnd) != S_OK || !hwnd) return __LINE__;
	int panel_result = check_swapchain_panel();
	if (panel_result) return panel_result;
	/* Showing the window needs a desktop the host may not provide headless; the emulated surface exists either way. */
	ICoreWindow_Activate(window);

	/* The compositor and its object identity through ICompositionObject. */
	HSTRING compositor_key = NULL;
	if (WindowsCreateString(compositor_name, ARRAYSIZE(compositor_name) - 1, &compositor_key) != S_OK) return __LINE__;
	IInspectable *compositor_object = NULL;
	if (RoActivateInstance(compositor_key, &compositor_object) != S_OK || !compositor_object) return __LINE__;
	ICompositor *compositor = NULL;
	if (compositor_object->lpVtbl->QueryInterface(compositor_object,
		&IID___x_ABI_CWindows_CUI_CComposition_CICompositor, (void **)&compositor) != S_OK || !compositor) return __LINE__;
	ICompositionObject *object = NULL;
	if (compositor_object->lpVtbl->QueryInterface(compositor_object,
		&IID___x_ABI_CWindows_CUI_CComposition_CICompositionObject, (void **)&object) != S_OK || !object) return __LINE__;
	ICompositor *through_object = NULL;
	if (object->lpVtbl->get_Compositor(object, &through_object) != S_OK || !through_object) return __LINE__;
	IUnknown *compositor_unknown = NULL, *through_unknown = NULL;
	if (compositor->lpVtbl->QueryInterface(compositor, &IID_IUnknown, (void **)&compositor_unknown) != S_OK) return __LINE__;
	if (through_object->lpVtbl->QueryInterface(through_object, &IID_IUnknown, (void **)&through_unknown) != S_OK) return __LINE__;
	if (compositor_unknown != through_unknown) return __LINE__;
	through_unknown->lpVtbl->Release(through_unknown);
	compositor_unknown->lpVtbl->Release(compositor_unknown);
	through_object->lpVtbl->Release(through_object);
	object->lpVtbl->Release(object);

	/* GPU surfaces must fail explicitly, with no brush returned and no made-up pixels. */
	__x_ABI_CWindows_CUI_CComposition_CICompositionSurfaceBrush *surface_brush = (void *)(UINT_PTR)1;
	if (compositor->lpVtbl->CreateSurfaceBrush(compositor, &surface_brush) != E_NOTIMPL || surface_brush) return __LINE__;
	surface_brush = (void *)(UINT_PTR)1;
	if (compositor->lpVtbl->CreateSurfaceBrushWithSurface(compositor, NULL, &surface_brush) != E_NOTIMPL || surface_brush) return __LINE__;

	/* The target for the current view binds the thread's CoreWindow. */
	ICompositionTarget *target = NULL;
	if (compositor->lpVtbl->CreateTargetForCurrentView(compositor, &target) != S_OK || !target) return __LINE__;

	/* The tree: a background the size of the client, a red square, a blue square above it. */
	IContainerVisual *root = NULL;
	if (compositor->lpVtbl->CreateContainerVisual(compositor, &root) != S_OK || !root) return __LINE__;
	__x_ABI_CWindows_CFoundation_CRect bounds;
	if (ICoreWindow_get_Bounds(window, &bounds) != S_OK) return __LINE__;
	ISpriteVisual *background = create_sprite(compositor, color_of(255, 255, 255), 0, 0, bounds.Width, bounds.Height);
	if (!background) return __LINE__;
	ISpriteVisual *red = create_sprite(compositor, color_of(255, 0, 0), 0, 0, 50, 50);
	if (!red) return __LINE__;
	ISpriteVisual *blue = create_sprite(compositor, color_of(0, 0, 255), 60, 10, 50, 50);
	if (!blue) return __LINE__;
	if (append_child(root, (ISpriteVisual *)background)) return __LINE__;
	if (append_child(root, red)) return __LINE__;
	if (append_child(root, blue)) return __LINE__;
	IVisual *root_visual = NULL;
	if (root->lpVtbl->QueryInterface(root, &IID___x_ABI_CWindows_CUI_CComposition_CIVisual, (void **)&root_visual) != S_OK ||
		!root_visual) return __LINE__;
	if (target->lpVtbl->put_Root(target, root_visual) != S_OK) return __LINE__;
	root_visual->lpVtbl->Release(root_visual);

	/* Presentation lands in the window surface: the background shows between the squares, red covers its rectangle, and the
	 * blue square drawn above it covers its own. */
	if (color_at(hwnd, 60, 60, RGB(255, 255, 255))) return __LINE__;
	if (color_at(hwnd, 10, 10, RGB(255, 0, 0))) return __LINE__;
	if (color_at(hwnd, 70, 20, RGB(0, 0, 255))) return __LINE__;

	/* Moving a visual re-presents; the old place falls back to what is under it. */
	IVisual *blue_visual = NULL;
	if (blue->lpVtbl->QueryInterface(blue, &IID___x_ABI_CWindows_CUI_CComposition_CIVisual, (void **)&blue_visual) != S_OK ||
		!blue_visual) return __LINE__;
	Vector3 moved;
	moved.X = 10;
	moved.Y = 60;
	moved.Z = 0;
	if (blue_visual->lpVtbl->put_Offset(blue_visual, moved) != S_OK) return __LINE__;
	if (color_at(hwnd, 70, 20, RGB(255, 255, 255))) return __LINE__;
	if (color_at(hwnd, 20, 70, RGB(0, 0, 255))) return __LINE__;

	/* Visibility and brush mutations present too. */
	if (blue_visual->lpVtbl->put_IsVisible(blue_visual, FALSE) != S_OK) return __LINE__;
	if (color_at(hwnd, 20, 70, RGB(255, 255, 255))) return __LINE__;
	if (blue_visual->lpVtbl->put_IsVisible(blue_visual, TRUE) != S_OK) return __LINE__;
	if (color_at(hwnd, 20, 70, RGB(0, 0, 255))) return __LINE__;
	IColorBrush *green = create_brush(compositor, color_of(0, 255, 0));
	if (!green) return __LINE__;
	if (red->lpVtbl->put_Brush(red, (ICompositionBrush *)green) != S_OK) return __LINE__;
	if (color_at(hwnd, 10, 10, RGB(0, 255, 0))) return __LINE__;
	green->lpVtbl->Release(green);
	blue_visual->lpVtbl->Release(blue_visual);

	/* A native resize re-presents the tree into the enlarged surface: the client paints the whole window red first, so only
	 * a fresh presentation can restore the tree everywhere and clear the newly exposed area. */
	HDC dc = GetDC(hwnd);
	HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));
	RECT client;
	if (!dc || !brush || !GetClientRect(hwnd, &client) || !FillRect(dc, &client, brush)) return __LINE__;
	DeleteObject(brush);
	ReleaseDC(hwnd, dc);
	if (!SetWindowPos(hwnd, NULL, 0, 0, 900, 700, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE)) return __LINE__;
	if (color_at(hwnd, 10, 10, RGB(0, 255, 0))) return __LINE__;
	if (color_at(hwnd, 850, 650, RGB(0, 0, 0))) return __LINE__;

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
