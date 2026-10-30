/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
#ifndef RAVEN_XAML_INTERNAL_H
#define RAVEN_XAML_INTERNAL_H

#include <windows.h>
#include <stdint.h>
#include <activation.h>
#include <windows.ui.core.h>
#include <windows.system.h>

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
