/* Generated from Windows.ApplicationModel.winmd (SHA-256 25c17722258acd8f3370612d6c34c6464578741f28bacbfa2e7fe56c1d1536f2) by scripts/generate-winrt-header.py. Do not edit. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext;
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_QualifierValues)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *Reset)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext *This);
	HRESULT (STDMETHODCALLTYPE *Reset2)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *OverrideToMatch)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *Clone)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Languages)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Languages)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextVtbl;

interface __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext {
	CONST_VTBL __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext, 0x2fa22f4b, 0x707e, 0x4b27, 0xad, 0x0d, 0xd0, 0xd8, 0xcd, 0x46, 0x8f, 0xd2);
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics;
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *CreateMatchingContext)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics *This, uint64_t a1, uint64_t a2);
	END_INTERFACE
} __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStaticsVtbl;

interface __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics {
	CONST_VTBL __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStaticsVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics, 0x98be9d6c, 0x6338, 0x4b31, 0x99, 0xdf, 0xb2, 0xb4, 0x42, 0xf1, 0x71, 0x49);
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2;
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *GetForCurrentView)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *SetGlobalQualifierValue)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *ResetGlobalQualifierValues)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 *This);
	HRESULT (STDMETHODCALLTYPE *ResetGlobalQualifierValues2)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetForViewIndependentUse)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2Vtbl;

interface __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 {
	CONST_VTBL __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2, 0x41f752ef, 0x12af, 0x41b9, 0xab, 0x36, 0xb1, 0xeb, 0x4b, 0x51, 0x24, 0x60);
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3 __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3;
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *SetGlobalQualifierValue)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3 *This, uint64_t a1, uint64_t a2, uint64_t a3);
	END_INTERFACE
} __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3Vtbl;

interface __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3 {
	CONST_VTBL __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3, 0x20cf492c, 0xaf0f, 0x450b, 0x9d, 0xa6, 0x10, 0x6d, 0xd0, 0xc2, 0x9a, 0x39);
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4 __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4;
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *GetForUIContext)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4 *This, uint64_t a1, uint64_t a2);
	END_INTERFACE
} __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4Vtbl;

interface __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4 {
	CONST_VTBL __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4, 0x22eb9ccd, 0xfb31, 0x4bfa, 0xb8, 0x6b, 0xdf, 0x9d, 0x9d, 0x7b, 0xdc, 0x39);
