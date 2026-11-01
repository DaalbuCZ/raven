/* Generated from Windows.UI.Xaml.winmd (SHA-256 c55875b03b9829c2e6ed6ad8b2d1ba33bd80c31208ba3bcda1a12ac46eddc41d) by scripts/generate-winrt-header.py. Pointer argument types adapted to the WinRT ABI. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgs __x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgs;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgs *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgs *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgs *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgs *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgs *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgs *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Window)(__x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgs *This, uintptr_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgsVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgs {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgsVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIWindowCreatedEventArgs, 0x31b71470, 0xfeff, 0x4654, 0xaf, 0x48, 0x9b, 0x39, 0x8a, 0xb5, 0x77, 0x2b);
