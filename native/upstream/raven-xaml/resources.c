/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.ApplicationModel.Resources.Core.ResourceContext class of the Windows Runtime.
 *
 * The application model asks for a resource context before it reads resources: the per-view and view-independent contexts are
 * returned as empty contexts with no qualifiers or languages yet. Resource maps and the resources.pri index are the next layer;
 * until it exists a context clones, resets and accepts qualifier overrides, and the collection properties answer E_NOTIMPL. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <windows.foundation.h>
#include <weakreference.h>
#include "windows_applicationmodel_resources.h"

typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext resource_context;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics static1;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 static2;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3 static3;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4 static4;

static const WCHAR class_name[] = L"Windows.ApplicationModel.Resources.Core.ResourceContext";

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

/* GetIids answers the interfaces the object really implements, never an empty list: .NET Native walks it to resolve the
 * parameterized interfaces of a projected object. */
static HRESULT static_iids(const IID *list, ULONG size, ULONG *count, IID **iids)
{
	IID *copy;
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	copy = (IID *)HeapAlloc(GetProcessHeap(), 0, size * sizeof *copy);
	if (!copy)
		return E_OUTOFMEMORY;
	memcpy(copy, list, size * sizeof *copy);
	*count = size;
	*iids = copy;
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

#define STUB1(iface, name) static HRESULT WINAPI name(iface *This, uint64_t a1) { (void)This; (void)a1;return E_NOTIMPL; }
#define STUB2(iface, name) \
	static HRESULT WINAPI name(iface *This, uint64_t a1, uint64_t a2) { (void)This; (void)a1; (void)a2;return E_NOTIMPL; }
#define STUB3(iface, name) \
	static HRESULT WINAPI name(iface *This, uint64_t a1, uint64_t a2, uint64_t a3) \
	{ (void)This; (void)a1; (void)a2; (void)a3;return E_NOTIMPL; }

/* Weak references: the helper lifted into xaml_internal.h, implemented here. The source part's IUnknown and lifetime are the
 * owner's; the reference it hands out resolves through the owner's own QueryInterface while the owner lives and answers a null
 * interface once it does not — the reference holds no reference on the owner, so it never keeps it alive and never dangles into
 * it after the owner's disconnect. */
struct raven_weak_target {
	LONG refs; /* one for the owner's embedded weak part, one per live weak reference */
	void *owner;
	LONG *owner_refs;
	raven_weak_query_fn owner_query;
};

static const struct raven_weak_ref_vtbl raven_weak_ref_table;

/* the standalone reference object .NET Native resolves; its state is fixed at creation and its refcount is its own */
struct raven_weak_ref_vtbl {
	HRESULT (WINAPI *QueryInterface)(IWeakReference *, REFIID, void **);
	ULONG (WINAPI *AddRef)(IWeakReference *);
	ULONG (WINAPI *Release)(IWeakReference *);
	HRESULT (WINAPI *GetIids)(IWeakReference *, ULONG *, IID **);
	HRESULT (WINAPI *GetRuntimeClassName)(IWeakReference *, HSTRING *);
	HRESULT (WINAPI *GetTrustLevel)(IWeakReference *, TrustLevel *);
	HRESULT (WINAPI *Resolve)(IWeakReference *, REFIID, IInspectable **);
};

struct raven_weak_ref {
	const struct raven_weak_ref_vtbl *lpVtbl;
	struct raven_weak_target *target;
	LONG refs;
};

void raven_weak_source_init(struct raven_weak_source *weak, void *owner, LONG *owner_refs, raven_weak_query_fn owner_query)
{
	struct raven_weak_target *target;
	weak->lpVtbl = &raven_weak_source_table;
	weak->target = NULL;
	target = (struct raven_weak_target *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *target);
	if (!target)
		return; /* a part without a target answers every weak ask with a refusal rather than a dangling resolution */
	target->refs = 1;
	target->owner = owner;
	target->owner_refs = owner_refs;
	target->owner_query = owner_query;
	weak->target = target;
}

void raven_weak_source_disconnect(struct raven_weak_source *weak)
{
	struct raven_weak_target *target = (struct raven_weak_target *)weak->target;
	if (!target)
		return;
	weak->target = NULL;
	/* the owner is freeing itself: a later resolution must answer null, never reach the owner's memory */
	target->owner = NULL;
	target->owner_refs = NULL;
	target->owner_query = NULL;
	if (!InterlockedDecrement(&target->refs))
		HeapFree(GetProcessHeap(), 0, target);
}

static HRESULT WINAPI weak_source_query(IWeakReferenceSource *iface, REFIID iid, void **out)
{
	struct raven_weak_source *self = (struct raven_weak_source *)iface;
	struct raven_weak_target *target = (struct raven_weak_target *)self->target;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!target || !target->owner)
		return E_NOINTERFACE; /* the part is only used while the owner lives, so this is unreachable in practice */
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID_IWeakReferenceSource)) {
		InterlockedIncrement(target->owner_refs);
		*out = iface;
		return S_OK;
	}
	if (equal_iid(iid, &IID_IAgileObject)) {
		/* the marker follows the owner: an agile owner answers through its own query, a thread-affine one refuses */
		return target->owner_query(target->owner, iid, out);
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI weak_source_addref(IWeakReferenceSource *iface)
{
	return (ULONG)InterlockedIncrement(((struct raven_weak_target *)((struct raven_weak_source *)iface)->target)->owner_refs);
}

static ULONG WINAPI weak_source_release(IWeakReferenceSource *iface)
{
	return (ULONG)InterlockedDecrement(((struct raven_weak_target *)((struct raven_weak_source *)iface)->target)->owner_refs);
}

static HRESULT WINAPI weak_source_iids(IWeakReferenceSource *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI weak_source_name(IWeakReferenceSource *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	/* The source is a part of the owner; the owner's own class name answers GetRuntimeClassName through its QueryInterface. */
	return E_NOTIMPL; /* not observed: callers read the class name from the object, not from this part */
}

static HRESULT WINAPI weak_source_trust(IWeakReferenceSource *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI weak_source_get_weak_reference(IWeakReferenceSource *iface, IWeakReference **out)
{
	struct raven_weak_source *self = (struct raven_weak_source *)iface;
	struct raven_weak_target *target = (struct raven_weak_target *)self->target;
	struct raven_weak_ref *reference;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!target || !target->owner)
		return E_NOTIMPL; /* the owner disconnected: nothing left to reference weakly */
	reference = (struct raven_weak_ref *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *reference);
	if (!reference)
		return E_OUTOFMEMORY;
	reference->lpVtbl = &raven_weak_ref_table;
	reference->target = target;
	reference->refs = 1;
	InterlockedIncrement(&target->refs);
	InterlockedIncrement(&raven_xaml_objects);
	*out = (IWeakReference *)reference;
	return S_OK;
}

const struct raven_weak_source_vtbl raven_weak_source_table = {
	weak_source_query, weak_source_addref, weak_source_release,
	weak_source_iids, weak_source_name, weak_source_trust,
	weak_source_get_weak_reference,
};

static HRESULT WINAPI weak_ref_query(IWeakReference *iface, REFIID iid, void **out)
{
	struct raven_weak_ref *self = (struct raven_weak_ref *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	/* the reference object itself is agile: its target block is fixed at creation, its refcount is interlocked, and
	 * Resolve only reads the disconnected-checked target — the owner's own agility is a separate question its
	 * QueryInterface answers */
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID_IWeakReference) && !equal_iid(iid, &IID_IAgileObject))
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI weak_ref_addref(IWeakReference *iface)
{
	return (ULONG)InterlockedIncrement(&((struct raven_weak_ref *)iface)->refs);
}

static ULONG WINAPI weak_ref_release(IWeakReference *iface)
{
	struct raven_weak_ref *self = (struct raven_weak_ref *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		struct raven_weak_target *target = (struct raven_weak_target *)self->target;
		if (target && !InterlockedDecrement(&target->refs))
			HeapFree(GetProcessHeap(), 0, target);
		HeapFree(GetProcessHeap(), 0, self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI weak_ref_iids(IWeakReference *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI weak_ref_name(IWeakReference *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return E_NOTIMPL; /* not observed: see weak_source_name */
}

static HRESULT WINAPI weak_ref_trust(IWeakReference *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI weak_ref_resolve(IWeakReference *iface, REFIID iid, IInspectable **out)
{
	struct raven_weak_ref *self = (struct raven_weak_ref *)iface;
	struct raven_weak_target *target = (struct raven_weak_target *)self->target;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!target || !target->owner || !target->owner_refs || *target->owner_refs <= 0)
		return S_OK; /* the owner is gone: a weak resolution answers null, not an error */
	return target->owner_query(target->owner, iid, (void **)out);
}

static const struct raven_weak_ref_vtbl raven_weak_ref_table = {
	weak_ref_query, weak_ref_addref, weak_ref_release,
	weak_ref_iids, weak_ref_name, weak_ref_trust,
	weak_ref_resolve,
};

struct factory;
struct context;

struct part_activation { const IActivationFactoryVtbl *lpVtbl; struct factory *owner; };
struct part1 { const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStaticsVtbl *lpVtbl; struct factory *owner; };
struct part2 { const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2Vtbl *lpVtbl; struct factory *owner; };
struct part3 { const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3Vtbl *lpVtbl; struct factory *owner; };
struct part4 { const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4Vtbl *lpVtbl; struct factory *owner; };

struct factory {
	struct part_activation activation;
	struct part1 statics1;
	struct part2 statics2;
	struct part3 statics3;
	struct part4 statics4;
	LONG refs;
};

struct context {
	const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextVtbl *lpVtbl;
	LONG refs;
	struct raven_weak_source weak;
};

static HRESULT factory_query(struct factory *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory))
		*out = &self->activation;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics))
		*out = &self->statics1;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2))
		*out = &self->statics2;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3))
		*out = &self->statics3;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4))
		*out = &self->statics4;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

#define PART_BOILERPLATE(part, iface) \
	static HRESULT WINAPI part##_query(iface *iface##_unused, REFIID iid, void **out) \
	{ return factory_query(((struct part *)iface##_unused)->owner, iid, out); } \
	static ULONG WINAPI part##_addref(iface *iface##_unused) \
	{ return (ULONG)InterlockedIncrement(&((struct part *)iface##_unused)->owner->refs); } \
	static ULONG WINAPI part##_release(iface *iface##_unused) \
	{ return (ULONG)InterlockedDecrement(&((struct part *)iface##_unused)->owner->refs); } \
	static HRESULT WINAPI part##_iids(iface *iface##_unused, ULONG *count, IID **iids) \
	{ (void)iface##_unused; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI part##_name(iface *iface##_unused, HSTRING *name) \
	{ (void)iface##_unused; return runtime_class_name(name); } \
	static HRESULT WINAPI part##_trust(iface *iface##_unused, TrustLevel *level) \
	{ (void)iface##_unused; return inspectable_trust(level); }

PART_BOILERPLATE(part1, static1)
PART_BOILERPLATE(part2, static2)
PART_BOILERPLATE(part3, static3)
PART_BOILERPLATE(part4, static4)

static HRESULT WINAPI activation_query(IActivationFactory *iface, REFIID iid, void **out)
{
	return factory_query(((struct part_activation *)iface)->owner, iid, out);
}

static ULONG WINAPI activation_addref(IActivationFactory *iface)
{
	return (ULONG)InterlockedIncrement(&((struct part_activation *)iface)->owner->refs);
}

static ULONG WINAPI activation_release(IActivationFactory *iface)
{
	return (ULONG)InterlockedDecrement(&((struct part_activation *)iface)->owner->refs);
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

/* A context is asked for, never constructed by name. */
static HRESULT WINAPI activation_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return 0x8000000eU;
}

static HRESULT WINAPI context_query(resource_context *iface, REFIID iid, void **out);
static ULONG WINAPI context_addref(resource_context *iface);
static ULONG WINAPI context_release(resource_context *iface);
static HRESULT WINAPI context_iids(resource_context *iface, ULONG *count, IID **iids);
static HRESULT WINAPI context_name(resource_context *iface, HSTRING *name);
static HRESULT WINAPI context_trust(resource_context *iface, TrustLevel *level);
static HRESULT WINAPI context_qualifiers(resource_context *iface, uint64_t out);
static HRESULT WINAPI context_reset(resource_context *iface);
static HRESULT WINAPI context_reset_list(resource_context *iface, uint64_t list);
static HRESULT WINAPI context_override(resource_context *iface, uint64_t list);
static HRESULT WINAPI context_clone(resource_context *iface, uint64_t out);
static HRESULT WINAPI context_languages(resource_context *iface, uint64_t out);
static HRESULT WINAPI context_put_languages(resource_context *iface, uint64_t list);

static const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextVtbl context_table = {
	.QueryInterface = context_query,
	.AddRef = context_addref,
	.Release = context_release,
	.GetIids = context_iids,
	.GetRuntimeClassName = context_name,
	.GetTrustLevel = context_trust,
	.get_QualifierValues = context_qualifiers,
	.Reset = context_reset,
	.Reset2 = context_reset_list,
	.OverrideToMatch = context_override,
	.Clone = context_clone,
	.get_Languages = context_languages,
	.put_Languages = context_put_languages,
};

/* ResourceContext.Languages: the UI language list .NET Native reads after it subscribes to qualifier changes; a bare E_NOTIMPL
 * ends the application's resource initialization. The list is the host's default UI language, read-only: Windows answers put with
 * S_OK and no carry-over because the context is not yet configurable. */
DEFINE_GUID(IID___FIVectorView_1_HSTRING_local, 0x2f13c006, 0xa03a, 0x5f69, 0xb0, 0x90, 0x75, 0xa4, 0x3e, 0x33, 0x42, 0x3e);
#define lang_list_iid IID___FIVectorView_1_HSTRING

struct language_list {
	const __FIVectorView_1_HSTRINGVtbl *lpVtbl;
	LONG refs;
	HSTRING language;
	struct raven_weak_source weak;
};

static const WCHAR *language_list_value(void)
{
	static WCHAR name[LOCALE_NAME_MAX_LENGTH];
	if (!name[0]) {
		int length = LCIDToLocaleName((LCID)(uintptr_t)GetUserDefaultUILanguage(), name, LOCALE_NAME_MAX_LENGTH, 0);
		if (!length)
			lstrcpynW(name, L"en-US", LOCALE_NAME_MAX_LENGTH);
	}
	return name;
}

static HRESULT WINAPI lang_query(__FIVectorView_1_HSTRING *iface, REFIID iid, void **out)
{
	struct language_list *self = (struct language_list *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IWeakReferenceSource))
		*out = &self->weak;
	else if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___FIVectorView_1_HSTRING))
		*out = iface;
	else
		/* IAgileObject stays refused: the lazily built locale name behind the list is written without synchronization,
		 * so the list is not free-threaded and the marker would claim a thread safety it does not have */
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI lang_addref(__FIVectorView_1_HSTRING *iface)
{
	return (ULONG)InterlockedIncrement(&((struct language_list *)iface)->refs);
}

static ULONG WINAPI lang_release(__FIVectorView_1_HSTRING *iface)
{
	struct language_list *self = (struct language_list *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		raven_weak_source_disconnect(&self->weak);
		if (self->language)
			WindowsDeleteString(self->language);
		HeapFree(GetProcessHeap(), 0, self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI lang_iids(__FIVectorView_1_HSTRING *iface, ULONG *count, IID **iids)
{
	static const IID list[] = { IID___FIVectorView_1_HSTRING };
	(void)iface;
	return static_iids(list, (ULONG)ARRAYSIZE(list), count, iids);
}

static HRESULT WINAPI lang_name(__FIVectorView_1_HSTRING *iface, HSTRING *name)
{
	static const WCHAR list_name_text[] = L"Windows.Foundation.Collections.IVectorView`1<String>";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(list_name_text, (UINT32)lstrlenW(list_name_text), name);
}

static HRESULT WINAPI lang_trust(__FIVectorView_1_HSTRING *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT language_read(HSTRING *out)
{
	const WCHAR *text = language_list_value();
	if (!out)
		return E_POINTER;
	*out = NULL;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), out);
}

static HRESULT WINAPI lang_get_at(__FIVectorView_1_HSTRING *iface, UINT32 index, HSTRING *out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	if (index)
		return 0x8000000bU; /* E_BOUNDS: the list holds exactly the default UI language */
	return language_read(out);
}

static HRESULT WINAPI lang_size(__FIVectorView_1_HSTRING *iface, UINT32 *size)
{
	(void)iface;
	if (!size)
		return E_POINTER;
	*size = 1;
	return S_OK;
}

static HRESULT WINAPI lang_index_of(__FIVectorView_1_HSTRING *iface, HSTRING value, UINT32 *index, boolean *found)
{
	UINT32 other_length = 0;
	const WCHAR *text = value ? WindowsGetStringRawBuffer(value, &other_length) : NULL;
	(void)iface;
	if (!index || !found)
		return E_POINTER;
	*found = FALSE;
	*index = 0;
	if (text) {
		const WCHAR *own = language_list_value();
		if (CompareStringOrdinal(text, (int)other_length, own, -1, FALSE) == CSTR_EQUAL)
			*found = TRUE;
	}
	return S_OK;
}

static HRESULT WINAPI lang_get_many(__FIVectorView_1_HSTRING *iface, UINT32 start, UINT32 items_size, HSTRING *items,
	UINT32 *count)
{
	(void)iface;
	if (!items || !count)
		return E_POINTER;
	*count = 0;
	if (start)
		return 0x8000000bU; /* E_BOUNDS */
	if (!items_size)
		return S_OK;
	if (language_read(items))
		return S_OK;
	*count = 1;
	return S_OK;
}

static const __FIVectorView_1_HSTRINGVtbl language_table = {
	.QueryInterface = lang_query, .AddRef = lang_addref, .Release = lang_release,
	.GetIids = lang_iids, .GetRuntimeClassName = lang_name, .GetTrustLevel = lang_trust,
	.GetAt = lang_get_at, .get_Size = lang_size, .IndexOf = lang_index_of, .GetMany = lang_get_many,
};

static struct language_list *create_language_list(void)
{
	struct language_list *list = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *list);
	if (!list)
		return NULL;
	list->lpVtbl = &language_table;
	list->refs = 1;
	raven_weak_source_init(&list->weak, list, &list->refs, (raven_weak_query_fn)lang_query);
	InterlockedIncrement(&raven_xaml_objects);
	return list;
}

static struct context *create_context(void)
{
	struct context *context = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *context);
	if (!context)
		return NULL;
	context->lpVtbl = &context_table;
	context->refs = 1;
	raven_weak_source_init(&context->weak, context, &context->refs, (raven_weak_query_fn)context_query);
	InterlockedIncrement(&raven_xaml_objects);
	return context;
}

static HRESULT WINAPI context_query(resource_context *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IWeakReferenceSource)) {
		struct context *self = (struct context *)iface;
		InterlockedIncrement(&self->refs);
		*out = &self->weak;
		return S_OK;
	}
	/* Agile: the context holds no mutable state — every member is a no-op or allocates a fresh object, the qualifier
	 * values are a static read-only table and the reference count is interlocked — so any thread may call it and no
	 * cross-apartment marshaling is ever needed for it. IMarshal stays unimplemented (see xaml_internal.h). */
	if (equal_iid(iid, &IID_IAgileObject) ||
		equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext)) {
		InterlockedIncrement(&((struct context *)iface)->refs);
		*out = iface;
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI context_addref(resource_context *iface)
{
	return (ULONG)InterlockedIncrement(&((struct context *)iface)->refs);
}

static ULONG WINAPI context_release(resource_context *iface)
{
	struct context *self = (struct context *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		raven_weak_source_disconnect(&self->weak);
		HeapFree(GetProcessHeap(), 0, self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI context_iids(resource_context *iface, ULONG *count, IID **iids)
{
	static const IID list[] = {
		IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext,
	};
	(void)iface;
	return static_iids(list, (ULONG)ARRAYSIZE(list), count, iids);
}

static HRESULT WINAPI context_name(resource_context *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name);
}

static HRESULT WINAPI context_trust(resource_context *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

/* The qualifier and language collections need their generic interfaces; no observed startup path reads them yet. */
/* The qualifier map XAML reads before it resolves resources: the host's language, scale and contrast, which are the values a
 * resource context starts with. The map is read-only and constant: insertion, removal and clearing answer E_NOTIMPL, and because
 * no entry ever changes, MapChanged handlers are accepted and stored but never raised. .NET Native subscribes through
 * IObservableMap<HSTRING,HSTRING> right after it acquires QualifierValues, so the interface must answer or the runtime's
 * interface dispatch misfires and it fail-fasts. The instantiation IID below is the RFC 4122 version-5 UUID (SHA-1) of the
 * instance signature "pinterface({65df2bf5-bf39-41b5-aebc-5a9d865e472b};string;string)" under the WinRT namespace
 * 11f47ad5-7b73-42c0-abae-878b1e16adee; the same computation reproduces the precomputed GUIDs of mingw's windows.foundation.h
 * for IMap`2, IMapView`2, IObservableMap`2<HSTRING,IInspectable> and IIterable`1<HSTRING>. */
DEFINE_GUID(IID___FIObservableMap_2_HSTRING_HSTRING, 0x1e036276, 0x2f60, 0x55f6, 0xb7, 0xf3, 0xf8, 0x60, 0x79, 0xe6, 0x90, 0x0b);

typedef struct __FIObservableMap_2_HSTRING_HSTRING __FIObservableMap_2_HSTRING_HSTRING;
typedef struct __FIMapChangedEventHandler_2_HSTRING_HSTRING __FIMapChangedEventHandler_2_HSTRING_HSTRING;

/* IMapChangedEventHandler<HSTRING,HSTRING>: the delegate a subscriber passes. The map never changes, so Invoke is declared for
 * layout only and is never called. */
typedef struct __FIMapChangedEventHandler_2_HSTRING_HSTRINGVtbl {
	HRESULT (WINAPI *QueryInterface)(__FIMapChangedEventHandler_2_HSTRING_HSTRING *, REFIID, void **);
	ULONG (WINAPI *AddRef)(__FIMapChangedEventHandler_2_HSTRING_HSTRING *);
	ULONG (WINAPI *Release)(__FIMapChangedEventHandler_2_HSTRING_HSTRING *);
	HRESULT (WINAPI *Invoke)(__FIMapChangedEventHandler_2_HSTRING_HSTRING *,
		__FIObservableMap_2_HSTRING_HSTRING *, __FIMapChangedEventArgs_1_HSTRING *);
} __FIMapChangedEventHandler_2_HSTRING_HSTRINGVtbl;

struct __FIMapChangedEventHandler_2_HSTRING_HSTRING {
	CONST_VTBL __FIMapChangedEventHandler_2_HSTRING_HSTRINGVtbl *lpVtbl;
};

/* The IObservableMap<HSTRING,HSTRING> interface carries only its two event methods over IInspectable; the map methods are
 * reached through QueryInterface for IMap`2, exactly as the Windows runtime class does. */
typedef struct __FIObservableMap_2_HSTRING_HSTRINGVtbl {
	HRESULT (WINAPI *QueryInterface)(__FIObservableMap_2_HSTRING_HSTRING *, REFIID, void **);
	ULONG (WINAPI *AddRef)(__FIObservableMap_2_HSTRING_HSTRING *);
	ULONG (WINAPI *Release)(__FIObservableMap_2_HSTRING_HSTRING *);
	HRESULT (WINAPI *GetIids)(__FIObservableMap_2_HSTRING_HSTRING *, ULONG *, IID **);
	HRESULT (WINAPI *GetRuntimeClassName)(__FIObservableMap_2_HSTRING_HSTRING *, HSTRING *);
	HRESULT (WINAPI *GetTrustLevel)(__FIObservableMap_2_HSTRING_HSTRING *, TrustLevel *);
	HRESULT (WINAPI *add_MapChanged)(__FIObservableMap_2_HSTRING_HSTRING *, __FIMapChangedEventHandler_2_HSTRING_HSTRING *,
		EventRegistrationToken *);
	HRESULT (WINAPI *remove_MapChanged)(__FIObservableMap_2_HSTRING_HSTRING *, EventRegistrationToken);
} __FIObservableMap_2_HSTRING_HSTRINGVtbl;

struct qualifiers;

struct qualifier_view {
	const __FIMapView_2_HSTRING_HSTRINGVtbl *lpVtbl;
	struct qualifiers *owner;
};

struct qualifier_observable {
	const __FIObservableMap_2_HSTRING_HSTRINGVtbl *lpVtbl;
	struct qualifiers *owner;
};

struct qualifier_subscription {
	LONGLONG token;
	__FIMapChangedEventHandler_2_HSTRING_HSTRING *handler;
};

struct qualifiers {
	const __FIMap_2_HSTRING_HSTRINGVtbl *lpVtbl;
	LONG refs;
	struct qualifier_view view;
	struct qualifier_observable observable;
	struct raven_weak_source weak;
	/* the subscription list is the map's only mutable state; the critical section is what lets the map answer the
	 * IAgileObject marker honestly */
	CRITICAL_SECTION lock;
	struct qualifier_subscription *subscriptions;
	size_t subscription_count, subscription_capacity;
	LONGLONG next_token;
};

typedef struct qualifier_value { const WCHAR *name, *value; } qualifier_value;

static const qualifier_value qualifier_values[] = {
	{L"language", L"en-US"},
	{L"scale", L"1.0"},
	{L"contrast", L"standard"},
};

static const WCHAR *match_qualifier(const WCHAR *text, UINT32 length)
{
	if (!text)
		return NULL;
	for (size_t index = 0; index < ARRAYSIZE(qualifier_values); ++index)
		if (CompareStringOrdinal(text, (int)length, qualifier_values[index].name, -1, FALSE) == CSTR_EQUAL)
			return qualifier_values[index].value;
	return NULL;
}

static const WCHAR *qualifier_lookup(HSTRING key, UINT32 *length)
{
	const WCHAR *text;
	if (!key) {
		*length = 0;
		return NULL;
	}
	/* the buffer and its length must be read before the length is passed on: the argument expression *length would be
	 * evaluated before the call writes through it */
	text = WindowsGetStringRawBuffer(key, length);
	return match_qualifier(text, *length);
}

static HRESULT qualifier_read(HSTRING key, HSTRING *out)
{
	UINT32 length = 0;
	const WCHAR *value = key ? qualifier_lookup(key, &length) : NULL;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!value)
		return 0x8000000bU; /* E_BOUNDS */
	return WindowsCreateString(value, (UINT32)lstrlenW(value), out);
}

static uint32_t qualifier_count(void) { return (uint32_t)ARRAYSIZE(qualifier_values); }

static ULONG WINAPI qualifier_addref(struct qualifiers *owner);

static HRESULT WINAPI map_lookup(__FIMap_2_HSTRING_HSTRING *iface, HSTRING key, HSTRING *value)
{
	(void)iface;
	return qualifier_read(key, value);
}

static HRESULT WINAPI map_size(__FIMap_2_HSTRING_HSTRING *iface, UINT32 *size)
{
	(void)iface;
	if (!size)
		return E_POINTER;
	*size = qualifier_count();
	return S_OK;
}

static HRESULT WINAPI map_has_key(__FIMap_2_HSTRING_HSTRING *iface, HSTRING key, boolean *found)
{
	UINT32 length = 0;
	(void)iface;
	if (!found)
		return E_POINTER;
	*found = (boolean)(key && qualifier_lookup(key, &length));
	return S_OK;
}

static HRESULT WINAPI map_get_view(__FIMap_2_HSTRING_HSTRING *iface, __FIMapView_2_HSTRING_HSTRING **view)
{
	struct qualifiers *self = (struct qualifiers *)iface;
	if (!view)
		return E_POINTER;
	qualifier_addref(self);
	*view = (__FIMapView_2_HSTRING_HSTRING *)&self->view;
	return S_OK;
}

static HRESULT WINAPI map_insert(__FIMap_2_HSTRING_HSTRING *iface, HSTRING key, HSTRING value, boolean *replaced)
{
	(void)iface; (void)key; (void)value; (void)replaced;
	return E_NOTIMPL;
}

static HRESULT WINAPI map_remove(__FIMap_2_HSTRING_HSTRING *iface, HSTRING key)
{
	(void)iface; (void)key;
	return E_NOTIMPL;
}

static HRESULT WINAPI map_clear(__FIMap_2_HSTRING_HSTRING *iface)
{
	(void)iface;
	return E_NOTIMPL;
}

static HRESULT WINAPI map_query(__FIMap_2_HSTRING_HSTRING *iface, REFIID iid, void **out);
static ULONG WINAPI map_addref(__FIMap_2_HSTRING_HSTRING *iface);
static ULONG WINAPI map_release(__FIMap_2_HSTRING_HSTRING *iface);
static HRESULT WINAPI map_iids(__FIMap_2_HSTRING_HSTRING *iface, ULONG *count, IID **iids);
static HRESULT WINAPI map_name(__FIMap_2_HSTRING_HSTRING *iface, HSTRING *name);
static HRESULT WINAPI map_trust(__FIMap_2_HSTRING_HSTRING *iface, TrustLevel *level);

static HRESULT WINAPI view_query(__FIMapView_2_HSTRING_HSTRING *iface, REFIID iid, void **out);
static ULONG WINAPI view_addref(__FIMapView_2_HSTRING_HSTRING *iface);
static ULONG WINAPI view_release(__FIMapView_2_HSTRING_HSTRING *iface);
static HRESULT WINAPI view_iids(__FIMapView_2_HSTRING_HSTRING *iface, ULONG *count, IID **iids);
static HRESULT WINAPI view_name(__FIMapView_2_HSTRING_HSTRING *iface, HSTRING *name);
static HRESULT WINAPI view_trust(__FIMapView_2_HSTRING_HSTRING *iface, TrustLevel *level);
static HRESULT WINAPI view_lookup(__FIMapView_2_HSTRING_HSTRING *iface, HSTRING key, HSTRING *value);
static HRESULT WINAPI view_size(__FIMapView_2_HSTRING_HSTRING *iface, UINT32 *size);
static HRESULT WINAPI view_has_key(__FIMapView_2_HSTRING_HSTRING *iface, HSTRING key, boolean *found);
static HRESULT WINAPI view_split(__FIMapView_2_HSTRING_HSTRING *iface, __FIMapView_2_HSTRING_HSTRING **first,
	__FIMapView_2_HSTRING_HSTRING **second);

static HRESULT WINAPI observable_query(__FIObservableMap_2_HSTRING_HSTRING *iface, REFIID iid, void **out);
static ULONG WINAPI observable_addref(__FIObservableMap_2_HSTRING_HSTRING *iface);
static ULONG WINAPI observable_release(__FIObservableMap_2_HSTRING_HSTRING *iface);
static HRESULT WINAPI observable_iids(__FIObservableMap_2_HSTRING_HSTRING *iface, ULONG *count, IID **iids);
static HRESULT WINAPI observable_name(__FIObservableMap_2_HSTRING_HSTRING *iface, HSTRING *name);
static HRESULT WINAPI observable_trust(__FIObservableMap_2_HSTRING_HSTRING *iface, TrustLevel *level);
static HRESULT WINAPI observable_add_map_changed(__FIObservableMap_2_HSTRING_HSTRING *iface,
	__FIMapChangedEventHandler_2_HSTRING_HSTRING *handler, EventRegistrationToken *token);
static HRESULT WINAPI observable_remove_map_changed(__FIObservableMap_2_HSTRING_HSTRING *iface,
	EventRegistrationToken token);

static const __FIMap_2_HSTRING_HSTRINGVtbl map_table = {
	.QueryInterface = map_query, .AddRef = map_addref, .Release = map_release,
	.GetIids = map_iids, .GetRuntimeClassName = map_name, .GetTrustLevel = map_trust,
	.Lookup = map_lookup, .get_Size = map_size, .HasKey = map_has_key, .GetView = map_get_view,
	.Insert = map_insert, .Remove = map_remove, .Clear = map_clear,
};

static const __FIMapView_2_HSTRING_HSTRINGVtbl view_table = {
	.QueryInterface = view_query, .AddRef = view_addref, .Release = view_release,
	.GetIids = view_iids, .GetRuntimeClassName = view_name, .GetTrustLevel = view_trust,
	.Lookup = view_lookup, .get_Size = view_size, .HasKey = view_has_key, .Split = view_split,
};

static const __FIObservableMap_2_HSTRING_HSTRINGVtbl observable_table = {
	.QueryInterface = observable_query, .AddRef = observable_addref, .Release = observable_release,
	.GetIids = observable_iids, .GetRuntimeClassName = observable_name, .GetTrustLevel = observable_trust,
	.add_MapChanged = observable_add_map_changed, .remove_MapChanged = observable_remove_map_changed,
};

static HRESULT WINAPI map_query(__FIMap_2_HSTRING_HSTRING *iface, REFIID iid, void **out)
{
	struct qualifiers *self = (struct qualifiers *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IWeakReferenceSource)) {
		*out = &self->weak;
	} else if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___FIMap_2_HSTRING_HSTRING))
		*out = iface;
	else if (equal_iid(iid, &IID___FIMapView_2_HSTRING_HSTRING))
		*out = &self->view;
	else if (equal_iid(iid, &IID___FIObservableMap_2_HSTRING_HSTRING))
		*out = &self->observable;
	else if (equal_iid(iid, &IID_IAgileObject))
		/* Agile: the map is read-only over a static qualifier table, its reference count is interlocked and its one
		 * mutable structure — the MapChanged subscription list — is guarded by the map's critical section, so every
		 * member is callable from any apartment and no cross-apartment marshaling is ever needed. IMarshal stays
		 * unimplemented (see xaml_internal.h). */
		*out = iface;
	else
		return E_NOINTERFACE;
	map_addref(iface);
	return S_OK;
}

static ULONG WINAPI map_addref(__FIMap_2_HSTRING_HSTRING *iface)
{
	return (ULONG)InterlockedIncrement(&((struct qualifiers *)iface)->refs);
}

static ULONG WINAPI map_release(__FIMap_2_HSTRING_HSTRING *iface)
{
	struct qualifiers *self = (struct qualifiers *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		raven_weak_source_disconnect(&self->weak);
		EnterCriticalSection(&self->lock);
		for (size_t index = 0; index < self->subscription_count; ++index) {
			__FIMapChangedEventHandler_2_HSTRING_HSTRING *handler = self->subscriptions[index].handler;
			if (handler)
				handler->lpVtbl->Release(handler);
		}
		if (self->subscriptions)
			HeapFree(GetProcessHeap(), 0, self->subscriptions);
		LeaveCriticalSection(&self->lock);
		DeleteCriticalSection(&self->lock);
		HeapFree(GetProcessHeap(), 0, self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI map_iids(__FIMap_2_HSTRING_HSTRING *iface, ULONG *count, IID **iids)
{
	static const IID list[] = {
		IID___FIMap_2_HSTRING_HSTRING,
		IID___FIMapView_2_HSTRING_HSTRING,
		IID___FIObservableMap_2_HSTRING_HSTRING,
	};
	(void)iface;
	return static_iids(list, (ULONG)ARRAYSIZE(list), count, iids);
}

static HRESULT WINAPI map_name(__FIMap_2_HSTRING_HSTRING *iface, HSTRING *name)
{
	static const WCHAR map_name_text[] = L"Windows.Foundation.Collections.IMap`2<String,String>";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(map_name_text, (UINT32)lstrlenW(map_name_text), name);
}

static HRESULT WINAPI map_trust(__FIMap_2_HSTRING_HSTRING *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static ULONG WINAPI qualifier_addref(struct qualifiers *owner)
{
	return (ULONG)InterlockedIncrement(&owner->refs);
}

static ULONG WINAPI view_addref(__FIMapView_2_HSTRING_HSTRING *iface)
{
	return qualifier_addref(((struct qualifier_view *)iface)->owner);
}

static ULONG WINAPI view_release(__FIMapView_2_HSTRING_HSTRING *iface)
{
	struct qualifiers *owner = ((struct qualifier_view *)iface)->owner;
	return map_release((__FIMap_2_HSTRING_HSTRING *)owner);
}

static HRESULT WINAPI view_iids(__FIMapView_2_HSTRING_HSTRING *iface, ULONG *count, IID **iids)
{
	static const IID list[] = {
		IID___FIMapView_2_HSTRING_HSTRING,
		IID___FIMap_2_HSTRING_HSTRING,
		IID___FIObservableMap_2_HSTRING_HSTRING,
	};
	(void)iface;
	return static_iids(list, (ULONG)ARRAYSIZE(list), count, iids);
}

static HRESULT WINAPI view_name(__FIMapView_2_HSTRING_HSTRING *iface, HSTRING *name)
{
	static const WCHAR view_name_text[] = L"Windows.Foundation.Collections.IMapView`2<String,String>";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(view_name_text, (UINT32)lstrlenW(view_name_text), name);
}

static HRESULT WINAPI view_trust(__FIMapView_2_HSTRING_HSTRING *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI view_lookup(__FIMapView_2_HSTRING_HSTRING *iface, HSTRING key, HSTRING *value)
{
	(void)iface;
	return qualifier_read(key, value);
}

static HRESULT WINAPI view_size(__FIMapView_2_HSTRING_HSTRING *iface, UINT32 *size)
{
	(void)iface;
	if (!size)
		return E_POINTER;
	*size = qualifier_count();
	return S_OK;
}

static HRESULT WINAPI view_has_key(__FIMapView_2_HSTRING_HSTRING *iface, HSTRING key, boolean *found)
{
	UINT32 length = 0;
	(void)iface;
	if (!found)
		return E_POINTER;
	*found = (boolean)(key && qualifier_lookup(key, &length));
	return S_OK;
}

static HRESULT WINAPI view_split(__FIMapView_2_HSTRING_HSTRING *iface, __FIMapView_2_HSTRING_HSTRING **first,
	__FIMapView_2_HSTRING_HSTRING **second)
{
	struct qualifiers *owner = ((struct qualifier_view *)iface)->owner;
	if (!first || !second)
		return E_POINTER;
	qualifier_addref(owner);
	qualifier_addref(owner);
	*first = (__FIMapView_2_HSTRING_HSTRING *)&owner->view;
	*second = (__FIMapView_2_HSTRING_HSTRING *)&owner->view;
	return S_OK;
}

static HRESULT WINAPI view_query(__FIMapView_2_HSTRING_HSTRING *iface, REFIID iid, void **out)
{
	struct qualifiers *self = ((struct qualifier_view *)iface)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IWeakReferenceSource))
		*out = &self->weak;
	else if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___FIMapView_2_HSTRING_HSTRING))
		*out = iface;
	else if (equal_iid(iid, &IID___FIMap_2_HSTRING_HSTRING))
		*out = self;
	else if (equal_iid(iid, &IID___FIObservableMap_2_HSTRING_HSTRING))
		*out = &self->observable;
	else if (equal_iid(iid, &IID_IAgileObject))
		/* agile through the owner map: see map_query for the thread-safety justification */
		*out = iface;
	else
		return E_NOINTERFACE;
	qualifier_addref(self);
	return S_OK;
}

static ULONG WINAPI observable_addref(__FIObservableMap_2_HSTRING_HSTRING *iface)
{
	return qualifier_addref(((struct qualifier_observable *)iface)->owner);
}

static ULONG WINAPI observable_release(__FIObservableMap_2_HSTRING_HSTRING *iface)
{
	return map_release((__FIMap_2_HSTRING_HSTRING *)((struct qualifier_observable *)iface)->owner);
}

static HRESULT WINAPI observable_iids(__FIObservableMap_2_HSTRING_HSTRING *iface, ULONG *count, IID **iids)
{
	static const IID list[] = {
		IID___FIObservableMap_2_HSTRING_HSTRING,
		IID___FIMap_2_HSTRING_HSTRING,
		IID___FIMapView_2_HSTRING_HSTRING,
	};
	(void)iface;
	return static_iids(list, (ULONG)ARRAYSIZE(list), count, iids);
}

static HRESULT WINAPI observable_name(__FIObservableMap_2_HSTRING_HSTRING *iface, HSTRING *name)
{
	static const WCHAR observable_name_text[] = L"Windows.Foundation.Collections.IObservableMap`2<String,String>";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(observable_name_text, (UINT32)lstrlenW(observable_name_text), name);
}

static HRESULT WINAPI observable_trust(__FIObservableMap_2_HSTRING_HSTRING *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI observable_add_map_changed(__FIObservableMap_2_HSTRING_HSTRING *iface,
	__FIMapChangedEventHandler_2_HSTRING_HSTRING *handler, EventRegistrationToken *token)
{
	struct qualifiers *self = ((struct qualifier_observable *)iface)->owner;
	if (!token)
		return E_POINTER;
	if (!handler)
		return E_INVALIDARG;
	EnterCriticalSection(&self->lock);
	if (self->subscription_count == self->subscription_capacity) {
		const size_t capacity = self->subscription_capacity ? self->subscription_capacity * 2 : 4;
		struct qualifier_subscription *grown = self->subscriptions
			? (struct qualifier_subscription *)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, self->subscriptions,
				capacity * sizeof *grown)
			: (struct qualifier_subscription *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, capacity * sizeof *grown);
		if (!grown) {
			LeaveCriticalSection(&self->lock);
			return E_OUTOFMEMORY;
		}
		self->subscriptions = grown;
		self->subscription_capacity = capacity;
	}
	handler->lpVtbl->AddRef(handler);
	self->subscriptions[self->subscription_count].token = ++self->next_token;
	self->subscriptions[self->subscription_count].handler = handler;
	++self->subscription_count;
	token->value = self->next_token;
	LeaveCriticalSection(&self->lock);
	return S_OK;
}

static HRESULT WINAPI observable_remove_map_changed(__FIObservableMap_2_HSTRING_HSTRING *iface,
	EventRegistrationToken token)
{
	struct qualifiers *self = ((struct qualifier_observable *)iface)->owner;
	EnterCriticalSection(&self->lock);
	for (size_t index = 0; index < self->subscription_count; ++index) {
		if (self->subscriptions[index].token != token.value)
			continue;
		__FIMapChangedEventHandler_2_HSTRING_HSTRING *handler = self->subscriptions[index].handler;
		self->subscriptions[index] = self->subscriptions[self->subscription_count - 1];
		--self->subscription_count;
		if (handler)
			handler->lpVtbl->Release(handler);
		break;
	}
	LeaveCriticalSection(&self->lock);
	return S_OK;
}

static HRESULT WINAPI observable_query(__FIObservableMap_2_HSTRING_HSTRING *iface, REFIID iid, void **out)
{
	struct qualifiers *self = ((struct qualifier_observable *)iface)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IWeakReferenceSource))
		*out = &self->weak;
	else if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___FIObservableMap_2_HSTRING_HSTRING))
		*out = iface;
	else if (equal_iid(iid, &IID___FIMap_2_HSTRING_HSTRING))
		*out = self;
	else if (equal_iid(iid, &IID___FIMapView_2_HSTRING_HSTRING))
		*out = &self->view;
	else if (equal_iid(iid, &IID_IAgileObject))
		/* agile through the owner map: see map_query for the thread-safety justification */
		*out = iface;
	else
		return E_NOINTERFACE;
	qualifier_addref(self);
	return S_OK;
}

static struct qualifiers *create_qualifiers(void)
{
	struct qualifiers *map = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *map);
	if (!map)
		return NULL;
	map->lpVtbl = &map_table;
	map->refs = 1;
	map->view.lpVtbl = &view_table;
	map->view.owner = map;
	map->observable.lpVtbl = &observable_table;
	map->observable.owner = map;
	raven_weak_source_init(&map->weak, map, &map->refs, (raven_weak_query_fn)map_query);
	InitializeCriticalSection(&map->lock);
	InterlockedIncrement(&raven_xaml_objects);
	return map;
}

static HRESULT WINAPI context_qualifiers(resource_context *iface, uint64_t out)
{
	struct qualifiers *map;
	(void)iface;
	if (!out)
		return E_POINTER;
	if (!(map = create_qualifiers()))
		return E_OUTOFMEMORY;
	/* QualifierValues is declared as IObservableMap<HSTRING,HSTRING>: the caller receives the observable part and reaches the
	 * map methods through its QueryInterface. */
	*(void **)(uintptr_t)out = &map->observable;
	return S_OK;
}

static HRESULT WINAPI context_reset(resource_context *iface)
{
	(void)iface;
	return S_OK;
}

static HRESULT WINAPI context_reset_list(resource_context *iface, uint64_t list)
{
	(void)iface;
	(void)list;
	return S_OK;
}

static HRESULT WINAPI context_override(resource_context *iface, uint64_t list)
{
	(void)iface;
	(void)list;
	return S_OK;
}

static HRESULT WINAPI context_clone(resource_context *iface, uint64_t out)
{
	struct context *clone;
	(void)iface;
	if (!out)
		return E_POINTER;
	if (!(clone = create_context()))
		return E_OUTOFMEMORY;
	*(void **)(uintptr_t)out = clone;
	return S_OK;
}

static HRESULT WINAPI context_languages(resource_context *iface, uint64_t out)
{
	struct language_list *list;
	(void)iface;
	if (!out)
		return E_POINTER;
	if (!(list = create_language_list()))
		return E_OUTOFMEMORY;
	*(void **)(uintptr_t)out = list;
	return S_OK;
}

static HRESULT WINAPI context_put_languages(resource_context *iface, uint64_t list)
{
	(void)iface;
	(void)list;
	return S_OK;
}

STUB2(static1, statics1_matching)
STUB2(static4, statics4_for_ui)

static HRESULT WINAPI statics2_current_view(static2 *iface, uint64_t out)
{
	struct context *context;
	(void)iface;
	if (!out)
		return E_POINTER;
	if (!(context = create_context()))
		return E_OUTOFMEMORY;
	*(void **)(uintptr_t)out = context;
	return S_OK;
}

static HRESULT WINAPI statics2_independent(static2 *iface, uint64_t out)
{
	return statics2_current_view(iface, out);
}

static HRESULT WINAPI statics2_set_global(static2 *iface, uint64_t name, uint64_t value)
{
	(void)iface;
	(void)name;
	(void)value;
	return S_OK;
}

static HRESULT WINAPI statics2_reset_global(static2 *iface)
{
	(void)iface;
	return S_OK;
}

static HRESULT WINAPI statics2_reset_global_list(static2 *iface, uint64_t list)
{
	(void)iface;
	(void)list;
	return S_OK;
}

static HRESULT WINAPI statics3_set_global(static3 *iface, uint64_t name, uint64_t value, uint64_t language)
{
	(void)iface;
	(void)name;
	(void)value;
	(void)language;
	return S_OK;
}

static const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStaticsVtbl statics1_table = {
	.QueryInterface = part1_query,
	.AddRef = part1_addref,
	.Release = part1_release,
	.GetIids = part1_iids,
	.GetRuntimeClassName = part1_name,
	.GetTrustLevel = part1_trust,
	.CreateMatchingContext = statics1_matching,
};

static const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2Vtbl statics2_table = {
	.QueryInterface = part2_query,
	.AddRef = part2_addref,
	.Release = part2_release,
	.GetIids = part2_iids,
	.GetRuntimeClassName = part2_name,
	.GetTrustLevel = part2_trust,
	.GetForCurrentView = statics2_current_view,
	.SetGlobalQualifierValue = statics2_set_global,
	.ResetGlobalQualifierValues = statics2_reset_global,
	.ResetGlobalQualifierValues2 = statics2_reset_global_list,
	.GetForViewIndependentUse = statics2_independent,
};

static const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics3Vtbl statics3_table = {
	.QueryInterface = part3_query,
	.AddRef = part3_addref,
	.Release = part3_release,
	.GetIids = part3_iids,
	.GetRuntimeClassName = part3_name,
	.GetTrustLevel = part3_trust,
	.SetGlobalQualifierValue = statics3_set_global,
};

static const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics4Vtbl statics4_table = {
	.QueryInterface = part4_query,
	.AddRef = part4_addref,
	.Release = part4_release,
	.GetIids = part4_iids,
	.GetRuntimeClassName = part4_name,
	.GetTrustLevel = part4_trust,
	.GetForUIContext = statics4_for_ui,
};

static const IActivationFactoryVtbl activation_table = {
	.QueryInterface = activation_query,
	.AddRef = activation_addref,
	.Release = activation_release,
	.GetIids = activation_iids,
	.GetRuntimeClassName = activation_name,
	.GetTrustLevel = activation_trust,
	.ActivateInstance = activation_activate,
};

static struct factory factory_instance = {
	{&activation_table, &factory_instance},
	{&statics1_table, &factory_instance},
	{&statics2_table, &factory_instance},
	{&statics3_table, &factory_instance},
	{&statics4_table, &factory_instance},
	1,
};

HRESULT raven_xaml_resource_context_factory(HSTRING name, IActivationFactory **out)
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
