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
#include <stddef.h>
#include <string.h>
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
/* Where a device's PhysicalDeviceRect comes from: the desktop surface the cursor roams, a rect the
 * host enumeration reported, or nothing the engine could learn (the getter answers E_NOTIMPL). */
enum device_rect_kind { device_rect_none, device_rect_desktop, device_rect_fixed };
struct pointer_device {
	IPointerDevice device;
	LONG refs;
	PointerDeviceType type;
	boolean integrated_known, integrated;
	boolean contacts_known;
	UINT32 contacts;
	enum device_rect_kind rect;
	Rect physical;
};
static const IPointerDeviceVtbl device_table;
/* The engine's single desktop mouse: every PointerPoint hands out this object (point_device), so
 * GetPointerDevice(1) must answer this same identity — PointerId 1 everywhere, matching the events. */
static struct pointer_device mouse_device_storage = {
	{&device_table}, 0, PointerDeviceType_Mouse, TRUE, FALSE, TRUE, 1, device_rect_desktop, {0},
};
static IPointerDevice *mouse_device = &mouse_device_storage.device;
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
	*out = mouse_device;
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
static struct pointer_device *device_owner(IPointerDevice *iface)
{
	return CONTAINING_RECORD(iface, struct pointer_device, device);
}
static ULONG WINAPI device_addref(IPointerDevice *iface)
{
	struct pointer_device *self = device_owner(iface);
	InterlockedIncrement(&raven_xaml_objects);
	return InterlockedIncrement(&self->refs);
}
static ULONG WINAPI device_release(IPointerDevice *iface)
{
	struct pointer_device *self = device_owner(iface);
	LONG refs = InterlockedDecrement(&self->refs);
	InterlockedDecrement(&raven_xaml_objects);
	if (!refs && self != &mouse_device_storage) FREE(self); /* the catalog's own reference keeps enumerated devices alive */
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
	*out = device_owner(iface)->type;
	return S_OK;
}
static HRESULT WINAPI device_integrated(IPointerDevice *iface, boolean *out)
{
	if (!out) return E_POINTER;
	/* Raw input does not say whether a digitizer is built into a display; only the sources that
	 * answer (user32's pointer table, the mouse) set integrated_known, the rest stay a documented
	 * failure rather than a guess. */
	if (!device_owner(iface)->integrated_known) return E_NOTIMPL;
	*out = device_owner(iface)->integrated;
	return S_OK;
}
static HRESULT WINAPI device_contacts(IPointerDevice *iface, UINT32 *out)
{
	if (!out) return E_POINTER;
	/* MaxContacts needs the host's active-contact count; a mouse reports one contact, touch and pen
	 * only when the host table provided it. */
	if (!device_owner(iface)->contacts_known) return E_NOTIMPL;
	*out = device_owner(iface)->contacts;
	return S_OK;
}
static HRESULT WINAPI device_physical_rect(IPointerDevice *iface, Rect *out)
{
	if (!out) return E_POINTER;
	struct pointer_device *self = device_owner(iface);
	if (self->rect == device_rect_none) return E_NOTIMPL;
	if (self->rect == device_rect_fixed) {
		*out = self->physical;
		return S_OK;
	}
	/* A mouse has no digitizer extent of its own; the surface its cursor roams is the virtual
	 * screen, which is the emulated desktop's honest answer. */
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
	/* No source in the emulated desktop exposes a HID report descriptor: the mouse is a Win32 cursor
	 * device, not a HID collection, so there is no usage list to answer honestly. */
	return E_NOTIMPL;
}
static const IPointerDeviceVtbl device_table = {
	device_query, device_addref, device_release, device_iids, device_class, device_trust,
	device_type, device_integrated, device_contacts, device_physical_rect, device_screen_rect, device_usages
};
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

/* Device enumeration behind the PointerDevice statics. The catalog is built once from what the host
 * reports and kept for the process, so every GetPointerDevices view answers the same object
 * identities; the engine's mouse is always present because its event pipeline synthesizes mouse
 * input even when the host's tables list nothing (wine desktops often list no raw devices). */
static HRESULT device_create(PointerDeviceType type, boolean integrated_known, boolean integrated,
	boolean contacts_known, UINT32 contacts, enum device_rect_kind rect, const Rect *physical,
	struct pointer_device **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	struct pointer_device *self = ALLOC(sizeof *self);
	if (!self) return E_OUTOFMEMORY;
	self->device.lpVtbl = &device_table;
	self->refs = 1; /* the catalog owns this reference */
	self->type = type;
	self->integrated_known = integrated_known;
	self->integrated = integrated;
	self->contacts_known = contacts_known;
	self->contacts = contacts;
	self->rect = rect;
	if (physical) self->physical = *physical;
	InterlockedIncrement(&raven_xaml_objects);
	*out = self;
	return S_OK;
}
static HRESULT device_create_mouse(struct pointer_device **out)
{
	/* Documented mouse values: never integrated, a single contact, the desktop surface as extent. */
	return device_create(PointerDeviceType_Mouse, TRUE, FALSE, TRUE, 1, device_rect_desktop, NULL, out);
}
struct device_list {
	struct pointer_device **items;
	UINT32 count, capacity;
};
static HRESULT device_list_push(struct device_list *list, struct pointer_device *device)
{
	if (list->count == list->capacity) {
		UINT32 sized = list->capacity ? list->capacity * 2 : 4;
		/* HeapReAlloc refuses a NULL block on some hosts, so the first growth allocates fresh. */
		struct pointer_device **items = list->items ?
			HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, list->items, sized * sizeof *items) :
			ALLOC(sized * sizeof *items);
		if (!items) return E_OUTOFMEMORY;
		list->items = items;
		list->capacity = sized;
	}
	list->items[list->count++] = device;
	return S_OK;
}
/* Maps a HID digitizer usage (HID usage tables, digitizer page 0x0D) to a pointer device type. */
static PointerDeviceType digitizer_type(UINT32 usage)
{
	return (usage == 0x04 || usage == 0x05) ? PointerDeviceType_Touch : PointerDeviceType_Pen;
}
static void catalog_scan_touch_user32(struct device_list *list)
{
	/* user32's GetPointerDevices is the host table the real statics sit on: it reports the device
	 * type, integration and active-contact count directly. */
	UINT32 count = 0;
	if (!GetPointerDevices(&count, NULL) || !count) return;
	POINTER_DEVICE_INFO *devices = ALLOC(count * sizeof *devices);
	if (!devices) return;
	if (GetPointerDevices(&count, devices)) {
		for (UINT32 i = 0; i < count; ++i) {
			boolean integrated_known = TRUE;
			boolean integrated = TRUE;
			PointerDeviceType type;
			/* POINTER_DEVICE_TYPE has no mouse member: this table only carries touch and pen. */
			switch (devices[i].pointerDeviceType) {
			case POINTER_DEVICE_TYPE_INTEGRATED_PEN:
				type = PointerDeviceType_Pen;
				break;
			case POINTER_DEVICE_TYPE_EXTERNAL_PEN:
				type = PointerDeviceType_Pen;
				integrated = FALSE;
				break;
			case POINTER_DEVICE_TYPE_TOUCH_PAD:
				type = PointerDeviceType_Touch;
				break;
			case POINTER_DEVICE_TYPE_TOUCH:
				type = PointerDeviceType_Touch;
				break;
			default:
				continue; /* unknown host type: leave it out rather than invent a mapping */
			}
			Rect rect;
			enum device_rect_kind rect_kind = device_rect_none;
			RECT physical, display;
			if (GetPointerDeviceRects(devices[i].device, &physical, &display)) {
				rect.X = (FLOAT)physical.left;
				rect.Y = (FLOAT)physical.top;
				rect.Width = (FLOAT)(physical.right - physical.left);
				rect.Height = (FLOAT)(physical.bottom - physical.top);
				rect_kind = device_rect_fixed;
			}
			struct pointer_device *device = NULL;
			if (FAILED(device_create(type, integrated_known, integrated, TRUE, devices[i].maxActiveContacts,
				rect_kind, rect_kind == device_rect_fixed ? &rect : NULL, &device)))
				continue;
			if (FAILED(device_list_push(list, device))) {
				IPointerDevice_Release(&device->device);
			}
		}
	}
	FREE(devices);
}
static void catalog_scan_touch_hid(struct device_list *list)
{
	/* Fallback when the host pointer table is absent: raw-input HID devices on the digitizer usage
	 * page, mapped by usage (0x01 puck and 0x02 pen answer Pen, 0x04 touch screen and 0x05 touch pad
	 * answer Touch). Raw input exposes no integration, contact count or extent, so those getters
	 * stay a documented E_NOTIMPL for devices from this path. */
	UINT raw_count = 0;
	if (GetRawInputDeviceList(NULL, &raw_count, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1 || !raw_count) return;
	RAWINPUTDEVICELIST *raw = ALLOC(raw_count * sizeof *raw);
	if (!raw) return;
	UINT filled = GetRawInputDeviceList(raw, &raw_count, sizeof(RAWINPUTDEVICELIST));
	for (UINT i = 0; filled != (UINT)-1 && i < filled; ++i) {
		if (raw[i].dwType != RIM_TYPEHID) continue;
		RID_DEVICE_INFO info;
		UINT size = sizeof info;
		info.cbSize = sizeof info;
		if (GetRawInputDeviceInfoW(raw[i].hDevice, RIDI_DEVICEINFO, &info, &size) == (UINT)-1) continue;
		UINT32 usage_page = info.hid.usUsagePage, usage = info.hid.usUsage;
		if (usage_page != 0x0d || (usage != 0x01 && usage != 0x02 && usage != 0x04 && usage != 0x05)) continue;
		struct pointer_device *device = NULL;
		if (FAILED(device_create(digitizer_type(usage), FALSE, FALSE, FALSE, 0, device_rect_none, NULL, &device)))
			continue;
		if (FAILED(device_list_push(list, device)))
			IPointerDevice_Release(&device->device);
	}
	FREE(raw);
}
static struct pointer_device **device_catalog;
static UINT32 device_catalog_count;
static void catalog_ensure(void)
{
	if (device_catalog) return;
	struct device_list list = {0};
	boolean mouse_present = FALSE;
	/* The mouse: every raw-input mouse the host lists is a pointer device. The first is the engine's
	 * mouse (the identity the events hand out); further mice are distinct devices with the same
	 * documented mouse values, since the engine's event stream cannot distinguish them. */
	UINT raw_count = 0;
	if (GetRawInputDeviceList(NULL, &raw_count, sizeof(RAWINPUTDEVICELIST)) != (UINT)-1 && raw_count) {
		RAWINPUTDEVICELIST *raw = ALLOC(raw_count * sizeof *raw);
		if (raw) {
			UINT filled = GetRawInputDeviceList(raw, &raw_count, sizeof(RAWINPUTDEVICELIST));
			for (UINT i = 0; filled != (UINT)-1 && i < filled; ++i) {
				if (raw[i].dwType != RIM_TYPEMOUSE) continue;
				if (!mouse_present) {
					/* the catalog carries the static mouse without owning a reference */
					if (SUCCEEDED(device_list_push(&list, &mouse_device_storage))) mouse_present = TRUE;
					continue;
				}
				struct pointer_device *device = NULL;
				if (SUCCEEDED(device_create_mouse(&device))) {
					if (SUCCEEDED(device_list_push(&list, device))) continue;
					IPointerDevice_Release(&device->device);
				}
			}
			FREE(raw);
		}
	}
	if (!mouse_present) {
		/* Host enumeration reported no mouse; the engine's event pipeline synthesizes mouse input
		 * regardless, so the device exists and GetPointerDevice(1) must resolve. */
		if (SUCCEEDED(device_list_push(&list, &mouse_device_storage))) mouse_present = TRUE;
	}
	UINT32 before = list.count;
	catalog_scan_touch_user32(&list);
	if (list.count == before) catalog_scan_touch_hid(&list);
	/* Never a wrong count: with the mouse guaranteed above, the catalog is non-empty even when the
	 * host reports nothing. */
	if (InterlockedCompareExchangePointer((void *)&device_catalog, list.items, NULL)) {
		/* Another thread published first: drop this build's owned references. */
		while (list.count) {
			--list.count;
			if (list.items[list.count] != &mouse_device_storage) IPointerDevice_Release(&list.items[list.count]->device);
		}
		FREE(list.items);
	} else {
		device_catalog_count = list.count;
	}
}
struct device_statics {
	IPointerDeviceStatics statics;
	LONG refs;
};
static HRESULT WINAPI statics_query(IPointerDeviceStatics *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) &&
		!IsEqualIID(iid, &IID_IPointerDeviceStatics)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct device_statics *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI statics_addref(IPointerDeviceStatics *iface)
{
	InterlockedIncrement(&raven_xaml_objects);
	return InterlockedIncrement(&((struct device_statics *)iface)->refs);
}
static ULONG WINAPI statics_release(IPointerDeviceStatics *iface)
{
	LONG refs = InterlockedDecrement(&((struct device_statics *)iface)->refs);
	InterlockedDecrement(&raven_xaml_objects);
	return (ULONG)refs;
}
static HRESULT WINAPI statics_iids(IPointerDeviceStatics *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI statics_class(IPointerDeviceStatics *iface, HSTRING *out)
{
	(void)iface;
	return class_name(L"Windows.Devices.Input.PointerDeviceStatics", out);
}
static HRESULT WINAPI statics_trust(IPointerDeviceStatics *iface, TrustLevel *out)
{
	(void)iface;
	return trust(out);
}
static HRESULT WINAPI statics_GetPointerDevice(IPointerDeviceStatics *iface, UINT32 pointer_id, IPointerDevice **out)
{
	(void)iface;
	if (!out) return E_POINTER;
	*out = NULL;
	/* Every pointer the engine raises reports PointerId 1 (see point_id), so 1 is the only id a
	 * caller can observe; anything else is an unknown pointer. The real statics reject unknown ids —
	 * this answers E_INVALIDARG, the engine's contract for an out-of-space argument. */
	if (pointer_id != 1) return E_INVALIDARG;
	*out = mouse_device;
	IPointerDevice_AddRef(*out);
	return S_OK;
}
struct device_view {
	__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice view;
	LONG refs;
	UINT32 count;
	IPointerDevice **items;
};
static struct device_view *device_view_owner(__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice *iface)
{
	return CONTAINING_RECORD(iface, struct device_view, view);
}
static HRESULT WINAPI device_view_query(__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) &&
		!IsEqualIID(iid, &IID___FIVectorView_1_Windows__CDevices__CInput__CPointerDevice)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&device_view_owner(iface)->refs);
	return S_OK;
}
static ULONG WINAPI device_view_addref(__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice *iface)
{
	return InterlockedIncrement(&device_view_owner(iface)->refs);
}
static ULONG WINAPI device_view_release(__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice *iface)
{
	struct device_view *self = device_view_owner(iface);
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		for (UINT32 i = 0; i < self->count; ++i) IPointerDevice_Release(self->items[i]);
		FREE(self->items);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}
static HRESULT WINAPI device_view_iids(__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return no_iids(count, iids);
}
static HRESULT WINAPI device_view_class(__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice *iface, HSTRING *out)
{
	(void)iface;
	static const WCHAR name[] = L"Windows.Foundation.Collections.IVectorView`1<Windows.Devices.Input.PointerDevice>";
	return class_name(name, out);
}
static HRESULT WINAPI device_view_trust(__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice *iface, TrustLevel *out)
{
	(void)iface;
	return trust(out);
}
static HRESULT WINAPI device_view_get_at(__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice *iface, UINT32 index,
	IPointerDevice **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	struct device_view *self = device_view_owner(iface);
	if (index >= self->count) return E_BOUNDS;
	*out = self->items[index];
	IPointerDevice_AddRef(*out);
	return S_OK;
}
static HRESULT WINAPI device_view_size(__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice *iface, UINT32 *out)
{
	if (!out) return E_POINTER;
	*out = device_view_owner(iface)->count;
	return S_OK;
}
static HRESULT WINAPI device_view_index_of(__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice *iface,
	IPointerDevice *element, UINT32 *index, BOOLEAN *found)
{
	if (!index || !found) return E_POINTER;
	struct device_view *self = device_view_owner(iface);
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
static HRESULT WINAPI device_view_get_many(__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice *iface,
	UINT32 start_index, UINT32 items_size, IPointerDevice **items, UINT32 *out)
{
	if (!out) return E_POINTER;
	*out = 0;
	struct device_view *self = device_view_owner(iface);
	if (start_index > self->count) return E_BOUNDS;
	UINT32 copied = self->count - start_index;
	if (copied > items_size) copied = items_size;
	if (copied && !items) return E_POINTER;
	for (UINT32 i = 0; i < copied; ++i) {
		items[i] = self->items[start_index + i];
		IPointerDevice_AddRef(items[i]);
	}
	*out = copied;
	return S_OK;
}
static const __FIVectorView_1_Windows__CDevices__CInput__CPointerDeviceVtbl device_view_table = {
	device_view_query, device_view_addref, device_view_release, device_view_iids, device_view_class,
	device_view_trust, device_view_get_at, device_view_size, device_view_index_of, device_view_get_many
};
static HRESULT WINAPI statics_GetPointerDevices(IPointerDeviceStatics *iface,
	__FIVectorView_1_Windows__CDevices__CInput__CPointerDevice **out)
{
	(void)iface;
	if (!out) return E_POINTER;
	*out = NULL;
	catalog_ensure();
	struct device_view *view = ALLOC(sizeof *view);
	if (!view) return E_OUTOFMEMORY;
	if (device_catalog_count) {
		view->items = ALLOC(device_catalog_count * sizeof *view->items);
		if (!view->items) {
			FREE(view);
			return E_OUTOFMEMORY;
		}
		for (UINT32 i = 0; i < device_catalog_count; ++i) {
			view->items[i] = &device_catalog[i]->device;
			IPointerDevice_AddRef(view->items[i]);
		}
	}
	view->view.lpVtbl = &device_view_table;
	view->refs = 1;
	view->count = device_catalog_count;
	InterlockedIncrement(&raven_xaml_objects);
	*out = &view->view;
	return S_OK;
}
static const IPointerDeviceStaticsVtbl statics_table = {
	statics_query, statics_addref, statics_release, statics_iids, statics_class, statics_trust,
	statics_GetPointerDevice, statics_GetPointerDevices
};
/* The class answers the activation chain as a statics-only runtime class: one COM object carries the statics and a
 * faulting activation part, the way the DependencyProperty factory does. The statics part sits at the front, so the
 * statics methods' plain owner casts hold; the activation part reaches the owner by its own offset. */
struct device_factory {
	struct device_statics statics;
	struct { const IActivationFactoryVtbl *lpVtbl; } activation;
};
static HRESULT WINAPI device_activation_query(IActivationFactory *iface, REFIID iid, void **out)
{
	struct device_factory *self = (struct device_factory *)((char *)iface - offsetof(struct device_factory, activation));
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) ||
		IsEqualIID(iid, &IID_IActivationFactory)) {
		*out = iface;
	} else if (IsEqualIID(iid, &IID_IPointerDeviceStatics)) {
		return IPointerDeviceStatics_QueryInterface(&self->statics.statics, iid, out);
	} else {
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->statics.refs);
	return S_OK;
}
static ULONG WINAPI device_activation_addref(IActivationFactory *iface)
{
	struct device_factory *self = (struct device_factory *)((char *)iface - offsetof(struct device_factory, activation));
	return (ULONG)InterlockedIncrement(&self->statics.refs);
}
static ULONG WINAPI device_activation_release(IActivationFactory *iface)
{
	struct device_factory *self = (struct device_factory *)((char *)iface - offsetof(struct device_factory, activation));
	return (ULONG)InterlockedDecrement(&self->statics.refs);
}
static HRESULT WINAPI device_activation_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return no_iids(count, iids);
}
static HRESULT WINAPI device_activation_name(IActivationFactory *iface, HSTRING *out)
{
	(void)iface;
	return class_name(L"Windows.Devices.Input.PointerDevice", out);
}
static HRESULT WINAPI device_activation_activate(IActivationFactory *iface, IInspectable **out)
{
	/* No default activation: devices exist only in the host's pointer table, never constructed by applications. */
	(void)iface;
	if (out)
		*out = NULL;
	return E_NOTIMPL;
}
static HRESULT WINAPI device_activation_trust(IActivationFactory *iface, TrustLevel *out)
{
	(void)iface;
	return trust(out);
}
static const IActivationFactoryVtbl device_activation_table = {
	device_activation_query, device_activation_addref, device_activation_release, device_activation_iids,
	device_activation_name, device_activation_trust, device_activation_activate
};
static struct device_factory device_factory_storage = {{{&statics_table}, 0}, {&device_activation_table}};
__declspec(dllexport) HRESULT raven_pointer_device_statics(IPointerDeviceStatics **out)
{
	if (!out) return E_POINTER;
	*out = &device_factory_storage.statics.statics;
	IPointerDeviceStatics_AddRef(*out);
	return S_OK;
}
static const WCHAR pointer_device_class[] = L"Windows.Devices.Input.PointerDevice";
/* The activation-chain entry: answers only the class name, the way the other chain members do. */
HRESULT raven_pointer_device_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (!text || length != ARRAYSIZE(pointer_device_class) - 1 ||
		memcmp(text, pointer_device_class, length * sizeof(WCHAR)) != 0)
		return CLASS_E_CLASSNOTAVAILABLE;
	InterlockedIncrement(&device_factory_storage.statics.refs);
	*out = (IActivationFactory *)&device_factory_storage.activation;
	return S_OK;
}
