#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <weakreference.h>
#include <asyncinfo.h>
#include "windows_devices_display.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);

typedef __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics statics_i;
typedef __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor monitor_i;
typedef __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2 monitor2_i;
typedef __FIAsyncOperation_1_Windows__CDevices__CDisplay__CDisplayMonitor monitor_op_i;
typedef __FIReference_1_Windows__CFoundation__CSize size_ref_i;

/* The monitor Raven's display model gives the guest, as the CCD APIs and the seeded EDID describe it. */
static const WCHAR device_path[] =
	L"\\\\?\\DISPLAY#RAVEN0000#5&2f5f805e&0&UID256#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}";
static const WCHAR interface_id[] = L"{E6F07B5F-EE97-4A90-B076-33F57BF4EAA7}";
static const WCHAR display_name[] = L"Raven Monitor";
static const WCHAR adapter_path[] = L"\\\\.\\DISPLAY1";
static const WCHAR class_text[] = L"Windows.Devices.Display.DisplayMonitor";

/* mingw's weakreference.h models both interfaces as IUnknown-only, but the Windows ABI — the one raven's objects
 * implement and .NET Native is compiled against — derives them from IInspectable, so the one method rides vtable
 * slot 6. The fixture calls the documented shape by hand. */
struct weak_source_table7 {
	void *base[6];
	HRESULT (WINAPI *GetWeakReference)(void *, void **);
};

struct weak_reference_table7 {
	void *base[6];
	HRESULT (WINAPI *Resolve)(void *, REFIID, void **);
};

static HSTRING class_string(const WCHAR *text)
{
	HSTRING value = NULL;
	if (WindowsCreateString(text, lstrlenW(text), &value) != S_OK)
		return NULL;
	return value;
}

static BOOL string_is(HSTRING value, const WCHAR *expected)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(value, &length);
	return (UINT32)lstrlenW(expected) == length &&
		CompareStringOrdinal(text, (int)length, expected, -1, FALSE) == CSTR_EQUAL;
}

static BOOL string_contains(HSTRING value, const WCHAR *expected)
{
	UINT32 length = 0, wanted = (UINT32)lstrlenW(expected);
	const WCHAR *text = WindowsGetStringRawBuffer(value, &length);
	UINT32 at;
	if (length < wanted)
		return FALSE;
	for (at = 0; at + wanted <= length; ++at)
		if (CompareStringOrdinal(text + at, (int)wanted, expected, (int)wanted, TRUE) == CSTR_EQUAL)
			return TRUE;
	return FALSE;
}

static BOOL near_float(float value, float expected, float tolerance)
{
	float diff = value > expected ? value - expected : expected - value;
	return diff <= tolerance;
}

/* The completed handler the operations invoke: the operations finish inside the call, so the handler assigned after
 * creation fires at once; the fixture counts the invocations to prove both. The handler shape is the delegate ABI —
 * query, addref, release, invoke — and raven never queries one. */
struct handler_vtbl {
	HRESULT (WINAPI *query)(void *, REFIID, void **);
	ULONG (WINAPI *addref)(void *);
	ULONG (WINAPI *release)(void *);
	HRESULT (WINAPI *invoke)(void *, void *, AsyncStatus);
};

struct handler {
	const struct handler_vtbl *lpVtbl;
	LONG refs;
	ULONG calls;
	AsyncStatus status;
};

static ULONG WINAPI handler_addref(void *iface)
{
	return (ULONG)InterlockedIncrement(&((struct handler *)iface)->refs);
}

static ULONG WINAPI handler_release(void *iface)
{
	return (ULONG)InterlockedDecrement(&((struct handler *)iface)->refs);
}

static HRESULT WINAPI handler_invoke(void *iface, void *operation, AsyncStatus status)
{
	struct handler *self = (struct handler *)iface;
	(void)operation;
	self->calls++;
	self->status = status;
	return S_OK;
}

static const struct handler_vtbl handler_table = {
	NULL,
	handler_addref,
	handler_release,
	handler_invoke,
};

static struct handler handler_instance = {&handler_table, 1, 0, (AsyncStatus)0};

/* One shape fits every operation the fixture awaits: the IAsyncOperation layout puts the completed slot seventh and
 * GetResults ninth, whatever the result type is. */
static void *await_operation(IUnknown *operation, AsyncStatus *status)
{
	void **table = (void **)*(void **)operation;
	HRESULT (WINAPI *put)(void *, void *) = (HRESULT (WINAPI *)(void *, void *))table[6];
	HRESULT (WINAPI *get_results)(void *, void **) = (HRESULT (WINAPI *)(void *, void **))table[8];
	void *result = NULL;
	handler_instance.calls = 0;
	handler_instance.status = (AsyncStatus)0;
	if (put(operation, &handler_instance) != S_OK)
		return NULL;
	if (handler_instance.calls != 1)
		return NULL;
	if (status)
		*status = handler_instance.status;
	if (get_results(operation, &result) != S_OK)
		return NULL;
	return result;
}

/* The EDID the registry holds is a valid 1.4 base block describing the panel: the header, the "RVN" PnP id, the
 * 53 by 30 centimetre screen size and a checksum that closes over the block. */
static BOOL edid_is_the_modeled_panel(UINT32 size, const BYTE *edid)
{
	UINT32 sum = 0, at;
	if (size != 128)
		return FALSE;
	if (edid[0] != 0x00 || edid[1] != 0xff || edid[6] != 0xff || edid[7] != 0x00)
		return FALSE;
	if (edid[8] != 0xce || edid[9] != 0x4a) /* the "RVN" PnP id, little-endian */
		return FALSE;
	if (edid[18] != 1 || edid[19] != 4) /* EDID 1.4 */
		return FALSE;
	if (edid[21] != 53 || edid[22] != 30) /* the 53 by 30 centimetre panel */
		return FALSE;
	for (at = 0; at < 127; ++at)
		sum += edid[at];
	return ((sum + edid[127]) & 0xff) == 0;
}

/* The display monitor: the activation hands the class's factory and the statics face the query names; the selector
 * names the monitor interface class, the id resolutions resolve Raven's one monitor (a foreign id fails the await
 * the way Windows' own resolution does), and the instance answers the identity and geometry members the model gives
 * it, the descriptor read back from the registry and the later contract's advanced-color face. */
int raven_entry(void)
{
	static const WCHAR name[] = L"Windows.Devices.Display.DisplayMonitor";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function RoGetActivationFactory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	HSTRING class_key = NULL;
	statics_i *statics = NULL;
	HSTRING selector = NULL;
	IInspectable *operation = NULL;
	AsyncStatus status = (AsyncStatus)0;
	monitor_i *monitor = NULL;
	HSTRING text = NULL;
	int32_t connection_kind = -1, connector = -1, usage = -1;
	__x_ABI_CWindows_CGraphics_CDisplayAdapterId adapter_id = {0, 0};
	UINT32 target_id = 0, descriptor_size = 0;
	__x_ABI_CWindows_CGraphics_CSizeInt32 resolution = {0, 0};
	__x_ABI_CWindows_CFoundation_CPoint point = {0, 0};
	float dpi_x = 0, dpi_y = 0, luminance = -1;
	BYTE *edid = NULL;
	IInspectable *size_box = NULL;
	size_ref_i *size_reference = NULL;
	__x_ABI_CWindows_CFoundation_CSize size = {0, 0};
	monitor2_i *monitor2 = NULL;
	boolean dolby = TRUE;
	void *parameterized = NULL;
	void *weak_source = NULL;
	IUnknown *weak_reference = NULL;
	IInspectable *resolved = NULL;
	IAsyncInfo *async_info = NULL;
	HRESULT op_error = S_OK;
	if (!RoInitialize || !RoGetActivationFactory) return 20;
	if (FAILED(RoInitialize(1))) return 1;
	if (WindowsCreateString(name, ARRAYSIZE(name) - 1, &class_key) != S_OK || !class_key) return 21;
	if (RoGetActivationFactory(class_key, &IID__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics,
		(void **)&statics) != S_OK || !statics) return 2;
	/* the selector names the monitor interface class the device path carries */
	if (statics->lpVtbl->GetDeviceSelector(statics, &selector) != S_OK || !selector) return 3;
	if (!string_contains(selector, interface_id)) return 4;
	WindowsDeleteString(selector);
	/* the id resolutions resolve Raven's one monitor; the operation serves the parameterized face the projection
	 * computes for the await's cast */
	{
		HSTRING path_key = class_string(device_path);
		if (!path_key) return 22;
		if (statics->lpVtbl->FromIdAsync(statics, path_key, (IInspectable **)&operation) != S_OK || !operation) return 5;
		WindowsDeleteString(path_key);
	}
	if (operation->lpVtbl->QueryInterface(operation,
		&IID___FIAsyncOperation_1_Windows__CDevices__CDisplay__CDisplayMonitor, &parameterized) != S_OK || !parameterized) return 6;
	((IUnknown *)parameterized)->lpVtbl->Release((IUnknown *)parameterized);
	if (!(monitor = (monitor_i *)await_operation((IUnknown *)operation, &status)) || status != Completed) return 7;
	operation->lpVtbl->Release(operation);
	operation = NULL;
	/* the instance's runtime class is the class the activation asked for */
	{
		HSTRING runtime_name = NULL;
		if (monitor->lpVtbl->GetRuntimeClassName(monitor, &runtime_name) != S_OK) return 8;
		if (!string_is(runtime_name, name)) return 9;
		WindowsDeleteString(runtime_name);
	}
	/* the identity members: the modeled monitor's name, path and adapter */
	if (monitor->lpVtbl->get_DisplayName(monitor, &text) != S_OK || !string_is(text, display_name)) return 10;
	WindowsDeleteString(text);
	if (monitor->lpVtbl->get_DeviceId(monitor, &text) != S_OK || !string_is(text, device_path)) return 11;
	WindowsDeleteString(text);
	if (monitor->lpVtbl->get_DisplayAdapterDeviceId(monitor, &text) != S_OK || !string_is(text, adapter_path)) return 12;
	WindowsDeleteString(text);
	/* the geometry members: the CCD model's adapter LUID, target id, internal connection and the desktop resolution */
	if (monitor->lpVtbl->get_DisplayAdapterId(monitor, &adapter_id) != S_OK || adapter_id.LowPart != 1 || adapter_id.HighPart != 0) return 13;
	if (monitor->lpVtbl->get_DisplayAdapterTargetId(monitor, &target_id) != S_OK || target_id != 256) return 14;
	if (monitor->lpVtbl->get_ConnectionKind(monitor, &connection_kind) != S_OK || connection_kind != DisplayMonitorConnectionKind_Internal) return 15;
	if (monitor->lpVtbl->get_PhysicalConnector(monitor, &connector) != S_OK || connector != DisplayMonitorPhysicalConnectorKind_Unknown) return 16;
	if (monitor->lpVtbl->get_UsageKind(monitor, &usage) != S_OK || usage != DisplayMonitorUsageKind_Standard) return 17;
	if (monitor->lpVtbl->get_NativeResolutionInRawPixels(monitor, &resolution) != S_OK ||
		resolution.Width != 1920 || resolution.Height != 1080) return 18;
	/* the panel: the boxed physical size the EDID's screen size gives, and the raw DPI over it */
	if (monitor->lpVtbl->get_PhysicalSizeInInches(monitor, &size_box) != S_OK || !size_box) return 19;
	if (size_box->lpVtbl->QueryInterface(size_box, &IID___FIReference_1_Windows__CFoundation__CSize,
		(void **)&size_reference) != S_OK || !size_reference) return 23;
	if (size_reference->lpVtbl->get_Value(size_reference, &size) != S_OK) return 24;
	if (!near_float(size.Width, 530.0f / 25.4f, 0.01f) || !near_float(size.Height, 300.0f / 25.4f, 0.01f)) return 25;
	size_reference->lpVtbl->Release(size_reference);
	size_box->lpVtbl->Release(size_box);
	if (monitor->lpVtbl->get_RawDpiX(monitor, &dpi_x) != S_OK || !near_float(dpi_x, 1920.0f * 25.4f / 530.0f, 0.05f)) return 26;
	if (monitor->lpVtbl->get_RawDpiY(monitor, &dpi_y) != S_OK || !near_float(dpi_y, 1080.0f * 25.4f / 300.0f, 0.05f)) return 27;
	/* the chromaticity the EDID's sRGB block encodes */
	if (monitor->lpVtbl->get_RedPrimary(monitor, &point) != S_OK || !near_float(point.X, 0.64f, 0.001f) || !near_float(point.Y, 0.33f, 0.001f)) return 28;
	if (monitor->lpVtbl->get_GreenPrimary(monitor, &point) != S_OK || !near_float(point.X, 0.30f, 0.001f) || !near_float(point.Y, 0.60f, 0.001f)) return 29;
	if (monitor->lpVtbl->get_BluePrimary(monitor, &point) != S_OK || !near_float(point.X, 0.15f, 0.001f) || !near_float(point.Y, 0.06f, 0.001f)) return 30;
	if (monitor->lpVtbl->get_WhitePoint(monitor, &point) != S_OK || !near_float(point.X, 0.3127f, 0.001f) || !near_float(point.Y, 0.329f, 0.001f)) return 31;
	/* the luminance metadata the modeled monitor does not carry */
	if (monitor->lpVtbl->get_MaxLuminanceInNits(monitor, &luminance) != S_OK || luminance != 0.0f) return 32;
	if (monitor->lpVtbl->get_MinLuminanceInNits(monitor, &luminance) != S_OK || luminance != 0.0f) return 33;
	if (monitor->lpVtbl->get_MaxAverageFullFrameLuminanceInNits(monitor, &luminance) != S_OK || luminance != 0.0f) return 34;
	/* the descriptor bytes: the EDID the registry holds, read back and checked against the model. The descriptor's
	 * array buffer stays with the test process the way the imaging fixture's detached pixels do. */
	if (monitor->lpVtbl->GetDescriptor(monitor, DisplayMonitorDescriptorKind_Edid, &descriptor_size, &edid) != S_OK || !edid) return 35;
	if (!edid_is_the_modeled_panel(descriptor_size, edid)) return 36;
	if (monitor->lpVtbl->GetDescriptor(monitor, DisplayMonitorDescriptorKind_DisplayId, &descriptor_size, &edid) == S_OK || edid) return 37;
	/* the later contract's advanced-color face: the sRGB panel supports no Dolby Vision HDR mode */
	if (monitor->lpVtbl->QueryInterface(monitor, &IID__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2,
		(void **)&monitor2) != S_OK || !monitor2) return 38;
	if (monitor2->lpVtbl->get_IsDolbyVisionSupportedInHdrMode(monitor2, &dolby) != S_OK || dolby) return 39;
	monitor2->lpVtbl->Release(monitor2);
	monitor2 = NULL;
	/* the weak contract: the source hands a reference whose resolution names the object while it lives */
	if (monitor->lpVtbl->QueryInterface(monitor, &IID_IWeakReferenceSource, &weak_source) != S_OK || !weak_source) return 40;
	{
		struct weak_source_table7 *table = (struct weak_source_table7 *)*(void ***)weak_source;
		if (table->GetWeakReference(weak_source, (void **)&weak_reference) != S_OK || !weak_reference) return 41;
	}
	{
		struct weak_reference_table7 *table = (struct weak_reference_table7 *)*(void ***)weak_reference;
		if (table->Resolve(weak_reference, &IID_IInspectable, (void **)&resolved) != S_OK || !resolved) return 42;
	}
	resolved->lpVtbl->Release(resolved);
	((IUnknown *)weak_reference)->lpVtbl->Release((IUnknown *)weak_reference);
	((IUnknown *)weak_source)->lpVtbl->Release((IUnknown *)weak_source);
	weak_source = NULL;
	weak_reference = NULL;
	monitor->lpVtbl->Release(monitor);
	monitor = NULL;
	/* the interface-id resolution names the same monitor */
	{
		HSTRING interface_key = class_string(interface_id);
		if (!interface_key) return 43;
		if (statics->lpVtbl->FromInterfaceIdAsync(statics, interface_key, (IInspectable **)&operation) != S_OK || !operation) return 44;
		WindowsDeleteString(interface_key);
	}
	if (!(monitor = (monitor_i *)await_operation((IUnknown *)operation, &status)) || status != Completed) return 45;
	operation->lpVtbl->Release(operation);
	operation = NULL;
	monitor->lpVtbl->Release(monitor);
	monitor = NULL;
	/* a foreign device id fails the await with the Element not found the resolution gives, through IAsyncInfo */
	{
		HSTRING foreign_key = class_string(L"\\\\?\\DISPLAY#OTHER0000#5&2f5f805e&0&UID256#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}");
		if (!foreign_key) return 46;
		if (statics->lpVtbl->FromIdAsync(statics, foreign_key, (IInspectable **)&operation) != S_OK || !operation) return 47;
		WindowsDeleteString(foreign_key);
	}
	if (await_operation((IUnknown *)operation, &status) != NULL || status != Error) return 48;
	if (operation->lpVtbl->QueryInterface(operation, &IID_IAsyncInfo, (void **)&async_info) != S_OK || !async_info) return 49;
	if (async_info->lpVtbl->get_ErrorCode(async_info, &op_error) != S_OK || op_error != (HRESULT)0x80070490) return 50;
	async_info->lpVtbl->Release(async_info);
	operation->lpVtbl->Release(operation);
	operation = NULL;
	/* the statics face names the runtime class too */
	{
		HSTRING runtime_name = NULL;
		if (statics->lpVtbl->GetRuntimeClassName(statics, &runtime_name) != S_OK) return 51;
		if (!string_is(runtime_name, name)) return 52;
		WindowsDeleteString(runtime_name);
	}
	WindowsDeleteString(class_key);
	class_key = NULL;
	statics->lpVtbl->Release(statics);
	return 0;
}
