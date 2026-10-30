/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
#ifndef RAVEN_XAML_DESKTOP_WINDOW_H
#define RAVEN_XAML_DESKTOP_WINDOW_H
#include "events.h"
#include "xaml_internal.h"
enum raven_window_event {
	RAVEN_WINDOW_ACTIVATED, RAVEN_WINDOW_CLOSED, RAVEN_WINDOW_SIZE, RAVEN_WINDOW_VISIBLE,
	RAVEN_WINDOW_KEY_DOWN, RAVEN_WINDOW_KEY_UP, RAVEN_WINDOW_CHARACTER, RAVEN_WINDOW_INPUT_ENABLED,
	RAVEN_WINDOW_POINTER_ENTERED, RAVEN_WINDOW_POINTER_EXITED, RAVEN_WINDOW_POINTER_MOVED,
	RAVEN_WINDOW_POINTER_PRESSED, RAVEN_WINDOW_POINTER_RELEASED, RAVEN_WINDOW_POINTER_WHEEL,
	RAVEN_WINDOW_POINTER_CAPTURE_LOST, RAVEN_WINDOW_EVENT_COUNT
};
struct raven_desktop_window;
HRESULT raven_desktop_create(__x_ABI_CWindows_CUI_CCore_CICoreWindow *owner, struct raven_desktop_window **out);
HRESULT raven_desktop_close(struct raven_desktop_window *window);
void raven_desktop_free(struct raven_desktop_window *window);
HRESULT raven_desktop_handle(struct raven_desktop_window *window, HWND *out);
HRESULT raven_desktop_bounds(struct raven_desktop_window *window, __x_ABI_CWindows_CFoundation_CRect *out);
HRESULT raven_desktop_pointer_position(struct raven_desktop_window *window, __x_ABI_CWindows_CFoundation_CPoint *out);
HRESULT raven_desktop_capture(struct raven_desktop_window *window, BOOL capture);
HRESULT raven_desktop_visible(struct raven_desktop_window *window, boolean *out);
HRESULT raven_desktop_activate(struct raven_desktop_window *window);
HRESULT raven_desktop_input(struct raven_desktop_window *window, boolean *out);
HRESULT raven_desktop_put_input(struct raven_desktop_window *window, boolean enabled);
HRESULT raven_desktop_error(struct raven_desktop_window *window);
HRESULT raven_desktop_handled(struct raven_desktop_window *window, boolean handled);
HRESULT raven_desktop_add(struct raven_desktop_window *window, enum raven_window_event event, IUnknown *handler, EventRegistrationToken *token);
HRESULT raven_desktop_remove(struct raven_desktop_window *window, enum raven_window_event event, EventRegistrationToken token);
void raven_desktop_module(HINSTANCE instance, DWORD reason, void *reserved);
#endif
