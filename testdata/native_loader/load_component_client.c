/* SPDX-License-Identifier: MIT */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <roapi.h>
#include <winstring.h>
#include <windows.foundation.h>
#include "windows_ui_xaml_application.h"
#include "windows_ui_xaml_elements.h"
#include "windows_ui_xaml_dictionary.h"
#ifndef E_BOUNDS
#define E_BOUNDS ((HRESULT)0x8000000b)
#endif

typedef __x_ABI_CWindows_CUI_CXaml_CIApplicationStatics app_statics;
typedef __x_ABI_CWindows_CFoundation_CIUriRuntimeClassFactory uri_factory;
typedef __x_ABI_CWindows_CFoundation_CIUriRuntimeClass uri_i;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock textblock;
static HSTRING make(const WCHAR *text)
{
	HSTRING value = NULL;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), &value) == S_OK ? value : NULL;
}
int raven_entry(void)
{
	IActivationFactory *app_factory = NULL;
	app_statics *statics = NULL;
	uri_factory *uris = NULL;
	IInspectable *target = NULL, *wrong = NULL;
	uri_i *uri = NULL, *missing = NULL, *invalid = NULL, *dictionaries = NULL, *missing_type = NULL;
	textblock *text = NULL;
	HSTRING app_name = make(L"Windows.UI.Xaml.Application"), uri_name = make(L"Windows.Foundation.Uri");
	HSTRING text_name = make(L"Windows.UI.Xaml.Controls.TextBlock"), path = make(L"ms-appx:///Fixture/Message.xaml");
	HSTRING absent = make(L"ms-appx:///Fixture/Absent.xaml"), broken = make(L"ms-appx:///Fixture/Invalid.xaml"), value = NULL;
	HSTRING dict_path = make(L"ms-appx:///Fixture/Dictionaries.xaml"), bad_type_path = make(L"ms-appx:///Fixture/MissingType.xaml");
	if (FAILED(RoInitialize(1)) || !app_name || !uri_name || !text_name || !path || !absent || !broken || !dict_path || !bad_type_path) return 1;
	if (RoGetActivationFactory(app_name, &IID_IActivationFactory, (void **)&app_factory) != S_OK ||
		IActivationFactory_QueryInterface(app_factory, &IID__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics,
			(void **)&statics) != S_OK || !statics) return 2;
	if (RoGetActivationFactory(uri_name, &IID___x_ABI_CWindows_CFoundation_CIUriRuntimeClassFactory,
		(void **)&uris) != S_OK || !uris || uris->lpVtbl->CreateUri(uris, path, &uri) != S_OK || !uri ||
		uris->lpVtbl->CreateUri(uris, absent, &missing) != S_OK || !missing ||
		uris->lpVtbl->CreateUri(uris, broken, &invalid) != S_OK || !invalid ||
		uris->lpVtbl->CreateUri(uris, dict_path, &dictionaries) != S_OK || !dictionaries ||
		uris->lpVtbl->CreateUri(uris, bad_type_path, &missing_type) != S_OK || !missing_type) return 3;
	if (RoActivateInstance(text_name, &target) != S_OK || !target ||
		RoActivateInstance(app_name, &wrong) != S_OK || !wrong) return 4;
	if (statics->lpVtbl->LoadComponent(statics, NULL, (IInspectable *)uri) != E_POINTER ||
		statics->lpVtbl->LoadComponent(statics, target, (IInspectable *)missing) != E_BOUNDS ||
		statics->lpVtbl->LoadComponent(statics, target, (IInspectable *)invalid) != E_INVALIDARG ||
		statics->lpVtbl->LoadComponent2(statics, target, (IInspectable *)uri, 1) != E_NOTIMPL ||
		statics->lpVtbl->LoadComponent(statics, wrong, (IInspectable *)uri) != E_INVALIDARG) return 5;
	if (statics->lpVtbl->LoadComponent2(statics, target, (IInspectable *)uri, 0) != S_OK) return 6;
	if (IInspectable_QueryInterface(target, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock, (void **)&text) != S_OK || !text ||
		text->lpVtbl->get_Text(text, (uint64_t)(uintptr_t)&value) != S_OK) return 7;
	UINT32 length = 0;
	const WCHAR *raw = WindowsGetStringRawBuffer(value, &length);
	if (length != 6 || CompareStringOrdinal(raw, length, L"Loaded", -1, FALSE) != CSTR_EQUAL) return 8;
	WindowsDeleteString(value); text->lpVtbl->Release(text); IInspectable_Release(target);
	{
		__x_ABI_CWindows_CUI_CXaml_CIApplication *app = NULL;
		__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *dictionary = NULL, *first = NULL, *second = NULL;
		__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *merged = NULL;
		IInspectable *resources = NULL, *before = NULL;
		UINT32 count = 0;
		/* Application.Resources exists before anything is loaded (the getter creates it lazily) and a failed load must
		 * leave that same dictionary in place. */
		if (IInspectable_QueryInterface(wrong, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication, (void **)&app) != S_OK || !app ||
			app->lpVtbl->get_Resources(app, &before) != S_OK || !before) return 9;
		if (SUCCEEDED(statics->lpVtbl->LoadComponent(statics, wrong, (IInspectable *)missing_type)) ||
			app->lpVtbl->get_Resources(app, &resources) != S_OK || resources != before) return 10;
		IInspectable_Release(resources); resources = NULL;
		if (statics->lpVtbl->LoadComponent(statics, wrong, (IInspectable *)dictionaries) != S_OK ||
			app->lpVtbl->get_Resources(app, &resources) != S_OK || !resources ||
			IInspectable_QueryInterface(resources, &IID__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary, (void **)&dictionary) != S_OK ||
			dictionary->lpVtbl->get_MergedDictionaries(dictionary, (void **)&merged) != S_OK || !merged ||
			merged->lpVtbl->get_Size(merged, &count) != S_OK || count != 2 ||
			merged->lpVtbl->GetAt(merged, 0, &first) != S_OK || !first ||
			merged->lpVtbl->GetAt(merged, 1, &second) != S_OK || !second || first == second) return 11;
		/* the second merged dictionary names its document through Source, and the loader filled it from the PRI */
		{
			__FIMap_2_IInspectable_IInspectable *second_map = NULL;
			IInspectable *second_source = NULL;
			UINT32 entries = 0;
			if (IInspectable_QueryInterface((IInspectable *)second, &IID___FIMap_2_IInspectable_IInspectable,
				(void **)&second_map) != S_OK || !second_map) return 12;
			if (second_map->lpVtbl->get_Size(second_map, &entries) != S_OK || entries != 2) return 13;
			if (second->lpVtbl->get_Source(second, (uint64_t)(uintptr_t)&second_source) != S_OK || !second_source) return 14;
			IInspectable_Release(second_source);
			second_map->lpVtbl->Release(second_map);
		}
		first->lpVtbl->Release(first); second->lpVtbl->Release(second); merged->lpVtbl->Release(merged);
		dictionary->lpVtbl->Release(dictionary); IInspectable_Release(resources); IInspectable_Release(before);
		app->lpVtbl->Release(app);
	}
	/* A Source nested in an Application load sees an earlier merged dictionary, without publishing partial resources. */
	{
		typedef IInspectable *(*boxed_string_fn)(const WCHAR *, UINT32);
		typedef HRESULT (*brush_color_fn)(IInspectable *, void *);
		struct color { BYTE a, r, g, b; } color;
		HMODULE component = LoadLibraryW(L"raven-xaml.dll");
		boxed_string_fn boxed = component ? (boxed_string_fn)(void *)GetProcAddress(component, "raven_xaml_boxed_string_create") : NULL;
		brush_color_fn brush_color = component ? (brush_color_fn)(void *)GetProcAddress(component, "raven_xaml_solid_brush_color") : NULL;
		HSTRING app_path = make(L"ms-appx:///Fixture/AmbientApplication.xaml"), source_path = make(L"ms-appx:///Fixture/AmbientSource.xaml");
		HSTRING dictionary_name = make(L"Windows.UI.Xaml.ResourceDictionary");
		uri_i *app_uri = NULL, *source_uri = NULL;
		IInspectable *app_object = NULL, *fresh_app = NULL, *dictionary_object = NULL, *resources = NULL, *brush = NULL, *key = NULL;
		__x_ABI_CWindows_CUI_CXaml_CIApplication *app = NULL;
		__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *dictionary = NULL, *second = NULL;
		__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *merged = NULL;
		__FIMap_2_IInspectable_IInspectable *map = NULL;
		if (!boxed || !brush_color || !app_path || !source_path || !dictionary_name ||
			uris->lpVtbl->CreateUri(uris, app_path, &app_uri) != S_OK || !app_uri ||
			uris->lpVtbl->CreateUri(uris, source_path, &source_uri) != S_OK || !source_uri ||
			RoActivateInstance(app_name, &app_object) != S_OK || !app_object ||
			RoActivateInstance(dictionary_name, &dictionary_object) != S_OK || !dictionary_object) return 15;
		if (statics->lpVtbl->LoadComponent(statics, dictionary_object, (IInspectable *)source_uri) != E_NOTIMPL) return 16;
		if (statics->lpVtbl->LoadComponent(statics, app_object, (IInspectable *)app_uri) != S_OK) return 17;
		if (IInspectable_QueryInterface(app_object, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication, (void **)&app) != S_OK || !app ||
			app->lpVtbl->get_Resources(app, &resources) != S_OK || !resources ||
			IInspectable_QueryInterface(resources, &IID__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary, (void **)&dictionary) != S_OK || !dictionary ||
			dictionary->lpVtbl->get_MergedDictionaries(dictionary, (void **)&merged) != S_OK || !merged ||
			merged->lpVtbl->GetAt(merged, 1, &second) != S_OK || !second ||
			IInspectable_QueryInterface((IInspectable *)second, &IID___FIMap_2_IInspectable_IInspectable, (void **)&map) != S_OK || !map) return 18;
		key = boxed(L"AmbientBrush", 12);
		if (!key || map->lpVtbl->Lookup(map, key, &brush) != S_OK || !brush || brush_color(brush, &color) != S_OK ||
			color.a != 0xff || color.r != 0x11 || color.g != 0x22 || color.b != 0x33) return 19;
		/* A new current Application has no parent color: a later unrelated load must not inherit a stale frame. */
		if (RoActivateInstance(app_name, &fresh_app) != S_OK || !fresh_app ||
			statics->lpVtbl->LoadComponent(statics, dictionary_object, (IInspectable *)source_uri) != E_NOTIMPL) return 20;
		IInspectable_Release(fresh_app); IInspectable_Release(brush); IInspectable_Release(key);
		map->lpVtbl->Release(map); second->lpVtbl->Release(second); merged->lpVtbl->Release(merged);
		dictionary->lpVtbl->Release(dictionary); IInspectable_Release(resources); app->lpVtbl->Release(app);
		IInspectable_Release(dictionary_object); IInspectable_Release(app_object);
		app_uri->lpVtbl->Release(app_uri); source_uri->lpVtbl->Release(source_uri);
		WindowsDeleteString(app_path); WindowsDeleteString(source_path); WindowsDeleteString(dictionary_name);
		FreeLibrary(component);
	}
	/* The dependency's two differently sized theme filenames must become valid URIs and both merge. */
	{
		HSTRING name = make(L"Microsoft.UI.Xaml.Controls.XamlControlsResources");
		IInspectable *controls = NULL;
		__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *dictionary = NULL;
		__FIVector_1_Windows__CUI__CXaml__CResourceDictionary *merged = NULL;
		UINT32 count = 0;
		if (RoActivateInstance(name, &controls) != S_OK || !controls ||
			IInspectable_QueryInterface(controls, &IID__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary, (void **)&dictionary) != S_OK ||
			dictionary->lpVtbl->get_MergedDictionaries(dictionary, (void **)&merged) != S_OK || !merged ||
			merged->lpVtbl->get_Size(merged, &count) != S_OK || count != 2) return 21;
		for (UINT32 i = 0; i < count; ++i) {
			__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *child = NULL;
			__FIMap_2_IInspectable_IInspectable *map = NULL;
			IInspectable *source = NULL;
			UINT32 entries = 0;
			if (merged->lpVtbl->GetAt(merged, i, &child) != S_OK || !child ||
				IInspectable_QueryInterface((IInspectable *)child, &IID___FIMap_2_IInspectable_IInspectable, (void **)&map) != S_OK ||
				map->lpVtbl->get_Size(map, &entries) != S_OK || entries != 1 ||
				child->lpVtbl->get_Source(child, (uint64_t)(uintptr_t)&source) != S_OK || !source) return 22;
			IInspectable_Release(source); map->lpVtbl->Release(map); child->lpVtbl->Release(child);
		}
		merged->lpVtbl->Release(merged); dictionary->lpVtbl->Release(dictionary); IInspectable_Release(controls);
		WindowsDeleteString(name);
	}
	IInspectable_Release(wrong);
	uri->lpVtbl->Release(uri); missing->lpVtbl->Release(missing); invalid->lpVtbl->Release(invalid);
	dictionaries->lpVtbl->Release(dictionaries); missing_type->lpVtbl->Release(missing_type); uris->lpVtbl->Release(uris);
	statics->lpVtbl->Release(statics); IActivationFactory_Release(app_factory);
	WindowsDeleteString(app_name); WindowsDeleteString(uri_name); WindowsDeleteString(text_name);
	WindowsDeleteString(path); WindowsDeleteString(absent); WindowsDeleteString(broken);
	WindowsDeleteString(dict_path); WindowsDeleteString(bad_type_path);
	return 0;
}
