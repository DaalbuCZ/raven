/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
#ifndef RAVEN_XAML_INTERNAL_H
#define RAVEN_XAML_INTERNAL_H

#include <windows.h>
#include <stdint.h>
#include <activation.h>
#include <weakreference.h>
#include <windows.ui.core.h>
#include <windows.system.h>

/* ---- Weak references (implemented in resources.c, shared by every object .NET Native projects) ----
 *
 * .NET Native takes IWeakReferenceSource on every WinRT object it projects and treats its absence as a hard error, so the
 * source is a part any owner can embed. The source's IUnknown and lifetime are the owner's; the reference it hands out is a
 * standalone object over a shared target block that holds no reference on the owner: the owner calls
 * raven_weak_source_disconnect from its release path as it frees itself, and a resolution after that answers a null
 * interface — the documented weak contract. A resolution racing the owner's destruction is the caller's synchronization
 * failure, as in Windows.
 *
 * IMarshal is not implemented anywhere in the component: honest apartment marshaling needs cross-apartment invocation
 * infrastructure the engine does not have. Objects answer the IAgileObject marker only where they are genuinely
 * free-threaded (each QI path states its thread-safety justification) and refuse it where they are thread-affine; the
 * traced Shapr3D run survives both refusals. */
typedef HRESULT (WINAPI *raven_weak_query_fn)(void *owner, REFIID iid, void **out);

/* mingw's weakreference.h models both interfaces as IUnknown-only; the Windows ABI derives them from IInspectable, with the
 * one method at vtable slot 6, which is the layout .NET Native is compiled against. */
struct raven_weak_source_vtbl {
	HRESULT (WINAPI *QueryInterface)(IWeakReferenceSource *, REFIID, void **);
	ULONG (WINAPI *AddRef)(IWeakReferenceSource *);
	ULONG (WINAPI *Release)(IWeakReferenceSource *);
	HRESULT (WINAPI *GetIids)(IWeakReferenceSource *, ULONG *, IID **);
	HRESULT (WINAPI *GetRuntimeClassName)(IWeakReferenceSource *, HSTRING *);
	HRESULT (WINAPI *GetTrustLevel)(IWeakReferenceSource *, TrustLevel *);
	HRESULT (WINAPI *GetWeakReference)(IWeakReferenceSource *, IWeakReference **);
};

struct raven_weak_source {
	const struct raven_weak_source_vtbl *lpVtbl;
	void *target;
};

/* the owner embeds the part, gives it its refcount and its QueryInterface, and disconnects it exactly once on release */
void raven_weak_source_init(struct raven_weak_source *weak, void *owner, LONG *owner_refs, raven_weak_query_fn owner_query);
void raven_weak_source_disconnect(struct raven_weak_source *weak);
extern const struct raven_weak_source_vtbl raven_weak_source_table;

/* The composition interfaces the rendering bridge passes between modules; only pointers appear here. */
typedef struct __x_ABI_CWindows_CUI_CComposition_CICompositor __x_ABI_CWindows_CUI_CComposition_CICompositor;
typedef struct __x_ABI_CWindows_CUI_CComposition_CIVisual __x_ABI_CWindows_CUI_CComposition_CIVisual;

extern LONG raven_xaml_objects;
HRESULT raven_current_window(__x_ABI_CWindows_CUI_CCore_CICoreWindow **out);
HRESULT raven_application_factory(HSTRING name, IActivationFactory **out);
HRESULT raven_xaml_application_factory(HSTRING name, IActivationFactory **out);
HRESULT raven_xaml_window_factory(HSTRING name, IActivationFactory **out);
HRESULT raven_xaml_controls_factory(HSTRING name, IActivationFactory **out);
const WCHAR *raven_xaml_class_by_stable(uint32_t stable);
const WCHAR *raven_xaml_property_by_stable(uint32_t stable);
HRESULT raven_xaml_create_named(const WCHAR *name, UINT32 length, IInspectable **out);
HRESULT raven_xaml_create_stable(uint32_t stable, IInspectable **out);
void raven_xaml_page_set_frame(IInspectable *page, IInspectable *frame);
HRESULT raven_xaml_element_set_name(IInspectable *object, const WCHAR *text, UINT32 length);
HRESULT raven_xaml_element_set_integer(IInspectable *object, const WCHAR *name, UINT32 length, int64_t value);
HRESULT raven_xaml_element_set_text(IInspectable *object, const WCHAR *name, UINT32 length, const WCHAR *text, UINT32 text_length);
/* A compiled type reference applied to a property whose value is a type name (SetValueTypeConvertedResolvedType). The
 * engine's only such property is Style.TargetType, which stores the authored short name; other properties answer S_FALSE
 * so the loader refuses the form instead of applying a guess. */
HRESULT raven_xaml_element_set_type(IInspectable *object, const WCHAR *name, UINT32 length, const WCHAR *type_name,
	UINT32 type_length);
HRESULT raven_xaml_element_apply_value(IInspectable *object, const WCHAR *name, UINT32 length, IInspectable *value);
IInspectable *raven_xaml_element_resources_existing(IInspectable *object);
HRESULT raven_xaml_dictionary_lookup(IInspectable *dictionary, const WCHAR *text, UINT32 length, IInspectable **out);
HRESULT raven_xaml_dictionary_lookup_theme(IInspectable *dictionary, const WCHAR *text, UINT32 length,
	const WCHAR *theme, UINT32 theme_length, IInspectable **out);
HRESULT raven_xaml_element_set_object(IInspectable *object, const WCHAR *name, UINT32 length, IInspectable *value);
HRESULT raven_xaml_window_set_name(IInspectable *object, const WCHAR *text, UINT32 length);
BOOL raven_xaml_window_notify(DWORD reason, void *reserved);
HRESULT raven_xaml_resource_context_factory(HSTRING name, IActivationFactory **out);
HRESULT raven_xaml_memory_manager_factory(HSTRING name, IActivationFactory **out);
HRESULT raven_storage_factory(HSTRING name, IActivationFactory **out);
HRESULT raven_style_factory(HSTRING name, IActivationFactory **out);
HRESULT raven_automation_factory(HSTRING name, IActivationFactory **out);
HRESULT raven_dictionary_factory(HSTRING name, IActivationFactory **out);
HRESULT raven_mrt_factory(HSTRING name, IActivationFactory **out);
HRESULT raven_package_factory(HSTRING name, IActivationFactory **out);
HRESULT raven_xaml_elements_factory(HSTRING name, IActivationFactory **out);
HRESULT raven_xaml_elements_set_name(IInspectable *object, const WCHAR *text, UINT32 length);
HRESULT raven_xaml_elements_set_text(IInspectable *object, const WCHAR *name, UINT32 length, const WCHAR *text, UINT32 text_length);
HRESULT raven_xaml_elements_set_object(IInspectable *object, const WCHAR *name, UINT32 length, IInspectable *value);
HRESULT raven_xaml_application_current(IInspectable **out);
HRESULT raven_xaml_window_current(IInspectable **out);
HRESULT raven_launch_arguments_create(IInspectable **out);
HRESULT raven_xaml_load_into(const void *data, uint32_t size, IInspectable *target);
HRESULT raven_pri_get_blob(const WCHAR *name, UINT32 name_length, const BYTE **data, UINT32 *length);
/* The engine-internal Uri creation and boxed string the loader uses for the properties a compiled document sets directly
 * (a ResourceDictionary's Source) and for the keys it inserts (a dictionary's keyed value): the same object the public
 * factory and the style defaults hand out, without a RoGetActivationFactory round trip. Boxed strings come back with one
 * reference; the caller releases through IInspectable. */
HRESULT raven_xaml_uri_create(const WCHAR *text, UINT32 length, IInspectable **out);
IInspectable *raven_xaml_boxed_string(const WCHAR *text, UINT32 length);
HRESULT raven_dispatcher_queue_current(__x_ABI_CWindows_CSystem_CIDispatcherQueue **out);
HRESULT raven_window_post(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window,
	__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler *callback, LONG priority, boolean *accepted);
HRESULT raven_window_timer(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window, UINT_PTR id, BOOL *handled);
HRESULT raven_window_composition_surface(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window, HWND *hwnd, float *scale);
HRESULT raven_composition_factory(HSTRING name, IActivationFactory **out);
void raven_composition_size(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window);
BOOL raven_composition_notify(DWORD reason, void *reserved);
HRESULT raven_composition_instance(__x_ABI_CWindows_CUI_CComposition_CICompositor **out);
HRESULT raven_composition_text_extent(const WCHAR *text, UINT32 length, float font_size, float scale,
	float *width, float *height);
HRESULT raven_composition_attach(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window,
	__x_ABI_CWindows_CUI_CComposition_CIVisual *root);
HRESULT raven_render_hosted(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window, IInspectable *element);
void raven_render_changed(void);
HRESULT raven_xaml_elements_set_float(IInspectable *object, const WCHAR *name, UINT32 length, float value);

/* What the rendering pass reads from an element: the grids fill their area, the frames show their page, the content controls
 * show their content. */
enum element_render_kind { element_render_none, element_render_grid, element_render_frame, element_render_page };
HRESULT raven_xaml_element_render_info(IInspectable *object, enum element_render_kind *kind, COLORREF *color, BYTE *alpha,
	float *width, float *height, UINT32 *child_count);
HRESULT raven_xaml_grid_render_child(IInspectable *object, UINT32 index, IInspectable **out);
HRESULT raven_xaml_frame_render_child(IInspectable *object, IInspectable **out);
BOOL raven_application_notify(DWORD reason, void *reserved);

/* The ControlTemplate surface (xaml_controls.c): raven_xaml_control_template_root answers S_OK for an engine
 * ControlTemplate (root possibly NULL, the caller's AddRef) and S_FALSE for any other object — the kind check the
 * Template puts, the "Template" setter route and the loader's template-node handling share. The is-template probe
 * drives the same vtable check without a root read, and the factory builds templates for the loader path. */
HRESULT raven_xaml_control_template_root(IInspectable *template_object, IInspectable **out);
HRESULT raven_xaml_control_template_put_root(IInspectable *template_object, IInspectable *root);
int raven_xaml_object_is_control_template(IInspectable *object);
HRESULT raven_xaml_control_template_factory(IActivationFactory **out);
/* The templated control's single visual child, what the render walk consults first: the root of the applied
 * ControlTemplate, and — for an engine ContentPresenter — the templated parent's Content, which is how a template
 * shows the control's logical content. */
HRESULT raven_xaml_element_template_child(IInspectable *object, IInspectable **out);

/* What the layout pass reads back from a grid beyond its children: the fixed lengths of its row and column definitions and the
 * attached Grid.Row and Grid.Column each element was given, plus the attached Grid.RowSpan and Grid.ColumnSpan (the span out
 * parameters are optional; a stored span below one reads back as one). Only the fixed pixel lengths lay out; an Auto or Star
 * definition is reported with its unit and the pass treats it as zero until the measure stage distributes the free space. */
struct raven_xaml_grid_track { double value; UINT32 unit; };
HRESULT raven_xaml_grid_render_rows(IInspectable *object, struct raven_xaml_grid_track *tracks, UINT32 capacity, UINT32 *count);
HRESULT raven_xaml_grid_render_columns(IInspectable *object, struct raven_xaml_grid_track *tracks, UINT32 capacity, UINT32 *count);
HRESULT raven_xaml_element_render_slot(IInspectable *object, UINT32 *row, UINT32 *column, UINT32 *row_span, UINT32 *column_span);
HRESULT raven_xaml_elements_set_integer(IInspectable *object, const WCHAR *name, UINT32 length, int64_t value);

#endif
