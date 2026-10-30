#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <windows.ui.core.h>
#include "windows_ui_xaml_window.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);

typedef __x_ABI_CWindows_CUI_CXaml_CIWindow window1;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindow2 window2;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindow3 window3;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindow4 window4;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindowStatics window_statics;
typedef __x_ABI_CWindows_CUI_CCore_CICoreWindow core_window;
typedef __x_ABI_CWindows_CUI_CCore_CICoreDispatcher core_dispatcher;
typedef __x_ABI_CWindows_CFoundation_CRect xaml_rect;

/* Window.Current of the calling thread: one window per thread, hidden until Activate, with a CoreWindow and its dispatcher, the
 * later interfaces, content, class name and a new window after Close. */
int raven_entry(void)
{
	static const WCHAR name[] = L"Windows.UI.Xaml.Window";
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
	window_statics *statics = NULL;
	if (activation->lpVtbl->QueryInterface(activation, &IID__x_ABI_CWindows_CUI_CXaml_CIWindowStatics, (void **)&statics) != S_OK ||
		!statics) return 3;
	window1 *first = NULL, *second = NULL;
	if (statics->lpVtbl->get_Current(statics, (uint64_t)(uintptr_t)&first) != S_OK || !first) return 4;
	if (statics->lpVtbl->get_Current(statics, (uint64_t)(uintptr_t)&second) != S_OK || second != first) return 5;
	boolean visible = TRUE;
	if (first->lpVtbl->get_Visible(first, (uint64_t)(uintptr_t)&visible) != S_OK || visible) return 6;
	xaml_rect bounds = {0, 0, 0, 0};
	if (first->lpVtbl->get_Bounds(first, (uint64_t)(uintptr_t)&bounds) != S_OK) return 7;
	core_window *core = NULL;
	if (first->lpVtbl->get_CoreWindow(first, (uint64_t)(uintptr_t)&core) != S_OK || !core) return 8;
	core_dispatcher *dispatcher = NULL;
	if (first->lpVtbl->get_Dispatcher(first, (uint64_t)(uintptr_t)&dispatcher) != S_OK || !dispatcher) return 9;
	window2 *later = NULL;
	if (first->lpVtbl->QueryInterface(first, &IID__x_ABI_CWindows_CUI_CXaml_CIWindow2, (void **)&later) != S_OK || !later) return 10;
	window3 *third = NULL;
	if (first->lpVtbl->QueryInterface(first, &IID__x_ABI_CWindows_CUI_CXaml_CIWindow3, (void **)&third) != S_OK || !third) return 11;
	window4 *fourth = NULL;
	if (first->lpVtbl->QueryInterface(first, &IID__x_ABI_CWindows_CUI_CXaml_CIWindow4, (void **)&fourth) != S_OK || !fourth) return 12;
	if (first->lpVtbl->Activate(first) != S_OK) return 13;
	if (first->lpVtbl->get_Visible(first, (uint64_t)(uintptr_t)&visible) != S_OK || !visible) return 14;
	if (first->lpVtbl->put_Content(first, 0) != S_OK) return 15;
	void *content = (void *)1;
	if (first->lpVtbl->get_Content(first, (uint64_t)(uintptr_t)&content) != S_OK || content) return 16;
	HSTRING runtime_name = NULL;
	UINT32 length = 0;
	if (first->lpVtbl->GetRuntimeClassName(first, &runtime_name) != S_OK) return 17;
	const WCHAR *text = WindowsGetStringRawBuffer(runtime_name, &length);
	if (length != ARRAYSIZE(name) - 1 || CompareStringOrdinal(text, (int)length, name, -1, FALSE) != CSTR_EQUAL) return 18;
	WindowsDeleteString(runtime_name);
	if (first->lpVtbl->Close(first) != S_OK) return 19;
	window1 *replacement = NULL;
	if (statics->lpVtbl->get_Current(statics, (uint64_t)(uintptr_t)&replacement) != S_OK || !replacement ||
		replacement == first) return 22;
	replacement->lpVtbl->Release(replacement);
	fourth->lpVtbl->Release(fourth);
	third->lpVtbl->Release(third);
	later->lpVtbl->Release(later);
	dispatcher->lpVtbl->Release(dispatcher);
	core->lpVtbl->Release(core);
	second->lpVtbl->Release(second);
	first->lpVtbl->Release(first);
	statics->lpVtbl->Release(statics);
	activation->lpVtbl->Release(activation);
	HSTRING missing_key = NULL;
	if (WindowsCreateString(missing, ARRAYSIZE(missing) - 1, &missing_key) != S_OK) return 23;
	IInspectable *object = (void *)1;
	if (RoGetActivationFactory(missing_key, &IID_IActivationFactory, (void **)&object) != REGDB_E_CLASSNOTREG || object)
		return 24;
	WindowsDeleteString(missing_key);
	WindowsDeleteString(class_key);
	return 0;
}
