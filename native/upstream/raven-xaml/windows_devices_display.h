/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The Windows.Devices.Display interfaces this component serves, in the shape scripts/generate-winrt-header.py's
 * committed headers take: vtable order and interface identifiers hand-extracted from the metadata that generator
 * pins (revision 58217a69, crates/libs/windows/src/Windows/Devices/Display/mod.rs) and cross-checked against the
 * Microsoft.Windows.SDK.Contracts 10.0.19041.2 winmd's own GuidAttribute blobs (the same three identifiers there,
 * and the same member order: the statics' three members and the instance's twenty). The guest's own Windows.winmd
 * is a trimmed stub, so the contracts winmd is the authority a guest's projection was compiled against.
 *
 * The generic instances' identifiers are the signature computation the Windows Runtime type system specifies —
 * the RFC 4122 version-5 UUID (SHA-1) of the signature under the namespace 11f47ad5-7b73-42c0-abae-878b1e16adee —
 * the same computation scripts/generate-winrt-header.py makes and the same one raven's imaging component verified
 * against mingw's precomputed instances: IAsyncOperation`1's template identifier {9fc2b0bb-e446-44e2-aa61-9cab-
 * 8f636af2} and IReference`1's {61c17706-2d65-11e0-9ae8-d48564015472} both reproduce the instances mingw's headers
 * carry, so the instances here are the identifiers a .NET Native guest computes for them (its Shapr3D.dll carries
 * the BitmapDecoder instance of IAsyncOperation`1 as a constant, and it is exactly this computation's answer).
 * Each struct is declared once: a duplicated typedef here would type-confuse the two widths' compilations. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>
#include <asyncinfo.h>
#include <windows.foundation.h>

/* Windows.Graphics.DisplayAdapterId, the LUID the adapter is known by. */
typedef struct __x_ABI_CWindows_CGraphics_CDisplayAdapterId {
	UINT32 LowPart;
	int32_t HighPart;
} __x_ABI_CWindows_CGraphics_CDisplayAdapterId;

/* Windows.Graphics.SizeInt32. */
typedef struct __x_ABI_CWindows_CGraphics_CSizeInt32 {
	int32_t Width;
	int32_t Height;
} __x_ABI_CWindows_CGraphics_CSizeInt32;

/* Windows.Devices.Display's enumerations, as the metadata gives them. */
typedef enum __x_ABI_CWindows_CDevices_CDisplay_CDisplayMonitorConnectionKind {
	DisplayMonitorConnectionKind_Internal = 0,
	DisplayMonitorConnectionKind_Wired = 1,
	DisplayMonitorConnectionKind_Wireless = 2,
	DisplayMonitorConnectionKind_Virtual = 3
} __x_ABI_CWindows_CDevices_CDisplay_CDisplayMonitorConnectionKind;

typedef enum __x_ABI_CWindows_CDevices_CDisplay_CDisplayMonitorDescriptorKind {
	DisplayMonitorDescriptorKind_Edid = 0,
	DisplayMonitorDescriptorKind_DisplayId = 1
} __x_ABI_CWindows_CDevices_CDisplay_CDisplayMonitorDescriptorKind;

typedef enum __x_ABI_CWindows_CDevices_CDisplay_CDisplayMonitorPhysicalConnectorKind {
	DisplayMonitorPhysicalConnectorKind_Unknown = 0,
	DisplayMonitorPhysicalConnectorKind_HD15 = 1,
	DisplayMonitorPhysicalConnectorKind_AnalogTV = 2,
	DisplayMonitorPhysicalConnectorKind_Dvi = 3,
	DisplayMonitorPhysicalConnectorKind_Hdmi = 4,
	DisplayMonitorPhysicalConnectorKind_Lvds = 5,
	DisplayMonitorPhysicalConnectorKind_Sdi = 6,
	DisplayMonitorPhysicalConnectorKind_DisplayPort = 7
} __x_ABI_CWindows_CDevices_CDisplay_CDisplayMonitorPhysicalConnectorKind;

typedef enum __x_ABI_CWindows_CDevices_CDisplay_CDisplayMonitorUsageKind {
	DisplayMonitorUsageKind_Standard = 0,
	DisplayMonitorUsageKind_HeadMounted = 1,
	DisplayMonitorUsageKind_SpecialPurpose = 2
} __x_ABI_CWindows_CDevices_CDisplay_CDisplayMonitorUsageKind;

/* IDisplayMonitorStatics, the statics face the DisplayMonitor activation names (0x6eae698f_a228_4c05_821d_b695d667de8e,
 * the identifier Shapr.Native.dll's path asks for): the device selector and the two monitors the ids name. */
typedef struct __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics;
typedef struct __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *GetDeviceSelector)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics *This, HSTRING *value);
	HRESULT (STDMETHODCALLTYPE *FromIdAsync)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics *This, HSTRING device_id, IInspectable **operation);
	HRESULT (STDMETHODCALLTYPE *FromInterfaceIdAsync)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics *This, HSTRING device_interface_id, IInspectable **operation);
	END_INTERFACE
} __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStaticsVtbl;

interface __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics {
	CONST_VTBL __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStaticsVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorStatics, 0x6eae698f, 0xa228, 0x4c05, 0x82, 0x1d, 0xb6, 0x95, 0xd6, 0x67, 0xde, 0x8e);

/* IDisplayMonitor, the instance face the projection reads the monitor through: the identity and geometry members in
 * the metadata's order (0x1f6b15d4_1d01_4c51_87e2_6f954a772b59, the contracts winmd's GuidAttribute agrees). */
typedef struct __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor;
typedef struct __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_DeviceId)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, HSTRING *value);
	HRESULT (STDMETHODCALLTYPE *get_DisplayName)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, HSTRING *value);
	HRESULT (STDMETHODCALLTYPE *get_ConnectionKind)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, int32_t *value);
	HRESULT (STDMETHODCALLTYPE *get_PhysicalConnector)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, int32_t *value);
	HRESULT (STDMETHODCALLTYPE *get_DisplayAdapterDeviceId)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, HSTRING *value);
	HRESULT (STDMETHODCALLTYPE *get_DisplayAdapterId)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, __x_ABI_CWindows_CGraphics_CDisplayAdapterId *value);
	HRESULT (STDMETHODCALLTYPE *get_DisplayAdapterTargetId)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *get_UsageKind)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, int32_t *value);
	HRESULT (STDMETHODCALLTYPE *get_NativeResolutionInRawPixels)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, __x_ABI_CWindows_CGraphics_CSizeInt32 *value);
	HRESULT (STDMETHODCALLTYPE *get_PhysicalSizeInInches)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *get_RawDpiX)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, float *value);
	HRESULT (STDMETHODCALLTYPE *get_RawDpiY)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, float *value);
	HRESULT (STDMETHODCALLTYPE *get_RedPrimary)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, __x_ABI_CWindows_CFoundation_CPoint *value);
	HRESULT (STDMETHODCALLTYPE *get_GreenPrimary)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, __x_ABI_CWindows_CFoundation_CPoint *value);
	HRESULT (STDMETHODCALLTYPE *get_BluePrimary)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, __x_ABI_CWindows_CFoundation_CPoint *value);
	HRESULT (STDMETHODCALLTYPE *get_WhitePoint)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, __x_ABI_CWindows_CFoundation_CPoint *value);
	HRESULT (STDMETHODCALLTYPE *get_MaxLuminanceInNits)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, float *value);
	HRESULT (STDMETHODCALLTYPE *get_MinLuminanceInNits)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, float *value);
	HRESULT (STDMETHODCALLTYPE *get_MaxAverageFullFrameLuminanceInNits)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, float *value);
	HRESULT (STDMETHODCALLTYPE *GetDescriptor)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor *This, int32_t kind, UINT32 *value_size, BYTE **value);
	END_INTERFACE
} __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorVtbl;

interface __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor {
	CONST_VTBL __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitorVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor, 0x1f6b15d4, 0x1d01, 0x4c51, 0x87, 0xe2, 0x6f, 0x95, 0x4a, 0x77, 0x2b, 0x59);

/* IDisplayMonitor2: the one later-contract addition (0x023018e6_cb23_5830_96df_a7bf6e602577, the contracts winmd's
 * GuidAttribute agrees). */
typedef struct __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2 __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2;
typedef struct __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_IsDolbyVisionSupportedInHdrMode)(__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2 *This, boolean *value);
	END_INTERFACE
} __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2Vtbl;

interface __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2 {
	CONST_VTBL __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor2, 0x023018e6, 0xcb23, 0x5830, 0x96, 0xdf, 0xa7, 0xbf, 0x6e, 0x60, 0x25, 0x77);

/* ---- The parameterized faces the statics' async members and the boxed physical size answer, stamped from one
 * shape. The completed handlers are delegate objects the guest owns: the operation invokes and releases them and no
 * face of raven ever queries one, so only the shape matters. ---- */

#define RAVEN_DISPLAY_ASYNC_OPERATION(name, result_type) \
	typedef struct __FIAsyncOperationCompletedHandler_1_##name __FIAsyncOperationCompletedHandler_1_##name; \
	typedef struct __FIAsyncOperation_1_##name __FIAsyncOperation_1_##name; \
	typedef struct __FIAsyncOperation_1_##name##Vtbl { \
		BEGIN_INTERFACE \
		HRESULT (STDMETHODCALLTYPE *QueryInterface)(__FIAsyncOperation_1_##name *This, REFIID riid, void **object); \
		ULONG (STDMETHODCALLTYPE *AddRef)(__FIAsyncOperation_1_##name *This); \
		ULONG (STDMETHODCALLTYPE *Release)(__FIAsyncOperation_1_##name *This); \
		HRESULT (STDMETHODCALLTYPE *GetIids)(__FIAsyncOperation_1_##name *This, ULONG *count, IID **iids); \
		HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__FIAsyncOperation_1_##name *This, HSTRING *type_name); \
		HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__FIAsyncOperation_1_##name *This, TrustLevel *level); \
		HRESULT (STDMETHODCALLTYPE *put_Completed)(__FIAsyncOperation_1_##name *This, __FIAsyncOperationCompletedHandler_1_##name *handler); \
		HRESULT (STDMETHODCALLTYPE *get_Completed)(__FIAsyncOperation_1_##name *This, __FIAsyncOperationCompletedHandler_1_##name **handler); \
		HRESULT (STDMETHODCALLTYPE *GetResults)(__FIAsyncOperation_1_##name *This, result_type results); \
		END_INTERFACE \
	} __FIAsyncOperation_1_##name##Vtbl; \
	interface __FIAsyncOperation_1_##name { \
		CONST_VTBL __FIAsyncOperation_1_##name##Vtbl *lpVtbl; \
	}; \
	typedef struct __FIAsyncOperationCompletedHandler_1_##name##Vtbl { \
		HRESULT (STDMETHODCALLTYPE *QueryInterface)(__FIAsyncOperationCompletedHandler_1_##name *This, REFIID riid, void **object); \
		ULONG (STDMETHODCALLTYPE *AddRef)(__FIAsyncOperationCompletedHandler_1_##name *This); \
		ULONG (STDMETHODCALLTYPE *Release)(__FIAsyncOperationCompletedHandler_1_##name *This); \
		HRESULT (STDMETHODCALLTYPE *Invoke)(__FIAsyncOperationCompletedHandler_1_##name *This, __FIAsyncOperation_1_##name *operation, AsyncStatus status); \
	} __FIAsyncOperationCompletedHandler_1_##name##Vtbl; \
	struct __FIAsyncOperationCompletedHandler_1_##name { \
		CONST_VTBL __FIAsyncOperationCompletedHandler_1_##name##Vtbl *lpVtbl; \
	}; \

RAVEN_DISPLAY_ASYNC_OPERATION(Windows__CDevices__CDisplay__CDisplayMonitor, __x_ABI_CWindows_CDevices_CDisplay_CIDisplayMonitor **)

/* IAsyncOperation`1<Windows.Devices.Display.DisplayMonitor>: the signature computation over the instance interface
 * above, the identifier a .NET Native guest computes for the await's cast. */
DEFINE_GUID(IID___FIAsyncOperation_1_Windows__CDevices__CDisplay__CDisplayMonitor, 0x28c0e623, 0x4e46, 0x58c3, 0xad, 0x82, 0x50, 0x2b, 0xde, 0xcc, 0x43, 0x45);

/* IReference`1<Windows.Foundation.Size>: the boxed physical size PhysicalSizeInInches hands out. */
typedef struct __FIReference_1_Windows__CFoundation__CSize __FIReference_1_Windows__CFoundation__CSize;
typedef struct __FIReference_1_Windows__CFoundation__CSizeVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__FIReference_1_Windows__CFoundation__CSize *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__FIReference_1_Windows__CFoundation__CSize *This);
	ULONG (STDMETHODCALLTYPE *Release)(__FIReference_1_Windows__CFoundation__CSize *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__FIReference_1_Windows__CFoundation__CSize *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__FIReference_1_Windows__CFoundation__CSize *This, HSTRING *type_name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__FIReference_1_Windows__CFoundation__CSize *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_Value)(__FIReference_1_Windows__CFoundation__CSize *This, __x_ABI_CWindows_CFoundation_CSize *value);
	END_INTERFACE
} __FIReference_1_Windows__CFoundation__CSizeVtbl;

interface __FIReference_1_Windows__CFoundation__CSize {
	CONST_VTBL __FIReference_1_Windows__CFoundation__CSizeVtbl *lpVtbl;
};

DEFINE_GUID(IID___FIReference_1_Windows__CFoundation__CSize, 0x61723086, 0x8e53, 0x5276, 0x9f, 0x36, 0x2a, 0x4b, 0xb9, 0x3e, 0x2b, 0x75);
