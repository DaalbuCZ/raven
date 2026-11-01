/* Generated from Windows.UI.Xaml.winmd (SHA-256 c55875b03b9829c2e6ed6ad8b2d1ba33bd80c31208ba3bcda1a12ac46eddc41d) by scripts/generate-winrt-header.py. Pointer argument types adapted to the WinRT ABI. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides __x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIApplicationOverridesVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *OnActivated)(__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides *This, uintptr_t a1);
	HRESULT (STDMETHODCALLTYPE *OnLaunched)(__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides *This, uintptr_t a1);
	HRESULT (STDMETHODCALLTYPE *OnFileActivated)(__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides *This, uintptr_t a1);
	HRESULT (STDMETHODCALLTYPE *OnSearchActivated)(__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides *This, uintptr_t a1);
	HRESULT (STDMETHODCALLTYPE *OnShareTargetActivated)(__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides *This, uintptr_t a1);
	HRESULT (STDMETHODCALLTYPE *OnFileOpenPickerActivated)(__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides *This, uintptr_t a1);
	HRESULT (STDMETHODCALLTYPE *OnFileSavePickerActivated)(__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides *This, uintptr_t a1);
	HRESULT (STDMETHODCALLTYPE *OnCachedFileUpdaterActivated)(__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides *This, uintptr_t a1);
	HRESULT (STDMETHODCALLTYPE *OnWindowCreated)(__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides *This, uintptr_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIApplicationOverridesVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIApplicationOverridesVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIApplicationOverrides, 0x25f99ff7, 0x9347, 0x459a, 0x9f, 0xac, 0xb2, 0xd0, 0xe1, 0x1c, 0x1a, 0x0f);
