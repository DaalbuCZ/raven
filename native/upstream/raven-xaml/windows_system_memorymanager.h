/* Generated from Windows.System.winmd (SHA-256 c22b67a7e366fb646b220f631f212d3271fbdef454b0c31d7c9b3f0b78e646e6) by scripts/generate-winrt-header.py. Do not edit. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CSystem_CIMemoryManagerStatics __x_ABI_CWindows_CSystem_CIMemoryManagerStatics;
typedef struct __x_ABI_CWindows_CSystem_CIMemoryManagerStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_AppMemoryUsage)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_AppMemoryUsageLimit)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_AppMemoryUsageLevel)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_AppMemoryUsageIncreased)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_AppMemoryUsageIncreased)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_AppMemoryUsageDecreased)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_AppMemoryUsageDecreased)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_AppMemoryUsageLimitChanging)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_AppMemoryUsageLimitChanging)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CSystem_CIMemoryManagerStaticsVtbl;

interface __x_ABI_CWindows_CSystem_CIMemoryManagerStatics {
	CONST_VTBL __x_ABI_CWindows_CSystem_CIMemoryManagerStaticsVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CSystem_CIMemoryManagerStatics, 0x5c6c279c, 0xd7ca, 0x4779, 0x91, 0x88, 0x40, 0x57, 0x21, 0x9c, 0xe6, 0x4c);
typedef struct __x_ABI_CWindows_CSystem_CIMemoryManagerStatics2 __x_ABI_CWindows_CSystem_CIMemoryManagerStatics2;
typedef struct __x_ABI_CWindows_CSystem_CIMemoryManagerStatics2Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics2 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics2 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics2 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics2 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics2 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics2 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *GetAppMemoryReport)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics2 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetProcessMemoryReport)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics2 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CSystem_CIMemoryManagerStatics2Vtbl;

interface __x_ABI_CWindows_CSystem_CIMemoryManagerStatics2 {
	CONST_VTBL __x_ABI_CWindows_CSystem_CIMemoryManagerStatics2Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CSystem_CIMemoryManagerStatics2, 0x6eee351f, 0x6d62, 0x423f, 0x94, 0x79, 0xb0, 0x1f, 0x9c, 0x9f, 0x76, 0x69);
typedef struct __x_ABI_CWindows_CSystem_CIMemoryManagerStatics3 __x_ABI_CWindows_CSystem_CIMemoryManagerStatics3;
typedef struct __x_ABI_CWindows_CSystem_CIMemoryManagerStatics3Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics3 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics3 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics3 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics3 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics3 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics3 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *TrySetAppMemoryUsageLimit)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics3 *This, uint64_t a1, uint64_t a2);
	END_INTERFACE
} __x_ABI_CWindows_CSystem_CIMemoryManagerStatics3Vtbl;

interface __x_ABI_CWindows_CSystem_CIMemoryManagerStatics3 {
	CONST_VTBL __x_ABI_CWindows_CSystem_CIMemoryManagerStatics3Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CSystem_CIMemoryManagerStatics3, 0x149b59ce, 0x92ad, 0x4e35, 0x89, 0xeb, 0x50, 0xdf, 0xb4, 0xc0, 0xd9, 0x1c);
typedef struct __x_ABI_CWindows_CSystem_CIMemoryManagerStatics4 __x_ABI_CWindows_CSystem_CIMemoryManagerStatics4;
typedef struct __x_ABI_CWindows_CSystem_CIMemoryManagerStatics4Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics4 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics4 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics4 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics4 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics4 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics4 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_ExpectedAppMemoryUsageLimit)(__x_ABI_CWindows_CSystem_CIMemoryManagerStatics4 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CSystem_CIMemoryManagerStatics4Vtbl;

interface __x_ABI_CWindows_CSystem_CIMemoryManagerStatics4 {
	CONST_VTBL __x_ABI_CWindows_CSystem_CIMemoryManagerStatics4Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CSystem_CIMemoryManagerStatics4, 0xc5a94828, 0xe84e, 0x4886, 0x8a, 0x0d, 0x44, 0xb3, 0x19, 0x0e, 0x3b, 0x72);
typedef struct __x_ABI_CWindows_CSystem_CIAppMemoryReport __x_ABI_CWindows_CSystem_CIAppMemoryReport;
typedef struct __x_ABI_CWindows_CSystem_CIAppMemoryReportVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CSystem_CIAppMemoryReport *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CSystem_CIAppMemoryReport *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CSystem_CIAppMemoryReport *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CSystem_CIAppMemoryReport *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CSystem_CIAppMemoryReport *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CSystem_CIAppMemoryReport *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_PrivateCommitUsage)(__x_ABI_CWindows_CSystem_CIAppMemoryReport *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_PeakPrivateCommitUsage)(__x_ABI_CWindows_CSystem_CIAppMemoryReport *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_TotalCommitUsage)(__x_ABI_CWindows_CSystem_CIAppMemoryReport *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_TotalCommitLimit)(__x_ABI_CWindows_CSystem_CIAppMemoryReport *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CSystem_CIAppMemoryReportVtbl;

interface __x_ABI_CWindows_CSystem_CIAppMemoryReport {
	CONST_VTBL __x_ABI_CWindows_CSystem_CIAppMemoryReportVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CSystem_CIAppMemoryReport, 0x6d65339b, 0x4d6f, 0x45bc, 0x9c, 0x5e, 0xe4, 0x9b, 0x3f, 0xf2, 0x75, 0x8d);
typedef struct __x_ABI_CWindows_CSystem_CIAppMemoryReport2 __x_ABI_CWindows_CSystem_CIAppMemoryReport2;
typedef struct __x_ABI_CWindows_CSystem_CIAppMemoryReport2Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CSystem_CIAppMemoryReport2 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CSystem_CIAppMemoryReport2 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CSystem_CIAppMemoryReport2 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CSystem_CIAppMemoryReport2 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CSystem_CIAppMemoryReport2 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CSystem_CIAppMemoryReport2 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_ExpectedTotalCommitLimit)(__x_ABI_CWindows_CSystem_CIAppMemoryReport2 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CSystem_CIAppMemoryReport2Vtbl;

interface __x_ABI_CWindows_CSystem_CIAppMemoryReport2 {
	CONST_VTBL __x_ABI_CWindows_CSystem_CIAppMemoryReport2Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CSystem_CIAppMemoryReport2, 0x5f7f3738, 0x51b7, 0x42dc, 0xb7, 0xed, 0x79, 0xba, 0x46, 0xd2, 0x88, 0x57);
typedef struct __x_ABI_CWindows_CSystem_CIProcessMemoryReport __x_ABI_CWindows_CSystem_CIProcessMemoryReport;
typedef struct __x_ABI_CWindows_CSystem_CIProcessMemoryReportVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CSystem_CIProcessMemoryReport *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CSystem_CIProcessMemoryReport *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CSystem_CIProcessMemoryReport *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CSystem_CIProcessMemoryReport *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CSystem_CIProcessMemoryReport *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CSystem_CIProcessMemoryReport *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_PrivateWorkingSetUsage)(__x_ABI_CWindows_CSystem_CIProcessMemoryReport *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_TotalWorkingSetUsage)(__x_ABI_CWindows_CSystem_CIProcessMemoryReport *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CSystem_CIProcessMemoryReportVtbl;

interface __x_ABI_CWindows_CSystem_CIProcessMemoryReport {
	CONST_VTBL __x_ABI_CWindows_CSystem_CIProcessMemoryReportVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CSystem_CIProcessMemoryReport, 0x087305a8, 0x9b70, 0x4782, 0x87, 0x41, 0x3a, 0x98, 0x2b, 0x6c, 0xe5, 0xe4);
