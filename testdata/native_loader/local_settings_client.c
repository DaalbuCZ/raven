/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Run in successive guest processes: write boxed values, reopen and remove one, then verify the removal persisted. */
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <windows.foundation.h>
#include <windows.storage.h>
#include <weakreference.h>

#ifndef E_BOUNDS
#define E_BOUNDS ((HRESULT)0x8000000b)
#endif

typedef __x_ABI_CWindows_CStorage_CIApplicationDataStatics appdata_statics;
typedef __x_ABI_CWindows_CStorage_CIApplicationData appdata;
typedef __x_ABI_CWindows_CStorage_CIApplicationDataContainer container;
typedef __x_ABI_CWindows_CFoundation_CCollections_CIPropertySet property_set;
typedef __x_ABI_CWindows_CFoundation_CIPropertyValueStatics value_statics;
typedef __x_ABI_CWindows_CFoundation_CIPropertyValue property_value;
typedef __FIMap_2_HSTRING_IInspectable property_map;
typedef __FIMapView_2_HSTRING_Windows__CStorage__CApplicationDataContainer container_view;
typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);
struct weak_ref;
struct weak_source_vtbl {
	void *query, *addref, *release, *iids, *name, *trust;
	HRESULT (WINAPI *get)(void *, struct weak_ref **);
};
struct weak_ref_vtbl {
	void *query, *addref;
	ULONG (WINAPI *release)(struct weak_ref *);
	void *iids, *name, *trust;
	HRESULT (WINAPI *resolve)(struct weak_ref *, REFIID, IInspectable **);
};
struct weak_source { const struct weak_source_vtbl *lpVtbl; };
struct weak_ref { const struct weak_ref_vtbl *lpVtbl; };
struct changed_handler {
	__FIMapChangedEventHandler_2_HSTRING_IInspectable iface;
	LONG refs;
	unsigned calls, errors;
	__x_ABI_CWindows_CFoundation_CCollections_CCollectionChange expected;
};
static HRESULT WINAPI changed_query(__FIMapChangedEventHandler_2_HSTRING_IInspectable *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID___FIMapChangedEventHandler_2_HSTRING_IInspectable)) return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&((struct changed_handler *)iface)->refs);
	return S_OK;
}
static ULONG WINAPI changed_addref(__FIMapChangedEventHandler_2_HSTRING_IInspectable *iface)
{ return (ULONG)InterlockedIncrement(&((struct changed_handler *)iface)->refs); }
static ULONG WINAPI changed_release(__FIMapChangedEventHandler_2_HSTRING_IInspectable *iface)
{ return (ULONG)InterlockedDecrement(&((struct changed_handler *)iface)->refs); }
static HRESULT WINAPI changed_invoke(__FIMapChangedEventHandler_2_HSTRING_IInspectable *iface,
	__FIObservableMap_2_HSTRING_IInspectable *sender, __FIMapChangedEventArgs_1_HSTRING *args)
{
	struct changed_handler *self = (struct changed_handler *)iface;
	__x_ABI_CWindows_CFoundation_CCollections_CCollectionChange kind;
	IUnknown *identity = NULL;
	if (!sender || args->lpVtbl->QueryInterface(args, &IID_IUnknown, (void **)&identity) != S_OK || !identity ||
		args->lpVtbl->get_CollectionChanged(args, &kind) != S_OK || kind != self->expected)
		++self->errors;
	if (identity) identity->lpVtbl->Release(identity);
	++self->calls;
	return S_OK;
}
static const __FIMapChangedEventHandler_2_HSTRING_IInspectableVtbl changed_table = {
	changed_query, changed_addref, changed_release, changed_invoke
};

static int check_int(property_map *map, HSTRING key, INT32 expected)
{
	IInspectable *item = NULL;
	property_value *value = NULL;
	INT32 result = 0;
	if (map->lpVtbl->Lookup(map, key, &item) != S_OK || !item) return 0;
	if (item->lpVtbl->QueryInterface(item, &IID___x_ABI_CWindows_CFoundation_CIPropertyValue, (void **)&value) != S_OK || !value)
		return 0;
	const HRESULT status = value->lpVtbl->GetInt32(value, &result);
	value->lpVtbl->Release(value);
	item->lpVtbl->Release(item);
	return status == S_OK && result == expected;
}

static int check_string(property_map *map, HSTRING key)
{
	static const WCHAR expected[] = L"hello";
	IInspectable *item = NULL;
	property_value *value = NULL;
	HSTRING result = NULL;
	UINT32 length = 0;
	if (map->lpVtbl->Lookup(map, key, &item) != S_OK || !item) return 0;
	if (item->lpVtbl->QueryInterface(item, &IID___x_ABI_CWindows_CFoundation_CIPropertyValue, (void **)&value) != S_OK || !value)
		return 0;
	const HRESULT status = value->lpVtbl->GetString(value, &result);
	const WCHAR *text = WindowsGetStringRawBuffer(result, &length);
	const int matches = status == S_OK && length == ARRAYSIZE(expected) - 1 &&
		CompareStringOrdinal(text, (int)length, expected, -1, FALSE) == CSTR_EQUAL;
	WindowsDeleteString(result);
	value->lpVtbl->Release(value);
	item->lpVtbl->Release(item);
	return matches;
}

static HRESULT container_map(container *item, property_map **out)
{
	property_set *set = NULL;
	HRESULT status = item->lpVtbl->get_Values(item, &set);
	*out = NULL;
	if (FAILED(status)) return status;
	status = set->lpVtbl->QueryInterface(set, &IID___FIMap_2_HSTRING_IInspectable, (void **)out);
	set->lpVtbl->Release(set);
	return status;
}

static int put_int(property_map *map, value_statics *maker, HSTRING key, INT32 number)
{
	IInspectable *boxed = NULL;
	boolean replaced = TRUE;
	if (maker->lpVtbl->CreateInt32(maker, number, &boxed) != S_OK || !boxed) return 0;
	const HRESULT status = map->lpVtbl->Insert(map, key, boxed, &replaced);
	boxed->lpVtbl->Release(boxed);
	return status == S_OK && !replaced;
}

static int resolves_weak(IInspectable *object, REFIID iid)
{
	struct weak_source *source = NULL;
	struct weak_ref *weak = NULL;
	IInspectable *resolved = NULL;
	IUnknown *expected = NULL, *actual = NULL;
	int result = 0;
	if (object->lpVtbl->QueryInterface(object, &IID_IWeakReferenceSource, (void **)&source) != S_OK || !source ||
		source->lpVtbl->get(source, &weak) != S_OK || !weak || weak->lpVtbl->resolve(weak, iid, &resolved) != S_OK || !resolved ||
		object->lpVtbl->QueryInterface(object, &IID_IUnknown, (void **)&expected) != S_OK || !expected ||
		resolved->lpVtbl->QueryInterface(resolved, &IID_IUnknown, (void **)&actual) != S_OK || !actual)
		goto done;
	result = expected == actual;
done:
	if (actual) actual->lpVtbl->Release(actual);
	if (expected) expected->lpVtbl->Release(expected);
	if (resolved) resolved->lpVtbl->Release(resolved);
	if (weak) weak->lpVtbl->release(weak);
	if (source) ((IUnknown *)source)->lpVtbl->Release((IUnknown *)source);
	return result;
}

static int expires_weak(container *root)
{
	HSTRING name = NULL;
	container *child = NULL;
	property_map *map = NULL;
	struct weak_source *source = NULL;
	struct weak_ref *child_weak = NULL, *values_weak = NULL;
	IInspectable *resolved = NULL;
	int result = 33;
	if (WindowsCreateString(L"weak-release", 12, &name) != S_OK ||
		root->lpVtbl->CreateContainer(root, name, ApplicationDataCreateDisposition_Always, &child) != S_OK || !child ||
		container_map(child, &map) != S_OK || !map ||
		child->lpVtbl->QueryInterface(child, &IID_IWeakReferenceSource, (void **)&source) != S_OK || !source ||
		source->lpVtbl->get(source, &child_weak) != S_OK || !child_weak) goto done;
	((IUnknown *)source)->lpVtbl->Release((IUnknown *)source);
	source = NULL;
	if (map->lpVtbl->QueryInterface(map, &IID_IWeakReferenceSource, (void **)&source) != S_OK || !source ||
		source->lpVtbl->get(source, &values_weak) != S_OK || !values_weak) goto done;
	((IUnknown *)source)->lpVtbl->Release((IUnknown *)source);
	source = NULL;
	map->lpVtbl->Release(map);
	map = NULL;
	child->lpVtbl->Release(child);
	child = NULL;
	if (child_weak->lpVtbl->resolve(child_weak, &IID___x_ABI_CWindows_CStorage_CIApplicationDataContainer, &resolved) != S_OK || resolved ||
		values_weak->lpVtbl->resolve(values_weak, &IID___x_ABI_CWindows_CFoundation_CCollections_CIPropertySet, &resolved) != S_OK || resolved ||
		root->lpVtbl->DeleteContainer(root, name) != S_OK) goto done;
	result = 0;
done:
	if (resolved) resolved->lpVtbl->Release(resolved);
	if (values_weak) values_weak->lpVtbl->release(values_weak);
	if (child_weak) child_weak->lpVtbl->release(child_weak);
	if (source) ((IUnknown *)source)->lpVtbl->Release((IUnknown *)source);
	if (map) map->lpVtbl->Release(map);
	if (child) child->lpVtbl->Release(child);
	WindowsDeleteString(name);
	return result;
}

static int nested_depth(container *root)
{
	HSTRING name = NULL;
	container *current = root, *next = NULL;
	int result = 31;
	if (WindowsCreateString(L"depth", 5, &name) != S_OK) return result;
	current->lpVtbl->AddRef(current);
	for (int level = 0; level < 32; ++level) {
		result = 40 + level;
		if (current->lpVtbl->CreateContainer(current, name, ApplicationDataCreateDisposition_Always, &next) != S_OK || !next)
			goto done;
		current->lpVtbl->Release(current);
		current = next;
		next = NULL;
	}
	result = 80;
	if (SUCCEEDED(current->lpVtbl->CreateContainer(current, name, ApplicationDataCreateDisposition_Always, &next)) || next)
		goto done;
	current->lpVtbl->Release(current);
	current = NULL;
	result = 81;
	if (root->lpVtbl->DeleteContainer(root, name) != S_OK) goto done;
	result = 0;
done:
	if (next) next->lpVtbl->Release(next);
	if (current) current->lpVtbl->Release(current);
	WindowsDeleteString(name);
	return result;
}

/* A deleted container must not acquire access to a new container with the same name through an old reference. */
static int nested_aba(container *root, HSTRING key, HSTRING value_key, factory_function factory)
{
	HSTRING name = NULL;
	container *old = NULL, *fresh = NULL, *fresh_child = NULL;
	container_view *old_view = NULL;
	property_map *old_map = NULL, *fresh_map = NULL;
	value_statics *maker = NULL;
	IInspectable *item = NULL, *boxed = NULL;
	boolean replaced = FALSE;
	int result = 32;
	if (WindowsCreateString(L"recreated", 9, &name) != S_OK ||
		factory(value_key, &IID___x_ABI_CWindows_CFoundation_CIPropertyValueStatics, (void **)&maker) != S_OK || !maker ||
		root->lpVtbl->CreateContainer(root, name, ApplicationDataCreateDisposition_Always, &old) != S_OK || !old ||
		container_map(old, &old_map) != S_OK || !old_map || old->lpVtbl->get_Containers(old, &old_view) != S_OK || !old_view ||
		!put_int(old_map, maker, key, 1) ||
		root->lpVtbl->DeleteContainer(root, name) != S_OK ||
		root->lpVtbl->CreateContainer(root, name, ApplicationDataCreateDisposition_Always, &fresh) != S_OK || !fresh ||
		container_map(fresh, &fresh_map) != S_OK || !fresh_map || !put_int(fresh_map, maker, key, 2) ||
		fresh->lpVtbl->CreateContainer(fresh, name, ApplicationDataCreateDisposition_Always, &fresh_child) != S_OK || !fresh_child)
		goto done;
	boolean found = FALSE;
	if (SUCCEEDED(old_view->lpVtbl->HasKey(old_view, name, &found)) && found) goto done;
	if (SUCCEEDED(old_map->lpVtbl->Lookup(old_map, key, &item)) || item ||
		maker->lpVtbl->CreateInt32(maker, 3, &boxed) != S_OK || !boxed ||
		SUCCEEDED(old_map->lpVtbl->Insert(old_map, key, boxed, &replaced)) || !check_int(fresh_map, key, 2)) goto done;
	if (root->lpVtbl->DeleteContainer(root, name) != S_OK) goto done;
	result = 0;
done:
	if (boxed) boxed->lpVtbl->Release(boxed);
	if (item) item->lpVtbl->Release(item);
	if (fresh_map) fresh_map->lpVtbl->Release(fresh_map);
	if (old_map) old_map->lpVtbl->Release(old_map);
	if (old_view) old_view->lpVtbl->Release(old_view);
	if (fresh_child) fresh_child->lpVtbl->Release(fresh_child);
	if (fresh) fresh->lpVtbl->Release(fresh);
	if (old) old->lpVtbl->Release(old);
	if (maker) maker->lpVtbl->Release(maker);
	WindowsDeleteString(name);
	return result;
}

/* Four independent invocations prove that the hierarchy and values survive process exit. */
static int nested_case(const WCHAR *mode, container *root, property_map *root_map,
	factory_function factory, HSTRING value_key, HSTRING key)
{
	HSTRING first = NULL, sibling_name = NULL, grand_name = NULL, actual_name = NULL;
	container *child = NULL, *sibling = NULL, *grand = NULL, *view_child = NULL;
	container_view *view = NULL;
	property_map *child_map = NULL, *sibling_map = NULL, *grand_map = NULL;
	value_statics *maker = NULL;
	boolean found = FALSE;
	UINT32 size = 0, length = 0;
	int status = 30;
	if (WindowsCreateString(L"fixture-child", 13, &first) != S_OK ||
		WindowsCreateString(L"\x010d\x00ed" L"slo", 5, &sibling_name) != S_OK ||
		WindowsCreateString(L"fixture-grandchild", 18, &grand_name) != S_OK) goto done;
	const int write = !lstrcmpW(mode, L"nested-write");
	const int read = !lstrcmpW(mode, L"nested-read");
	const int verify = !lstrcmpW(mode, L"nested-verify");
	const int guard = !lstrcmpW(mode, L"nested-guard");
	const int collision = !lstrcmpW(mode, L"nested-collision");
	const __x_ABI_CWindows_CStorage_CApplicationDataCreateDisposition disposition = write ?
		ApplicationDataCreateDisposition_Always : ApplicationDataCreateDisposition_Existing;
	if (!write && !read && !verify && !guard && !collision) goto done;
	if (guard || collision) {
		/* Refuse damaged identities or unsafe deletion before changing any setting. */
		if (root->lpVtbl->CreateContainer(root, sibling_name, disposition, &sibling) != S_OK || !sibling ||
			container_map(sibling, &sibling_map) != S_OK || !sibling_map ||
			!check_int(root_map, key, 10) || !check_int(sibling_map, key, 30) ||
			(collision ? SUCCEEDED(root->lpVtbl->CreateContainer(root, first, ApplicationDataCreateDisposition_Always, &child)) :
				SUCCEEDED(root->lpVtbl->DeleteContainer(root, first))) || child ||
			!check_int(root_map, key, 10) || !check_int(sibling_map, key, 30)) goto done;
		status = 0;
		goto done;
	}
	if (verify) {
		if (SUCCEEDED(root->lpVtbl->CreateContainer(root, first, ApplicationDataCreateDisposition_Existing, &child)) || child) goto done;
	} else {
		if (root->lpVtbl->CreateContainer(root, first, disposition, &child) != S_OK || !child ||
			child->lpVtbl->CreateContainer(child, grand_name, disposition, &grand) != S_OK || !grand ||
			container_map(child, &child_map) != S_OK || !child_map ||
			container_map(grand, &grand_map) != S_OK || !grand_map) goto done;
		if (!resolves_weak((IInspectable *)child, &IID___x_ABI_CWindows_CStorage_CIApplicationDataContainer) ||
			!resolves_weak((IInspectable *)child_map, &IID___x_ABI_CWindows_CFoundation_CCollections_CIPropertySet)) goto done;
		if (child->lpVtbl->get_Name(child, &actual_name) != S_OK || !actual_name) goto done;
		const WCHAR *actual = WindowsGetStringRawBuffer(actual_name, &length);
		if (length != 13 || CompareStringOrdinal(actual, length, L"fixture-child", -1, FALSE) != CSTR_EQUAL) goto done;
	}
	if (root->lpVtbl->CreateContainer(root, sibling_name, disposition, &sibling) != S_OK || !sibling ||
		container_map(sibling, &sibling_map) != S_OK || !sibling_map) goto done;
	if (root->lpVtbl->get_Containers(root, &view) != S_OK || !view ||
		view->lpVtbl->get_Size(view, &size) != S_OK || size != (verify ? 1U : 2U) ||
		view->lpVtbl->HasKey(view, first, &found) != S_OK || found != (boolean)!verify) goto done;
	if (!verify) {
		if (view->lpVtbl->Lookup(view, first, &view_child) != S_OK || !view_child) goto done;
		property_map *from_view = NULL;
		if (container_map(view_child, &from_view) != S_OK || !from_view) goto done;
		if (!write && !check_int(from_view, key, 20)) { from_view->lpVtbl->Release(from_view); goto done; }
		from_view->lpVtbl->Release(from_view);
	}
	if (write) {
		__FIObservableMap_2_HSTRING_IInspectable *root_events = NULL, *child_events = NULL;
		struct changed_handler root_handler = {{(void *)&changed_table}, 1, 0, 0, CollectionChange_ItemInserted};
		struct changed_handler child_handler = {{(void *)&changed_table}, 1, 0, 0, CollectionChange_ItemInserted};
		EventRegistrationToken root_token = {0}, child_token = {0};
		int ok = root_map->lpVtbl->QueryInterface(root_map, &IID___FIObservableMap_2_HSTRING_IInspectable, (void **)&root_events) == S_OK && root_events &&
			child_map->lpVtbl->QueryInterface(child_map, &IID___FIObservableMap_2_HSTRING_IInspectable, (void **)&child_events) == S_OK && child_events &&
			root_events->lpVtbl->add_MapChanged(root_events, &root_handler.iface, &root_token) == S_OK &&
			child_events->lpVtbl->add_MapChanged(child_events, &child_handler.iface, &child_token) == S_OK &&
			factory(value_key, &IID___x_ABI_CWindows_CFoundation_CIPropertyValueStatics, (void **)&maker) == S_OK && maker &&
			put_int(root_map, maker, key, 10) && put_int(child_map, maker, key, 20) &&
			put_int(sibling_map, maker, key, 30) && put_int(grand_map, maker, key, 40) &&
			root_handler.calls == 1 && child_handler.calls == 1 && !root_handler.errors && !child_handler.errors;
		if (child_token.value && child_events->lpVtbl->remove_MapChanged(child_events, child_token) != S_OK) ok = 0;
		if (root_token.value && root_events->lpVtbl->remove_MapChanged(root_events, root_token) != S_OK) ok = 0;
		if (root_handler.refs != 1 || child_handler.refs != 1) ok = 0;
		if (child_events) child_events->lpVtbl->Release(child_events);
		if (root_events) root_events->lpVtbl->Release(root_events);
		if (!ok) goto done;
	} else if (!check_int(root_map, key, 10) || !check_int(sibling_map, key, 30) ||
		(!verify && (!check_int(child_map, key, 20) || !check_int(grand_map, key, 40)))) goto done;
	if (read) {
		IInspectable *stale = NULL;
		if (root->lpVtbl->DeleteContainer(root, first) != S_OK ||
			SUCCEEDED(child_map->lpVtbl->Lookup(child_map, key, &stale)) || stale ||
			SUCCEEDED(grand_map->lpVtbl->Lookup(grand_map, key, &stale)) || stale) goto done;
	}
	if (verify && (root->lpVtbl->DeleteContainer(root, sibling_name) != S_OK || root_map->lpVtbl->Remove(root_map, key) != S_OK)) goto done;
	status = 0;
done:
	if (maker) maker->lpVtbl->Release(maker);
	if (grand_map) grand_map->lpVtbl->Release(grand_map);
	if (sibling_map) sibling_map->lpVtbl->Release(sibling_map);
	if (child_map) child_map->lpVtbl->Release(child_map);
	if (view_child) view_child->lpVtbl->Release(view_child);
	if (view) view->lpVtbl->Release(view);
	if (grand) grand->lpVtbl->Release(grand);
	if (sibling) sibling->lpVtbl->Release(sibling);
	if (child) child->lpVtbl->Release(child);
	WindowsDeleteString(actual_name);
	WindowsDeleteString(first); WindowsDeleteString(sibling_name); WindowsDeleteString(grand_name);
	return status;
}

int raven_entry(void)
{
	static const WCHAR appdata_name[] = L"Windows.Storage.ApplicationData";
	static const WCHAR value_name[] = L"Windows.Foundation.PropertyValue";
	static const WCHAR int_name[] = L"fixture-integer";
	static const WCHAR text_name[] = L"fixture-text";
	static const WCHAR null_name[] = L"fixture-null";
	static const WCHAR text_value[] = L"hello";
	const WCHAR *mode = GetCommandLineW();
	for (const WCHAR *at = mode; *at; ++at)
		if (*at == L' ') mode = at + 1;
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function initialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function factory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	HSTRING app_key = NULL, value_key = NULL, int_key = NULL, text_key = NULL, null_key = NULL, text = NULL;
	appdata_statics *statics = NULL;
	appdata *current = NULL;
	container *settings = NULL;
	property_set *values = NULL;
	property_map *map = NULL;
	value_statics *maker = NULL;
	IInspectable *boxed = NULL;
	boolean replaced = FALSE;
	if (!initialize || !factory || FAILED(initialize(1))) return 2;
	if (WindowsCreateString(appdata_name, ARRAYSIZE(appdata_name) - 1, &app_key) != S_OK ||
		WindowsCreateString(value_name, ARRAYSIZE(value_name) - 1, &value_key) != S_OK ||
		WindowsCreateString(int_name, ARRAYSIZE(int_name) - 1, &int_key) != S_OK ||
		WindowsCreateString(text_name, ARRAYSIZE(text_name) - 1, &text_key) != S_OK ||
		WindowsCreateString(null_name, ARRAYSIZE(null_name) - 1, &null_key) != S_OK) return 3;
	if (factory(app_key, &IID___x_ABI_CWindows_CStorage_CIApplicationDataStatics, (void **)&statics) != S_OK || !statics ||
		statics->lpVtbl->get_Current(statics, &current) != S_OK || !current ||
		current->lpVtbl->get_LocalSettings(current, &settings) != S_OK || !settings ||
		settings->lpVtbl->get_Values(settings, &values) != S_OK || !values ||
		values->lpVtbl->QueryInterface(values, &IID___FIMap_2_HSTRING_IInspectable, (void **)&map) != S_OK || !map) return 4;
	/* The container and the values collection are distinct COM objects; weak resolution preserves each identity. */
	IUnknown *container_id = NULL, *values_id = NULL;
	if (settings->lpVtbl->QueryInterface(settings, &IID_IUnknown, (void **)&container_id) != S_OK ||
		values->lpVtbl->QueryInterface(values, &IID_IUnknown, (void **)&values_id) != S_OK ||
		!container_id || !values_id || container_id == values_id) return 15;
	container_id->lpVtbl->Release(container_id);
	values_id->lpVtbl->Release(values_id);
	struct weak_source *source = NULL;
	struct weak_ref *weak = NULL;
	IInspectable *resolved = NULL;
	if (values->lpVtbl->QueryInterface(values, &IID_IWeakReferenceSource, (void **)&source) != S_OK || !source ||
		source->lpVtbl->get(source, &weak) != S_OK || !weak ||
		weak->lpVtbl->resolve(weak, &IID___x_ABI_CWindows_CFoundation_CCollections_CIPropertySet, &resolved) != S_OK ||
		(void *)resolved != (void *)values) return 16;
	resolved->lpVtbl->Release(resolved);
	weak->lpVtbl->release(weak);
	((IUnknown *)source)->lpVtbl->Release((IUnknown *)source);
	if (!lstrcmpW(mode, L"nested-weak")) {
		const int result = expires_weak(settings);
		if (result) return result;
	} else if (!lstrcmpW(mode, L"nested-aba")) {
		const int result = nested_aba(settings, int_key, value_key, factory);
		if (result) return result;
	} else if (!lstrcmpW(mode, L"nested-depth")) {
		const int result = nested_depth(settings);
		if (result) return result;
	} else if (!lstrcmpW(mode, L"nested-write") || !lstrcmpW(mode, L"nested-read") ||
		!lstrcmpW(mode, L"nested-verify") || !lstrcmpW(mode, L"nested-guard") ||
		!lstrcmpW(mode, L"nested-collision")) {
		const int result = nested_case(mode, settings, map, factory, value_key, int_key);
		if (result) return result;
	} else if (!lstrcmpW(mode, L"write")) {
		__FIObservableMap_2_HSTRING_IInspectable *observable = NULL;
		struct changed_handler handler = {{(void *)&changed_table}, 1, 0, 0, CollectionChange_ItemInserted};
		EventRegistrationToken token;
		if (values->lpVtbl->QueryInterface(values, &IID___FIObservableMap_2_HSTRING_IInspectable, (void **)&observable) != S_OK ||
			!observable || observable->lpVtbl->add_MapChanged(observable, &handler.iface, &token) != S_OK) return 18;
		if (factory(value_key, &IID___x_ABI_CWindows_CFoundation_CIPropertyValueStatics, (void **)&maker) != S_OK || !maker) return 5;
		if (maker->lpVtbl->CreateInt32(maker, -12345, &boxed) != S_OK || !boxed ||
			map->lpVtbl->Insert(map, int_key, boxed, &replaced) != S_OK || replaced) return 6;
		boxed->lpVtbl->Release(boxed); boxed = NULL;
		if (WindowsCreateString(text_value, ARRAYSIZE(text_value) - 1, &text) != S_OK ||
			maker->lpVtbl->CreateString(maker, text, &boxed) != S_OK || !boxed ||
			map->lpVtbl->Insert(map, text_key, boxed, &replaced) != S_OK || replaced) return 7;
		boxed->lpVtbl->Release(boxed); boxed = NULL;
		/* A WinRT object which is not a supported setting must not replace an existing value. */
		if (map->lpVtbl->Insert(map, int_key, (IInspectable *)current, &replaced) != E_INVALIDARG ||
			!check_int(map, int_key, -12345)) return 8;
		if (!check_string(map, text_key)) return 9;
		if (handler.calls != 2 || handler.errors || observable->lpVtbl->remove_MapChanged(observable, token) != S_OK || handler.refs != 1)
			return 19;
		observable->lpVtbl->Release(observable);
		if (map->lpVtbl->Insert(map, null_key, NULL, &replaced) != S_OK || replaced) return 25;
	} else if (!lstrcmpW(mode, L"bad")) {
		IInspectable *item = NULL;
		boolean found = TRUE;
		if (SUCCEEDED(map->lpVtbl->Lookup(map, int_key, &item)) || item ||
			SUCCEEDED(map->lpVtbl->HasKey(map, int_key, &found)) || found) return 20;
		if (SUCCEEDED(map->lpVtbl->Clear(map))) return 22;
		if (factory(value_key, &IID___x_ABI_CWindows_CFoundation_CIPropertyValueStatics, (void **)&maker) != S_OK || !maker ||
			maker->lpVtbl->CreateInt32(maker, 17, &boxed) != S_OK || !boxed ||
			SUCCEEDED(map->lpVtbl->Insert(map, int_key, boxed, &replaced))) return 21;
		boxed->lpVtbl->Release(boxed); boxed = NULL;
	} else if (!lstrcmpW(mode, L"clear")) {
		UINT32 size = 99;
		__FIObservableMap_2_HSTRING_IInspectable *observable = NULL;
		struct changed_handler handler = {{(void *)&changed_table}, 1, 0, 0, CollectionChange_Reset};
		EventRegistrationToken token;
		if (values->lpVtbl->QueryInterface(values, &IID___FIObservableMap_2_HSTRING_IInspectable, (void **)&observable) != S_OK ||
			observable->lpVtbl->add_MapChanged(observable, &handler.iface, &token) != S_OK ||
			map->lpVtbl->Clear(map) != S_OK || map->lpVtbl->get_Size(map, &size) != S_OK || size ||
			handler.calls != 1 || handler.errors || observable->lpVtbl->remove_MapChanged(observable, token) != S_OK ||
			handler.refs != 1) return 23;
		observable->lpVtbl->Release(observable);
	} else if (!lstrcmpW(mode, L"empty")) {
		UINT32 size = 99;
		if (map->lpVtbl->get_Size(map, &size) != S_OK || size || map->lpVtbl->Clear(map) != S_OK) return 24;
	} else if (!lstrcmpW(mode, L"read")) {
		IInspectable *null_value = (void *)1;
		boolean found = FALSE;
		if (map->lpVtbl->HasKey(map, null_key, &found) != S_OK || !found ||
			map->lpVtbl->Lookup(map, null_key, &null_value) != S_OK || null_value ||
			map->lpVtbl->Insert(map, null_key, NULL, &replaced) != S_OK || !replaced ||
			map->lpVtbl->Remove(map, null_key) != S_OK) return 26;
		if (!check_int(map, int_key, -12345) || !check_string(map, text_key)) return 10;
		if (map->lpVtbl->Remove(map, int_key) != S_OK) return 11;
	} else if (!lstrcmpW(mode, L"verify")) {
		IInspectable *missing = (void *)1;
		boolean found = TRUE;
		if (map->lpVtbl->Lookup(map, int_key, &missing) != S_OK || missing ||
			map->lpVtbl->HasKey(map, int_key, &found) != S_OK || found || !check_string(map, text_key)) return 12;
		if (map->lpVtbl->Remove(map, text_key) != S_OK) return 13;
	} else return 14;
	if (text) WindowsDeleteString(text);
	if (maker) maker->lpVtbl->Release(maker);
	map->lpVtbl->Release(map);
	values->lpVtbl->Release(values);
	settings->lpVtbl->Release(settings);
	current->lpVtbl->Release(current);
	statics->lpVtbl->Release(statics);
	WindowsDeleteString(app_key); WindowsDeleteString(value_key); WindowsDeleteString(int_key); WindowsDeleteString(text_key); WindowsDeleteString(null_key);
	return 0;
}
