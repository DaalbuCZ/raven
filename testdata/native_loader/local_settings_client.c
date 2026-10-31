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
		args->lpVtbl->get_CollectionChanged(args, &kind) != S_OK || kind != CollectionChange_ItemInserted)
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

int raven_entry(void)
{
	static const WCHAR appdata_name[] = L"Windows.Storage.ApplicationData";
	static const WCHAR value_name[] = L"Windows.Foundation.PropertyValue";
	static const WCHAR int_name[] = L"fixture-integer";
	static const WCHAR text_name[] = L"fixture-text";
	static const WCHAR text_value[] = L"hello";
	const WCHAR *mode = GetCommandLineW();
	for (const WCHAR *at = mode; *at; ++at)
		if (*at == L' ') mode = at + 1;
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function initialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function factory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	HSTRING app_key = NULL, value_key = NULL, int_key = NULL, text_key = NULL, text = NULL;
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
		WindowsCreateString(text_name, ARRAYSIZE(text_name) - 1, &text_key) != S_OK) return 3;
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
	container *nested = (void *)1;
	if (settings->lpVtbl->CreateContainer(settings, int_key, ApplicationDataCreateDisposition_Always, &nested) != E_NOTIMPL || nested)
		return 17;
	if (!lstrcmpW(mode, L"write")) {
		__FIObservableMap_2_HSTRING_IInspectable *observable = NULL;
		struct changed_handler handler = {{(void *)&changed_table}, 1, 0, 0};
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
	} else if (!lstrcmpW(mode, L"bad")) {
		IInspectable *item = NULL;
		boolean found = TRUE;
		if (SUCCEEDED(map->lpVtbl->Lookup(map, int_key, &item)) || item ||
			SUCCEEDED(map->lpVtbl->HasKey(map, int_key, &found)) || found) return 20;
		if (map->lpVtbl->Clear(map) != E_NOTIMPL) return 22;
		if (factory(value_key, &IID___x_ABI_CWindows_CFoundation_CIPropertyValueStatics, (void **)&maker) != S_OK || !maker ||
			maker->lpVtbl->CreateInt32(maker, 17, &boxed) != S_OK || !boxed ||
			SUCCEEDED(map->lpVtbl->Insert(map, int_key, boxed, &replaced))) return 21;
		boxed->lpVtbl->Release(boxed); boxed = NULL;
	} else if (!lstrcmpW(mode, L"read")) {
		if (!check_int(map, int_key, -12345) || !check_string(map, text_key)) return 10;
		if (map->lpVtbl->Remove(map, int_key) != S_OK) return 11;
	} else if (!lstrcmpW(mode, L"verify")) {
		IInspectable *missing = (void *)1;
		if (map->lpVtbl->Lookup(map, int_key, &missing) != E_BOUNDS || missing || !check_string(map, text_key)) return 12;
		if (map->lpVtbl->Remove(map, text_key) != S_OK) return 13;
	} else return 14;
	if (text) WindowsDeleteString(text);
	if (maker) maker->lpVtbl->Release(maker);
	map->lpVtbl->Release(map);
	values->lpVtbl->Release(values);
	settings->lpVtbl->Release(settings);
	current->lpVtbl->Release(current);
	statics->lpVtbl->Release(statics);
	WindowsDeleteString(app_key); WindowsDeleteString(value_key); WindowsDeleteString(int_key); WindowsDeleteString(text_key);
	return 0;
}
