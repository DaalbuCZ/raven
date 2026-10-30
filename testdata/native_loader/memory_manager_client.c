#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include "windows_system_memorymanager.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);

typedef __x_ABI_CWindows_CSystem_CIMemoryManagerStatics statics1;
typedef __x_ABI_CWindows_CSystem_CIMemoryManagerStatics2 statics2;
typedef __x_ABI_CWindows_CSystem_CIMemoryManagerStatics3 statics3;
typedef __x_ABI_CWindows_CSystem_CIMemoryManagerStatics4 statics4;
typedef __x_ABI_CWindows_CSystem_CIAppMemoryReport app_report;
typedef __x_ABI_CWindows_CSystem_CIAppMemoryReport2 app_report2;
typedef __x_ABI_CWindows_CSystem_CIProcessMemoryReport process_report;

/* The application's memory budget: usage and limit are reported, the level follows the ratio, and the later statics are present. */
int raven_entry(void)
{
	static const WCHAR name[] = L"Windows.System.MemoryManager";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function RoGetActivationFactory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	if (!RoInitialize || !RoGetActivationFactory) return 20;
	if (FAILED(RoInitialize(1))) return 1;
	HSTRING class_key = NULL;
	if (WindowsCreateString(name, ARRAYSIZE(name) - 1, &class_key) != S_OK || !class_key) return 21;
	IActivationFactory *activation = NULL;
	if (RoGetActivationFactory(class_key, &IID_IActivationFactory, (void **)&activation) != S_OK || !activation) return 2;
	statics1 *first = NULL;
	if (activation->lpVtbl->QueryInterface(activation, &IID__x_ABI_CWindows_CSystem_CIMemoryManagerStatics, (void **)&first) != S_OK ||
		!first) return 3;
	uint64_t usage = 0, limit = 0;
	int32_t level = -1;
	if (first->lpVtbl->get_AppMemoryUsage(first, (uint64_t)(uintptr_t)&usage) != S_OK || !usage) return 4;
	if (first->lpVtbl->get_AppMemoryUsageLimit(first, (uint64_t)(uintptr_t)&limit) != S_OK || !limit || limit < usage) return 5;
	if (first->lpVtbl->get_AppMemoryUsageLevel(first, (uint64_t)(uintptr_t)&level) != S_OK || level < 0 || level > 2) return 6;
	statics2 *second = NULL;
	statics3 *third = NULL;
	statics4 *fourth = NULL;
	if (first->lpVtbl->QueryInterface(first, &IID__x_ABI_CWindows_CSystem_CIMemoryManagerStatics2, (void **)&second) != S_OK ||
		!second) return 7;
	app_report *app_usage = NULL;
	if (second->lpVtbl->GetAppMemoryReport(second, (uint64_t)(uintptr_t)&app_usage) != S_OK || !app_usage) return 13;
	uint64_t total_limit = 0;
	if (app_usage->lpVtbl->get_TotalCommitLimit(app_usage, (uint64_t)(uintptr_t)&total_limit) != S_OK || total_limit != limit) return 14;
	app_report2 *expected_usage = NULL;
	if (app_usage->lpVtbl->QueryInterface(app_usage, &IID__x_ABI_CWindows_CSystem_CIAppMemoryReport2, (void **)&expected_usage) != S_OK ||
		!expected_usage) return 15;
	uint64_t expected_limit = 0;
	if (expected_usage->lpVtbl->get_ExpectedTotalCommitLimit(expected_usage, (uint64_t)(uintptr_t)&expected_limit) != S_OK ||
		expected_limit != limit) return 16;
	process_report *process_usage = NULL;
	if (second->lpVtbl->GetProcessMemoryReport(second, (uint64_t)(uintptr_t)&process_usage) != S_OK || !process_usage) return 17;
	uint64_t working_set = 0;
	if (process_usage->lpVtbl->get_PrivateWorkingSetUsage(process_usage, (uint64_t)(uintptr_t)&working_set) != S_OK || !working_set) return 18;
	if (first->lpVtbl->QueryInterface(first, &IID__x_ABI_CWindows_CSystem_CIMemoryManagerStatics3, (void **)&third) != S_OK ||
		!third) return 8;
	if (first->lpVtbl->QueryInterface(first, &IID__x_ABI_CWindows_CSystem_CIMemoryManagerStatics4, (void **)&fourth) != S_OK ||
		!fourth) return 9;
	uint64_t expected = 0;
	if (fourth->lpVtbl->get_ExpectedAppMemoryUsageLimit(fourth, (uint64_t)(uintptr_t)&expected) != S_OK || expected != limit) return 10;
	HSTRING runtime_name = NULL;
	UINT32 length = 0;
	if (first->lpVtbl->GetRuntimeClassName(first, &runtime_name) != S_OK) return 11;
	const WCHAR *text = WindowsGetStringRawBuffer(runtime_name, &length);
	if (length != ARRAYSIZE(name) - 1 || CompareStringOrdinal(text, (int)length, name, -1, FALSE) != CSTR_EQUAL) return 12;
	WindowsDeleteString(runtime_name);
	process_usage->lpVtbl->Release(process_usage);
	expected_usage->lpVtbl->Release(expected_usage);
	app_usage->lpVtbl->Release(app_usage);
	fourth->lpVtbl->Release(fourth);
	third->lpVtbl->Release(third);
	second->lpVtbl->Release(second);
	first->lpVtbl->Release(first);
	activation->lpVtbl->Release(activation);
	WindowsDeleteString(class_key);
	return 0;
}
