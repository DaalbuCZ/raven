/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
#ifndef RAVEN_XAML_EVENTS_H
#define RAVEN_XAML_EVENTS_H
#include <windows.h>
#include <windows.foundation.h>
struct raven_registration;
struct raven_event_list {
	CRITICAL_SECTION lock;
	struct raven_registration *head;
	BOOL closed;
};
HRESULT raven_event_add(struct raven_event_list *list, IUnknown *handler, EventRegistrationToken *token);
HRESULT raven_event_remove(struct raven_event_list *list, EventRegistrationToken token);
void raven_event_close(struct raven_event_list *list);
HRESULT raven_event_snapshot(struct raven_event_list *list, IUnknown ***handlers, ULONG *count);
#endif
