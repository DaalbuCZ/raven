/* SPDX-License-Identifier: MIT */
#define INITGUID
#define CONST_VTABLE
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <roapi.h>
#include <winstring.h>
#include <windows.foundation.h>
#include <weakreference.h>

typedef __x_ABI_CWindows_CFoundation_CIUriRuntimeClass uri_i;
typedef __x_ABI_CWindows_CFoundation_CIUriRuntimeClassFactory factory_i;
static BOOL same(HSTRING value, const WCHAR *expected)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(value, &length);
	return length == (UINT32)lstrlenW(expected) && CompareStringOrdinal(text, length, expected, -1, FALSE) == CSTR_EQUAL;
}
#define CHECK_STRING(method, expected, code) do { \
	HSTRING actual = NULL; \
	if (uri->lpVtbl->method(uri, &actual) != S_OK || !same(actual, expected)) return code; \
	WindowsDeleteString(actual); \
} while (0)
int raven_entry(void)
{
	HSTRING name = NULL, input = NULL;
	IActivationFactory *activation = NULL;
	factory_i *factory = NULL;
	uri_i *uri = NULL;
	__x_ABI_CWindows_CFoundation_CIStringable *stringable = NULL;
	IWeakReferenceSource *source = NULL;
	if (FAILED(RoInitialize(1))) return 1;
	if (WindowsCreateString(L"Windows.Foundation.Uri", 22, &name) != S_OK) return 2;
	if (RoGetActivationFactory(name, &IID_IActivationFactory, (void **)&activation) != S_OK || !activation) return 3;
	if (IActivationFactory_QueryInterface(activation, &IID___x_ABI_CWindows_CFoundation_CIUriRuntimeClassFactory,
		(void **)&factory) != S_OK || !factory) return 4;
	static const WCHAR text[] = L"ms-appx:///Views/Home.xaml?x=1#section";
	if (WindowsCreateString(text, ARRAYSIZE(text)-1, &input) != S_OK ||
		factory->lpVtbl->CreateUri(factory, input, &uri) != S_OK || !uri) return 5;
	WindowsDeleteString(input); input = NULL;
	CHECK_STRING(get_RawUri, text, 6);
	CHECK_STRING(get_AbsoluteUri, text, 7);
	CHECK_STRING(get_SchemeName, L"ms-appx", 8);
	CHECK_STRING(get_Path, L"/Views/Home.xaml", 9);
	CHECK_STRING(get_Query, L"?x=1", 10);
	CHECK_STRING(get_Fragment, L"#section", 11);
	if (uri->lpVtbl->get_Path(uri, NULL) != E_POINTER) return 12;
	if (uri->lpVtbl->QueryInterface(uri, &IID___x_ABI_CWindows_CFoundation_CIStringable, (void **)&stringable) != S_OK ||
		!stringable) return 13;
	HSTRING text_result = NULL;
	if (stringable->lpVtbl->ToString(stringable, &text_result) != S_OK || !same(text_result, text)) return 14;
	WindowsDeleteString(text_result); stringable->lpVtbl->Release(stringable);
	if (uri->lpVtbl->QueryInterface(uri, &IID_IWeakReferenceSource, (void **)&source) != S_OK || !source) return 15;
	IWeakReferenceSource_Release(source);
	uri->lpVtbl->Release(uri); uri = NULL;
	if (WindowsCreateString(L"relative/path", 13, &input) != S_OK ||
		factory->lpVtbl->CreateUri(factory, input, &uri) != E_INVALIDARG || uri) return 16;
	WindowsDeleteString(input); input = NULL;
	if (WindowsCreateString(L"https://example.com/a%20b", 25, &input) != S_OK ||
		factory->lpVtbl->CreateUri(factory, input, &uri) != E_NOTIMPL || uri) return 17;
	WindowsDeleteString(input);
	factory->lpVtbl->Release(factory); IActivationFactory_Release(activation); WindowsDeleteString(name);
	return 0;
}
