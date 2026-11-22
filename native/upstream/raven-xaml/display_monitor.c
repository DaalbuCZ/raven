/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.Devices.Display.DisplayMonitor class of the Windows Runtime.
 *
 * A packaged application asks for the monitor a device id or interface id names: the factory serves the statics
 * interface the activation query names (GetDeviceSelector, FromIdAsync, FromInterfaceIdAsync — the three members the
 * pinned metadata declares, the contract the guest's Shapr.Native.dll asks for), and each instance answers the
 * default interface's identity and geometry members. The values are the ones Raven's own display model gives the
 * guest: the one virtual monitor the CCD APIs describe (adapter LUID 1, 1920x1080@60, "Raven Monitor" on the
 * \\?\DISPLAY#RAVEN0000# device path) and the EDID register_monitor_edid seeds — the 53 by 30 centimetre panel size
 * the raw DPI derives from, the sRGB primaries the chromaticity block encodes, and the descriptor bytes GetDescriptor
 * reads back from the registry the seed lives in. The statics' async members complete inside the creation call the
 * way storage.c's operations do: Raven has one monitor, so the await never suspends. Every member a guest reaches is
 * named in Raven's trace, so the next run tells which ones the application reads. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <stddef.h>
#include "xaml_internal.h"
#include "windows_devices_display.h"

typedef __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics statics1;
typedef __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStaticsVtbl statics1Vtbl;
typedef __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor monitor1;
typedef __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorVtbl monitor1Vtbl;
typedef __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2 monitor2;
typedef __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2Vtbl monitor2Vtbl;
typedef __FIReference_1_Windows__CFoundation__CSize size_ref_i;
typedef __FIReference_1_Windows__CFoundation__CSizeVtbl size_ref_iVtbl;

static const WCHAR class_name[] = L"Windows.Devices.Display.DisplayMonitor";
static const WCHAR size_ref_type_name[] = L"Windows.Foundation.IReference`1<Windows.Foundation.Size>";

/* The one monitor Raven's display model gives the guest, as the CCD APIs answer it: the target's device path (the
 * id DisplayMonitor answers DeviceId with), its friendly name, and the adapter the path hangs from. */
static const WCHAR monitor_device_path[] =
	L"\\\\?\\DISPLAY#RAVEN0000#5&2f5f805e&0&UID256#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}";
static const WCHAR monitor_display_name_text[] = L"Raven Monitor";
static const WCHAR adapter_device_path[] = L"\\\\.\\DISPLAY1";

/* FromIdAsync matches the device interface path's enumerator and instance (the part before the interface GUID); a
 * guest that asks for another id is answered the failure Windows' own resolution gives. */
static const WCHAR device_path_prefix[] = L"\\\\?\\DISPLAY#RAVEN0000#5&2f5f805e&0&UID256";

/* The monitor interface class the selector names and FromInterfaceIdAsync matches: GUID_DEVINTERFACE_MONITOR, the
 * identifier the modeled device path itself carries and raven-setupapi serves one interface of. */
static const WCHAR monitor_interface_id[] = L"{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}";
static const WCHAR device_selector[] = L"System.Devices.InterfaceClassGuid:=\"{E6F07B5F-EE97-4A90-B076-33F57BF4EAA7}\" "
	L"AND System.Devices.InterfaceEnabled:=System.StructuredQueryType.Boolean#True";

/* The registry value register_monitor_edid seeds the descriptor bytes in: the honest source GetDescriptor reads. */
static const WCHAR edid_key_path[] = L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\RAVEN0000\\5&2f5f805e&0&UID256\\Device Parameters";

/* The panel the EDID describes: 53 by 30 centimetres, at the desktop's 1920x1080. The raw DPI is the resolution over
 * the physical inches; the physical size in inches is the panel size converted. */
enum { panel_width_mm = 530, panel_height_mm = 300, panel_pixels_x = 1920, panel_pixels_y = 1080 };

/* Windows.Devices.Display's enumerations, as the metadata gives them: the modeled monitor connects internally (the
 * CCD target's output technology), is a standard-use panel over an unknown connector, and its chromaticity block is
 * the sRGB set the EDID encodes. */
enum { connection_internal = 0 };
enum { connector_unknown = 0 };
enum { usage_standard = 0 };
enum { descriptor_edid = 0, descriptor_display_id = 1 };

#ifndef HRESULT_FROM_WIN32
#define HRESULT_FROM_WIN32(x) ((HRESULT)(x) <= 0 ? (HRESULT)(x) : (HRESULT)(((x) & 0x0000ffff) | 0x80070000))
#endif
#ifndef E_NOT_FOUND_NATIVE
#define E_NOT_FOUND_NATIVE ((HRESULT)0x80070490) /* HRESULT_FROM_WIN32(ERROR_NOT_FOUND) */
#endif

/* The one member-name trace ravenhost.dll carries to the host, as raven-shell's activation trace does; nothing is
 * emitted when the host is absent. */
static void trace_member(const char *member)
{
	static void (WINAPI *raven_trace)(const char *);
	static BOOL looked;
	char line[160] = "raven-xaml: DisplayMonitor.";
	size_t at = 27, length;
	if (!looked) {
		raven_trace = (void (WINAPI *)(const char *))GetProcAddress(LoadLibraryW(L"ravenhost.dll"), "RavenTrace");
		looked = TRUE;
	}
	if (!raven_trace)
		return;
	for (length = 0; member[length] && at < ARRAYSIZE(line) - 1; ++length)
		line[at++] = member[length];
	line[at] = 0;
	raven_trace(line);
}

/* The interface identifier a query refused, in short form: the next run tells which face the guest wanted and the
 * object does not serve. */
static void trace_query_refuse(const char *face, REFIID iid)
{
	char line[96];
	wsprintfA(line, "refuse %08x-%04x-%04x", (unsigned int)iid->Data1, (unsigned int)iid->Data2, (unsigned int)iid->Data3);
	trace_member(line);
}

static HRESULT inspectable_iids(ULONG *count, IID **iids)
{
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}

static HRESULT inspectable_trust(TrustLevel *level)
{
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

static HRESULT runtime_class_name(HSTRING *name, const WCHAR *text)
{
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), name);
}

static int equal_iid(REFIID left, const IID *right)
{
	return left->Data1 == right->Data1 && left->Data2 == right->Data2 && left->Data3 == right->Data3 &&
		left->Data4[0] == right->Data4[0] && left->Data4[1] == right->Data4[1] &&
		left->Data4[2] == right->Data4[2] && left->Data4[3] == right->Data4[3] &&
		left->Data4[4] == right->Data4[4] && left->Data4[5] == right->Data4[5] &&
		left->Data4[6] == right->Data4[6] && left->Data4[7] == right->Data4[7];
}

/* The query, refcounting and release the objects share; each object's vtable sits first and its reference count
 * directly after, so the release frees the block the object opened. */
static ULONG shared_addref(LONG *refs)
{
	return (ULONG)InterlockedIncrement(refs);
}

/* ---- The monitor object: the one monitor the display model gives, answered member by member. ---- */

struct monitor_object;
static HRESULT WINAPI monitor_query_any(struct monitor_object *self, REFIID iid, void **out);
static ULONG monitor_release_refs(struct monitor_object *self);

struct monitor_object {
	monitor1 iface;
	LONG refs;
	struct raven_weak_source weak;
	struct monitor_part2 { const monitor2Vtbl *lpVtbl; struct monitor_object *owner; } two;
};

/* The object answers the agile marker: every member reads immutable model values and the registry-held descriptor,
 * and none of them touches thread-affine state. */
static HRESULT WINAPI monitor_query_any(struct monitor_object *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor) ||
		equal_iid(iid, &IID_IAgileObject))
		*out = &self->iface;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2))
		*out = &self->two;
	else if (equal_iid(iid, &IID_IWeakReferenceSource))
		*out = &self->weak;
	else {
		trace_query_refuse("monitor", iid);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static HRESULT WINAPI monitor_query(monitor1 *iface, REFIID iid, void **out)
{
	return monitor_query_any((struct monitor_object *)iface, iid, out);
}

static ULONG WINAPI monitor_addref(monitor1 *iface)
{
	return shared_addref(&((struct monitor_object *)iface)->refs);
}

static ULONG WINAPI monitor_release(monitor1 *iface)
{
	return monitor_release_refs((struct monitor_object *)iface);
}

static HRESULT WINAPI monitor_iids(monitor1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI monitor_name(monitor1 *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, class_name);
}

static HRESULT WINAPI monitor_trust(monitor1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI monitor_string_member(monitor1 *iface, const WCHAR *value, const char *member, HSTRING *out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	trace_member(member);
	return runtime_class_name(out, value);
}

static HRESULT WINAPI monitor_device_id(monitor1 *iface, HSTRING *value)
{
	return monitor_string_member(iface, monitor_device_path, "get_DeviceId", value);
}

static HRESULT WINAPI monitor_display_name(monitor1 *iface, HSTRING *value)
{
	return monitor_string_member(iface, monitor_display_name_text, "get_DisplayName", value);
}

static HRESULT WINAPI monitor_adapter_device_id(monitor1 *iface, HSTRING *value)
{
	return monitor_string_member(iface, adapter_device_path, "get_DisplayAdapterDeviceId", value);
}

static HRESULT WINAPI monitor_connection_kind(monitor1 *iface, int32_t *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_ConnectionKind");
	/* the CCD target's output technology: internal */
	*value = connection_internal;
	return S_OK;
}

static HRESULT WINAPI monitor_physical_connector(monitor1 *iface, int32_t *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_PhysicalConnector");
	/* the modeled target carries no connector metadata */
	*value = connector_unknown;
	return S_OK;
}

static HRESULT WINAPI monitor_adapter_id(monitor1 *iface, __x_ABI_CWindows_CGraphics_CDisplayAdapterId *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_DisplayAdapterId");
	/* the adapter LUID the CCD path names: 1 */
	value->LowPart = 1;
	value->HighPart = 0;
	return S_OK;
}

static HRESULT WINAPI monitor_adapter_target_id(monitor1 *iface, UINT32 *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_DisplayAdapterTargetId");
	/* the target id the device path's UID256 carries */
	*value = 256;
	return S_OK;
}

static HRESULT WINAPI monitor_usage_kind(monitor1 *iface, int32_t *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_UsageKind");
	*value = usage_standard;
	return S_OK;
}

static HRESULT WINAPI monitor_native_resolution(monitor1 *iface, __x_ABI_CWindows_CGraphics_CSizeInt32 *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_NativeResolutionInRawPixels");
	value->Width = panel_pixels_x;
	value->Height = panel_pixels_y;
	return S_OK;
}

/* The boxed physical size PhysicalSizeInInches hands out: an IReference`1<Size> over the panel dimensions the EDID
 * carries, created per call the way a Windows box is. */
struct size_ref_object {
	size_ref_i iface;
	LONG refs;
	struct raven_weak_source weak;
	float width, height;
};

static HRESULT WINAPI size_ref_query(size_ref_i *iface, REFIID iid, void **out)
{
	struct size_ref_object *self = (struct size_ref_object *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___FIReference_1_Windows__CFoundation__CSize) || equal_iid(iid, &IID_IAgileObject))
		*out = iface;
	else if (equal_iid(iid, &IID_IWeakReferenceSource))
		*out = &self->weak;
	else {
		trace_query_refuse("size-ref", iid);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI size_ref_addref(size_ref_i *iface)
{
	return shared_addref(&((struct size_ref_object *)iface)->refs);
}

static ULONG WINAPI size_ref_release(size_ref_i *iface)
{
	struct size_ref_object *self = (struct size_ref_object *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		raven_weak_source_disconnect(&self->weak);
		HeapFree(GetProcessHeap(), 0, self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI size_ref_iids(size_ref_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI size_ref_name(size_ref_i *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, size_ref_type_name);
}

static HRESULT WINAPI size_ref_trust(size_ref_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI size_ref_value(size_ref_i *iface, __x_ABI_CWindows_CFoundation_CSize *value)
{
	struct size_ref_object *self = (struct size_ref_object *)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_PhysicalSizeInInches.get_Value");
	value->Width = self->width;
	value->Height = self->height;
	return S_OK;
}

static const size_ref_iVtbl size_ref_table = {
	.QueryInterface = size_ref_query,
	.AddRef = size_ref_addref,
	.Release = size_ref_release,
	.GetIids = size_ref_iids,
	.GetRuntimeClassName = size_ref_name,
	.GetTrustLevel = size_ref_trust,
	.get_Value = size_ref_value,
};

static HRESULT create_size_ref(float width, float height, size_ref_i **out)
{
	struct size_ref_object *self = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *self);
	if (!self)
		return E_OUTOFMEMORY;
	self->iface.lpVtbl = &size_ref_table;
	self->refs = 1;
	self->width = width;
	self->height = height;
	raven_weak_source_init(&self->weak, self, &self->refs, (raven_weak_query_fn)size_ref_query);
	InterlockedIncrement(&raven_xaml_objects);
	*out = &self->iface;
	return S_OK;
}

static HRESULT WINAPI monitor_physical_size(monitor1 *iface, IInspectable **value)
{
	float width_inches = (float)(panel_width_mm / 25.4), height_inches = (float)(panel_height_mm / 25.4);
	size_ref_i *box;
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	trace_member("get_PhysicalSizeInInches");
	{
		HRESULT result = create_size_ref(width_inches, height_inches, &box);
		if (FAILED(result))
			return result;
	}
	*value = (IInspectable *)box;
	return S_OK;
}

static HRESULT WINAPI monitor_raw_dpi_x(monitor1 *iface, float *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_RawDpiX");
	*value = (float)(panel_pixels_x * 25.4 / panel_width_mm);
	return S_OK;
}

static HRESULT WINAPI monitor_raw_dpi_y(monitor1 *iface, float *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_RawDpiY");
	*value = (float)(panel_pixels_y * 25.4 / panel_height_mm);
	return S_OK;
}

/* The chromaticity the EDID's sRGB block encodes, as the primaries the projection reads. */
static HRESULT WINAPI monitor_point_member(monitor1 *iface, float x, float y, const char *member,
	__x_ABI_CWindows_CFoundation_CPoint *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_member(member);
	value->X = x;
	value->Y = y;
	return S_OK;
}

static HRESULT WINAPI monitor_red_primary(monitor1 *iface, __x_ABI_CWindows_CFoundation_CPoint *value)
{
	return monitor_point_member(iface, 0.64f, 0.33f, "get_RedPrimary", value);
}

static HRESULT WINAPI monitor_green_primary(monitor1 *iface, __x_ABI_CWindows_CFoundation_CPoint *value)
{
	return monitor_point_member(iface, 0.30f, 0.60f, "get_GreenPrimary", value);
}

static HRESULT WINAPI monitor_blue_primary(monitor1 *iface, __x_ABI_CWindows_CFoundation_CPoint *value)
{
	return monitor_point_member(iface, 0.15f, 0.06f, "get_BluePrimary", value);
}

static HRESULT WINAPI monitor_white_point(monitor1 *iface, __x_ABI_CWindows_CFoundation_CPoint *value)
{
	return monitor_point_member(iface, 0.3127f, 0.329f, "get_WhitePoint", value);
}

/* The modeled EDID carries no luminance descriptors and the CCD model serves no luminance block, so the luminance
 * members answer the zero Windows gives a monitor whose metadata names none. */
static HRESULT WINAPI monitor_luminance(monitor1 *iface, const char *member, float *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_member(member);
	*value = 0.0f;
	return S_OK;
}

static HRESULT WINAPI monitor_max_luminance(monitor1 *iface, float *value)
{
	return monitor_luminance(iface, "get_MaxLuminanceInNits", value);
}

static HRESULT WINAPI monitor_min_luminance(monitor1 *iface, float *value)
{
	return monitor_luminance(iface, "get_MinLuminanceInNits", value);
}

static HRESULT WINAPI monitor_max_average_luminance(monitor1 *iface, float *value)
{
	return monitor_luminance(iface, "get_MaxAverageFullFrameLuminanceInNits", value);
}

/* The descriptor bytes: the EDID the registry holds (the one register_monitor_edid seeded, read back so the answer
 * is what the guest's own device enumeration sees), each call handing out a fresh CoTaskMemAlloc'd copy the WinRT
 * array contract lets the caller free. The DisplayId descriptor is one the modeled monitor does not carry. */
static HRESULT read_edid_descriptor(UINT32 *value_size, BYTE **value)
{
	HKEY key;
	LONG opened, queried;
	DWORD type = 0, size = 0;
	BYTE *buffer;
	*value_size = 0;
	*value = NULL;
	opened = RegOpenKeyExW(HKEY_LOCAL_MACHINE, edid_key_path, 0, KEY_READ, &key);
	if (opened != ERROR_SUCCESS)
		return E_NOT_FOUND_NATIVE;
	queried = RegQueryValueExW(key, L"EDID", NULL, &type, NULL, &size);
	if (queried != ERROR_SUCCESS || !size || type != REG_BINARY) {
		RegCloseKey(key);
		return E_NOT_FOUND_NATIVE;
	}
	buffer = (BYTE *)CoTaskMemAlloc(size);
	if (!buffer) {
		RegCloseKey(key);
		return E_OUTOFMEMORY;
	}
	queried = RegQueryValueExW(key, L"EDID", NULL, NULL, buffer, &size);
	RegCloseKey(key);
	if (queried != ERROR_SUCCESS) {
		CoTaskMemFree(buffer);
		return E_NOT_FOUND_NATIVE;
	}
	*value_size = (UINT32)size;
	*value = buffer;
	return S_OK;
}

static HRESULT WINAPI monitor_get_descriptor(monitor1 *iface, int32_t kind, UINT32 *value_size, BYTE **value)
{
	(void)iface;
	if (!value_size || !value)
		return E_POINTER;
	*value_size = 0;
	*value = NULL;
	if (kind == descriptor_display_id) {
		trace_member("GetDescriptor(DisplayId)");
		return E_NOT_FOUND_NATIVE;
	}
	if (kind != descriptor_edid) {
		trace_member("GetDescriptor");
		return E_INVALIDARG;
	}
	trace_member("GetDescriptor(Edid)");
	return read_edid_descriptor(value_size, value);
}

static const monitor1Vtbl monitor_table = {
	.QueryInterface = monitor_query,
	.AddRef = monitor_addref,
	.Release = monitor_release,
	.GetIids = monitor_iids,
	.GetRuntimeClassName = monitor_name,
	.GetTrustLevel = monitor_trust,
	.get_DeviceId = monitor_device_id,
	.get_DisplayName = monitor_display_name,
	.get_ConnectionKind = monitor_connection_kind,
	.get_PhysicalConnector = monitor_physical_connector,
	.get_DisplayAdapterDeviceId = monitor_adapter_device_id,
	.get_DisplayAdapterId = monitor_adapter_id,
	.get_DisplayAdapterTargetId = monitor_adapter_target_id,
	.get_UsageKind = monitor_usage_kind,
	.get_NativeResolutionInRawPixels = monitor_native_resolution,
	.get_PhysicalSizeInInches = monitor_physical_size,
	.get_RawDpiX = monitor_raw_dpi_x,
	.get_RawDpiY = monitor_raw_dpi_y,
	.get_RedPrimary = monitor_red_primary,
	.get_GreenPrimary = monitor_green_primary,
	.get_BluePrimary = monitor_blue_primary,
	.get_WhitePoint = monitor_white_point,
	.get_MaxLuminanceInNits = monitor_max_luminance,
	.get_MinLuminanceInNits = monitor_min_luminance,
	.get_MaxAverageFullFrameLuminanceInNits = monitor_max_average_luminance,
	.GetDescriptor = monitor_get_descriptor,
};

/* The IDisplayMonitor2 face the instance's query serves: the later contract's one addition. */
static HRESULT WINAPI monitor_two_query(monitor2 *iface, REFIID iid, void **out)
{
	return monitor_query_any(((struct monitor_part2 *)iface)->owner, iid, out);
}

static ULONG WINAPI monitor_two_addref(monitor2 *iface)
{
	return shared_addref(&((struct monitor_part2 *)iface)->owner->refs);
}

/* The IDisplayMonitor2 face shares the object's one reference count, so its release frees the object when it holds
 * the last one, exactly as the primary face's release does. */
static ULONG monitor_release_refs(struct monitor_object *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		raven_weak_source_disconnect(&self->weak);
		HeapFree(GetProcessHeap(), 0, self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static ULONG WINAPI monitor_two_release(monitor2 *iface)
{
	return monitor_release_refs(((struct monitor_part2 *)iface)->owner);
}

static HRESULT WINAPI monitor_two_iids(monitor2 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI monitor_two_name(monitor2 *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, class_name);
}

static HRESULT WINAPI monitor_two_trust(monitor2 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI monitor_two_dolby_vision(monitor2 *iface, boolean *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_IsDolbyVisionSupportedInHdrMode");
	/* the CCD model answers an sRGB panel with no advanced color to enable */
	*value = FALSE;
	return S_OK;
}

static const monitor2Vtbl monitor_two_table = {
	.QueryInterface = monitor_two_query,
	.AddRef = monitor_two_addref,
	.Release = monitor_two_release,
	.GetIids = monitor_two_iids,
	.GetRuntimeClassName = monitor_two_name,
	.GetTrustLevel = monitor_two_trust,
	.get_IsDolbyVisionSupportedInHdrMode = monitor_two_dolby_vision,
};

static struct monitor_object *create_monitor(void)
{
	struct monitor_object *self = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *self);
	if (!self)
		return NULL;
	self->iface.lpVtbl = &monitor_table;
	self->two.lpVtbl = &monitor_two_table;
	self->two.owner = self;
	self->refs = 1;
	raven_weak_source_init(&self->weak, self, &self->refs, (raven_weak_query_fn)monitor_query_any);
	InterlockedIncrement(&raven_xaml_objects);
	return self;
}

/* ---- The async operation: the shape storage.c's completed operations take, instantiated for the monitor result.
 * Each operation finishes before the statics member returns — Raven's one monitor is known at the call — so the
 * completed handler, if the guest set one, runs inside the creation call. ---- */

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

#ifndef E_ILLEGAL_METHOD_CALL
#define E_ILLEGAL_METHOD_CALL ((HRESULT)0x8000000e)
#endif
#ifndef E_ILLEGAL_DELEGATE_ASSIGNMENT
#define E_ILLEGAL_DELEGATE_ASSIGNMENT ((HRESULT)0x80000018)
#endif
#ifndef RO_E_CLOSED
#define RO_E_CLOSED ((HRESULT)0x80000013)
#endif

typedef __FIAsyncOperation_1_Windows__CDevices__CDisplay__CDisplayMonitor monitor_op_i;
typedef __FIAsyncOperation_1_Windows__CDevices__CDisplay__CDisplayMonitorVtbl monitor_op_iVtbl;
typedef __FIAsyncOperationCompletedHandler_1_Windows__CDevices__CDisplay__CDisplayMonitor monitor_done_i;

/* The hand-declared interfaces have no COBJMACROS wrappers: addref, release and invoke through the vtable. The
 * completed handlers share the delegate shape, so one helper serves every operation's handler calls. */
static void display_release_iunknown(void *iface)
{
	((IUnknown *)iface)->lpVtbl->Release((IUnknown *)iface);
}

static void display_addref_iunknown(void *iface)
{
	((IUnknown *)iface)->lpVtbl->AddRef((IUnknown *)iface);
}

static void display_handler_release(void *handler)
{
	void **table = *(void ***)handler;
	((ULONG (WINAPI *)(void *))table[2])(handler);
}

static void display_handler_addref(void *handler)
{
	void **table = *(void ***)handler;
	((ULONG (WINAPI *)(void *))table[1])(handler);
}

static void display_handler_invoke(void *handler, void *operation, AsyncStatus status)
{
	void **table = *(void ***)handler;
	((HRESULT (WINAPI *)(void *, void *, AsyncStatus))table[3])(handler, operation, status);
}

struct async_core {
	LONG refs;
	CRITICAL_SECTION lock;
	AsyncStatus status;
	HRESULT error;
	UINT32 id;
	BOOL closed, assigned;
	void *completed;
};

static LONG next_display_async_id;

#define DISPLAY_ASYNC_LIFETIME(name, iface_type, own_iids, handler_type) \
	static HRESULT WINAPI name##_query(iface_type *iface, REFIID iid, void **out) \
	{ \
		struct name *self = (struct name *)iface; \
		UINT which; \
		if (!out) \
			return E_POINTER; \
		*out = NULL; \
		if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IAgileObject)) \
			*out = iface; \
		else if (equal_iid(iid, &IID_IWeakReferenceSource)) \
			*out = &self->weak; \
		else { \
			for (which = 0; which < 1 && !*out; ++which) \
				if (equal_iid(iid, &own_iids[which])) \
					*out = iface; \
			if (!*out) { \
				if (equal_iid(iid, &IID_IAsyncInfo)) \
					*out = &self->info; \
				else { \
					trace_query_refuse("op", iid); \
					return E_NOINTERFACE; \
				} \
			} \
		} \
		InterlockedIncrement(&self->core.refs); \
		return S_OK; \
	} \
	static ULONG WINAPI name##_addref(iface_type *iface) \
	{ \
		return (ULONG)InterlockedIncrement(&((struct name *)iface)->core.refs); \
	} \
	static ULONG WINAPI name##_release(iface_type *iface) \
	{ \
		struct name *self = (struct name *)iface; \
		const LONG refs = InterlockedDecrement(&self->core.refs); \
		if (!refs) { \
			if (self->core.completed) \
				display_handler_release(self->core.completed); \
			name##_free(self); \
			DeleteCriticalSection(&self->core.lock); \
			FREE(self); \
			InterlockedDecrement(&raven_xaml_objects); \
		} \
		return (ULONG)refs; \
	} \
	static HRESULT WINAPI name##_iids(iface_type *iface, ULONG *count, IID **iids) \
	{ \
		(void)iface; \
		return inspectable_iids(count, iids); \
	} \
	static HRESULT WINAPI name##_class(iface_type *iface, HSTRING *out) \
	{ \
		(void)iface; \
		return runtime_class_name(out, name##_type_name); \
	} \
	static HRESULT WINAPI name##_trust(iface_type *iface, TrustLevel *level) \
	{ \
		(void)iface; \
		return inspectable_trust(level); \
	} \
	static struct name *name##_owner(IAsyncInfo *info) \
	{ \
		return CONTAINING_RECORD(info, struct name, info); \
	} \
	static HRESULT WINAPI name##_info_query(IAsyncInfo *info, REFIID iid, void **out) \
	{ \
		return name##_query(&name##_owner(info)->iface, iid, out); \
	} \
	static ULONG WINAPI name##_info_addref(IAsyncInfo *info) \
	{ \
		return name##_addref(&name##_owner(info)->iface); \
	} \
	static ULONG WINAPI name##_info_release(IAsyncInfo *info) \
	{ \
		return name##_release(&name##_owner(info)->iface); \
	} \
	static HRESULT WINAPI name##_info_iids(IAsyncInfo *info, ULONG *count, IID **iids) \
	{ \
		(void)info; \
		return inspectable_iids(count, iids); \
	} \
	static HRESULT WINAPI name##_info_class(IAsyncInfo *info, HSTRING *out) \
	{ \
		(void)info; \
		return runtime_class_name(out, name##_type_name); \
	} \
	static HRESULT WINAPI name##_info_trust(IAsyncInfo *info, TrustLevel *level) \
	{ \
		(void)info; \
		return inspectable_trust(level); \
	} \
	static HRESULT WINAPI name##_info_id(IAsyncInfo *info, UINT32 *out) \
	{ \
		struct name *self = name##_owner(info); \
		HRESULT result; \
		if (!out) \
			return E_POINTER; \
		EnterCriticalSection(&self->core.lock); \
		result = self->core.closed ? RO_E_CLOSED : S_OK; \
		*out = self->core.id; \
		LeaveCriticalSection(&self->core.lock); \
		return result; \
	} \
	static HRESULT WINAPI name##_info_status(IAsyncInfo *info, AsyncStatus *out) \
	{ \
		struct name *self = name##_owner(info); \
		HRESULT result; \
		if (!out) \
			return E_POINTER; \
		EnterCriticalSection(&self->core.lock); \
		result = self->core.closed ? RO_E_CLOSED : S_OK; \
		*out = self->core.status; \
		LeaveCriticalSection(&self->core.lock); \
		return result; \
	} \
	static HRESULT WINAPI name##_info_error(IAsyncInfo *info, HRESULT *out) \
	{ \
		struct name *self = name##_owner(info); \
		HRESULT result; \
		if (!out) \
			return E_POINTER; \
		EnterCriticalSection(&self->core.lock); \
		result = self->core.closed ? RO_E_CLOSED : S_OK; \
		*out = self->core.error; \
		LeaveCriticalSection(&self->core.lock); \
		return result; \
	} \
	static HRESULT WINAPI name##_info_cancel(IAsyncInfo *info) \
	{ \
		(void)info; \
		return S_OK; \
	} \
	static HRESULT WINAPI name##_info_close(IAsyncInfo *info) \
	{ \
		struct name *self = name##_owner(info); \
		void *handler = NULL; \
		HRESULT result = S_OK; \
		EnterCriticalSection(&self->core.lock); \
		if (self->core.status == Started) result = E_ILLEGAL_METHOD_CALL; \
		else { \
			self->core.closed = TRUE; \
			handler = self->core.completed; \
			self->core.completed = NULL; \
		} \
		LeaveCriticalSection(&self->core.lock); \
		if (handler) \
			display_handler_release(handler); \
		return result; \
	} \
	static const IAsyncInfoVtbl name##_info_table = { \
		name##_info_query, name##_info_addref, name##_info_release, name##_info_iids, name##_info_class, \
		name##_info_trust, name##_info_id, name##_info_status, name##_info_error, name##_info_cancel, \
		name##_info_close, \
	};

#define DISPLAY_ASYNC_COMPLETED(name, iface_type, handler_type) \
	static HRESULT WINAPI name##_put_completed(iface_type *iface, handler_type *handler) \
	{ \
		struct name *self = (struct name *)iface; \
		HRESULT result = S_OK; \
		AsyncStatus status; \
		EnterCriticalSection(&self->core.lock); \
		status = self->core.status; \
		if (self->core.closed) result = RO_E_CLOSED; \
		else if (self->core.assigned) result = E_ILLEGAL_DELEGATE_ASSIGNMENT; \
		else { \
			self->core.assigned = TRUE; \
			self->core.completed = handler; \
			if (handler) { \
				display_handler_addref(handler); \
				if (status != Started) display_handler_addref(handler); \
			} \
		} \
		LeaveCriticalSection(&self->core.lock); \
		if (SUCCEEDED(result) && handler && status != Started) { \
			display_handler_invoke(handler, iface, status); \
			display_handler_release(handler); \
		} \
		return result; \
	} \
	static HRESULT WINAPI name##_get_completed(iface_type *iface, handler_type **handler) \
	{ \
		struct name *self = (struct name *)iface; \
		HRESULT result = S_OK; \
		if (!handler) \
			return E_POINTER; \
		*handler = NULL; \
		EnterCriticalSection(&self->core.lock); \
		if (self->core.closed) result = RO_E_CLOSED; \
		else if ((*handler = (handler_type *)self->core.completed)) display_handler_addref(*handler); \
		LeaveCriticalSection(&self->core.lock); \
		return result; \
	}

#define DISPLAY_ASYNC_FINISH(name, iface_type, handler_type) \
	static void name##_finish(iface_type *iface, HRESULT error, AsyncStatus status) \
	{ \
		struct name *self = (struct name *)iface; \
		handler_type *handler = NULL; \
		EnterCriticalSection(&self->core.lock); \
		if (self->core.status == Started) { \
			self->core.status = status; \
			self->core.error = error; \
			handler = (handler_type *)self->core.completed; \
			if (handler) display_handler_addref(handler); \
		} \
		LeaveCriticalSection(&self->core.lock); \
		if (handler) { \
			display_handler_invoke(handler, iface, status); \
			display_handler_release(handler); \
		} \
	}

#define DISPLAY_ASYNC_NEW(name, iface_type) \
	static struct name *name##_new(void) \
	{ \
		struct name *self = ALLOC(sizeof *self); \
		if (!self) \
			return NULL; \
		self->iface.lpVtbl = (void *)&name##_table; \
		self->info.lpVtbl = (void *)&name##_info_table; \
		self->core.refs = 1; \
		self->core.status = Started; \
		self->core.error = S_OK; \
		self->core.id = (UINT32)InterlockedIncrement(&next_display_async_id); \
		InitializeCriticalSection(&self->core.lock); \
		raven_weak_source_init(&self->weak, self, &self->core.refs, (raven_weak_query_fn)name##_query); \
		InterlockedIncrement(&raven_xaml_objects); \
		return self; \
	}

struct monitor_operation {
	monitor_op_i iface;
	IAsyncInfo info;
	struct async_core core;
	struct raven_weak_source weak;
	monitor1 *result;
};

static const WCHAR monitor_operation_type_name[] = L"Windows.Foundation.IAsyncOperation`1<Windows.Devices.Display.DisplayMonitor>";

/* The parameterized operation identifier: the signature computation the type system specifies, the one the guest's
 * .NET Native projection computes for the await's cast (the same computation reproduced the BitmapDecoder instance
 * its own image carries as a constant). A query the computation does not name is traced and refused. */
static const IID monitor_op_iids[1] = {
	{0x28c0e623, 0x4e46, 0x58c3, {0xad, 0x82, 0x50, 0x2b, 0xde, 0xcc, 0x43, 0x45}},
};

static void monitor_operation_free(struct monitor_operation *self)
{
	raven_weak_source_disconnect(&self->weak);
	if (self->result)
		display_release_iunknown(self->result);
}

DISPLAY_ASYNC_LIFETIME(monitor_operation, monitor_op_i, monitor_op_iids, monitor_done_i)
DISPLAY_ASYNC_COMPLETED(monitor_operation, monitor_op_i, monitor_done_i)
DISPLAY_ASYNC_FINISH(monitor_operation, monitor_op_i, monitor_done_i)

static HRESULT WINAPI monitor_operation_results(monitor_op_i *iface, monitor1 **results)
{
	struct monitor_operation *self = (struct monitor_operation *)iface;
	HRESULT result;
	if (!results)
		return E_POINTER;
	*results = NULL;
	EnterCriticalSection(&self->core.lock);
	result = self->core.closed ? RO_E_CLOSED : self->core.status == Started ? E_ILLEGAL_METHOD_CALL : self->core.error;
	LeaveCriticalSection(&self->core.lock);
	if (SUCCEEDED(result) && self->result) {
		*results = self->result;
		display_addref_iunknown(*results);
	}
	return result;
}

static const monitor_op_iVtbl monitor_operation_table = {
	.QueryInterface = monitor_operation_query,
	.AddRef = monitor_operation_addref,
	.Release = monitor_operation_release,
	.GetIids = monitor_operation_iids,
	.GetRuntimeClassName = monitor_operation_class,
	.GetTrustLevel = monitor_operation_trust,
	.put_Completed = monitor_operation_put_completed,
	.get_Completed = monitor_operation_get_completed,
	.GetResults = monitor_operation_results,
};

DISPLAY_ASYNC_NEW(monitor_operation, monitor_op_i)

/* The creation ref of the result transfers to the operation; the operation hands the guest its own reference at
 * GetResults and releases the stored one when it dies. */
static void monitor_operation_ready(struct monitor_operation *self, monitor1 *result, HRESULT error)
{
	if (SUCCEEDED(error))
		self->result = result;
	monitor_operation_finish(&self->iface, error, SUCCEEDED(error) ? Completed : Error);
}

/* The statics' resolution: the id the guest names matches Raven's one monitor, or the resolution fails the way
 * Windows' own would (Element not found), the failure riding the await. */
static BOOL device_path_matches(HSTRING asked)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!asked)
		return FALSE;
	text = WindowsGetStringRawBuffer(asked, &length);
	if (length < ARRAYSIZE(device_path_prefix) - 1)
		return FALSE;
	return CompareStringOrdinal(text, (int)(ARRAYSIZE(device_path_prefix) - 1), device_path_prefix,
		(int)(ARRAYSIZE(device_path_prefix) - 1), TRUE) == CSTR_EQUAL;
}

static BOOL interface_id_matches(HSTRING asked)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!asked)
		return FALSE;
	text = WindowsGetStringRawBuffer(asked, &length);
	if (length != ARRAYSIZE(monitor_interface_id) - 1)
		return FALSE;
	return CompareStringOrdinal(text, (int)length, monitor_interface_id, -1, TRUE) == CSTR_EQUAL;
}

static HRESULT resolve_monitor_async(HSTRING asked, BOOL interface_id, IInspectable **operation)
{
	struct monitor_operation *op;
	struct monitor_object *monitor;
	HRESULT error;
	if (operation)
		*operation = NULL;
	if (!operation)
		return E_POINTER;
	if (!(op = monitor_operation_new()))
		return E_OUTOFMEMORY;
	if (interface_id ? interface_id_matches(asked) : device_path_matches(asked)) {
		monitor = create_monitor();
		error = monitor ? S_OK : E_OUTOFMEMORY;
	} else {
		monitor = NULL;
		error = E_NOT_FOUND_NATIVE;
	}
	monitor_operation_ready(op, (monitor1 *)monitor, error);
	*operation = (IInspectable *)&op->iface;
	return S_OK;
}

/* ---- The activation factory: the class is statics-driven, so the factory carries the IActivationFactory face the
 * activation names and the IDisplayMonitorStatics face the statics query names. ---- */

struct factory;
struct part_activation { const IActivationFactoryVtbl *lpVtbl; struct factory *owner; };
struct part_statics { const statics1Vtbl *lpVtbl; struct factory *owner; };

struct factory {
	struct part_activation activation;
	struct part_statics statics;
	LONG refs;
};

static HRESULT factory_query(struct factory *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory))
		*out = &self->activation;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics))
		*out = &self->statics;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

#define PART_BOILERPLATE(part, iface) \
	static ULONG WINAPI part##_addref(iface *iface_unused) \
	{ return (ULONG)InterlockedIncrement(&((struct part *)iface_unused)->owner->refs); } \
	static ULONG WINAPI part##_release(iface *iface_unused) \
	{ return (ULONG)InterlockedDecrement(&((struct part *)iface_unused)->owner->refs); } \
	static HRESULT WINAPI part##_iids(iface *iface_unused, ULONG *count, IID **iids) \
	{ (void)iface_unused; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI part##_trust(iface *iface_unused, TrustLevel *level) \
	{ (void)iface_unused; return inspectable_trust(level); }

PART_BOILERPLATE(part_activation, IActivationFactory)

static HRESULT WINAPI part_activation_query(IActivationFactory *iface, REFIID iid, void **out)
{
	return factory_query(((struct part_activation *)iface)->owner, iid, out);
}

static HRESULT WINAPI part_activation_name(IActivationFactory *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, class_name);
}

PART_BOILERPLATE(part_statics, statics1)

static HRESULT WINAPI part_statics_query(statics1 *iface, REFIID iid, void **out)
{
	return factory_query(((struct part_statics *)iface)->owner, iid, out);
}

static HRESULT WINAPI part_statics_name(statics1 *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, class_name);
}

static HRESULT WINAPI activation_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	/* the real DisplayMonitor has no default constructor: only the statics build instances */
	trace_member("ActivateInstance");
	return E_NOTIMPL;
}

static const IActivationFactoryVtbl activation_table = {
	.QueryInterface = part_activation_query,
	.AddRef = part_activation_addref,
	.Release = part_activation_release,
	.GetIids = part_activation_iids,
	.GetRuntimeClassName = part_activation_name,
	.GetTrustLevel = part_activation_trust,
	.ActivateInstance = activation_activate,
};

static HRESULT WINAPI statics_get_device_selector(statics1 *iface, HSTRING *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_member("GetDeviceSelector");
	return runtime_class_name(value, device_selector);
}

static HRESULT WINAPI statics_from_id_async(statics1 *iface, HSTRING device_id, IInspectable **operation)
{
	(void)iface;
	trace_member("FromIdAsync");
	return resolve_monitor_async(device_id, FALSE, operation);
}

static HRESULT WINAPI statics_from_interface_id_async(statics1 *iface, HSTRING device_interface_id, IInspectable **operation)
{
	(void)iface;
	trace_member("FromInterfaceIdAsync");
	return resolve_monitor_async(device_interface_id, TRUE, operation);
}

static const statics1Vtbl statics1_table = {
	.QueryInterface = part_statics_query,
	.AddRef = part_statics_addref,
	.Release = part_statics_release,
	.GetIids = part_statics_iids,
	.GetRuntimeClassName = part_statics_name,
	.GetTrustLevel = part_statics_trust,
	.GetDeviceSelector = statics_get_device_selector,
	.FromIdAsync = statics_from_id_async,
	.FromInterfaceIdAsync = statics_from_interface_id_async,
};

static struct factory factory_instance = {
	{&activation_table, &factory_instance},
	{&statics1_table, &factory_instance},
	1,
};

HRESULT raven_xaml_display_monitor_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (length != ARRAYSIZE(class_name) - 1 || CompareStringOrdinal(text, (int)length, class_name, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	InterlockedIncrement(&factory_instance.refs);
	*out = (IActivationFactory *)&factory_instance.activation;
	return S_OK;
}
