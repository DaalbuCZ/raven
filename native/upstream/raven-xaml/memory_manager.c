/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.System.MemoryManager class of the Windows Runtime.
 *
 * A packaged application asks the manager how much memory it uses and what its budget is. The usage is a share of the host's
 * physical memory, as a packaged application's accounting would give; the budget is half the machine's memory, and the level
 * follows the ratio. Event subscriptions are accepted and never raised, and the report objects answer E_NOTIMPL: no observed
 * startup path has asked for them yet. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include "windows_system_memorymanager.h"

typedef __x_ABI_CWindows_CSystem_CIMemoryManagerStatics static1;
typedef __x_ABI_CWindows_CSystem_CIMemoryManagerStatics2 static2;
typedef __x_ABI_CWindows_CSystem_CIMemoryManagerStatics3 static3;
typedef __x_ABI_CWindows_CSystem_CIMemoryManagerStatics4 static4;

static const WCHAR class_name[] = L"Windows.System.MemoryManager";

enum { usage_low = 0, usage_medium = 1, usage_high = 2 };

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

static HRESULT runtime_class_name(HSTRING *name)
{
	if (!name)
		return E_POINTER;
	return WindowsCreateString(class_name, (UINT32)lstrlenW(class_name), name);
}

static int equal_iid(REFIID left, const IID *right)
{
	return left->Data1 == right->Data1 && left->Data2 == right->Data2 && left->Data3 == right->Data3 &&
		left->Data4[0] == right->Data4[0] && left->Data4[1] == right->Data4[1] &&
		left->Data4[2] == right->Data4[2] && left->Data4[3] == right->Data4[3] &&
		left->Data4[4] == right->Data4[4] && left->Data4[5] == right->Data4[5] &&
		left->Data4[6] == right->Data4[6] && left->Data4[7] == right->Data4[7];
}

#define STUB1(iface, name) static HRESULT WINAPI name(iface *This, uint64_t a1) { (void)This; (void)a1;{ raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
#define STUB2(iface, name) \
	static HRESULT WINAPI name(iface *This, uint64_t a1, uint64_t a2) { (void)This; (void)a1; (void)a2;{ raven_trace_not_implemented(__func__); return E_NOTIMPL; } }

struct factory;

struct part_activation { const IActivationFactoryVtbl *lpVtbl; struct factory *owner; };
struct part1 { const __x_ABI_CWindows_CSystem_CIMemoryManagerStaticsVtbl *lpVtbl; struct factory *owner; };
struct part2 { const __x_ABI_CWindows_CSystem_CIMemoryManagerStatics2Vtbl *lpVtbl; struct factory *owner; };
struct part3 { const __x_ABI_CWindows_CSystem_CIMemoryManagerStatics3Vtbl *lpVtbl; struct factory *owner; };
struct part4 { const __x_ABI_CWindows_CSystem_CIMemoryManagerStatics4Vtbl *lpVtbl; struct factory *owner; };

struct factory {
	struct part_activation activation;
	struct part1 statics1;
	struct part2 statics2;
	struct part3 statics3;
	struct part4 statics4;
	LONG refs;
};

static HRESULT factory_query(struct factory *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory))
		*out = &self->activation;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CSystem_CIMemoryManagerStatics))
		*out = &self->statics1;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CSystem_CIMemoryManagerStatics2))
		*out = &self->statics2;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CSystem_CIMemoryManagerStatics3))
		*out = &self->statics3;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CSystem_CIMemoryManagerStatics4))
		*out = &self->statics4;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

#define PART_BOILERPLATE(part, iface) \
	static HRESULT WINAPI part##_query(iface *iface##_unused, REFIID iid, void **out) \
	{ return factory_query(((struct part *)iface##_unused)->owner, iid, out); } \
	static ULONG WINAPI part##_addref(iface *iface##_unused) \
	{ return (ULONG)InterlockedIncrement(&((struct part *)iface##_unused)->owner->refs); } \
	static ULONG WINAPI part##_release(iface *iface##_unused) \
	{ return (ULONG)InterlockedDecrement(&((struct part *)iface##_unused)->owner->refs); } \
	static HRESULT WINAPI part##_iids(iface *iface##_unused, ULONG *count, IID **iids) \
	{ (void)iface##_unused; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI part##_name(iface *iface##_unused, HSTRING *name) \
	{ (void)iface##_unused; return runtime_class_name(name); } \
	static HRESULT WINAPI part##_trust(iface *iface##_unused, TrustLevel *level) \
	{ (void)iface##_unused; return inspectable_trust(level); }

PART_BOILERPLATE(part1, static1)
PART_BOILERPLATE(part2, static2)
PART_BOILERPLATE(part3, static3)
PART_BOILERPLATE(part4, static4)

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
	return runtime_class_name(name);
}

static HRESULT WINAPI activation_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

/* ActivateInstance has no meaning for a class that only has statics. */
static HRESULT WINAPI activation_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return 0x8000000eU;
}

static void memory_values(uint64_t *usage, uint64_t *limit)
{
	MEMORYSTATUSEX status;
	status.dwLength = sizeof status;
	if (!GlobalMemoryStatusEx(&status)) {
		*limit = 512ull << 20;
		*usage = 128ull << 20;
		return;
	}
	*limit = status.ullTotalPhys / 2;
	*usage = status.ullTotalPhys / 8;
	if (*usage > *limit)
		*usage = *limit;
}

static HRESULT WINAPI usage_get(static1 *iface, uint64_t out)
{
	uint64_t usage, limit;
	(void)iface;
	if (!out)
		return E_POINTER;
	memory_values(&usage, &limit);
	(void)limit;
	*(uint64_t *)(uintptr_t)out = usage;
	return S_OK;
}

static HRESULT WINAPI limit_get(static1 *iface, uint64_t out)
{
	uint64_t usage, limit;
	(void)iface;
	if (!out)
		return E_POINTER;
	memory_values(&usage, &limit);
	(void)usage;
	*(uint64_t *)(uintptr_t)out = limit;
	return S_OK;
}

static HRESULT WINAPI level_get(static1 *iface, uint64_t out)
{
	uint64_t usage, limit;
	int32_t level;
	(void)iface;
	if (!out)
		return E_POINTER;
	memory_values(&usage, &limit);
	level = usage * 4 < limit ? usage_low : usage * 4 > limit * 3 ? usage_high : usage_medium;
	*(int32_t *)(uintptr_t)out = level;
	return S_OK;
}

/* The token identifies a subscription that is never raised; removing it is accepted. */
static HRESULT WINAPI event_add(static1 *iface, uint64_t handler, uint64_t token)
{
	(void)iface;
	(void)handler;
	if (token)
		*(uint64_t *)(uintptr_t)token = 1;
	return S_OK;
}

static HRESULT WINAPI event_remove(static1 *iface, uint64_t token)
{
	(void)iface;
	(void)token;
	return S_OK;
}

static HRESULT WINAPI expected_limit(static4 *iface, uint64_t out)
{
	uint64_t usage, limit;
	(void)iface;
	if (!out)
		return E_POINTER;
	memory_values(&usage, &limit);
	*(uint64_t *)(uintptr_t)out = limit;
	return S_OK;
}


STUB2(static3, statics3_try_set_limit)

typedef __x_ABI_CWindows_CSystem_CIAppMemoryReport app_report;
typedef __x_ABI_CWindows_CSystem_CIAppMemoryReport2 app_report2;
typedef __x_ABI_CWindows_CSystem_CIProcessMemoryReport process_report;

struct app_report_object;
struct process_report_object;

struct app_report2_part {
	const __x_ABI_CWindows_CSystem_CIAppMemoryReport2Vtbl *lpVtbl;
	struct app_report_object *owner;
};

struct app_report_object {
	const __x_ABI_CWindows_CSystem_CIAppMemoryReportVtbl *lpVtbl;
	LONG refs;
	struct app_report2_part second;
};

struct process_report_object {
	const __x_ABI_CWindows_CSystem_CIProcessMemoryReportVtbl *lpVtbl;
	LONG refs;
};

static HRESULT write_u64(uint64_t out, uint64_t value)
{
	if (!out)
		return E_POINTER;
	*(uint64_t *)(uintptr_t)out = value;
	return S_OK;
}

static HRESULT WINAPI app_private(app_report *iface, uint64_t out)
{
	uint64_t usage, limit;
	(void)iface;
	memory_values(&usage, &limit);
	(void)limit;
	return write_u64(out, usage);
}

static HRESULT WINAPI app_peak(app_report *iface, uint64_t out)
{
	return app_private(iface, out);
}

static HRESULT WINAPI app_total_usage(app_report *iface, uint64_t out)
{
	return app_private(iface, out);
}

static HRESULT WINAPI app_total_limit(app_report *iface, uint64_t out)
{
	uint64_t usage, limit;
	(void)iface;
	memory_values(&usage, &limit);
	(void)usage;
	return write_u64(out, limit);
}

static HRESULT WINAPI app_expected_limit(app_report2 *iface, uint64_t out)
{
	uint64_t usage, limit;
	(void)iface;
	memory_values(&usage, &limit);
	(void)usage;
	return write_u64(out, limit);
}

static HRESULT WINAPI process_private(process_report *iface, uint64_t out)
{
	return app_private((app_report *)iface, out);
}

static HRESULT WINAPI process_total(process_report *iface, uint64_t out)
{
	return app_private((app_report *)iface, out);
}

static HRESULT WINAPI app_q(app_report *iface, REFIID iid, void **out);
static ULONG WINAPI app_addref(app_report *iface);
static ULONG WINAPI app_release(app_report *iface);
static HRESULT WINAPI app_iids(app_report *iface, ULONG *count, IID **iids);
static HRESULT WINAPI app_name(app_report *iface, HSTRING *name);
static HRESULT WINAPI app_trust(app_report *iface, TrustLevel *level);
static HRESULT WINAPI app2_q(app_report2 *iface, REFIID iid, void **out);
static ULONG WINAPI app2_addref(app_report2 *iface);
static ULONG WINAPI app2_release(app_report2 *iface);
static HRESULT WINAPI app2_iids(app_report2 *iface, ULONG *count, IID **iids);
static HRESULT WINAPI app2_name(app_report2 *iface, HSTRING *name);
static HRESULT WINAPI app2_trust(app_report2 *iface, TrustLevel *level);
static HRESULT WINAPI process_q(process_report *iface, REFIID iid, void **out);
static ULONG WINAPI process_addref(process_report *iface);
static ULONG WINAPI process_release(process_report *iface);
static HRESULT WINAPI process_iids(process_report *iface, ULONG *count, IID **iids);
static HRESULT WINAPI process_name(process_report *iface, HSTRING *name);
static HRESULT WINAPI process_trust(process_report *iface, TrustLevel *level);

static const __x_ABI_CWindows_CSystem_CIAppMemoryReportVtbl app_report_table = {
	.QueryInterface = app_q, .AddRef = app_addref, .Release = app_release,
	.GetIids = app_iids, .GetRuntimeClassName = app_name, .GetTrustLevel = app_trust,
	.get_PrivateCommitUsage = app_private, .get_PeakPrivateCommitUsage = app_peak,
	.get_TotalCommitUsage = app_total_usage, .get_TotalCommitLimit = app_total_limit,
};

static const __x_ABI_CWindows_CSystem_CIAppMemoryReport2Vtbl app_report2_table = {
	.QueryInterface = app2_q, .AddRef = app2_addref, .Release = app2_release,
	.GetIids = app2_iids, .GetRuntimeClassName = app2_name, .GetTrustLevel = app2_trust,
	.get_ExpectedTotalCommitLimit = app_expected_limit,
};

static const __x_ABI_CWindows_CSystem_CIProcessMemoryReportVtbl process_report_table = {
	.QueryInterface = process_q, .AddRef = process_addref, .Release = process_release,
	.GetIids = process_iids, .GetRuntimeClassName = process_name, .GetTrustLevel = process_trust,
	.get_PrivateWorkingSetUsage = process_private, .get_TotalWorkingSetUsage = process_total,
};

static HRESULT WINAPI app_q(app_report *iface, REFIID iid, void **out)
{
	struct app_report_object *self = (struct app_report_object *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CSystem_CIAppMemoryReport))
		*out = iface;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CSystem_CIAppMemoryReport2))
		*out = &self->second;
	else
		return E_NOINTERFACE;
	app_addref(iface);
	return S_OK;
}

static ULONG WINAPI app_addref(app_report *iface)
{
	return (ULONG)InterlockedIncrement(&((struct app_report_object *)iface)->refs);
}

static ULONG WINAPI app_release(app_report *iface)
{
	struct app_report_object *self = (struct app_report_object *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		HeapFree(GetProcessHeap(), 0, self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI app_iids(app_report *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT app_report_name(HSTRING *name, const WCHAR *text)
{
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), name);
}

static HRESULT WINAPI app_name(app_report *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Windows.System.AppMemoryReport";
	(void)iface;
	return app_report_name(name, text);
}

static HRESULT WINAPI app_trust(app_report *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI app2_q(app_report2 *iface, REFIID iid, void **out)
{
	return app_q((app_report *)((struct app_report2_part *)iface)->owner, iid, out);
}

static ULONG WINAPI app2_addref(app_report2 *iface)
{
	return app_addref((app_report *)((struct app_report2_part *)iface)->owner);
}

static ULONG WINAPI app2_release(app_report2 *iface)
{
	return app_release((app_report *)((struct app_report2_part *)iface)->owner);
}

static HRESULT WINAPI app2_iids(app_report2 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI app2_name(app_report2 *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Windows.System.AppMemoryReport";
	(void)iface;
	return app_report_name(name, text);
}

static HRESULT WINAPI app2_trust(app_report2 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI process_q(process_report *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID__x_ABI_CWindows_CSystem_CIProcessMemoryReport))
		return E_NOINTERFACE;
	process_addref(iface);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI process_addref(process_report *iface)
{
	return (ULONG)InterlockedIncrement(&((struct process_report_object *)iface)->refs);
}

static ULONG WINAPI process_release(process_report *iface)
{
	struct process_report_object *self = (struct process_report_object *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		HeapFree(GetProcessHeap(), 0, self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI process_iids(process_report *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI process_name(process_report *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Windows.System.ProcessMemoryReport";
	(void)iface;
	return app_report_name(name, text);
}

static HRESULT WINAPI process_trust(process_report *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static struct app_report_object *create_app_report(void)
{
	struct app_report_object *report = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *report);
	if (!report)
		return NULL;
	report->lpVtbl = &app_report_table;
	report->refs = 1;
	report->second.lpVtbl = &app_report2_table;
	report->second.owner = report;
	InterlockedIncrement(&raven_xaml_objects);
	return report;
}

static struct process_report_object *create_process_report(void)
{
	struct process_report_object *report = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *report);
	if (!report)
		return NULL;
	report->lpVtbl = &process_report_table;
	report->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	return report;
}

static HRESULT WINAPI statics2_report_app(static2 *iface, uint64_t out)
{
	struct app_report_object *report;
	(void)iface;
	if (!out)
		return E_POINTER;
	if (!(report = create_app_report()))
		return E_OUTOFMEMORY;
	*(void **)(uintptr_t)out = report;
	return S_OK;
}

static HRESULT WINAPI statics2_report_process(static2 *iface, uint64_t out)
{
	struct process_report_object *report;
	(void)iface;
	if (!out)
		return E_POINTER;
	if (!(report = create_process_report()))
		return E_OUTOFMEMORY;
	*(void **)(uintptr_t)out = report;
	return S_OK;
}

static const __x_ABI_CWindows_CSystem_CIMemoryManagerStaticsVtbl statics1_table = {
	.QueryInterface = part1_query,
	.AddRef = part1_addref,
	.Release = part1_release,
	.GetIids = part1_iids,
	.GetRuntimeClassName = part1_name,
	.GetTrustLevel = part1_trust,
	.get_AppMemoryUsage = usage_get,
	.get_AppMemoryUsageLimit = limit_get,
	.get_AppMemoryUsageLevel = level_get,
	.add_AppMemoryUsageIncreased = event_add,
	.remove_AppMemoryUsageIncreased = event_remove,
	.add_AppMemoryUsageDecreased = event_add,
	.remove_AppMemoryUsageDecreased = event_remove,
	.add_AppMemoryUsageLimitChanging = event_add,
	.remove_AppMemoryUsageLimitChanging = event_remove,
};

static const __x_ABI_CWindows_CSystem_CIMemoryManagerStatics2Vtbl statics2_table = {
	.QueryInterface = part2_query,
	.AddRef = part2_addref,
	.Release = part2_release,
	.GetIids = part2_iids,
	.GetRuntimeClassName = part2_name,
	.GetTrustLevel = part2_trust,
	.GetAppMemoryReport = statics2_report_app,
	.GetProcessMemoryReport = statics2_report_process,
};

static const __x_ABI_CWindows_CSystem_CIMemoryManagerStatics3Vtbl statics3_table = {
	.QueryInterface = part3_query,
	.AddRef = part3_addref,
	.Release = part3_release,
	.GetIids = part3_iids,
	.GetRuntimeClassName = part3_name,
	.GetTrustLevel = part3_trust,
	.TrySetAppMemoryUsageLimit = statics3_try_set_limit,
};

static const __x_ABI_CWindows_CSystem_CIMemoryManagerStatics4Vtbl statics4_table = {
	.QueryInterface = part4_query,
	.AddRef = part4_addref,
	.Release = part4_release,
	.GetIids = part4_iids,
	.GetRuntimeClassName = part4_name,
	.GetTrustLevel = part4_trust,
	.get_ExpectedAppMemoryUsageLimit = expected_limit,
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
	{&statics3_table, &factory_instance},
	{&statics4_table, &factory_instance},
	1,
};

HRESULT raven_xaml_memory_manager_factory(HSTRING name, IActivationFactory **out)
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
