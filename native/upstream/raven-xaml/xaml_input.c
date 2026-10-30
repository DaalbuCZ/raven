/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: pointer routing from the CoreWindow to the hosted element tree.
 *
 * The render pass records every projected element's DIP rectangle here (raven_input_record, in paint order: ancestors before
 * descendants, so a reverse walk finds the topmost element at a point first). When a tree is hosted the routing subscribes once
 * to the hosting CoreWindow's pointer events, through the CoreWindow's own event surface; each event's DIP position is
 * hit-tested against the recorded geometry and delivered to the elements:
 *
 *   - PointerEntered/Exited fire per element when the pointer starts or stops being over it. Every recorded hit-testable
 *     element containing the position hovers — ancestry and occlusion between siblings are not modeled, because the flat
 *     projection has no tree to walk; moving onto a button therefore also moves onto the grid behind it.
 *   - PointerPressed and PointerWheelChanged go to the topmost element under the pointer.
 *   - PointerReleased goes to the pressed element even when the pointer moved away — the minimal capture semantics of this
 *     stage: the press holds the target until the release, and the release checks its own position against the pressed
 *     element's bounds (the ClickMode.Release contract). Without a pressed element the release goes to the element under
 *     the pointer.
 *   - PointerCaptureLost goes to the pressed element, which also ends the press.
 *
 * Delivery goes through the element classes (raven_xaml_element_pointer), which run the element's own behavior and then the
 * application's handlers; a failing handler never breaks the routing. The subscription and the geometry end when the hosted
 * tree is replaced or the CoreWindow closes, which the routing observes through the window's Closed event. Everything here
 * runs on the hosting UI thread — the render pass and the routed events both do — so there is no lock between them. */
#define INITGUID
#define CONST_VTABLE
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_UI_Input
#define WIDL_using_Windows_Foundation
#include <windows.h>
#include <winstring.h>
#include <windows.ui.core.h>
#include <windows.foundation.h>
#include "events.h"
#include "xaml_internal.h"
#include "xaml_input.h"
#include "windows_ui_xaml_uielement.h"

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

typedef __x_ABI_CWindows_CUI_CCore_CICoreWindow core_window;
typedef __x_ABI_CWindows_CUI_CCore_CIPointerEventArgs pointer_args1;
typedef __x_ABI_CWindows_CFoundation_CPoint point1;

/* One recorded element rectangle of the latest render pass. The element is held by its IUnknown, the one identity every part
 * of the object answers, and by a reference for as long as the geometry lists it. */
struct input_hit {
	IUnknown *element;
	float x, y, width, height;
};

enum input_event {
	INPUT_ENTERED, INPUT_EXITED, INPUT_MOVED, INPUT_PRESSED, INPUT_RELEASED, INPUT_WHEEL, INPUT_CAPTURE_LOST, INPUT_CLOSED
};

/* One hosted tree per window: the recorded geometry, the elements currently under the pointer, and the pressed element. */
struct input_root {
	core_window *window;
	struct input_hit *hits;
	UINT32 count, capacity;
	IUnknown **hovered; /* in paint order, each element by reference */
	UINT32 hover_count;
	IUnknown *pressed;
	IUnknown *element; /* the hosted tree's identity, to recognize a re-hosting of the same tree */
	struct input_pointer_handler *handlers[INPUT_CLOSED];
	struct input_closed_handler *closed;
	EventRegistrationToken tokens[INPUT_CLOSED + 1];
	struct input_root *next;
};

struct input_pointer_handler {
	const __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgsVtbl *lpVtbl;
	LONG refs;
	struct input_root *root;
	enum input_event event;
};

struct input_closed_handler {
	const __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCoreWindowEventArgsVtbl *lpVtbl;
	LONG refs;
	struct input_root *root;
};

static struct input_root *input_roots;
/* Bumped whenever a root is torn down: a handler that closes the window mid-dispatch frees the root this dispatch is walking,
 * so every delivery is followed by a generation check before the next access. */
static LONG input_generation;

static void input_dispatch(struct input_root *root, enum input_event event, pointer_args1 *args);
static void input_detach(struct input_root *root);

static struct input_root *root_of(core_window *window)
{
	for (struct input_root *root = input_roots; root; root = root->next)
		if (root->window == window)
			return root;
	return NULL;
}

static IUnknown *element_identity(IInspectable *element)
{
	IUnknown *unknown = NULL;
	if (element)
		element->lpVtbl->QueryInterface(element, &IID_IUnknown, (void **)&unknown);
	return unknown;
}

static BOOL hit_contains(const struct input_hit *hit, const point1 *position)
{
	return position->X >= hit->x && position->X < hit->x + hit->width &&
		position->Y >= hit->y && position->Y < hit->y + hit->height;
}

/* An element is hit-testable unless its IUIElement says otherwise; the page and frame wrappers do not answer IUIElement and
 * count as visible (they cannot receive a routed event, they only sit in the recorded geometry). */
static boolean hit_visible(const struct input_hit *hit)
{
	__x_ABI_CWindows_CUI_CXaml_CIUIElement *ui = NULL;
	boolean visible = TRUE;
	if (SUCCEEDED(hit->element->lpVtbl->QueryInterface(hit->element, &IID__x_ABI_CWindows_CUI_CXaml_CIUIElement,
		(void **)&ui)) && ui) {
		ui->lpVtbl->get_IsHitTestVisible(ui, (uint64_t)(uintptr_t)&visible);
		ui->lpVtbl->Release(ui);
	}
	return visible;
}

/* The topmost recorded element containing the position: the paint order walked in reverse. */
static struct input_hit *hit_test(struct input_root *root, const point1 *position)
{
	for (UINT32 index = root->count; index > 0; --index) {
		struct input_hit *hit = &root->hits[index - 1];
		if (hit->element && hit_contains(hit, position) && hit_visible(hit))
			return hit;
	}
	return NULL;
}

static int hovered_contains(IUnknown **set, UINT32 count, IUnknown *element)
{
	for (UINT32 index = 0; index < count; ++index)
		if (set[index] == element)
			return 1;
	return 0;
}

/* Rebuild the hover set for the position and deliver the transitions: Exited for the elements the pointer left and Entered for
 * the ones it reached, both topmost first. The set borrows nothing: each member is held by reference for as long as it
 * hovers, because a handler can re-render (and so clear the recorded geometry) at any point. */
static void hover_update(struct input_root *root, const point1 *position, pointer_args1 *args, LONG generation)
{
	IUnknown **fresh = NULL;
	UINT32 fresh_count = 0;
	if (root->count) {
		fresh = ALLOC((size_t)root->count * sizeof *fresh);
		if (!fresh)
			return; /* keep the previous hover set; the next event rebuilds it */
		for (UINT32 index = 0; index < root->count; ++index) {
			struct input_hit *hit = &root->hits[index];
			if (hit->element && hit_contains(hit, position) && hit_visible(hit))
				fresh[fresh_count++] = hit->element;
		}
	}
	for (UINT32 index = root->hover_count; index > 0; --index) {
		if (hovered_contains(fresh, fresh_count, root->hovered[index - 1]))
			continue;
		raven_xaml_element_pointer((IInspectable *)root->hovered[index - 1], RAVEN_ELEMENT_POINTER_EXITED, args);
		if (input_generation != generation) {
			FREE(fresh);
			return;
		}
		IUnknown_Release(root->hovered[index - 1]);
	}
	for (UINT32 index = fresh_count; index > 0; --index) {
		if (hovered_contains(root->hovered, root->hover_count, fresh[index - 1]))
			continue;
		IUnknown_AddRef(fresh[index - 1]);
		raven_xaml_element_pointer((IInspectable *)fresh[index - 1], RAVEN_ELEMENT_POINTER_ENTERED, args);
		if (input_generation != generation) {
			IUnknown_Release(fresh[index - 1]);
			FREE(fresh);
			return;
		}
	}
	if (root->hovered)
		FREE(root->hovered);
	root->hovered = fresh;
	root->hover_count = fresh_count;
}

static void input_dispatch(struct input_root *root, enum input_event event, pointer_args1 *args)
{
	const LONG generation = input_generation;
	point1 position = {0, 0};
	if (event != INPUT_CAPTURE_LOST) {
		IPointerPoint *point = NULL;
		if (FAILED(IPointerEventArgs_get_CurrentPoint(args, &point)) || !point)
			return;
		IPointerPoint_get_Position(point, &position);
		IPointerPoint_Release(point);
	}
	switch (event) {
	case INPUT_ENTERED:
	case INPUT_MOVED:
		hover_update(root, &position, args, generation);
		if (input_generation != generation || event != INPUT_MOVED || !root->hover_count)
			break;
		/* The topmost hovered element receives the move; the set is in paint order. */
		raven_xaml_element_pointer((IInspectable *)root->hovered[root->hover_count - 1],
			RAVEN_ELEMENT_POINTER_MOVED, args);
		break;
	case INPUT_PRESSED: {
		struct input_hit *hit = hit_test(root, &position);
		if (!hit)
			break;
		IUnknown *target = hit->element;
		IUnknown_AddRef(target);
		raven_xaml_element_pointer((IInspectable *)target, RAVEN_ELEMENT_POINTER_PRESSED, args);
		if (input_generation != generation) {
			IUnknown_Release(target);
			break;
		}
		if (root->pressed)
			IUnknown_Release(root->pressed);
		root->pressed = target; /* the press holds the target until the release */
		break;
	}
	case INPUT_RELEASED: {
		IUnknown *target = root->pressed;
		if (target)
			root->pressed = NULL; /* take the reference over before delivering */
		else {
			struct input_hit *hit = hit_test(root, &position);
			target = hit ? hit->element : NULL;
			if (target)
				IUnknown_AddRef(target);
		}
		if (target) {
			raven_xaml_element_pointer((IInspectable *)target, RAVEN_ELEMENT_POINTER_RELEASED, args);
			IUnknown_Release(target);
		}
		break;
	}
	case INPUT_WHEEL: {
		struct input_hit *hit = hit_test(root, &position);
		if (hit)
			raven_xaml_element_pointer((IInspectable *)hit->element, RAVEN_ELEMENT_POINTER_WHEEL, args);
		break;
	}
	case INPUT_EXITED:
		for (UINT32 index = root->hover_count; index > 0; --index) {
			IUnknown *element = root->hovered[index - 1];
			raven_xaml_element_pointer((IInspectable *)element, RAVEN_ELEMENT_POINTER_EXITED, args);
			if (input_generation != generation)
				return; /* the detach released the rest of the set */
			IUnknown_Release(element);
		}
		root->hover_count = 0;
		break;
	case INPUT_CAPTURE_LOST:
		if (root->pressed) {
			IUnknown *target = root->pressed;
			root->pressed = NULL;
			raven_xaml_element_pointer((IInspectable *)target, RAVEN_ELEMENT_POINTER_CAPTURE_LOST, args);
			IUnknown_Release(target);
		}
		break;
	case INPUT_CLOSED:
		input_detach(root);
		break;
	}
}

static HRESULT WINAPI pointer_handler_query(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *This,
	REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) &&
		!IsEqualIID(iid, &IID___FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs))
		return E_NOINTERFACE;
	*out = This;
	InterlockedIncrement(&((struct input_pointer_handler *)This)->refs);
	return S_OK;
}
static ULONG WINAPI pointer_handler_addref(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *This)
{
	return (ULONG)InterlockedIncrement(&((struct input_pointer_handler *)This)->refs);
}
static ULONG WINAPI pointer_handler_release(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *This)
{
	struct input_pointer_handler *handler = (struct input_pointer_handler *)This;
	const LONG refs = InterlockedDecrement(&handler->refs);
	if (!refs) {
		FREE(handler);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}
static HRESULT WINAPI pointer_handler_invoke(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *This,
	ICoreWindow *window, IPointerEventArgs *args)
{
	struct input_pointer_handler *handler = (struct input_pointer_handler *)This;
	(void)window;
	input_dispatch(handler->root, handler->event, args);
	/* An element handler failing must not fail the CoreWindow's own dispatch; the routing never propagates errors. */
	return S_OK;
}
static const __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgsVtbl input_pointer_table = {
	pointer_handler_query, pointer_handler_addref, pointer_handler_release, pointer_handler_invoke
};

static HRESULT WINAPI closed_handler_query(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCoreWindowEventArgs *This,
	REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) &&
		!IsEqualIID(iid, &IID___FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCoreWindowEventArgs))
		return E_NOINTERFACE;
	*out = This;
	InterlockedIncrement(&((struct input_closed_handler *)This)->refs);
	return S_OK;
}
static ULONG WINAPI closed_handler_addref(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCoreWindowEventArgs *This)
{
	return (ULONG)InterlockedIncrement(&((struct input_closed_handler *)This)->refs);
}
static ULONG WINAPI closed_handler_release(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCoreWindowEventArgs *This)
{
	struct input_closed_handler *handler = (struct input_closed_handler *)This;
	const LONG refs = InterlockedDecrement(&handler->refs);
	if (!refs) {
		FREE(handler);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}
static HRESULT WINAPI closed_handler_invoke(__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCoreWindowEventArgs *This,
	ICoreWindow *window, ICoreWindowEventArgs *args)
{
	struct input_closed_handler *handler = (struct input_closed_handler *)This;
	(void)window;
	(void)args;
	input_detach(handler->root);
	return S_OK;
}
static const __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCoreWindowEventArgsVtbl input_closed_table = {
	closed_handler_query, closed_handler_addref, closed_handler_release, closed_handler_invoke
};

static struct input_pointer_handler *input_handler_create(struct input_root *root, enum input_event event)
{
	struct input_pointer_handler *handler = ALLOC(sizeof *handler);
	if (!handler)
		return NULL;
	handler->lpVtbl = &input_pointer_table;
	handler->refs = 1;
	handler->root = root;
	handler->event = event;
	InterlockedIncrement(&raven_xaml_objects);
	return handler;
}

static void input_unsubscribe(struct input_root *root)
{
	for (int event = 0; event < INPUT_CLOSED; ++event) {
		struct input_pointer_handler *handler = root->handlers[event];
		if (!handler)
			continue;
		root->handlers[event] = NULL;
		switch (event) {
		case INPUT_ENTERED: ICoreWindow_remove_PointerEntered(root->window, root->tokens[event]); break;
		case INPUT_EXITED: ICoreWindow_remove_PointerExited(root->window, root->tokens[event]); break;
		case INPUT_MOVED: ICoreWindow_remove_PointerMoved(root->window, root->tokens[event]); break;
		case INPUT_PRESSED: ICoreWindow_remove_PointerPressed(root->window, root->tokens[event]); break;
		case INPUT_RELEASED: ICoreWindow_remove_PointerReleased(root->window, root->tokens[event]); break;
		case INPUT_WHEEL: ICoreWindow_remove_PointerWheelChanged(root->window, root->tokens[event]); break;
		case INPUT_CAPTURE_LOST: ICoreWindow_remove_PointerCaptureLost(root->window, root->tokens[event]); break;
		default: break;
		}
		handler->lpVtbl->Release((void *)handler);
	}
	if (root->closed) {
		struct input_closed_handler *handler = root->closed;
		root->closed = NULL;
		ICoreWindow_remove_Closed(root->window, root->tokens[INPUT_CLOSED]);
		handler->lpVtbl->Release((void *)handler);
	}
}

/* Unsubscribe and drop everything the root holds. Runs on the UI thread, either from the window's Closed dispatch or from a
 * re-hosting; the generation bump tells the dispatch that is walking this root to stop touching it. */
static void input_detach(struct input_root *root)
{
	struct input_root **link = &input_roots;
	InterlockedIncrement(&input_generation);
	while (*link && *link != root)
		link = &(*link)->next;
	if (*link == root)
		*link = root->next;
	input_unsubscribe(root);
	if (root->hovered) {
		for (UINT32 index = 0; index < root->hover_count; ++index)
			IUnknown_Release(root->hovered[index]);
		FREE(root->hovered);
	}
	if (root->pressed)
		IUnknown_Release(root->pressed);
	if (root->element)
		IUnknown_Release(root->element);
	if (root->hits) {
		for (UINT32 index = 0; index < root->count; ++index)
			if (root->hits[index].element)
				IUnknown_Release(root->hits[index].element);
		FREE(root->hits);
	}
	ICoreWindow_Release(root->window);
	FREE(root);
	InterlockedDecrement(&raven_xaml_objects);
}

void raven_input_begin(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window)
{
	struct input_root *root = window ? root_of(window) : NULL;
	if (!root)
		return;
	for (UINT32 index = 0; index < root->count; ++index)
		if (root->hits[index].element)
			IUnknown_Release(root->hits[index].element);
	root->count = 0;
}

void raven_input_record(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window, IInspectable *element,
	float x, float y, float width, float height)
{
	struct input_root *root;
	IUnknown *identity;
	if (!window || !element)
		return;
	root = root_of(window);
	if (!root)
		return;
	identity = element_identity(element);
	if (!identity)
		return;
	if (root->count == root->capacity) {
		const UINT32 capacity = root->capacity ? root->capacity * 2 : 16;
		struct input_hit *hits = ALLOC((size_t)capacity * sizeof *hits);
		if (!hits) {
			IUnknown_Release(identity);
			return;
		}
		if (root->hits) {
			for (UINT32 index = 0; index < root->count; ++index)
				hits[index] = root->hits[index];
			FREE(root->hits);
		}
		root->hits = hits;
		root->capacity = capacity;
	}
	root->hits[root->count].element = identity;
	root->hits[root->count].x = x;
	root->hits[root->count].y = y;
	root->hits[root->count].width = width;
	root->hits[root->count].height = height;
	++root->count;
}

HRESULT raven_input_hosted(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window, IInspectable *element)
{
	struct input_root *root;
	IUnknown *identity;
	if (!window)
		return E_POINTER;
	root = root_of(window);
	if (root) {
		/* The same tree hosted again keeps its subscription; anything else replaces it. */
		int same = FALSE;
		identity = element_identity(element);
		if (identity) {
			same = root->element == identity;
			IUnknown_Release(identity);
		}
		if (same)
			return S_OK;
		input_detach(root);
	}
	if (!element)
		return S_OK;
	root = ALLOC(sizeof *root);
	if (!root)
		return E_OUTOFMEMORY;
	root->window = window;
	ICoreWindow_AddRef(window);
	root->hits = NULL;
	root->count = 0;
	root->capacity = 0;
	root->hovered = NULL;
	root->hover_count = 0;
	root->pressed = NULL;
	root->element = element_identity(element);
	root->next = input_roots;
	input_roots = root;
	InterlockedIncrement(&raven_xaml_objects);
	for (int event = 0; event < INPUT_CLOSED; ++event)
		root->handlers[event] = NULL;
	root->closed = NULL;
	/* One subscription per hosted tree, through the CoreWindow's own event surface. */
	for (int event = 0; event < INPUT_CLOSED; ++event) {
		struct input_pointer_handler *handler = input_handler_create(root, (enum input_event)event);
		HRESULT result;
		if (!handler) {
			result = E_OUTOFMEMORY;
			input_detach(root);
			return result;
		}
		root->handlers[event] = handler;
		switch (event) {
		case INPUT_ENTERED: result = ICoreWindow_add_PointerEntered(window, (void *)handler, &root->tokens[event]); break;
		case INPUT_EXITED: result = ICoreWindow_add_PointerExited(window, (void *)handler, &root->tokens[event]); break;
		case INPUT_MOVED: result = ICoreWindow_add_PointerMoved(window, (void *)handler, &root->tokens[event]); break;
		case INPUT_PRESSED: result = ICoreWindow_add_PointerPressed(window, (void *)handler, &root->tokens[event]); break;
		case INPUT_RELEASED: result = ICoreWindow_add_PointerReleased(window, (void *)handler, &root->tokens[event]); break;
		case INPUT_WHEEL: result = ICoreWindow_add_PointerWheelChanged(window, (void *)handler, &root->tokens[event]); break;
		default: result = ICoreWindow_add_PointerCaptureLost(window, (void *)handler, &root->tokens[event]); break;
		}
		if (result != S_OK) {
			input_detach(root);
			return result;
		}
	}
	{
		struct input_closed_handler *handler = ALLOC(sizeof *handler);
		HRESULT result;
		if (!handler) {
			input_detach(root);
			return E_OUTOFMEMORY;
		}
		handler->lpVtbl = &input_closed_table;
		handler->refs = 1;
		handler->root = root;
		InterlockedIncrement(&raven_xaml_objects);
		root->closed = handler;
		result = ICoreWindow_add_Closed(window, (void *)handler, &root->tokens[INPUT_CLOSED]);
		if (result != S_OK) {
			input_detach(root);
			return result;
		}
	}
	return S_OK;
}

BOOL raven_input_element_contains(IInspectable *element, const __x_ABI_CWindows_CFoundation_CPoint *position)
{
	IUnknown *identity;
	BOOL contained = FALSE;
	if (!element || !position)
		return FALSE;
	identity = element_identity(element);
	if (!identity)
		return FALSE;
	for (struct input_root *root = input_roots; root && !contained; root = root->next)
		for (UINT32 index = 0; index < root->count; ++index)
			if (root->hits[index].element == identity && hit_contains(&root->hits[index], position)) {
				contained = TRUE;
				break;
			}
	IUnknown_Release(identity);
	return contained;
}
