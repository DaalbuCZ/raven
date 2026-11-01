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
	uri_i *uri = NULL, *missing = NULL, *invalid = NULL;
	textblock *text = NULL;
	HSTRING app_name = make(L"Windows.UI.Xaml.Application"), uri_name = make(L"Windows.Foundation.Uri");
	HSTRING text_name = make(L"Windows.UI.Xaml.Controls.TextBlock"), path = make(L"ms-appx:///Fixture/Message.xaml");
	HSTRING absent = make(L"ms-appx:///Fixture/Absent.xaml"), broken = make(L"ms-appx:///Fixture/Invalid.xaml"), value = NULL;
	if (FAILED(RoInitialize(1)) || !app_name || !uri_name || !text_name || !path || !absent || !broken) return 1;
	if (RoGetActivationFactory(app_name, &IID_IActivationFactory, (void **)&app_factory) != S_OK ||
		IActivationFactory_QueryInterface(app_factory, &IID__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics,
			(void **)&statics) != S_OK || !statics) return 2;
	if (RoGetActivationFactory(uri_name, &IID___x_ABI_CWindows_CFoundation_CIUriRuntimeClassFactory,
		(void **)&uris) != S_OK || !uris || uris->lpVtbl->CreateUri(uris, path, &uri) != S_OK || !uri ||
		uris->lpVtbl->CreateUri(uris, absent, &missing) != S_OK || !missing ||
		uris->lpVtbl->CreateUri(uris, broken, &invalid) != S_OK || !invalid) return 3;
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
	WindowsDeleteString(value); text->lpVtbl->Release(text); IInspectable_Release(target); IInspectable_Release(wrong);
	uri->lpVtbl->Release(uri); missing->lpVtbl->Release(missing); invalid->lpVtbl->Release(invalid); uris->lpVtbl->Release(uris);
	statics->lpVtbl->Release(statics); IActivationFactory_Release(app_factory);
	WindowsDeleteString(app_name); WindowsDeleteString(uri_name); WindowsDeleteString(text_name);
	WindowsDeleteString(path); WindowsDeleteString(absent); WindowsDeleteString(broken);
	return 0;
}
