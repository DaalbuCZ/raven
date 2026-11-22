/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.System.Diagnostics.SystemDiagnosticInfo class of the Windows Runtime.
 *
 * A packaged application asks the diagnostics what the machine's memory and processors are doing: the factory serves
 * the statics interface a guest's activation query names (GetForCurrentSystem and the architecture questions), and the
 * instance answers the default interface's usage properties. The usage objects hand out reports whose values come from
 * the host the way GlobalMemoryStatusEx and GetSystemTimes give them: the memory report is the machine's commit and
 * physical memory, the processor report the kernel, user and idle time since the system started. Every member a guest
 * reaches is named in Raven's trace, so the next run tells which ones the application reads. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include "windows_system_diagnostics.h"

typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics static1;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStaticsVtbl static1Vtbl;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2 static2;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2Vtbl static2Vtbl;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfo diagnostic1;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoVtbl diagnostic1Vtbl;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsage cpu_usage;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageVtbl cpu_usageVtbl;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport cpu_report;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReportVtbl cpu_reportVtbl;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsage memory_usage;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageVtbl memory_usageVtbl;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport memory_report;
typedef __x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReportVtbl memory_reportVtbl;

struct diagnostic_object;
struct memory_usage_object;
struct memory_report_object;
struct cpu_usage_object;
struct cpu_report_object;
HRESULT raven_xaml_memory_usage_create(struct memory_usage_object **out);
HRESULT raven_xaml_memory_report_create(struct memory_report_object **out);
HRESULT raven_xaml_cpu_usage_create(struct cpu_usage_object **out);
HRESULT raven_xaml_cpu_report_create(struct cpu_report_object **out);

static const WCHAR class_name[] = L"Windows.System.Diagnostics.SystemDiagnosticInfo";
static const WCHAR cpu_usage_text[] = L"Windows.System.Diagnostics.SystemCpuUsage";
static const WCHAR cpu_report_text[] = L"Windows.System.Diagnostics.SystemCpuUsageReport";
static const WCHAR memory_usage_text[] = L"Windows.System.Diagnostics.SystemMemoryUsage";
static const WCHAR memory_report_text[] = L"Windows.System.Diagnostics.SystemMemoryUsageReport";

/* Windows.System.ProcessorArchitecture's enumeration, as the metadata gives it. */
enum { architecture_x86 = 0, architecture_arm = 5, architecture_x64 = 9, architecture_neutral = 11,
	architecture_arm64 = 12, architecture_x86_on_arm64 = 14, architecture_unknown = 65535 };

/* The one member-name trace ravenhost.dll carries to the host, as raven-shell's activation trace does; nothing is
 * emitted when the host is absent. */
static void trace_member(const char *member)
{
	static void (WINAPI *raven_trace)(const char *);
	static BOOL looked;
	char line[160] = "raven-xaml: SystemDiagnosticInfo.";
	size_t at = 33, length;
	if (!looked) {
		raven_trace = (void (WINAPI *)(const char *))GetProcAddress(LoadLibraryW(L"ravenhost.dll"), "RavenTrace");
		looked = TRUE;
	}
	if (!raven_trace)
		return;
	for (length = 0; member[length] && at < ARRAYSIZE(line) - 1; ++length)
		line[at++] = member[length];
	line[at] = 0;
	raven_trace(line);
}

static HRESULT inspectable_iids(ULONG *count, IID **iids)
{
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}

static HRESULT inspectable_trust(TrustLevel *level)
{
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

static HRESULT runtime_class_name(HSTRING *name, const WCHAR *text)
{
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), name);
}

static int equal_iid(REFIID left, const IID *right)
{
	return left->Data1 == right->Data1 && left->Data2 == right->Data2 && left->Data3 == right->Data3 &&
		left->Data4[0] == right->Data4[0] && left->Data4[1] == right->Data4[1] &&
		left->Data4[2] == right->Data4[2] && left->Data4[3] == right->Data4[3] &&
		left->Data4[4] == right->Data4[4] && left->Data4[5] == right->Data4[5] &&
		left->Data4[6] == right->Data4[6] && left->Data4[7] == right->Data4[7];
}

/* The query, refcounting and release the usage and report objects share; each object's vtable sits first and its
 * reference count directly after, so the release frees the block the object opened. */
static HRESULT usage_query(void *iface, REFIID iid, void **out, const IID *own, LONG *refs)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) && !equal_iid(iid, own))
		return E_NOINTERFACE;
	InterlockedIncrement(refs);
	*out = iface;
	return S_OK;
}

static ULONG usage_addref(LONG *refs)
{
	return (ULONG)InterlockedIncrement(refs);
}

static ULONG usage_release(LONG *refs, void *block)
{
	const LONG refs_left = InterlockedDecrement(refs);
	if (!refs_left) {
		HeapFree(GetProcessHeap(), 0, block);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs_left;
}

/* ---- The system object the statics hand out: the default interface's usage properties. ---- */

struct diagnostic_object;
struct memory_usage_object;
struct cpu_usage_object;

struct diagnostic_object {
	const diagnostic1Vtbl *lpVtbl;
	LONG refs;
};

HRESULT raven_xaml_memory_usage_create(struct memory_usage_object **out);
HRESULT raven_xaml_cpu_usage_create(struct cpu_usage_object **out);

static HRESULT WINAPI diagnostic_query(diagnostic1 *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfo))
		return E_NOINTERFACE;
	InterlockedIncrement(&((struct diagnostic_object *)iface)->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI diagnostic_addref(diagnostic1 *iface)
{
	return (ULONG)InterlockedIncrement(&((struct diagnostic_object *)iface)->refs);
}

static ULONG WINAPI diagnostic_release(diagnostic1 *iface)
{
	struct diagnostic_object *self = (struct diagnostic_object *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		HeapFree(GetProcessHeap(), 0, self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI diagnostic_iids(diagnostic1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI diagnostic_name(diagnostic1 *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, class_name);
}

static HRESULT WINAPI diagnostic_trust(diagnostic1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI diagnostic_memory(diagnostic1 *iface, uint64_t out)
{
	struct memory_usage_object *usage;
	HRESULT result;
	(void)iface;
	if (!out)
		return E_POINTER;
	trace_member("get_MemoryUsage");
	if (FAILED(result = raven_xaml_memory_usage_create(&usage)))
		return result;
	*(void **)(uintptr_t)out = usage;
	return S_OK;
}

static HRESULT WINAPI diagnostic_cpu(diagnostic1 *iface, uint64_t out)
{
	struct cpu_usage_object *usage;
	HRESULT result;
	(void)iface;
	if (!out)
		return E_POINTER;
	trace_member("get_CpuUsage");
	if (FAILED(result = raven_xaml_cpu_usage_create(&usage)))
		return result;
	*(void **)(uintptr_t)out = usage;
	return S_OK;
}

static const diagnostic1Vtbl diagnostic_table = {
	.QueryInterface = diagnostic_query,
	.AddRef = diagnostic_addref,
	.Release = diagnostic_release,
	.GetIids = diagnostic_iids,
	.GetRuntimeClassName = diagnostic_name,
	.GetTrustLevel = diagnostic_trust,
	.get_MemoryUsage = diagnostic_memory,
	.get_CpuUsage = diagnostic_cpu,
};

static struct diagnostic_object *create_diagnostic(void)
{
	struct diagnostic_object *object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *object);
	if (!object)
		return NULL;
	object->lpVtbl = &diagnostic_table;
	object->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	return object;
}

/* ---- The memory usage object and its report. ---- */

struct memory_usage_object {
	const memory_usageVtbl *lpVtbl;
	LONG refs;
};

struct memory_report_object {
	const memory_reportVtbl *lpVtbl;
	LONG refs;
	UINT64 total;
	UINT64 available;
	UINT64 committed;
};

/* The memory report reads the host's physical and commit memory, the way the manager's usage does. */
static void memory_values(UINT64 *total, UINT64 *available, UINT64 *committed)
{
	MEMORYSTATUSEX status;
	status.dwLength = sizeof status;
	if (!GlobalMemoryStatusEx(&status)) {
		*total = 512ull << 20;
		*available = 384ull << 20;
		*committed = 128ull << 20;
		return;
	}
	*total = status.ullTotalPhys;
	*available = status.ullAvailPhys;
	*committed = status.ullTotalPageFile > status.ullAvailPageFile ? status.ullTotalPageFile - status.ullAvailPageFile : 0;
}

static HRESULT report_u64(UINT64 value, uint64_t out, const char *member)
{
	if (!out)
		return E_POINTER;
	trace_member(member);
	*(UINT64 *)(uintptr_t)out = value;
	return S_OK;
}

static HRESULT WINAPI memory_usage_query(memory_usage *iface, REFIID iid, void **out)
{
	return usage_query(iface, iid, out, &IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsage,
		&((struct memory_usage_object *)iface)->refs);
}

static ULONG WINAPI memory_usage_addref(memory_usage *iface)
{
	return usage_addref(&((struct memory_usage_object *)iface)->refs);
}

static ULONG WINAPI memory_usage_release(memory_usage *iface)
{
	return usage_release(&((struct memory_usage_object *)iface)->refs, iface);
}

static HRESULT WINAPI memory_usage_iids(memory_usage *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI memory_usage_name(memory_usage *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, memory_usage_text);
}

static HRESULT WINAPI memory_usage_trust(memory_usage *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI memory_usage_report(memory_usage *iface, uint64_t out)
{
	struct memory_report_object *report;
	HRESULT result;
	(void)iface;
	if (!out)
		return E_POINTER;
	trace_member("GetReport");
	if (FAILED(result = raven_xaml_memory_report_create(&report)))
		return result;
	*(void **)(uintptr_t)out = report;
	return S_OK;
}

static const memory_usageVtbl memory_usage_table = {
	.QueryInterface = memory_usage_query,
	.AddRef = memory_usage_addref,
	.Release = memory_usage_release,
	.GetIids = memory_usage_iids,
	.GetRuntimeClassName = memory_usage_name,
	.GetTrustLevel = memory_usage_trust,
	.GetReport = memory_usage_report,
};

HRESULT raven_xaml_memory_usage_create(struct memory_usage_object **out)
{
	struct memory_usage_object *usage;
	if (!out)
		return E_POINTER;
	*out = NULL;
	usage = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *usage);
	if (!usage)
		return E_OUTOFMEMORY;
	usage->lpVtbl = &memory_usage_table;
	usage->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	*out = usage;
	return S_OK;
}

static HRESULT WINAPI memory_report_query(memory_report *iface, REFIID iid, void **out)
{
	return usage_query(iface, iid, out, &IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemMemoryUsageReport,
		&((struct memory_report_object *)iface)->refs);
}

static ULONG WINAPI memory_report_addref(memory_report *iface)
{
	return usage_addref(&((struct memory_report_object *)iface)->refs);
}

static ULONG WINAPI memory_report_release(memory_report *iface)
{
	return usage_release(&((struct memory_report_object *)iface)->refs, iface);
}

static HRESULT WINAPI memory_report_iids(memory_report *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI memory_report_name(memory_report *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, memory_report_text);
}

static HRESULT WINAPI memory_report_trust(memory_report *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI memory_report_total(memory_report *iface, uint64_t out)
{
	(void)iface;
	return report_u64(((struct memory_report_object *)iface)->total, out, "get_TotalPhysicalSizeInBytes");
}

static HRESULT WINAPI memory_report_available(memory_report *iface, uint64_t out)
{
	(void)iface;
	return report_u64(((struct memory_report_object *)iface)->available, out, "get_AvailableSizeInBytes");
}

static HRESULT WINAPI memory_report_committed(memory_report *iface, uint64_t out)
{
	(void)iface;
	return report_u64(((struct memory_report_object *)iface)->committed, out, "get_CommittedSizeInBytes");
}

static const memory_reportVtbl memory_report_table = {
	.QueryInterface = memory_report_query,
	.AddRef = memory_report_addref,
	.Release = memory_report_release,
	.GetIids = memory_report_iids,
	.GetRuntimeClassName = memory_report_name,
	.GetTrustLevel = memory_report_trust,
	.get_TotalPhysicalSizeInBytes = memory_report_total,
	.get_AvailableSizeInBytes = memory_report_available,
	.get_CommittedSizeInBytes = memory_report_committed,
};

HRESULT raven_xaml_memory_report_create(struct memory_report_object **out)
{
	struct memory_report_object *report;
	if (!out)
		return E_POINTER;
	*out = NULL;
	report = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *report);
	if (!report)
		return E_OUTOFMEMORY;
	report->lpVtbl = &memory_report_table;
	report->refs = 1;
	memory_values(&report->total, &report->available, &report->committed);
	InterlockedIncrement(&raven_xaml_objects);
	*out = report;
	return S_OK;
}

/* ---- The processor usage object and its report: the kernel, user and idle time since the system started. ---- */

struct cpu_usage_object {
	const cpu_usageVtbl *lpVtbl;
	LONG refs;
};

struct cpu_report_object {
	const cpu_reportVtbl *lpVtbl;
	LONG refs;
	UINT64 kernel;
	UINT64 user;
	UINT64 idle;
};

static HRESULT WINAPI cpu_usage_query(cpu_usage *iface, REFIID iid, void **out)
{
	return usage_query(iface, iid, out, &IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsage,
		&((struct cpu_usage_object *)iface)->refs);
}

static ULONG WINAPI cpu_usage_addref(cpu_usage *iface)
{
	return usage_addref(&((struct cpu_usage_object *)iface)->refs);
}

static ULONG WINAPI cpu_usage_release(cpu_usage *iface)
{
	return usage_release(&((struct cpu_usage_object *)iface)->refs, iface);
}

static HRESULT WINAPI cpu_usage_iids(cpu_usage *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI cpu_usage_name(cpu_usage *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, cpu_usage_text);
}

static HRESULT WINAPI cpu_usage_trust(cpu_usage *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI cpu_usage_report(cpu_usage *iface, uint64_t out)
{
	struct cpu_report_object *report;
	HRESULT result;
	(void)iface;
	if (!out)
		return E_POINTER;
	trace_member("GetReport");
	if (FAILED(result = raven_xaml_cpu_report_create(&report)))
		return result;
	*(void **)(uintptr_t)out = report;
	return S_OK;
}

static const cpu_usageVtbl cpu_usage_table = {
	.QueryInterface = cpu_usage_query,
	.AddRef = cpu_usage_addref,
	.Release = cpu_usage_release,
	.GetIids = cpu_usage_iids,
	.GetRuntimeClassName = cpu_usage_name,
	.GetTrustLevel = cpu_usage_trust,
	.GetReport = cpu_usage_report,
};

HRESULT raven_xaml_cpu_usage_create(struct cpu_usage_object **out)
{
	struct cpu_usage_object *usage;
	if (!out)
		return E_POINTER;
	*out = NULL;
	usage = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *usage);
	if (!usage)
		return E_OUTOFMEMORY;
	usage->lpVtbl = &cpu_usage_table;
	usage->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	*out = usage;
	return S_OK;
}

static HRESULT WINAPI cpu_report_query(cpu_report *iface, REFIID iid, void **out)
{
	return usage_query(iface, iid, out, &IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemCpuUsageReport,
		&((struct cpu_report_object *)iface)->refs);
}

static ULONG WINAPI cpu_report_addref(cpu_report *iface)
{
	return usage_addref(&((struct cpu_report_object *)iface)->refs);
}

static ULONG WINAPI cpu_report_release(cpu_report *iface)
{
	return usage_release(&((struct cpu_report_object *)iface)->refs, iface);
}

static HRESULT WINAPI cpu_report_iids(cpu_report *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI cpu_report_name(cpu_report *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, cpu_report_text);
}

static HRESULT WINAPI cpu_report_trust(cpu_report *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI cpu_report_kernel(cpu_report *iface, uint64_t out)
{
	(void)iface;
	return report_u64(((struct cpu_report_object *)iface)->kernel, out, "get_KernelTime");
}

static HRESULT WINAPI cpu_report_user(cpu_report *iface, uint64_t out)
{
	(void)iface;
	return report_u64(((struct cpu_report_object *)iface)->user, out, "get_UserTime");
}

static HRESULT WINAPI cpu_report_idle(cpu_report *iface, uint64_t out)
{
	(void)iface;
	return report_u64(((struct cpu_report_object *)iface)->idle, out, "get_IdleTime");
}

static const cpu_reportVtbl cpu_report_table = {
	.QueryInterface = cpu_report_query,
	.AddRef = cpu_report_addref,
	.Release = cpu_report_release,
	.GetIids = cpu_report_iids,
	.GetRuntimeClassName = cpu_report_name,
	.GetTrustLevel = cpu_report_trust,
	.get_KernelTime = cpu_report_kernel,
	.get_UserTime = cpu_report_user,
	.get_IdleTime = cpu_report_idle,
};

HRESULT raven_xaml_cpu_report_create(struct cpu_report_object **out)
{
	struct cpu_report_object *report;
	FILETIME idle, kernel, user;
	if (!out)
		return E_POINTER;
	*out = NULL;
	report = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *report);
	if (!report)
		return E_OUTOFMEMORY;
	report->lpVtbl = &cpu_report_table;
	report->refs = 1;
	/* GetSystemTimes gives the time each state has held since the system started, in 100-nanosecond units the
	 * TimeSpan the report's members carry. */
	if (GetSystemTimes(&idle, &kernel, &user)) {
		report->idle = ((UINT64)idle.dwHighDateTime << 32) | idle.dwLowDateTime;
		report->kernel = ((UINT64)kernel.dwHighDateTime << 32) | kernel.dwLowDateTime;
		report->user = ((UINT64)user.dwHighDateTime << 32) | user.dwLowDateTime;
	}
	InterlockedIncrement(&raven_xaml_objects);
	*out = report;
	return S_OK;
}

/* ---- The activation factory: the statics the guest's activation query asks the class for. ---- */

struct factory;
struct part_activation { const IActivationFactoryVtbl *lpVtbl; struct factory *owner; };
struct part1 { const static1Vtbl *lpVtbl; struct factory *owner; };
struct part2 { const static2Vtbl *lpVtbl; struct factory *owner; };

struct factory {
	struct part_activation activation;
	struct part1 statics1;
	struct part2 statics2;
	LONG refs;
};

static HRESULT factory_query(struct factory *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory))
		*out = &self->activation;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics))
		*out = &self->statics1;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CSystem_CDiagnostics_CISystemDiagnosticInfoStatics2))
		*out = &self->statics2;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

#define PART_BOILERPLATE(part, iface) \
	static HRESULT WINAPI part##_query(iface *iface_unused, REFIID iid, void **out) \
	{ return factory_query(((struct part *)iface_unused)->owner, iid, out); } \
	static ULONG WINAPI part##_addref(iface *iface_unused) \
	{ return (ULONG)InterlockedIncrement(&((struct part *)iface_unused)->owner->refs); } \
	static ULONG WINAPI part##_release(iface *iface_unused) \
	{ return (ULONG)InterlockedDecrement(&((struct part *)iface_unused)->owner->refs); } \
	static HRESULT WINAPI part##_iids(iface *iface_unused, ULONG *count, IID **iids) \
	{ (void)iface_unused; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI part##_name(iface *iface_unused, HSTRING *name) \
	{ (void)iface_unused; return runtime_class_name(name, class_name); } \
	static HRESULT WINAPI part##_trust(iface *iface_unused, TrustLevel *level) \
	{ (void)iface_unused; return inspectable_trust(level); }

PART_BOILERPLATE(part1, static1)
PART_BOILERPLATE(part2, static2)

static HRESULT WINAPI activation_query(IActivationFactory *iface, REFIID iid, void **out)
{
	return factory_query(((struct part_activation *)iface)->owner, iid, out);
}

static ULONG WINAPI activation_addref(IActivationFactory *iface)
{
	return (ULONG)InterlockedIncrement(&((struct part_activation *)iface)->owner->refs);
}

static ULONG WINAPI activation_release(IActivationFactory *iface)
{
	return (ULONG)InterlockedDecrement(&((struct part_activation *)iface)->owner->refs);
}

static HRESULT WINAPI activation_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI activation_name(IActivationFactory *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, class_name);
}

static HRESULT WINAPI activation_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI activation_activate(IActivationFactory *iface, IInspectable **out)
{
	struct diagnostic_object *object;
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	trace_member("ActivateInstance");
	if (!(object = create_diagnostic()))
		return E_OUTOFMEMORY;
	*out = (IInspectable *)(diagnostic1 *)object;
	return S_OK;
}

static HRESULT WINAPI statics1_current_system(static1 *iface, uint64_t out)
{
	struct diagnostic_object *object;
	(void)iface;
	if (!out)
		return E_POINTER;
	trace_member("GetForCurrentSystem");
	if (!(object = create_diagnostic()))
		return E_OUTOFMEMORY;
	*(void **)(uintptr_t)out = object;
	return S_OK;
}

static HRESULT WINAPI statics2_supported(static2 *iface, int32_t architecture, uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	trace_member("IsArchitectureSupported");
	/* The host runs the machine's own width, and its architecture is the one supported. */
	*(boolean *)(uintptr_t)out = architecture == architecture_x64 || architecture == architecture_x86 ||
		architecture == architecture_neutral;
	return S_OK;
}

static HRESULT WINAPI statics2_preferred(static2 *iface, uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	trace_member("get_PreferredArchitecture");
	/* The width the serving library was built for is the width the process runs. */
	*(int32_t *)(uintptr_t)out = sizeof(void *) == 8 ? architecture_x64 : architecture_x86;
	return S_OK;
}

static const static1Vtbl statics1_table = {
	.QueryInterface = part1_query,
	.AddRef = part1_addref,
	.Release = part1_release,
	.GetIids = part1_iids,
	.GetRuntimeClassName = part1_name,
	.GetTrustLevel = part1_trust,
	.GetForCurrentSystem = statics1_current_system,
};

static const static2Vtbl statics2_table = {
	.QueryInterface = part2_query,
	.AddRef = part2_addref,
	.Release = part2_release,
	.GetIids = part2_iids,
	.GetRuntimeClassName = part2_name,
	.GetTrustLevel = part2_trust,
	.IsArchitectureSupported = statics2_supported,
	.get_PreferredArchitecture = statics2_preferred,
};

static const IActivationFactoryVtbl activation_table = {
	.QueryInterface = activation_query,
	.AddRef = activation_addref,
	.Release = activation_release,
	.GetIids = activation_iids,
	.GetRuntimeClassName = activation_name,
	.GetTrustLevel = activation_trust,
	.ActivateInstance = activation_activate,
};

static struct factory factory_instance = {
	{&activation_table, &factory_instance},
	{&statics1_table, &factory_instance},
	{&statics2_table, &factory_instance},
	1,
};

HRESULT raven_xaml_system_diagnostics_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (length != ARRAYSIZE(class_name) - 1 || CompareStringOrdinal(text, (int)length, class_name, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	InterlockedIncrement(&factory_instance.refs);
	*out = (IActivationFactory *)&factory_instance.activation;
	return S_OK;
}
