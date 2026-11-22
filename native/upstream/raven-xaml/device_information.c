/* SPDX-License-Identifier: MIT
 * Windows.Devices.Enumeration.DeviceInformation: the device enumeration the app's setup path asks for at the
 * graphics stage. The activation and the statics exist first; the members the projection calls are served in the
 * metadata order the windows-rs crate's IDeviceInformationStatics_Vtbl fixes (the same shape as the system winmd).
 * The enumeration is honest in exactly one way — raven ships no PnP registry, so every FindAllAsync answers an
 * empty set (the codec_query.c precedent) and CreateFromIdAsync answers the documented not-found completion; the
 * DeviceWatcher class is not served yet, so its members refuse with E_NOTIMPL and the refused QIs are traced with
 * full GUIDs so the faces the app casts next land in the launch log. */
#include "xaml_internal.h"
#include <winstring.h>
#include <string.h>

static const WCHAR device_information_class[] = L"Windows.Devices.Enumeration.DeviceInformation";
static const WCHAR device_vector_class[] =
	L"Windows.Foundation.Collections.IVectorView`1<Windows.Devices.Enumeration.DeviceInformation>";
static const WCHAR device_operation_class[] =
	L"Windows.Foundation.IAsyncOperation`1<Windows.Foundation.Collections.IVectorView`1<Windows.Devices.Enumeration.DeviceInformation>>";

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

/* The faces, on record: the statics the activation refuses to hand to the app (c17f100e-3a46-4a78-8013-769dc9b97390,
 * from the traced launch's refused RoGetActivationFactory), the parameterized IVectorView`1<DeviceInformation> the
 * await's continuation casts the results for, and the parameterized IAsyncOperation`1<IVectorView`1<...>> the await
 * drives — the instance identifiers computed by the SHA-1 rule over the generic signatures (the windows-collections
 * bbe1fa4c and windows-future 9fc2b0bb generic identifiers), verified against the display_monitor header's instance. */
static const IID device_statics_iid = {0xc17f100e, 0x3a46, 0x4a78, {0x80, 0x13, 0x76, 0x9d, 0xc9, 0xb9, 0x73, 0x90}};
static const IID device_vector_iid = {0xe170688f, 0x3495, 0x5bf6, {0xaa, 0xb5, 0x9c, 0xac, 0x17, 0xe0, 0xf1, 0x0f}};
static const IID device_operation_iid = {0xf02f08f2, 0xf30f, 0x5b83, {0xac, 0xc5, 0x57, 0xe1, 0xf9, 0xda, 0xa0, 0xdd}};

static int equal_iid(REFIID left, const IID *right)
{
	return left->Data1 == right->Data1 && left->Data2 == right->Data2 &&
		left->Data3 == right->Data3 && !memcmp(left->Data4, right->Data4, 8);
}
static HRESULT inspectable_iids(ULONG *count, IID **iids)
{
	if (!count || !iids)
		return E_POINTER;
	*iids = (IID *)HeapAlloc(GetProcessHeap(), 0, 2 * sizeof(IID));
	if (!*iids)
		return E_OUTOFMEMORY;
	**iids = IID_IUnknown;
	(*iids)[1] = IID_IInspectable;
	*count = 2;
	return S_OK;
}
static HRESULT inspectable_trust(TrustLevel *level)
{
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

/* The refused casts name the faces still to serve, with the object kind that refused them (the codec_query.c
 * convention: a one-line note the debugger play relays under RAVEN_TRACE_DEBUG). */
static void trace_refused_iid(const char *who, REFIID iid)
{
	char note[128];
	wsprintfA(note, "raven-xaml: device %s has no interface %08lx-%04lx-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
		who, (unsigned long)iid->Data1, (unsigned long)iid->Data2, (unsigned)iid->Data3, iid->Data4[0], iid->Data4[1],
		iid->Data4[2], iid->Data4[3], iid->Data4[4], iid->Data4[5], iid->Data4[6], iid->Data4[7]);
	raven_trace_note(note);
}

static void trace_member(const char *member)
{
	char note[96];
	lstrcpynA(note, "raven-xaml: device ", ARRAYSIZE(note));
	lstrcpynA(note + lstrlenA(note), member, ARRAYSIZE(note) - lstrlenA(note));
	raven_trace_note(note);
}

static HRESULT class_name(HSTRING *out, const WCHAR *text)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), out);
}

/* ---- The empty result set: the parameterized IVectorView`1<DeviceInformation> face the await's continuation casts
 * the GetResults return for. The set is empty, so get_Size answers 0, GetAt answers E_BOUNDS, IndexOf answers
 * not-found, GetMany answers zero elements — the codec_query.c shape. ---- */

typedef struct device_vector_vtbl device_vector_vtbl;
struct device_vector_vtbl {
	BEGIN_INTERFACE
	HRESULT (WINAPI *QueryInterface)(IInspectable *This, REFIID riid, void **object);
	ULONG (WINAPI *AddRef)(IInspectable *This);
	ULONG (WINAPI *Release)(IInspectable *This);
	HRESULT (WINAPI *GetIids)(IInspectable *This, ULONG *count, IID **iids);
	HRESULT (WINAPI *GetRuntimeClassName)(IInspectable *This, HSTRING *name);
	HRESULT (WINAPI *GetTrustLevel)(IInspectable *This, TrustLevel *level);
	HRESULT (WINAPI *GetAt)(IInspectable *This, UINT32 index, IInspectable **value);
	HRESULT (WINAPI *get_Size)(IInspectable *This, UINT32 *value);
	HRESULT (WINAPI *IndexOf)(IInspectable *This, IInspectable *element, UINT32 *index, boolean *found);
	HRESULT (WINAPI *GetMany)(IInspectable *This, UINT32 start_index, UINT32 items_size, IInspectable **items, UINT32 *value);
	END_INTERFACE
};
typedef struct device_vector device_vector;
typedef struct device_vector_part { const void *lpVtbl; device_vector *owner; } device_vector_part;
struct device_vector {
	device_vector_part inspectable;
	LONG refs;
};
static LONG device_vectors;
static HRESULT WINAPI device_vector_query(IInspectable *iface, REFIID iid, void **out)
{
	device_vector *self = ((device_vector_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IAgileObject) ||
		equal_iid(iid, &device_vector_iid))
		*out = &self->inspectable;
	else {
		trace_refused_iid("vector", iid);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
	return S_OK;
}
static ULONG WINAPI device_vector_addref(IInspectable *iface)
{ return (ULONG)InterlockedIncrement(&((device_vector_part *)iface)->owner->refs); }
static ULONG WINAPI device_vector_release(IInspectable *iface)
{
	device_vector *self = ((device_vector_part *)iface)->owner;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		FREE(self);
		InterlockedDecrement(&device_vectors);
	}
	return (ULONG)refs;
}
static HRESULT WINAPI device_vector_iids(IInspectable *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI device_vector_name(IInspectable *iface, HSTRING *out)
{ (void)iface; return class_name(out, device_vector_class); }
static HRESULT WINAPI device_vector_trust(IInspectable *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static HRESULT WINAPI device_vector_get_at(IInspectable *iface, UINT32 index, IInspectable **value)
{
	(void)iface; (void)index;
	if (!value)
		return E_POINTER;
	*value = NULL;
	return E_BOUNDS;
}
static HRESULT WINAPI device_vector_get_size(IInspectable *iface, UINT32 *value)
{ (void)iface; if (!value) return E_POINTER; *value = 0; return S_OK; }
static HRESULT WINAPI device_vector_index_of(IInspectable *iface, IInspectable *element, UINT32 *index, boolean *found)
{
	(void)iface; (void)element;
	if (!index || !found)
		return E_POINTER;
	*index = 0;
	*found = 0;
	return S_OK;
}
static HRESULT WINAPI device_vector_get_many(IInspectable *iface, UINT32 start_index, UINT32 items_size, IInspectable **items, UINT32 *value)
{
	(void)iface; (void)start_index; (void)items; (void)items_size;
	if (!value)
		return E_POINTER;
	*value = 0;
	return S_OK;
}
static const device_vector_vtbl device_vector_table = {
	device_vector_query, device_vector_addref, device_vector_release,
	device_vector_iids, device_vector_name, device_vector_trust,
	device_vector_get_at, device_vector_get_size, device_vector_index_of, device_vector_get_many,
};
static IInspectable *device_vector_new(void)
{
	device_vector *self = ALLOC(sizeof *self);
	if (!self)
		return NULL;
	self->inspectable.lpVtbl = &device_vector_table;
	self->inspectable.owner = self;
	self->refs = 1;
	InterlockedIncrement(&device_vectors);
	return (IInspectable *)&self->inspectable;
}

/* ---- The completed operation, two faces over one object: the parameterized IAsyncOperation`1 face the await
 * drives (put_Completed 6, get_Completed 7, GetResults 8) and the IAsyncInfo face the projection reaches by QI
 * (get_Id 6, get_Status 7, get_ErrorCode 8, Cancel 9, Close 10). Two terminal states: the FindAllAsync family
 * completes with the empty set; CreateFromIdAsync over an id raven's registry does not carry completes with the
 * documented not-found error the app's await turns into its own fallback. ---- */

typedef struct device_operation_vtbl device_operation_vtbl;
struct device_operation_vtbl {
	BEGIN_INTERFACE
	HRESULT (WINAPI *QueryInterface)(void *This, REFIID riid, void **object);
	ULONG (WINAPI *AddRef)(void *This);
	ULONG (WINAPI *Release)(void *This);
	HRESULT (WINAPI *GetIids)(void *This, ULONG *count, IID **iids);
	HRESULT (WINAPI *GetRuntimeClassName)(void *This, HSTRING *name);
	HRESULT (WINAPI *GetTrustLevel)(void *This, TrustLevel *level);
	HRESULT (WINAPI *put_Completed)(void *This, void *handler);
	HRESULT (WINAPI *get_Completed)(void *This, void **handler);
	HRESULT (WINAPI *GetResults)(void *This, IInspectable **results);
	END_INTERFACE
};
typedef struct device_operation_info_vtbl device_operation_info_vtbl;
struct device_operation_info_vtbl {
	BEGIN_INTERFACE
	HRESULT (WINAPI *QueryInterface)(void *This, REFIID riid, void **object);
	ULONG (WINAPI *AddRef)(void *This);
	ULONG (WINAPI *Release)(void *This);
	HRESULT (WINAPI *GetIids)(void *This, ULONG *count, IID **iids);
	HRESULT (WINAPI *GetRuntimeClassName)(void *This, HSTRING *name);
	HRESULT (WINAPI *GetTrustLevel)(void *This, TrustLevel *level);
	HRESULT (WINAPI *get_Id)(void *This, UINT32 *value);
	HRESULT (WINAPI *get_Status)(void *This, AsyncStatus *value);
	HRESULT (WINAPI *get_ErrorCode)(void *This, HRESULT *value);
	HRESULT (WINAPI *Cancel)(void *This);
	HRESULT (WINAPI *Close)(void *This);
	END_INTERFACE
};
typedef struct device_operation device_operation;
typedef struct device_operation_part { const void *lpVtbl; device_operation *owner; } device_operation_part;
struct device_operation {
	device_operation_part parameterized;
	device_operation_part asyncinfo;
	LONG refs;
	IInspectable *result;
	HRESULT error;
};
static LONG device_operations;
static device_operation *device_operation_owner(void *iface) { return ((device_operation_part *)iface)->owner; }
static HRESULT WINAPI device_operation_query(void *iface, REFIID iid, void **out)
{
	device_operation *self = device_operation_owner(iface);
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IAgileObject) ||
		equal_iid(iid, &device_operation_iid))
		*out = &self->parameterized;
	else if (equal_iid(iid, &IID_IAsyncInfo))
		*out = &self->asyncinfo;
	else {
		trace_refused_iid("operation", iid);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
	return S_OK;
}
static ULONG WINAPI device_operation_addref(void *iface)
{ return (ULONG)InterlockedIncrement(&device_operation_owner(iface)->refs); }
static ULONG WINAPI device_operation_release(void *iface)
{
	device_operation *self = device_operation_owner(iface);
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->result) self->result->lpVtbl->Release(self->result);
		FREE(self);
		InterlockedDecrement(&device_operations);
	}
	return (ULONG)refs;
}
static HRESULT WINAPI device_operation_iids(void *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI device_operation_name(void *iface, HSTRING *out)
{ (void)iface; return class_name(out, device_operation_class); }
static HRESULT WINAPI device_operation_trust(void *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static HRESULT WINAPI device_operation_get_id(void *iface, UINT32 *out)
{ (void)iface; if (!out) return E_POINTER; *out = 1; return S_OK; }
static HRESULT WINAPI device_operation_get_status(void *iface, AsyncStatus *out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = device_operation_owner(iface)->error ? Error : Completed;
	return S_OK;
}
static HRESULT WINAPI device_operation_get_error(void *iface, HRESULT *out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = device_operation_owner(iface)->error;
	return S_OK;
}
static HRESULT WINAPI device_operation_cancel(void *iface)
{ (void)iface; raven_trace_note("raven-xaml: device operation Cancel\n"); return S_OK; }
static HRESULT WINAPI device_operation_close(void *iface)
{ (void)iface; return S_OK; }
/* the completed-handler convention (the codec round's): the handler is an IUnknown whose fourth slot is
 * Invoke(asyncOp, status); the operation is already terminal, so put_Completed invokes it synchronously */
static HRESULT WINAPI device_operation_put_completed(void *iface, void *handler)
{
	device_operation *self = device_operation_owner(iface);
	char note[96];
	wsprintfA(note, "raven-xaml: device operation put_completed handler=%p\n", handler);
	raven_trace_note(note);
	if (handler) {
		void **vtable = *(void ***)(uintptr_t)handler;
		void (*invoke)(void *, IUnknown *, AsyncStatus) = (void (*)(void *, IUnknown *, AsyncStatus))vtable[3];
		invoke(handler, iface, self->error ? Error : Completed);
	}
	return S_OK;
}
static HRESULT WINAPI device_operation_get_completed(void *iface, void **handler)
{
	(void)iface;
	if (!handler)
		return E_POINTER;
	*handler = NULL;
	return S_OK;
}
static HRESULT WINAPI device_operation_get_results(void *iface, IInspectable **results)
{
	device_operation *self = device_operation_owner(iface);
	if (!results)
		return E_POINTER;
	*results = NULL;
	if (self->error) {
		char note[96];
		wsprintfA(note, "raven-xaml: device operation GetResults error=%08lx\n", (unsigned long)self->error);
		raven_trace_note(note);
		return self->error;
	}
	if (!self->result) {
		self->result = device_vector_new();
		if (!self->result)
			return E_OUTOFMEMORY;
	}
	*results = self->result;
	(*results)->lpVtbl->AddRef(*results);
	raven_trace_note("raven-xaml: device operation GetResults answered the empty set\n");
	return S_OK;
}
static const device_operation_vtbl device_operation_table = {
	device_operation_query, device_operation_addref, device_operation_release,
	device_operation_iids, device_operation_name, device_operation_trust,
	device_operation_put_completed, device_operation_get_completed, device_operation_get_results,
};
static const device_operation_info_vtbl device_operation_info_table = {
	device_operation_query, device_operation_addref, device_operation_release,
	device_operation_iids, device_operation_name, device_operation_trust,
	device_operation_get_id, device_operation_get_status, device_operation_get_error,
	device_operation_cancel, device_operation_close,
};
static device_operation *device_operation_new(HRESULT error)
{
	device_operation *self = ALLOC(sizeof *self);
	if (!self)
		return NULL;
	self->parameterized.lpVtbl = (const void *)&device_operation_table;
	self->parameterized.owner = self;
	self->asyncinfo.lpVtbl = (const void *)&device_operation_info_table;
	self->asyncinfo.owner = self;
	self->refs = 1;
	self->error = error;
	InterlockedIncrement(&device_operations);
	return self;
}
static HRESULT device_operation_hand(HRESULT error, IInspectable **out)
{
	device_operation *operation;
	if (!out)
		return E_POINTER;
	*out = NULL;
	operation = device_operation_new(error);
	if (!operation)
		return E_OUTOFMEMORY;
	*out = (IInspectable *)&operation->parameterized;
	return S_OK;
}

/* ---- The statics: IDeviceInformationStatics, the ten members in the metadata order the windows-rs crate's
 * IDeviceInformationStatics_Vtbl fixes. ---- */

typedef struct device_statics_vtbl device_statics_vtbl;
struct device_statics_vtbl {
	BEGIN_INTERFACE
	HRESULT (WINAPI *QueryInterface)(IInspectable *This, REFIID riid, void **object);
	ULONG (WINAPI *AddRef)(IInspectable *This);
	ULONG (WINAPI *Release)(IInspectable *This);
	HRESULT (WINAPI *GetIids)(IInspectable *This, ULONG *count, IID **iids);
	HRESULT (WINAPI *GetRuntimeClassName)(IInspectable *This, HSTRING *name);
	HRESULT (WINAPI *GetTrustLevel)(IInspectable *This, TrustLevel *level);
	HRESULT (WINAPI *CreateFromIdAsync)(IInspectable *This, HSTRING device_id, IInspectable **operation);
	HRESULT (WINAPI *CreateFromIdAsyncAdditionalProperties)(IInspectable *This, HSTRING device_id, void *additional_properties, IInspectable **operation);
	HRESULT (WINAPI *FindAllAsync)(IInspectable *This, IInspectable **operation);
	HRESULT (WINAPI *FindAllAsyncDeviceClass)(IInspectable *This, UINT32 device_class, IInspectable **operation);
	HRESULT (WINAPI *FindAllAsyncAqsFilter)(IInspectable *This, HSTRING filter, IInspectable **operation);
	HRESULT (WINAPI *FindAllAsyncAqsFilterAndAdditionalProperties)(IInspectable *This, HSTRING filter, void *additional_properties, IInspectable **operation);
	HRESULT (WINAPI *CreateWatcher)(IInspectable *This, IInspectable **watcher);
	HRESULT (WINAPI *CreateWatcherDeviceClass)(IInspectable *This, UINT32 device_class, IInspectable **watcher);
	HRESULT (WINAPI *CreateWatcherAqsFilter)(IInspectable *This, HSTRING filter, IInspectable **watcher);
	HRESULT (WINAPI *CreateWatcherAqsFilterAndAdditionalProperties)(IInspectable *This, HSTRING filter, void *additional_properties, IInspectable **watcher);
	END_INTERFACE
};
interface __x_ABI_CWindows_CDevices_CEnumeration_CIDeviceInformationStatics { CONST_VTBL device_statics_vtbl *lpVtbl; };

typedef struct device_statics_object { const device_statics_vtbl *lpVtbl; } device_statics_object;

static HRESULT WINAPI device_statics_query(IInspectable *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &device_statics_iid))
		*out = iface;
	else {
		trace_refused_iid("statics", iid);
		return E_NOINTERFACE;
	}
	return S_OK;
}
static ULONG WINAPI device_statics_addref(IInspectable *iface) { (void)iface; return 2; }
static ULONG WINAPI device_statics_release(IInspectable *iface) { (void)iface; return 1; }
static HRESULT WINAPI device_statics_iids(IInspectable *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI device_statics_name(IInspectable *iface, HSTRING *out)
{ (void)iface; return class_name(out, device_information_class); }
static HRESULT WINAPI device_statics_trust(IInspectable *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }

static HRESULT WINAPI device_statics_create_from_id(IInspectable *iface, HSTRING device_id, IInspectable **operation)
{
	(void)iface; (void)device_id;
	if (!operation)
		return E_POINTER;
	trace_member("CreateFromIdAsync");
	/* the id names a device raven's registry does not carry: the documented not-found completion */
	return device_operation_hand(HRESULT_FROM_WIN32(ERROR_NOT_FOUND), operation);
}
static HRESULT WINAPI device_statics_create_from_id_props(IInspectable *iface, HSTRING device_id, void *additional_properties, IInspectable **operation)
{
	(void)iface; (void)device_id; (void)additional_properties;
	if (!operation)
		return E_POINTER;
	trace_member("CreateFromIdAsyncAdditionalProperties");
	return device_operation_hand(HRESULT_FROM_WIN32(ERROR_NOT_FOUND), operation);
}
static HRESULT WINAPI device_statics_find_all(IInspectable *iface, IInspectable **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	trace_member("FindAllAsync");
	return device_operation_hand(S_OK, operation);
}
static HRESULT WINAPI device_statics_find_all_class(IInspectable *iface, UINT32 device_class, IInspectable **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	{
		char note[96];
		wsprintfA(note, "raven-xaml: device FindAllAsyncDeviceClass class=%u\n", device_class);
		raven_trace_note(note);
	}
	return device_operation_hand(S_OK, operation);
}
static HRESULT WINAPI device_statics_find_all_filter(IInspectable *iface, HSTRING filter, IInspectable **operation)
{
	(void)iface; (void)filter;
	if (!operation)
		return E_POINTER;
	trace_member("FindAllAsyncAqsFilter");
	return device_operation_hand(S_OK, operation);
}
static HRESULT WINAPI device_statics_find_all_filter_props(IInspectable *iface, HSTRING filter, void *additional_properties, IInspectable **operation)
{
	(void)iface; (void)filter; (void)additional_properties;
	if (!operation)
		return E_POINTER;
	trace_member("FindAllAsyncAqsFilterAndAdditionalProperties");
	return device_operation_hand(S_OK, operation);
}
static HRESULT WINAPI device_statics_create_watcher(IInspectable *iface, IInspectable **watcher)
{
	(void)iface;
	if (!watcher)
		return E_POINTER;
	*watcher = NULL;
	trace_member("CreateWatcher");
	return E_NOTIMPL;
}
static HRESULT WINAPI device_statics_create_watcher_class(IInspectable *iface, UINT32 device_class, IInspectable **watcher)
{
	(void)iface;
	if (!watcher)
		return E_POINTER;
	*watcher = NULL;
	trace_member("CreateWatcherDeviceClass");
	return E_NOTIMPL;
}
static HRESULT WINAPI device_statics_create_watcher_filter(IInspectable *iface, HSTRING filter, IInspectable **watcher)
{
	(void)iface; (void)filter;
	if (!watcher)
		return E_POINTER;
	*watcher = NULL;
	trace_member("CreateWatcherAqsFilter");
	return E_NOTIMPL;
}
static HRESULT WINAPI device_statics_create_watcher_filter_props(IInspectable *iface, HSTRING filter, void *additional_properties, IInspectable **watcher)
{
	(void)iface; (void)filter; (void)additional_properties;
	if (!watcher)
		return E_POINTER;
	*watcher = NULL;
	trace_member("CreateWatcherAqsFilterAndAdditionalProperties");
	return E_NOTIMPL;
}
static const device_statics_vtbl device_statics_table = {
	device_statics_query, device_statics_addref, device_statics_release,
	device_statics_iids, device_statics_name, device_statics_trust,
	device_statics_create_from_id, device_statics_create_from_id_props,
	device_statics_find_all, device_statics_find_all_class,
	device_statics_find_all_filter, device_statics_find_all_filter_props,
	device_statics_create_watcher, device_statics_create_watcher_class,
	device_statics_create_watcher_filter, device_statics_create_watcher_filter_props,
};
static device_statics_object device_statics_instance = {&device_statics_table};

/* ---- The activation factory: the class is statics-driven (no default constructor — every instance the statics
 * build), so the factory carries the IActivationFactory face the activation names and the IDeviceInformationStatics
 * face the statics query names. ---- */

typedef struct device_factory_object { const IActivationFactoryVtbl *lpVtbl; } device_factory_object;
static HRESULT WINAPI device_factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory))
		*out = iface;
	else if (equal_iid(iid, &device_statics_iid))
		*out = (IInspectable *)&device_statics_instance;
	else {
		trace_refused_iid("factory", iid);
		return E_NOINTERFACE;
	}
	return S_OK;
}
static ULONG WINAPI device_factory_addref(IActivationFactory *iface) { (void)iface; return 2; }
static ULONG WINAPI device_factory_release(IActivationFactory *iface) { (void)iface; return 1; }
static HRESULT WINAPI device_factory_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI device_factory_name(IActivationFactory *iface, HSTRING *name)
{ (void)iface; return class_name(name, device_information_class); }
static HRESULT WINAPI device_factory_trust(IActivationFactory *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static HRESULT WINAPI device_factory_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	trace_member("ActivateInstance");
	return E_NOTIMPL;
}
static const IActivationFactoryVtbl device_factory_table = {
	device_factory_query, device_factory_addref, device_factory_release,
	device_factory_iids, device_factory_name, device_factory_trust, device_factory_activate,
};
static device_factory_object device_factory_instance = {&device_factory_table};

HRESULT raven_xaml_device_information_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (length != ARRAYSIZE(device_information_class) - 1 ||
		CompareStringOrdinal(text, (int)length, device_information_class, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	*out = (IActivationFactory *)&device_factory_instance;
	return S_OK;
}
