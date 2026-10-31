/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the windowing core of the Windows Runtime that a XAML application runs on, as far as the first
 * stage needs it. Windows.UI.Core.CoreWindow is the window of the calling UI thread and Windows.UI.Core.CoreDispatcher
 * carries its work: a handler posted with RunAsync runs when the dispatcher processes events, and the IAsyncAction it
 * returns completes after the handler returns.
 *
 * The rendering side of XAML is not here yet. This component establishes the activation and threading spine the rest
 * of the engine plugs into: a class registered in the runtime catalog, a per-thread window object, and a queue that
 * will be driven by Raven's desktop message loop. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <windows.ui.core.h>
#include <windows.foundation.h>
#include "xaml_internal.h"
#include "desktop_window.h"
#include "dispatcher_queue.h"
#include "corewindow_interop.h"
#include "xaml_styles.h" /* the DependencyProperty statics entry the chain below reaches */
#include "pointer.h" /* the PointerDevice statics entry the chain below reaches */

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))
#ifndef E_ILLEGAL_METHOD_CALL
#define E_ILLEGAL_METHOD_CALL ((HRESULT)0x8000000e)
#endif
#ifndef E_ILLEGAL_DELEGATE_ASSIGNMENT
#define E_ILLEGAL_DELEGATE_ASSIGNMENT ((HRESULT)0x80000018)
#endif
#ifndef RO_E_CLOSED
#define RO_E_CLOSED ((HRESULT)0x80000013)
#endif

typedef __x_ABI_CWindows_CUI_CCore_CICoreWindow core_window;
typedef __x_ABI_CWindows_CUI_CCore_CICoreWindowStatic core_window_static;
typedef __x_ABI_CWindows_CUI_CCore_CICoreDispatcher core_dispatcher;
typedef __x_ABI_CWindows_CUI_CCore_CIDispatchedHandler dispatched_handler;
typedef __x_ABI_CWindows_CFoundation_CIAsyncAction async_action;
typedef __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler completed_handler;

static const WCHAR window_name[] = L"Windows.UI.Core.CoreWindow";
static const WCHAR cursor_name[] = L"Windows.UI.Core.CoreCursor";
static const WCHAR dispatcher_name[] = L"Windows.UI.Core.CoreDispatcher";
static const WCHAR action_name[] = L"Windows.Foundation.IAsyncAction";

LONG raven_xaml_objects;
static LONG next_action_id;
static DWORD window_tls = TLS_OUT_OF_INDEXES;

static int equal_iid(REFIID left, const IID *right)
{
	return left->Data1 == right->Data1 && left->Data2 == right->Data2 && left->Data3 == right->Data3 &&
		left->Data4[0] == right->Data4[0] && left->Data4[1] == right->Data4[1] &&
		left->Data4[2] == right->Data4[2] && left->Data4[3] == right->Data4[3] &&
		left->Data4[4] == right->Data4[4] && left->Data4[5] == right->Data4[5] &&
		left->Data4[6] == right->Data4[6] && left->Data4[7] == right->Data4[7];
}

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

/* Work posted to a dispatcher waits here until the dispatcher processes events. */
struct queued {
	struct queued *next;
	dispatched_handler *handler;
	async_action *action;
	LONG priority;
};

struct window {
	const __x_ABI_CWindows_CUI_CCore_CICoreWindowVtbl *lpVtbl;
	LONG refs;
	DWORD thread;
	CRITICAL_SECTION lock;
	struct queued *head, *tail;
	HANDLE ready;
	BOOL closed;
	ULONG work_epoch; /* posts above idle priority since a RunIdleAsync item queued (see dispatcher_run_idle_async) */
	__x_ABI_CWindows_CSystem_CIDispatcherQueue *system_queue;
	core_dispatcher *dispatcher;
	__x_ABI_CWindows_CUI_CCore_CICoreCursor *cursor; /* AddRef'd; NULL until an application puts one: the platform default */
	raven_core_window_interop interop;
	struct raven_desktop_window *desktop;
};

struct dispatcher {
	const __x_ABI_CWindows_CUI_CCore_CICoreDispatcherVtbl *lpVtbl;
	LONG refs;
	struct window *window;
};

struct action {
	const __x_ABI_CWindows_CFoundation_CIAsyncActionVtbl *lpVtbl;
	LONG refs;
	IAsyncInfo info;
	CRITICAL_SECTION lock;
	AsyncStatus status;
	HRESULT error;
	UINT32 id;
	BOOL closed, assigned;
	completed_handler *completed;
};

static HRESULT WINAPI action_query(async_action *iface, REFIID iid, void **out);
static ULONG WINAPI action_addref(async_action *iface);
static ULONG WINAPI action_release(async_action *iface);
static HRESULT WINAPI action_iids(async_action *iface, ULONG *count, IID **iids);
static HRESULT WINAPI action_class_name(async_action *iface, HSTRING *name);
static HRESULT WINAPI action_trust(async_action *iface, TrustLevel *level);
static HRESULT WINAPI action_put_completed(async_action *iface, completed_handler *handler);
static HRESULT WINAPI action_get_completed(async_action *iface, completed_handler **handler);
static HRESULT WINAPI action_results(async_action *iface);

static const __x_ABI_CWindows_CFoundation_CIAsyncActionVtbl action_table = {
	.QueryInterface = action_query,
	.AddRef = action_addref,
	.Release = action_release,
	.GetIids = action_iids,
	.GetRuntimeClassName = action_class_name,
	.GetTrustLevel = action_trust,
	.put_Completed = action_put_completed,
	.get_Completed = action_get_completed,
	.GetResults = action_results,
};

static HRESULT WINAPI dispatcher_query(core_dispatcher *iface, REFIID iid, void **out);
static ULONG WINAPI dispatcher_addref(core_dispatcher *iface);
static ULONG WINAPI dispatcher_release(core_dispatcher *iface);
static HRESULT WINAPI dispatcher_iids(core_dispatcher *iface, ULONG *count, IID **iids);
static HRESULT WINAPI dispatcher_class_name(core_dispatcher *iface, HSTRING *name);
static HRESULT WINAPI dispatcher_trust(core_dispatcher *iface, TrustLevel *level);
static HRESULT WINAPI dispatcher_has_thread_access(core_dispatcher *iface, boolean *value);
static HRESULT WINAPI dispatcher_process_events(core_dispatcher *iface, __x_ABI_CWindows_CUI_CCore_CCoreProcessEventsOption options);
static HRESULT WINAPI dispatcher_run_async(core_dispatcher *iface, __x_ABI_CWindows_CUI_CCore_CCoreDispatcherPriority priority,
	dispatched_handler *callback, async_action **operation);
static HRESULT WINAPI dispatcher_run_idle_async(core_dispatcher *iface,
	__x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandler *callback, async_action **operation);

static const __x_ABI_CWindows_CUI_CCore_CICoreDispatcherVtbl dispatcher_table = {
	.QueryInterface = dispatcher_query,
	.AddRef = dispatcher_addref,
	.Release = dispatcher_release,
	.GetIids = dispatcher_iids,
	.GetRuntimeClassName = dispatcher_class_name,
	.GetTrustLevel = dispatcher_trust,
	.get_HasThreadAccess = dispatcher_has_thread_access,
	.ProcessEvents = dispatcher_process_events,
	.RunAsync = dispatcher_run_async,
	.RunIdleAsync = dispatcher_run_idle_async,
};

static HRESULT WINAPI window_query(core_window *iface, REFIID iid, void **out);
static ULONG WINAPI window_addref(core_window *iface);
static ULONG WINAPI window_release(core_window *iface);
static HRESULT WINAPI window_iids(core_window *iface, ULONG *count, IID **iids);
static HRESULT WINAPI window_class_name(core_window *iface, HSTRING *name);
static HRESULT WINAPI window_trust(core_window *iface, TrustLevel *level);
static HRESULT WINAPI window_bounds(core_window *iface, __x_ABI_CWindows_CFoundation_CRect *value);
static HRESULT WINAPI window_dispatcher(core_window *iface, core_dispatcher **value);
static HRESULT WINAPI window_input_enabled(core_window *iface, boolean *value);
static HRESULT WINAPI window_put_input_enabled(core_window *iface, boolean value);
static HRESULT WINAPI window_visible(core_window *iface, boolean *value);
static HRESULT WINAPI window_activate(core_window *iface);
static HRESULT WINAPI window_close(core_window *iface);

static HRESULT WINAPI window_add_Activated(core_window *iface,
	__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowActivatedEventArgs *handler, EventRegistrationToken *token)
{
	return raven_desktop_add(((struct window *)iface)->desktop, RAVEN_WINDOW_ACTIVATED, (IUnknown *)handler, token);
}
static HRESULT WINAPI window_remove_Activated(core_window *iface, EventRegistrationToken token)
{
	return raven_desktop_remove(((struct window *)iface)->desktop, RAVEN_WINDOW_ACTIVATED, token);
}
static HRESULT WINAPI window_add_Closed(core_window *iface,
	__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCoreWindowEventArgs *handler, EventRegistrationToken *token)
{
	return raven_desktop_add(((struct window *)iface)->desktop, RAVEN_WINDOW_CLOSED, (IUnknown *)handler, token);
}
static HRESULT WINAPI window_remove_Closed(core_window *iface, EventRegistrationToken token)
{
	return raven_desktop_remove(((struct window *)iface)->desktop, RAVEN_WINDOW_CLOSED, token);
}
static HRESULT WINAPI window_add_SizeChanged(core_window *iface,
	__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CWindowSizeChangedEventArgs *handler, EventRegistrationToken *token)
{
	return raven_desktop_add(((struct window *)iface)->desktop, RAVEN_WINDOW_SIZE, (IUnknown *)handler, token);
}
static HRESULT WINAPI window_remove_SizeChanged(core_window *iface, EventRegistrationToken token)
{
	return raven_desktop_remove(((struct window *)iface)->desktop, RAVEN_WINDOW_SIZE, token);
}
static HRESULT WINAPI window_add_VisibilityChanged(core_window *iface,
	__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CVisibilityChangedEventArgs *handler, EventRegistrationToken *token)
{
	return raven_desktop_add(((struct window *)iface)->desktop, RAVEN_WINDOW_VISIBLE, (IUnknown *)handler, token);
}
static HRESULT WINAPI window_remove_VisibilityChanged(core_window *iface, EventRegistrationToken token)
{
	return raven_desktop_remove(((struct window *)iface)->desktop, RAVEN_WINDOW_VISIBLE, token);
}
static HRESULT WINAPI window_add_KeyDown(core_window *iface,
	__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CKeyEventArgs *handler, EventRegistrationToken *token)
{
	return raven_desktop_add(((struct window *)iface)->desktop, RAVEN_WINDOW_KEY_DOWN, (IUnknown *)handler, token);
}
static HRESULT WINAPI window_remove_KeyDown(core_window *iface, EventRegistrationToken token)
{
	return raven_desktop_remove(((struct window *)iface)->desktop, RAVEN_WINDOW_KEY_DOWN, token);
}
static HRESULT WINAPI window_add_KeyUp(core_window *iface,
	__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CKeyEventArgs *handler, EventRegistrationToken *token)
{
	return raven_desktop_add(((struct window *)iface)->desktop, RAVEN_WINDOW_KEY_UP, (IUnknown *)handler, token);
}
static HRESULT WINAPI window_remove_KeyUp(core_window *iface, EventRegistrationToken token)
{
	return raven_desktop_remove(((struct window *)iface)->desktop, RAVEN_WINDOW_KEY_UP, token);
}
static HRESULT WINAPI window_add_CharacterReceived(core_window *iface,
	__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCharacterReceivedEventArgs *handler, EventRegistrationToken *token)
{
	return raven_desktop_add(((struct window *)iface)->desktop, RAVEN_WINDOW_CHARACTER, (IUnknown *)handler, token);
}
static HRESULT WINAPI window_remove_CharacterReceived(core_window *iface, EventRegistrationToken token)
{
	return raven_desktop_remove(((struct window *)iface)->desktop, RAVEN_WINDOW_CHARACTER, token);
}
static HRESULT WINAPI window_add_InputEnabled(core_window *iface,
	__FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CInputEnabledEventArgs *handler, EventRegistrationToken *token)
{
	return raven_desktop_add(((struct window *)iface)->desktop, RAVEN_WINDOW_INPUT_ENABLED, (IUnknown *)handler, token);
}
static HRESULT WINAPI window_remove_InputEnabled(core_window *iface, EventRegistrationToken token)
{
	return raven_desktop_remove(((struct window *)iface)->desktop, RAVEN_WINDOW_INPUT_ENABLED, token);
}
static HRESULT WINAPI interop_query(raven_core_window_interop *iface, REFIID iid, void **out)
{
	return window_query((core_window *)CONTAINING_RECORD(iface, struct window, interop), iid, out);
}
static ULONG WINAPI interop_addref(raven_core_window_interop *iface)
{
	return window_addref((core_window *)CONTAINING_RECORD(iface, struct window, interop));
}
static ULONG WINAPI interop_release(raven_core_window_interop *iface)
{
	return window_release((core_window *)CONTAINING_RECORD(iface, struct window, interop));
}
static HRESULT WINAPI interop_handle(raven_core_window_interop *iface, HWND *out)
{
	return raven_desktop_handle(CONTAINING_RECORD(iface, struct window, interop)->desktop, out);
}
static HRESULT WINAPI interop_handled(raven_core_window_interop *iface, boolean handled)
{
	return raven_desktop_handled(CONTAINING_RECORD(iface, struct window, interop)->desktop, handled);
}
static const struct raven_core_window_interop_vtable interop_table = {
	interop_query, interop_addref, interop_release, interop_handle, interop_handled
};

/* Cursor objects. CoreCursor is a plain runtime class: an activatable "Windows.UI.Core.CoreCursor" whose factory builds a
 * cursor of a CoreCursorType and whose RoActivateInstance answers the default, the arrow. The window's PointerCursor reads
 * as that arrow until an application puts one; a NULL put is legal and restores the platform default, so get answers the
 * same arrow instance again. Every cursor answers the standard 32x32 cursor size the platform uses for the built-in types;
 * the real ICoreCursor also exposes it as Size, but mingw's windows.ui.core.h declares get_Id/get_Type instead, so the id
 * the factory received stands in for it here (the default arrow answers 0). CoreCursorType_Custom names a cursor bitmap in
 * the application package's resources: with no package resources to load from, the factory rejects it. */
struct cursor {
	const __x_ABI_CWindows_CUI_CCore_CICoreCursorVtbl *lpVtbl;
	LONG refs;
	__x_ABI_CWindows_CUI_CCore_CCoreCursorType type;
	UINT32 id;
	BOOL allocated; /* the default arrow is a static instance that never dies */
};

static HRESULT WINAPI cursor_query(__x_ABI_CWindows_CUI_CCore_CICoreCursor *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID___x_ABI_CWindows_CUI_CCore_CICoreCursor))
		return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct cursor *)iface)->refs);
	return S_OK;
}

static ULONG WINAPI cursor_addref(__x_ABI_CWindows_CUI_CCore_CICoreCursor *iface)
{
	return (ULONG)InterlockedIncrement(&((struct cursor *)iface)->refs);
}

static ULONG WINAPI cursor_release(__x_ABI_CWindows_CUI_CCore_CICoreCursor *iface)
{
	struct cursor *self = (struct cursor *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs && self->allocated) {
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI cursor_iids(__x_ABI_CWindows_CUI_CCore_CICoreCursor *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI cursor_class_name(__x_ABI_CWindows_CUI_CCore_CICoreCursor *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(cursor_name, (UINT32)lstrlenW(cursor_name), name);
}

static HRESULT WINAPI cursor_trust(__x_ABI_CWindows_CUI_CCore_CICoreCursor *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI cursor_get_id(__x_ABI_CWindows_CUI_CCore_CICoreCursor *iface, UINT32 *value)
{
	if (!value)
		return E_POINTER;
	*value = ((struct cursor *)iface)->id;
	return S_OK;
}

static HRESULT WINAPI cursor_get_type(__x_ABI_CWindows_CUI_CCore_CICoreCursor *iface, __x_ABI_CWindows_CUI_CCore_CCoreCursorType *value)
{
	if (!value)
		return E_POINTER;
	*value = ((struct cursor *)iface)->type;
	return S_OK;
}

static const __x_ABI_CWindows_CUI_CCore_CICoreCursorVtbl cursor_table = {
	.QueryInterface = cursor_query,
	.AddRef = cursor_addref,
	.Release = cursor_release,
	.GetIids = cursor_iids,
	.GetRuntimeClassName = cursor_class_name,
	.GetTrustLevel = cursor_trust,
	.get_Id = cursor_get_id,
	.get_Type = cursor_get_type,
};

static struct cursor default_cursor = {&cursor_table, 1, CoreCursorType_Arrow, 0, FALSE};

/* The type range follows the same contract guard the public header declares the last values under. */
static __x_ABI_CWindows_CUI_CCore_CCoreCursorType cursor_type_last(void)
{
#if WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION >= 0x50000
	return CoreCursorType_Person;
#else
	return CoreCursorType_Wait;
#endif
}

static HRESULT cursor_create(__x_ABI_CWindows_CUI_CCore_CCoreCursorType type, UINT32 id, __x_ABI_CWindows_CUI_CCore_CICoreCursor **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (type < CoreCursorType_Arrow || type > cursor_type_last())
		return E_INVALIDARG;
	struct cursor *self = ALLOC(sizeof *self);
	if (!self)
		return E_OUTOFMEMORY;
	self->lpVtbl = &cursor_table;
	self->refs = 1;
	self->type = type;
	self->id = id;
	self->allocated = TRUE;
	InterlockedIncrement(&raven_xaml_objects);
	*out = (__x_ABI_CWindows_CUI_CCore_CICoreCursor *)self;
	return S_OK;
}

struct cursor_activation {
	const IActivationFactoryVtbl *lpVtbl;
	struct cursor_factory *owner;
};

struct cursor_icf {
	const __x_ABI_CWindows_CUI_CCore_CICoreCursorFactoryVtbl *lpVtbl;
	struct cursor_factory *owner;
};

struct cursor_factory {
	struct cursor_activation activation;
	LONG refs;
	struct cursor_icf factory;
};

static HRESULT cursor_factory_query(struct cursor_factory *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory)) {
		InterlockedIncrement(&self->refs);
		*out = &self->activation;
		return S_OK;
	}
	if (equal_iid(iid, &IID___x_ABI_CWindows_CUI_CCore_CICoreCursorFactory)) {
		InterlockedIncrement(&self->refs);
		*out = &self->factory;
		return S_OK;
	}
	return E_NOINTERFACE;
}

static HRESULT WINAPI cursor_activation_query(IActivationFactory *iface, REFIID iid, void **out)
{
	return cursor_factory_query(((struct cursor_activation *)iface)->owner, iid, out);
}

static ULONG WINAPI cursor_activation_addref(IActivationFactory *iface)
{
	return (ULONG)InterlockedIncrement(&((struct cursor_activation *)iface)->owner->refs);
}

static ULONG WINAPI cursor_activation_release(IActivationFactory *iface)
{
	return (ULONG)InterlockedDecrement(&((struct cursor_activation *)iface)->owner->refs);
}

static HRESULT WINAPI cursor_activation_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI cursor_activation_class_name(IActivationFactory *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(cursor_name, (UINT32)lstrlenW(cursor_name), name);
}

static HRESULT WINAPI cursor_activation_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI cursor_activation_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return cursor_create(CoreCursorType_Arrow, 0, (__x_ABI_CWindows_CUI_CCore_CICoreCursor **)out);
}

static const IActivationFactoryVtbl cursor_activation_table = {
	.QueryInterface = cursor_activation_query,
	.AddRef = cursor_activation_addref,
	.Release = cursor_activation_release,
	.GetIids = cursor_activation_iids,
	.GetRuntimeClassName = cursor_activation_class_name,
	.GetTrustLevel = cursor_activation_trust,
	.ActivateInstance = cursor_activation_activate,
};

static HRESULT WINAPI cursor_icf_query(__x_ABI_CWindows_CUI_CCore_CICoreCursorFactory *iface, REFIID iid, void **out)
{
	return cursor_factory_query(((struct cursor_icf *)iface)->owner, iid, out);
}

static ULONG WINAPI cursor_icf_addref(__x_ABI_CWindows_CUI_CCore_CICoreCursorFactory *iface)
{
	return (ULONG)InterlockedIncrement(&((struct cursor_icf *)iface)->owner->refs);
}

static ULONG WINAPI cursor_icf_release(__x_ABI_CWindows_CUI_CCore_CICoreCursorFactory *iface)
{
	return (ULONG)InterlockedDecrement(&((struct cursor_icf *)iface)->owner->refs);
}

static HRESULT WINAPI cursor_icf_iids(__x_ABI_CWindows_CUI_CCore_CICoreCursorFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI cursor_icf_class_name(__x_ABI_CWindows_CUI_CCore_CICoreCursorFactory *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(cursor_name, (UINT32)lstrlenW(cursor_name), name);
}

static HRESULT WINAPI cursor_icf_trust(__x_ABI_CWindows_CUI_CCore_CICoreCursorFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI cursor_icf_create(__x_ABI_CWindows_CUI_CCore_CICoreCursorFactory *iface,
	__x_ABI_CWindows_CUI_CCore_CCoreCursorType type, UINT32 id, __x_ABI_CWindows_CUI_CCore_CICoreCursor **out)
{
	(void)iface;
	/* CoreCursorType_Custom loads the cursor bitmap the resource id names in the application package; the emulated
	 * desktop has no package resources to load from, so it stays unsupported. Out-of-range types are rejected with
	 * E_INVALIDARG by cursor_create. */
	if (type == CoreCursorType_Custom)
		return E_NOTIMPL;
	return cursor_create(type, id, out);
}

static const __x_ABI_CWindows_CUI_CCore_CICoreCursorFactoryVtbl cursor_icf_table = {
	.QueryInterface = cursor_icf_query,
	.AddRef = cursor_icf_addref,
	.Release = cursor_icf_release,
	.GetIids = cursor_icf_iids,
	.GetRuntimeClassName = cursor_icf_class_name,
	.GetTrustLevel = cursor_icf_trust,
	.CreateCursor = cursor_icf_create,
};

static struct cursor_factory cursor_factory_instance = {
	{&cursor_activation_table, &cursor_factory_instance}, 1, {&cursor_icf_table, &cursor_factory_instance}};

/* Unsupported contracts fail explicitly; every slot follows the public mingw WinRT header. */
static HRESULT WINAPI window_unsupported_get_AutomationHostProvider(core_window *iface, IInspectable **value)
{
	if (!value) return E_POINTER;
	ZeroMemory(value, sizeof *value);
	return E_NOTIMPL;
}

static HRESULT WINAPI window_unsupported_get_CustomProperties(core_window *iface, __x_ABI_CWindows_CFoundation_CCollections_CIPropertySet **value)
{
	if (!value) return E_POINTER;
	ZeroMemory(value, sizeof *value);
	return E_NOTIMPL;
}

static HRESULT WINAPI window_unsupported_get_FlowDirection(core_window *iface, __x_ABI_CWindows_CUI_CCore_CCoreWindowFlowDirection *value)
{
	if (!value) return E_POINTER;
	ZeroMemory(value, sizeof *value);
	return E_NOTIMPL;
}

static HRESULT WINAPI window_unsupported_put_FlowDirection(core_window *iface, __x_ABI_CWindows_CUI_CCore_CCoreWindowFlowDirection value)
{
	return E_NOTIMPL;
}

/* The window's cursor: the arrow instance until an application puts one; a NULL put is legal and means the platform
 * default, so get answers the same arrow again afterwards. */
static HRESULT WINAPI window_get_PointerCursor(core_window *iface, __x_ABI_CWindows_CUI_CCore_CICoreCursor **value)
{
	struct window *self = (struct window *)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	EnterCriticalSection(&self->lock);
	if (self->closed) {
		LeaveCriticalSection(&self->lock);
		return RO_E_CLOSED;
	}
	if (self->cursor)
		*value = self->cursor;
	else
		*value = (__x_ABI_CWindows_CUI_CCore_CICoreCursor *)&default_cursor;
	__x_ABI_CWindows_CUI_CCore_CICoreCursor_AddRef(*value);
	LeaveCriticalSection(&self->lock);
	return S_OK;
}

static HRESULT WINAPI window_put_PointerCursor(core_window *iface, __x_ABI_CWindows_CUI_CCore_CICoreCursor *value)
{
	struct window *self = (struct window *)iface;
	EnterCriticalSection(&self->lock);
	if (self->closed) {
		LeaveCriticalSection(&self->lock);
		return RO_E_CLOSED;
	}
	if (value)
		__x_ABI_CWindows_CUI_CCore_CICoreCursor_AddRef(value);
	__x_ABI_CWindows_CUI_CCore_CICoreCursor *previous = self->cursor;
	self->cursor = value;
	LeaveCriticalSection(&self->lock);
	if (previous)
		__x_ABI_CWindows_CUI_CCore_CICoreCursor_Release(previous);
	return S_OK;
}

static HRESULT WINAPI window_get_PointerPosition(core_window *iface, __x_ABI_CWindows_CFoundation_CPoint *value)
{
	return raven_desktop_pointer_position(((struct window *)iface)->desktop, value);
}

static HRESULT WINAPI window_unsupported_GetAsyncKeyState(core_window *iface, __x_ABI_CWindows_CSystem_CVirtualKey key, __x_ABI_CWindows_CUI_CCore_CCoreVirtualKeyStates *state)
{
	if (!state) return E_POINTER;
	ZeroMemory(state, sizeof *state);
	return E_NOTIMPL;
}

static HRESULT WINAPI window_unsupported_GetKeyState(core_window *iface, __x_ABI_CWindows_CSystem_CVirtualKey key, __x_ABI_CWindows_CUI_CCore_CCoreVirtualKeyStates *state)
{
	if (!state) return E_POINTER;
	ZeroMemory(state, sizeof *state);
	return E_NOTIMPL;
}

static HRESULT WINAPI window_ReleasePointerCapture(core_window *iface)
{
	return raven_desktop_capture(((struct window *)iface)->desktop, FALSE);
}

static HRESULT WINAPI window_SetPointerCapture(core_window *iface)
{
	return raven_desktop_capture(((struct window *)iface)->desktop, TRUE);
}

static HRESULT WINAPI window_unsupported_add_AutomationProviderRequested(core_window *iface, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CAutomationProviderRequestedEventArgs *handler, EventRegistrationToken *cookie)
{
	if (!cookie) return E_POINTER;
	cookie->value = 0;
	if (!handler) return E_POINTER;
	return E_NOTIMPL;
}

static HRESULT WINAPI window_unsupported_remove_AutomationProviderRequested(core_window *iface, EventRegistrationToken cookie)
{
	return E_NOTIMPL;
}

static HRESULT WINAPI window_add_PointerCaptureLost(core_window *iface, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *handler, EventRegistrationToken *cookie)
{
	return raven_desktop_add(((struct window *)iface)->desktop, RAVEN_WINDOW_POINTER_CAPTURE_LOST, (IUnknown *)handler, cookie);
}

static HRESULT WINAPI window_remove_PointerCaptureLost(core_window *iface, EventRegistrationToken cookie)
{
	return raven_desktop_remove(((struct window *)iface)->desktop, RAVEN_WINDOW_POINTER_CAPTURE_LOST, cookie);
}

static HRESULT WINAPI window_add_PointerEntered(core_window *iface, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *handler, EventRegistrationToken *cookie)
{
	return raven_desktop_add(((struct window *)iface)->desktop, RAVEN_WINDOW_POINTER_ENTERED, (IUnknown *)handler, cookie);
}

static HRESULT WINAPI window_remove_PointerEntered(core_window *iface, EventRegistrationToken cookie)
{
	return raven_desktop_remove(((struct window *)iface)->desktop, RAVEN_WINDOW_POINTER_ENTERED, cookie);
}

static HRESULT WINAPI window_add_PointerExited(core_window *iface, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *handler, EventRegistrationToken *cookie)
{
	return raven_desktop_add(((struct window *)iface)->desktop, RAVEN_WINDOW_POINTER_EXITED, (IUnknown *)handler, cookie);
}

static HRESULT WINAPI window_remove_PointerExited(core_window *iface, EventRegistrationToken cookie)
{
	return raven_desktop_remove(((struct window *)iface)->desktop, RAVEN_WINDOW_POINTER_EXITED, cookie);
}

static HRESULT WINAPI window_add_PointerMoved(core_window *iface, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *handler, EventRegistrationToken *cookie)
{
	return raven_desktop_add(((struct window *)iface)->desktop, RAVEN_WINDOW_POINTER_MOVED, (IUnknown *)handler, cookie);
}

static HRESULT WINAPI window_remove_PointerMoved(core_window *iface, EventRegistrationToken cookie)
{
	return raven_desktop_remove(((struct window *)iface)->desktop, RAVEN_WINDOW_POINTER_MOVED, cookie);
}

static HRESULT WINAPI window_add_PointerPressed(core_window *iface, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *handler, EventRegistrationToken *cookie)
{
	return raven_desktop_add(((struct window *)iface)->desktop, RAVEN_WINDOW_POINTER_PRESSED, (IUnknown *)handler, cookie);
}

static HRESULT WINAPI window_remove_PointerPressed(core_window *iface, EventRegistrationToken cookie)
{
	return raven_desktop_remove(((struct window *)iface)->desktop, RAVEN_WINDOW_POINTER_PRESSED, cookie);
}

static HRESULT WINAPI window_add_PointerReleased(core_window *iface, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *handler, EventRegistrationToken *cookie)
{
	return raven_desktop_add(((struct window *)iface)->desktop, RAVEN_WINDOW_POINTER_RELEASED, (IUnknown *)handler, cookie);
}

static HRESULT WINAPI window_remove_PointerReleased(core_window *iface, EventRegistrationToken cookie)
{
	return raven_desktop_remove(((struct window *)iface)->desktop, RAVEN_WINDOW_POINTER_RELEASED, cookie);
}

static HRESULT WINAPI window_unsupported_add_TouchHitTesting(core_window *iface, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CTouchHitTestingEventArgs *handler, EventRegistrationToken *cookie)
{
	if (!cookie) return E_POINTER;
	cookie->value = 0;
	if (!handler) return E_POINTER;
	return E_NOTIMPL;
}

static HRESULT WINAPI window_unsupported_remove_TouchHitTesting(core_window *iface, EventRegistrationToken cookie)
{
	return E_NOTIMPL;
}

static HRESULT WINAPI window_add_PointerWheelChanged(core_window *iface, __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs *handler, EventRegistrationToken *cookie)
{
	return raven_desktop_add(((struct window *)iface)->desktop, RAVEN_WINDOW_POINTER_WHEEL, (IUnknown *)handler, cookie);
}

static HRESULT WINAPI window_remove_PointerWheelChanged(core_window *iface, EventRegistrationToken cookie)
{
	return raven_desktop_remove(((struct window *)iface)->desktop, RAVEN_WINDOW_POINTER_WHEEL, cookie);
}

static const __x_ABI_CWindows_CUI_CCore_CICoreWindowVtbl window_table = {
	.QueryInterface = window_query,
	.AddRef = window_addref,
	.Release = window_release,
	.GetIids = window_iids,
	.GetRuntimeClassName = window_class_name,
	.GetTrustLevel = window_trust,
	.get_Bounds = window_bounds,
	.get_Dispatcher = window_dispatcher,
	.get_IsInputEnabled = window_input_enabled,
	.put_IsInputEnabled = window_put_input_enabled,
	.get_Visible = window_visible,
	.Activate = window_activate,
	.Close = window_close,
	.add_Activated = window_add_Activated,
	.remove_Activated = window_remove_Activated,
	.add_Closed = window_add_Closed,
	.remove_Closed = window_remove_Closed,
	.add_SizeChanged = window_add_SizeChanged,
	.remove_SizeChanged = window_remove_SizeChanged,
	.add_VisibilityChanged = window_add_VisibilityChanged,
	.remove_VisibilityChanged = window_remove_VisibilityChanged,
	.add_KeyDown = window_add_KeyDown,
	.remove_KeyDown = window_remove_KeyDown,
	.add_KeyUp = window_add_KeyUp,
	.remove_KeyUp = window_remove_KeyUp,
	.add_CharacterReceived = window_add_CharacterReceived,
	.remove_CharacterReceived = window_remove_CharacterReceived,
	.add_InputEnabled = window_add_InputEnabled,
	.remove_InputEnabled = window_remove_InputEnabled,
	.get_AutomationHostProvider = window_unsupported_get_AutomationHostProvider,
	.get_CustomProperties = window_unsupported_get_CustomProperties,
	.get_FlowDirection = window_unsupported_get_FlowDirection,
	.put_FlowDirection = window_unsupported_put_FlowDirection,
	.get_PointerCursor = window_get_PointerCursor,
	.put_PointerCursor = window_put_PointerCursor,
	.get_PointerPosition = window_get_PointerPosition,
	.GetAsyncKeyState = window_unsupported_GetAsyncKeyState,
	.GetKeyState = window_unsupported_GetKeyState,
	.ReleasePointerCapture = window_ReleasePointerCapture,
	.SetPointerCapture = window_SetPointerCapture,
	.add_AutomationProviderRequested = window_unsupported_add_AutomationProviderRequested,
	.remove_AutomationProviderRequested = window_unsupported_remove_AutomationProviderRequested,
	.add_PointerCaptureLost = window_add_PointerCaptureLost,
	.remove_PointerCaptureLost = window_remove_PointerCaptureLost,
	.add_PointerEntered = window_add_PointerEntered,
	.remove_PointerEntered = window_remove_PointerEntered,
	.add_PointerExited = window_add_PointerExited,
	.remove_PointerExited = window_remove_PointerExited,
	.add_PointerMoved = window_add_PointerMoved,
	.remove_PointerMoved = window_remove_PointerMoved,
	.add_PointerPressed = window_add_PointerPressed,
	.remove_PointerPressed = window_remove_PointerPressed,
	.add_PointerReleased = window_add_PointerReleased,
	.remove_PointerReleased = window_remove_PointerReleased,
	.add_TouchHitTesting = window_unsupported_add_TouchHitTesting,
	.remove_TouchHitTesting = window_unsupported_remove_TouchHitTesting,
	.add_PointerWheelChanged = window_add_PointerWheelChanged,
	.remove_PointerWheelChanged = window_remove_PointerWheelChanged,
};

struct activation {
	const IActivationFactoryVtbl *lpVtbl;
	struct factory *owner;
};

struct statics {
	const __x_ABI_CWindows_CUI_CCore_CICoreWindowStaticVtbl *lpVtbl;
	struct factory *owner;
};

struct factory {
	struct activation activation;
	LONG refs;
	struct statics statics;
};

static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out);
static HRESULT WINAPI factory_query(IActivationFactory *iface, REFIID iid, void **out);
static ULONG WINAPI factory_addref(IActivationFactory *iface);
static ULONG WINAPI factory_release(IActivationFactory *iface);
static HRESULT WINAPI factory_iids(IActivationFactory *iface, ULONG *count, IID **iids);
static HRESULT WINAPI factory_class_name(IActivationFactory *iface, HSTRING *name);
static HRESULT WINAPI factory_trust(IActivationFactory *iface, TrustLevel *level);

static const IActivationFactoryVtbl factory_table = {
	.QueryInterface = factory_query,
	.AddRef = factory_addref,
	.Release = factory_release,
	.GetIids = factory_iids,
	.GetRuntimeClassName = factory_class_name,
	.GetTrustLevel = factory_trust,
	.ActivateInstance = factory_activate,
};

static HRESULT WINAPI statics_for_current_thread(core_window_static *iface, core_window **out);
static HRESULT WINAPI statics_query(core_window_static *iface, REFIID iid, void **out);
static ULONG WINAPI statics_addref(core_window_static *iface);
static ULONG WINAPI statics_release(core_window_static *iface);
static HRESULT WINAPI statics_iids(core_window_static *iface, ULONG *count, IID **iids);
static HRESULT WINAPI statics_class_name(core_window_static *iface, HSTRING *name);
static HRESULT WINAPI statics_trust(core_window_static *iface, TrustLevel *level);

static const __x_ABI_CWindows_CUI_CCore_CICoreWindowStaticVtbl statics_table = {
	.QueryInterface = statics_query,
	.AddRef = statics_addref,
	.Release = statics_release,
	.GetIids = statics_iids,
	.GetRuntimeClassName = statics_class_name,
	.GetTrustLevel = statics_trust,
	.GetForCurrentThread = statics_for_current_thread,
};

static struct factory factory_instance = {
	{&factory_table, &factory_instance}, 1, {&statics_table, &factory_instance}};

/* Async actions */

static struct action *info_owner(IAsyncInfo *iface)
{
	return CONTAINING_RECORD(iface, struct action, info);
}

/* Publish the result and take a callback reference under the same lock as put_Completed. */
static void action_finish(async_action *iface, HRESULT result, AsyncStatus status)
{
	struct action *self = (struct action *)iface;
	completed_handler *handler = NULL;
	EnterCriticalSection(&self->lock);
	if (self->status == Started) {
		self->status = status;
		self->error = result;
		handler = self->completed;
		if (handler)
			__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_AddRef(handler);
	}
	LeaveCriticalSection(&self->lock);
	if (handler) {
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Invoke(handler, iface, status);
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Release(handler);
	}
}

static HRESULT WINAPI action_query(async_action *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CFoundation_CIAsyncAction))
		*out = iface;
	else if (equal_iid(iid, &IID_IAsyncInfo))
		*out = &((struct action *)iface)->info;
	else
		return E_NOINTERFACE;
	action_addref(iface);
	return S_OK;
}

static ULONG WINAPI action_addref(async_action *iface)
{
	return (ULONG)InterlockedIncrement(&((struct action *)iface)->refs);
}

static ULONG WINAPI action_release(async_action *iface)
{
	struct action *self = (struct action *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->completed)
			__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Release(self->completed);
		DeleteCriticalSection(&self->lock);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI action_iids(async_action *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI action_class_name(async_action *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(action_name, (UINT32)lstrlenW(action_name), name);
}

static HRESULT WINAPI action_trust(async_action *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI action_put_completed(async_action *iface, completed_handler *handler)
{
	struct action *self = (struct action *)iface;
	HRESULT result = S_OK;
	AsyncStatus status;
	EnterCriticalSection(&self->lock);
	status = self->status;
	if (self->closed)
		result = RO_E_CLOSED;
	else if (self->assigned)
		result = E_ILLEGAL_DELEGATE_ASSIGNMENT;
	else {
		self->assigned = TRUE;
		self->completed = handler;
		if (handler) {
			__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_AddRef(handler);
			if (status != Started)
				__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_AddRef(handler);
		}
	}
	LeaveCriticalSection(&self->lock);
	if (SUCCEEDED(result) && handler && status != Started) {
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Invoke(handler, iface, status);
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Release(handler);
	}
	return result;
}

static HRESULT WINAPI action_get_completed(async_action *iface, completed_handler **handler)
{
	struct action *self = (struct action *)iface;
	HRESULT result = S_OK;
	if (!handler)
		return E_POINTER;
	*handler = NULL;
	EnterCriticalSection(&self->lock);
	if (self->closed)
		result = RO_E_CLOSED;
	else if ((*handler = self->completed))
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_AddRef(*handler);
	LeaveCriticalSection(&self->lock);
	return result;
}

static HRESULT WINAPI action_results(async_action *iface)
{
	struct action *self = (struct action *)iface;
	HRESULT result;
	EnterCriticalSection(&self->lock);
	result = self->closed ? RO_E_CLOSED : self->status == Started ? E_ILLEGAL_METHOD_CALL : self->error;
	LeaveCriticalSection(&self->lock);
	return result;
}

static HRESULT WINAPI info_query(IAsyncInfo *iface, REFIID iid, void **out)
{
	return action_query((async_action *)info_owner(iface), iid, out);
}
static ULONG WINAPI info_addref(IAsyncInfo *iface) { return action_addref((async_action *)info_owner(iface)); }
static ULONG WINAPI info_release(IAsyncInfo *iface) { return action_release((async_action *)info_owner(iface)); }
static HRESULT WINAPI info_iids(IAsyncInfo *iface, ULONG *count, IID **iids)
{
	return action_iids((async_action *)info_owner(iface), count, iids);
}
static HRESULT WINAPI info_class_name(IAsyncInfo *iface, HSTRING *name)
{
	return action_class_name((async_action *)info_owner(iface), name);
}
static HRESULT WINAPI info_trust(IAsyncInfo *iface, TrustLevel *level)
{
	return action_trust((async_action *)info_owner(iface), level);
}
static HRESULT WINAPI info_id(IAsyncInfo *iface, UINT32 *value)
{
	struct action *self = info_owner(iface);
	HRESULT result;
	if (!value) return E_POINTER;
	EnterCriticalSection(&self->lock);
	result = self->closed ? RO_E_CLOSED : S_OK;
	*value = self->id;
	LeaveCriticalSection(&self->lock);
	return result;
}
static HRESULT WINAPI info_status(IAsyncInfo *iface, AsyncStatus *value)
{
	struct action *self = info_owner(iface);
	HRESULT result;
	if (!value) return E_POINTER;
	EnterCriticalSection(&self->lock);
	result = self->closed ? RO_E_CLOSED : S_OK;
	*value = self->status;
	LeaveCriticalSection(&self->lock);
	return result;
}
static HRESULT WINAPI info_error(IAsyncInfo *iface, HRESULT *value)
{
	struct action *self = info_owner(iface);
	HRESULT result;
	if (!value) return E_POINTER;
	EnterCriticalSection(&self->lock);
	result = self->closed ? RO_E_CLOSED : S_OK;
	*value = self->error;
	LeaveCriticalSection(&self->lock);
	return result;
}
static HRESULT WINAPI info_cancel(IAsyncInfo *iface)
{
	struct action *self = info_owner(iface);
	EnterCriticalSection(&self->lock);
	BOOL closed = self->closed;
	LeaveCriticalSection(&self->lock);
	if (closed) return RO_E_CLOSED;
	action_finish((async_action *)self, HRESULT_FROM_WIN32(ERROR_CANCELLED), Canceled);
	return S_OK;
}
static HRESULT WINAPI info_close(IAsyncInfo *iface)
{
	struct action *self = info_owner(iface);
	completed_handler *handler = NULL;
	HRESULT result = S_OK;
	EnterCriticalSection(&self->lock);
	if (self->status == Started)
		result = E_ILLEGAL_METHOD_CALL;
	else {
		self->closed = TRUE;
		handler = self->completed;
		self->completed = NULL;
	}
	LeaveCriticalSection(&self->lock);
	if (handler)
		__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler_Release(handler);
	return result;
}
static IAsyncInfoVtbl info_table = {
	info_query, info_addref, info_release, info_iids, info_class_name, info_trust,
	info_id, info_status, info_error, info_cancel, info_close
};

/* Dispatchers */

static HRESULT WINAPI dispatcher_query(core_dispatcher *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CUI_CCore_CICoreDispatcher)) {
		InterlockedIncrement(&((struct dispatcher *)iface)->refs);
		*out = iface;
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI dispatcher_addref(core_dispatcher *iface)
{
	return (ULONG)InterlockedIncrement(&((struct dispatcher *)iface)->refs);
}

static ULONG WINAPI dispatcher_release(core_dispatcher *iface)
{
	struct dispatcher *self = (struct dispatcher *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		window_release((core_window *)self->window);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI dispatcher_iids(core_dispatcher *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI dispatcher_class_name(core_dispatcher *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(dispatcher_name, (UINT32)lstrlenW(dispatcher_name), name);
}

static HRESULT WINAPI dispatcher_trust(core_dispatcher *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI dispatcher_has_thread_access(core_dispatcher *iface, boolean *value)
{
	const struct dispatcher *self = (const struct dispatcher *)iface;
	if (!value)
		return E_POINTER;
	*value = (boolean)(self->window->thread == GetCurrentThreadId());
	return S_OK;
}

/* The ready event is reset while holding the queue lock so a concurrent post cannot be lost. */
static HRESULT WINAPI dispatcher_process_events(core_dispatcher *iface, __x_ABI_CWindows_CUI_CCore_CCoreProcessEventsOption options)
{
	struct dispatcher *self = (struct dispatcher *)iface;
	struct window *window = self->window;
	BOOL dispatched = FALSE;
	if (window->thread != GetCurrentThreadId())
		return RPC_E_WRONG_THREAD;
	if (options < CoreProcessEventsOption_ProcessOneAndAllPending || options > CoreProcessEventsOption_ProcessAllIfPresent)
		return E_INVALIDARG;
	for (;;) {
		struct queued *node;
		MSG message;
		BOOL desktop_event = FALSE;
		HRESULT event_error = raven_desktop_error(window->desktop);
		if (FAILED(event_error)) return event_error;
		/* Retrieve one desktop event before dispatcher work, so input and paint keep moving. */
		if (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
			if (message.message == WM_QUIT)
				return window_close((core_window *)window);
			TranslateMessage(&message);
			DispatchMessageW(&message);
			event_error = raven_desktop_error(window->desktop);
			if (FAILED(event_error)) return event_error;
			dispatched = desktop_event = TRUE;
			if (options == CoreProcessEventsOption_ProcessOneIfPresent)
				break;
		}
		EnterCriticalSection(&window->lock);
		if (window->closed) {
			LeaveCriticalSection(&window->lock);
			break;
		}
		node = window->head;
		if (node) {
			window->head = node->next;
			if (!window->head)
				window->tail = NULL;
		} else
			ResetEvent(window->ready);
		LeaveCriticalSection(&window->lock);
		if (!node) {
			if (desktop_event)
				continue;
			if (options != CoreProcessEventsOption_ProcessUntilQuit &&
				(options != CoreProcessEventsOption_ProcessOneAndAllPending || dispatched))
				break;
			DWORD wait = MsgWaitForMultipleObjectsEx(1, &window->ready, INFINITE,
				QS_ALLINPUT, MWMO_INPUTAVAILABLE);
			if (wait == WAIT_FAILED)
				return HRESULT_FROM_WIN32(GetLastError());
			continue;
		}
		HRESULT invoked = S_OK;
		if (node->action) {
			struct action *action = (struct action *)node->action;
			AsyncStatus status;
			info_status(&action->info, &status);
			if (status == Started) {
				HRESULT result = __x_ABI_CWindows_CUI_CCore_CIDispatchedHandler_Invoke(node->handler);
				action_finish(node->action, result, FAILED(result) ? Error : Completed);
			}
		} else invoked = __x_ABI_CWindows_CUI_CCore_CIDispatchedHandler_Invoke(node->handler);
		__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler_Release(node->handler);
		if (node->action) node->action->lpVtbl->Release(node->action);
		FREE(node);
		if (FAILED(invoked)) return invoked;
		dispatched = TRUE;
		if (options == CoreProcessEventsOption_ProcessOneIfPresent)
			break;
	}
	return S_OK;
}

static void queue_insert(struct window *window, struct queued *node)
{
	struct queued **position = &window->head;
	while (*position && (*position)->priority >= node->priority) position = &(*position)->next;
	node->next = *position;
	*position = node;
	if (!node->next) window->tail = node;
	/* Work above idle priority marks the queue busy for IsDispatcherIdle; idle-priority siblings and the below-idle
	 * timer tasks do not count (called with the queue lock held). */
	if (node->priority > CoreDispatcherPriority_Idle)
		++window->work_epoch;
	SetEvent(window->ready);
}
HRESULT raven_window_post(core_window *iface, dispatched_handler *callback, LONG priority, boolean *accepted)
{
	if (!accepted) return E_POINTER;
	*accepted = FALSE;
	if (!callback) return E_POINTER;
	struct queued *node = ALLOC(sizeof *node);
	if (!node) return E_OUTOFMEMORY;
	__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler_AddRef(callback);
	node->handler = callback;
	node->priority = priority;
	struct window *window = (struct window *)iface;
	EnterCriticalSection(&window->lock);
	if (!window->closed) {
		queue_insert(window, node);
		*accepted = TRUE;
	}
	LeaveCriticalSection(&window->lock);
	if (!*accepted) {
		__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler_Release(callback);
		FREE(node);
	}
	return S_OK;
}

static HRESULT WINAPI dispatcher_run_async(core_dispatcher *iface, __x_ABI_CWindows_CUI_CCore_CCoreDispatcherPriority priority,
	dispatched_handler *callback, async_action **operation)
{
	struct dispatcher *self = (struct dispatcher *)iface;
	struct action *action;
	struct queued *node;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	if (!callback)
		return E_POINTER;
	if (priority < CoreDispatcherPriority_Idle || priority > CoreDispatcherPriority_High)
		return E_INVALIDARG;
	if (!(action = ALLOC(sizeof *action)))
		return E_OUTOFMEMORY;
	action->lpVtbl = &action_table;
	action->refs = 2; /* the caller's reference and the queue's */
	action->status = Started;
	action->info.lpVtbl = &info_table;
	action->id = (UINT32)InterlockedIncrement(&next_action_id);
	InitializeCriticalSection(&action->lock);
	InterlockedIncrement(&raven_xaml_objects);
	if (!(node = ALLOC(sizeof *node))) {
		action->refs = 1;
		action_release((async_action *)action);
		return E_OUTOFMEMORY;
	}
	__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler_AddRef(callback);
	node->handler = callback;
	node->action = (async_action *)action;
	node->priority = priority;
	EnterCriticalSection(&self->window->lock);
	if (self->window->closed) {
		LeaveCriticalSection(&self->window->lock);
		action_finish((async_action *)action, S_OK, Completed);
		__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler_Release(callback);
		FREE(node);
		action_release((async_action *)action);
		*operation = (async_action *)action;
		return S_OK;
	}
	queue_insert(self->window, node);
	LeaveCriticalSection(&self->window->lock);
	*operation = (async_action *)action;
	return S_OK;
}

/* RunIdleAsync. The idle callback queues as an ordinary work item at CoreDispatcherPriority_Idle, so the existing pump
 * owns ordering, cancellation, completion and failure handling exactly as for RunAsync: the item is dispatched only
 * when it reaches the head of the queue, i.e. after every higher-priority item already queued has run, and the pump's
 * wait (MsgWaitForMultipleObjectsEx on the queue event and desktop messages) still blocks once nothing is queued. A
 * self-re-queueing idle callback therefore keeps the pump busy the same way a re-queueing RunAsync callback does.
 *
 * IsDispatcherIdle reports, at the moment of the call, whether any work item of priority higher than idle has been
 * posted since the idle item itself was queued. The queue counts such posts (queue_insert) and the property compares
 * that count live under the queue lock, so work that arrived while the item waited — or arrives while the callback
 * runs — turns it false as the public contract describes. A plain "is anything queued now" check cannot express that:
 * an idle item is only ever dequeued when nothing of higher priority remains, so the queue would always read empty by
 * the time the callback runs. The args object references the window so a handler that retains it reads the last state
 * after the queue is gone. */
struct idle_args {
	const __x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandlerArgsVtbl *lpVtbl;
	LONG refs;
	struct window *window;
	ULONG epoch;
};

static const WCHAR idle_args_name[] = L"Windows.UI.Core.IdleDispatchedHandlerArgs";

static HRESULT WINAPI idle_args_query(__x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandlerArgs *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandlerArgs)) {
		InterlockedIncrement(&((struct idle_args *)iface)->refs);
		*out = iface;
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI idle_args_addref(__x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandlerArgs *iface)
{
	return (ULONG)InterlockedIncrement(&((struct idle_args *)iface)->refs);
}

static ULONG WINAPI idle_args_release(__x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandlerArgs *iface)
{
	struct idle_args *self = (struct idle_args *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		window_release((core_window *)self->window);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI idle_args_iids(__x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandlerArgs *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI idle_args_class_name(__x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandlerArgs *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(idle_args_name, (UINT32)lstrlenW(idle_args_name), name);
}

static HRESULT WINAPI idle_args_trust(__x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandlerArgs *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI idle_args_is_idle(__x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandlerArgs *iface, boolean *value)
{
	struct idle_args *self = (struct idle_args *)iface;
	if (!value)
		return E_POINTER;
	EnterCriticalSection(&self->window->lock);
	*value = (boolean)(self->window->work_epoch == self->epoch);
	LeaveCriticalSection(&self->window->lock);
	return S_OK;
}

static const __x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandlerArgsVtbl idle_args_table = {
	idle_args_query, idle_args_addref, idle_args_release, idle_args_iids, idle_args_class_name, idle_args_trust,
	idle_args_is_idle,
};

/* The queued shape of an idle callback: an IDispatchedHandler the pump can invoke, carrying the idle delegate and its
 * args. The pump completed the action before invoking a queued item (cancelled actions are skipped) and completes it
 * after it returns — Error status on failure — so the adapter adds nothing to that contract. */
struct idle_work {
	const __x_ABI_CWindows_CUI_CCore_CIDispatchedHandlerVtbl *lpVtbl;
	LONG refs;
	__x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandler *handler;
	struct idle_args *args;
};

static HRESULT WINAPI idle_work_query(dispatched_handler *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID___x_ABI_CWindows_CUI_CCore_CIDispatchedHandler)) {
		InterlockedIncrement(&((struct idle_work *)iface)->refs);
		*out = iface;
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI idle_work_addref(dispatched_handler *iface)
{
	return (ULONG)InterlockedIncrement(&((struct idle_work *)iface)->refs);
}

static ULONG WINAPI idle_work_release(dispatched_handler *iface)
{
	struct idle_work *self = (struct idle_work *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		__x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandler_Release(self->handler);
		idle_args_release((void *)self->args);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI idle_work_invoke(dispatched_handler *iface)
{
	struct idle_work *self = (struct idle_work *)iface;
	return self->handler->lpVtbl->Invoke(self->handler, (void *)self->args);
}

static const __x_ABI_CWindows_CUI_CCore_CIDispatchedHandlerVtbl idle_work_table = {
	idle_work_query, idle_work_addref, idle_work_release, idle_work_invoke,
};

static HRESULT WINAPI dispatcher_run_idle_async(core_dispatcher *iface,
	__x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandler *callback, async_action **operation)
{
	struct dispatcher *self = (struct dispatcher *)iface;
	struct action *action;
	struct idle_args *args;
	struct idle_work *work;
	struct queued *node;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	if (!callback)
		return E_POINTER;
	/* Mirrors RunAsync deliberately: same action lifecycle, same closed-window behavior, so the documented action
	 * contract (status, Completed, GetResults, Close, Cancel before the callback runs) holds unchanged. */
	if (!(action = ALLOC(sizeof *action)))
		return E_OUTOFMEMORY;
	action->lpVtbl = &action_table;
	action->refs = 2; /* the caller's reference and the queue's */
	action->status = Started;
	action->info.lpVtbl = &info_table;
	action->id = (UINT32)InterlockedIncrement(&next_action_id);
	InitializeCriticalSection(&action->lock);
	InterlockedIncrement(&raven_xaml_objects);
	if (!(args = ALLOC(sizeof *args))) {
		action->refs = 1;
		action_release((async_action *)action);
		return E_OUTOFMEMORY;
	}
	args->lpVtbl = &idle_args_table;
	args->refs = 1;
	args->window = self->window;
	window_addref((core_window *)self->window); /* the args reads the window's epoch for as long as it lives */
	InterlockedIncrement(&raven_xaml_objects);
	if (!(work = ALLOC(sizeof *work))) {
		args->refs = 1;
		idle_args_release((void *)args);
		action->refs = 1;
		action_release((async_action *)action);
		return E_OUTOFMEMORY;
	}
	work->lpVtbl = &idle_work_table;
	work->refs = 1;
	work->handler = callback;
	work->args = args;
	InterlockedIncrement(&raven_xaml_objects);
	__x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandler_AddRef(callback);
	if (!(node = ALLOC(sizeof *node))) {
		__x_ABI_CWindows_CUI_CCore_CIIdleDispatchedHandler_Release(callback);
		idle_work_release((dispatched_handler *)work);
		action->refs = 1;
		action_release((async_action *)action);
		return E_OUTOFMEMORY;
	}
	node->handler = (dispatched_handler *)work;
	node->action = (async_action *)action;
	node->priority = CoreDispatcherPriority_Idle;
	EnterCriticalSection(&self->window->lock);
	/* The snapshot shares the lock with the insert, so every post counted here happened before the item queued. */
	args->epoch = self->window->work_epoch;
	if (self->window->closed) {
		LeaveCriticalSection(&self->window->lock);
		action_finish((async_action *)action, S_OK, Completed);
		FREE(node);
		idle_work_release((dispatched_handler *)work);
		action_release((async_action *)action);
		*operation = (async_action *)action;
		return S_OK;
	}
	__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler_AddRef((dispatched_handler *)work);
	queue_insert(self->window, node);
	LeaveCriticalSection(&self->window->lock);
	idle_work_release((dispatched_handler *)work); /* the creator's reference; the queue node holds its own */
	*operation = (async_action *)action;
	return S_OK;
}

/* Windows */

static HRESULT WINAPI window_query(core_window *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CUI_CCore_CICoreWindow)) {
		InterlockedIncrement(&((struct window *)iface)->refs);
		*out = iface;
		return S_OK;
	}
	if (equal_iid(iid, &raven_core_window_interop_iid)) {
		window_addref(iface);
		*out = &((struct window *)iface)->interop;
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI window_addref(core_window *iface)
{
	return (ULONG)InterlockedIncrement(&((struct window *)iface)->refs);
}

static ULONG WINAPI window_release(core_window *iface)
{
	struct window *self = (struct window *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		struct queued *node = self->head;
		while (node) {
			struct queued *next = node->next;
			__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler_Release(node->handler);
			if (node->action) node->action->lpVtbl->Release(node->action);
			FREE(node);
			node = next;
		}
		if (self->dispatcher)
			dispatcher_release(self->dispatcher);
		if (self->cursor)
			__x_ABI_CWindows_CUI_CCore_CICoreCursor_Release(self->cursor);
		raven_desktop_free(self->desktop);
		CloseHandle(self->ready);
		DeleteCriticalSection(&self->lock);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI window_iids(core_window *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI window_class_name(core_window *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(window_name, (UINT32)lstrlenW(window_name), name);
}

static HRESULT WINAPI window_trust(core_window *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI window_bounds(core_window *iface, __x_ABI_CWindows_CFoundation_CRect *value)
{
	return raven_desktop_bounds(((struct window *)iface)->desktop, value);
}

static HRESULT WINAPI window_dispatcher(core_window *iface, core_dispatcher **value)
{
	struct window *self = (struct window *)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	EnterCriticalSection(&self->lock);
	if (self->closed) {
		LeaveCriticalSection(&self->lock);
		return RO_E_CLOSED;
	}
	if (!self->dispatcher) {
		struct dispatcher *dispatcher;
		if (!(dispatcher = ALLOC(sizeof *dispatcher))) {
			LeaveCriticalSection(&self->lock);
			return E_OUTOFMEMORY;
		}
		dispatcher->lpVtbl = &dispatcher_table;
		dispatcher->refs = 1;
		dispatcher->window = self;
		InterlockedIncrement(&self->refs);
		InterlockedIncrement(&raven_xaml_objects);
		self->dispatcher = (core_dispatcher *)dispatcher;
	}
	InterlockedIncrement(&((struct dispatcher *)self->dispatcher)->refs);
	*value = self->dispatcher;
	LeaveCriticalSection(&self->lock);
	return S_OK;
}

static HRESULT WINAPI window_input_enabled(core_window *iface, boolean *value)
{
	return raven_desktop_input(((struct window *)iface)->desktop, value);
}
static HRESULT WINAPI window_put_input_enabled(core_window *iface, boolean value)
{
	return raven_desktop_put_input(((struct window *)iface)->desktop, value);
}
static HRESULT WINAPI window_visible(core_window *iface, boolean *value)
{
	return raven_desktop_visible(((struct window *)iface)->desktop, value);
}
static HRESULT WINAPI window_activate(core_window *iface)
{
	return raven_desktop_activate(((struct window *)iface)->desktop);
}

static HRESULT WINAPI window_close(core_window *iface)
{
	struct window *self = (struct window *)iface;
	struct queued *node;
	if (self->thread != GetCurrentThreadId())
		return RPC_E_WRONG_THREAD;
	EnterCriticalSection(&self->lock);
	if (self->closed) {
		LeaveCriticalSection(&self->lock);
		return S_OK;
	}
	self->closed = TRUE;
	node = self->head;
	self->head = self->tail = NULL;
	SetEvent(self->ready);
	LeaveCriticalSection(&self->lock);
	HRESULT shutdown = self->system_queue ? raven_dispatcher_queue_close(self->system_queue) : S_OK;
	while (node) {
		struct queued *next = node->next;
		if (node->action) action_finish(node->action, S_OK, Completed);
		__x_ABI_CWindows_CUI_CCore_CIDispatchedHandler_Release(node->handler);
		if (node->action) action_release(node->action);
		FREE(node);
		node = next;
	}
	HRESULT closed = self->desktop ? raven_desktop_close(self->desktop) : S_OK;
	return FAILED(shutdown) ? shutdown : closed;
}

/* The class factory and its statics */

static HRESULT WINAPI factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
	struct factory *self = ((struct activation *)iface)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID_IActivationFactory)) {
		InterlockedIncrement(&self->refs);
		*out = &self->activation;
		return S_OK;
	}
	if (equal_iid(iid, &IID___x_ABI_CWindows_CUI_CCore_CICoreWindowStatic)) {
		InterlockedIncrement(&self->refs);
		*out = &self->statics;
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI factory_addref(IActivationFactory *iface)
{
	return (ULONG)InterlockedIncrement(&((struct activation *)iface)->owner->refs);
}

static ULONG WINAPI factory_release(IActivationFactory *iface)
{
	return (ULONG)InterlockedDecrement(&((struct activation *)iface)->owner->refs);
}

static HRESULT WINAPI factory_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI factory_class_name(IActivationFactory *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(window_name, (UINT32)lstrlenW(window_name), name);
}

static HRESULT WINAPI factory_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	/* The window of a thread is asked for, never constructed by name. */
	return E_ILLEGAL_METHOD_CALL;
}

static HRESULT WINAPI statics_query(core_window_static *iface, REFIID iid, void **out)
{
	struct statics *self = (struct statics *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID_IActivationFactory)) {
		InterlockedIncrement(&self->owner->refs);
		*out = &self->owner->activation;
		return S_OK;
	}
	if (equal_iid(iid, &IID___x_ABI_CWindows_CUI_CCore_CICoreWindowStatic)) {
		InterlockedIncrement(&self->owner->refs);
		*out = iface;
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI statics_addref(core_window_static *iface)
{
	return (ULONG)InterlockedIncrement(&((struct statics *)iface)->owner->refs);
}

static ULONG WINAPI statics_release(core_window_static *iface)
{
	return (ULONG)InterlockedDecrement(&((struct statics *)iface)->owner->refs);
}

static HRESULT WINAPI statics_iids(core_window_static *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI statics_class_name(core_window_static *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(window_name, (UINT32)lstrlenW(window_name), name);
}

static HRESULT WINAPI statics_trust(core_window_static *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI statics_for_current_thread(core_window_static *iface, core_window **out)
{
	struct window *window;
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (window_tls == TLS_OUT_OF_INDEXES)
		return E_OUTOFMEMORY;
	window = TlsGetValue(window_tls);
	if (!window) {
		if (!(window = ALLOC(sizeof *window)))
			return E_OUTOFMEMORY;
		window->lpVtbl = &window_table;
		window->refs = 1; /* the thread's window lives as long as the thread */
		window->thread = GetCurrentThreadId();
		window->interop.lpVtbl = &interop_table;
		window->ready = CreateEventW(NULL, TRUE, FALSE, NULL);
		if (!window->ready) {
			FREE(window);
			return E_OUTOFMEMORY;
		}
		InitializeCriticalSection(&window->lock);
		InterlockedIncrement(&raven_xaml_objects);
		HRESULT result = raven_desktop_create((core_window *)window, &window->desktop);
		if (FAILED(result)) {
			window_release((core_window *)window);
			return result;
		}
		if (!TlsSetValue(window_tls, window)) {
			window_close((core_window *)window);
			window_release((core_window *)window);
			return E_OUTOFMEMORY;
		}
	}
	InterlockedIncrement(&window->refs);
	*out = (core_window *)window;
	return S_OK;
}

HRESULT raven_current_window(core_window **out)
{
	return statics_for_current_thread(NULL, out);
}

HRESULT raven_dispatcher_queue_current(__x_ABI_CWindows_CSystem_CIDispatcherQueue **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	struct window *window = window_tls == TLS_OUT_OF_INDEXES ? NULL : TlsGetValue(window_tls);
	if (!window || window->closed) return S_OK;
	if (!window->system_queue) {
		HWND hwnd;
		HRESULT result = raven_desktop_handle(window->desktop, &hwnd);
		if (FAILED(result)) return result;
		result = raven_dispatcher_queue_create((core_window *)window, hwnd, &window->system_queue);
		if (FAILED(result)) return result;
	}
	window->system_queue->lpVtbl->AddRef(window->system_queue);
	*out = window->system_queue;
	return S_OK;
}
HRESULT raven_window_timer(core_window *iface, UINT_PTR id, BOOL *handled)
{
	return raven_dispatcher_queue_timer(((struct window *)iface)->system_queue, id, handled);
}

/* The surface a composition target for this window presents into: the desktop window handle and its DIP-to-pixel scale. */
HRESULT raven_window_composition_surface(core_window *iface, HWND *hwnd, float *scale)
{
	struct window *self = (struct window *)iface;
	if (!hwnd || !scale)
		return E_POINTER;
	*hwnd = NULL;
	*scale = 1.0f;
	if (!self->desktop)
		return S_FALSE;
	const HRESULT result = raven_desktop_handle(self->desktop, hwnd);
	if (FAILED(result))
		return result;
	const UINT dpi = GetDpiForWindow(*hwnd);
	*scale = dpi ? (float)dpi / 96.0f : 1.0f;
	return S_OK;
}

/* The cursor class answers by name, the way CoreWindow does at the top of this dispatch: no runtime catalog entry routes
 * "Windows.UI.Core.CoreCursor" yet (RoGetActivationFactory cannot find it), so the component's own DllGetActivationFactory
 * is the only path to it until the catalog lists the class. */
static HRESULT raven_core_cursor_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	if (length != ARRAYSIZE(cursor_name) - 1 ||
		CompareStringOrdinal(text, (int)length, cursor_name, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	InterlockedIncrement(&cursor_factory_instance.refs);
	*out = (IActivationFactory *)&cursor_factory_instance.activation;
	return S_OK;
}

/* The property-set factory lives in application.c; its class catalog line is pending, so the export also serves guests directly. */
HRESULT raven_property_set_factory(HSTRING name, IActivationFactory **out);

/* PropertyValue lives in property_value.c; declared here so the manifest-driven activation chain can route to it. */
HRESULT raven_property_value_factory(HSTRING name, IActivationFactory **out);

/* Manifest-driven activation: a packaged app's manifest names an Applications/Application@EntryPoint runtime class — the
 * guest application's own XAML Application subclass (e.g. "Shapr3D.App"), derived from the class an app's
 * IXamlMetadataProvider serves. Real activation maps the entry point to that app-provided class; this component cannot
 * serve it, because the object the entry point names is created by the guest's own runtime (Application.Start), not by
 * any factory here. The chain below therefore answers CLASS_E_CLASSNOTAVAILABLE for an entry-point name like any
 * unknown class — activating it would fake an Application object the guest never created. The entry point travels the
 * honest route instead: the manifest reader in package.c captures it, and CoreApplication.Run passes it to the view's
 * IFrameworkView::Load, the argument a packaged activation carries. A host probe cannot exercise RoGetActivationFactory
 * for an entry-point-derived name either: the fixtures run without a packaged guest process. The same constraint bounds
 * CoreApplication.RunWithActivationFactories: it is the entry where the activation host would pull this class's factory
 * from the guest's own IGetActivationFactory callback, and with no packaged launch to deliver it the route only
 * validates the callback and refuses the run (E_NOTIMPL, see application.c) — it never manufactures a factory here. */

__declspec(dllexport) HRESULT WINAPI DllGetActivationFactory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	if (!out)
		return E_POINTER;
	*out = NULL;
	const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	if (length != ARRAYSIZE(window_name) - 1 ||
		CompareStringOrdinal(text, (int)length, window_name, -1, FALSE) != CSTR_EQUAL)
	{
		HRESULT result = raven_core_cursor_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_application_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_xaml_application_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_xaml_window_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_xaml_controls_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_xaml_elements_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_composition_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_xaml_resource_context_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_xaml_memory_manager_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_storage_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_style_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_dependency_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_pointer_device_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_property_set_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_property_value_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_automation_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_dictionary_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_mrt_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_package_factory(name, out);
		if (result == CLASS_E_CLASSNOTAVAILABLE)
			result = raven_dispatcher_queue_factory(name, out);
		return result;
	}
	InterlockedIncrement(&factory_instance.refs);
	*out = (IActivationFactory *)&factory_instance.activation;
	return S_OK;
}

__declspec(dllexport) HRESULT WINAPI DllCanUnloadNow(void) { return raven_xaml_objects ? S_FALSE : S_OK; }

/* Drop the thread's references; a caller holding a dispatcher can still inspect its closed queue. */
static void detach_window(void)
{
	struct window *window = window_tls == TLS_OUT_OF_INDEXES ? NULL : TlsGetValue(window_tls);
	core_dispatcher *dispatcher;
	__x_ABI_CWindows_CSystem_CIDispatcherQueue *system_queue;
	if (!window)
		return;
	window_close((core_window *)window);
	TlsSetValue(window_tls, NULL);
	EnterCriticalSection(&window->lock);
	dispatcher = window->dispatcher;
	window->dispatcher = NULL;
	system_queue = window->system_queue;
	window->system_queue = NULL;
	LeaveCriticalSection(&window->lock);
	if (dispatcher)
		dispatcher_release(dispatcher);
	if (system_queue) system_queue->lpVtbl->Release(system_queue);
	window_release((core_window *)window);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
	if (reason == DLL_PROCESS_ATTACH) {
		raven_desktop_module(instance, reason, reserved);
		window_tls = TlsAlloc();
		if (window_tls == TLS_OUT_OF_INDEXES)
			return FALSE;
		if (!raven_composition_notify(reason, reserved) || !raven_application_notify(reason, reserved) ||
			!raven_xaml_window_notify(reason, reserved)) {
			raven_xaml_window_notify(DLL_PROCESS_DETACH, (void *)1);
			TlsFree(window_tls);
			window_tls = TLS_OUT_OF_INDEXES;
			return FALSE;
		}
	} else if (reason == DLL_THREAD_DETACH) {
		raven_application_notify(reason, reserved);
		raven_xaml_window_notify(reason, reserved);
		detach_window();
	} else if (reason == DLL_PROCESS_DETACH && window_tls != TLS_OUT_OF_INDEXES) {
		raven_composition_notify(reason, reserved);
		raven_application_notify(reason, reserved);
		raven_xaml_window_notify(reason, reserved);
		if (!reserved)
			detach_window();
		TlsFree(window_tls);
		window_tls = TLS_OUT_OF_INDEXES;
		raven_desktop_module(instance, reason, reserved);
	}
	return TRUE;
}
