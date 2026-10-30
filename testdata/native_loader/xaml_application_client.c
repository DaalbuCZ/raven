#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include "windows_ui_xaml_application.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);
HRESULT WINAPI GetRestrictedErrorInfo(void **info);

typedef __x_ABI_CWindows_CUI_CXaml_CIApplication application1;
typedef __x_ABI_CWindows_CUI_CXaml_CIApplication2 application2;
typedef __x_ABI_CWindows_CUI_CXaml_CIApplication3 application3;
typedef __x_ABI_CWindows_CUI_CXaml_CIApplicationStatics application_statics;
typedef __x_ABI_CWindows_CUI_CXaml_CIApplicationFactory application_factory;

/* The initialization callback the runtime passes to Application.Start: a WinRT delegate is an IUnknown with one Invoke slot. */
struct callback;

struct callback_vtbl {
	HRESULT (WINAPI *query)(struct callback *self, REFIID iid, void **out);
	ULONG (WINAPI *addref)(struct callback *self);
	ULONG (WINAPI *release)(struct callback *self);
	HRESULT (WINAPI *invoke)(struct callback *self, void *parameters);
};

struct callback {
	const struct callback_vtbl *lpVtbl;
	LONG refs;
	LONG invoked;
	LONG parameters_ok;
};

static HRESULT WINAPI callback_query(struct callback *self, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = self;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI callback_addref(struct callback *self) { return (ULONG)InterlockedIncrement(&self->refs); }
static ULONG WINAPI callback_release(struct callback *self) { return (ULONG)InterlockedDecrement(&self->refs); }

static HRESULT WINAPI callback_invoke(struct callback *self, void *parameters)
{
	IInspectable *params = (IInspectable *)parameters;
	IInspectable *queried = NULL;
	if (!params) return E_POINTER;
	if (params->lpVtbl->QueryInterface(params, &IID__x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParams,
		(void **)&queried) != S_OK || !queried) return E_NOINTERFACE;
	queried->lpVtbl->Release(queried);
	InterlockedIncrement(&self->parameters_ok);
	InterlockedIncrement(&self->invoked);
	return S_OK;
}

static const struct callback_vtbl callback_table = {callback_query, callback_addref, callback_release, callback_invoke};

/* The Application class of the Windows Runtime: activation, the factory and statics, a created object with the three application
 * interfaces, Application.Start giving its callback initialization parameters, and Application.Current returning the application
 * the runtime created. */
int raven_entry(void)
{
	static const WCHAR name[] = L"Windows.UI.Xaml.Application";
	static const WCHAR missing[] = L"Windows.Raven.Missing";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function RoGetActivationFactory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	if (!RoInitialize || !RoGetActivationFactory) return 20;
	if (FAILED(RoInitialize(1))) return 1;
	HSTRING class_key = NULL;
	if (WindowsCreateString(name, ARRAYSIZE(name) - 1, &class_key) != S_OK || !class_key) return 21;
	IActivationFactory *activation = NULL;
	if (RoGetActivationFactory(class_key, &IID_IActivationFactory, (void **)&activation) != S_OK || !activation) return 2;
	application_statics *statics = NULL;
	if (activation->lpVtbl->QueryInterface(activation, &IID__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics, (void **)&statics) != S_OK ||
		!statics) return 3;
	application_factory *factory = NULL;
	if (statics->lpVtbl->QueryInterface(statics, &IID__x_ABI_CWindows_CUI_CXaml_CIApplicationFactory, (void **)&factory) != S_OK ||
		!factory) return 4;
	void *inner = (void *)1, *instance = NULL;
	if (factory->lpVtbl->CreateInstance(factory, 0, (uint64_t)(uintptr_t)&inner, (uint64_t)(uintptr_t)&instance) != S_OK ||
		!instance || inner) return 5;
	application1 *application = NULL;
	if (((IUnknown *)instance)->lpVtbl->QueryInterface((IUnknown *)instance, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication,
		(void **)&application) != S_OK || !application) return 6;
	HSTRING runtime_name = NULL;
	UINT32 length = 0;
	if (application->lpVtbl->GetRuntimeClassName(application, &runtime_name) != S_OK) return 7;
	const WCHAR *text = WindowsGetStringRawBuffer(runtime_name, &length);
	if (length != ARRAYSIZE(name) - 1 || CompareStringOrdinal(text, (int)length, name, -1, FALSE) != CSTR_EQUAL) return 8;
	WindowsDeleteString(runtime_name);
	application2 *later = NULL;
	if (application->lpVtbl->QueryInterface(application, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication2, (void **)&later) != S_OK ||
		!later) return 9;
	application3 *latest = NULL;
	if (later->lpVtbl->QueryInterface(later, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication3, (void **)&latest) != S_OK ||
		!latest) return 10;
	struct callback init = {&callback_table, 1, 0, 0};
	if (statics->lpVtbl->Start(statics, (uint64_t)(uintptr_t)&init) != S_OK || !init.invoked || !init.parameters_ok)
		return 12;
	application1 *current = NULL;
	if (statics->lpVtbl->get_Current(statics, (uint64_t)(uintptr_t)&current) != S_OK || !current) return 13;
	if (current != application) return 14;
	current->lpVtbl->Release(current);
	latest->lpVtbl->Release(latest);
	later->lpVtbl->Release(later);
	application->lpVtbl->Release(application);
	((IUnknown *)instance)->lpVtbl->Release((IUnknown *)instance);
	factory->lpVtbl->Release(factory);
	statics->lpVtbl->Release(statics);
	activation->lpVtbl->Release(activation);
	HSTRING missing_key = NULL;
	if (WindowsCreateString(missing, ARRAYSIZE(missing) - 1, &missing_key) != S_OK) return 22;
	IInspectable *object = (void *)1;
	if (RoGetActivationFactory(missing_key, &IID_IActivationFactory, (void **)&object) != REGDB_E_CLASSNOTREG || object)
		return 11;
	void *restricted = (void *)1;
	if (GetRestrictedErrorInfo(&restricted) != S_FALSE || restricted) return 15;
	WindowsDeleteString(missing_key);
	WindowsDeleteString(class_key);
	return 0;
}
