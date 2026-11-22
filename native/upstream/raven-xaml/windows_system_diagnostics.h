/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The Windows.System.Diagnostics interfaces this component serves, in the shape scripts/generate-winrt-header.py's
 * committed headers take: vtable order and interface identifiers hand-extracted from the windows-rs metadata that
 * generator pins (revision 58217a69, crates/libs/windows/src/Windows/System/Diagnostics/mod.rs, which the generator
 * does not declare). The default interface's identifier is the one a guest's factory query names for the class's
 * statics, and the members' order is the vtable order a guest binary is compiled against. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics;
typedef struct __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *GetForCurrentSystem)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStaticsVtbl;

interface __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics {
	CONST_VTBL __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStaticsVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics, 0xd404ac21, 0xfc7d, 0x45f0, 0x9a, 0x3f, 0x39, 0x20, 0x3a, 0xed, 0x9f, 0x7e);
typedef struct __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2 __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2;
typedef struct __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *IsArchitectureSupported)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2 *This, int32_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *get_PreferredArchitecture)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2Vtbl;

interface __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2 {
	CONST_VTBL __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2, 0x79ded189, 0x6af9, 0x4da9, 0xa4, 0x22, 0x15, 0xf7, 0x32, 0x55, 0xb3, 0xeb);
typedef struct __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfo __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfo;
typedef struct __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfo *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfo *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfo *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfo *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfo *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfo *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_MemoryUsage)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfo *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_CpuUsage)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfo *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoVtbl;

interface __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfo {
	CONST_VTBL __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfo, 0xa290fe05, 0xdff3, 0x407f, 0x9a, 0x1b, 0x0b, 0x2b, 0x31, 0x7c, 0xa8, 0x00);
typedef struct __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsage __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsage;
typedef struct __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsage *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsage *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsage *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsage *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsage *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsage *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *GetReport)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsage *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageVtbl;

interface __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsage {
	CONST_VTBL __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsage, 0x6037b3ac, 0x02d6, 0x4234, 0x83, 0x62, 0x7f, 0xe3, 0xad, 0xc8, 0x1f, 0x5f);
typedef struct __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport;
typedef struct __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReportVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_KernelTime)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_UserTime)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IdleTime)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReportVtbl;

interface __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport {
	CONST_VTBL __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReportVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport, 0x2c26d0b2, 0x9483, 0x4f62, 0xab, 0x57, 0x82, 0xb2, 0x9d, 0x97, 0x19, 0xb8);
typedef struct __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsage __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsage;
typedef struct __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsage *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsage *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsage *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsage *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsage *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsage *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *GetReport)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsage *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageVtbl;

interface __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsage {
	CONST_VTBL __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsage, 0x17ffc595, 0x1702, 0x49cf, 0xaa, 0x27, 0x2f, 0x0a, 0x32, 0x59, 0x14, 0x04);
typedef struct __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport;
typedef struct __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReportVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_TotalPhysicalSizeInBytes)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_AvailableSizeInBytes)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_CommittedSizeInBytes)(__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReportVtbl;

interface __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport {
	CONST_VTBL __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReportVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport, 0x38663c87, 0x2a9f, 0x403a, 0xbd, 0x19, 0x2c, 0xf3, 0xe8, 0x16, 0x95, 0x00);
