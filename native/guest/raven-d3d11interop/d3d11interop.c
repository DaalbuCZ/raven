/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven Direct3D interop component: the two constructors Windows' d3d11.dll exports for handing a COM device or surface to WinRT,
 * CreateDirect3D11DeviceFromDXGIDevice and CreateDirect3D11SurfaceFromDXGISurface. The Direct3D library the runtime provides
 * (DXVK) does not carry them, so the loader serves a guest's import of them from this component when the Direct3D library lacks
 * them, as d3d11.dll itself does on Windows.
 *
 * Each constructor returns the runtime object Windows provides: an IInspectable over the DXGI device or surface, queryable as
 * IDirect3DDevice or IDirect3DSurface and unwrappable back to the COM pointer through IDirect3DDxgiInterfaceAccess::GetInterface.
 * The interfaces are COM objects written here after the shape of the Windows.Graphics.DirectX.Direct3D11 projection; a call that
 * nothing implements shows up by name in the trace (RAVEN_TRACE_DEBUG=1), the way the Direct2D component reports its gaps. */
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <winstring.h>
#include <dxgi.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* The host trace service, as the other guest components use it. */
__declspec(dllimport) void WINAPI RavenTrace(const char *text);

/* IInspectable is opaque here: the C headers the component builds against do not carry the projection, and the object holds it
 * only as the pointer the constructors hand back. */
typedef interface IInspectable IInspectable;

/* The Windows.Graphics.DirectX.Direct3D11 projection interfaces and IInspectable, whose GUIDs the C headers here do not carry. */
static const GUID raven_IID_IUnknown = {0x00000000, 0x0000, 0x0000, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
static const GUID raven_IID_IInspectable = {0xaf86e2e0, 0xb12d, 0x4c6a, {0x9c, 0x5a, 0xd7, 0xaa, 0x65, 0x10, 0x1e, 0x90}};
static const GUID raven_IID_IDirect3DDevice = {0xe5c6af7d, 0x3684, 0x40be, {0x8d, 0xa8, 0xfc, 0x5a, 0x75, 0xdb, 0xc6, 0xb8}};
static const GUID raven_IID_IDirect3DSurface = {0x150d1540, 0x0581, 0x40cf, {0x8c, 0x5b, 0x24, 0x99, 0xb2, 0x3c, 0xca, 0x7b}};
/* d3d11.h declares the interop unwrapping interface; its GUID stays named here so the component needs no import library for it. */
static const GUID raven_IID_IDirect3DDxgiInterfaceAccess = {0xa9b1d0b9, 0xb3e7, 0x4e96, {0xb6, 0xa0, 0xe4, 0x0f, 0x34, 0xa9, 0xab, 0x25}};

static BOOL same_guid(REFIID a, const GUID *b)
{
	return memcmp(a, b, sizeof(GUID)) == 0;
}

static void trace_unimplemented(const char *interface_name, const char *method)
{
	char text[160];
	size_t at = 0;
	const char *part[] = {"d3d11interop: ", interface_name, "::", method, " is not implemented"};
	for (size_t index = 0; index < 5; ++index)
		for (size_t length = 0; part[index][length] && at < sizeof(text) - 1; ++length)
			text[at++] = part[index][length];
	text[at] = 0;
	RavenTrace(text);
}

static HRESULT no_interface(REFIID iid)
{
	static const char digits[] = "0123456789abcdef";
	static const BYTE order[16] = {3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15};
	char text[64] = "d3d11interop: no interface ";
	size_t at = 27;
	const BYTE *bytes = (const BYTE *)iid;
	for (size_t index = 0; index < 16; ++index) {
		if (index == 4 || index == 6 || index == 8 || index == 10)
			text[at++] = '-';
		text[at++] = digits[bytes[order[index]] >> 4];
		text[at++] = digits[bytes[order[index]] & 15];
	}
	text[at] = 0;
	RavenTrace(text);
	return E_NOINTERFACE;
}

/* The wrapper. Every interface the object carries is one of the four table pointers below; each points at the first slot of a
 * table whose query interface answers with the same pointer for IUnknown, as identity requires. The wrapped DXGI device or
 * surface is held through IUnknown so both constructors share the shape. */
struct interop_object {
	void *unknown;     /* IUnknown */
	void *inspectable; /* IInspectable */
	void *projected;   /* IDirect3DDevice on the device wrapper, IDirect3DSurface on the surface wrapper */
	void *access;      /* IDirect3DDxgiInterfaceAccess */
	LONG refs;
	IUnknown *dxgi;    /* the IDXGIDevice or IDXGISurface handed over */
	BOOL surface;      /* whether the projected interface is IDirect3DSurface */
};

/* The interface pointer is the address of its own field, so the object lives just before it. */
#define WRAPPER(field, pointer) ((struct interop_object *)((char *)(pointer) - offsetof(struct interop_object, field)))

static HRESULT STDMETHODCALLTYPE wrapper_QueryInterface(void *iface, REFIID riid, void **out)
{
	struct interop_object *self;
	void *wanted;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (iface == WRAPPER(unknown, iface)->unknown)
		self = WRAPPER(unknown, iface);
	else if (iface == WRAPPER(inspectable, iface)->inspectable)
		self = WRAPPER(inspectable, iface);
	else if (iface == WRAPPER(projected, iface)->projected)
		self = WRAPPER(projected, iface);
	else
		self = WRAPPER(access, iface);
	if (same_guid(riid, &raven_IID_IUnknown)) {
		wanted = &self->unknown;
	} else if (same_guid(riid, &raven_IID_IInspectable)) {
		wanted = &self->inspectable;
	} else if (same_guid(riid, &raven_IID_IDirect3DDevice) && !self->surface) {
		wanted = &self->projected;
	} else if (same_guid(riid, &raven_IID_IDirect3DSurface) && self->surface) {
		wanted = &self->projected;
	} else if (same_guid(riid, &raven_IID_IDirect3DDxgiInterfaceAccess)) {
		wanted = &self->access;
	} else {
		return no_interface(riid);
	}
	InterlockedIncrement(&self->refs);
	*out = wanted;
	return S_OK;
}

static ULONG STDMETHODCALLTYPE wrapper_AddRef(void *iface)
{
	return (ULONG)InterlockedIncrement(&WRAPPER(unknown, iface)->refs);
}

static ULONG STDMETHODCALLTYPE wrapper_Release(void *iface)
{
	struct interop_object *self = WRAPPER(unknown, iface);
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		IUnknown *dxgi = self->dxgi;
		HeapFree(GetProcessHeap(), 0, self);
		if (dxgi)
			IUnknown_Release(dxgi);
	}
	return (ULONG)refs;
}

/* IInspectable: the runtime object's interfaces, name and trust. */
static HRESULT STDMETHODCALLTYPE wrapper_GetIids(void *iface, ULONG *count, IID **iids)
{
	(void)iface;
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE wrapper_GetRuntimeClassName(void *iface, HSTRING *name)
{
	/* The projected class Windows names: the device wrapper and the surface wrapper report their own. */
	static const WCHAR device_class[] = L"Windows.Graphics.DirectX.Direct3D11.Direct3DDevice";
	static const WCHAR surface_class[] = L"Windows.Graphics.DirectX.Direct3D11.Direct3DSurface";
	struct interop_object *self = WRAPPER(inspectable, iface);
	if (!name)
		return E_POINTER;
	*name = NULL;
	return WindowsCreateString(self->surface ? surface_class : device_class,
		self->surface ? (UINT32)(sizeof(surface_class) / sizeof(WCHAR) - 1) : (UINT32)(sizeof(device_class) / sizeof(WCHAR) - 1), name);
}

static HRESULT STDMETHODCALLTYPE wrapper_GetTrustLevel(void *iface, INT32 *level)
{
	(void)iface;
	if (!level)
		return E_POINTER;
	*level = 0; /* BaseTrust */
	return S_OK;
}

/* IDirect3DDevice carries no methods of its own: it is the IInspectable token the graphics interop passes around. */

/* IDirect3DSurface::Description hands out the surface's shape as a Windows.Foundation IReference of the projected struct, whose
   parameterized interface this component does not build; a caller that needs it shows here by name. */
static HRESULT STDMETHODCALLTYPE wrapper_GetDescription(void *iface, void **description)
{
	(void)iface;
	if (!description)
		return E_POINTER;
	*description = NULL;
	trace_unimplemented("IDirect3DSurface", "get_Description");
	return E_NOTIMPL;
}

/* IDirect3DDxgiInterfaceAccess::GetInterface: the way back from the runtime object to the COM device or surface. */
static HRESULT STDMETHODCALLTYPE wrapper_GetInterface(void *iface, REFIID riid, void **out)
{
	struct interop_object *self = WRAPPER(access, iface);
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!self->dxgi)
		return E_NOINTERFACE;
	return IUnknown_QueryInterface(self->dxgi, riid, out);
}

/* IUnknown's three slots, then IInspectable's three, are the head of every WinRT interface table. */
static void *unknown_table[3];
static void *inspectable_table[6];
static void *device_table[6];
static void *surface_table[7];
static void *access_table[7];

static BOOL CALLBACK tables_init(PINIT_ONCE once, void *parameter, void **context)
{
	(void)once; (void)parameter; (void)context;
	unknown_table[0] = (void *)wrapper_QueryInterface;
	unknown_table[1] = (void *)wrapper_AddRef;
	unknown_table[2] = (void *)wrapper_Release;
	memcpy(inspectable_table, unknown_table, sizeof(unknown_table));
	inspectable_table[3] = (void *)wrapper_GetIids;
	inspectable_table[4] = (void *)wrapper_GetRuntimeClassName;
	inspectable_table[5] = (void *)wrapper_GetTrustLevel;
	memcpy(device_table, inspectable_table, sizeof(inspectable_table));
	memcpy(surface_table, inspectable_table, sizeof(inspectable_table));
	surface_table[6] = (void *)wrapper_GetDescription;
	memcpy(access_table, inspectable_table, sizeof(inspectable_table));
	access_table[6] = (void *)wrapper_GetInterface;
	return TRUE;
}

static INIT_ONCE tables_once = INIT_ONCE_STATIC_INIT;

static HRESULT wrapper_create(IUnknown *dxgi, BOOL surface, void **out)
{
	struct interop_object *self;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!dxgi)
		return E_POINTER;
	InitOnceExecuteOnce(&tables_once, tables_init, NULL, NULL);
	if (!(self = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*self))))
		return E_OUTOFMEMORY;
	self->unknown = unknown_table;
	self->inspectable = inspectable_table;
	self->projected = surface ? surface_table : device_table;
	self->access = access_table;
	self->refs = 1;
	self->dxgi = dxgi;
	self->surface = surface;
	IUnknown_AddRef(dxgi);
	/* The constructor hands back the IInspectable of the runtime object. */
	*out = &self->inspectable;
	return S_OK;
}

/* The Direct3D library's exports, served here when the library itself lacks them. */
HRESULT WINAPI CreateDirect3D11DeviceFromDXGIDevice(IDXGIDevice *device, IInspectable **out)
{
	return wrapper_create((IUnknown *)device, FALSE, (void **)out);
}

HRESULT WINAPI CreateDirect3D11SurfaceFromDXGISurface(IDXGISurface *surface, IUnknown **out)
{
	return wrapper_create((IUnknown *)surface, TRUE, (void **)out);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
	(void)reserved;
	if (reason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(instance);
	return TRUE;
}
