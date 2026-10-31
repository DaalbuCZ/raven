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

/* IVector<PointerPoint> snapshots: built once from retained samples, owning one reference per
 * element so a history handed to event args outlives the sample ring that produced it. */
struct point_vector {
	__FIVector_1_Windows__CUI__CInput__CPointerPoint vector;
	LONG refs;
	UINT32 count, capacity;
	IPointerPoint **items;
};
struct point_view {
	__FIVectorView_1_Windows__CUI__CInput__CPointerPoint view;
	LONG refs;
	UINT32 count;
	IPointerPoint **items;
};
static struct point_vector *vector_owner(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface)
{
	return CONTAINING_RECORD(iface, struct point_vector, vector);
}
static struct point_view *view_owner(__FIVectorView_1_Windows__CUI__CInput__CPointerPoint *iface)
{
	return CONTAINING_RECORD(iface, struct point_view, view);
}
static HRESULT WINAPI vector_query(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) &&
		!IsEqualIID(iid, &IID___FIVector_1_Windows__CUI__CInput__CPointerPoint)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&vector_owner(iface)->refs);
	return S_OK;
}
static ULONG WINAPI vector_addref(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface)
{
	return InterlockedIncrement(&vector_owner(iface)->refs);
}
static void vector_dispose(struct point_vector *self)
{
	while (self->count) {
		--self->count;
		IPointerPoint_Release(self->items[self->count]);
	}
	FREE(self->items);
	FREE(self);
	InterlockedDecrement(&raven_xaml_objects);
}
static ULONG WINAPI vector_release(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface)
{
	struct point_vector *self = vector_owner(iface);
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) vector_dispose(self);
	return (ULONG)refs;
}
static HRESULT WINAPI vector_iids(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return no_iids(count, iids);
}
static HRESULT WINAPI vector_class(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface, HSTRING *out)
{
	(void)iface;
	static const WCHAR name[] = L"Windows.Foundation.Collections.IVector`1<Windows.UI.Input.PointerPoint>";
	return class_name(name, out);
}
static HRESULT WINAPI vector_trust(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface, TrustLevel *out)
{
	(void)iface;
	return trust(out);
}
static HRESULT WINAPI vector_get_at(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface, UINT32 index, IPointerPoint **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	struct point_vector *self = vector_owner(iface);
	if (index >= self->count) return E_BOUNDS;
	*out = self->items[index];
	IPointerPoint_AddRef(*out);
	return S_OK;
}
static HRESULT WINAPI vector_size(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface, UINT32 *out)
{
	if (!out) return E_POINTER;
	*out = vector_owner(iface)->count;
	return S_OK;
}
static HRESULT WINAPI vector_index_of(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface,
	IPointerPoint *element, UINT32 *index, BOOLEAN *found)
{
	if (!index || !found) return E_POINTER;
	struct point_vector *self = vector_owner(iface);
	for (UINT32 i = 0; i < self->count; ++i) {
		if (self->items[i] == element) {
			*index = i;
			*found = TRUE;
			return S_OK;
		}
	}
	*index = 0;
	*found = FALSE;
	return S_OK;
}
static HRESULT vector_grow(struct point_vector *self, UINT32 capacity)
{
	if (capacity <= self->capacity) return S_OK;
	UINT32 sized = self->capacity ? self->capacity * 2 : 4;
	while (sized < capacity) sized *= 2;
	IPointerPoint **items = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, self->items, sized * sizeof *items);
	if (!items) return E_OUTOFMEMORY;
	self->items = items;
	self->capacity = sized;
	return S_OK;
}
/* Takes ownership of one reference; the collection's own Appends AddRef first. */
static HRESULT vector_take(struct point_vector *self, UINT32 index, IPointerPoint *value)
{
	HRESULT result = vector_grow(self, self->count + 1);
	if (FAILED(result)) return result;
	MoveMemory(self->items + index + 1, self->items + index, (self->count - index) * sizeof *self->items);
	self->items[index] = value;
	++self->count;
	return S_OK;
}
static HRESULT WINAPI vector_set_at(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface, UINT32 index, IPointerPoint *value)
{
	if (!value) return E_POINTER;
	struct point_vector *self = vector_owner(iface);
	if (index >= self->count) return E_BOUNDS;
	IPointerPoint_AddRef(value);
	IPointerPoint_Release(self->items[index]);
	self->items[index] = value;
	return S_OK;
}
static HRESULT WINAPI vector_insert_at(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface, UINT32 index, IPointerPoint *value)
{
	if (!value) return E_POINTER;
	struct point_vector *self = vector_owner(iface);
	if (index > self->count) return E_BOUNDS;
	IPointerPoint_AddRef(value);
	HRESULT result = vector_take(self, index, value);
	if (FAILED(result)) {
		IPointerPoint_Release(value);
		return result;
	}
	return S_OK;
}
static HRESULT WINAPI vector_remove_at(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface, UINT32 index)
{
	struct point_vector *self = vector_owner(iface);
	if (index >= self->count) return E_BOUNDS;
	IPointerPoint_Release(self->items[index]);
	MoveMemory(self->items + index, self->items + index + 1, (self->count - index - 1) * sizeof *self->items);
	--self->count;
	return S_OK;
}
static HRESULT WINAPI vector_append(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface, IPointerPoint *value)
{
	if (!value) return E_POINTER;
	struct point_vector *self = vector_owner(iface);
	IPointerPoint_AddRef(value);
	HRESULT result = vector_take(self, self->count, value);
	if (FAILED(result)) {
		IPointerPoint_Release(value);
		return result;
	}
	return S_OK;
}
static HRESULT WINAPI vector_remove_at_end(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface)
{
	struct point_vector *self = vector_owner(iface);
	if (!self->count) return E_BOUNDS;
	--self->count;
	IPointerPoint_Release(self->items[self->count]);
	return S_OK;
}
static HRESULT WINAPI vector_clear(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface)
{
	struct point_vector *self = vector_owner(iface);
	while (self->count) {
		--self->count;
		IPointerPoint_Release(self->items[self->count]);
	}
	return S_OK;
}
static HRESULT WINAPI vector_get_many(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface, UINT32 start_index,
	UINT32 items_size, IPointerPoint **items, UINT32 *out)
{
	if (!out) return E_POINTER;
	*out = 0;
	struct point_vector *self = vector_owner(iface);
	if (start_index > self->count) return E_BOUNDS;
	UINT32 copied = self->count - start_index;
	if (copied > items_size) copied = items_size;
	if (copied && !items) return E_POINTER;
	for (UINT32 i = 0; i < copied; ++i) {
		items[i] = self->items[start_index + i];
		IPointerPoint_AddRef(items[i]);
	}
	*out = copied;
	return S_OK;
}
static HRESULT WINAPI vector_replace_all(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface,
	UINT32 count, IPointerPoint **items)
{
	struct point_vector *self = vector_owner(iface);
	if (count && !items) return E_POINTER;
	IPointerPoint **fresh = NULL;
	if (count) {
		fresh = ALLOC(count * sizeof *fresh);
		if (!fresh) return E_OUTOFMEMORY;
		for (UINT32 i = 0; i < count; ++i) {
			fresh[i] = items[i];
			IPointerPoint_AddRef(fresh[i]);
		}
	}
	while (self->count) {
		--self->count;
		IPointerPoint_Release(self->items[self->count]);
	}
	FREE(self->items);
	self->items = fresh;
	self->capacity = count;
	self->count = count;
	return S_OK;
}
static HRESULT WINAPI view_query(__FIVectorView_1_Windows__CUI__CInput__CPointerPoint *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) &&
		!IsEqualIID(iid, &IID___FIVectorView_1_Windows__CUI__CInput__CPointerPoint)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&view_owner(iface)->refs);
	return S_OK;
}
static ULONG WINAPI view_addref(__FIVectorView_1_Windows__CUI__CInput__CPointerPoint *iface)
{
	return InterlockedIncrement(&view_owner(iface)->refs);
}
static ULONG WINAPI view_release(__FIVectorView_1_Windows__CUI__CInput__CPointerPoint *iface)
{
	struct point_view *self = view_owner(iface);
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		for (UINT32 i = 0; i < self->count; ++i) IPointerPoint_Release(self->items[i]);
		FREE(self->items);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}
static HRESULT WINAPI view_iids(__FIVectorView_1_Windows__CUI__CInput__CPointerPoint *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return no_iids(count, iids);
}
static HRESULT WINAPI view_class(__FIVectorView_1_Windows__CUI__CInput__CPointerPoint *iface, HSTRING *out)
{
	(void)iface;
	static const WCHAR name[] = L"Windows.Foundation.Collections.IVectorView`1<Windows.UI.Input.PointerPoint>";
	return class_name(name, out);
}
static HRESULT WINAPI view_trust(__FIVectorView_1_Windows__CUI__CInput__CPointerPoint *iface, TrustLevel *out)
{
	(void)iface;
	return trust(out);
}
static HRESULT WINAPI view_get_at(__FIVectorView_1_Windows__CUI__CInput__CPointerPoint *iface, UINT32 index, IPointerPoint **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	struct point_view *self = view_owner(iface);
	if (index >= self->count) return E_BOUNDS;
	*out = self->items[index];
	IPointerPoint_AddRef(*out);
	return S_OK;
}
static HRESULT WINAPI view_size(__FIVectorView_1_Windows__CUI__CInput__CPointerPoint *iface, UINT32 *out)
{
	if (!out) return E_POINTER;
	*out = view_owner(iface)->count;
	return S_OK;
}
static HRESULT WINAPI view_index_of(__FIVectorView_1_Windows__CUI__CInput__CPointerPoint *iface,
	IPointerPoint *element, UINT32 *index, BOOLEAN *found)
{
	if (!index || !found) return E_POINTER;
	struct point_view *self = view_owner(iface);
	for (UINT32 i = 0; i < self->count; ++i) {
		if (self->items[i] == element) {
			*index = i;
			*found = TRUE;
			return S_OK;
		}
	}
	*index = 0;
	*found = FALSE;
	return S_OK;
}
static HRESULT WINAPI view_get_many(__FIVectorView_1_Windows__CUI__CInput__CPointerPoint *iface, UINT32 start_index,
	UINT32 items_size, IPointerPoint **items, UINT32 *out)
{
	if (!out) return E_POINTER;
	*out = 0;
	struct point_view *self = view_owner(iface);
	if (start_index > self->count) return E_BOUNDS;
	UINT32 copied = self->count - start_index;
	if (copied > items_size) copied = items_size;
	if (copied && !items) return E_POINTER;
	for (UINT32 i = 0; i < copied; ++i) {
		items[i] = self->items[start_index + i];
		IPointerPoint_AddRef(items[i]);
	}
	*out = copied;
	return S_OK;
}
static const __FIVectorView_1_Windows__CUI__CInput__CPointerPointVtbl view_table = {
	view_query, view_addref, view_release, view_iids, view_class, view_trust,
	view_get_at, view_size, view_index_of, view_get_many
};
static HRESULT WINAPI vector_get_view(__FIVector_1_Windows__CUI__CInput__CPointerPoint *iface,
	__FIVectorView_1_Windows__CUI__CInput__CPointerPoint **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	struct point_vector *self = vector_owner(iface);
	struct point_view *view = ALLOC(sizeof *view);
	if (!view) return E_OUTOFMEMORY;
	if (self->count) {
		view->items = ALLOC(self->count * sizeof *view->items);
		if (!view->items) {
			FREE(view);
			return E_OUTOFMEMORY;
		}
		for (UINT32 i = 0; i < self->count; ++i) {
			view->items[i] = self->items[i];
			IPointerPoint_AddRef(view->items[i]);
		}
	}
	view->view.lpVtbl = &view_table;
	view->refs = 1;
	view->count = self->count;
	InterlockedIncrement(&raven_xaml_objects);
	*out = &view->view;
	return S_OK;
}
static const __FIVector_1_Windows__CUI__CInput__CPointerPointVtbl vector_table = {
	vector_query, vector_addref, vector_release, vector_iids, vector_class, vector_trust,
	vector_get_at, vector_size, vector_get_view, vector_index_of,
	vector_set_at, vector_insert_at, vector_remove_at, vector_append, vector_remove_at_end, vector_clear,
	vector_get_many, vector_replace_all
};
__declspec(dllexport) HRESULT raven_pointer_history_create(const struct raven_mouse_sample *samples, UINT32 count,
	__FIVector_1_Windows__CUI__CInput__CPointerPoint **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (count && !samples) return E_POINTER;
	struct point_vector *self = ALLOC(sizeof *self);
	if (!self) return E_OUTOFMEMORY;
	self->vector.lpVtbl = &vector_table;
	self->refs = 1;
	if (count) {
		self->items = ALLOC(count * sizeof *self->items);
		if (!self->items) {
			FREE(self);
			return E_OUTOFMEMORY;
		}
		self->capacity = count;
		for (UINT32 i = 0; i < count; ++i) {
			HRESULT result = raven_mouse_point_create(&samples[i], &self->items[i]);
			if (FAILED(result)) {
				while (i) {
					--i;
					IPointerPoint_Release(self->items[i]);
				}
				FREE(self->items);
				FREE(self);
				return result;
			}
		}
		self->count = count;
	}
	InterlockedIncrement(&raven_xaml_objects);
	*out = &self->vector;
	return S_OK;
}
