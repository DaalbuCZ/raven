/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.UI.Xaml.Window class of the Windows Runtime.
 *
 * A XAML application creates its main window from Application.OnLaunched. Window.Current is the window of the calling UI thread:
 * the first call creates it over that thread's CoreWindow, Activate shows it, and Close ends it so the next call starts a new one.
 * Content is stored as the interface a XAML tree would be built from; this stage does not instantiate UI elements yet, and the
 * composition and UI-context interfaces answer E_NOTIMPL.
 */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include "windows_ui_xaml_window.h"

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

typedef __x_ABI_CWindows_CUI_CXaml_CIWindow xaml_window1;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindow2 xaml_window2;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindow3 xaml_window3;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindow4 xaml_window4;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindowStatics xaml_window_statics;
typedef __x_ABI_CWindows_CUI_CCore_CICoreWindow core_window;
typedef __x_ABI_CWindows_CUI_CCore_CICoreDispatcher core_dispatcher;
typedef __x_ABI_CWindows_CFoundation_CRect xaml_rect;

static const WCHAR class_name[] = L"Windows.UI.Xaml.Window";
static DWORD window_tls = TLS_OUT_OF_INDEXES;

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

static HRESULT runtime_class_name(HSTRING *name)
{
	if (!name)
		return E_POINTER;
	return WindowsCreateString(class_name, (UINT32)lstrlenW(class_name), name);
}

static int equal_iid(REFIID left, const IID *right)
{
	return left->Data1 == right->Data1 && left->Data2 == right->Data2 && left->Data3 == right->Data3 &&
		left->Data4[0] == right->Data4[0] && left->Data4[1] == right->Data4[1] &&
		left->Data4[2] == right->Data4[2] && left->Data4[3] == right->Data4[3] &&
		left->Data4[4] == right->Data4[4] && left->Data4[5] == right->Data4[5] &&
		left->Data4[6] == right->Data4[6] && left->Data4[7] == right->Data4[7];
}

struct xaml_window;

struct window1_part { const __x_ABI_CWindows_CUI_CXaml_CIWindowVtbl *lpVtbl; struct xaml_window *owner; };
struct window2_part { const __x_ABI_CWindows_CUI_CXaml_CIWindow2Vtbl *lpVtbl; struct xaml_window *owner; };
struct window3_part { const __x_ABI_CWindows_CUI_CXaml_CIWindow3Vtbl *lpVtbl; struct xaml_window *owner; };
struct window4_part { const __x_ABI_CWindows_CUI_CXaml_CIWindow4Vtbl *lpVtbl; struct xaml_window *owner; };

struct xaml_window {
	struct window1_part first;
	struct window2_part second;
	struct window3_part third;
	struct window4_part fourth;
	LONG refs;
	HSTRING name;
	core_window *core;
	IInspectable *content;
	boolean visible;
	boolean closed;
};

static HRESULT window_query(struct xaml_window *self, REFIID iid, void **out);
static ULONG window_addref(struct xaml_window *self);
static ULONG window_release(struct xaml_window *self);

#define PART_BOILERPLATE(part, iface, iid_name) \
	static HRESULT WINAPI part##_query(iface *iface##_unused, REFIID iid, void **out) \
	{ return window_query(((struct part *)iface##_unused)->owner, iid, out); } \
	static ULONG WINAPI part##_addref(iface *iface##_unused) \
	{ return window_addref(((struct part *)iface##_unused)->owner); } \
	static ULONG WINAPI part##_release(iface *iface##_unused) \
	{ return window_release(((struct part *)iface##_unused)->owner); } \
	static HRESULT WINAPI part##_iids(iface *iface##_unused, ULONG *count, IID **iids) \
	{ (void)iface##_unused; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI part##_name(iface *iface##_unused, HSTRING *name) \
	{ (void)iface##_unused; return runtime_class_name(name); } \
	static HRESULT WINAPI part##_trust(iface *iface##_unused, TrustLevel *level) \
	{ (void)iface##_unused; return inspectable_trust(level); }

PART_BOILERPLATE(window1_part, xaml_window1, IWindow)
PART_BOILERPLATE(window2_part, xaml_window2, IWindow2)
PART_BOILERPLATE(window3_part, xaml_window3, IWindow3)
PART_BOILERPLATE(window4_part, xaml_window4, IWindow4)

static HRESULT window_query(struct xaml_window *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIWindow))
		*out = &self->first;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIWindow2))
		*out = &self->second;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIWindow3))
		*out = &self->third;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIWindow4))
		*out = &self->fourth;
	else
		return E_NOINTERFACE;
	window_addref(self);
	return S_OK;
}

static ULONG window_addref(struct xaml_window *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG window_release(struct xaml_window *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->name)
			WindowsDeleteString(self->name);
		if (self->content)
			IInspectable_Release(self->content);
		if (self->core)
			__x_ABI_CWindows_CUI_CCore_CICoreWindow_Release(self->core);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static void forget_current(struct xaml_window *self)
{
	if (window_tls == TLS_OUT_OF_INDEXES || TlsGetValue(window_tls) != self)
		return;
	TlsSetValue(window_tls, NULL);
	window_release(self);
}

static HRESULT WINAPI window_bounds(xaml_window1 *iface, uint64_t out)
{
	struct xaml_window *self = ((struct window1_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	if (!self->core) {
		*(xaml_rect *)(uintptr_t)out = (xaml_rect){0, 0, 0, 0};
		return S_OK;
	}
	return __x_ABI_CWindows_CUI_CCore_CICoreWindow_get_Bounds(self->core, (xaml_rect *)(uintptr_t)out);
}

static HRESULT WINAPI window_visible(xaml_window1 *iface, uint64_t out)
{
	struct xaml_window *self = ((struct window1_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	*(boolean *)(uintptr_t)out = self->visible;
	return S_OK;
}

static HRESULT WINAPI window_content(xaml_window1 *iface, uint64_t out)
{
	struct xaml_window *self = ((struct window1_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = self->content;
	if (self->content)
		IInspectable_AddRef(self->content);
	return S_OK;
}

static HRESULT WINAPI window_put_content(xaml_window1 *iface, uint64_t value)
{
	struct xaml_window *self = ((struct window1_part *)iface)->owner;
	IInspectable *content = (IInspectable *)(uintptr_t)value;
	if (content)
		IInspectable_AddRef(content);
	if (self->content)
		IInspectable_Release(self->content);
	self->content = content;
	/* The hosted tree renders into the window's composition target. */
	return raven_render_hosted(self->core, content);
}

static HRESULT WINAPI window_core_window(xaml_window1 *iface, uint64_t out)
{
	struct xaml_window *self = ((struct window1_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	if (!self->core)
		return S_OK;
	__x_ABI_CWindows_CUI_CCore_CICoreWindow_AddRef(self->core);
	*(void **)(uintptr_t)out = self->core;
	return S_OK;
}

static HRESULT WINAPI window_dispatcher(xaml_window1 *iface, uint64_t out)
{
	struct xaml_window *self = ((struct window1_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	if (!self->core)
		return S_OK;
	return __x_ABI_CWindows_CUI_CCore_CICoreWindow_get_Dispatcher(self->core, (core_dispatcher **)(uintptr_t)out);
}

/* The window's events are accepted with a token and never raised; the XAML tree this window will host is not built yet. */
static HRESULT WINAPI window_event_add(xaml_window1 *iface, uint64_t handler, uint64_t token)
{
	(void)iface;
	(void)handler;
	if (token)
		*(uint64_t *)(uintptr_t)token = 1;
	return S_OK;
}

static HRESULT WINAPI window_event_remove(xaml_window1 *iface, uint64_t token)
{
	(void)iface;
	(void)token;
	return S_OK;
}

static HRESULT WINAPI window_activate(xaml_window1 *iface)
{
	struct xaml_window *self = ((struct window1_part *)iface)->owner;
	if (self->closed)
		return S_OK;
	/* The XAML window becomes visible even where the host cannot show the CoreWindow (a headless test run). */
	if (self->core)
		(void)__x_ABI_CWindows_CUI_CCore_CICoreWindow_Activate(self->core);
	self->visible = TRUE;
	return S_OK;
}

static HRESULT WINAPI window_close(xaml_window1 *iface)
{
	struct xaml_window *self = ((struct window1_part *)iface)->owner;
	if (!self->closed) {
		self->closed = TRUE;
		self->visible = FALSE;
		if (self->core)
			__x_ABI_CWindows_CUI_CCore_CICoreWindow_Close(self->core);
		forget_current(self);
	}
	return S_OK;
}

static HRESULT WINAPI window_set_title_bar(xaml_window2 *iface, uint64_t element)
{
	(void)iface;
	(void)element;
	return S_OK;
}

static HRESULT WINAPI window_compositor(xaml_window3 *iface, uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	/* The composition layer is a later stage. */
	return E_NOTIMPL;
}

static HRESULT WINAPI window_ui_context(xaml_window4 *iface, uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	return E_NOTIMPL;
}

static const __x_ABI_CWindows_CUI_CXaml_CIWindowVtbl window1_table = {
	.QueryInterface = window1_part_query,
	.AddRef = window1_part_addref,
	.Release = window1_part_release,
	.GetIids = window1_part_iids,
	.GetRuntimeClassName = window1_part_name,
	.GetTrustLevel = window1_part_trust,
	.get_Bounds = window_bounds,
	.get_Visible = window_visible,
	.get_Content = window_content,
	.put_Content = window_put_content,
	.get_CoreWindow = window_core_window,
	.get_Dispatcher = window_dispatcher,
	.add_Activated = window_event_add,
	.remove_Activated = window_event_remove,
	.add_Closed = window_event_add,
	.remove_Closed = window_event_remove,
	.add_SizeChanged = window_event_add,
	.remove_SizeChanged = window_event_remove,
	.add_VisibilityChanged = window_event_add,
	.remove_VisibilityChanged = window_event_remove,
	.Activate = window_activate,
	.Close = window_close,
};

static const __x_ABI_CWindows_CUI_CXaml_CIWindow2Vtbl window2_table = {
	.QueryInterface = window2_part_query,
	.AddRef = window2_part_addref,
	.Release = window2_part_release,
	.GetIids = window2_part_iids,
	.GetRuntimeClassName = window2_part_name,
	.GetTrustLevel = window2_part_trust,
	.SetTitleBar = window_set_title_bar,
};

static const __x_ABI_CWindows_CUI_CXaml_CIWindow3Vtbl window3_table = {
	.QueryInterface = window3_part_query,
	.AddRef = window3_part_addref,
	.Release = window3_part_release,
	.GetIids = window3_part_iids,
	.GetRuntimeClassName = window3_part_name,
	.GetTrustLevel = window3_part_trust,
	.get_Compositor = window_compositor,
};

static const __x_ABI_CWindows_CUI_CXaml_CIWindow4Vtbl window4_table = {
	.QueryInterface = window4_part_query,
	.AddRef = window4_part_addref,
	.Release = window4_part_release,
	.GetIids = window4_part_iids,
	.GetRuntimeClassName = window4_part_name,
	.GetTrustLevel = window4_part_trust,
	.get_UIContext = window_ui_context,
};

static struct xaml_window *create_window(void)
{
	struct xaml_window *window = ALLOC(sizeof *window);
	if (!window)
		return NULL;
	window->first.lpVtbl = &window1_table;
	window->first.owner = window;
	window->second.lpVtbl = &window2_table;
	window->second.owner = window;
	window->third.lpVtbl = &window3_table;
	window->third.owner = window;
	window->fourth.lpVtbl = &window4_table;
	window->fourth.owner = window;
	window->refs = 1;
	if (FAILED(raven_current_window(&window->core)) || !window->core) {
		FREE(window);
		return NULL;
	}
	InterlockedIncrement(&raven_xaml_objects);
	return window;
}

static HRESULT WINAPI statics_current(xaml_window_statics *iface, uint64_t out)
{
	struct xaml_window *window;
	(void)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	if (window_tls == TLS_OUT_OF_INDEXES)
		return E_OUTOFMEMORY;
	window = TlsGetValue(window_tls);
	if (!window || window->closed) {
		if (!(window = create_window()))
			return E_OUTOFMEMORY;
		if (!TlsSetValue(window_tls, window)) {
			window_release(window);
			return E_OUTOFMEMORY;
		}
	}
	window_addref(window);
	*(void **)(uintptr_t)out = &window->first;
	return S_OK;
}

struct window_factory;

struct factory_activation { const IActivationFactoryVtbl *lpVtbl; struct window_factory *owner; };
struct factory_statics {
	const __x_ABI_CWindows_CUI_CXaml_CIWindowStaticsVtbl *lpVtbl;
	struct window_factory *owner;
};

struct window_factory {
	struct factory_activation activation;
	struct factory_statics statics;
	LONG refs;
};

static HRESULT factory_query(struct window_factory *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory))
		*out = &self->activation;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIWindowStatics))
		*out = &self->statics;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static HRESULT WINAPI activation_query(IActivationFactory *iface, REFIID iid, void **out)
{
	return factory_query(((struct factory_activation *)iface)->owner, iid, out);
}

static ULONG WINAPI activation_addref(IActivationFactory *iface)
{
	return (ULONG)InterlockedIncrement(&((struct factory_activation *)iface)->owner->refs);
}

static ULONG WINAPI activation_release(IActivationFactory *iface)
{
	return (ULONG)InterlockedDecrement(&((struct factory_activation *)iface)->owner->refs);
}

static HRESULT WINAPI activation_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI activation_name(IActivationFactory *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name);
}

static HRESULT WINAPI activation_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

/* A window is asked for through Window.Current, never constructed by name. */
static HRESULT WINAPI activation_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return 0x8000000eU;
}

static HRESULT WINAPI statics_query(xaml_window_statics *iface, REFIID iid, void **out)
{
	return factory_query(((struct factory_statics *)iface)->owner, iid, out);
}

static ULONG WINAPI statics_addref(xaml_window_statics *iface)
{
	return (ULONG)InterlockedIncrement(&((struct factory_statics *)iface)->owner->refs);
}

static ULONG WINAPI statics_release(xaml_window_statics *iface)
{
	return (ULONG)InterlockedDecrement(&((struct factory_statics *)iface)->owner->refs);
}

static HRESULT WINAPI statics_iids(xaml_window_statics *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI statics_name(xaml_window_statics *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name);
}

static HRESULT WINAPI statics_trust(xaml_window_statics *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static const IActivationFactoryVtbl activation_table = {
	.QueryInterface = activation_query,
	.AddRef = activation_addref,
	.Release = activation_release,
	.GetIids = activation_iids,
	.GetRuntimeClassName = activation_name,
	.GetTrustLevel = activation_trust,
	.ActivateInstance = activation_activate,
};

static const __x_ABI_CWindows_CUI_CXaml_CIWindowStaticsVtbl statics_table = {
	.QueryInterface = statics_query,
	.AddRef = statics_addref,
	.Release = statics_release,
	.GetIids = statics_iids,
	.GetRuntimeClassName = statics_name,
	.GetTrustLevel = statics_trust,
	.get_Current = statics_current,
};

static struct window_factory factory_instance = {
	{&activation_table, &factory_instance}, {&statics_table, &factory_instance}, 1};

HRESULT raven_xaml_window_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (length != ARRAYSIZE(class_name) - 1 || CompareStringOrdinal(text, (int)length, class_name, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	InterlockedIncrement(&factory_instance.refs);
	*out = (IActivationFactory *)&factory_instance.activation;
	return S_OK;
}

HRESULT raven_xaml_window_set_name(IInspectable *object, const WCHAR *text, UINT32 length)
{
	xaml_window1 *window = NULL;
	if (!object)
		return E_POINTER;
	if (FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CIWindow, (void **)&window)) || !window)
		return S_FALSE;
	if (window->lpVtbl == &window1_table) {
		struct xaml_window *self = ((struct window1_part *)window)->owner;
		if (self->name) {
			WindowsDeleteString(self->name);
			self->name = NULL;
		}
		if (text && length)
			WindowsCreateString(text, length, &self->name);
	}
	window->lpVtbl->Release(window);
	return S_OK;
}

BOOL raven_xaml_window_notify(DWORD reason, void *reserved)
{
	if (reason == DLL_PROCESS_ATTACH) {
		window_tls = TlsAlloc();
		return window_tls != TLS_OUT_OF_INDEXES;
	}
	if (window_tls == TLS_OUT_OF_INDEXES)
		return TRUE;
	/* At process exit the guest is being torn down, as for the CoreWindow: no guest state is touched then. */
	if (reason == DLL_THREAD_DETACH || (reason == DLL_PROCESS_DETACH && !reserved)) {
		struct xaml_window *window = TlsGetValue(window_tls);
		TlsSetValue(window_tls, NULL);
		if (window)
			window_release(window);
	}
	if (reason == DLL_PROCESS_DETACH) {
		TlsFree(window_tls);
		window_tls = TLS_OUT_OF_INDEXES;
	}
	return TRUE;
}
