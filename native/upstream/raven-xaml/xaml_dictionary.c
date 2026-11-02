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
 * application and element classes do; ThemeDictionaries is the one member that hands out a second object, because a map that
 * is not the dictionary itself is what the property returns.
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
 * Mutations do not re-render yet: nothing in the render pass reads resources until the style stage arrives.
 *
 * Source is the one member that loads contents the caller did not Insert: setting it resolves the Uri's local package path
 * through the resource index (the same PRI path Application.LoadComponent uses), loads the compiled dictionary document into
 * a fresh dictionary and adopts its entries. The load is atomic — a document that fails leaves this dictionary as it was —
 * and a Source that resolves back into itself is refused at a bounded recursion depth instead of looping. The Windows
 * behavior of clearing Source with NULL is not reproduced: the setter refuses NULL, because whether the contents clear with
 * it cannot be verified here. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include "windows_ui_xaml_dictionary.h"
#include <windows.foundation.h>

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

typedef __x_ABI_CWindows_CUI_CXaml_CIResourceDictionary resource1;
typedef __x_ABI_CWindows_CFoundation_CIUriRuntimeClass uri1;
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
struct dictionary_store;

struct resource_part { const __x_ABI_CWindows_CUI_CXaml_CIResourceDictionaryVtbl *lpVtbl; struct dictionary *owner; };
struct map_part {
	const __FIMap_2_IInspectable_IInspectableVtbl *lpVtbl;
	struct dictionary *owner;
	struct dictionary_store *store;
};

struct dictionary_entry {
	IInspectable *key;
	IInspectable *value;
};

/* One map's storage. The dictionary owns two: its own contents — the IMap it derives from — and ThemeDictionaries, whose
 * keys ("Default", "Dark", "Light", "HighContrast") select the dictionary a theme stage reads. */
struct dictionary_store {
	struct dictionary_entry *entries;
	UINT32 count, capacity;
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
	struct map_part theme;
	LONG refs;
	struct dictionary_store store;
	struct dictionary_store themes;
	IInspectable *source;
	struct merged_vector *merged;
};

static struct dictionary *create_dictionary(void);
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

static struct dictionary_entry *store_find(struct dictionary_store *store, IInspectable *key)
{
	for (UINT32 index = 0; index < store->count; ++index)
		if (key_matches(store->entries[index].key, key))
			return &store->entries[index];
	return NULL;
}

static void store_clear(struct dictionary_store *store)
{
	for (UINT32 index = 0; index < store->count; ++index) {
		if (store->entries[index].key)
			store->entries[index].key->lpVtbl->Release(store->entries[index].key);
		if (store->entries[index].value)
			store->entries[index].value->lpVtbl->Release(store->entries[index].value);
	}
	if (store->entries)
		FREE(store->entries);
	store->entries = NULL;
	store->count = 0;
	store->capacity = 0;
}

/* The entries of one store move into another: Source adoption and its rollback both need the transfer, never a copy. */
static void store_adopt(struct dictionary_store *into, struct dictionary_store *from)
{
	store_clear(into);
	*into = *from;
	from->entries = NULL;
	from->count = 0;
	from->capacity = 0;
}

static ULONG dictionary_addref(struct dictionary *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG dictionary_release(struct dictionary *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		store_clear(&self->store);
		store_clear(&self->themes);
		if (self->source)
			self->source->lpVtbl->Release(self->source);
		if (self->merged)
			merged_release(self->merged);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

/* The value ProvideStaticResourceValue (node 34) and ProvideThemeResourceValue (node 35) put in a dictionary: a keyed
 * reference, not the resource itself, because a compiled App.xaml names resources another dictionary in the same tree
 * defines — a merged dictionary that loads later, or an enclosing scope. The loader follows the reference when the entry
 * is read, through the same scope stack and with a hop limit, so a chain of references terminates. A WinRT caller never
 * sees one: the object answers IInspectable only. */
struct resource_reference {
	const IInspectableVtbl *lpVtbl;
	LONG refs;
	WCHAR *key;
	UINT32 length;
	BOOL theme;
};

static const WCHAR reference_class_name[] = L"Windows.UI.Xaml.ResourceReference";

static HRESULT WINAPI reference_query(IInspectable *This, REFIID iid, void **out)
{
	struct resource_reference *self = (struct resource_reference *)This;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable))
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	*out = self;
	return S_OK;
}

static ULONG WINAPI reference_addref(IInspectable *This)
{
	return (ULONG)InterlockedIncrement(&((struct resource_reference *)This)->refs);
}

static ULONG WINAPI reference_release(IInspectable *This)
{
	struct resource_reference *self = (struct resource_reference *)This;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->key)
			FREE(self->key);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI reference_iids(IInspectable *This, ULONG *count, IID **iids)
{
	(void)This;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI reference_name(IInspectable *This, HSTRING *name)
{
	(void)This;
	return make_name(reference_class_name, name);
}

static HRESULT WINAPI reference_trust(IInspectable *This, TrustLevel *level)
{
	(void)This;
	return inspectable_trust(level);
}

static const IInspectableVtbl reference_table = {
	reference_query, reference_addref, reference_release, reference_iids, reference_name, reference_trust
};

HRESULT raven_xaml_resource_reference_create(const WCHAR *key, UINT32 length, BOOL theme, IInspectable **out)
{
	struct resource_reference *self;
	if (!out || !key || !length)
		return E_POINTER;
	*out = NULL;
	if (!(self = ALLOC(sizeof *self)))
		return E_OUTOFMEMORY;
	if (!(self->key = HeapAlloc(GetProcessHeap(), 0, (SIZE_T)length * sizeof(WCHAR)))) {
		FREE(self);
		return E_OUTOFMEMORY;
	}
	memcpy(self->key, key, (SIZE_T)length * sizeof(WCHAR));
	self->length = length;
	self->theme = theme;
	self->refs = 1;
	self->lpVtbl = &reference_table;
	InterlockedIncrement(&raven_xaml_objects);
	*out = (IInspectable *)self;
	return S_OK;
}

BOOL raven_xaml_resource_reference_key(IInspectable *object, const WCHAR **key, UINT32 *length, BOOL *theme)
{
	struct resource_reference *self;
	if (!object || object->lpVtbl != &reference_table)
		return FALSE;
	self = (struct resource_reference *)object;
	if (key)
		*key = self->key;
	if (length)
		*length = self->length;
	if (theme)
		*theme = self->theme;
	return TRUE;
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

/* The source document a WinRT caller sets: the Uri rides back out unchanged, and the entries arrive through a nested load
 * of the compiled dictionary document. The nested load is bounded, because a document whose Source names itself would
 * otherwise recurse without end; the Windows behavior of setting NULL is not reproduced (see the file comment). */
#define SOURCE_DEPTH 16
static LONG source_depth;

static HRESULT adopt_source(struct dictionary *self, IInspectable *locator)
{
	static const WCHAR prefix[] = L"ms-appx:///";
	static const WCHAR files[] = L"/Files";
	static const WCHAR xaml_suffix[] = L".xaml";
	WCHAR path[1024];
	const WCHAR *text;
	uri1 *uri = NULL;
	HSTRING absolute = NULL;
	const BYTE *blob = NULL;
	UINT32 length = 0, path_length, bytes = 0;
	struct dictionary *loaded = NULL;
	HRESULT result;
	if (!locator)
		return E_INVALIDARG;
	result = IInspectable_QueryInterface(locator, &IID___x_ABI_CWindows_CFoundation_CIUriRuntimeClass, (void **)&uri);
	if (FAILED(result))
		return result;
	result = uri->lpVtbl->get_AbsoluteUri(uri, &absolute);
	uri->lpVtbl->Release(uri);
	if (FAILED(result))
		return result;
	text = WindowsGetStringRawBuffer(absolute, &length);
	if (length < ARRAYSIZE(prefix) - 1 + ARRAYSIZE(xaml_suffix) - 1 ||
		length + ARRAYSIZE(files) >= ARRAYSIZE(path) ||
		CompareStringOrdinal(text, ARRAYSIZE(prefix) - 1, prefix, -1, TRUE) != CSTR_EQUAL ||
		CompareStringOrdinal(text + length - (ARRAYSIZE(xaml_suffix) - 1), ARRAYSIZE(xaml_suffix) - 1, xaml_suffix, -1, TRUE) != CSTR_EQUAL) {
		result = E_NOTIMPL;
		goto done;
	}
	/* Shortcut: only local package XAML without escaping or URI qualifiers is resolved here; the query, fragment, escape
	 * and backslash forms answer E_NOTIMPL the way Application.LoadComponent refuses them. */
	for (UINT32 index = ARRAYSIZE(prefix) - 1; index < length; ++index)
		if (!text[index] || text[index] == L'?' || text[index] == L'#' || text[index] == L'%' || text[index] == L'\\') {
			result = E_NOTIMPL;
			goto done;
		}
	memcpy(path, files, (ARRAYSIZE(files) - 1) * sizeof(WCHAR));
	path_length = ARRAYSIZE(files) - 1 + length - (ARRAYSIZE(prefix) - 2);
	memcpy(path + ARRAYSIZE(files) - 1, text + ARRAYSIZE(prefix) - 2,
		(length - (ARRAYSIZE(prefix) - 2) - 5) * sizeof(WCHAR));
	path_length -= 1; /* .xaml becomes .xbf */
	memcpy(path + path_length - 4, L".xbf", 4 * sizeof(WCHAR));
	result = raven_pri_get_blob(path, path_length, &blob, &bytes);
	if (FAILED(result))
		goto done;
	if (!(loaded = create_dictionary())) {
		result = E_OUTOFMEMORY;
		goto done;
	}
	result = raven_xaml_load_into(blob, bytes, (IInspectable *)&loaded->resource);
	if (FAILED(result))
		goto done;
	/* Adoption replaces the contents: the compiled markup sets Source on a fresh dictionary, and a WinRT caller that wants
	 * to keep earlier entries can merge them in later. Both maps move — a document's ThemeDictionaries is as much part of it
	 * as its own entries. The Uri becomes the Source only once the document landed. */
	store_adopt(&self->store, &loaded->store);
	store_adopt(&self->themes, &loaded->themes);
	if (self->source)
		self->source->lpVtbl->Release(self->source);
	self->source = locator;
	locator->lpVtbl->AddRef(locator);
done:
	if (loaded)
		dictionary_release(loaded);
	WindowsDeleteString(absolute);
	return result;
}

static HRESULT WINAPI resource_get_source(resource1 *iface, uint64_t out)
{
	struct dictionary *self = ((struct resource_part *)iface)->owner;
	IInspectable **value = (IInspectable **)(uintptr_t)out;
	if (!value)
		return E_POINTER;
	*value = self->source;
	if (*value)
		(*value)->lpVtbl->AddRef(*value);
	return S_OK;
}

static HRESULT WINAPI resource_put_source(resource1 *iface, uint64_t value)
{
	struct dictionary *self = ((struct resource_part *)iface)->owner;
	HRESULT result;
	if (InterlockedIncrement(&source_depth) > SOURCE_DEPTH) {
		InterlockedDecrement(&source_depth);
		/* A Source graph that names itself compiles to no usable dictionary; the document is refused, not followed. */
		return E_INVALIDARG;
	}
	result = adopt_source(self, (IInspectable *)(uintptr_t)value);
	InterlockedDecrement(&source_depth);
	return result;
}

static HRESULT WINAPI resource_get_merged(resource1 *iface, void **out)
{
	struct dictionary *self = ((struct resource_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	InterlockedIncrement(&self->merged->refs);
	*out = self->merged;
	return S_OK;
}

/* ThemeDictionaries is a map of its own: the property hands out the same live storage a WinRT caller fills, and the loader
 * inserts into it when a compiled document names the property. */
static HRESULT WINAPI resource_get_theme(resource1 *iface, uint64_t out)
{
	struct dictionary *self = ((struct resource_part *)iface)->owner;
	__FIMap_2_IInspectable_IInspectable **value = (__FIMap_2_IInspectable_IInspectable **)(uintptr_t)out;
	if (!value)
		return E_POINTER;
	*value = (__FIMap_2_IInspectable_IInspectable *)&self->theme;
	dictionary_addref(self);
	return S_OK;
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

/* One store's content lookup: the key a compiled document names resolves through the same boxed-string comparison the map
 * uses, so a resource key and an entry inserted by the document meet in the middle. */
static HRESULT store_lookup(struct dictionary_store *store, const WCHAR *text, UINT32 length, IInspectable **out)
{
	IInspectable *key;
	struct dictionary_entry *entry;
	HRESULT result = E_BOUNDS;
	if (!out || !text)
		return E_POINTER;
	*out = NULL;
	key = raven_xaml_boxed_string(text, length);
	if (!key)
		return E_OUTOFMEMORY;
	entry = store_find(store, key);
	if (entry && entry->value) {
		entry->value->lpVtbl->AddRef(entry->value);
		*out = entry->value;
		result = S_OK;
	}
	key->lpVtbl->Release(key);
	return result;
}

/* The dictionary a QI handed back is one identity: its map part names the owner and the store that part reads. */
static struct dictionary *dictionary_of(IInspectable *object)
{
	__FIMap_2_IInspectable_IInspectable *map = NULL;
	struct dictionary *self = NULL;
	if (!object)
		return NULL;
	if (FAILED(object->lpVtbl->QueryInterface(object, &IID___FIMap_2_IInspectable_IInspectable, (void **)&map)) || !map)
		return NULL;
	self = ((struct map_part *)map)->owner;
	map->lpVtbl->Release(map);
	return self;
}

#define LOOKUP_DEPTH 16

/* Resource lookup in one dictionary: its own entries first, then its merged dictionaries newest-first, the same precedence
 * the real lookup gives a merged dictionary appended later. Theme dictionaries are a separate stage (see below). */
static HRESULT dictionary_lookup_depth(struct dictionary *self, const WCHAR *text, UINT32 length, IInspectable **out,
	int depth)
{
	HRESULT result = store_lookup(&self->store, text, length, out);
	if (SUCCEEDED(result) || depth >= LOOKUP_DEPTH)
		return result;
	for (UINT32 index = self->merged->count; index; ) {
		struct dictionary *merged = dictionary_of((IInspectable *)self->merged->items[--index]);
		if (!merged)
			continue;
		result = dictionary_lookup_depth(merged, text, length, out, depth + 1);
		if (SUCCEEDED(result))
			return result;
	}
	return E_BOUNDS;
}

HRESULT raven_xaml_dictionary_lookup(IInspectable *object, const WCHAR *text, UINT32 length, IInspectable **out)
{
	struct dictionary *self;
	if (!out)
		return E_POINTER;
	*out = NULL;
	self = dictionary_of(object);
	if (!self)
		return E_NOINTERFACE;
	return dictionary_lookup_depth(self, text, length, out, 0);
}

/* A theme resource adds the ThemeDictionaries stage: the selector the requested theme names ("Dark"/"Light"), then
 * "Default", each one a dictionary the entry is looked up in. The plain stage runs first, as in the real lookup. */
HRESULT raven_xaml_dictionary_lookup_theme(IInspectable *object, const WCHAR *text, UINT32 length,
	const WCHAR *theme, UINT32 theme_length, IInspectable **out)
{
	static const WCHAR fallback[] = L"Default";
	struct dictionary *self;
	IInspectable *selected = NULL;
	HRESULT result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	self = dictionary_of(object);
	if (!self)
		return E_NOINTERFACE;
	result = dictionary_lookup_depth(self, text, length, out, 0);
	if (SUCCEEDED(result))
		return result;
	if (theme && theme_length)
		result = store_lookup(&self->themes, theme, theme_length, &selected);
	if (FAILED(result))
		result = store_lookup(&self->themes, fallback, ARRAYSIZE(fallback) - 1, &selected);
	if (FAILED(result) || !selected)
		return E_BOUNDS;
	result = raven_xaml_dictionary_lookup(selected, text, length, out);
	selected->lpVtbl->Release(selected);
	return result;
}

static HRESULT WINAPI map_lookup(__FIMap_2_IInspectable_IInspectable *iface, IInspectable *key, IInspectable **value)
{
	struct map_part *part = (struct map_part *)iface;
	struct dictionary_entry *entry;
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (!key)
		return E_POINTER;
	entry = store_find(part->store, key);
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
	*value = ((struct map_part *)iface)->store->count;
	return S_OK;
}

static HRESULT WINAPI map_has_key(__FIMap_2_IInspectable_IInspectable *iface, IInspectable *key, boolean *found)
{
	struct map_part *part = (struct map_part *)iface;
	if (!found)
		return E_POINTER;
	*found = FALSE;
	if (!key)
		return E_POINTER;
	*found = store_find(part->store, key) ? TRUE : FALSE;
	return S_OK;
}

static HRESULT WINAPI map_get_view(__FIMap_2_IInspectable_IInspectable *iface,
	__FIMapView_2_IInspectable_IInspectable **view);

static HRESULT WINAPI map_insert(__FIMap_2_IInspectable_IInspectable *iface, IInspectable *key, IInspectable *value,
	boolean *replaced)
{
	struct map_part *part = (struct map_part *)iface;
	struct dictionary_store *store = part->store;
	struct dictionary_entry *entry;
	if (replaced)
		*replaced = FALSE;
	if (!key)
		return E_POINTER;
	entry = store_find(store, key);
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
	if (store->count == store->capacity) {
		const UINT32 capacity = store->capacity ? store->capacity * 2 : 4;
		struct dictionary_entry *entries =
			HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size_t)capacity * sizeof *entries);
		if (!entries)
			return E_OUTOFMEMORY;
		if (store->entries) {
			for (UINT32 index = 0; index < store->count; ++index)
				entries[index] = store->entries[index];
			FREE(store->entries);
		}
		store->entries = entries;
		store->capacity = capacity;
	}
	store->entries[store->count].key = key;
	store->entries[store->count].value = value;
	key->lpVtbl->AddRef(key);
	if (value)
		value->lpVtbl->AddRef(value);
	++store->count;
	return S_OK;
}

static HRESULT WINAPI map_remove(__FIMap_2_IInspectable_IInspectable *iface, IInspectable *key)
{
	struct dictionary_store *store = ((struct map_part *)iface)->store;
	if (!key)
		return E_POINTER;
	for (UINT32 index = 0; index < store->count; ++index) {
		if (!key_matches(store->entries[index].key, key))
			continue;
		store->entries[index].key->lpVtbl->Release(store->entries[index].key);
		if (store->entries[index].value)
			store->entries[index].value->lpVtbl->Release(store->entries[index].value);
		for (UINT32 rest = index; rest + 1 < store->count; ++rest)
			store->entries[rest] = store->entries[rest + 1];
		--store->count;
		return S_OK;
	}
	return E_BOUNDS;
}

static HRESULT WINAPI map_clear(__FIMap_2_IInspectable_IInspectable *iface)
{
	struct dictionary_store *store = ((struct map_part *)iface)->store;
	for (UINT32 index = 0; index < store->count; ++index) {
		store->entries[index].key->lpVtbl->Release(store->entries[index].key);
		if (store->entries[index].value)
			store->entries[index].value->lpVtbl->Release(store->entries[index].value);
	}
	store->count = 0;
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

/* The ThemeDictionaries map: its own COM identity, because it is not the dictionary. QI(IUnknown) on it answers the map,
 * while the dictionary's own IMap part answers the dictionary; the reference count is still the dictionary's, since the
 * storage the map reads lives and dies with it. */
static HRESULT WINAPI theme_query(__FIMap_2_IInspectable_IInspectable *iface, REFIID iid, void **out)
{
	struct dictionary *self = ((struct map_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID___FIMap_2_IInspectable_IInspectable))
		return E_NOINTERFACE;
	dictionary_addref(self);
	*out = iface;
	return S_OK;
}

static const __FIMap_2_IInspectable_IInspectableVtbl theme_table = {
	.QueryInterface = theme_query,
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
	struct dictionary_store *store;
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
	struct dictionary_store *store = ((struct map_view *)iface)->store;
	struct dictionary_entry *entry;
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (!key)
		return E_POINTER;
	entry = store_find(store, key);
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
	*value = ((struct map_view *)iface)->store->count;
	return S_OK;
}

static HRESULT WINAPI view_has_key(__FIMapView_2_IInspectable_IInspectable *iface, IInspectable *key, boolean *found)
{
	struct dictionary_store *store = ((struct map_view *)iface)->store;
	if (!found)
		return E_POINTER;
	*found = FALSE;
	if (!key)
		return E_POINTER;
	*found = store_find(store, key) ? TRUE : FALSE;
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
	struct map_part *part = (struct map_part *)iface;
	struct map_view *instance;
	if (!view)
		return E_POINTER;
	*view = NULL;
	if (!(instance = ALLOC(sizeof *instance)))
		return E_OUTOFMEMORY;
	instance->lpVtbl = &map_view_table;
	instance->owner = part->owner;
	instance->store = part->store;
	instance->refs = 1;
	dictionary_addref(part->owner);
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
	dictionary->map.store = &dictionary->store;
	dictionary->theme.lpVtbl = &theme_table;
	dictionary->theme.owner = dictionary;
	dictionary->theme.store = &dictionary->themes;
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
