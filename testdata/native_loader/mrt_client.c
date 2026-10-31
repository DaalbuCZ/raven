/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The ResourceManager fixture: the build lays the index at <exe-dir>\<exe-stem>\resources.pri, the fixture copies it to
   <exe-dir>\resources.pri with its greeting patched, so the value the manager answers proves which of the two locations
   the documented resolution picked. */
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <eventtoken.h>
#include <weakreference.h>
#include "windows_applicationmodel_resources.h"
#include "windows_applicationmodel_resources_core.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);

typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics manager_statics_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager manager_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2 manager2_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap map_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource named_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate candidate_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3 candidate3_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 context_statics2_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext context_i;

/* The parameterized candidate view, declared after the same derivation the component uses. */
typedef struct fixture_vector_view fixture_vector_view;
typedef struct fixture_vector_view_vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(fixture_vector_view *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(fixture_vector_view *This);
	ULONG (STDMETHODCALLTYPE *Release)(fixture_vector_view *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(fixture_vector_view *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(fixture_vector_view *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(fixture_vector_view *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetAt)(fixture_vector_view *This, UINT32 index, candidate_i **value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(fixture_vector_view *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *IndexOf)(fixture_vector_view *This, candidate_i *element, UINT32 *index, boolean *found);
	HRESULT (STDMETHODCALLTYPE *GetMany)(fixture_vector_view *This, UINT32 start_index, UINT32 items_size,
		candidate_i **items, UINT32 *value);
	END_INTERFACE
} fixture_vector_view_vtbl;

struct fixture_vector_view {
	CONST_VTBL fixture_vector_view_vtbl *lpVtbl;
};

/* IVectorView`1<ResourceCandidate>, the SHA-1 of the interface signature over the parameterized-interface namespace. */
DEFINE_GUID(fixture_view_iid, 0xe28e92f0, 0x9ffb, 0x5ea7, 0x9f, 0xc9, 0xa7, 0x3b, 0xda, 0x47, 0x18, 0x86);

/* IVectorView`1<ResourceQualifier>, the same derivation for the candidate's qualifier view. */
typedef struct fixture_qualifier_view fixture_qualifier_view;
typedef struct fixture_qualifier_view_vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(fixture_qualifier_view *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(fixture_qualifier_view *This);
	ULONG (STDMETHODCALLTYPE *Release)(fixture_qualifier_view *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(fixture_qualifier_view *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(fixture_qualifier_view *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(fixture_qualifier_view *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetAt)(fixture_qualifier_view *This, UINT32 index, void **value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(fixture_qualifier_view *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *IndexOf)(fixture_qualifier_view *This, void *element, UINT32 *index, boolean *found);
	HRESULT (STDMETHODCALLTYPE *GetMany)(fixture_qualifier_view *This, UINT32 start_index, UINT32 items_size,
		void **items, UINT32 *value);
	END_INTERFACE
} fixture_qualifier_view_vtbl;

struct fixture_qualifier_view {
	CONST_VTBL fixture_qualifier_view_vtbl *lpVtbl;
};

DEFINE_GUID(fixture_qualifier_view_iid, 0xb82c58dc, 0x1cc0, 0x53f0, 0xb0, 0xf4, 0x66, 0xef, 0x39, 0xa8, 0x1c, 0xd6);

/* IMap`2<HSTRING,HSTRING>, the map behind QualifierValues. */
typedef struct fixture_map fixture_map;
typedef struct fixture_map_vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(fixture_map *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(fixture_map *This);
	ULONG (STDMETHODCALLTYPE *Release)(fixture_map *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(fixture_map *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(fixture_map *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(fixture_map *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *Lookup)(fixture_map *This, HSTRING key, HSTRING *value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(fixture_map *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *HasKey)(fixture_map *This, HSTRING key, boolean *found);
	HRESULT (STDMETHODCALLTYPE *GetView)(fixture_map *This, void **view);
	HRESULT (STDMETHODCALLTYPE *Insert)(fixture_map *This, HSTRING key, HSTRING value, boolean *replaced);
	HRESULT (STDMETHODCALLTYPE *Remove)(fixture_map *This, HSTRING key);
	HRESULT (STDMETHODCALLTYPE *Clear)(fixture_map *This);
	END_INTERFACE
} fixture_map_vtbl;

struct fixture_map {
	CONST_VTBL fixture_map_vtbl *lpVtbl;
};

DEFINE_GUID(fixture_map_iid, 0xf6d1f700, 0x49c2, 0x52ae, 0x81, 0x54, 0x82, 0x6f, 0x99, 0x08, 0x77, 0x3c);

/* IObservableMap<HSTRING,HSTRING>: the QualifierValues property's declared type, carrying the two event methods over
   IInspectable; the map methods are reached through QueryInterface for IMap`2. */
typedef struct fixture_observable_map fixture_observable_map;
typedef struct fixture_observable_map_vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(fixture_observable_map *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(fixture_observable_map *This);
	ULONG (STDMETHODCALLTYPE *Release)(fixture_observable_map *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(fixture_observable_map *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(fixture_observable_map *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(fixture_observable_map *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *add_MapChanged)(fixture_observable_map *This, void *handler, EventRegistrationToken *token);
	HRESULT (STDMETHODCALLTYPE *remove_MapChanged)(fixture_observable_map *This, EventRegistrationToken token);
	END_INTERFACE
} fixture_observable_map_vtbl;

struct fixture_observable_map {
	CONST_VTBL fixture_observable_map_vtbl *lpVtbl;
};

DEFINE_GUID(fixture_observable_map_iid, 0x1e036276, 0x2f60, 0x55f6, 0xb7, 0xf3, 0xf8, 0x60, 0x79, 0xe6, 0x90, 0x0b);

/* IVectorView`1<String>, the Languages property's declared type. */
typedef struct fixture_vector_string fixture_vector_string;
typedef struct fixture_vector_string_vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(fixture_vector_string *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(fixture_vector_string *This);
	ULONG (STDMETHODCALLTYPE *Release)(fixture_vector_string *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(fixture_vector_string *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(fixture_vector_string *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(fixture_vector_string *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetAt)(fixture_vector_string *This, UINT32 index, HSTRING *value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(fixture_vector_string *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *IndexOf)(fixture_vector_string *This, HSTRING element, UINT32 *index, boolean *found);
	HRESULT (STDMETHODCALLTYPE *GetMany)(fixture_vector_string *This, UINT32 start_index, UINT32 items_size,
		HSTRING *items, UINT32 *value);
	END_INTERFACE
} fixture_vector_string_vtbl;

struct fixture_vector_string {
	CONST_VTBL fixture_vector_string_vtbl *lpVtbl;
};

/* The MapChanged handler the fixture subscribes: the component holds it through its AddRef while subscribed. */
typedef struct fixture_handler fixture_handler;
typedef struct fixture_handler_vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(fixture_handler *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(fixture_handler *This);
	ULONG (STDMETHODCALLTYPE *Release)(fixture_handler *This);
	HRESULT (STDMETHODCALLTYPE *Invoke)(fixture_handler *This, void *sender, void *args);
	END_INTERFACE
} fixture_handler_vtbl;

struct fixture_handler {
	CONST_VTBL fixture_handler_vtbl *lpVtbl;
	ULONG refs;
	ULONG invoked;
};

DEFINE_GUID(fixture_handler_iid, 0x6a1a6b3b, 0x0f2e, 0x5b3a, 0x9a, 0x1c, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88);

static HRESULT WINAPI fixture_handler_query(fixture_handler *This, REFIID riid, void **object)
{
	if (!object) return E_POINTER;
	*object = NULL;
	if (!IsEqualIID(riid, &IID_IUnknown) && !IsEqualIID(riid, &IID_IInspectable) && !IsEqualIID(riid, &fixture_handler_iid))
		return E_NOINTERFACE;
	*object = This;
	This->lpVtbl->AddRef(This);
	return S_OK;
}

static ULONG WINAPI fixture_handler_addref(fixture_handler *This) { return InterlockedIncrement((LONG *)&This->refs); }

static ULONG WINAPI fixture_handler_release(fixture_handler *This)
{
	ULONG refs = InterlockedDecrement((LONG *)&This->refs);
	if (!refs) HeapFree(GetProcessHeap(), 0, This);
	return refs;
}

static HRESULT WINAPI fixture_handler_invoke(fixture_handler *This, void *sender, void *args)
{
	(void)sender; (void)args;
	++This->invoked;
	return S_OK;
}

static const fixture_handler_vtbl fixture_handler_vtbl_table = {
	fixture_handler_query, fixture_handler_addref, fixture_handler_release, fixture_handler_invoke};

/* IWeakReferenceSource and IWeakReference with the Windows ABI: mingw's weakreference.h models them IUnknown-only, but the
   interfaces derive IInspectable and the one method of each sits at vtable slot 6. */
typedef struct fixture_weak_ref fixture_weak_ref;
typedef struct fixture_weak_source fixture_weak_source;
typedef struct fixture_weak_source_vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(fixture_weak_source *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(fixture_weak_source *This);
	ULONG (STDMETHODCALLTYPE *Release)(fixture_weak_source *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(fixture_weak_source *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(fixture_weak_source *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(fixture_weak_source *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetWeakReference)(fixture_weak_source *This, fixture_weak_ref **reference);
	END_INTERFACE
} fixture_weak_source_vtbl;

struct fixture_weak_source {
	CONST_VTBL fixture_weak_source_vtbl *lpVtbl;
};

typedef struct fixture_weak_ref_vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(fixture_weak_ref *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(fixture_weak_ref *This);
	ULONG (STDMETHODCALLTYPE *Release)(fixture_weak_ref *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(fixture_weak_ref *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(fixture_weak_ref *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(fixture_weak_ref *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *Resolve)(fixture_weak_ref *This, REFIID iid, IInspectable **object);
	END_INTERFACE
} fixture_weak_ref_vtbl;

struct fixture_weak_ref {
	CONST_VTBL fixture_weak_ref_vtbl *lpVtbl;
};

DEFINE_GUID(fixture_weak_source_iid, 0x00000038, 0x0000, 0x0000, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
DEFINE_GUID(fixture_weak_ref_iid, 0x00000037, 0x0000, 0x0000, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

/* The handler the component sees is a heap object of the handler shape, freed after the unsubscribe below. */
static fixture_handler *fixture_handler_create(void)
{
	fixture_handler *handler = (fixture_handler *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *handler);
	if (!handler)
		return NULL;
	handler->lpVtbl = (fixture_handler_vtbl *)&fixture_handler_vtbl_table;
	handler->refs = 1;
	return handler;
}

#ifndef E_BOUNDS
#define E_BOUNDS ((HRESULT)0x8000000b)
#endif

static int same_text(HSTRING value, const WCHAR *expected)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(value, &length);
	return length == (UINT32)lstrlenW(expected) &&
		CompareStringOrdinal(text, (int)length, expected, -1, FALSE) == CSTR_EQUAL;
}

/* The executable's directory and the directory named after it, the two places the documented resolution looks. */
static BOOL exe_paths(WCHAR *exe_dir /* MAX_PATH */, WCHAR *stem_dir /* MAX_PATH */)
{
	WCHAR full[MAX_PATH];
	DWORD length = GetModuleFileNameW(NULL, full, MAX_PATH);
	UINT32 cut = 0, i, stem;
	if (!length || length >= MAX_PATH)
		return FALSE;
	for (i = 0; i < length; ++i)
		if (full[i] == L'\\' || full[i] == L'/')
			cut = i;
	memcpy(exe_dir, full, cut * sizeof(WCHAR));
	exe_dir[cut] = 0;
	lstrcpyW(stem_dir, exe_dir);
	stem = cut + 1;
	lstrcatW(stem_dir, L"\\");
	if (lstrlenW(exe_dir) + 1 + (UINT32)lstrlenW(full + stem) >= MAX_PATH)
		return FALSE;
	lstrcatW(stem_dir, full + stem);
	/* drop the extension */
	stem = (UINT32)lstrlenW(stem_dir);
	if (stem > 4 && stem_dir[stem - 4] == L'.')
		stem_dir[stem - 4] = 0;
	return stem_dir[0] != 0;
}

static BYTE *read_whole_file(const WCHAR *path, UINT32 *length)
{
	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER size;
	BYTE *buffer;
	DWORD read = 0;
	*length = 0;
	if (file == INVALID_HANDLE_VALUE)
		return NULL;
	if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 1024 * 1024) {
		CloseHandle(file);
		return NULL;
	}
	buffer = (BYTE *)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)size.QuadPart);
	if (!buffer) {
		CloseHandle(file);
		return NULL;
	}
	if (!ReadFile(file, buffer, (DWORD)size.QuadPart, &read, NULL) || read != (DWORD)size.QuadPart) {
		HeapFree(GetProcessHeap(), 0, buffer);
		CloseHandle(file);
		return NULL;
	}
	CloseHandle(file);
	*length = (UINT32)read;
	return buffer;
}

static int write_whole_file(const WCHAR *path, const BYTE *data, UINT32 length)
{
	HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	DWORD written = 0;
	if (file == INVALID_HANDLE_VALUE)
		return 41;
	if (!WriteFile(file, data, length, &written, NULL) || written != length) {
		CloseHandle(file);
		return 42;
	}
	CloseHandle(file);
	return 0;
}

/* The greeting the CMake-copied index carries and its patched form: same byte length, so the copy stays a valid index
   and the value answers which file the manager loaded. */
static const WCHAR greeting_original[] = L"Hello from the PRI";
static const WCHAR greeting_patched[] = L"Hello from the DIR";

static BYTE *patched_copy(const BYTE *data, UINT32 length, UINT32 *out_length)
{
	const BYTE *pattern = (const BYTE *)greeting_original;
	const BYTE *replacement = (const BYTE *)greeting_patched;
	UINT32 pattern_length = (UINT32)lstrlenW(greeting_original) * 2;
	UINT32 i, hits = 0, at = 0;
	BYTE *copy = (BYTE *)HeapAlloc(GetProcessHeap(), 0, length);
	if (!copy)
		return NULL;
	memcpy(copy, data, length);
	for (i = 0; i + pattern_length <= length; ++i) {
		if (!memcmp(copy + i, pattern, pattern_length)) {
			++hits;
			at = i;
		}
	}
	if (hits != 1) {
		HeapFree(GetProcessHeap(), 0, copy);
		return NULL;
	}
	memcpy(copy + at, replacement, pattern_length);
	*out_length = length;
	return copy;
}

int raven_entry(void)
{
	static const WCHAR name[] = L"Windows.ApplicationModel.Resources.Core.ResourceManager";
	static const WCHAR context_name[] = L"Windows.ApplicationModel.Resources.Core.ResourceContext";
	static const WCHAR greeting_name[] = L"Fixture/Greeting";
	static const WCHAR farewell_name[] = L"Fixture/Farewell";
	WCHAR exe_dir[MAX_PATH], stem_dir[MAX_PATH], stem_pri[MAX_PATH], exe_pri[MAX_PATH];
	BYTE *source = NULL, *copy = NULL;
	UINT32 source_length = 0, copy_length = 0;
	int step, patched = 0;
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function RoGetActivationFactory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	HSTRING class_key = NULL, context_key = NULL, greeting_key = NULL, farewell_key = NULL, missing_key = NULL;
	HSTRING subtree_key = NULL, subtree_greeting_key = NULL, text = NULL;
	IActivationFactory *activation = NULL, *activation_again = NULL;
	manager_statics_i *statics = NULL;
	manager_i *manager = NULL, *manager_again = NULL;
	manager2_i *manager2 = NULL;
	map_i *map = NULL, *subtree = NULL;
	named_i *named = NULL;
	candidate_i *candidate = NULL, *view_candidate = NULL;
	candidate3_i *candidate3 = NULL;
	fixture_vector_view *view = NULL;
	fixture_qualifier_view *qualifiers = NULL;
	context_statics2_i *context_statics = NULL;
	context_i *context = NULL, *default_context = NULL;
	IActivationFactory *context_activation = NULL;
	boolean flag = 0;
	UINT32 size = 0;
	int32_t kind = -1;

	/* the index the build copied sits in the directory named after the executable, and carries the unpatched greeting;
	   the layouts without an index (the shared-path mrt suite) exercise the empty-index behavior instead */
	if (!exe_paths(exe_dir, stem_dir)) return 20;
	if ((UINT32)lstrlenW(exe_dir) + 1 + 1 + lstrlenW(L"resources.pri") + 1 >= MAX_PATH ||
		(UINT32)lstrlenW(stem_dir) + 1 + lstrlenW(L"resources.pri") + 1 >= MAX_PATH) return 21;
	lstrcpyW(stem_pri, stem_dir);
	lstrcatW(stem_pri, L"\\resources.pri");
	lstrcpyW(exe_pri, exe_dir);
	lstrcatW(exe_pri, L"\\resources.pri");
	source = read_whole_file(stem_pri, &source_length);
	if (source) {
		UINT32 pattern_length = (UINT32)lstrlenW(greeting_original) * 2;
		UINT32 i, hits = 0;
		for (i = 0; i + pattern_length <= source_length; ++i)
			if (!memcmp(source + i, greeting_original, pattern_length))
				++hits;
		if (hits != 1) { HeapFree(GetProcessHeap(), 0, source); return 23; }
		/* the copy beside the executable carries the patched greeting: whichever index the manager reads decides the
		   value it answers, so the patch makes the resolution's choice observable */
		copy = patched_copy(source, source_length, &copy_length);
		HeapFree(GetProcessHeap(), 0, source);
		if (!copy) return 24;
		step = write_whole_file(exe_pri, copy, copy_length);
		HeapFree(GetProcessHeap(), 0, copy);
		if (step) return step;
		patched = 1;
	} else {
		DeleteFileW(exe_pri); /* a leftover copy would change which index the manager reads */
	}

	if (!RoInitialize || !RoGetActivationFactory) return 25;
	if (FAILED(RoInitialize(1))) return 1;
	if (WindowsCreateString(name, ARRAYSIZE(name) - 1, &class_key) != S_OK || !class_key) return 26;
	if (RoGetActivationFactory(class_key, &IID_IActivationFactory, (void **)&activation) != S_OK || !activation) return 2;
	/* a second activation of the class hands out the same factory object */
	if (RoGetActivationFactory(class_key, &IID_IActivationFactory, (void **)&activation_again) != S_OK ||
		activation_again != activation) return 3;
	if (activation->lpVtbl->QueryInterface(activation, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics,
		(void **)&statics) != S_OK || !statics) return 4;
	/* the manager of the process is a singleton: two asks answer the same object */
	if (statics->lpVtbl->get_Current(statics, (uint64_t)(uintptr_t)&manager) != S_OK || !manager) return 5;
	if (statics->lpVtbl->get_Current(statics, (uint64_t)(uintptr_t)&manager_again) != S_OK || manager_again != manager) return 6;
	if (manager->lpVtbl->GetRuntimeClassName(manager, &text) != S_OK ||
		!same_text(text, L"Windows.ApplicationModel.Resources.Core.ResourceManager")) return 7;
	WindowsDeleteString(text);
	text = NULL;
	if (manager->lpVtbl->QueryInterface(manager, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2,
		(void **)&manager2) != S_OK || !manager2) return 8;
	/* the main map is the root of the index beside the executable */
	if (manager->lpVtbl->get_MainResourceMap(manager, (uint64_t)(uintptr_t)&map) != S_OK || !map) return 9;
	if (map->lpVtbl->GetRuntimeClassName(map, &text) != S_OK ||
		!same_text(text, L"Windows.ApplicationModel.Resources.Core.ResourceMap")) return 10;
	WindowsDeleteString(text);
	text = NULL;
	/* the manager's default context comes from the ResourceContext class, whatever the index holds */
	if (manager->lpVtbl->get_DefaultContext(manager, (uint64_t)(uintptr_t)&default_context) != S_OK ||
		!default_context) return 43;
	if (WindowsCreateString(greeting_name, ARRAYSIZE(greeting_name) - 1, &greeting_key) != S_OK || !greeting_key) return 27;
	if (WindowsCreateString(farewell_name, ARRAYSIZE(farewell_name) - 1, &farewell_key) != S_OK || !farewell_key) return 28;
	if (WindowsCreateString(L"Fixture/Missing", 15, &missing_key) != S_OK || !missing_key) return 29;
	if (WindowsCreateString(L"Fixture", 7, &subtree_key) != S_OK || !subtree_key) return 30;
	if (WindowsCreateString(L"Greeting", 8, &subtree_greeting_key) != S_OK || !subtree_greeting_key) return 31;
	if (!patched) {
		/* no index in this layout: every lookup answers the documented miss, without crashing */
		if (map->lpVtbl->GetValue(map, (uint64_t)(uintptr_t)greeting_key, (uint64_t)(uintptr_t)&named) != E_BOUNDS || named) return 50;
		if (map->lpVtbl->GetSubtree(map, (uint64_t)(uintptr_t)subtree_key, (uint64_t)(uintptr_t)&subtree) != E_BOUNDS || subtree) return 51;
	} else {
	/* the greeting answers with the patched value: the index beside the executable is the one loaded */
	if (map->lpVtbl->GetValue(map, (uint64_t)(uintptr_t)greeting_key, (uint64_t)(uintptr_t)&named) != S_OK || !named) return 11;
	if (named->lpVtbl->GetRuntimeClassName(named, &text) != S_OK ||
		!same_text(text, L"Windows.ApplicationModel.Resources.Core.NamedResource")) return 12;
	WindowsDeleteString(text);
	text = NULL;
	if (named->lpVtbl->Resolve(named, (uint64_t)(uintptr_t)&candidate) != S_OK || !candidate) return 13;
	if (candidate->lpVtbl->get_ValueAsString(candidate, (uint64_t)(uintptr_t)&text) != S_OK ||
		!same_text(text, greeting_patched)) return 14;
	WindowsDeleteString(text);
	text = NULL;
	/* the candidate is a string candidate (ResourceCandidateKind.String), the default and only match, and its
	   qualifier view is the empty one */
	if (candidate->lpVtbl->QueryInterface(candidate, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3,
		(void **)&candidate3) != S_OK || !candidate3) return 15;
	if (candidate3->lpVtbl->get_Kind(candidate3, (uint64_t)(uintptr_t)&kind) != S_OK || kind != 0) return 16;
	if (candidate->lpVtbl->get_IsDefault(candidate, (uint64_t)(uintptr_t)&flag) != S_OK || !flag) return 17;
	if (candidate->lpVtbl->get_Qualifiers(candidate, (uint64_t)(uintptr_t)&qualifiers) != S_OK || !qualifiers) return 18;
	if (qualifiers->lpVtbl->get_Size(qualifiers, &size) != S_OK || size != 0) {
		qualifiers->lpVtbl->Release(qualifiers);
		return 19;
	}
	qualifiers->lpVtbl->Release(qualifiers);
	qualifiers = NULL;
	candidate3->lpVtbl->Release(candidate3);
	candidate3 = NULL;
	/* the item's candidates come back as a vector view holding the one candidate */
	if (named->lpVtbl->get_Candidates(named, (uint64_t)(uintptr_t)&view) != S_OK || !view) return 33;
	if (view->lpVtbl->get_Size(view, &size) != S_OK || size != 1) return 34;
	if (view->lpVtbl->GetAt(view, 0, &view_candidate) != S_OK || !view_candidate) return 35;
	if (view_candidate->lpVtbl->get_ValueAsString(view_candidate, (uint64_t)(uintptr_t)&text) != S_OK ||
		!same_text(text, greeting_patched)) return 36;
	WindowsDeleteString(text);
	text = NULL;
	view_candidate->lpVtbl->Release(view_candidate);
	view_candidate = NULL;
	view->lpVtbl->Release(view);
	view = NULL;
	candidate->lpVtbl->Release(candidate);
	candidate = NULL;
	named->lpVtbl->Release(named);
	named = NULL;
	/* the context overload answers the same with the default (NULL) context and with a real one */
	if (map->lpVtbl->GetValue2(map, (uint64_t)(uintptr_t)greeting_key, 0, (uint64_t)(uintptr_t)&named) != S_OK || !named) return 37;
	if (named->lpVtbl->Resolve(named, (uint64_t)(uintptr_t)&candidate) != S_OK || !candidate) return 37;
	if (candidate->lpVtbl->get_ValueAsString(candidate, (uint64_t)(uintptr_t)&text) != S_OK ||
		!same_text(text, greeting_patched)) return 37;
	WindowsDeleteString(text);
	text = NULL;
	candidate->lpVtbl->Release(candidate);
	candidate = NULL;
	named->lpVtbl->Release(named);
	named = NULL;
	if (WindowsCreateString(context_name, ARRAYSIZE(context_name) - 1, &context_key) != S_OK || !context_key) return 38;
	if (RoGetActivationFactory(context_key, &IID_IActivationFactory, (void **)&context_activation) != S_OK) return 39;
	if (context_activation->lpVtbl->QueryInterface(context_activation,
		&IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2,
		(void **)&context_statics) != S_OK || !context_statics) return 39;
	if (context_statics->lpVtbl->GetForViewIndependentUse(context_statics, (uint64_t)(uintptr_t)&context) != S_OK ||
		!context) return 39;
	if (map->lpVtbl->GetValue2(map, (uint64_t)(uintptr_t)greeting_key, (uint64_t)(uintptr_t)context,
		(uint64_t)(uintptr_t)&named) != S_OK || !named) return 40;
	if (named->lpVtbl->Resolve(named, (uint64_t)(uintptr_t)&candidate) != S_OK || !candidate) return 40;
	if (candidate->lpVtbl->get_ValueAsString(candidate, (uint64_t)(uintptr_t)&text) != S_OK ||
		!same_text(text, greeting_patched)) return 40;
	WindowsDeleteString(text);
	text = NULL;
	candidate->lpVtbl->Release(candidate);
	candidate = NULL;
	named->lpVtbl->Release(named);
	named = NULL;
	/* QualifierValues is the observable map of qualifiers: the runtime's interface dispatch subscribes MapChanged on it, the
	   XAML resource read looks the language up, and the projection takes a weak reference to the context. */
	{
		fixture_observable_map *observable = NULL;
		fixture_map *values = NULL;
		fixture_handler *handler = fixture_handler_create();
		fixture_vector_string *languages = NULL;
		fixture_weak_source *weak_source = NULL;
		fixture_weak_ref *weak = NULL;
		context_i *resolved = NULL;
		HSTRING key = NULL, value = NULL, language = NULL;
		EventRegistrationToken token;
		if (!handler) return 60;
		if (context->lpVtbl->get_QualifierValues(context, (uint64_t)(uintptr_t)&observable) != S_OK || !observable) return 61;
		if (observable->lpVtbl->QueryInterface(observable, &fixture_map_iid, (void **)&values) != S_OK || !values) return 62;
		/* the language qualifier answers from the host's default UI language, and a key the map does not hold answers
		   the documented miss */
		if (WindowsCreateString(L"language", 8, &key) != S_OK || !key) return 63;
		if (values->lpVtbl->Lookup(values, key, &value) != S_OK || !value) return 64;
		if (!same_text(value, L"en-US")) return 65;
		WindowsDeleteString(value);
		value = NULL;
		WindowsDeleteString(key);
		key = NULL;
		if (values->lpVtbl->Lookup(values, greeting_key, &value) != E_BOUNDS || value) return 66;
		/* the subscription answers a token, holds the handler while it is subscribed, and the removal takes it back */
		if (observable->lpVtbl->add_MapChanged(observable, handler, &token) != S_OK || !token.value) return 67;
		if (handler->refs != 2) return 68; /* the map holds the handler while it is subscribed */
		if (observable->lpVtbl->remove_MapChanged(observable, token) != S_OK) return 69;
		if (handler->refs != 1) return 70; /* the removal released the map's reference */
		/* the context answers the weak reference the projection takes, and the weak reference resolves to the context */
		if (context->lpVtbl->QueryInterface(context, &fixture_weak_source_iid, (void **)&weak_source) != S_OK ||
			!weak_source) return 71;
		if (weak_source->lpVtbl->GetWeakReference(weak_source, &weak) != S_OK || !weak) return 72;
		if (weak->lpVtbl->Resolve(weak, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContext,
			(IInspectable **)&resolved) != S_OK || !resolved) return 73;
		resolved->lpVtbl->Release(resolved);
		resolved = NULL;
		weak->lpVtbl->Release(weak);
		weak = NULL;
		weak_source->lpVtbl->Release(weak_source);
		weak_source = NULL;
		/* Languages is the read-only view over the host's default UI language */
		if (context->lpVtbl->get_Languages(context, (uint64_t)(uintptr_t)&languages) != S_OK || !languages) return 74;
		if (languages->lpVtbl->get_Size(languages, &size) != S_OK || size != 1) return 75;
		if (languages->lpVtbl->GetAt(languages, 0, &language) != S_OK || !language) return 76;
		if (!WindowsGetStringLen(language)) return 77;
		WindowsDeleteString(language);
		language = NULL;
		if (languages->lpVtbl->GetAt(languages, 1, &language) != E_BOUNDS || language) return 78;
		WindowsDeleteString(language);
		language = NULL;
		/* the view is its own IUnknown: the release through it is the last reference */
		languages->lpVtbl->Release(languages);
		languages = NULL;
		observable->lpVtbl->Release(observable);
		observable = NULL;
		values->lpVtbl->Release(values);
		values = NULL;
		fixture_handler_release(handler);
		handler = NULL;
	}
	context->lpVtbl->Release(context);
	context = NULL;
	context_statics->lpVtbl->Release(context_statics);
	context_statics = NULL;
	/* the second string answers from the same index */
	if (map->lpVtbl->GetValue(map, (uint64_t)(uintptr_t)farewell_key, (uint64_t)(uintptr_t)&named) != S_OK || !named) return 41;
	if (named->lpVtbl->Resolve(named, (uint64_t)(uintptr_t)&candidate) != S_OK || !candidate) return 41;
	if (candidate->lpVtbl->get_ValueAsString(candidate, (uint64_t)(uintptr_t)&text) != S_OK ||
		!same_text(text, L"Goodbye")) return 41;
	WindowsDeleteString(text);
	text = NULL;
	candidate->lpVtbl->Release(candidate);
	candidate = NULL;
	named->lpVtbl->Release(named);
	named = NULL;
	/* a subtree narrows the map: the same greeting answers under its scope-local name */
	if (map->lpVtbl->GetSubtree(map, (uint64_t)(uintptr_t)subtree_key, (uint64_t)(uintptr_t)&subtree) != S_OK ||
		!subtree) return 42;
	if (subtree->lpVtbl->GetValue(subtree, (uint64_t)(uintptr_t)subtree_greeting_key, (uint64_t)(uintptr_t)&named) != S_OK ||
		!named) return 42;
	if (named->lpVtbl->Resolve(named, (uint64_t)(uintptr_t)&candidate) != S_OK || !candidate) return 42;
	if (candidate->lpVtbl->get_ValueAsString(candidate, (uint64_t)(uintptr_t)&text) != S_OK ||
		!same_text(text, greeting_patched)) return 42;
	WindowsDeleteString(text);
	text = NULL;
	candidate->lpVtbl->Release(candidate);
	candidate = NULL;
	named->lpVtbl->Release(named);
	named = NULL;
	subtree->lpVtbl->Release(subtree);
	subtree = NULL;
	/* a missing name answers the failure with a null out, without crashing */
	if (map->lpVtbl->GetValue(map, (uint64_t)(uintptr_t)missing_key, (uint64_t)(uintptr_t)&named) != E_BOUNDS || named) return 44;
	if (map->lpVtbl->GetSubtree(map, (uint64_t)(uintptr_t)missing_key, (uint64_t)(uintptr_t)&subtree) != E_BOUNDS || subtree) return 45;
	} /* end of the indexed-layout checks */
	/* the resource reference scheme check answers the documented prefix */
	if (WindowsCreateString(L"ms-resource:Something", 21, &text) != S_OK) return 46;
	if (statics->lpVtbl->IsResourceReference(statics, (uint64_t)(uintptr_t)text, (uint64_t)(uintptr_t)&flag) != S_OK || !flag) return 47;
	WindowsDeleteString(text);
	text = NULL;
	if (WindowsCreateString(L"plain", 5, &text) != S_OK) return 48;
	if (statics->lpVtbl->IsResourceReference(statics, (uint64_t)(uintptr_t)text, (uint64_t)(uintptr_t)&flag) != S_OK || flag) return 49;
	WindowsDeleteString(text);
	text = NULL;

	default_context->lpVtbl->Release(default_context);
	default_context = NULL;
	manager2->lpVtbl->Release(manager2);
	manager2 = NULL;
	manager_again->lpVtbl->Release(manager_again);
	manager_again = NULL;
	manager->lpVtbl->Release(manager);
	manager = NULL;
	map->lpVtbl->Release(map);
	map = NULL;
	statics->lpVtbl->Release(statics);
	statics = NULL;
	if (context_activation)
		context_activation->lpVtbl->Release(context_activation);
	context_activation = NULL;
	activation_again->lpVtbl->Release(activation_again);
	activation_again = NULL;
	activation->lpVtbl->Release(activation);
	activation = NULL;
	WindowsDeleteString(subtree_greeting_key);
	WindowsDeleteString(subtree_key);
	WindowsDeleteString(missing_key);
	WindowsDeleteString(farewell_key);
	WindowsDeleteString(greeting_key);
	WindowsDeleteString(context_key);
	WindowsDeleteString(class_key);
	return 0;
}
