/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
#define INITGUID
#define CONST_VTABLE
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define WIDL_using_Windows_UI_Input
#define WIDL_using_Windows_Devices_Input
#define WIDL_using_Windows_Foundation
#include <windows.h>
#include <winstring.h>
#include "pointer.h"
#include "xaml_internal.h"
#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

struct mouse_point {
	IPointerPoint point;
	IPointerPointProperties properties;
	LONG refs;
	struct raven_mouse_sample sample;
};
static IPointerDevice mouse_device;
static LONG device_refs;
static HRESULT no_iids(ULONG *count, IID **iids)
{
	if (!count || !iids) return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}
static HRESULT class_name(const WCHAR *name, HSTRING *out)
{
	if (!out) return E_POINTER;
	return WindowsCreateString(name, (UINT32)lstrlenW(name), out);
}
static HRESULT trust(TrustLevel *out)
{
	if (!out) return E_POINTER;
	*out = BaseTrust;
	return S_OK;
}
static ULONG reference(struct mouse_point *self) { return InterlockedIncrement(&self->refs); }
static ULONG release(struct mouse_point *self)
{
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return refs;
}
static HRESULT WINAPI point_query(IPointerPoint *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) && !IsEqualIID(iid, &IID_IPointerPoint)) return E_NOINTERFACE;
	*out = iface;
	reference((void *)iface);
	return S_OK;
}
static ULONG WINAPI point_addref(IPointerPoint *iface) { return reference((void *)iface); }
static ULONG WINAPI point_release(IPointerPoint *iface) { return release((void *)iface); }
static HRESULT WINAPI point_iids(IPointerPoint *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI point_class(IPointerPoint *iface, HSTRING *out) { return class_name(L"Windows.UI.Input.PointerPoint", out); }
static HRESULT WINAPI point_trust(IPointerPoint *iface, TrustLevel *out) { return trust(out); }
static HRESULT WINAPI point_device(IPointerPoint *iface, IPointerDevice **out)
{
	if (!out) return E_POINTER;
	*out = &mouse_device;
	IPointerDevice_AddRef(*out);
	return S_OK;
}
static HRESULT WINAPI point_position(IPointerPoint *iface, Point *out)
{
	if (!out) return E_POINTER;
	*out = ((struct mouse_point *)iface)->sample.position;
	return S_OK;
}
static HRESULT WINAPI point_id(IPointerPoint *iface, UINT32 *out)
{
	if (!out) return E_POINTER;
	*out = 1; /* Raven's single desktop mouse keeps the same pointer identity across windows. */
	return S_OK;
}
static HRESULT WINAPI point_frame(IPointerPoint *iface, UINT32 *out)
{
	if (!out) return E_POINTER;
	*out = ((struct mouse_point *)iface)->sample.frame;
	return S_OK;
}
static HRESULT WINAPI point_timestamp(IPointerPoint *iface, UINT64 *out)
{
	if (!out) return E_POINTER;
	*out = ((struct mouse_point *)iface)->sample.timestamp;
	return S_OK;
}
static HRESULT WINAPI point_contact(IPointerPoint *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = !!(((struct mouse_point *)iface)->sample.buttons & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2));
	return S_OK;
}
static HRESULT WINAPI point_properties(IPointerPoint *iface, IPointerPointProperties **out)
{
	if (!out) return E_POINTER;
	*out = &((struct mouse_point *)iface)->properties;
	reference((void *)iface);
	return S_OK;
}
static struct mouse_point *properties_owner(IPointerPointProperties *iface) { return CONTAINING_RECORD(iface, struct mouse_point, properties); }
static HRESULT WINAPI properties_query(IPointerPointProperties *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) && !IsEqualIID(iid, &IID_IPointerPointProperties)) return E_NOINTERFACE;
	*out = iface;
	reference(properties_owner(iface));
	return S_OK;
}
static ULONG WINAPI properties_addref(IPointerPointProperties *iface) { return reference(properties_owner(iface)); }
static ULONG WINAPI properties_release(IPointerPointProperties *iface) { return release(properties_owner(iface)); }
static HRESULT WINAPI properties_iids(IPointerPointProperties *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI properties_class(IPointerPointProperties *iface, HSTRING *out) { return class_name(L"Windows.UI.Input.PointerPointProperties", out); }
static HRESULT WINAPI properties_trust(IPointerPointProperties *iface, TrustLevel *out) { return trust(out); }

static HRESULT WINAPI properties_Pressure(IPointerPointProperties *iface, FLOAT *out)
{
	if (!out) return E_POINTER;
	*out = 0.5f;
	return S_OK;
}

static HRESULT WINAPI properties_IsInverted(IPointerPointProperties *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = FALSE;
	return S_OK;
}

static HRESULT WINAPI properties_IsEraser(IPointerPointProperties *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = FALSE;
	return S_OK;
}

static HRESULT WINAPI properties_Orientation(IPointerPointProperties *iface, FLOAT *out)
{
	if (!out) return E_POINTER;
	*out = 0.0f;
	return S_OK;
}

static HRESULT WINAPI properties_XTilt(IPointerPointProperties *iface, FLOAT *out)
{
	if (!out) return E_POINTER;
	*out = 0.0f;
	return S_OK;
}

static HRESULT WINAPI properties_YTilt(IPointerPointProperties *iface, FLOAT *out)
{
	if (!out) return E_POINTER;
	*out = 0.0f;
	return S_OK;
}

static HRESULT WINAPI properties_Twist(IPointerPointProperties *iface, FLOAT *out)
{
	if (!out) return E_POINTER;
	*out = 0.0f;
	return S_OK;
}

static HRESULT WINAPI properties_TouchConfidence(IPointerPointProperties *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = TRUE;
	return S_OK;
}

static HRESULT WINAPI properties_IsPrimary(IPointerPointProperties *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = TRUE;
	return S_OK;
}

static HRESULT WINAPI properties_IsInRange(IPointerPointProperties *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = TRUE;
	return S_OK;
}

static HRESULT WINAPI properties_IsCanceled(IPointerPointProperties *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = FALSE;
	return S_OK;
}

static HRESULT WINAPI properties_IsBarrelButtonPressed(IPointerPointProperties *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = FALSE;
	return S_OK;
}

static HRESULT WINAPI properties_IsLeftButtonPressed(IPointerPointProperties *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = !!(properties_owner(iface)->sample.buttons & MK_LBUTTON);
	return S_OK;
}

static HRESULT WINAPI properties_IsRightButtonPressed(IPointerPointProperties *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = !!(properties_owner(iface)->sample.buttons & MK_RBUTTON);
	return S_OK;
}

static HRESULT WINAPI properties_IsMiddleButtonPressed(IPointerPointProperties *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = !!(properties_owner(iface)->sample.buttons & MK_MBUTTON);
	return S_OK;
}

static HRESULT WINAPI properties_IsXButton1Pressed(IPointerPointProperties *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = !!(properties_owner(iface)->sample.buttons & MK_XBUTTON1);
	return S_OK;
}

static HRESULT WINAPI properties_IsXButton2Pressed(IPointerPointProperties *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = !!(properties_owner(iface)->sample.buttons & MK_XBUTTON2);
	return S_OK;
}

static HRESULT WINAPI properties_MouseWheelDelta(IPointerPointProperties *iface, INT32 *out)
{
	if (!out) return E_POINTER;
	*out = properties_owner(iface)->sample.wheel;
	return S_OK;
}

static HRESULT WINAPI properties_IsHorizontalMouseWheel(IPointerPointProperties *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = properties_owner(iface)->sample.horizontal;
	return S_OK;
}

static HRESULT WINAPI properties_PointerUpdateKind(IPointerPointProperties *iface, PointerUpdateKind *out)
{
	if (!out) return E_POINTER;
	*out = properties_owner(iface)->sample.update;
	return S_OK;
}

static HRESULT WINAPI properties_rect(IPointerPointProperties *iface, Rect *out)
{
	if (!out) return E_POINTER;
	Point position = properties_owner(iface)->sample.position;
	out->X = position.X;
	out->Y = position.Y;
	out->Width = out->Height = 0;
	return S_OK;
}
static HRESULT WINAPI properties_has_usage(IPointerPointProperties *iface, UINT32 page, UINT32 usage, boolean *out)
{
	if (!out) return E_POINTER;
	*out = FALSE;
	return S_OK;
}
static HRESULT WINAPI properties_usage(IPointerPointProperties *iface, UINT32 page, UINT32 usage, INT32 *out)
{
	if (!out) return E_POINTER;
	*out = 0;
	return E_INVALIDARG;
}
static ULONG WINAPI device_addref(IPointerDevice *iface)
{
	InterlockedIncrement(&raven_xaml_objects);
	return InterlockedIncrement(&device_refs);
}
static ULONG WINAPI device_release(IPointerDevice *iface)
{
	LONG refs = InterlockedDecrement(&device_refs);
	InterlockedDecrement(&raven_xaml_objects);
	return refs;
}
static HRESULT WINAPI device_query(IPointerDevice *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) && !IsEqualIID(iid, &IID_IPointerDevice)) return E_NOINTERFACE;
	*out = iface;
	device_addref(iface);
	return S_OK;
}
static HRESULT WINAPI device_iids(IPointerDevice *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI device_class(IPointerDevice *iface, HSTRING *out) { return class_name(L"Windows.Devices.Input.PointerDevice", out); }
static HRESULT WINAPI device_trust(IPointerDevice *iface, TrustLevel *out) { return trust(out); }
static HRESULT WINAPI device_type(IPointerDevice *iface, PointerDeviceType *out)
{
	if (!out) return E_POINTER;
	*out = PointerDeviceType_Mouse;
	return S_OK;
}
static HRESULT WINAPI device_integrated(IPointerDevice *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = FALSE;
	return S_OK;
}
static HRESULT WINAPI device_contacts(IPointerDevice *iface, UINT32 *out)
{
	if (!out) return E_POINTER;
	*out = 1;
	return S_OK;
}
static HRESULT WINAPI device_physical_rect(IPointerDevice *iface, Rect *out)
{
	if (!out) return E_POINTER;
	out->X = (FLOAT)GetSystemMetrics(SM_XVIRTUALSCREEN);
	out->Y = (FLOAT)GetSystemMetrics(SM_YVIRTUALSCREEN);
	out->Width = (FLOAT)GetSystemMetrics(SM_CXVIRTUALSCREEN);
	out->Height = (FLOAT)GetSystemMetrics(SM_CYVIRTUALSCREEN);
	if (out->Width <= 0) out->Width = (FLOAT)GetSystemMetrics(SM_CXSCREEN);
	if (out->Height <= 0) out->Height = (FLOAT)GetSystemMetrics(SM_CYSCREEN);
	return S_OK;
}
static HRESULT WINAPI device_screen_rect(IPointerDevice *iface, Rect *out)
{
	HRESULT result = device_physical_rect(iface, out);
	if (FAILED(result)) return result;
	UINT dpi = GetDpiForSystem();
	FLOAT scale = dpi ? 96.0f / dpi : 1.0f;
	out->X *= scale; out->Y *= scale; out->Width *= scale; out->Height *= scale;
	return S_OK;
}
static HRESULT WINAPI device_usages(IPointerDevice *iface, __FIVectorView_1_PointerDeviceUsage **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	/* shortcut: the desktop mouse has no exposed HID usage collection; add a device-backed collection with HID input. */
	return E_NOTIMPL;
}
static const IPointerDeviceVtbl device_table = {
	device_query, device_addref, device_release, device_iids, device_class, device_trust,
	device_type, device_integrated, device_contacts, device_physical_rect, device_screen_rect, device_usages
};
static IPointerDevice mouse_device = {&device_table};
static const IPointerPointVtbl point_table = {
	point_query, point_addref, point_release, point_iids, point_class, point_trust,
	point_device, point_position, point_position, point_id, point_frame, point_timestamp, point_contact, point_properties
};
static const IPointerPointPropertiesVtbl properties_table = {
	.QueryInterface = properties_query, .AddRef = properties_addref, .Release = properties_release,
	.GetIids = properties_iids, .GetRuntimeClassName = properties_class, .GetTrustLevel = properties_trust,
	.get_ContactRect = properties_rect, .get_ContactRectRaw = properties_rect,
	.HasUsage = properties_has_usage, .GetUsageValue = properties_usage,
	.get_Pressure = properties_Pressure,
	.get_IsInverted = properties_IsInverted,
	.get_IsEraser = properties_IsEraser,
	.get_Orientation = properties_Orientation,
	.get_XTilt = properties_XTilt,
	.get_YTilt = properties_YTilt,
	.get_Twist = properties_Twist,
	.get_TouchConfidence = properties_TouchConfidence,
	.get_IsPrimary = properties_IsPrimary,
	.get_IsInRange = properties_IsInRange,
	.get_IsCanceled = properties_IsCanceled,
	.get_IsBarrelButtonPressed = properties_IsBarrelButtonPressed,
	.get_IsLeftButtonPressed = properties_IsLeftButtonPressed,
	.get_IsRightButtonPressed = properties_IsRightButtonPressed,
	.get_IsMiddleButtonPressed = properties_IsMiddleButtonPressed,
	.get_IsXButton1Pressed = properties_IsXButton1Pressed,
	.get_IsXButton2Pressed = properties_IsXButton2Pressed,
	.get_MouseWheelDelta = properties_MouseWheelDelta,
	.get_IsHorizontalMouseWheel = properties_IsHorizontalMouseWheel,
	.get_PointerUpdateKind = properties_PointerUpdateKind,
};
HRESULT raven_mouse_point_create(const struct raven_mouse_sample *sample, IPointerPoint **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!sample) return E_POINTER;
	struct mouse_point *self = ALLOC(sizeof *self);
	if (!self) return E_OUTOFMEMORY;
	self->point.lpVtbl = &point_table;
	self->properties.lpVtbl = &properties_table;
	self->refs = 1;
	self->sample = *sample;
	InterlockedIncrement(&raven_xaml_objects);
	*out = &self->point;
	return S_OK;
}
