/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.UI.Xaml.ResourceDictionary class.
 *
 * A ResourceDictionary is what an application's App.xaml compiles to and what Application.Resources holds: a map of keys to
 * resources, plus the dictionaries merged into it. The class answers IResourceDictionary, whose own members are the merged
 * dictionary vector and the Source and ThemeDictionaries properties, and the parameterized interface
 * Windows.Foundation.Collections.IMap`2<IInspectable, IInspectable> it derives from — the interface the compiled markup and a
 * WinRT caller use to Insert and Lookup. The object is one COM identity: every part answers for the others, the way the
 * application and element classes do.
 *
 * The interface declarations come from Windows.UI.Xaml.winmd by scripts/generate-winrt-header.py; the parameterized
 * instantiations are the hand-derived declarations in windows_ui_xaml_dictionary.h, whose IIDs follow the signature rule and
 * appear in the real Windows.UI.Xaml.dll. The map's vtable carries the seven methods of the generic definition without the
 * inherited IIterable`1.First, the same rule the children vector of xaml_elements.c follows, so no IIterable part exists here
 * yet; enumeration of the contents arrives with the style stage that walks it.
 *
 * Keys: the compiled markup passes string keys, which a WinRT caller sees boxed, so a key that answers
 * IReference`1<HSTRING> compares by its string content (WindowsCompareStringOrdinal) and every other key compares by IUnknown
 * identity. Values are IInspectable references the dictionary owns; Insert replaces an existing key's value and reports the
 * replacement, Lookup and Remove of a missing key fail with E_BOUNDS per the IMap contract, and Clear empties the map.
 * Mutations do not re-render yet: nothing in the render pass reads resources until the style stage arrives. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include "windows_ui_xaml_dictionary.h"

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

typedef __x_ABI_CWindows_CUI_CXaml_CIResourceDictionary resource1;
typedef __FIReference_1_HSTRING boxed_string;

static const WCHAR class_name[] = L"Windows.UI.Xaml.ResourceDictionary";
static const WCHAR map_class_name[] =
	L"Windows.Foundation.Collections.IMap`2<Windows.Foundation.IInspectable, Windows.Foundation.IInspectable>";
static const WCHAR map_view_class_name[] =
	L"Windows.Foundation.Collections.IMapView`2<Windows.Foundation.IInspectable, Windows.Foundation.IInspectable>";
static const WCHAR merged_class_name[] =
	L"Windows.Foundation.Collections.IVector`1<Windows.UI.Xaml.ResourceDictionary>";

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

static HRESULT make_name(const WCHAR *text, HSTRING *name)
{
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), name);
}

/* The dictionary */

struct dictionary;
struct merged_vector;

struct resource_part { const __x_ABI_CWindows_CUI_CXaml_CIResourceDictionaryVtbl *lpVtbl; struct dictionary *owner; };
struct map_part { const __FIMap_2_IInspectable_IInspectableVtbl *lpVtbl; struct dictionary *owner; };

struct dictionary_entry {
	IInspectable *key;
	IInspectable *value;
};

/* The merged dictionaries: the vector IResourceDictionary.MergedDictionaries hands out, appended to by the compiled App.xaml
 * (XamlControlsResources and the theme dictionaries). The same subset the children vector implements is real; the slots the
 * loader does not need yet answer E_NOTIMPL at their exact signatures. */
struct merged_vector {
	const __FIVector_1_Windows__CUI__CXaml__CResourceDictionaryVtbl *lpVtbl;
	LONG refs;
	resource1 **items;
	UINT32 count, capacity;
};

struct dictionary {
	struct resource_part resource;
	struct map_part map;
	LONG refs;
	struct dictionary_entry *entries;
	UINT32 count, capacity;
	struct merged_vector *merged;
};

static HRESULT dictionary_query_any(struct dictionary *self, REFIID iid, void **out);
static ULONG dictionary_addref(struct dictionary *self);
static ULONG dictionary_release(struct dictionary *self);
ULONG merged_release(struct merged_vector *self);

/* Two boxed strings compare by their content; anything else compares by IUnknown identity, since one object can arrive through
 * different interfaces. A boxed string never matches a non-string key by content. */
static int key_matches(IInspectable *stored, IInspectable *probe)
{
	boxed_string *left = NULL, *right = NULL;
	const int left_string = SUCCEEDED(stored->lpVtbl->QueryInterface(stored, &IID___FIReference_1_HSTRING,
		(void **)&left)) && left;
	int matches = 0;
	if (left_string &&
		SUCCEEDED(probe->lpVtbl->QueryInterface(probe, &IID___FIReference_1_HSTRING, (void **)&right)) && right) {
		HSTRING one = NULL, other = NULL;
		INT32 order = 1;
		if (left->lpVtbl->get_Value(left, &one) == S_OK && right->lpVtbl->get_Value(right, &other) == S_OK &&
			WindowsCompareStringOrdinal(one, other, &order) == S_OK)
			matches = order == 0;
		if (one)
			WindowsDeleteString(one);
		if (other)
			WindowsDeleteString(other);
	} else {
		IUnknown *one = NULL, *other = NULL;
		if (SUCCEEDED(stored->lpVtbl->QueryInterface(stored, &IID_IUnknown, (void **)&one)) && one &&
			SUCCEEDED(probe->lpVtbl->QueryInterface(probe, &IID_IUnknown, (void **)&other)) && other)
			matches = one == other;
		if (one)
			one->lpVtbl->Release(one);
		if (other)
			other->lpVtbl->Release(other);
	}
	if (left)
		left->lpVtbl->Release(left);
	if (right)
		right->lpVtbl->Release(right);
	return matches;
}

static struct dictionary_entry *dictionary_find(struct dictionary *self, IInspectable *key)
{
	for (UINT32 index = 0; index < self->count; ++index)
		if (key_matches(self->entries[index].key, key))
			return &self->entries[index];
	return NULL;
}

static ULONG dictionary_addref(struct dictionary *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG dictionary_release(struct dictionary *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		for (UINT32 index = 0; index < self->count; ++index) {
			if (self->entries[index].key)
				self->entries[index].key->lpVtbl->Release(self->entries[index].key);
			if (self->entries[index].value)
				self->entries[index].value->lpVtbl->Release(self->entries[index].value);
		}
		if (self->entries)
			FREE(self->entries);
		if (self->merged)
			merged_release(self->merged);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

/* The IResourceDictionary part */

static HRESULT WINAPI resource_query(resource1 *iface, REFIID iid, void **out)
{
	return dictionary_query_any(((struct resource_part *)iface)->owner, iid, out);
}

static ULONG WINAPI resource_addref(resource1 *iface)
{
	return dictionary_addref(((struct resource_part *)iface)->owner);
}

static ULONG WINAPI resource_release(resource1 *iface)
{
	return dictionary_release(((struct resource_part *)iface)->owner);
}

static HRESULT WINAPI resource_iids(resource1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI resource_name(resource1 *iface, HSTRING *name)
{
	(void)iface;
	return make_name(class_name, name);
}

static HRESULT WINAPI resource_trust(resource1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

/* Source takes a Windows.Foundation.Uri, a class the engine does not implement yet. */
static HRESULT WINAPI resource_get_source(resource1 *iface, uint64_t out)
{
	(void)iface;
	if (out)
		*(void **)(uintptr_t)out = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI resource_put_source(resource1 *iface, uint64_t value)
{
	(void)iface;
	(void)value;
	return E_NOTIMPL;
}

static HRESULT WINAPI resource_get_merged(resource1 *iface, uint64_t out)
{
	struct dictionary *self = ((struct resource_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	InterlockedIncrement(&self->merged->refs);
	*(void **)(uintptr_t)out = self->merged;
	return S_OK;
}

/* ThemeDictionaries is a second map the theme stage fills; until that stage the slot answers E_NOTIMPL. */
static HRESULT WINAPI resource_get_theme(resource1 *iface, uint64_t out)
{
	(void)iface;
	if (out)
		*(void **)(uintptr_t)out = NULL;
	return E_NOTIMPL;
}

static const __x_ABI_CWindows_CUI_CXaml_CIResourceDictionaryVtbl resource_table = {
	.QueryInterface = resource_query,
	.AddRef = resource_addref,
	.Release = resource_release,
	.GetIids = resource_iids,
	.GetRuntimeClassName = resource_name,
	.GetTrustLevel = resource_trust,
	.get_Source = resource_get_source,
	.put_Source = resource_put_source,
	.get_MergedDictionaries = resource_get_merged,
	.get_ThemeDictionaries = resource_get_theme,
};

/* The IMap<Object, Object> part: the slots the compiled markup and a WinRT caller use. */

static HRESULT WINAPI map_query(__FIMap_2_IInspectable_IInspectable *iface, REFIID iid, void **out)
{
	return dictionary_query_any(((struct map_part *)iface)->owner, iid, out);
}

static ULONG WINAPI map_addref(__FIMap_2_IInspectable_IInspectable *iface)
{
	return dictionary_addref(((struct map_part *)iface)->owner);
}

static ULONG WINAPI map_release(__FIMap_2_IInspectable_IInspectable *iface)
{
	return dictionary_release(((struct map_part *)iface)->owner);
}

static HRESULT WINAPI map_iids(__FIMap_2_IInspectable_IInspectable *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI map_name(__FIMap_2_IInspectable_IInspectable *iface, HSTRING *name)
{
	(void)iface;
	return make_name(map_class_name, name);
}

static HRESULT WINAPI map_trust(__FIMap_2_IInspectable_IInspectable *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI map_lookup(__FIMap_2_IInspectable_IInspectable *iface, IInspectable *key, IInspectable **value)
{
	struct dictionary *self = ((struct map_part *)iface)->owner;
	struct dictionary_entry *entry;
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (!key)
		return E_POINTER;
	entry = dictionary_find(self, key);
	if (!entry)
		return E_BOUNDS;
	*value = entry->value;
	if (*value)
		(*value)->lpVtbl->AddRef(*value);
	return S_OK;
}

static HRESULT WINAPI map_size(__FIMap_2_IInspectable_IInspectable *iface, UINT32 *value)
{
	if (!value)
		return E_POINTER;
	*value = ((struct map_part *)iface)->owner->count;
	return S_OK;
}

static HRESULT WINAPI map_has_key(__FIMap_2_IInspectable_IInspectable *iface, IInspectable *key, boolean *found)
{
	struct dictionary *self = ((struct map_part *)iface)->owner;
	if (!found)
		return E_POINTER;
	*found = FALSE;
	if (!key)
		return E_POINTER;
	*found = dictionary_find(self, key) ? TRUE : FALSE;
	return S_OK;
}

static HRESULT WINAPI map_get_view(__FIMap_2_IInspectable_IInspectable *iface,
	__FIMapView_2_IInspectable_IInspectable **view);

static HRESULT WINAPI map_insert(__FIMap_2_IInspectable_IInspectable *iface, IInspectable *key, IInspectable *value,
	boolean *replaced)
{
	struct dictionary *self = ((struct map_part *)iface)->owner;
	struct dictionary_entry *entry;
	if (replaced)
		*replaced = FALSE;
	if (!key)
		return E_POINTER;
	entry = dictionary_find(self, key);
	if (entry) {
		if (replaced)
			*replaced = TRUE;
		if (value)
			value->lpVtbl->AddRef(value);
		if (entry->value)
			entry->value->lpVtbl->Release(entry->value);
		entry->value = value;
		return S_OK;
	}
	if (self->count == self->capacity) {
		const UINT32 capacity = self->capacity ? self->capacity * 2 : 4;
		struct dictionary_entry *entries =
			HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size_t)capacity * sizeof *entries);
		if (!entries)
			return E_OUTOFMEMORY;
		if (self->entries) {
			for (UINT32 index = 0; index < self->count; ++index)
				entries[index] = self->entries[index];
			FREE(self->entries);
		}
		self->entries = entries;
		self->capacity = capacity;
	}
	self->entries[self->count].key = key;
	self->entries[self->count].value = value;
	key->lpVtbl->AddRef(key);
	if (value)
		value->lpVtbl->AddRef(value);
	++self->count;
	return S_OK;
}

static HRESULT WINAPI map_remove(__FIMap_2_IInspectable_IInspectable *iface, IInspectable *key)
{
	struct dictionary *self = ((struct map_part *)iface)->owner;
	if (!key)
		return E_POINTER;
	for (UINT32 index = 0; index < self->count; ++index) {
		if (!key_matches(self->entries[index].key, key))
			continue;
		self->entries[index].key->lpVtbl->Release(self->entries[index].key);
		if (self->entries[index].value)
			self->entries[index].value->lpVtbl->Release(self->entries[index].value);
		for (UINT32 rest = index; rest + 1 < self->count; ++rest)
			self->entries[rest] = self->entries[rest + 1];
		--self->count;
		return S_OK;
	}
	return E_BOUNDS;
}

static HRESULT WINAPI map_clear(__FIMap_2_IInspectable_IInspectable *iface)
{
	struct dictionary *self = ((struct map_part *)iface)->owner;
	for (UINT32 index = 0; index < self->count; ++index) {
		self->entries[index].key->lpVtbl->Release(self->entries[index].key);
		if (self->entries[index].value)
			self->entries[index].value->lpVtbl->Release(self->entries[index].value);
	}
	self->count = 0;
	return S_OK;
}

static const __FIMap_2_IInspectable_IInspectableVtbl map_table = {
	.QueryInterface = map_query,
	.AddRef = map_addref,
	.Release = map_release,
	.GetIids = map_iids,
	.GetRuntimeClassName = map_name,
	.GetTrustLevel = map_trust,
	.Lookup = map_lookup,
	.get_Size = map_size,
	.HasKey = map_has_key,
	.GetView = map_get_view,
	.Insert = map_insert,
	.Remove = map_remove,
	.Clear = map_clear,
};

/* The IMapView<Object, Object> GetView hands out: it reads the dictionary's storage, so it sees later mutations the way the
 * live map does. Split, which a real view offers on oversized maps, is not implemented yet. */

struct map_view {
	const __FIMapView_2_IInspectable_IInspectableVtbl *lpVtbl;
	struct dictionary *owner;
	LONG refs;
};

static HRESULT WINAPI view_query(__FIMapView_2_IInspectable_IInspectable *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID___FIMapView_2_IInspectable_IInspectable))
		return E_NOINTERFACE;
	InterlockedIncrement(&((struct map_view *)iface)->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI view_addref(__FIMapView_2_IInspectable_IInspectable *iface)
{
	return (ULONG)InterlockedIncrement(&((struct map_view *)iface)->refs);
}

static ULONG WINAPI view_release(__FIMapView_2_IInspectable_IInspectable *iface)
{
	struct map_view *self = (struct map_view *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		dictionary_release(self->owner);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI view_iids(__FIMapView_2_IInspectable_IInspectable *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI view_name(__FIMapView_2_IInspectable_IInspectable *iface, HSTRING *name)
{
	(void)iface;
	return make_name(map_view_class_name, name);
}

static HRESULT WINAPI view_trust(__FIMapView_2_IInspectable_IInspectable *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI view_lookup(__FIMapView_2_IInspectable_IInspectable *iface, IInspectable *key, IInspectable **value)
{
	struct dictionary *self = ((struct map_view *)iface)->owner;
	struct dictionary_entry *entry;
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (!key)
		return E_POINTER;
	entry = dictionary_find(self, key);
	if (!entry)
		return E_BOUNDS;
	*value = entry->value;
	if (*value)
		(*value)->lpVtbl->AddRef(*value);
	return S_OK;
}

static HRESULT WINAPI view_size(__FIMapView_2_IInspectable_IInspectable *iface, UINT32 *value)
{
	if (!value)
		return E_POINTER;
	*value = ((struct map_view *)iface)->owner->count;
	return S_OK;
}

static HRESULT WINAPI view_has_key(__FIMapView_2_IInspectable_IInspectable *iface, IInspectable *key, boolean *found)
{
	struct dictionary *self = ((struct map_view *)iface)->owner;
	if (!found)
		return E_POINTER;
	*found = FALSE;
	if (!key)
		return E_POINTER;
	*found = dictionary_find(self, key) ? TRUE : FALSE;
	return S_OK;
}

static HRESULT WINAPI view_split(__FIMapView_2_IInspectable_IInspectable *iface,
	__FIMapView_2_IInspectable_IInspectable **first, __FIMapView_2_IInspectable_IInspectable **second)
{
	(void)iface;
	if (first)
		*first = NULL;
	if (second)
		*second = NULL;
	return E_NOTIMPL;
}

static const __FIMapView_2_IInspectable_IInspectableVtbl map_view_table = {
	.QueryInterface = view_query,
	.AddRef = view_addref,
	.Release = view_release,
	.GetIids = view_iids,
	.GetRuntimeClassName = view_name,
	.GetTrustLevel = view_trust,
	.Lookup = view_lookup,
	.get_Size = view_size,
	.HasKey = view_has_key,
	.Split = view_split,
};

static HRESULT WINAPI map_get_view(__FIMap_2_IInspectable_IInspectable *iface,
	__FIMapView_2_IInspectable_IInspectable **view)
{
	struct dictionary *self = ((struct map_part *)iface)->owner;
	struct map_view *instance;
	if (!view)
		return E_POINTER;
	*view = NULL;
	if (!(instance = ALLOC(sizeof *instance)))
		return E_OUTOFMEMORY;
	instance->lpVtbl = &map_view_table;
	instance->owner = self;
	instance->refs = 1;
	dictionary_addref(self);
	InterlockedIncrement(&raven_xaml_objects);
	*view = (__FIMapView_2_IInspectable_IInspectable *)instance;
	return S_OK;
}

/* The merged dictionary vector's methods. */

static HRESULT WINAPI merged_query(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface, REFIID iid, void **out)
{
	struct merged_vector *self = (struct merged_vector *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID___FIVector_1_Windows__CUI__CXaml__CResourceDictionary))
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	*out = self;
	return S_OK;
}

ULONG merged_release(struct merged_vector *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		for (UINT32 index = 0; index < self->count; ++index)
			if (self->items[index])
				self->items[index]->lpVtbl->Release(self->items[index]);
		if (self->items)
			FREE(self->items);
		FREE(self);
	}
	return (ULONG)refs;
}

/* The release the vector's own vtable carries; the dictionary's release reaches merged_release directly. */
static ULONG WINAPI merged_vector_release(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface)
{
	return merged_release((struct merged_vector *)iface);
}

static ULONG WINAPI merged_addref(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface)
{
	return (ULONG)InterlockedIncrement(&((struct merged_vector *)iface)->refs);
}

static HRESULT WINAPI merged_iids(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI merged_name(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface, HSTRING *name)
{
	(void)iface;
	return make_name(merged_class_name, name);
}

static HRESULT WINAPI merged_trust(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI merged_get_at(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface, UINT32 index,
	resource1 **value)
{
	struct merged_vector *self = (struct merged_vector *)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (index >= self->count)
		return E_BOUNDS;
	*value = self->items[index];
	(*value)->lpVtbl->AddRef(*value);
	return S_OK;
}

static HRESULT WINAPI merged_size(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface, UINT32 *value)
{
	if (!value)
		return E_POINTER;
	*value = ((struct merged_vector *)iface)->count;
	return S_OK;
}

static HRESULT WINAPI merged_get_view(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface, void **value)
{
	(void)iface;
	if (value)
		*value = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI merged_index_of(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface, resource1 *element,
	UINT32 *index, boolean *value)
{
	struct merged_vector *self = (struct merged_vector *)iface;
	if (!index || !value)
		return E_POINTER;
	*value = FALSE;
	*index = 0;
	/* Collection identity is IUnknown identity, as in the children vector. */
	IUnknown *unknown = NULL;
	if (!element || FAILED(((IInspectable *)element)->lpVtbl->QueryInterface((IInspectable *)element, &IID_IUnknown,
		(void **)&unknown)))
		return S_OK;
	for (UINT32 position = 0; position < self->count; ++position) {
		IUnknown *other = NULL;
		if (FAILED(((IInspectable *)self->items[position])->lpVtbl->QueryInterface((IInspectable *)self->items[position],
			&IID_IUnknown, (void **)&other)))
			continue;
		const int same = other == unknown;
		other->lpVtbl->Release(other);
		if (same) {
			*index = position;
			*value = TRUE;
			break;
		}
	}
	unknown->lpVtbl->Release(unknown);
	return S_OK;
}

static HRESULT WINAPI merged_set_at(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface, UINT32 index,
	resource1 *value)
{
	(void)iface;
	(void)index;
	(void)value;
	return E_NOTIMPL;
}

static HRESULT WINAPI merged_insert_at(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface, UINT32 index,
	resource1 *value)
{
	(void)iface;
	(void)index;
	(void)value;
	return E_NOTIMPL;
}

static HRESULT WINAPI merged_remove_at(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface, UINT32 index)
{
	(void)iface;
	(void)index;
	return E_NOTIMPL;
}

static HRESULT WINAPI merged_append(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface, resource1 *value)
{
	struct merged_vector *self = (struct merged_vector *)iface;
	if (!value)
		return E_POINTER;
	if (self->count == self->capacity) {
		const UINT32 capacity = self->capacity ? self->capacity * 2 : 4;
		resource1 **items = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size_t)capacity * sizeof *items);
		if (!items)
			return E_OUTOFMEMORY;
		if (self->items) {
			for (UINT32 index = 0; index < self->count; ++index)
				items[index] = self->items[index];
			FREE(self->items);
		}
		self->items = items;
		self->capacity = capacity;
	}
	self->items[self->count] = value;
	value->lpVtbl->AddRef(value);
	++self->count;
	return S_OK;
}

static HRESULT WINAPI merged_remove_at_end(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface)
{
	(void)iface;
	return E_NOTIMPL;
}

static HRESULT WINAPI merged_clear(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface)
{
	struct merged_vector *self = (struct merged_vector *)iface;
	for (UINT32 index = 0; index < self->count; ++index)
		if (self->items[index])
			self->items[index]->lpVtbl->Release(self->items[index]);
	self->count = 0;
	return S_OK;
}

static HRESULT WINAPI merged_get_many(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface, UINT32 start_index,
	UINT32 items_size, resource1 **items, UINT32 *value)
{
	(void)iface;
	(void)start_index;
	(void)items_size;
	(void)items;
	if (value)
		*value = 0;
	return E_NOTIMPL;
}

static HRESULT WINAPI merged_replace_all(__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *iface, UINT32 count,
	resource1 **items)
{
	(void)iface;
	(void)count;
	(void)items;
	return E_NOTIMPL;
}

static const __FIVector_1_Windows__CUI__CXaml__CResourceDictionaryVtbl merged_table = {
	.QueryInterface = merged_query,
	.AddRef = merged_addref,
	.Release = merged_vector_release,
	.GetIids = merged_iids,
	.GetRuntimeClassName = merged_name,
	.GetTrustLevel = merged_trust,
	.GetAt = merged_get_at,
	.get_Size = merged_size,
	.GetView = merged_get_view,
	.IndexOf = merged_index_of,
	.SetAt = merged_set_at,
	.InsertAt = merged_insert_at,
	.RemoveAt = merged_remove_at,
	.Append = merged_append,
	.RemoveAtEnd = merged_remove_at_end,
	.Clear = merged_clear,
	.GetMany = merged_get_many,
	.ReplaceAll = merged_replace_all,
};

static struct merged_vector *create_merged(void)
{
	struct merged_vector *vector = ALLOC(sizeof *vector);
	if (!vector)
		return NULL;
	vector->lpVtbl = &merged_table;
	vector->refs = 1;
	return vector;
}

/* One COM identity: the resource and map parts answer for the whole object. */

static HRESULT dictionary_query_any(struct dictionary *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary))
		*out = &self->resource;
	else if (equal_iid(iid, &IID___FIMap_2_IInspectable_IInspectable))
		*out = &self->map;
	else
		return E_NOINTERFACE;
	dictionary_addref(self);
	return S_OK;
}

static struct dictionary *create_dictionary(void)
{
	struct dictionary *dictionary = ALLOC(sizeof *dictionary);
	if (!dictionary)
		return NULL;
	dictionary->resource.lpVtbl = &resource_table;
	dictionary->resource.owner = dictionary;
	dictionary->map.lpVtbl = &map_table;
	dictionary->map.owner = dictionary;
	dictionary->refs = 1;
	dictionary->merged = create_merged();
	if (!dictionary->merged) {
		FREE(dictionary);
		return NULL;
	}
	InterlockedIncrement(&raven_xaml_objects);
	return dictionary;
}

/* The factory, reached by name through the catalog like every class of the component. */

struct dictionary_factory {
	const IActivationFactoryVtbl *lpVtbl;
	LONG refs;
};

static HRESULT WINAPI factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
	struct dictionary_factory *self = (struct dictionary_factory *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) && !equal_iid(iid, &IID_IActivationFactory))
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	*out = self;
	return S_OK;
}

static ULONG WINAPI factory_addref(IActivationFactory *iface)
{
	return (ULONG)InterlockedIncrement(&((struct dictionary_factory *)iface)->refs);
}

static ULONG WINAPI factory_release(IActivationFactory *iface)
{
	return (ULONG)InterlockedDecrement(&((struct dictionary_factory *)iface)->refs);
}

static HRESULT WINAPI factory_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI factory_name(IActivationFactory *iface, HSTRING *name)
{
	(void)iface;
	return make_name(class_name, name);
}

static HRESULT WINAPI factory_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out)
{
	struct dictionary *dictionary;
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!(dictionary = create_dictionary()))
		return E_OUTOFMEMORY;
	*out = (IInspectable *)&dictionary->resource;
	return S_OK;
}

static const IActivationFactoryVtbl factory_table = {
	.QueryInterface = factory_query,
	.AddRef = factory_addref,
	.Release = factory_release,
	.GetIids = factory_iids,
	.GetRuntimeClassName = factory_name,
	.GetTrustLevel = factory_trust,
	.ActivateInstance = factory_activate,
};

static struct dictionary_factory factory_instance = {&factory_table, 1};

HRESULT raven_dictionary_factory(HSTRING name, IActivationFactory **out)
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
	*out = (IActivationFactory *)&factory_instance;
	return S_OK;
}
