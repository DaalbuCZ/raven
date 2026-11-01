/* Generated from Windows.UI.Xaml.winmd (SHA-256 c55875b03b9829c2e6ed6ad8b2d1ba33bd80c31208ba3bcda1a12ac46eddc41d) by scripts/generate-winrt-header.py. Resources, RequestedTheme and LoadComponent parameter types adapted to the WinRT ABI. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CIApplication __x_ABI_CWindows_CUI_CXaml_CIApplication;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIApplicationVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Resources)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This, IInspectable **a1);
	HRESULT (STDMETHODCALLTYPE *put_Resources)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This, IInspectable *a1);
	HRESULT (STDMETHODCALLTYPE *get_DebugSettings)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_RequestedTheme)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This, INT32 *a1);
	HRESULT (STDMETHODCALLTYPE *put_RequestedTheme)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This, INT32 a1);
	HRESULT (STDMETHODCALLTYPE *add_UnhandledException)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_UnhandledException)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_Suspending)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_Suspending)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_Resuming)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_Resuming)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *Exit)(__x_ABI_CWindows_CUI_CXaml_CIApplication *This);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIApplicationVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIApplication {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIApplicationVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIApplication, 0x74b861a1, 0x7487, 0x46a9, 0x9a, 0x6e, 0xc7, 0x8b, 0x51, 0x27, 0x26, 0xc5);
typedef struct __x_ABI_CWindows_CUI_CXaml_CIApplication2 __x_ABI_CWindows_CUI_CXaml_CIApplication2;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIApplication2Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIApplication2 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIApplication2 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIApplication2 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIApplication2 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIApplication2 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIApplication2 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_FocusVisualKind)(__x_ABI_CWindows_CUI_CXaml_CIApplication2 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_FocusVisualKind)(__x_ABI_CWindows_CUI_CXaml_CIApplication2 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_RequiresPointerMode)(__x_ABI_CWindows_CUI_CXaml_CIApplication2 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_RequiresPointerMode)(__x_ABI_CWindows_CUI_CXaml_CIApplication2 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_LeavingBackground)(__x_ABI_CWindows_CUI_CXaml_CIApplication2 *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_LeavingBackground)(__x_ABI_CWindows_CUI_CXaml_CIApplication2 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_EnteredBackground)(__x_ABI_CWindows_CUI_CXaml_CIApplication2 *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_EnteredBackground)(__x_ABI_CWindows_CUI_CXaml_CIApplication2 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIApplication2Vtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIApplication2 {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIApplication2Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIApplication2, 0x019104be, 0x522a, 0x5904, 0xf5, 0x2f, 0xde, 0x72, 0x01, 0x04, 0x29, 0xe0);
typedef struct __x_ABI_CWindows_CUI_CXaml_CIApplication3 __x_ABI_CWindows_CUI_CXaml_CIApplication3;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIApplication3Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIApplication3 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIApplication3 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIApplication3 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIApplication3 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIApplication3 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIApplication3 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_HighContrastAdjustment)(__x_ABI_CWindows_CUI_CXaml_CIApplication3 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_HighContrastAdjustment)(__x_ABI_CWindows_CUI_CXaml_CIApplication3 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIApplication3Vtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIApplication3 {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIApplication3Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIApplication3, 0xb775ad7c, 0x18b8, 0x45ca, 0xa1, 0xb0, 0xdc, 0x48, 0x3e, 0x4b, 0x10, 0x28);
typedef struct __x_ABI_CWindows_CUI_CXaml_CIApplicationStatics __x_ABI_CWindows_CUI_CXaml_CIApplicationStatics;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIApplicationStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Current)(__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *Start)(__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *LoadComponent)(__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics *This, IInspectable *a1, IInspectable *a2);
	HRESULT (STDMETHODCALLTYPE *LoadComponent2)(__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics *This, IInspectable *a1, IInspectable *a2, INT32 a3);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIApplicationStaticsVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIApplicationStatics {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIApplicationStaticsVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics, 0x06499997, 0xf7b4, 0x45fe, 0xb7, 0x63, 0x75, 0x77, 0xd1, 0xd3, 0xcb, 0x4a);
typedef struct __x_ABI_CWindows_CUI_CXaml_CIApplicationFactory __x_ABI_CWindows_CUI_CXaml_CIApplicationFactory;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIApplicationFactoryVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIApplicationFactory *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIApplicationFactory *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIApplicationFactory *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIApplicationFactory *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIApplicationFactory *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIApplicationFactory *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *CreateInstance)(__x_ABI_CWindows_CUI_CXaml_CIApplicationFactory *This, uint64_t a1, uint64_t a2, uint64_t a3);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIApplicationFactoryVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIApplicationFactory {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIApplicationFactoryVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIApplicationFactory, 0x93bbe361, 0xbe5a, 0x4ee3, 0xb4, 0xa3, 0x95, 0x11, 0x8d, 0xc9, 0x7a, 0x89);
typedef struct __x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParams __x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParams;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParamsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParams *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParams *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParams *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParams *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParams *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParams *This, TrustLevel *level);

	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParamsVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParams {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParamsVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParams, 0x751b792e, 0x5772, 0x4488, 0x8b, 0x87, 0xf5, 0x47, 0xfa, 0xa6, 0x44, 0x74);
