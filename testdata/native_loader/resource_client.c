/* The resource-dictionary fixture: activates Windows.UI.Xaml.ResourceDictionary through the runtime catalog, exercises the
 * IMap<Object, Object> surface the compiled markup uses (string keys inserted, looked up, replaced and cleared), the
 * IResourceDictionary members, and Application.Current.Resources storing the dictionary. Freestanding C like the other
 * guests: hand-rolled COM, no CRT beyond the freestanding one the build links. */
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <roapi.h>
#include <activation.h>
#include <windows.foundation.h>
#include "windows_ui_xaml_application.h"
#include "windows_ui_xaml_dictionary.h"
#include "windows_ui_xaml_elements.h"

typedef __x_ABI_CWindows_CUI_CXaml_CIApplication application_i;
typedef __x_ABI_CWindows_CUI_CXaml_CIResourceDictionary resource_i;
typedef __FIMap_2_IInspectable_IInspectable map_i;
typedef __FIMapView_2_IInspectable_IInspectable map_view_i;
typedef __FIVector_1_Windows__CUI__CXaml__CResourceDictionary merged_i;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIGrid grid_i;
typedef __x_ABI_CWindows_CFoundation_CIUriRuntimeClass uri_i;
typedef __x_ABI_CWindows_CFoundation_CIUriRuntimeClassFactory uri_factory_i;

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);
typedef HRESULT (WINAPI *unload_function)(void);

static int same_text(HSTRING value, const WCHAR *expected)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(value, &length);
	return length == lstrlenW(expected) && CompareStringOrdinal(text, (int)length, expected, -1, FALSE) == CSTR_EQUAL;
}

static int same_unknown(void *left, void *right)
{
	IUnknown *one = NULL, *other = NULL;
	const int same = left && right &&
		SUCCEEDED(((IUnknown *)left)->lpVtbl->QueryInterface((IUnknown *)left, &IID_IUnknown, (void **)&one)) && one &&
		SUCCEEDED(((IUnknown *)right)->lpVtbl->QueryInterface((IUnknown *)right, &IID_IUnknown, (void **)&other)) &&
		other && one == other;
	if (one)
		one->lpVtbl->Release(one);
	if (other)
		other->lpVtbl->Release(other);
	return same;
}

/* The IInspectable plumbing the fixture's own objects share; each table needs the slot's exact first parameter, so the helpers
 * are stamped per type. */
#define FIXTURE_IIDS(type, name) \
	static HRESULT WINAPI name(type *iface, ULONG *count, IID **iids) \
	{ (void)iface; if (!count || !iids) return E_POINTER; *count = 0; *iids = NULL; return S_OK; }
#define FIXTURE_TRUST(type, name) \
	static HRESULT WINAPI name(type *iface, TrustLevel *level) \
	{ (void)iface; if (!level) return E_POINTER; *level = BaseTrust; return S_OK; }

/* A boxed string: what a WinRT caller passes as a string resource key. The inspectable part carries the identity, the box
 * part answers IReference<HSTRING>, and QI reaches one from the other as COM identity requires. */
struct fixture_key;
struct fixture_key_box {
	const __FIReference_1_HSTRINGVtbl *lpVtbl;
	struct fixture_key *owner;
};
struct fixture_key {
	const IInspectableVtbl *lpVtbl;
	struct fixture_key_box box;
	LONG refs;
	HSTRING value;
};

static ULONG fixture_key_addref(struct fixture_key *key)
{
	return (ULONG)InterlockedIncrement(&key->refs);
}

static ULONG fixture_key_release(struct fixture_key *key)
{
	const LONG refs = InterlockedDecrement(&key->refs);
	if (!refs) {
		if (key->value)
			WindowsDeleteString(key->value);
		HeapFree(GetProcessHeap(), 0, key);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI key_query(IInspectable *iface, REFIID iid, void **object)
{
	struct fixture_key *key = (struct fixture_key *)iface;
	if (!object)
		return E_POINTER;
	*object = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable)) {
		*object = iface;
	} else if (IsEqualIID(iid, &IID___FIReference_1_HSTRING)) {
		*object = &key->box;
	} else
		return E_NOINTERFACE;
	fixture_key_addref(key);
	return S_OK;
}

static HRESULT WINAPI key_box_query(__FIReference_1_HSTRING *iface, REFIID iid, void **object)
{
	struct fixture_key *key = ((struct fixture_key_box *)iface)->owner;
	if (!object)
		return E_POINTER;
	*object = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable)) {
		*object = key;
	} else if (IsEqualIID(iid, &IID___FIReference_1_HSTRING)) {
		*object = iface;
	} else
		return E_NOINTERFACE;
	fixture_key_addref(key);
	return S_OK;
}

static ULONG WINAPI key_addref(IInspectable *iface) { return fixture_key_addref((struct fixture_key *)iface); }
static ULONG WINAPI key_release(IInspectable *iface) { return fixture_key_release((struct fixture_key *)iface); }
static ULONG WINAPI key_box_addref(__FIReference_1_HSTRING *iface)
{
	return fixture_key_addref(((struct fixture_key_box *)iface)->owner);
}
static ULONG WINAPI key_box_release(__FIReference_1_HSTRING *iface)
{
	return fixture_key_release(((struct fixture_key_box *)iface)->owner);
}

FIXTURE_IIDS(IInspectable, key_iids)
FIXTURE_IIDS(__FIReference_1_HSTRING, key_box_iids)
FIXTURE_TRUST(IInspectable, key_trust)
FIXTURE_TRUST(__FIReference_1_HSTRING, key_box_trust)

static HRESULT WINAPI key_name(IInspectable *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Raven.Fixture.ResourceKey";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, ARRAYSIZE(text) - 1, name);
}

static HRESULT WINAPI key_box_name(__FIReference_1_HSTRING *iface, HSTRING *name)
{
	return key_name((IInspectable *)((struct fixture_key_box *)iface)->owner, name);
}

static HRESULT WINAPI key_box_value(__FIReference_1_HSTRING *iface, HSTRING *value)
{
	struct fixture_key *key = ((struct fixture_key_box *)iface)->owner;
	if (!value)
		return E_POINTER;
	*value = NULL;
	/* the caller deletes what it receives, so the stored string is handed out duplicated */
	return WindowsDuplicateString(key->value, value);
}

static const IInspectableVtbl key_inspectable_table = {
	key_query, key_addref, key_release, key_iids, key_name, key_trust,
};

static const __FIReference_1_HSTRINGVtbl key_box_table = {
	key_box_query, key_box_addref, key_box_release, key_box_iids, key_box_name, key_box_trust,
	key_box_value,
};

static struct fixture_key *make_key(const WCHAR *text)
{
	struct fixture_key *key = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *key);
	if (!key)
		return NULL;
	key->lpVtbl = &key_inspectable_table;
	key->box.lpVtbl = &key_box_table;
	key->box.owner = key;
	key->refs = 1;
	if (WindowsCreateString(text, (UINT32)lstrlenW(text), &key->value) != S_OK) {
		HeapFree(GetProcessHeap(), 0, key);
		return NULL;
	}
	return key;
}

/* A plain value the fixture stores under a key: an IInspectable carrying a number, so the round trip can be checked beyond
 * identity. */
struct fixture_value {
	const IInspectableVtbl *lpVtbl;
	LONG refs;
	int number;
};

static HRESULT WINAPI value_query(IInspectable *iface, REFIID iid, void **object)
{
	if (!object)
		return E_POINTER;
	*object = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable))
		return E_NOINTERFACE;
	InterlockedIncrement(&((struct fixture_value *)iface)->refs);
	*object = iface;
	return S_OK;
}

static ULONG WINAPI value_addref(IInspectable *iface)
{
	return (ULONG)InterlockedIncrement(&((struct fixture_value *)iface)->refs);
}

static ULONG WINAPI value_release(IInspectable *iface)
{
	struct fixture_value *value = (struct fixture_value *)iface;
	const LONG refs = InterlockedDecrement(&value->refs);
	if (!refs)
		HeapFree(GetProcessHeap(), 0, value);
	return (ULONG)refs;
}

FIXTURE_IIDS(IInspectable, value_iids)
FIXTURE_TRUST(IInspectable, value_trust)

static HRESULT WINAPI value_name(IInspectable *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Raven.Fixture.ResourceValue";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, ARRAYSIZE(text) - 1, name);
}

static const IInspectableVtbl value_table = {
	value_query, value_addref, value_release, value_iids, value_name, value_trust,
};

static struct fixture_value *make_value(int number)
{
	struct fixture_value *value = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *value);
	if (!value)
		return NULL;
	value->lpVtbl = &value_table;
	value->refs = 1;
	value->number = number;
	return value;
}

static HRESULT activate(const WCHAR *name, IInspectable **out)
{
	HSTRING key = NULL;
	IActivationFactory *factory = NULL;
	HRESULT result;
	if (WindowsCreateString(name, (UINT32)lstrlenW(name), &key) != S_OK)
		return E_FAIL;
	result = RoGetActivationFactory(key, &IID_IActivationFactory, (void **)&factory);
	WindowsDeleteString(key);
	if (FAILED(result))
		return result;
	result = factory->lpVtbl->ActivateInstance(factory, out);
	factory->lpVtbl->Release(factory);
	return result;
}

/* The Uri a ResourceDictionary.Source takes, through the public factory the way an application creates one. */
static HRESULT make_uri(const WCHAR *text, uri_i **out)
{
	static const WCHAR uri_name[] = L"Windows.Foundation.Uri";
	HSTRING name = NULL, value = NULL;
	uri_factory_i *uris = NULL;
	HRESULT result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	result = WindowsCreateString(uri_name, ARRAYSIZE(uri_name) - 1, &name);
	if (FAILED(result))
		return result;
	result = RoGetActivationFactory(name, &IID___x_ABI_CWindows_CFoundation_CIUriRuntimeClassFactory, (void **)&uris);
	WindowsDeleteString(name);
	if (FAILED(result))
		return result;
	result = WindowsCreateString(text, (UINT32)lstrlenW(text), &value);
	if (SUCCEEDED(result)) {
		result = uris->lpVtbl->CreateUri(uris, value, out);
		WindowsDeleteString(value);
	}
	uris->lpVtbl->Release(uris);
	return result;
}

/* A stored value is the boxed string the loader put there: the IReference<HSTRING> the map's key comparison also reads. */
static int boxed_text_is(IInspectable *value, const WCHAR *expected)
{
	__FIReference_1_HSTRING *box = NULL;
	HSTRING text = NULL;
	int same = value &&
		SUCCEEDED(value->lpVtbl->QueryInterface(value, &IID___FIReference_1_HSTRING, (void **)&box)) && box &&
		SUCCEEDED(box->lpVtbl->get_Value(box, &text)) && text && same_text(text, expected);
	if (text)
		WindowsDeleteString(text);
	if (box)
		box->lpVtbl->Release(box);
	return same;
}

/* One runtime object per check; the codes are unique so a failure names its step. The DllCanUnloadNow check runs after the
 * dictionary's references drop and before the Application activation, because Application.Current retains the application
 * object for the process's lifetime by design. */
int raven_entry(void)
{
	static const WCHAR dictionary_name[] = L"Windows.UI.Xaml.ResourceDictionary";
	static const WCHAR application_name[] = L"Windows.UI.Xaml.Application";
	static const WCHAR grid_name[] = L"Windows.UI.Xaml.Controls.Grid";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function initialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function get_factory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	IInspectable *dictionary = NULL, *grid = NULL;
	resource_i *resource = NULL;
	map_i *map = NULL;
	UINT32 size = 0;
	boolean flag = 0;
	struct fixture_value *number = NULL, *replacement = NULL;
	struct fixture_key *alpha = NULL, *beta = NULL, *gamma = NULL, *probe = NULL, *missing = NULL;
	IInspectable *found = NULL;
	if (!initialize || !get_factory)
		return 1;
	if (initialize(1) != S_OK)
		return 2;
	/* the class activates by name and answers both of its interfaces */
	if (activate(dictionary_name, &dictionary) != S_OK || !dictionary)
		return 3;
	if (dictionary->lpVtbl->QueryInterface(dictionary, &IID__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary,
		(void **)&resource) != S_OK || !resource)
		return 4;
	if (dictionary->lpVtbl->QueryInterface(dictionary, &IID___FIMap_2_IInspectable_IInspectable, (void **)&map) != S_OK ||
		!map)
		return 5;
	if (map->lpVtbl->get_Size(map, &size) != S_OK || size)
		return 6;
	/* the runtime class name is the class, not the interface */
	{
		HSTRING name = NULL;
		if (resource->lpVtbl->GetRuntimeClassName(resource, &name) != S_OK || !same_text(name, dictionary_name))
			return 7;
		WindowsDeleteString(name);
	}
	/* three values under string keys: a number, a string and one of the engine's own objects */
	if (!(number = make_value(42)))
		return 8;
	if (!(alpha = make_key(L"alpha")) || !(beta = make_key(L"beta")) || !(gamma = make_key(L"gamma")))
		return 9;
	if (map->lpVtbl->Insert(map, (IInspectable *)alpha, (IInspectable *)number, &flag) != S_OK || flag)
		return 10;
	if (map->lpVtbl->Insert(map, (IInspectable *)beta, (IInspectable *)beta, &flag) != S_OK || flag)
		return 11;
	if (activate(grid_name, &grid) != S_OK || !grid)
		return 12;
	if (map->lpVtbl->Insert(map, (IInspectable *)gamma, grid, &flag) != S_OK || flag)
		return 13;
	if (map->lpVtbl->get_Size(map, &size) != S_OK || size != 3)
		return 14;
	/* lookup round-trips all three with correct identities; fresh boxes with equal content find them, so keys compare by
	 * string content and not by object identity */
	if (!(probe = make_key(L"alpha")))
		return 15;
	if (map->lpVtbl->Lookup(map, (IInspectable *)probe, &found) != S_OK || !found)
		return 16;
	if (!same_unknown(found, (IInspectable *)number) || ((struct fixture_value *)found)->number != 42)
		return 17;
	found->lpVtbl->Release(found);
	found = NULL;
	fixture_key_release(probe);
	probe = NULL;
	if (!(probe = make_key(L"beta")))
		return 18;
	if (map->lpVtbl->Lookup(map, (IInspectable *)probe, &found) != S_OK || !found)
		return 19;
	if (!same_unknown(found, (IInspectable *)beta))
		return 20;
	found->lpVtbl->Release(found);
	found = NULL;
	fixture_key_release(probe);
	probe = NULL;
	if (!(probe = make_key(L"gamma")))
		return 21;
	if (map->lpVtbl->Lookup(map, (IInspectable *)probe, &found) != S_OK || !found)
		return 22;
	if (!same_unknown(found, grid))
		return 23;
	{
		grid_i *found_grid = NULL;
		if (found->lpVtbl->QueryInterface(found, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid,
			(void **)&found_grid) != S_OK || !found_grid)
			return 24;
		found_grid->lpVtbl->Release(found_grid);
	}
	found->lpVtbl->Release(found);
	found = NULL;
	/* HasKey answers true for a present key and false for an absent one, and Lookup of an absent key fails with E_BOUNDS */
	if (map->lpVtbl->HasKey(map, (IInspectable *)probe, &flag) != S_OK || !flag)
		return 25;
	fixture_key_release(probe);
	probe = NULL;
	if (!(missing = make_key(L"missing")))
		return 26;
	if (map->lpVtbl->HasKey(map, (IInspectable *)missing, &flag) != S_OK || flag)
		return 27;
	if (map->lpVtbl->Lookup(map, (IInspectable *)missing, &found) != E_BOUNDS || found)
		return 28;
	/* an insert over an existing key replaces the value and reports the replacement */
	if (!(replacement = make_value(7)))
		return 29;
	if (map->lpVtbl->Insert(map, (IInspectable *)alpha, (IInspectable *)replacement, &flag) != S_OK || !flag)
		return 30;
	if (map->lpVtbl->get_Size(map, &size) != S_OK || size != 3)
		return 31;
	if (map->lpVtbl->Lookup(map, (IInspectable *)alpha, &found) != S_OK || !found ||
		!same_unknown(found, (IInspectable *)replacement))
		return 32;
	found->lpVtbl->Release(found);
	found = NULL;
	/* GetView hands out a live view over the same storage */
	{
		map_view_i *view = NULL;
		IInspectable *view_found = NULL;
		if (map->lpVtbl->GetView(map, &view) != S_OK || !view)
			return 33;
		if (view->lpVtbl->get_Size(view, &size) != S_OK || size != 3)
			return 34;
		if (view->lpVtbl->Lookup(view, (IInspectable *)alpha, &view_found) != S_OK || !view_found ||
			!same_unknown(view_found, (IInspectable *)replacement))
			return 35;
		view_found->lpVtbl->Release(view_found);
		view->lpVtbl->Release(view);
	}
	/* MergedDictionaries is a vector of dictionaries, empty until the app fills it */
	{
		merged_i *merged = NULL;
		resource_i *appended = NULL, *at_zero = NULL;
		IInspectable *appended_inspectable = NULL;
		if (resource->lpVtbl->get_MergedDictionaries(resource, (void **)&merged) != S_OK || !merged)
			return 36;
		if (merged->lpVtbl->get_Size(merged, &size) != S_OK || size)
			return 37;
		if (activate(dictionary_name, &appended_inspectable) != S_OK || !appended_inspectable)
			return 38;
		if (appended_inspectable->lpVtbl->QueryInterface(appended_inspectable,
			&IID__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary, (void **)&appended) != S_OK || !appended)
			return 39;
		if (merged->lpVtbl->Append(merged, appended) != S_OK)
			return 40;
		if (merged->lpVtbl->get_Size(merged, &size) != S_OK || size != 1)
			return 41;
		if (merged->lpVtbl->GetAt(merged, 0, &at_zero) != S_OK || !at_zero ||
			!same_unknown(at_zero, appended_inspectable))
			return 42;
		at_zero->lpVtbl->Release(at_zero);
		appended->lpVtbl->Release(appended);
		appended_inspectable->lpVtbl->Release(appended_inspectable);
		merged->lpVtbl->Release(merged);
	}
	/* Clear empties the map */
	if (map->lpVtbl->Clear(map) != S_OK)
		return 43;
	if (map->lpVtbl->get_Size(map, &size) != S_OK || size)
		return 44;
	if (map->lpVtbl->HasKey(map, (IInspectable *)alpha, &flag) != S_OK || flag)
		return 45;
	/* ThemeDictionaries is a live map of its own: its entries never reach the dictionary's own map and its view reads the
	 * theme storage */
	{
		map_i *themes = NULL;
		map_view_i *theme_view = NULL;
		IInspectable *theme_value = NULL, *theme_found = NULL;
		struct fixture_key *theme_key = make_key(L"Default");
		if (!theme_key)
			return 100;
		if (resource->lpVtbl->get_ThemeDictionaries(resource, (uint64_t)(uintptr_t)&themes) != S_OK || !themes)
			return 101;
		if (themes->lpVtbl->get_Size(themes, &size) != S_OK || size)
			return 102;
		if (same_unknown(themes, dictionary))
			return 103;
		if (activate(dictionary_name, &theme_value) != S_OK || !theme_value)
			return 104;
		if (themes->lpVtbl->Insert(themes, (IInspectable *)theme_key, theme_value, &flag) != S_OK || flag)
			return 105;
		if (themes->lpVtbl->get_Size(themes, &size) != S_OK || size != 1)
			return 106;
		if (themes->lpVtbl->Lookup(themes, (IInspectable *)theme_key, &theme_found) != S_OK || !theme_found ||
			!same_unknown(theme_found, theme_value))
			return 107;
		theme_found->lpVtbl->Release(theme_found);
		theme_found = NULL;
		if (map->lpVtbl->Lookup(map, (IInspectable *)theme_key, &theme_found) != E_BOUNDS || theme_found)
			return 108;
		if (themes->lpVtbl->GetView(themes, &theme_view) != S_OK || !theme_view)
			return 109;
		if (theme_view->lpVtbl->get_Size(theme_view, &size) != S_OK || size != 1)
			return 110;
		theme_view->lpVtbl->Release(theme_view);
		if (themes->lpVtbl->Remove(themes, (IInspectable *)theme_key) != S_OK)
			return 111;
		if (themes->lpVtbl->get_Size(themes, &size) != S_OK || size)
			return 112;
		themes->lpVtbl->Release(themes);
		theme_value->lpVtbl->Release(theme_value);
		fixture_key_release(theme_key);
	}
	/* Source loads a compiled dictionary document through the PRI and adopts its entries; the Uri rides back out unchanged
	 * and a failed load leaves the dictionary as it was */
	{
		uri_i *source_uri = NULL, *missing_uri = NULL, *loop_uri = NULL, *remote_uri = NULL;
		IInspectable *source_value = NULL;
		struct fixture_key *greeting = make_key(L"Greeting");
		struct fixture_key *farewell = make_key(L"Farewell");
		struct fixture_key *accent = make_key(L"Accent");
		struct fixture_key *theme = make_key(L"Default");
		if (!greeting || !farewell || !accent || !theme)
			return 120;
		if (resource->lpVtbl->get_Source(resource, (uint64_t)(uintptr_t)&source_value) != S_OK || source_value)
			return 121;
		if (make_uri(L"ms-appx:///Fixture/Source.xaml", &source_uri) != S_OK || !source_uri)
			return 122;
		if (make_uri(L"ms-appx:///Fixture/Absent.xaml", &missing_uri) != S_OK || !missing_uri ||
			make_uri(L"ms-appx:///Fixture/Loop.xaml", &loop_uri) != S_OK || !loop_uri ||
			make_uri(L"http://example.com/Fixture/Source.xaml", &remote_uri) != S_OK || !remote_uri)
			return 123;
		if (resource->lpVtbl->put_Source(resource, (uint64_t)(uintptr_t)source_uri) != S_OK)
			return 124;
		if (resource->lpVtbl->get_Source(resource, (uint64_t)(uintptr_t)&source_value) != S_OK || !source_value ||
			!same_unknown(source_value, (IInspectable *)source_uri))
			return 125;
		source_value->lpVtbl->Release(source_value);
		source_value = NULL;
		if (map->lpVtbl->get_Size(map, &size) != S_OK || size != 2)
			return 126;
		if (map->lpVtbl->Lookup(map, (IInspectable *)greeting, &found) != S_OK || !found ||
			!boxed_text_is(found, L"Hello from Source"))
			return 127;
		found->lpVtbl->Release(found);
		found = NULL;
		if (map->lpVtbl->Lookup(map, (IInspectable *)farewell, &found) != S_OK || !found ||
			!boxed_text_is(found, L"Goodbye"))
			return 128;
		found->lpVtbl->Release(found);
		found = NULL;
		/* an absent document, a self-referential one and a non-local scheme each fail without touching what loaded */
		if (resource->lpVtbl->put_Source(resource, (uint64_t)(uintptr_t)missing_uri) != E_BOUNDS ||
			resource->lpVtbl->put_Source(resource, (uint64_t)(uintptr_t)loop_uri) != E_INVALIDARG ||
			resource->lpVtbl->put_Source(resource, (uint64_t)(uintptr_t)remote_uri) != E_NOTIMPL)
			return 129;
		if (map->lpVtbl->get_Size(map, &size) != S_OK || size != 2 ||
			resource->lpVtbl->get_Source(resource, (uint64_t)(uintptr_t)&source_value) != S_OK || !source_value ||
			!same_unknown(source_value, (IInspectable *)source_uri))
			return 130;
		source_value->lpVtbl->Release(source_value);
		source_value = NULL;
		/* a document with ThemeDictionaries fills the second map the loader names by property */
		{
			uri_i *theme_uri = NULL;
			map_i *themes = NULL;
			IInspectable *default_dictionary = NULL;
			map_i *default_map = NULL;
			if (make_uri(L"ms-appx:///Fixture/Theme.xaml", &theme_uri) != S_OK || !theme_uri)
				return 131;
			if (resource->lpVtbl->put_Source(resource, (uint64_t)(uintptr_t)theme_uri) != S_OK)
				return 132;
			if (map->lpVtbl->Lookup(map, (IInspectable *)greeting, &found) != E_BOUNDS || found)
				return 133;
			if (resource->lpVtbl->get_ThemeDictionaries(resource, (uint64_t)(uintptr_t)&themes) != S_OK || !themes)
				return 134;
			if (themes->lpVtbl->get_Size(themes, &size) != S_OK || size != 1)
				return 135;
			if (themes->lpVtbl->Lookup(themes, (IInspectable *)theme, &default_dictionary) != S_OK || !default_dictionary)
				return 136;
			if (default_dictionary->lpVtbl->QueryInterface(default_dictionary,
				&IID___FIMap_2_IInspectable_IInspectable, (void **)&default_map) != S_OK || !default_map)
				return 137;
			if (default_map->lpVtbl->Lookup(default_map, (IInspectable *)accent, &found) != S_OK || !found ||
				!boxed_text_is(found, L"red"))
				return 138;
			found->lpVtbl->Release(found);
			found = NULL;
			default_map->lpVtbl->Release(default_map);
			default_dictionary->lpVtbl->Release(default_dictionary);
			themes->lpVtbl->Release(themes);
			theme_uri->lpVtbl->Release(theme_uri);
		}
		source_uri->lpVtbl->Release(source_uri);
		missing_uri->lpVtbl->Release(missing_uri);
		loop_uri->lpVtbl->Release(loop_uri);
		remote_uri->lpVtbl->Release(remote_uri);
		fixture_key_release(greeting);
		fixture_key_release(farewell);
		fixture_key_release(accent);
		fixture_key_release(theme);
	}
	/* release everything and check the component unloads */
	map->lpVtbl->Release(map);
	resource->lpVtbl->Release(resource);
	dictionary->lpVtbl->Release(dictionary);
	grid->lpVtbl->Release(grid);
	number->lpVtbl->Release((IInspectable *)number);
	replacement->lpVtbl->Release((IInspectable *)replacement);
	fixture_key_release(alpha);
	fixture_key_release(beta);
	fixture_key_release(gamma);
	fixture_key_release(missing);
	{
		unload_function unload = (unload_function)GetProcAddress(GetModuleHandleW(L"raven-xaml.dll"),
			"DllCanUnloadNow");
		if (!unload || unload() != S_OK)
			return 46;
	}
	/* Application.Current.Resources stores the dictionary the application assigns; the fresh application reports no resources
	 * until the first put */
	{
		IInspectable *application = NULL, *assigned = NULL, *stored = NULL;
		application_i *app = NULL;
		map_i *app_map = NULL;
		struct fixture_value *app_value = make_value(5);
		struct fixture_key *app_key = make_key(L"app.key");
		IInspectable *app_found = NULL;
		if (!app_value || !app_key)
			return 47;
		if (activate(application_name, &application) != S_OK || !application)
			return 48;
		if (application->lpVtbl->QueryInterface(application, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication,
			(void **)&app) != S_OK || !app)
			return 49;
		if (app->lpVtbl->get_Resources(app, &stored) != S_OK || stored)
			return 50;
		if (activate(dictionary_name, &assigned) != S_OK || !assigned)
			return 51;
		if (app->lpVtbl->put_Resources(app, assigned) != S_OK)
			return 52;
		if (app->lpVtbl->get_Resources(app, &stored) != S_OK || !stored)
			return 53;
		if (!same_unknown(stored, assigned))
			return 54;
		/* the dictionary reached through Application.Current carries the map surface */
		if (stored->lpVtbl->QueryInterface(stored, &IID___FIMap_2_IInspectable_IInspectable, (void **)&app_map) != S_OK ||
			!app_map)
			return 55;
		if (app_map->lpVtbl->Insert(app_map, (IInspectable *)app_key, (IInspectable *)app_value, &flag) != S_OK || flag)
			return 56;
		if (app_map->lpVtbl->Lookup(app_map, (IInspectable *)app_key, &app_found) != S_OK || !app_found ||
			!same_unknown(app_found, (IInspectable *)app_value))
			return 57;
		app_found->lpVtbl->Release(app_found);
		app_map->lpVtbl->Release(app_map);
		/* my reference from get_Resources holds the dictionary while the property clears */
		stored->lpVtbl->Release(stored);
		stored = NULL;
		if (app->lpVtbl->put_Resources(app, 0) != S_OK)
			return 58;
		if (app->lpVtbl->get_Resources(app, &stored) != S_OK || stored)
			return 59;
		app->lpVtbl->Release(app);
		application->lpVtbl->Release(application);
		assigned->lpVtbl->Release(assigned);
		app_value->lpVtbl->Release((IInspectable *)app_value);
		fixture_key_release(app_key);
	}
	return 0;
}
