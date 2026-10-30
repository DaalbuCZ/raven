/* Generated from Windows.UI.Xaml.winmd (SHA-256 c55875b03b9829c2e6ed6ad8b2d1ba33bd80c31208ba3bcda1a12ac46eddc41d) by scripts/generate-winrt-header.py. Do not edit. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_CacheSize)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_CacheSize)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_CanGoBack)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_CanGoForward)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_CurrentSourcePageType)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_SourcePageType)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_SourcePageType)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_BackStackDepth)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_Navigated)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_Navigated)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_Navigating)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_Navigating)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_NavigationFailed)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_NavigationFailed)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_NavigationStopped)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_NavigationStopped)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GoBack)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This);
	HRESULT (STDMETHODCALLTYPE *GoForward)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This);
	HRESULT (STDMETHODCALLTYPE *Navigate)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1, uint64_t a2, uint64_t a3);
	HRESULT (STDMETHODCALLTYPE *GetNavigationState)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *SetNavigationState)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame, 0x68ea500b, 0x3fe9, 0x4735, 0xb6, 0xa0, 0xe7, 0x60, 0x19, 0x33, 0xb0, 0x89);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2 __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_BackStack)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_ForwardStack)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *Navigate)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2 *This, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2Vtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2 {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2, 0x3f367f87, 0x9f5a, 0x4b04, 0xb8, 0x18, 0xb5, 0x54, 0xc0, 0x69, 0x59, 0x7a);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3 __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *GoBack)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3Vtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3 {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3, 0x648a2b4d, 0x53ca, 0x4b5a, 0xaa, 0x8e, 0x3c, 0xc7, 0x44, 0x0f, 0x4a, 0x67);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4 __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *SetNavigationState)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4 *This, uint64_t a1, uint64_t a2);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4Vtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4 {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4, 0x9b17c21a, 0xbd2b, 0x4a00, 0x99, 0xeb, 0x94, 0x6b, 0xe9, 0x61, 0x80, 0x84);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5 __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_IsNavigationStackEnabled)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_IsNavigationStackEnabled)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *NavigateToType)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5 *This, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5Vtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5 {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5, 0x453fb92e, 0x82b5, 0x5f93, 0x92, 0xab, 0x15, 0x34, 0x18, 0x06, 0xc9, 0xda);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactory __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactory;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactoryVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactory *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactory *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactory *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactory *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactory *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactory *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *CreateInstance)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactory *This, uint64_t a1, uint64_t a2, uint64_t a3);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactoryVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactory {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactoryVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactory, 0x02ee93d4, 0x448e, 0x469e, 0x97, 0x99, 0x0a, 0x8a, 0x1f, 0x70, 0xf1, 0x71);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_CacheSizeProperty)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_CanGoBackProperty)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_CanGoForwardProperty)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_CurrentSourcePageTypeProperty)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_SourcePageTypeProperty)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_BackStackDepthProperty)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStaticsVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStaticsVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics, 0xd5acf0e9, 0xd967, 0x471b, 0x84, 0x59, 0xc8, 0xaf, 0x43, 0x41, 0x51, 0x1b);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2 __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_BackStackProperty)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_ForwardStackProperty)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2Vtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2 {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2, 0xe4b5cdfe, 0x42f0, 0x4462, 0xa1, 0xc1, 0x5d, 0x25, 0x9d, 0xd8, 0x2a, 0x03);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5 __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_IsNavigationStackEnabledProperty)(__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5Vtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5 {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5, 0x53c89b25, 0xf0e0, 0x58f1, 0xa0, 0x6d, 0x44, 0x10, 0x75, 0x91, 0x4f, 0xf8);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIPage __x_ABI_CWindows_CUI_CXaml_CControls_CIPage;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIPageVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Frame)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_NavigationCacheMode)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_NavigationCacheMode)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_TopAppBar)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_TopAppBar)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_BottomAppBar)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_BottomAppBar)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIPageVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIPage {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIPageVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPage, 0xc4b27075, 0xe641, 0x453e, 0x82, 0x4d, 0x01, 0x2f, 0xc7, 0xcf, 0x95, 0xcf);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactory __x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactory;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactoryVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactory *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactory *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactory *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactory *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactory *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactory *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *CreateInstance)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactory *This, uint64_t a1, uint64_t a2, uint64_t a3);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactoryVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactory {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactoryVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactory, 0xdfa149ac, 0x1849, 0x445e, 0x93, 0x7c, 0x40, 0xa9, 0x59, 0x0c, 0xc0, 0x76);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides __x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverridesVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *OnNavigatedFrom)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *OnNavigatedTo)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *OnNavigatingFrom)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverridesVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverridesVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides, 0x230b9cb8, 0x3419, 0x4306, 0xaa, 0xbe, 0x4a, 0x8e, 0x68, 0x0d, 0x36, 0x27);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIPageStatics __x_ABI_CWindows_CUI_CXaml_CControls_CIPageStatics;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIPageStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageStatics *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageStatics *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageStatics *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageStatics *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageStatics *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageStatics *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_FrameProperty)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageStatics *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_TopAppBarProperty)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageStatics *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_BottomAppBarProperty)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageStatics *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIPageStaticsVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIPageStatics {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIPageStaticsVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPageStatics, 0x376dde71, 0xf725, 0x46dd, 0x8d, 0x80, 0xee, 0xea, 0xf7, 0xda, 0x74, 0xe5);
