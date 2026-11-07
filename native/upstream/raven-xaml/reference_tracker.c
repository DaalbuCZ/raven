/* SPDX-License-Identifier: MIT
 * See LICENSES/MIT.txt.
 */
/* IReferenceTracker (reference_tracker.h in this directory): the lifetime-coordination face most XAML
 * framework objects implement, and the one the .NET runtime's SharedLibrary asks for when a XAML object
 * is handed to managed code — the traced Shapr3D run asks it of the Frame exactly once, immediately
 * before its wrapper machinery tears a cached managed wrapper down twice.
 *
 * The part follows the weak-reference source's shape: the owner embeds it, the part's IUnknown forwards
 * to the owner, and the tracker notifications keep real per-object state (connected sources, the
 * tracker-source reference balance, the peg). FindTrackerTargets reports no targets: every object the
 * raven component graph holds is a native COM object kept alive by plain strong references, and the
 * walk exists to name managed objects reachable only through XAML-internal weak references, of which
 * the components hold none — reporting none can only delay a collection, never collect a live object.
 *
 * The manager is one process-wide instance, the way the real framework's manager is: the runtime hands
 * it its host once and drives the round handshake through it. */
#define INITGUID
#define WIN32_LEAN_AND_MEAN
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include "xaml_internal.h"
#include "windows_ui_xaml_hosting_referencetracker.h"

typedef __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker reference_tracker_i;
typedef __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerVtbl reference_tracker_vtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager tracker_manager_i;
typedef __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManagerVtbl tracker_manager_vtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost tracker_host_i;
typedef __x_ABI_CWindows_CUI_CXaml_CHosting_CIFindReferenceTargetsCallback find_targets_callback_i;

static int equal_iid(REFIID left, const IID *right)
{
	return memcmp(left, right, sizeof(IID)) == 0;
}

const tracker_manager_vtbl raven_tracker_manager_table;

/* The process-wide manager the runtime drives its collection rounds against. */
static struct raven_tracker_manager {
	const tracker_manager_vtbl *lpVtbl;
	LONG refs;
	tracker_host_i *host;
	uint32_t rounds; /* ReferenceTrackingStarted calls seen */
	uint32_t completed; /* FindTrackerTargetsCompleted calls seen */
	boolean round_active;
} raven_tracker_manager_state = { &raven_tracker_manager_table, 1, NULL, 0, 0, FALSE };

static HRESULT WINAPI tracker_query(reference_tracker_i *iface, REFIID iid, void **out)
{
	struct raven_tracker_part *self = (struct raven_tracker_part *)iface;
	{
		char note[80];
		wsprintfA(note, "raven-tracker: query %08lx-%04lx\n", (unsigned long)iid->Data1, (unsigned long)iid->Data2);
		OutputDebugStringA(note);
	}
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker)) {
		InterlockedIncrement(self->owner_refs);
		*out = iface;
		return S_OK;
	}
	if (equal_iid(iid, &IID_IAgileObject)) {
		/* the marker follows the owner, the way the weak source's does: the owner's own query answers */
		return self->owner_query(self->owner, iid, out);
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI tracker_addref(reference_tracker_i *iface)
{
	return (ULONG)InterlockedIncrement(((struct raven_tracker_part *)iface)->owner_refs);
}

static ULONG WINAPI tracker_release(reference_tracker_i *iface)
{
	return (ULONG)InterlockedDecrement(((struct raven_tracker_part *)iface)->owner_refs);
}

static HRESULT WINAPI tracker_connect(reference_tracker_i *iface)
{
	struct raven_tracker_part *self = (struct raven_tracker_part *)iface;
	self->connected++;
	return S_OK;
}

static HRESULT WINAPI tracker_disconnect(reference_tracker_i *iface)
{
	struct raven_tracker_part *self = (struct raven_tracker_part *)iface;
	if (self->connected)
		self->connected--;
	return S_OK;
}

static HRESULT WINAPI tracker_find_targets(reference_tracker_i *iface, find_targets_callback_i *callback)
{
	(void)iface;
	(void)callback;
	/* No managed targets to name: the components hold only strong native references (see the file
	 * comment). The callback is left untouched, S_OK reports a completed (empty) walk. */
	return S_OK;
}

static HRESULT WINAPI tracker_get_manager(reference_tracker_i *iface, tracker_manager_i **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = (tracker_manager_i *)&raven_tracker_manager_state;
	InterlockedIncrement(&raven_tracker_manager_state.refs);
	return S_OK;
}

static HRESULT WINAPI tracker_addref_from_source(reference_tracker_i *iface)
{
	((struct raven_tracker_part *)iface)->tracker_refs++;
	return S_OK;
}

static HRESULT WINAPI tracker_release_from_source(reference_tracker_i *iface)
{
	struct raven_tracker_part *self = (struct raven_tracker_part *)iface;
	if (self->tracker_refs)
		self->tracker_refs--;
	return S_OK;
}

static HRESULT WINAPI tracker_peg_from_source(reference_tracker_i *iface)
{
	((struct raven_tracker_part *)iface)->pegged = TRUE;
	return S_OK;
}

static const reference_tracker_vtbl raven_tracker_table = {
	tracker_query, tracker_addref, tracker_release,
	tracker_connect, tracker_disconnect, tracker_find_targets, tracker_get_manager,
	tracker_addref_from_source, tracker_release_from_source, tracker_peg_from_source,
};

void raven_tracker_part_init(struct raven_tracker_part *part, void *owner, LONG *owner_refs,
	raven_weak_query_fn owner_query)
{
	part->lpVtbl = &raven_tracker_table;
	part->owner = owner;
	part->owner_refs = owner_refs;
	part->owner_query = owner_query;
	part->connected = 0;
	part->tracker_refs = 0;
	part->pegged = FALSE;
}

/* ---- the manager ---- */

static HRESULT WINAPI tracker_manager_query(tracker_manager_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager)) {
		InterlockedIncrement(&((struct raven_tracker_manager *)iface)->refs);
		*out = iface;
		return S_OK;
	}
	/* the manager is process-wide state read under the engine's single-apartment contract */
	if (equal_iid(iid, &IID_IAgileObject)) {
		InterlockedIncrement(&((struct raven_tracker_manager *)iface)->refs);
		*out = iface;
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG WINAPI tracker_manager_addref(tracker_manager_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct raven_tracker_manager *)iface)->refs);
}

static ULONG WINAPI tracker_manager_release(tracker_manager_i *iface)
{
	/* the singleton lives for the process, the way the framework's manager does; the count only reports */
	return (ULONG)InterlockedDecrement(&((struct raven_tracker_manager *)iface)->refs);
}

static HRESULT WINAPI tracker_manager_started(tracker_manager_i *iface)
{
	struct raven_tracker_manager *self = (struct raven_tracker_manager *)iface;
	self->rounds++;
	self->round_active = TRUE;
	return S_OK;
}

static HRESULT WINAPI tracker_manager_completed(tracker_manager_i *iface, boolean findFailed)
{
	struct raven_tracker_manager *self = (struct raven_tracker_manager *)iface;
	self->completed++;
	self->round_active = FALSE;
	(void)findFailed; /* the failure flag names a walk that refused; none of ours refuse */
	return S_OK;
}

static HRESULT WINAPI tracker_manager_reference_tracking_completed(tracker_manager_i *iface)
{
	((struct raven_tracker_manager *)iface)->round_active = FALSE;
	return S_OK;
}

static HRESULT WINAPI tracker_manager_set_host(tracker_manager_i *iface, tracker_host_i *host)
{
	struct raven_tracker_manager *self = (struct raven_tracker_manager *)iface;
	if (host)
		IUnknown_AddRef((IUnknown *)host);
	if (self->host)
		IUnknown_Release((IUnknown *)self->host);
	self->host = host;
	return S_OK;
}

const tracker_manager_vtbl raven_tracker_manager_table = {
	tracker_manager_query, tracker_manager_addref, tracker_manager_release,
	tracker_manager_started, tracker_manager_completed,
	tracker_manager_reference_tracking_completed, tracker_manager_set_host,
};
