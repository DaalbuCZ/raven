#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include "windows_system_diagnostics.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);

typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics statics1;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2 statics2;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfo diagnostic1;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsage cpu_usage;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport cpu_report;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsage memory_usage;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport memory_report;

/* The system's diagnostics: the activation asks the statics interface the guest names, the current system hands the
 * machine's object, and its usage objects give the memory and processor reports. */
int raven_entry(void)
{
	static const WCHAR name[] = L"Windows.System.Diagnostics.SystemDiagnosticInfo";
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
	if (activation->lpVtbl->QueryInterface(activation, &IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics,
		(void **)&first) != S_OK || !first) return 3;
	diagnostic1 *system = NULL;
	if (first->lpVtbl->GetForCurrentSystem(first, (uint64_t)(uintptr_t)&system) != S_OK || !system) return 4;
	memory_usage *memory = NULL;
	if (system->lpVtbl->get_MemoryUsage(system, (uint64_t)(uintptr_t)&memory) != S_OK || !memory) return 5;
	memory_report *memory_usage_report = NULL;
	if (memory->lpVtbl->GetReport(memory, (uint64_t)(uintptr_t)&memory_usage_report) != S_OK || !memory_usage_report) return 6;
	UINT64 total = 0, available = 0, committed = 0;
	if (memory_usage_report->lpVtbl->get_TotalPhysicalSizeInBytes(memory_usage_report, (uint64_t)(uintptr_t)&total) != S_OK ||
		!total) return 7;
	if (memory_usage_report->lpVtbl->get_AvailableSizeInBytes(memory_usage_report, (uint64_t)(uintptr_t)&available) != S_OK ||
		available > total) return 8;
	if (memory_usage_report->lpVtbl->get_CommittedSizeInBytes(memory_usage_report, (uint64_t)(uintptr_t)&committed) != S_OK) return 9;
	cpu_usage *cpu = NULL;
	if (system->lpVtbl->get_CpuUsage(system, (uint64_t)(uintptr_t)&cpu) != S_OK || !cpu) return 10;
	cpu_report *cpu_usage_report = NULL;
	if (cpu->lpVtbl->GetReport(cpu, (uint64_t)(uintptr_t)&cpu_usage_report) != S_OK || !cpu_usage_report) return 11;
	UINT64 kernel = 0, user = 0, idle = 0;
	/* The host models the kernel time as zero (process_info_api's GetSystemTimes), so the answer only needs to come. */
	if (cpu_usage_report->lpVtbl->get_KernelTime(cpu_usage_report, (uint64_t)(uintptr_t)&kernel) != S_OK) return 12;
	if (cpu_usage_report->lpVtbl->get_UserTime(cpu_usage_report, (uint64_t)(uintptr_t)&user) != S_OK) return 13;
	if (cpu_usage_report->lpVtbl->get_IdleTime(cpu_usage_report, (uint64_t)(uintptr_t)&idle) != S_OK) return 14;
	statics2 *second = NULL;
	if (first->lpVtbl->QueryInterface(first, &IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2,
		(void **)&second) != S_OK || !second) return 15;
	boolean supported = FALSE;
	if (second->lpVtbl->IsArchitectureSupported(second, 9 /* ProcessorArchitecture.X64 */, (uint64_t)(uintptr_t)&supported) != S_OK ||
		!supported) return 16;
	int32_t preferred = 0;
	if (second->lpVtbl->get_PreferredArchitecture(second, (uint64_t)(uintptr_t)&preferred) != S_OK) return 17;
	HSTRING runtime_name = NULL;
	UINT32 length = 0;
	if (memory_usage_report->lpVtbl->GetRuntimeClassName(memory_usage_report, &runtime_name) != S_OK) return 18;
	static const WCHAR report_name[] = L"Windows.System.Diagnostics.SystemMemoryUsageReport";
	const WCHAR *text = WindowsGetStringRawBuffer(runtime_name, &length);
	if (length != ARRAYSIZE(report_name) - 1 || CompareStringOrdinal(text, (int)length, report_name, -1, FALSE) != CSTR_EQUAL) return 19;
	WindowsDeleteString(runtime_name);
	second->lpVtbl->Release(second);
	cpu_usage_report->lpVtbl->Release(cpu_usage_report);
	cpu->lpVtbl->Release(cpu);
	memory_usage_report->lpVtbl->Release(memory_usage_report);
	memory->lpVtbl->Release(memory);
	system->lpVtbl->Release(system);
	first->lpVtbl->Release(first);
	activation->lpVtbl->Release(activation);
	WindowsDeleteString(class_key);
	return 0;
}
