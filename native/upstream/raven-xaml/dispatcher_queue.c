/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
#define INITGUID
#define CONST_VTABLE
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define WIDL_using_Windows_System
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_Foundation
#include <windows.h>
#include <winstring.h>
#include "dispatcher_queue.h"
#include "events.h"
#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))
#define RO_E_CLOSED ((HRESULT)0x80000013)
#define E_ILLEGAL_METHOD_CALL ((HRESULT)0x8000000e)
#define E_ILLEGAL_DELEGATE_ASSIGNMENT ((HRESULT)0x80000018)

typedef __FITypedEventHandler_2_Windows__CSystem__CDispatcherQueueTimer_IInspectable tick_handler;
typedef __FITypedEventHandler_2_Windows__CSystem__CDispatcherQueue_Windows__CSystem__CDispatcherQueueShutdownStartingEventArgs starting_handler;
typedef __FITypedEventHandler_2_Windows__CSystem__CDispatcherQueue_IInspectable completed_handler;
static const WCHAR queue_name[] = L"Windows.System.DispatcherQueue";
static const WCHAR controller_name[] = L"Windows.System.DispatcherQueueController";
static const WCHAR timer_name[] = L"Windows.System.DispatcherQueueTimer";
static LONG next_timer_id, next_action_id;
struct timer;
struct queue {
	IDispatcherQueue iface;
	IDispatcherQueue2 access;
	LONG refs;
	DWORD thread;
	CRITICAL_SECTION lock;
	BOOL closed;
	BOOL shutdown_started;
	/* Deferred controller shutdown: the count of outstanding ShutdownStarting deferrals keeps the queue open and
	 * close_deferred records that the close work ran while one was outstanding, so the last Complete re-posts it. */
	int shutdown_deferrals;
	BOOL close_deferred;
	ICoreWindow *window;
	HWND hwnd;
	struct timer *timers;
	struct raven_event_list starting, completed;
};
struct timer {
	IDispatcherQueueTimer iface;
	LONG refs;
	struct queue *queue;
	struct timer *next;
	TimeSpan interval;
	BOOL running, repeating, queued;
	UINT_PTR id;
	UINT64 generation, due;
	struct raven_event_list tick;
};
struct work {
	IDispatchedHandler iface;
	LONG refs;
	IDispatcherQueueHandler *callback;
	struct timer *timer;
	UINT64 generation;
};
static HRESULT no_iids(ULONG *count, IID **iids)
{
	if (!count || !iids) return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}
static HRESULT class_string(const WCHAR *name, HSTRING *out)
{
	if (!out) return E_POINTER;
	return WindowsCreateString(name, (UINT32)lstrlenW(name), out);
}
static HRESULT trust(TrustLevel *out)
{
	if (!out) return E_POINTER;
	*out = BaseTrust;
	return S_OK;
}
static HRESULT last_error(void)
{
	DWORD error = GetLastError();
	return HRESULT_FROM_WIN32(error ? error : ERROR_GEN_FAILURE);
}
static HRESULT WINAPI queue_query(IDispatcherQueue *iface, REFIID iid, void **out);
static ULONG WINAPI queue_addref(IDispatcherQueue *iface);
static ULONG WINAPI queue_release(IDispatcherQueue *iface);
static ULONG WINAPI timer_addref(IDispatcherQueueTimer *iface);
static ULONG WINAPI timer_release(IDispatcherQueueTimer *iface);

static ULONG WINAPI queue_addref(IDispatcherQueue *iface) { return InterlockedIncrement(&((struct queue *)iface)->refs); }
static ULONG WINAPI queue_release(IDispatcherQueue *iface)
{
	struct queue *self = (void *)iface;
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		raven_event_close(&self->starting);
		raven_event_close(&self->completed);
		DeleteCriticalSection(&self->starting.lock);
		DeleteCriticalSection(&self->completed.lock);
		DeleteCriticalSection(&self->lock);
		ICoreWindow_Release(self->window);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return refs;
}
static HRESULT WINAPI queue_query(IDispatcherQueue *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID_IDispatcherQueue) || IsEqualIID(iid, &IID_IAgileObject)) *out = iface;
	else if (IsEqualIID(iid, &IID_IDispatcherQueue2)) *out = &((struct queue *)iface)->access;
	else return E_NOINTERFACE;
	queue_addref(iface);
	return S_OK;
}
static HRESULT WINAPI queue_iids(IDispatcherQueue *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI queue_class(IDispatcherQueue *iface, HSTRING *out) { return class_string(queue_name, out); }
static HRESULT WINAPI queue_trust(IDispatcherQueue *iface, TrustLevel *out) { return trust(out); }
static struct queue *access_owner(IDispatcherQueue2 *iface) { return CONTAINING_RECORD(iface, struct queue, access); }
static HRESULT WINAPI access_query(IDispatcherQueue2 *iface, REFIID iid, void **out) { return queue_query(&access_owner(iface)->iface, iid, out); }
static ULONG WINAPI access_addref(IDispatcherQueue2 *iface) { return queue_addref(&access_owner(iface)->iface); }
static ULONG WINAPI access_release(IDispatcherQueue2 *iface) { return queue_release(&access_owner(iface)->iface); }
static HRESULT WINAPI access_iids(IDispatcherQueue2 *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI access_class(IDispatcherQueue2 *iface, HSTRING *out) { return class_string(queue_name, out); }
static HRESULT WINAPI access_trust(IDispatcherQueue2 *iface, TrustLevel *out) { return trust(out); }
static HRESULT WINAPI access_thread(IDispatcherQueue2 *iface, boolean *out)
{
	if (!out) return E_POINTER;
	*out = access_owner(iface)->thread == GetCurrentThreadId();
	return S_OK;
}
static const IDispatcherQueue2Vtbl access_table = {access_query, access_addref, access_release, access_iids, access_class, access_trust, access_thread};

static ULONG WINAPI timer_addref(IDispatcherQueueTimer *iface) { return InterlockedIncrement(&((struct timer *)iface)->refs); }
static ULONG WINAPI timer_release(IDispatcherQueueTimer *iface)
{
	struct timer *self = (void *)iface;
	struct queue *queue = self->queue;
	/* List lookup and the final decrement share the lock; a timer cannot be resurrected while being removed. */
	EnterCriticalSection(&queue->lock);
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		struct timer **position = &queue->timers;
		while (*position && *position != self) position = &(*position)->next;
		if (*position) *position = self->next;
	}
	LeaveCriticalSection(&queue->lock);
	if (!refs) {
		raven_event_close(&self->tick);
		DeleteCriticalSection(&self->tick.lock);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
		queue_release(&queue->iface);
	}
	return refs;
}
static HRESULT WINAPI timer_query(IDispatcherQueueTimer *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) && !IsEqualIID(iid, &IID_IDispatcherQueueTimer) && !IsEqualIID(iid, &IID_IAgileObject)) return E_NOINTERFACE;
	*out = iface;
	timer_addref(iface);
	return S_OK;
}
static HRESULT WINAPI timer_iids(IDispatcherQueueTimer *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI timer_class(IDispatcherQueueTimer *iface, HSTRING *out) { return class_string(timer_name, out); }
static HRESULT WINAPI timer_trust(IDispatcherQueueTimer *iface, TrustLevel *out) { return trust(out); }
static UINT64 interval_ms(struct timer *self)
{
	return (UINT64)(self->interval.Duration / 10000) + (self->interval.Duration % 10000 != 0);
}
/* The Win32 timer is only a wakeup; the stored deadline prevents early ticks, including intervals beyond its maximum period. */
static HRESULT timer_arm(struct timer *self, BOOL restart)
{
	struct queue *queue = self->queue;
	UINT64 milliseconds = interval_ms(self), now = GetTickCount64();
	if (milliseconds > ~(UINT64)0 - now) { OutputDebugStringA("raven-xaml: dispatcher delay overflow\n"); return E_INVALIDARG; }
	UINT_PTR id = 0x40000000U | (UINT32)InterlockedIncrement(&next_timer_id);
	UINT delay = milliseconds > USER_TIMER_MAXIMUM ? USER_TIMER_MAXIMUM : (UINT)milliseconds;
	if (!SetTimer(queue->hwnd, id, delay, NULL)) return last_error();
	if (self->id) KillTimer(queue->hwnd, self->id);
	self->id = id;
	self->due = now + milliseconds;
	if (restart) { ++self->generation; self->queued = FALSE; }
	if (!self->running) { self->running = TRUE; timer_addref(&self->iface); }
	return S_OK;
}
static BOOL timer_stop_locked(struct timer *self)
{
	BOOL was_running = self->running;
	if (self->id) KillTimer(self->queue->hwnd, self->id);
	self->id = 0;
	self->running = self->queued = FALSE;
	++self->generation;
	return was_running;
}
static HRESULT WINAPI timer_interval(IDispatcherQueueTimer *iface, TimeSpan *out)
{
	if (!out) return E_POINTER;
	struct timer *self = (void *)iface;
	EnterCriticalSection(&self->queue->lock);
	*out = self->interval;
	LeaveCriticalSection(&self->queue->lock);
	return S_OK;
}
static HRESULT WINAPI timer_put_interval(IDispatcherQueueTimer *iface, TimeSpan value)
{
	if (value.Duration < 0) { OutputDebugStringA("raven-xaml: dispatcher negative delay\n"); return E_INVALIDARG; }
	struct timer *self = (void *)iface;
	EnterCriticalSection(&self->queue->lock);
	TimeSpan previous = self->interval;
	self->interval = value;
	HRESULT result = self->running ? timer_arm(self, TRUE) : S_OK;
	if (FAILED(result)) self->interval = previous;
	LeaveCriticalSection(&self->queue->lock);
	return result;
}
static HRESULT WINAPI timer_running(IDispatcherQueueTimer *iface, boolean *out)
{
	if (!out) return E_POINTER;
	struct timer *self = (void *)iface;
	EnterCriticalSection(&self->queue->lock);
	*out = self->running;
	LeaveCriticalSection(&self->queue->lock);
	return S_OK;
}
static HRESULT WINAPI timer_repeating(IDispatcherQueueTimer *iface, boolean *out)
{
	if (!out) return E_POINTER;
	struct timer *self = (void *)iface;
	EnterCriticalSection(&self->queue->lock);
	*out = self->repeating;
	LeaveCriticalSection(&self->queue->lock);
	return S_OK;
}
static HRESULT WINAPI timer_put_repeating(IDispatcherQueueTimer *iface, boolean value)
{
	struct timer *self = (void *)iface;
	EnterCriticalSection(&self->queue->lock);
	BOOL previous = self->repeating;
	self->repeating = !!value;
	HRESULT result = self->running ? timer_arm(self, TRUE) : S_OK;
	if (FAILED(result)) self->repeating = previous;
	LeaveCriticalSection(&self->queue->lock);
	return result;
}
static HRESULT WINAPI timer_start(IDispatcherQueueTimer *iface)
{
	struct timer *self = (void *)iface;
	EnterCriticalSection(&self->queue->lock);
	HRESULT result = self->queue->closed ? RO_E_CLOSED : timer_arm(self, TRUE);
	LeaveCriticalSection(&self->queue->lock);
	return result;
}
static HRESULT WINAPI timer_stop(IDispatcherQueueTimer *iface)
{
	struct timer *self = (void *)iface;
	EnterCriticalSection(&self->queue->lock);
	BOOL active = timer_stop_locked(self);
	LeaveCriticalSection(&self->queue->lock);
	if (active) timer_release(iface);
	return S_OK;
}
static HRESULT WINAPI timer_add_tick(IDispatcherQueueTimer *iface, tick_handler *handler, EventRegistrationToken *token)
{
	return raven_event_add(&((struct timer *)iface)->tick, (IUnknown *)handler, token);
}
static HRESULT WINAPI timer_remove_tick(IDispatcherQueueTimer *iface, EventRegistrationToken token)
{
	return raven_event_remove(&((struct timer *)iface)->tick, token);
}
static const IDispatcherQueueTimerVtbl timer_table = {
	timer_query, timer_addref, timer_release, timer_iids, timer_class, timer_trust,
	timer_interval, timer_put_interval, timer_running, timer_repeating, timer_put_repeating, timer_start, timer_stop, timer_add_tick, timer_remove_tick
};
static HRESULT WINAPI queue_create_timer(IDispatcherQueue *iface, IDispatcherQueueTimer **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	struct queue *self = (void *)iface;
	struct timer *timer = ALLOC(sizeof *timer);
	if (!timer) return E_OUTOFMEMORY;
	timer->iface.lpVtbl = &timer_table;
	timer->refs = 1;
	timer->repeating = TRUE;
	timer->queue = self;
	InitializeCriticalSection(&timer->tick.lock);
	EnterCriticalSection(&self->lock);
	if (self->closed) {
		LeaveCriticalSection(&self->lock);
		DeleteCriticalSection(&timer->tick.lock);
		FREE(timer);
		return RO_E_CLOSED;
	}
	queue_addref(iface);
	timer->next = self->timers;
	self->timers = timer;
	InterlockedIncrement(&raven_xaml_objects);
	LeaveCriticalSection(&self->lock);
	*out = &timer->iface;
	return S_OK;
}

static HRESULT notify_tick(struct timer *timer)
{
	IUnknown **handlers;
	ULONG count;
	HRESULT result = raven_event_snapshot(&timer->tick, &handlers, &count);
	if (FAILED(result)) return result;
	for (ULONG i = 0; i < count; ++i) {
		if (SUCCEEDED(result)) result = ((tick_handler *)handlers[i])->lpVtbl->Invoke((tick_handler *)handlers[i], &timer->iface, NULL);
		IUnknown_Release(handlers[i]);
	}
	FREE(handlers);
	return result;
}
static HRESULT WINAPI work_query(IDispatchedHandler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IDispatchedHandler)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct work *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI work_addref(IDispatchedHandler *iface) { return InterlockedIncrement(&((struct work *)iface)->refs); }
static ULONG WINAPI work_release(IDispatchedHandler *iface)
{
	struct work *self = (void *)iface;
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->callback) IDispatcherQueueHandler_Release(self->callback);
		if (self->timer) timer_release(&self->timer->iface);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return refs;
}
static HRESULT WINAPI work_invoke(IDispatchedHandler *iface)
{
	struct work *self = (void *)iface;
	if (self->callback) return IDispatcherQueueHandler_Invoke(self->callback);
	struct timer *timer = self->timer;
	struct queue *queue = timer->queue;
	BOOL release_active = FALSE;
	HRESULT result = S_OK;
	EnterCriticalSection(&queue->lock);
	if (queue->closed || !timer->running || timer->generation != self->generation) {
		LeaveCriticalSection(&queue->lock);
		return S_OK;
	}
	timer->queued = FALSE;
	if (timer->repeating) {
		result = timer_arm(timer, FALSE);
		if (FAILED(result)) release_active = timer_stop_locked(timer);
	} else release_active = timer_stop_locked(timer);
	LeaveCriticalSection(&queue->lock);
	if (release_active) timer_release(&timer->iface);
	HRESULT notified = notify_tick(timer);
	return FAILED(result) ? result : notified;
}
static const IDispatchedHandlerVtbl work_table = {work_query, work_addref, work_release, work_invoke};
static struct work *new_work(void)
{
	struct work *work = ALLOC(sizeof *work);
	if (work) {
		work->iface.lpVtbl = &work_table;
		work->refs = 1;
		InterlockedIncrement(&raven_xaml_objects);
	}
	return work;
}
static HRESULT WINAPI queue_enqueue_priority(IDispatcherQueue *iface, DispatcherQueuePriority priority,
	IDispatcherQueueHandler *callback, boolean *accepted)
{
	if (!accepted) return E_POINTER;
	*accepted = FALSE;
	if (!callback) return E_POINTER;
	LONG rank;
	switch (priority) {
	case DispatcherQueuePriority_Low: rank = CoreDispatcherPriority_Low; break;
	case DispatcherQueuePriority_Normal: rank = CoreDispatcherPriority_Normal; break;
	case DispatcherQueuePriority_High: rank = CoreDispatcherPriority_High; break;
	default: OutputDebugStringA("raven-xaml: dispatcher priority refused\n"); return E_INVALIDARG;
	}
	struct work *work = new_work();
	if (!work) return E_OUTOFMEMORY;
	struct queue *self = (void *)iface;
	/* A closed queue rejects work even while its window still exists, as a closed window does. */
	EnterCriticalSection(&self->lock);
	BOOL closed = self->closed;
	LeaveCriticalSection(&self->lock);
	if (closed) {
		work_release(&work->iface);
		return S_OK;
	}
	work->callback = callback;
	IDispatcherQueueHandler_AddRef(callback);
	HRESULT result = raven_window_post(self->window, &work->iface, rank, accepted);
	work_release(&work->iface);
	return result;
}
static HRESULT WINAPI queue_enqueue(IDispatcherQueue *iface, IDispatcherQueueHandler *callback, boolean *accepted)
{
	return queue_enqueue_priority(iface, DispatcherQueuePriority_Normal, callback, accepted);
}

HRESULT raven_dispatcher_queue_timer(IDispatcherQueue *iface, UINT_PTR id, BOOL *handled)
{
	*handled = FALSE;
	if (!iface) return S_OK;
	struct queue *queue = (void *)iface;
	BOOL release_active = FALSE;
	HRESULT result = S_OK;
	EnterCriticalSection(&queue->lock);
	struct timer *timer = queue->timers;
	while (timer && timer->id != id) timer = timer->next;
	if (!timer || queue->closed || !timer->running) {
		LeaveCriticalSection(&queue->lock);
		return S_OK;
	}
	*handled = TRUE;
	UINT64 now = GetTickCount64();
	if (now < timer->due) {
		UINT64 remaining = timer->due - now;
		if (!SetTimer(queue->hwnd, id, remaining > USER_TIMER_MAXIMUM ? USER_TIMER_MAXIMUM : (UINT)remaining, NULL)) {
			result = last_error();
			release_active = timer_stop_locked(timer);
		}
		LeaveCriticalSection(&queue->lock);
		if (release_active) timer_release(&timer->iface);
		return result;
	}
	if (timer->queued) { LeaveCriticalSection(&queue->lock); return S_OK; }
	struct work *work = new_work();
	if (!work) { LeaveCriticalSection(&queue->lock); return E_OUTOFMEMORY; }
	timer_addref(&timer->iface);
	work->timer = timer;
	work->generation = timer->generation;
	timer->queued = TRUE;
	KillTimer(queue->hwnd, id);
	timer->id = 0;
	boolean accepted = FALSE;
	/* Timer tasks follow even CoreDispatcherPriority_Idle work, as the WinRT contract requires. */
	result = raven_window_post(queue->window, &work->iface, CoreDispatcherPriority_Idle - 1, &accepted);
	if (FAILED(result) || !accepted) release_active = timer_stop_locked(timer);
	LeaveCriticalSection(&queue->lock);
	if (release_active) timer_release(&timer->iface);
	work_release(&work->iface);
	return result;
}

struct shutdown_args { IDispatcherQueueShutdownStartingEventArgs iface; LONG refs; struct queue *queue; BOOL deferral_ok; };
/* Built with the deferral object section below; forward-declared because the args vtable needs it first. */
static HRESULT queue_deferral_create(struct queue *queue, IDeferral **out);
static HRESULT WINAPI shutdown_query(IDispatcherQueueShutdownStartingEventArgs *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) && !IsEqualIID(iid, &IID_IDispatcherQueueShutdownStartingEventArgs)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct shutdown_args *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI shutdown_addref(IDispatcherQueueShutdownStartingEventArgs *iface) { return InterlockedIncrement(&((struct shutdown_args *)iface)->refs); }
static ULONG WINAPI shutdown_release(IDispatcherQueueShutdownStartingEventArgs *iface)
{
	LONG refs = InterlockedDecrement(&((struct shutdown_args *)iface)->refs);
	if (!refs) { FREE(iface); InterlockedDecrement(&raven_xaml_objects); }
	return refs;
}
static HRESULT WINAPI shutdown_iids(IDispatcherQueueShutdownStartingEventArgs *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI shutdown_class(IDispatcherQueueShutdownStartingEventArgs *iface, HSTRING *out) { return class_string(L"Windows.System.DispatcherQueueShutdownStartingEventArgs", out); }
static HRESULT WINAPI shutdown_trust(IDispatcherQueueShutdownStartingEventArgs *iface, TrustLevel *out) { return trust(out); }
static HRESULT WINAPI shutdown_get_deferral(IDispatcherQueueShutdownStartingEventArgs *iface, IDeferral **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	struct shutdown_args *self = (void *)iface;
	/* Refusal: the immediate CoreWindow-owned close (raven_dispatcher_queue_close) finishes synchronously on the
	 * window thread and cannot wait for a deferral. Deferrals are honored on the controller's own pump, where the
	 * close work is an ordinary queued item that can be re-posted. */
	if (!self->deferral_ok) { raven_trace_not_implemented(__func__); return E_NOTIMPL; }
	return queue_deferral_create(self->queue, out);
}
static const IDispatcherQueueShutdownStartingEventArgsVtbl shutdown_table = {
	shutdown_query, shutdown_addref, shutdown_release, shutdown_iids, shutdown_class, shutdown_trust, shutdown_get_deferral
};
static HRESULT notify_shutdown(struct queue *queue, BOOL starting, BOOL deferral_ok)
{
	IUnknown **handlers;
	ULONG count;
	HRESULT result = raven_event_snapshot(starting ? &queue->starting : &queue->completed, &handlers, &count);
	if (FAILED(result)) return result;
	struct shutdown_args *args = NULL;
	if (starting && count) {
		args = ALLOC(sizeof *args);
		if (!args) result = E_OUTOFMEMORY;
		else {
			args->iface.lpVtbl = &shutdown_table;
			args->refs = 1;
			args->queue = queue;
			args->deferral_ok = deferral_ok;
			InterlockedIncrement(&raven_xaml_objects);
		}
	}
	for (ULONG i = 0; i < count; ++i) {
		if (SUCCEEDED(result)) {
			if (starting) result = ((starting_handler *)handlers[i])->lpVtbl->Invoke((starting_handler *)handlers[i], &queue->iface, &args->iface);
			else result = ((completed_handler *)handlers[i])->lpVtbl->Invoke((completed_handler *)handlers[i], &queue->iface, NULL);
		}
		IUnknown_Release(handlers[i]);
	}
	FREE(handlers);
	if (args) shutdown_release(&args->iface);
	return result;
}

/* A ShutdownStarting deferral: outstanding deferrals keep the controller queue open so the handler can keep posting
 * work; the last Complete re-posts the close work below every client priority. The deferral owns a queue reference,
 * so a deferral held past ShutdownQueueAsync cannot free the queue out from under the drain. */
static void post_close_work(struct queue *queue);
struct queue_deferral {
	IDeferral iface;
	LONG refs;
	struct queue *queue;
	BOOL completed;
};
static HRESULT WINAPI deferral_query(IDeferral *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) &&
		!IsEqualIID(iid, &IID___x_ABI_CWindows_CFoundation_CIDeferral)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct queue_deferral *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI deferral_addref(IDeferral *iface) { return InterlockedIncrement(&((struct queue_deferral *)iface)->refs); }
static ULONG WINAPI deferral_release(IDeferral *iface)
{
	struct queue_deferral *self = (void *)iface;
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		IDispatcherQueue_Release(&self->queue->iface);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return refs;
}
static HRESULT WINAPI deferral_iids(IDeferral *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI deferral_class(IDeferral *iface, HSTRING *out) { return class_string(L"Windows.Foundation.Deferral", out); }
static HRESULT WINAPI deferral_trust(IDeferral *iface, TrustLevel *out) { return trust(out); }
static HRESULT WINAPI deferral_complete(IDeferral *iface)
{
	struct queue_deferral *self = (void *)iface;
	struct queue *queue = self->queue;
	/* Complete is idempotent: only the first call counts against the queue's outstanding-deferral tally. */
	BOOL post = FALSE;
	EnterCriticalSection(&queue->lock);
	if (!self->completed) {
		self->completed = TRUE;
		if (queue->shutdown_deferrals && !--queue->shutdown_deferrals && queue->close_deferred && !queue->closed) {
			queue->close_deferred = FALSE;
			post = TRUE;
		}
	}
	LeaveCriticalSection(&queue->lock);
	if (post) post_close_work(queue);
	return S_OK;
}
static const IDeferralVtbl deferral_table = {
	deferral_query, deferral_addref, deferral_release, deferral_iids, deferral_class, deferral_trust, deferral_complete
};
static HRESULT queue_deferral_create(struct queue *queue, IDeferral **out)
{
	struct queue_deferral *deferral = ALLOC(sizeof *deferral);
	if (!deferral) return E_OUTOFMEMORY;
	deferral->iface.lpVtbl = &deferral_table;
	deferral->refs = 1;
	deferral->completed = FALSE;
	deferral->queue = queue;
	IDispatcherQueue_AddRef(&queue->iface);
	EnterCriticalSection(&queue->lock);
	++queue->shutdown_deferrals;
	LeaveCriticalSection(&queue->lock);
	InterlockedIncrement(&raven_xaml_objects);
	*out = &deferral->iface;
	return S_OK;
}
static HRESULT WINAPI queue_add_starting(IDispatcherQueue *iface, starting_handler *handler, EventRegistrationToken *token)
{
	return raven_event_add(&((struct queue *)iface)->starting, (IUnknown *)handler, token);
}
static HRESULT WINAPI queue_remove_starting(IDispatcherQueue *iface, EventRegistrationToken token)
{
	return raven_event_remove(&((struct queue *)iface)->starting, token);
}
static HRESULT WINAPI queue_add_completed(IDispatcherQueue *iface, completed_handler *handler, EventRegistrationToken *token)
{
	return raven_event_add(&((struct queue *)iface)->completed, (IUnknown *)handler, token);
}
static HRESULT WINAPI queue_remove_completed(IDispatcherQueue *iface, EventRegistrationToken token)
{
	return raven_event_remove(&((struct queue *)iface)->completed, token);
}
static const IDispatcherQueueVtbl queue_table = {
	queue_query, queue_addref, queue_release, queue_iids, queue_class, queue_trust, queue_create_timer,
	queue_enqueue, queue_enqueue_priority, queue_add_starting, queue_remove_starting, queue_add_completed, queue_remove_completed
};
HRESULT raven_dispatcher_queue_create(ICoreWindow *window, HWND hwnd, IDispatcherQueue **out)
{
	*out = NULL;
	struct queue *queue = ALLOC(sizeof *queue);
	if (!queue) return E_OUTOFMEMORY;
	queue->iface.lpVtbl = &queue_table;
	queue->access.lpVtbl = &access_table;
	queue->refs = 1;
	queue->thread = GetCurrentThreadId();
	queue->window = window;
	queue->hwnd = hwnd;
	ICoreWindow_AddRef(window);
	InitializeCriticalSection(&queue->lock);
	InitializeCriticalSection(&queue->starting.lock);
	InitializeCriticalSection(&queue->completed.lock);
	InterlockedIncrement(&raven_xaml_objects);
	*out = &queue->iface;
	return S_OK;
}
HRESULT raven_dispatcher_queue_close(IDispatcherQueue *iface)
{
	struct queue *queue = (void *)iface;
	EnterCriticalSection(&queue->lock);
	if (queue->closed) { LeaveCriticalSection(&queue->lock); return S_OK; }
	queue->closed = TRUE;
	/* A controller shutdown raises ShutdownStarting while the pump still runs; this path raises it only when that has not happened. */
	BOOL raise_starting = !queue->shutdown_started;
	queue->shutdown_started = TRUE;
	LeaveCriticalSection(&queue->lock);
	HRESULT result = raise_starting ? notify_shutdown(queue, TRUE, FALSE) : S_OK;
	EnterCriticalSection(&queue->lock);
	struct timer *timer = queue->timers;
	if (timer) timer_addref(&timer->iface);
	LeaveCriticalSection(&queue->lock);
	while (timer) {
		EnterCriticalSection(&queue->lock);
		struct timer *next = timer->next;
		if (next) timer_addref(&next->iface);
		BOOL active = timer_stop_locked(timer);
		LeaveCriticalSection(&queue->lock);
		raven_event_close(&timer->tick);
		if (active) timer_release(&timer->iface);
		timer_release(&timer->iface);
		timer = next;
	}
	HRESULT completed = notify_shutdown(queue, FALSE, FALSE);
	raven_event_close(&queue->starting);
	raven_event_close(&queue->completed);
	return FAILED(result) ? result : completed;
}

/* The shutdown action: the shared async-action shape, completed only after the dedicated thread has exited. */
typedef __x_ABI_CWindows_CFoundation_CIAsyncAction async_action;
typedef __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler action_completed;
static const WCHAR action_name[] = L"Windows.Foundation.IAsyncAction";
static ULONG WINAPI action_addref(async_action *iface);
struct shutdown_action {
	async_action iface;
	IAsyncInfo info;
	LONG refs;
	CRITICAL_SECTION lock;
	AsyncStatus status;
	HRESULT error;
	UINT32 id;
	BOOL closed, assigned;
	action_completed *completed;
};
static void action_finish(async_action *iface, HRESULT result, AsyncStatus status)
{
	struct shutdown_action *self = (void *)iface;
	action_completed *handler = NULL;
	EnterCriticalSection(&self->lock);
	if (self->status == Started) {
		self->status = status;
		self->error = result;
		handler = self->completed;
		if (handler) __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_AddRef(handler);
	}
	LeaveCriticalSection(&self->lock);
	if (handler) {
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Invoke(handler, iface, status);
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Release(handler);
	}
}
static HRESULT WINAPI action_query(async_action *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID___x_ABI_CWindows_CFoundation_CIAsyncAction)) *out = iface;
	else if (IsEqualIID(iid, &IID_IAsyncInfo)) *out = &((struct shutdown_action *)iface)->info;
	else return E_NOINTERFACE;
	action_addref(iface);
	return S_OK;
}
static ULONG WINAPI action_addref(async_action *iface) { return InterlockedIncrement(&((struct shutdown_action *)iface)->refs); }
static ULONG WINAPI action_release(async_action *iface)
{
	struct shutdown_action *self = (void *)iface;
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->completed) __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Release(self->completed);
		DeleteCriticalSection(&self->lock);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return refs;
}
static HRESULT WINAPI action_iids(async_action *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI action_class(async_action *iface, HSTRING *out) { return class_string(action_name, out); }
static HRESULT WINAPI action_trust(async_action *iface, TrustLevel *out) { return trust(out); }
static HRESULT WINAPI action_put_completed(async_action *iface, action_completed *handler)
{
	struct shutdown_action *self = (void *)iface;
	HRESULT result = S_OK;
	AsyncStatus status;
	EnterCriticalSection(&self->lock);
	status = self->status;
	if (self->closed) result = RO_E_CLOSED;
	else if (self->assigned) result = E_ILLEGAL_DELEGATE_ASSIGNMENT;
	else {
		self->assigned = TRUE;
		self->completed = handler;
		if (handler) {
			__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_AddRef(handler);
			if (status != Started) __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_AddRef(handler);
		}
	}
	LeaveCriticalSection(&self->lock);
	if (SUCCEEDED(result) && handler && status != Started) {
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Invoke(handler, iface, status);
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Release(handler);
	}
	return result;
}
static HRESULT WINAPI action_get_completed(async_action *iface, action_completed **handler)
{
	struct shutdown_action *self = (void *)iface;
	HRESULT result = S_OK;
	if (!handler) return E_POINTER;
	*handler = NULL;
	EnterCriticalSection(&self->lock);
	if (self->closed) result = RO_E_CLOSED;
	else if ((*handler = self->completed)) __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_AddRef(*handler);
	LeaveCriticalSection(&self->lock);
	return result;
}
static HRESULT WINAPI action_results(async_action *iface)
{
	struct shutdown_action *self = (void *)iface;
	HRESULT result;
	EnterCriticalSection(&self->lock);
	result = self->closed ? RO_E_CLOSED : self->status == Started ? E_ILLEGAL_METHOD_CALL : self->error;
	LeaveCriticalSection(&self->lock);
	return result;
}
static struct shutdown_action *info_owner(IAsyncInfo *iface) { return CONTAINING_RECORD(iface, struct shutdown_action, info); }
static HRESULT WINAPI info_query(IAsyncInfo *iface, REFIID iid, void **out) { return action_query(&info_owner(iface)->iface, iid, out); }
static ULONG WINAPI info_addref(IAsyncInfo *iface) { return action_addref(&info_owner(iface)->iface); }
static ULONG WINAPI info_release(IAsyncInfo *iface) { return action_release(&info_owner(iface)->iface); }
static HRESULT WINAPI info_iids(IAsyncInfo *iface, ULONG *count, IID **iids) { return action_iids(&info_owner(iface)->iface, count, iids); }
static HRESULT WINAPI info_class(IAsyncInfo *iface, HSTRING *out) { return action_class(&info_owner(iface)->iface, out); }
static HRESULT WINAPI info_trust(IAsyncInfo *iface, TrustLevel *out) { return action_trust(&info_owner(iface)->iface, out); }
static HRESULT WINAPI info_id(IAsyncInfo *iface, UINT32 *out)
{
	if (!out) return E_POINTER;
	struct shutdown_action *self = info_owner(iface);
	EnterCriticalSection(&self->lock);
	HRESULT result = self->closed ? RO_E_CLOSED : S_OK;
	*out = self->id;
	LeaveCriticalSection(&self->lock);
	return result;
}
static HRESULT WINAPI info_status(IAsyncInfo *iface, AsyncStatus *out)
{
	if (!out) return E_POINTER;
	struct shutdown_action *self = info_owner(iface);
	EnterCriticalSection(&self->lock);
	HRESULT result = self->closed ? RO_E_CLOSED : S_OK;
	*out = self->status;
	LeaveCriticalSection(&self->lock);
	return result;
}
static HRESULT WINAPI info_error(IAsyncInfo *iface, HRESULT *out)
{
	if (!out) return E_POINTER;
	struct shutdown_action *self = info_owner(iface);
	EnterCriticalSection(&self->lock);
	HRESULT result = self->closed ? RO_E_CLOSED : S_OK;
	*out = self->error;
	LeaveCriticalSection(&self->lock);
	return result;
}
static HRESULT WINAPI info_cancel(IAsyncInfo *iface)
{
	/* The drain on the dedicated thread cannot be cancelled; the action still completes when the thread exits. */
	(void)iface;
	return S_OK;
}
static HRESULT WINAPI info_close(IAsyncInfo *iface)
{
	struct shutdown_action *self = info_owner(iface);
	action_completed *handler = NULL;
	HRESULT result = S_OK;
	EnterCriticalSection(&self->lock);
	if (self->status == Started) result = E_ILLEGAL_METHOD_CALL;
	else {
		self->closed = TRUE;
		handler = self->completed;
		self->completed = NULL;
	}
	LeaveCriticalSection(&self->lock);
	if (handler) __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Release(handler);
	return result;
}
static const IAsyncInfoVtbl info_table = {
	info_query, info_addref, info_release, info_iids, info_class, info_trust, info_id, info_status, info_error, info_cancel, info_close
};
static const __x_ABI_CWindows_CFoundation_CIAsyncActionVtbl action_table = {
	action_query, action_addref, action_release, action_iids, action_class, action_trust,
	action_put_completed, action_get_completed, action_results
};
static struct shutdown_action *new_action(void)
{
	struct shutdown_action *action = ALLOC(sizeof *action);
	if (!action) return NULL;
	action->iface.lpVtbl = &action_table;
	action->info.lpVtbl = &info_table;
	action->refs = 1;
	action->status = Started;
	action->id = (UINT32)InterlockedIncrement(&next_action_id);
	InitializeCriticalSection(&action->lock);
	InterlockedIncrement(&raven_xaml_objects);
	return action;
}

/* Shutdown work runs on the dedicated thread: the starter raises ShutdownStarting while the queue still accepts work, the
 * closer ends the drain by closing the window, which raises ShutdownCompleted through the existing queue close. */
struct shutdown_work {
	IDispatchedHandler iface;
	LONG refs;
	IDispatcherQueue *queue;
	ICoreWindow *window;
	BOOL close;
};
static HRESULT WINAPI shutdown_work_query(IDispatchedHandler *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IDispatchedHandler)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct shutdown_work *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI shutdown_work_addref(IDispatchedHandler *iface) { return InterlockedIncrement(&((struct shutdown_work *)iface)->refs); }
static ULONG WINAPI shutdown_work_release(IDispatchedHandler *iface)
{
	struct shutdown_work *self = (void *)iface;
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->queue) IDispatcherQueue_Release(self->queue);
		if (self->window) ICoreWindow_Release(self->window);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return refs;
}
static HRESULT WINAPI shutdown_work_invoke(IDispatchedHandler *iface)
{
	struct shutdown_work *self = (void *)iface;
	if (self->close) {
		struct queue *queue = (void *)self->queue;
		/* An outstanding ShutdownStarting deferral defers the close: the last Complete re-posts this work, so the
		 * drain keeps dispatching the work posted during the deferral window first. */
		BOOL defer = FALSE;
		EnterCriticalSection(&queue->lock);
		if (queue->shutdown_deferrals) {
			queue->close_deferred = TRUE;
			defer = TRUE;
		}
		LeaveCriticalSection(&queue->lock);
		if (defer) return S_OK;
		/* The window closes on its own thread, completing pending actions and raising ShutdownCompleted with the queue close. */
		ICoreWindow_Close(self->window);
		return S_OK;
	}
	struct queue *queue = (void *)self->queue;
	EnterCriticalSection(&queue->lock);
	BOOL raise = !queue->closed && !queue->shutdown_started;
	if (raise) queue->shutdown_started = TRUE;
	LeaveCriticalSection(&queue->lock);
	if (raise) notify_shutdown(queue, TRUE, TRUE);
	/* A failing ShutdownStarting handler must not stall the drain. */
	return S_OK;
}
static const IDispatchedHandlerVtbl shutdown_work_table = {shutdown_work_query, shutdown_work_addref, shutdown_work_release, shutdown_work_invoke};
static struct shutdown_work *new_shutdown_work(IDispatcherQueue *queue, ICoreWindow *window, BOOL close)
{
	struct shutdown_work *work = ALLOC(sizeof *work);
	if (!work) return NULL;
	work->iface.lpVtbl = &shutdown_work_table;
	work->refs = 1;
	work->queue = queue;
	work->window = window;
	work->close = close;
	if (queue) IDispatcherQueue_AddRef(queue);
	if (window) ICoreWindow_AddRef(window);
	InterlockedIncrement(&raven_xaml_objects);
	return work;
}
/* The re-posted close: queued below every client priority so work posted during the deferral window drains first.
 * A rejected post means the window already closed some other way and the queue went down with it. */
static void post_close_work(struct queue *queue)
{
	struct shutdown_work *closer = new_shutdown_work(&queue->iface, queue->window, TRUE);
	if (!closer) return;
	boolean accepted = FALSE;
	raven_window_post(queue->window, &closer->iface, CoreDispatcherPriority_Idle - 1, &accepted);
	shutdown_work_release(&closer->iface);
}

/* A standalone controller owns a dedicated thread whose CoreWindow carries the queue, like every thread-owned queue here.
 * The controller always outlives its thread: the thread exits before the shutdown returns, and the last external reference
 * shuts the queue down instead of leaving the thread behind. */
struct controller {
	IDispatcherQueueController iface;
	LONG refs;
	CRITICAL_SECTION lock;
	IDispatcherQueue *queue;
	ICoreWindow *window;
	HANDLE thread;
	DWORD thread_id;
	HANDLE ready;
	HRESULT start;
	BOOL shutting;
};
static DWORD WINAPI controller_thread(void *context)
{
	struct controller *self = context;
	self->thread_id = GetCurrentThreadId();
	HRESULT result = raven_current_window(&self->window);
	if (SUCCEEDED(result)) result = raven_dispatcher_queue_current(&self->queue);
	self->start = result;
	SetEvent(self->ready);
	if (FAILED(result)) return 1;
	ICoreDispatcher *dispatcher = NULL;
	result = ICoreWindow_get_Dispatcher(self->window, &dispatcher);
	if (SUCCEEDED(result)) {
		/* The pump ends when the drain closes the window; a host close or WM_QUIT ends it the same way. */
		result = ICoreDispatcher_ProcessEvents(dispatcher, CoreProcessEventsOption_ProcessUntilQuit);
		ICoreDispatcher_Release(dispatcher);
	}
	return SUCCEEDED(result) ? 0 : 2;
}
static void controller_free(struct controller *self)
{
	if (self->thread) CloseHandle(self->thread);
	if (self->ready) CloseHandle(self->ready);
	if (self->queue) IDispatcherQueue_Release(self->queue);
	if (self->window) ICoreWindow_Release(self->window);
	DeleteCriticalSection(&self->lock);
	FREE(self);
	InterlockedDecrement(&raven_xaml_objects);
}
/* Runs the whole shutdown on the calling thread and drops the reference the thread was created with, which may free the
 * controller; the caller must not touch it afterwards. Returns FALSE when a shutdown is already running. */
static BOOL controller_stop(struct controller *self)
{
	BOOL run = FALSE;
	EnterCriticalSection(&self->lock);
	if (!self->shutting) {
		self->shutting = TRUE;
		run = TRUE;
	}
	IDispatcherQueue *queue = self->queue;
	ICoreWindow *window = self->window;
	HANDLE thread = self->thread;
	LeaveCriticalSection(&self->lock);
	if (!run) return FALSE;
	if (queue && window) {
		/* The starter runs before pending work at high priority; the closer sits below every client priority so the drain
		 * empties the queue first. Work enqueued during ShutdownStarting still dispatches, per the public contract. */
		struct shutdown_work *starter = new_shutdown_work(queue, window, FALSE);
		struct shutdown_work *closer = starter ? new_shutdown_work(queue, window, TRUE) : NULL;
		boolean accepted = FALSE;
		if (starter) raven_window_post(window, &starter->iface, CoreDispatcherPriority_High, &accepted);
		if (closer) raven_window_post(window, &closer->iface, CoreDispatcherPriority_Idle - 1, &accepted);
		if (starter) shutdown_work_release(&starter->iface);
		if (closer) shutdown_work_release(&closer->iface);
	}
	/* The drain runs to completion on the pump thread; a handler that never returns hangs shutdown, as on Windows. */
	if (thread) WaitForSingleObject(thread, INFINITE);
	if (InterlockedDecrement(&self->refs)) return TRUE;
	controller_free(self);
	return TRUE;
}
static HRESULT WINAPI controller_query(IDispatcherQueueController *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID_IDispatcherQueueController) || IsEqualIID(iid, &IID_IAgileObject)) *out = iface;
	else return E_NOINTERFACE;
	InterlockedIncrement(&((struct controller *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI controller_addref(IDispatcherQueueController *iface) { return InterlockedIncrement(&((struct controller *)iface)->refs); }
static ULONG WINAPI controller_release(IDispatcherQueueController *iface)
{
	struct controller *self = (void *)iface;
	LONG refs = InterlockedDecrement(&self->refs);
	/* The dedicated thread holds the last other reference; shutting down joins it before the object can go away. */
	if (refs == 1 && self->thread && !self->shutting) {
		controller_stop(self);
		return 0;
	}
	if (!refs) controller_free(self);
	return refs;
}
static HRESULT WINAPI controller_iids(IDispatcherQueueController *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI controller_class(IDispatcherQueueController *iface, HSTRING *out) { return class_string(controller_name, out); }
static HRESULT WINAPI controller_trust(IDispatcherQueueController *iface, TrustLevel *out) { return trust(out); }
static HRESULT WINAPI controller_get_queue(IDispatcherQueueController *iface, IDispatcherQueue **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	struct controller *self = (void *)iface;
	if (!self->queue) return E_FAIL;
	IDispatcherQueue_AddRef(self->queue);
	*out = self->queue;
	return S_OK;
}
static HRESULT WINAPI controller_shutdown_queue(IDispatcherQueueController *iface, async_action **operation)
{
	if (!operation) return E_POINTER;
	*operation = NULL;
	struct controller *self = (void *)iface;
	/* The pump thread cannot join itself, and a queue is shut down once. */
	if (self->thread_id == GetCurrentThreadId() || !controller_stop(self)) return E_ILLEGAL_METHOD_CALL;
	struct shutdown_action *action = new_action();
	if (!action) return E_OUTOFMEMORY;
	/* ShutdownQueueAsync joins the thread, so the action never escapes while Started. */
	action_finish(&action->iface, S_OK, Completed);
	*operation = &action->iface;
	return S_OK;
}
static const IDispatcherQueueControllerVtbl controller_table = {
	controller_query, controller_addref, controller_release, controller_iids, controller_class, controller_trust,
	controller_get_queue, controller_shutdown_queue
};

static LONG factory_refs = 1;
static IActivationFactory activation_factory, controller_factory;
static IDispatcherQueueStatics queue_statics;
static IDispatcherQueueControllerStatics controller_statics;
/* Both classes share the factory refcount; each activation answers only its own statics. */
static HRESULT factory_query(IActivationFactory *factory, IUnknown *statics, REFIID statics_iid, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID_IActivationFactory)) *out = factory;
	else if (IsEqualIID(iid, statics_iid)) *out = statics;
	else return E_NOINTERFACE;
	InterlockedIncrement(&factory_refs);
	return S_OK;
}
static HRESULT queue_factory_query(REFIID iid, void **out)
{
	return factory_query(&activation_factory, (IUnknown *)&queue_statics, &IID_IDispatcherQueueStatics, iid, out);
}
static HRESULT controller_factory_query(REFIID iid, void **out)
{
	return factory_query(&controller_factory, (IUnknown *)&controller_statics, &IID_IDispatcherQueueControllerStatics, iid, out);
}

static HRESULT WINAPI factory_interface_query(IActivationFactory *iface, REFIID iid, void **out)
{
	return iface == &controller_factory ? controller_factory_query(iid, out) : queue_factory_query(iid, out);
}
static ULONG WINAPI factory_addref(IActivationFactory *iface) { return InterlockedIncrement(&factory_refs); }
static ULONG WINAPI factory_release(IActivationFactory *iface) { return InterlockedDecrement(&factory_refs); }
static HRESULT WINAPI factory_iids(IActivationFactory *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI factory_class(IActivationFactory *iface, HSTRING *out) { return class_string(queue_name, out); }
static HRESULT WINAPI controller_factory_class(IActivationFactory *iface, HSTRING *out) { return class_string(controller_name, out); }
static HRESULT WINAPI factory_trust(IActivationFactory *iface, TrustLevel *out) { return trust(out); }

static HRESULT WINAPI statics_query(IDispatcherQueueStatics *iface, REFIID iid, void **out) { return queue_factory_query(iid, out); }
static ULONG WINAPI statics_addref(IDispatcherQueueStatics *iface) { return InterlockedIncrement(&factory_refs); }
static ULONG WINAPI statics_release(IDispatcherQueueStatics *iface) { return InterlockedDecrement(&factory_refs); }
static HRESULT WINAPI statics_iids(IDispatcherQueueStatics *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI statics_class(IDispatcherQueueStatics *iface, HSTRING *out) { return class_string(queue_name, out); }
static HRESULT WINAPI statics_trust(IDispatcherQueueStatics *iface, TrustLevel *out) { return trust(out); }

static HRESULT WINAPI dedicated_query(IDispatcherQueueControllerStatics *iface, REFIID iid, void **out) { return controller_factory_query(iid, out); }
static ULONG WINAPI dedicated_addref(IDispatcherQueueControllerStatics *iface) { return InterlockedIncrement(&factory_refs); }
static ULONG WINAPI dedicated_release(IDispatcherQueueControllerStatics *iface) { return InterlockedDecrement(&factory_refs); }
static HRESULT WINAPI dedicated_iids(IDispatcherQueueControllerStatics *iface, ULONG *count, IID **iids) { return no_iids(count, iids); }
static HRESULT WINAPI dedicated_class(IDispatcherQueueControllerStatics *iface, HSTRING *out) { return class_string(controller_name, out); }
static HRESULT WINAPI dedicated_trust(IDispatcherQueueControllerStatics *iface, TrustLevel *out) { return trust(out); }

static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	return E_ILLEGAL_METHOD_CALL;
}
static HRESULT WINAPI statics_current(IDispatcherQueueStatics *iface, IDispatcherQueue **out)
{
	return raven_dispatcher_queue_current(out);
}
/* The controller's queue must exist before the controller escapes: callers enqueue on it right away. */
static HRESULT WINAPI dedicated_create(IDispatcherQueueControllerStatics *iface, IDispatcherQueueController **out)
{
	(void)iface;
	if (!out) return E_POINTER;
	*out = NULL;
	struct controller *controller = ALLOC(sizeof *controller);
	if (!controller) return E_OUTOFMEMORY;
	controller->iface.lpVtbl = &controller_table;
	controller->refs = 1;
	InitializeCriticalSection(&controller->lock);
	InterlockedIncrement(&raven_xaml_objects);
	HRESULT failed = S_OK;
	controller->ready = CreateEventW(NULL, TRUE, FALSE, NULL);
	if (!controller->ready) failed = last_error();
	else {
		controller->thread = CreateThread(NULL, 0, controller_thread, controller, 0, NULL);
		if (!controller->thread) failed = last_error();
		else InterlockedIncrement(&controller->refs); /* the dedicated thread's reference */
	}
	if (SUCCEEDED(failed)) {
		WaitForSingleObject(controller->ready, INFINITE);
		if (FAILED(controller->start)) failed = controller->start;
	}
	if (FAILED(failed)) {
		controller_release(&controller->iface);
		return failed;
	}
	*out = &controller->iface;
	return S_OK;
}
/* The native CoreMessaging entry point uses the same dedicated controller as the WinRT factory. */
struct dispatcher_queue_options { DWORD size, thread_type, apartment_type; };
__declspec(dllexport) HRESULT WINAPI CreateDispatcherQueueController(struct dispatcher_queue_options options,
	IDispatcherQueueController **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (options.size != sizeof options || (options.thread_type != 1 && options.thread_type != 2) || options.apartment_type > 2)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at dispatcher_queue.c:1165\n"); return E_INVALIDARG; }
	/* shortcut: current-thread queues and explicit COM apartments need their own lifecycle; implement before accepting them. */
	if (options.thread_type != 1 || options.apartment_type != 0) { raven_trace_not_implemented(__func__); return E_NOTIMPL; }
	return dedicated_create(NULL, out);
}
static const IActivationFactoryVtbl factory_table = {
	factory_interface_query, factory_addref, factory_release, factory_iids, factory_class, factory_trust, factory_activate
};
static const IActivationFactoryVtbl controller_factory_table = {
	factory_interface_query, factory_addref, factory_release, factory_iids, controller_factory_class, factory_trust, factory_activate
};
static const IDispatcherQueueStaticsVtbl statics_table = {
	statics_query, statics_addref, statics_release, statics_iids, statics_class, statics_trust, statics_current
};
static const IDispatcherQueueControllerStaticsVtbl dedicated_table = {
	dedicated_query, dedicated_addref, dedicated_release, dedicated_iids, dedicated_class, dedicated_trust, dedicated_create
};
static IActivationFactory activation_factory = {&factory_table};
static IDispatcherQueueStatics queue_statics = {&statics_table};
static IActivationFactory controller_factory = {&controller_factory_table};
static IDispatcherQueueControllerStatics controller_statics = {&dedicated_table};
HRESULT raven_dispatcher_queue_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length;
	const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	if (length == ARRAYSIZE(queue_name) - 1 && CompareStringOrdinal(text, (int)length, queue_name, -1, FALSE) == CSTR_EQUAL)
		return queue_factory_query(&IID_IActivationFactory, (void **)out);
	if (length == ARRAYSIZE(controller_name) - 1 && CompareStringOrdinal(text, (int)length, controller_name, -1, FALSE) == CSTR_EQUAL)
		return controller_factory_query(&IID_IActivationFactory, (void **)out);
	return CLASS_E_CLASSNOTAVAILABLE;
}
