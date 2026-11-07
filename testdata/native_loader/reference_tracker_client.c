#define WIN32_LEAN_AND_MEAN
#define CINTERFACE
#define COBJMACROS
#define INITGUID
#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <activation.h>
#include <stdint.h>

/* IReferenceTracker, the face the .NET runtime negotiates with on XAML objects: the Frame answers the
 * ask, the tracker notifications keep their counters, GetReferenceTrackerManager hands out the
 * process-wide manager, and the manager's round handshake and host registration run. The declarations
 * are raven's own windows_ui_xaml_hosting_referencetracker.h — MinGW carries none. */
#include "windows_ui_xaml_hosting_referencetracker.h"

typedef __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker IReferenceTrackerR;
typedef __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager IReferenceTrackerManagerR;
typedef __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost IReferenceTrackerHostR;
#define IID_IReferenceTrackerR IID___x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker
#define IID_IReferenceTrackerManagerR IID___x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager
#define IID_IReferenceTrackerHostR IID___x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost

typedef HRESULT (WINAPI *activation_function)(HSTRING, REFIID, void **);

static HRESULT make_string(const WCHAR *text, HSTRING *out)
{
	return WindowsCreateString(text, (UINT32)lstrlenW(text), out);
}

/* A stand-in host the manager can register, in case the runtime hands one over during the round. */
struct fixture_host {
	IReferenceTrackerHostR host;
	LONG refs;
	int disconnected, released_disconnected, thread_ends, targets, pressure;
};

static HRESULT WINAPI host_query(IReferenceTrackerHostR *This, REFIID iid, void **out)
{
	struct fixture_host *self = (struct fixture_host *)This;
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID_IReferenceTrackerHostR)) {
		InterlockedIncrement(&self->refs);
		*out = This;
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI fixture_host_addref(IReferenceTrackerHostR *This) { return InterlockedIncrement(&((struct fixture_host *)This)->refs); }
static ULONG WINAPI fixture_host_release(IReferenceTrackerHostR *This) { return InterlockedDecrement(&((struct fixture_host *)This)->refs); }
static HRESULT WINAPI fixture_host_disconnect_unused(IReferenceTrackerHostR *This, uint32_t options) { (void)options; ((struct fixture_host *)This)->disconnected++; return S_OK; }
static HRESULT WINAPI fixture_host_release_disconnected(IReferenceTrackerHostR *This) { ((struct fixture_host *)This)->released_disconnected++; return S_OK; }
static HRESULT WINAPI fixture_host_thread_end(IReferenceTrackerHostR *This) { ((struct fixture_host *)This)->thread_ends++; return S_OK; }
static HRESULT WINAPI fixture_host_get_target(IReferenceTrackerHostR *This, IUnknown *unknown, __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTarget **out)
{
	(void)unknown; (void)out;
	((struct fixture_host *)This)->targets++;
	return E_NOTIMPL;
}
static HRESULT WINAPI fixture_host_add_pressure(IReferenceTrackerHostR *This, uint64_t bytes) { (void)bytes; ((struct fixture_host *)This)->pressure++; return S_OK; }
static HRESULT WINAPI fixture_host_remove_pressure(IReferenceTrackerHostR *This, uint64_t bytes) { (void)bytes; ((struct fixture_host *)This)->pressure--; return S_OK; }

static const __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHostVtbl fixture_host_table = {
	host_query, fixture_host_addref, fixture_host_release,
	fixture_host_disconnect_unused, fixture_host_release_disconnected, fixture_host_thread_end,
	fixture_host_get_target, fixture_host_add_pressure, fixture_host_remove_pressure,
};

int raven_entry(void)
{
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	activation_function activate_factory = runtime ? (activation_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	if (!activate_factory)
		return 1;
	HSTRING key = NULL;
	IActivationFactory *factory = NULL;
	IInspectable *instance = NULL;
	IReferenceTrackerR *tracker = NULL;
	IReferenceTrackerManagerR *manager = NULL, *manager_again = NULL;
	struct fixture_host fixture;
	ULONG refs;
	HRESULT result;

	/* The catalog carries the Frame; its factory activates an instance the tracker ask serves. */
	if (make_string(L"Windows.UI.Xaml.Controls.Frame", &key) != S_OK)
		return 2;
	if (activate_factory(key, &IID_IActivationFactory, (void **)&factory) != S_OK || !factory)
		return 3;
	if (factory->lpVtbl->ActivateInstance(factory, &instance) != S_OK || !instance)
		return 4;
	if (instance->lpVtbl->QueryInterface(instance, &IID_IReferenceTrackerR, (void **)&tracker) != S_OK || !tracker)
		return 5;

	/* The notifications keep their counters and answer, the way a tracker source drives them. */
	if (tracker->lpVtbl->ConnectFromTrackerSource(tracker) != S_OK)
		return 6;
	if (tracker->lpVtbl->AddRefFromTrackerSource(tracker) != S_OK)
		return 7;
	if (tracker->lpVtbl->AddRefFromTrackerSource(tracker) != S_OK)
		return 7;
	if (tracker->lpVtbl->PegFromTrackerSource(tracker) != S_OK)
		return 8;
	if (tracker->lpVtbl->ReleaseFromTrackerSource(tracker) != S_OK)
		return 9;
	if (tracker->lpVtbl->DisconnectFromTrackerSource(tracker) != S_OK)
		return 10;

	/* FindTrackerTargets walks; with no managed targets reachable it completes without invoking the callback. */
	if (tracker->lpVtbl->FindTrackerTargets(tracker, NULL) != S_OK)
		return 11;

	/* The manager comes from the object, once per ask, and the two hands are the same singleton. */
	if (tracker->lpVtbl->GetReferenceTrackerManager(tracker, &manager) != S_OK || !manager)
		return 12;
	if (tracker->lpVtbl->GetReferenceTrackerManager(tracker, &manager_again) != S_OK || !manager_again)
		return 13;
	if (manager != manager_again)
		return 14;
	manager_again->lpVtbl->Release(manager_again);

	/* The round handshake records the round; the host registration keeps the handed host alive. */
	if (manager->lpVtbl->ReferenceTrackingStarted(manager) != S_OK)
		return 15;
	if (manager->lpVtbl->FindTrackerTargetsCompleted(manager, FALSE) != S_OK)
		return 16;
	if (manager->lpVtbl->ReferenceTrackingCompleted(manager) != S_OK)
		return 17;

	memset(&fixture, 0, sizeof fixture);
	fixture.host.lpVtbl = &fixture_host_table;
	fixture.refs = 1;
	if (manager->lpVtbl->SetReferenceTrackerHost(manager, &fixture.host) != S_OK)
		return 18;
	if (fixture.refs != 2) /* the manager's registration holds the host */
		return 19;
	if (manager->lpVtbl->SetReferenceTrackerHost(manager, NULL) != S_OK)
		return 20;
	if (fixture.refs != 1)
		return 21;

	/* The tracker part rides the frame's lifetime: its Release runs the frame's own release path. */
	refs = tracker->lpVtbl->Release(tracker);
	if (!refs)
		return 22; /* the instance reference outlives this part reference */
	if (instance->lpVtbl->Release(instance) != S_OK)
		return 23;
	if (manager->lpVtbl->Release(manager) != (ULONG)1)
		return 24; /* the singleton lives for the process: the initial hand plus the two taken and given back */

	factory->lpVtbl->Release(factory);
	WindowsDeleteString(key);
	return 0;
}
