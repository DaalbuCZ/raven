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

static struct context *create_context(void)
{
	struct context *context = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *context);
	if (!context)
		return NULL;
	context->lpVtbl = &context_table;
	context->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	return context;
}

static HRESULT WINAPI context_query(resource_context *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext))
		return E_NOINTERFACE;
	InterlockedIncrement(&((struct context *)iface)->refs);
	*out = iface;
	return S_OK;
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
		HeapFree(GetProcessHeap(), 0, self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI context_iids(resource_context *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
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
 * resource context starts with. The map is read-only: insertion, removal and clearing answer E_NOTIMPL. */
struct qualifiers;

struct qualifier_view {
	const __FIMapView_2_HSTRING_HSTRINGVtbl *lpVtbl;
	struct qualifiers *owner;
};

struct qualifiers {
	const __FIMap_2_HSTRING_HSTRINGVtbl *lpVtbl;
	LONG refs;
	struct qualifier_view view;
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
	if (!key) {
		*length = 0;
		return NULL;
	}
	return match_qualifier(WindowsGetStringRawBuffer(key, length), *length);
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

static HRESULT WINAPI map_query(__FIMap_2_HSTRING_HSTRING *iface, REFIID iid, void **out)
{
	struct qualifiers *self = (struct qualifiers *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___FIMap_2_HSTRING_HSTRING))
		*out = iface;
	else if (equal_iid(iid, &IID___FIMapView_2_HSTRING_HSTRING))
		*out = &self->view;
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
		HeapFree(GetProcessHeap(), 0, self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI map_iids(__FIMap_2_HSTRING_HSTRING *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI map_name(__FIMap_2_HSTRING_HSTRING *iface, HSTRING *name)
{
	static const WCHAR map_name_text[] = L"Windows.Foundation.Collections.IMap`2<Windows.Foundation.HSTRING,Windows.Foundation.HSTRING>";
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
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI view_name(__FIMapView_2_HSTRING_HSTRING *iface, HSTRING *name)
{
	static const WCHAR view_name_text[] = L"Windows.Foundation.Collections.IMapView`2<Windows.Foundation.HSTRING,Windows.Foundation.HSTRING>";
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
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___FIMapView_2_HSTRING_HSTRING))
		*out = iface;
	else if (equal_iid(iid, &IID___FIMap_2_HSTRING_HSTRING))
		*out = self;
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
	*(void **)(uintptr_t)out = map;
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
	(void)iface;
	(void)out;

	return E_NOTIMPL;
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
