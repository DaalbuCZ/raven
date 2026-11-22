#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include "windows_graphics_imaging.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);

typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform transform1;
typedef __x_ABI_CWindows_CGraphics_CImaging_CBitmapBounds bitmap_bounds;

/* The bitmap transform: the activation hands the class's factory, the instance it activates carries the options a
 * guest stores — rotation, flip, interpolation, crop bounds and scale — and each property answers what was set. */
int raven_entry(void)
{
	static const WCHAR name[] = L"Windows.Graphics.Imaging.BitmapTransform";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function RoGetActivationFactory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	if (!RoInitialize || !RoGetActivationFactory) return 20;
	if (FAILED(RoInitialize(1))) return 1;
	HSTRING class_key = NULL;
	if (WindowsCreateString(name, ARRAYSIZE(name) - 1, &class_key) != S_OK || !class_key) return 21;
	IActivationFactory *activation = NULL;
	if (RoGetActivationFactory(class_key, &IID_IActivationFactory, (void **)&activation) != S_OK || !activation) return 2;
	IInspectable *inspectable = NULL;
	if (activation->lpVtbl->ActivateInstance(activation, &inspectable) != S_OK || !inspectable) return 3;
	transform1 *transform = NULL;
	if (inspectable->lpVtbl->QueryInterface(inspectable,
		&IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform, (void **)&transform) != S_OK || !transform) return 4;
	/* A fresh transform opens with the class's defaults: no rotation, no flip, an empty crop and a linear filter. */
	int32_t rotation = -1, flip = -1, interpolation = -1;
	if (transform->lpVtbl->get_Rotation(transform, &rotation) != S_OK || rotation != 0 /* BitmapRotation.None */) return 5;
	if (transform->lpVtbl->get_Flip(transform, &flip) != S_OK || flip != 0 /* BitmapFlip.None */) return 6;
	if (transform->lpVtbl->get_InterpolationMode(transform, &interpolation) != S_OK ||
		interpolation != 1 /* BitmapInterpolationMode.Linear */) return 7;
	bitmap_bounds bounds;
	memset(&bounds, 0xff, sizeof bounds);
	if (transform->lpVtbl->get_Bounds(transform, &bounds) != S_OK || bounds.X || bounds.Y || bounds.Width || bounds.Height) return 8;
	/* The options the guest stores come back from the properties that carry them. */
	if (transform->lpVtbl->put_Rotation(transform, 1 /* Clockwise90Degrees */) != S_OK) return 9;
	if (transform->lpVtbl->put_Flip(transform, 2 /* Vertical */) != S_OK) return 10;
	if (transform->lpVtbl->put_InterpolationMode(transform, 3 /* Fant */) != S_OK) return 11;
	if (transform->lpVtbl->put_ScaledWidth(transform, 128) != S_OK) return 12;
	if (transform->lpVtbl->put_ScaledHeight(transform, 64) != S_OK) return 13;
	bounds.X = 4;
	bounds.Y = 8;
	bounds.Width = 100;
	bounds.Height = 50;
	if (transform->lpVtbl->put_Bounds(transform, bounds) != S_OK) return 14;
	rotation = -1;
	flip = -1;
	interpolation = -1;
	UINT32 scaled_width = 0, scaled_height = 0;
	if (transform->lpVtbl->get_Rotation(transform, &rotation) != S_OK || rotation != 1) return 15;
	if (transform->lpVtbl->get_Flip(transform, &flip) != S_OK || flip != 2) return 16;
	if (transform->lpVtbl->get_InterpolationMode(transform, &interpolation) != S_OK || interpolation != 3) return 17;
	if (transform->lpVtbl->get_ScaledWidth(transform, &scaled_width) != S_OK || scaled_width != 128) return 18;
	if (transform->lpVtbl->get_ScaledHeight(transform, &scaled_height) != S_OK || scaled_height != 64) return 19;
	memset(&bounds, 0, sizeof bounds);
	if (transform->lpVtbl->get_Bounds(transform, &bounds) != S_OK || bounds.X != 4 || bounds.Y != 8 ||
		bounds.Width != 100 || bounds.Height != 50) return 20;
	/* The runtime class the instance names is the class the activation asked for. */
	HSTRING runtime_name = NULL;
	UINT32 length = 0;
	if (transform->lpVtbl->GetRuntimeClassName(transform, &runtime_name) != S_OK) return 21;
	const WCHAR *text = WindowsGetStringRawBuffer(runtime_name, &length);
	if (length != ARRAYSIZE(name) - 1 || CompareStringOrdinal(text, (int)length, name, -1, FALSE) != CSTR_EQUAL) return 22;
	WindowsDeleteString(runtime_name);
	/* The same instance answers the default interface's identifier a second time without error. */
	transform1 *again = NULL;
	if (transform->lpVtbl->QueryInterface(transform,
		&IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform, (void **)&again) != S_OK || !again) return 23;
	again->lpVtbl->Release(again);
	transform->lpVtbl->Release(transform);
	inspectable->lpVtbl->Release(inspectable);
	activation->lpVtbl->Release(activation);
	WindowsDeleteString(class_key);
	return 0;
}
