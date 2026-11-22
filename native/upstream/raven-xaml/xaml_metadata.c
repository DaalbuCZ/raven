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
#include <windows.ui.xaml.interop.h>
#include "windows_ui_xaml_markup.h"

typedef __x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName xaml_type_name;

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
	{73, L"Resources"},
	{404, L"Resources"}, /* FrameworkElement.Resources */
	/* SolidColorBrush.Color: the property the compiled theme dictionaries apply their kind-8 constants to (the brush
	 * itself builds through the loader's create_known). Index verified against scripts/xbf-stable-indices.json. */
	{476, L"SolidColorBrush_Color"},
	{1960, L"XamlCompositionBrushBase_FallbackColor"},
	{687, L"MergedDictionaries"},
	{688, L"Source"},
	{689, L"ThemeDictionaries"},
	{560, L"Template"},
	/* ContentPresenter.ContentTemplate: the template-adjacent member beside Control.Template — the loader stores it on
	 * the presenter element through the engine-internal store (not applied to the render yet, see the presenter
	 * comment in xaml_elements.c). Index verified against scripts/xbf-stable-indices.json's numbering convention
	 * (property index == array position). */
	{525, L"ContentTemplate"},
	/* ContentControl.ContentTemplate: the same member name on the content-control owner. The engine's dispatch applies
	 * "ContentTemplate" by simple name, so this resolves end-to-end only where the store accepts it — an engine
	 * ContentPresenter; a real ContentControl (Button) answers the skip convention (S_FALSE) and the loader moves on.
	 * Index verified against the same convention (properties[833] == "ContentControl_ContentTemplate"). */
	{833, L"ContentTemplate"},
	/* TextElement.FontSize and TextElement.Foreground: the attached text properties a real document carries on the
	 * text elements. The engine's float and text dispatches apply "FontSize" and "Foreground" by simple name, so both
	 * resolve end-to-end on the engine's text element (TextBlock); an element without the text storage answers the
	 * skip convention. Indices verified against the same convention (properties[251]/[255]). */
	{251, L"FontSize"},
	{255, L"Foreground"},
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
	/* Style.TargetType: the one property whose value is a compiled type rather than a constant
	 * (SetValueTypeConvertedResolvedType). The loader applies it through the style setter, which stores the short name. */
	{244, L"BasedOn"},
	{247, L"TargetType"},
};

static const struct metadata_entry entries[] = {
	{601, L"ElementTheme", L"Windows.UI.Xaml.ElementTheme"},
	{31, L"Application", L"Windows.UI.Xaml.Application"},
	{371, L"ResourceDictionary", L"Windows.UI.Xaml.ResourceDictionary"},
	{141, L"Window", L"Windows.UI.Xaml.Window"},
	{509, L"Frame", L"Windows.UI.Xaml.Controls.Frame"},
	{525, L"Page", L"Windows.UI.Xaml.Controls.Page"},
	/* The root the UILibrary controls' XBFs bind (TitleBarControl.xbf's CreateTypeBegin UserControl rides the
	 * stable index, the same convention Page's 525 uses; index from the dumped XBF stable-type table). */
	{491, L"UserControl", L"Windows.UI.Xaml.Controls.UserControl"},
	{442, L"Grid", L"Windows.UI.Xaml.Controls.Grid"},
	{396, L"TextBlock", L"Windows.UI.Xaml.Controls.TextBlock"},
	{540, L"Button", L"Windows.UI.Xaml.Controls.Button"},
	/* A compiled document declares its styles inline, so the Style class is named by stable index like the controls. */
	{114, L"Style", L"Windows.UI.Xaml.Style"},
	{106, L"RowDefinition", L"Windows.UI.Xaml.Controls.RowDefinition"},
	{44, L"ColumnDefinition", L"Windows.UI.Xaml.Controls.ColumnDefinition"},
	/* The shape elements and the visual-state tree the UILibrary control XBFs declare: the stable indices the
	 * dumped documents carry (verified against xbf-stable-indices.json), served by the engine's element factories. */
	{286, L"Border", L"Windows.UI.Xaml.Controls.Border"},
	{481, L"StackPanel", L"Windows.UI.Xaml.Controls.StackPanel"},
	{326, L"Image", L"Windows.UI.Xaml.Controls.Image"},
	{263, L"Setter", L"Windows.UI.Xaml.Setter"},
	{137, L"VisualState", L"Windows.UI.Xaml.VisualState"},
	{138, L"VisualStateGroup", L"Windows.UI.Xaml.VisualStateGroup"},
	/* The template classes a real app XBF names by stable index: the property Control.Template (560) above and these two
	 * types — the template value and the presenter inside it. ContentPresenter is not in the runtime catalog, so its
	 * activation falls to the loader's engine-internal factory path, not RoActivateInstance. */
	{191, L"ControlTemplate", L"Windows.UI.Xaml.Controls.ControlTemplate"},
	{296, L"ContentPresenter", L"Windows.UI.Xaml.Controls.ContentPresenter"},
	/* The media brush the compiled theme dictionaries build: the XBFs' xmlns is "using:Microsoft.UI.Xaml.Media" (the
	 * WinUI namespace), so that is the runtime class the stable index carries; activation lands on the loader's
	 * create_known, which serves it from the engine (xaml_styles.c). */
	{265, L"SolidColorBrush", L"Microsoft.UI.Xaml.Media.SolidColorBrush"},
	/* The shape class a compiled document names by stable index (ConfirmDialog.xbf's Shape_Fill rectangles ride
	 * stable 470, verified against the generated xbf_type_names table): the catalog carries the class and the
	 * engine's element factory serves it. */
	{470, L"Rectangle", L"Windows.UI.Xaml.Shapes.Rectangle"},
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

/* The application's XAML metadata provider: the XamlTypeInfo provider .NET Native generates for every compiled XAML app,
 * implemented on the Application object's managed side. The object Application.Current hands out is the composition outer's
 * face, so its QueryInterface reaches the managed CCW the provider rides. */
static HRESULT provider_from_application(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider **out)
{
	static const IID *provider_iid = &IID__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider;
	IInspectable *current = NULL;
	HRESULT result = raven_xaml_application_current(&current);
	if (FAILED(result))
		return result;
	if (!current) {
		raven_trace_note("raven-xaml: metadata provider: no Application.Current\n");
		return CLASS_E_CLASSNOTAVAILABLE;
	}
	result = current->lpVtbl->QueryInterface(current, provider_iid, (void **)out);
	current->lpVtbl->Release(current);
	if (FAILED(result) || !*out) {
		char note[96];
		wsprintfA(note, "raven-xaml: metadata provider QI refused 0x%08x\n", (unsigned)result);
		raven_trace_note(note);
		return CLASS_E_CLASSNOTAVAILABLE;
	}
	return S_OK;
}

/* Resolve one type name over the provider's documented overloads, stopping at the first that answers a type:
 * GetXamlType(TypeName) — the primary slot the Windows framework drives with the TypeName exactly as the caller passed it,
 * whose managed thunk turns the struct back into System.Type — then GetXamlType(String) with the name as carried, and
 * finally with the plain full name before the assembly qualifier, the key form the generated XamlTypeInfo map stores. */
static HRESULT provider_type_by_name(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider *provider,
	const xaml_type_name *original, const WCHAR *name, UINT32 length,
	__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType **out)
{
	HSTRING full_name = NULL;
	HRESULT result;
	*out = NULL;
	if (original && original->Name) {
		result = provider->lpVtbl->GetXamlType(provider, (uint64_t)(uintptr_t)original, (uint64_t)(uintptr_t)out);
		if (SUCCEEDED(result) && *out)
			return S_OK;
	}
	result = WindowsCreateString(name, length, &full_name);
	if (FAILED(result))
		return result;
	result = provider->lpVtbl->GetXamlType2(provider, (uint64_t)(uintptr_t)full_name, (uint64_t)(uintptr_t)out);
	WindowsDeleteString(full_name);
	if (SUCCEEDED(result) && *out)
		return S_OK;
	UINT32 plain = 0;
	for (UINT32 index = 0; index < length; ++index) {
		if (name[index] == L',')
			break;
		plain = index + 1;
	}
	if (!plain || plain == length)
		return FAILED(result) ? result : CLASS_E_CLASSNOTAVAILABLE;
	result = WindowsCreateString(name, plain, &full_name);
	if (FAILED(result))
		return result;
	result = provider->lpVtbl->GetXamlType2(provider, (uint64_t)(uintptr_t)full_name, (uint64_t)(uintptr_t)out);
	WindowsDeleteString(full_name);
	return result;
}

/* ActivateInstance: the documented page creation — the managed XamlTypeInfo instantiates the app's page (its constructor
 * runs the page's InitializeComponent) and hands the managed CCW back as the IInspectable. The type name rides the trace
 * so a failed activation names the class whose constructor threw. */
static HRESULT provider_activate(__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *type,
	const WCHAR *name, UINT32 length, IInspectable **out)
{
	HRESULT result = type->lpVtbl->ActivateInstance(type, (uint64_t)(uintptr_t)out);
	if (FAILED(result)) {
		char note[96];
		if (name) {
			UINT32 printed = length > 40 ? 40 : length;
			wsprintfA(note, "raven-xaml: activate %.*ls failed 0x%08x\n", (int)printed, name, (unsigned)result);
		} else {
			wsprintfA(note, "raven-xaml: xamltype ActivateInstance failed 0x%08x\n", (unsigned)result);
		}
		raven_trace_note(note);
	}
	if (FAILED(result) || !*out)
		return FAILED(result) ? result : CLASS_E_CLASSNOTAVAILABLE;
	return S_OK;
}

/* A type the framework table does not know is an application type: the XAML compiler generated the provider on the application
 * object, and its GetXamlType overloads followed by the type's ActivateInstance are the documented creation path. */
static HRESULT create_from_provider(const WCHAR *name, UINT32 length, IInspectable **out)
{
	__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider *provider = NULL;
	__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *type = NULL;
	HRESULT result;
	*out = NULL;
	result = provider_from_application(&provider);
	if (FAILED(result))
		return result;
	result = provider_type_by_name(provider, NULL, name, length, &type);
	provider->lpVtbl->Release(provider);
	if (FAILED(result) || !type)
		return FAILED(result) ? result : CLASS_E_CLASSNOTAVAILABLE;
	result = provider_activate(type, name, length, out);
	type->lpVtbl->Release(type);
	return result;
}

/* Frame.Navigate's application types: the TypeName the app passed rides the provider's primary overload unchanged — the
 * exact shape the Windows framework drives the managed XamlTypeInfo with (the .NET Native Type-to-TypeName marshalling
 * carries the assembly-qualified name and its kind). */
HRESULT raven_xaml_create_from_type(const xaml_type_name *type, IInspectable **out)
{
	__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider *provider = NULL;
	__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType *xaml_type = NULL;
	const WCHAR *text;
	UINT32 length = 0;
	HRESULT result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!type || !type->Name)
		return E_INVALIDARG;
	result = provider_from_application(&provider);
	if (FAILED(result))
		return result;
	text = WindowsGetStringRawBuffer(type->Name, &length);
	result = provider_type_by_name(provider, type, text, length, &xaml_type);
	provider->lpVtbl->Release(provider);
	if (FAILED(result) || !xaml_type)
		return FAILED(result) ? result : CLASS_E_CLASSNOTAVAILABLE;
	result = provider_activate(xaml_type, text, length, out);
	xaml_type->lpVtbl->Release(xaml_type);
	return result;
}

HRESULT raven_xaml_create_named(const WCHAR *name, UINT32 length, IInspectable **out)
{
	HSTRING key = NULL;
	HRESULT result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!name)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_metadata.c:177\n"); return E_INVALIDARG; }
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
