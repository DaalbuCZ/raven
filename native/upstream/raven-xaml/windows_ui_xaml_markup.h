/* Generated from Windows.UI.Xaml.winmd (SHA-256 c55875b03b9829c2e6ed6ad8b2d1ba33bd80c31208ba3bcda1a12ac46eddc41d) by scripts/generate-winrt-header.py. Do not edit. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider;
typedef struct __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProviderVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *GetXamlType)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *GetXamlType2)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *GetXmlnsDefinitions)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProviderVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProviderVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider, 0xb3765d69, 0x68a5, 0x4b32, 0x88, 0x61, 0xfd, 0xb9, 0x0c, 0x1f, 0x58, 0x36);
typedef struct __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType;
typedef struct __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlTypeVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_BaseType)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_ContentProperty)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_FullName)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsArray)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsCollection)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsConstructible)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsDictionary)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsMarkupExtension)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsBindable)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_ItemType)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_KeyType)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_UnderlyingType)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *ActivateInstance)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *CreateFromString)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *GetMember)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *AddToVector)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *AddToMap)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This, uint64_t a1, uint64_t a2, uint64_t a3);
	HRESULT (STDMETHODCALLTYPE *RunInitializer)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *This);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlTypeVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlTypeVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType, 0x7920eab1, 0xa2e5, 0x479a, 0xbd, 0x50, 0x6c, 0xef, 0x3c, 0x0b, 0x49, 0x70);
typedef struct __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType2 __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType2;
typedef struct __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType2Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType2 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType2 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType2 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType2 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType2 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType2 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_BoxedType)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType2 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType2Vtbl;

interface __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType2 {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType2Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType2, 0x9f0c6e3b, 0x433b, 0x56ad, 0x8f, 0x69, 0x78, 0xa4, 0xdd, 0x3e, 0x64, 0xf9);
typedef struct __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember;
typedef struct __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMemberVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_IsAttachable)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsDependencyProperty)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsReadOnly)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Name)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_TargetType)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Type)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetValue)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *SetValue)(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember *This, uint64_t a1, uint64_t a2);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMemberVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMemberVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMember, 0xc541f58c, 0x43a9, 0x4216, 0xb7, 0x18, 0xe0, 0xb1, 0x1b, 0x14, 0xe9, 0x3e);
