/* Generated from Windows.UI.Xaml.winmd (SHA-256 c55875b03b9829c2e6ed6ad8b2d1ba33bd80c31208ba3bcda1a12ac46eddc41d) by scripts/generate-winrt-header.py. Do not edit. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase __x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBaseVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_ClickMode)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_ClickMode)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsPointerOver)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsPressed)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Command)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Command)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_CommandParameter)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_CommandParameter)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_Click)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_Click)(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBaseVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBaseVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase, 0xfa002c1a, 0x494e, 0x46cf, 0x91, 0xd4, 0xe1, 0x4a, 0x8d, 0x79, 0x86, 0x74);
