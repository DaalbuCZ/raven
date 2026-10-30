/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the automation peer classes.
 *
 * FrameworkElementAutomationPeer is the peer the engine answers for an element. The statics the winmd
 * declares for the class (generated into windows_ui_xaml_automation_peers.h) are CreatePeerForElement
 * and FromElement: both hand out one peer per element per process. The identity is a registry keyed by
 * the element pointer as the engine hands it out — the IUIElement entry a Children vector returns for
 * the element, the same pointer every caller receives — so the key identifies the object across
 * callers. The registry holds one reference to each peer for the process lifetime and entries are
 * never removed, so peers live for the process; a peer in turn holds its owner element alive, which
 * keeps the key valid for every later lookup. The consequence is by design: once a peer exists for an
 * element the object count never reaches zero again, and DllCanUnloadNow answers S_FALSE for the rest
 * of the process.
 *
 * The peer answers IAutomationPeer (the interface the class inherits from AutomationPeer) and
 * IFrameworkElementAutomationPeer (its own default interface, which carries get_Owner). GetName is
 * real — the element's automation name through the readback in xaml_elements.c (xaml_automation.h);
 * IsContentElement and IsControlElement answer TRUE; GetPattern answers E_NOTIMPL for every pattern
 * because no pattern provider is implemented; the rest of the declared slots answer E_NOTIMPL at their
 * exact signatures. The versioned IAutomationPeer2..9 and the Overrides interfaces the base class also
 * carries are not answered yet, and neither are the interfaces only a subclass's projection would use.
 *
 * The factory answers IActivationFactory plus the two interfaces the winmd attaches to the class: the
 * statics (IFrameworkElementAutomationPeerStatics) and the composable factory
 * (IFrameworkElementAutomationPeerFactory). The factory interfaces reject the aggregation outer, which
 * nothing here consumes. The activation key is the Controls.Primitives spelling the catalog registers;
 * the runtime class name is the Automation.Peers namespace the winmd declares the class in. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <string.h>
#include "windows_ui_xaml_uielement.h"
#include "windows_ui_xaml_automation_peers.h"
#include "xaml_automation.h"

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))
#define REALLOC(block, size) HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (block), (size))

typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer peer_automation1;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeerVtbl peer_automation1_vtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeer peer_framework1;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerVtbl peer_framework1_vtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics peer_statics1;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStaticsVtbl peer_statics1_vtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerFactory peer_composable1;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerFactoryVtbl peer_composable1_vtbl;

/* The IUIElement the peers key on and get_Owner returns: the element pointer as the engine hands it
 * out (the Children vector's entry, which is the element object itself); every part of the element
 * answers for the same refcount, so the reference the peer takes through it holds the object. */
typedef __x_ABI_CWindows_CUI_CXaml_CIUIElement ui_element;

/* The runtime class of the peer object, the namespace the winmd declares the class in. */
static const WCHAR peer_class[] = L"Windows.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer";
/* The activation key the catalog routes to this component (registry.cpp); the Controls.Primitives
 * spelling the activatable class is registered under, not the runtime class namespace above. */
static const WCHAR peer_key[] = L"Windows.UI.Xaml.Controls.Primitives.FrameworkElementAutomationPeer";

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

struct peer;

/* The parts of the peer: every part answers as the one COM object the way the element parts do, with
 * the identity, the refcount and the owner living on the peer itself. */
struct peer_automation_part { const peer_automation1_vtbl *lpVtbl; struct peer *owner; };
struct peer_framework_part { const peer_framework1_vtbl *lpVtbl; struct peer *owner; };

struct peer {
	const IInspectableVtbl *lpVtbl;
	struct peer_automation_part automation;
	struct peer_framework_part framework;
	LONG refs;
	/* The element the peer answers for, AddRef'd through its IUIElement part: this is both the
	 * registry key and what get_Owner returns. NULL for the default-constructed peer. */
	ui_element *owner;
};

static HRESULT peer_query(IInspectable *This, REFIID iid, void **out);
static ULONG peer_addref(struct peer *self);
static ULONG peer_release(struct peer *self);

static HRESULT WINAPI peer_inspectable_query(IInspectable *This, REFIID iid, void **out)
{
	return peer_query(This, iid, out);
}
static ULONG WINAPI peer_inspectable_addref(IInspectable *This)
{
	return peer_addref((struct peer *)This);
}
static ULONG WINAPI peer_inspectable_release(IInspectable *This)
{
	return peer_release((struct peer *)This);
}
static HRESULT WINAPI peer_inspectable_iids(IInspectable *This, ULONG *count, IID **iids)
{
	(void)This;
	return inspectable_iids(count, iids);
}
static HRESULT WINAPI peer_inspectable_name(IInspectable *This, HSTRING *name)
{
	(void)This;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(peer_class, (UINT32)lstrlenW(peer_class), name);
}
static HRESULT WINAPI peer_inspectable_trust(IInspectable *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}
static const IInspectableVtbl peer_inspectable_table = {
	peer_inspectable_query, peer_inspectable_addref, peer_inspectable_release,
	peer_inspectable_iids, peer_inspectable_name, peer_inspectable_trust
};

/* The slots the engine does not implement answer E_NOTIMPL with the exact signature the generated
 * header declares. Getters clear their out parameter first; an in-only slot touches nothing. */
static HRESULT WINAPI peer_stub0(peer_automation1 *This)
{
	(void)This;
	return E_NOTIMPL;
}
static HRESULT WINAPI peer_stub1(peer_automation1 *This, uint64_t a1)
{
	(void)This;
	if (a1)
		*(void **)(uintptr_t)a1 = NULL;
	return E_NOTIMPL;
}
static HRESULT WINAPI peer_stub1_in(peer_automation1 *This, uint64_t a1)
{
	(void)This;
	(void)a1;
	return E_NOTIMPL;
}
/* The two-argument slots carry an out pointer in the second word (GetPattern, GetPeerFromPoint); no
 * pattern provider is implemented, so both answer E_NOTIMPL the way an empty GetPatternCore override
 * would surface through the base implementation. */
static HRESULT WINAPI peer_stub2(peer_automation1 *This, uint64_t a1, uint64_t a2)
{
	(void)This;
	(void)a1;
	if (a2)
		*(void **)(uintptr_t)a2 = NULL;
	return E_NOTIMPL;
}
static HRESULT WINAPI peer_stub3(peer_automation1 *This, uint64_t a1, uint64_t a2, uint64_t a3)
{
	(void)This;
	(void)a1;
	(void)a2;
	(void)a3;
	return E_NOTIMPL;
}

/* The element is a content element and a control element: the engine's elements are the controls of
 * the page, and the base class defaults both to TRUE for the controls it builds peers for. */
static HRESULT WINAPI peer_is_content_element(peer_automation1 *This, uint64_t out)
{
	(void)This;
	if (!out)
		return E_POINTER;
	*(boolean *)(uintptr_t)out = TRUE;
	return S_OK;
}

/* GetName reads the element's automation name through the readback in xaml_elements.c; the default-
 * constructed peer has no element to read a name from. */
static HRESULT WINAPI peer_get_name(peer_automation1 *This, uint64_t out)
{
	struct peer *self = ((struct peer_automation_part *)This)->owner;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	if (!self->owner)
		return E_FAIL;
	return raven_xaml_element_automation_name((IInspectable *)self->owner, (HSTRING *)(uintptr_t)out);
}

static HRESULT WINAPI peer_automation_query(peer_automation1 *This, REFIID iid, void **out)
{
	return peer_query((IInspectable *)((struct peer_automation_part *)This)->owner, iid, out);
}
static ULONG WINAPI peer_automation_addref(peer_automation1 *This)
{
	return peer_addref(((struct peer_automation_part *)This)->owner);
}
static ULONG WINAPI peer_automation_release(peer_automation1 *This)
{
	return peer_release(((struct peer_automation_part *)This)->owner);
}
static HRESULT WINAPI peer_automation_iids(peer_automation1 *This, ULONG *count, IID **iids)
{
	(void)This;
	return inspectable_iids(count, iids);
}
static HRESULT WINAPI peer_automation_name(peer_automation1 *This, HSTRING *name)
{
	(void)This;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(peer_class, (UINT32)lstrlenW(peer_class), name);
}
static HRESULT WINAPI peer_automation_trust(peer_automation1 *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}

/* The vtable follows the generated windows_ui_xaml_automation_peers.h slot for slot: IAutomationPeer
 * declares 27 methods after the IInspectable ones, and every unimplemented slot carries the E_NOTIMPL
 * stub of its arity. */
static const peer_automation1_vtbl peer_automation_table = {
	.QueryInterface = peer_automation_query,
	.AddRef = peer_automation_addref,
	.Release = peer_automation_release,
	.GetIids = peer_automation_iids,
	.GetRuntimeClassName = peer_automation_name,
	.GetTrustLevel = peer_automation_trust,
	.get_EventsSource = peer_stub1,
	.put_EventsSource = peer_stub1_in,
	.GetPattern = peer_stub2,
	.RaiseAutomationEvent = peer_stub1_in,
	.RaisePropertyChangedEvent = peer_stub3,
	.GetAcceleratorKey = peer_stub1,
	.GetAccessKey = peer_stub1,
	.GetAutomationControlType = peer_stub1,
	.GetAutomationId = peer_stub1,
	.GetBoundingRectangle = peer_stub1,
	.GetChildren = peer_stub1,
	.GetClassName = peer_stub1,
	.GetClickablePoint = peer_stub1,
	.GetHelpText = peer_stub1,
	.GetItemStatus = peer_stub1,
	.GetItemType = peer_stub1,
	.GetLabeledBy = peer_stub1,
	.GetLocalizedControlType = peer_stub1,
	.GetName = peer_get_name,
	.GetOrientation = peer_stub1,
	.HasKeyboardFocus = peer_stub1,
	.IsContentElement = peer_is_content_element,
	.IsControlElement = peer_is_content_element,
	.IsEnabled = peer_stub1,
	.IsKeyboardFocusable = peer_stub1,
	.IsOffscreen = peer_stub1,
	.IsPassword = peer_stub1,
	.IsRequiredForForm = peer_stub1,
	.SetFocus = peer_stub0,
	.GetParent = peer_stub1,
	.InvalidatePeer = peer_stub0,
	.GetPeerFromPoint = peer_stub2,
	.GetLiveSetting = peer_stub1,
};

static HRESULT WINAPI peer_framework_query(peer_framework1 *This, REFIID iid, void **out)
{
	return peer_query((IInspectable *)((struct peer_framework_part *)This)->owner, iid, out);
}
static ULONG WINAPI peer_framework_addref(peer_framework1 *This)
{
	return peer_addref(((struct peer_framework_part *)This)->owner);
}
static ULONG WINAPI peer_framework_release(peer_framework1 *This)
{
	return peer_release(((struct peer_framework_part *)This)->owner);
}
static HRESULT WINAPI peer_framework_iids(peer_framework1 *This, ULONG *count, IID **iids)
{
	(void)This;
	return inspectable_iids(count, iids);
}
static HRESULT WINAPI peer_framework_name(peer_framework1 *This, HSTRING *name)
{
	(void)This;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(peer_class, (UINT32)lstrlenW(peer_class), name);
}
static HRESULT WINAPI peer_framework_trust(peer_framework1 *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}
/* The element the peer answers for, AddRef'd; the default-constructed peer reports no owner. */
static HRESULT WINAPI peer_get_owner(peer_framework1 *This, uint64_t out)
{
	struct peer *self = ((struct peer_framework_part *)This)->owner;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = self->owner;
	if (self->owner)
		self->owner->lpVtbl->AddRef(self->owner);
	return S_OK;
}

static const peer_framework1_vtbl peer_framework_table = {
	.QueryInterface = peer_framework_query,
	.AddRef = peer_framework_addref,
	.Release = peer_framework_release,
	.GetIids = peer_framework_iids,
	.GetRuntimeClassName = peer_framework_name,
	.GetTrustLevel = peer_framework_trust,
	.get_Owner = peer_get_owner,
};

static HRESULT peer_query(IInspectable *This, REFIID iid, void **out)
{
	struct peer *self = (struct peer *)This;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable))
		*out = self;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer))
		*out = &self->automation;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeer))
		*out = &self->framework;
	else
		return E_NOINTERFACE;
	peer_addref(self);
	return S_OK;
}

static ULONG peer_addref(struct peer *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG peer_release(struct peer *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		/* Unreachable while the registry holds its reference: peers live for the process. The
		 * release still drops the owner and the object count for the shape of the object. */
		if (self->owner)
			self->owner->lpVtbl->Release(self->owner);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

/* The peer registry */

/* One entry per element a peer was created for, keyed by the element pointer the engine handed out. Entries
 * are only ever added: the registry holds one reference to each peer for the process lifetime, so the
 * peer and its owner (and with the owner the key) stay valid for every later lookup. */
struct peer_entry {
	ui_element *element;
	struct peer *peer;
};

/* The registry lock: a critical section the first registrar initializes, gated by an inline
 * InterlockedCompareExchange so no init hook is needed. */
static CRITICAL_SECTION peer_registry_lock;
static LONG peer_registry_lock_state; /* 0 uninitialized, 1 initializing, 2 ready */

static void peer_registry_enter(void)
{
	if (InterlockedCompareExchange(&peer_registry_lock_state, 1, 0) == 0) {
		InitializeCriticalSection(&peer_registry_lock);
		InterlockedExchange(&peer_registry_lock_state, 2);
		return;
	}
	while (InterlockedCompareExchange(&peer_registry_lock_state, 2, 2) != 2)
		;
	EnterCriticalSection(&peer_registry_lock);
}

static void peer_registry_leave(void)
{
	LeaveCriticalSection(&peer_registry_lock);
}

static struct peer_entry *peer_registry;
static UINT32 peer_registry_count, peer_registry_capacity;

static struct peer *peer_registry_find(ui_element *element)
{
	struct peer *found = NULL;
	peer_registry_enter();
	for (UINT32 index = 0; index < peer_registry_count; ++index) {
		if (peer_registry[index].element == element) {
			found = peer_registry[index].peer;
			peer_addref(found);
			break;
		}
	}
	peer_registry_leave();
	return found;
}

/* Registers a freshly created peer; the registry's own reference is one of the two the creator holds. */
static BOOL peer_registry_add(ui_element *element, struct peer *peer)
{
	BOOL registered = FALSE;
	peer_registry_enter();
	if (peer_registry_count == peer_registry_capacity) {
		const UINT32 capacity = peer_registry_capacity ? peer_registry_capacity * 2 : 8;
		/* HeapReAlloc needs an existing block: the first grow allocates. */
		struct peer_entry *entries = peer_registry ? REALLOC(peer_registry, capacity * sizeof *entries)
			: ALLOC(capacity * sizeof *entries);
		if (entries) {
			peer_registry = entries;
			peer_registry_capacity = capacity;
		}
	}
	if (peer_registry_count < peer_registry_capacity) {
		peer_registry[peer_registry_count].element = element;
		peer_registry[peer_registry_count].peer = peer;
		++peer_registry_count;
		registered = TRUE;
	}
	peer_registry_leave();
	return registered;
}

/* Fills a fresh peer's shape: the three parts and the owner the caller AddRef'd. */
static void peer_init(struct peer *peer, ui_element *owner)
{
	peer->lpVtbl = &peer_inspectable_table;
	peer->automation.lpVtbl = &peer_automation_table;
	peer->automation.owner = peer;
	peer->framework.lpVtbl = &peer_framework_table;
	peer->framework.owner = peer;
	peer->owner = owner;
}

/* The one peer for an element: a lookup hit returns the existing peer, otherwise the peer is created,
 * bound to the element and registered. Both creation paths the winmd declares (the statics and the
 * composable factory) come through here, so the identity holds whoever created the peer. */
static HRESULT peer_for_element(ui_element *element, void **out)
{
	struct peer *peer = NULL;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!element)
		return E_POINTER;
	if ((peer = peer_registry_find(element))) {
			*out = &peer->framework;
		return S_OK;
	}
	peer = ALLOC(sizeof *peer);
	if (!peer)
		return E_OUTOFMEMORY;
	peer_init(peer, element);
	/* The creation holds one reference for the caller; the registry takes its own so the peer lives for the
	 * process lifetime the registry promises. */
	peer->refs = 1;
	element->lpVtbl->AddRef(element);
	if (!peer_registry_add(element, peer)) {
		element->lpVtbl->Release(element);
		FREE(peer);
		return E_OUTOFMEMORY;
	}
	peer_addref(peer);
	InterlockedIncrement(&raven_xaml_objects);
	*out = &peer->framework;
	return S_OK;
}

/* The factory */

/* The interfaces the winmd attaches to the class: the activation factory itself, the statics and the
 * composable factory. The parts answer as the one factory object the way the element parts do. */
struct peer_statics_part { const peer_statics1_vtbl *lpVtbl; };
struct peer_composable_part { const peer_composable1_vtbl *lpVtbl; };

struct automation_factory {
	const IActivationFactoryVtbl *lpVtbl;
	struct peer_statics_part statics;
	struct peer_composable_part composable;
	LONG refs;
};

/* The one factory object of the class; the part vtables below answer as it. */
static struct automation_factory automation_factory_instance;

static HRESULT factory_query(REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID_IActivationFactory))
		*out = &automation_factory_instance;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics))
		*out = &automation_factory_instance.statics;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerFactory))
		*out = &automation_factory_instance.composable;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&automation_factory_instance.refs);
	return S_OK;
}

static ULONG factory_addref(void)
{
	return (ULONG)InterlockedIncrement(&automation_factory_instance.refs);
}

static ULONG factory_release(void)
{
	/* The factory is a static like the element factories; the reference count only tracks the callers. */
	return (ULONG)InterlockedDecrement(&automation_factory_instance.refs);
}

static HRESULT factory_iids(ULONG *count, IID **iids)
{
	return inspectable_iids(count, iids);
}

static HRESULT factory_name(HSTRING *name)
{
	if (!name)
		return E_POINTER;
	return WindowsCreateString(peer_class, (UINT32)lstrlenW(peer_class), name);
}

static HRESULT factory_trust(TrustLevel *level)
{
	return inspectable_trust(level);
}

/* The default construction of the class: a peer with no owner. The engine's elements always reach their
 * peers through the statics, so the ownerless peer exists for the shape of the factory. */
static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out)
{
	struct peer *peer = NULL;
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	peer = ALLOC(sizeof *peer);
	if (!peer)
		return E_OUTOFMEMORY;
	peer_init(peer, NULL);
	peer->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	*out = (IInspectable *)peer;
	return S_OK;
}

static HRESULT WINAPI factory_activation_query(IActivationFactory *iface, REFIID iid, void **out)
{
	(void)iface;
	return factory_query(iid, out);
}
static ULONG WINAPI factory_activation_addref(IActivationFactory *iface)
{
	(void)iface;
	return factory_addref();
}
static ULONG WINAPI factory_activation_release(IActivationFactory *iface)
{
	(void)iface;
	return factory_release();
}
static HRESULT WINAPI factory_activation_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return factory_iids(count, iids);
}
static HRESULT WINAPI factory_activation_name(IActivationFactory *iface, HSTRING *name)
{
	(void)iface;
	return factory_name(name);
}
static HRESULT WINAPI factory_activation_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return factory_trust(level);
}

static const IActivationFactoryVtbl factory_activation_table = {
	.QueryInterface = factory_activation_query,
	.AddRef = factory_activation_addref,
	.Release = factory_activation_release,
	.GetIids = factory_activation_iids,
	.GetRuntimeClassName = factory_activation_name,
	.GetTrustLevel = factory_activation_trust,
	.ActivateInstance = factory_activate,
};

/* The statics interface the class carries: FromElement answers the peer an element already has (no peer,
 * no error), CreatePeerForElement creates and registers one. */
static HRESULT WINAPI factory_statics_query(peer_statics1 *This, REFIID iid, void **out)
{
	(void)This;
	return factory_query(iid, out);
}
static ULONG WINAPI factory_statics_addref(peer_statics1 *This)
{
	(void)This;
	return factory_addref();
}
static ULONG WINAPI factory_statics_release(peer_statics1 *This)
{
	(void)This;
	return factory_release();
}
static HRESULT WINAPI factory_statics_iids(peer_statics1 *This, ULONG *count, IID **iids)
{
	(void)This;
	return factory_iids(count, iids);
}
static HRESULT WINAPI factory_statics_name(peer_statics1 *This, HSTRING *name)
{
	(void)This;
	return factory_name(name);
}
static HRESULT WINAPI factory_statics_trust(peer_statics1 *This, TrustLevel *level)
{
	(void)This;
	return factory_trust(level);
}
static HRESULT WINAPI factory_from_element(peer_statics1 *This, uint64_t element, uint64_t out)
{
	struct peer *peer = NULL;
	(void)This;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	if (!element)
		return E_POINTER;
	if ((peer = peer_registry_find((ui_element *)(uintptr_t)element)))
		*(void **)(uintptr_t)out = &peer->framework;
	return S_OK;
}
static HRESULT WINAPI factory_create_peer_for_element(peer_statics1 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return peer_for_element((ui_element *)(uintptr_t)element, (void **)(uintptr_t)out);
}

static const peer_statics1_vtbl factory_statics_table = {
	.QueryInterface = factory_statics_query,
	.AddRef = factory_statics_addref,
	.Release = factory_statics_release,
	.GetIids = factory_statics_iids,
	.GetRuntimeClassName = factory_statics_name,
	.GetTrustLevel = factory_statics_trust,
	.FromElement = factory_from_element,
	.CreatePeerForElement = factory_create_peer_for_element,
};

/* The composable factory the class carries: CreateInstanceWithOwner is the creation path the framework
 * itself projects. The aggregation outer is rejected — nothing here consumes a composed peer — and the
 * no-outer call answers the one peer of the element like the statics do. */
static HRESULT WINAPI factory_composable_query(peer_composable1 *This, REFIID iid, void **out)
{
	(void)This;
	return factory_query(iid, out);
}
static ULONG WINAPI factory_composable_addref(peer_composable1 *This)
{
	(void)This;
	return factory_addref();
}
static ULONG WINAPI factory_composable_release(peer_composable1 *This)
{
	(void)This;
	return factory_release();
}
static HRESULT WINAPI factory_composable_iids(peer_composable1 *This, ULONG *count, IID **iids)
{
	(void)This;
	return factory_iids(count, iids);
}
static HRESULT WINAPI factory_composable_name(peer_composable1 *This, HSTRING *name)
{
	(void)This;
	return factory_name(name);
}
static HRESULT WINAPI factory_composable_trust(peer_composable1 *This, TrustLevel *level)
{
	(void)This;
	return factory_trust(level);
}
static HRESULT WINAPI factory_create_instance_with_owner(peer_composable1 *This, uint64_t element, uint64_t outer,
	uint64_t inner, uint64_t result)
{
	(void)This;
	if (outer)
		return E_NOTIMPL;
	if (inner)
		*(void **)(uintptr_t)inner = NULL;
	return peer_for_element((ui_element *)(uintptr_t)element, (void **)(uintptr_t)result);
}

static const peer_composable1_vtbl factory_composable_table = {
	.QueryInterface = factory_composable_query,
	.AddRef = factory_composable_addref,
	.Release = factory_composable_release,
	.GetIids = factory_composable_iids,
	.GetRuntimeClassName = factory_composable_name,
	.GetTrustLevel = factory_composable_trust,
	.CreateInstanceWithOwner = factory_create_instance_with_owner,
};

static struct automation_factory automation_factory_instance = {
	&factory_activation_table,
	{&factory_statics_table},
	{&factory_composable_table},
	1,
};

/* The activation factory entry the chain in xaml.c reaches: only the class the catalog registers. */
HRESULT raven_automation_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (!text || length != ARRAYSIZE(peer_key) - 1 ||
		CompareStringOrdinal(text, (int)length, peer_key, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	InterlockedIncrement(&automation_factory_instance.refs);
	*out = (IActivationFactory *)&automation_factory_instance;
	return S_OK;
}
