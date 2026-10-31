/* Generated from Windows.UI.Xaml.winmd (SHA-256 c55875b03b9829c2e6ed6ad8b2d1ba33bd80c31208ba3bcda1a12ac46eddc41d) by scripts/generate-winrt-header.py. Do not edit. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProvider __x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProvider;
typedef struct __x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProviderVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProvider *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProvider *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProvider *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProvider *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProvider *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProvider *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *Invoke)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProvider *This);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProviderVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProvider {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProviderVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProvider, 0xf7d1a187, 0xb13c, 0x4540, 0xb0, 0x9e, 0x67, 0x78, 0xe2, 0xdc, 0x9b, 0xa5);
