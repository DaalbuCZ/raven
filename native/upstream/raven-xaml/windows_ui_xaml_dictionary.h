/* Generated from Windows.UI.Xaml.winmd (SHA-256 c55875b03b9829c2e6ed6ad8b2d1ba33bd80c31208ba3bcda1a12ac46eddc41d) by
 * scripts/generate-winrt-header.py; get_MergedDictionaries is adapted to its pointer ABI. The parameterized-interface
 * sections below are hand derived, because a generic instantiation has no winmd of its own (see the derivation notes). */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CIResourceDictionary __x_ABI_CWindows_CUI_CXaml_CIResourceDictionary;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIResourceDictionaryVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Source)(__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Source)(__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_MergedDictionaries)(__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *This, void **a1);
	HRESULT (STDMETHODCALLTYPE *get_ThemeDictionaries)(__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIResourceDictionaryVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIResourceDictionary {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIResourceDictionaryVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary, 0xc1ea4f24, 0xd6de, 0x4191, 0x8e, 0x3a, 0xf4, 0x86, 0x01, 0xf7, 0x48, 0x9c);

/* Windows.Foundation.Collections.IReference`1<HSTRING> — the boxed string. The compiled markup and a WinRT caller pass string
 * resource keys as this; the dictionary compares two such keys by their string content. mingw has no precomputed instantiation
 * for it. The IID is the signature rule's SHA-1 over the namespace GUID d57af411-737b-c042-abae-878b1e16adee in little-endian
 * bytes plus the UTF-8 signature "pinterface({61c17706-2d65-11e0-9ae8-d48564015472};string)" (IReference`1's GUID is from
 * Windows.Foundation.winmd), with the version and variant fixups; the same derivation reproduces mingw's precomputed
 * IVector<HSTRING>, IMap<HSTRING,HSTRING> and IMap<HSTRING,IInspectable> exactly, and the result appears in the real
 * Windows.UI.Xaml.dll. */
typedef struct __FIReference_1_HSTRING __FIReference_1_HSTRING;
typedef struct __FIReference_1_HSTRINGVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__FIReference_1_HSTRING *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__FIReference_1_HSTRING *This);
	ULONG (STDMETHODCALLTYPE *Release)(__FIReference_1_HSTRING *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__FIReference_1_HSTRING *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__FIReference_1_HSTRING *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__FIReference_1_HSTRING *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_Value)(__FIReference_1_HSTRING *This, HSTRING *value);
	END_INTERFACE
} __FIReference_1_HSTRINGVtbl;

interface __FIReference_1_HSTRING {
	CONST_VTBL __FIReference_1_HSTRINGVtbl *lpVtbl;
};

DEFINE_GUID(IID___FIReference_1_HSTRING, 0xfd416dfb, 0x2a07, 0x52eb, 0xaa, 0xe3, 0xdf, 0xce, 0x14, 0x11, 0x6c, 0x05);

/* Windows.Foundation.Collections.IMap`2<IInspectable, IInspectable>: the default view of a ResourceDictionary's contents,
 * which IResourceDictionary derives from. The signature
 * "pinterface({3c2925fe-8519-45c1-aa79-197b6718c1c1};cinterface(IInspectable);cinterface(IInspectable))" derives
 * f5f69427-55ed-5512-8429-d4f6626dfcdd; it appears once in the real Windows.UI.Xaml.dll. The vtable carries the seven methods
 * of the generic definition in metadata order — Lookup, get_Size, HasKey, GetView, Insert, Remove, Clear — without the
 * inherited IIterable`1.First, the same shape mingw's precomputed IMap<HSTRING,HSTRING> declares and the generator confirms
 * against the winmd. */
typedef struct __FIMapView_2_IInspectable_IInspectable __FIMapView_2_IInspectable_IInspectable;
typedef struct __FIMap_2_IInspectable_IInspectable __FIMap_2_IInspectable_IInspectable;
typedef struct __FIMap_2_IInspectable_IInspectableVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__FIMap_2_IInspectable_IInspectable *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__FIMap_2_IInspectable_IInspectable *This);
	ULONG (STDMETHODCALLTYPE *Release)(__FIMap_2_IInspectable_IInspectable *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__FIMap_2_IInspectable_IInspectable *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__FIMap_2_IInspectable_IInspectable *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__FIMap_2_IInspectable_IInspectable *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *Lookup)(__FIMap_2_IInspectable_IInspectable *This, IInspectable *key, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(__FIMap_2_IInspectable_IInspectable *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *HasKey)(__FIMap_2_IInspectable_IInspectable *This, IInspectable *key, boolean *found);
	HRESULT (STDMETHODCALLTYPE *GetView)(__FIMap_2_IInspectable_IInspectable *This,
		__FIMapView_2_IInspectable_IInspectable **view);
	HRESULT (STDMETHODCALLTYPE *Insert)(__FIMap_2_IInspectable_IInspectable *This, IInspectable *key, IInspectable *value,
		boolean *replaced);
	HRESULT (STDMETHODCALLTYPE *Remove)(__FIMap_2_IInspectable_IInspectable *This, IInspectable *key);
	HRESULT (STDMETHODCALLTYPE *Clear)(__FIMap_2_IInspectable_IInspectable *This);
	END_INTERFACE
} __FIMap_2_IInspectable_IInspectableVtbl;

interface __FIMap_2_IInspectable_IInspectable {
	CONST_VTBL __FIMap_2_IInspectable_IInspectableVtbl *lpVtbl;
};

DEFINE_GUID(IID___FIMap_2_IInspectable_IInspectable, 0xf5f69427, 0x55ed, 0x5512, 0x84, 0x29, 0xd4, 0xf6, 0x62, 0x6d, 0xfc, 0xdd);

/* Windows.Foundation.Collections.IMapView`2<IInspectable, IInspectable>, what IMap`2.GetView hands out. The signature
 * "pinterface({e480ce40-a338-4ada-adcf-272272e48cb9};cinterface(IInspectable);cinterface(IInspectable))" derives
 * efe76d10-cb60-50ad-8a4f-6885cd6212a1; it appears once in the real Windows.UI.Xaml.dll. Vtable: Lookup, get_Size, HasKey,
 * Split, in metadata order. */
typedef struct __FIMapView_2_IInspectable_IInspectableVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__FIMapView_2_IInspectable_IInspectable *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__FIMapView_2_IInspectable_IInspectable *This);
	ULONG (STDMETHODCALLTYPE *Release)(__FIMapView_2_IInspectable_IInspectable *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__FIMapView_2_IInspectable_IInspectable *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__FIMapView_2_IInspectable_IInspectable *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__FIMapView_2_IInspectable_IInspectable *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *Lookup)(__FIMapView_2_IInspectable_IInspectable *This, IInspectable *key, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(__FIMapView_2_IInspectable_IInspectable *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *HasKey)(__FIMapView_2_IInspectable_IInspectable *This, IInspectable *key, boolean *found);
	HRESULT (STDMETHODCALLTYPE *Split)(__FIMapView_2_IInspectable_IInspectable *This,
		__FIMapView_2_IInspectable_IInspectable **first, __FIMapView_2_IInspectable_IInspectable **second);
	END_INTERFACE
} __FIMapView_2_IInspectable_IInspectableVtbl;

interface __FIMapView_2_IInspectable_IInspectable {
	CONST_VTBL __FIMapView_2_IInspectable_IInspectableVtbl *lpVtbl;
};

DEFINE_GUID(IID___FIMapView_2_IInspectable_IInspectable, 0xefe76d10, 0xcb60, 0x50ad, 0x8a, 0x4f, 0x68, 0x85, 0xcd, 0x62, 0x12, 0xa1);

/* Windows.Foundation.Collections.IVector`1<ResourceDictionary>, the type of IResourceDictionary.MergedDictionaries. The
 * signature "pinterface({913337e9-11a1-4345-a3a2-4e7f956e222d};rc(Windows.UI.Xaml.ResourceDictionary;
 * {c1ea4f24-d6de-4191-8e3a-f48601f7489c}))" derives fe820a0c-694d-518b-8ec5-372993f6ceaf; it appears once in the real
 * Windows.UI.Xaml.dll. A runtime-class argument carries the GUID of its default interface, the way the validated
 * IVectorView<StorageFile> instantiation does (rc(Windows.Storage.StorageFile;{IStorageFile's IID})). The vtable carries the
 * twelve IVector`1 methods without the inherited IIterable`1.First, the shape xaml_elements.c's children vector uses. */
typedef struct __FIVector_1_Windows__CUI__CXaml__CResourceDictionary __FIVector_1_Windows__CUI__CXaml__CResourceDictionary;
typedef struct __FIVector_1_Windows__CUI__CXaml__CResourceDictionaryVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This, REFIID riid,
		void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This);
	ULONG (STDMETHODCALLTYPE *Release)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetAt)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This, UINT32 index,
		__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary **value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *GetView)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This, void **value);
	HRESULT (STDMETHODCALLTYPE *IndexOf)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This,
		__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *element, UINT32 *index, boolean *value);
	HRESULT (STDMETHODCALLTYPE *SetAt)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This, UINT32 index,
		__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *value);
	HRESULT (STDMETHODCALLTYPE *InsertAt)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This, UINT32 index,
		__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *value);
	HRESULT (STDMETHODCALLTYPE *RemoveAt)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This, UINT32 index);
	HRESULT (STDMETHODCALLTYPE *Append)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This,
		__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *value);
	HRESULT (STDMETHODCALLTYPE *RemoveAtEnd)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This);
	HRESULT (STDMETHODCALLTYPE *Clear)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This);
	HRESULT (STDMETHODCALLTYPE *GetMany)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This, UINT32 start_index,
		UINT32 items_size, __x_ABI_CWindows_CUI_CXaml_CIResourceDictionary **items, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *ReplaceAll)(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *This, UINT32 count,
		__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary **items);
	END_INTERFACE
} __FIVector_1_Windows__CUI__CXaml__CResourceDictionaryVtbl;

interface __FIVector_1_Windows__CUI__CXaml__CResourceDictionary {
	CONST_VTBL __FIVector_1_Windows__CUI__CXaml__CResourceDictionaryVtbl *lpVtbl;
};

DEFINE_GUID(IID___FIVector_1_Windows__CUI__CXaml__CResourceDictionary, 0xfe820a0c, 0x694d, 0x518b, 0x8e, 0xc5, 0x37, 0x29, 0x93, 0xf6, 0xce, 0xaf);
