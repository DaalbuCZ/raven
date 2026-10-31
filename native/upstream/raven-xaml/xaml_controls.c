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
#include <windows.ui.xaml.interop.h>
#include "windows_ui_xaml_controls.h"
#include "windows_ui_xaml_elements.h"
#include "windows_ui_xaml_window.h"

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))
#include <stddef.h>

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

/* The visual-child hook of the element classes and the template kind checks ride xaml_internal.h; the loader drives
 * raven_xaml_object_is_control_template below when a compiled document declares a template node. */

static const WCHAR frame_class[] = L"Windows.UI.Xaml.Controls.Frame";
static const WCHAR page_class[] = L"Windows.UI.Xaml.Controls.Page";
static const WCHAR template_class[] = L"Windows.UI.Xaml.Controls.ControlTemplate";

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
	{ (void)This; if (a1) *(void **)(uintptr_t)a1 = NULL; return E_NOTIMPL; }
#define STUB2(iface, name) static HRESULT WINAPI name(iface *This, uint64_t a1, uint64_t a2) \
	{ (void)This; (void)a1; (void)a2; return E_NOTIMPL; }

struct frame;

struct frame1_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameVtbl *lpVtbl; struct frame *owner; };
struct frame2_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2Vtbl *lpVtbl; struct frame *owner; };
struct frame3_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3Vtbl *lpVtbl; struct frame *owner; };
struct frame4_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4Vtbl *lpVtbl; struct frame *owner; };
struct frame5_part { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5Vtbl *lpVtbl; struct frame *owner; };

struct frame {
	struct frame1_part first;
	struct frame2_part second;
	struct frame3_part third;
	struct frame4_part fourth;
	struct frame5_part fifth;
	LONG refs;
	HSTRING name;
	HSTRING source_type;
	IInspectable *page;
	int32_t cache_size;
	boolean stack_enabled;
};

struct page {
	const __x_ABI_CWindows_CUI_CXaml_CControls_CIPageVtbl *lpVtbl;
	const __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControlVtbl *content_lpVtbl;
	LONG refs;
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
	{ return frame_query_any(((struct part *)This)->owner, iid, out); } \
	static ULONG WINAPI addref_fn(iface *This) { return frame_addref(((struct part *)This)->owner); } \
	static ULONG WINAPI release_fn(iface *This) { return frame_release(((struct part *)This)->owner); }

FRAME_BOILERPLATE(frame1_part, frame1, frame1_query, frame1_addref, frame1_release)
FRAME_BOILERPLATE(frame2_part, frame2, frame2_query, frame2_addref, frame2_release)
FRAME_BOILERPLATE(frame3_part, frame3, frame3_query, frame3_addref, frame3_release)
FRAME_BOILERPLATE(frame4_part, frame4, frame4_query, frame4_addref, frame4_release)
FRAME_BOILERPLATE(frame5_part, frame5, frame5_query, frame5_addref, frame5_release)

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

static HRESULT frame_query_any(struct frame *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame))
		*out = &self->first;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2))
		*out = &self->second;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3))
		*out = &self->third;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4))
		*out = &self->fourth;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5))
		*out = &self->fifth;
	else
		return E_NOINTERFACE;
	frame_addref(self);
	return S_OK;
}

static ULONG frame_addref(struct frame *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG frame_release(struct frame *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->name)
			WindowsDeleteString(self->name);
		if (self->source_type)
			WindowsDeleteString(self->source_type);
		if (self->page)
			IInspectable_Release(self->page);
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

static HRESULT frame_navigate(struct frame *self, uint64_t type_address, uint64_t result)
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
		return S_OK;
	}
	frame_set_source(self, text, length);
	if (self->page)
		IInspectable_Release(self->page);
	self->page = page_object;
	raven_xaml_page_set_frame(page_object, (IInspectable *)&self->first);
	if (navigated)
		*navigated = TRUE;
	raven_render_changed();
	return S_OK;
}

static HRESULT WINAPI frame_navigate_iface(frame1 *iface, uint64_t type, uint64_t parameter, uint64_t result)
{
	(void)parameter;
	return frame_navigate(((struct frame1_part *)iface)->owner, type, result);
}

static HRESULT WINAPI frame_navigate2(frame2 *iface, uint64_t type, uint64_t parameter, uint64_t transition, uint64_t result)
{
	(void)parameter;
	(void)transition;
	return frame_navigate(((struct frame2_part *)iface)->owner, type, result);
}

static HRESULT WINAPI frame_navigate_type(frame5 *iface, uint64_t type, uint64_t parameter, uint64_t transition, uint64_t result)
{
	(void)parameter;
	(void)transition;
	return frame_navigate(((struct frame5_part *)iface)->owner, type, result);
}

static HRESULT WINAPI frame_go_back(frame1 *iface) { (void)iface; return S_OK; }
static HRESULT WINAPI frame_go_forward(frame1 *iface) { (void)iface; return S_OK; }
static HRESULT WINAPI frame_go_back_one(frame3 *iface, uint64_t count) { (void)iface; (void)count; return E_NOTIMPL; }
static HRESULT WINAPI frame_state_out(frame1 *iface, uint64_t out)
{
	(void)iface;
	if (out)
		*(void **)(uintptr_t)out = NULL;
	return E_NOTIMPL;
}
static HRESULT WINAPI frame_state_in(frame1 *iface, uint64_t state) { (void)iface; (void)state; return E_NOTIMPL; }
static HRESULT WINAPI frame_set_state2(frame4 *iface, uint64_t state, uint64_t frame_state) { (void)iface; (void)state; (void)frame_state; return E_NOTIMPL; }
static HRESULT WINAPI frame_stack_out(frame2 *iface, uint64_t out)
{
	(void)iface;
	if (out)
		*(void **)(uintptr_t)out = NULL;
	return E_NOTIMPL;
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
	.add_Navigated = frame_event_add,
	.remove_Navigated = frame_event_remove,
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
	frame->refs = 1;
	frame->stack_enabled = TRUE;
	InterlockedIncrement(&raven_xaml_objects);
	return frame;
}

/* Page */

static HRESULT WINAPI page_query(page1 *iface, REFIID iid, void **out);
static ULONG WINAPI page_addref(page1 *iface);
static ULONG WINAPI page_release(page1 *iface);
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
	.AddRef = page_addref,
	.Release = page_release,
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

static HRESULT WINAPI page_query(page1 *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPage)) {
		page_addref(iface);
		*out = iface;
		return S_OK;
	}
	if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl)) {
		page_addref(iface);
		*out = &((struct page *)iface)->content_lpVtbl;
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI page_addref(page1 *iface)
{
	return (ULONG)InterlockedIncrement(&((struct page *)iface)->refs);
}

static ULONG WINAPI page_release(page1 *iface)
{
	struct page *self = (struct page *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->name) WindowsDeleteString(self->name);
		if (self->frame) IInspectable_Release(self->frame);
		if (self->top_app_bar) IInspectable_Release(self->top_app_bar);
		if (self->bottom_app_bar) IInspectable_Release(self->bottom_app_bar);
		if (self->content) IInspectable_Release(self->content);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

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
	return page_query((page1 *)page_of_content(iface), iid, out);
}

static ULONG WINAPI page_content_addref(page_content_control *iface)
{
	return page_addref((page1 *)page_of_content(iface));
}

static ULONG WINAPI page_content_release(page_content_control *iface)
{
	return page_release((page1 *)page_of_content(iface));
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
	if (frame)
		IInspectable_AddRef(frame);
	if (page->lpVtbl == &page_table) {
		struct page *self = (struct page *)page;
		if (self->frame)
			IInspectable_Release(self->frame);
		self->frame = frame;
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
		return E_INVALIDARG;
	}
	if (iface->lpVtbl != &template_table) {
		iface->lpVtbl->Release(iface);
		if (root)
			IInspectable_Release(root);
		return E_INVALIDARG;
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

enum factory_kind { factory_frame, factory_page, factory_template };

struct controls_factory;

struct controls_part { const IActivationFactoryVtbl *lpVtbl; struct controls_factory *owner; };
struct controls_part1 { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStaticsVtbl *lpVtbl; struct controls_factory *owner; };
struct controls_part2 { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics2Vtbl *lpVtbl; struct controls_factory *owner; };
struct controls_part5 { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameStatics5Vtbl *lpVtbl; struct controls_factory *owner; };
struct controls_page_statics { const __x_ABI_CWindows_CUI_CXaml_CControls_CIPageStaticsVtbl *lpVtbl; struct controls_factory *owner; };
struct controls_frame_create { const __x_ABI_CWindows_CUI_CXaml_CControls_CIFrameFactoryVtbl *lpVtbl; struct controls_factory *owner; };
struct controls_page_create { const __x_ABI_CWindows_CUI_CXaml_CControls_CIPageFactoryVtbl *lpVtbl; struct controls_factory *owner; };

struct controls_factory {
	struct controls_part activation;
	struct controls_part1 statics1;
	struct controls_part2 statics2;
	struct controls_part5 statics5;
	struct controls_page_statics page_statics;
	struct controls_frame_create frame_create;
	struct controls_page_create page_create;
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
			self->kind == factory_page ? page_class : template_class; \
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

static struct controls_factory frame_factory_instance = {
	{&activation_table, NULL}, {&frame_statics1_table, NULL}, {&frame_statics2_table, NULL},
	{&frame_statics5_table, NULL}, {NULL, NULL}, {&frame_create_table, NULL},
	{NULL, NULL}, 1, factory_frame,
};

static struct controls_factory page_factory_instance = {
	{&activation_table, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
	{&page_statics_table, NULL}, {NULL, NULL}, {&page_create_table, NULL}, 1, factory_page,
};

static struct controls_factory template_factory_instance = {
	{&activation_table, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
	{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, 1, factory_template,
};

static LONG factories_ready;

/* A static initializer cannot name the object being defined, so the back pointers are filled once, before first use. */
static void prepare_factories(void)
{
	if (InterlockedCompareExchange(&factories_ready, 1, 0) == 0) {
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
		self->kind == factory_page ? page_class : template_class;
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
		struct frame *frame = create_frame();
		if (!frame)
			return E_OUTOFMEMORY;
		*out = (IInspectable *)&frame->first;
	} else if (self->kind == factory_page) {
		struct page *page = create_page();
		if (!page)
			return E_OUTOFMEMORY;
		*out = (IInspectable *)page;
	} else {
		struct control_template *template = create_control_template();
		if (!template)
			return E_OUTOFMEMORY;
		*out = (IInspectable *)template;
	}
	return S_OK;
}

/* Composable factories: the created base is returned as the instance and no separate inner object is reported. */
static HRESULT WINAPI frame_create(frame_factory *iface, uint64_t outer, uint64_t inner, uint64_t instance)
{
	struct frame *frame;
	(void)iface;
	(void)outer;
	if (!instance)
		return E_POINTER;
	*(void **)(uintptr_t)instance = NULL;
	if (inner)
		*(void **)(uintptr_t)inner = NULL;
	if (!(frame = create_frame()))
		return E_OUTOFMEMORY;
	*(void **)(uintptr_t)instance = &frame->first;
	return S_OK;
}

static HRESULT WINAPI page_create(page_factory *iface, uint64_t outer, uint64_t inner, uint64_t instance)
{
	struct page *page;
	(void)iface;
	(void)outer;
	if (!instance)
		return E_POINTER;
	*(void **)(uintptr_t)instance = NULL;
	if (inner)
		*(void **)(uintptr_t)inner = NULL;
	if (!(page = create_page()))
		return E_OUTOFMEMORY;
	*(void **)(uintptr_t)instance = page;
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
	if (same_text(name, length, L"Template") || same_text(name, length, L"ContentTemplate"))
		return raven_xaml_elements_set_object(object, name, length, value);
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
	if (length == ARRAYSIZE(frame_class) - 1 && CompareStringOrdinal(text, (int)length, frame_class, -1, FALSE) == CSTR_EQUAL)
		factory = &frame_factory_instance;
	else if (length == ARRAYSIZE(page_class) - 1 && CompareStringOrdinal(text, (int)length, page_class, -1, FALSE) == CSTR_EQUAL)
		factory = &page_factory_instance;
	else if (length == ARRAYSIZE(template_class) - 1 && CompareStringOrdinal(text, (int)length, template_class, -1, FALSE) == CSTR_EQUAL)
		factory = &template_factory_instance;
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
