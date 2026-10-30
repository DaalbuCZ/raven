/* Generated from Windows.UI.Xaml.winmd (SHA-256 c55875b03b9829c2e6ed6ad8b2d1ba33bd80c31208ba3bcda1a12ac46eddc41d) by scripts/generate-winrt-header.py. Do not edit. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CIWindow __x_ABI_CWindows_CUI_CXaml_CIWindow;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIWindowVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Bounds)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Visible)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Content)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Content)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_CoreWindow)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Dispatcher)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_Activated)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_Activated)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_Closed)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_Closed)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_SizeChanged)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_SizeChanged)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_VisibilityChanged)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_VisibilityChanged)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *Activate)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This);
	HRESULT (STDMETHODCALLTYPE *Close)(__x_ABI_CWindows_CUI_CXaml_CIWindow *This);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIWindowVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIWindow {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIWindowVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIWindow, 0x3276167d, 0xc9f6, 0x462d, 0x9d, 0xe2, 0xae, 0x4c, 0x1f, 0xd8, 0xc2, 0xe5);
typedef struct __x_ABI_CWindows_CUI_CXaml_CIWindow2 __x_ABI_CWindows_CUI_CXaml_CIWindow2;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIWindow2Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIWindow2 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIWindow2 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIWindow2 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIWindow2 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIWindow2 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIWindow2 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *SetTitleBar)(__x_ABI_CWindows_CUI_CXaml_CIWindow2 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIWindow2Vtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIWindow2 {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIWindow2Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIWindow2, 0xd384759f, 0x34f6, 0x4482, 0x84, 0x35, 0xf5, 0x52, 0xf9, 0xb2, 0x4c, 0xc8);
typedef struct __x_ABI_CWindows_CUI_CXaml_CIWindow3 __x_ABI_CWindows_CUI_CXaml_CIWindow3;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIWindow3Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIWindow3 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIWindow3 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIWindow3 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIWindow3 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIWindow3 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIWindow3 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Compositor)(__x_ABI_CWindows_CUI_CXaml_CIWindow3 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIWindow3Vtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIWindow3 {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIWindow3Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIWindow3, 0xb70bdc9d, 0x1c35, 0x462a, 0x9b, 0x97, 0x80, 0x8d, 0x5a, 0xf9, 0xf2, 0x8e);
typedef struct __x_ABI_CWindows_CUI_CXaml_CIWindow4 __x_ABI_CWindows_CUI_CXaml_CIWindow4;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIWindow4Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIWindow4 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIWindow4 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIWindow4 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIWindow4 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIWindow4 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIWindow4 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_UIContext)(__x_ABI_CWindows_CUI_CXaml_CIWindow4 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIWindow4Vtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIWindow4 {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIWindow4Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIWindow4, 0xbfe1b8ce, 0x6c40, 0x50f9, 0x85, 0x4c, 0x70, 0x21, 0xd2, 0xbc, 0x9d, 0xe6);
typedef struct __x_ABI_CWindows_CUI_CXaml_CIWindowStatics __x_ABI_CWindows_CUI_CXaml_CIWindowStatics;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIWindowStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIWindowStatics *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIWindowStatics *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIWindowStatics *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIWindowStatics *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIWindowStatics *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIWindowStatics *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Current)(__x_ABI_CWindows_CUI_CXaml_CIWindowStatics *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIWindowStaticsVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIWindowStatics {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIWindowStaticsVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIWindowStatics, 0x93328409, 0x4ea1, 0x4afa, 0x83, 0xdc, 0x0c, 0x4e, 0x73, 0xe8, 0x8b, 0xb1);
