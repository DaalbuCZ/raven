/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the first control classes of the element tree.
 *
 * Grid, TextBlock and Button are the controls the XAML engine builds its pages from. Grid owns its children through IPanel, whose
 * Children is an IVector<UIElement>, and its row and column definitions through IGrid, whose RowDefinitions and ColumnDefinitions
 * are the IVector`1 over RowDefinition and ColumnDefinition; TextBlock stores its text through ITextBlock; Button stores its
 * content through the IContentControl it inherits from ContentControl. The definition classes carry the fixed lengths the grid
 * lays its children into. The interface declarations come from Windows.UI.Xaml.winmd by scripts/generate-winrt-header.py; members
 * the engine does not implement yet answer E_NOTIMPL, and every stub keeps the exact signature of its slot so a call never depends
 * on an argument count the metadata did not give.
 *
 * The collections are the parameterized interface Windows.Foundation.Collections.IVector`1. A parameterized interface has no winmd
 * of its own: its GUID is the SHA-1 of its signature (pinterface({913337e9-11a1-4345-a3a2-4e7f956e222d};...), namespace
 * d57af411-737b-c042-abae-878b1e16adee), and its vtable carries the twelve IVector`1 methods of the generic definition, without
 * the inherited IIterable`1.First.
 *
 * Every element answers IUIElement (the default interface of UIElement, generated into windows_ui_xaml_uielement.h), so the
 * render pass's flat projection makes the three classes hit-testable peers: the routing (xaml_input.c) hit-tests the pointer
 * position against the geometry the pass recorded and delivers the pointer events here, where the handlers the application
 * stored on the element wait. Button additionally answers IButtonBase (generated into windows_ui_xaml_buttonbase.h) and runs
 * the ClickMode.Release behavior of ButtonBase on the events it receives. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <string.h>
#include <windows.ui.composition.h>
#include "events.h"
#include "windows_ui_xaml_elements.h"
#include "windows_ui_xaml_layout.h"
#include "xaml_element_ui.h"
#include "windows_ui_xaml_buttonbase.h"
#include "xaml_input.h"
#include "xaml_automation.h"
#include "windows_ui_xaml_dictionary.h" /* the boxed string __FIReference_1_HSTRING; mingw precomputes no instantiation */
#include "windows_ui_xaml_style.h" /* the Style interface the BasedOn assignment stores through */
#include "xaml_styles.h"

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

typedef __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock textblock1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPanel panel1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIGrid grid1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIButton button1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl content_control;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition row_definition1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition column_definition1;

/* The default interface of UIElement; the vector stores element interface pointers, which are word-sized references. */
typedef struct __x_ABI_CWindows_CUI_CXaml_CIUIElement ui_element;

/* The GUID of IVector<UIElement>, derived from the signature of the two winmd GUIDs as the Windows Runtime parameterized-type
 * rule defines it. The two definition vectors derive the same way; both facts were validated against the precomputed
 * IVector<HSTRING> in mingw's windows.foundation.h and against the derived IVector<UIElement> before use. */
DEFINE_GUID(IID___FIVector_1_UIElement, 0x393d6210, 0xfa21, 0x573a, 0xa2, 0x38, 0xf0, 0xe8, 0x73, 0x3b, 0x3c, 0x94);
DEFINE_GUID(IID___FIVector_1_RowDefinition, 0x2a17524a, 0x48ef, 0x579b, 0x9e, 0x63, 0xe1, 0xce, 0x52, 0xad, 0xca, 0xe2);
DEFINE_GUID(IID___FIVector_1_ColumnDefinition, 0xcaf7c66c, 0x1ccb, 0x5ada, 0xaf, 0xce, 0xfc, 0x62, 0xd2, 0xdb, 0x2c, 0x66);

#include "windows_ui_xaml_control.h"

/* The ControlTemplate surface declarations (root read, kind probe, factory) ride xaml_internal.h. */

static const WCHAR grid_class[] = L"Windows.UI.Xaml.Controls.Grid";
static const WCHAR textblock_class[] = L"Windows.UI.Xaml.Controls.TextBlock";
static const WCHAR button_class[] = L"Windows.UI.Xaml.Controls.Button";
static const WCHAR row_definition_class[] = L"Windows.UI.Xaml.Controls.RowDefinition";
static const WCHAR column_definition_class[] = L"Windows.UI.Xaml.Controls.ColumnDefinition";
static const WCHAR vector_class_name[] = L"Windows.Foundation.Collections.IVector`1<Windows.UI.Xaml.IUIElement>";
static const WCHAR row_vector_class_name[] = L"Windows.Foundation.Collections.IVector`1<Windows.UI.Xaml.Controls.RowDefinition>";
static const WCHAR column_vector_class_name[] = L"Windows.Foundation.Collections.IVector`1<Windows.UI.Xaml.Controls.ColumnDefinition>";

/* The real IButtonFactory ABI (Windows.Foundation.UniversalApiContract.winmd, uuid 80A13C19-843A-451C-8CF5-44C701B0E216,
 * resolved through the SDK metadata's CustomAttribute blob on the IButtonFactory TypeDef): the factory face the .NET
 * Native projection of `new Button()` drives, the same composable CreateInstance(outer, inner, instance) shape the
 * IControlFactory face above xaml_controls.c serves — composable classes have no default constructor in the ABI. */
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIButtonFactory button_factory_create_i;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIButtonFactoryVtbl button_factory_create_vtbl;
struct __x_ABI_CWindows_CUI_CXaml_CControls_CIButtonFactoryVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(button_factory_create_i *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(button_factory_create_i *This);
	ULONG (STDMETHODCALLTYPE *Release)(button_factory_create_i *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(button_factory_create_i *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(button_factory_create_i *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(button_factory_create_i *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *CreateInstance)(button_factory_create_i *This, uint64_t outer, uint64_t inner, uint64_t instance);
	END_INTERFACE
};
interface __x_ABI_CWindows_CUI_CXaml_CControls_CIButtonFactory { CONST_VTBL button_factory_create_vtbl *lpVtbl; };
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CControls_CIButtonFactory, 0x80a13c19, 0x843a, 0x451c, 0x8c, 0xf5, 0x44, 0xc7,
	0x01, 0xb0, 0xe2, 0x16);

static HRESULT inspectable_iids(ULONG *count, IID **iids)
{
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}

/* The element-family GetIids the .NET Native projection's castclass consumes: SharedLibrary decides a managed
 * cast of a projected object from the object's IInspectable::GetIids list (the suppress_transition round pinned
 * the walk; docs/uwp-xaml.md), so an empty list turns every cast into InvalidCastException even when QueryInterface
 * would answer. The list stays honest — an object lists exactly the faces its own QueryInterface grants, probed
 * here against the element-family candidates — so the cast verdict and a later member call through the cast face
 * can never disagree. The candidates are the IIDs the engine's element classes actually serve (the offline decode
 * of Shapr3D.dll's Button EEType 0x66938 shows the app's cast targets inside this set: IFrameworkElement itself —
 * EETypeRva 0x1afa0, name hash 0xea8ccd28 verified against the recovered ComputeNameHashCode — plus FE2/3/4/6). */
extern const IID IID_frame_framework;   /* a391d09b-4a99-4b7c-9d8d-6fa5d01f6fbf, Windows.UI.Xaml.IFrameworkElement (xaml_controls.c) */
extern const IID IID_frame_theme;       /* 792a5d91-62a1-40bf-a0ce-f9c131fcb7a7, IFrameworkElement6 (xaml_controls.c) */
extern const IID IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2;   /* f19104be-422a-4904-a52f-ee72010429e5 (xaml_controls.c) */
extern const IID IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3;   /* c81c2720-5c52-4bbe-a199-2b1e34f00f70 (xaml_controls.c) */
extern const IID IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4;   /* 6b765bb3-fba3-4404-bdee-1a45d1ca5f21 (xaml_controls.c) */
static const IID *const element_family_iids[] = {
	&IID__x_ABI_CWindows_CUI_CXaml_CIUIElement,
	&IID_frame_framework,
	&IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2,
	&IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3,
	&IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4,
	&IID_frame_theme,
	&IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid,
	&IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
	&IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
	&IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton,
	&IID__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase,
	&IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
	&IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControl,
};

static HRESULT element_honest_iids(HRESULT (*query)(void *, REFIID, void **), void *owner, ULONG *count, IID **iids)
{
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	const ULONG cap = sizeof element_family_iids / sizeof element_family_iids[0];
	IID *list = (IID *)HeapAlloc(GetProcessHeap(), 0, cap * sizeof *list);
	if (!list)
		return E_OUTOFMEMORY;
	ULONG found = 0;
	for (ULONG index = 0; index < cap; ++index) {
		void *face = NULL;
		if (query(owner, element_family_iids[index], &face) == S_OK && face) {
			list[found++] = *element_family_iids[index];
			IUnknown_Release((IUnknown *)face);
		}
	}
	if (!found) {
		HeapFree(GetProcessHeap(), 0, list);
		return S_OK;
	}
	*count = found;
	*iids = list;
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
#define STUB1_SET(iface, name) static HRESULT WINAPI name(iface *This, uint64_t a1) \
	{ (void)This; (void)a1; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
#define STUB2(iface, name) static HRESULT WINAPI name(iface *This, uint64_t a1, uint64_t a2) \
	{ (void)This; (void)a1; (void)a2; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }

/* The UIElement part every element class answers: the pointer events the routing delivers, the hit-test visibility, and the
 * owner plumbing, since the refcount and the identity live on the owning element and every part of it must answer as one COM
 * object. One stored handler list per pointer event; the tokens revoke exactly what add handed out, following the event
 * pattern of the CoreWindow's own events (events.c). The class name is the runtime class of the owner: a grid answers IUIElement
 * as Windows.UI.Xaml.Controls.Grid, the way a real projection reports the object rather than the interface. */

static HRESULT WINAPI ui_query(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, REFIID iid, void **out)
{
	struct element_ui *self = (struct element_ui *)This;
	return self->owner_query(self->owner, iid, out);
}
static ULONG WINAPI ui_addref(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This)
{
	struct element_ui *self = (struct element_ui *)This;
	return self->owner_addref(self->owner);
}
static ULONG WINAPI ui_release(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This)
{
	struct element_ui *self = (struct element_ui *)This;
	return self->owner_release(self->owner);
}
static HRESULT WINAPI ui_iids(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, ULONG *count, IID **iids)
{
	struct element_ui *self = (struct element_ui *)This;
	return element_honest_iids(self->owner_query, self->owner, count, iids);
}
static HRESULT WINAPI ui_name(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, HSTRING *name)
{
	const WCHAR *text = ((struct element_ui *)This)->class_name;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), name);
}
static HRESULT WINAPI ui_trust(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}

/* The slots the engine does not implement answer E_NOTIMPL with the exact signature the generated header declares, so a call
 * never depends on an argument count the metadata did not give; getters clear their out parameter first. */
static HRESULT WINAPI ui_stub0(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This)
{
	(void)This;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}
static HRESULT WINAPI ui_stub1(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1)
{
	(void)This;
	if (a1)
		*(void **)(uintptr_t)a1 = NULL;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}
static HRESULT WINAPI ui_stub1_set(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1)
{
	(void)This;
	(void)a1;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}
static HRESULT WINAPI ui_stub2(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2)
{
	(void)This;
	(void)a1;
	(void)a2;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}
static HRESULT WINAPI __attribute__((unused)) ui_stub3(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2, uint64_t a3)
{
	(void)This;
	(void)a1;
	(void)a2;
	(void)a3;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

#define UI_POINTER_ADDREMOVE(NAME, KIND) \
	static HRESULT WINAPI ui_add_##NAME(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t handler, uint64_t token) \
	{ \
		return raven_event_add(&((struct element_ui *)This)->events[KIND], (IUnknown *)(uintptr_t)handler, \
			(EventRegistrationToken *)(uintptr_t)token); \
	} \
	static HRESULT WINAPI ui_remove_##NAME(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t token) \
	{ \
		EventRegistrationToken value; \
		value.value = (INT64)token; \
		return raven_event_remove(&((struct element_ui *)This)->events[KIND], value); \
	}

UI_POINTER_ADDREMOVE(pointer_pressed, RAVEN_ELEMENT_POINTER_PRESSED)
UI_POINTER_ADDREMOVE(pointer_moved, RAVEN_ELEMENT_POINTER_MOVED)
UI_POINTER_ADDREMOVE(pointer_released, RAVEN_ELEMENT_POINTER_RELEASED)
UI_POINTER_ADDREMOVE(pointer_entered, RAVEN_ELEMENT_POINTER_ENTERED)
UI_POINTER_ADDREMOVE(pointer_exited, RAVEN_ELEMENT_POINTER_EXITED)
UI_POINTER_ADDREMOVE(pointer_capture_lost, RAVEN_ELEMENT_POINTER_CAPTURE_LOST)
UI_POINTER_ADDREMOVE(pointer_wheel_changed, RAVEN_ELEMENT_POINTER_WHEEL)

static HRESULT WINAPI ui_get_hit_test_visible(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t out)
{
	if (!out)
		return E_POINTER;
	*(boolean *)(uintptr_t)out = ((struct element_ui *)This)->hit_test_visible;
	return S_OK;
}
static HRESULT WINAPI ui_put_hit_test_visible(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t value)
{
	((struct element_ui *)This)->hit_test_visible = !!value;
	return S_OK;
}

struct routed_registration { struct routed_registration *next; IUnknown *identity; EventRegistrationToken token; int kind; };
static int routed_pointer_kind(IInspectable *event);
/* shortcut: handledEventsToo and keyboard delivery need routed Handled state and keyboard focus. */
static HRESULT WINAPI ui_add_handler(ui_element *iface, IInspectable *event, IUnknown *handler, boolean handled_events_too)
{
 struct element_ui *self = (struct element_ui *)iface;
 if (!event || !handler) return E_POINTER;
 int kind = routed_pointer_kind(event);
 if (kind < 0) return E_NOTIMPL;
 struct routed_registration *entry = ALLOC(sizeof *entry);
 if (!entry) return E_OUTOFMEMORY;
 HRESULT hr = IUnknown_QueryInterface(handler, &IID_IUnknown, (void **)&entry->identity);
 if (SUCCEEDED(hr)) hr = raven_event_add(&self->events[kind], handler, &entry->token);
 if (FAILED(hr)) { if (entry->identity) IUnknown_Release(entry->identity); FREE(entry); return hr; }
 entry->kind = kind; entry->next = self->routed_handlers; self->routed_handlers = entry;
 (void)handled_events_too; return S_OK;
}
static HRESULT WINAPI ui_remove_handler(ui_element *iface, IInspectable *event, IUnknown *handler)
{
 struct element_ui *self = (struct element_ui *)iface;
 if (!event || !handler) return E_POINTER;
 int kind = routed_pointer_kind(event);
 if (kind < 0) return E_NOTIMPL;
 IUnknown *identity = NULL; HRESULT hr = IUnknown_QueryInterface(handler, &IID_IUnknown, (void **)&identity);
 if (FAILED(hr)) return hr;
 struct routed_registration **at = &self->routed_handlers;
 while (*at && ((*at)->kind != kind || (*at)->identity != identity)) at = &(*at)->next;
 if (*at) { struct routed_registration *entry = *at; *at = entry->next; raven_event_remove(&self->events[kind], entry->token); IUnknown_Release(entry->identity); FREE(entry); }
 IUnknown_Release(identity); return S_OK;
}

/* The vtable follows the generated windows_ui_xaml_uielement.h slot for slot: IUIElement declares 96 methods after the
 * IInspectable ones, and every unimplemented slot carries the E_NOTIMPL stub of its arity. */
static const __x_ABI_CWindows_CUI_CXaml_CIUIElementVtbl uielement_table = {
	.QueryInterface = ui_query,
	.AddRef = ui_addref,
	.Release = ui_release,
	.GetIids = ui_iids,
	.GetRuntimeClassName = ui_name,
	.GetTrustLevel = ui_trust,
	.get_DesiredSize = ui_stub1,
	.get_AllowDrop = ui_stub1,
	.put_AllowDrop = ui_stub1_set,
	.get_Opacity = ui_stub1,
	.put_Opacity = ui_stub1_set,
	.get_Clip = ui_stub1,
	.put_Clip = ui_stub1_set,
	.get_RenderTransform = ui_stub1,
	.put_RenderTransform = ui_stub1_set,
	.get_Projection = ui_stub1,
	.put_Projection = ui_stub1_set,
	.get_RenderTransformOrigin = ui_stub1,
	.put_RenderTransformOrigin = ui_stub1_set,
	.get_IsHitTestVisible = ui_get_hit_test_visible,
	.put_IsHitTestVisible = ui_put_hit_test_visible,
	.get_Visibility = ui_stub1,
	.put_Visibility = ui_stub1_set,
	.get_RenderSize = ui_stub1,
	.get_UseLayoutRounding = ui_stub1,
	.put_UseLayoutRounding = ui_stub1_set,
	.get_Transitions = ui_stub1,
	.put_Transitions = ui_stub1_set,
	.get_CacheMode = ui_stub1,
	.put_CacheMode = ui_stub1_set,
	.get_IsTapEnabled = ui_stub1,
	.put_IsTapEnabled = ui_stub1_set,
	.get_IsDoubleTapEnabled = ui_stub1,
	.put_IsDoubleTapEnabled = ui_stub1_set,
	.get_IsRightTapEnabled = ui_stub1,
	.put_IsRightTapEnabled = ui_stub1_set,
	.get_IsHoldingEnabled = ui_stub1,
	.put_IsHoldingEnabled = ui_stub1_set,
	.get_ManipulationMode = ui_stub1,
	.put_ManipulationMode = ui_stub1_set,
	.get_PointerCaptures = ui_stub1,
	.add_KeyUp = ui_stub2,
	.remove_KeyUp = ui_stub1_set,
	.add_KeyDown = ui_stub2,
	.remove_KeyDown = ui_stub1_set,
	.add_GotFocus = ui_stub2,
	.remove_GotFocus = ui_stub1_set,
	.add_LostFocus = ui_stub2,
	.remove_LostFocus = ui_stub1_set,
	.add_DragEnter = ui_stub2,
	.remove_DragEnter = ui_stub1_set,
	.add_DragLeave = ui_stub2,
	.remove_DragLeave = ui_stub1_set,
	.add_DragOver = ui_stub2,
	.remove_DragOver = ui_stub1_set,
	.add_Drop = ui_stub2,
	.remove_Drop = ui_stub1_set,
	.add_PointerPressed = ui_add_pointer_pressed,
	.remove_PointerPressed = ui_remove_pointer_pressed,
	.add_PointerMoved = ui_add_pointer_moved,
	.remove_PointerMoved = ui_remove_pointer_moved,
	.add_PointerReleased = ui_add_pointer_released,
	.remove_PointerReleased = ui_remove_pointer_released,
	.add_PointerEntered = ui_add_pointer_entered,
	.remove_PointerEntered = ui_remove_pointer_entered,
	.add_PointerExited = ui_add_pointer_exited,
	.remove_PointerExited = ui_remove_pointer_exited,
	.add_PointerCaptureLost = ui_add_pointer_capture_lost,
	.remove_PointerCaptureLost = ui_remove_pointer_capture_lost,
	.add_PointerCanceled = ui_stub2,
	.remove_PointerCanceled = ui_stub1_set,
	.add_PointerWheelChanged = ui_add_pointer_wheel_changed,
	.remove_PointerWheelChanged = ui_remove_pointer_wheel_changed,
	.add_Tapped = ui_stub2,
	.remove_Tapped = ui_stub1_set,
	.add_DoubleTapped = ui_stub2,
	.remove_DoubleTapped = ui_stub1_set,
	.add_Holding = ui_stub2,
	.remove_Holding = ui_stub1_set,
	.add_RightTapped = ui_stub2,
	.remove_RightTapped = ui_stub1_set,
	.add_ManipulationStarting = ui_stub2,
	.remove_ManipulationStarting = ui_stub1_set,
	.add_ManipulationInertiaStarting = ui_stub2,
	.remove_ManipulationInertiaStarting = ui_stub1_set,
	.add_ManipulationStarted = ui_stub2,
	.remove_ManipulationStarted = ui_stub1_set,
	.add_ManipulationDelta = ui_stub2,
	.remove_ManipulationDelta = ui_stub1_set,
	.add_ManipulationCompleted = ui_stub2,
	.remove_ManipulationCompleted = ui_stub1_set,
	.Measure = ui_stub1,
	.Arrange = ui_stub1,
	.CapturePointer = ui_stub2,
	.ReleasePointerCapture = ui_stub1,
	.ReleasePointerCaptures = ui_stub0,
	.AddHandler = ui_add_handler,
	.RemoveHandler = ui_remove_handler,
	.TransformToVisual = ui_stub2,
	.InvalidateMeasure = ui_stub0,
	.InvalidateArrange = ui_stub0,
	.UpdateLayout = ui_stub0,
};

void element_ui_init(struct element_ui *self, void *owner, HRESULT (*owner_query)(void *, REFIID, void **),
	ULONG (*owner_addref)(void *), ULONG (*owner_release)(void *), const WCHAR *class_name)
{
	self->lpVtbl = &uielement_table;
	self->owner = owner;
	self->owner_query = owner_query;
	self->owner_addref = owner_addref;
	self->owner_release = owner_release;
	self->class_name = class_name;
	self->hit_test_visible = TRUE; /* an element is hit-testable unless the application says otherwise */
	for (int event = 0; event < RAVEN_ELEMENT_POINTER_COUNT; ++event)
		InitializeCriticalSection(&self->events[event].lock);
}

void element_ui_release(struct element_ui *self)
{
 while (self->routed_handlers) {
  struct routed_registration *entry = self->routed_handlers; self->routed_handlers = entry->next;
  IUnknown_Release(entry->identity); FREE(entry);
 }
	for (int event = 0; event < RAVEN_ELEMENT_POINTER_COUNT; ++event) {
		raven_event_close(&self->events[event]);
		DeleteCriticalSection(&self->events[event].lock);
	}
}

/* The instance handlers the application stored on the element's IUIElement, each with the element as sender and the
 * CoreWindow's snapshot as the routed arguments. A handler failure is the application's business and never reaches the
 * caller; the routing stays alive whatever it returns. Both the target delivery and the bubbling pass reach the handlers
 * through here. */
static void element_deliver_handlers(struct element_ui *self, __x_ABI_CWindows_CUI_CXaml_CIUIElement *ui,
	enum raven_element_pointer kind, __x_ABI_CWindows_CUI_CCore_CIPointerEventArgs *args)
{
	IUnknown **handlers = NULL;
	ULONG count = 0;
	if (raven_event_snapshot(&self->events[kind], &handlers, &count) == S_OK) {
		for (ULONG index = 0; index < count; ++index) {
			__FITypedEventHandler_2_Windows__CUI__CXaml__CUIElement_Windows__CUI__CXaml__CInput__CPointerRoutedEventArgs *handler =
				(void *)handlers[index];
			handler->lpVtbl->Invoke(handler, ui, (__x_ABI_CWindows_CUI_CXaml_CInput_CIPointerRoutedEventArgs *)args);
			IUnknown_Release(handlers[index]);
		}
		FREE(handlers);
	}
}

/* A query that answers success with a pointer no live interface can carry means the recorded element was freed and its
 * memory reused; delivering into it would fault. The note names both pointers so the trace shows which recorded hit died
 * (temporary diagnosis of the freed-element delivery the traced Shapr3D launch reached). */
static int element_face_plausible(const void *face)
{
	return (uintptr_t)face > 0x10000 && ((uintptr_t)face & 7) == 0;
}

static void element_face_note(const void *element, const void *face)
{
	char line[130];
	wsprintfA(line, "raven-xaml: element %p answered IUIElement with %p; freed or corrupt, delivery skipped\n",
		element, face);
	raven_trace_note(line);
}

static int element_deliverable(const void *element, void **ui, IInspectable *identity)
{
	__x_ABI_CWindows_CUI_CXaml_CIUIElement *face = NULL;
	if (!element_face_plausible(element))
		goto freed;
	if (FAILED(identity->lpVtbl->QueryInterface(identity, &IID__x_ABI_CWindows_CUI_CXaml_CIUIElement, (void **)&face)) || !face)
		return FALSE; /* the page and frame wrappers do not answer IUIElement and receive nothing */
	if (!element_face_plausible(face))
		goto freed;
	*ui = face;
	return TRUE;
freed:
	element_face_note(element, face);
	return FALSE;
}

/* The target delivery: the element's own behavior first (a button tracks IsPressed and raises Click), then the handlers the
 * application stored. */
void raven_xaml_element_pointer(IInspectable *element, enum raven_element_pointer kind,
	__x_ABI_CWindows_CUI_CCore_CIPointerEventArgs *args)
{
	__x_ABI_CWindows_CUI_CXaml_CIUIElement *ui = NULL;
	if (!element || !args || !element_deliverable(element, (void **)&ui, element))
		return;
	struct element_ui *self = (struct element_ui *)ui;
	if (self->behavior)
		self->behavior(self->owner, element, kind, args);
	element_deliver_handlers(self, ui, kind, args);
	ui->lpVtbl->Release(ui);
}

/* The bubbling pass the routing reaches for every element above the target: the instance handlers only, never the element's
 * own behavior — the behavior belongs to the event's target alone, so a button under another button's press must not click
 * twice. Elements that do not answer IUIElement (the page and frame wrappers) receive nothing. */
void raven_xaml_element_bubble(IInspectable *element, enum raven_element_pointer kind,
	__x_ABI_CWindows_CUI_CCore_CIPointerEventArgs *args)
{
	__x_ABI_CWindows_CUI_CXaml_CIUIElement *ui = NULL;
	if (!element || !args || !element_deliverable(element, (void **)&ui, element))
		return;
	element_deliver_handlers((struct element_ui *)ui, ui, kind, args);
	ui->lpVtbl->Release(ui);
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

/* The children collection */

struct children_vector;

struct children_vector_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(struct children_vector *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(struct children_vector *This);
	ULONG (STDMETHODCALLTYPE *Release)(struct children_vector *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(struct children_vector *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(struct children_vector *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(struct children_vector *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetAt)(struct children_vector *This, UINT32 index, ui_element **value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(struct children_vector *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *GetView)(struct children_vector *This, void **value);
	HRESULT (STDMETHODCALLTYPE *IndexOf)(struct children_vector *This, ui_element *element, UINT32 *index, boolean *value);
	HRESULT (STDMETHODCALLTYPE *SetAt)(struct children_vector *This, UINT32 index, ui_element *value);
	HRESULT (STDMETHODCALLTYPE *InsertAt)(struct children_vector *This, UINT32 index, ui_element *value);
	HRESULT (STDMETHODCALLTYPE *RemoveAt)(struct children_vector *This, UINT32 index);
	HRESULT (STDMETHODCALLTYPE *Append)(struct children_vector *This, ui_element *value);
	HRESULT (STDMETHODCALLTYPE *RemoveAtEnd)(struct children_vector *This);
	HRESULT (STDMETHODCALLTYPE *Clear)(struct children_vector *This);
	HRESULT (STDMETHODCALLTYPE *GetMany)(struct children_vector *This, UINT32 start_index, UINT32 items_size,
		ui_element **items, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *ReplaceAll)(struct children_vector *This, UINT32 count, ui_element **items);
};

struct children_vector {
	const struct children_vector_vtbl *lpVtbl;
	LONG refs;
	/* The parameterized identity: the QI answers the instantiation's IID and the runtime class name spells its signature. */
	const IID *iid;
	const WCHAR *class_name;
	/* A definitions vector drives the layout pass, so an append re-renders the hosted trees; the children vector renders when
	 * the loader has finished attaching the object, not on each append. */
	BOOL definitions;
	ui_element **items;
	UINT32 count, capacity;
};

static HRESULT WINAPI vector_query(struct children_vector *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) && !equal_iid(iid, self->iid))
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	*out = self;
	return S_OK;
}

static ULONG WINAPI vector_addref(struct children_vector *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG WINAPI vector_release(struct children_vector *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		for (UINT32 index = 0; index < self->count; ++index)
			if (self->items[index])
				((IInspectable *)self->items[index])->lpVtbl->Release((IInspectable *)self->items[index]);
		if (self->items)
			FREE(self->items);
		FREE(self);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI vector_iids(struct children_vector *self, ULONG *count, IID **iids)
{
	(void)self;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI vector_name(struct children_vector *self, HSTRING *name)
{
	if (!name)
		return E_POINTER;
	return WindowsCreateString(self->class_name, (UINT32)lstrlenW(self->class_name), name);
}

static HRESULT WINAPI vector_trust(struct children_vector *self, TrustLevel *level)
{
	(void)self;
	return inspectable_trust(level);
}

static HRESULT WINAPI vector_get_at(struct children_vector *self, UINT32 index, ui_element **value)
{
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (index >= self->count)
		return E_BOUNDS;
	*value = self->items[index];
	if (*value)
		((IInspectable *)*value)->lpVtbl->AddRef((IInspectable *)*value);
	return S_OK;
}

static HRESULT WINAPI vector_size(struct children_vector *self, UINT32 *value)
{
	if (!value)
		return E_POINTER;
	*value = self->count;
	return S_OK;
}

static HRESULT WINAPI vector_get_view(struct children_vector *self, void **value)
{
	(void)self;
	if (value)
		*value = NULL;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI vector_index_of(struct children_vector *self, ui_element *element, UINT32 *index, boolean *value)
{
	if (!index || !value)
		return E_POINTER;
	*value = FALSE;
	*index = 0;
	/* Collection identity is IUnknown identity: two references to one element can arrive through different interfaces. */
	IUnknown *unknown = NULL;
	if (!element || FAILED(((IInspectable *)element)->lpVtbl->QueryInterface((IInspectable *)element, &IID_IUnknown,
		(void **)&unknown)))
		return S_OK;
	for (UINT32 position = 0; position < self->count; ++position) {
		IUnknown *other = NULL;
		if (FAILED(((IInspectable *)self->items[position])->lpVtbl->QueryInterface((IInspectable *)self->items[position],
			&IID_IUnknown, (void **)&other)))
			continue;
		const int same = other == unknown;
		other->lpVtbl->Release(other);
		if (same) {
			*index = position;
			*value = TRUE;
			break;
		}
	}
	unknown->lpVtbl->Release(unknown);
	return S_OK;
}

static HRESULT WINAPI vector_set_at(struct children_vector *self, UINT32 index, ui_element *value)
{
	(void)self;
	(void)index;
	(void)value;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI vector_insert_at(struct children_vector *self, UINT32 index, ui_element *value)
{
	(void)self;
	(void)index;
	(void)value;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI vector_remove_at(struct children_vector *self, UINT32 index)
{
	(void)self;
	(void)index;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI vector_append(struct children_vector *self, ui_element *value)
{
	if (!value)
		return E_POINTER;
	if (self->count == self->capacity) {
		const UINT32 capacity = self->capacity ? self->capacity * 2 : 4;
		ui_element **items = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size_t)capacity * sizeof *items);
		if (!items)
			return E_OUTOFMEMORY;
		if (self->items) {
			for (UINT32 index = 0; index < self->count; ++index)
				items[index] = self->items[index];
			FREE(self->items);
		}
		self->items = items;
		self->capacity = capacity;
	}
	self->items[self->count] = value;
	((IInspectable *)value)->lpVtbl->AddRef((IInspectable *)value);
	++self->count;
	if (self->definitions)
		raven_render_changed();
	return S_OK;
}

static HRESULT WINAPI vector_remove_at_end(struct children_vector *self)
{
	(void)self;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI vector_clear(struct children_vector *self)
{
	for (UINT32 index = 0; index < self->count; ++index)
		if (self->items[index])
			((IInspectable *)self->items[index])->lpVtbl->Release((IInspectable *)self->items[index]);
	self->count = 0;
	return S_OK;
}

static HRESULT WINAPI vector_get_many(struct children_vector *self, UINT32 start_index, UINT32 items_size,
	ui_element **items, UINT32 *value)
{
	(void)self;
	(void)start_index;
	(void)items_size;
	(void)items;
	if (value)
		*value = 0;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI vector_replace_all(struct children_vector *self, UINT32 count, ui_element **items)
{
	(void)self;
	(void)count;
	(void)items;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

static const struct children_vector_vtbl vector_table = {
	.QueryInterface = vector_query,
	.AddRef = vector_addref,
	.Release = vector_release,
	.GetIids = vector_iids,
	.GetRuntimeClassName = vector_name,
	.GetTrustLevel = vector_trust,
	.GetAt = vector_get_at,
	.get_Size = vector_size,
	.GetView = vector_get_view,
	.IndexOf = vector_index_of,
	.SetAt = vector_set_at,
	.InsertAt = vector_insert_at,
	.RemoveAt = vector_remove_at,
	.Append = vector_append,
	.RemoveAtEnd = vector_remove_at_end,
	.Clear = vector_clear,
	.GetMany = vector_get_many,
	.ReplaceAll = vector_replace_all,
};

static struct children_vector *create_children(const IID *iid, const WCHAR *class_name, BOOL definitions)
{
	struct children_vector *vector = ALLOC(sizeof *vector);
	if (!vector)
		return NULL;
	vector->lpVtbl = &vector_table;
	vector->refs = 1;
	vector->iid = iid;
	vector->class_name = class_name;
	vector->definitions = definitions;
	return vector;
}

/* Grid */

struct grid;

struct grid1_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIGridVtbl *lpVtbl; struct grid *owner; };
struct panel_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIPanelVtbl *lpVtbl; struct grid *owner; };

/* What the element presents itself as: the grid's own machinery (children, the scalar sizes, the attached
 * slots, the UIElement face) covers the shape elements the compiled control XBFs declare, so Border,
 * StackPanel and Image build over the same struct — the shapes answer the Panel/UIElement faces and name
 * themselves their own class; only the IGrid face stays the grid's. The Rectangle (the shape class a compiled
 * document declares directly, not a panel) rides the same struct too and answers the IGrid face beside the
 * Panel/UIElement ones, so the attached Grid.Row it is given stores and the layout pass reads it back — the
 * machinery bargain the other shapes already have, extended to the one face its layout needs. */
enum grid_shape { grid_shape_grid, grid_shape_border, grid_shape_stackpanel, grid_shape_image, grid_shape_rectangle };

struct grid {
	struct grid1_part first;
	struct panel_part panel;
	struct element_ui ui;
	LONG refs;
	HSTRING name;
	const WCHAR *class_name; /* the runtime class the parts name themselves: the grid's own or the shape's */
	unsigned shape; /* enum grid_shape */
	struct children_vector *children, *rows, *columns, *groups;
	COLORREF color;
	BYTE alpha;
	IInspectable *background;
	/* The Rectangle's Shape.Fill rides the brush slot above (the shape's single brush — a rectangle has no
	 * Background); the outline and corner radii the class also carries are stored without rendering yet, the
	 * way the Border stores what it cannot paint. */
	IInspectable *stroke;
	COLORREF stroke_color;
	BYTE stroke_alpha;
	float stroke_thickness;
	float radius_x, radius_y;
	/* The Style the element was given (an owned reference); put_Style applied its setters into the fields above at the
	 * put, so the reference records what was applied rather than overriding the own state at read time. */
	IInspectable *style;
	IInspectable *resources;
	float width, height;
	/* The attached Grid.Row and Grid.Column this element was given; children without them lay out at cell (0, 0). The spans
	 * sit beside them in the same order the store selector uses; a child without a span reads back one track. */
	UINT32 row, column;
	UINT32 row_span, column_span;
};

static HRESULT grid_query_any(struct grid *self, REFIID iid, void **out);
static ULONG grid_addref(struct grid *self);
static ULONG grid_release(struct grid *self);

#define GRID_PART(part, iface, prefix) \
	static HRESULT WINAPI prefix##_query(iface *This, REFIID iid, void **out) \
	{ return grid_query_any(((struct part *)This)->owner, iid, out); } \
	static ULONG WINAPI prefix##_addref(iface *This) { return grid_addref(((struct part *)This)->owner); } \
	static ULONG WINAPI prefix##_release(iface *This) { return grid_release(((struct part *)This)->owner); } \
	static HRESULT WINAPI prefix##_iids(iface *This, ULONG *count, IID **iids) \
	{ return element_honest_iids((element_query_function)grid_query_any, ((struct part *)This)->owner, count, iids); } \
	static HRESULT WINAPI prefix##_name(iface *This, HSTRING *name) \
	{ \
		struct grid *self = ((struct part *)This)->owner; \
		if (!name) return E_POINTER; \
		return WindowsCreateString(self->class_name, (UINT32)lstrlenW(self->class_name), name); \
	} \
	static HRESULT WINAPI prefix##_trust(iface *This, TrustLevel *level) \
	{ (void)This; return inspectable_trust(level); }

GRID_PART(grid1_part, grid1, gridp)
GRID_PART(panel_part, panel1, panelp)

static HRESULT grid_query_any(struct grid *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		((self->shape == grid_shape_grid || self->shape == grid_shape_rectangle) &&
			equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid)))
		*out = &self->first;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel))
		*out = &self->panel;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIUIElement))
		*out = &self->ui;
	else
		return E_NOINTERFACE;
	grid_addref(self);
	return S_OK;
}

static ULONG grid_addref(struct grid *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG grid_release(struct grid *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->name)
			WindowsDeleteString(self->name);
		if (self->children)
			vector_release(self->children);
		if (self->rows)
			vector_release(self->rows);
		if (self->columns)
			vector_release(self->columns);
		if (self->groups)
			vector_release(self->groups);
		if (self->background)
			self->background->lpVtbl->Release(self->background);
		if (self->stroke)
			self->stroke->lpVtbl->Release(self->stroke);
		if (self->style)
			self->style->lpVtbl->Release(self->style);
		if (self->resources)
			self->resources->lpVtbl->Release(self->resources);
		element_ui_release(&self->ui);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI grid_row_definitions(grid1 *iface, uint64_t out)
{
	struct grid *self = ((struct grid1_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	InterlockedIncrement(&self->rows->refs);
	*(void **)(uintptr_t)out = self->rows;
	return S_OK;
}

static HRESULT WINAPI grid_column_definitions(grid1 *iface, uint64_t out)
{
	struct grid *self = ((struct grid1_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	InterlockedIncrement(&self->columns->refs);
	*(void **)(uintptr_t)out = self->columns;
	return S_OK;
}

static HRESULT WINAPI panel_children(panel1 *iface, uint64_t out)
{
	struct grid *self = ((struct panel_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	InterlockedIncrement(&self->children->refs);
	*(void **)(uintptr_t)out = self->children;
	return S_OK;
}

static HRESULT WINAPI panel_background(panel1 *iface, uint64_t out)
{
	struct grid *self = ((struct panel_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = self->background;
	if (self->background)
		self->background->lpVtbl->AddRef(self->background);
	return S_OK;
}

/* Panel.Background takes a Brush; the composition colour brush is the brush the engine can read a colour from today, so a
 * brush of that kind fills the panel and any other brush is stored without effect until the media brushes arrive. The
 * engine SolidColorBrush the theme dictionaries build reads through the same colour reader, so a dictionary brush fills
 * the panel too. */
static HRESULT WINAPI panel_put_background(panel1 *iface, uint64_t value)
{
	struct grid *self = ((struct panel_part *)iface)->owner;
	IInspectable *brush = (IInspectable *)(uintptr_t)value;
	COLORREF color = 0;
	BYTE alpha = 0;
	if (brush) {
		__x_ABI_CWindows_CUI_CColor read;
		if (raven_solid_brush_get_color(brush, &read) != S_OK) {
			__x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush *color_brush = NULL;
			if (SUCCEEDED(brush->lpVtbl->QueryInterface(brush,
				&IID___x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush, (void **)&color_brush)) && color_brush) {
				if (color_brush->lpVtbl->get_Color(color_brush, &read) == S_OK) {
					color = RGB(read.R, read.G, read.B);
					alpha = read.A;
				}
				color_brush->lpVtbl->Release(color_brush);
			}
		} else {
			color = RGB(read.R, read.G, read.B);
			alpha = read.A;
		}
		brush->lpVtbl->AddRef(brush);
	}
	if (self->background)
		self->background->lpVtbl->Release(self->background);
	self->background = brush;
	self->color = color;
	self->alpha = alpha;
	raven_render_changed();
	return S_OK;
}

STUB1(panel1, panel_items_host)
STUB1(panel1, panel_transitions)
STUB1_SET(panel1, panel_put_transitions)

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIGridVtbl grid1_table = {
	.QueryInterface = gridp_query,
	.AddRef = gridp_addref,
	.Release = gridp_release,
	.GetIids = gridp_iids,
	.GetRuntimeClassName = gridp_name,
	.GetTrustLevel = gridp_trust,
	.get_RowDefinitions = grid_row_definitions,
	.get_ColumnDefinitions = grid_column_definitions,
};

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIPanelVtbl panel_table = {
	.QueryInterface = panelp_query,
	.AddRef = panelp_addref,
	.Release = panelp_release,
	.GetIids = panelp_iids,
	.GetRuntimeClassName = panelp_name,
	.GetTrustLevel = panelp_trust,
	.get_Children = panel_children,
	.get_Background = panel_background,
	.put_Background = panel_put_background,
	.get_IsItemsHost = panel_items_host,
	.get_ChildrenTransitions = panel_transitions,
	.put_ChildrenTransitions = panel_put_transitions,
};

static const WCHAR border_class[] = L"Windows.UI.Xaml.Controls.Border";
static const WCHAR stackpanel_class[] = L"Windows.UI.Xaml.Controls.StackPanel";
static const WCHAR image_class[] = L"Windows.UI.Xaml.Controls.Image";
static const WCHAR rectangle_class[] = L"Windows.UI.Xaml.Shapes.Rectangle";

static struct grid *create_grid_shape(enum grid_shape shape)
{
	static const WCHAR *const names[] = {
		[grid_shape_grid] = NULL, /* the grid keeps its own class constant */
		[grid_shape_border] = border_class,
		[grid_shape_stackpanel] = stackpanel_class,
		[grid_shape_image] = image_class,
		[grid_shape_rectangle] = rectangle_class,
	};
	const WCHAR *class_name = shape == grid_shape_grid ? grid_class : names[shape];
	struct grid *grid = ALLOC(sizeof *grid);
	if (!grid)
		return NULL;
	grid->first.lpVtbl = &grid1_table;
	grid->first.owner = grid;
	grid->panel.lpVtbl = &panel_table;
	grid->panel.owner = grid;
	element_ui_init(&grid->ui, grid, (element_query_function)grid_query_any, (element_reference_function)grid_addref,
		(element_reference_function)grid_release, class_name);
	grid->class_name = class_name;
	grid->shape = shape;
	grid->refs = 1;
	grid->children = create_children(&IID___FIVector_1_UIElement, vector_class_name, FALSE);
	grid->rows = create_children(&IID___FIVector_1_RowDefinition, row_vector_class_name, TRUE);
	grid->columns = create_children(&IID___FIVector_1_ColumnDefinition, column_vector_class_name, TRUE);
	grid->groups = create_children(&IID___FIVector_1_UIElement, vector_class_name, FALSE);
	if (!grid->children || !grid->rows || !grid->columns || !grid->groups) {
		if (grid->children)
			vector_release(grid->children);
		if (grid->rows)
			vector_release(grid->rows);
		if (grid->columns)
			vector_release(grid->columns);
		if (grid->groups)
			vector_release(grid->groups);
		FREE(grid);
		return NULL;
	}
	InterlockedIncrement(&raven_xaml_objects);
	return grid;
}

static struct grid *create_grid(void)
{
	return create_grid_shape(grid_shape_grid);
}

/* TextBlock */

struct textblock {
	const __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlockVtbl *lpVtbl;
	struct element_ui ui;
	LONG refs;
	HSTRING name, text;
	/* FontSize in DIPs (the UWP default is 14) and the Foreground colour, which accepts a composition colour brush the
	 * way Panel.Background does plus the compiled colour-name path. The XAML default foreground is black — the light
	 * theme's SystemControlForegroundBaseHighBrush over a light background — and the CPU presentation clears the window
	 * to black, so a fixture checking pixels sets an explicit Foreground. */
	float font_size;
	COLORREF foreground;
	BYTE foreground_alpha;
	IInspectable *foreground_brush;
	/* The Style the element was given (an owned reference); put_Style applied its setters into the fields above at the
	 * put, so the reference records what was applied rather than overriding the own state at read time. */
	IInspectable *style;
	IInspectable *resources;
	/* The attached Grid.Row and Grid.Column this element was given; children without them lay out at cell (0, 0). The spans
	 * sit beside them in the same order the store selector uses; a child without a span reads back one track. */
	UINT32 row, column;
	UINT32 row_span, column_span;
};

/* The slot words the generated ABI uses carry a float as its IEEE-754 bit pattern in the low 32 bits; every caller of
 * put_FontSize packs it the same way. */
static float float_of_word(uint64_t word)
{
	float value = 0;
	memcpy(&value, &word, sizeof value);
	return value;
}

static HRESULT WINAPI textblock_query(textblock1 *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock)) {
		InterlockedIncrement(&((struct textblock *)iface)->refs);
		*out = iface;
		return S_OK;
	}
	if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIUIElement)) {
		InterlockedIncrement(&((struct textblock *)iface)->refs);
		*out = &((struct textblock *)iface)->ui;
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI textblock_addref(textblock1 *iface)
{
	return (ULONG)InterlockedIncrement(&((struct textblock *)iface)->refs);
}

static ULONG WINAPI textblock_release(textblock1 *iface)
{
	struct textblock *self = (struct textblock *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->name) WindowsDeleteString(self->name);
		if (self->text) WindowsDeleteString(self->text);
		if (self->foreground_brush)
			self->foreground_brush->lpVtbl->Release(self->foreground_brush);
		if (self->style)
			self->style->lpVtbl->Release(self->style);
		if (self->resources)
			self->resources->lpVtbl->Release(self->resources);
		element_ui_release(&self->ui);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI textblock_iids(textblock1 *iface, ULONG *count, IID **iids)
{
	return element_honest_iids((element_query_function)textblock_query, (struct textblock *)iface, count, iids);
}

static HRESULT WINAPI textblock_name(textblock1 *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(textblock_class, (UINT32)lstrlenW(textblock_class), name);
}

static HRESULT WINAPI textblock_trust(textblock1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI text_font_size(textblock1 *iface, uint64_t out)
{
	if (!out)
		return E_POINTER;
	*(float *)(uintptr_t)out = ((struct textblock *)iface)->font_size;
	return S_OK;
}
static HRESULT WINAPI text_put_font_size(textblock1 *iface, uint64_t value)
{
	struct textblock *self = (struct textblock *)iface;
	const float size = float_of_word(value);
	if (size <= 0.0f)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:1093\n"); return E_INVALIDARG; }
	self->font_size = size;
	raven_render_changed();
	return S_OK;
}
STUB1(textblock1, text_font_family)
STUB1_SET(textblock1, text_put_font_family)
STUB1(textblock1, text_font_weight)
STUB1_SET(textblock1, text_put_font_weight)
STUB1(textblock1, text_font_style)
STUB1_SET(textblock1, text_put_font_style)
STUB1(textblock1, text_font_stretch)
STUB1_SET(textblock1, text_put_font_stretch)
STUB1(textblock1, text_spacing)
STUB1_SET(textblock1, text_put_spacing)
static HRESULT WINAPI text_foreground(textblock1 *iface, uint64_t out)
{
	struct textblock *self = (struct textblock *)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = self->foreground_brush;
	if (self->foreground_brush)
		self->foreground_brush->lpVtbl->AddRef(self->foreground_brush);
	return S_OK;
}
/* Foreground takes a Brush; like Panel.Background the composition colour brush is the brush the engine can read a colour
 * from today, and any other brush is stored without effect until the media brushes arrive. */
static HRESULT WINAPI text_put_foreground(textblock1 *iface, uint64_t value)
{
	struct textblock *self = (struct textblock *)iface;
	IInspectable *brush = (IInspectable *)(uintptr_t)value;
	COLORREF color = 0;
	BYTE alpha = 0;
	if (brush) {
		__x_ABI_CWindows_CUI_CColor read;
		if (raven_solid_brush_get_color(brush, &read) != S_OK) {
			__x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush *color_brush = NULL;
			if (SUCCEEDED(brush->lpVtbl->QueryInterface(brush,
				&IID___x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush, (void **)&color_brush)) && color_brush) {
				if (color_brush->lpVtbl->get_Color(color_brush, &read) == S_OK) {
					color = RGB(read.R, read.G, read.B);
					alpha = read.A;
				}
				color_brush->lpVtbl->Release(color_brush);
			}
		} else {
			color = RGB(read.R, read.G, read.B);
			alpha = read.A;
		}
		brush->lpVtbl->AddRef(brush);
	}
	if (self->foreground_brush)
		self->foreground_brush->lpVtbl->Release(self->foreground_brush);
	self->foreground_brush = brush;
	self->foreground = color;
	self->foreground_alpha = alpha;
	raven_render_changed();
	return S_OK;
}
STUB1(textblock1, text_wrapping)
STUB1_SET(textblock1, text_put_wrapping)
STUB1(textblock1, text_trimming)
STUB1_SET(textblock1, text_put_trimming)
STUB1(textblock1, text_alignment)
STUB1_SET(textblock1, text_put_alignment)
static HRESULT WINAPI textblock_text(textblock1 *iface, uint64_t out)
{
	struct textblock *self = (struct textblock *)iface;
	if (!out)
		return E_POINTER;
	*(HSTRING *)(uintptr_t)out = self->text;
	if (self->text)
		WindowsDuplicateString(self->text, (HSTRING *)(uintptr_t)out);
	return S_OK;
}
static HRESULT WINAPI textblock_put_text(textblock1 *iface, uint64_t value)
{
	struct textblock *self = (struct textblock *)iface;
	HSTRING string = (HSTRING)(uintptr_t)value;
	UINT32 length = 0;
	const WCHAR *text = string ? WindowsGetStringRawBuffer(string, &length) : NULL;
	store_name(&self->text, text, length);
	raven_render_changed();
	return S_OK;
}
STUB1(textblock1, text_inlines)
STUB1(textblock1, text_padding)
STUB1_SET(textblock1, text_put_padding)
STUB1(textblock1, text_line_height)
STUB1_SET(textblock1, text_put_line_height)
STUB1(textblock1, text_stacking)
STUB1_SET(textblock1, text_put_stacking)
STUB1(textblock1, text_selection_enabled)
STUB1_SET(textblock1, text_put_selection_enabled)
STUB1(textblock1, text_selected)
STUB1(textblock1, text_content_start)
STUB1(textblock1, text_content_end)
STUB1(textblock1, text_selection_start)
STUB1(textblock1, text_selection_end)
STUB1(textblock1, text_baseline)
STUB2(textblock1, text_add_selection)
STUB1(textblock1, text_remove_selection)
STUB2(textblock1, text_add_context)
STUB1(textblock1, text_remove_context)
static HRESULT WINAPI textblock_select_all(textblock1 *iface)
{
	(void)iface;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}
STUB2(textblock1, text_select)
STUB2(textblock1, text_focus)

static const __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlockVtbl textblock_table = {
	.QueryInterface = textblock_query,
	.AddRef = textblock_addref,
	.Release = textblock_release,
	.GetIids = textblock_iids,
	.GetRuntimeClassName = textblock_name,
	.GetTrustLevel = textblock_trust,
	.get_FontSize = text_font_size,
	.put_FontSize = text_put_font_size,
	.get_FontFamily = text_font_family,
	.put_FontFamily = text_put_font_family,
	.get_FontWeight = text_font_weight,
	.put_FontWeight = text_put_font_weight,
	.get_FontStyle = text_font_style,
	.put_FontStyle = text_put_font_style,
	.get_FontStretch = text_font_stretch,
	.put_FontStretch = text_put_font_stretch,
	.get_CharacterSpacing = text_spacing,
	.put_CharacterSpacing = text_put_spacing,
	.get_Foreground = text_foreground,
	.put_Foreground = text_put_foreground,
	.get_TextWrapping = text_wrapping,
	.put_TextWrapping = text_put_wrapping,
	.get_TextTrimming = text_trimming,
	.put_TextTrimming = text_put_trimming,
	.get_TextAlignment = text_alignment,
	.put_TextAlignment = text_put_alignment,
	.get_Text = textblock_text,
	.put_Text = textblock_put_text,
	.get_Inlines = text_inlines,
	.get_Padding = text_padding,
	.put_Padding = text_put_padding,
	.get_LineHeight = text_line_height,
	.put_LineHeight = text_put_line_height,
	.get_LineStackingStrategy = text_stacking,
	.put_LineStackingStrategy = text_put_stacking,
	.get_IsTextSelectionEnabled = text_selection_enabled,
	.put_IsTextSelectionEnabled = text_put_selection_enabled,
	.get_SelectedText = text_selected,
	.get_ContentStart = text_content_start,
	.get_ContentEnd = text_content_end,
	.get_SelectionStart = text_selection_start,
	.get_SelectionEnd = text_selection_end,
	.get_BaselineOffset = text_baseline,
	.add_SelectionChanged = text_add_selection,
	.remove_SelectionChanged = text_remove_selection,
	.add_ContextMenuOpening = text_add_context,
	.remove_ContextMenuOpening = text_remove_context,
	.SelectAll = textblock_select_all,
	.Select = text_select,
	.Focus = text_focus,
};

static struct textblock *create_textblock(void)
{
	struct textblock *textblock = ALLOC(sizeof *textblock);
	if (!textblock)
		return NULL;
	textblock->lpVtbl = &textblock_table;
	element_ui_init(&textblock->ui, textblock, (element_query_function)textblock_query,
		(element_reference_function)textblock_addref, (element_reference_function)textblock_release, textblock_class);
	textblock->refs = 1;
	/* The UWP defaults: FontSize 14 and the light theme's black foreground. */
	textblock->font_size = 14.0f;
	textblock->foreground = RGB(0, 0, 0);
	textblock->foreground_alpha = 255;
	InterlockedIncrement(&raven_xaml_objects);
	return textblock;
}

/* Button */

struct button;

struct button1_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIButtonVtbl *lpVtbl; struct button *owner; };
struct buttonbase_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBaseVtbl *lpVtbl; struct button *owner; };
struct content_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControlVtbl *lpVtbl; struct button *owner; };
struct control_part { const control1_vtbl *lpVtbl; struct button *owner; };

/* The FrameworkElement continuation family the frame carries (xaml_controls.c's fe2/fe3/fe4/theme parts,
 * hand-declared there the same way because mingw's windows.ui.xaml.h models none of them): the projected
 * FrameworkElement surface spreads its members across the version interfaces, and generated app code casts
 * the object to the exact version declaring the member it touches. The traced Shapr3D constructor created a
 * fresh Button through IButtonFactory and cast it to a face in this family — the traced cast message names
 * the target EETypeRva 0x1afa0, the face after the WelcomePage round's IFrameworkElement3 (0x1af60) in the
 * app's type table — and the button answered only its control faces, so the projection threw the
 * InvalidCastException the unhandled rethrow fail-fasts on. The button now carries the same family the
 * frame answers, with the same storage defaults. */
extern const IID IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2;   /* f19104be-422a-4904-a52f-ee72010429e5 (xaml_controls.c) */
extern const IID IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3;   /* c81c2720-5c52-4bbe-a199-2b1e34f00f70 (xaml_controls.c) */
extern const IID IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4;   /* 6b765bb3-fba3-4404-bdee-1a45d1ca5f21 (xaml_controls.c) */
extern const IID IID_frame_theme;                                     /* 792a5d91-62a1-40bf-a0ce-f9c131fcb7a7 (xaml_controls.c, IFrameworkElement6) */
struct button_framework_thickness { double left, top, right, bottom; };
typedef struct __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2 button_fe2_iface;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3 button_fe3_iface;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4 button_fe4_iface;
struct button_fe2_part { const void *lpVtbl; struct button *owner; };
struct button_fe3_part { const void *lpVtbl; struct button *owner; };
struct button_fe4_part { const void *lpVtbl; struct button *owner; };
struct button_theme_part { const void *lpVtbl; struct button *owner; };
/* The three vtbls and the IFrameworkElement6 (ActualTheme) face, slot order the winmd's member order the
 * frame's tables reconstruct. */
struct button_fe2_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(button_fe2_iface *, REFIID, void **);
	ULONG (STDMETHODCALLTYPE *AddRef)(button_fe2_iface *);
	ULONG (STDMETHODCALLTYPE *Release)(button_fe2_iface *);
	HRESULT (STDMETHODCALLTYPE *GetIids)(button_fe2_iface *, ULONG *, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(button_fe2_iface *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(button_fe2_iface *, TrustLevel *);
	HRESULT (STDMETHODCALLTYPE *get_RequestedTheme)(button_fe2_iface *, INT32 *);
	HRESULT (STDMETHODCALLTYPE *put_RequestedTheme)(button_fe2_iface *, INT32);
	HRESULT (STDMETHODCALLTYPE *add_DataContextChanged)(button_fe2_iface *, IUnknown *, EventRegistrationToken *);
	HRESULT (STDMETHODCALLTYPE *remove_DataContextChanged)(button_fe2_iface *, EventRegistrationToken);
	HRESULT (STDMETHODCALLTYPE *GetBindingExpression)(button_fe2_iface *, void *, void **);
};
struct button_fe3_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(button_fe3_iface *, REFIID, void **);
	ULONG (STDMETHODCALLTYPE *AddRef)(button_fe3_iface *);
	ULONG (STDMETHODCALLTYPE *Release)(button_fe3_iface *);
	HRESULT (STDMETHODCALLTYPE *GetIids)(button_fe3_iface *, ULONG *, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(button_fe3_iface *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(button_fe3_iface *, TrustLevel *);
	HRESULT (STDMETHODCALLTYPE *add_Loading)(button_fe3_iface *, IUnknown *, EventRegistrationToken *);
	HRESULT (STDMETHODCALLTYPE *remove_Loading)(button_fe3_iface *, EventRegistrationToken);
};
struct button_fe4_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(button_fe4_iface *, REFIID, void **);
	ULONG (STDMETHODCALLTYPE *AddRef)(button_fe4_iface *);
	ULONG (STDMETHODCALLTYPE *Release)(button_fe4_iface *);
	HRESULT (STDMETHODCALLTYPE *GetIids)(button_fe4_iface *, ULONG *, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(button_fe4_iface *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(button_fe4_iface *, TrustLevel *);
	HRESULT (STDMETHODCALLTYPE *get_AllowFocusOnInteraction)(button_fe4_iface *, INT32 *);
	HRESULT (STDMETHODCALLTYPE *put_AllowFocusOnInteraction)(button_fe4_iface *, INT32);
	HRESULT (STDMETHODCALLTYPE *get_FocusVisualMargin)(button_fe4_iface *, struct button_framework_thickness *);
	HRESULT (STDMETHODCALLTYPE *put_FocusVisualMargin)(button_fe4_iface *, struct button_framework_thickness);
	HRESULT (STDMETHODCALLTYPE *get_FocusVisualSecondaryThickness)(button_fe4_iface *, struct button_framework_thickness *);
	HRESULT (STDMETHODCALLTYPE *put_FocusVisualSecondaryThickness)(button_fe4_iface *, struct button_framework_thickness);
	HRESULT (STDMETHODCALLTYPE *get_FocusVisualPrimaryThickness)(button_fe4_iface *, struct button_framework_thickness *);
	HRESULT (STDMETHODCALLTYPE *put_FocusVisualPrimaryThickness)(button_fe4_iface *, struct button_framework_thickness);
	HRESULT (STDMETHODCALLTYPE *get_FocusVisualSecondaryBrush)(button_fe4_iface *, IInspectable **);
	HRESULT (STDMETHODCALLTYPE *put_FocusVisualSecondaryBrush)(button_fe4_iface *, IInspectable *);
	HRESULT (STDMETHODCALLTYPE *get_FocusVisualPrimaryBrush)(button_fe4_iface *, IInspectable **);
	HRESULT (STDMETHODCALLTYPE *put_FocusVisualPrimaryBrush)(button_fe4_iface *, IInspectable *);
	HRESULT (STDMETHODCALLTYPE *get_AllowFocusWhenDisabled)(button_fe4_iface *, INT32 *);
	HRESULT (STDMETHODCALLTYPE *put_AllowFocusWhenDisabled)(button_fe4_iface *, INT32);
};
struct button_theme_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(struct button_theme_part *, REFIID, void **);
	ULONG (STDMETHODCALLTYPE *AddRef)(struct button_theme_part *);
	ULONG (STDMETHODCALLTYPE *Release)(struct button_theme_part *);
	HRESULT (STDMETHODCALLTYPE *GetIids)(struct button_theme_part *, ULONG *, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(struct button_theme_part *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(struct button_theme_part *, TrustLevel *);
	HRESULT (STDMETHODCALLTYPE *get_ActualTheme)(struct button_theme_part *, INT32 *);
	HRESULT (STDMETHODCALLTYPE *add_ActualThemeChanged)(struct button_theme_part *, IUnknown *, EventRegistrationToken *);
	HRESULT (STDMETHODCALLTYPE *remove_ActualThemeChanged)(struct button_theme_part *, EventRegistrationToken);
};

/* Windows.UI.Xaml.IFrameworkElement itself — the interface the app's castclass of a fresh Button asks the
 * projection for (Shapr3D.dll EETypeRva 0x1afa0, name hash 0xea8ccd28 verified against the recovered .NET Native
 * ComputeNameHashCode; the app image's own interface record pairs the EEType pointer with the GUID a391d09b-4a99-
 * 4b7c-9d8d-6fa5d01f6fbf, the same value the frame's face already carries). The 47 members follow the slot order
 * the frame's face reconstructed from the winmd (xaml_controls.c): the members the button has storage for answer
 * from it, the rest keep the frame's safe shapes (null-filled out slots on E_NOTIMPL). */
struct button_framework_part { const void *lpVtbl; struct button *owner; };
struct button_framework_vtbl {
	IInspectableVtbl inspectable;
	void *slots[47];
};

struct button {
	struct button1_part first;
	struct buttonbase_part base;
	struct content_part content;
	struct control_part control;
	struct element_ui ui;
	LONG refs;
	HSTRING name;
	IInspectable *content_value;
	/* The ControlTemplate the control carries (an owned reference): put_Template and the "Template" setter route here,
	 * and the render hook answers the template's root as the control's single visual child while it is set. The stored
	 * Content is NOT rendered while a template is applied — the engine has no chrome ContentPresenter to host it (the
	 * real control's template carries one), so a template that wants the content shown must place it itself. */
	IInspectable *template_value;
	IInspectable *command, *command_parameter;
	struct raven_event_list click;
	/* The Control.IsEnabledChanged subscriptions the IControl face retains; the Click pair shares the same
	 * machinery. */
	struct raven_event_list is_enabled_changed;
	UINT32 click_mode; /* enum raven_xaml_click_mode; the zero initialization is ClickMode.Release, the platform default */
	boolean is_pressed, is_pointer_over;
	/* The size the layout pass gave the button: its Width and Height, or the cell it lays out into. The routing records the
	 * rectangle this produces and the release-within-bounds check reads it back. */
	float width, height;
	/* The Style the element was given (an owned reference); put_Style applied its setters at the put. The Background a
	 * setter carried sits in the fields below it — the control chrome does not render yet, so the colour waits for that
	 * stage the way the reference itself waited for the style stage. */
	IInspectable *style;
	IInspectable *resources;
	COLORREF color;
	BYTE alpha;
	/* The attached Grid.Row and Grid.Column this element was given; children without them lay out at cell (0, 0). The spans
	 * sit beside them in the same order the store selector uses; a child without a span reads back one track. */
	UINT32 row, column;
	UINT32 row_span, column_span;
	/* The FrameworkElement continuation storage: the fields the fe2/fe3/fe4/theme faces read and write, with
	 * the real metadata defaults (RequestedTheme from the app theme, AllowFocusOnInteraction true, the
	 * thicknesses empty, the brushes null, AllowFocusWhenDisabled false). The event lists retain the
	 * DataContextChanged / Loading / ActualThemeChanged subscriptions the version faces carry. */
	INT32 requested_theme, allow_focus_on_interaction, allow_focus_when_disabled;
	struct button_framework_thickness focus_visual_margin, focus_visual_secondary_thickness, focus_visual_primary_thickness;
	IInspectable *focus_visual_secondary_brush, *focus_visual_primary_brush;
	struct raven_event_list data_context_changed, loading, actual_theme_changed;
	struct button_fe2_part fe2;
	struct button_fe3_part fe3;
	struct button_fe4_part fe4;
	struct button_theme_part theme;
	/* The IFrameworkElement members' storage (the base face the generated cast obtains): the extents as
	 * doubles with the real metadata defaults (Width/Height NaN auto, Min 0, Max PositiveInfinity — set in
	 * create_button), the alignments Stretch, FlowDirection LeftToRight, an empty Margin, and the object
	 * members Tag and DataContext (Style and Resources ride the button's own fields). The Loaded/Unloaded/
	 * SizeChanged/LayoutUpdated pairs share the event machinery the other faces use. */
	double fw_width, fw_height, fw_min_width, fw_max_width, fw_min_height, fw_max_height;
	INT32 fw_horizontal_alignment, fw_vertical_alignment, fw_flow_direction;
	struct button_framework_thickness fw_margin;
	IInspectable *fw_tag, *fw_data_context;
	struct raven_event_list fw_loaded, fw_unloaded, fw_size_changed, fw_layout_updated;
	struct button_framework_part fw;
};

static HRESULT button_query_any(struct button *self, REFIID iid, void **out);
static ULONG button_addref(struct button *self);
static ULONG button_release(struct button *self);

#define BUTTON_PART(part, iface, prefix) \
	static HRESULT WINAPI prefix##_query(iface *This, REFIID iid, void **out) \
	{ return button_query_any(((struct part *)This)->owner, iid, out); } \
	static ULONG WINAPI prefix##_addref(iface *This) { return button_addref(((struct part *)This)->owner); } \
	static ULONG WINAPI prefix##_release(iface *This) { return button_release(((struct part *)This)->owner); } \
	static HRESULT WINAPI prefix##_iids(iface *This, ULONG *count, IID **iids) \
	{ return element_honest_iids((element_query_function)button_query_any, ((struct part *)This)->owner, count, iids); } \
	static HRESULT WINAPI prefix##_name(iface *This, HSTRING *name) \
	{ (void)This; if (!name) return E_POINTER; return WindowsCreateString(button_class, (UINT32)lstrlenW(button_class), name); } \
	static HRESULT WINAPI prefix##_trust(iface *This, TrustLevel *level) \
	{ (void)This; return inspectable_trust(level); }

BUTTON_PART(button1_part, button1, buttonp)
BUTTON_PART(buttonbase_part, __x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase, basep)
BUTTON_PART(content_part, content_control, contentp)
BUTTON_PART(control_part, control1, controlp)

static HRESULT button_query_any(struct button *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton))
		*out = &self->first;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase))
		*out = &self->base;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIUIElement))
		*out = &self->ui;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl))
		*out = &self->content;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControl))
		*out = &self->control;
	else if (equal_iid(iid, &IID_frame_framework))
		*out = &self->fw;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement2))
		*out = &self->fe2;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement3))
		*out = &self->fe3;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIFrameworkElement4))
		*out = &self->fe4;
	else if (equal_iid(iid, &IID_frame_theme))
		*out = &self->theme;
	else {
		/* Which face the caller asks the button for and the button refuses — the cast the .NET projection
		 * drives on a fresh Button lands here, and a refused face is the InvalidCastException shape.
		 * RAVEN_TRACE_ELEMENT_QI=1 in the guest enables it; the note rides raven-shell's RavenTrace host
		 * call, which the trace carries without the debugger play. */
		static int element_qi_gate = -1;
		if (element_qi_gate < 0) {
			char flag[8] = "";
			element_qi_gate = GetEnvironmentVariableA("RAVEN_TRACE_ELEMENT_QI", flag, sizeof flag) ? 1 : 0;
		}
		if (element_qi_gate) {
			static void (WINAPI *host_trace)(const char *);
			if (!host_trace)
				host_trace = (void (WINAPI *)(const char *))GetProcAddress(LoadLibraryW(L"ravenhost.dll"), "RavenTrace");
			if (host_trace) {
				char line[160];
				wsprintfA(line, "raven-element: refused QI %08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
					(unsigned long)iid->Data1, iid->Data2, iid->Data3, iid->Data4[0], iid->Data4[1],
					iid->Data4[2], iid->Data4[3], iid->Data4[4], iid->Data4[5], iid->Data4[6], iid->Data4[7]);
				host_trace(line);
			}
		}
		return E_NOINTERFACE;
	}
	button_addref(self);
	return S_OK;
}

static ULONG button_addref(struct button *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG button_release(struct button *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->name)
			WindowsDeleteString(self->name);
		if (self->content_value)
			IInspectable_Release(self->content_value);
		if (self->template_value)
			IInspectable_Release(self->template_value);
		if (self->command)
			IInspectable_Release(self->command);
		if (self->command_parameter)
			IInspectable_Release(self->command_parameter);
		if (self->style)
			IInspectable_Release(self->style);
		if (self->resources)
			IInspectable_Release(self->resources);
		raven_event_close(&self->click);
		DeleteCriticalSection(&self->click.lock);
		raven_event_close(&self->is_enabled_changed);
		DeleteCriticalSection(&self->is_enabled_changed.lock);
		raven_event_close(&self->data_context_changed);
		DeleteCriticalSection(&self->data_context_changed.lock);
		raven_event_close(&self->loading);
		DeleteCriticalSection(&self->loading.lock);
		raven_event_close(&self->actual_theme_changed);
		DeleteCriticalSection(&self->actual_theme_changed.lock);
		if (self->focus_visual_secondary_brush)
			IInspectable_Release(self->focus_visual_secondary_brush);
		if (self->focus_visual_primary_brush)
			IInspectable_Release(self->focus_visual_primary_brush);
		element_ui_release(&self->ui);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

/* The FrameworkElement continuation members: RequestedTheme (FE2) rides the button's theme field, the
 * DataContextChanged / Loading / ActualThemeChanged pairs retain through the shared event lists, the FE4
 * focus-visual scalars/thicknesses/brushes store over the button's own fields with the metadata defaults,
 * GetBindingExpression answers the null a no-binding runtime answers. The ABI shapes mirror the frame's
 * fe2/fe3/fe4/theme tables (xaml_controls.c). */
#define BUTTON_FE_BOILERPLATE(prefix, iface_type, part_type) \
	static HRESULT WINAPI prefix##_query(iface_type *This, REFIID iid, void **out) \
	{ return button_query_any(((struct part_type *)This)->owner, iid, out); } \
	static ULONG WINAPI prefix##_addref(iface_type *This) { return button_addref(((struct part_type *)This)->owner); } \
	static ULONG WINAPI prefix##_release(iface_type *This) { return button_release(((struct part_type *)This)->owner); } \
	static HRESULT WINAPI prefix##_iids(iface_type *This, ULONG *count, IID **iids) \
	{ return element_honest_iids((element_query_function)button_query_any, ((struct part_type *)This)->owner, count, iids); } \
	static HRESULT WINAPI prefix##_name(iface_type *This, HSTRING *name) \
	{ (void)This; if (!name) return E_POINTER; return WindowsCreateString(button_class, (UINT32)lstrlenW(button_class), name); } \
	static HRESULT WINAPI prefix##_trust(iface_type *This, TrustLevel *level) \
	{ (void)This; return inspectable_trust(level); }
BUTTON_FE_BOILERPLATE(button_fe2, button_fe2_iface, button_fe2_part)
BUTTON_FE_BOILERPLATE(button_fe3, button_fe3_iface, button_fe3_part)
BUTTON_FE_BOILERPLATE(button_fe4, button_fe4_iface, button_fe4_part)
BUTTON_FE_BOILERPLATE(button_theme, struct button_theme_part, button_theme_part)
static HRESULT WINAPI button_fe2_get_requested_theme(button_fe2_iface *This, INT32 *out)
{ (void)This; if (!out) return E_POINTER; *out = ((struct button_fe2_part *)This)->owner->requested_theme; return S_OK; }
static HRESULT WINAPI button_fe2_put_requested_theme(button_fe2_iface *This, INT32 value)
{ ((struct button_fe2_part *)This)->owner->requested_theme = value; return S_OK; }
static HRESULT WINAPI button_fe2_add_data_context_changed(button_fe2_iface *This, IUnknown *handler, EventRegistrationToken *token)
{ if (!handler || !token) return E_POINTER; return raven_event_add(&((struct button_fe2_part *)This)->owner->data_context_changed, handler, token); }
static HRESULT WINAPI button_fe2_remove_data_context_changed(button_fe2_iface *This, EventRegistrationToken token)
{ return raven_event_remove(&((struct button_fe2_part *)This)->owner->data_context_changed, token); }
static HRESULT WINAPI button_fe2_get_binding_expression(button_fe2_iface *This, void *dp, void **out)
{ (void)This; (void)dp; if (!out) return E_POINTER; *out = NULL; return S_OK; }
static HRESULT WINAPI button_fe3_add_loading(button_fe3_iface *This, IUnknown *handler, EventRegistrationToken *token)
{ if (!handler || !token) return E_POINTER; return raven_event_add(&((struct button_fe3_part *)This)->owner->loading, handler, token); }
static HRESULT WINAPI button_fe3_remove_loading(button_fe3_iface *This, EventRegistrationToken token)
{ return raven_event_remove(&((struct button_fe3_part *)This)->owner->loading, token); }
#define BUTTON_FE4_INT(name, field) \
	static HRESULT WINAPI button_fe4_get_##name(button_fe4_iface *This, INT32 *out) \
	{ (void)This; if (!out) return E_POINTER; *out = ((struct button_fe4_part *)This)->owner->field; return S_OK; } \
	static HRESULT WINAPI button_fe4_put_##name(button_fe4_iface *This, INT32 value) \
	{ ((struct button_fe4_part *)This)->owner->field = value; return S_OK; }
BUTTON_FE4_INT(AllowFocusOnInteraction, allow_focus_on_interaction)
BUTTON_FE4_INT(AllowFocusWhenDisabled, allow_focus_when_disabled)
#undef BUTTON_FE4_INT
#define BUTTON_FE4_THICKNESS(name, field) \
	static HRESULT WINAPI button_fe4_get_##name(button_fe4_iface *This, struct button_framework_thickness *out) \
	{ (void)This; if (!out) return E_POINTER; *out = ((struct button_fe4_part *)This)->owner->field; return S_OK; } \
	static HRESULT WINAPI button_fe4_put_##name(button_fe4_iface *This, struct button_framework_thickness value) \
	{ ((struct button_fe4_part *)This)->owner->field = value; return S_OK; }
BUTTON_FE4_THICKNESS(FocusVisualMargin, focus_visual_margin)
BUTTON_FE4_THICKNESS(FocusVisualSecondaryThickness, focus_visual_secondary_thickness)
BUTTON_FE4_THICKNESS(FocusVisualPrimaryThickness, focus_visual_primary_thickness)
#undef BUTTON_FE4_THICKNESS
#define BUTTON_FE4_BRUSH(name, field) \
	static HRESULT WINAPI button_fe4_get_##name(button_fe4_iface *This, IInspectable **out) \
	{ if (!out) return E_POINTER; *out = ((struct button_fe4_part *)This)->owner->field; if (*out) IInspectable_AddRef(*out); return S_OK; } \
	static HRESULT WINAPI button_fe4_put_##name(button_fe4_iface *This, IInspectable *value) \
	{ \
		struct button *self = ((struct button_fe4_part *)This)->owner; \
		if (value) IInspectable_AddRef(value); \
		if (self->field) IInspectable_Release(self->field); \
		self->field = value; return S_OK; \
	}
BUTTON_FE4_BRUSH(FocusVisualSecondaryBrush, focus_visual_secondary_brush)
BUTTON_FE4_BRUSH(FocusVisualPrimaryBrush, focus_visual_primary_brush)
#undef BUTTON_FE4_BRUSH
static HRESULT WINAPI button_theme_get_actual_theme(struct button_theme_part *This, INT32 *out)
{ (void)This; if (!out) return E_POINTER; *out = 1; return S_OK; } /* the app theme the frame's face answers */
static HRESULT WINAPI button_theme_add_actual_theme_changed(struct button_theme_part *This, IUnknown *handler, EventRegistrationToken *token)
{ if (!handler || !token) return E_POINTER; return raven_event_add(&((struct button_theme_part *)This)->owner->actual_theme_changed, handler, token); }
static HRESULT WINAPI button_theme_remove_actual_theme_changed(struct button_theme_part *This, EventRegistrationToken token)
{ return raven_event_remove(&((struct button_theme_part *)This)->owner->actual_theme_changed, token); }
static const struct button_fe2_vtbl button_fe2_table = {
	button_fe2_query, button_fe2_addref, button_fe2_release,
	button_fe2_iids, button_fe2_name, button_fe2_trust,
	button_fe2_get_requested_theme, button_fe2_put_requested_theme,
	button_fe2_add_data_context_changed, button_fe2_remove_data_context_changed,
	button_fe2_get_binding_expression,
};
static const struct button_fe3_vtbl button_fe3_table = {
	button_fe3_query, button_fe3_addref, button_fe3_release,
	button_fe3_iids, button_fe3_name, button_fe3_trust,
	button_fe3_add_loading, button_fe3_remove_loading,
};
static const struct button_fe4_vtbl button_fe4_table = {
	button_fe4_query, button_fe4_addref, button_fe4_release,
	button_fe4_iids, button_fe4_name, button_fe4_trust,
	button_fe4_get_AllowFocusOnInteraction, button_fe4_put_AllowFocusOnInteraction,
	button_fe4_get_FocusVisualMargin, button_fe4_put_FocusVisualMargin,
	button_fe4_get_FocusVisualSecondaryThickness, button_fe4_put_FocusVisualSecondaryThickness,
	button_fe4_get_FocusVisualPrimaryThickness, button_fe4_put_FocusVisualPrimaryThickness,
	button_fe4_get_FocusVisualSecondaryBrush, button_fe4_put_FocusVisualSecondaryBrush,
	button_fe4_get_FocusVisualPrimaryBrush, button_fe4_put_FocusVisualPrimaryBrush,
	button_fe4_get_AllowFocusWhenDisabled, button_fe4_put_AllowFocusWhenDisabled,
};
static const struct button_theme_vtbl button_theme_table = {
	button_theme_query, button_theme_addref, button_theme_release,
	button_theme_iids, button_theme_name, button_theme_trust,
	button_theme_get_actual_theme, button_theme_add_actual_theme_changed,
	button_theme_remove_actual_theme_changed,
};

/* The IFrameworkElement base face's members, in the winmd slot order the frame's face documents. */
static HRESULT WINAPI button_fw_query(struct button_framework_part *This, REFIID iid, void **out)
{ return button_query_any(This->owner, iid, out); }
static ULONG WINAPI button_fw_addref(struct button_framework_part *This)
{ return button_addref(This->owner); }
static ULONG WINAPI button_fw_release(struct button_framework_part *This)
{ return button_release(This->owner); }
static HRESULT WINAPI button_fw_iids(struct button_framework_part *This, ULONG *count, IID **iids)
{ return element_honest_iids((element_query_function)button_query_any, This->owner, count, iids); }
static HRESULT WINAPI button_fw_name_iface(struct button_framework_part *This, HSTRING *name)
{ (void)This; if (!name) return E_POINTER; return WindowsCreateString(button_class, (UINT32)lstrlenW(button_class), name); }
static HRESULT WINAPI button_fw_trust_iface(struct button_framework_part *This, TrustLevel *level)
{ (void)This; return inspectable_trust(level); }
static HRESULT WINAPI button_fw_triggers(struct button_framework_part *This, IInspectable **out)
{ (void)This; if (!out) return E_POINTER; *out = NULL; return E_NOTIMPL; }
static HRESULT WINAPI button_fw_get_resources(struct button_framework_part *This, IInspectable **out)
{ if (!out) return E_POINTER; *out = This->owner->resources; if (*out) IInspectable_AddRef(*out); return S_OK; }
static HRESULT WINAPI button_fw_put_resources(struct button_framework_part *This, IInspectable *value)
{
	struct button *self = This->owner;
	if (value) IInspectable_AddRef(value);
	if (self->resources) IInspectable_Release(self->resources);
	self->resources = value;
	return S_OK;
}
#define BUTTON_FW_OBJECT(name, field) \
	static HRESULT WINAPI button_fw_get_##name(struct button_framework_part *This, IInspectable **out) \
	{ if (!out) return E_POINTER; *out = This->owner->field; if (*out) IInspectable_AddRef(*out); return S_OK; } \
	static HRESULT WINAPI button_fw_put_##name(struct button_framework_part *This, IInspectable *value) \
	{ \
		struct button *self = This->owner; \
		if (value) IInspectable_AddRef(value); \
		if (self->field) IInspectable_Release(self->field); \
		self->field = value; return S_OK; \
	}
BUTTON_FW_OBJECT(Tag, fw_tag)
BUTTON_FW_OBJECT(DataContext, fw_data_context)
BUTTON_FW_OBJECT(Style, style)
#undef BUTTON_FW_OBJECT
static HRESULT WINAPI button_fw_get_language(struct button_framework_part *This, HSTRING *out)
{ (void)This; if (!out) return E_POINTER; *out = NULL; return S_OK; }
static HRESULT WINAPI button_fw_put_language(struct button_framework_part *This, HSTRING value)
{ (void)This; (void)value; return S_OK; }
#define BUTTON_FW_ACTUAL(name, field) \
	static HRESULT WINAPI button_fw_get_##name(struct button_framework_part *This, double *out) \
	{ if (!out) return E_POINTER; *out = (double)This->owner->field; return S_OK; }
BUTTON_FW_ACTUAL(ActualWidth, width)
BUTTON_FW_ACTUAL(ActualHeight, height)
#undef BUTTON_FW_ACTUAL
#define BUTTON_FW_DOUBLE(name, field) \
	static HRESULT WINAPI button_fw_get_##name(struct button_framework_part *This, double *out) \
	{ if (!out) return E_POINTER; *out = This->owner->field; return S_OK; } \
	static HRESULT WINAPI button_fw_put_##name(struct button_framework_part *This, double value) \
	{ This->owner->field = value; return S_OK; }
BUTTON_FW_DOUBLE(Width, fw_width)
BUTTON_FW_DOUBLE(Height, fw_height)
BUTTON_FW_DOUBLE(MinWidth, fw_min_width)
BUTTON_FW_DOUBLE(MaxWidth, fw_max_width)
BUTTON_FW_DOUBLE(MinHeight, fw_min_height)
BUTTON_FW_DOUBLE(MaxHeight, fw_max_height)
#undef BUTTON_FW_DOUBLE
#define BUTTON_FW_INT(name, field) \
	static HRESULT WINAPI button_fw_get_##name(struct button_framework_part *This, INT32 *out) \
	{ if (!out) return E_POINTER; *out = This->owner->field; return S_OK; } \
	static HRESULT WINAPI button_fw_put_##name(struct button_framework_part *This, INT32 value) \
	{ This->owner->field = value; return S_OK; }
BUTTON_FW_INT(HorizontalAlignment, fw_horizontal_alignment)
BUTTON_FW_INT(VerticalAlignment, fw_vertical_alignment)
BUTTON_FW_INT(FlowDirection, fw_flow_direction)
#undef BUTTON_FW_INT
static HRESULT WINAPI button_fw_get_margin(struct button_framework_part *This, struct button_framework_thickness *out)
{ if (!out) return E_POINTER; *out = This->owner->fw_margin; return S_OK; }
static HRESULT WINAPI button_fw_put_margin(struct button_framework_part *This, struct button_framework_thickness value)
{ This->owner->fw_margin = value; return S_OK; }
static HRESULT WINAPI button_fw_get_name(struct button_framework_part *This, HSTRING *out)
{ if (!out) return E_POINTER; return WindowsDuplicateString(This->owner->name, out); }
static HRESULT WINAPI button_fw_put_name(struct button_framework_part *This, HSTRING value)
{
	struct button *self = This->owner;
	HSTRING copy = NULL;
	HRESULT hr = WindowsDuplicateString(value, &copy);
	if (FAILED(hr)) return hr;
	if (self->name) WindowsDeleteString(self->name);
	self->name = copy;
	return S_OK;
}
static HRESULT WINAPI button_fw_get_base_uri(struct button_framework_part *This, IInspectable **out)
{ (void)This; if (!out) return E_POINTER; *out = NULL; return S_OK; }
static HRESULT WINAPI button_fw_get_parent(struct button_framework_part *This, IInspectable **out)
{ (void)This; if (!out) return E_POINTER; *out = NULL; return S_OK; }
#define BUTTON_FW_EVENT(name, list) \
	static HRESULT WINAPI button_fw_add_##name(struct button_framework_part *This, IUnknown *handler, EventRegistrationToken *token) \
	{ if (!handler || !token) return E_POINTER; return raven_event_add(&This->owner->list, handler, token); } \
	static HRESULT WINAPI button_fw_remove_##name(struct button_framework_part *This, EventRegistrationToken token) \
	{ return raven_event_remove(&This->owner->list, token); }
BUTTON_FW_EVENT(loaded, fw_loaded)
BUTTON_FW_EVENT(unloaded, fw_unloaded)
BUTTON_FW_EVENT(size_changed, fw_size_changed)
BUTTON_FW_EVENT(layout_updated, fw_layout_updated)
#undef BUTTON_FW_EVENT
static HRESULT WINAPI button_fw_find_name(struct button_framework_part *This, HSTRING name, IInspectable **out)
{ (void)This; (void)name; if (!out) return E_POINTER; *out = NULL; return E_NOTIMPL; }
static HRESULT WINAPI button_fw_set_binding(struct button_framework_part *This, void *dp, void *binding)
{ (void)This; (void)dp; (void)binding; return E_NOTIMPL; }
/* The part's member functions take the part-typed first parameter; the IInspectable vtbl's slots cast them the
 * way the engine's part tables cast their typed handlers ((element_query_function) and friends). */
typedef HRESULT (STDMETHODCALLTYPE *button_fw_qi_t)(IInspectable *, REFIID, void **);
typedef ULONG (STDMETHODCALLTYPE *button_fw_ref_t)(IInspectable *);
typedef HRESULT (STDMETHODCALLTYPE *button_fw_iids_t)(IInspectable *, ULONG *, IID **);
typedef HRESULT (STDMETHODCALLTYPE *button_fw_name_t)(IInspectable *, HSTRING *);
typedef HRESULT (STDMETHODCALLTYPE *button_fw_trust_t)(IInspectable *, TrustLevel *);
static const struct button_framework_vtbl button_framework_table = {
	{
		(button_fw_qi_t)button_fw_query,
		(button_fw_ref_t)button_fw_addref,
		(button_fw_ref_t)button_fw_release,
		(button_fw_iids_t)button_fw_iids,
		(button_fw_name_t)button_fw_name_iface,
		(button_fw_trust_t)button_fw_trust_iface,
	},
	{
		(void *)button_fw_triggers,                 /* 6  get_Triggers */
		(void *)button_fw_get_resources,            /* 7  get_Resources */
		(void *)button_fw_put_resources,            /* 8  put_Resources */
		(void *)button_fw_get_Tag,                  /* 9  get_Tag */
		(void *)button_fw_put_Tag,                  /* 10 put_Tag */
		(void *)button_fw_get_language,             /* 11 get_Language */
		(void *)button_fw_put_language,             /* 12 put_Language */
		(void *)button_fw_get_ActualWidth,          /* 13 get_ActualWidth */
		(void *)button_fw_get_ActualHeight,         /* 14 get_ActualHeight */
		(void *)button_fw_get_Width,                /* 15 get_Width */
		(void *)button_fw_put_Width,                /* 16 put_Width */
		(void *)button_fw_get_Height,               /* 17 get_Height */
		(void *)button_fw_put_Height,               /* 18 put_Height */
		(void *)button_fw_get_MinWidth,             /* 19 get_MinWidth */
		(void *)button_fw_put_MinWidth,             /* 20 put_MinWidth */
		(void *)button_fw_get_MaxWidth,             /* 21 get_MaxWidth */
		(void *)button_fw_put_MaxWidth,             /* 22 put_MaxWidth */
		(void *)button_fw_get_MinHeight,            /* 23 get_MinHeight */
		(void *)button_fw_put_MinHeight,            /* 24 put_MinHeight */
		(void *)button_fw_get_MaxHeight,            /* 25 get_MaxHeight */
		(void *)button_fw_put_MaxHeight,            /* 26 put_MaxHeight */
		(void *)button_fw_get_HorizontalAlignment,  /* 27 get_HorizontalAlignment */
		(void *)button_fw_put_HorizontalAlignment,  /* 28 put_HorizontalAlignment */
		(void *)button_fw_get_VerticalAlignment,    /* 29 get_VerticalAlignment */
		(void *)button_fw_put_VerticalAlignment,    /* 30 put_VerticalAlignment */
		(void *)button_fw_get_margin,               /* 31 get_Margin */
		(void *)button_fw_put_margin,               /* 32 put_Margin */
		(void *)button_fw_get_name,                 /* 33 get_Name */
		(void *)button_fw_put_name,                 /* 34 put_Name */
		(void *)button_fw_get_base_uri,             /* 35 get_BaseUri */
		(void *)button_fw_get_DataContext,          /* 36 get_DataContext */
		(void *)button_fw_put_DataContext,          /* 37 put_DataContext */
		(void *)button_fw_get_Style,                /* 38 get_Style */
		(void *)button_fw_put_Style,                /* 39 put_Style */
		(void *)button_fw_get_parent,               /* 40 get_Parent */
		(void *)button_fw_get_FlowDirection,        /* 41 get_FlowDirection */
		(void *)button_fw_put_FlowDirection,        /* 42 put_FlowDirection */
		(void *)button_fw_add_loaded, (void *)button_fw_remove_loaded,
		(void *)button_fw_add_unloaded, (void *)button_fw_remove_unloaded,
		(void *)button_fw_add_size_changed, (void *)button_fw_remove_size_changed,
		(void *)button_fw_add_layout_updated, (void *)button_fw_remove_layout_updated,
		(void *)button_fw_find_name,                /* 51 FindName */
		(void *)button_fw_set_binding               /* 52 SetBinding */
	}
};
/* IButton declares no members of its own. */

/* The RoutedEventArgs snapshot a Click raise carries: an IInspectable with the runtime class name, since the event carries no
 * data until the routed-event stage of the engine. */
struct routed_arguments {
	const IInspectableVtbl *lpVtbl;
	LONG refs;
};

static HRESULT WINAPI routed_query(IInspectable *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable))
		return E_NOINTERFACE;
	InterlockedIncrement(&((struct routed_arguments *)iface)->refs);
	*out = iface;
	return S_OK;
}
static ULONG WINAPI routed_addref(IInspectable *iface)
{
	return (ULONG)InterlockedIncrement(&((struct routed_arguments *)iface)->refs);
}
static ULONG WINAPI routed_release(IInspectable *iface)
{
	struct routed_arguments *self = (struct routed_arguments *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}
static HRESULT WINAPI routed_iids(IInspectable *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}
static HRESULT WINAPI routed_name(IInspectable *iface, HSTRING *name)
{
	static const WCHAR routed_class[] = L"Windows.UI.Xaml.RoutedEventArgs";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(routed_class, (UINT32)lstrlenW(routed_class), name);
}
static HRESULT WINAPI routed_trust(IInspectable *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}
static const IInspectableVtbl routed_table = {
	routed_query, routed_addref, routed_release, routed_iids, routed_name, routed_trust
};

static IInspectable *create_routed_arguments(void)
{
	struct routed_arguments *args = ALLOC(sizeof *args);
	if (!args)
		return NULL;
	args->lpVtbl = &routed_table;
	args->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	return (IInspectable *)args;
}

static void button_raise_click(struct button *self)
{
	IUnknown **handlers = NULL;
	ULONG count = 0;
	if (FAILED(raven_event_snapshot(&self->click, &handlers, &count)))
		return;
	if (count) {
		IInspectable *args = create_routed_arguments();
		if (args) {
			for (ULONG index = 0; index < count; ++index) {
				__FITypedEventHandler_2_Windows__CObject_Windows__CUI__CXaml__CRoutedEventArgs *handler =
					(void *)handlers[index];
				handler->lpVtbl->Invoke(handler, (IInspectable *)&self->first, (__x_ABI_CWindows_CUI_CXaml_CRoutedEventArgs *)args);
				IUnknown_Release(handlers[index]);
			}
			args->lpVtbl->Release(args);
		} else {
			for (ULONG index = 0; index < count; ++index)
				IUnknown_Release(handlers[index]);
		}
	}
	FREE(handlers);
}

/* The automation raise: the peer's Invoke reaches the button's Click through the button's own raise, the way the pointer
 * behavior does, but independent of the click mode — the real ButtonAutomationPeer.Invoke calls OnClick directly, so a
 * Release-mode button still clicks. A non-button answers nothing. */
void raven_xaml_button_invoke(IInspectable *element)
{
	button1 *button = NULL;
	if (!element || FAILED(element->lpVtbl->QueryInterface(element,
		&IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton, (void **)&button)) || !button)
		return;
	button_raise_click(((struct button1_part *)button)->owner);
	button->lpVtbl->Release(button);
}

/* The internal pointer behavior of the button, run before the application's handlers: IsPressed follows the press, and Click
 * follows the click mode — ClickMode.Release raises on a release within the button's bounds (the press reached the button
 * because it was the hit target, so only the release position needs the bounds check), ClickMode.Press on the press,
 * ClickMode.Hover on the entry. */
static void button_pointer_behavior(void *owner, IInspectable *element, enum raven_element_pointer kind,
	__x_ABI_CWindows_CUI_CCore_CIPointerEventArgs *args)
{
	struct button *self = owner;
	switch (kind) {
	case RAVEN_ELEMENT_POINTER_ENTERED:
		self->is_pointer_over = TRUE;
		if (self->click_mode == raven_xaml_click_hover)
			button_raise_click(self);
		break;
	case RAVEN_ELEMENT_POINTER_EXITED:
		self->is_pointer_over = FALSE;
		break;
	case RAVEN_ELEMENT_POINTER_PRESSED:
		self->is_pressed = TRUE;
		if (self->click_mode == raven_xaml_click_press)
			button_raise_click(self);
		break;
	case RAVEN_ELEMENT_POINTER_RELEASED: {
		const boolean was_pressed = self->is_pressed;
		self->is_pressed = FALSE;
		if (self->click_mode == raven_xaml_click_release && was_pressed) {
			__x_ABI_CWindows_CUI_CInput_CIPointerPoint *point = NULL;
			__x_ABI_CWindows_CFoundation_CPoint position = {0, 0};
			if (__x_ABI_CWindows_CUI_CCore_CIPointerEventArgs_get_CurrentPoint(args, &point) == S_OK && point) {
				__x_ABI_CWindows_CUI_CInput_CIPointerPoint_get_Position(point, &position);
				point->lpVtbl->Release(point);
			}
			if (raven_input_element_contains(element, &position))
				button_raise_click(self);
		}
		break;
	}
	default:
		break;
	}
}

static HRESULT WINAPI base_get_click_mode(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t out)
{
	struct button *self = ((struct buttonbase_part *)This)->owner;
	if (!out)
		return E_POINTER;
	*(enum raven_xaml_click_mode *)(uintptr_t)out = (enum raven_xaml_click_mode)self->click_mode;
	return S_OK;
}
static HRESULT WINAPI base_put_click_mode(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t value)
{
	struct button *self = ((struct buttonbase_part *)This)->owner;
	if (value > raven_xaml_click_hover)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:1551\n"); return E_INVALIDARG; }
	self->click_mode = (UINT32)value;
	return S_OK;
}
static HRESULT WINAPI base_get_is_pointer_over(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t out)
{
	struct button *self = ((struct buttonbase_part *)This)->owner;
	if (!out)
		return E_POINTER;
	*(boolean *)(uintptr_t)out = self->is_pointer_over;
	return S_OK;
}
static HRESULT WINAPI base_get_is_pressed(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t out)
{
	struct button *self = ((struct buttonbase_part *)This)->owner;
	if (!out)
		return E_POINTER;
	*(boolean *)(uintptr_t)out = self->is_pressed;
	return S_OK;
}
static HRESULT WINAPI base_get_command(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t out)
{
	struct button *self = ((struct buttonbase_part *)This)->owner;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = self->command;
	if (self->command)
		IInspectable_AddRef(self->command);
	return S_OK;
}
static HRESULT WINAPI base_put_command(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t value)
{
	struct button *self = ((struct buttonbase_part *)This)->owner;
	IInspectable *command = (IInspectable *)(uintptr_t)value;
	if (command)
		IInspectable_AddRef(command);
	if (self->command)
		IInspectable_Release(self->command);
	self->command = command;
	return S_OK;
}
static HRESULT WINAPI base_get_command_parameter(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t out)
{
	struct button *self = ((struct buttonbase_part *)This)->owner;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = self->command_parameter;
	if (self->command_parameter)
		IInspectable_AddRef(self->command_parameter);
	return S_OK;
}
static HRESULT WINAPI base_put_command_parameter(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t value)
{
	struct button *self = ((struct buttonbase_part *)This)->owner;
	IInspectable *parameter = (IInspectable *)(uintptr_t)value;
	if (parameter)
		IInspectable_AddRef(parameter);
	if (self->command_parameter)
		IInspectable_Release(self->command_parameter);
	self->command_parameter = parameter;
	return S_OK;
}
static HRESULT WINAPI base_add_click(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t handler, uint64_t token)
{
	struct button *self = ((struct buttonbase_part *)This)->owner;
	return raven_event_add(&self->click, (IUnknown *)(uintptr_t)handler, (EventRegistrationToken *)(uintptr_t)token);
}
static HRESULT WINAPI base_remove_click(__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *This, uint64_t token)
{
	struct button *self = ((struct buttonbase_part *)This)->owner;
	EventRegistrationToken value;
	value.value = (INT64)token;
	return raven_event_remove(&self->click, value);
}

static HRESULT WINAPI button_content(content_control *iface, uint64_t out)
{
	struct button *self = ((struct content_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = self->content_value;
	if (self->content_value)
		IInspectable_AddRef(self->content_value);
	return S_OK;
}

static HRESULT WINAPI button_put_content(content_control *iface, uint64_t value)
{
	struct button *self = ((struct content_part *)iface)->owner;
	IInspectable *object = (IInspectable *)(uintptr_t)value;
	if (object)
		IInspectable_AddRef(object);
	if (self->content_value)
		IInspectable_Release(self->content_value);
	self->content_value = object;
	return S_OK;
}

STUB1(content_control, content_template)
STUB1(content_control, content_put_template)
STUB1(content_control, content_selector)
STUB1(content_control, content_put_selector)
STUB1(content_control, content_transitions)
STUB1(content_control, content_put_transitions)

/* The Template property of IControl: the put owns the reference and validates the kind — a value that is no engine
 * ControlTemplate answers E_INVALIDARG and the previous template stays, the wrong-kind convention the style application
 * rides. Re-put replaces, a NULL put clears; the render follows through the visual-child hook without a re-apply of
 * anything else. */
/* The template-root presenter walk is defined with the ContentPresenter element below; the store runs it in both
 * directions — unbind the replaced root, bind the new one. */
static void presenter_bind_walk(IInspectable *element, IInspectable *owner, int clear, int depth);

/* The element-agnostic template store: every engine element that carries a template slot stores through here, so the
 * presenter binding rides the store and not the button — the loader's deferred template-scope application, the style
 * applier and the direct put all land on the same walk. The store takes the OWNER element (the IInspectable the
 * presenters' non-owning back-pointer answers) and the slot; today the button is the only engine element with a
 * template slot (raven_xaml_element_put_template dispatches to it), and a kind that gains one rides this store
 * unchanged. The presenters inside the replaced root lose their back-pointer first: the old template releases with the
 * swap, and a binding that outlived the owning element would dangle. The bindings of the new root follow the store —
 * the loader defers the apply until the template's root is attached, so every store sees the final tree. */
static void element_store_template(IInspectable *owner, IInspectable **slot, IInspectable *template_object)
{
	IInspectable *old_root = NULL;
	if (*slot && raven_xaml_control_template_root(*slot, &old_root) == S_OK && old_root) {
		presenter_bind_walk(old_root, NULL, 1, 0);
		old_root->lpVtbl->Release(old_root);
	}
	if (template_object)
		template_object->lpVtbl->AddRef(template_object);
	if (*slot)
		(*slot)->lpVtbl->Release(*slot);
	*slot = template_object;
	if (template_object) {
		IInspectable *new_root = NULL;
		if (raven_xaml_control_template_root(template_object, &new_root) == S_OK && new_root) {
			presenter_bind_walk(new_root, owner, 0, 0);
			new_root->lpVtbl->Release(new_root);
		}
	}
	raven_render_changed();
}

static void button_store_template(struct button *self, IInspectable *template_object)
{
	element_store_template((IInspectable *)&self->first, &self->template_value, template_object);
}

static HRESULT button_put_template(struct button *self, IInspectable *template_object)
{
	IInspectable *root = NULL;
	HRESULT kind;
	if (!template_object) {
		button_store_template(self, NULL);
		return S_OK;
	}
	kind = raven_xaml_control_template_root(template_object, &root);
	if (kind != S_OK)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:1709\n"); return E_INVALIDARG; }
	if (root)
		root->lpVtbl->Release(root);
	button_store_template(self, template_object);
	return S_OK;
}

static HRESULT WINAPI control_get_template(control1 *This, uint64_t out)
{
	struct button *self = ((struct control_part *)This)->owner;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = self->template_value;
	if (self->template_value)
		self->template_value->lpVtbl->AddRef(self->template_value);
	return S_OK;
}

static HRESULT WINAPI control_put_template(control1 *This, uint64_t value)
{
	return button_put_template(((struct control_part *)This)->owner, (IInspectable *)(uintptr_t)value);
}

/* The IControl members the engine does not store answer E_NOTIMPL: the slots carry the exact interface order, the values
 * wait for the stages that will read them. */
#define CONTROL_STUB(name) static HRESULT WINAPI name(control1 *This, uint64_t a1) \
	{ (void)This; (void)a1; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
#define CONTROL_STUB2(name) static HRESULT WINAPI name(control1 *This, uint64_t a1, uint64_t a2) \
	{ (void)This; (void)a1; (void)a2; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }

CONTROL_STUB(control_font_size_get)
CONTROL_STUB(control_font_size_put)
CONTROL_STUB(control_font_family_get)
CONTROL_STUB(control_font_family_put)
CONTROL_STUB(control_font_weight_get)
CONTROL_STUB(control_font_weight_put)
CONTROL_STUB(control_font_style_get)
CONTROL_STUB(control_font_style_put)
CONTROL_STUB(control_font_stretch_get)
CONTROL_STUB(control_font_stretch_put)
CONTROL_STUB(control_character_spacing_get)
CONTROL_STUB(control_character_spacing_put)
CONTROL_STUB(control_foreground_get)
CONTROL_STUB(control_foreground_put)
CONTROL_STUB(control_is_tab_stop_get)
CONTROL_STUB(control_is_tab_stop_put)
CONTROL_STUB(control_is_enabled_get)
CONTROL_STUB(control_is_enabled_put)
CONTROL_STUB(control_tab_index_get)
CONTROL_STUB(control_tab_index_put)
CONTROL_STUB(control_tab_navigation_get)
CONTROL_STUB(control_tab_navigation_put)
CONTROL_STUB(control_padding_get)
CONTROL_STUB(control_padding_put)
CONTROL_STUB(control_horizontal_alignment_get)
CONTROL_STUB(control_horizontal_alignment_put)
CONTROL_STUB(control_vertical_alignment_get)
CONTROL_STUB(control_vertical_alignment_put)
static HRESULT WINAPI control_background_get(control1 *iface, IInspectable **value)
{ (void)iface; (void)value; return E_NOTIMPL; }
static HRESULT WINAPI control_background_put(control1 *iface, IInspectable *value)
{ (void)iface; (void)value; return E_NOTIMPL; }
CONTROL_STUB(control_border_thickness_get)
CONTROL_STUB(control_border_thickness_put)
CONTROL_STUB(control_border_brush_get)
CONTROL_STUB(control_border_brush_put)
CONTROL_STUB(control_focus_state_get)
/* IsEnabledChanged rides the shared event machinery: the stub's flattened slot 43 (the six IInspectable
 * methods plus this interface's own thirty-seventh member, the winmd's declaration order — the interface
 * does not flatten its ancestors' members) lands here with the handler as the marshal's CCW and the token
 * out-pointer packed the way the Click pair packs it. */
static HRESULT WINAPI control_is_enabled_changed_add(control1 *This, uint64_t handler, uint64_t token)
{
	return raven_event_add(&((struct control_part *)This)->owner->is_enabled_changed,
		(IUnknown *)(uintptr_t)handler, (EventRegistrationToken *)(uintptr_t)token);
}
static HRESULT WINAPI control_is_enabled_changed_remove(control1 *This, uint64_t token)
{
	EventRegistrationToken value;
	value.value = (INT64)token;
	return raven_event_remove(&((struct control_part *)This)->owner->is_enabled_changed, value);
}
CONTROL_STUB(control_apply_template)
CONTROL_STUB2(control_focus)

static const control1_vtbl control_table = {
	.QueryInterface = controlp_query,
	.AddRef = controlp_addref,
	.Release = controlp_release,
	.GetIids = controlp_iids,
	.GetRuntimeClassName = controlp_name,
	.GetTrustLevel = controlp_trust,
	.get_FontSize = control_font_size_get,
	.put_FontSize = control_font_size_put,
	.get_FontFamily = control_font_family_get,
	.put_FontFamily = control_font_family_put,
	.get_FontWeight = control_font_weight_get,
	.put_FontWeight = control_font_weight_put,
	.get_FontStyle = control_font_style_get,
	.put_FontStyle = control_font_style_put,
	.get_FontStretch = control_font_stretch_get,
	.put_FontStretch = control_font_stretch_put,
	.get_CharacterSpacing = control_character_spacing_get,
	.put_CharacterSpacing = control_character_spacing_put,
	.get_Foreground = control_foreground_get,
	.put_Foreground = control_foreground_put,
	.get_IsTabStop = control_is_tab_stop_get,
	.put_IsTabStop = control_is_tab_stop_put,
	.get_IsEnabled = control_is_enabled_get,
	.put_IsEnabled = control_is_enabled_put,
	.get_TabIndex = control_tab_index_get,
	.put_TabIndex = control_tab_index_put,
	.get_TabNavigation = control_tab_navigation_get,
	.put_TabNavigation = control_tab_navigation_put,
	.get_Template = control_get_template,
	.put_Template = control_put_template,
	.get_Padding = control_padding_get,
	.put_Padding = control_padding_put,
	.get_HorizontalContentAlignment = control_horizontal_alignment_get,
	.put_HorizontalContentAlignment = control_horizontal_alignment_put,
	.get_VerticalContentAlignment = control_vertical_alignment_get,
	.put_VerticalContentAlignment = control_vertical_alignment_put,
	.get_Background = control_background_get,
	.put_Background = control_background_put,
	.get_BorderThickness = control_border_thickness_get,
	.put_BorderThickness = control_border_thickness_put,
	.get_BorderBrush = control_border_brush_get,
	.put_BorderBrush = control_border_brush_put,
	.get_FocusState = control_focus_state_get,
	.add_IsEnabledChanged = control_is_enabled_changed_add,
	.remove_IsEnabledChanged = control_is_enabled_changed_remove,
	.ApplyTemplate = control_apply_template,
	.Focus = control_focus,
};

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIButtonVtbl button1_table = {
	.QueryInterface = buttonp_query,
	.AddRef = buttonp_addref,
	.Release = buttonp_release,
	.GetIids = buttonp_iids,
	.GetRuntimeClassName = buttonp_name,
	.GetTrustLevel = buttonp_trust,
};

/* IButtonBase, generated from the Primitives namespace of the same winmd: the click mode, the press and hover state, the
 * command plumbing and the Click handlers. */
static const __x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBaseVtbl buttonbase_table = {
	.QueryInterface = basep_query,
	.AddRef = basep_addref,
	.Release = basep_release,
	.GetIids = basep_iids,
	.GetRuntimeClassName = basep_name,
	.GetTrustLevel = basep_trust,
	.get_ClickMode = base_get_click_mode,
	.put_ClickMode = base_put_click_mode,
	.get_IsPointerOver = base_get_is_pointer_over,
	.get_IsPressed = base_get_is_pressed,
	.get_Command = base_get_command,
	.put_Command = base_put_command,
	.get_CommandParameter = base_get_command_parameter,
	.put_CommandParameter = base_put_command_parameter,
	.add_Click = base_add_click,
	.remove_Click = base_remove_click,
};

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControlVtbl button_content_table = {
	.QueryInterface = contentp_query,
	.AddRef = contentp_addref,
	.Release = contentp_release,
	.GetIids = contentp_iids,
	.GetRuntimeClassName = contentp_name,
	.GetTrustLevel = contentp_trust,
	.get_Content = button_content,
	.put_Content = button_put_content,
	.get_ContentTemplate = content_template,
	.put_ContentTemplate = content_put_template,
	.get_ContentTemplateSelector = content_selector,
	.put_ContentTemplateSelector = content_put_selector,
	.get_ContentTransitions = content_transitions,
	.put_ContentTransitions = content_put_transitions,
};

static struct button *create_button(void)
{
	struct button *button = ALLOC(sizeof *button);
	if (!button)
		return NULL;
	button->first.lpVtbl = &button1_table;
	button->first.owner = button;
	button->base.lpVtbl = &buttonbase_table;
	button->base.owner = button;
	button->content.lpVtbl = &button_content_table;
	button->content.owner = button;
	button->control.lpVtbl = &control_table;
	button->control.owner = button;
	button->fe2.lpVtbl = &button_fe2_table;
	button->fe2.owner = button;
	button->fe3.lpVtbl = &button_fe3_table;
	button->fe3.owner = button;
	button->fe4.lpVtbl = &button_fe4_table;
	button->fe4.owner = button;
	button->theme.lpVtbl = &button_theme_table;
	button->theme.owner = button;
	button->fw.lpVtbl = &button_framework_table;
	button->fw.owner = button;
	element_ui_init(&button->ui, button, (element_query_function)button_query_any, (element_reference_function)button_addref,
		(element_reference_function)button_release, button_class);
	button->ui.behavior = button_pointer_behavior;
	InitializeCriticalSection(&button->click.lock);
	InitializeCriticalSection(&button->is_enabled_changed.lock);
	InitializeCriticalSection(&button->data_context_changed.lock);
	InitializeCriticalSection(&button->loading.lock);
	InitializeCriticalSection(&button->actual_theme_changed.lock);
	InitializeCriticalSection(&button->fw_loaded.lock);
	InitializeCriticalSection(&button->fw_unloaded.lock);
	InitializeCriticalSection(&button->fw_size_changed.lock);
	InitializeCriticalSection(&button->fw_layout_updated.lock);
	/* The metadata defaults the frame's constructor sets: the app theme (the launch runs dark) and
	 * AllowFocusOnInteraction true; the thicknesses and brushes carry the zero/null defaults. The
	 * IFrameworkElement extents take the real defaults: Width/Height NaN (auto), Min 0, Max PositiveInfinity,
	 * the alignments Stretch, FlowDirection LeftToRight. */
	button->requested_theme = 1;
	button->allow_focus_on_interaction = 1;
	button->fw_width = button->fw_height = __builtin_nan("");
	button->fw_max_width = button->fw_max_height = __builtin_inf();
	button->fw_horizontal_alignment = button->fw_vertical_alignment = 3; /* Stretch */
	button->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	return button;
}

/* RowDefinition and ColumnDefinition: the tracks a grid lays its children into. The engine supports the fixed pixel lengths
 * only; an Auto or Star length is stored and reported with its unit and lays out as zero until the measure stage distributes
 * the free space. The Height and Width slots are the hand declarations in windows_ui_xaml_layout.h, since a GridLength crosses
 * the ABI as a 16-byte struct the compiler passes per architecture. */

struct row_definition {
	const __x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinitionVtbl *lpVtbl;
	LONG refs;
	double height;
	UINT32 unit;
	/* The extent bounds the projected face reports; MinHeight starts at 0, MaxHeight at PositiveInfinity
	 * (the real WinRT metadata defaults) and ActualHeight at 0 until a measure pass exists. */
	double min_height, max_height, actual_height;
};

struct column_definition {
	const __x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinitionVtbl *lpVtbl;
	LONG refs;
	double width;
	UINT32 unit;
	double min_width, max_width, actual_width;
};

static HRESULT WINAPI row_definition_query(row_definition1 *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition))
		return E_NOINTERFACE;
	InterlockedIncrement(&((struct row_definition *)iface)->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI row_definition_addref(row_definition1 *iface)
{
	return (ULONG)InterlockedIncrement(&((struct row_definition *)iface)->refs);
}

static ULONG WINAPI row_definition_release(row_definition1 *iface)
{
	struct row_definition *self = (struct row_definition *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI row_definition_iids(row_definition1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI row_definition_name(row_definition1 *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(row_definition_class, (UINT32)lstrlenW(row_definition_class), name);
}

static HRESULT WINAPI row_definition_trust(row_definition1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI row_height(row_definition1 *iface, __x_ABI_CWindows_CUI_CXaml_CGridLength *value)
{
	struct row_definition *self = (struct row_definition *)iface;
	if (!value)
		return E_POINTER;
	value->Value = self->height;
	value->GridUnitType = (__x_ABI_CWindows_CUI_CXaml_CGridUnitType)self->unit;
	return S_OK;
}

static HRESULT WINAPI row_put_height(row_definition1 *iface, __x_ABI_CWindows_CUI_CXaml_CGridLength value)
{
	struct row_definition *self = (struct row_definition *)iface;
	self->height = value.Value;
	self->unit = (UINT32)value.GridUnitType;
	raven_render_changed();
	return S_OK;
}

/* The bound getters answer the stored extents (the defaults the real metadata carries: MinHeight 0,
 * MaxHeight PositiveInfinity, ActualHeight 0 until a measure pass exists). The application's generated
 * getters unbox the returned double directly, so a refusal here derefs null (the traced TitleBarControl
 * c0000005 reading SystemButtonsColumn's bounds). */
static HRESULT WINAPI row_max_height(row_definition1 *iface, uint64_t out)
{
	if (!out)
		return E_POINTER;
	*(double *)(uintptr_t)out = ((struct row_definition *)iface)->max_height;
	return S_OK;
}
static HRESULT WINAPI row_put_max_height(row_definition1 *iface, double value)
{
	((struct row_definition *)iface)->max_height = value;
	raven_render_changed();
	return S_OK;
}
static HRESULT WINAPI row_min_height(row_definition1 *iface, uint64_t out)
{
	if (!out)
		return E_POINTER;
	*(double *)(uintptr_t)out = ((struct row_definition *)iface)->min_height;
	return S_OK;
}
static HRESULT WINAPI row_put_min_height(row_definition1 *iface, double value)
{
	((struct row_definition *)iface)->min_height = value;
	raven_render_changed();
	return S_OK;
}
static HRESULT WINAPI row_actual_height(row_definition1 *iface, uint64_t out)
{
	if (!out)
		return E_POINTER;
	*(double *)(uintptr_t)out = ((struct row_definition *)iface)->actual_height;
	return S_OK;
}

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinitionVtbl row_definition_table = {
	.QueryInterface = row_definition_query,
	.AddRef = row_definition_addref,
	.Release = row_definition_release,
	.GetIids = row_definition_iids,
	.GetRuntimeClassName = row_definition_name,
	.GetTrustLevel = row_definition_trust,
	.get_Height = row_height,
	.put_Height = row_put_height,
	.get_MaxHeight = row_max_height,
	.put_MaxHeight = row_put_max_height,
	.get_MinHeight = row_min_height,
	.put_MinHeight = row_put_min_height,
	.get_ActualHeight = row_actual_height,
};

static HRESULT WINAPI column_definition_query(column_definition1 *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition))
		return E_NOINTERFACE;
	InterlockedIncrement(&((struct column_definition *)iface)->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI column_definition_addref(column_definition1 *iface)
{
	return (ULONG)InterlockedIncrement(&((struct column_definition *)iface)->refs);
}

static ULONG WINAPI column_definition_release(column_definition1 *iface)
{
	struct column_definition *self = (struct column_definition *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI column_definition_iids(column_definition1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI column_definition_name(column_definition1 *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(column_definition_class, (UINT32)lstrlenW(column_definition_class), name);
}

static HRESULT WINAPI column_definition_trust(column_definition1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI column_width(column_definition1 *iface, __x_ABI_CWindows_CUI_CXaml_CGridLength *value)
{
	struct column_definition *self = (struct column_definition *)iface;
	if (!value)
		return E_POINTER;
	value->Value = self->width;
	value->GridUnitType = (__x_ABI_CWindows_CUI_CXaml_CGridUnitType)self->unit;
	return S_OK;
}

static HRESULT WINAPI column_put_width(column_definition1 *iface, __x_ABI_CWindows_CUI_CXaml_CGridLength value)
{
	struct column_definition *self = (struct column_definition *)iface;
	self->width = value.Value;
	self->unit = (UINT32)value.GridUnitType;
	raven_render_changed();
	return S_OK;
}

/* The bound getters answer the stored extents (the defaults the real metadata carries: MinWidth 0,
 * MaxWidth PositiveInfinity, ActualWidth 0 until a measure pass exists). The application's generated
 * getters unbox the returned double directly, so a refusal here derefs null (the traced TitleBarControl
 * c0000005 reading SystemButtonsColumn's bounds). */
static HRESULT WINAPI column_max_width(column_definition1 *iface, uint64_t out)
{
	if (!out)
		return E_POINTER;
	*(double *)(uintptr_t)out = ((struct column_definition *)iface)->max_width;
	return S_OK;
}
static HRESULT WINAPI column_put_max_width(column_definition1 *iface, double value)
{
	((struct column_definition *)iface)->max_width = value;
	raven_render_changed();
	return S_OK;
}
static HRESULT WINAPI column_min_width(column_definition1 *iface, uint64_t out)
{
	if (!out)
		return E_POINTER;
	*(double *)(uintptr_t)out = ((struct column_definition *)iface)->min_width;
	return S_OK;
}
static HRESULT WINAPI column_put_min_width(column_definition1 *iface, double value)
{
	((struct column_definition *)iface)->min_width = value;
	raven_render_changed();
	return S_OK;
}
static HRESULT WINAPI column_actual_width(column_definition1 *iface, uint64_t out)
{
	if (!out)
		return E_POINTER;
	*(double *)(uintptr_t)out = ((struct column_definition *)iface)->actual_width;
	return S_OK;
}

static const __x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinitionVtbl column_definition_table = {
	.QueryInterface = column_definition_query,
	.AddRef = column_definition_addref,
	.Release = column_definition_release,
	.GetIids = column_definition_iids,
	.GetRuntimeClassName = column_definition_name,
	.GetTrustLevel = column_definition_trust,
	.get_Width = column_width,
	.put_Width = column_put_width,
	.get_MaxWidth = column_max_width,
	.put_MaxWidth = column_put_max_width,
	.get_MinWidth = column_min_width,
	.put_MinWidth = column_put_min_width,
	.get_ActualWidth = column_actual_width,
};

static struct row_definition *create_row_definition(void)
{
	struct row_definition *definition = ALLOC(sizeof *definition);
	if (!definition)
		return NULL;
	definition->lpVtbl = &row_definition_table;
	definition->refs = 1;
	definition->max_height = 1.7976931348623157e+308; /* PositiveInfinity, the real metadata default */
	InterlockedIncrement(&raven_xaml_objects);
	return definition;
}

static struct column_definition *create_column_definition(void)
{
	struct column_definition *definition = ALLOC(sizeof *definition);
	if (!definition)
		return NULL;
	definition->lpVtbl = &column_definition_table;
	definition->refs = 1;
	definition->max_width = 1.7976931348623157e+308; /* PositiveInfinity, the real metadata default */
	InterlockedIncrement(&raven_xaml_objects);
	return definition;
}

/* ContentPresenter: the element a control template places to show the templated control's logical Content. The engine
 * scope is deliberately narrow — the presenter carries no chrome, no IContentPresenter surface and no framework-element
 * properties; it answers IInspectable with the class name and resolves its templated parent's Content through the same
 * raven_xaml_element_template_child hook the render walk already consults first for a templated control, so both the
 * measure and the paint pick the content up through the existing single-child path with no render-pass change.
 *
 * The templated parent is a NON-OWNING back-pointer bound when an element stores a template (element_store_template
 * walks the template root and marks every presenter inside it — the button is today the only engine element with a
 * template slot). The bound element owns the template, the template owns the root, and the root is the only thing that
 * can hold this presenter, so the pointer cannot outlive the element that owns it; the store clears the bindings of the
 * replaced root first, which covers the replace and the clear, and a presenter the application retained across a clear
 * answers no content (the pointer reads NULL) rather than a dangling one. Presenters are created through the
 * engine-internal elements factory (the loader's known-class path and the raven_xaml_content_presenter_factory seam);
 * the runtime catalog in xaml.c does not list the class yet. */
static const WCHAR presenter_class[] = L"Windows.UI.Xaml.Controls.ContentPresenter";

struct content_presenter {
	const IInspectableVtbl *lpVtbl;
	LONG refs;
	/* Non-owning; see the constraint comment above. */
	IInspectable *templated_parent;
	/* The ContentPresenter.ContentTemplate the loader stores (an owned reference; stable property 525). Applied at the
	 * template-child hook: when the presenter answers its render child and both the templated parent and its Content
	 * resolve, the content template's root is the child and its own presenters bind to the Content object (not the
	 * templated control) through the same walk element_store_template runs. NULL clears, a non-template value answers
	 * E_INVALIDARG. */
	IInspectable *content_template;
};

static HRESULT WINAPI presenter_query(IInspectable *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable))
		return E_NOINTERFACE;
	InterlockedIncrement(&((struct content_presenter *)iface)->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI presenter_addref(IInspectable *iface)
{
	return (ULONG)InterlockedIncrement(&((struct content_presenter *)iface)->refs);
}

static ULONG WINAPI presenter_release(IInspectable *iface)
{
	struct content_presenter *self = (struct content_presenter *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->content_template)
			self->content_template->lpVtbl->Release(self->content_template);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI presenter_iids(IInspectable *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI presenter_name(IInspectable *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(presenter_class, (UINT32)lstrlenW(presenter_class), name);
}

static HRESULT WINAPI presenter_trust(IInspectable *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static const IInspectableVtbl presenter_table = {
	.QueryInterface = presenter_query,
	.AddRef = presenter_addref,
	.Release = presenter_release,
	.GetIids = presenter_iids,
	.GetRuntimeClassName = presenter_name,
	.GetTrustLevel = presenter_trust,
};

static struct content_presenter *create_content_presenter(void)
{
	struct content_presenter *presenter = ALLOC(sizeof *presenter);
	if (!presenter)
		return NULL;
	presenter->lpVtbl = &presenter_table;
	presenter->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	return presenter;
}

/* The engine presenter behind an IInspectable, or NULL: the vtable is the engine's own, the way the template and the
 * button kind checks work — a foreign IInspectable answers nothing here. The caller owns its reference to object. */
static struct content_presenter *presenter_from_object(IInspectable *object)
{
	IInspectable *inspectable = NULL;
	if (!object)
		return NULL;
	if (FAILED(object->lpVtbl->QueryInterface(object, &IID_IInspectable, (void **)&inspectable)) || !inspectable)
		return NULL;
	struct content_presenter *presenter = inspectable->lpVtbl == &presenter_table ?
		(struct content_presenter *)inspectable : NULL;
	inspectable->lpVtbl->Release(inspectable);
	return presenter;
}

/* The ContentPresenter.ContentTemplate store (the loader's "ContentTemplate" object property, stable 525): an engine
 * ControlTemplate only, the same wrong-kind answer put_Template gives; re-put replaces, NULL clears. The stored
 * template applies at the template-child hook — the presenter's render child becomes the content template's root once
 * the templated parent's Content resolves, and the content template's own presenters bind to that Content object
 * lazily there (the content is unknown at store time, and it can change). Replacing or clearing the store unbinds the
 * replaced root's presenters first, the same direction element_store_template runs. S_FALSE for an object that is not
 * an engine presenter, the skip convention. */
static HRESULT presenter_store_content_template(IInspectable *object, IInspectable *value)
{
	struct content_presenter *presenter = presenter_from_object(object);
	if (!presenter)
		return S_FALSE;
	if (presenter->content_template) {
		IInspectable *old_root = NULL;
		if (raven_xaml_control_template_root(presenter->content_template, &old_root) == S_OK && old_root) {
			presenter_bind_walk(old_root, NULL, 1, 0);
			old_root->lpVtbl->Release(old_root);
		}
	}
	if (value) {
		IInspectable *root = NULL;
		const HRESULT kind = raven_xaml_control_template_root(value, &root);
		if (root)
			root->lpVtbl->Release(root);
		if (kind != S_OK)
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:2258\n"); return E_INVALIDARG; }
	}
	if (value)
		value->lpVtbl->AddRef(value);
	if (presenter->content_template)
		presenter->content_template->lpVtbl->Release(presenter->content_template);
	presenter->content_template = value;
	/* The store changes what the presenter's template-child hook answers once the content resolves, so the projection
	 * follows it the way it follows every other stored value — the same raise element_store_template runs. */
	raven_render_changed();
	return S_OK;
}

/* The fixture-facing readback of the stored ContentTemplate: S_OK with the reference (or NULL — a cleared presenter is
 * a valid answer) for an engine presenter, S_FALSE otherwise. The export only exists because the presenter carries no
 * public property surface yet; the fixture reaches it through GetProcAddress like the other engine seams. */
__declspec(dllexport) HRESULT raven_xaml_element_get_content_template(IInspectable *object, IInspectable **out)
{
	struct content_presenter *presenter = presenter_from_object(object);
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!presenter)
		return S_FALSE;
	if (presenter->content_template) {
		*out = presenter->content_template;
		(*out)->lpVtbl->AddRef(*out);
	}
	return S_OK;
}

/* The fixture-facing store behind the get above, the same store the loader's "ContentTemplate" property and the
 * elements dispatch serve: an engine ControlTemplate only (E_INVALIDARG otherwise), re-put replaces, NULL clears, and
 * the replaced root's presenters unbind first. S_FALSE for an object that is not an engine presenter, the skip
 * convention. Lives beside the get because the presenter has no public property surface — the fixtures reach both
 * through GetProcAddress like the other engine seams. */
__declspec(dllexport) HRESULT raven_xaml_element_put_content_template(IInspectable *object, IInspectable *value)
{
	if (!object)
		return E_POINTER;
	return presenter_store_content_template(object, value);
}

/* The template-root walk that binds and unbinds the presenters inside an applied template. Grids contribute their
 * children (the vector the panel serves, read directly — the walk runs on the element side where the struct is visible);
 * a presenter marks itself and contributes nothing (the engine presenter carries no tree of its own). Anything the walk
 * does not recognise stops it: other container kinds inside a template have no child readback this engine stores. The
 * depth cap mirrors the style chain's guard against a cycle. */
#define TEMPLATE_WALK_DEPTH 32

static void presenter_bind_walk(IInspectable *element, IInspectable *owner, int clear, int depth)
{
	grid1 *grid = NULL;
	if (!element || depth >= TEMPLATE_WALK_DEPTH)
		return;
	struct content_presenter *presenter = presenter_from_object(element);
	if (presenter) {
		presenter->templated_parent = clear ? NULL : owner;
		/* The clear cascades into a stored content template: when the outer tree unbinds this presenter (a replaced
		 * or cleared control template, a replaced or cleared content template), the presenters inside the content
		 * template's root must lose their back-pointer too — they answer the content only while the presenter that
		 * owns the content template is itself bound. Binding does not cascade: the content resolves at the
		 * template-child hook, so the inner presenters bind lazily there against the current Content object. */
		if (clear && presenter->content_template) {
			IInspectable *inner_root = NULL;
			if (raven_xaml_control_template_root(presenter->content_template, &inner_root) == S_OK && inner_root) {
				presenter_bind_walk(inner_root, NULL, 1, depth + 1);
				inner_root->lpVtbl->Release(inner_root);
			}
		}
		return;
	}
	if (FAILED(element->lpVtbl->QueryInterface(element, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid,
		(void **)&grid)) || !grid)
		return;
	if (grid->lpVtbl != &grid1_table) {
		grid->lpVtbl->Release(grid);
		return;
	}
	struct grid *self = ((struct grid1_part *)grid)->owner;
	grid->lpVtbl->Release(grid);
	if (!self->children)
		return;
	for (UINT32 index = 0; index < self->children->count; ++index)
		presenter_bind_walk((IInspectable *)self->children->items[index], owner, clear, depth + 1);
}

/* The visual-state tree the compiled control templates carry (ThemedImage.xbf's VisualStateManager.VisualStateGroups):
 * VisualStateGroup holds named states, VisualState holds a Setter collection. The engine stores the tree so the
 * documents load; nothing drives the states yet — GoToState arrives with the control surfaces that raise it. */
static const WCHAR visual_state_group_class[] = L"Windows.UI.Xaml.VisualStateGroup";
static const WCHAR visual_state_class[] = L"Windows.UI.Xaml.VisualState";

struct visual_state_group {
	const IInspectableVtbl *lpVtbl;
	LONG refs;
	HSTRING name;
	struct children_vector *states;
};

struct visual_state {
	const IInspectableVtbl *lpVtbl;
	LONG refs;
	HSTRING name;
	struct children_vector *setters;
};

static HRESULT WINAPI visual_state_group_query(IInspectable *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable))
		return E_NOINTERFACE;
	InterlockedIncrement(&((struct visual_state_group *)iface)->refs);
	*out = iface;
	return S_OK;
}

static HRESULT WINAPI visual_state_query(IInspectable *iface, REFIID iid, void **out)
{
	return visual_state_group_query(iface, iid, out);
}

static ULONG WINAPI visual_state_group_addref(IInspectable *iface)
{
	return (ULONG)InterlockedIncrement(&((struct visual_state_group *)iface)->refs);
}

static ULONG WINAPI visual_state_group_release(IInspectable *iface)
{
	struct visual_state_group *self = (struct visual_state_group *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->name)
			WindowsDeleteString(self->name);
		if (self->states)
			vector_release(self->states);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static ULONG WINAPI visual_state_release(IInspectable *iface)
{
	struct visual_state *self = (struct visual_state *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->name)
			WindowsDeleteString(self->name);
		if (self->setters)
			vector_release(self->setters);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI group_name_get(IInspectable *iface, HSTRING *out)
{
	if (!out)
		return E_POINTER;
	return WindowsDuplicateString(((struct visual_state_group *)iface)->name, out);
}

static HRESULT WINAPI state_name_get(IInspectable *iface, HSTRING *out)
{
	if (!out)
		return E_POINTER;
	return WindowsDuplicateString(((struct visual_state *)iface)->name, out);
}

static HRESULT WINAPI group_iids(IInspectable *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }

static HRESULT WINAPI group_trust(IInspectable *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }

static const IInspectableVtbl visual_state_group_table = {
	visual_state_group_query, visual_state_group_addref, visual_state_group_release,
	group_iids, group_name_get, group_trust
};

static const IInspectableVtbl visual_state_table = {
	visual_state_query, visual_state_group_addref, visual_state_release,
	group_iids, state_name_get, group_trust
};

static struct visual_state_group *create_visual_state_group(void)
{
	struct visual_state_group *group = ALLOC(sizeof *group);
	if (!group)
		return NULL;
	group->lpVtbl = &visual_state_group_table;
	group->refs = 1;
	group->name = NULL;
	group->states = create_children(&IID___FIVector_1_UIElement, vector_class_name, FALSE);
	if (!group->states) {
		FREE(group);
		return NULL;
	}
	InterlockedIncrement(&raven_xaml_objects);
	return group;
}

static struct visual_state *create_visual_state(void)
{
	struct visual_state *state = ALLOC(sizeof *state);
	if (!state)
		return NULL;
	state->lpVtbl = &visual_state_table;
	state->refs = 1;
	state->name = NULL;
	state->setters = create_children(&IID___FIVector_1_UIElement, vector_class_name, FALSE);
	if (!state->setters) {
		FREE(state);
		return NULL;
	}
	InterlockedIncrement(&raven_xaml_objects);
	return state;
}

/* The named collections the visual-state tree and the shape elements carry: VisualStateGroups on the grid-shaped
 * elements, States on a group, Setters on a state. The loader's collection pushes reach here before the panel and
 * grid faces; S_FALSE means the object has no such collection. */
HRESULT __declspec(dllexport) raven_xaml_elements_named_collection(IInspectable *object, const WCHAR *name,
	UINT32 length, IInspectable **out)
{
	if (!object || !name || !out)
		return E_POINTER;
	*out = NULL;
	if (same_text(name, length, L"VisualStateGroups")) {
		grid1 *grid = NULL;
		if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, (void **)&grid)))
			return S_FALSE;
		struct grid *self = NULL;
		if (grid->lpVtbl == &grid1_table)
			self = ((struct grid1_part *)grid)->owner;
		grid->lpVtbl->Release(grid);
		if (!self || !self->groups)
			return S_FALSE;
		InterlockedIncrement(&self->groups->refs);
		*out = (IInspectable *)self->groups;
		return S_OK;
	}
	{
		IInspectable *inspectable = NULL;
		if (FAILED(object->lpVtbl->QueryInterface(object, &IID_IInspectable, (void **)&inspectable)))
			return S_FALSE;
		if (inspectable->lpVtbl == &visual_state_group_table && same_text(name, length, L"States")) {
			struct visual_state_group *self = (struct visual_state_group *)inspectable;
			InterlockedIncrement(&self->states->refs);
			*out = (IInspectable *)self->states;
			inspectable->lpVtbl->Release(inspectable);
			return S_OK;
		}
		if (inspectable->lpVtbl == &visual_state_table && same_text(name, length, L"Setters")) {
			struct visual_state *self = (struct visual_state *)inspectable;
			InterlockedIncrement(&self->setters->refs);
			*out = (IInspectable *)self->setters;
			inspectable->lpVtbl->Release(inspectable);
			return S_OK;
		}
		inspectable->lpVtbl->Release(inspectable);
	}
	return S_FALSE;
}

/* Loader application: names, text and object properties on the three classes. */

static int elements_name(IInspectable *object, const WCHAR *text, UINT32 length)
{
	grid1 *grid = NULL;
	textblock1 *textblock = NULL;
	button1 *button = NULL;
	panel1 *panel = NULL;
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, (void **)&grid)) && grid) {
		if (grid->lpVtbl == &grid1_table)
			store_name(&((struct grid1_part *)grid)->owner->name, text, length);
		grid->lpVtbl->Release(grid);
		return 1;
	}
	/* The shape elements (Border, StackPanel, Image) ride the panel face: same owner machinery, their own class. */
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel, (void **)&panel)) && panel) {
		if (panel->lpVtbl == &panel_table)
			store_name(&((struct panel_part *)panel)->owner->name, text, length);
		panel->lpVtbl->Release(panel);
		return 1;
	}
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock, (void **)&textblock)) && textblock) {
		if (textblock->lpVtbl == &textblock_table)
			store_name(&((struct textblock *)textblock)->name, text, length);
		textblock->lpVtbl->Release(textblock);
		return 1;
	}
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton, (void **)&button)) && button) {
		if (button->lpVtbl == &button1_table)
			store_name(&((struct button1_part *)button)->owner->name, text, length);
		button->lpVtbl->Release(button);
		return 1;
	}
	return 0;
}

HRESULT raven_xaml_elements_set_name(IInspectable *object, const WCHAR *text, UINT32 length)
{
	if (!object)
		return E_POINTER;
	return elements_name(object, text, length) ? S_OK : S_FALSE;
}

/* The colour names the compiled-markup path accepts for Background until the media brushes and resource dictionaries arrive.
 * The spellings follow the XAML Colors class. */
static int named_color(const WCHAR *text, UINT32 length, COLORREF *color, BYTE *alpha)
{
	static const struct { const WCHAR *name; COLORREF color; } table[] = {
		{L"Black", RGB(0, 0, 0)}, {L"Gray", RGB(128, 128, 128)}, {L"White", RGB(255, 255, 255)},
		{L"Red", RGB(255, 0, 0)}, {L"Green", RGB(0, 128, 0)}, {L"Lime", RGB(0, 255, 0)},
		{L"Blue", RGB(0, 0, 255)}, {L"Yellow", RGB(255, 255, 0)}, {L"Cyan", RGB(0, 255, 255)},
		{L"Magenta", RGB(255, 0, 255)}, {L"Brown", RGB(165, 42, 42)}, {L"Orange", RGB(255, 165, 0)},
	};
	if (!text)
		return 0;
	for (size_t index = 0; index < ARRAYSIZE(table); ++index)
		if (same_text(text, length, table[index].name)) {
			*color = table[index].color;
			*alpha = 255;
			return 1;
		}
	if (same_text(text, length, L"Transparent")) {
		*color = 0;
		*alpha = 0;
		return 1;
	}
	return 0;
}

/* The colour store takes an owned brush reference the caller hands it (a colour NAME passes NULL, the way the compiled
 * colour constants always did), so a value that was a real brush keeps answering the getter. */
static void grid_store_color(struct grid *self, COLORREF color, BYTE alpha, IInspectable *brush)
{
	self->color = color;
	self->alpha = alpha;
	if (self->background)
		self->background->lpVtbl->Release(self->background);
	self->background = brush;
	raven_render_changed();
}

/* The Rectangle's outline store: the same shape as the fill store, into the stroke fields the rectangle carries.
 * Stored without rendering yet — the fields hold what the document gave until the shape-painting stage. */
static void grid_store_stroke(struct grid *self, COLORREF color, BYTE alpha, IInspectable *brush)
{
	self->stroke_color = color;
	self->stroke_alpha = alpha;
	if (self->stroke)
		self->stroke->lpVtbl->Release(self->stroke);
	self->stroke = brush;
	raven_render_changed();
}

/* The Fill application note: the engine's fill store is not a traced API call, so a launch that passes the former
 * Rectangle frontier carries its evidence only through this note — scoped to the one property, the way the frame
 * creation note above the factories is. */
static void rectangle_fill_note(COLORREF color, BYTE alpha)
{
	char note[64];
	wsprintfA(note, "raven-xaml: Rectangle fill stored %02x%02x%02x alpha %u\n",
		(unsigned)GetRValue(color), (unsigned)GetGValue(color), (unsigned)GetBValue(color), (unsigned)alpha);
	raven_trace_note(note);
}

/* The compiled colour-name path of the TextBlock foreground: the named colour replaces the brush reference, the way the
 * grid's Background constant does. */
static void textblock_store_color(struct textblock *self, COLORREF color, BYTE alpha, IInspectable *brush)
{
	self->foreground = color;
	self->foreground_alpha = alpha;
	if (self->foreground_brush)
		self->foreground_brush->lpVtbl->Release(self->foreground_brush);
	self->foreground_brush = brush;
	raven_render_changed();
}

/* The attached Grid.Row, Grid.Column, Grid.RowSpan and Grid.ColumnSpan of an element: stored on the element classes the engine
 * implements, read back by the layout pass, which lays out an element kind that does not store them at cell (0, 0) with a
 * one-track span. The store keeps the value as given; a stored span below one reads back as one track. */
enum grid_slot { grid_slot_row, grid_slot_column, grid_slot_row_span, grid_slot_column_span };

static HRESULT store_slot(IInspectable *object, enum grid_slot slot, UINT32 value)
{
	grid1 *grid = NULL;
	textblock1 *textblock = NULL;
	button1 *button = NULL;
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, (void **)&grid)) && grid) {
		if (grid->lpVtbl == &grid1_table) {
			struct grid *self = ((struct grid1_part *)grid)->owner;
			/* The four attached fields sit together in the class layout, in selector order. */
			UINT32 *fields[] = {&self->row, &self->column, &self->row_span, &self->column_span};
			*fields[slot] = value;
		}
		grid->lpVtbl->Release(grid);
		raven_render_changed();
		return S_OK;
	}
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock, (void **)&textblock)) && textblock) {
		if (textblock->lpVtbl == &textblock_table) {
			struct textblock *self = (struct textblock *)textblock;
			/* The four attached fields sit together in the class layout, in selector order. */
			UINT32 *fields[] = {&self->row, &self->column, &self->row_span, &self->column_span};
			*fields[slot] = value;
		}
		textblock->lpVtbl->Release(textblock);
		raven_render_changed();
		return S_OK;
	}
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton, (void **)&button)) && button) {
		if (button->lpVtbl == &button1_table) {
			struct button *self = ((struct button1_part *)button)->owner;
			/* The four attached fields sit together in the class layout, in selector order. */
			UINT32 *fields[] = {&self->row, &self->column, &self->row_span, &self->column_span};
			*fields[slot] = value;
		}
		button->lpVtbl->Release(button);
		raven_render_changed();
		return S_OK;
	}
	return S_FALSE;
}

/* The integer constants a forwarded Grid.Row, Grid.Column, Grid.RowSpan and Grid.ColumnSpan reach: the loader's integer
 * dispatch lives in xaml_controls.c, which forwards the names it does not take itself to the element layer, so the
 * compiled-markup constants arrive here (the fixture's constants arrive as float nodes through
 * raven_xaml_elements_set_float below). */
HRESULT raven_xaml_elements_set_integer(IInspectable *object, const WCHAR *name, UINT32 length, int64_t value)
{
	if (!object)
		return E_POINTER;
	if (same_text(name, length, L"Row"))
		return value < 0 || value > 0xffff ? E_INVALIDARG : store_slot(object, grid_slot_row, (UINT32)value);
	if (same_text(name, length, L"Column"))
		return value < 0 || value > 0xffff ? E_INVALIDARG : store_slot(object, grid_slot_column, (UINT32)value);
	if (same_text(name, length, L"RowSpan"))
		return value < 0 || value > 0xffff ? E_INVALIDARG : store_slot(object, grid_slot_row_span, (UINT32)value);
	if (same_text(name, length, L"ColumnSpan"))
		return value < 0 || value > 0xffff ? E_INVALIDARG : store_slot(object, grid_slot_column_span, (UINT32)value);
	return S_FALSE;
}

HRESULT raven_xaml_elements_set_float(IInspectable *object, const WCHAR *name, UINT32 length, float value)
{
	grid1 *grid = NULL;
	if (!object)
		return E_POINTER;
	if (same_text(name, length, L"Row") || same_text(name, length, L"Column") ||
		same_text(name, length, L"RowSpan") || same_text(name, length, L"ColumnSpan")) {
		if (value < 0.0f || value > 65535.0f)
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:2503\n"); return E_INVALIDARG; }
		return store_slot(object,
			same_text(name, length, L"Row") ? grid_slot_row :
			same_text(name, length, L"Column") ? grid_slot_column :
			same_text(name, length, L"RowSpan") ? grid_slot_row_span : grid_slot_column_span,
			(UINT32)value);
	}
	if (same_text(name, length, L"FontSize")) {
		/* The text size in DIPs; the real property rejects non-positive sizes and so does this. */
		textblock1 *textblock = NULL;
		if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
			(void **)&textblock)) || !textblock)
			return S_FALSE;
		if (textblock->lpVtbl == &textblock_table) {
			struct textblock *self = (struct textblock *)textblock;
			if (value <= 0.0f) {
				textblock->lpVtbl->Release(textblock);
				{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:2520\n"); return E_INVALIDARG; }
			}
			self->font_size = value;
			raven_render_changed();
		}
		textblock->lpVtbl->Release(textblock);
		return S_OK;
	}
	if (!same_text(name, length, L"Width") && !same_text(name, length, L"Height") &&
		!same_text(name, length, L"StrokeThickness") && !same_text(name, length, L"RadiusX") &&
		!same_text(name, length, L"RadiusY"))
		return S_FALSE;
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, (void **)&grid)) && grid) {
		if (grid->lpVtbl == &grid1_table) {
			struct grid *self = ((struct grid1_part *)grid)->owner;
			if (same_text(name, length, L"Width"))
				self->width = value;
			else if (same_text(name, length, L"Height"))
				self->height = value;
			else if (self->shape == grid_shape_rectangle && same_text(name, length, L"StrokeThickness"))
				self->stroke_thickness = value;
			else if (self->shape == grid_shape_rectangle && same_text(name, length, L"RadiusX"))
				self->radius_x = value;
			else if (self->shape == grid_shape_rectangle && same_text(name, length, L"RadiusY"))
				self->radius_y = value;
			raven_render_changed();
		}
		grid->lpVtbl->Release(grid);
		return S_OK;
	}
	/* The button carries its own size too: the fixture positions it with Width and Height, and the render pass reads both back
	 * for the rectangle the hit test works from. */
	button1 *button = NULL;
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton, (void **)&button)) && button) {
		if (button->lpVtbl == &button1_table) {
			struct button *self = ((struct button1_part *)button)->owner;
			if (same_text(name, length, L"Width"))
				self->width = value;
			else
				self->height = value;
			raven_render_changed();
		}
		button->lpVtbl->Release(button);
		return S_OK;
	}
	return S_FALSE;
}

HRESULT raven_xaml_elements_set_text(IInspectable *object, const WCHAR *name, UINT32 length, const WCHAR *text, UINT32 text_length)
{
	grid1 *grid = NULL;
	textblock1 *textblock = NULL;
	HRESULT result;
	if (!object)
		return E_POINTER;
	if (same_text(name, length, L"Background")) {
		COLORREF color = 0;
		BYTE alpha = 0;
		const int named = named_color(text, text_length, &color, &alpha);
		if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, (void **)&grid)) || !grid)
			return S_FALSE;
		if (grid->lpVtbl == &grid1_table && named)
			grid_store_color(((struct grid1_part *)grid)->owner, color, alpha, NULL);
		grid->lpVtbl->Release(grid);
		return S_OK;
	}
	if (same_text(name, length, L"Foreground")) {
		COLORREF color = 0;
		BYTE alpha = 0;
		textblock1 *textblock = NULL;
		const int named = named_color(text, text_length, &color, &alpha);
		if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
			(void **)&textblock)) || !textblock)
			return S_FALSE;
		if (textblock->lpVtbl == &textblock_table && named)
			textblock_store_color((struct textblock *)textblock, color, alpha, NULL);
		textblock->lpVtbl->Release(textblock);
		return S_OK;
	}
	if (!same_text(name, length, L"Text"))
		return S_FALSE;
	if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock, (void **)&textblock)) || !textblock)
		return S_FALSE;
	HSTRING value = NULL;
	result = WindowsCreateString(text, text_length, &value);
	if (SUCCEEDED(result)) {
		result = textblock->lpVtbl->put_Text(textblock, (uint64_t)(uintptr_t)value);
		WindowsDeleteString(value);
	}
	textblock->lpVtbl->Release(textblock);
	return result;
}

/* The "Template" object property (the loader path) routes to the fixture-facing export defined later in this file: the
 * same store put_Template serves, so a compiled document that declares a ControlTemplate under a button reaches the
 * engine state the style route and the direct put share. An element without control storage answers S_FALSE through
 * the export's own dispatch — the skip convention every property the engine does not store follows. */
HRESULT raven_xaml_element_put_template(IInspectable *object, IInspectable *template_object);

/* The Resources slot of the element classes that carry one, looked up the way the Style slots are: the grid, the text block
 * and the button. The QI reference is released once the slot address is known, which the caller's own reference keeps
 * valid. A class outside the three answers null, which the caller reads as the skip convention. */
static IInspectable **element_resources_slot(IInspectable *object)
{
	IInspectable **slot = NULL;
	grid1 *grid = NULL;
	textblock1 *textblock = NULL;
	button1 *button = NULL;
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, (void **)&grid)) && grid) {
		if (grid->lpVtbl == &grid1_table)
			slot = &((struct grid1_part *)grid)->owner->resources;
		grid->lpVtbl->Release(grid);
	} else if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
		(void **)&textblock)) && textblock) {
		if (textblock->lpVtbl == &textblock_table)
			slot = &((struct textblock *)textblock)->resources;
		textblock->lpVtbl->Release(textblock);
	} else if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton,
		(void **)&button)) && button) {
		if (button->lpVtbl == &button1_table)
			slot = &((struct button1_part *)button)->owner->resources;
		button->lpVtbl->Release(button);
	}
	return slot;
}

/* FrameworkElement.Resources: the dictionary a compiled page fills with SetValue before adding its keys, and reads back
 * through the property. The element keeps an owned reference; a second assignment replaces the first. Resource lookup does
 * not consult the dictionary yet, so nothing consumes the stored reference. */
HRESULT __declspec(dllexport) raven_xaml_element_put_resources(IInspectable *object, IInspectable *dictionary)
{
	IInspectable **slot;
	if (!object)
		return E_POINTER;
	slot = element_resources_slot(object);
	if (!slot)
		return S_FALSE;
	if (dictionary)
		IInspectable_AddRef(dictionary);
	if (*slot)
		IInspectable_Release(*slot);
	*slot = dictionary;
	return S_OK;
}

/* The dictionary the element holds, borrowed and not created: resource lookup walks the scope stack and must not turn
 * every element it passes into one that owns an empty dictionary. */
IInspectable *raven_xaml_element_resources_existing(IInspectable *object)
{
	IInspectable **slot;
	if (!object)
		return NULL;
	slot = element_resources_slot(object);
	return slot ? *slot : NULL;
}

HRESULT __declspec(dllexport) raven_xaml_element_get_resources(IInspectable *object, IInspectable **out)
{
	IInspectable **slot;
	if (!object || !out)
		return E_POINTER;
	*out = NULL;
	slot = element_resources_slot(object);
	if (!slot)
		return S_FALSE;
	if (!*slot) {
		/* Created on first read, so an element that never uses Resources carries no dictionary. */
		IInspectable *created = NULL;
		const HRESULT made = raven_xaml_create_stable(371, &created);
		if (FAILED(made) || !created)
			return FAILED(made) ? made : E_OUTOFMEMORY;
		*slot = created;
	}
	IInspectable_AddRef(*slot);
	*out = *slot;
	return S_OK;
}

HRESULT raven_xaml_elements_set_object(IInspectable *object, const WCHAR *name, UINT32 length, IInspectable *value)
{
	panel1 *panel = NULL;
	content_control *content = NULL;
	HRESULT result;
	if (!object)
		return E_POINTER;
	if (same_text(name, length, L"Children")) {
		void *children = NULL;
		if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel, (void **)&panel)) || !panel)
			return S_FALSE;
		result = panel->lpVtbl->get_Children(panel, (uint64_t)(uintptr_t)&children);
		panel->lpVtbl->Release(panel);
		if (FAILED(result) || !children)
			return FAILED(result) ? result : S_FALSE;
		/* The vector is whatever IVector<UIElement> the panel handed out; the vtable layout is the parameterized ABI, so
		 * Append reaches the panel's own collection. */
		result = ((struct children_vector *)children)->lpVtbl->Append((struct children_vector *)children, (ui_element *)value);
		((IInspectable *)children)->lpVtbl->Release((IInspectable *)children);
		if (SUCCEEDED(result))
			raven_render_changed();
		return result;
	}
	if (same_text(name, length, L"RowDefinitions") || same_text(name, length, L"ColumnDefinitions")) {
		grid1 *grid = NULL;
		if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, (void **)&grid)) || !grid)
			return S_FALSE;
		if (grid->lpVtbl != &grid1_table) {
			grid->lpVtbl->Release(grid);
			return S_FALSE;
		}
		struct grid *self = ((struct grid1_part *)grid)->owner;
		struct children_vector *vector = same_text(name, length, L"RowDefinitions") ? self->rows : self->columns;
		grid->lpVtbl->Release(grid);
		result = vector->lpVtbl->Append(vector, (ui_element *)value);
		if (SUCCEEDED(result))
			raven_render_changed();
		return result;
	}
	if (same_text(name, length, L"Content")) {
		if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl, (void **)&content)) || !content)
			return S_FALSE;
		result = content->lpVtbl->put_Content(content, (uint64_t)(uintptr_t)value);
		content->lpVtbl->Release(content);
		return result;
	}
	if (same_text(name, length, L"Template")) {
		/* The value must be an engine ControlTemplate and the target must carry control storage; put_Template answers
		 * E_INVALIDARG for the wrong kind and S_FALSE for an element without it, which the caller reads as the skip. */
		return raven_xaml_element_put_template(object, value);
	}
	if (same_text(name, length, L"ContentTemplate")) {
		/* The presenter's own content template (stable 525): an engine ControlTemplate stored on the presenter, S_FALSE
		 * for any other element — the skip convention. */
		return presenter_store_content_template(object, value);
	}
	if (same_text(name, length, L"Resources"))
		return raven_xaml_element_put_resources(object, value);
	return S_FALSE;
}

/* The per-element Style property and the application of a style's setters.
 *
 * put_Style is the application point: storing the style runs its Setters against the element's OWN storage — the BasedOn
 * chain's setters first, the style's own after (raven_style_apply in xaml_styles.c), so a property the derived style sets
 * lands after what the base applied and wins, the real XAML precedence. Each setter goes through the same stores the
 * loader and the direct property puts use, so the pixel follows a styled value the way it follows a direct put: a colour
 * through the named-colour path the compiled Background uses (the value an IInspectable HSTRING of the colour name) or a
 * composition colour brush, a number into the font size or the extent. The application is validated before it mutates: a
 * setter whose property the element stores but whose value is of the wrong kind fails the whole put with E_INVALIDARG —
 * chosen over S_FALSE because S_FALSE reads as "succeeded with no effect" and would hide the bad setter — and the
 * element keeps its previous state. A property the element has no storage for is skipped: without DependencyProperty the
 * engine cannot tell a name it does not store from one it will, so only a wrong KIND on a stored property fails. A NULL
 * put clears the reference and applies nothing: the engine RE-APPLIES on put and does not revert, so a property an old
 * style set and the new one does not stays where the old one left it — the honest approximation until the engine tracks
 * which values a style application owns. A setter appended to an already-applied style does not re-apply; put_Style is
 * the application point. */

/* The BasedOn-chain application xaml_styles.c provides for put_Style: it walks the style and its bases (base setters
 * first, a depth cap guarding a cycle) and hands every stored setter pair to the callback. Declared here rather than in
 * xaml_styles.h because the shared style header carries only the surface the previous stage needed. */
HRESULT raven_style_apply(IInspectable *style,
	HRESULT (*apply)(void *context, const WCHAR *name, UINT32 length, IInspectable *value, int check_only),
	void *context, int check_only);

/* A boxed string value: IReference`1<HSTRING>, what a WinRT caller passes for a string setter value. */
static int style_boxed_text(IInspectable *value, HSTRING *out)
{
	__FIReference_1_HSTRING *box = NULL;
	HSTRING string = NULL;
	if (FAILED(value->lpVtbl->QueryInterface(value, &IID___FIReference_1_HSTRING, (void **)&box)) || !box)
		return 0;
	const int boxed = box->lpVtbl->get_Value(box, &string) == S_OK && string;
	box->lpVtbl->Release(box);
	*out = string;
	return boxed;
}

/* A boxed number value: IReference`1<double>, the boxing a WinRT caller uses for the numeric properties. */
static int style_boxed_number(IInspectable *value, float *number)
{
	__FIReference_1_DOUBLE *box = NULL;
	double read = 0;
	if (FAILED(value->lpVtbl->QueryInterface(value, &IID___FIReference_1_DOUBLE, (void **)&box)) || !box)
		return 0;
	const int boxed = box->lpVtbl->get_Value(box, &read) == S_OK;
	box->lpVtbl->Release(box);
	*number = (float)read;
	return boxed;
}

/* A brush value: the composition colour brush is the brush the engine can read a colour from, the way Panel.Background
 * treats one; the engine SolidColorBrush the theme dictionaries build reads through the same reader, so a dictionary
 * brush lands where a composition colour brush would. */
static int style_brush_color(IInspectable *value, COLORREF *color, BYTE *alpha)
{
	__x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush *brush = NULL;
	if (SUCCEEDED(value->lpVtbl->QueryInterface(value, &IID___x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush,
		(void **)&brush)) && brush) {
		__x_ABI_CWindows_CUI_CColor read;
		int matched = 0;
		if (brush->lpVtbl->get_Color(brush, &read) == S_OK) {
			*color = RGB(read.R, read.G, read.B);
			*alpha = read.A;
			matched = 1;
		}
		brush->lpVtbl->Release(brush);
		return matched;
	}
	{
		__x_ABI_CWindows_CUI_CColor read;
		if (raven_solid_brush_get_color(value, &read) != S_OK)
			return 0;
		*color = RGB(read.R, read.G, read.B);
		*alpha = read.A;
	}
	return 1;
}

/* The application of one setter value to the grid's own storage. check_only validates the value kind without mutating:
 * put_Style runs the whole chain through it before it stores anything, so a wrong-kind value fails the application with
 * E_INVALIDARG and leaves the element exactly as it was. */
static HRESULT grid_apply_setter(void *state, const WCHAR *name, UINT32 length, IInspectable *value, int check_only)
{
	struct grid *self = state;
	if (same_text(name, length, L"Background")) {
		COLORREF color = 0;
		BYTE alpha = 0;
		if (value && style_brush_color(value, &color, &alpha)) {
			if (!check_only) {
				value->lpVtbl->AddRef(value);
				grid_store_color(self, color, alpha, value);
			}
			return S_OK;
		}
		HSTRING string = NULL;
		if (value && style_boxed_text(value, &string)) {
			UINT32 text_length = 0;
			const WCHAR *text = WindowsGetStringRawBuffer(string, &text_length);
			const int named = named_color(text, text_length, &color, &alpha);
			WindowsDeleteString(string);
			if (named) {
				if (!check_only)
					grid_store_color(self, color, alpha, NULL);
				return S_OK;
			}
		}
		/* A value that is neither a colour brush nor a colour name — a number where a colour is wanted, or a spelling
		 * the engine does not know — fails the application; put_Style answers E_INVALIDARG and stores nothing. */
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:2859\n"); return E_INVALIDARG; }
	}
	/* The Rectangle's own shape properties. The fill rides the brush slot the Background store keeps (a rectangle has
	 * no Background); a colour brush lands there the way a Background does, and so does the boxed colour a compiled
	 * theme dictionary hands a ThemeResource lookup and the named-colour constant. A value the engine cannot read
	 * fails the application the way the Background setter does — the trace then names the runtime class the resource
	 * resolved to. Stroke stores beside it; the thickness and the corner radii are numbers. The outline and radii are
	 * stored without rendering yet, the way the Border stores what it cannot paint. */
	if (self->shape == grid_shape_rectangle &&
		(same_text(name, length, L"Fill") || same_text(name, length, L"Stroke"))) {
		const int fill = same_text(name, length, L"Fill");
		COLORREF color = 0;
		BYTE alpha = 0;
		if (value && style_brush_color(value, &color, &alpha)) {
			if (!check_only) {
				value->lpVtbl->AddRef(value);
				if (fill) {
					grid_store_color(self, color, alpha, value);
					rectangle_fill_note(color, alpha);
				} else
					grid_store_stroke(self, color, alpha, value);
			}
			return S_OK;
		}
		__x_ABI_CWindows_CUI_CColor boxed;
		if (value && raven_color_value_of(value, &boxed) == S_OK) {
			if (!check_only) {
				color = RGB(boxed.R, boxed.G, boxed.B);
				alpha = boxed.A;
				if (fill) {
					grid_store_color(self, color, alpha, NULL);
					rectangle_fill_note(color, alpha);
				} else
					grid_store_stroke(self, color, alpha, NULL);
			}
			return S_OK;
		}
		HSTRING string = NULL;
		if (value && style_boxed_text(value, &string)) {
			UINT32 text_length = 0;
			const WCHAR *text = WindowsGetStringRawBuffer(string, &text_length);
			const int named = named_color(text, text_length, &color, &alpha);
			WindowsDeleteString(string);
			if (named) {
				if (!check_only) {
					if (fill) {
						grid_store_color(self, color, alpha, NULL);
						rectangle_fill_note(color, alpha);
					} else
						grid_store_stroke(self, color, alpha, NULL);
				}
				return S_OK;
			}
		}
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:3212\n"); return E_INVALIDARG; }
	}
	if (self->shape == grid_shape_rectangle &&
		(same_text(name, length, L"StrokeThickness") || same_text(name, length, L"RadiusX") ||
			same_text(name, length, L"RadiusY"))) {
		float number = 0;
		if (!value || !style_boxed_number(value, &number))
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:3222\n"); return E_INVALIDARG; }
		if (!check_only) {
			if (same_text(name, length, L"StrokeThickness"))
				self->stroke_thickness = number;
			else if (same_text(name, length, L"RadiusX"))
				self->radius_x = number;
			else
				self->radius_y = number;
			raven_render_changed();
		}
		return S_OK;
	}
	if (same_text(name, length, L"Width") || same_text(name, length, L"Height")) {
		float number = 0;
		if (!value || !style_boxed_number(value, &number))
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:2864\n"); return E_INVALIDARG; }
		if (!check_only) {
			if (same_text(name, length, L"Width"))
				self->width = number;
			else
				self->height = number;
			raven_render_changed();
		}
		return S_OK;
	}
	/* A property the grid does not store is skipped: without DependencyProperty the engine cannot tell a name it has no
	 * storage for from one it will, so only a wrong kind on a stored property fails. */
	return S_OK;
}

/* The same application for a TextBlock: the Foreground colour, the font size (the real property rejects a non-positive
 * size and so does the application) and the text. */
static HRESULT textblock_apply_setter(void *state, const WCHAR *name, UINT32 length, IInspectable *value, int check_only)
{
	struct textblock *self = state;
	if (same_text(name, length, L"Foreground")) {
		COLORREF color = 0;
		BYTE alpha = 0;
		if (value && style_brush_color(value, &color, &alpha)) {
			if (!check_only) {
				value->lpVtbl->AddRef(value);
				textblock_store_color(self, color, alpha, value);
			}
			return S_OK;
		}
		HSTRING string = NULL;
		if (value && style_boxed_text(value, &string)) {
			UINT32 text_length = 0;
			const WCHAR *text = WindowsGetStringRawBuffer(string, &text_length);
			const int named = named_color(text, text_length, &color, &alpha);
			WindowsDeleteString(string);
			if (named) {
				if (!check_only)
					textblock_store_color(self, color, alpha, NULL);
				return S_OK;
			}
		}
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:2906\n"); return E_INVALIDARG; }
	}
	if (same_text(name, length, L"FontSize")) {
		float number = 0;
		if (!value || !style_boxed_number(value, &number))
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:2911\n"); return E_INVALIDARG; }
		if (number <= 0.0f)
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:2913\n"); return E_INVALIDARG; }
		if (!check_only) {
			self->font_size = number;
			raven_render_changed();
		}
		return S_OK;
	}
	if (same_text(name, length, L"Text")) {
		HSTRING string = NULL;
		if (!value || !style_boxed_text(value, &string))
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:2923\n"); return E_INVALIDARG; }
		if (!check_only) {
			UINT32 text_length = 0;
			const WCHAR *text = WindowsGetStringRawBuffer(string, &text_length);
			store_name(&self->text, text, text_length);
			raven_render_changed();
		}
		WindowsDeleteString(string);
		return S_OK;
	}
	return S_OK;
}

/* The same application for a Button: the extent (the render pass reads both fields for the rectangle the hit test works
 * from) and the Background colour, which the control chrome does not render yet — the fields store it until that stage,
 * the way the style reference itself was stored before this one. */
static HRESULT button_apply_setter(void *state, const WCHAR *name, UINT32 length, IInspectable *value, int check_only)
{
	struct button *self = state;
	if (same_text(name, length, L"Background")) {
		COLORREF color = 0;
		BYTE alpha = 0;
		if (value && style_brush_color(value, &color, &alpha)) {
			if (!check_only) {
				self->color = color;
				self->alpha = alpha;
			}
			return S_OK;
		}
		HSTRING string = NULL;
		if (value && style_boxed_text(value, &string)) {
			UINT32 text_length = 0;
			const WCHAR *text = WindowsGetStringRawBuffer(string, &text_length);
			const int named = named_color(text, text_length, &color, &alpha);
			WindowsDeleteString(string);
			if (named) {
				if (!check_only) {
					self->color = color;
					self->alpha = alpha;
				}
				return S_OK;
			}
		}
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:2966\n"); return E_INVALIDARG; }
	}
	if (same_text(name, length, L"Width") || same_text(name, length, L"Height")) {
		float number = 0;
		if (!value || !style_boxed_number(value, &number))
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:2971\n"); return E_INVALIDARG; }
		if (!check_only) {
			if (same_text(name, length, L"Width"))
				self->width = number;
			else
				self->height = number;
			raven_render_changed();
		}
		return S_OK;
	}
	if (same_text(name, length, L"Template")) {
		/* The value must be an engine ControlTemplate: the wrong kind — a string box, a number, a Style — fails the
		 * application with E_INVALIDARG the way the colour setters do, and the check-only pass leaves the previous
		 * template untouched. A NULL value clears. */
		IInspectable *root = NULL;
		if (value && raven_xaml_control_template_root(value, &root) != S_OK)
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:2987\n"); return E_INVALIDARG; }
		if (root)
			root->lpVtbl->Release(root);
		if (!check_only)
			button_store_template(self, value);
		return S_OK;
	}
	return S_OK;
}

/* The adapter raven_style_apply calls: the context carries the element's state, the per-kind setter application and the
 * slot the owned style reference is stored in (the same field in every element class this handles). */
struct style_target {
	IInspectable **slot;
	void *state;
	HRESULT (*apply)(void *state, const WCHAR *name, UINT32 length, IInspectable *value, int check_only);
};

static HRESULT style_apply_dispatch(void *context, const WCHAR *name, UINT32 length, IInspectable *value, int check_only)
{
	const struct style_target *target = context;
	return target->apply(target->state, name, length, value, check_only);
}

/* The element's setter application: which class it is, its state, and the slot its owned style reference lives in. Both the
 * Style put and a resource value resolved by the loader go through it. */
static int element_style_target(IInspectable *object, struct style_target *target)
{
	grid1 *grid = NULL;
	textblock1 *textblock = NULL;
	button1 *button = NULL;
	if (!object || !target)
		return 0;
	target->slot = NULL;
	target->state = NULL;
	target->apply = NULL;
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, (void **)&grid)) && grid) {
		if (grid->lpVtbl != &grid1_table) {
			grid->lpVtbl->Release(grid);
			return 0;
		}
		struct grid *self = ((struct grid1_part *)grid)->owner;
		target->slot = &self->style;
		target->state = self;
		target->apply = grid_apply_setter;
		grid->lpVtbl->Release(grid);
		return 1;
	}
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
		(void **)&textblock)) && textblock) {
		if (textblock->lpVtbl != &textblock_table) {
			textblock->lpVtbl->Release(textblock);
			return 0;
		}
		struct textblock *self = (struct textblock *)textblock;
		target->slot = &self->style;
		target->state = self;
		target->apply = textblock_apply_setter;
		textblock->lpVtbl->Release(textblock);
		return 1;
	}
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton,
		(void **)&button)) && button) {
		if (button->lpVtbl != &button1_table) {
			button->lpVtbl->Release(button);
			return 0;
		}
		struct button *self = ((struct button1_part *)button)->owner;
		target->slot = &self->style;
		target->state = self;
		target->apply = button_apply_setter;
		button->lpVtbl->Release(button);
		return 1;
	}
	return 0;
}

/* A value the loader resolved from a resource dictionary: object properties first (Style, Resources, Template, Content),
 * then the per-kind setter stores, so a resource holding a brush, a colour name or a number lands where a style setter
 * would put it. S_FALSE is the skip convention for a name and element the engine stores nowhere. */
HRESULT __declspec(dllexport) raven_xaml_element_apply_value(IInspectable *object, const WCHAR *name, UINT32 length,
	IInspectable *value)
{
	struct style_target target;
	HRESULT result;
	INT32 enum_value;
	__x_ABI_CWindows_CUI_CColor color;
	if (!object)
		return E_POINTER;
	if (raven_enum_value_of(value, &enum_value)) {
		result = raven_xaml_element_set_integer(object, name, length, enum_value);
		if (result != S_FALSE)
			return result;
	}
	if (raven_color_value_of(value, &color) == S_OK) {
		result = raven_acrylic_brush_set_color(object, name, length, &color);
		if (result != S_FALSE)
			return result;
	}
	if (((length == 21 && CompareStringOrdinal(name, (int)length, L"SolidColorBrush_Color", -1, FALSE) == CSTR_EQUAL) ||
		(length == 5 && CompareStringOrdinal(name, (int)length, L"Color", -1, FALSE) == CSTR_EQUAL)) &&
		raven_color_value_of(value, &color) == S_OK) {
		result = raven_solid_brush_put_color(object, &color);
		if (result != S_FALSE)
			return result;
	}
	/* A Style's own object properties: the compiled App.xaml's styles take their BasedOn through a resource reference
	 * (SetValueFromStaticResource Style_BasedOn), which lands here with the resolved base style. */
	if (length == 7 && CompareStringOrdinal(name, 7, L"BasedOn", -1, FALSE) == CSTR_EQUAL) {
		__x_ABI_CWindows_CUI_CXaml_CIStyle *style = NULL, *base = NULL;
		{
			char note[96];
			wsprintfA(note, "raven-xaml: BasedOn apply on object %p value %p\n", (void *)object, (void *)value);
			raven_trace_note(note);
		}
		if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CIStyle, (void **)&style)) || !style)
			return S_FALSE;
		if (FAILED(value->lpVtbl->QueryInterface(value, &IID__x_ABI_CWindows_CUI_CXaml_CIStyle, (void **)&base)) || !base) {
			style->lpVtbl->Release(style);
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_elements.c:3101\n"); return E_INVALIDARG; }
		}
		base->lpVtbl->Release(base);
		result = style->lpVtbl->put_BasedOn(style, (uint64_t)(uintptr_t)value);
		style->lpVtbl->Release(style);
		return result;
	}
	result = raven_xaml_elements_set_object(object, name, length, value);
	if (result != S_FALSE || !element_style_target(object, &target))
		return result;
	return target.apply(target.state, name, length, value, FALSE);
}

/* The fixture-facing exports of the style property; the component exports only what it marks, so the style fixture reaches
 * these through GetProcAddress the way it reaches RavenXamlLoad. */
__declspec(dllexport) HRESULT raven_xaml_element_put_style(IInspectable *object, IInspectable *style)
{
	struct style_target target;
	HRESULT result;
	if (!object)
		return E_POINTER;
	if (!element_style_target(object, &target))
		return S_FALSE;
	/* Validate the whole chain before anything lands: a wrong-kind setter fails the put with E_INVALIDARG and the element
	 * keeps its previous state, including the style it already had. */
	if (style) {
		result = raven_style_apply(style, style_apply_dispatch, &target, TRUE);
		if (FAILED(result))
			return result;
	}
	/* Store the owned reference; a NULL put clears it. The engine re-applies on put and does not revert: a property an
	 * old style set and the new one does not stays where the old one left it. */
	if (style)
		style->lpVtbl->AddRef(style);
	if (*target.slot)
		(*target.slot)->lpVtbl->Release(*target.slot);
	*target.slot = style;
	/* Apply for real: the base setters first, the style's own after. Each store re-renders the way the direct put does;
	 * the final changed call covers the NULL put and a style with no setter the element stores. */
	if (style)
		raven_style_apply(style, style_apply_dispatch, &target, FALSE);
	raven_render_changed();
	return S_OK;
}

__declspec(dllexport) HRESULT raven_xaml_element_get_style(IInspectable *object, IInspectable **out)
{
	grid1 *grid = NULL;
	textblock1 *textblock = NULL;
	button1 *button = NULL;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!object)
		return S_FALSE;
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, (void **)&grid)) && grid) {
		if (grid->lpVtbl != &grid1_table) {
			grid->lpVtbl->Release(grid);
			return S_FALSE;
		}
		struct grid *self = ((struct grid1_part *)grid)->owner;
		*out = self->style;
		grid->lpVtbl->Release(grid);
	} else if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
		(void **)&textblock)) && textblock) {
		if (textblock->lpVtbl != &textblock_table) {
			textblock->lpVtbl->Release(textblock);
			return S_FALSE;
		}
		*out = ((struct textblock *)textblock)->style;
		textblock->lpVtbl->Release(textblock);
	} else if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton, (void **)&button)) && button) {
		if (button->lpVtbl != &button1_table) {
			button->lpVtbl->Release(button);
			return S_FALSE;
		}
		*out = ((struct button1_part *)button)->owner->style;
		button->lpVtbl->Release(button);
	} else {
		return S_FALSE;
	}
	if (*out)
		(*out)->lpVtbl->AddRef(*out);
	return S_OK;
}

/* The fixture-facing exports of the Template property (the button the only element class with control storage so far):
 * the same put the IControl part serves, so a fixture can drive clear/replace through GetProcAddress without declaring
 * the 41-slot IControl vtable. Other classes answer S_FALSE. */
__declspec(dllexport) HRESULT raven_xaml_element_put_template(IInspectable *object, IInspectable *template_object)
{
	button1 *button = NULL;
	if (!object)
		return E_POINTER;
	if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton,
		(void **)&button)) || !button)
		return S_FALSE;
	if (button->lpVtbl != &button1_table) {
		button->lpVtbl->Release(button);
		return S_FALSE;
	}
	const HRESULT result = button_put_template(((struct button1_part *)button)->owner, template_object);
	button->lpVtbl->Release(button);
	return result;
}

__declspec(dllexport) HRESULT raven_xaml_element_get_template(IInspectable *object, IInspectable **out)
{
	button1 *button = NULL;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!object)
		return S_FALSE;
	if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton,
		(void **)&button)) || !button)
		return S_FALSE;
	if (button->lpVtbl != &button1_table) {
		button->lpVtbl->Release(button);
		return S_FALSE;
	}
	struct button *self = ((struct button1_part *)button)->owner;
	if (self->template_value) {
		*out = self->template_value;
		(*out)->lpVtbl->AddRef(*out);
	}
	button->lpVtbl->Release(button);
	return S_OK;
}

/* The templated control's single visual child, what the render walk's raven_xaml_frame_render_child hook consults first:
 * the root of the applied ControlTemplate, so a templated control renders its template tree in place of its content slot
 * and the walk needs no template-aware step of its own. An engine ContentPresenter answers the templated parent's
 * Content here — the same hook the walk reads, so a presenter inside a template shows the control's logical content in
 * both the measure and the paint without a render-path change. A presenter with a stored ContentTemplate applies it on
 * this same hook: the answer becomes the content template's root and the content template's own presenters bind to the
 * resolved Content object (not the templated control) through the walk element_store_template runs — the content is
 * unknown at store time and can change, so the bind is lazy and re-runs per resolution, idempotent for unchanged
 * content. S_FALSE for an object that is neither an engine button nor an engine presenter; S_OK with a NULL out for a
 * button without a template, a presenter with no binding, or a content template whose content did not resolve. */
HRESULT __declspec(dllexport) raven_xaml_element_template_child(IInspectable *object, IInspectable **out)
{
	button1 *button = NULL;
	IInspectable *root = NULL;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!object)
		return S_FALSE;
	{
		struct content_presenter *presenter = presenter_from_object(object);
		if (presenter) {
			/* The non-owning back-pointer: NULL after a clear, so a retained presenter answers no content rather than
			 * a dangling one. */
			if (!presenter->templated_parent)
				return S_OK;
			__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *content = NULL;
			if (FAILED(presenter->templated_parent->lpVtbl->QueryInterface(presenter->templated_parent,
				&IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl, (void **)&content)) || !content)
				return S_OK;
			if (presenter->content_template) {
				/* The content template: the Content must resolve to flow through the template — no content renders
				 * nothing, the real ContentTemplate answer. The content object the walk binds to must itself carry
				 * the content the inner presenters resolve: an element that is no ContentControl (a TextBlock, a
				 * Grid) or a non-element value (a string) leaves the inner presenters unbound — the walk stops at
				 * it — and they answer nothing rather than something invented. */
				IInspectable *content_value = NULL, *template_root = NULL;
				content->lpVtbl->get_Content(content, (uint64_t)(uintptr_t)&content_value);
				content->lpVtbl->Release(content);
				if (!content_value)
					return S_OK;
				if (raven_xaml_control_template_root(presenter->content_template, &template_root) == S_OK &&
					template_root) {
					presenter_bind_walk(template_root, content_value, 0, 0);
					*out = template_root;
				}
				content_value->lpVtbl->Release(content_value);
				return S_OK;
			}
			/* The plain presenter: the get_Content AddRef is the answer's own reference. */
			content->lpVtbl->get_Content(content, (uint64_t)(uintptr_t)out);
			content->lpVtbl->Release(content);
			return S_OK;
		}
	}
	if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton,
		(void **)&button)) || !button)
		return S_FALSE;
	if (button->lpVtbl != &button1_table) {
		button->lpVtbl->Release(button);
		return S_FALSE;
	}
	struct button *self = ((struct button1_part *)button)->owner;
	if (self->template_value &&
		raven_xaml_control_template_root(self->template_value, &root) == S_OK && root)
		*out = root;
	button->lpVtbl->Release(button);
	return S_OK;
}

/* The information the rendering pass reads back from an element: what it is, the colour it fills, the size it was given and
 * how many visual children it has. */

HRESULT raven_xaml_element_render_info(IInspectable *object, enum element_render_kind *kind, COLORREF *color, BYTE *alpha,
	float *width, float *height, UINT32 *child_count)
{
	grid1 *grid = NULL;
	if (!kind || !color || !alpha || !width || !height || !child_count)
		return E_POINTER;
	*kind = element_render_none;
	*color = 0;
	*alpha = 0;
	*width = 0;
	*height = 0;
	*child_count = 0;
	if (!object)
		return S_OK;
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, (void **)&grid)) && grid) {
		if (grid->lpVtbl == &grid1_table) {
			struct grid *self = ((struct grid1_part *)grid)->owner;
			*kind = element_render_grid;
			*color = self->color;
			*alpha = self->alpha;
			*width = self->width;
			*height = self->height;
			*child_count = self->children ? self->children->count : 0;
			/* A stored style contributed at put time: its setters landed in the fields above, so the pass reads the
			 * own state untouched, the way it reads every directly-set value. */
		}
		grid->lpVtbl->Release(grid);
		return S_OK;
	}
	/* A button paints no chrome yet, but its size drives the rectangle the layout gives it and the hit test works from. */
	button1 *button = NULL;
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton, (void **)&button)) && button) {
		if (button->lpVtbl == &button1_table) {
			struct button *self = ((struct button1_part *)button)->owner;
			*width = self->width;
			*height = self->height;
		}
		button->lpVtbl->Release(button);
	}
	return S_OK;
}

/* The text payload the rendering pass draws for a TextBlock: the text, the font size in DIPs and the foreground colour.
 * S_FALSE for every element that is not a TextBlock; a TextBlock without text reports a zero length and paints nothing.
 * The text stays owned by the element — the caller copies it synchronously, while the reference here is alive. */
HRESULT raven_xaml_element_render_text(IInspectable *object, const WCHAR **text, UINT32 *length, float *font_size,
	COLORREF *color, BYTE *alpha)
{
	textblock1 *textblock = NULL;
	if (!text || !length || !font_size || !color || !alpha)
		return E_POINTER;
	*text = NULL;
	*length = 0;
	*font_size = 0;
	*color = 0;
	*alpha = 0;
	if (!object)
		return S_FALSE;
	if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
		(void **)&textblock)) || !textblock)
		return S_FALSE;
	HRESULT result = S_FALSE;
	if (textblock->lpVtbl == &textblock_table) {
		struct textblock *self = (struct textblock *)textblock;
		if (self->text)
			*text = WindowsGetStringRawBuffer(self->text, length);
		if (!*text)
			*length = 0;
		*font_size = self->font_size;
		*color = self->foreground;
		*alpha = self->foreground_alpha;
		/* A stored style contributed at put time: its setters landed in the fields above, so the pass reads the own
		 * state untouched. */
		result = S_OK;
	}
	textblock->lpVtbl->Release(textblock);
	return result;
}

/* The automation name the peers report for an element, the xaml_automation.h precedence: the stored Name first, a TextBlock
 * without one falls back to its Text, everything else to the runtime class name. An element of one of the classes here reads
 * its own storage and falls back to the class constant; any other IInspectable falls back to the runtime class name its
 * GetRuntimeClassName reports, so a peer of an element the engine grows later still answers a name. */
HRESULT raven_xaml_element_automation_name(IInspectable *element, HSTRING *out)
{
	textblock1 *textblock = NULL;
	grid1 *grid = NULL;
	button1 *button = NULL;
	HSTRING source = NULL;
	const WCHAR *fallback = NULL;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!element)
		return E_POINTER;
	if (SUCCEEDED(element->lpVtbl->QueryInterface(element, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
		(void **)&textblock)) && textblock) {
		if (textblock->lpVtbl == &textblock_table) {
			struct textblock *self = (struct textblock *)textblock;
			source = self->name ? self->name : self->text;
			fallback = self->ui.class_name;
		}
		textblock->lpVtbl->Release(textblock);
	}
	if (!source && !fallback &&
		SUCCEEDED(element->lpVtbl->QueryInterface(element, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid,
			(void **)&grid)) && grid) {
		if (grid->lpVtbl == &grid1_table) {
			struct grid *self = ((struct grid1_part *)grid)->owner;
			source = self->name;
			fallback = self->ui.class_name;
		}
		grid->lpVtbl->Release(grid);
	}
	if (!source && !fallback &&
		SUCCEEDED(element->lpVtbl->QueryInterface(element, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton,
			(void **)&button)) && button) {
		if (button->lpVtbl == &button1_table) {
			struct button *self = ((struct button1_part *)button)->owner;
			source = self->name;
			fallback = self->ui.class_name;
		}
		button->lpVtbl->Release(button);
	}
	if (source)
		return WindowsDuplicateString(source, out);
	if (fallback)
		return WindowsCreateString(fallback, (UINT32)lstrlenW(fallback), out);
	/* An element of a class this file does not implement answers its own runtime class name. */
	if (element->lpVtbl->GetRuntimeClassName(element, out) == S_OK && *out)
		return S_OK;
	return *out ? S_OK : E_FAIL;
}

HRESULT raven_xaml_element_render_child(IInspectable *object, UINT32 index, IInspectable **out);

HRESULT raven_xaml_grid_render_child(IInspectable *object, UINT32 index, IInspectable **out)
{
	grid1 *grid = NULL;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, (void **)&grid)) || !grid)
		return S_FALSE;
	if (grid->lpVtbl != &grid1_table) {
		grid->lpVtbl->Release(grid);
		return S_FALSE;
	}
	struct children_vector *children = ((struct grid1_part *)grid)->owner->children;
	if (!children || index >= children->count) {
		grid->lpVtbl->Release(grid);
		return S_OK;
	}
	*out = (IInspectable *)children->items[index];
	(*out)->lpVtbl->AddRef(*out);
	grid->lpVtbl->Release(grid);
	return S_OK;
}

/* The layout pass reads back what the grid lays its children into: the fixed lengths of its definitions and the attached
 * row/column of each child. */

static int definition_value(IInspectable *item, double *value, UINT32 *unit)
{
	row_definition1 *row = NULL;
	column_definition1 *column = NULL;
	if (SUCCEEDED(item->lpVtbl->QueryInterface(item, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition, (void **)&row)) && row) {
		if (row->lpVtbl == &row_definition_table) {
			struct row_definition *self = (struct row_definition *)row;
			*value = self->height;
			*unit = self->unit;
		}
		row->lpVtbl->Release(row);
		return 1;
	}
	if (SUCCEEDED(item->lpVtbl->QueryInterface(item, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition, (void **)&column)) && column) {
		if (column->lpVtbl == &column_definition_table) {
			struct column_definition *self = (struct column_definition *)column;
			*value = self->width;
			*unit = self->unit;
		}
		column->lpVtbl->Release(column);
		return 1;
	}
	return 0;
}

static void tracks_of(struct children_vector *vector, struct raven_xaml_grid_track *tracks, UINT32 capacity, UINT32 *count)
{
	UINT32 filled = 0;
	if (!vector)
		return;
	for (UINT32 index = 0; index < vector->count && filled < capacity; ++index) {
		double value = 0;
		UINT32 unit = 0;
		if (definition_value((IInspectable *)vector->items[index], &value, &unit)) {
			tracks[filled].value = value;
			tracks[filled].unit = unit;
			++filled;
		}
	}
	*count = filled;
}

HRESULT raven_xaml_grid_render_rows(IInspectable *object, struct raven_xaml_grid_track *tracks, UINT32 capacity, UINT32 *count)
{
	grid1 *grid = NULL;
	if (!tracks || !count)
		return E_POINTER;
	*count = 0;
	if (!object)
		return E_POINTER;
	if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, (void **)&grid)) || !grid)
		return S_FALSE;
	if (grid->lpVtbl != &grid1_table) {
		grid->lpVtbl->Release(grid);
		return S_FALSE;
	}
	tracks_of(((struct grid1_part *)grid)->owner->rows, tracks, capacity, count);
	grid->lpVtbl->Release(grid);
	return S_OK;
}

HRESULT raven_xaml_grid_render_columns(IInspectable *object, struct raven_xaml_grid_track *tracks, UINT32 capacity, UINT32 *count)
{
	grid1 *grid = NULL;
	if (!tracks || !count)
		return E_POINTER;
	*count = 0;
	if (!object)
		return E_POINTER;
	if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, (void **)&grid)) || !grid)
		return S_FALSE;
	if (grid->lpVtbl != &grid1_table) {
		grid->lpVtbl->Release(grid);
		return S_FALSE;
	}
	tracks_of(((struct grid1_part *)grid)->owner->columns, tracks, capacity, count);
	grid->lpVtbl->Release(grid);
	return S_OK;
}

/* The attached cell and spans the layout pass reads back: the row and column the child was given (children without them land
 * at cell (0, 0)) plus the row and column spans. The span out-parameters are optional; the default is one track, and a stored
 * span below one reads back as one, so the pass never sees an empty span — a span of zero or negative occupies the child's own
 * track only, and the pass clamps a span reaching past the last track. */
HRESULT raven_xaml_element_render_slot(IInspectable *object, UINT32 *row, UINT32 *column, UINT32 *row_span, UINT32 *column_span)
{
	grid1 *grid = NULL;
	textblock1 *textblock = NULL;
	button1 *button = NULL;
	if (!row || !column)
		return E_POINTER;
	*row = 0;
	*column = 0;
	if (row_span)
		*row_span = 1;
	if (column_span)
		*column_span = 1;
	if (!object)
		return S_OK;
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, (void **)&grid)) && grid) {
		if (grid->lpVtbl == &grid1_table) {
			struct grid *self = ((struct grid1_part *)grid)->owner;
			*row = self->row;
			*column = self->column;
			if (row_span)
				*row_span = self->row_span > 1 ? self->row_span : 1;
			if (column_span)
				*column_span = self->column_span > 1 ? self->column_span : 1;
		}
		grid->lpVtbl->Release(grid);
		return S_OK;
	}
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock, (void **)&textblock)) && textblock) {
		if (textblock->lpVtbl == &textblock_table) {
			struct textblock *self = (struct textblock *)textblock;
			*row = self->row;
			*column = self->column;
			if (row_span)
				*row_span = self->row_span > 1 ? self->row_span : 1;
			if (column_span)
				*column_span = self->column_span > 1 ? self->column_span : 1;
		}
		textblock->lpVtbl->Release(textblock);
		return S_OK;
	}
	if (SUCCEEDED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton, (void **)&button)) && button) {
		if (button->lpVtbl == &button1_table) {
			struct button *self = ((struct button1_part *)button)->owner;
			*row = self->row;
			*column = self->column;
			if (row_span)
				*row_span = self->row_span > 1 ? self->row_span : 1;
			if (column_span)
				*column_span = self->column_span > 1 ? self->column_span : 1;
		}
		button->lpVtbl->Release(button);
	}
	return S_OK;
}

/* Factories */

enum element_kind { element_grid, element_textblock, element_button, element_row_definition, element_column_definition,
	element_presenter, element_border, element_stackpanel, element_image, element_rectangle };

struct element_factory {
	const IActivationFactoryVtbl *lpVtbl;
	LONG refs;
	enum element_kind kind;
};

/* The IButtonFactory face's part: a standalone object the factory hands out (its own refcount, the factory's
 * lifetime stays independent). Defined after the element factories; the query forward-references it. */
struct button_factory_create_part;
static struct button_factory_create_part button_create_part;

static HRESULT WINAPI element_factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
	struct element_factory *self = (struct element_factory *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) && !equal_iid(iid, &IID_IActivationFactory)) {
		/* The IButtonFactory face the .NET Native projection's `new Button()` drives: Windows serves its factory
		 * interface on the same factory object, so the refusal the projection fail-fasts on is served here. */
		if (self->kind == element_button && equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CIButtonFactory)) {
			InterlockedIncrement(&self->refs);
			*out = &button_create_part;
			return S_OK;
		}
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI element_factory_addref(IActivationFactory *iface)
{
	return (ULONG)InterlockedIncrement(&((struct element_factory *)iface)->refs);
}

static ULONG WINAPI element_factory_release(IActivationFactory *iface)
{
	return (ULONG)InterlockedDecrement(&((struct element_factory *)iface)->refs);
}

static HRESULT WINAPI element_factory_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI element_factory_name(IActivationFactory *iface, HSTRING *name)
{
	struct element_factory *self = (struct element_factory *)iface;
	const WCHAR *text = self->kind == element_grid ? grid_class :
		self->kind == element_textblock ? textblock_class :
		self->kind == element_button ? button_class :
		self->kind == element_row_definition ? row_definition_class :
		self->kind == element_presenter ? presenter_class :
		self->kind == element_border ? border_class :
		self->kind == element_stackpanel ? stackpanel_class :
		self->kind == element_image ? image_class :
		self->kind == element_rectangle ? rectangle_class : column_definition_class;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), name);
}

static HRESULT WINAPI element_factory_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI element_factory_activate(IActivationFactory *iface, IInspectable **out)
{
	struct element_factory *self = (struct element_factory *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (self->kind == element_grid) {
		struct grid *grid = create_grid();
		if (!grid)
			return E_OUTOFMEMORY;
		*out = (IInspectable *)&grid->first;
	} else if (self->kind == element_textblock) {
		struct textblock *textblock = create_textblock();
		if (!textblock)
			return E_OUTOFMEMORY;
		*out = (IInspectable *)textblock;
		/* The constructor's initial style: the engine's authored default for the class. */
		if (raven_style_default_attach(*out) != S_OK)
			return E_OUTOFMEMORY;
	} else if (self->kind == element_button) {
		struct button *button = create_button();
		if (!button)
			return E_OUTOFMEMORY;
		*out = (IInspectable *)&button->first;
		if (raven_style_default_attach(*out) != S_OK)
			return E_OUTOFMEMORY;
	} else if (self->kind == element_row_definition) {
		struct row_definition *definition = create_row_definition();
		if (!definition)
			return E_OUTOFMEMORY;
		*out = (IInspectable *)definition;
	} else if (self->kind == element_presenter) {
		struct content_presenter *presenter = create_content_presenter();
		if (!presenter)
			return E_OUTOFMEMORY;
		*out = (IInspectable *)presenter;
	} else if (self->kind == element_border || self->kind == element_stackpanel || self->kind == element_image ||
		self->kind == element_rectangle) {
		/* The shape elements the compiled control XBFs declare build over the grid's machinery (children, the
		 * scalar sizes, the attached slots); they answer the Panel/UIElement faces and name themselves their
		 * own class — only the IGrid face stays the grid's, which the Rectangle answers too so the attached
		 * row it is given stores and the layout pass reads it back. */
		struct grid *shape = create_grid_shape(self->kind == element_border ? grid_shape_border :
			self->kind == element_stackpanel ? grid_shape_stackpanel :
			self->kind == element_image ? grid_shape_image : grid_shape_rectangle);
		if (!shape)
			return E_OUTOFMEMORY;
		*out = (IInspectable *)&shape->first;
		if (self->kind == element_rectangle)
			raven_trace_note("raven-xaml: Rectangle activated\n");
	} else {
		struct column_definition *definition = create_column_definition();
		if (!definition)
			return E_OUTOFMEMORY;
		*out = (IInspectable *)definition;
	}
	return S_OK;
}

static const IActivationFactoryVtbl element_factory_table = {
	.QueryInterface = element_factory_query,
	.AddRef = element_factory_addref,
	.Release = element_factory_release,
	.GetIids = element_factory_iids,
	.GetRuntimeClassName = element_factory_name,
	.GetTrustLevel = element_factory_trust,
	.ActivateInstance = element_factory_activate,
};

static struct element_factory grid_factory = {&element_factory_table, 1, element_grid};
static struct element_factory textblock_factory = {&element_factory_table, 1, element_textblock};
static struct element_factory button_factory = {&element_factory_table, 1, element_button};

/* The IButtonFactory.CreateInstance the projection constructor drives: the same creation ActivateInstance performs
 * (the element's engine default style included). The base controls compose over an outer the button object cannot
 * aggregate yet, so a non-null outer answers E_NOINTERFACE honestly rather than dropping the composed identity. */
struct button_factory_create_part {
	const button_factory_create_vtbl *lpVtbl;
	LONG refs;
};

static HRESULT WINAPI button_create_query(button_factory_create_i *This, REFIID iid, void **out)
{
	struct button_factory_create_part *self = (struct button_factory_create_part *)This;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CIButtonFactory) && !equal_iid(iid, &IID_IUnknown) &&
		!equal_iid(iid, &IID_IInspectable))
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	*out = This;
	return S_OK;
}

static ULONG WINAPI button_create_addref(button_factory_create_i *This)
{
	struct button_factory_create_part *self = (struct button_factory_create_part *)This;
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG WINAPI button_create_release(button_factory_create_i *This)
{
	struct button_factory_create_part *self = (struct button_factory_create_part *)This;
	return (ULONG)InterlockedDecrement(&self->refs);
}

static HRESULT WINAPI button_create_iids(button_factory_create_i *This, ULONG *count, IID **iids)
{
	(void)This;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI button_create_name(button_factory_create_i *This, HSTRING *name)
{
	(void)This;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(button_class, (UINT32)lstrlenW(button_class), name);
}

static HRESULT WINAPI button_create_trust(button_factory_create_i *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}

static HRESULT WINAPI button_create_instance(button_factory_create_i *This, uint64_t outer, uint64_t inner, uint64_t instance)
{
	struct button *button;
	(void)This;
	if (!instance || (outer && !inner))
		return E_POINTER;
	*(void **)(uintptr_t)instance = NULL;
	if (inner)
		*(void **)(uintptr_t)inner = NULL;
	if (outer)
		return E_NOINTERFACE;
	if (!(button = create_button()))
		return E_OUTOFMEMORY;
	*(void **)(uintptr_t)instance = &button->first;
	if (raven_style_default_attach(*(IInspectable **)(uintptr_t)instance) != S_OK)
		return E_OUTOFMEMORY;
	return S_OK;
}

static const button_factory_create_vtbl button_create_vtbl = {
	.QueryInterface = button_create_query,
	.AddRef = button_create_addref,
	.Release = button_create_release,
	.GetIids = button_create_iids,
	.GetRuntimeClassName = button_create_name,
	.GetTrustLevel = button_create_trust,
	.CreateInstance = button_create_instance,
};

static struct button_factory_create_part button_create_part = {&button_create_vtbl, 1};
static struct element_factory row_definition_factory = {&element_factory_table, 1, element_row_definition};
static struct element_factory column_definition_factory = {&element_factory_table, 1, element_column_definition};
static struct element_factory presenter_factory = {&element_factory_table, 1, element_presenter};
static struct element_factory border_factory = {&element_factory_table, 1, element_border};
static struct element_factory stackpanel_factory = {&element_factory_table, 1, element_stackpanel};
static struct element_factory image_factory = {&element_factory_table, 1, element_image};
static struct element_factory rectangle_factory = {&element_factory_table, 1, element_rectangle};

/* UIElement statics expose stable RoutedEvent identities, in metadata slot order. */
DEFINE_GUID(IID_ui_statics, 0x58d3573b, 0xf52c, 0x45be, 0x98, 0x8b, 0xa5, 0x86, 0x95, 0x64, 0x87, 0x3c);
DEFINE_GUID(IID_routed_event, 0xa6b25818, 0x43c1, 0x4c70, 0x86, 0x5c, 0x7b, 0xdd, 0x5a, 0x32, 0xe3, 0x27);
static HRESULT WINAPI routed_event_query(IInspectable *iface, REFIID iid, void **out)
{
 if (!out) return E_POINTER;
 *out = NULL;
 if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) && !IsEqualIID(iid, &IID_IAgileObject) && !IsEqualIID(iid, &IID_routed_event)) return E_NOINTERFACE;
 *out = iface; return S_OK;
}
static ULONG WINAPI routed_event_addref(IInspectable *iface) { (void)iface; return 2; }
static ULONG WINAPI routed_event_release(IInspectable *iface) { (void)iface; return 1; }
static HRESULT WINAPI routed_event_iids(IInspectable *iface, ULONG *count, IID **iids) { (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI routed_event_name(IInspectable *iface, HSTRING *name)
{ static const WCHAR text[] = L"Windows.UI.Xaml.RoutedEvent"; (void)iface; return name ? WindowsCreateString(text, ARRAYSIZE(text)-1, name) : E_POINTER; }
static HRESULT WINAPI routed_event_trust(IInspectable *iface, TrustLevel *level) { (void)iface; return inspectable_trust(level); }
static const IInspectableVtbl routed_event_table = {routed_event_query, routed_event_addref, routed_event_release, routed_event_iids, routed_event_name, routed_event_trust};
static IInspectable routed_events[23] = {
{(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table}, {(IInspectableVtbl *)&routed_event_table} };
static int routed_pointer_kind(IInspectable *event)
{
 if (event == &routed_events[0]) return RAVEN_ELEMENT_KEY_DOWN;
 if (event == &routed_events[1]) return RAVEN_ELEMENT_KEY_UP;
 if (event == &routed_events[2]) return RAVEN_ELEMENT_POINTER_ENTERED;
 if (event == &routed_events[3]) return RAVEN_ELEMENT_POINTER_PRESSED;
 if (event == &routed_events[4]) return RAVEN_ELEMENT_POINTER_MOVED;
 if (event == &routed_events[5]) return RAVEN_ELEMENT_POINTER_RELEASED;
 if (event == &routed_events[6]) return RAVEN_ELEMENT_POINTER_EXITED;
 if (event == &routed_events[7]) return RAVEN_ELEMENT_POINTER_CAPTURE_LOST;
 if (event == &routed_events[9]) return RAVEN_ELEMENT_POINTER_WHEEL;
 return -1;
}
static HRESULT routed_get(unsigned index, IInspectable **out)
{ if (!out) return E_POINTER; *out = &routed_events[index]; return S_OK; }
static HRESULT WINAPI routed_get_0(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(0, out); }
static HRESULT WINAPI routed_get_1(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(1, out); }
static HRESULT WINAPI routed_get_2(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(2, out); }
static HRESULT WINAPI routed_get_3(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(3, out); }
static HRESULT WINAPI routed_get_4(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(4, out); }
static HRESULT WINAPI routed_get_5(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(5, out); }
static HRESULT WINAPI routed_get_6(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(6, out); }
static HRESULT WINAPI routed_get_7(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(7, out); }
static HRESULT WINAPI routed_get_8(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(8, out); }
static HRESULT WINAPI routed_get_9(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(9, out); }
static HRESULT WINAPI routed_get_10(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(10, out); }
static HRESULT WINAPI routed_get_11(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(11, out); }
static HRESULT WINAPI routed_get_12(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(12, out); }
static HRESULT WINAPI routed_get_13(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(13, out); }
static HRESULT WINAPI routed_get_14(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(14, out); }
static HRESULT WINAPI routed_get_15(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(15, out); }
static HRESULT WINAPI routed_get_16(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(16, out); }
static HRESULT WINAPI routed_get_17(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(17, out); }
static HRESULT WINAPI routed_get_18(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(18, out); }
static HRESULT WINAPI routed_get_19(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(19, out); }
static HRESULT WINAPI routed_get_20(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(20, out); }
static HRESULT WINAPI routed_get_21(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(21, out); }
static HRESULT WINAPI routed_get_22(IInspectable *iface, IInspectable **out) { (void)iface; return routed_get(22, out); }
/* shortcut: dependency-property statics require real UIElement property registrations. */
static HRESULT WINAPI ui_property_unimplemented(IInspectable *iface, IInspectable **out)
{ (void)iface; if (!out) return E_POINTER; *out = NULL; return E_NOTIMPL; }
struct ui_statics_table { IInspectableVtbl base; void *slots[40]; };
static IActivationFactory ui_activation;
static IInspectable ui_statics;
static HRESULT WINAPI ui_factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
 (void)iface; if (!out) return E_POINTER; *out = NULL;
 if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID_IAgileObject) || IsEqualIID(iid, &IID_IActivationFactory)) *out = &ui_activation;
 else if (IsEqualIID(iid, &IID_ui_statics)) *out = &ui_statics;
 else return E_NOINTERFACE;
 return S_OK;
}
static ULONG WINAPI ui_factory_addref(IActivationFactory *iface) { (void)iface; return 2; }
static ULONG WINAPI ui_factory_release(IActivationFactory *iface) { (void)iface; return 1; }
static HRESULT WINAPI ui_factory_iids(IActivationFactory *iface, ULONG *count, IID **iids) { (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI ui_factory_name(IActivationFactory *iface, HSTRING *out)
{ static const WCHAR text[] = L"Windows.UI.Xaml.UIElement"; (void)iface; return out ? WindowsCreateString(text, ARRAYSIZE(text)-1, out) : E_POINTER; }
static HRESULT WINAPI ui_factory_trust(IActivationFactory *iface, TrustLevel *level) { (void)iface; return inspectable_trust(level); }
static HRESULT WINAPI ui_factory_activate(IActivationFactory *iface, IInspectable **out)
{ (void)iface; if (!out) return E_POINTER; *out = NULL; return E_NOTIMPL; }
static const IActivationFactoryVtbl ui_activation_table = {ui_factory_query,ui_factory_addref,ui_factory_release,ui_factory_iids,ui_factory_name,ui_factory_trust,ui_factory_activate};
static const struct ui_statics_table ui_statics_vtbl = {
 {(void *)ui_factory_query,(void *)ui_factory_addref,(void *)ui_factory_release,(void *)ui_factory_iids,(void *)ui_factory_name,(void *)ui_factory_trust},
 {(void *)routed_get_0, (void *)routed_get_1, (void *)routed_get_2, (void *)routed_get_3, (void *)routed_get_4, (void *)routed_get_5, (void *)routed_get_6, (void *)routed_get_7, (void *)routed_get_8, (void *)routed_get_9, (void *)routed_get_10, (void *)routed_get_11, (void *)routed_get_12, (void *)routed_get_13, (void *)routed_get_14, (void *)routed_get_15, (void *)routed_get_16, (void *)routed_get_17, (void *)routed_get_18, (void *)routed_get_19, (void *)routed_get_20, (void *)routed_get_21, (void *)routed_get_22, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented, (void *)ui_property_unimplemented }
};
static IActivationFactory ui_activation = {(IActivationFactoryVtbl *)&ui_activation_table};
static IInspectable ui_statics = {(IInspectableVtbl *)&ui_statics_vtbl};

HRESULT raven_xaml_elements_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	struct element_factory *factory;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (same_text(text, length, L"Windows.UI.Xaml.UIElement")) { *out = &ui_activation; return S_OK; }
	if (same_text(text, length, grid_class))
		factory = &grid_factory;
	else if (same_text(text, length, textblock_class))
		factory = &textblock_factory;
	else if (same_text(text, length, button_class))
		factory = &button_factory;
	else if (same_text(text, length, row_definition_class))
		factory = &row_definition_factory;
	else if (same_text(text, length, column_definition_class))
		factory = &column_definition_factory;
	else if (same_text(text, length, presenter_class))
		factory = &presenter_factory;
	else if (same_text(text, length, border_class))
		factory = &border_factory;
	else if (same_text(text, length, stackpanel_class))
		factory = &stackpanel_factory;
	else if (same_text(text, length, image_class))
		factory = &image_factory;
	else if (same_text(text, length, rectangle_class))
		factory = &rectangle_factory;
	else
		return CLASS_E_CLASSNOTAVAILABLE;
	InterlockedIncrement(&factory->refs);
	*out = (IActivationFactory *)factory;
	return S_OK;
}

/* The engine-internal seam the fixtures reach the presenter through (dllexport+GetProcAddress, the pattern every
 * engine-internal entry follows): the runtime catalog does not list the class, so activation goes through here or
 * through the loader's known-class path. */
HRESULT __declspec(dllexport) raven_xaml_content_presenter_factory(IActivationFactory **out)
{
	if (!out)
		return E_POINTER;
	*out = (IActivationFactory *)&presenter_factory;
	InterlockedIncrement(&presenter_factory.refs);
	return S_OK;
}
