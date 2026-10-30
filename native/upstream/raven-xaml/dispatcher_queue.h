/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
#ifndef RAVEN_XAML_DISPATCHER_QUEUE_H
#define RAVEN_XAML_DISPATCHER_QUEUE_H
#include <windows.system.h>
#include "xaml_internal.h"
HRESULT raven_dispatcher_queue_factory(HSTRING name, IActivationFactory **out);
HRESULT raven_dispatcher_queue_create(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window, HWND hwnd,
	__x_ABI_CWindows_CSystem_CIDispatcherQueue **out);
HRESULT raven_dispatcher_queue_close(__x_ABI_CWindows_CSystem_CIDispatcherQueue *queue);
HRESULT raven_dispatcher_queue_timer(__x_ABI_CWindows_CSystem_CIDispatcherQueue *queue, UINT_PTR id, BOOL *handled);
#endif
