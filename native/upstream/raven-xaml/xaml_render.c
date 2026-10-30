/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the rendering path from the element tree to the composition stage.
 *
 * A hosted element tree — the content a XAML window carries, or the page a frame navigated to — projects onto the composition
 * visuals the previous stage provides: every grid becomes a background sprite sized to its area (the window for the root, its
 * Width and Height or its cell for a child), its children render above it in document order placed into the cells its row and
 * column definitions describe, and the flat sprite list attaches to the composition target bound to the hosting window. A cell
 * is the fixed pixel length of its track; Auto and Star lengths and slots beyond the definitions lay out as zero, and an axis
 * without definitions gives every child the grid's extent, until the measure stage arrives. A child's attached RowSpan and
 * ColumnSpan merge its own track with the ones after it into one cell: the spanning child fills the merged region without
 * growing any Auto track (the WPF distribution of a spanning want across the spanned Auto tracks is not implemented), a span
 * reaching past the last track clamps to the tracks that exist, and a span below one is one track. A TextBlock paints its text at the
 * top-left of the cell the layout gave it through the text payload its sprite carries — measuring is not here yet, so the block
 * does not influence its own size, and the measure and arrange stage will. Control chrome does not render yet, and a grid with
 * no colour fills nothing. The pass rebuilds the visual tree on every change — measure and arrange, damage tracking and the GPU
 * path are the later work — and re-renders when the hosted content, a background, a size, the child or definition list changes. */
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <windows.ui.composition.h>
#include <windows.foundation.numerics.h>
#include "windows_ui_xaml_elements.h"
#include "windows_ui_xaml_layout.h"
#include "xaml_input.h"

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

/* The fixed-capacity definition arrays one grid reads; a grid with more tracks than this lays the rest out beyond the last read
 * one, as zero-size cells at the end of the tracks. */
#define RENDER_TRACKS 32

typedef __x_ABI_CWindows_CUI_CComposition_CICompositor compositor1;
typedef __x_ABI_CWindows_CUI_CComposition_CIContainerVisual container1;
typedef __x_ABI_CWindows_CUI_CComposition_CISpriteVisual sprite1;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionBrush brush1;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush colorbrush1;
typedef __x_ABI_CWindows_CUI_CComposition_CIVisual visual1;
typedef __x_ABI_CWindows_CUI_CComposition_CIVisualCollection collection1;
typedef __x_ABI_CWindows_CUI_CColor ui_color;
typedef __x_ABI_CWindows_CFoundation_CNumerics_CVector2 vector2;
typedef __x_ABI_CWindows_CFoundation_CNumerics_CVector3 vector3;

/* The text payload crosses the modules by function: the element layer reads the TextBlock state back (xaml_elements.c) and
 * the composition layer takes the copy its sprite owns (xaml_composition.c). The shared internal header does not carry these
 * two declarations yet, so they stand here where the projection uses them. */
HRESULT raven_composition_sprite_text(visual1 *visual, const WCHAR *text, UINT32 length, float font_size,
	COLORREF color, BYTE alpha);
HRESULT raven_xaml_element_render_text(IInspectable *element, const WCHAR **text, UINT32 *length, float *font_size,
	COLORREF *color, BYTE *alpha);

struct render_root {
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *window;
	IInspectable *element;
	struct render_root *next;
};

static struct render_root *render_roots;
static LONG render_ready;

static void render_lock_init(void)
{
	/* The hosted roots are touched from the UI threads only; the flag keeps the list creation single-shot. */
	if (InterlockedCompareExchange(&render_ready, 1, 0) == 0)
		render_roots = NULL;
}

/* One hosted tree per window: hosting again replaces the element. The routing follows the hosting, since pointer events only
 * make sense while a tree is projected. */

HRESULT raven_render_hosted(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window, IInspectable *element)
{
	if (!window)
		return E_POINTER;
	HRESULT routed = raven_input_hosted(window, element);
	if (FAILED(routed))
		return routed;
	render_lock_init();
	struct render_root **link = &render_roots;
	while (*link && (*link)->window != window)
		link = &(*link)->next;
	if (*link) {
		if ((*link)->element)
			(*link)->element->lpVtbl->Release((*link)->element);
		(*link)->element = element;
		/* A replaced root is owned the same way the first one was: without this reference the third hosting of a
		 * window releases an element the caller still holds. */
		if (element)
			element->lpVtbl->AddRef(element);
	} else {
		struct render_root *root = ALLOC(sizeof *root);
		if (!root)
			return E_OUTOFMEMORY;
		root->window = window;
		root->element = element;
		if (element)
			element->lpVtbl->AddRef(element);
		root->next = render_roots;
		render_roots = root;
	}
	raven_render_changed();
	return S_OK;
}

/* The projection of one element into its parent's visual collection. */

static HRESULT render_append_sprite(container1 *parent, compositor1 *compositor, COLORREF color, BYTE alpha,
	float x, float y, float width, float height)
{
	sprite1 *sprite = NULL;
	colorbrush1 *brush = NULL;
	collection1 *children = NULL;
	HRESULT result;
	if ((result = compositor->lpVtbl->CreateSpriteVisual(compositor, &sprite)) != S_OK || !sprite)
		return result;
	if ((result = compositor->lpVtbl->CreateColorBrushWithColor(compositor, (ui_color){alpha, GetRValue(color),
		GetGValue(color), GetBValue(color)}, &brush)) != S_OK || !brush)
		goto done;
	{
		visual1 *part = NULL;
		if ((result = sprite->lpVtbl->QueryInterface(sprite, &IID___x_ABI_CWindows_CUI_CComposition_CIVisual,
			(void **)&part)) == S_OK && part) {
			vector2 size = {width, height};
			vector3 offset = {x, y, 0};
			part->lpVtbl->put_Size(part, size);
			part->lpVtbl->put_Offset(part, offset);
			part->lpVtbl->Release(part);
		}
	}
	if ((result = sprite->lpVtbl->put_Brush(sprite, (brush1 *)brush)) != S_OK)
		goto done;
	if ((result = parent->lpVtbl->get_Children(parent, &children)) != S_OK || !children)
		goto done;
	{
		/* The collection stores Visual references; the sprite's own interface is a different part of the object. */
		visual1 *part = NULL;
		if ((result = sprite->lpVtbl->QueryInterface(sprite, &IID___x_ABI_CWindows_CUI_CComposition_CIVisual,
			(void **)&part)) == S_OK && part) {
			result = children->lpVtbl->InsertAtTop(children, part);
			part->lpVtbl->Release(part);
		}
	}
done:
	if (children)
		children->lpVtbl->Release(children);
	if (brush)
		brush->lpVtbl->Release(brush);
	if (sprite)
		sprite->lpVtbl->Release(sprite);
	return result;
}

/* The text sprite of a TextBlock: no background brush — a TextBlock has no background of its own — and the text payload
 * the presentation draws at the sprite's origin. The composition side copies the text, so the element's own buffer never
 * outlives the projection that read it. */
static HRESULT render_append_text_sprite(container1 *parent, compositor1 *compositor, const WCHAR *text, UINT32 length,
	float font_size, COLORREF color, BYTE alpha, float x, float y, float width, float height)
{
	sprite1 *sprite = NULL;
	collection1 *children = NULL;
	HRESULT result;
	if ((result = compositor->lpVtbl->CreateSpriteVisual(compositor, &sprite)) != S_OK || !sprite)
		return result;
	{
		visual1 *part = NULL;
		if ((result = sprite->lpVtbl->QueryInterface(sprite, &IID___x_ABI_CWindows_CUI_CComposition_CIVisual,
			(void **)&part)) == S_OK && part) {
			vector2 size = {width, height};
			vector3 offset = {x, y, 0};
			part->lpVtbl->put_Size(part, size);
			part->lpVtbl->put_Offset(part, offset);
			part->lpVtbl->Release(part);
		}
	}
	if (result == S_OK)
		result = raven_composition_sprite_text((visual1 *)sprite, text, length, font_size, color, alpha);
	if (result != S_OK || (result = parent->lpVtbl->get_Children(parent, &children)) != S_OK || !children)
		goto done;
	{
		/* The collection stores Visual references; the sprite's own interface is a different part of the object. */
		visual1 *part = NULL;
		if ((result = sprite->lpVtbl->QueryInterface(sprite, &IID___x_ABI_CWindows_CUI_CComposition_CIVisual,
			(void **)&part)) == S_OK && part) {
			result = children->lpVtbl->InsertAtTop(children, part);
			part->lpVtbl->Release(part);
		}
	}
done:
	if (children)
		children->lpVtbl->Release(children);
	if (sprite)
		sprite->lpVtbl->Release(sprite);
	return result;
}

/* The single visual child of the non-panel containers: a frame shows its current page, a content control (a Page, a Button)
 * shows its content. */
static IInspectable *render_single_child(IInspectable *element)
{
	IInspectable *child = NULL;
	if (raven_xaml_frame_render_child(element, &child) == S_OK && child)
		return child;
	__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *content_control = NULL;
	if (SUCCEEDED(element->lpVtbl->QueryInterface(element,
		&IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl, (void **)&content_control)) && content_control) {
		content_control->lpVtbl->get_Content(content_control, (uint64_t)(uintptr_t)&child);
		content_control->lpVtbl->Release(content_control);
	}
	return child;
}

/* One axis of the grid's cells. Fixed pixel tracks take their length; Auto tracks take the largest content that wants them;
 * Star tracks share what remains after fixed and auto, proportionally to their values (equal shares when no star carries a
 * weight). The measured wants come from the children the caller measured, correlated by their row/column slot; a slot beyond
 * the definitions is a zero-size cell after the last track, and an axis without definitions gives every child the grid's
 * extent. The span rule: a child spanning several tracks does not grow any single Auto track — its want covers the merged
 * region rather than one track, and the WPF distribution of a spanning want across the spanned Auto tracks is not
 * implemented — so those wants are skipped here and the spanning child fills whatever the merged region resolved to. */
static void track_resolve(const struct raven_xaml_grid_track *tracks, UINT32 count, float extent,
	const float *desired, const UINT32 *slots, const UINT32 *spans, UINT32 measured, float *offsets, float *sizes)
{
	float fixed_length = 0, star_weight = 0;
	float auto_lengths[RENDER_TRACKS] = {0};
	for (UINT32 index = 0; index < count && index < RENDER_TRACKS; ++index) {
		if (tracks[index].unit == GridUnitType_Pixel)
			fixed_length += (float)tracks[index].value;
		else if (tracks[index].unit == GridUnitType_Star && tracks[index].value > 0)
			star_weight += (float)tracks[index].value;
	}
	for (UINT32 child = 0; child < measured; ++child) {
		const UINT32 slot = slots[child];
		if (spans[child] > 1)
			continue;
		if (slot < count && slot < RENDER_TRACKS && tracks[slot].unit == GridUnitType_Auto &&
			desired[child] > auto_lengths[slot])
			auto_lengths[slot] = desired[child];
	}
	float remaining = extent - fixed_length;
	for (UINT32 index = 0; index < count && index < RENDER_TRACKS; ++index)
		if (tracks[index].unit == GridUnitType_Auto)
			remaining -= auto_lengths[index];
	if (remaining < 0)
		remaining = 0;
	UINT32 star_count = 0;
	for (UINT32 index = 0; index < count && index < RENDER_TRACKS; ++index)
		if (tracks[index].unit == GridUnitType_Star)
			++star_count;
	float origin = 0;
	for (UINT32 index = 0; index < count && index < RENDER_TRACKS; ++index) {
		float size = 0;
		if (tracks[index].unit == GridUnitType_Pixel)
			size = (float)tracks[index].value;
		else if (tracks[index].unit == GridUnitType_Auto)
			size = auto_lengths[index];
		else if (tracks[index].unit == GridUnitType_Star)
			size = star_weight > 0 ? remaining * ((float)tracks[index].value / star_weight)
				: remaining / star_count;
		offsets[index] = origin;
		sizes[index] = size;
		origin += size;
	}
}

/* The cell one child lands in: its track's resolved rectangle, a zero-size cell after the last track for slots beyond the
 * definitions, and the grid's extent when the axis has no definitions at all. A span merges the tracks from the child's own
 * through the last spanned one into one cell running from its track's origin to the end of the last spanned track; a span
 * reaching past the last track clamps to the tracks that exist, a span below one is the child's own track, and a slot beyond
 * the definitions stays the zero-size cell after the end whatever the span says. */
static void track_cell(const float *offsets, const float *sizes, UINT32 count, UINT32 index, UINT32 span, float extent,
	float *offset, float *size)
{
	if (!count) {
		*offset = 0.0f;
		*size = extent;
		return;
	}
	if (index >= count) {
		float total = 0;
		for (UINT32 position = 0; position < count; ++position)
			total += sizes[position];
		*offset = total;
		*size = 0;
		return;
	}
	if (span < 1)
		span = 1;
	if (span > count - index)
		span = count - index;
	if (span == 1) {
		*offset = offsets[index];
		*size = sizes[index];
		return;
	}
	*offset = offsets[index];
	*size = offsets[index + span - 1] + sizes[index + span - 1] - offsets[index];
}

static HRESULT render_element(IInspectable *element, container1 *parent, compositor1 *compositor,
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *window, float scale, float parent_width, float parent_height,
	float origin_x, float origin_y);

/* The size an element wants on one axis when the layout asks: its own Width/Height when given, otherwise what its content
 * needs — a text block's measured run, a nested grid's resolved tracks, a content control's child. Star tracks measure as
 * zero (they share what is left over), so a grid of stars wants nothing and fills whatever cell it is given. */
static HRESULT measure_element(IInspectable *element, float scale, float *width, float *height)
{
	enum element_render_kind kind;
	COLORREF color;
	BYTE alpha;
	float own_width, own_height;
	UINT32 child_count;
	HRESULT result = raven_xaml_element_render_info(element, &kind, &color, &alpha, &own_width, &own_height, &child_count);
	if (FAILED(result))
		return result;
	float wanted_width = 0, wanted_height = 0;
	if (kind == element_render_grid) {
		struct raven_xaml_grid_track rows[RENDER_TRACKS], columns[RENDER_TRACKS];
		UINT32 row_count = 0, column_count = 0;
		raven_xaml_grid_render_rows(element, rows, RENDER_TRACKS, &row_count);
		raven_xaml_grid_render_columns(element, columns, RENDER_TRACKS, &column_count);
		float column_sizes[RENDER_TRACKS] = {0}, row_sizes[RENDER_TRACKS] = {0};
		for (UINT32 index = 0; index < child_count; ++index) {
			IInspectable *child = NULL;
			if (raven_xaml_grid_render_child(element, index, &child) != S_OK || !child)
				continue;
			float child_width = 0, child_height = 0;
			measure_element(child, scale, &child_width, &child_height);
			UINT32 row = 0, column = 0, row_span = 1, column_span = 1;
			raven_xaml_element_render_slot(child, &row, &column, &row_span, &column_span);
			/* The span rule: a child spanning tracks does not grow the Auto track it starts in. */
			if (column_span <= 1 && column < column_count && column < RENDER_TRACKS &&
				columns[column].unit == GridUnitType_Auto && child_width > column_sizes[column])
				column_sizes[column] = child_width;
			if (row_span <= 1 && row < row_count && row < RENDER_TRACKS &&
				rows[row].unit == GridUnitType_Auto && child_height > row_sizes[row])
				row_sizes[row] = child_height;
			child->lpVtbl->Release(child);
		}
		for (UINT32 index = 0; index < column_count && index < RENDER_TRACKS; ++index)
			wanted_width += columns[index].unit == GridUnitType_Pixel ? (float)columns[index].value : column_sizes[index];
		for (UINT32 index = 0; index < row_count && index < RENDER_TRACKS; ++index)
			wanted_height += rows[index].unit == GridUnitType_Pixel ? (float)rows[index].value : row_sizes[index];
	} else {
		const WCHAR *text = NULL;
		UINT32 text_length = 0;
		float font_size = 0;
		COLORREF text_color = 0;
		BYTE text_alpha = 0;
		if (raven_xaml_element_render_text(element, &text, &text_length, &font_size, &text_color, &text_alpha) == S_OK) {
			/* A text block wants its measured run; a block without text wants nothing. */
			if (text_length) {
				float text_width = 0, text_height = 0;
				if (raven_composition_text_extent(text, text_length, font_size, scale, &text_width, &text_height) == S_OK) {
					wanted_width = text_width;
					wanted_height = text_height;
				}
			}
		} else {
			IInspectable *child = render_single_child(element);
			if (child) {
				measure_element(child, scale, &wanted_width, &wanted_height);
				child->lpVtbl->Release(child);
			}
		}
	}
	*width = own_width > 0 ? own_width : wanted_width;
	*height = own_height > 0 ? own_height : wanted_height;
	return S_OK;
}

static HRESULT render_element(IInspectable *element, container1 *parent, compositor1 *compositor,
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *window, float scale, float parent_width, float parent_height,
	float origin_x, float origin_y)
{
	enum element_render_kind kind;
	COLORREF color;
	BYTE alpha;
	float width, height;
	UINT32 child_count;
	HRESULT result = raven_xaml_element_render_info(element, &kind, &color, &alpha, &width, &height, &child_count);
	if (FAILED(result))
		return result;
	const float area_width = width > 0 ? width : parent_width;
	const float area_height = height > 0 ? height : parent_height;
	/* The hit-test geometry follows the projection: the rectangle the layout gave this element, recorded in paint order so the
	 * reverse walk of the routing finds the topmost element first. */
	raven_input_record(window, element, origin_x, origin_y, area_width, area_height);
	if (kind == element_render_grid && alpha) {
		struct raven_xaml_grid_track rows[RENDER_TRACKS], columns[RENDER_TRACKS];
		UINT32 row_count = 0, column_count = 0;
		raven_xaml_grid_render_rows(element, rows, RENDER_TRACKS, &row_count);
		raven_xaml_grid_render_columns(element, columns, RENDER_TRACKS, &column_count);
		result = render_append_sprite(parent, compositor, color, alpha, origin_x, origin_y, area_width, area_height);
		if (FAILED(result))
			return result;
		/* The children the Auto tracks measure: the first RENDER_TRACKS children, each measured once, correlated by its
		 * row/column slot and span. Children beyond the cap render without sizing Auto tracks. */
		float desired_width[RENDER_TRACKS] = {0}, desired_height[RENDER_TRACKS] = {0};
		UINT32 child_rows[RENDER_TRACKS] = {0}, child_columns[RENDER_TRACKS] = {0};
		UINT32 child_row_spans[RENDER_TRACKS] = {0}, child_column_spans[RENDER_TRACKS] = {0};
		const UINT32 measured = child_count < RENDER_TRACKS ? child_count : RENDER_TRACKS;
		for (UINT32 index = 0; index < measured; ++index) {
			IInspectable *child = NULL;
			if (raven_xaml_grid_render_child(element, index, &child) == S_OK && child) {
				measure_element(child, scale, &desired_width[index], &desired_height[index]);
				raven_xaml_element_render_slot(child, &child_rows[index], &child_columns[index],
					&child_row_spans[index], &child_column_spans[index]);
				child->lpVtbl->Release(child);
			}
		}
		float column_offsets[RENDER_TRACKS], column_sizes[RENDER_TRACKS];
		float row_offsets[RENDER_TRACKS], row_sizes[RENDER_TRACKS];
		track_resolve(columns, column_count, area_width, desired_width, child_columns, child_column_spans, measured,
			column_offsets, column_sizes);
		track_resolve(rows, row_count, area_height, desired_height, child_rows, child_row_spans, measured,
			row_offsets, row_sizes);
		for (UINT32 index = 0; index < child_count; ++index) {
			IInspectable *child = NULL;
			if (raven_xaml_grid_render_child(element, index, &child) == S_OK && child) {
				UINT32 row = 0, column = 0, row_span = 1, column_span = 1;
				float offset_x, offset_y, cell_width, cell_height;
				raven_xaml_element_render_slot(child, &row, &column, &row_span, &column_span);
				track_cell(column_offsets, column_sizes, column_count, column, column_span, area_width, &offset_x, &cell_width);
				track_cell(row_offsets, row_sizes, row_count, row, row_span, area_height, &offset_y, &cell_height);
				render_element(child, parent, compositor, window, scale, cell_width, cell_height,
					origin_x + offset_x, origin_y + offset_y);
				child->lpVtbl->Release(child);
			}
		}
		return S_OK;
	}
	/* A TextBlock paints its text at the top-left of the cell the layout gave it; the sprite carries the text payload and
	 * the presentation draws it. A block without text paints nothing, and the block does not yet influence its own size —
	 * the measure and arrange stage will. */
	const WCHAR *text = NULL;
	UINT32 text_length = 0;
	float font_size = 0;
	COLORREF text_color = 0;
	BYTE text_alpha = 0;
	if (raven_xaml_element_render_text(element, &text, &text_length, &font_size, &text_color, &text_alpha) == S_OK) {
		if (text_length) {
			result = render_append_text_sprite(parent, compositor, text, text_length, font_size, text_color, text_alpha,
				origin_x, origin_y, area_width, area_height);
			if (FAILED(result))
				return result;
		}
		return S_OK;
	}
	IInspectable *child = render_single_child(element);
	if (child) {
		render_element(child, parent, compositor, window, scale, area_width, area_height, origin_x, origin_y);
		child->lpVtbl->Release(child);
	}
	return S_OK;
}

static void render_pass(struct render_root *root)
{
	compositor1 *compositor = NULL;
	container1 *visual = NULL;
	if (raven_composition_instance(&compositor) != S_OK || !compositor)
		return;
	if (compositor->lpVtbl->CreateContainerVisual(compositor, &visual) != S_OK || !visual) {
		compositor->lpVtbl->Release(compositor);
		return;
	}
	/* The pass rebuilds the hit-test geometry of this window from scratch. */
	raven_input_begin(root->window);
	if (root->element) {
		/* The root fills the window's client area in DIPs. */
		__x_ABI_CWindows_CUI_CCore_CICoreWindow *window = root->window;
		__x_ABI_CWindows_CFoundation_CRect bounds;
		float width = 0, height = 0;
		if (__x_ABI_CWindows_CUI_CCore_CICoreWindow_get_Bounds(root->window, &bounds) == S_OK) {
			width = bounds.Width;
			height = bounds.Height;
		}
		float dpi_scale = 1.0f;
		HWND surface = NULL;
		if (raven_window_composition_surface(root->window, &surface, &dpi_scale) != S_OK)
			dpi_scale = 1.0f;
		render_element(root->element, visual, compositor, window, dpi_scale, width, height, 0.0f, 0.0f);
	}
	raven_composition_attach(root->window, (visual1 *)visual);
	visual->lpVtbl->Release(visual);
	compositor->lpVtbl->Release(compositor);
}

/* Any change to a hosted tree rebuilds its projection; the composition present makes it visible. */

void raven_render_changed(void)
{
	if (!render_ready)
		return;
	for (struct render_root *root = render_roots; root; root = root->next)
		render_pass(root);
}
