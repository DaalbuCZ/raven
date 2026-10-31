/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML metadata: the framework types the engine can instantiate, by their stable XBF index or their full name.
 *
 * An XBF file refers to framework types by stable index (the same numbering Microsoft's XamlTypeInfo uses) and to application
 * types through its own type table. This table covers the framework types implemented here; a name from the application's type
 * table is resolved by the same activation path when it names a class the runtime catalog knows. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <roapi.h>
#include <winstring.h>
#include "windows_ui_xaml_markup.h"

struct metadata_entry {
	uint32_t stable;
	const WCHAR *name;
	const WCHAR *class_name;
};

struct stable_property_entry {
	uint32_t stable;
	const WCHAR *name;
};

/* The framework properties the loader applies. The stable indices are those Microsoft's XamlTypeInfo uses; Grid.Row and
 * Grid.Column are the attached properties a child element carries, Grid.RowSpan and Grid.ColumnSpan the attached track counts
 * beside them, RowDefinitions and ColumnDefinitions the collections a grid holds its layout tracks in, and FontSize and
 * Foreground the text properties a TextBlock draws with. */
static const struct stable_property_entry properties[] = {
	{560, L"Template"},
	/* ContentPresenter.ContentTemplate: the template-adjacent member beside Control.Template — the loader stores it on
	 * the presenter element through the engine-internal store (not applied to the render yet, see the presenter
	 * comment in xaml_elements.c). Index verified against scripts/xbf-stable-indices.json's numbering convention
	 * (property index == array position). */
	{525, L"ContentTemplate"},
	{1047, L"CacheSize"},
	{832, L"Content"},
	{2408, L"Content"},
	{999, L"Content"},
	{647, L"Background"},
	{648, L"Children"},
	{784, L"FontSize"},
	{788, L"Foreground"},
	{800, L"Text"},
	{409, L"Width"},
	{393, L"Height"},
	{1116, L"NavigationCacheMode"},
	{1052, L"SourcePageType"},
	{2282, L"IsNavigationStackEnabled"},
	{860, L"Row"},
	{857, L"Column"},
	{862, L"RowSpan"},
	{859, L"ColumnSpan"},
	{861, L"RowDefinitions"},
	{858, L"ColumnDefinitions"},
};

static const struct metadata_entry entries[] = {
	{31, L"Application", L"Windows.UI.Xaml.Application"},
	{141, L"Window", L"Windows.UI.Xaml.Window"},
	{509, L"Frame", L"Windows.UI.Xaml.Controls.Frame"},
	{525, L"Page", L"Windows.UI.Xaml.Controls.Page"},
	{442, L"Grid", L"Windows.UI.Xaml.Controls.Grid"},
	{396, L"TextBlock", L"Windows.UI.Xaml.Controls.TextBlock"},
	{540, L"Button", L"Windows.UI.Xaml.Controls.Button"},
	{106, L"RowDefinition", L"Windows.UI.Xaml.Controls.RowDefinition"},
	{44, L"ColumnDefinition", L"Windows.UI.Xaml.Controls.ColumnDefinition"},
	/* The template classes a real app XBF names by stable index: the property Control.Template (560) above and these two
	 * types — the template value and the presenter inside it. ContentPresenter is not in the runtime catalog, so its
	 * activation falls to the loader's engine-internal factory path, not RoActivateInstance. */
	{191, L"ControlTemplate", L"Windows.UI.Xaml.Controls.ControlTemplate"},
	{296, L"ContentPresenter", L"Windows.UI.Xaml.Controls.ContentPresenter"},
};

const WCHAR *raven_xaml_class_by_stable(uint32_t stable)
{
	for (size_t index = 0; index < ARRAYSIZE(entries); ++index)
		if (entries[index].stable == stable)
			return entries[index].class_name;
	return NULL;
}

const WCHAR *raven_xaml_property_by_stable(uint32_t stable)
{
	for (size_t index = 0; index < ARRAYSIZE(properties); ++index)
		if (properties[index].stable == stable)
			return properties[index].name;
	return NULL;
}

static int same_text(const WCHAR *text, UINT32 length, const WCHAR *name)
{
	return text && CompareStringOrdinal(text, (int)length, name, -1, FALSE) == CSTR_EQUAL;
}

/* A type the framework table does not know is an application type: the XAML compiler generated the provider on the application
 * object, and its by-name GetXamlType overload followed by the type's ActivateInstance is the documented creation path. */
static HRESULT create_from_provider(const WCHAR *name, UINT32 length, IInspectable **out)
{
	static const IID *provider_iid = &IID__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider;
	IInspectable *current = NULL;
	__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider *provider = NULL;
	__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *type = NULL;
	HSTRING full_name = NULL;
	HRESULT result;
	*out = NULL;
	result = raven_xaml_application_current(&current);
	if (FAILED(result))
		return result;
	if (!current)
		return CLASS_E_CLASSNOTAVAILABLE;
	result = current->lpVtbl->QueryInterface(current, provider_iid, (void **)&provider);
	current->lpVtbl->Release(current);
	if (FAILED(result) || !provider)
		return CLASS_E_CLASSNOTAVAILABLE;
	result = WindowsCreateString(name, length, &full_name);
	if (SUCCEEDED(result)) {
		/* The second GetXamlType slot is the by-full-name overload; the first takes the interop TypeName. */
		result = provider->lpVtbl->GetXamlType2(provider, (uint64_t)(uintptr_t)full_name, (uint64_t)(uintptr_t)&type);
		WindowsDeleteString(full_name);
	}
	provider->lpVtbl->Release(provider);
	if (FAILED(result) || !type)
		return CLASS_E_CLASSNOTAVAILABLE;
	result = type->lpVtbl->ActivateInstance(type, (uint64_t)(uintptr_t)out);
	type->lpVtbl->Release(type);
	if (FAILED(result) || !*out)
		return FAILED(result) ? result : CLASS_E_CLASSNOTAVAILABLE;
	return S_OK;
}

HRESULT raven_xaml_create_named(const WCHAR *name, UINT32 length, IInspectable **out)
{
	HSTRING key = NULL;
	HRESULT result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!name)
		return E_INVALIDARG;
	for (size_t index = 0; index < ARRAYSIZE(entries); ++index) {
		if (!same_text(name, length, entries[index].class_name) && !same_text(name, length, entries[index].name))
			continue;
		result = WindowsCreateString(entries[index].class_name, (UINT32)lstrlenW(entries[index].class_name), &key);
		if (FAILED(result))
			return result;
		result = RoActivateInstance(key, out);
		WindowsDeleteString(key);
		return result;
	}
	return create_from_provider(name, length, out);
}

HRESULT raven_xaml_create_stable(uint32_t stable, IInspectable **out)
{
	const WCHAR *class_name = raven_xaml_class_by_stable(stable);
	if (!class_name)
		return CLASS_E_CLASSNOTAVAILABLE;
	return raven_xaml_create_named(class_name, (UINT32)lstrlenW(class_name), out);
}
