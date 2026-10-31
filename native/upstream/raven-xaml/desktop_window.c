/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
#define INITGUID
#define CONST_VTABLE
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define WIDL_using_Windows_UI_Input
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_System
#include <windows.h>
#include <windowsx.h>
#include <winstring.h>
#include "desktop_window.h"
#include "pointer.h"
#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))
#define RO_E_CLOSED ((HRESULT)0x80000013)
#define POINTER_HISTORY_CAP 8

struct raven_desktop_window {
	ICoreWindow *owner;
	HWND hwnd;
	DWORD thread;
	BOOL closing, notified_closed;
	boolean message_handled;
	LONG error;
	WCHAR high_surrogate;
	BOOL pointer_inside;
	struct raven_mouse_sample mouse;
	struct raven_mouse_sample history[POINTER_HISTORY_CAP];
	UINT32 history_count, history_next;
	struct raven_event_list events[RAVEN_WINDOW_EVENT_COUNT];
};
static HINSTANCE module_instance;
static LONG pointer_frame;
static const WCHAR window_class[] = L"Raven.Xaml.CoreWindow";

struct event_args {
	ICoreWindowEventArgs handled;
	LONG refs, is_handled;
	enum raven_window_event event;
	IWindowSizeChangedEventArgs size;
	IVisibilityChangedEventArgs visible;
	IWindowActivatedEventArgs activated;
	IKeyEventArgs key;
	ICharacterReceivedEventArgs character;
	IInputEnabledEventArgs input;
	IPointerEventArgs pointer;
	IPointerPoint *point;
	__FIVector_1_Windows__CUI__CInput__CPointerPoint *history;
	VirtualKeyModifiers modifiers;
	Size dimensions;
	boolean flag;
	CoreWindowActivationState activation;
	UINT32 keycode;
	CorePhysicalKeyStatus key_status;
};
static const WCHAR *args_name(enum raven_window_event event)
{
	if (event >= RAVEN_WINDOW_POINTER_ENTERED) return L"Windows.UI.Core.PointerEventArgs";
	switch (event) {
	case RAVEN_WINDOW_SIZE: return L"Windows.UI.Core.WindowSizeChangedEventArgs";
	case RAVEN_WINDOW_VISIBLE: return L"Windows.UI.Core.VisibilityChangedEventArgs";
	case RAVEN_WINDOW_ACTIVATED: return L"Windows.UI.Core.WindowActivatedEventArgs";
	case RAVEN_WINDOW_KEY_DOWN: case RAVEN_WINDOW_KEY_UP: return L"Windows.UI.Core.KeyEventArgs";
	case RAVEN_WINDOW_CHARACTER: return L"Windows.UI.Core.CharacterReceivedEventArgs";
	case RAVEN_WINDOW_INPUT_ENABLED: return L"Windows.UI.Core.InputEnabledEventArgs";
	default: return L"Windows.UI.Core.CoreWindowEventArgs";
	}
}
static HRESULT args_query(struct event_args *self, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID_ICoreWindowEventArgs))
		*out = &self->handled;
	else if (self->event == RAVEN_WINDOW_SIZE && IsEqualIID(iid, &IID_IWindowSizeChangedEventArgs)) *out = &self->size;
	else if (self->event == RAVEN_WINDOW_VISIBLE && IsEqualIID(iid, &IID_IVisibilityChangedEventArgs)) *out = &self->visible;
	else if (self->event == RAVEN_WINDOW_ACTIVATED && IsEqualIID(iid, &IID_IWindowActivatedEventArgs)) *out = &self->activated;
	else if ((self->event == RAVEN_WINDOW_KEY_DOWN || self->event == RAVEN_WINDOW_KEY_UP) && IsEqualIID(iid, &IID_IKeyEventArgs)) *out = &self->key;
	else if (self->event == RAVEN_WINDOW_CHARACTER && IsEqualIID(iid, &IID_ICharacterReceivedEventArgs)) *out = &self->character;
	else if (self->event == RAVEN_WINDOW_INPUT_ENABLED && IsEqualIID(iid, &IID_IInputEnabledEventArgs)) *out = &self->input;
	else if (self->event >= RAVEN_WINDOW_POINTER_ENTERED && IsEqualIID(iid, &IID_IPointerEventArgs)) *out = &self->pointer;
	else return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}
static ULONG args_addref(struct event_args *self) { return InterlockedIncrement(&self->refs); }
static ULONG args_release(struct event_args *self)
{
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->point) IPointerPoint_Release(self->point);
		if (self->history) __FIVector_1_Windows__CUI__CInput__CPointerPoint_Release(self->history);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return refs;
}
static HRESULT args_iids(ULONG *count, IID **iids)
{
	if (!count || !iids) return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}
static HRESULT args_class(struct event_args *self, HSTRING *out)
{
	if (!out) return E_POINTER;
	const WCHAR *name = args_name(self->event);
	return WindowsCreateString(name, (UINT32)lstrlenW(name), out);
}
static HRESULT args_trust(TrustLevel *out)
{
	if (!out) return E_POINTER;
	*out = BaseTrust;
	return S_OK;
}

static HRESULT WINAPI handled_query(ICoreWindowEventArgs *iface, REFIID iid, void **out)
{ return args_query(CONTAINING_RECORD(iface, struct event_args, handled), iid, out); }
static ULONG WINAPI handled_addref(ICoreWindowEventArgs *iface)
{ return args_addref(CONTAINING_RECORD(iface, struct event_args, handled)); }
static ULONG WINAPI handled_release(ICoreWindowEventArgs *iface)
{ return args_release(CONTAINING_RECORD(iface, struct event_args, handled)); }
static HRESULT WINAPI handled_iids(ICoreWindowEventArgs *iface, ULONG *count, IID **iids) { return args_iids(count, iids); }
static HRESULT WINAPI handled_class(ICoreWindowEventArgs *iface, HSTRING *out)
{ return args_class(CONTAINING_RECORD(iface, struct event_args, handled), out); }
static HRESULT WINAPI handled_trust(ICoreWindowEventArgs *iface, TrustLevel *out) { return args_trust(out); }

static HRESULT WINAPI size_query(IWindowSizeChangedEventArgs *iface, REFIID iid, void **out)
{ return args_query(CONTAINING_RECORD(iface, struct event_args, size), iid, out); }
static ULONG WINAPI size_addref(IWindowSizeChangedEventArgs *iface)
{ return args_addref(CONTAINING_RECORD(iface, struct event_args, size)); }
static ULONG WINAPI size_release(IWindowSizeChangedEventArgs *iface)
{ return args_release(CONTAINING_RECORD(iface, struct event_args, size)); }
static HRESULT WINAPI size_iids(IWindowSizeChangedEventArgs *iface, ULONG *count, IID **iids) { return args_iids(count, iids); }
static HRESULT WINAPI size_class(IWindowSizeChangedEventArgs *iface, HSTRING *out)
{ return args_class(CONTAINING_RECORD(iface, struct event_args, size), out); }
static HRESULT WINAPI size_trust(IWindowSizeChangedEventArgs *iface, TrustLevel *out) { return args_trust(out); }

static HRESULT WINAPI visible_query(IVisibilityChangedEventArgs *iface, REFIID iid, void **out)
{ return args_query(CONTAINING_RECORD(iface, struct event_args, visible), iid, out); }
static ULONG WINAPI visible_addref(IVisibilityChangedEventArgs *iface)
{ return args_addref(CONTAINING_RECORD(iface, struct event_args, visible)); }
static ULONG WINAPI visible_release(IVisibilityChangedEventArgs *iface)
{ return args_release(CONTAINING_RECORD(iface, struct event_args, visible)); }
static HRESULT WINAPI visible_iids(IVisibilityChangedEventArgs *iface, ULONG *count, IID **iids) { return args_iids(count, iids); }
static HRESULT WINAPI visible_class(IVisibilityChangedEventArgs *iface, HSTRING *out)
{ return args_class(CONTAINING_RECORD(iface, struct event_args, visible), out); }
static HRESULT WINAPI visible_trust(IVisibilityChangedEventArgs *iface, TrustLevel *out) { return args_trust(out); }

static HRESULT WINAPI activated_query(IWindowActivatedEventArgs *iface, REFIID iid, void **out)
{ return args_query(CONTAINING_RECORD(iface, struct event_args, activated), iid, out); }
static ULONG WINAPI activated_addref(IWindowActivatedEventArgs *iface)
{ return args_addref(CONTAINING_RECORD(iface, struct event_args, activated)); }
static ULONG WINAPI activated_release(IWindowActivatedEventArgs *iface)
{ return args_release(CONTAINING_RECORD(iface, struct event_args, activated)); }
static HRESULT WINAPI activated_iids(IWindowActivatedEventArgs *iface, ULONG *count, IID **iids) { return args_iids(count, iids); }
static HRESULT WINAPI activated_class(IWindowActivatedEventArgs *iface, HSTRING *out)
{ return args_class(CONTAINING_RECORD(iface, struct event_args, activated), out); }
static HRESULT WINAPI activated_trust(IWindowActivatedEventArgs *iface, TrustLevel *out) { return args_trust(out); }

static HRESULT WINAPI key_query(IKeyEventArgs *iface, REFIID iid, void **out)
{ return args_query(CONTAINING_RECORD(iface, struct event_args, key), iid, out); }
static ULONG WINAPI key_addref(IKeyEventArgs *iface)
{ return args_addref(CONTAINING_RECORD(iface, struct event_args, key)); }
static ULONG WINAPI key_release(IKeyEventArgs *iface)
{ return args_release(CONTAINING_RECORD(iface, struct event_args, key)); }
static HRESULT WINAPI key_iids(IKeyEventArgs *iface, ULONG *count, IID **iids) { return args_iids(count, iids); }
static HRESULT WINAPI key_class(IKeyEventArgs *iface, HSTRING *out)
{ return args_class(CONTAINING_RECORD(iface, struct event_args, key), out); }
static HRESULT WINAPI key_trust(IKeyEventArgs *iface, TrustLevel *out) { return args_trust(out); }

static HRESULT WINAPI character_query(ICharacterReceivedEventArgs *iface, REFIID iid, void **out)
{ return args_query(CONTAINING_RECORD(iface, struct event_args, character), iid, out); }
static ULONG WINAPI character_addref(ICharacterReceivedEventArgs *iface)
{ return args_addref(CONTAINING_RECORD(iface, struct event_args, character)); }
static ULONG WINAPI character_release(ICharacterReceivedEventArgs *iface)
{ return args_release(CONTAINING_RECORD(iface, struct event_args, character)); }
static HRESULT WINAPI character_iids(ICharacterReceivedEventArgs *iface, ULONG *count, IID **iids) { return args_iids(count, iids); }
static HRESULT WINAPI character_class(ICharacterReceivedEventArgs *iface, HSTRING *out)
{ return args_class(CONTAINING_RECORD(iface, struct event_args, character), out); }
static HRESULT WINAPI character_trust(ICharacterReceivedEventArgs *iface, TrustLevel *out) { return args_trust(out); }

static HRESULT WINAPI input_query(IInputEnabledEventArgs *iface, REFIID iid, void **out)
{ return args_query(CONTAINING_RECORD(iface, struct event_args, input), iid, out); }
static ULONG WINAPI input_addref(IInputEnabledEventArgs *iface)
{ return args_addref(CONTAINING_RECORD(iface, struct event_args, input)); }
static ULONG WINAPI input_release(IInputEnabledEventArgs *iface)
{ return args_release(CONTAINING_RECORD(iface, struct event_args, input)); }
static HRESULT WINAPI input_iids(IInputEnabledEventArgs *iface, ULONG *count, IID **iids) { return args_iids(count, iids); }
static HRESULT WINAPI input_class(IInputEnabledEventArgs *iface, HSTRING *out)
{ return args_class(CONTAINING_RECORD(iface, struct event_args, input), out); }
static HRESULT WINAPI input_trust(IInputEnabledEventArgs *iface, TrustLevel *out) { return args_trust(out); }

static HRESULT WINAPI handled_get(ICoreWindowEventArgs *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = !!InterlockedCompareExchange(&CONTAINING_RECORD(iface, struct event_args, handled)->is_handled, 0, 0);
	return S_OK;
}
static HRESULT WINAPI handled_put(ICoreWindowEventArgs *iface, boolean value)
{
	InterlockedExchange(&CONTAINING_RECORD(iface, struct event_args, handled)->is_handled, !!value);
	return S_OK;
}
static HRESULT WINAPI size_get(IWindowSizeChangedEventArgs *iface, Size *out)
{
	if (!out) return E_POINTER;
	*out = CONTAINING_RECORD(iface, struct event_args, size)->dimensions;
	return S_OK;
}
static HRESULT WINAPI visible_get(IVisibilityChangedEventArgs *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = CONTAINING_RECORD(iface, struct event_args, visible)->flag;
	return S_OK;
}
static HRESULT WINAPI activated_get(IWindowActivatedEventArgs *iface, CoreWindowActivationState *out)
{
	if (!out) return E_POINTER;
	*out = CONTAINING_RECORD(iface, struct event_args, activated)->activation;
	return S_OK;
}
static HRESULT WINAPI key_get(IKeyEventArgs *iface, VirtualKey *out)
{
	if (!out) return E_POINTER;
	*out = (VirtualKey)CONTAINING_RECORD(iface, struct event_args, key)->keycode;
	return S_OK;
}
static HRESULT WINAPI key_status(IKeyEventArgs *iface, CorePhysicalKeyStatus *out)
{
	if (!out) return E_POINTER;
	*out = CONTAINING_RECORD(iface, struct event_args, key)->key_status;
	return S_OK;
}
static HRESULT WINAPI character_get(ICharacterReceivedEventArgs *iface, UINT32 *out)
{
	if (!out) return E_POINTER;
	*out = CONTAINING_RECORD(iface, struct event_args, character)->keycode;
	return S_OK;
}
static HRESULT WINAPI character_status(ICharacterReceivedEventArgs *iface, CorePhysicalKeyStatus *out)
{
	if (!out) return E_POINTER;
	*out = CONTAINING_RECORD(iface, struct event_args, character)->key_status;
	return S_OK;
}
static HRESULT WINAPI input_get(IInputEnabledEventArgs *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = CONTAINING_RECORD(iface, struct event_args, input)->flag;
	return S_OK;
}
static const ICoreWindowEventArgsVtbl handled_table = {
	handled_query, handled_addref, handled_release, handled_iids, handled_class, handled_trust, handled_get, handled_put
};
static const IWindowSizeChangedEventArgsVtbl size_table = {size_query, size_addref, size_release, size_iids, size_class, size_trust, size_get};
static const IVisibilityChangedEventArgsVtbl visible_table = {visible_query, visible_addref, visible_release, visible_iids, visible_class, visible_trust, visible_get};
static const IWindowActivatedEventArgsVtbl activated_table = {activated_query, activated_addref, activated_release, activated_iids, activated_class, activated_trust, activated_get};
static const IKeyEventArgsVtbl key_table = {key_query, key_addref, key_release, key_iids, key_class, key_trust, key_get, key_status};
static const ICharacterReceivedEventArgsVtbl character_table = {character_query, character_addref, character_release, character_iids, character_class, character_trust, character_get, character_status};
static const IInputEnabledEventArgsVtbl input_table = {input_query, input_addref, input_release, input_iids, input_class, input_trust, input_get};

static HRESULT last_error(void)
{
	DWORD error = GetLastError();
	return HRESULT_FROM_WIN32(error ? error : ERROR_GEN_FAILURE);
}

static HRESULT WINAPI pointer_query(IPointerEventArgs *iface, REFIID iid, void **out)
{
	return args_query(CONTAINING_RECORD(iface, struct event_args, pointer), iid, out);
}
static ULONG WINAPI pointer_addref(IPointerEventArgs *iface)
{
	return args_addref(CONTAINING_RECORD(iface, struct event_args, pointer));
}
static ULONG WINAPI pointer_release(IPointerEventArgs *iface)
{
	return args_release(CONTAINING_RECORD(iface, struct event_args, pointer));
}
static HRESULT WINAPI pointer_iids(IPointerEventArgs *iface, ULONG *count, IID **iids)
{
	return args_iids(count, iids);
}
static HRESULT WINAPI pointer_class(IPointerEventArgs *iface, HSTRING *out)
{
	return args_class(CONTAINING_RECORD(iface, struct event_args, pointer), out);
}
static HRESULT WINAPI pointer_trust(IPointerEventArgs *iface, TrustLevel *out)
{
	return args_trust(out);
}
static HRESULT WINAPI pointer_current(IPointerEventArgs *iface, IPointerPoint **out)
{
	if (!out) return E_POINTER;
	*out = CONTAINING_RECORD(iface, struct event_args, pointer)->point;
	IPointerPoint_AddRef(*out);
	return S_OK;
}
static HRESULT WINAPI pointer_modifiers(IPointerEventArgs *iface, VirtualKeyModifiers *out)
{
	if (!out) return E_POINTER;
	*out = CONTAINING_RECORD(iface, struct event_args, pointer)->modifiers;
	return S_OK;
}
static HRESULT WINAPI pointer_intermediate(IPointerEventArgs *iface, __FIVector_1_Windows__CUI__CInput__CPointerPoint **out)
{
	if (!out) return E_POINTER;
	/* The snapshot was taken when the args were built, so a retained args object answers the
	 * history it had then, not whatever the window ring holds now. */
	*out = CONTAINING_RECORD(iface, struct event_args, pointer)->history;
	if (*out) __FIVector_1_Windows__CUI__CInput__CPointerPoint_AddRef(*out);
	return S_OK;
}
static const IPointerEventArgsVtbl pointer_table = {
	pointer_query, pointer_addref, pointer_release, pointer_iids, pointer_class, pointer_trust,
	pointer_current, pointer_modifiers, pointer_intermediate
};

static float dip_scale(HWND hwnd)
{
	UINT dpi = GetDpiForWindow(hwnd);
	return dpi ? 96.0f / dpi : 1.0f;
}
static struct event_args *new_args(enum raven_window_event event, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	struct event_args *args = ALLOC(sizeof *args);
	if (!args) return NULL;
	args->handled.lpVtbl = &handled_table;
	args->size.lpVtbl = &size_table;
	args->visible.lpVtbl = &visible_table;
	args->activated.lpVtbl = &activated_table;
	args->key.lpVtbl = &key_table;
	args->character.lpVtbl = &character_table;
	args->input.lpVtbl = &input_table;
	args->event = event;
	args->refs = 1;
	args->flag = !!wparam;
	args->activation = LOWORD(wparam) == WA_INACTIVE ? CoreWindowActivationState_Deactivated :
		LOWORD(wparam) == WA_CLICKACTIVE ? CoreWindowActivationState_PointerActivated : CoreWindowActivationState_CodeActivated;
	args->keycode = (UINT32)wparam;
	UINT32 bits = (UINT32)(ULONG_PTR)lparam;
	args->key_status.RepeatCount = bits & 0xffff;
	args->key_status.ScanCode = (bits >> 16) & 0xff;
	args->key_status.IsExtendedKey = (bits >> 24) & 1;
	args->key_status.IsMenuKeyDown = (bits >> 29) & 1;
	args->key_status.WasKeyDown = (bits >> 30) & 1;
	args->key_status.IsKeyReleased = (bits >> 31) & 1;
	if (event == RAVEN_WINDOW_SIZE) {
		float scale = dip_scale(hwnd);
		args->dimensions.Width = LOWORD(lparam) * scale;
		args->dimensions.Height = HIWORD(lparam) * scale;
	}
	InterlockedIncrement(&raven_xaml_objects);
	return args;
}

static void record_error(struct raven_desktop_window *window, HRESULT error)
{
	if (FAILED(error)) InterlockedCompareExchange(&window->error, error, S_OK);
}
HRESULT raven_desktop_error(struct raven_desktop_window *window)
{
	return window ? (HRESULT)InterlockedExchange(&window->error, S_OK) : S_OK;
}

static BOOL fire_event_sample(struct raven_desktop_window *window, enum raven_window_event event, WPARAM wparam, LPARAM lparam,
	const struct raven_mouse_sample *sample)
{
	IUnknown **handlers;
	ULONG count;
	HRESULT result = raven_event_snapshot(&window->events[event], &handlers, &count);
	if (FAILED(result)) { record_error(window, result); return FALSE; }
	if (!count) return FALSE;
	struct event_args *args = new_args(event, window->hwnd, wparam, lparam);
	if (!args) {
		record_error(window, E_OUTOFMEMORY);
		for (ULONG i = 0; i < count; ++i) IUnknown_Release(handlers[i]);
		FREE(handlers);
		return FALSE;
	}
	if (sample) {
		args->pointer.lpVtbl = &pointer_table;
		args->modifiers = sample->modifiers;
		result = raven_mouse_point_create(sample, &args->point);
		if (FAILED(result)) {
			record_error(window, result);
			for (ULONG i = 0; i < count; ++i) IUnknown_Release(handlers[i]);
			args_release(args);
			FREE(handlers);
			return FALSE;
		}
		/* Intermediate points are the samples retained before the current one, oldest first; the
		 * ring already holds the current sample, so entries with frames at or above its frame drop. */
		struct raven_mouse_sample older[POINTER_HISTORY_CAP];
		UINT32 kept = 0;
		UINT32 start = (window->history_next + POINTER_HISTORY_CAP - window->history_count) % POINTER_HISTORY_CAP;
		for (UINT32 i = 0; i < window->history_count; ++i) {
			struct raven_mouse_sample *entry = &window->history[(start + i) % POINTER_HISTORY_CAP];
			if (entry->frame < sample->frame) older[kept++] = *entry;
		}
		result = raven_pointer_history_create(older, kept, &args->history);
		if (FAILED(result)) {
			record_error(window, result);
			for (ULONG i = 0; i < count; ++i) IUnknown_Release(handlers[i]);
			args_release(args);
			FREE(handlers);
			return FALSE;
		}
	}
	for (ULONG i = 0; i < count; ++i) {
#define CALL_EVENT(Name, field) do { \
	typedef __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__C##Name handler_type; \
	handler_type *handler = (void *)handlers[i]; \
	record_error(window, handler->lpVtbl->Invoke(handler, window->owner, &args->field)); \
} while (0)
		switch (event) {
		case RAVEN_WINDOW_ACTIVATED: CALL_EVENT(WindowActivatedEventArgs, activated); break;
		case RAVEN_WINDOW_CLOSED: CALL_EVENT(CoreWindowEventArgs, handled); break;
		case RAVEN_WINDOW_SIZE: CALL_EVENT(WindowSizeChangedEventArgs, size); break;
		case RAVEN_WINDOW_VISIBLE: CALL_EVENT(VisibilityChangedEventArgs, visible); break;
		case RAVEN_WINDOW_KEY_DOWN: case RAVEN_WINDOW_KEY_UP: CALL_EVENT(KeyEventArgs, key); break;
		case RAVEN_WINDOW_CHARACTER: CALL_EVENT(CharacterReceivedEventArgs, character); break;
		case RAVEN_WINDOW_INPUT_ENABLED: CALL_EVENT(InputEnabledEventArgs, input); break;
		case RAVEN_WINDOW_POINTER_ENTERED: case RAVEN_WINDOW_POINTER_EXITED: case RAVEN_WINDOW_POINTER_MOVED:
		case RAVEN_WINDOW_POINTER_PRESSED: case RAVEN_WINDOW_POINTER_RELEASED: case RAVEN_WINDOW_POINTER_WHEEL:
		case RAVEN_WINDOW_POINTER_CAPTURE_LOST: CALL_EVENT(PointerEventArgs, pointer); break;
		default: break;
		}
#undef CALL_EVENT
		IUnknown_Release(handlers[i]);
	}
	BOOL handled = args->is_handled || window->message_handled;
	args_release(args);
	FREE(handlers);
	return handled;
}

static BOOL fire_event(struct raven_desktop_window *window, enum raven_window_event event, WPARAM wparam, LPARAM lparam)
{
	return fire_event_sample(window, event, wparam, lparam, NULL);
}
static UINT64 pointer_timestamp(void)
{
	UINT64 now = GetTickCount64();
	UINT32 time = (UINT32)GetMessageTime();
	if (!time) return now * 1000;
	UINT64 extended = (now & ~0xffffffffULL) | time;
	if (extended > now) extended = extended >= (1ULL << 32) ? extended - (1ULL << 32) : now;
	return extended * 1000;
}
static BOOL window_mouse_event(struct raven_desktop_window *window, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct raven_mouse_sample sample = window->mouse;
	POINT position = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
	BOOL wheel = message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL;
	if (message == WM_MOUSELEAVE || message == WM_CAPTURECHANGED) {
		if (!GetCursorPos(&position) || !ScreenToClient(window->hwnd, &position)) {
			record_error(window, last_error());
			return FALSE;
		}
		sample.buttons &= ~(MK_CONTROL | MK_SHIFT);
		if (GetKeyState(VK_CONTROL) & 0x8000) sample.buttons |= MK_CONTROL;
		if (GetKeyState(VK_SHIFT) & 0x8000) sample.buttons |= MK_SHIFT;
	} else {
		sample.buttons = LOWORD(wparam);
		if (wheel && !ScreenToClient(window->hwnd, &position)) {
			record_error(window, last_error());
			return FALSE;
		}
	}
	FLOAT scale = dip_scale(window->hwnd);
	sample.position.X = position.x * scale;
	sample.position.Y = position.y * scale;
	sample.frame = (UINT32)InterlockedIncrement(&pointer_frame);
	sample.timestamp = pointer_timestamp();
	sample.modifiers = (sample.buttons & MK_CONTROL ? VirtualKeyModifiers_Control : 0) |
		(sample.buttons & MK_SHIFT ? VirtualKeyModifiers_Shift : 0) |
		(GetKeyState(VK_MENU) & 0x8000 ? VirtualKeyModifiers_Menu : 0) |
		((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000 ? VirtualKeyModifiers_Windows : 0);
	sample.wheel = wheel ? GET_WHEEL_DELTA_WPARAM(wparam) : 0;
	sample.horizontal = message == WM_MOUSEHWHEEL;
	sample.update = PointerUpdateKind_Other;
	enum raven_window_event event = RAVEN_WINDOW_POINTER_MOVED;
	switch (message) {
	case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
		sample.buttons |= MK_LBUTTON; sample.update = PointerUpdateKind_LeftButtonPressed; break;
	case WM_LBUTTONUP: sample.buttons &= ~MK_LBUTTON; sample.update = PointerUpdateKind_LeftButtonReleased; break;
	case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
		sample.buttons |= MK_RBUTTON; sample.update = PointerUpdateKind_RightButtonPressed; break;
	case WM_RBUTTONUP: sample.buttons &= ~MK_RBUTTON; sample.update = PointerUpdateKind_RightButtonReleased; break;
	case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
		sample.buttons |= MK_MBUTTON; sample.update = PointerUpdateKind_MiddleButtonPressed; break;
	case WM_MBUTTONUP: sample.buttons &= ~MK_MBUTTON; sample.update = PointerUpdateKind_MiddleButtonReleased; break;
	case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
		if (GET_XBUTTON_WPARAM(wparam) == XBUTTON1) { sample.buttons |= MK_XBUTTON1; sample.update = PointerUpdateKind_XButton1Pressed; }
		else { sample.buttons |= MK_XBUTTON2; sample.update = PointerUpdateKind_XButton2Pressed; }
		break;
	case WM_XBUTTONUP:
		if (GET_XBUTTON_WPARAM(wparam) == XBUTTON1) { sample.buttons &= ~MK_XBUTTON1; sample.update = PointerUpdateKind_XButton1Released; }
		else { sample.buttons &= ~MK_XBUTTON2; sample.update = PointerUpdateKind_XButton2Released; }
		break;
	case WM_MOUSELEAVE: window->pointer_inside = FALSE; event = RAVEN_WINDOW_POINTER_EXITED; break;
	case WM_CAPTURECHANGED: event = RAVEN_WINDOW_POINTER_CAPTURE_LOST; break;
	case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL: event = RAVEN_WINDOW_POINTER_WHEEL; break;
	}
	if (sample.update != PointerUpdateKind_Other)
		event = (sample.update & 1) ? RAVEN_WINDOW_POINTER_PRESSED : RAVEN_WINDOW_POINTER_RELEASED;
	window->mouse = sample;
	/* Every sample enters the ring before its event fires, so the args snapshot holds the samples
	 * retained before the current one; Enter and Move for the same message share one ring entry. */
	window->history[window->history_next] = sample;
	window->history_next = (window->history_next + 1) % POINTER_HISTORY_CAP;
	if (window->history_count < POINTER_HISTORY_CAP) ++window->history_count;
	if (message == WM_MOUSEMOVE && !window->pointer_inside) {
		window->pointer_inside = TRUE;
		TRACKMOUSEEVENT tracking = {sizeof tracking, TME_LEAVE, window->hwnd, 0};
		if (!TrackMouseEvent(&tracking)) record_error(window, last_error());
		fire_event_sample(window, RAVEN_WINDOW_POINTER_ENTERED, 0, 0, &sample);
		if (window->closing) return TRUE;
	}
	return fire_event_sample(window, event, 0, 0, &sample);
}

static BOOL character_event(struct raven_desktop_window *window, UINT32 code, LPARAM status)
{
	WCHAR high = window->high_surrogate;
	window->high_surrogate = 0;
	if (high && code >= 0xdc00 && code <= 0xdfff)
		code = 0x10000 + ((high - 0xd800) << 10) + code - 0xdc00;
	else {
		if (high) fire_event(window, RAVEN_WINDOW_CHARACTER, 0xfffd, status);
		if (code >= 0xd800 && code <= 0xdbff) {
			window->high_surrogate = (WCHAR)code;
			return TRUE;
		}
		if (code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff)) code = 0xfffd;
	}
	return fire_event(window, RAVEN_WINDOW_CHARACTER, code, status);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct raven_desktop_window *window = (void *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	if (message == WM_NCCREATE) {
		window = ((CREATESTRUCTW *)lparam)->lpCreateParams;
		window->hwnd = hwnd;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)window);
	}
	if (!window) return DefWindowProcW(hwnd, message, wparam, lparam);
	ICoreWindow_AddRef(window->owner);
	boolean previous_handled = window->message_handled;
	window->message_handled = FALSE;
	BOOL handled = FALSE;
	LRESULT result = 0;
	switch (message) {
	case WM_TIMER: {
		BOOL recognized = FALSE;
		record_error(window, raven_window_timer(window->owner, (UINT_PTR)wparam, &recognized));
		handled = recognized;
		break;
	}
	case WM_CLOSE:
		record_error(window, ICoreWindow_Close(window->owner));
		handled = TRUE;
		break;
	case WM_NCDESTROY:
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
		window->hwnd = NULL;
		if (!window->closing) record_error(window, ICoreWindow_Close(window->owner));
		break;
	case WM_SIZE:
		fire_event(window, RAVEN_WINDOW_SIZE, wparam, lparam);
		raven_composition_size(window->owner);
		break;
	case WM_SHOWWINDOW: fire_event(window, RAVEN_WINDOW_VISIBLE, wparam, lparam); break;
	case WM_ACTIVATE: fire_event(window, RAVEN_WINDOW_ACTIVATED, wparam, lparam); break;
	case WM_ENABLE:
		if (!wparam) {
			window->pointer_inside = FALSE;
			window->high_surrogate = 0;
			TRACKMOUSEEVENT tracking = {sizeof tracking, TME_CANCEL | TME_LEAVE, hwnd, 0};
			if (!TrackMouseEvent(&tracking)) record_error(window, last_error());
		}
		fire_event(window, RAVEN_WINDOW_INPUT_ENABLED, wparam, lparam);
		break;
	case WM_MOUSEMOVE: case WM_MOUSELEAVE: case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK: case WM_MBUTTONDOWN: case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK: case WM_XBUTTONDOWN: case WM_XBUTTONUP: case WM_XBUTTONDBLCLK:
	case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
		if (IsWindowEnabled(hwnd)) handled = window_mouse_event(window, message, wparam, lparam);
		if (message == WM_XBUTTONDOWN || message == WM_XBUTTONUP || message == WM_XBUTTONDBLCLK) {
			handled = TRUE; result = TRUE;
		}
		break;
	case WM_CAPTURECHANGED: handled = window_mouse_event(window, message, wparam, lparam); break;
	case WM_KEYDOWN: case WM_SYSKEYDOWN:
		if (IsWindowEnabled(hwnd)) handled = fire_event(window, RAVEN_WINDOW_KEY_DOWN, wparam, lparam);
		break;
	case WM_KEYUP: case WM_SYSKEYUP:
		if (IsWindowEnabled(hwnd)) handled = fire_event(window, RAVEN_WINDOW_KEY_UP, wparam, lparam);
		break;
	case WM_CHAR: case WM_SYSCHAR:
		if (IsWindowEnabled(hwnd)) handled = character_event(window, (UINT32)wparam, lparam);
		break;
	case WM_UNICHAR:
		if (wparam == UNICODE_NOCHAR) { handled = TRUE; result = TRUE; }
		else if (IsWindowEnabled(hwnd)) handled = character_event(window, (UINT32)wparam, lparam);
		break;
	}
	handled |= window->message_handled;
	window->message_handled = previous_handled;
	if (!handled) result = DefWindowProcW(hwnd, message, wparam, lparam);
	ICoreWindow_Release(window->owner);
	return result;
}

void raven_desktop_module(HINSTANCE instance, DWORD reason, void *reserved)
{
	if (reason == DLL_PROCESS_ATTACH) module_instance = instance;
	else if (reason == DLL_PROCESS_DETACH && !reserved) UnregisterClassW(window_class, module_instance);
}
HRESULT raven_desktop_create(ICoreWindow *owner, struct raven_desktop_window **out)
{
	*out = NULL;
	WNDCLASSW definition = {0};
	definition.hInstance = module_instance;
	definition.lpfnWndProc = window_proc;
	definition.lpszClassName = window_class;
	if (!RegisterClassW(&definition) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
		return last_error();
	struct raven_desktop_window *window = ALLOC(sizeof *window);
	if (!window) return E_OUTOFMEMORY;
	window->owner = owner;
	window->thread = GetCurrentThreadId();
	for (int i = 0; i < RAVEN_WINDOW_EVENT_COUNT; ++i) InitializeCriticalSection(&window->events[i].lock);
	RECT outer = {0, 0, 800, 600};
	AdjustWindowRectEx(&outer, WS_OVERLAPPEDWINDOW, FALSE, 0);
	HWND hwnd = CreateWindowExW(0, window_class, L"Raven", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, outer.right - outer.left, outer.bottom - outer.top,
		NULL, NULL, module_instance, window);
	if (!hwnd) {
		HRESULT result = last_error();
		for (int i = 0; i < RAVEN_WINDOW_EVENT_COUNT; ++i) DeleteCriticalSection(&window->events[i].lock);
		FREE(window);
		return result;
	}
	*out = window;
	return S_OK;
}
static HRESULT access_window(struct raven_desktop_window *window)
{
	if (window->thread != GetCurrentThreadId()) return RPC_E_WRONG_THREAD;
	return window->hwnd && !window->closing ? S_OK : RO_E_CLOSED;
}
HRESULT raven_desktop_handle(struct raven_desktop_window *window, HWND *out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	HRESULT result = access_window(window);
	if (SUCCEEDED(result)) *out = window->hwnd;
	return result;
}
HRESULT raven_desktop_bounds(struct raven_desktop_window *window, Rect *out)
{
	if (!out) return E_POINTER;
	HRESULT result = access_window(window);
	if (FAILED(result)) return result;
	RECT client, outer;
	POINT origin = {0};
	if (!GetClientRect(window->hwnd, &client) || !GetWindowRect(window->hwnd, &outer) || !ClientToScreen(window->hwnd, &origin))
		return last_error();
	float scale = dip_scale(window->hwnd);
	out->X = (origin.x - outer.left) * scale;
	out->Y = (origin.y - outer.top) * scale;
	out->Width = (client.right - client.left) * scale;
	out->Height = (client.bottom - client.top) * scale;
	return S_OK;
}
HRESULT raven_desktop_pointer_position(struct raven_desktop_window *window, Point *out)
{
	if (!out) return E_POINTER;
	HRESULT result = access_window(window);
	if (FAILED(result)) return result;
	POINT position;
	if (!GetCursorPos(&position) || !ScreenToClient(window->hwnd, &position)) return last_error();
	FLOAT scale = dip_scale(window->hwnd);
	out->X = position.x * scale;
	out->Y = position.y * scale;
	return S_OK;
}
HRESULT raven_desktop_capture(struct raven_desktop_window *window, BOOL capture)
{
	HRESULT result = access_window(window);
	if (FAILED(result)) return result;
	if (capture) {
		SetCapture(window->hwnd);
		if (GetCapture() != window->hwnd) return E_FAIL;
	} else if (GetCapture() == window->hwnd && !ReleaseCapture()) return last_error();
	return raven_desktop_error(window);
}

HRESULT raven_desktop_visible(struct raven_desktop_window *window, boolean *out)
{
	if (!out) return E_POINTER;
	HRESULT result = access_window(window);
	if (SUCCEEDED(result)) *out = IsWindowVisible(window->hwnd) != FALSE;
	return result;
}
HRESULT raven_desktop_activate(struct raven_desktop_window *window)
{
	HRESULT result = access_window(window);
	if (FAILED(result)) return result;
	SetLastError(ERROR_SUCCESS);
	ShowWindow(window->hwnd, SW_SHOW);
	if (!IsWindowVisible(window->hwnd)) {
		DWORD error = GetLastError();
		return HRESULT_FROM_WIN32(error ? error : ERROR_GEN_FAILURE);
	}
	SetForegroundWindow(window->hwnd);
	SetFocus(window->hwnd);
	return raven_desktop_error(window);
}
HRESULT raven_desktop_input(struct raven_desktop_window *window, boolean *out)
{
	if (!out) return E_POINTER;
	HRESULT result = access_window(window);
	if (SUCCEEDED(result)) *out = IsWindowEnabled(window->hwnd) != FALSE;
	return result;
}
HRESULT raven_desktop_put_input(struct raven_desktop_window *window, boolean enabled)
{
	HRESULT result = access_window(window);
	if (FAILED(result)) return result;
	EnableWindow(window->hwnd, !!enabled);
	if (!!IsWindowEnabled(window->hwnd) != !!enabled) return E_FAIL;
	return raven_desktop_error(window);
}
HRESULT raven_desktop_handled(struct raven_desktop_window *window, boolean handled)
{
	HRESULT result = access_window(window);
	if (SUCCEEDED(result)) window->message_handled = !!handled;
	return result;
}
HRESULT raven_desktop_add(struct raven_desktop_window *window, enum raven_window_event event, IUnknown *handler, EventRegistrationToken *token)
{
	return raven_event_add(&window->events[event], handler, token);
}
HRESULT raven_desktop_remove(struct raven_desktop_window *window, enum raven_window_event event, EventRegistrationToken token)
{
	return raven_event_remove(&window->events[event], token);
}
HRESULT raven_desktop_close(struct raven_desktop_window *window)
{
	if (window->thread != GetCurrentThreadId()) return RPC_E_WRONG_THREAD;
	if (window->closing) return S_OK;
	window->closing = TRUE;
	if (window->hwnd && !DestroyWindow(window->hwnd)) {
		window->closing = FALSE;
		return last_error();
	}
	if (!window->notified_closed) {
		window->notified_closed = TRUE;
		fire_event(window, RAVEN_WINDOW_CLOSED, 0, 0);
	}
	for (int i = 0; i < RAVEN_WINDOW_EVENT_COUNT; ++i) raven_event_close(&window->events[i]);
	return raven_desktop_error(window);
}
void raven_desktop_free(struct raven_desktop_window *window)
{
	if (!window) return;
	/* The HWND is destroyed by the owning UI thread before its TLS reference is dropped. */
	for (int i = 0; i < RAVEN_WINDOW_EVENT_COUNT; ++i) {
		raven_event_close(&window->events[i]);
		DeleteCriticalSection(&window->events[i].lock);
	}
	FREE(window);
}
