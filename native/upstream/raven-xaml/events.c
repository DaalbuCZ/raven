/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "events.h"
#include "xaml_internal.h"
#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))
#define RO_E_CLOSED ((HRESULT)0x80000013)
static LONG token_id;
struct raven_registration {
	struct raven_registration *next;
	EventRegistrationToken token;
	IUnknown *handler;
};

HRESULT raven_event_add(struct raven_event_list *list, IUnknown *handler, EventRegistrationToken *token)
{
	if (!token) return E_POINTER;
	token->value = 0;
	if (!handler) return E_POINTER;
	struct raven_registration *node = ALLOC(sizeof *node);
	if (!node) return E_OUTOFMEMORY;
	IUnknown_AddRef(handler);
	node->handler = handler;
	node->token.value = InterlockedIncrement(&token_id);
	EnterCriticalSection(&list->lock);
	if (list->closed) {
		LeaveCriticalSection(&list->lock);
		IUnknown_Release(handler);
		FREE(node);
		return RO_E_CLOSED;
	}
	struct raven_registration **position = &list->head;
	while (*position) position = &(*position)->next;
	*position = node;
	*token = node->token;
	InterlockedIncrement(&raven_xaml_objects);
	LeaveCriticalSection(&list->lock);
	return S_OK;
}
HRESULT raven_event_remove(struct raven_event_list *list, EventRegistrationToken token)
{
	struct raven_registration *removed = NULL;
	EnterCriticalSection(&list->lock);
	struct raven_registration **position = &list->head;
	while (*position && (*position)->token.value != token.value) position = &(*position)->next;
	if (*position) {
		removed = *position;
		*position = removed->next;
	}
	LeaveCriticalSection(&list->lock);
	if (removed) {
		IUnknown_Release(removed->handler);
		FREE(removed);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return S_OK;
}
void raven_event_close(struct raven_event_list *list)
{
	EnterCriticalSection(&list->lock);
	struct raven_registration *node = list->head;
	list->head = NULL;
	list->closed = TRUE;
	LeaveCriticalSection(&list->lock);
	while (node) {
		struct raven_registration *next = node->next;
		IUnknown_Release(node->handler);
		FREE(node);
		InterlockedDecrement(&raven_xaml_objects);
		node = next;
	}
}
/* Callbacks can revoke themselves or register another handler without invalidating this dispatch. */
HRESULT raven_event_snapshot(struct raven_event_list *list, IUnknown ***handlers, ULONG *count)
{
	*handlers = NULL;
	*count = 0;
	EnterCriticalSection(&list->lock);
	for (struct raven_registration *node = list->head; node; node = node->next) ++*count;
	if (*count) {
		*handlers = ALLOC(*count * sizeof **handlers);
		if (!*handlers) {
			LeaveCriticalSection(&list->lock);
			return E_OUTOFMEMORY;
		}
		ULONG index = 0;
		for (struct raven_registration *node = list->head; node; node = node->next) {
			(*handlers)[index++] = node->handler;
			IUnknown_AddRef(node->handler);
		}
	}
	LeaveCriticalSection(&list->lock);
	return S_OK;
}

