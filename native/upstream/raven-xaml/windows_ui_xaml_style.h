/* Generated from Windows.UI.Xaml.winmd (SHA-256 c55875b03b9829c2e6ed6ad8b2d1ba33bd80c31208ba3bcda1a12ac46eddc41d) by
 * scripts/generate-winrt-header.py. Do not edit the generated section; two kinds of section are hand written, following the
 * precedent of windows_ui_xaml_layout.h and windows_ui_xaml_dictionary.h:
 *
 * - The get_TargetType and put_TargetType slots carry a Windows.UI.Xaml.Interop.TypeName, a 16-byte struct the ABI hands over
 *   by value (a pointer to a temporary on x64, four stack words on x86). mingw declares the real struct in
 *   windows.ui.xaml.interop.h, which the freestanding build includes here, so the slots use it the way the layout header's
 *   GridLength slots do.
 * - Windows.Foundation.Collections.IVector`1<SetterBase> has no winmd of its own; the declaration below is hand derived like
 *   the dictionary's instantiations. The signature
 *   "pinterface({913337e9-11a1-4345-a3a2-4e7f956e222d};rc(Windows.UI.Xaml.SetterBase;
 *   {418be27c-2ac4-4f22-8097-dea3aeeb2fb3}))" derives bd55a7fd-7501-5519-9ffb-d0074240db61; the derivation reproduces the
 *   validated IVector<ResourceDictionary> instantiation exactly and the result appears once in the real Windows.UI.Xaml.dll.
 *   A runtime-class argument carries the GUID of its default interface, so SetterBase's is ISetterBase's. The vtable carries
 *   the twelve IVector`1 methods without the inherited IIterable`1.First, the shape the children vector of xaml_elements.c
 *   uses.
 *
 * Style has no statics class in the winmd: the only factory member is IStyleFactory.CreateInstance. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>
#include <windows.ui.xaml.interop.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CIStyle __x_ABI_CWindows_CUI_CXaml_CIStyle;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIStyleVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIStyle *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIStyle *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIStyle *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIStyle *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIStyle *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIStyle *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_IsSealed)(__x_ABI_CWindows_CUI_CXaml_CIStyle *This, boolean *value);
	HRESULT (STDMETHODCALLTYPE *get_Setters)(__x_ABI_CWindows_CUI_CXaml_CIStyle *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_TargetType)(__x_ABI_CWindows_CUI_CXaml_CIStyle *This,
		__x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName *value);
	HRESULT (STDMETHODCALLTYPE *put_TargetType)(__x_ABI_CWindows_CUI_CXaml_CIStyle *This,
		__x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName value);
	HRESULT (STDMETHODCALLTYPE *get_BasedOn)(__x_ABI_CWindows_CUI_CXaml_CIStyle *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_BasedOn)(__x_ABI_CWindows_CUI_CXaml_CIStyle *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *Seal)(__x_ABI_CWindows_CUI_CXaml_CIStyle *This);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIStyleVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIStyle {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIStyleVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIStyle, 0xc4a9f225, 0x9db7, 0x4a7d, 0xb6, 0xd1, 0xf7, 0x4e, 0xdb, 0x92, 0x93, 0xc2);
typedef struct __x_ABI_CWindows_CUI_CXaml_CIStyleFactory __x_ABI_CWindows_CUI_CXaml_CIStyleFactory;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIStyleFactoryVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIStyleFactory *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIStyleFactory *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIStyleFactory *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIStyleFactory *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIStyleFactory *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIStyleFactory *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *CreateInstance)(__x_ABI_CWindows_CUI_CXaml_CIStyleFactory *This, uint64_t a1, uint64_t a2);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIStyleFactoryVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIStyleFactory {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIStyleFactoryVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIStyleFactory, 0xa36824e3, 0x3d81, 0x4ce5, 0xaa, 0x51, 0x8b, 0x41, 0x0f, 0x60, 0x2f, 0xcd);
typedef struct __x_ABI_CWindows_CUI_CXaml_CISetter __x_ABI_CWindows_CUI_CXaml_CISetter;
typedef struct __x_ABI_CWindows_CUI_CXaml_CISetterVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CISetter *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CISetter *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CISetter *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CISetter *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CISetter *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CISetter *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Property)(__x_ABI_CWindows_CUI_CXaml_CISetter *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Property)(__x_ABI_CWindows_CUI_CXaml_CISetter *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Value)(__x_ABI_CWindows_CUI_CXaml_CISetter *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Value)(__x_ABI_CWindows_CUI_CXaml_CISetter *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CISetterVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CISetter {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CISetterVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CISetter, 0xa73ded29, 0xb4ae, 0x4a81, 0xbe, 0x85, 0xe6, 0x90, 0xba, 0x0d, 0x3b, 0x6e);
typedef struct __x_ABI_CWindows_CUI_CXaml_CISetter2 __x_ABI_CWindows_CUI_CXaml_CISetter2;
typedef struct __x_ABI_CWindows_CUI_CXaml_CISetter2Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CISetter2 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CISetter2 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CISetter2 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CISetter2 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CISetter2 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CISetter2 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Target)(__x_ABI_CWindows_CUI_CXaml_CISetter2 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Target)(__x_ABI_CWindows_CUI_CXaml_CISetter2 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CISetter2Vtbl;

interface __x_ABI_CWindows_CUI_CXaml_CISetter2 {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CISetter2Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CISetter2, 0x70169561, 0x05b1, 0x4fa3, 0x9d, 0x53, 0x8e, 0x0c, 0x8c, 0x74, 0x7a, 0xfc);
typedef struct __x_ABI_CWindows_CUI_CXaml_CISetterBase __x_ABI_CWindows_CUI_CXaml_CISetterBase;
typedef struct __x_ABI_CWindows_CUI_CXaml_CISetterBaseVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CISetterBase *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CISetterBase *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CISetterBase *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CISetterBase *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CISetterBase *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CISetterBase *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_IsSealed)(__x_ABI_CWindows_CUI_CXaml_CISetterBase *This, boolean *value);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CISetterBaseVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CISetterBase {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CISetterBaseVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CISetterBase, 0x418be27c, 0x2ac4, 0x4f22, 0x80, 0x97, 0xde, 0xa3, 0xae, 0xeb, 0x2f, 0xb3);
typedef struct __x_ABI_CWindows_CUI_CXaml_CISetterBaseCollection __x_ABI_CWindows_CUI_CXaml_CISetterBaseCollection;
typedef struct __x_ABI_CWindows_CUI_CXaml_CISetterBaseCollectionVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CISetterBaseCollection *This, REFIID riid,
		void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CISetterBaseCollection *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CISetterBaseCollection *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CISetterBaseCollection *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CISetterBaseCollection *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CISetterBaseCollection *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_IsSealed)(__x_ABI_CWindows_CUI_CXaml_CISetterBaseCollection *This, boolean *value);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CISetterBaseCollectionVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CISetterBaseCollection {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CISetterBaseCollectionVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CISetterBaseCollection, 0x03c40ca8, 0x909e, 0x4117, 0x81, 0x1c, 0xa4, 0x52, 0x94, 0x96, 0xbd, 0xf1);
typedef struct __x_ABI_CWindows_CUI_CXaml_CISetterFactory __x_ABI_CWindows_CUI_CXaml_CISetterFactory;
typedef struct __x_ABI_CWindows_CUI_CXaml_CISetterFactoryVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CISetterFactory *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CISetterFactory *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CISetterFactory *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CISetterFactory *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CISetterFactory *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CISetterFactory *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *CreateInstance)(__x_ABI_CWindows_CUI_CXaml_CISetterFactory *This, uint64_t a1, uint64_t a2,
		uint64_t a3);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CISetterFactoryVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CISetterFactory {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CISetterFactoryVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CISetterFactory, 0xd3ca3d42, 0x09b1, 0x49d5, 0x88, 0x91, 0xe7, 0xb5, 0x64, 0x8e, 0x02, 0xa2);

/* Windows.Foundation.Collections.IVector`1<SetterBase>, the type of IStyle.Setters; see the derivation note above. */
typedef struct __FIVector_1_Windows__CUI__CXaml__CSetterBase __FIVector_1_Windows__CUI__CXaml__CSetterBase;
typedef struct __FIVector_1_Windows__CUI__CXaml__CSetterBaseVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This, REFIID riid,
		void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This);
	ULONG (STDMETHODCALLTYPE *Release)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetAt)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This, UINT32 index,
		__x_ABI_CWindows_CUI_CXaml_CISetterBase **value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *GetView)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *IndexOf)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This,
		__x_ABI_CWindows_CUI_CXaml_CISetterBase *value, UINT32 *index, boolean *found);
	HRESULT (STDMETHODCALLTYPE *SetAt)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This, UINT32 index,
		__x_ABI_CWindows_CUI_CXaml_CISetterBase *value);
	HRESULT (STDMETHODCALLTYPE *InsertAt)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This, UINT32 index,
		__x_ABI_CWindows_CUI_CXaml_CISetterBase *value);
	HRESULT (STDMETHODCALLTYPE *RemoveAt)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This, UINT32 index);
	HRESULT (STDMETHODCALLTYPE *Append)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This,
		__x_ABI_CWindows_CUI_CXaml_CISetterBase *value);
	HRESULT (STDMETHODCALLTYPE *RemoveAtEnd)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This);
	HRESULT (STDMETHODCALLTYPE *Clear)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This);
	HRESULT (STDMETHODCALLTYPE *GetMany)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This, UINT32 start_index,
		UINT32 items_size, __x_ABI_CWindows_CUI_CXaml_CISetterBase **items, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *ReplaceAll)(__FIVector_1_Windows__CUI__CXaml__CSetterBase *This, UINT32 count,
		__x_ABI_CWindows_CUI_CXaml_CISetterBase **items);
	END_INTERFACE
} __FIVector_1_Windows__CUI__CXaml__CSetterBaseVtbl;

interface __FIVector_1_Windows__CUI__CXaml__CSetterBase {
	CONST_VTBL __FIVector_1_Windows__CUI__CXaml__CSetterBaseVtbl *lpVtbl;
};

DEFINE_GUID(IID___FIVector_1_Windows__CUI__CXaml__CSetterBase, 0xbd55a7fd, 0x7501, 0x5519, 0x9f, 0xfb, 0xd0, 0x07, 0x42, 0x40, 0xdb, 0x61);
