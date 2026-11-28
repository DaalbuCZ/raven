/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: Windows.UI.Xaml.Controls.Frame and Page.
 *
 * A Frame navigates to a page by type name: the name is resolved through the metadata provider and the page is created through
 * the runtime catalog, then attached to the frame. The frame keeps the current page and its source type, reports that it cannot
 * go back or forward, and accepts its navigation events without raising them. Page carries the frame it was navigated into, its
 * navigation cache mode and the optional app bars. ControlTemplate is the runtime class a Control's Template property and a
 * Style's "Template" setter carry: its content tree rides the same visual-child hook the render walk reads first, so a
 * templated control renders the template root the way it renders a frame's page. The navigation stack, its serialization and
 * the dependency-property statics are not implemented yet. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include "windows_ui_xaml_dictionary.h" /* the boxed string __FIReference_1_HSTRING the peer bridge receives */
#include "windows_ui_xaml_style.h"
#include "xaml_styles.h"
#include "xaml_element_ui.h"
#include <windows.ui.xaml.interop.h>
#include "windows_ui_xaml_contentdialog.h"
#include "windows_ui_xaml_controls.h"
#include "windows_ui_xaml_control.h"
#include "windows_ui_xaml_elements.h"
#include "windows_ui_xaml_window.h"
#include "swapchain_panel.h"

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))
#include <stddef.h>
#include <string.h>
#include "events.h"

typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame frame1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2 frame2;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3 frame3;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4 frame4;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5 frame5;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactory frame_factory;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics frame_statics1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2 frame_statics2;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5 frame_statics5;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPage page1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactory page_factory;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPageStatics page_statics;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl page_content_control;
typedef __x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName xaml_type_name;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIDependencyProperty __x_ABI_CWindows_CUI_CXaml_CIDependencyProperty;
typedef __x_ABI_CWindows_CUI_CCore_CICoreDispatcher ICoreDispatcher;

/* The IFrameworkElement2/3/4 continuation faces (Windows.UI.Xaml.winmd, uuids f19104be-422a-4904-a52f-
 * ee72010429e5, c81c2720-5c52-4bbe-a199-2b1e34f00f70, 6b765bb3-fba3-4404-bdee-1a45d1ca5f21): the projected
 * FrameworkElement surface spreads its members across the version interfaces, and the generated app code
 * casts the object to the exact version declaring the member it touches (the traced WelcomePage ctor cast
 * to IFrameworkElement3 — EETypeRva 0x1af60 — and died when the object answered only IFrameworkElement).
 * Hand-declared the way IDependencyObject is; the members delegate to the same storage the 47-slot face
 * uses. */
typedef struct __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_RequestedTheme)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_RequestedTheme)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_DataContextChanged)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_DataContextChanged)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetBindingExpression)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This, uint64_t a1, uint64_t a2);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2Vtbl;
interface __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 { CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2Vtbl *lpVtbl; };
DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2, 0xf19104be, 0x422a, 0x4904, 0xa5, 0x2f, 0xee, 0x72, 0x01, 0x04, 0x29, 0xe5);

typedef struct __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *add_Loading)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_Loading)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3Vtbl;
interface __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 { CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3Vtbl *lpVtbl; };
DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3, 0xc81c2720, 0x5c52, 0x4bbe, 0xa1, 0x99, 0x2b, 0x1e, 0x34, 0xf0, 0x0f, 0x70);

typedef struct __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_AllowFocusOnInteraction)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_AllowFocusOnInteraction)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_FocusVisualMargin)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_FocusVisualMargin)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_FocusVisualSecondaryThickness)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_FocusVisualSecondaryThickness)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_FocusVisualPrimaryThickness)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_FocusVisualPrimaryThickness)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_FocusVisualSecondaryBrush)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_FocusVisualSecondaryBrush)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_FocusVisualPrimaryBrush)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_FocusVisualPrimaryBrush)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_AllowFocusWhenDisabled)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_AllowFocusWhenDisabled)(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4Vtbl;
interface __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 { CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4Vtbl *lpVtbl; };
DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4, 0x6b765bb3, 0xfba3, 0x4404, 0xbd, 0xee, 0x1a, 0x45, 0xd1, 0xca, 0x5f, 0x21);

/* The real IDependencyObject ABI (mingw's windows.ui.xaml.h, uuid 5C526665-F60E-4912-AF59-5FE0680F089D):
 * the six dependency-value methods after the IInspectable six. The component builds against hand headers
 * that predate the first object needing this interface, so it is declared here the way IControlTemplate
 * is; the elements will share it when the trace brings them here. */
typedef struct __x_ABI_CWindows_CUI_CXaml_CIDependencyObject dependency_object1;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIDependencyObjectVtbl dependency_object1_vtbl;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIDependencyObjectVtbl __x_ABI_CWindows_CUI_CXaml_CIDependencyObjectVtbl;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIDependencyObject __x_ABI_CWindows_CUI_CXaml_CIDependencyObject;
struct __x_ABI_CWindows_CUI_CXaml_CIDependencyObjectVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(dependency_object1 *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(dependency_object1 *This);
	ULONG (STDMETHODCALLTYPE *Release)(dependency_object1 *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(dependency_object1 *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(dependency_object1 *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(dependency_object1 *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetValue)(dependency_object1 *This, __x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *dp, IInspectable **result);
	HRESULT (STDMETHODCALLTYPE *SetValue)(dependency_object1 *This, __x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *dp, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *ClearValue)(dependency_object1 *This, __x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *dp);
	HRESULT (STDMETHODCALLTYPE *ReadLocalValue)(dependency_object1 *This, __x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *dp, IInspectable **result);
	HRESULT (STDMETHODCALLTYPE *GetAnimationBaseValue)(dependency_object1 *This, __x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *dp, IInspectable **result);
	HRESULT (STDMETHODCALLTYPE *get_Dispatcher)(dependency_object1 *This, ICoreDispatcher **value);
	END_INTERFACE
};
interface __x_ABI_CWindows_CUI_CXaml_CIDependencyObject { CONST_VTBL dependency_object1_vtbl *lpVtbl; };
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CIDependencyObject, 0x5c526665, 0xf60e, 0x4912, 0xaf, 0x59, 0x5f, 0xe0, 0x68, 0x0f, 0x08, 0x9d);

/* IFrameworkElement6 from Windows.UI.Xaml.winmd. */
typedef struct frame_theme_iface frame_theme_iface;
typedef struct frame_theme_vtbl {
 HRESULT (WINAPI *QueryInterface)(frame_theme_iface *, REFIID, void **);
 ULONG (WINAPI *AddRef)(frame_theme_iface *);
 ULONG (WINAPI *Release)(frame_theme_iface *);
 HRESULT (WINAPI *GetIids)(frame_theme_iface *, ULONG *, IID **);
 HRESULT (WINAPI *GetRuntimeClassName)(frame_theme_iface *, HSTRING *);
 HRESULT (WINAPI *GetTrustLevel)(frame_theme_iface *, TrustLevel *);
 HRESULT (WINAPI *get_ActualTheme)(frame_theme_iface *, INT32 *);
 HRESULT (WINAPI *add_ActualThemeChanged)(frame_theme_iface *, IUnknown *, INT64 *);
 HRESULT (WINAPI *remove_ActualThemeChanged)(frame_theme_iface *, INT64);
} frame_theme_vtbl;
DEFINE_GUID(IID_frame_theme, 0x792a5d91, 0x62a1, 0x40bf, 0xa0, 0xce, 0xf9, 0xc1, 0x31, 0xfc, 0xb7, 0xa7);

/* The WinUI 2.8 DXAML peer bridge (Microsoft.UI.Xaml.dll private, uuid 1791587A-2A7C-4981-A2D8-AB99FFFC65B9).
 * WinUI's CoreServices singleton holds the OS XAML peer object and delivers private callbacks to it through
 * this face: the traced Shapr3D run stores the frame's non-delegating inner as that peer, and the ProgressRing
 * core construction queries the peer for it and calls the second method with the control's class name
 * ("Microsoft.UI.Xaml.Controls.ProgressRing") boxed as IReference<String> — microsoft.ui.xaml.dll+1695ae does
 * not check that QI's result, so a refusal is a null interface call (c0000005 at +25919). The first method has
 * no observed caller yet; the bridge answers the ask and accepts the delivered name. */
typedef struct frame_peer_bridge_part frame_peer_bridge_part;
typedef struct frame_peer_bridge_vtbl {
 HRESULT (WINAPI *QueryInterface)(frame_peer_bridge_part *, REFIID, void **);
 ULONG (WINAPI *AddRef)(frame_peer_bridge_part *);
 ULONG (WINAPI *Release)(frame_peer_bridge_part *);
 HRESULT (WINAPI *GetIids)(frame_peer_bridge_part *, ULONG *, IID **);
 HRESULT (WINAPI *GetRuntimeClassName)(frame_peer_bridge_part *, HSTRING *);
 HRESULT (WINAPI *GetTrustLevel)(frame_peer_bridge_part *, TrustLevel *);
 HRESULT (WINAPI *Method1)(frame_peer_bridge_part *, void *);
 HRESULT (WINAPI *DeliverClassName)(frame_peer_bridge_part *, IUnknown *);
} frame_peer_bridge_vtbl;
DEFINE_GUID(IID_frame_peer_bridge, 0x1791587a, 0x2a7c, 0x4981, 0xa2, 0xd8, 0xab, 0x99, 0xff, 0xfc, 0x65, 0xb9);

/* The second peer face of the same family (Microsoft.UI.Xaml.dll private, uuid 29FED85D-3D22-43A1-ADD0-
 * 17027C08B212): generated DXAML accessors (microsoft.ui.xaml.dll+382754 over the +0x60 member, +1f1084 over
 * the +0x30 member) fetch it from the peer and call its first method (slot 6) with two framework context
 * objects and an out, HRESULT-checked — again without checking the QI that produced the face. */
typedef struct frame_peer_shim_part frame_peer_shim_part;
typedef struct frame_peer_shim_vtbl {
 HRESULT (WINAPI *QueryInterface)(frame_peer_shim_part *, REFIID, void **);
 ULONG (WINAPI *AddRef)(frame_peer_shim_part *);
 ULONG (WINAPI *Release)(frame_peer_shim_part *);
 HRESULT (WINAPI *GetIids)(frame_peer_shim_part *, ULONG *, IID **);
 HRESULT (WINAPI *GetRuntimeClassName)(frame_peer_shim_part *, HSTRING *);
 HRESULT (WINAPI *GetTrustLevel)(frame_peer_shim_part *, TrustLevel *);
 HRESULT (WINAPI *Method1)(frame_peer_shim_part *, void *, void *, void **);
 HRESULT (WINAPI *Method2)(frame_peer_shim_part *);
} frame_peer_shim_vtbl;
DEFINE_GUID(IID_frame_peer_shim, 0x29fed85d, 0x3d22, 0x43a1, 0xad, 0xd0, 0x17, 0x02, 0x7c, 0x08, 0xb2, 0x12);

/* The real IControlTemplate ABI (Windows.UI.Xaml.winmd, uuid EFD2418E-41E0-48BB-8B82-91EDA1BA3FE2): TargetType get/put
 * after the six IInspectable methods, nothing else — the real interface carries no content member, the template's tree
 * arriving from XAML parsing, so the engine-internal root attach is the export below. The hand headers cannot carry this
 * interface (they list only the classes earlier stages needed), so it is declared here once and the IID is defined in
 * every TU that includes the declaration (DEFINE_GUID is selectany under INITGUID). */
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplate template1;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplateVtbl template1_vtbl;
struct __x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplateVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(template1 *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(template1 *This);
	ULONG (STDMETHODCALLTYPE *Release)(template1 *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(template1 *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(template1 *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(template1 *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_TargetType)(template1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_TargetType)(template1 *This, uint64_t value);
	END_INTERFACE
};
interface __x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplate { CONST_VTBL template1_vtbl *lpVtbl; };
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplate, 0xefd2418e, 0x41e0, 0x48bb, 0x8b, 0x82, 0x91,
	0xed, 0xa1, 0xba, 0x3f, 0xe2);

/* The real INavigationEventArgs ABI (Windows.UI.Xaml.winmd, uuid B6AA9834-6691-44D1-BDF7-58820C27B0D0, generated with
 * scripts/generate-xaml-header.py): content, parameter, source page type, navigation mode and URI after the IInspectable
 * six. The frame builds the argument its Navigated handlers and the page's OnNavigatedTo receive; the hand headers
 * predate the navigation namespace the way they predated IDependencyObject and IControlTemplate. */
typedef struct __x_ABI_CWindows_CUI_CXaml_CNavigation_CINavigationEventArgs navigation_args1;
typedef struct __x_ABI_CWindows_CUI_CXaml_CNavigation_CINavigationEventArgsVtbl navigation_args1_vtbl;
struct __x_ABI_CWindows_CUI_CXaml_CNavigation_CINavigationEventArgsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(navigation_args1 *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(navigation_args1 *This);
	ULONG (STDMETHODCALLTYPE *Release)(navigation_args1 *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(navigation_args1 *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(navigation_args1 *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(navigation_args1 *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_Content)(navigation_args1 *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_Parameter)(navigation_args1 *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_SourcePageType)(navigation_args1 *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_NavigationMode)(navigation_args1 *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_Uri)(navigation_args1 *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *put_Uri)(navigation_args1 *This, uint64_t value);
	END_INTERFACE
};
interface __x_ABI_CWindows_CUI_CXaml_CNavigation_CINavigationEventArgs { CONST_VTBL navigation_args1_vtbl *lpVtbl; };
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CNavigation_CINavigationEventArgs, 0xb6aa9834, 0x6691, 0x44d1, 0xbd, 0xf7,
	0x58, 0x82, 0x0c, 0x27, 0xb0, 0xd0);

/* The page-lifecycle override face the framework drives after navigation (uuid 230B9CB8-3419-4306-AABE-4A8E680D3627,
 * already in windows_ui_xaml_controls.h): OnNavigatedFrom, OnNavigatedTo, OnNavigatingFrom. */
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides page_overrides_i;

/* The real IControlFactory and IControlStatics ABIs (Windows.UI.Xaml.winmd, uuids B12B1D5A-6419-4E16-B212-A45336B75778
 * and 473B305B-877D-4C35-8FCA-46A05F96607A, generated with scripts/generate-xaml-header.py): the composable creation
 * the WinUI dependency drives when its controls compose over the base Control, and the dependency-property getters
 * its projections read. The hand headers predate them the way they predate IControlTemplate. */
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIControlFactory control_factory_i;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIControlFactoryVtbl control_factory_vtbl;
struct __x_ABI_CWindows_CUI_CXaml_CControls_CIControlFactoryVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(control_factory_i *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(control_factory_i *This);
	ULONG (STDMETHODCALLTYPE *Release)(control_factory_i *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(control_factory_i *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(control_factory_i *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(control_factory_i *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *CreateInstance)(control_factory_i *This, uint64_t outer, uint64_t inner, uint64_t instance);
	END_INTERFACE
};
interface __x_ABI_CWindows_CUI_CXaml_CControls_CIControlFactory { CONST_VTBL control_factory_vtbl *lpVtbl; };
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlFactory, 0xb12b1d5a, 0x6419, 0x4e16, 0xb2, 0x12, 0xa4, 0x53,
	0x36, 0xb7, 0x57, 0x78);

/* The real IUserControl, IUserControlFactory and IUserControlStatics ABIs (Windows.UI.Xaml.winmd, uuids
 * A7A69EC9-EA35-4679-BF29-F4F09286D314, 38B1ED92-A28A-4972-93DF-F4F759B8AFD2 and
 * E8887976-2C5C-41CF-BE6A-9E44BEFDF655, verified against the SDK 10.0.16299 generated header):
 * UserControl carries Content, its factory composes CreateInstance(outer, inner, IUserControl) and its
 * statics expose the Content dependency property. */
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIUserControl usercontrol_i;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIUserControlVtbl usercontrol_vtbl;
struct __x_ABI_CWindows_CUI_CXaml_CControls_CIUserControlVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(usercontrol_i *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(usercontrol_i *This);
	ULONG (STDMETHODCALLTYPE *Release)(usercontrol_i *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(usercontrol_i *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(usercontrol_i *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(usercontrol_i *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_Content)(usercontrol_i *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *put_Content)(usercontrol_i *This, IInspectable *value);
	END_INTERFACE
};
interface __x_ABI_CWindows_CUI_CXaml_CControls_CIUserControl { CONST_VTBL usercontrol_vtbl *lpVtbl; };
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CControls_CIUserControl, 0xa7a69ec9, 0xea35, 0x4679, 0xbf, 0x29, 0xf4, 0xf0,
	0x92, 0x86, 0xd3, 0x14);

typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIUserControlFactory usercontrol_factory_i;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIUserControlFactoryVtbl usercontrol_factory_vtbl;
struct __x_ABI_CWindows_CUI_CXaml_CControls_CIUserControlFactoryVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(usercontrol_factory_i *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(usercontrol_factory_i *This);
	ULONG (STDMETHODCALLTYPE *Release)(usercontrol_factory_i *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(usercontrol_factory_i *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(usercontrol_factory_i *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(usercontrol_factory_i *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *CreateInstance)(usercontrol_factory_i *This, uint64_t outer, uint64_t inner, uint64_t instance);
	END_INTERFACE
};
interface __x_ABI_CWindows_CUI_CXaml_CControls_CIUserControlFactory { CONST_VTBL usercontrol_factory_vtbl *lpVtbl; };
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CControls_CIUserControlFactory, 0x38b1ed92, 0xa28a, 0x4972, 0x93, 0xdf, 0xf4, 0xf7,
	0x59, 0xb8, 0xaf, 0xd2);

typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIUserControlStatics usercontrol_statics_i;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIUserControlStaticsVtbl usercontrol_statics_vtbl;
struct __x_ABI_CWindows_CUI_CXaml_CControls_CIUserControlStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(usercontrol_statics_i *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(usercontrol_statics_i *This);
	ULONG (STDMETHODCALLTYPE *Release)(usercontrol_statics_i *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(usercontrol_statics_i *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(usercontrol_statics_i *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(usercontrol_statics_i *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_ContentProperty)(usercontrol_statics_i *This, uint64_t out);
	END_INTERFACE
};
interface __x_ABI_CWindows_CUI_CXaml_CControls_CIUserControlStatics { CONST_VTBL usercontrol_statics_vtbl *lpVtbl; };
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CControls_CIUserControlStatics, 0xe8887976, 0x2c5c, 0x41cf, 0xbe, 0x6a, 0x9e, 0x44,
	0xbe, 0xfd, 0xf6, 0x55);

typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIControlStatics control_statics_i;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIControlStaticsVtbl control_statics_vtbl;
struct __x_ABI_CWindows_CUI_CXaml_CControls_CIControlStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(control_statics_i *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(control_statics_i *This);
	ULONG (STDMETHODCALLTYPE *Release)(control_statics_i *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(control_statics_i *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(control_statics_i *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(control_statics_i *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_FontSizeProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_FontFamilyProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_FontWeightProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_FontStyleProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_FontStretchProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_CharacterSpacingProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_ForegroundProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_IsTabStopProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_IsEnabledProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_TabIndexProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_TabNavigationProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_TemplateProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_PaddingProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_HorizontalContentAlignmentProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_VerticalContentAlignmentProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_BackgroundProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_BorderThicknessProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_BorderBrushProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_DefaultStyleKeyProperty)(control_statics_i *This, uint64_t out);
	HRESULT (STDMETHODCALLTYPE *get_FocusStateProperty)(control_statics_i *This, uint64_t out);
	END_INTERFACE
};
interface __x_ABI_CWindows_CUI_CXaml_CControls_CIControlStatics { CONST_VTBL control_statics_vtbl *lpVtbl; };
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlStatics, 0x473b305b, 0x877d, 0x4c35, 0x8f, 0xca, 0x46, 0xa0,
	0x5f, 0x96, 0x60, 0x7a);

/* The visual-child hook of the element classes and the template kind checks ride xaml_internal.h; the loader drives
 * raven_xaml_object_is_control_template below when a compiled document declares a template node. */

static const WCHAR frame_class[] = L"Windows.UI.Xaml.Controls.Frame";
static const WCHAR page_class[] = L"Windows.UI.Xaml.Controls.Page";
static const WCHAR template_class[] = L"Windows.UI.Xaml.Controls.ControlTemplate";
static const WCHAR control_class[] = L"Windows.UI.Xaml.Controls.Control";
static const WCHAR contentdialog_class[] = L"Windows.UI.Xaml.Controls.ContentDialog";

static HRESULT inspectable_iids(ULONG *count, IID **iids)
{
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}

static HRESULT inspectable_trust(TrustLevel *level)
{
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

static int equal_iid(REFIID left, const IID *right)
{
	return left->Data1 == right->Data1 && left->Data2 == right->Data2 && left->Data3 == right->Data3 &&
		left->Data4[0] == right->Data4[0] && left->Data4[1] == right->Data4[1] &&
		left->Data4[2] == right->Data4[2] && left->Data4[3] == right->Data4[3] &&
		left->Data4[4] == right->Data4[4] && left->Data4[5] == right->Data4[5] &&
		left->Data4[6] == right->Data4[6] && left->Data4[7] == right->Data4[7];
}

#define STUB1(iface, name) static HRESULT WINAPI name(iface *This, uint64_t a1) \
	{ (void)This; if (a1) *(void **)(uintptr_t)a1 = NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
#define STUB2(iface, name) static HRESULT WINAPI name(iface *This, uint64_t a1, uint64_t a2) \
	{ (void)This; (void)a1; (void)a2; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }

struct frame;

struct frame1_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameVtbl *lpVtbl; struct frame *owner; };
struct frame2_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2Vtbl *lpVtbl; struct frame *owner; };
struct frame3_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3Vtbl *lpVtbl; struct frame *owner; };
struct frame4_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4Vtbl *lpVtbl; struct frame *owner; };
struct frame5_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5Vtbl *lpVtbl; struct frame *owner; };
struct frame_theme_iface { const frame_theme_vtbl *lpVtbl; struct frame *owner; };
struct frame_framework { const void *lpVtbl; struct frame *owner; };
struct frame_theme_handler { IUnknown *handler; INT64 token; struct frame_theme_handler *next; };
struct framework_handler { IUnknown *handler; INT64 token; unsigned kind; struct framework_handler *next; };
struct frame_content_part;
struct frame_content_vtbl {
 HRESULT (WINAPI *QueryInterface)(struct frame_content_part *, REFIID, void **);
 ULONG (WINAPI *AddRef)(struct frame_content_part *), (WINAPI *Release)(struct frame_content_part *);
 HRESULT (WINAPI *GetIids)(struct frame_content_part *, ULONG *, IID **);
 HRESULT (WINAPI *GetRuntimeClassName)(struct frame_content_part *, HSTRING *);
 HRESULT (WINAPI *GetTrustLevel)(struct frame_content_part *, TrustLevel *);
 HRESULT (WINAPI *get_Content)(struct frame_content_part *, IInspectable **);
 HRESULT (WINAPI *put_Content)(struct frame_content_part *, IInspectable *);
 HRESULT (WINAPI *get_ContentTemplate)(struct frame_content_part *, IInspectable **);
 HRESULT (WINAPI *put_ContentTemplate)(struct frame_content_part *, IInspectable *);
 HRESULT (WINAPI *get_ContentTemplateSelector)(struct frame_content_part *, IInspectable **);
 HRESULT (WINAPI *put_ContentTemplateSelector)(struct frame_content_part *, IInspectable *);
 HRESULT (WINAPI *get_ContentTransitions)(struct frame_content_part *, IInspectable **);
 HRESULT (WINAPI *put_ContentTransitions)(struct frame_content_part *, IInspectable *);
};
struct frame_content_part { const struct frame_content_vtbl *lpVtbl; struct frame *owner; };
struct frame_control_part { const control1_vtbl *lpVtbl; struct frame *owner; };
struct frame_inner_part { const IInspectableVtbl *lpVtbl; struct frame *owner; };
struct frame_peer_bridge_part { const frame_peer_bridge_vtbl *lpVtbl; struct frame *owner; };
struct frame_peer_shim_part { const frame_peer_shim_vtbl *lpVtbl; struct frame *owner; };
struct frame_dependency_part { const __x_ABI_CWindows_CUI_CXaml_CIDependencyObjectVtbl *lpVtbl; struct frame *owner; };
struct frame_property { __x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *property; IInspectable *value; struct frame_property *next; };

struct frame_usercontrol_part;
struct frame_usercontrol_part { const usercontrol_vtbl *lpVtbl; struct frame *owner; };
struct framework_thickness { double left, top, right, bottom; };
/* The standalone Page the .NET Native projection of an app's Page-derived class composes with: the same
 * frame machinery the UserControl factory uses, with the IPage and IPageOverrides faces the composed
 * projection's cast walks need. */
struct frame_page_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIPageVtbl *lpVtbl; struct frame *owner; };
struct frame_page_overrides_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverridesVtbl *lpVtbl; struct frame *owner; };
struct frame_contentdialog_part { const contentdialog_vtbl *lpVtbl; struct frame *owner; };
struct frame_contentdialog2_part { const contentdialog2_vtbl *lpVtbl; struct frame *owner; };
struct frame_fe2_part { const __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2Vtbl *lpVtbl; struct frame *owner; };
struct frame_fe3_part { const __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3Vtbl *lpVtbl; struct frame *owner; };
struct frame_fe4_part { const __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4Vtbl *lpVtbl; struct frame *owner; };
struct frame {
	struct frame1_part first;
	struct frame2_part second;
	struct frame3_part third;
	struct frame4_part fourth;
	struct frame5_part fifth;
	struct frame_inner_part inner;
	struct frame_peer_bridge_part peer_bridge;
	struct frame_peer_shim_part peer_shim;
	struct raven_tracker_part tracker;
	struct frame_dependency_part dependency;
	struct frame_control_part control;
	struct frame_content_part content;
	struct frame_usercontrol_part usercontrol;
	struct frame_page_part page_face;
	struct frame_page_overrides_part page_overrides;
	struct frame_contentdialog_part contentdialog;
	struct frame_contentdialog2_part contentdialog2;
	struct frame_fe2_part fe2;
	struct frame_fe3_part fe3;
	struct frame_fe4_part fe4;
	frame_theme_iface theme;
	struct frame_framework framework;
	struct element_ui ui;
	struct frame_theme_handler *theme_handlers;
	struct framework_handler *framework_handlers;
	INT64 next_framework_token;
	INT64 next_theme_token;
	/* The Navigated subscriptions the framework raises after a completed navigation; the other frame events
	 * (Navigating, NavigationFailed, NavigationStopped) remain accepted without raising. */
	struct raven_event_list navigated;
	/* The Control.IsEnabledChanged subscriptions (the ThemedImage-style ctor wiring): the marshal hands the
	 * handler over as the CCW of the app's delegate and the slot pair retains it through the shared machinery. */
	struct raven_event_list is_enabled_changed;
	struct frame_property *properties;
	BOOL dependency_only;
	/* The standalone Control the WinUI dependency composes its controls over (Windows.UI.Xaml.Controls.Control
	 * activated through IControlFactory): the engine's Control surface rides the frame machinery, but the object
	 * names itself the Control class and never participates in navigation. */
	BOOL standalone_control;
	/* The same machinery for Windows.UI.Xaml.Controls.UserControl, whose factory the .NET Native projection of
	 * an app's UserControl-derived class composes with: the object names itself the UserControl class, answers
	 * IUserControl (Content) and keeps everything the Control base surface carries. */
	BOOL standalone_usercontrol;
	/* Windows.UI.Xaml.Controls.ContentDialog: names itself the ContentDialog class over the Control surface.
	 * The dialog-specific face arrives once the projection's cast IID is on record (the frame's refusal trace
	 * names it); activation exists first so the constructor can run. */
	BOOL standalone_contentdialog;
	/* Windows.UI.Xaml.Controls.Page: the page factory composes the frame machinery for the projection the
	 * same way (the CCW's interface walk needs the FrameworkElement family a bare page struct refuses). */
	BOOL standalone_page;
	int32_t navigation_cache_mode;
	IInspectable *page_top_app_bar, *page_bottom_app_bar;
	IInspectable *outer;
	LONG refs;
	HSTRING name;
	HSTRING source_type;
	IInspectable *page, *background, *content_template, *content_selector, *content_transitions;
	IInspectable *usercontrol_content;
	/* The standalone ContentDialog's own surface the projected IContentDialog/IContentDialog2 faces store:
	 * the title objects, the button texts and commands, the styles, the default button and the button-enable
	 * flags, plus the six dialog event subscriptions the constructor wires (accepted without raising until a
	 * presentation path exists). */
	IInspectable *dialog_title, *dialog_title_template;
	uint64_t dialog_full_size, dialog_is_primary_enabled, dialog_is_secondary_enabled, dialog_default_button;
	HSTRING dialog_primary_text, dialog_secondary_text, dialog_close_text;
	IInspectable *dialog_primary_command, *dialog_secondary_command, *dialog_primary_command_parameter,
		*dialog_secondary_command_parameter, *dialog_close_command, *dialog_close_command_parameter,
		*dialog_primary_style, *dialog_secondary_style, *dialog_close_style;
	struct raven_event_list dialog_closing, dialog_closed, dialog_opened;
	struct raven_event_list dialog_primary_click, dialog_secondary_click, dialog_close_click;
	/* The object the Navigate call carried as its parameter: Windows delivers it to the Navigated handlers and
	 * the page's OnNavigatedTo through NavigationEventArgs.Parameter; a Navigate without a parameter holds null. */
	IInspectable *navigation_parameter;
	/* The element's Resources dictionary, created on the first dictionary-typed access (see the
	 * IMap<Object,Object> ask the frame answers below). */
	IInspectable *resources;
	/* The FrameworkElement extent and misc surface the 46-slot IFrameworkElement face reads and writes
	 * honestly; the constructor sets the real metadata defaults (Width/Height NaN auto, Min 0, Max
	 * PositiveInfinity, the alignments Stretch, FlowDirection LeftToRight, an empty Margin). */
	double width, height, min_width, max_width, min_height, max_height;
	double actual_width, actual_height;
	INT32 horizontal_alignment, vertical_alignment, flow_direction;
	struct framework_thickness margin;
	IInspectable *tag, *data_context, *style;
	HSTRING language;
	/* The IFrameworkElement2/3/4 continuation surfaces: RequestedTheme defaults to the app's theme, the
	 * focus-visual fields to the real metadata defaults (AllowFocusOnInteraction true, the thicknesses
	 * empty, the brushes null, AllowFocusWhenDisabled false). */
	INT32 requested_theme, allow_focus_on_interaction, allow_focus_when_disabled;
	struct framework_thickness focus_visual_margin, focus_visual_secondary_thickness, focus_visual_primary_thickness;
	IInspectable *focus_visual_secondary_brush, *focus_visual_primary_brush;
	/* The scalar Control properties the projected IControl stores. Each rides one ABI slot word: floats as
	 * IEEE-754 bits in the low 32 (the convention the generated callers pack), enums and ints in the low 32,
	 * bools in the low byte; the object-typed members carry pointers and hold references like Background. */
	uint64_t font_size, font_weight, font_style, font_stretch, character_spacing;
	uint64_t is_tab_stop, is_enabled, tab_index, tab_navigation;
	uint64_t horizontal_content_alignment, vertical_content_alignment;
	IInspectable *font_family, *foreground, *border_brush, *template;
	int32_t cache_size;
	boolean stack_enabled;
};

struct page;
struct page_inner_part { const IInspectableVtbl *lpVtbl; struct page *owner; };
struct page {
	const __x_ABI_CWindows_CUI_CXaml_CControls_CIPageVtbl *lpVtbl;
	const __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControlVtbl *content_lpVtbl;
	/* The non-delegating IUnknown the composable factory hands the managed projection: the .NET Native page
	 * constructor composes with the managed CCW as the outer, and an unanswered inner leaves the projection's
	 * identity null — the null interface call the traced page construction died on. */
	struct page_inner_part inner;
	LONG refs;
	IInspectable *outer;
	HSTRING name;
	IInspectable *frame, *top_app_bar, *bottom_app_bar, *content;
	int32_t cache_mode;
};

static HRESULT frame_query_any(struct frame *self, REFIID iid, void **out);
static ULONG frame_addref(struct frame *self);
static ULONG frame_release(struct frame *self);
static HRESULT WINAPI frame_iids(frame1 *This, ULONG *count, IID **iids);
static HRESULT WINAPI frame_trust(frame1 *This, TrustLevel *level);
static HRESULT WINAPI frame_name(frame1 *This, HSTRING *name);
static HRESULT WINAPI frame2_iids(frame2 *This, ULONG *count, IID **iids);
static HRESULT WINAPI frame2_trust(frame2 *This, TrustLevel *level);
static HRESULT WINAPI frame2_name(frame2 *This, HSTRING *name);
static HRESULT WINAPI frame3_iids(frame3 *This, ULONG *count, IID **iids);
static HRESULT WINAPI frame3_trust(frame3 *This, TrustLevel *level);
static HRESULT WINAPI frame3_name(frame3 *This, HSTRING *name);
static HRESULT WINAPI frame4_iids(frame4 *This, ULONG *count, IID **iids);
static HRESULT WINAPI frame4_trust(frame4 *This, TrustLevel *level);
static HRESULT WINAPI frame4_name(frame4 *This, HSTRING *name);
static HRESULT WINAPI frame5_iids(frame5 *This, ULONG *count, IID **iids);
static HRESULT WINAPI frame5_trust(frame5 *This, TrustLevel *level);
static HRESULT WINAPI frame5_name(frame5 *This, HSTRING *name);

#define FRAME_BOILERPLATE(part, iface, query_fn, addref_fn, release_fn) \
	static HRESULT WINAPI query_fn(iface *This, REFIID iid, void **out) \
	{ struct frame *s=((struct part *)This)->owner; return s->outer ? IInspectable_QueryInterface(s->outer,iid,out) : frame_query_any(s,iid,out); } \
	static ULONG WINAPI addref_fn(iface *This) { struct frame *s=((struct part *)This)->owner; return s->outer ? IInspectable_AddRef(s->outer) : frame_addref(s); } \
	static ULONG WINAPI release_fn(iface *This) { struct frame *s=((struct part *)This)->owner; return s->outer ? IInspectable_Release(s->outer) : frame_release(s); }

FRAME_BOILERPLATE(frame1_part, frame1, frame1_query, frame1_addref, frame1_release)
FRAME_BOILERPLATE(frame2_part, frame2, frame2_query, frame2_addref, frame2_release)
FRAME_BOILERPLATE(frame3_part, frame3, frame3_query, frame3_addref, frame3_release)
FRAME_BOILERPLATE(frame4_part, frame4, frame4_query, frame4_addref, frame4_release)
FRAME_BOILERPLATE(frame5_part, frame5, frame5_query, frame5_addref, frame5_release)
FRAME_BOILERPLATE(frame_control_part, control1, frame_control_query, frame_control_addref, frame_control_release)
FRAME_BOILERPLATE(frame_content_part, struct frame_content_part, frame_content_query, frame_content_addref, frame_content_release)
FRAME_BOILERPLATE(frame_theme_iface, frame_theme_iface, frame_theme_query, frame_theme_addref, frame_theme_release)
FRAME_BOILERPLATE(frame_peer_bridge_part, frame_peer_bridge_part, frame_peer_bridge_query, frame_peer_bridge_addref, frame_peer_bridge_release)
FRAME_BOILERPLATE(frame_peer_shim_part, frame_peer_shim_part, frame_peer_shim_query, frame_peer_shim_addref, frame_peer_shim_release)
FRAME_BOILERPLATE(frame_framework, IInspectable, frame_framework_query, frame_framework_addref, frame_framework_release)
/* The page faces' functions take the real CIPage/CIPageOverrides pointers (the tables are the generated
 * vtbls), so the boilerplate macro's part-typed signatures do not apply; the bodies cast to the part. */
static HRESULT WINAPI frame_page_query(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This, REFIID iid, void **out)
{
	struct frame *self = ((struct frame_page_part *)This)->owner;
	return self->outer ? IInspectable_QueryInterface(self->outer, iid, out) : frame_query_any(self, iid, out);
}
static ULONG WINAPI frame_page_addref(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This)
{
	struct frame *self = ((struct frame_page_part *)This)->owner;
	return self->outer ? IInspectable_AddRef(self->outer) : frame_addref(self);
}
static ULONG WINAPI frame_page_release(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This)
{
	struct frame *self = ((struct frame_page_part *)This)->owner;
	return self->outer ? IInspectable_Release(self->outer) : frame_release(self);
}
static HRESULT WINAPI frame_page_overrides_query(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *This, REFIID iid, void **out)
{
	struct frame *self = ((struct frame_page_overrides_part *)This)->owner;
	return self->outer ? IInspectable_QueryInterface(self->outer, iid, out) : frame_query_any(self, iid, out);
}
static ULONG WINAPI frame_page_overrides_addref(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *This)
{
	struct frame *self = ((struct frame_page_overrides_part *)This)->owner;
	return self->outer ? IInspectable_AddRef(self->outer) : frame_addref(self);
}
static ULONG WINAPI frame_page_overrides_release(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *This)
{
	struct frame *self = ((struct frame_page_overrides_part *)This)->owner;
	return self->outer ? IInspectable_Release(self->outer) : frame_release(self);
}

#define FRAME_INSPECTABLE(iface, prefix, text) \
	static HRESULT WINAPI prefix##_iids(iface *This, ULONG *count, IID **iids) \
	{ (void)This; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI prefix##_trust(iface *This, TrustLevel *level) \
	{ (void)This; return inspectable_trust(level); } \
	static HRESULT WINAPI prefix##_name(iface *This, HSTRING *name) \
	{ (void)This; if (!name) return E_POINTER; return WindowsCreateString(text, (UINT32)lstrlenW(text), name); }

FRAME_INSPECTABLE(frame1, frame, frame_class)
FRAME_INSPECTABLE(frame2, frame2, frame_class)
FRAME_INSPECTABLE(frame3, frame3, frame_class)
FRAME_INSPECTABLE(frame4, frame4, frame_class)
FRAME_INSPECTABLE(frame5, frame5, frame_class)
FRAME_INSPECTABLE(control1, frame_control, frame_class)
FRAME_INSPECTABLE(struct frame_content_part, frame_content, frame_class)
FRAME_INSPECTABLE(frame_theme_iface, frame_theme, frame_class)
FRAME_INSPECTABLE(frame_peer_bridge_part, frame_peer_bridge, frame_class)
FRAME_INSPECTABLE(frame_peer_shim_part, frame_peer_shim, frame_class)
FRAME_INSPECTABLE(IInspectable, frame_framework, frame_class)
static HRESULT WINAPI frame_page_iids(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This, ULONG *count, IID **iids)
{ (void)This; return inspectable_iids(count, iids); }
static HRESULT WINAPI frame_page_name(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This, HSTRING *name)
{ (void)This; if (!name) return E_POINTER; return WindowsCreateString(page_class, (UINT32)lstrlenW(page_class), name); }
static HRESULT WINAPI frame_page_trust(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *This, TrustLevel *level)
{ (void)This; return inspectable_trust(level); }
static HRESULT WINAPI frame_page_overrides_iids(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *This, ULONG *count, IID **iids)
{ (void)This; return inspectable_iids(count, iids); }
static HRESULT WINAPI frame_page_overrides_name(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *This, HSTRING *name)
{ (void)This; if (!name) return E_POINTER; return WindowsCreateString(page_class, (UINT32)lstrlenW(page_class), name); }
static HRESULT WINAPI frame_page_overrides_trust(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *This, TrustLevel *level)
{ (void)This; return inspectable_trust(level); }

/* shortcut: theme inheritance and live theme changes are not yet implemented. */
static HRESULT WINAPI frame_actual_theme(frame_theme_iface *iface, INT32 *out)
{ (void)iface; if (!out) return E_POINTER; *out = 1; return S_OK; }
static HRESULT WINAPI frame_add_theme_changed(frame_theme_iface *iface, IUnknown *handler, INT64 *token)
{
 struct frame *self = iface->owner;
 if (!handler || !token) return E_POINTER;
 struct frame_theme_handler *entry = ALLOC(sizeof *entry);
 if (!entry) return E_OUTOFMEMORY;
 IUnknown_AddRef(handler); entry->handler = handler;
 entry->token = ++self->next_theme_token; entry->next = self->theme_handlers; self->theme_handlers = entry;
 *token = entry->token; return S_OK;
}
static HRESULT WINAPI frame_remove_theme_changed(frame_theme_iface *iface, INT64 token)
{
 struct frame_theme_handler **at = &iface->owner->theme_handlers;
 while (*at && (*at)->token != token) at = &(*at)->next;
 if (*at) { struct frame_theme_handler *entry = *at; *at = entry->next; IUnknown_Release(entry->handler); FREE(entry); }
 return S_OK;
}
static const frame_theme_vtbl frame_theme_table = {
 frame_theme_query, frame_theme_addref, frame_theme_release, frame_theme_iids, frame_theme_name, frame_theme_trust,
 frame_actual_theme, frame_add_theme_changed, frame_remove_theme_changed
};

/* ---- The WinUI 2.8 DXAML peer bridge ----
 *
 * The private face Microsoft.UI.Xaml.dll asks of the OS XAML peer it keeps in its CoreServices singleton. The
 * traced launch reaches exactly one call: the second method (vtable slot 7) with the class name of the control
 * whose DXAML core is being constructed ("Microsoft.UI.Xaml.Controls.ProgressRing") boxed as IReference<String>,
 * HRESULT-checked by the caller. The first method has no observed caller; answering it E_NOTIMPL with a note
 * keeps the unobserved path diagnosable instead of silently wrong. */
static HRESULT WINAPI frame_peer_bridge_method1(frame_peer_bridge_part *iface, void *argument)
{
	char note[80];
	(void)argument;
	wsprintfA(note, "raven-frame: peer bridge method1\n");
	OutputDebugStringA(note);
	return E_NOTIMPL;
}
static HRESULT WINAPI frame_peer_bridge_class_name(frame_peer_bridge_part *iface, IUnknown *value)
{
	struct frame *self = iface->owner;
	__FIReference_1_HSTRING *reference = NULL;
	HSTRING text = NULL;
	char note[100];
	if (!value)
		return E_POINTER;
	if (FAILED(IUnknown_QueryInterface(value, &IID___FIReference_1_HSTRING, (void **)&reference)) || !reference)
		return S_OK;
	HRESULT result = reference->lpVtbl->get_Value(reference, &text);
	reference->lpVtbl->Release(reference);
	if (FAILED(result) || !text)
		return S_OK;
	UINT32 length = 0;
	const WCHAR *chars = WindowsGetStringRawBuffer(text, &length);
	wsprintfA(note, "raven-frame: peer bridge class %.48ls\n", length ? chars : L"");
	OutputDebugStringA(note);
	/* The name is delivered per DXAML core construction, not per frame state; accepting it is the contract. */
	(void)self;
	WindowsDeleteString(text);
	return S_OK;
}
static const frame_peer_bridge_vtbl frame_peer_bridge_table = {
 frame_peer_bridge_query, frame_peer_bridge_addref, frame_peer_bridge_release,
 frame_peer_bridge_iids, frame_peer_bridge_name, frame_peer_bridge_trust,
 frame_peer_bridge_method1, frame_peer_bridge_class_name
};

/* The second peer face: the generated DXAML accessors call its first method with the two framework context
 * objects they resolved and an out. The traced construction ignores the out value (microsoft.ui.xaml.dll+39a810
 * is a nop after the call), so accepting the delivery with an unset out is the observed contract; the HRESULT
 * itself is checked and a failure throws into the managed constructor. */
static HRESULT WINAPI frame_peer_shim_method1(frame_peer_shim_part *iface, void *context1, void *context2, void **out)
{
	char note[80];
	(void)context1; (void)context2;
	if (out)
		*out = NULL;
	wsprintfA(note, "raven-frame: peer shim method1\n");
	OutputDebugStringA(note);
	return S_OK;
}
static HRESULT WINAPI frame_peer_shim_method2(frame_peer_shim_part *iface)
{
	char note[80];
	(void)iface;
	wsprintfA(note, "raven-frame: peer shim method2\n");
	OutputDebugStringA(note);
	return E_NOTIMPL;
}
static const frame_peer_shim_vtbl frame_peer_shim_table = {
 frame_peer_shim_query, frame_peer_shim_addref, frame_peer_shim_release,
 frame_peer_shim_iids, frame_peer_shim_name, frame_peer_shim_trust,
 frame_peer_shim_method1, frame_peer_shim_method2
};

DEFINE_GUID(IID_frame_framework, 0xa391d09b, 0x4a99, 0x4b7c, 0x9d, 0x8d, 0x6f, 0xa5, 0xd0, 0x1f, 0x6f, 0xbf);
/* Windows.UI.Xaml.IFrameworkElement itself: the identifier the real Windows.UI.Xaml.winmd carries on the
 * type (read out of the GuidAttribute blob), so the QI that lands here IS the projected FrameworkElement
 * face the generated app code calls through. */

/* The members with no store and no caller yet keep their ABI and answer E_NOTIMPL, but the getters now
 * write NULL into the out slot first — a caller that ignores the HRESULT then reads a safe null instead of
 * garbage (the traced constructor reads the extent getters' out slots straight through). The putters take
 * a value register, not a pointer, so they never write through it. */
static HRESULT WINAPI framework_get_null(IInspectable *self, void **out)
{ (void)self; if (out) *out = NULL; return E_NOTIMPL; }
static HRESULT WINAPI framework_put_nop(IInspectable *self, void *value)
{ (void)self; (void)value; return E_NOTIMPL; }
static HRESULT WINAPI framework_find_binding(IInspectable *self, void *dp, void *binding)
{ (void)self; (void)dp; (void)binding; return E_NOTIMPL; }
static HRESULT WINAPI framework_name_get(IInspectable *iface, HSTRING *out)
{
 if (!out) return E_POINTER;
 return WindowsDuplicateString(((struct frame_framework *)iface)->owner->name, out);
}
static HRESULT WINAPI framework_name_put(IInspectable *iface, HSTRING name)
{
 struct frame *self = ((struct frame_framework *)iface)->owner;
 HSTRING copy = NULL; HRESULT hr = WindowsDuplicateString(name, &copy);
 if (FAILED(hr)) return hr;
 if (self->name) WindowsDeleteString(self->name);
 self->name = copy; return S_OK;
}
/* The scalar FrameworkElement properties store over the frame's own fields (the defaults the constructor
 * sets are the real metadata defaults: Width/Height NaN (auto), Min 0, Max PositiveInfinity, the
 * alignments Stretch, FlowDirection LeftToRight, an empty Margin). The getters fill the out slot the ABI
 * hands them — doubles whole, the 16-byte Thickness through the pointer the x64 struct convention passes —
 * so a generated caller always reads written memory. The actual extents have no measure pass behind them
 * yet: until one runs, the getter answers the current CoreWindow bounds (the extents the element would be
 * arranged into), 0 when no window is attached. */
#define FRAMEWORK_DOUBLE_PROPERTY(get_name, put_name, field) \
	static HRESULT WINAPI framework_get_##get_name(IInspectable *iface, double *out) \
	{ \
		if (!out) return E_POINTER; *out = ((struct frame_framework *)iface)->owner->field; return S_OK; \
	} \
	static HRESULT WINAPI framework_put_##put_name(IInspectable *iface, double value) \
	{ ((struct frame_framework *)iface)->owner->field = value; return S_OK; }
FRAMEWORK_DOUBLE_PROPERTY(Width, Width, width)
FRAMEWORK_DOUBLE_PROPERTY(Height, Height, height)
FRAMEWORK_DOUBLE_PROPERTY(MinWidth, MinWidth, min_width)
FRAMEWORK_DOUBLE_PROPERTY(MaxWidth, MaxWidth, max_width)
FRAMEWORK_DOUBLE_PROPERTY(MinHeight, MinHeight, min_height)
FRAMEWORK_DOUBLE_PROPERTY(MaxHeight, MaxHeight, max_height)
#undef FRAMEWORK_DOUBLE_PROPERTY
#define FRAMEWORK_INT_PROPERTY(get_name, put_name, field) \
	static HRESULT WINAPI framework_get_##get_name(IInspectable *iface, INT32 *out) \
	{ \
		if (!out) return E_POINTER; *out = ((struct frame_framework *)iface)->owner->field; return S_OK; \
	} \
	static HRESULT WINAPI framework_put_##put_name(IInspectable *iface, INT32 value) \
	{ ((struct frame_framework *)iface)->owner->field = value; return S_OK; }
FRAMEWORK_INT_PROPERTY(HorizontalAlignment, HorizontalAlignment, horizontal_alignment)
FRAMEWORK_INT_PROPERTY(VerticalAlignment, VerticalAlignment, vertical_alignment)
FRAMEWORK_INT_PROPERTY(FlowDirection, FlowDirection, flow_direction)
#undef FRAMEWORK_INT_PROPERTY
static HRESULT WINAPI framework_get_actual_width(IInspectable *iface, double *out)
{
	struct frame *self = ((struct frame_framework *)iface)->owner;
	if (!out) return E_POINTER;
	*out = self->actual_width;
	if (*out > 0.0) return S_OK;
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *core = NULL;
	if (FAILED(raven_current_window(&core)) || !core)
		return S_OK;
	__x_ABI_CWindows_CFoundation_CRect bounds = {0, 0, 0, 0};
	if (core->lpVtbl->get_Bounds(core, &bounds) == S_OK)
		*out = bounds.Width;
	core->lpVtbl->Release(core);
	return S_OK;
}
static HRESULT WINAPI framework_get_actual_height(IInspectable *iface, double *out)
{
	struct frame *self = ((struct frame_framework *)iface)->owner;
	if (!out) return E_POINTER;
	*out = self->actual_height;
	if (*out > 0.0) return S_OK;
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *core = NULL;
	if (FAILED(raven_current_window(&core)) || !core)
		return S_OK;
	__x_ABI_CWindows_CFoundation_CRect bounds = {0, 0, 0, 0};
	if (core->lpVtbl->get_Bounds(core, &bounds) == S_OK)
		*out = bounds.Height;
	core->lpVtbl->Release(core);
	return S_OK;
}
static HRESULT WINAPI framework_get_margin(IInspectable *iface, struct framework_thickness *out)
{
	if (!out) return E_POINTER;
	*out = ((struct frame_framework *)iface)->owner->margin;
	return S_OK;
}
static HRESULT WINAPI framework_put_margin(IInspectable *iface, struct framework_thickness value)
{
	((struct frame_framework *)iface)->owner->margin = value;
	return S_OK;
}
static HRESULT WINAPI framework_get_object(IInspectable *iface, void **out, void *stored)
{
	(void)iface;
	if (!out) return E_POINTER;
	*out = stored;
	if (stored) IInspectable_AddRef((IInspectable *)stored);
	return S_OK;
}
#define FRAMEWORK_OBJECT_PROPERTY(get_name, put_name, field) \
	static HRESULT WINAPI framework_get_##get_name(IInspectable *iface, IInspectable **out) \
	{ \
		return framework_get_object(iface, (void **)out, (void *)((struct frame_framework *)iface)->owner->field); \
	} \
	static HRESULT WINAPI framework_put_##put_name(IInspectable *iface, IInspectable *value) \
	{ \
		struct frame *self = ((struct frame_framework *)iface)->owner; \
		if (value) IInspectable_AddRef(value); \
		if (self->field) IInspectable_Release((IInspectable *)self->field); \
		self->field = (IInspectable *)value; return S_OK; \
	}
FRAMEWORK_OBJECT_PROPERTY(Tag, Tag, tag)
FRAMEWORK_OBJECT_PROPERTY(DataContext, DataContext, data_context)
FRAMEWORK_OBJECT_PROPERTY(Style, Style, style)
#undef FRAMEWORK_OBJECT_PROPERTY
static HRESULT WINAPI framework_get_language(IInspectable *iface, HSTRING *out)
{
	(void)iface;
	if (!out) return E_POINTER;
	return WindowsDuplicateString(((struct frame_framework *)iface)->owner->language, out);
}
static HRESULT WINAPI framework_put_language(IInspectable *iface, HSTRING value)
{
	struct frame *self = ((struct frame_framework *)iface)->owner;
	HSTRING copy = NULL; HRESULT hr = WindowsDuplicateString(value, &copy);
	if (FAILED(hr)) return hr;
	if (self->language) WindowsDeleteString(self->language);
	self->language = copy;
	return S_OK;
}
/* Resources is the element's own dictionary: the same object the IMap<Object,Object> ask on the frame
 * answers with, created on the first touch. The projected property hands the map face of it. */
static HRESULT WINAPI framework_get_resources(IInspectable *iface, IInspectable **out)
{
	struct frame *self = ((struct frame_framework *)iface)->owner;
	if (!out) return E_POINTER;
	*out = NULL;
	if (!self->resources) {
		IInspectable *created = NULL;
		const HRESULT made = raven_xaml_create_stable(371, &created);
		if (FAILED(made) || !created)
			return E_OUTOFMEMORY;
		self->resources = created;
	}
	return IInspectable_QueryInterface(self->resources, &IID___FIMap_2_IInspectable_IInspectable, (void **)out);
}
static HRESULT WINAPI framework_put_resources(IInspectable *iface, IInspectable *value)
{
	struct frame *self = ((struct frame_framework *)iface)->owner;
	if (value) IInspectable_AddRef(value);
	if (self->resources) IInspectable_Release(self->resources);
	self->resources = value;
	return S_OK;
}
static HRESULT WINAPI framework_get_base_uri(IInspectable *iface, IInspectable **out)
{
	(void)iface;
	if (!out) return E_POINTER;
	/* An element built in code has no base URI until it joins parsed XAML; null is the real answer then. */
	*out = NULL;
	return S_OK;
}
static HRESULT WINAPI framework_get_parent(IInspectable *iface, IInspectable **out)
{
	(void)iface;
	if (!out) return E_POINTER;
	/* The frame is the navigation root: it has no XAML parent. */
	*out = NULL;
	return S_OK;
}
static HRESULT WINAPI framework_triggers(IInspectable *iface, IInspectable **out)
{
	(void)iface;
	if (!out) return E_POINTER;
	/* The engine builds no Trigger collection yet; a null out with E_NOTIMPL keeps an unchecked caller
	 * from reading garbage while a checking caller throws like it would against the unimplemented face. */
	*out = NULL;
	return E_NOTIMPL;
}
/* shortcut: registrations are retained; raise them when Frame participates in layout and loaded/unloaded transitions. */
static HRESULT framework_event_add(IInspectable *iface, IUnknown *handler, INT64 *token, unsigned kind)
{
 struct frame *self = ((struct frame_framework *)iface)->owner;
 if (!handler || !token) return E_POINTER;
 struct framework_handler *entry = ALLOC(sizeof *entry);
 if (!entry) return E_OUTOFMEMORY;
 IUnknown_AddRef(handler); entry->handler = handler; entry->kind = kind;
 entry->token = ++self->next_framework_token; entry->next = self->framework_handlers; self->framework_handlers = entry;
 *token = entry->token; return S_OK;
}
static HRESULT framework_event_remove(IInspectable *iface, INT64 token, unsigned kind)
{
 struct framework_handler **at = &((struct frame_framework *)iface)->owner->framework_handlers;
 while (*at && ((*at)->token != token || (*at)->kind != kind)) at = &(*at)->next;
 if (*at) { struct framework_handler *entry = *at; *at = entry->next; IUnknown_Release(entry->handler); FREE(entry); }
 return S_OK;
}
#define FRAMEWORK_EVENT(name, kind) \
 static HRESULT WINAPI framework_add_##name(IInspectable *iface, IUnknown *handler, INT64 *token) \
 { return framework_event_add(iface, handler, token, kind); } \
 static HRESULT WINAPI framework_remove_##name(IInspectable *iface, INT64 token) \
 { return framework_event_remove(iface, token, kind); }
FRAMEWORK_EVENT(loaded, 0)
FRAMEWORK_EVENT(unloaded, 1)
FRAMEWORK_EVENT(size_changed, 2)
FRAMEWORK_EVENT(layout_updated, 3)
static HRESULT WINAPI framework_find_name(IInspectable *self, HSTRING name, IInspectable **out)
{ (void)self; (void)name; if (!out) return E_POINTER; *out = NULL; return E_NOTIMPL; }
/* Slot order is the real Windows.UI.Xaml.IFrameworkElement order, reconstructed from the GuidAttribute-
 * carrying Windows.UI.Xaml.winmd (the file the generated headers were built from, SHA-256 c55875b0...):
 * slot 6 get_Triggers, 7/8 Resources get/put, 9/10 Tag get/put, 11/12 Language get/put, 13 ActualWidth,
 * 14 ActualHeight, 15/16 Width get/put, 17/18 Height get/put, 19/20 MinWidth get/put, 21/22 MaxWidth
 * get/put, 23/24 MinHeight get/put, 25/26 MaxHeight get/put, 27/28 HorizontalAlignment get/put, 29/30
 * VerticalAlignment get/put, 31/32 Margin get/put, 33/34 Name get/put, 35 BaseUri, 36/37 DataContext
 * get/put, 38/39 Style get/put, 40 Parent, 41/42 FlowDirection get/put, 43-50 the Loaded/Unloaded/
 * SizeChanged/LayoutUpdated event pairs, 51 FindName, 52 SetBinding. */
struct framework_vtbl { IInspectableVtbl inspectable; void *slots[47]; };
static const struct framework_vtbl frame_framework_table = {
 { frame_framework_query, frame_framework_addref, frame_framework_release, frame_framework_iids, frame_framework_name, frame_framework_trust },
 {
  (void *)framework_triggers,                 /* 6  get_Triggers */
  (void *)framework_get_resources,            /* 7  get_Resources */
  (void *)framework_put_resources,            /* 8  put_Resources */
  (void *)framework_get_Tag,                  /* 9  get_Tag */
  (void *)framework_put_Tag,                  /* 10 put_Tag */
  (void *)framework_get_language,             /* 11 get_Language */
  (void *)framework_put_language,             /* 12 put_Language */
  (void *)framework_get_actual_width,         /* 13 get_ActualWidth */
  (void *)framework_get_actual_height,        /* 14 get_ActualHeight */
  (void *)framework_get_Width,                /* 15 get_Width */
  (void *)framework_put_Width,                /* 16 put_Width */
  (void *)framework_get_Height,               /* 17 get_Height */
  (void *)framework_put_Height,               /* 18 put_Height */
  (void *)framework_get_MinWidth,             /* 19 get_MinWidth */
  (void *)framework_put_MinWidth,             /* 20 put_MinWidth */
  (void *)framework_get_MaxWidth,             /* 21 get_MaxWidth */
  (void *)framework_put_MaxWidth,             /* 22 put_MaxWidth */
  (void *)framework_get_MinHeight,            /* 23 get_MinHeight */
  (void *)framework_put_MinHeight,            /* 24 put_MinHeight */
  (void *)framework_get_MaxHeight,            /* 25 get_MaxHeight */
  (void *)framework_put_MaxHeight,            /* 26 put_MaxHeight */
  (void *)framework_get_HorizontalAlignment,  /* 27 get_HorizontalAlignment */
  (void *)framework_put_HorizontalAlignment,  /* 28 put_HorizontalAlignment */
  (void *)framework_get_VerticalAlignment,    /* 29 get_VerticalAlignment */
  (void *)framework_put_VerticalAlignment,    /* 30 put_VerticalAlignment */
  (void *)framework_get_margin,               /* 31 get_Margin */
  (void *)framework_put_margin,               /* 32 put_Margin */
  (void *)framework_name_get,                 /* 33 get_Name */
  (void *)framework_name_put,                 /* 34 put_Name */
  (void *)framework_get_base_uri,             /* 35 get_BaseUri */
  (void *)framework_get_DataContext,          /* 36 get_DataContext */
  (void *)framework_put_DataContext,          /* 37 put_DataContext */
  (void *)framework_get_Style,                /* 38 get_Style */
  (void *)framework_put_Style,                /* 39 put_Style */
  (void *)framework_get_parent,               /* 40 get_Parent */
  (void *)framework_get_FlowDirection,        /* 41 get_FlowDirection */
  (void *)framework_put_FlowDirection,        /* 42 put_FlowDirection */
  (void *)framework_add_loaded, (void *)framework_remove_loaded,          /* 43-44 */
  (void *)framework_add_unloaded, (void *)framework_remove_unloaded,      /* 45-46 */
  (void *)framework_add_size_changed, (void *)framework_remove_size_changed, /* 47-48 */
  (void *)framework_add_layout_updated, (void *)framework_remove_layout_updated, /* 49-50 */
  (void *)framework_find_name,                /* 51 FindName */
  (void *)framework_find_binding              /* 52 SetBinding */
 }
};

/* shortcut: the scalar Control properties store and return honestly over the frame's own fields. The getter
 * fills the out slot word (the caller reads the value from the ABI slot — floats as IEEE-754 bits in the low
 * 32, enums/ints in the low 32, bools in the low byte); the object-typed members hold references the way
 * Background does. Rendering does not consume these yet, but the projected Control constructors set them. */
static HRESULT WINAPI frame_control_get_word(control1 *iface, uint64_t *value, uint64_t word)
{
	(void)iface;
	if (!value) return E_POINTER;
	*value = word;
	return S_OK;
}
static HRESULT WINAPI frame_control_put_word(control1 *iface, uint64_t *field, uint64_t word)
{
	(void)iface;
	*field = word;
	return S_OK;
}
static HRESULT WINAPI frame_control_get_object(control1 *iface, uint64_t *value, IInspectable *stored)
{
	(void)iface;
	if (!value) return E_POINTER;
	*(IInspectable **)value = stored;
	if (stored) IInspectable_AddRef(stored);
	return S_OK;
}
static HRESULT WINAPI frame_control_put_object(control1 *iface, IInspectable **field, uint64_t word)
{
	struct frame *self = ((struct frame_control_part *)iface)->owner;
	IInspectable *value = (IInspectable *)(uintptr_t)word;
	if (value) IInspectable_AddRef(value);
	if (*field) IInspectable_Release(*field);
	*field = value;
	return S_OK;
}

/* shortcut: the members with no storage or caller remain E_NOTIMPL — the event subscriptions that would
 * never fire and the template-application call that would have to instantiate a template the engine cannot
 * build yet. */
static HRESULT WINAPI frame_control_unimplemented(control1 *self, uint64_t value)
{ (void)self; (void)value; return E_NOTIMPL; }
static HRESULT WINAPI frame_control_unimplemented2(control1 *self, uint64_t a, uint64_t b)
{ (void)self; (void)a; (void)b; return E_NOTIMPL; }

/* IsEnabledChanged rides the shared event machinery: the stub's flattened slot 43 (the six IInspectable
 * methods plus this interface's own thirty-seventh member, the winmd's declaration order — the interface
 * does not flatten its ancestors' members) lands here with the handler as the marshal's CCW and the token
 * out-pointer packed the way every caller packs an EventRegistrationToken slot. */
static HRESULT WINAPI frame_control_add_IsEnabledChanged(control1 *iface, uint64_t handler, uint64_t token)
{
	return raven_event_add(&((struct frame_control_part *)iface)->owner->is_enabled_changed,
		(IUnknown *)(uintptr_t)handler, (EventRegistrationToken *)(uintptr_t)token);
}
static HRESULT WINAPI frame_control_remove_IsEnabledChanged(control1 *iface, uint64_t token)
{
	EventRegistrationToken value;
	value.value = (INT64)token;
	return raven_event_remove(&((struct frame_control_part *)iface)->owner->is_enabled_changed, value);
}

#define FRAME_CONTROL_WORD_PROPERTY(name, field) \
	static HRESULT WINAPI frame_control_get_##name(control1 *iface, uint64_t value) \
	{ return frame_control_get_word(iface, (uint64_t *)(uintptr_t)value, ((struct frame_control_part *)iface)->owner->field); } \
	static HRESULT WINAPI frame_control_put_##name(control1 *iface, uint64_t value) \
	{ return frame_control_put_word(iface, &((struct frame_control_part *)iface)->owner->field, value); }
FRAME_CONTROL_WORD_PROPERTY(FontSize, font_size)
FRAME_CONTROL_WORD_PROPERTY(FontWeight, font_weight)
FRAME_CONTROL_WORD_PROPERTY(FontStyle, font_style)
FRAME_CONTROL_WORD_PROPERTY(FontStretch, font_stretch)
FRAME_CONTROL_WORD_PROPERTY(CharacterSpacing, character_spacing)
FRAME_CONTROL_WORD_PROPERTY(IsTabStop, is_tab_stop)
FRAME_CONTROL_WORD_PROPERTY(IsEnabled, is_enabled)
FRAME_CONTROL_WORD_PROPERTY(TabIndex, tab_index)
FRAME_CONTROL_WORD_PROPERTY(TabNavigation, tab_navigation)
FRAME_CONTROL_WORD_PROPERTY(HorizontalContentAlignment, horizontal_content_alignment)
FRAME_CONTROL_WORD_PROPERTY(VerticalContentAlignment, vertical_content_alignment)
#undef FRAME_CONTROL_WORD_PROPERTY
#define FRAME_CONTROL_OBJECT_PROPERTY(name, field) \
	static HRESULT WINAPI frame_control_get_##name(control1 *iface, uint64_t value) \
	{ return frame_control_get_object(iface, (uint64_t *)(uintptr_t)value, ((struct frame_control_part *)iface)->owner->field); } \
	static HRESULT WINAPI frame_control_put_##name(control1 *iface, uint64_t value) \
	{ return frame_control_put_object(iface, &((struct frame_control_part *)iface)->owner->field, value); }
FRAME_CONTROL_OBJECT_PROPERTY(FontFamily, font_family)
FRAME_CONTROL_OBJECT_PROPERTY(Foreground, foreground)
FRAME_CONTROL_OBJECT_PROPERTY(Template, template)
FRAME_CONTROL_OBJECT_PROPERTY(BorderBrush, border_brush)
#undef FRAME_CONTROL_OBJECT_PROPERTY

/* shortcut: Background is retained but Frame does not paint it until the frame render path supports brushes. */
static HRESULT WINAPI frame_control_background(control1 *iface, IInspectable **out)
{
 if (!out) return E_POINTER;
 *out = ((struct frame_control_part *)iface)->owner->background;
 if (*out) IInspectable_AddRef(*out);
 return S_OK;
}
static HRESULT WINAPI frame_control_put_background(control1 *iface, IInspectable *brush)
{
 struct frame *self = ((struct frame_control_part *)iface)->owner;
 if (brush) IInspectable_AddRef(brush);
 if (self->background) IInspectable_Release(self->background);
 self->background = brush;
 return S_OK;
}

/* FocusState: no focus machinery drives the Control yet — Unfocused is the honest resting answer. */
static HRESULT WINAPI frame_control_get_FocusState(control1 *iface, uint64_t value)
{ return frame_control_get_word(iface, (uint64_t *)(uintptr_t)value, 0); }

static const control1_vtbl frame_control_table = {
	.QueryInterface = frame_control_query,
	.AddRef = frame_control_addref,
	.Release = frame_control_release,
	.GetIids = frame_control_iids,
	.GetRuntimeClassName = frame_control_name,
	.GetTrustLevel = frame_control_trust,
	.get_FontSize = frame_control_get_FontSize,
	.put_FontSize = frame_control_put_FontSize,
	.get_FontFamily = frame_control_get_FontFamily,
	.put_FontFamily = frame_control_put_FontFamily,
	.get_FontWeight = frame_control_get_FontWeight,
	.put_FontWeight = frame_control_put_FontWeight,
	.get_FontStyle = frame_control_get_FontStyle,
	.put_FontStyle = frame_control_put_FontStyle,
	.get_FontStretch = frame_control_get_FontStretch,
	.put_FontStretch = frame_control_put_FontStretch,
	.get_CharacterSpacing = frame_control_get_CharacterSpacing,
	.put_CharacterSpacing = frame_control_put_CharacterSpacing,
	.get_Foreground = frame_control_get_Foreground,
	.put_Foreground = frame_control_put_Foreground,
	.get_IsTabStop = frame_control_get_IsTabStop,
	.put_IsTabStop = frame_control_put_IsTabStop,
	.get_IsEnabled = frame_control_get_IsEnabled,
	.put_IsEnabled = frame_control_put_IsEnabled,
	.get_TabIndex = frame_control_get_TabIndex,
	.put_TabIndex = frame_control_put_TabIndex,
	.get_TabNavigation = frame_control_get_TabNavigation,
	.put_TabNavigation = frame_control_put_TabNavigation,
	.get_Template = frame_control_get_Template,
	.put_Template = frame_control_put_Template,
	.get_Padding = frame_control_unimplemented,
	.put_Padding = frame_control_unimplemented,
	.get_HorizontalContentAlignment = frame_control_get_HorizontalContentAlignment,
	.put_HorizontalContentAlignment = frame_control_put_HorizontalContentAlignment,
	.get_VerticalContentAlignment = frame_control_get_VerticalContentAlignment,
	.put_VerticalContentAlignment = frame_control_put_VerticalContentAlignment,
	.get_Background = frame_control_background,
	.put_Background = frame_control_put_background,
	.get_BorderThickness = frame_control_unimplemented,
	.put_BorderThickness = frame_control_unimplemented,
	.get_BorderBrush = frame_control_get_BorderBrush,
	.put_BorderBrush = frame_control_put_BorderBrush,
	.get_FocusState = frame_control_get_FocusState,
	.add_IsEnabledChanged = frame_control_add_IsEnabledChanged,
	.remove_IsEnabledChanged = frame_control_remove_IsEnabledChanged,
	.ApplyTemplate = frame_control_unimplemented,
	.Focus = frame_control_unimplemented2,
};

/* shortcut: template and transition values are retained, but are not applied to Frame's rendering yet. */
#define FRAME_CONTENT_ACCESSORS(suffix, field) \
 static HRESULT WINAPI frame_get_##suffix(struct frame_content_part *iface, IInspectable **out) \
 { if (!out) return E_POINTER; *out=iface->owner->field; if (*out) IInspectable_AddRef(*out); return S_OK; } \
 static HRESULT WINAPI frame_put_##suffix(struct frame_content_part *iface, IInspectable *value) \
 { struct frame *self=iface->owner; if (value) IInspectable_AddRef(value); if (self->field) IInspectable_Release(self->field); self->field=value; raven_render_changed(); return S_OK; }
FRAME_CONTENT_ACCESSORS(content, page)
FRAME_CONTENT_ACCESSORS(content_template, content_template)
FRAME_CONTENT_ACCESSORS(content_selector, content_selector)
FRAME_CONTENT_ACCESSORS(content_transitions, content_transitions)
#undef FRAME_CONTENT_ACCESSORS
static const struct frame_content_vtbl frame_content_table = {
 frame_content_query, frame_content_addref, frame_content_release, frame_content_iids, frame_content_name, frame_content_trust,
 frame_get_content, frame_put_content, frame_get_content_template, frame_put_content_template,
 frame_get_content_selector, frame_put_content_selector, frame_get_content_transitions, frame_put_content_transitions
};

/* IUserControl of the standalone UserControl: Content stores the element the compiled document applied at the
 * page-scope pop, exactly Frame's Content does. The face is served only by frames the UserControl factory made
 * (standalone_usercontrol), so a Frame never answers the cast walks that probe for it. */
static const WCHAR usercontrol_class[] = L"Windows.UI.Xaml.Controls.UserControl";
static HRESULT WINAPI frame_usercontrol_query(usercontrol_i *iface, REFIID iid, void **out)
{
	struct frame *self = ((struct frame_usercontrol_part *)iface)->owner;
	return self->outer ? IInspectable_QueryInterface(self->outer, iid, out) : frame_query_any(self, iid, out);
}
static ULONG WINAPI frame_usercontrol_addref(usercontrol_i *iface)
{ return ((struct frame_usercontrol_part *)iface)->owner->outer ?
	IInspectable_AddRef(((struct frame_usercontrol_part *)iface)->owner->outer) :
	frame_addref(((struct frame_usercontrol_part *)iface)->owner); }
static ULONG WINAPI frame_usercontrol_release(usercontrol_i *iface)
{ return ((struct frame_usercontrol_part *)iface)->owner->outer ?
	IInspectable_Release(((struct frame_usercontrol_part *)iface)->owner->outer) :
	frame_release(((struct frame_usercontrol_part *)iface)->owner); }
static HRESULT WINAPI frame_usercontrol_iids(usercontrol_i *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI frame_usercontrol_name(usercontrol_i *iface, HSTRING *name)
{ (void)iface; if (!name) return E_POINTER;
	return WindowsCreateString(usercontrol_class, (UINT32)lstrlenW(usercontrol_class), name); }
static HRESULT WINAPI frame_usercontrol_trust(usercontrol_i *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static HRESULT WINAPI frame_usercontrol_get_content(usercontrol_i *iface, IInspectable **out)
{
	struct frame *self = ((struct frame_usercontrol_part *)iface)->owner;
	if (!out) return E_POINTER;
	*out = self->usercontrol_content;
	if (*out) (*out)->lpVtbl->AddRef(*out);
	return S_OK;
}
static HRESULT WINAPI frame_usercontrol_put_content(usercontrol_i *iface, IInspectable *value)
{
	struct frame *self = ((struct frame_usercontrol_part *)iface)->owner;
	if (value) value->lpVtbl->AddRef(value);
	if (self->usercontrol_content) self->usercontrol_content->lpVtbl->Release(self->usercontrol_content);
	self->usercontrol_content = value;
	raven_render_changed();
	return S_OK;
}
static const usercontrol_vtbl frame_usercontrol_table = {
	frame_usercontrol_query, frame_usercontrol_addref, frame_usercontrol_release,
	frame_usercontrol_iids, frame_usercontrol_name, frame_usercontrol_trust,
	frame_usercontrol_get_content, frame_usercontrol_put_content,
};

/* IContentDialog and IContentDialog2 of the standalone ContentDialog the contentdialog factory composes:
 * the faces the traced launch's cast walk needed (the frame refused 38dc4404-... and the marshal threw the
 * InvalidCastException the fail-fast wall carried). The properties store honestly over the dialog state, the
 * event subscriptions ride the shared event machinery (accepted without raising until a presentation path
 * exists), and the presentation members answer E_NOTIMPL with a note naming the next frontier. */
static HRESULT WINAPI frame_contentdialog_query(contentdialog_i *iface, REFIID iid, void **out)
{
	struct frame *self = ((struct frame_contentdialog_part *)iface)->owner;
	return self->outer ? IInspectable_QueryInterface(self->outer, iid, out) : frame_query_any(self, iid, out);
}
static ULONG WINAPI frame_contentdialog_addref(contentdialog_i *iface)
{ return ((struct frame_contentdialog_part *)iface)->owner->outer ?
	IInspectable_AddRef(((struct frame_contentdialog_part *)iface)->owner->outer) :
	frame_addref(((struct frame_contentdialog_part *)iface)->owner); }
static ULONG WINAPI frame_contentdialog_release(contentdialog_i *iface)
{ return ((struct frame_contentdialog_part *)iface)->owner->outer ?
	IInspectable_Release(((struct frame_contentdialog_part *)iface)->owner->outer) :
	frame_release(((struct frame_contentdialog_part *)iface)->owner); }
static HRESULT WINAPI frame_contentdialog_iids(contentdialog_i *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI frame_contentdialog_name(contentdialog_i *iface, HSTRING *name)
{ (void)iface; if (!name) return E_POINTER;
	return WindowsCreateString(contentdialog_class, (UINT32)lstrlenW(contentdialog_class), name); }
static HRESULT WINAPI frame_contentdialog_trust(contentdialog_i *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }

static HRESULT WINAPI frame_contentdialog_get_object(contentdialog_i *iface, IInspectable **out, IInspectable *stored)
{ (void)iface; if (!out) return E_POINTER; *out = stored; if (*out) IInspectable_AddRef(*out); return S_OK; }
static HRESULT WINAPI frame_contentdialog_put_object(contentdialog_i *iface, IInspectable **field, IInspectable *value)
{ struct frame *self = ((struct frame_contentdialog_part *)iface)->owner; (void)self;
	if (value) IInspectable_AddRef(value); if (*field) IInspectable_Release(*field); *field = value; return S_OK; }
static HRESULT WINAPI frame_contentdialog_get_text(contentdialog_i *iface, HSTRING *out, HSTRING stored)
{ (void)iface; if (!out) return E_POINTER; return WindowsDuplicateString(stored, out); }
static HRESULT WINAPI frame_contentdialog_put_text(contentdialog_i *iface, HSTRING *field, HSTRING value)
{ HSTRING copy = NULL; HRESULT hr = WindowsDuplicateString(value, &copy);
	if (FAILED(hr)) return hr; if (*field) WindowsDeleteString(*field); *field = copy; return S_OK; }
static HRESULT WINAPI frame_contentdialog_get_bool(contentdialog_i *iface, boolean *out, uint64_t stored)
{ (void)iface; if (!out) return E_POINTER; *out = (boolean)(stored & 0xff); return S_OK; }
static HRESULT WINAPI frame_contentdialog_put_bool(contentdialog_i *iface, uint64_t *field, boolean value)
{ (void)iface; *field = (uint64_t)value; return S_OK; }

#define FRAME_CONTENTDIALOG_OBJECT_PROPERTY(name, field) \
	static HRESULT WINAPI frame_contentdialog_get_##name(contentdialog_i *iface, IInspectable **out) \
	{ return frame_contentdialog_get_object(iface, out, ((struct frame_contentdialog_part *)iface)->owner->field); } \
	static HRESULT WINAPI frame_contentdialog_put_##name(contentdialog_i *iface, IInspectable *value) \
	{ return frame_contentdialog_put_object(iface, &((struct frame_contentdialog_part *)iface)->owner->field, value); }
FRAME_CONTENTDIALOG_OBJECT_PROPERTY(Title, dialog_title)
FRAME_CONTENTDIALOG_OBJECT_PROPERTY(TitleTemplate, dialog_title_template)
FRAME_CONTENTDIALOG_OBJECT_PROPERTY(PrimaryButtonCommand, dialog_primary_command)
FRAME_CONTENTDIALOG_OBJECT_PROPERTY(SecondaryButtonCommand, dialog_secondary_command)
FRAME_CONTENTDIALOG_OBJECT_PROPERTY(PrimaryButtonCommandParameter, dialog_primary_command_parameter)
FRAME_CONTENTDIALOG_OBJECT_PROPERTY(SecondaryButtonCommandParameter, dialog_secondary_command_parameter)
#undef FRAME_CONTENTDIALOG_OBJECT_PROPERTY
#define FRAME_CONTENTDIALOG_TEXT_PROPERTY(name, field) \
	static HRESULT WINAPI frame_contentdialog_get_##name(contentdialog_i *iface, HSTRING *out) \
	{ return frame_contentdialog_get_text(iface, out, ((struct frame_contentdialog_part *)iface)->owner->field); } \
	static HRESULT WINAPI frame_contentdialog_put_##name(contentdialog_i *iface, HSTRING value) \
	{ return frame_contentdialog_put_text(iface, &((struct frame_contentdialog_part *)iface)->owner->field, value); }
FRAME_CONTENTDIALOG_TEXT_PROPERTY(PrimaryButtonText, dialog_primary_text)
FRAME_CONTENTDIALOG_TEXT_PROPERTY(SecondaryButtonText, dialog_secondary_text)
#undef FRAME_CONTENTDIALOG_TEXT_PROPERTY
#define FRAME_CONTENTDIALOG_BOOL_PROPERTY(name, field) \
	static HRESULT WINAPI frame_contentdialog_get_##name(contentdialog_i *iface, boolean *out) \
	{ return frame_contentdialog_get_bool(iface, out, ((struct frame_contentdialog_part *)iface)->owner->field); } \
	static HRESULT WINAPI frame_contentdialog_put_##name(contentdialog_i *iface, boolean value) \
	{ return frame_contentdialog_put_bool(iface, &((struct frame_contentdialog_part *)iface)->owner->field, value); }
FRAME_CONTENTDIALOG_BOOL_PROPERTY(FullSizeDesired, dialog_full_size)
FRAME_CONTENTDIALOG_BOOL_PROPERTY(IsPrimaryButtonEnabled, dialog_is_primary_enabled)
FRAME_CONTENTDIALOG_BOOL_PROPERTY(IsSecondaryButtonEnabled, dialog_is_secondary_enabled)
#undef FRAME_CONTENTDIALOG_BOOL_PROPERTY
#define FRAME_CONTENTDIALOG_EVENT(name, list) \
	static HRESULT WINAPI frame_contentdialog_add_##name(contentdialog_i *iface, void *handler, EventRegistrationToken *token) \
	{ return raven_event_add(&((struct frame_contentdialog_part *)iface)->owner->list, (IUnknown *)handler, token); } \
	static HRESULT WINAPI frame_contentdialog_remove_##name(contentdialog_i *iface, EventRegistrationToken token) \
	{ return raven_event_remove(&((struct frame_contentdialog_part *)iface)->owner->list, token); }
FRAME_CONTENTDIALOG_EVENT(Closing, dialog_closing)
FRAME_CONTENTDIALOG_EVENT(Closed, dialog_closed)
FRAME_CONTENTDIALOG_EVENT(Opened, dialog_opened)
FRAME_CONTENTDIALOG_EVENT(PrimaryButtonClick, dialog_primary_click)
FRAME_CONTENTDIALOG_EVENT(SecondaryButtonClick, dialog_secondary_click)
#undef FRAME_CONTENTDIALOG_EVENT

/* ---- The ShowAsync operation: the completed IAsyncOperation`1<ContentDialogResult> storage.c's and
 * display_monitor.c's sync-completing operations take, instantiated for the enum result. Windows' documented
 * semantics complete the operation when the dialog closes, and the documented result for a dialog taken down
 * without a button click — Hide, or its own close path — is None ("No button was tapped"). Raven has no
 * presentation path yet, so there is no user interaction to wait for and nothing honest to leave pending: the
 * operation finishes before ShowAsync returns, result None, Status Completed from the first read, put_Completed
 * (if the guest set one) invoked inside ShowAsync itself, GetResults handing the enum value. A pending-forever
 * operation would hang the app's await silently, and a fake button result would claim an interaction that never
 * happened — the immediate None keeps the app's pipeline moving and the loud trace note keeps the next launch
 * honest about what the app does with the result. The presentation path (a real visible dialog raising Opened
 * and the button events to Primary/Secondary) stays future work gated on the surface/present stage. ---- */

static const WCHAR dialog_operation_type_name[] =
	L"Windows.Foundation.IAsyncOperation`1<Windows.UI.Xaml.Controls.ContentDialogResult>";

/* the async failure codes winerror.h does not carry (the display_monitor.c guards) */
#ifndef E_ILLEGAL_METHOD_CALL
#define E_ILLEGAL_METHOD_CALL ((HRESULT)0x8000000e)
#endif
#ifndef E_ILLEGAL_DELEGATE_ASSIGNMENT
#define E_ILLEGAL_DELEGATE_ASSIGNMENT ((HRESULT)0x80000018)
#endif
#ifndef RO_E_CLOSED
#define RO_E_CLOSED ((HRESULT)0x80000013)
#endif

typedef __FIAsyncOperation_1_Windows__CUI__CXaml__CControls__CContentDialogResult dialog_op_i;
typedef __FIAsyncOperation_1_Windows__CUI__CXaml__CControls__CContentDialogResultVtbl dialog_op_iVtbl;
typedef __FIAsyncOperationCompletedHandler_1_Windows__CUI__CXaml__CControls__CContentDialogResult dialog_done_i;

/* the two parameterized instance identifiers windows_ui_xaml_contentdialog.h computes and documents */
static const IID dialog_operation_instance_iids[2] = {
	{0x1f23bdd1, 0x06dc, 0x5be9, {0x9a, 0x60, 0x0b, 0x4d, 0x94, 0xd4, 0xd7, 0x2c}},
	{0xd5476170, 0x5fa4, 0x5660, {0xab, 0xec, 0x08, 0xaf, 0xbc, 0x01, 0x68, 0x58}},
};

static LONG next_dialog_async_id;

/* The hand-declared interfaces have no COBJMACROS wrappers: the completed delegate is the guest's own object, so
 * addref, release and invoke go through its vtable (the display_monitor.c helper shape). */
static void dialog_handler_addref(void *handler)
{
	void **table = *(void ***)handler;
	((ULONG (WINAPI *)(void *))table[1])(handler);
}
static void dialog_handler_release(void *handler)
{
	void **table = *(void ***)handler;
	((ULONG (WINAPI *)(void *))table[2])(handler);
}
static void dialog_handler_invoke(void *handler, void *operation, AsyncStatus status)
{
	void **table = *(void ***)handler;
	((HRESULT (WINAPI *)(void *, void *, AsyncStatus))table[3])(handler, operation, status);
}

struct dialog_operation {
	dialog_op_i iface;
	IAsyncInfo info;
	struct {
		LONG refs;
		CRITICAL_SECTION lock;
		AsyncStatus status;
		HRESULT error;
		UINT32 id;
		BOOL closed, assigned;
		void *completed;
	} core;
	INT32 result;
	struct raven_weak_source weak;
};

static void dialog_operation_free(struct dialog_operation *self)
{
	raven_weak_source_disconnect(&self->weak);
	DeleteCriticalSection(&self->core.lock);
}

static HRESULT WINAPI dialog_operation_query(dialog_op_i *iface, REFIID iid, void **out)
{
	struct dialog_operation *self = (struct dialog_operation *)iface;
	UINT which;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IAgileObject))
		*out = iface;
	else if (equal_iid(iid, &IID_IWeakReferenceSource))
		*out = &self->weak;
	else {
		for (which = 0; which < 2 && !*out; ++which)
			if (equal_iid(iid, &dialog_operation_instance_iids[which]))
				*out = iface;
		if (!*out) {
			if (equal_iid(iid, &IID_IAsyncInfo))
				*out = &self->info;
			else {
				char note[112];
				wsprintfA(note, "raven-xaml: dialog operation has no interface %08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
					(unsigned long)iid->Data1, iid->Data2, iid->Data3, iid->Data4[0], iid->Data4[1], iid->Data4[2],
					iid->Data4[3], iid->Data4[4], iid->Data4[5], iid->Data4[6], iid->Data4[7]);
				raven_trace_note(note);
				return E_NOINTERFACE;
			}
		}
	}
	InterlockedIncrement(&self->core.refs);
	return S_OK;
}
static ULONG WINAPI dialog_operation_addref(dialog_op_i *iface)
{ return (ULONG)InterlockedIncrement(&((struct dialog_operation *)iface)->core.refs); }
static ULONG WINAPI dialog_operation_release(dialog_op_i *iface)
{
	struct dialog_operation *self = (struct dialog_operation *)iface;
	const LONG refs = InterlockedDecrement(&self->core.refs);
	if (!refs) {
		if (self->core.completed)
			dialog_handler_release(self->core.completed);
		dialog_operation_free(self);
		FREE(self);
	}
	return (ULONG)refs;
}
static HRESULT WINAPI dialog_operation_iids(dialog_op_i *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI dialog_operation_class(dialog_op_i *iface, HSTRING *out)
{ (void)iface; if (!out) return E_POINTER;
	return WindowsCreateString(dialog_operation_type_name, (UINT32)lstrlenW(dialog_operation_type_name), out); }
static HRESULT WINAPI dialog_operation_trust(dialog_op_i *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }

static struct dialog_operation *dialog_operation_owner(IAsyncInfo *info)
{ return CONTAINING_RECORD(info, struct dialog_operation, info); }
static HRESULT WINAPI dialog_operation_info_query(IAsyncInfo *info, REFIID iid, void **out)
{ return dialog_operation_query(&dialog_operation_owner(info)->iface, iid, out); }
static ULONG WINAPI dialog_operation_info_addref(IAsyncInfo *info)
{ return dialog_operation_addref(&dialog_operation_owner(info)->iface); }
static ULONG WINAPI dialog_operation_info_release(IAsyncInfo *info)
{ return dialog_operation_release(&dialog_operation_owner(info)->iface); }
static HRESULT WINAPI dialog_operation_info_iids(IAsyncInfo *info, ULONG *count, IID **iids)
{ (void)info; return inspectable_iids(count, iids); }
static HRESULT WINAPI dialog_operation_info_class(IAsyncInfo *info, HSTRING *out)
{ return dialog_operation_class(&dialog_operation_owner(info)->iface, out); }
static HRESULT WINAPI dialog_operation_info_trust(IAsyncInfo *info, TrustLevel *level)
{ return dialog_operation_trust(&dialog_operation_owner(info)->iface, level); }
static HRESULT WINAPI dialog_operation_info_id(IAsyncInfo *info, UINT32 *out)
{
	struct dialog_operation *self = dialog_operation_owner(info);
	HRESULT result;
	if (!out)
		return E_POINTER;
	EnterCriticalSection(&self->core.lock);
	result = self->core.closed ? RO_E_CLOSED : S_OK;
	*out = self->core.id;
	LeaveCriticalSection(&self->core.lock);
	return result;
}
static HRESULT WINAPI dialog_operation_info_status(IAsyncInfo *info, AsyncStatus *out)
{
	struct dialog_operation *self = dialog_operation_owner(info);
	HRESULT result;
	if (!out)
		return E_POINTER;
	EnterCriticalSection(&self->core.lock);
	result = self->core.closed ? RO_E_CLOSED : S_OK;
	*out = self->core.status;
	LeaveCriticalSection(&self->core.lock);
	return result;
}
static HRESULT WINAPI dialog_operation_info_error(IAsyncInfo *info, HRESULT *out)
{
	struct dialog_operation *self = dialog_operation_owner(info);
	HRESULT result;
	if (!out)
		return E_POINTER;
	EnterCriticalSection(&self->core.lock);
	result = self->core.closed ? RO_E_CLOSED : S_OK;
	*out = self->core.error;
	LeaveCriticalSection(&self->core.lock);
	return result;
}
static HRESULT WINAPI dialog_operation_info_cancel(IAsyncInfo *info)
{ (void)info; return S_OK; }
static HRESULT WINAPI dialog_operation_info_close(IAsyncInfo *info)
{
	struct dialog_operation *self = dialog_operation_owner(info);
	void *handler = NULL;
	HRESULT result = S_OK;
	EnterCriticalSection(&self->core.lock);
	if (self->core.status == Started) result = E_ILLEGAL_METHOD_CALL;
	else {
		self->core.closed = TRUE;
		handler = self->core.completed;
		self->core.completed = NULL;
	}
	LeaveCriticalSection(&self->core.lock);
	if (handler)
		dialog_handler_release(handler);
	return result;
}
static const IAsyncInfoVtbl dialog_operation_info_table = {
	dialog_operation_info_query, dialog_operation_info_addref, dialog_operation_info_release,
	dialog_operation_info_iids, dialog_operation_info_class, dialog_operation_info_trust,
	dialog_operation_info_id, dialog_operation_info_status, dialog_operation_info_error,
	dialog_operation_info_cancel, dialog_operation_info_close,
};

/* the operation is already terminal when it escapes ShowAsync, so put_Completed invokes the handler synchronously
 * (the storage.c/display_monitor.c convention) and get_Completed hands the stored one back */
static HRESULT WINAPI dialog_operation_put_completed(dialog_op_i *iface, dialog_done_i *handler)
{
	struct dialog_operation *self = (struct dialog_operation *)iface;
	HRESULT result = S_OK;
	AsyncStatus status;
	EnterCriticalSection(&self->core.lock);
	status = self->core.status;
	if (self->core.closed) result = RO_E_CLOSED;
	else if (self->core.assigned) result = E_ILLEGAL_DELEGATE_ASSIGNMENT;
	else {
		self->core.assigned = TRUE;
		self->core.completed = handler;
		if (handler) {
			dialog_handler_addref(handler);
			if (status != Started) dialog_handler_addref(handler);
		}
	}
	LeaveCriticalSection(&self->core.lock);
	if (SUCCEEDED(result) && handler && status != Started) {
		dialog_handler_invoke(handler, iface, status);
		dialog_handler_release(handler);
	}
	return result;
}
static HRESULT WINAPI dialog_operation_get_completed(dialog_op_i *iface, dialog_done_i **handler)
{
	struct dialog_operation *self = (struct dialog_operation *)iface;
	HRESULT result = S_OK;
	if (!handler)
		return E_POINTER;
	*handler = NULL;
	EnterCriticalSection(&self->core.lock);
	if (self->core.closed) result = RO_E_CLOSED;
	else if ((*handler = (dialog_done_i *)self->core.completed)) dialog_handler_addref(*handler);
	LeaveCriticalSection(&self->core.lock);
	return result;
}
static HRESULT WINAPI dialog_operation_results(dialog_op_i *iface, INT32 *results)
{
	struct dialog_operation *self = (struct dialog_operation *)iface;
	HRESULT result;
	if (!results)
		return E_POINTER;
	EnterCriticalSection(&self->core.lock);
	result = self->core.closed ? RO_E_CLOSED : self->core.status == Started ? E_ILLEGAL_METHOD_CALL : self->core.error;
	*results = self->core.status == Started ? 0 : self->result;
	LeaveCriticalSection(&self->core.lock);
	return result;
}
static const dialog_op_iVtbl dialog_operation_table = {
	dialog_operation_query, dialog_operation_addref, dialog_operation_release,
	dialog_operation_iids, dialog_operation_class, dialog_operation_trust,
	dialog_operation_put_completed, dialog_operation_get_completed, dialog_operation_results,
};

static struct dialog_operation *dialog_operation_new(void)
{
	struct dialog_operation *self = ALLOC(sizeof *self);
	if (!self)
		return NULL;
	self->iface.lpVtbl = &dialog_operation_table;
	self->info.lpVtbl = &dialog_operation_info_table;
	self->core.refs = 1;
	self->core.status = Started;
	self->core.error = S_OK;
	self->core.id = (UINT32)InterlockedIncrement(&next_dialog_async_id);
	self->result = contentdialog_result_none;
	InitializeCriticalSection(&self->core.lock);
	raven_weak_source_init(&self->weak, self, &self->core.refs, (raven_weak_query_fn)dialog_operation_query);
	return self;
}

/* the terminal transition: the status/error land, and a handler the guest set before the completion fires here
 * (outside the lock, the storage.c convention) */
static void dialog_operation_finish(struct dialog_operation *self, HRESULT error, AsyncStatus status)
{
	dialog_done_i *handler = NULL;
	EnterCriticalSection(&self->core.lock);
	if (self->core.status == Started) {
		self->core.status = status;
		self->core.error = error;
		handler = (dialog_done_i *)self->core.completed;
		if (handler) dialog_handler_addref(handler);
	}
	LeaveCriticalSection(&self->core.lock);
	if (handler) {
		dialog_handler_invoke(handler, &self->iface, status);
		dialog_handler_release(handler);
	}
}

static HRESULT WINAPI frame_contentdialog_hide(contentdialog_i *iface)
{ (void)iface;
	/* with no presentation path the dialog is already not visible: Hide is the no-op Windows' own gives on a
	 * dialog that is not shown, and the app may call it between ShowAsync and the await's continuation */
	raven_trace_note("raven-xaml: ContentDialog Hide answered S_OK (no presentation path)\n");
	return S_OK; }
static HRESULT WINAPI frame_contentdialog_show_async(contentdialog_i *iface, IInspectable **operation)
{
	struct dialog_operation *op;
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	if (!(op = dialog_operation_new()))
		return E_OUTOFMEMORY;
	dialog_operation_finish(op, S_OK, Completed);
	*operation = (IInspectable *)&op->iface;
	raven_trace_note("raven-xaml: ContentDialog.ShowAsync -> completed None (no presentation path)\n");
	return S_OK;
}

static const contentdialog_vtbl frame_contentdialog_table = {
	frame_contentdialog_query, frame_contentdialog_addref, frame_contentdialog_release,
	frame_contentdialog_iids, frame_contentdialog_name, frame_contentdialog_trust,
	frame_contentdialog_get_Title, frame_contentdialog_put_Title,
	frame_contentdialog_get_TitleTemplate, frame_contentdialog_put_TitleTemplate,
	frame_contentdialog_get_FullSizeDesired, frame_contentdialog_put_FullSizeDesired,
	frame_contentdialog_get_PrimaryButtonText, frame_contentdialog_put_PrimaryButtonText,
	frame_contentdialog_get_SecondaryButtonText, frame_contentdialog_put_SecondaryButtonText,
	frame_contentdialog_get_PrimaryButtonCommand, frame_contentdialog_put_PrimaryButtonCommand,
	frame_contentdialog_get_SecondaryButtonCommand, frame_contentdialog_put_SecondaryButtonCommand,
	frame_contentdialog_get_PrimaryButtonCommandParameter, frame_contentdialog_put_PrimaryButtonCommandParameter,
	frame_contentdialog_get_SecondaryButtonCommandParameter, frame_contentdialog_put_SecondaryButtonCommandParameter,
	frame_contentdialog_get_IsPrimaryButtonEnabled, frame_contentdialog_put_IsPrimaryButtonEnabled,
	frame_contentdialog_get_IsSecondaryButtonEnabled, frame_contentdialog_put_IsSecondaryButtonEnabled,
	frame_contentdialog_add_Closing, frame_contentdialog_remove_Closing,
	frame_contentdialog_add_Closed, frame_contentdialog_remove_Closed,
	frame_contentdialog_add_Opened, frame_contentdialog_remove_Opened,
	frame_contentdialog_add_PrimaryButtonClick, frame_contentdialog_remove_PrimaryButtonClick,
	frame_contentdialog_add_SecondaryButtonClick, frame_contentdialog_remove_SecondaryButtonClick,
	frame_contentdialog_hide, frame_contentdialog_show_async,
};

static HRESULT WINAPI frame_contentdialog2_query(contentdialog2_i *iface, REFIID iid, void **out)
{
	struct frame *self = ((struct frame_contentdialog2_part *)iface)->owner;
	return self->outer ? IInspectable_QueryInterface(self->outer, iid, out) : frame_query_any(self, iid, out);
}
static ULONG WINAPI frame_contentdialog2_addref(contentdialog2_i *iface)
{ return ((struct frame_contentdialog2_part *)iface)->owner->outer ?
	IInspectable_AddRef(((struct frame_contentdialog2_part *)iface)->owner->outer) :
	frame_addref(((struct frame_contentdialog2_part *)iface)->owner); }
static ULONG WINAPI frame_contentdialog2_release(contentdialog2_i *iface)
{ return ((struct frame_contentdialog2_part *)iface)->owner->outer ?
	IInspectable_Release(((struct frame_contentdialog2_part *)iface)->owner->outer) :
	frame_release(((struct frame_contentdialog2_part *)iface)->owner); }
static HRESULT WINAPI frame_contentdialog2_iids(contentdialog2_i *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI frame_contentdialog2_name(contentdialog2_i *iface, HSTRING *name)
{ (void)iface; if (!name) return E_POINTER;
	return WindowsCreateString(contentdialog_class, (UINT32)lstrlenW(contentdialog_class), name); }
static HRESULT WINAPI frame_contentdialog2_trust(contentdialog2_i *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static HRESULT WINAPI frame_contentdialog2_get_CloseButtonText(contentdialog2_i *iface, HSTRING *out)
{ return frame_contentdialog_get_text((contentdialog_i *)iface, out, ((struct frame_contentdialog2_part *)iface)->owner->dialog_close_text); }
static HRESULT WINAPI frame_contentdialog2_put_CloseButtonText(contentdialog2_i *iface, HSTRING value)
{ return frame_contentdialog_put_text((contentdialog_i *)iface, &((struct frame_contentdialog2_part *)iface)->owner->dialog_close_text, value); }
static HRESULT WINAPI frame_contentdialog2_get_CloseButtonCommand(contentdialog2_i *iface, IInspectable **out)
{ return frame_contentdialog_get_object((contentdialog_i *)iface, out, ((struct frame_contentdialog2_part *)iface)->owner->dialog_close_command); }
static HRESULT WINAPI frame_contentdialog2_put_CloseButtonCommand(contentdialog2_i *iface, IInspectable *value)
{ return frame_contentdialog_put_object((contentdialog_i *)iface, &((struct frame_contentdialog2_part *)iface)->owner->dialog_close_command, value); }
static HRESULT WINAPI frame_contentdialog2_get_CloseButtonCommandParameter(contentdialog2_i *iface, IInspectable **out)
{ return frame_contentdialog_get_object((contentdialog_i *)iface, out, ((struct frame_contentdialog2_part *)iface)->owner->dialog_close_command_parameter); }
static HRESULT WINAPI frame_contentdialog2_put_CloseButtonCommandParameter(contentdialog2_i *iface, IInspectable *value)
{ return frame_contentdialog_put_object((contentdialog_i *)iface, &((struct frame_contentdialog2_part *)iface)->owner->dialog_close_command_parameter, value); }
static HRESULT WINAPI frame_contentdialog2_get_PrimaryButtonStyle(contentdialog2_i *iface, IInspectable **out)
{ return frame_contentdialog_get_object((contentdialog_i *)iface, out, ((struct frame_contentdialog2_part *)iface)->owner->dialog_primary_style); }
static HRESULT WINAPI frame_contentdialog2_put_PrimaryButtonStyle(contentdialog2_i *iface, IInspectable *value)
{ return frame_contentdialog_put_object((contentdialog_i *)iface, &((struct frame_contentdialog2_part *)iface)->owner->dialog_primary_style, value); }
static HRESULT WINAPI frame_contentdialog2_get_SecondaryButtonStyle(contentdialog2_i *iface, IInspectable **out)
{ return frame_contentdialog_get_object((contentdialog_i *)iface, out, ((struct frame_contentdialog2_part *)iface)->owner->dialog_secondary_style); }
static HRESULT WINAPI frame_contentdialog2_put_SecondaryButtonStyle(contentdialog2_i *iface, IInspectable *value)
{ return frame_contentdialog_put_object((contentdialog_i *)iface, &((struct frame_contentdialog2_part *)iface)->owner->dialog_secondary_style, value); }
static HRESULT WINAPI frame_contentdialog2_get_CloseButtonStyle(contentdialog2_i *iface, IInspectable **out)
{ return frame_contentdialog_get_object((contentdialog_i *)iface, out, ((struct frame_contentdialog2_part *)iface)->owner->dialog_close_style); }
static HRESULT WINAPI frame_contentdialog2_put_CloseButtonStyle(contentdialog2_i *iface, IInspectable *value)
{ return frame_contentdialog_put_object((contentdialog_i *)iface, &((struct frame_contentdialog2_part *)iface)->owner->dialog_close_style, value); }
static HRESULT WINAPI frame_contentdialog2_get_DefaultButton(contentdialog2_i *iface, INT32 *out)
{ (void)iface; if (!out) return E_POINTER; *out = (INT32)((struct frame_contentdialog2_part *)iface)->owner->dialog_default_button; return S_OK; }
static HRESULT WINAPI frame_contentdialog2_put_DefaultButton(contentdialog2_i *iface, INT32 value)
{ (void)iface; ((struct frame_contentdialog2_part *)iface)->owner->dialog_default_button = (uint64_t)value; return S_OK; }
static HRESULT WINAPI frame_contentdialog2_add_CloseButtonClick(contentdialog2_i *iface, void *handler, EventRegistrationToken *token)
{ return raven_event_add(&((struct frame_contentdialog2_part *)iface)->owner->dialog_close_click, (IUnknown *)handler, token); }
static HRESULT WINAPI frame_contentdialog2_remove_CloseButtonClick(contentdialog2_i *iface, EventRegistrationToken token)
{ return raven_event_remove(&((struct frame_contentdialog2_part *)iface)->owner->dialog_close_click, token); }

static const contentdialog2_vtbl frame_contentdialog2_table = {
	frame_contentdialog2_query, frame_contentdialog2_addref, frame_contentdialog2_release,
	frame_contentdialog2_iids, frame_contentdialog2_name, frame_contentdialog2_trust,
	frame_contentdialog2_get_CloseButtonText, frame_contentdialog2_put_CloseButtonText,
	frame_contentdialog2_get_CloseButtonCommand, frame_contentdialog2_put_CloseButtonCommand,
	frame_contentdialog2_get_CloseButtonCommandParameter, frame_contentdialog2_put_CloseButtonCommandParameter,
	frame_contentdialog2_get_PrimaryButtonStyle, frame_contentdialog2_put_PrimaryButtonStyle,
	frame_contentdialog2_get_SecondaryButtonStyle, frame_contentdialog2_put_SecondaryButtonStyle,
	frame_contentdialog2_get_CloseButtonStyle, frame_contentdialog2_put_CloseButtonStyle,
	frame_contentdialog2_get_DefaultButton, frame_contentdialog2_put_DefaultButton,
	frame_contentdialog2_add_CloseButtonClick, frame_contentdialog2_remove_CloseButtonClick,
};

/* IPage and IPageOverrides of the standalone Page the page factory composes (see page_create): the
 * projection's cast walk needs the whole FrameworkElement family the frame serves, so the page rides the
 * frame machinery and keeps only the page-specific surface here. Frame answers null until a navigation
 * takes the page (the real runtime binds it when the frame hands the page over); the cache mode and the
 * app bars store; the overrides accept the lifecycle calls — the CCW's own IPageOverrides implementation
 * serves the managed OnNavigatedTo when the class overrides it, and the framework raises the lifecycle
 * through exactly that face (frame_navigated_deliver). */
static HRESULT WINAPI frame_page_get_frame(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *iface, uint64_t out)
{
	(void)iface;
	if (!out) return E_POINTER;
	*(IInspectable **)(uintptr_t)out = NULL;
	return S_OK;
}
static HRESULT WINAPI frame_page_get_cache_mode(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *iface, uint64_t out)
{
	if (!out) return E_POINTER;
	*(int32_t *)(uintptr_t)out = ((struct frame_page_part *)iface)->owner->navigation_cache_mode;
	return S_OK;
}
static HRESULT WINAPI frame_page_put_cache_mode(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *iface, uint64_t value)
{
	((struct frame_page_part *)iface)->owner->navigation_cache_mode = (int32_t)value;
	return S_OK;
}
static HRESULT WINAPI frame_page_get_top_app_bar(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *iface, uint64_t out)
{
	if (!out) return E_POINTER;
	IInspectable *stored = ((struct frame_page_part *)iface)->owner->page_top_app_bar;
	*(IInspectable **)(uintptr_t)out = stored;
	if (stored) IInspectable_AddRef(stored);
	return S_OK;
}
static HRESULT WINAPI frame_page_put_top_app_bar(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *iface, uint64_t value)
{
	struct frame *self = ((struct frame_page_part *)iface)->owner;
	IInspectable *incoming = (IInspectable *)(uintptr_t)value;
	if (incoming) IInspectable_AddRef(incoming);
	if (self->page_top_app_bar) IInspectable_Release(self->page_top_app_bar);
	self->page_top_app_bar = incoming;
	return S_OK;
}
static HRESULT WINAPI frame_page_get_bottom_app_bar(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *iface, uint64_t out)
{
	if (!out) return E_POINTER;
	IInspectable *stored = ((struct frame_page_part *)iface)->owner->page_bottom_app_bar;
	*(IInspectable **)(uintptr_t)out = stored;
	if (stored) IInspectable_AddRef(stored);
	return S_OK;
}
static HRESULT WINAPI frame_page_put_bottom_app_bar(__x_ABI_CWindows_CUI_CXaml_CControls_CIPage *iface, uint64_t value)
{
	struct frame *self = ((struct frame_page_part *)iface)->owner;
	IInspectable *incoming = (IInspectable *)(uintptr_t)value;
	if (incoming) IInspectable_AddRef(incoming);
	if (self->page_bottom_app_bar) IInspectable_Release(self->page_bottom_app_bar);
	self->page_bottom_app_bar = incoming;
	return S_OK;
}
static HRESULT WINAPI frame_overrides_navigated_from(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *iface, uint64_t args)
{ (void)iface; (void)args; return S_OK; }
static HRESULT WINAPI frame_overrides_navigated_to(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *iface, uint64_t args)
{ (void)iface; (void)args; return S_OK; }
static HRESULT WINAPI frame_overrides_navigating_from(__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides *iface, uint64_t args)
{ (void)iface; (void)args; return S_OK; }
static const __x_ABI_CWindows_CUI_CXaml_CControls_CIPageVtbl frame_page_table = {
	frame_page_query, frame_page_addref, frame_page_release,
	frame_page_iids, frame_page_name, frame_page_trust,
	frame_page_get_frame, frame_page_get_cache_mode, frame_page_put_cache_mode,
	frame_page_get_top_app_bar, frame_page_put_top_app_bar,
	frame_page_get_bottom_app_bar, frame_page_put_bottom_app_bar,
};
static const __x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverridesVtbl frame_page_overrides_table = {
	frame_page_overrides_query, frame_page_overrides_addref, frame_page_overrides_release,
	frame_page_overrides_iids, frame_page_overrides_name, frame_page_overrides_trust,
	frame_overrides_navigated_from, frame_overrides_navigated_to, frame_overrides_navigating_from,
};

/* The IFrameworkElement2/3/4 faces: each version's members answer over the same frame storage, the way the
 * real runtime's one object answers every version. RequestedTheme stores; the DataContextChanged and
 * Loading subscriptions are retained through the framework event list and never raised yet;
 * GetBindingExpression answers the no-binding null; the focus-visual fields store with the metadata
 * defaults set at construction. */
/* The version faces reuse the frame_framework IInspectable head; the generated vtbl types want their own
 * pointer types, so each table entry goes through a cast wrapper. */
static HRESULT WINAPI frame_fe2_query(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This, REFIID iid, void **out)
{ return frame_framework_query((IInspectable *)This, iid, out); }
static ULONG WINAPI frame_fe2_addref(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This)
{ return frame_framework_addref((IInspectable *)This); }
static ULONG WINAPI frame_fe2_release(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This)
{ return frame_framework_release((IInspectable *)This); }
static HRESULT WINAPI frame_fe2_get_requested_theme(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *iface, uint64_t out)
{
	(void)iface;
	if (!out) return E_POINTER;
	*(INT32 *)(uintptr_t)out = ((struct frame_fe2_part *)iface)->owner->requested_theme;
	return S_OK;
}
static HRESULT WINAPI frame_fe2_put_requested_theme(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *iface, uint64_t value)
{
	((struct frame_fe2_part *)iface)->owner->requested_theme = (INT32)value;
	return S_OK;
}
static HRESULT WINAPI frame_fe2_add_data_context_changed(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *iface, uint64_t handler, uint64_t token)
{ return framework_event_add((IInspectable *)iface, (IUnknown *)(uintptr_t)handler, (INT64 *)(uintptr_t)token, 4); }
static HRESULT WINAPI frame_fe2_remove_data_context_changed(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *iface, uint64_t token)
{ return framework_event_remove((IInspectable *)iface, (INT64)token, 4); }
static HRESULT WINAPI frame_fe2_get_binding_expression(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *iface, uint64_t dp, uint64_t out)
{
	(void)iface; (void)dp;
	if (!out) return E_POINTER;
	*(IInspectable **)(uintptr_t)out = NULL; /* no binding machinery: the runtime answers null then too */
	return S_OK;
}
static HRESULT WINAPI frame_fe3_query(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *This, REFIID iid, void **out)
{ return frame_framework_query((IInspectable *)This, iid, out); }
static ULONG WINAPI frame_fe3_addref(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *This)
{ return frame_framework_addref((IInspectable *)This); }
static ULONG WINAPI frame_fe3_release(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *This)
{ return frame_framework_release((IInspectable *)This); }
static HRESULT WINAPI frame_fe3_add_loading(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *iface, uint64_t handler, uint64_t token)
{ return framework_event_add((IInspectable *)iface, (IUnknown *)(uintptr_t)handler, (INT64 *)(uintptr_t)token, 5); }
static HRESULT WINAPI frame_fe3_remove_loading(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *iface, uint64_t token)
{ return framework_event_remove((IInspectable *)iface, (INT64)token, 5); }
#define FRAME_FE4_BOOL_PROPERTY(name, field) \
	static HRESULT WINAPI frame_fe4_get_##name(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *iface, uint64_t out) \
	{ (void)iface; if (!out) return E_POINTER; *(INT32 *)(uintptr_t)out = ((struct frame_fe4_part *)iface)->owner->field; return S_OK; } \
	static HRESULT WINAPI frame_fe4_put_##name(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *iface, uint64_t value) \
	{ ((struct frame_fe4_part *)iface)->owner->field = (INT32)value; return S_OK; }
FRAME_FE4_BOOL_PROPERTY(allow_focus_on_interaction, allow_focus_on_interaction)
FRAME_FE4_BOOL_PROPERTY(allow_focus_when_disabled, allow_focus_when_disabled)
#undef FRAME_FE4_BOOL_PROPERTY
#define FRAME_FE4_THICKNESS_PROPERTY(name, field) \
	static HRESULT WINAPI frame_fe4_get_##name(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *iface, uint64_t out) \
	{ if (!out) return E_POINTER; *(struct framework_thickness *)(uintptr_t)out = ((struct frame_fe4_part *)iface)->owner->field; return S_OK; } \
	static HRESULT WINAPI frame_fe4_put_##name(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *iface, uint64_t value) \
	{ ((struct frame_fe4_part *)iface)->owner->field = *(const struct framework_thickness *)(uintptr_t)value; return S_OK; }
FRAME_FE4_THICKNESS_PROPERTY(focus_visual_margin, focus_visual_margin)
FRAME_FE4_THICKNESS_PROPERTY(focus_visual_secondary_thickness, focus_visual_secondary_thickness)
FRAME_FE4_THICKNESS_PROPERTY(focus_visual_primary_thickness, focus_visual_primary_thickness)
#undef FRAME_FE4_THICKNESS_PROPERTY
#define FRAME_FE4_BRUSH_PROPERTY(name, field) \
	static HRESULT WINAPI frame_fe4_get_##name(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *iface, uint64_t out) \
	{ (void)iface; if (!out) return E_POINTER; IInspectable *stored = ((struct frame_fe4_part *)iface)->owner->field; \
		*(IInspectable **)(uintptr_t)out = stored; if (stored) IInspectable_AddRef(stored); return S_OK; } \
	static HRESULT WINAPI frame_fe4_put_##name(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *iface, uint64_t value) \
	{ \
		struct frame *self = ((struct frame_fe4_part *)iface)->owner; \
		IInspectable *incoming = (IInspectable *)(uintptr_t)value; \
		if (incoming) IInspectable_AddRef(incoming); \
		if (self->field) IInspectable_Release(self->field); \
		self->field = incoming; return S_OK; \
	}
FRAME_FE4_BRUSH_PROPERTY(focus_visual_secondary_brush, focus_visual_secondary_brush)
FRAME_FE4_BRUSH_PROPERTY(focus_visual_primary_brush, focus_visual_primary_brush)
#undef FRAME_FE4_BRUSH_PROPERTY
static HRESULT WINAPI frame_fe2_iids(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This, ULONG *count, IID **iids)
{ return frame_framework_iids((IInspectable *)This, count, iids); }
static HRESULT WINAPI frame_fe2_name(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This, HSTRING *name)
{ return frame_framework_name((IInspectable *)This, name); }
static HRESULT WINAPI frame_fe2_trust(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 *This, TrustLevel *level)
{ return frame_framework_trust((IInspectable *)This, level); }
static const __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2Vtbl frame_fe2_table = {
	frame_fe2_query, frame_fe2_addref, frame_fe2_release,
	frame_fe2_iids, frame_fe2_name, frame_fe2_trust,
	frame_fe2_get_requested_theme, frame_fe2_put_requested_theme,
	frame_fe2_add_data_context_changed, frame_fe2_remove_data_context_changed,
	frame_fe2_get_binding_expression,
};
static HRESULT WINAPI frame_fe3_iids(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *This, ULONG *count, IID **iids)
{ return frame_framework_iids((IInspectable *)This, count, iids); }
static HRESULT WINAPI frame_fe3_name(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *This, HSTRING *name)
{ return frame_framework_name((IInspectable *)This, name); }
static HRESULT WINAPI frame_fe3_trust(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 *This, TrustLevel *level)
{ return frame_framework_trust((IInspectable *)This, level); }
static const __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3Vtbl frame_fe3_table = {
	frame_fe3_query, frame_fe3_addref, frame_fe3_release,
	frame_fe3_iids, frame_fe3_name, frame_fe3_trust,
	frame_fe3_add_loading, frame_fe3_remove_loading,
};
static HRESULT WINAPI frame_fe4_query(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, REFIID iid, void **out)
{ return frame_framework_query((IInspectable *)This, iid, out); }
static ULONG WINAPI frame_fe4_addref(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This)
{ return frame_framework_addref((IInspectable *)This); }
static ULONG WINAPI frame_fe4_release(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This)
{ return frame_framework_release((IInspectable *)This); }
static HRESULT WINAPI frame_fe4_iids(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, ULONG *count, IID **iids)
{ return frame_framework_iids((IInspectable *)This, count, iids); }
static HRESULT WINAPI frame_fe4_name(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, HSTRING *name)
{ return frame_framework_name((IInspectable *)This, name); }
static HRESULT WINAPI frame_fe4_trust(__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 *This, TrustLevel *level)
{ return frame_framework_trust((IInspectable *)This, level); }
static const __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4Vtbl frame_fe4_table = {
	frame_fe4_query, frame_fe4_addref, frame_fe4_release,
	frame_fe4_iids, frame_fe4_name, frame_fe4_trust,
	frame_fe4_get_allow_focus_on_interaction, frame_fe4_put_allow_focus_on_interaction,
	frame_fe4_get_focus_visual_margin, frame_fe4_put_focus_visual_margin,
	frame_fe4_get_focus_visual_secondary_thickness, frame_fe4_put_focus_visual_secondary_thickness,
	frame_fe4_get_focus_visual_primary_thickness, frame_fe4_put_focus_visual_primary_thickness,
	frame_fe4_get_focus_visual_secondary_brush, frame_fe4_put_focus_visual_secondary_brush,
	frame_fe4_get_focus_visual_primary_brush, frame_fe4_put_focus_visual_primary_brush,
	frame_fe4_get_allow_focus_when_disabled, frame_fe4_put_allow_focus_when_disabled,
};

static HRESULT frame_query_any(struct frame *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (self->dependency_only && !equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID_IAgileObject) && !equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CIDependencyObject))
		return E_NOINTERFACE;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable)) {
		*out = self->outer ? (void *)&self->inner : self->dependency_only ? (void *)&self->dependency : (void *)&self->first;
		frame_addref(self);
		return S_OK;
	} else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame))
		*out = &self->first;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2))
		*out = &self->second;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3))
		*out = &self->third;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4))
		*out = &self->fourth;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5))
		*out = &self->fifth;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIUIElement))
		*out = &self->ui;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl))
		*out = &self->content;
	else if (equal_iid(iid, &IID_frame_framework))
		*out = &self->framework;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2))
		*out = &self->fe2;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3))
		*out = &self->fe3;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4))
		*out = &self->fe4;
	else if (equal_iid(iid, &IID_frame_theme))
		*out = &self->theme;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControl)) {
		/* The ctor's ComCall path queries this face before its delegate marshal; the throw happens
		 * inside the marshal, before the slot pair would be entered. */
		*out = &self->control;
	}
	else if (self->standalone_usercontrol && equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CIUserControl))
		*out = &self->usercontrol;
	else if (self->standalone_contentdialog && equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialog))
		*out = &self->contentdialog;
	else if (self->standalone_contentdialog && equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialog2))
		*out = &self->contentdialog2;
	else if (self->standalone_page && equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPage))
		*out = &self->page_face;
	else if (self->standalone_page && equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides))
		*out = &self->page_overrides;
	else if (equal_iid(iid, &IID_IAgileObject)) {
		/* The object runs apartment-free in the engine's single process, the way the runtime's agility
		 * negotiation expects of a XAML object. The handed pointer must release against whatever was
		 * AddRef'd: the delegating first part would land its Release on the outer, whose count never
		 * gained the reference — the traced Shapr3D run loses one outer reference per agility ask and
		 * its wrapper machinery tears a wrapper down twice — so the aggregated frame answers with the
		 * non-delegating inner part, which releases against the frame like the AddRef does. */
		*out = self->outer ? (void *)&self->inner : self->dependency_only ? (void *)&self->dependency : (void *)&self->first;
		frame_addref(self);
		return S_OK;
	} else if (equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CIDependencyObject)) {
		*out = &self->dependency;
		if (self->outer) IInspectable_AddRef(self->outer); else frame_addref(self);
		return S_OK;
	} else if (equal_iid(iid, &IID_frame_peer_bridge)) {
		/* The WinUI 2.8 DXAML peer bridge: Microsoft.UI.Xaml.dll stores the frame's inner face as its
		 * CoreServices peer and queries it for this private interface without checking the result
		 * (microsoft.ui.xaml.dll+1695ae in the traced run) — a refusal is the null interface call. */
		*out = &self->peer_bridge;
	} else if (equal_iid(iid, &IID_frame_peer_shim)) {
		/* The second peer face of the same family: fetched by generated accessors (+382754/+1f1084) whose
		 * QI result is likewise dereferenced unchecked. */
		*out = &self->peer_shim;
	} else if (equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker)) {
		/* The tracker face most XAML framework objects carry; .NET Native's SharedLibrary asks for it when
		 * the object is handed to managed code and coordinates wrapper lifetimes through it. */
		*out = &self->tracker;
		frame_addref(self);
		return S_OK;
	} else if (equal_iid(iid, &IID___FIMap_2_IInspectable_IInspectable)) {
		/* The dictionary-typed access the .NET Native CCW resolves against the element: the traced
		 * TitleBarControl activation probes the object for IMap<Object,Object> and derefs the refused
		 * result unchecked (the c0000005 unboxing a keyed lookup's return at shapr3d.dll+282c07f). A XAML
		 * object's dictionary face is its FrameworkElement.Resources, so the frame keeps one dictionary
		 * (created on first ask, released with the frame) and answers with the map face the engine's
		 * ResourceDictionary serves; Lookup/HasKey/Insert land in the element's real resources. */
		if (!self->resources) {
			IInspectable *created = NULL;
			const HRESULT made = raven_xaml_create_stable(371, &created);
			if (FAILED(made) || !created)
				return E_NOINTERFACE;
			self->resources = created;
		}
		IInspectable_AddRef(self->resources);
		return IInspectable_QueryInterface(self->resources, iid, out);
	} else {
		char line[256];
		wsprintfA(line, "raven-xaml: frame has no interface %08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
			(unsigned long)iid->Data1, iid->Data2, iid->Data3, iid->Data4[0], iid->Data4[1], iid->Data4[2],
			iid->Data4[3], iid->Data4[4], iid->Data4[5], iid->Data4[6], iid->Data4[7]);
		OutputDebugStringA(line);
				return E_NOINTERFACE;
	}
	if (self->outer) IInspectable_AddRef(self->outer); else frame_addref(self);
	return S_OK;
}

static HRESULT WINAPI frame_inner_query(IInspectable *iface, REFIID iid, void **out) { return frame_query_any(((struct frame_inner_part *)iface)->owner,iid,out); }
static ULONG WINAPI frame_inner_addref(IInspectable *iface) { return frame_addref(((struct frame_inner_part *)iface)->owner); }
static ULONG WINAPI frame_inner_release(IInspectable *iface) { return frame_release(((struct frame_inner_part *)iface)->owner); }
static HRESULT WINAPI frame_inner_iids(IInspectable *iface,ULONG *count,IID **out) { (void)iface; return inspectable_iids(count,out); }
static HRESULT WINAPI frame_inner_name(IInspectable *iface,HSTRING *out) {
	struct frame *self = ((struct frame_inner_part *)iface)->owner;
	const WCHAR *text = self->standalone_page ? page_class :
		self->standalone_usercontrol ? usercontrol_class :
		self->standalone_contentdialog ? contentdialog_class :
		self->standalone_control ? control_class :
		self->dependency_only ? L"Windows.UI.Xaml.DependencyObject" : frame_class;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), out);
}
static HRESULT WINAPI frame_inner_trust(IInspectable *iface,TrustLevel *out) { (void)iface; return inspectable_trust(out); }
static const IInspectableVtbl frame_inner_table={frame_inner_query,frame_inner_addref,frame_inner_release,frame_inner_iids,frame_inner_name,frame_inner_trust};

/* The tracker part forwards its IUnknown to the frame through this adapter, the shape the weak source takes. */
static HRESULT WINAPI frame_owner_query(void *owner, REFIID iid, void **out)
{
	return frame_query_any((struct frame *)owner, iid, out);
}

static HRESULT frame_ui_query(void *owner, REFIID iid, void **out)
{
 struct frame *self = owner;
 return self->outer ? IInspectable_QueryInterface(self->outer, iid, out) : frame_query_any(self, iid, out);
}
static ULONG frame_ui_addref(void *owner)
{ struct frame *self = owner; return self->outer ? IInspectable_AddRef(self->outer) : frame_addref(self); }
static ULONG frame_ui_release(void *owner)
{ struct frame *self = owner; return self->outer ? IInspectable_Release(self->outer) : frame_release(self); }

/* ---- IDependencyObject ----
 *
 * The base XAML interface the managed interop asks for before it drives the object's properties (the
 * traced Shapr3D run asks it of the Frame right after activation, names the frame RootFrame, and dies on
 * a null reference when the ask is refused). Local values are owned by the Frame and registered metadata supplies
 * defaults and change notifications. The dispatcher is the view's own. */
static HRESULT WINAPI frame_dependency_query(__x_ABI_CWindows_CUI_CXaml_CIDependencyObject *iface, REFIID iid, void **out)
{
	struct frame *self = ((struct frame_dependency_part *)iface)->owner;
	return self->outer ? IInspectable_QueryInterface(self->outer, iid, out) : frame_query_any(self, iid, out);
}

static ULONG WINAPI frame_dependency_addref(__x_ABI_CWindows_CUI_CXaml_CIDependencyObject *iface)
{
	struct frame *self = ((struct frame_dependency_part *)iface)->owner;
	return self->outer ? IInspectable_AddRef(self->outer) : frame_addref(self);
}

static ULONG WINAPI frame_dependency_release(__x_ABI_CWindows_CUI_CXaml_CIDependencyObject *iface)
{
	struct frame *self = ((struct frame_dependency_part *)iface)->owner;
	return self->outer ? IInspectable_Release(self->outer) : frame_release(self);
}

static HRESULT WINAPI frame_dependency_iids(__x_ABI_CWindows_CUI_CXaml_CIDependencyObject *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI frame_dependency_name(__x_ABI_CWindows_CUI_CXaml_CIDependencyObject *iface, HSTRING *name)
{
	struct frame *self = ((struct frame_dependency_part *)iface)->owner;
	if (!name)
		return E_POINTER;
	const WCHAR *text = self->dependency_only ? L"Windows.UI.Xaml.DependencyObject" : frame_class;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), name);
}

static HRESULT WINAPI frame_dependency_trust(__x_ABI_CWindows_CUI_CXaml_CIDependencyObject *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI frame_dependency_get_value(__x_ABI_CWindows_CUI_CXaml_CIDependencyObject *iface,
	__x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *dp, IInspectable **result)
{
	struct frame *self = ((struct frame_dependency_part *)iface)->owner;
	raven_dependency_property_interop *interop = NULL;
	if (!result) return E_POINTER;
	*result = NULL;
	if (!dp) { RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
	for (struct frame_property *entry = self->properties; entry; entry = entry->next)
		if (entry->property == dp) {
			*result = entry->value;
			if (*result) (*result)->lpVtbl->AddRef(*result);
			return S_OK;
		}
	/* Which DP the caller reads through GetValue and which kind of answer it gets — the contract the
	 * generated ctors ride. RAVEN_TRACE_FRAME_DP=1 in the guest enables it. */
	static int dp_trace_gate = -1;
	if (dp_trace_gate < 0) {
		char flag[8] = "";
		dp_trace_gate = GetEnvironmentVariableA("RAVEN_TRACE_FRAME_DP", flag, sizeof flag) ? 1 : 0;
	}
	if (dp_trace_gate) {
		char *dp_name = NULL;
		raven_dependency_property_interop *name_interop = NULL;
		if (SUCCEEDED(dp->lpVtbl->QueryInterface(dp, &raven_dependency_property_interop_iid, (void **)&name_interop))) {
			HSTRING text = NULL;
			if (SUCCEEDED(name_interop->lpVtbl->get_Name(name_interop, &text)) && text) {
				UINT32 len = 0;
				const WCHAR *chars = WindowsGetStringRawBuffer(text, &len);
				UINT32 bytes = (len + 1) * (UINT32)sizeof(*chars);
				dp_name = (char *)HeapAlloc(GetProcessHeap(), 0, bytes);
				if (dp_name)
					WideCharToMultiByte(CP_ACP, 0, chars, (int)len, dp_name, (int)bytes, NULL, NULL);
				WindowsDeleteString(text);
			}
			name_interop->lpVtbl->Release(name_interop);
		}
		char line[192];
		wsprintfA(line, "raven-frame frame-dp: GetValue receiver=d%u s%u dp=%s\n",
			self->dependency_only, self->standalone_control, dp_name ? dp_name : "?");
		OutputDebugStringA(line);
		if (dp_name)
			HeapFree(GetProcessHeap(), 0, dp_name);
	}
	HRESULT status = dp->lpVtbl->QueryInterface(dp, &raven_dependency_property_interop_iid, (void **)&interop);
	if (FAILED(status)) return status;
	status = interop->lpVtbl->get_DefaultValue(interop, result);
	if (dp_trace_gate) {
		char line[128];
		wsprintfA(line, "raven-frame frame-dp: default=%p kind=%s\n", (void *)*result,
			*result ? "metadata" : "none");
		OutputDebugStringA(line);
	}
	/* Read the registered property type before the interop is released: a metadata-less registration whose
	 * type is CoreDispatcher answers the view's own dispatcher as its default (the traced ThemedImage ctor
	 * registers a "Dispatcher" DP of type Windows.UI.Core.CoreDispatcher and its marshaler builds the
	 * managed wrapper of the read value — an UnsetValue marker handed to a dispatcher-typed read is the
	 * wrapper-build failure that throws). */
	xaml_type_name registered;
	registered.Kind = 0;
	registered.Name = NULL;
	if (SUCCEEDED(status) && !*result)
		interop->lpVtbl->get_TypeName(interop, &registered);
	interop->lpVtbl->Release(interop);
	if (FAILED(status)) return status;
	if (!*result) {
		UINT32 type_length = 0;
		const WCHAR *type_text = registered.Name ? WindowsGetStringRawBuffer(registered.Name, &type_length) : NULL;
		const WCHAR *dispatcher_type = L"Windows.UI.Core.CoreDispatcher"; /* 30 chars */
		if (type_text && type_length == 30 &&
			CompareStringOrdinal(type_text, (int)type_length, dispatcher_type, 30, FALSE) == CSTR_EQUAL) {
			ICoreDispatcher *dispatcher = NULL;
			__x_ABI_CWindows_CUI_CCore_CICoreWindow *core = NULL;
			HRESULT dispatcher_status = raven_current_window(&core);
			if (SUCCEEDED(dispatcher_status) && core) {
				dispatcher_status = core->lpVtbl->get_Dispatcher(core, &dispatcher);
				core->lpVtbl->Release(core);
			}
			WindowsDeleteString(registered.Name);
			if (SUCCEEDED(dispatcher_status) && dispatcher) {
				*result = (IInspectable *)dispatcher;
				return S_OK;
			}
			return dispatcher_status;
		}
		if (registered.Name)
			WindowsDeleteString(registered.Name);
		/* A metadata-less registration of any other type carries no default object; the real runtime answers
		 * DependencyProperty.UnsetValue (GetValue never hands out a null reference, and the application's
		 * generated getters unbox the read directly — the traced TitleBarControl c0000005). */
		return raven_xaml_dependency_unset_value(result);
	}
	return S_OK;
}

static HRESULT WINAPI frame_dependency_put_value(__x_ABI_CWindows_CUI_CXaml_CIDependencyObject *iface,
	__x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *dp, IInspectable *value)
{
	struct frame *self = ((struct frame_dependency_part *)iface)->owner;
	struct frame_property *entry;
	IInspectable *old = NULL;
	if (!dp) { RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
	HRESULT status = frame_dependency_get_value(iface, dp, &old);
	if (FAILED(status)) return status;
	for (entry = self->properties; entry && entry->property != dp; entry = entry->next) {}
	if (!entry) {
		entry = ALLOC(sizeof *entry);
		if (!entry) { if (old) old->lpVtbl->Release(old); return E_OUTOFMEMORY; }
		dp->lpVtbl->AddRef(dp); entry->property = dp; entry->next = self->properties; self->properties = entry;
	}
	if (value) value->lpVtbl->AddRef(value);
	if (entry->value) entry->value->lpVtbl->Release(entry->value);
	entry->value = value;
	iface->lpVtbl->AddRef(iface);
	/* shortcut: reference equality can notify twice for equal boxed values; compare values when boxing semantics are shared. */
	status = old == value ? S_OK : raven_dependency_notify(dp, iface, old, value);
	if (old) old->lpVtbl->Release(old);
	iface->lpVtbl->Release(iface);
	return status;
}

static HRESULT WINAPI frame_dependency_clear_value(__x_ABI_CWindows_CUI_CXaml_CIDependencyObject *iface,
	__x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *dp)
{
	struct frame *self = ((struct frame_dependency_part *)iface)->owner;
	struct frame_property **at;
	IInspectable *old, *next = NULL;
	if (!dp) { RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
	for (at = &self->properties; *at && (*at)->property != dp; at = &(*at)->next) {}
	if (!*at) return S_OK;
	struct frame_property *entry = *at;
	*at = entry->next; old = entry->value;
	HRESULT status = frame_dependency_get_value(iface, dp, &next);
	iface->lpVtbl->AddRef(iface);
	if (SUCCEEDED(status) && old != next) status = raven_dependency_notify(dp, iface, old, next);
	if (old) old->lpVtbl->Release(old);
	if (next) next->lpVtbl->Release(next);
	entry->property->lpVtbl->Release(entry->property); FREE(entry);
	iface->lpVtbl->Release(iface);
	return status;
}

static HRESULT WINAPI frame_dependency_read_local(__x_ABI_CWindows_CUI_CXaml_CIDependencyObject *iface,
	__x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *dp, IInspectable **result)
{
	struct frame *self = ((struct frame_dependency_part *)iface)->owner;
	if (!result) return E_POINTER;
	*result = NULL;
	if (!dp) { RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
	for (struct frame_property *entry = self->properties; entry; entry = entry->next)
		if (entry->property == dp) {
			*result = entry->value;
			if (*result) (*result)->lpVtbl->AddRef(*result);
			return S_OK;
		}
	return raven_dependency_unset_value(result);
}

static HRESULT WINAPI frame_dependency_animation_base(__x_ABI_CWindows_CUI_CXaml_CIDependencyObject *iface,
	__x_ABI_CWindows_CUI_CXaml_CIDependencyProperty *dp, IInspectable **result)
{
	return frame_dependency_get_value(iface, dp, result);
}

static HRESULT WINAPI frame_dependency_dispatcher(__x_ABI_CWindows_CUI_CXaml_CIDependencyObject *iface,
	ICoreDispatcher **value)
{
	struct frame *self = ((struct frame_dependency_part *)iface)->owner;
	(void)self;
	if (!value)
		return E_POINTER;
	*value = NULL;
	{
		__x_ABI_CWindows_CUI_CCore_CICoreWindow *core = NULL;
		HRESULT result = raven_current_window(&core);
		if (FAILED(result))
			return result;
		result = core->lpVtbl->get_Dispatcher(core, value);
		core->lpVtbl->Release(core);
		return result;
	}
}

static const __x_ABI_CWindows_CUI_CXaml_CIDependencyObjectVtbl frame_dependency_table = {
	frame_dependency_query, frame_dependency_addref, frame_dependency_release,
	frame_dependency_iids, frame_dependency_name, frame_dependency_trust,
	frame_dependency_get_value, frame_dependency_put_value, frame_dependency_clear_value,
	frame_dependency_read_local, frame_dependency_animation_base, frame_dependency_dispatcher,
};

static ULONG frame_addref(struct frame *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG frame_release(struct frame *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		element_ui_release(&self->ui);
		while (self->framework_handlers) {
			struct framework_handler *entry = self->framework_handlers;
			self->framework_handlers = entry->next; IUnknown_Release(entry->handler); FREE(entry);
		}
		while (self->theme_handlers) {
			struct frame_theme_handler *entry = self->theme_handlers;
			self->theme_handlers = entry->next; IUnknown_Release(entry->handler); FREE(entry);
		}
		raven_event_close(&self->navigated);
		DeleteCriticalSection(&self->navigated.lock);
		raven_event_close(&self->is_enabled_changed);
		DeleteCriticalSection(&self->is_enabled_changed.lock);
		if (self->standalone_contentdialog) {
			struct raven_event_list *dialog_lists[] = {&self->dialog_closing, &self->dialog_closed, &self->dialog_opened,
				&self->dialog_primary_click, &self->dialog_secondary_click, &self->dialog_close_click};
			for (unsigned i = 0; i < sizeof dialog_lists / sizeof *dialog_lists; ++i) {
				raven_event_close(dialog_lists[i]);
				DeleteCriticalSection(&dialog_lists[i]->lock);
			}
			if (self->dialog_primary_text) WindowsDeleteString(self->dialog_primary_text);
			if (self->dialog_secondary_text) WindowsDeleteString(self->dialog_secondary_text);
			if (self->dialog_close_text) WindowsDeleteString(self->dialog_close_text);
			if (self->dialog_title) IInspectable_Release(self->dialog_title);
			if (self->dialog_title_template) IInspectable_Release(self->dialog_title_template);
			if (self->dialog_primary_command) IInspectable_Release(self->dialog_primary_command);
			if (self->dialog_secondary_command) IInspectable_Release(self->dialog_secondary_command);
			if (self->dialog_primary_command_parameter) IInspectable_Release(self->dialog_primary_command_parameter);
			if (self->dialog_secondary_command_parameter) IInspectable_Release(self->dialog_secondary_command_parameter);
			if (self->dialog_close_command) IInspectable_Release(self->dialog_close_command);
			if (self->dialog_close_command_parameter) IInspectable_Release(self->dialog_close_command_parameter);
			if (self->dialog_primary_style) IInspectable_Release(self->dialog_primary_style);
			if (self->dialog_secondary_style) IInspectable_Release(self->dialog_secondary_style);
			if (self->dialog_close_style) IInspectable_Release(self->dialog_close_style);
		}
		while (self->properties) {
			struct frame_property *entry = self->properties;
			self->properties = entry->next;
			if (entry->value) entry->value->lpVtbl->Release(entry->value);
			entry->property->lpVtbl->Release(entry->property);
			FREE(entry);
		}
		if (self->name)
			WindowsDeleteString(self->name);
		if (self->source_type)
			WindowsDeleteString(self->source_type);
		if (self->page)
			IInspectable_Release(self->page);
		if (self->navigation_parameter)
			IInspectable_Release(self->navigation_parameter);
		if (self->background)
			IInspectable_Release(self->background);
		if (self->font_family)
			IInspectable_Release(self->font_family);
		if (self->foreground)
			IInspectable_Release(self->foreground);
		if (self->border_brush)
			IInspectable_Release(self->border_brush);
		if (self->template)
			IInspectable_Release(self->template);
		if (self->content_template) IInspectable_Release(self->content_template);
		if (self->content_selector) IInspectable_Release(self->content_selector);
		if (self->content_transitions) IInspectable_Release(self->content_transitions);
		if (self->usercontrol_content) self->usercontrol_content->lpVtbl->Release(self->usercontrol_content);
		if (self->resources) IInspectable_Release(self->resources);
		if (self->page_top_app_bar) IInspectable_Release(self->page_top_app_bar);
		if (self->page_bottom_app_bar) IInspectable_Release(self->page_bottom_app_bar);
		if (self->focus_visual_secondary_brush) IInspectable_Release(self->focus_visual_secondary_brush);
		if (self->focus_visual_primary_brush) IInspectable_Release(self->focus_visual_primary_brush);
		if (self->tag) IInspectable_Release(self->tag);
		if (self->data_context) IInspectable_Release(self->data_context);
		if (self->style) IInspectable_Release(self->style);
		if (self->language)
			WindowsDeleteString(self->language);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static void frame_set_source(struct frame *self, const WCHAR *text, UINT32 length)
{
	HSTRING value = NULL;
	if (self->source_type) {
		WindowsDeleteString(self->source_type);
		self->source_type = NULL;
	}
	if (text && length && SUCCEEDED(WindowsCreateString(text, length, &value)))
		self->source_type = value;
}

static void write_type_name(uint64_t out, HSTRING source)
{
	xaml_type_name *value = (xaml_type_name *)(uintptr_t)out;
	UINT32 length = 0;
	const WCHAR *text;
	value->Name = NULL;
	value->Kind = TypeKind_Metadata;
	if (!source)
		return;
	text = WindowsGetStringRawBuffer(source, &length);
	WindowsCreateString(text, length, &value->Name);
}

static HRESULT WINAPI frame_cache_size(frame1 *iface, uint64_t out)
{
	if (!out)
		return E_POINTER;
	*(int32_t *)(uintptr_t)out = ((struct frame1_part *)iface)->owner->cache_size;
	return S_OK;
}

static HRESULT WINAPI frame_put_cache_size(frame1 *iface, uint64_t value)
{
	((struct frame1_part *)iface)->owner->cache_size = (int32_t)value;
	return S_OK;
}

static HRESULT WINAPI frame_false(frame1 *iface, uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*(boolean *)(uintptr_t)out = FALSE;
	return S_OK;
}

static HRESULT WINAPI frame_depth(frame1 *iface, uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*(int32_t *)(uintptr_t)out = 0;
	return S_OK;
}

static HRESULT WINAPI frame_current_type(frame1 *iface, uint64_t out)
{
	struct frame *self = ((struct frame1_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	write_type_name(out, self->source_type);
	return S_OK;
}

static HRESULT WINAPI frame_put_source_type(frame1 *iface, uint64_t value)
{
	struct frame *self = ((struct frame1_part *)iface)->owner;
	const xaml_type_name *type = (const xaml_type_name *)(uintptr_t)value;
	UINT32 length = 0;
	const WCHAR *text;
	if (!type || !type->Name) {
		frame_set_source(self, NULL, 0);
		return S_OK;
	}
	text = WindowsGetStringRawBuffer(type->Name, &length);
	frame_set_source(self, text, length);
	return S_OK;
}

static HRESULT WINAPI frame_event_add(frame1 *iface, uint64_t handler, uint64_t token)
{
	(void)iface;
	(void)handler;
	if (token)
		*(uint64_t *)(uintptr_t)token = 1;
	return S_OK;
}

static HRESULT WINAPI frame_event_remove(frame1 *iface, uint64_t token)
{
	(void)iface;
	(void)token;
	return S_OK;
}

/* Navigated is the one frame event the engine raises: the framework notifies the handlers and then drives the page's
 * OnNavigatedTo after a completed Navigate. */
static HRESULT WINAPI frame_navigated_add(frame1 *iface, uint64_t handler, uint64_t token)
{
	return raven_event_add(&((struct frame1_part *)iface)->owner->navigated, (IUnknown *)(uintptr_t)handler,
		(EventRegistrationToken *)(uintptr_t)token);
}

static HRESULT WINAPI frame_navigated_remove(frame1 *iface, uint64_t token)
{
	EventRegistrationToken value;
	value.value = (INT64)token;
	return raven_event_remove(&((struct frame1_part *)iface)->owner->navigated, value);
}

/* The NavigationEventArgs a completed navigation hands to the Navigated handlers and the page's OnNavigatedTo:
 * the created page as the content, the navigated-to type as the source page type, navigation mode New, the
 * parameter the Navigate call carried (null when the caller passed none) and no URI. */
struct navigation_event_args;
struct navigation_args_part { const navigation_args1_vtbl *lpVtbl; struct navigation_event_args *owner; };
struct navigation_args_inner { const IInspectableVtbl *lpVtbl; struct navigation_event_args *owner; };
struct navigation_event_args {
	struct navigation_args_part iface;
	struct navigation_args_inner inner;
	LONG refs;
	IInspectable *content;
	IInspectable *parameter;
	HSTRING source_type;
};
static const WCHAR navigation_args_class[] = L"Windows.UI.Xaml.Navigation.NavigationEventArgs";

static HRESULT navigation_args_owner_query(struct navigation_event_args *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable)) {
		*out = &self->inner;
	} else if (equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CNavigation_CINavigationEventArgs)) {
		*out = &self->iface;
	} else {
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
	return S_OK;
}
static HRESULT WINAPI navigation_args_iquery(IInspectable *iface, REFIID iid, void **out)
{ return navigation_args_owner_query(((struct navigation_args_inner *)iface)->owner, iid, out); }
static ULONG WINAPI navigation_args_iaddref(IInspectable *iface)
{ return (ULONG)InterlockedIncrement(&((struct navigation_args_inner *)iface)->owner->refs); }
static ULONG WINAPI navigation_args_irelease(IInspectable *iface)
{
	struct navigation_event_args *self = ((struct navigation_args_inner *)iface)->owner;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->content)
			IInspectable_Release(self->content);
		if (self->parameter)
			IInspectable_Release(self->parameter);
		if (self->source_type)
			WindowsDeleteString(self->source_type);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}
static HRESULT WINAPI navigation_args_iids(IInspectable *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI navigation_args_name(IInspectable *iface, HSTRING *name)
{
	(void)iface;
	if (!name) return E_POINTER;
	return WindowsCreateString(navigation_args_class, (UINT32)lstrlenW(navigation_args_class), name);
}
static HRESULT WINAPI navigation_args_trust(IInspectable *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static const IInspectableVtbl navigation_args_inspectable_table = {
	navigation_args_iquery, navigation_args_iaddref, navigation_args_irelease,
	navigation_args_iids, navigation_args_name, navigation_args_trust,
};

static struct navigation_event_args *navigation_args_of(navigation_args1 *iface)
{ return ((struct navigation_args_part *)iface)->owner; }
static HRESULT WINAPI navigation_args_content(navigation_args1 *iface, uint64_t out)
{
	struct navigation_event_args *self = navigation_args_of(iface);
	if (!out) return E_POINTER;
	*(void **)(uintptr_t)out = self->content;
	if (self->content)
		IInspectable_AddRef(self->content);
	return S_OK;
}
static HRESULT WINAPI navigation_args_parameter(navigation_args1 *iface, uint64_t out)
{
	struct navigation_event_args *self = navigation_args_of(iface);
	if (!out) return E_POINTER;
	*(void **)(uintptr_t)out = self->parameter;
	if (self->parameter)
		IInspectable_AddRef(self->parameter);
	return S_OK;
}
static HRESULT WINAPI navigation_args_source_type(navigation_args1 *iface, uint64_t out)
{
	struct navigation_event_args *self = navigation_args_of(iface);
	UINT32 length = 0;
	const WCHAR *text;
	if (!out) return E_POINTER;
	((xaml_type_name *)(uintptr_t)out)->Kind = TypeKind_Metadata;
	((xaml_type_name *)(uintptr_t)out)->Name = NULL;
	if (!self->source_type)
		return S_OK;
	text = WindowsGetStringRawBuffer(self->source_type, &length);
	WindowsCreateString(text, length, &((xaml_type_name *)(uintptr_t)out)->Name);
	return S_OK;
}
static HRESULT WINAPI navigation_args_mode(navigation_args1 *iface, uint64_t out)
{ (void)iface; if (!out) return E_POINTER; *(int32_t *)(uintptr_t)out = 0; return S_OK; }
static HRESULT WINAPI navigation_args_get_uri(navigation_args1 *iface, uint64_t out)
{ (void)iface; if (!out) return E_POINTER; *(void **)(uintptr_t)out = NULL; return S_OK; }
static HRESULT WINAPI navigation_args_put_uri(navigation_args1 *iface, uint64_t value)
{ (void)iface; (void)value; return S_OK; }
static const navigation_args1_vtbl navigation_args_table = {
	(HRESULT (STDMETHODCALLTYPE *)(navigation_args1 *, REFIID, void **))navigation_args_iquery,
	(ULONG (STDMETHODCALLTYPE *)(navigation_args1 *))navigation_args_iaddref,
	(ULONG (STDMETHODCALLTYPE *)(navigation_args1 *))navigation_args_irelease,
	(HRESULT (STDMETHODCALLTYPE *)(navigation_args1 *, ULONG *, IID **))navigation_args_iids,
	(HRESULT (STDMETHODCALLTYPE *)(navigation_args1 *, HSTRING *))navigation_args_name,
	(HRESULT (STDMETHODCALLTYPE *)(navigation_args1 *, TrustLevel *))navigation_args_trust,
	navigation_args_content, navigation_args_parameter, navigation_args_source_type,
	navigation_args_mode, navigation_args_get_uri, navigation_args_put_uri,
};

static IInspectable *navigation_args_create(struct frame *self)
{
	struct navigation_event_args *args = ALLOC(sizeof *args);
	if (!args)
		return NULL;
	args->iface.lpVtbl = &navigation_args_table;
	args->iface.owner = args;
	args->inner.lpVtbl = &navigation_args_inspectable_table;
	args->inner.owner = args;
	args->refs = 1;
	args->content = self->page;
	if (args->content)
		IInspectable_AddRef(args->content);
	args->parameter = self->navigation_parameter;
	if (args->parameter)
		IInspectable_AddRef(args->parameter);
	if (self->source_type) {
		UINT32 length = 0;
		const WCHAR *text = WindowsGetStringRawBuffer(self->source_type, &length);
		WindowsCreateString(text, length, &args->source_type);
	}
	InterlockedIncrement(&raven_xaml_objects);
	return (IInspectable *)&args->inner;
}

/* The page-lifecycle call the framework drives after the Navigated event: the page's OnNavigatedTo override through
 * the IPageOverrides face its CCW serves when the page class overrides it. A handler or override failure is traced,
 * never propagated: the navigation itself completed. */
static void frame_navigated_deliver(struct frame *self)
{
	IUnknown **handlers = NULL;
	ULONG count = 0;
	IInspectable *args = NULL;
	typedef HRESULT (STDMETHODCALLTYPE *typed_handler_invoke)(IUnknown *, IInspectable *, IInspectable *);
	HRESULT result = raven_event_snapshot(&self->navigated, &handlers, &count);
	if (FAILED(result))
		return;
	if (count)
		args = navigation_args_create(self);
	for (ULONG index = 0; index < count; ++index) {
		if (args) {
			typed_handler_invoke invoke = (typed_handler_invoke)(*(void ***)handlers[index])[3];
			const HRESULT called = invoke(handlers[index], (IInspectable *)&self->first, args);
			if (FAILED(called)) {
				char note[96];
				wsprintfA(note, "raven-frame: Navigated handler failed 0x%08x\n", (unsigned)called);
				raven_trace_note(note);
			}
		}
		IUnknown_Release(handlers[index]);
	}
	FREE(handlers);
	if (self->page) {
		page_overrides_i *overrides = NULL;
		result = self->page->lpVtbl->QueryInterface(self->page, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPageOverrides,
			(void **)&overrides);
		char note[96];
		if (SUCCEEDED(result) && overrides) {
			result = overrides->lpVtbl->OnNavigatedTo(overrides, (uint64_t)(uintptr_t)args);
			overrides->lpVtbl->Release(overrides);
			wsprintfA(note, "raven-frame: page OnNavigatedTo 0x%08x\n", (unsigned)result);
		} else {
			wsprintfA(note, "raven-frame: page overrides refused 0x%08x\n", (unsigned)result);
		}
		raven_trace_note(note);
	}
	if (args)
		IInspectable_Release(args);
}

static HRESULT frame_navigate(struct frame *self, uint64_t type_address, uint64_t parameter_address, uint64_t result)
{
	const xaml_type_name *type = (const xaml_type_name *)(uintptr_t)type_address;
	boolean *navigated = (boolean *)(uintptr_t)result;
	IInspectable *page_object = NULL;
	UINT32 length = 0;
	const WCHAR *text;
	if (navigated)
		*navigated = FALSE;
	if (!type || !type->Name)
		return S_OK;
	text = WindowsGetStringRawBuffer(type->Name, &length);
	if (FAILED(raven_xaml_create_named(text, length, &page_object)) || !page_object) {
		if (page_object)
			IInspectable_Release(page_object);
		page_object = NULL;
		/* The framework table does not carry application page types: the app's own IXamlMetadataProvider (the
		 * XamlTypeInfo provider on its Application CCW) is the documented creation path — GetXamlType with the
		 * TypeName exactly as navigated, then the type's ActivateInstance. */
		if (FAILED(raven_xaml_create_from_type(type, &page_object)) || !page_object) {
			if (page_object)
				IInspectable_Release(page_object);
			raven_trace_note("raven-xaml: frame navigate found no page\n");
			return S_OK;
		}
		raven_trace_note("raven-xaml: frame navigate page created through the app metadata provider\n");
	}
	frame_set_source(self, text, length);
	/* Hold the caller's parameter the way Windows does: it reaches the Navigated handlers and the page's
	 * OnNavigatedTo as NavigationEventArgs.Parameter and survives until the next Navigate replaces it. */
	if (self->navigation_parameter)
		IInspectable_Release(self->navigation_parameter);
	self->navigation_parameter = (IInspectable *)(uintptr_t)parameter_address;
	if (self->navigation_parameter)
		IInspectable_AddRef(self->navigation_parameter);
	if (self->page)
		IInspectable_Release(self->page);
	self->page = page_object;
	raven_xaml_page_set_frame(page_object, (IInspectable *)&self->first);
	if (navigated)
		*navigated = TRUE;
	frame_navigated_deliver(self);
	raven_render_changed();
	return S_OK;
}

static HRESULT WINAPI frame_navigate_iface(frame1 *iface, uint64_t type, uint64_t parameter, uint64_t result)
{
	return frame_navigate(((struct frame1_part *)iface)->owner, type, parameter, result);
}

static HRESULT WINAPI frame_navigate2(frame2 *iface, uint64_t type, uint64_t parameter, uint64_t transition, uint64_t result)
{
	(void)transition;
	return frame_navigate(((struct frame2_part *)iface)->owner, type, parameter, result);
}

static HRESULT WINAPI frame_navigate_type(frame5 *iface, uint64_t type, uint64_t parameter, uint64_t transition, uint64_t result)
{
	(void)transition;
	return frame_navigate(((struct frame5_part *)iface)->owner, type, parameter, result);
}

static HRESULT WINAPI frame_go_back(frame1 *iface) { (void)iface; return S_OK; }
static HRESULT WINAPI frame_go_forward(frame1 *iface) { (void)iface; return S_OK; }
static HRESULT WINAPI frame_go_back_one(frame3 *iface, uint64_t count) { (void)iface; (void)count; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI frame_state_out(frame1 *iface, uint64_t out)
{
	(void)iface;
	if (out)
		*(void **)(uintptr_t)out = NULL;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}
static HRESULT WINAPI frame_state_in(frame1 *iface, uint64_t state) { (void)iface; (void)state; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI frame_set_state2(frame4 *iface, uint64_t state, uint64_t frame_state) { (void)iface; (void)state; (void)frame_state; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI frame_stack_out(frame2 *iface, uint64_t out)
{
	(void)iface;
	if (out)
		*(void **)(uintptr_t)out = NULL;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}
static HRESULT WINAPI frame_stack_enabled(frame5 *iface, uint64_t out)
{
	if (!out)
		return E_POINTER;
	*(boolean *)(uintptr_t)out = ((struct frame5_part *)iface)->owner->stack_enabled;
	return S_OK;
}
static HRESULT WINAPI frame_put_stack_enabled(frame5 *iface, uint64_t value)
{
	((struct frame5_part *)iface)->owner->stack_enabled = (boolean)value;
	return S_OK;
}

STUB1(frame_statics1, frame_prop_cache)
STUB1(frame_statics1, frame_prop_can_back)
STUB1(frame_statics1, frame_prop_can_forward)
STUB1(frame_statics1, frame_prop_current_type)
STUB1(frame_statics1, frame_prop_source_type)
STUB1(frame_statics1, frame_prop_depth)
STUB1(frame_statics2, frame_prop_back_stack)
STUB1(frame_statics2, frame_prop_forward_stack)
STUB1(frame_statics5, frame_prop_stack_enabled)

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameVtbl frame1_table = {
	.QueryInterface = frame1_query,
	.AddRef = frame1_addref,
	.Release = frame1_release,
	.GetIids = frame_iids,
	.GetRuntimeClassName = frame_name,
	.GetTrustLevel = frame_trust,
	.get_CacheSize = frame_cache_size,
	.put_CacheSize = frame_put_cache_size,
	.get_CanGoBack = frame_false,
	.get_CanGoForward = frame_false,
	.get_CurrentSourcePageType = frame_current_type,
	.get_SourcePageType = frame_current_type,
	.put_SourcePageType = frame_put_source_type,
	.get_BackStackDepth = frame_depth,
	.add_Navigated = frame_navigated_add,
	.remove_Navigated = frame_navigated_remove,
	.add_Navigating = frame_event_add,
	.remove_Navigating = frame_event_remove,
	.add_NavigationFailed = frame_event_add,
	.remove_NavigationFailed = frame_event_remove,
	.add_NavigationStopped = frame_event_add,
	.remove_NavigationStopped = frame_event_remove,
	.GoBack = frame_go_back,
	.GoForward = frame_go_forward,
	.Navigate = frame_navigate_iface,
	.GetNavigationState = frame_state_out,
	.SetNavigationState = frame_state_in,
};

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2Vtbl frame2_table = {
	.QueryInterface = frame2_query,
	.AddRef = frame2_addref,
	.Release = frame2_release,
	.GetIids = frame2_iids,
	.GetRuntimeClassName = frame2_name,
	.GetTrustLevel = frame2_trust,
	.get_BackStack = frame_stack_out,
	.get_ForwardStack = frame_stack_out,
	.Navigate = frame_navigate2,
};

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3Vtbl frame3_table = {
	.QueryInterface = frame3_query,
	.AddRef = frame3_addref,
	.Release = frame3_release,
	.GetIids = frame3_iids,
	.GetRuntimeClassName = frame3_name,
	.GetTrustLevel = frame3_trust,
	.GoBack = frame_go_back_one,
};

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4Vtbl frame4_table = {
	.QueryInterface = frame4_query,
	.AddRef = frame4_addref,
	.Release = frame4_release,
	.GetIids = frame4_iids,
	.GetRuntimeClassName = frame4_name,
	.GetTrustLevel = frame4_trust,
	.SetNavigationState = frame_set_state2,
};

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5Vtbl frame5_table = {
	.QueryInterface = frame5_query,
	.AddRef = frame5_addref,
	.Release = frame5_release,
	.GetIids = frame5_iids,
	.GetRuntimeClassName = frame5_name,
	.GetTrustLevel = frame5_trust,
	.get_IsNavigationStackEnabled = frame_stack_enabled,
	.put_IsNavigationStackEnabled = frame_put_stack_enabled,
	.NavigateToType = frame_navigate_type,
};

/* The single-ABI-slot float: the generated callers pack FontSize's IEEE-754 bits in the low 32 slot word
 * (the convention xaml_elements.c's TextBlock accessors already decode with the same helper). */
static uint64_t control_float_word(float value)
{
	uint64_t word = 0;
	memcpy(&word, &value, sizeof value);
	return word;
}

static struct frame *create_frame(void)
{
	struct frame *frame = ALLOC(sizeof *frame);
	if (!frame)
		return NULL;
	frame->first.lpVtbl = &frame1_table;
	frame->first.owner = frame;
	frame->second.lpVtbl = &frame2_table;
	frame->second.owner = frame;
	frame->third.lpVtbl = &frame3_table;
	frame->third.owner = frame;
	frame->fourth.lpVtbl = &frame4_table;
	frame->fourth.owner = frame;
	frame->fifth.lpVtbl = &frame5_table;
	frame->fifth.owner = frame;
	frame->inner.lpVtbl = &frame_inner_table;
	frame->inner.owner = frame;
	frame->peer_bridge.lpVtbl = &frame_peer_bridge_table;
	frame->peer_bridge.owner = frame;
	frame->peer_shim.lpVtbl = &frame_peer_shim_table;
	frame->peer_shim.owner = frame;
	InitializeCriticalSection(&frame->navigated.lock);
	InitializeCriticalSection(&frame->is_enabled_changed.lock);
	raven_tracker_part_init(&frame->tracker, frame, &frame->refs, frame_owner_query);
	element_ui_init(&frame->ui, frame, frame_ui_query, frame_ui_addref, frame_ui_release, frame_class);
	frame->framework.lpVtbl = &frame_framework_table;
	frame->framework.owner = frame;
	frame->theme.lpVtbl = &frame_theme_table;
	frame->theme.owner = frame;
	frame->content.lpVtbl = &frame_content_table;
	frame->content.owner = frame;
	frame->usercontrol.lpVtbl = &frame_usercontrol_table;
	frame->usercontrol.owner = frame;
	frame->control.lpVtbl = &frame_control_table;
	frame->control.owner = frame;
	frame->dependency.lpVtbl = &frame_dependency_table;
	frame->dependency.owner = frame;
	frame->page_face.lpVtbl = &frame_page_table;
	frame->page_face.owner = frame;
	frame->page_overrides.lpVtbl = &frame_page_overrides_table;
	frame->page_overrides.owner = frame;
	frame->contentdialog.lpVtbl = &frame_contentdialog_table;
	frame->contentdialog.owner = frame;
	frame->contentdialog2.lpVtbl = &frame_contentdialog2_table;
	frame->contentdialog2.owner = frame;
	frame->fe2.lpVtbl = &frame_fe2_table;
	frame->fe2.owner = frame;
	frame->fe3.lpVtbl = &frame_fe3_table;
	frame->fe3.owner = frame;
	frame->fe4.lpVtbl = &frame_fe4_table;
	frame->fe4.owner = frame;
	frame->refs = 1;
	frame->stack_enabled = TRUE;
	/* The Control scalar defaults the projected IControl getters return before any setter ran: the UWP
	 * defaults (FontSize 14 double-word packed as float bits, IsEnabled true, IsTabStop false, the
	 * alignment enums centered, TabNavigation Once). */
	frame->font_size = control_float_word(14.0f);
	frame->font_weight = 400;
	frame->is_tab_stop = 0;
	frame->is_enabled = 1;
	frame->tab_navigation = 1;
	frame->horizontal_content_alignment = 1;
	frame->vertical_content_alignment = 1;
	/* The FrameworkElement metadata defaults the honest face reports before any setter ran: Width/Height
	 * NaN (Auto), MinWidth/MinHeight 0, MaxWidth/MaxHeight PositiveInfinity, both alignments Stretch (3),
	 * FlowDirection LeftToRight (0), an empty Margin. */
	{
		static const double auto_size = __builtin_nan("");
		static const double max_size = 1.7976931348623157e+308; /* PositiveInfinity, as the column definitions record it */
		frame->width = auto_size;
		frame->height = auto_size;
		frame->max_width = max_size;
		frame->max_height = max_size;
		frame->horizontal_alignment = 3;
		frame->vertical_alignment = 3;
		frame->requested_theme = 1;
		frame->allow_focus_on_interaction = 1;
	}
	InterlockedIncrement(&raven_xaml_objects);
	return frame;
}

/* Page */

static HRESULT page_query_any(struct page *self, REFIID iid, void **out);
static ULONG page_addref(struct page *self);
static ULONG page_release(struct page *self);
static HRESULT WINAPI page_query(page1 *iface, REFIID iid, void **out);
static ULONG WINAPI page_addref_iface(page1 *iface);
static ULONG WINAPI page_release_iface(page1 *iface);
static HRESULT WINAPI page_iids(page1 *iface, ULONG *count, IID **iids);
static HRESULT WINAPI page_name(page1 *iface, HSTRING *name);
static HRESULT WINAPI page_trust(page1 *iface, TrustLevel *level);
static HRESULT WINAPI page_frame(page1 *iface, uint64_t out);
static HRESULT WINAPI page_cache_mode(page1 *iface, uint64_t out);
static HRESULT WINAPI page_put_cache_mode(page1 *iface, uint64_t value);
static HRESULT WINAPI page_top_app_bar(page1 *iface, uint64_t out);
static HRESULT WINAPI page_put_top_app_bar(page1 *iface, uint64_t value);
static HRESULT WINAPI page_bottom_app_bar(page1 *iface, uint64_t out);
static HRESULT WINAPI page_put_bottom_app_bar(page1 *iface, uint64_t value);

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIPageVtbl page_table = {
	.QueryInterface = page_query,
	.AddRef = page_addref_iface,
	.Release = page_release_iface,
	.GetIids = page_iids,
	.GetRuntimeClassName = page_name,
	.GetTrustLevel = page_trust,
	.get_Frame = page_frame,
	.get_NavigationCacheMode = page_cache_mode,
	.put_NavigationCacheMode = page_put_cache_mode,
	.get_TopAppBar = page_top_app_bar,
	.put_TopAppBar = page_put_top_app_bar,
	.get_BottomAppBar = page_bottom_app_bar,
	.put_BottomAppBar = page_put_bottom_app_bar,
};

static struct page *page_of_content(page_content_control *iface)
{
	return (struct page *)((char *)iface - offsetof(struct page, content_lpVtbl));
}

/* The page's faces delegate identity and lifetime to the composition outer when the managed page constructor
 * composed with one, exactly the frame's pattern; the non-delegating inner part serves the factory's inner
 * answer and the object's own count. */
static HRESULT page_query_any(struct page *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable)) {
		*out = self->outer ? (void *)&self->inner : (void *)self;
		page_addref(self);
		return S_OK;
	}
	if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPage))
		*out = self;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl))
		*out = &self->content_lpVtbl;
	else {
		/* Which projected interface the caller asks the page's inner for and the inner refuses —
		 * the refusal list is the page-face contract as the generated callers see it.
		 * RAVEN_TRACE_PAGE_QI=1 in the guest enables it. */
		static int page_qi_gate = -1;
		if (page_qi_gate < 0) {
			char flag[8] = "";
			page_qi_gate = GetEnvironmentVariableA("RAVEN_TRACE_PAGE_QI", flag, sizeof flag) ? 1 : 0;
		}
		if (page_qi_gate) {
			char line[160];
			wsprintfA(line, "raven-page: refused QI %08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
				(unsigned long)iid->Data1, iid->Data2, iid->Data3, iid->Data4[0], iid->Data4[1],
				iid->Data4[2], iid->Data4[3], iid->Data4[4], iid->Data4[5], iid->Data4[6], iid->Data4[7]);
			OutputDebugStringA(line);
		}
		return E_NOINTERFACE;
	}
	if (self->outer)
		IInspectable_AddRef(self->outer);
	else
		page_addref(self);
	return S_OK;
}

static ULONG page_addref(struct page *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG page_release(struct page *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->name) WindowsDeleteString(self->name);
		if (self->frame) IInspectable_Release(self->frame);
		if (self->top_app_bar) IInspectable_Release(self->top_app_bar);
		if (self->bottom_app_bar) IInspectable_Release(self->bottom_app_bar);
		if (self->content) IInspectable_Release(self->content);
		if (self->outer) IInspectable_Release(self->outer);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI page_query(page1 *iface, REFIID iid, void **out)
{
	struct page *self = (struct page *)iface;
	if (self->outer)
		return IInspectable_QueryInterface(self->outer, iid, out);
	return page_query_any(self, iid, out);
}

static ULONG WINAPI page_addref_iface(page1 *iface)
{
	struct page *self = (struct page *)iface;
	return self->outer ? IInspectable_AddRef(self->outer) : page_addref(self);
}

static ULONG WINAPI page_release_iface(page1 *iface)
{
	struct page *self = (struct page *)iface;
	return self->outer ? IInspectable_Release(self->outer) : page_release(self);
}

/* The non-delegating IUnknown the composable factory hands the managed projection. */
static HRESULT WINAPI page_inner_query(IInspectable *iface, REFIID iid, void **out)
{ return page_query_any(((struct page_inner_part *)iface)->owner, iid, out); }
static ULONG WINAPI page_inner_addref(IInspectable *iface)
{ return page_addref(((struct page_inner_part *)iface)->owner); }
static ULONG WINAPI page_inner_release(IInspectable *iface)
{ return page_release(((struct page_inner_part *)iface)->owner); }
static HRESULT WINAPI page_inner_iids(IInspectable *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI page_inner_name(IInspectable *iface, HSTRING *name)
{
	(void)iface;
	if (!name) return E_POINTER;
	return WindowsCreateString(page_class, (UINT32)lstrlenW(page_class), name);
}
static HRESULT WINAPI page_inner_trust(IInspectable *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static const IInspectableVtbl page_inner_table = {
	page_inner_query, page_inner_addref, page_inner_release,
	page_inner_iids, page_inner_name, page_inner_trust,
};

static HRESULT WINAPI page_iids(page1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI page_name(page1 *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(page_class, (UINT32)lstrlenW(page_class), name);
}

static HRESULT WINAPI page_trust(page1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI page_frame(page1 *iface, uint64_t out)
{
	struct page *self = (struct page *)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = self->frame;
	if (self->frame)
		IInspectable_AddRef(self->frame);
	return S_OK;
}

static HRESULT WINAPI page_cache_mode(page1 *iface, uint64_t out)
{
	if (!out)
		return E_POINTER;
	*(int32_t *)(uintptr_t)out = ((struct page *)iface)->cache_mode;
	return S_OK;
}

static HRESULT WINAPI page_put_cache_mode(page1 *iface, uint64_t value)
{
	((struct page *)iface)->cache_mode = (int32_t)value;
	return S_OK;
}

static HRESULT page_put_object(struct page *self, IInspectable **slot, uint64_t value)
{
	IInspectable *object = (IInspectable *)(uintptr_t)value;
	if (object)
		IInspectable_AddRef(object);
	if (*slot)
		IInspectable_Release(*slot);
	*slot = object;
	return S_OK;
}

static HRESULT page_get_object(struct page *self, IInspectable **slot, uint64_t out)
{
	(void)self;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = *slot;
	if (*slot)
		IInspectable_AddRef(*slot);
	return S_OK;
}

static HRESULT WINAPI page_top_app_bar(page1 *iface, uint64_t out)
{
	return page_get_object((struct page *)iface, &((struct page *)iface)->top_app_bar, out);
}

static HRESULT WINAPI page_put_top_app_bar(page1 *iface, uint64_t value)
{
	return page_put_object((struct page *)iface, &((struct page *)iface)->top_app_bar, value);
}

static HRESULT WINAPI page_bottom_app_bar(page1 *iface, uint64_t out)
{
	return page_get_object((struct page *)iface, &((struct page *)iface)->bottom_app_bar, out);
}

static HRESULT WINAPI page_put_bottom_app_bar(page1 *iface, uint64_t value)
{
	return page_put_object((struct page *)iface, &((struct page *)iface)->bottom_app_bar, value);
}

/* Page inherits ContentControl: its Content part carries the element the loader hosts on the page. */

static HRESULT WINAPI page_content_query(page_content_control *iface, REFIID iid, void **out)
{
	struct page *self = page_of_content(iface);
	if (self->outer)
		return IInspectable_QueryInterface(self->outer, iid, out);
	return page_query_any(self, iid, out);
}

static ULONG WINAPI page_content_addref(page_content_control *iface)
{
	struct page *self = page_of_content(iface);
	return self->outer ? IInspectable_AddRef(self->outer) : page_addref(self);
}

static ULONG WINAPI page_content_release(page_content_control *iface)
{
	struct page *self = page_of_content(iface);
	return self->outer ? IInspectable_Release(self->outer) : page_release(self);
}

STUB1(page_content_control, page_content_template)
STUB1(page_content_control, page_content_put_template)
STUB1(page_content_control, page_content_selector)
STUB1(page_content_control, page_content_put_selector)
STUB1(page_content_control, page_content_transitions)
STUB1(page_content_control, page_content_put_transitions)

static HRESULT WINAPI page_content_get(page_content_control *iface, uint64_t out)
{
	struct page *self = page_of_content(iface);
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = self->content;
	if (self->content)
		IInspectable_AddRef(self->content);
	return S_OK;
}

static HRESULT WINAPI page_content_put(page_content_control *iface, uint64_t value)
{
	struct page *self = page_of_content(iface);
	IInspectable *object = (IInspectable *)(uintptr_t)value;
	if (object)
		IInspectable_AddRef(object);
	if (self->content)
		IInspectable_Release(self->content);
	self->content = object;
	return S_OK;
}

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControlVtbl page_content_table = {
	.QueryInterface = page_content_query,
	.AddRef = page_content_addref,
	.Release = page_content_release,
	.GetIids = (HRESULT (STDMETHODCALLTYPE *)(page_content_control *, ULONG *, IID **))page_iids,
	.GetRuntimeClassName = (HRESULT (STDMETHODCALLTYPE *)(page_content_control *, HSTRING *))page_name,
	.GetTrustLevel = (HRESULT (STDMETHODCALLTYPE *)(page_content_control *, TrustLevel *))page_trust,
	.get_Content = page_content_get,
	.put_Content = page_content_put,
	.get_ContentTemplate = page_content_template,
	.put_ContentTemplate = page_content_put_template,
	.get_ContentTemplateSelector = page_content_selector,
	.put_ContentTemplateSelector = page_content_put_selector,
	.get_ContentTransitions = page_content_transitions,
	.put_ContentTransitions = page_content_put_transitions,
};

static struct page *create_page(void)
{
	struct page *page = ALLOC(sizeof *page);
	if (!page)
		return NULL;
	page->lpVtbl = &page_table;
	page->content_lpVtbl = &page_content_table;
	page->inner.lpVtbl = &page_inner_table;
	page->inner.owner = page;
	page->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	return page;
}

void raven_xaml_page_set_frame(IInspectable *object, IInspectable *frame)
{
	page1 *page = NULL;
	if (!object)
		return;
	if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPage, (void **)&page)) || !page)
		return;
	/* A raven-built page carries the IPage face directly; an application page arrives as the managed CCW whose
	 * IPage face is its own projection (the frame reference rides the CCW's Page base state instead), so only the
	 * engine-owned face stores the reference — and only where it is stored is it retained. */
	if (page->lpVtbl == &page_table) {
		if (frame)
			IInspectable_AddRef(frame);
		struct page *self = (struct page *)page;
		if (self->frame)
			IInspectable_Release(self->frame);
		self->frame = frame;
	} else {
		char note[80];
		wsprintfA(note, "raven-xaml: page set frame skipped (managed page face)\n");
		raven_trace_note(note);
	}
	page->lpVtbl->Release(page);
}

static void store_name(HSTRING *slot, const WCHAR *text, UINT32 length);

/* ControlTemplate: the runtime class a Control's Template property and a Style's "Template" setter carry. The real
 * interface is TargetType only; the content tree arrives through the engine-internal attach (the XBF-declared template
 * loader path is future work), and the class is activatable, not composable, so the activation factory alone answers it.
 * The template owns its root element; the render walk reaches the root through the raven_xaml_frame_render_child hook,
 * which means a control with an applied template renders the template's tree in place of its own single visual child. */

struct control_template {
	const template1_vtbl *lpVtbl;
	LONG refs;
	HSTRING target_type; /* the TargetType name; the interop Kind slot reads back Metadata the way the frame's page type does */
	IInspectable *root; /* the content tree the engine-internal attach owns */
};

static HRESULT WINAPI template_query(template1 *iface, REFIID iid, void **out)
{
	struct control_template *self = (struct control_template *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplate))
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI template_addref(template1 *iface)
{
	return (ULONG)InterlockedIncrement(&((struct control_template *)iface)->refs);
}

static ULONG WINAPI template_release(template1 *iface)
{
	struct control_template *self = (struct control_template *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->target_type)
			WindowsDeleteString(self->target_type);
		if (self->root)
			IInspectable_Release(self->root);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI template_iids(template1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI template_name(template1 *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(template_class, (UINT32)lstrlenW(template_class), name);
}

static HRESULT WINAPI template_trust(template1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI template_target_type(template1 *iface, uint64_t out)
{
	write_type_name(out, ((struct control_template *)iface)->target_type);
	return S_OK;
}

static HRESULT WINAPI template_put_target_type(template1 *iface, uint64_t value)
{
	struct control_template *self = (struct control_template *)iface;
	const xaml_type_name *type = (const xaml_type_name *)(uintptr_t)value;
	UINT32 length = 0;
	const WCHAR *text = NULL;
	if (type && type->Name)
		text = WindowsGetStringRawBuffer(type->Name, &length);
	store_name(&self->target_type, text, text ? length : 0);
	return S_OK;
}

static const template1_vtbl template_table = {
	.QueryInterface = template_query,
	.AddRef = template_addref,
	.Release = template_release,
	.GetIids = template_iids,
	.GetRuntimeClassName = template_name,
	.GetTrustLevel = template_trust,
	.get_TargetType = template_target_type,
	.put_TargetType = template_put_target_type,
};

static struct control_template *create_control_template(void)
{
	struct control_template *template = ALLOC(sizeof *template);
	if (!template)
		return NULL;
	template->lpVtbl = &template_table;
	template->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	return template;
}

/* The engine-internal template surface, the dllexport+GetProcAddress pattern the fixtures reach the other engine-internal
 * entries through. raven_xaml_control_template_root answers S_OK for an engine ControlTemplate (root possibly NULL, the
 * caller's AddRef) and S_FALSE for any other object — the kind check the Template puts and the "Template" setter route
 * use to reject a wrong-kind value with E_INVALIDARG. */
HRESULT __declspec(dllexport) raven_xaml_control_template_root(IInspectable *template_object, IInspectable **out)
{
	template1 *iface = NULL;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!template_object)
		return E_POINTER;
	if (FAILED(template_object->lpVtbl->QueryInterface(template_object,
		&IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplate, (void **)&iface)) || !iface)
		return S_FALSE;
	if (iface->lpVtbl != &template_table) {
		iface->lpVtbl->Release(iface);
		return S_FALSE;
	}
	struct control_template *self = (struct control_template *)iface;
	if (self->root) {
		*out = self->root;
		(*out)->lpVtbl->AddRef(*out);
	}
	iface->lpVtbl->Release(iface);
	return S_OK;
}

/* The kind probe the loader's template-node handling drives: non-zero for an engine ControlTemplate, zero for any
 * other object (including a foreign IControlTemplate implementation — the vtable check is the engine's own). */
int raven_xaml_object_is_control_template(IInspectable *object)
{
	template1 *iface = NULL;
	if (!object)
		return 0;
	if (FAILED(object->lpVtbl->QueryInterface(object,
		&IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplate, (void **)&iface)) || !iface)
		return 0;
	const int is_template = iface->lpVtbl == &template_table;
	iface->lpVtbl->Release(iface);
	return is_template;
}

HRESULT __declspec(dllexport) raven_xaml_control_template_put_root(IInspectable *template_object, IInspectable *root)
{
	IInspectable *probe = NULL;
	struct control_template *self = NULL;
	template1 *iface = NULL;
	if (!template_object)
		return E_POINTER;
	if (root)
		IInspectable_AddRef(root);
	if (FAILED(template_object->lpVtbl->QueryInterface(template_object,
		&IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplate, (void **)&iface)) || !iface) {
		if (root)
			IInspectable_Release(root);
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_controls.c:958\n"); { RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; } }
	}
	if (iface->lpVtbl != &template_table) {
		iface->lpVtbl->Release(iface);
		if (root)
			IInspectable_Release(root);
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_controls.c:964\n"); { RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; } }
	}
	self = (struct control_template *)iface;
	(void)raven_xaml_control_template_root(template_object, &probe);
	if (probe)
		IInspectable_Release(probe);
	if (self->root)
		IInspectable_Release(self->root);
	self->root = root;
	raven_render_changed();
	iface->lpVtbl->Release(iface);
	return S_OK;
}

/* Factory */

/* The panel stores the composition swapchain ISwapChainPanelNative::SetSwapChain hands it. Presentation rides the
 * chain's own path: the caller's Present runs the DXVK/vkd3d presenter over the DXVKDUMMYWINDOW hwnd, whose raven
 * host surface carries the frames; this object composes no pixels of its own and never invents panel content.
 * It still deliberately exposes no IUIElement/IPanel until layout can host GPU content. */
static const WCHAR swapchain_class[] = L"Windows.UI.Xaml.Controls.SwapChainPanel";
struct swapchain_panel;
struct swapchain_part { const raven_swapchain_panel_vtbl *lpVtbl; struct swapchain_panel *owner; };
struct swapchain_native_part { const raven_swapchain_native_vtbl *lpVtbl; struct swapchain_panel *owner; };
struct swapchain_panel {
	struct swapchain_part panel;
	struct swapchain_native_part native;
	LONG refs;
	DWORD thread;
	IDXGISwapChain *stored;
};
static HRESULT swapchain_query(struct swapchain_panel *self, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CISwapChainPanel))
		*out = &self->panel;
	else if (equal_iid(iid, &IID_ISwapChainPanelNative))
		*out = &self->native;
	else return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}
static ULONG swapchain_release(struct swapchain_panel *self)
{
	ULONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->stored) self->stored->lpVtbl->Release(self->stored);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return refs;
}
static HRESULT WINAPI sc_query(raven_swapchain_panel *iface, REFIID iid, void **out)
{ return swapchain_query(((struct swapchain_part *)iface)->owner, iid, out); }
static ULONG WINAPI sc_addref(raven_swapchain_panel *iface)
{ return InterlockedIncrement(&((struct swapchain_part *)iface)->owner->refs); }
static ULONG WINAPI sc_release(raven_swapchain_panel *iface)
{ return swapchain_release(((struct swapchain_part *)iface)->owner); }
static HRESULT WINAPI sc_iids(raven_swapchain_panel *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI sc_name(raven_swapchain_panel *iface, HSTRING *out)
{
	(void)iface;
	if (!out) return E_POINTER;
	return WindowsCreateString(swapchain_class, ARRAYSIZE(swapchain_class) - 1, out);
}
static HRESULT WINAPI sc_trust(raven_swapchain_panel *iface, TrustLevel *out)
{ (void)iface; return inspectable_trust(out); }
static HRESULT WINAPI sc_scale(raven_swapchain_panel *iface, FLOAT *out)
{
	(void)iface;
	if (!out) return E_POINTER;
	*out = 0;
	/* No layout attachment exists from which to obtain a composition scale. */
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}
static HRESULT WINAPI sc_add_scale(raven_swapchain_panel *iface, IUnknown *handler, EventRegistrationToken *token)
{
	(void)iface; (void)handler;
	if (!token) return E_POINTER;
	token->value = 0;
	/* No composition-scale source: do not accept a subscription that never fires. */
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}
static HRESULT WINAPI sc_remove_scale(raven_swapchain_panel *iface, EventRegistrationToken token)
{ (void)iface; (void)token; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI sc_input(raven_swapchain_panel *iface,
	INT32 types, __x_ABI_CWindows_CUI_CCore_CICoreInputSourceBase **out)
{
	(void)iface; (void)types;
	if (!out) return E_POINTER;
	*out = NULL;
	/* There is no independent GPU input surface or input worker. */
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}
static HRESULT WINAPI scn_query(raven_swapchain_native *iface, REFIID iid, void **out)
{ return swapchain_query(((struct swapchain_native_part *)iface)->owner, iid, out); }
static ULONG WINAPI scn_addref(raven_swapchain_native *iface)
{ return InterlockedIncrement(&((struct swapchain_native_part *)iface)->owner->refs); }
static ULONG WINAPI scn_release(raven_swapchain_native *iface)
{ return swapchain_release(((struct swapchain_native_part *)iface)->owner); }
static HRESULT WINAPI scn_set(raven_swapchain_native *iface, IDXGISwapChain *chain)
{
	struct swapchain_panel *self = ((struct swapchain_native_part *)iface)->owner;
	if (self->thread != GetCurrentThreadId()) return RPC_E_WRONG_THREAD;
	/* Attach: store the composition swapchain the way the internal contract storage does. The presentation
	 * path is the one the composition swapchain already carries: the app's render loop Presents into the
	 * DXVK dummy window (a raven-known hwnd whose host_surface backend owns the Wayland surface), so frames
	 * flow once the caller renders — the panel owns no compositor of its own and invents no pixels. */
	if (chain) chain->lpVtbl->AddRef(chain);
	IDXGISwapChain *old = self->stored;
	self->stored = chain;
	if (old) old->lpVtbl->Release(old);
	{
		char note[96];
		wsprintfA(note, "raven-xaml: swapchain panel %s %p\n", chain ? "attached" : "detached", (void *)chain);
		raven_trace_note(note);
	}
	return S_OK;
}
static const raven_swapchain_panel_vtbl sc_table = {
	sc_query, sc_addref, sc_release, sc_iids, sc_name, sc_trust,
	sc_scale, sc_scale, sc_add_scale, sc_remove_scale, sc_input,
};
static const raven_swapchain_native_vtbl scn_table = { scn_query, scn_addref, scn_release, scn_set };
static IInspectable *create_swapchain_panel(void)
{
	struct swapchain_panel *self = ALLOC(sizeof(*self));
	if (!self) return NULL;
	self->panel.lpVtbl = &sc_table; self->panel.owner = self;
	self->native.lpVtbl = &scn_table; self->native.owner = self;
	self->refs = 1; self->thread = GetCurrentThreadId();
	InterlockedIncrement(&raven_xaml_objects);
	return (IInspectable *)&self->panel;
}
/* Engine-internal contract storage, explicitly NOT native SetSwapChain or a WinRT property.
 * Retaining and returning a DXGI reference says nothing about whether its pixels can be shown. */
HRESULT __declspec(dllexport) raven_xaml_swapchain_panel_store(raven_swapchain_panel *iface, IDXGISwapChain *chain)
{
	if (!iface) return E_POINTER;
	if (iface->lpVtbl != &sc_table) { RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
	struct swapchain_panel *self = ((struct swapchain_part *)iface)->owner;
	if (self->thread != GetCurrentThreadId()) return RPC_E_WRONG_THREAD;
	if (chain) chain->lpVtbl->AddRef(chain);
	IDXGISwapChain *old = self->stored;
	self->stored = chain;
	if (old) old->lpVtbl->Release(old);
	return S_OK;
}
HRESULT __declspec(dllexport) raven_xaml_swapchain_panel_get(raven_swapchain_panel *iface, IDXGISwapChain **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!iface) return E_POINTER;
	if (iface->lpVtbl != &sc_table) { RAVEN_XAML_INVALIDARG(); return E_INVALIDARG; }
	struct swapchain_panel *self = ((struct swapchain_part *)iface)->owner;
	if (self->thread != GetCurrentThreadId()) return RPC_E_WRONG_THREAD;
	*out = self->stored;
	if (*out) (*out)->lpVtbl->AddRef(*out);
	return S_OK;
}

enum factory_kind { factory_frame, factory_page, factory_template, factory_swapchain, factory_control, factory_usercontrol, factory_contentdialog };

struct controls_factory;
struct controls_part { const IActivationFactoryVtbl *lpVtbl; struct controls_factory *owner; };
struct controls_part1 { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStaticsVtbl *lpVtbl; struct controls_factory *owner; };
struct controls_part2 { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2Vtbl *lpVtbl; struct controls_factory *owner; };
struct controls_part5 { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5Vtbl *lpVtbl; struct controls_factory *owner; };
struct controls_page_statics { const __x_ABI_CWindows_CUI_CXaml_CControls_CIPageStaticsVtbl *lpVtbl; struct controls_factory *owner; };
struct controls_frame_create { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactoryVtbl *lpVtbl; struct controls_factory *owner; };
struct controls_page_create { const __x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactoryVtbl *lpVtbl; struct controls_factory *owner; };
struct controls_control_create { const control_factory_vtbl *lpVtbl; struct controls_factory *owner; };
struct controls_control_statics { const control_statics_vtbl *lpVtbl; struct controls_factory *owner; };
struct controls_usercontrol_create { const usercontrol_factory_vtbl *lpVtbl; struct controls_factory *owner; };
struct controls_usercontrol_statics { const usercontrol_statics_vtbl *lpVtbl; struct controls_factory *owner; };
struct controls_contentdialog_create { const contentdialog_factory_vtbl *lpVtbl; struct controls_factory *owner; };

struct controls_factory {
	struct controls_part activation;
	struct controls_part1 statics1;
	struct controls_part2 statics2;
	struct controls_part5 statics5;
	struct controls_page_statics page_statics;
	struct controls_frame_create frame_create;
	struct controls_page_create page_create;
	struct controls_control_create control_create;
	struct controls_control_statics control_statics;
	struct controls_usercontrol_create usercontrol_create;
	struct controls_usercontrol_statics usercontrol_statics;
	struct controls_contentdialog_create contentdialog_create;
	LONG refs;
	enum factory_kind kind;
};

static HRESULT controls_query_parts(struct controls_factory *self, REFIID iid, void **out);

#define CONTROLS_PART(part, iface, prefix) \
	static HRESULT WINAPI prefix##_query(iface *iface##_unused, REFIID iid, void **out) \
	{ return controls_query_parts(((struct part *)iface##_unused)->owner, iid, out); } \
	static ULONG WINAPI prefix##_addref(iface *iface##_unused) \
	{ return (ULONG)InterlockedIncrement(&((struct part *)iface##_unused)->owner->refs); } \
	static ULONG WINAPI prefix##_release(iface *iface##_unused) \
	{ return (ULONG)InterlockedDecrement(&((struct part *)iface##_unused)->owner->refs); } \
	static HRESULT WINAPI prefix##_iids(iface *iface##_unused, ULONG *count, IID **iids) \
	{ (void)iface##_unused; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI prefix##_name(iface *iface##_unused, HSTRING *name) \
	{ \
		struct controls_factory *self = ((struct part *)iface##_unused)->owner; \
		const WCHAR *text = self->kind == factory_frame ? frame_class : \
			self->kind == factory_page ? page_class : \
			self->kind == factory_control ? control_class : \
			self->kind == factory_usercontrol ? usercontrol_class : \
			self->kind == factory_swapchain ? swapchain_class : template_class; \
		if (!name) return E_POINTER; \
		return WindowsCreateString(text, (UINT32)lstrlenW(text), name); \
	} \
	static HRESULT WINAPI prefix##_trust(iface *iface##_unused, TrustLevel *level) \
	{ (void)iface##_unused; return inspectable_trust(level); }

CONTROLS_PART(controls_part1, frame_statics1, ctl1)
CONTROLS_PART(controls_part2, frame_statics2, ctl2)
CONTROLS_PART(controls_part5, frame_statics5, ctl5)
CONTROLS_PART(controls_page_statics, page_statics, ctlp)
CONTROLS_PART(controls_frame_create, frame_factory, ctlf)
CONTROLS_PART(controls_page_create, page_factory, ctlpg)
CONTROLS_PART(controls_control_create, control_factory_i, ctlcf)
CONTROLS_PART(controls_control_statics, control_statics_i, ctlcs)
CONTROLS_PART(controls_usercontrol_create, usercontrol_factory_i, ctlucf)
CONTROLS_PART(controls_usercontrol_statics, usercontrol_statics_i, ctlucs)
CONTROLS_PART(controls_contentdialog_create, contentdialog_factory_i, ctlcdf)

static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out);
static HRESULT WINAPI factory_query(IActivationFactory *iface, REFIID iid, void **out);
static ULONG WINAPI factory_addref(IActivationFactory *iface);
static ULONG WINAPI factory_release(IActivationFactory *iface);
static HRESULT WINAPI factory_iids(IActivationFactory *iface, ULONG *count, IID **iids);
static HRESULT WINAPI factory_name(IActivationFactory *iface, HSTRING *name);
static HRESULT WINAPI factory_trust(IActivationFactory *iface, TrustLevel *level);
static HRESULT WINAPI frame_create(frame_factory *iface, uint64_t outer, uint64_t inner, uint64_t instance);
static HRESULT WINAPI page_create(page_factory *iface, uint64_t outer, uint64_t inner, uint64_t instance);

STUB1(page_statics, page_prop_frame)
STUB1(page_statics, page_prop_top_app_bar)
STUB1(page_statics, page_prop_bottom_app_bar)

static const IActivationFactoryVtbl activation_table = {
	.QueryInterface = factory_query,
	.AddRef = factory_addref,
	.Release = factory_release,
	.GetIids = factory_iids,
	.GetRuntimeClassName = factory_name,
	.GetTrustLevel = factory_trust,
	.ActivateInstance = factory_activate,
};

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStaticsVtbl frame_statics1_table = {
	.QueryInterface = ctl1_query, .AddRef = ctl1_addref, .Release = ctl1_release,
	.GetIids = ctl1_iids, .GetRuntimeClassName = ctl1_name, .GetTrustLevel = ctl1_trust,
	.get_CacheSizeProperty = frame_prop_cache,
	.get_CanGoBackProperty = frame_prop_can_back,
	.get_CanGoForwardProperty = frame_prop_can_forward,
	.get_CurrentSourcePageTypeProperty = frame_prop_current_type,
	.get_SourcePageTypeProperty = frame_prop_source_type,
	.get_BackStackDepthProperty = frame_prop_depth,
};

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2Vtbl frame_statics2_table = {
	.QueryInterface = ctl2_query, .AddRef = ctl2_addref, .Release = ctl2_release,
	.GetIids = ctl2_iids, .GetRuntimeClassName = ctl2_name, .GetTrustLevel = ctl2_trust,
	.get_BackStackProperty = frame_prop_back_stack,
	.get_ForwardStackProperty = frame_prop_forward_stack,
};

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5Vtbl frame_statics5_table = {
	.QueryInterface = ctl5_query, .AddRef = ctl5_addref, .Release = ctl5_release,
	.GetIids = ctl5_iids, .GetRuntimeClassName = ctl5_name, .GetTrustLevel = ctl5_trust,
	.get_IsNavigationStackEnabledProperty = frame_prop_stack_enabled,
};

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIPageStaticsVtbl page_statics_table = {
	.QueryInterface = ctlp_query, .AddRef = ctlp_addref, .Release = ctlp_release,
	.GetIids = ctlp_iids, .GetRuntimeClassName = ctlp_name, .GetTrustLevel = ctlp_trust,
	.get_FrameProperty = page_prop_frame,
	.get_TopAppBarProperty = page_prop_top_app_bar,
	.get_BottomAppBarProperty = page_prop_bottom_app_bar,
};

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactoryVtbl frame_create_table = {
	.QueryInterface = ctlf_query, .AddRef = ctlf_addref, .Release = ctlf_release,
	.GetIids = ctlf_iids, .GetRuntimeClassName = ctlf_name, .GetTrustLevel = ctlf_trust,
	.CreateInstance = frame_create,
};

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactoryVtbl page_create_table = {
	.QueryInterface = ctlpg_query, .AddRef = ctlpg_addref, .Release = ctlpg_release,
	.GetIids = ctlpg_iids, .GetRuntimeClassName = ctlpg_name, .GetTrustLevel = ctlpg_trust,
	.CreateInstance = page_create,
};

/* Control's dependency properties: the statics the WinUI projection reads while composing its controls over the
 * base Control. Each registers once through the engine's DependencyProperty store (the duplicate contract refuses
 * a second registration) and the object is cached for the factory's lifetime; the reads are main-thread
 * activation work. The types are the forms the compiled registrations carry. */
struct control_property_entry { const WCHAR *name; const WCHAR *type; IInspectable *object; };
static struct control_property_entry control_properties[] = {
	{L"FontSize", L"Double", NULL},
	{L"FontFamily", L"Windows.UI.Xaml.Media.FontFamily", NULL},
	{L"FontWeight", L"Windows.UI.Text.FontWeight", NULL},
	{L"FontStyle", L"Windows.UI.Text.FontStyle", NULL},
	{L"FontStretch", L"Windows.UI.Text.FontStretch", NULL},
	{L"CharacterSpacing", L"Int32", NULL},
	{L"Foreground", L"Windows.UI.Xaml.Media.Brush", NULL},
	{L"IsTabStop", L"Boolean", NULL},
	{L"IsEnabled", L"Boolean", NULL},
	{L"TabIndex", L"Int32", NULL},
	{L"TabNavigation", L"Windows.UI.Xaml.Input.KeyboardNavigationMode", NULL},
	{L"Template", L"Windows.UI.Xaml.Controls.ControlTemplate", NULL},
	{L"Padding", L"Windows.UI.Xaml.Thickness", NULL},
	{L"HorizontalContentAlignment", L"Windows.UI.Xaml.HorizontalAlignment", NULL},
	{L"VerticalContentAlignment", L"Windows.UI.Xaml.VerticalAlignment", NULL},
	{L"Background", L"Windows.UI.Xaml.Media.Brush", NULL},
	{L"BorderThickness", L"Windows.UI.Xaml.Thickness", NULL},
	{L"BorderBrush", L"Windows.UI.Xaml.Media.Brush", NULL},
	{L"DefaultStyleKey", L"Object", NULL},
	{L"FocusState", L"Windows.UI.Xaml.FocusState", NULL},
};
static HRESULT control_property_static(const WCHAR *name, IInspectable **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	for (size_t index = 0; index < ARRAYSIZE(control_properties); ++index) {
		struct control_property_entry *entry = &control_properties[index];
		if (CompareStringOrdinal(entry->name, -1, name, -1, FALSE) != CSTR_EQUAL)
			continue;
		if (!entry->object) {
			const HRESULT made = raven_xaml_property_register(entry->name, (UINT32)lstrlenW(entry->name),
				entry->type, (UINT32)lstrlenW(entry->type), control_class, (UINT32)lstrlenW(control_class),
				&entry->object);
			if (FAILED(made) || !entry->object)
				return FAILED(made) ? made : E_OUTOFMEMORY;
		}
		*out = entry->object;
		(*out)->lpVtbl->AddRef(*out);
		return S_OK;
	}
	return E_NOTIMPL;
}
#define CONTROL_PROPERTY(getter, NAME) \
	static HRESULT WINAPI getter(control_statics_i *iface, uint64_t out) \
	{ (void)iface; return control_property_static(NAME, (IInspectable **)(uintptr_t)out); }
CONTROL_PROPERTY(control_prop_font_size, L"FontSize")
CONTROL_PROPERTY(control_prop_font_family, L"FontFamily")
CONTROL_PROPERTY(control_prop_font_weight, L"FontWeight")
CONTROL_PROPERTY(control_prop_font_style, L"FontStyle")
CONTROL_PROPERTY(control_prop_font_stretch, L"FontStretch")
CONTROL_PROPERTY(control_prop_character_spacing, L"CharacterSpacing")
CONTROL_PROPERTY(control_prop_foreground, L"Foreground")
CONTROL_PROPERTY(control_prop_is_tab_stop, L"IsTabStop")
CONTROL_PROPERTY(control_prop_is_enabled, L"IsEnabled")
CONTROL_PROPERTY(control_prop_tab_index, L"TabIndex")
CONTROL_PROPERTY(control_prop_tab_navigation, L"TabNavigation")
CONTROL_PROPERTY(control_prop_template, L"Template")
CONTROL_PROPERTY(control_prop_padding, L"Padding")
CONTROL_PROPERTY(control_prop_horizontal_alignment, L"HorizontalContentAlignment")
CONTROL_PROPERTY(control_prop_vertical_alignment, L"VerticalContentAlignment")
CONTROL_PROPERTY(control_prop_background, L"Background")
CONTROL_PROPERTY(control_prop_border_thickness, L"BorderThickness")
CONTROL_PROPERTY(control_prop_border_brush, L"BorderBrush")
CONTROL_PROPERTY(control_prop_default_style_key, L"DefaultStyleKey")
CONTROL_PROPERTY(control_prop_focus_state, L"FocusState")

static const control_statics_vtbl control_statics_table = {
	.QueryInterface = ctlcs_query, .AddRef = ctlcs_addref, .Release = ctlcs_release,
	.GetIids = ctlcs_iids, .GetRuntimeClassName = ctlcs_name, .GetTrustLevel = ctlcs_trust,
	.get_FontSizeProperty = control_prop_font_size,
	.get_FontFamilyProperty = control_prop_font_family,
	.get_FontWeightProperty = control_prop_font_weight,
	.get_FontStyleProperty = control_prop_font_style,
	.get_FontStretchProperty = control_prop_font_stretch,
	.get_CharacterSpacingProperty = control_prop_character_spacing,
	.get_ForegroundProperty = control_prop_foreground,
	.get_IsTabStopProperty = control_prop_is_tab_stop,
	.get_IsEnabledProperty = control_prop_is_enabled,
	.get_TabIndexProperty = control_prop_tab_index,
	.get_TabNavigationProperty = control_prop_tab_navigation,
	.get_TemplateProperty = control_prop_template,
	.get_PaddingProperty = control_prop_padding,
	.get_HorizontalContentAlignmentProperty = control_prop_horizontal_alignment,
	.get_VerticalContentAlignmentProperty = control_prop_vertical_alignment,
	.get_BackgroundProperty = control_prop_background,
	.get_BorderThicknessProperty = control_prop_border_thickness,
	.get_BorderBrushProperty = control_prop_border_brush,
	.get_DefaultStyleKeyProperty = control_prop_default_style_key,
	.get_FocusStateProperty = control_prop_focus_state,
};

/* The composable Control creation the WinUI dependency drives: the engine's Control surface rides the frame
 * machinery, aggregated with the WinUI object as the outer exactly the way the frame's own composable creation
 * aggregates with the managed projection. */
static HRESULT WINAPI control_create(control_factory_i *iface, uint64_t outer, uint64_t inner, uint64_t instance)
{
	struct frame *frame;
	(void)iface;
	if (!instance || (outer && !inner))
		return E_POINTER;
	*(void **)(uintptr_t)instance = NULL;
	if (inner)
		*(void **)(uintptr_t)inner = NULL;
	if (!(frame = create_frame()))
		return E_OUTOFMEMORY;
	frame->standalone_control = TRUE;
	frame->outer = (IInspectable *)(uintptr_t)outer;
	if (outer) {
		*(void **)(uintptr_t)inner = &frame->inner;
		IInspectable_AddRef(frame->outer);
	}
	*(void **)(uintptr_t)instance = &frame->first;
	return S_OK;
}

static const control_factory_vtbl control_create_table = {
	.QueryInterface = ctlcf_query, .AddRef = ctlcf_addref, .Release = ctlcf_release,
	.GetIids = ctlcf_iids, .GetRuntimeClassName = ctlcf_name, .GetTrustLevel = ctlcf_trust,
	.CreateInstance = control_create,
};

/* UserControl's Content dependency property, registered through the engine's DependencyProperty store the way
 * Control's statics register theirs; the type is the form the compiled registrations carry. */
static HRESULT usercontrol_content_property_static(IInspectable **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	static IInspectable *content_property = NULL;
	if (!content_property) {
		const HRESULT made = raven_xaml_property_register(L"Content", (UINT32)lstrlenW(L"Content"),
			L"Object", (UINT32)lstrlenW(L"Object"), usercontrol_class, (UINT32)lstrlenW(usercontrol_class),
			&content_property);
		if (FAILED(made) || !content_property)
			return FAILED(made) ? made : E_OUTOFMEMORY;
	}
	*out = content_property;
	(*out)->lpVtbl->AddRef(*out);
	return S_OK;
}
static HRESULT WINAPI usercontrol_prop_content(usercontrol_statics_i *iface, uint64_t out)
{ (void)iface; return usercontrol_content_property_static((IInspectable **)(uintptr_t)out); }

static const usercontrol_statics_vtbl usercontrol_statics_table = {
	.QueryInterface = ctlucs_query, .AddRef = ctlucs_addref, .Release = ctlucs_release,
	.GetIids = ctlucs_iids, .GetRuntimeClassName = ctlucs_name, .GetTrustLevel = ctlucs_trust,
	.get_ContentProperty = usercontrol_prop_content,
};

/* The composable UserControl creation the .NET Native projection of an app's UserControl-derived class composes
 * with: the same aggregation the frame's and Control's factories use, with the default IUserControl face as the
 * instance answer and the object named the UserControl class. */
static HRESULT WINAPI usercontrol_create(usercontrol_factory_i *iface, uint64_t outer, uint64_t inner, uint64_t instance)
{
	struct frame *frame;
	(void)iface;
	if (!instance || (outer && !inner))
		return E_POINTER;
	*(void **)(uintptr_t)instance = NULL;
	if (inner)
		*(void **)(uintptr_t)inner = NULL;
	if (!(frame = create_frame()))
		return E_OUTOFMEMORY;
	frame->standalone_control = TRUE;
	frame->standalone_usercontrol = TRUE;
	frame->outer = (IInspectable *)(uintptr_t)outer;
	if (outer) {
		*(void **)(uintptr_t)inner = &frame->inner;
		IInspectable_AddRef(frame->outer);
	}
	*(void **)(uintptr_t)instance = &frame->usercontrol;
	return S_OK;
}

static const usercontrol_factory_vtbl usercontrol_create_table = {
	.QueryInterface = ctlucf_query, .AddRef = ctlucf_addref, .Release = ctlucf_release,
	.GetIids = ctlucf_iids, .GetRuntimeClassName = ctlucf_name, .GetTrustLevel = ctlucf_trust,
	.CreateInstance = usercontrol_create,
};

/* The composable ContentDialog creation the .NET Native projection drives: RoGetActivationFactory answers the
 * IContentDialogFactory face and the constructor composes the instance with the managed CCW as the outer, the
 * aggregation the Control/UserControl factories already carry. The dialog names itself the ContentDialog class
 * and keeps the whole Control base surface; its own face joins when the projection's cast IID is on record. */
static HRESULT WINAPI contentdialog_create(contentdialog_factory_i *iface, uint64_t outer, uint64_t inner, uint64_t instance)
{
	struct frame *frame;
	(void)iface;
	if (!instance || (outer && !inner))
		return E_POINTER;
	*(void **)(uintptr_t)instance = NULL;
	if (inner)
		*(void **)(uintptr_t)inner = NULL;
	if (!(frame = create_frame()))
		return E_OUTOFMEMORY;
	frame->standalone_control = TRUE;
	frame->standalone_contentdialog = TRUE;
	InitializeCriticalSection(&frame->dialog_closing.lock);
	InitializeCriticalSection(&frame->dialog_closed.lock);
	InitializeCriticalSection(&frame->dialog_opened.lock);
	InitializeCriticalSection(&frame->dialog_primary_click.lock);
	InitializeCriticalSection(&frame->dialog_secondary_click.lock);
	InitializeCriticalSection(&frame->dialog_close_click.lock);
	frame->dialog_is_primary_enabled = 1;
	frame->dialog_is_secondary_enabled = 1;
	frame->outer = (IInspectable *)(uintptr_t)outer;
	if (outer) {
		*(void **)(uintptr_t)inner = &frame->inner;
		IInspectable_AddRef(frame->outer);
	}
	*(void **)(uintptr_t)instance = &frame->first;
	return S_OK;
}

static const contentdialog_factory_vtbl contentdialog_create_table = {
	.QueryInterface = ctlcdf_query, .AddRef = ctlcdf_addref, .Release = ctlcdf_release,
	.GetIids = ctlcdf_iids, .GetRuntimeClassName = ctlcdf_name, .GetTrustLevel = ctlcdf_trust,
	.CreateInstance = contentdialog_create,
};

static struct controls_factory frame_factory_instance = {
	.activation = {&activation_table, NULL}, .statics1 = {&frame_statics1_table, NULL},
	.statics2 = {&frame_statics2_table, NULL}, .statics5 = {&frame_statics5_table, NULL},
	.frame_create = {&frame_create_table, NULL},
	.refs = 1, .kind = factory_frame,
};

static struct controls_factory page_factory_instance = {
	.activation = {&activation_table, NULL}, .page_statics = {&page_statics_table, NULL},
	.page_create = {&page_create_table, NULL},
	.refs = 1, .kind = factory_page,
};

static struct controls_factory template_factory_instance = {
	.activation = {&activation_table, NULL},
	.refs = 1, .kind = factory_template,
};

static struct controls_factory swapchain_factory_instance = {
	.activation = {&activation_table, NULL},
	.refs = 1, .kind = factory_swapchain,
};

static struct controls_factory control_factory_instance = {
	.activation = {&activation_table, NULL}, .control_create = {&control_create_table, NULL},
	.control_statics = {&control_statics_table, NULL},
	.refs = 1, .kind = factory_control,
};

static struct controls_factory usercontrol_factory_instance = {
	.activation = {&activation_table, NULL}, .usercontrol_create = {&usercontrol_create_table, NULL},
	.usercontrol_statics = {&usercontrol_statics_table, NULL},
	.refs = 1, .kind = factory_usercontrol,
};

static struct controls_factory contentdialog_factory_instance = {
	.activation = {&activation_table, NULL},
	.contentdialog_create = {&contentdialog_create_table, NULL},
	.refs = 1, .kind = factory_contentdialog,
};
static LONG factories_ready;

/* A static initializer cannot name the object being defined, so the back pointers are filled once, before first use. */
static void prepare_factories(void)
{
	if (InterlockedCompareExchange(&factories_ready, 1, 0) == 0) {
		swapchain_factory_instance.activation.owner = &swapchain_factory_instance;
		frame_factory_instance.activation.owner = &frame_factory_instance;
		frame_factory_instance.statics1.owner = &frame_factory_instance;
		frame_factory_instance.statics2.owner = &frame_factory_instance;
		frame_factory_instance.statics5.owner = &frame_factory_instance;
		frame_factory_instance.page_statics.owner = &frame_factory_instance;
		frame_factory_instance.frame_create.owner = &frame_factory_instance;
		frame_factory_instance.page_create.owner = &frame_factory_instance;
		page_factory_instance.activation.owner = &page_factory_instance;
		page_factory_instance.statics1.owner = &page_factory_instance;
		page_factory_instance.statics2.owner = &page_factory_instance;
		page_factory_instance.statics5.owner = &page_factory_instance;
		page_factory_instance.page_statics.owner = &page_factory_instance;
		page_factory_instance.frame_create.owner = &page_factory_instance;
		page_factory_instance.page_create.owner = &page_factory_instance;
		template_factory_instance.activation.owner = &template_factory_instance;
		template_factory_instance.statics1.owner = &template_factory_instance;
		template_factory_instance.statics2.owner = &template_factory_instance;
		template_factory_instance.statics5.owner = &template_factory_instance;
		template_factory_instance.page_statics.owner = &template_factory_instance;
		template_factory_instance.frame_create.owner = &template_factory_instance;
		template_factory_instance.page_create.owner = &template_factory_instance;
		swapchain_factory_instance.statics1.owner = &swapchain_factory_instance;
		swapchain_factory_instance.statics2.owner = &swapchain_factory_instance;
		swapchain_factory_instance.statics5.owner = &swapchain_factory_instance;
		swapchain_factory_instance.page_statics.owner = &swapchain_factory_instance;
		swapchain_factory_instance.frame_create.owner = &swapchain_factory_instance;
		swapchain_factory_instance.page_create.owner = &swapchain_factory_instance;
		control_factory_instance.activation.owner = &control_factory_instance;
		control_factory_instance.statics1.owner = &control_factory_instance;
		control_factory_instance.statics2.owner = &control_factory_instance;
		control_factory_instance.statics5.owner = &control_factory_instance;
		control_factory_instance.page_statics.owner = &control_factory_instance;
		control_factory_instance.frame_create.owner = &control_factory_instance;
		control_factory_instance.page_create.owner = &control_factory_instance;
		control_factory_instance.control_create.owner = &control_factory_instance;
		control_factory_instance.control_statics.owner = &control_factory_instance;
		usercontrol_factory_instance.activation.owner = &usercontrol_factory_instance;
		usercontrol_factory_instance.statics1.owner = &usercontrol_factory_instance;
		usercontrol_factory_instance.statics2.owner = &usercontrol_factory_instance;
		usercontrol_factory_instance.statics5.owner = &usercontrol_factory_instance;
		usercontrol_factory_instance.page_statics.owner = &usercontrol_factory_instance;
		usercontrol_factory_instance.frame_create.owner = &usercontrol_factory_instance;
		usercontrol_factory_instance.page_create.owner = &usercontrol_factory_instance;
		usercontrol_factory_instance.control_create.owner = &usercontrol_factory_instance;
		usercontrol_factory_instance.control_statics.owner = &usercontrol_factory_instance;
		usercontrol_factory_instance.usercontrol_create.owner = &usercontrol_factory_instance;
		usercontrol_factory_instance.usercontrol_statics.owner = &usercontrol_factory_instance;
		contentdialog_factory_instance.activation.owner = &contentdialog_factory_instance;
		contentdialog_factory_instance.statics1.owner = &contentdialog_factory_instance;
		contentdialog_factory_instance.statics2.owner = &contentdialog_factory_instance;
		contentdialog_factory_instance.statics5.owner = &contentdialog_factory_instance;
		contentdialog_factory_instance.page_statics.owner = &contentdialog_factory_instance;
		contentdialog_factory_instance.frame_create.owner = &contentdialog_factory_instance;
		contentdialog_factory_instance.page_create.owner = &contentdialog_factory_instance;
		contentdialog_factory_instance.control_create.owner = &contentdialog_factory_instance;
		contentdialog_factory_instance.control_statics.owner = &contentdialog_factory_instance;
		contentdialog_factory_instance.contentdialog_create.owner = &contentdialog_factory_instance;
		InterlockedExchange(&factories_ready, 2);
	} else {
		while (InterlockedCompareExchange(&factories_ready, 2, 2) != 2)
			Sleep(0);
	}
}

static HRESULT controls_query_parts(struct controls_factory *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory))
		*out = &self->activation;
	else if (self->kind == factory_frame && equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics))
		*out = &self->statics1;
	else if (self->kind == factory_frame && equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2))
		*out = &self->statics2;
	else if (self->kind == factory_frame && equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5))
		*out = &self->statics5;
	else if (self->kind == factory_frame && equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactory))
		*out = &self->frame_create;
	else if (self->kind == factory_page && equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPageStatics))
		*out = &self->page_statics;
	else if (self->kind == factory_page && equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactory))
		*out = &self->page_create;
	else if (self->kind == factory_control && equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlFactory))
		*out = &self->control_create;
	else if (self->kind == factory_control && equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlStatics))
		*out = &self->control_statics;
	else if (self->kind == factory_usercontrol && equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CIUserControlFactory))
		*out = &self->usercontrol_create;
	else if (self->kind == factory_usercontrol && equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CIUserControlStatics))
		*out = &self->usercontrol_statics;
	else if (self->kind == factory_contentdialog && equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialogFactory))
		*out = &self->contentdialog_create;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static HRESULT WINAPI factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
	return controls_query_parts(((struct controls_part *)iface)->owner, iid, out);
}

static ULONG WINAPI factory_addref(IActivationFactory *iface)
{
	return (ULONG)InterlockedIncrement(&((struct controls_part *)iface)->owner->refs);
}

static ULONG WINAPI factory_release(IActivationFactory *iface)
{
	return (ULONG)InterlockedDecrement(&((struct controls_part *)iface)->owner->refs);
}

static HRESULT WINAPI factory_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI factory_name(IActivationFactory *iface, HSTRING *name)
{
	struct controls_factory *self = ((struct controls_part *)iface)->owner;
	const WCHAR *text = self->kind == factory_frame ? frame_class :
		self->kind == factory_page ? page_class :
		self->kind == factory_swapchain ? swapchain_class :
		self->kind == factory_contentdialog ? contentdialog_class : template_class;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), name);
}

static HRESULT WINAPI factory_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out)
{
	struct controls_factory *self = ((struct controls_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (self->kind == factory_frame) {
		struct frame *frame;
		raven_trace_note("raven-xaml: frame ActivateInstance entered\n");
		frame = create_frame();
		if (!frame)
			return E_OUTOFMEMORY;
		*out = (IInspectable *)&frame->first;
		raven_trace_note("raven-xaml: frame activated\n");
	} else if (self->kind == factory_page) {
		struct page *page = create_page();
		if (!page)
			return E_OUTOFMEMORY;
		*out = (IInspectable *)page;
	} else if (self->kind == factory_swapchain) {
		*out = create_swapchain_panel();
		if (!*out) return E_OUTOFMEMORY;
	} else if (self->kind == factory_control) {
		struct frame *frame = create_frame();
		if (!frame)
			return E_OUTOFMEMORY;
		frame->standalone_control = TRUE;
		*out = (IInspectable *)&frame->first;
	} else if (self->kind == factory_usercontrol) {
		struct frame *frame = create_frame();
		if (!frame)
			return E_OUTOFMEMORY;
		frame->standalone_control = TRUE;
		frame->standalone_usercontrol = TRUE;
		*out = (IInspectable *)&frame->usercontrol;
	} else if (self->kind == factory_contentdialog) {
		struct frame *frame = create_frame();
		if (!frame)
			return E_OUTOFMEMORY;
		/* The dialog rides the Control surface (the IControl scalar storage the projected constructor sets)
		 * and names itself the ContentDialog class; the IContentDialog face joins once its IID is on record.
		 * The event lists carry the same critical sections contentdialog_create initializes — the x64
		 * controls fixture deadlocked in raven_event_close when this activation path skipped them. */
		InitializeCriticalSection(&frame->dialog_closing.lock);
		InitializeCriticalSection(&frame->dialog_closed.lock);
		InitializeCriticalSection(&frame->dialog_opened.lock);
		InitializeCriticalSection(&frame->dialog_primary_click.lock);
		InitializeCriticalSection(&frame->dialog_secondary_click.lock);
		InitializeCriticalSection(&frame->dialog_close_click.lock);
		frame->dialog_is_primary_enabled = 1;
		frame->dialog_is_secondary_enabled = 1;
		frame->standalone_control = TRUE;
		frame->standalone_contentdialog = TRUE;
		*out = (IInspectable *)&frame->first;
	} else {
		struct control_template *template = create_control_template();
		if (!template)
			return E_OUTOFMEMORY;
		*out = (IInspectable *)template;
	}
	return S_OK;
}

/* A composed frame exposes its nondelegating inner identity to its outer object. */
static HRESULT WINAPI frame_create(frame_factory *iface, uint64_t outer, uint64_t inner, uint64_t instance)
{
	struct frame *frame;
	(void)iface;
	raven_trace_note("raven-xaml: frame CreateInstance entered\n");
	if (!instance || (outer && !inner))
		return E_POINTER;
	*(void **)(uintptr_t)instance = NULL;
	if (inner)
		*(void **)(uintptr_t)inner = NULL;
	if (!(frame = create_frame()))
		return E_OUTOFMEMORY;
	frame->outer=(IInspectable *)(uintptr_t)outer;
	if (outer) {
		*(void **)(uintptr_t)inner=&frame->inner;
		IInspectable_AddRef(frame->outer);
	}
	*(void **)(uintptr_t)instance = &frame->first;
	return S_OK;
}

static HRESULT WINAPI page_create(page_factory *iface, uint64_t outer, uint64_t inner, uint64_t instance)
{
	struct frame *frame;
	(void)iface;
	if (!instance || (outer && !inner))
		return E_POINTER;
	*(void **)(uintptr_t)instance = NULL;
	if (inner)
		*(void **)(uintptr_t)inner = NULL;
	/* The projected page composes over the frame machinery like the UserControl does: the CCW's interface
	 * walk (IUIElement, IFrameworkElement and family, IContentControl) is what the bare page struct could
	 * not answer, and the cast failures it threw (WelcomePage to EEType 0x1af60/0x1afa0 — IPage and
	 * IPageOverrides) stopped the app's first page construction. The instance face is IPage; the object
	 * names itself the Page class; the page-specific surface rides the frame's page faces. */
	if (!(frame = create_frame()))
		return E_OUTOFMEMORY;
	frame->standalone_control = TRUE;
	frame->standalone_page = TRUE;
	frame->outer = (IInspectable *)(uintptr_t)outer;
	if (outer) {
		*(void **)(uintptr_t)inner = &frame->inner;
		IInspectable_AddRef(frame->outer);
	}
	*(void **)(uintptr_t)instance = &frame->page_face;
	return S_OK;
}

static int same_text(const WCHAR *text, UINT32 length, const WCHAR *name)
{
	return text && CompareStringOrdinal(text, (int)length, name, -1, FALSE) == CSTR_EQUAL;
}

static void store_name(HSTRING *slot, const WCHAR *text, UINT32 length)
{
	if (*slot) {
		WindowsDeleteString(*slot);
		*slot = NULL;
	}
	if (text && length)
		WindowsCreateString(text, length, slot);
}

HRESULT raven_xaml_element_set_name(IInspectable *object, const WCHAR *text, UINT32 length)
{
	frame1 *frame = NULL;
	page1 *page = NULL;
	if (!object)
		return E_POINTER;
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame, (void **)&frame)) && frame) {
		if (frame->lpVtbl == &frame1_table)
			store_name(&((struct frame1_part *)frame)->owner->name, text, length);
		frame->lpVtbl->Release(frame);
		return S_OK;
	}
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPage, (void **)&page)) && page) {
		if (page->lpVtbl == &page_table)
			store_name(&((struct page *)page)->name, text, length);
		page->lpVtbl->Release(page);
		return S_OK;
	}
	if (raven_xaml_elements_set_name(object, text, length) == S_OK)
		return S_OK;
	return raven_xaml_window_set_name(object, text, length);
}

HRESULT raven_xaml_element_set_integer(IInspectable *object, const WCHAR *name, UINT32 length, int64_t value)
{
	frame1 *frame = NULL;
	page1 *page = NULL;
	frame5 *fifth = NULL;
	if (!object)
		return E_POINTER;
	if (same_text(name, length, L"CacheSize")) {
		if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame, (void **)&frame)) && frame) {
			const HRESULT result = frame->lpVtbl->put_CacheSize(frame, (uint64_t)(int32_t)value);
			frame->lpVtbl->Release(frame);
			return result;
		}
		return S_FALSE;
	}
	if (same_text(name, length, L"NavigationCacheMode")) {
		if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPage, (void **)&page)) && page) {
			const HRESULT result = page->lpVtbl->put_NavigationCacheMode(page, (uint64_t)(int32_t)value);
			page->lpVtbl->Release(page);
			return result;
		}
		return S_FALSE;
	}
	if (same_text(name, length, L"IsNavigationStackEnabled")) {
		if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5, (void **)&fifth)) && fifth) {
			const HRESULT result = fifth->lpVtbl->put_IsNavigationStackEnabled(fifth, (uint64_t)(boolean)(value != 0));
			fifth->lpVtbl->Release(fifth);
			return result;
		}
		return S_FALSE;
	}
	return raven_xaml_elements_set_integer(object, name, length, value);
}

HRESULT raven_xaml_element_set_text(IInspectable *object, const WCHAR *name, UINT32 length, const WCHAR *text, UINT32 text_length)
{
	frame1 *frame = NULL;
	xaml_type_name type;
	if (!object)
		return E_POINTER;
	if (!same_text(name, length, L"SourcePageType"))
		return raven_xaml_elements_set_text(object, name, length, text, text_length);
	if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame, (void **)&frame)) || !frame)
		return S_FALSE;
	type.Name = NULL;
	type.Kind = TypeKind_Metadata;
	if (text && text_length)
		WindowsCreateString(text, text_length, &type.Name);
	const HRESULT result = frame->lpVtbl->put_SourcePageType(frame, (uint64_t)(uintptr_t)&type);
	if (type.Name)
		WindowsDeleteString(type.Name);
	frame->lpVtbl->Release(frame);
	return result;
}

HRESULT raven_xaml_element_set_object(IInspectable *object, const WCHAR *name, UINT32 length, IInspectable *value)
{
	__x_ABI_CWindows_CUI_CXaml_CIWindow *window = NULL;
	if (!object)
		return E_POINTER;
	if (same_text(name, length, L"Children"))
		return raven_xaml_elements_set_object(object, name, length, value);
	/* The "Template" object property (the loader's deferred template application) routes to the elements dispatch like
	 * the children do; the elements side answers the button's control storage or the skip convention. The presenter's
	 * "ContentTemplate" (stable 525) rides the same dispatch — it must be named before the exact "Content" check below,
	 * whose S_FALSE would otherwise swallow the longer name. */
	if (same_text(name, length, L"Template") || same_text(name, length, L"ContentTemplate") ||
		same_text(name, length, L"Resources"))
		return raven_xaml_elements_set_object(object, name, length, value);
	/* A Style's BasedOn is an object property: the base style is stored on the style and applied first when the
	 * derived style is put on an element. This keeps the BasedOn chain working for styles loaded from XAML, where
	 * the base arrives as a resolved resource object. */
	if (same_text(name, length, L"BasedOn")) {
		__x_ABI_CWindows_CUI_CXaml_CIStyle *style = NULL;
		if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CIStyle, (void **)&style)) && style) {
			const HRESULT result = style->lpVtbl->put_BasedOn(style, (uint64_t)(uintptr_t)value);
			style->lpVtbl->Release(style);
			return result;
		}
		return S_FALSE;
	}
	if (!same_text(name, length, L"Content"))
		return S_FALSE;
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CIWindow, (void **)&window)) && window) {
		const HRESULT result = window->lpVtbl->put_Content(window, (uint64_t)(uintptr_t)value);
		window->lpVtbl->Release(window);
		return result;
	}
	/* The content controls (Page, Button) take the value through the IContentControl they inherit; the panels take their
	 * definition collections the same way. */
	return raven_xaml_elements_set_object(object, name, length, value);
}

/* The page a hosted frame currently shows; the rendering walk treats it as the frame's single visual child. The same
 * hook answers a templated control's template root first: the render walk reads it before the content slot, so an
 * applied ControlTemplate replaces the control's rendered visuals with its tree and no render-path change is needed. */
HRESULT raven_xaml_frame_render_child(IInspectable *object, IInspectable **out)
{
	frame1 *frame = NULL;
	IInspectable *templated = NULL;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (raven_xaml_element_template_child(object, &templated) == S_OK && templated) {
		*out = templated;
		return S_OK;
	}
	if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame, (void **)&frame)) || !frame)
		return S_FALSE;
	if (frame->lpVtbl != &frame1_table) {
		frame->lpVtbl->Release(frame);
		return S_FALSE;
	}
	struct frame *self = ((struct frame1_part *)frame)->owner;
	if (self->page) {
		*out = self->page;
		(*out)->lpVtbl->AddRef(*out);
	}
	frame->lpVtbl->Release(frame);
	return S_OK;
}

/* The protected composable DependencyObject constructor used by WinUI-derived objects. */
static const WCHAR dependency_class[] = L"Windows.UI.Xaml.DependencyObject";
static const IID dependency_factory_iid = {0x9a03af92, 0x7d8a, 0x4937, {0x88, 0x4f, 0xec, 0xf3, 0x4f, 0xe0, 0x2a, 0xcb}};
struct dependency_factory_interface;
struct dependency_factory_vtbl {
	HRESULT (WINAPI *QueryInterface)(struct dependency_factory_interface *, REFIID, void **);
	ULONG (WINAPI *AddRef)(struct dependency_factory_interface *);
	ULONG (WINAPI *Release)(struct dependency_factory_interface *);
	HRESULT (WINAPI *GetIids)(struct dependency_factory_interface *, ULONG *, IID **);
	HRESULT (WINAPI *GetRuntimeClassName)(struct dependency_factory_interface *, HSTRING *);
	HRESULT (WINAPI *GetTrustLevel)(struct dependency_factory_interface *, TrustLevel *);
	HRESULT (WINAPI *CreateInstance)(struct dependency_factory_interface *, IInspectable *, IInspectable **,
		__x_ABI_CWindows_CUI_CXaml_CIDependencyObject **);
};
struct dependency_factory_interface { const struct dependency_factory_vtbl *lpVtbl; };
static const IActivationFactoryVtbl dependency_activation_table;
static const struct dependency_factory_vtbl dependency_factory_table;
static IActivationFactory dependency_activation = {&dependency_activation_table};
static struct dependency_factory_interface dependency_composable = {&dependency_factory_table};
static LONG dependency_factory_refs = 1;
static HRESULT dependency_factory_query(REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory))
		*out = &dependency_activation;
	else if (equal_iid(iid, &dependency_factory_iid)) *out = &dependency_composable;
	else return E_NOINTERFACE;
	InterlockedIncrement(&dependency_factory_refs); return S_OK;
}
static HRESULT WINAPI dependency_activation_query(IActivationFactory *self, REFIID iid, void **out)
{ (void)self; return dependency_factory_query(iid, out); }
static ULONG WINAPI dependency_activation_addref(IActivationFactory *self)
{ (void)self; return (ULONG)InterlockedIncrement(&dependency_factory_refs); }
static ULONG WINAPI dependency_activation_release(IActivationFactory *self)
{ (void)self; return (ULONG)InterlockedDecrement(&dependency_factory_refs); }
static HRESULT WINAPI dependency_activation_iids(IActivationFactory *self, ULONG *count, IID **iids)
{ (void)self; return inspectable_iids(count, iids); }
static HRESULT WINAPI dependency_activation_name(IActivationFactory *self, HSTRING *name)
{ (void)self; if (!name) return E_POINTER; return WindowsCreateString(dependency_class, ARRAYSIZE(dependency_class) - 1, name); }
static HRESULT WINAPI dependency_activation_trust(IActivationFactory *self, TrustLevel *level)
{ (void)self; return inspectable_trust(level); }
static HRESULT WINAPI dependency_activate(IActivationFactory *self, IInspectable **out)
{ (void)self; if (!out) return E_POINTER; *out = NULL; return E_NOTIMPL; }
static const IActivationFactoryVtbl dependency_activation_table = { dependency_activation_query, dependency_activation_addref,
	dependency_activation_release, dependency_activation_iids, dependency_activation_name, dependency_activation_trust, dependency_activate };
static HRESULT WINAPI dependency_composable_query(struct dependency_factory_interface *self, REFIID iid, void **out)
{ (void)self; return dependency_factory_query(iid, out); }
static ULONG WINAPI dependency_composable_addref(struct dependency_factory_interface *self)
{ (void)self; return (ULONG)InterlockedIncrement(&dependency_factory_refs); }
static ULONG WINAPI dependency_composable_release(struct dependency_factory_interface *self)
{ (void)self; return (ULONG)InterlockedDecrement(&dependency_factory_refs); }
static HRESULT WINAPI dependency_composable_iids(struct dependency_factory_interface *self, ULONG *count, IID **iids)
{ (void)self; return inspectable_iids(count, iids); }
static HRESULT WINAPI dependency_composable_name(struct dependency_factory_interface *self, HSTRING *name)
{ (void)self; if (!name) return E_POINTER; return WindowsCreateString(dependency_class, ARRAYSIZE(dependency_class) - 1, name); }
static HRESULT WINAPI dependency_composable_trust(struct dependency_factory_interface *self, TrustLevel *level)
{ (void)self; return inspectable_trust(level); }
static HRESULT WINAPI dependency_create(struct dependency_factory_interface *self, IInspectable *outer,
	IInspectable **inner, __x_ABI_CWindows_CUI_CXaml_CIDependencyObject **value)
{
	(void)self;
	if (!value || (outer && !inner)) return E_POINTER;
	*value = NULL; if (inner) *inner = NULL;
	struct frame *object = create_frame();
	if (!object) return E_OUTOFMEMORY;
	object->dependency_only = TRUE; object->outer = outer;
	if (outer) { *inner = (IInspectable *)&object->inner; IInspectable_AddRef(outer); }
	*value = (__x_ABI_CWindows_CUI_CXaml_CIDependencyObject *)&object->dependency;
	return S_OK;
}
static const struct dependency_factory_vtbl dependency_factory_table = { dependency_composable_query, dependency_composable_addref,
	dependency_composable_release, dependency_composable_iids, dependency_composable_name, dependency_composable_trust, dependency_create };

/* The transition token of Frame.Navigate's third overload: Windows.UI.Xaml.Media.Animation.SuppressNavigationTransitionInfo
 * tells the Frame the navigation happens without a transition animation. The engine navigates without transitions anyway,
 * so the activated object is an identity the app's marshalling can hold and hand to Navigate. Its interfaces carry no
 * members anything here consults: the ids of the class's marker face and its inherited INavigationTransitionInfo face are
 * the casts the traced launch made against the activated instance. */
static const WCHAR suppress_transition_class[] = L"Windows.UI.Xaml.Media.Animation.SuppressNavigationTransitionInfo";
static const IID IID_suppress_navigation_transition_info = {0x11d3b13a, 0x180e, 0x4789,
	{0xa8, 0xbe, 0x77, 0x12, 0x88, 0x28, 0x93, 0xe6}};
static const IID IID_navigation_transition_info = {0xa9b05091, 0xae4a, 0x4372,
	{0x86, 0x25, 0x21, 0xb7, 0xa8, 0xb9, 0x8c, 0xa4}};

struct suppress_transition {
	const IInspectableVtbl *lpVtbl;
	struct suppress_transition_face {
		const IInspectableVtbl *lpVtbl;
		struct suppress_transition *owner;
	} suppress, transition;
	struct raven_weak_source weak;
	LONG refs;
};

static HRESULT WINAPI suppress_transition_query(IInspectable *iface, REFIID iid, void **out);

static HRESULT WINAPI suppress_transition_query_owner(void *owner, REFIID iid, void **out)
{
	return suppress_transition_query((IInspectable *)owner, iid, out);
}

static HRESULT WINAPI suppress_transition_face_query(struct suppress_transition_face *iface, REFIID iid, void **out)
{
	return suppress_transition_query((IInspectable *)iface->owner, iid, out);
}

static ULONG WINAPI suppress_transition_face_addref(struct suppress_transition_face *iface)
{
	return (ULONG)InterlockedIncrement(&iface->owner->refs);
}

static ULONG WINAPI suppress_transition_face_release(struct suppress_transition_face *iface)
{
	return (ULONG)InterlockedDecrement(&iface->owner->refs);
}

/* The interface-cast walks of the traced launch ask GetIids through the marshalling driver with an out pointer that a
 * live stack slot can never carry (the observed one was 1); answering E_POINTER keeps the cast machinery working instead
 * of faulting, the way a refused cast is tolerated. */
static int iids_out_plausible(const void *pointer)
{
	return !pointer || ((uintptr_t)pointer > 0x10000 && ((uintptr_t)pointer & 7) == 0);
}

static HRESULT WINAPI suppress_transition_face_iids(struct suppress_transition_face *iface, ULONG *count, IID **iids)
{
	(void)iface;
	if (count && !iids_out_plausible(count))
		return E_POINTER;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI suppress_transition_face_name(struct suppress_transition_face *iface, HSTRING *out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	return WindowsCreateString(suppress_transition_class, (UINT32)lstrlenW(suppress_transition_class), out);
}

static HRESULT WINAPI suppress_transition_face_trust(struct suppress_transition_face *iface, TrustLevel *out)
{
	(void)iface;
	return inspectable_trust(out);
}

static const IInspectableVtbl suppress_transition_face_table = {
	(HRESULT (WINAPI *)(IInspectable *, REFIID, void **))suppress_transition_face_query,
	(ULONG (WINAPI *)(IInspectable *))suppress_transition_face_addref,
	(ULONG (WINAPI *)(IInspectable *))suppress_transition_face_release,
	(HRESULT (WINAPI *)(IInspectable *, ULONG *, IID **))suppress_transition_face_iids,
	(HRESULT (WINAPI *)(IInspectable *, HSTRING *))suppress_transition_face_name,
	(HRESULT (WINAPI *)(IInspectable *, TrustLevel *))suppress_transition_face_trust,
};

static HRESULT WINAPI suppress_transition_query(IInspectable *iface, REFIID iid, void **out)
{
	struct suppress_transition *self = (struct suppress_transition *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable)) {
		*out = self;
		InterlockedIncrement(&self->refs);
		return S_OK;
	}
	/* The class's own casts (the marker face 11d3b13a-... and the runtime-computed second face a9b05091-... the traced
	 * launch asked) are refused for now: a granted face leads the launch's cast walk into a SharedLibrary GetIids call
	 * whose out pointer is not a pointer a live implementation can accept (it faults, or faults the driver after an
	 * E_POINTER), while the refused cast is the behavior the launch tolerates and runs past. Wiring the faces needs the
	 * cast walk's contract pinned down first. */
	/* The object is apartment-free like every engine element (the state is the refs count alone), so the agility
	 * marker is answered, and .NET Native projects every object through the weak source part. */
	if (equal_iid(iid, &IID_IAgileObject)) {
		*out = self;
		InterlockedIncrement(&self->refs);
		return S_OK;
	}
	if (equal_iid(iid, &IID_IWeakReferenceSource)) {
		*out = &self->weak;
		InterlockedIncrement(&self->refs);
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI suppress_transition_addref(IInspectable *iface)
{
	struct suppress_transition *self = (struct suppress_transition *)iface;
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG WINAPI suppress_transition_release(IInspectable *iface)
{
	struct suppress_transition *self = (struct suppress_transition *)iface;
	return (ULONG)InterlockedDecrement(&self->refs);
}

static HRESULT WINAPI suppress_transition_iids(IInspectable *iface, ULONG *count, IID **iids)
{
	(void)iface;
	if (count && !iids_out_plausible(count))
		return E_POINTER;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI suppress_transition_name(IInspectable *iface, HSTRING *out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	return WindowsCreateString(suppress_transition_class, (UINT32)lstrlenW(suppress_transition_class), out);
}

static HRESULT WINAPI suppress_transition_trust(IInspectable *iface, TrustLevel *out)
{
	(void)iface;
	return inspectable_trust(out);
}

static const IInspectableVtbl suppress_transition_table = {
	suppress_transition_query, suppress_transition_addref, suppress_transition_release,
	suppress_transition_iids, suppress_transition_name, suppress_transition_trust,
};

static struct suppress_transition suppress_transition_instance = {
	&suppress_transition_table,
	{&suppress_transition_face_table, &suppress_transition_instance},
	{&suppress_transition_face_table, &suppress_transition_instance},
	{NULL, NULL},
	1,
};

static LONG suppress_transition_ready;

/* The static instance's weak-source part cannot name its owner in a static initializer, so it is wired once before the
 * factory is handed out. */
static void suppress_transition_prepare(void)
{
	if (InterlockedCompareExchange(&suppress_transition_ready, 1, 0) == 0)
		raven_weak_source_init(&suppress_transition_instance.weak, &suppress_transition_instance,
			&suppress_transition_instance.refs, suppress_transition_query_owner);
}

static HRESULT WINAPI suppress_transition_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = (IInspectable *)&suppress_transition_instance;
	InterlockedIncrement(&suppress_transition_instance.refs);
	return S_OK;
}

/* The factory object itself, seen through its IActivationFactory. */
static HRESULT WINAPI suppress_factory_query(IActivationFactory *iface, REFIID iid, void **out);
static ULONG WINAPI suppress_factory_addref(IActivationFactory *iface);
static ULONG WINAPI suppress_factory_release(IActivationFactory *iface);
static HRESULT WINAPI suppress_factory_iids(IActivationFactory *iface, ULONG *count, IID **iids);
static HRESULT WINAPI suppress_factory_name(IActivationFactory *iface, HSTRING *out);
static HRESULT WINAPI suppress_factory_trust(IActivationFactory *iface, TrustLevel *out);

static const IActivationFactoryVtbl suppress_transition_factory_table = {
	.QueryInterface = suppress_factory_query,
	.AddRef = suppress_factory_addref,
	.Release = suppress_factory_release,
	.GetIids = suppress_factory_iids,
	.GetRuntimeClassName = suppress_factory_name,
	.GetTrustLevel = suppress_factory_trust,
	.ActivateInstance = suppress_transition_activate,
};

struct suppress_factory {
	const IActivationFactoryVtbl *lpVtbl;
	LONG refs;
};

static struct suppress_factory suppress_factory_instance = { &suppress_transition_factory_table, 1 };

static HRESULT WINAPI suppress_factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
	struct suppress_factory *self = (struct suppress_factory *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory)) {
		*out = self;
		InterlockedIncrement(&self->refs);
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI suppress_factory_addref(IActivationFactory *iface)
{
	struct suppress_factory *self = (struct suppress_factory *)iface;
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG WINAPI suppress_factory_release(IActivationFactory *iface)
{
	struct suppress_factory *self = (struct suppress_factory *)iface;
	return (ULONG)InterlockedDecrement(&self->refs);
}

static HRESULT WINAPI suppress_factory_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI suppress_factory_name(IActivationFactory *iface, HSTRING *out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	return WindowsCreateString(suppress_transition_class, (UINT32)lstrlenW(suppress_transition_class), out);
}

static HRESULT WINAPI suppress_factory_trust(IActivationFactory *iface, TrustLevel *out)
{
	(void)iface;
	return inspectable_trust(out);
}

HRESULT raven_xaml_controls_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	struct controls_factory *factory;
	if (!out)
		return E_POINTER;
	*out = NULL;
	prepare_factories();
	text = WindowsGetStringRawBuffer(name, &length);
	if (length == ARRAYSIZE(suppress_transition_class) - 1 && CompareStringOrdinal(text, (int)length, suppress_transition_class, -1, FALSE) == CSTR_EQUAL) {
		suppress_transition_prepare();
		InterlockedIncrement(&suppress_factory_instance.refs);
		*out = (IActivationFactory *)&suppress_factory_instance;
		return S_OK;
	}
	if (length == ARRAYSIZE(dependency_class) - 1 && CompareStringOrdinal(text, (int)length, dependency_class, -1, FALSE) == CSTR_EQUAL) {
		InterlockedIncrement(&dependency_factory_refs);
		*out = &dependency_activation;
		return S_OK;
	}
	if (length == ARRAYSIZE(frame_class) - 1 && CompareStringOrdinal(text, (int)length, frame_class, -1, FALSE) == CSTR_EQUAL)
		factory = &frame_factory_instance;
	else if (length == ARRAYSIZE(page_class) - 1 && CompareStringOrdinal(text, (int)length, page_class, -1, FALSE) == CSTR_EQUAL)
		factory = &page_factory_instance;
	else if (length == ARRAYSIZE(control_class) - 1 && CompareStringOrdinal(text, (int)length, control_class, -1, FALSE) == CSTR_EQUAL)
		factory = &control_factory_instance;
	else if (length == ARRAYSIZE(usercontrol_class) - 1 && CompareStringOrdinal(text, (int)length, usercontrol_class, -1, FALSE) == CSTR_EQUAL)
		factory = &usercontrol_factory_instance;
	else if (length == ARRAYSIZE(template_class) - 1 && CompareStringOrdinal(text, (int)length, template_class, -1, FALSE) == CSTR_EQUAL)
		factory = &template_factory_instance;
	else if (length == ARRAYSIZE(swapchain_class) - 1 && CompareStringOrdinal(text, (int)length, swapchain_class, -1, FALSE) == CSTR_EQUAL)
		factory = &swapchain_factory_instance;
	else if (length == ARRAYSIZE(contentdialog_class) - 1 && CompareStringOrdinal(text, (int)length, contentdialog_class, -1, FALSE) == CSTR_EQUAL)
		factory = &contentdialog_factory_instance;
	else
		return CLASS_E_CLASSNOTAVAILABLE;
	InterlockedIncrement(&factory->refs);
	*out = (IActivationFactory *)&factory->activation;
	return S_OK;
}

/* The ControlTemplate factory through the engine-internal export: the runtime catalog does not list the class yet (the
 * registry entry is added separately), so RoGetActivationFactory cannot reach it until then; the fixtures drive
 * activation through this GetProcAddress the way they drive the other engine-internal exports. Hands out an AddRef'd
 * IActivationFactory. */
HRESULT __declspec(dllexport) raven_xaml_control_template_factory(IActivationFactory **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	prepare_factories();
	InterlockedIncrement(&template_factory_instance.refs);
	*out = (IActivationFactory *)&template_factory_instance.activation;
	return S_OK;
}

/* Until the integrator adds the runtime catalog entry, fixtures use the same factory directly. */
HRESULT __declspec(dllexport) raven_xaml_swapchain_panel_factory(IActivationFactory **out)
{
	if (!out) return E_POINTER;
	prepare_factories();
	InterlockedIncrement(&swapchain_factory_instance.refs);
	*out = (IActivationFactory *)&swapchain_factory_instance.activation;
	return S_OK;
}
