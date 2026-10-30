/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.UI.Composition object model and its presentation.
 *
 * A Compositor creates the visuals a framework tree is made of. CreateTargetForCurrentView binds a target to the calling
 * thread's CoreWindow, the way the UWP XAML framework attaches its own tree; the target's root then draws into that window.
 * The members the graphics stage does not implement yet (animations, effects, surfaces, property sets) answer E_NOTIMPL, and
 * every stub keeps the exact signature of its slot, so a call never depends on an argument count the metadata did not give.
 *
 * Presentation is the CPU fallback the composition stage starts from: the tree draws through GDI into the window surface,
 * synchronously on every mutation and on window size changes, so a caller always sees the tree it just described. Damage
 * tracking and the GPU path replace this later. Alpha blending does not apply yet: a brush paints its colour opaque, and a
 * visual with zero opacity paints nothing. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <stddef.h>
#include <string.h>
#include <winstring.h>
#include <windows.ui.composition.h>
#include <windows.foundation.numerics.h>

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

typedef __x_ABI_CWindows_CUI_CComposition_CICompositor compositor1;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositorVtbl compositor1Vtbl;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionObject composition_object1;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionObjectVtbl composition_object1Vtbl;
typedef __x_ABI_CWindows_CUI_CComposition_CIVisual visual1;
typedef __x_ABI_CWindows_CUI_CComposition_CIVisualVtbl visual1Vtbl;
typedef __x_ABI_CWindows_CUI_CComposition_CIContainerVisual container1;
typedef __x_ABI_CWindows_CUI_CComposition_CIContainerVisualVtbl container1Vtbl;
typedef __x_ABI_CWindows_CUI_CComposition_CISpriteVisual sprite1;
typedef __x_ABI_CWindows_CUI_CComposition_CISpriteVisualVtbl sprite1Vtbl;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionBrush brush1;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionBrushVtbl brush1Vtbl;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush colorbrush1;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionColorBrushVtbl colorbrush1Vtbl;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionTarget target1;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionTargetVtbl target1Vtbl;
typedef __x_ABI_CWindows_CUI_CColor ui_color;
typedef __x_ABI_CWindows_CFoundation_CNumerics_CVector2 vector2;
typedef __x_ABI_CWindows_CFoundation_CNumerics_CVector3 vector3;

static const WCHAR compositor_class[] = L"Windows.UI.Composition.Compositor";
static const WCHAR container_class[] = L"Windows.UI.Composition.ContainerVisual";
static const WCHAR sprite_class[] = L"Windows.UI.Composition.SpriteVisual";
static const WCHAR brush_class[] = L"Windows.UI.Composition.CompositionBrush";
static const WCHAR colorbrush_class[] = L"Windows.UI.Composition.CompositionColorBrush";
static const WCHAR target_class[] = L"Windows.UI.Composition.CompositionTarget";

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

/* Stubs for the slots the graphics stage does not implement. */
#define OUT_STUB(prefix, name) static HRESULT WINAPI name(prefix *This, void **value) \
	{ (void)This; if (value) *value = NULL; return E_NOTIMPL; }
#define IN_STUB(prefix, name, type) static HRESULT WINAPI name(prefix *This, type value) \
	{ (void)This; (void)value; return E_NOTIMPL; }

struct composition_brush;
struct composition_visual;
struct composition_container;
struct compositor_state;
struct composition_target;

/* The children collection: a VisualCollection owned by its container. InsertAtBottom inserts at the bottom of the draw
 * order, InsertAtTop at the top; the paint pass walks the array bottom to top. */
struct children_vector;

struct children_vector_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(struct children_vector *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(struct children_vector *This);
	ULONG (STDMETHODCALLTYPE *Release)(struct children_vector *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(struct children_vector *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(struct children_vector *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(struct children_vector *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_Count)(struct children_vector *This, INT32 *value);
	HRESULT (STDMETHODCALLTYPE *InsertAbove)(struct children_vector *This, visual1 *child, visual1 *sibling);
	HRESULT (STDMETHODCALLTYPE *InsertAtBottom)(struct children_vector *This, visual1 *child);
	HRESULT (STDMETHODCALLTYPE *InsertAtTop)(struct children_vector *This, visual1 *child);
	HRESULT (STDMETHODCALLTYPE *InsertBelow)(struct children_vector *This, visual1 *child, visual1 *sibling);
	HRESULT (STDMETHODCALLTYPE *Remove)(struct children_vector *This, visual1 *child);
	HRESULT (STDMETHODCALLTYPE *RemoveAll)(struct children_vector *This);
};

struct children_vector {
	const struct children_vector_vtbl *lpVtbl;
	LONG refs;
	struct composition_container *owner;
	visual1 **items;
	UINT32 count, capacity;
};

static HRESULT children_add(struct children_vector *self, visual1 *value, UINT32 position);

struct compositor_state {
	struct { const compositor1Vtbl *lpVtbl; struct compositor_state *owner; } first;
	struct { const composition_object1Vtbl *lpVtbl; struct compositor_state *owner; } object;
	LONG refs;
	struct composition_target *targets;
};

struct composition_target {
	struct { const target1Vtbl *lpVtbl; struct composition_target *owner; } first;
	struct { const composition_object1Vtbl *lpVtbl; struct composition_target *owner; } object;
	LONG refs;
	struct compositor_state *compositor;
	struct composition_target *next, *previous;
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *window;
	visual1 *root;
	struct composition_visual *root_data;
};

enum composition_kind { composition_container, composition_sprite };

struct composition_visual {
	struct { const visual1Vtbl *lpVtbl; struct composition_visual *owner; } first;
	struct { const composition_object1Vtbl *lpVtbl; struct composition_visual *owner; } object;
	LONG refs;
	enum composition_kind kind;
	struct compositor_state *compositor;
	struct composition_container *parent;
	vector3 offset;
	vector2 size;
	float opacity;
	boolean visible;
};

struct composition_container {
	struct composition_visual base;
	struct { const container1Vtbl *lpVtbl; struct composition_container *owner; } container;
	struct children_vector *children;
};

struct composition_sprite {
	struct composition_visual base;
	struct { const sprite1Vtbl *lpVtbl; struct composition_sprite *owner; } sprite;
	brush1 *brush;
	struct composition_brush *brush_data;
	/* The text payload a TextBlock's sprite carries: a UTF-16 copy the sprite owns and frees when the visual releases,
	 * with the font size in DIPs and the foreground colour the presentation draws it in. */
	WCHAR *text;
	UINT32 text_length;
	float text_font_size;
	COLORREF text_color;
	BYTE text_alpha;
};

struct composition_brush {
	struct { const colorbrush1Vtbl *lpVtbl; struct composition_brush *owner; } first;
	struct { const brush1Vtbl *lpVtbl; struct composition_brush *owner; } brush;
	struct { const composition_object1Vtbl *lpVtbl; struct composition_brush *owner; } object;
	LONG refs;
	struct compositor_state *compositor;
	ui_color color;
	HBRUSH fill;
};

static void *composition_alloc(size_t size)
{
	void *block = ALLOC(size);
	if (block)
		InterlockedIncrement(&raven_xaml_objects);
	return block;
}

static void compositor_dirty(struct compositor_state *compositor);

/* Compositor */

static ULONG compositor_release(struct compositor_state *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		/* Detach first: each target holds one compositor reference, and releasing it re-enters here. */
		struct composition_target *target = self->targets;
		self->targets = NULL;
		while (target) {
			struct composition_target *next = target->next;
			target->compositor = NULL;
			target->next = target->previous = NULL;
			target->first.lpVtbl->Release((target1 *)&target->first);
			target = next;
		}
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT compositor_query(struct compositor_state *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CUI_CComposition_CICompositor))
		*out = &self->first;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CUI_CComposition_CICompositionObject))
		*out = &self->object;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static HRESULT WINAPI compositor1_query(compositor1 *This, REFIID iid, void **out)
{
	return compositor_query((struct compositor_state *)((char *)This - offsetof(struct compositor_state, first)), iid, out);
}

static ULONG WINAPI compositor1_addref(compositor1 *This)
{
	return (ULONG)InterlockedIncrement(&((struct compositor_state *)((char *)This - offsetof(struct compositor_state, first)))->refs);
}

static ULONG WINAPI compositor1_release(compositor1 *This)
{
	return compositor_release((struct compositor_state *)((char *)This - offsetof(struct compositor_state, first)));
}

static HRESULT WINAPI compositor_object_query(composition_object1 *This, REFIID iid, void **out)
{
	return compositor_query((struct compositor_state *)((char *)This - offsetof(struct compositor_state, object)), iid, out);
}

static ULONG WINAPI compositor_object_addref(composition_object1 *This)
{
	return (ULONG)InterlockedIncrement(&((struct compositor_state *)((char *)This - offsetof(struct compositor_state, object)))->refs);
}

static ULONG WINAPI compositor_object_release(composition_object1 *This)
{
	return compositor_release((struct compositor_state *)((char *)This - offsetof(struct compositor_state, object)));
}

static HRESULT WINAPI compositor1_iids(compositor1 *This, ULONG *count, IID **iids)
{
	(void)This;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI compositor1_name(compositor1 *This, HSTRING *name)
{
	(void)This;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(compositor_class, (UINT32)lstrlenW(compositor_class), name);
}

static HRESULT WINAPI compositor1_trust(compositor1 *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}

static HRESULT WINAPI compositor_object_iids(composition_object1 *This, ULONG *count, IID **iids)
{
	(void)This;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI compositor_object_name(composition_object1 *This, HSTRING *name)
{
	(void)This;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(compositor_class, (UINT32)lstrlenW(compositor_class), name);
}

static HRESULT WINAPI compositor_object_trust(composition_object1 *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}

static HRESULT WINAPI compositor_get_compositor(composition_object1 *This, compositor1 **value)
{
	struct compositor_state *self = (struct compositor_state *)((char *)This - offsetof(struct compositor_state, object));
	if (!value)
		return E_POINTER;
	InterlockedIncrement(&self->refs);
	*value = (compositor1 *)&self->first;
	return S_OK;
}

static HRESULT WINAPI compositor_get_dispatcher(composition_object1 *This, __x_ABI_CWindows_CUI_CCore_CICoreDispatcher **value)
{
	(void)This;
	if (value)
		*value = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_get_properties(composition_object1 *This,
	__x_ABI_CWindows_CUI_CComposition_CICompositionPropertySet **value)
{
	(void)This;
	if (value)
		*value = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_start_animation(composition_object1 *This, HSTRING name,
	__x_ABI_CWindows_CUI_CComposition_CICompositionAnimation *animation)
{
	(void)This;
	(void)name;
	(void)animation;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_stop_animation(composition_object1 *This, HSTRING name)
{
	(void)This;
	(void)name;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_color_key_animation(compositor1 *This,
	__x_ABI_CWindows_CUI_CComposition_CIColorKeyFrameAnimation **result)
{
	(void)This;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_cubic_bezier(compositor1 *This, vector2 first, vector2 second,
	__x_ABI_CWindows_CUI_CComposition_CICubicBezierEasingFunction **result)
{
	(void)This;
	(void)first;
	(void)second;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_effect_factory(compositor1 *This,
	__x_ABI_CWindows_CGraphics_CEffects_CIGraphicsEffect *effect,
	__x_ABI_CWindows_CUI_CComposition_CICompositionEffectFactory **result)
{
	(void)This;
	(void)effect;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_effect_factory_with_properties(compositor1 *This,
	__x_ABI_CWindows_CGraphics_CEffects_CIGraphicsEffect *effect, __FIIterable_1_HSTRING *animatable,
	__x_ABI_CWindows_CUI_CComposition_CICompositionEffectFactory **result)
{
	(void)This;
	(void)effect;
	(void)animatable;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_expression_animation(compositor1 *This,
	__x_ABI_CWindows_CUI_CComposition_CIExpressionAnimation **result)
{
	(void)This;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_expression_with_expression(compositor1 *This, HSTRING expression,
	__x_ABI_CWindows_CUI_CComposition_CIExpressionAnimation **result)
{
	(void)This;
	(void)expression;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_inset_clip(compositor1 *This, __x_ABI_CWindows_CUI_CComposition_CIInsetClip **result)
{
	(void)This;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_inset_with_insets(compositor1 *This, float left, float top, float right,
	float bottom, __x_ABI_CWindows_CUI_CComposition_CIInsetClip **result)
{
	(void)This;
	(void)left;
	(void)top;
	(void)right;
	(void)bottom;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_linear_easing(compositor1 *This,
	__x_ABI_CWindows_CUI_CComposition_CILinearEasingFunction **result)
{
	(void)This;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_property_set(compositor1 *This,
	__x_ABI_CWindows_CUI_CComposition_CICompositionPropertySet **result)
{
	(void)This;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_quaternion_animation(compositor1 *This,
	__x_ABI_CWindows_CUI_CComposition_CIQuaternionKeyFrameAnimation **result)
{
	(void)This;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_scalar_animation(compositor1 *This,
	__x_ABI_CWindows_CUI_CComposition_CIScalarKeyFrameAnimation **result)
{
	(void)This;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_scoped_batch(compositor1 *This,
	__x_ABI_CWindows_CUI_CComposition_CCompositionBatchTypes type,
	__x_ABI_CWindows_CUI_CComposition_CICompositionScopedBatch **result)
{
	(void)This;
	(void)type;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_surface_brush(compositor1 *This,
	__x_ABI_CWindows_CUI_CComposition_CICompositionSurfaceBrush **result)
{
	(void)This;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_surface_brush_with_surface(compositor1 *This,
	__x_ABI_CWindows_CUI_CComposition_CICompositionSurface *surface,
	__x_ABI_CWindows_CUI_CComposition_CICompositionSurfaceBrush **result)
{
	(void)This;
	(void)surface;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_vector2_animation(compositor1 *This,
	__x_ABI_CWindows_CUI_CComposition_CIVector2KeyFrameAnimation **result)
{
	(void)This;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_vector3_animation(compositor1 *This,
	__x_ABI_CWindows_CUI_CComposition_CIVector3KeyFrameAnimation **result)
{
	(void)This;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_vector4_animation(compositor1 *This,
	__x_ABI_CWindows_CUI_CComposition_CIVector4KeyFrameAnimation **result)
{
	(void)This;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_get_commit_batch(compositor1 *This,
	__x_ABI_CWindows_CUI_CComposition_CCompositionBatchTypes type,
	__x_ABI_CWindows_CUI_CComposition_CICompositionCommitBatch **result)
{
	(void)This;
	(void)type;
	if (result)
		*result = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI compositor_create_color_brush(compositor1 *This, colorbrush1 **result);
static HRESULT WINAPI compositor_create_color_brush_with_color(compositor1 *This, ui_color color, colorbrush1 **result);
static HRESULT WINAPI compositor_create_container_visual(compositor1 *This, container1 **result);
static HRESULT WINAPI compositor_create_sprite_visual(compositor1 *This, sprite1 **result);
static HRESULT WINAPI compositor_create_target_for_current_view(compositor1 *This, target1 **result);

static const compositor1Vtbl compositor_first_table = {
	.QueryInterface = compositor1_query,
	.AddRef = compositor1_addref,
	.Release = compositor1_release,
	.GetIids = compositor1_iids,
	.GetRuntimeClassName = compositor1_name,
	.GetTrustLevel = compositor1_trust,
	.CreateColorKeyFrameAnimation = compositor_create_color_key_animation,
	.CreateColorBrush = compositor_create_color_brush,
	.CreateColorBrushWithColor = compositor_create_color_brush_with_color,
	.CreateContainerVisual = compositor_create_container_visual,
	.CreateCubicBezierEasingFunction = compositor_create_cubic_bezier,
	.CreateEffectFactory = compositor_create_effect_factory,
	.CreateEffectFactoryWithProperties = compositor_create_effect_factory_with_properties,
	.CreateExpressionAnimation = compositor_create_expression_animation,
	.CreateExpressionAnimationWithExpression = compositor_create_expression_with_expression,
	.CreateInsetClip = compositor_create_inset_clip,
	.CreateInsetClipWithInsets = compositor_create_inset_with_insets,
	.CreateLinearEasingFunction = compositor_create_linear_easing,
	.CreatePropertySet = compositor_create_property_set,
	.CreateQuaternionKeyFrameAnimation = compositor_create_quaternion_animation,
	.CreateScalarKeyFrameAnimation = compositor_create_scalar_animation,
	.CreateScopedBatch = compositor_create_scoped_batch,
	.CreateSpriteVisual = compositor_create_sprite_visual,
	.CreateSurfaceBrush = compositor_create_surface_brush,
	.CreateSurfaceBrushWithSurface = compositor_create_surface_brush_with_surface,
	.CreateTargetForCurrentView = compositor_create_target_for_current_view,
	.CreateVector2KeyFrameAnimation = compositor_create_vector2_animation,
	.CreateVector3KeyFrameAnimation = compositor_create_vector3_animation,
	.CreateVector4KeyFrameAnimation = compositor_create_vector4_animation,
	.GetCommitBatch = compositor_get_commit_batch,
};

static const composition_object1Vtbl compositor_object_table = {
	.QueryInterface = compositor_object_query,
	.AddRef = compositor_object_addref,
	.Release = compositor_object_release,
	.GetIids = compositor_object_iids,
	.GetRuntimeClassName = compositor_object_name,
	.GetTrustLevel = compositor_object_trust,
	.get_Compositor = compositor_get_compositor,
	.get_Dispatcher = compositor_get_dispatcher,
	.get_Properties = compositor_get_properties,
	.StartAnimation = compositor_start_animation,
	.StopAnimation = compositor_stop_animation,
};

/* Brushes */

static ULONG brush_release(struct composition_brush *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->fill)
			DeleteObject(self->fill);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT brush_query(struct composition_brush *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush))
		*out = &self->first;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CUI_CComposition_CICompositionBrush))
		*out = &self->brush;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CUI_CComposition_CICompositionObject))
		*out = &self->object;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static HRESULT WINAPI brush1_query(colorbrush1 *This, REFIID iid, void **out)
{
	return brush_query((struct composition_brush *)((char *)This - offsetof(struct composition_brush, first)), iid, out);
}

static ULONG WINAPI brush1_addref(colorbrush1 *This)
{
	return (ULONG)InterlockedIncrement(&((struct composition_brush *)((char *)This - offsetof(struct composition_brush, first)))->refs);
}

static ULONG WINAPI brush1_release(colorbrush1 *This)
{
	return brush_release((struct composition_brush *)((char *)This - offsetof(struct composition_brush, first)));
}

static HRESULT WINAPI brush_part_query(brush1 *This, REFIID iid, void **out)
{
	return brush_query((struct composition_brush *)((char *)This - offsetof(struct composition_brush, brush)), iid, out);
}

static ULONG WINAPI brush_part_addref(brush1 *This)
{
	return (ULONG)InterlockedIncrement(&((struct composition_brush *)((char *)This - offsetof(struct composition_brush, brush)))->refs);
}

static ULONG WINAPI brush_part_release(brush1 *This)
{
	return brush_release((struct composition_brush *)((char *)This - offsetof(struct composition_brush, brush)));
}

static HRESULT WINAPI brush_object_query(composition_object1 *This, REFIID iid, void **out)
{
	return brush_query((struct composition_brush *)((char *)This - offsetof(struct composition_brush, object)), iid, out);
}

static ULONG WINAPI brush_object_addref(composition_object1 *This)
{
	return (ULONG)InterlockedIncrement(&((struct composition_brush *)((char *)This - offsetof(struct composition_brush, object)))->refs);
}

static ULONG WINAPI brush_object_release(composition_object1 *This)
{
	return brush_release((struct composition_brush *)((char *)This - offsetof(struct composition_brush, object)));
}

static HRESULT WINAPI brush1_iids(colorbrush1 *This, ULONG *count, IID **iids)
{
	(void)This;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI brush1_name(colorbrush1 *This, HSTRING *name)
{
	(void)This;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(colorbrush_class, (UINT32)lstrlenW(colorbrush_class), name);
}

static HRESULT WINAPI brush1_trust(colorbrush1 *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}

static HRESULT WINAPI brush_part_iids(brush1 *This, ULONG *count, IID **iids)
{
	(void)This;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI brush_part_name(brush1 *This, HSTRING *name)
{
	(void)This;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(brush_class, (UINT32)lstrlenW(brush_class), name);
}

static HRESULT WINAPI brush_part_trust(brush1 *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}

static HRESULT WINAPI brush_object_iids(composition_object1 *This, ULONG *count, IID **iids)
{
	(void)This;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI brush_object_name(composition_object1 *This, HSTRING *name)
{
	(void)This;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(colorbrush_class, (UINT32)lstrlenW(colorbrush_class), name);
}

static HRESULT WINAPI brush_object_trust(composition_object1 *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}

static HRESULT WINAPI brush_get_color(colorbrush1 *This, ui_color *value)
{
	if (!value)
		return E_POINTER;
	*value = ((struct composition_brush *)This)->color;
	return S_OK;
}

static HRESULT WINAPI brush_put_color(colorbrush1 *This, ui_color value)
{
	struct composition_brush *self = (struct composition_brush *)This;
	self->color = value;
	if (self->fill) {
		DeleteObject(self->fill);
		self->fill = NULL;
	}
	compositor_dirty(self->compositor);
	return S_OK;
}

static HRESULT WINAPI brush_object_get_compositor(composition_object1 *This, compositor1 **value)
{
	struct composition_brush *self = (struct composition_brush *)((char *)This - offsetof(struct composition_brush, object));
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (!self->compositor)
		return S_OK;
	InterlockedIncrement(&self->compositor->refs);
	*value = (compositor1 *)&self->compositor->first;
	return S_OK;
}

static HRESULT WINAPI brush_object_get_dispatcher(composition_object1 *This, __x_ABI_CWindows_CUI_CCore_CICoreDispatcher **value)
{
	(void)This;
	if (value)
		*value = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI brush_object_get_properties(composition_object1 *This,
	__x_ABI_CWindows_CUI_CComposition_CICompositionPropertySet **value)
{
	(void)This;
	if (value)
		*value = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI brush_object_start_animation(composition_object1 *This, HSTRING name,
	__x_ABI_CWindows_CUI_CComposition_CICompositionAnimation *animation)
{
	(void)This;
	(void)name;
	(void)animation;
	return E_NOTIMPL;
}

static HRESULT WINAPI brush_object_stop_animation(composition_object1 *This, HSTRING name)
{
	(void)This;
	(void)name;
	return E_NOTIMPL;
}

static const colorbrush1Vtbl brush_first_table = {
	.QueryInterface = brush1_query,
	.AddRef = brush1_addref,
	.Release = brush1_release,
	.GetIids = brush1_iids,
	.GetRuntimeClassName = brush1_name,
	.GetTrustLevel = brush1_trust,
	.get_Color = brush_get_color,
	.put_Color = brush_put_color,
};

static const brush1Vtbl brush_brush_table = {
	.QueryInterface = brush_part_query,
	.AddRef = brush_part_addref,
	.Release = brush_part_release,
	.GetIids = brush_part_iids,
	.GetRuntimeClassName = brush_part_name,
	.GetTrustLevel = brush_part_trust,
};

static const composition_object1Vtbl brush_object_table = {
	.QueryInterface = brush_object_query,
	.AddRef = brush_object_addref,
	.Release = brush_object_release,
	.GetIids = brush_object_iids,
	.GetRuntimeClassName = brush_object_name,
	.GetTrustLevel = brush_object_trust,
	.get_Compositor = brush_object_get_compositor,
	.get_Dispatcher = brush_object_get_dispatcher,
	.get_Properties = brush_object_get_properties,
	.StartAnimation = brush_object_start_animation,
	.StopAnimation = brush_object_stop_animation,
};

static HBRUSH brush_fill(struct composition_brush *brush)
{
	if (!brush->fill)
		brush->fill = CreateSolidBrush(RGB(brush->color.R, brush->color.G, brush->color.B));
	return brush->fill;
}

/* Visuals */

static ULONG visual_release(struct composition_visual *self);

static HRESULT visual_query(struct composition_visual *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CUI_CComposition_CIVisual))
		*out = &self->first;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CUI_CComposition_CICompositionObject))
		*out = &self->object;
	else if (self->kind == composition_container &&
		equal_iid(iid, &IID___x_ABI_CWindows_CUI_CComposition_CIContainerVisual)) {
		struct composition_container *container = (struct composition_container *)
			((char *)self - offsetof(struct composition_container, base));
		*out = &container->container;
	} else if (self->kind == composition_sprite &&
		equal_iid(iid, &IID___x_ABI_CWindows_CUI_CComposition_CISpriteVisual)) {
		struct composition_sprite *sprite = (struct composition_sprite *)
			((char *)self - offsetof(struct composition_sprite, base));
		*out = &sprite->sprite;
	} else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static struct composition_visual *visual_of_part(void *part, size_t offset)
{
	return (struct composition_visual *)((char *)part - offset);
}

static HRESULT WINAPI visual1_query(visual1 *This, REFIID iid, void **out)
{
	return visual_query(visual_of_part(This, offsetof(struct composition_visual, first)), iid, out);
}

static ULONG WINAPI visual1_addref(visual1 *This)
{
	return (ULONG)InterlockedIncrement(&visual_of_part(This, offsetof(struct composition_visual, first))->refs);
}

static ULONG WINAPI visual1_release(visual1 *This)
{
	return visual_release(visual_of_part(This, offsetof(struct composition_visual, first)));
}

static HRESULT WINAPI visual_object_query(composition_object1 *This, REFIID iid, void **out)
{
	return visual_query(visual_of_part(This, offsetof(struct composition_visual, object)), iid, out);
}

static ULONG WINAPI visual_object_addref(composition_object1 *This)
{
	return (ULONG)InterlockedIncrement(&visual_of_part(This, offsetof(struct composition_visual, object))->refs);
}

static ULONG WINAPI visual_object_release(composition_object1 *This)
{
	return visual_release(visual_of_part(This, offsetof(struct composition_visual, object)));
}

static HRESULT WINAPI visual1_iids(visual1 *This, ULONG *count, IID **iids)
{
	(void)This;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI visual1_trust(visual1 *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}

static HRESULT WINAPI visual_name_for(struct composition_visual *self, HSTRING *name)
{
	const WCHAR *text = self->kind == composition_sprite ? sprite_class : container_class;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), name);
}

static HRESULT WINAPI container_visual_name(visual1 *This, HSTRING *name)
{
	(void)This;
	return WindowsCreateString(container_class, (UINT32)lstrlenW(container_class), name);
}

static HRESULT WINAPI sprite_visual_name(visual1 *This, HSTRING *name)
{
	(void)This;
	return WindowsCreateString(sprite_class, (UINT32)lstrlenW(sprite_class), name);
}

static HRESULT WINAPI visual_object_iids(composition_object1 *This, ULONG *count, IID **iids)
{
	(void)This;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI visual_object_name(composition_object1 *This, HSTRING *name)
{
	return visual_name_for(visual_of_part(This, offsetof(struct composition_visual, object)), name);
}

static HRESULT WINAPI visual_object_trust(composition_object1 *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}

static HRESULT WINAPI visual_object_get_compositor(composition_object1 *This, compositor1 **value)
{
	struct composition_visual *self = visual_of_part(This, offsetof(struct composition_visual, object));
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (!self->compositor)
		return S_OK;
	InterlockedIncrement(&self->compositor->refs);
	*value = (compositor1 *)&self->compositor->first;
	return S_OK;
}

static HRESULT WINAPI visual_object_get_dispatcher(composition_object1 *This, __x_ABI_CWindows_CUI_CCore_CICoreDispatcher **value)
{
	(void)This;
	if (value)
		*value = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI visual_object_get_properties(composition_object1 *This,
	__x_ABI_CWindows_CUI_CComposition_CICompositionPropertySet **value)
{
	(void)This;
	if (value)
		*value = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI visual_object_start_animation(composition_object1 *This, HSTRING name,
	__x_ABI_CWindows_CUI_CComposition_CICompositionAnimation *animation)
{
	(void)This;
	(void)name;
	(void)animation;
	return E_NOTIMPL;
}

static HRESULT WINAPI visual_object_stop_animation(composition_object1 *This, HSTRING name)
{
	(void)This;
	(void)name;
	return E_NOTIMPL;
}

static HRESULT WINAPI visual_get_anchor(visual1 *This, vector2 *value)
{
	(void)This;
	if (value) {
		value->X = 0;
		value->Y = 0;
	}
	return E_NOTIMPL;
}

IN_STUB(visual1, visual_put_anchor, vector2)
IN_STUB(visual1, visual_put_backface, __x_ABI_CWindows_CUI_CComposition_CCompositionBackfaceVisibility)
static HRESULT WINAPI visual_get_backface(visual1 *This,
	__x_ABI_CWindows_CUI_CComposition_CCompositionBackfaceVisibility *value)
{
	(void)This;
	if (value)
		*value = 0;
	return E_NOTIMPL;
}
IN_STUB(visual1, visual_put_border, __x_ABI_CWindows_CUI_CComposition_CCompositionBorderMode)
static HRESULT WINAPI visual_get_border(visual1 *This, __x_ABI_CWindows_CUI_CComposition_CCompositionBorderMode *value)
{
	(void)This;
	if (value)
		*value = 0;
	return E_NOTIMPL;
}
IN_STUB(visual1, visual_put_center, vector3)
static HRESULT WINAPI visual_get_center(visual1 *This, vector3 *value)
{
	(void)This;
	if (value) {
		value->X = 0;
		value->Y = 0;
		value->Z = 0;
	}
	return E_NOTIMPL;
}
static HRESULT WINAPI visual_get_clip(visual1 *This, __x_ABI_CWindows_CUI_CComposition_CICompositionClip **value)
{
	(void)This;
	if (value)
		*value = NULL;
	return E_NOTIMPL;
}
IN_STUB(visual1, visual_put_clip, __x_ABI_CWindows_CUI_CComposition_CICompositionClip *)
IN_STUB(visual1, visual_put_composite, __x_ABI_CWindows_CUI_CComposition_CCompositionCompositeMode)
static HRESULT WINAPI visual_get_composite(visual1 *This, __x_ABI_CWindows_CUI_CComposition_CCompositionCompositeMode *value)
{
	(void)This;
	if (value)
		*value = 0;
	return E_NOTIMPL;
}

static HRESULT WINAPI visual_get_visible(visual1 *This, boolean *value)
{
	if (!value)
		return E_POINTER;
	*value = visual_of_part(This, offsetof(struct composition_visual, first))->visible;
	return S_OK;
}

static HRESULT WINAPI visual_put_visible(visual1 *This, boolean value)
{
	struct composition_visual *self = visual_of_part(This, offsetof(struct composition_visual, first));
	self->visible = value;
	compositor_dirty(self->compositor);
	return S_OK;
}

static HRESULT WINAPI visual_get_offset(visual1 *This, vector3 *value)
{
	if (!value)
		return E_POINTER;
	*value = visual_of_part(This, offsetof(struct composition_visual, first))->offset;
	return S_OK;
}

static HRESULT WINAPI visual_put_offset(visual1 *This, vector3 value)
{
	struct composition_visual *self = visual_of_part(This, offsetof(struct composition_visual, first));
	self->offset = value;
	compositor_dirty(self->compositor);
	return S_OK;
}

static HRESULT WINAPI visual_get_opacity(visual1 *This, float *value)
{
	if (!value)
		return E_POINTER;
	*value = visual_of_part(This, offsetof(struct composition_visual, first))->opacity;
	return S_OK;
}

static HRESULT WINAPI visual_put_opacity(visual1 *This, float value)
{
	struct composition_visual *self = visual_of_part(This, offsetof(struct composition_visual, first));
	self->opacity = value;
	compositor_dirty(self->compositor);
	return S_OK;
}

IN_STUB(visual1, visual_put_orientation, __x_ABI_CWindows_CFoundation_CNumerics_CQuaternion)
static HRESULT WINAPI visual_get_orientation(visual1 *This, __x_ABI_CWindows_CFoundation_CNumerics_CQuaternion *value)
{
	(void)This;
	if (value) {
		value->X = 0;
		value->Y = 0;
		value->Z = 0;
		value->W = 0;
	}
	return E_NOTIMPL;
}

static HRESULT WINAPI visual_get_parent(visual1 *This, container1 **value)
{
	struct composition_visual *self = visual_of_part(This, offsetof(struct composition_visual, first));
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (!self->parent)
		return S_OK;
	InterlockedIncrement(&self->parent->base.refs);
	*value = (container1 *)&self->parent->container;
	return S_OK;
}

IN_STUB(visual1, visual_put_rotation, float)
static HRESULT WINAPI visual_get_rotation(visual1 *This, float *value)
{
	(void)This;
	if (value)
		*value = 0;
	return E_NOTIMPL;
}
IN_STUB(visual1, visual_put_rotation_degrees, float)
static HRESULT WINAPI visual_get_rotation_degrees(visual1 *This, float *value)
{
	(void)This;
	if (value)
		*value = 0;
	return E_NOTIMPL;
}
IN_STUB(visual1, visual_put_rotation_axis, vector3)
static HRESULT WINAPI visual_get_rotation_axis(visual1 *This, vector3 *value)
{
	(void)This;
	if (value) {
		value->X = 0;
		value->Y = 0;
		value->Z = 0;
	}
	return E_NOTIMPL;
}
IN_STUB(visual1, visual_put_scale, vector3)
static HRESULT WINAPI visual_get_scale(visual1 *This, vector3 *value)
{
	(void)This;
	if (value) {
		value->X = 1;
		value->Y = 1;
		value->Z = 1;
	}
	return E_NOTIMPL;
}

static HRESULT WINAPI visual_get_size(visual1 *This, vector2 *value)
{
	if (!value)
		return E_POINTER;
	*value = visual_of_part(This, offsetof(struct composition_visual, first))->size;
	return S_OK;
}

static HRESULT WINAPI visual_put_size(visual1 *This, vector2 value)
{
	struct composition_visual *self = visual_of_part(This, offsetof(struct composition_visual, first));
	self->size = value;
	compositor_dirty(self->compositor);
	return S_OK;
}

IN_STUB(visual1, visual_put_transform, __x_ABI_CWindows_CFoundation_CNumerics_CMatrix4x4)
static HRESULT WINAPI visual_get_transform(visual1 *This, __x_ABI_CWindows_CFoundation_CNumerics_CMatrix4x4 *value)
{
	(void)This;
	if (value)
		ZeroMemory(value, sizeof *value);
	return E_NOTIMPL;
}

#define VISUAL_TABLE(prefix, class_name_slot) \
	static const visual1Vtbl prefix##_table = { \
		.QueryInterface = visual1_query, \
		.AddRef = visual1_addref, \
		.Release = visual1_release, \
		.GetIids = visual1_iids, \
		.GetRuntimeClassName = class_name_slot, \
		.GetTrustLevel = visual1_trust, \
		.get_AnchorPoint = visual_get_anchor, \
		.put_AnchorPoint = visual_put_anchor, \
		.get_BackfaceVisibility = visual_get_backface, \
		.put_BackfaceVisibility = visual_put_backface, \
		.get_BorderMode = visual_get_border, \
		.put_BorderMode = visual_put_border, \
		.get_CenterPoint = visual_get_center, \
		.put_CenterPoint = visual_put_center, \
		.get_Clip = visual_get_clip, \
		.put_Clip = visual_put_clip, \
		.get_CompositeMode = visual_get_composite, \
		.put_CompositeMode = visual_put_composite, \
		.get_IsVisible = visual_get_visible, \
		.put_IsVisible = visual_put_visible, \
		.get_Offset = visual_get_offset, \
		.put_Offset = visual_put_offset, \
		.get_Opacity = visual_get_opacity, \
		.put_Opacity = visual_put_opacity, \
		.get_Orientation = visual_get_orientation, \
		.put_Orientation = visual_put_orientation, \
		.get_Parent = visual_get_parent, \
		.get_RotationAngle = visual_get_rotation, \
		.put_RotationAngle = visual_put_rotation, \
		.get_RotationAngleInDegrees = visual_get_rotation_degrees, \
		.put_RotationAngleInDegrees = visual_put_rotation_degrees, \
		.get_RotationAxis = visual_get_rotation_axis, \
		.put_RotationAxis = visual_put_rotation_axis, \
		.get_Scale = visual_get_scale, \
		.put_Scale = visual_put_scale, \
		.get_Size = visual_get_size, \
		.put_Size = visual_put_size, \
		.get_TransformMatrix = visual_get_transform, \
		.put_TransformMatrix = visual_put_transform, \
	};

VISUAL_TABLE(container_visual, container_visual_name)
VISUAL_TABLE(sprite_visual, sprite_visual_name)

static const composition_object1Vtbl visual_object_table = {
	.QueryInterface = visual_object_query,
	.AddRef = visual_object_addref,
	.Release = visual_object_release,
	.GetIids = visual_object_iids,
	.GetRuntimeClassName = visual_object_name,
	.GetTrustLevel = visual_object_trust,
	.get_Compositor = visual_object_get_compositor,
	.get_Dispatcher = visual_object_get_dispatcher,
	.get_Properties = visual_object_get_properties,
	.StartAnimation = visual_object_start_animation,
	.StopAnimation = visual_object_stop_animation,
};

static ULONG visual_release(struct composition_visual *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->kind == composition_sprite) {
			struct composition_sprite *sprite = (struct composition_sprite *)
				((char *)self - offsetof(struct composition_sprite, base));
			if (sprite->brush)
				sprite->brush->lpVtbl->Release(sprite->brush);
			/* The payload copy dies with the sprite: a re-render replaces the whole tree, so no payload outlives
			 * the visual that drew it and none is shared between sprites. */
			if (sprite->text) {
				HeapFree(GetProcessHeap(), 0, sprite->text);
				sprite->text = NULL;
			}
		} else {
			struct composition_container *container = (struct composition_container *)
				((char *)self - offsetof(struct composition_container, base));
			if (container->children)
				container->children->lpVtbl->Release(container->children);
		}
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

/* The children collection */

static HRESULT children_query(struct children_vector *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID___x_ABI_CWindows_CUI_CComposition_CIVisualCollection))
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	*out = self;
	return S_OK;
}

static HRESULT WINAPI children_query_iface(struct children_vector *This, REFIID iid, void **out)
{
	return children_query(This, iid, out);
}

static ULONG WINAPI children_addref(struct children_vector *This)
{
	return (ULONG)InterlockedIncrement(&This->refs);
}

static ULONG WINAPI children_release_iface(struct children_vector *This)
{
	const LONG refs = InterlockedDecrement(&This->refs);
	if (!refs) {
		for (UINT32 index = 0; index < This->count; ++index)
			This->items[index]->lpVtbl->Release(This->items[index]);
		if (This->items)
			FREE(This->items);
		FREE(This);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI children_iids(struct children_vector *This, ULONG *count, IID **iids)
{
	(void)This;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI children_name(struct children_vector *This, HSTRING *name)
{
	(void)This;
	if (!name)
		return E_POINTER;
	static const WCHAR class_name[] = L"Windows.UI.Composition.VisualCollection";
	return WindowsCreateString(class_name, (UINT32)lstrlenW(class_name), name);
}

static HRESULT WINAPI children_trust(struct children_vector *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}

static HRESULT WINAPI children_count(struct children_vector *This, INT32 *value)
{
	if (!value)
		return E_POINTER;
	*value = (INT32)This->count;
	return S_OK;
}

static HRESULT children_add(struct children_vector *self, visual1 *value, UINT32 position)
{
	struct composition_container *container = self->owner;
	if (!value)
		return E_POINTER;
	if (position > self->count)
		position = self->count;
	if (self->count == self->capacity) {
		const UINT32 capacity = self->capacity ? self->capacity * 2 : 4;
		visual1 **items = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size_t)capacity * sizeof *items);
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
	/* A visual lives in one tree: re-adding it moves it, the way a real compositor reparents. */
	struct composition_visual *child = visual_of_part(value, offsetof(struct composition_visual, first));
	if (child->parent && child->parent != container) {
		struct children_vector *previous = child->parent->children;
		for (UINT32 index = 0; index < previous->count; ++index) {
			if (previous->items[index] == value) {
				for (UINT32 rest = index; rest + 1 < previous->count; ++rest)
					previous->items[rest] = previous->items[rest + 1];
				--previous->count;
				break;
			}
		}
		value->lpVtbl->Release(value);
	}
	for (UINT32 index = self->count; index > position; --index)
		self->items[index] = self->items[index - 1];
	self->items[position] = value;
	value->lpVtbl->AddRef(value);
	++self->count;
	child->parent = container;
	compositor_dirty(container->base.compositor);
	return S_OK;
}

static HRESULT WINAPI children_insert_above(struct children_vector *This, visual1 *child, visual1 *sibling)
{
	UINT32 position = This->count;
	for (UINT32 index = 0; index < This->count; ++index) {
		if (This->items[index] == sibling) {
			position = index + 1;
			break;
		}
	}
	return children_add(This, child, position);
}

static HRESULT WINAPI children_insert_at_bottom(struct children_vector *This, visual1 *child)
{
	return children_add(This, child, 0);
}

static HRESULT WINAPI children_insert_at_top(struct children_vector *This, visual1 *child)
{
	return children_add(This, child, This->count);
}

static HRESULT WINAPI children_insert_below(struct children_vector *This, visual1 *child, visual1 *sibling)
{
	UINT32 position = 0;
	for (UINT32 index = 0; index < This->count; ++index) {
		if (This->items[index] == sibling) {
			position = index;
			break;
		}
	}
	return children_add(This, child, position);
}

static HRESULT WINAPI children_remove(struct children_vector *This, visual1 *child)
{
	for (UINT32 index = 0; index < This->count; ++index) {
		if (This->items[index] == child) {
			visual_of_part(child, offsetof(struct composition_visual, first))->parent = NULL;
			for (UINT32 rest = index; rest + 1 < This->count; ++rest)
				This->items[rest] = This->items[rest + 1];
			--This->count;
			child->lpVtbl->Release(child);
			compositor_dirty(This->owner->base.compositor);
			return S_OK;
		}
	}
	return E_INVALIDARG;
}

static HRESULT WINAPI children_remove_all(struct children_vector *This)
{
	for (UINT32 index = 0; index < This->count; ++index) {
		visual_of_part(This->items[index], offsetof(struct composition_visual, first))->parent = NULL;
		This->items[index]->lpVtbl->Release(This->items[index]);
	}
	This->count = 0;
	compositor_dirty(This->owner->base.compositor);
	return S_OK;
}

static const struct children_vector_vtbl children_table = {
	.QueryInterface = children_query_iface,
	.AddRef = children_addref,
	.Release = children_release_iface,
	.GetIids = children_iids,
	.GetRuntimeClassName = children_name,
	.GetTrustLevel = children_trust,
	.get_Count = children_count,
	.InsertAbove = children_insert_above,
	.InsertAtBottom = children_insert_at_bottom,
	.InsertAtTop = children_insert_at_top,
	.InsertBelow = children_insert_below,
	.Remove = children_remove,
	.RemoveAll = children_remove_all,
};

/* Container and sprite parts */

static HRESULT WINAPI container_get_children(container1 *This,
	__x_ABI_CWindows_CUI_CComposition_CIVisualCollection **value)
{
	struct composition_container *self = (struct composition_container *)
		((char *)This - offsetof(struct composition_container, container));
	if (!value)
		return E_POINTER;
	InterlockedIncrement(&self->children->refs);
	*value = (__x_ABI_CWindows_CUI_CComposition_CIVisualCollection *)self->children;
	return S_OK;
}

static HRESULT WINAPI sprite_get_brush(sprite1 *This, brush1 **value)
{
	struct composition_sprite *self = (struct composition_sprite *)
		((char *)This - offsetof(struct composition_sprite, sprite));
	if (!value)
		return E_POINTER;
	*value = self->brush;
	if (self->brush)
		self->brush->lpVtbl->AddRef(self->brush);
	return S_OK;
}

static HRESULT WINAPI sprite_put_brush(sprite1 *This, brush1 *value)
{
	struct composition_sprite *self = (struct composition_sprite *)
		((char *)This - offsetof(struct composition_sprite, sprite));
	struct composition_brush *brush_data = NULL;
	if (value) {
		colorbrush1 *color = NULL;
		if (FAILED(value->lpVtbl->QueryInterface(value, &IID___x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush,
			(void **)&color)) || !color)
			return E_NOINTERFACE;
		brush_data = (struct composition_brush *)color;
		color->lpVtbl->Release(color);
	}
	if (self->brush)
		self->brush->lpVtbl->Release(self->brush);
	self->brush = value;
	if (value)
		value->lpVtbl->AddRef(value);
	self->brush_data = brush_data;
	compositor_dirty(self->base.compositor);
	return S_OK;
}

/* Per-kind QI and refcounting for the container and sprite parts. */

#define KIND_PART(part, prefix, struct_type, member) \
	static HRESULT WINAPI prefix##_query(part *This, REFIID iid, void **out) \
	{ \
		struct struct_type *self = (struct struct_type *)((char *)This - offsetof(struct struct_type, member)); \
		return visual_query(&self->base, iid, out); \
	} \
	static ULONG WINAPI prefix##_addref(part *This) \
	{ \
		struct struct_type *self = (struct struct_type *)((char *)This - offsetof(struct struct_type, member)); \
		return (ULONG)InterlockedIncrement(&self->base.refs); \
	} \
	static ULONG WINAPI prefix##_release(part *This) \
	{ \
		struct struct_type *self = (struct struct_type *)((char *)This - offsetof(struct struct_type, member)); \
		return visual_release(&self->base); \
	} \
	static HRESULT WINAPI prefix##_iids(part *This, ULONG *count, IID **iids) \
	{ (void)This; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI prefix##_name(part *This, HSTRING *name) \
	{ (void)This; return visual_name_for(&((struct struct_type *)((char *)This - \
		offsetof(struct struct_type, member)))->base, name); } \
	static HRESULT WINAPI prefix##_trust(part *This, TrustLevel *level) \
	{ (void)This; return inspectable_trust(level); }

KIND_PART(container1, container_part, composition_container, container)
KIND_PART(sprite1, sprite_part, composition_sprite, sprite)

static const container1Vtbl container_part_full = {
	.QueryInterface = container_part_query,
	.AddRef = container_part_addref,
	.Release = container_part_release,
	.GetIids = container_part_iids,
	.GetRuntimeClassName = container_part_name,
	.GetTrustLevel = container_part_trust,
	.get_Children = container_get_children,
};

static const sprite1Vtbl sprite_part_full = {
	.QueryInterface = sprite_part_query,
	.AddRef = sprite_part_addref,
	.Release = sprite_part_release,
	.GetIids = sprite_part_iids,
	.GetRuntimeClassName = sprite_part_name,
	.GetTrustLevel = sprite_part_trust,
	.get_Brush = sprite_get_brush,
	.put_Brush = sprite_put_brush,
};

/* Targets */

static ULONG target_release(struct composition_target *self);

static HRESULT target_query(struct composition_target *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CUI_CComposition_CICompositionTarget))
		*out = &self->first;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CUI_CComposition_CICompositionObject))
		*out = &self->object;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static HRESULT WINAPI target1_query(target1 *This, REFIID iid, void **out)
{
	return target_query((struct composition_target *)((char *)This - offsetof(struct composition_target, first)), iid, out);
}

static ULONG WINAPI target1_addref(target1 *This)
{
	return (ULONG)InterlockedIncrement(&((struct composition_target *)((char *)This - offsetof(struct composition_target, first)))->refs);
}

static ULONG WINAPI target1_release(target1 *This)
{
	return target_release((struct composition_target *)((char *)This - offsetof(struct composition_target, first)));
}

static HRESULT WINAPI target_object_query(composition_object1 *This, REFIID iid, void **out)
{
	return target_query((struct composition_target *)((char *)This - offsetof(struct composition_target, object)), iid, out);
}

static ULONG WINAPI target_object_addref(composition_object1 *This)
{
	return (ULONG)InterlockedIncrement(&((struct composition_target *)((char *)This - offsetof(struct composition_target, object)))->refs);
}

static ULONG WINAPI target_object_release(composition_object1 *This)
{
	return target_release((struct composition_target *)((char *)This - offsetof(struct composition_target, object)));
}

static HRESULT WINAPI target1_iids(target1 *This, ULONG *count, IID **iids)
{
	(void)This;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI target1_name(target1 *This, HSTRING *name)
{
	(void)This;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(target_class, (UINT32)lstrlenW(target_class), name);
}

static HRESULT WINAPI target1_trust(target1 *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}

static HRESULT WINAPI target_object_iids(composition_object1 *This, ULONG *count, IID **iids)
{
	(void)This;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI target_object_name(composition_object1 *This, HSTRING *name)
{
	(void)This;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(target_class, (UINT32)lstrlenW(target_class), name);
}

static HRESULT WINAPI target_object_trust(composition_object1 *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}

static HRESULT WINAPI target_get_root(target1 *This, visual1 **value)
{
	struct composition_target *self = (struct composition_target *)((char *)This - offsetof(struct composition_target, first));
	if (!value)
		return E_POINTER;
	*value = self->root;
	if (self->root)
		self->root->lpVtbl->AddRef(self->root);
	return S_OK;
}

static HRESULT WINAPI target_put_root(target1 *This, visual1 *value)
{
	struct composition_target *self = (struct composition_target *)((char *)This - offsetof(struct composition_target, first));
	struct composition_visual *data = NULL;
	if (value) {
		visual1 *part = NULL;
		if (FAILED(value->lpVtbl->QueryInterface(value, &IID___x_ABI_CWindows_CUI_CComposition_CIVisual, (void **)&part)) || !part)
			return E_NOINTERFACE;
		data = visual_of_part(part, offsetof(struct composition_visual, first));
		part->lpVtbl->Release(part);
	}
	if (self->root)
		self->root->lpVtbl->Release(self->root);
	self->root = value;
	if (value)
		value->lpVtbl->AddRef(value);
	self->root_data = data;
	compositor_dirty(self->compositor);
	return S_OK;
}

static HRESULT WINAPI target_object_get_compositor(composition_object1 *This, compositor1 **value)
{
	struct composition_target *self = (struct composition_target *)((char *)This - offsetof(struct composition_target, object));
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (!self->compositor)
		return S_OK;
	InterlockedIncrement(&self->compositor->refs);
	*value = (compositor1 *)&self->compositor->first;
	return S_OK;
}

static HRESULT WINAPI target_object_get_dispatcher(composition_object1 *This, __x_ABI_CWindows_CUI_CCore_CICoreDispatcher **value)
{
	(void)This;
	if (value)
		*value = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI target_object_get_properties(composition_object1 *This,
	__x_ABI_CWindows_CUI_CComposition_CICompositionPropertySet **value)
{
	(void)This;
	if (value)
		*value = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI target_object_start_animation(composition_object1 *This, HSTRING name,
	__x_ABI_CWindows_CUI_CComposition_CICompositionAnimation *animation)
{
	(void)This;
	(void)name;
	(void)animation;
	return E_NOTIMPL;
}

static HRESULT WINAPI target_object_stop_animation(composition_object1 *This, HSTRING name)
{
	(void)This;
	(void)name;
	return E_NOTIMPL;
}

static const target1Vtbl target_first_table = {
	.QueryInterface = target1_query,
	.AddRef = target1_addref,
	.Release = target1_release,
	.GetIids = target1_iids,
	.GetRuntimeClassName = target1_name,
	.GetTrustLevel = target1_trust,
	.get_Root = target_get_root,
	.put_Root = target_put_root,
};

static const composition_object1Vtbl target_object_table = {
	.QueryInterface = target_object_query,
	.AddRef = target_object_addref,
	.Release = target_object_release,
	.GetIids = target_object_iids,
	.GetRuntimeClassName = target_object_name,
	.GetTrustLevel = target_object_trust,
	.get_Compositor = target_object_get_compositor,
	.get_Dispatcher = target_object_get_dispatcher,
	.get_Properties = target_object_get_properties,
	.StartAnimation = target_object_start_animation,
	.StopAnimation = target_object_stop_animation,
};

/* The live targets, for presentation on window size changes. */

static struct composition_target *live_targets;
static CRITICAL_SECTION targets_lock;
static LONG targets_lock_ready;

static void targets_attach(struct composition_target *target)
{
	if (InterlockedCompareExchange(&targets_lock_ready, 0, 0) != 2)
		return;
	EnterCriticalSection(&targets_lock);
	target->next = live_targets;
	target->previous = NULL;
	if (live_targets)
		live_targets->previous = target;
	live_targets = target;
	LeaveCriticalSection(&targets_lock);
}

static void targets_detach(struct composition_target *target)
{
	if (InterlockedCompareExchange(&targets_lock_ready, 0, 0) != 2)
		return;
	EnterCriticalSection(&targets_lock);
	if (target->previous)
		target->previous->next = target->next;
	else
		live_targets = target->next;
	if (target->next)
		target->next->previous = target->previous;
	target->next = target->previous = NULL;
	LeaveCriticalSection(&targets_lock);
}

static ULONG target_release(struct composition_target *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		targets_detach(self);
		if (self->root)
			self->root->lpVtbl->Release(self->root);
		if (self->window)
			self->window->lpVtbl->Release(self->window);
		/* Unlink from the compositor's own list; the compositor holds one reference per target. */
		struct compositor_state *compositor = self->compositor;
		if (compositor) {
			self->compositor = NULL;
			struct composition_target **link = &compositor->targets;
			while (*link && *link != self)
				link = &(*link)->next;
			if (*link)
				*link = self->next;
			compositor_release(compositor);
		}
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI compositor_create_color_brush(compositor1 *This, colorbrush1 **result)
{
	struct compositor_state *self = (struct compositor_state *)((char *)This - offsetof(struct compositor_state, first));
	if (!result)
		return E_POINTER;
	*result = NULL;
	struct composition_brush *brush = composition_alloc(sizeof *brush);
	if (!brush)
		return E_OUTOFMEMORY;
	brush->first.lpVtbl = &brush_first_table;
	brush->brush.lpVtbl = &brush_brush_table;
	brush->object.lpVtbl = &brush_object_table;
	brush->refs = 1;
	brush->compositor = self;
	*result = (colorbrush1 *)&brush->first;
	return S_OK;
}

static HRESULT WINAPI compositor_create_color_brush_with_color(compositor1 *This, ui_color color, colorbrush1 **result)
{
	colorbrush1 *brush = NULL;
	const HRESULT result_status = compositor_create_color_brush(This, &brush);
	if (FAILED(result_status))
		return result_status;
	brush->lpVtbl->put_Color(brush, color);
	*result = brush;
	return S_OK;
}

static HRESULT WINAPI compositor_create_container_visual(compositor1 *This, container1 **result);

static HRESULT WINAPI compositor_create_sprite_visual(compositor1 *This, sprite1 **result);

static HRESULT WINAPI compositor_create_target_for_current_view(compositor1 *This, target1 **result);

/* Presentation */

/* The text drawing of the CPU fallback.
 *
 * The payload draws through the host's font service — the RavenFont* exports the guest imports as ravenhost.dll — because that
 * is the one text path the emulated surface serves in every configuration: the GDI text providers exist only where the Skia
 * text build is compiled in, and an API the surface does not serve stops the guest instead of failing the call. The service
 * rasterizes one glyph at a time into byte-per-pixel monochrome coverage (mode 0), which keeps the glyphs deterministic for the
 * pixel fixtures the way a NONANTIALIASED quality would, and the composite lands on the window surface through the DIB blits
 * the raster stage serves. The fallback maps one DIP to one pixel at its 96 DPI, puts the run's top at the sprite's origin and
 * the baseline where the face's ascent puts it — the top the GDI character-height convention would place. The face is the
 * first of the preferred families the host font directories carry, Segoe UI (the UWP default) first. The run walks UTF-16
 * units; surrogate pairs, shaping and measuring are the text stage's work, and the composite is bounded by the sprite's
 * rectangle, which is the layout's bound for the block. On a surface without the font service the text paints nothing rather
 * than stopping the guest, and the opened face lives for the process, the way the compositor's own objects do. */

typedef DWORD (WINAPI *font_scan_function)(void *buffer, DWORD capacity);
typedef DWORD (WINAPI *font_open_function)(const void *data, DWORD size, DWORD face_index);
typedef DWORD (WINAPI *font_info_function)(DWORD font, INT32 *info);
typedef DWORD (WINAPI *font_glyphs_function)(DWORD font, const UINT32 *codepoints, DWORD count, UINT16 *glyphs);
typedef DWORD (WINAPI *font_metrics_function)(DWORD font, DWORD glyph, INT32 *out);
typedef DWORD (WINAPI *font_render_function)(DWORD font, DWORD glyph, INT32 em16, const INT32 *matrix, DWORD mode,
	void *buffer, DWORD capacity, INT32 *bounds, INT32 shift_x16, INT32 shift_y16);

static struct {
	LONG state;                 /* 0 unresolved, 1 unavailable, 2 ready: the presenters are the UI threads, so the resolution
	                               is single-shot the way the hosted root list of the render stage is */
	font_scan_function scan;
	font_open_function open;
	font_info_function info;
	font_glyphs_function glyphs;
	font_metrics_function metrics;
	font_render_function render;
	DWORD font;
	INT32 units_per_em, ascent, descent;
} text_fonts;

/* The faces the fallback prefers, in order; whatever else the scan returns serves when none of them is there. */
static const char *const text_font_families[] = {"Segoe UI", "Liberation Sans", "DejaVu Sans"};

static DWORD record_u32(const BYTE *at)
{
	return (DWORD)at[0] | (DWORD)at[1] << 8 | (DWORD)at[2] << 16 | (DWORD)at[3] << 24;
}

static size_t record_length(const char *text)
{
	size_t length = 0;
	while (text[length])
		++length;
	return length;
}

static int record_same(const char *left, const char *right)
{
	size_t at = 0;
	while (left[at] && left[at] == right[at])
		++at;
	return !left[at] && !right[at];
}

/* The scan records read: sizes, weight, style, width, flags, then the path, family, face, full and PostScript names. The
 * pick takes the best preferred family the records carry, else the first record at all. */
static int text_face_pick(const BYTE *records, DWORD size, const char **path, UINT32 *face_index)
{
	const char *first_path = NULL;
	UINT32 first_face = 0;
	int best = -1;
	for (DWORD at = 0; at + 24 <= size;) {
		const DWORD stride = record_u32(records + at);
		if (stride < 24 || stride > size - at)
			break;
		const char *walk = (const char *)records + at + 24;
		const char *record_path = walk;
		walk += record_length(walk) + 1;
		const char *record_family = walk;
		if (!first_path) {
			first_path = record_path;
			first_face = record_u32(records + at + 4);
		}
		for (size_t index = 0; index < ARRAYSIZE(text_font_families) && (best < 0 || (int)index < best); ++index)
			if (record_same(record_family, text_font_families[index])) {
				best = (int)index;
				*path = record_path;
				*face_index = record_u32(records + at + 4);
				break;
			}
		if (!best)
			return 1;
		at += stride;
	}
	if (best < 0 && first_path) {
		*path = first_path;
		*face_index = first_face;
		return 1;
	}
	return best >= 0;
}

/* The font file the record names is read and handed to the service whole. The scan carries the host file path, and the
 * guest file API reaches it through the Z: drive that maps the host root, with backslashes the way Windows reports the
 * path there. */
static void text_face_open(const char *path, UINT32 face_index)
{
	WCHAR wide[MAX_PATH];
	size_t length = 2;
	if (!path[0] || (path[0] == '/' && !path[1]))
		return;
	wide[0] = 'Z';
	wide[1] = ':';
	for (size_t at = 0; path[at] && length < ARRAYSIZE(wide) - 1; ++at)
		wide[length++] = path[at] == '/' ? '\\' : (WCHAR)(BYTE)path[at];
	wide[length] = 0;
	HANDLE file = CreateFileW(wide, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return;
	const DWORD size = GetFileSize(file, NULL);
	BYTE *data = size && size != INVALID_FILE_SIZE && size < (1u << 24) ? ALLOC(size) : NULL;
	DWORD read = 0;
	if (data && ReadFile(file, data, size, &read, NULL) && read == size) {
		text_fonts.font = text_fonts.open(data, size, face_index);
		INT32 info[20];
		if (text_fonts.font && text_fonts.info(text_fonts.font, info)) {
			text_fonts.units_per_em = info[0];
			text_fonts.ascent = info[1];
			text_fonts.descent = info[2];
		}
	}
	if (data)
		FREE(data);
	CloseHandle(file);
}

static void text_fonts_init(void)
{
	if (InterlockedCompareExchange(&text_fonts.state, 1, 0) != 0)
		return;
	HMODULE host = LoadLibraryW(L"ravenhost.dll");
	if (host) {
		text_fonts.scan = (font_scan_function)GetProcAddress(host, "RavenFontScan");
		text_fonts.open = (font_open_function)GetProcAddress(host, "RavenFontOpen");
		text_fonts.info = (font_info_function)GetProcAddress(host, "RavenFontInfo");
		text_fonts.glyphs = (font_glyphs_function)GetProcAddress(host, "RavenFontGlyphs");
		text_fonts.metrics = (font_metrics_function)GetProcAddress(host, "RavenFontMetrics");
		text_fonts.render = (font_render_function)GetProcAddress(host, "RavenFontRender");
	}
	const DWORD size = text_fonts.scan ? text_fonts.scan(NULL, 0) : 0;
	BYTE *records = size && size < (1u << 22) ? ALLOC(size) : NULL;
	if (records && text_fonts.scan(records, size) == size) {
		const char *path = NULL;
		UINT32 face_index = 0;
		if (text_face_pick(records, size, &path, &face_index))
			text_face_open(path, face_index);
	}
	if (records)
		FREE(records);
	if (!text_fonts.font || !text_fonts.units_per_em)
		return;
	text_fonts.state = 2;
}

/* The extent the paint would give this run: the sum of the glyph advances and the ascent+descent, in DIPs. The measure stage
 * sizes Auto tracks with it; no clamping, and an unresolvable font or empty run reports zero. */
HRESULT raven_composition_text_extent(const WCHAR *text, UINT32 length, float font_size, float scale,
	float *width, float *height)
{
	if (!width || !height)
		return E_POINTER;
	*width = 0;
	*height = 0;
	if (!text || !length)
		return S_OK;
	text_fonts_init();
	if (text_fonts.state != 2 || !text_fonts.font)
		return S_OK;
	const int px = (int)(font_size * scale + 0.5f);
	if (px <= 0 || px > 4096)
		return S_OK;
	const float design_scale = (float)px / (float)text_fonts.units_per_em;
	float run_width = 0;
	for (UINT32 index = 0; index < length; ++index) {
		const UINT32 codepoint = text[index];
		UINT16 glyph = 0;
		INT32 metrics[7];
		if (!text_fonts.glyphs(text_fonts.font, &codepoint, 1, &glyph) ||
			!text_fonts.metrics(text_fonts.font, glyph, metrics))
			return S_OK;
		run_width += metrics[1] * design_scale;
	}
	const float run_height = (text_fonts.ascent + text_fonts.descent) * design_scale;
	*width = run_width / scale;
	*height = run_height / scale;
	return S_OK;
}

/* The run rasterizes into one 32-bit DIB the destination is read into and written back over, the way a mask blit would
 * composite; the glyph pixels the sprite bound does not carry stay where they were. */
static void paint_sprite_text(HDC dc, struct composition_sprite *sprite, float x, float y, float width, float height,
	float scale)
{
	text_fonts_init();
	if (text_fonts.state != 2 || !text_fonts.font)
		return;
	/* The em size is the pixel size the 16.16 fixed point of the service wants; the bound keeps a runaway size from
	 * reaching the shift or the allocation. */
	const int px = (int)(sprite->text_font_size * scale + 0.5f);
	if (px <= 0 || px > 4096)
		return;
	const float design_scale = (float)px / (float)text_fonts.units_per_em;
	const int sprite_width = width > 0 ? (int)(width * scale + 0.5f) : 0;
	const int sprite_height = height > 0 ? (int)(height * scale + 0.5f) : 0;
	int run_width = 0;
	for (UINT32 index = 0; index < sprite->text_length; ++index) {
		const UINT32 codepoint = sprite->text[index];
		UINT16 glyph = 0;
		INT32 metrics[7];
		if (!text_fonts.glyphs(text_fonts.font, &codepoint, 1, &glyph) ||
			!text_fonts.metrics(text_fonts.font, glyph, metrics))
			return;
		run_width += (int)(metrics[1] * design_scale + 0.5f);
	}
	int run_height = (int)((text_fonts.ascent + text_fonts.descent) * design_scale + 0.5f);
	if (sprite_width > 0 && run_width > sprite_width)
		run_width = sprite_width;
	if (sprite_height > 0 && run_height > sprite_height)
		run_height = sprite_height;
	if (run_width <= 0 || run_height <= 0)
		return;
	BITMAPINFO definition;
	ZeroMemory(&definition, sizeof definition);
	definition.bmiHeader.biSize = sizeof definition.bmiHeader;
	definition.bmiHeader.biWidth = run_width;
	definition.bmiHeader.biHeight = -run_height;    /* top-down, the way the paint walks */
	definition.bmiHeader.biPlanes = 1;
	definition.bmiHeader.biBitCount = 32;
	definition.bmiHeader.biCompression = BI_RGB;
	BYTE *pixels = NULL;
	HDC memory = CreateCompatibleDC(dc);
	HBITMAP bitmap = memory ? CreateDIBSection(memory, &definition, DIB_RGB_COLORS, (void **)&pixels, NULL, 0) : NULL;
	if (!memory || !bitmap || !pixels) {
		if (bitmap)
			DeleteObject(bitmap);
		if (memory)
			DeleteDC(memory);
		return;
	}
	HGDIOBJ previous = SelectObject(memory, bitmap);
	const int origin_x = (int)(x + 0.5f), origin_y = (int)(y + 0.5f);
	BitBlt(memory, 0, 0, run_width, run_height, dc, origin_x, origin_y, SRCCOPY);
	const int baseline = (int)(text_fonts.ascent * design_scale + 0.5f);
	float pen = 0.0f;
	for (UINT32 index = 0; index < sprite->text_length; ++index) {
		const UINT32 codepoint = sprite->text[index];
		UINT16 glyph = 0;
		INT32 metrics[7];
		if (!text_fonts.glyphs(text_fonts.font, &codepoint, 1, &glyph) ||
			!text_fonts.metrics(text_fonts.font, glyph, metrics))
			break;
		INT32 bounds[4];
		const DWORD size = text_fonts.render(text_fonts.font, glyph, px << 16, NULL, 0, NULL, 0, bounds, 0, 0);
		if (size) {
			BYTE *coverage = ALLOC(size);
			if (coverage && text_fonts.render(text_fonts.font, glyph, px << 16, NULL, 0, coverage, size, bounds, 0, 0) == size) {
				/* The bounds are the ink box around the glyph origin on the baseline, y downward; the pixels the run
				 * bound does not carry are skipped. */
				const int left = (int)(pen + 0.5f) + bounds[0];
				const int top = baseline + bounds[1];
				const int rows = bounds[3] - bounds[1], columns = bounds[2] - bounds[0];
				for (int row = 0; row < rows; ++row) {
					const int y = top + row;
					if (y < 0 || y >= run_height)
						continue;
					for (int column = 0; column < columns; ++column) {
						const int x = left + column;
						if ((x < 0 || x >= run_width) || coverage[(size_t)row * columns + column] != 255)
							continue;
						BYTE *pixel = pixels + ((size_t)y * run_width + x) * 4;
						pixel[0] = GetBValue(sprite->text_color);
						pixel[1] = GetGValue(sprite->text_color);
						pixel[2] = GetRValue(sprite->text_color);
						pixel[3] = 255;
					}
				}
			}
			if (coverage)
				FREE(coverage);
		}
		pen += metrics[1] * design_scale;
	}
	BitBlt(dc, origin_x, origin_y, run_width, run_height, memory, 0, 0, SRCCOPY);
	SelectObject(memory, previous);
	DeleteObject(bitmap);
	DeleteDC(memory);
}

static void paint_visual(struct composition_visual *visual, HDC dc, float origin_x, float origin_y, float scale)
{
	if (!visual->visible || visual->opacity <= 0)
		return;
	const float x = origin_x + visual->offset.X * scale;
	const float y = origin_y + visual->offset.Y * scale;
	if (visual->kind == composition_sprite) {
		struct composition_sprite *sprite = (struct composition_sprite *)
			((char *)visual - offsetof(struct composition_sprite, base));
		if (sprite->brush_data && sprite->brush_data->color.A) {
			RECT rect;
			rect.left = (LONG)(x + 0.5f);
			rect.top = (LONG)(y + 0.5f);
			rect.right = (LONG)(x + visual->size.X * scale + 0.5f);
			rect.bottom = (LONG)(y + visual->size.Y * scale + 0.5f);
			FillRect(dc, &rect, brush_fill(sprite->brush_data));
		}
		/* The text payload draws above the background fill, at the sprite's origin; the font service section above carries
		 * the constraints of the path. */
		if (sprite->text && sprite->text_length && sprite->text_alpha)
			paint_sprite_text(dc, sprite, x, y, visual->size.X, visual->size.Y, scale);
	} else {
		struct composition_container *container = (struct composition_container *)
			((char *)visual - offsetof(struct composition_container, base));
		struct children_vector *children = container->children;
		for (UINT32 index = 0; index < children->count; ++index)
			paint_visual(visual_of_part(children->items[index], offsetof(struct composition_visual, first)),
				dc, x, y, scale);
	}
}

static void present_target(struct composition_target *target)
{
	HWND hwnd = NULL;
	float scale = 1.0f;
	if (raven_window_composition_surface(target->window, &hwnd, &scale) != S_OK || !hwnd)
		return;
	HDC dc = GetDC(hwnd);
	RECT client;
	if (dc && GetClientRect(hwnd, &client)) {
		/* The CPU fallback repaints the whole surface; a damage-tracked or GPU path replaces this. */
		FillRect(dc, &client, GetStockObject(BLACK_BRUSH));
		if (target->root_data)
			paint_visual(target->root_data, dc, 0, 0, scale);
	}
	if (dc)
		ReleaseDC(hwnd, dc);
}

static void compositor_dirty(struct compositor_state *compositor)
{
	if (!compositor)
		return;
	for (struct composition_target *target = compositor->targets; target; target = target->next)
		present_target(target);
}

/* Compositor creation methods */

static HRESULT WINAPI compositor_create_container_visual(compositor1 *This, container1 **result)
{
	struct compositor_state *self = (struct compositor_state *)((char *)This - offsetof(struct compositor_state, first));
	if (!result)
		return E_POINTER;
	*result = NULL;
	struct composition_container *container = composition_alloc(sizeof *container);
	if (!container)
		return E_OUTOFMEMORY;
	container->base.first.lpVtbl = &container_visual_table;
	container->base.first.owner = &container->base;
	container->base.object.lpVtbl = &visual_object_table;
	container->base.object.owner = &container->base;
	container->base.refs = 1;
	container->base.kind = composition_container;
	container->base.visible = TRUE;
	container->base.opacity = 1.0f;
	container->base.compositor = self;
	container->container.lpVtbl = &container_part_full;
	container->container.owner = container;
	container->children = composition_alloc(sizeof *container->children);
	if (!container->children) {
		visual_release(&container->base);
		return E_OUTOFMEMORY;
	}
	container->children->lpVtbl = &children_table;
	container->children->refs = 1;
	container->children->owner = container;
	*result = (container1 *)&container->container;
	return S_OK;
}

static HRESULT WINAPI compositor_create_sprite_visual(compositor1 *This, sprite1 **result)
{
	struct compositor_state *self = (struct compositor_state *)((char *)This - offsetof(struct compositor_state, first));
	if (!result)
		return E_POINTER;
	*result = NULL;
	struct composition_sprite *sprite = composition_alloc(sizeof *sprite);
	if (!sprite)
		return E_OUTOFMEMORY;
	sprite->base.first.lpVtbl = &sprite_visual_table;
	sprite->base.first.owner = &sprite->base;
	sprite->base.object.lpVtbl = &visual_object_table;
	sprite->base.object.owner = &sprite->base;
	sprite->base.refs = 1;
	sprite->base.kind = composition_sprite;
	sprite->base.visible = TRUE;
	sprite->base.opacity = 1.0f;
	sprite->base.compositor = self;
	sprite->sprite.lpVtbl = &sprite_part_full;
	sprite->sprite.owner = sprite;
	*result = (sprite1 *)&sprite->sprite;
	return S_OK;
}

static HRESULT WINAPI compositor_create_target_for_current_view(compositor1 *This, target1 **result)
{
	struct compositor_state *self = (struct compositor_state *)((char *)This - offsetof(struct compositor_state, first));
	if (!result)
		return E_POINTER;
	*result = NULL;
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *window = NULL;
	const HRESULT window_status = raven_current_window(&window);
	if (FAILED(window_status))
		return window_status;
	if (!window)
		return E_INVALIDARG;
	struct composition_target *target = composition_alloc(sizeof *target);
	if (!target) {
		window->lpVtbl->Release(window);
		return E_OUTOFMEMORY;
	}
	target->first.lpVtbl = &target_first_table;
	target->first.owner = target;
	target->object.lpVtbl = &target_object_table;
	target->object.owner = target;
	/* The compositor's target list owns one reference; the caller receives another. */
	target->refs = 2;
	target->window = window;
	target->compositor = self;
	InterlockedIncrement(&self->refs);
	target->next = self->targets;
	self->targets = target;
	targets_attach(target);
	*result = (target1 *)&target->first;
	return S_OK;
}

/* The factory and catalog entry */

static HRESULT WINAPI composition_factory_query(IActivationFactory *iface, REFIID iid, void **out);
static ULONG WINAPI composition_factory_addref(IActivationFactory *iface);
static ULONG WINAPI composition_factory_release(IActivationFactory *iface);
static HRESULT WINAPI composition_factory_iids(IActivationFactory *iface, ULONG *count, IID **iids);
static HRESULT WINAPI composition_factory_name(IActivationFactory *iface, HSTRING *name);
static HRESULT WINAPI composition_factory_trust(IActivationFactory *iface, TrustLevel *level);
static HRESULT WINAPI composition_factory_activate(IActivationFactory *iface, IInspectable **out);

static const IActivationFactoryVtbl composition_factory_table = {
	.QueryInterface = composition_factory_query,
	.AddRef = composition_factory_addref,
	.Release = composition_factory_release,
	.GetIids = composition_factory_iids,
	.GetRuntimeClassName = composition_factory_name,
	.GetTrustLevel = composition_factory_trust,
	.ActivateInstance = composition_factory_activate,
};

static struct compositor_state composition_factory_instance = {
	{&compositor_first_table, &composition_factory_instance},
	{&compositor_object_table, &composition_factory_instance}, 1, NULL};

static struct composition_factory {
	const IActivationFactoryVtbl *lpVtbl;
	LONG refs;
} composition_factory = {&composition_factory_table, 1};

static HRESULT WINAPI composition_factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) && !equal_iid(iid, &IID_IActivationFactory))
		return E_NOINTERFACE;
	InterlockedIncrement(&composition_factory.refs);
	*out = (void *)&composition_factory;
	return S_OK;
}

static ULONG WINAPI composition_factory_addref(IActivationFactory *iface)
{
	(void)iface;
	return (ULONG)InterlockedIncrement(&composition_factory.refs);
}

static ULONG WINAPI composition_factory_release(IActivationFactory *iface)
{
	(void)iface;
	return (ULONG)InterlockedDecrement(&composition_factory.refs);
}

static HRESULT WINAPI composition_factory_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI composition_factory_name(IActivationFactory *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(compositor_class, (UINT32)lstrlenW(compositor_class), name);
}

static HRESULT WINAPI composition_factory_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI composition_factory_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	/* A compositor is stateful: activation hands out the process compositor, whose targets carry the per-window state. */
	InterlockedIncrement(&composition_factory_instance.refs);
	*out = (IInspectable *)&composition_factory_instance.first;
	return S_OK;
}

HRESULT raven_composition_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (length != ARRAYSIZE(compositor_class) - 1 ||
		CompareStringOrdinal(text, (int)length, compositor_class, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	InterlockedIncrement(&composition_factory.refs);
	*out = (IActivationFactory *)&composition_factory;
	return S_OK;
}

/* The compositor the rendering pass builds trees with, and the attachment of a rendered root to the target bound to a window. */

HRESULT raven_composition_instance(compositor1 **out)
{
	if (!out)
		return E_POINTER;
	InterlockedIncrement(&composition_factory_instance.refs);
	*out = (compositor1 *)&composition_factory_instance.first;
	return S_OK;
}

HRESULT raven_composition_attach(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window, visual1 *root)
{
	struct composition_target *target = NULL;
	for (target = composition_factory_instance.targets; target; target = target->next)
		if (target->window == window)
			break;
	if (target)
		return target->first.lpVtbl->put_Root((target1 *)&target->first, root);
	/* A newly created target carries its creator's reference: configure it through that reference, then release it. */
	target1 *created = NULL;
	const HRESULT result = compositor_create_target_for_current_view(
		(compositor1 *)&composition_factory_instance.first, &created);
	if (FAILED(result))
		return result;
	target = (struct composition_target *)((char *)created - offsetof(struct composition_target, first));
	HRESULT attached = (target->window == window)
		? target->first.lpVtbl->put_Root((target1 *)&target->first, root) : E_INVALIDARG;
	created->lpVtbl->Release(created);
	return attached;
}

/* The text payload of a TextBlock's sprite, handed over by the rendering pass: the composition side takes its own copy,
 * replaces any payload the sprite carried before and frees the copy when the visual releases. A zero length or a null
 * text clears the payload, and a transparent foreground draws nothing in the presentation. */
HRESULT raven_composition_sprite_text(visual1 *visual, const WCHAR *text, UINT32 length, float font_size,
	COLORREF color, BYTE alpha)
{
	if (!visual)
		return E_INVALIDARG;
	/* The reference may arrive through any part of the sprite object; the IVisual query answers with the part the
	 * visual state hangs off, the way put_Root resolves its root. */
	visual1 *part = NULL;
	if (FAILED(visual->lpVtbl->QueryInterface(visual, &IID___x_ABI_CWindows_CUI_CComposition_CIVisual, (void **)&part)) ||
		!part)
		return E_NOINTERFACE;
	struct composition_visual *self = visual_of_part(part, offsetof(struct composition_visual, first));
	part->lpVtbl->Release(part);
	if (self->kind != composition_sprite)
		return E_INVALIDARG;
	struct composition_sprite *sprite = (struct composition_sprite *)
		((char *)self - offsetof(struct composition_sprite, base));
	WCHAR *copy = NULL;
	if (text && length) {
		copy = HeapAlloc(GetProcessHeap(), 0, (size_t)length * sizeof(WCHAR));
		if (!copy)
			return E_OUTOFMEMORY;
		memcpy(copy, text, (size_t)length * sizeof(WCHAR));
	}
	if (sprite->text)
		HeapFree(GetProcessHeap(), 0, sprite->text);
	sprite->text = copy;
	sprite->text_length = copy ? length : 0;
	sprite->text_font_size = font_size;
	sprite->text_color = color;
	sprite->text_alpha = alpha;
	compositor_dirty(self->compositor);
	return S_OK;
}

/* Window size changes re-present every target bound to the window. */

void raven_composition_size(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window)
{
	if (InterlockedCompareExchange(&targets_lock_ready, 0, 0) != 2)
		return;
	EnterCriticalSection(&targets_lock);
	for (struct composition_target *target = live_targets; target; target = target->next)
		if (target->window == window)
			present_target(target);
	LeaveCriticalSection(&targets_lock);
}

BOOL raven_composition_notify(DWORD reason, void *reserved)
{
	(void)reserved;
	if (reason == DLL_PROCESS_ATTACH) {
		if (InterlockedCompareExchange(&targets_lock_ready, 0, 0) == 0) {
			InitializeCriticalSection(&targets_lock);
			InterlockedExchange(&targets_lock_ready, 2);
		}
	} else if (reason == DLL_PROCESS_DETACH) {
		if (InterlockedCompareExchange(&targets_lock_ready, 2, 2) == 2) {
			InterlockedExchange(&targets_lock_ready, 0);
			DeleteCriticalSection(&targets_lock);
		}
	}
	return TRUE;
}
