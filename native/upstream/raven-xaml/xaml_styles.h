/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Engine-internal surface of the style stage, shared between xaml_styles.c (the Style, Setter and SetterBaseCollection
 * classes) and xaml_elements.c (the per-element style storage and the put_Style application that runs a style's — and its
 * BasedOn chain's — setters against the element's real storage). The fixture-facing entry points are marked
 * __declspec(dllexport) at their definitions — the component exports only what it marks, and the style fixture reaches
 * these through GetProcAddress the way it reaches RavenXamlLoad. */
#ifndef RAVEN_XAML_STYLES_H
#define RAVEN_XAML_STYLES_H

#include <windows.h>
#include <winstring.h>
#include <windows.ui.xaml.interop.h>

/* The DependencyProperty ABI, declared here because the engine builds against the hand headers: the interfaces and IIDs
 * come from mingw's windows.ui.xaml.h (and the SDK's windows.ui.xaml.idl behind it) — IDependencyProperty
 * (85b13970-9bc4-4e96-acf1-30c8fd3d55c8) carries exactly ONE member, GetMetadata; the class surface has no Name, TypeName
 * or OwnerType getters in the ABI at all, so the engine's metadata readers live on the internal
 * raven_dependency_property_interop below. IDependencyPropertyStatics (49e5f28f-8259-4d5c-aae0-83d56dbb68d9) answers
 * UnsetValue, Register and RegisterAttached. IPropertyMetadata (814ef30d-8d18-448a-8644-f2cb51e70380) is the metadata
 * Register takes its default value from. The TypeName slots use the real struct mingw declares, the way the style
 * header's TargetType slots do. */
typedef struct __x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback __x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata *This, REFIID riid,
		void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_DefaultValue)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata *This,
		IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *get_CreateDefaultValueCallback)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata *This,
		__x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback **value);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataVtbl *lpVtbl;
};

DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata, 0x814ef30d, 0x8d18, 0x448a, 0x86, 0x44, 0xf2, 0xcb,
	0x51, 0xe7, 0x03, 0x80);

typedef struct __x_ABI_CWindows_CUI_CXaml_CIDependencyProperty __x_ABI_CWindows_CUI_CXaml_CIDependencyProperty;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *This, REFIID riid,
		void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetMetadata)(__x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *This,
		__x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName type, __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata **result);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIDependencyProperty {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyVtbl *lpVtbl;
};

DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CIDependencyProperty, 0x85b13970, 0x9bc4, 0x4e96, 0xac, 0xf1, 0x30, 0xc8,
	0xfd, 0x3d, 0x55, 0xc8);

typedef struct __x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics __x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics *This, REFIID riid,
		void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics *This, ULONG *count,
		IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics *This,
		HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics *This,
		TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_UnsetValue)(__x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics *This,
		IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *Register)(__x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics *This, HSTRING name,
		__x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName property_type,
		__x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName owner_type,
		__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata *type_metadata,
		__x_ABI_CWindows_CUI_CXaml_CIDependencyProperty **result);
	HRESULT (STDMETHODCALLTYPE *RegisterAttached)(__x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics *This,
		HSTRING name, __x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName property_type,
		__x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName owner_type,
		__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata *default_metadata,
		__x_ABI_CWindows_CUI_CXaml_CIDependencyProperty **result);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStaticsVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStaticsVtbl *lpVtbl;
};

DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics, 0x49e5f28f, 0x8259, 0x4d5c, 0xaa, 0xe0, 0x83,
	0xd5, 0x6d, 0xbb, 0x68, 0xd9);

/* The PropertyMetadata activation surface, declared beside the DependencyProperty one for the same reason — the engine
 * builds against hand headers and mingw's windows.ui.xaml.h does not ship as a component include. The declarations and
 * IIDs were validated against two sources:
 *
 *  - mingw's windows.ui.xaml.h (the header the DependencyProperty section above cites), which carries IPropertyMetadata,
 *    IPropertyMetadataFactory, IPropertyMetadataStatics and the two delegates with the UUIDs below;
 *  - the Windows SDK IDL as recompiled by microsoft/win32metadata
 *    (generation/WinSDK/RecompiledIdlHeaders/winrt/windows.ui.xaml.idl), where
 *    `runtimeclass PropertyMetadata` is `[composable(Windows.UI.Xaml.IPropertyMetadataFactory, public, ...)]` with
 *    `[static(Windows.UI.Xaml.IPropertyMetadataStatics, ...)]`, `[marshaling_behavior(agile)]`, `[threading(both)]` and
 *    `[default] interface Windows.UI.Xaml.IPropertyMetadata`. The advertised factory IID c1b81cc0-… is therefore
 *    IPropertyMetadataFactory's, which is what WinUI 2.8's DllGetActivationFactory asks RoGetActivationFactory for.
 *
 * The two callbacks are delegates, so their ABI objects are IUnknown-derived (not IInspectable): the vtable is QI/AddRef/
 * Release plus the one Invoke slot. Their parameter types are only ever pointers here; the objects belong to the guest. */
typedef struct __x_ABI_CWindows_CUI_CXaml_CIDependencyObject __x_ABI_CWindows_CUI_CXaml_CIDependencyObject;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyChangedEventArgs
	__x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyChangedEventArgs;

typedef struct __x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallback __x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallback;
typedef struct __x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallbackVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback *This, REFIID riid,
		void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback *This);
	HRESULT (STDMETHODCALLTYPE *Invoke)(__x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback *This,
		IInspectable **result);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallbackVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallbackVtbl *lpVtbl;
};

DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback, 0xd6ecb12c, 0x15b5, 0x4ec8, 0xb9, 0x5c, 0xcd,
	0xd2, 0x08, 0xf0, 0x81, 0x53);

typedef struct __x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallbackVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallback *This, REFIID riid,
		void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallback *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallback *This);
	HRESULT (STDMETHODCALLTYPE *Invoke)(__x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallback *This,
		__x_ABI_CWindows_CUI_CXaml_CIDependencyObject *obj,
		__x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyChangedEventArgs *args);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallbackVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallback {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallbackVtbl *lpVtbl;
};

DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallback, 0x5a9f8a25, 0xd142, 0x44a4, 0x82, 0x31, 0xfd,
	0x67, 0x67, 0x24, 0xf2, 0x9b);

typedef struct __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactory __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactory;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactoryVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactory *This, REFIID riid,
		void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactory *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactory *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactory *This, ULONG *count,
		IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactory *This,
		HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactory *This,
		TrustLevel *level);
	/* PropertyMetadata(Object): the composable factory signature carries the base and inner slots even though the
	 * engine's PropertyMetadata has no composition; the constructors the projection exposes are the no-base form. */
	HRESULT (STDMETHODCALLTYPE *CreateInstanceWithDefaultValue)(
		__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactory *This, IInspectable *default_value,
		IInspectable *base_interface, IInspectable **inner_interface,
		__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata **value);
	/* PropertyMetadata(Object, PropertyChangedCallback) */
	HRESULT (STDMETHODCALLTYPE *CreateInstanceWithDefaultValueAndCallback)(
		__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactory *This, IInspectable *default_value,
		__x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallback *property_changed_callback,
		IInspectable *base_interface, IInspectable **inner_interface,
		__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata **value);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactoryVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactory {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactoryVtbl *lpVtbl;
};

DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactory, 0xc1b81cc0, 0x57cd, 0x4f2f, 0xb0, 0xa9, 0xe1,
	0x80, 0x1b, 0x28, 0xf7, 0x6b);

/* IPropertyMetadataStatics is the [static] interface the projection maps to PropertyMetadata.Create's four overloads:
 * two fixed-default forms and two create-default-value-factory forms. */
typedef struct __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics *This, REFIID riid,
		void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics *This, ULONG *count,
		IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics *This,
		HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics *This,
		TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *CreateWithDefaultValue)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics *This,
		IInspectable *default_value, __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata **result);
	HRESULT (STDMETHODCALLTYPE *CreateWithDefaultValueAndCallback)(
		__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics *This, IInspectable *default_value,
		__x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallback *property_changed_callback,
		__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata **result);
	HRESULT (STDMETHODCALLTYPE *CreateWithFactory)(__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics *This,
		__x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback *create_default_value_callback,
		__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata **result);
	HRESULT (STDMETHODCALLTYPE *CreateWithFactoryAndCallback)(
		__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics *This,
		__x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback *create_default_value_callback,
		__x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallback *property_changed_callback,
		__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata **result);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStaticsVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStaticsVtbl *lpVtbl;
};

DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics, 0x3b01077a, 0x6e06, 0x45e9, 0x8b, 0x5c, 0xaf,
	0x24, 0x34, 0x58, 0xc0, 0x62);

/* The engine-internal readers of the engine's own PropertyMetadata objects. The real IPropertyMetadata surface exposes
 * only DefaultValue and CreateDefaultValueCallback — Microsoft's own remarks say a PropertyMetadata deliberately has no
 * way to find its property-changed callback, because the dependency-property system is the only caller. The engine has no
 * property-change notification for dependency properties yet, so the callback is held and never invoked; this internal
 * part is how the engine (and the style fixture) can still verify the held delegate. CreateDefaultValue is the engine's
 * counterpart of what that property system does with the metadata: it hands back the fixed default, or invokes the
 * create-default-value factory and returns what it produced. The real ABI carries no such member, which is why it lives
 * here rather than on IPropertyMetadata. */
static const IID raven_property_metadata_interop_iid = {0x6f42d1a8, 0x3b57, 0x4c1e, {0xa8, 0x93, 0x2d, 0x5c, 0x71,
	0x0b, 0x64, 0xd9}};
typedef struct raven_property_metadata_interop raven_property_metadata_interop;
struct raven_property_metadata_interop_vtbl {
	HRESULT (WINAPI *QueryInterface)(raven_property_metadata_interop *, REFIID, void **);
	ULONG (WINAPI *AddRef)(raven_property_metadata_interop *);
	ULONG (WINAPI *Release)(raven_property_metadata_interop *);
	HRESULT (WINAPI *get_PropertyChangedCallback)(raven_property_metadata_interop *,
		__x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallback **);
	HRESULT (WINAPI *CreateDefaultValue)(raven_property_metadata_interop *, IInspectable **);
};
struct raven_property_metadata_interop { const struct raven_property_metadata_interop_vtbl *lpVtbl; };

/* The engine-internal metadata readers of the engine's own DependencyProperty objects, the way corewindow_interop.h is the
 * internal readers of the engine's CoreWindow. The real class surface has no such getters (the ABI carries GetMetadata
 * only), so these are the engine's own interface and its own GUID; the object answers it next to the real
 * IDependencyProperty. The TypeName slots hand the registered property and owner types back as the struct, the same
 * convention the Style's TargetType slots use — NOT boxed through IReference<TypeName>, because that instantiation's IID
 * needs the TypeName struct's metadata GUID in its signature and no source available to the engine carries it (mingw and
 * the SDK headers declare the struct with no GUID; no winmd is in tree), so a boxed IID here could not be the real one. */
static const IID raven_dependency_property_interop_iid = {0x3b9f8b2a, 0x7c41, 0x4d0e, {0x92, 0x6a, 0x51, 0xd8, 0x4b,
	0x7f, 0x33, 0xe2}};
typedef struct raven_dependency_property_interop raven_dependency_property_interop;
struct raven_dependency_property_interop_vtbl {
	HRESULT (WINAPI *QueryInterface)(raven_dependency_property_interop *, REFIID, void **);
	ULONG (WINAPI *AddRef)(raven_dependency_property_interop *);
	ULONG (WINAPI *Release)(raven_dependency_property_interop *);
	HRESULT (WINAPI *get_Name)(raven_dependency_property_interop *, HSTRING *);
	HRESULT (WINAPI *get_TypeName)(raven_dependency_property_interop *, __x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName *);
	HRESULT (WINAPI *get_OwnerType)(raven_dependency_property_interop *, __x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName *);
	HRESULT (WINAPI *get_DefaultValue)(raven_dependency_property_interop *, IInspectable **);
};
struct raven_dependency_property_interop { const struct raven_dependency_property_interop_vtbl *lpVtbl; };

/* Engine-internal setter application. A setter is addressed by its property NAME: the call stores a (property-name
 * string, value IInspectable) pair on the style — the pair shape the put_Style application walks. A setter created through
 * the Setter factory can carry a DependencyProperty instead (ISetter.put_Property): the DP's registered name becomes the
 * stored pair's name, so the application walks both kinds the same way. The value is owned; the style keeps its own
 * reference. */
HRESULT raven_style_add_setter(IInspectable *style, const WCHAR *property, UINT32 length, IInspectable *value);

/* What the put_Style application walks: the stored pairs in application order (the order raven_style_add_setter appended
 * them). setter_at hands out the property name pointing into the style's own storage and a value reference the caller
 * releases; both answer S_FALSE for an object that is not an engine Style. Both are fixture-facing as well — the style
 * fixture reads the stored pairs back through them. */
HRESULT raven_style_setter_count(IInspectable *style, UINT32 *count);
HRESULT raven_style_setter_at(IInspectable *style, UINT32 index, const WCHAR **property, UINT32 *length,
	IInspectable **value);

/* The authored default styles: one sealed engine Style per control class the put_Style application supports, built lazily
 * on first use from the same raven_style_add_setter path the application code uses. raven_style_default_attach puts the
 * class's default on an element through raven_xaml_element_put_style — the constructor's initial assignment, so a control
 * that never sees an explicit put_Style still carries the documented default values, and a later put_Style replaces it the
 * way any assignment does. The default objects are shared between every instance of the class and SEALED at authoring: an
 * application holding get_Style's answer cannot mutate what all of them point at, because every mutation slot of a sealed
 * style (Setters append/clear, Setter put_Value, put_TargetType, put_BasedOn) rejects. The objects live for the component's
 * lifetime. Answers S_FALSE for an element class the engine has no default for. */
HRESULT __declspec(dllexport) raven_style_default_attach(IInspectable *element);

/* The per-element Style property. Both store and clear an owned reference on the element classes the engine implements (the
 * grid, the text block and the button); other classes answer S_FALSE. put_Style re-renders, whether it attaches or clears. */
HRESULT raven_xaml_element_put_style(IInspectable *element, IInspectable *style);
HRESULT raven_xaml_element_get_style(IInspectable *element, IInspectable **style);

/* The activation entry the DllGetActivationFactory chain in xaml.c reaches for "Windows.UI.Xaml.DependencyProperty": a
 * statics-only class (the real runtimeclass has no default constructor), so the factory object answers
 * IDependencyPropertyStatics through its QI and ActivateInstance only faults. Answers ONLY the DependencyProperty name;
 * the Style and Setter names stay with raven_style_factory. */
HRESULT raven_dependency_factory(HSTRING name, IActivationFactory **out);

/* The activation entry the chain reaches for "Windows.UI.Xaml.PropertyMetadata": an activatable class, so the factory
 * object answers IPropertyMetadataFactory (the composable factory interface, which is the IID WinUI 2.8 asks
 * RoGetActivationFactory for), IPropertyMetadataStatics and the IAgileObject marker. ActivateInstance faults because the
 * class has no parameterless constructor — the two constructors are the factory's methods. Answers ONLY the
 * PropertyMetadata name. */
HRESULT raven_property_metadata_factory(HSTRING name, IActivationFactory **out);

/* The statics through the engine-internal export: the runtime catalog does not list the DependencyProperty class yet (the
 * registry entry is added separately), so RoGetActivationFactory cannot reach the statics until then; the style fixture
 * drives Register and RegisterAttached through this GetProcAddress the way it drives the other engine-internal exports.
 * Hands out an AddRef'd IDependencyPropertyStatics. */
HRESULT __declspec(dllexport) raven_xaml_dependency_statics(
	__x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics **out);

/* A fresh engine Setter, the Setter factory's ActivateInstance answer: the runtime catalog does not list the Setter class
 * yet either, so the fixture drives this through GetProcAddress the way it drives the other engine-internal exports; the
 * activation path answers the same object once the entry lands. */
HRESULT __declspec(dllexport) raven_xaml_setter_create(IInspectable **out);

#endif
