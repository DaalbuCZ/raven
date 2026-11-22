#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <asyncinfo.h>

typedef HRESULT (WINAPI *factory_function)(HSTRING, IActivationFactory **);

typedef enum { my_trust_base = 0 } my_trust;

/* The faces, self-declared: every vtable opens with the six IUnknown+IInspectable slots, then the metadata-order
 * members — the shapes raven's objects implement and .NET Native is compiled against. */
typedef struct device_statics_vtbl {
	HRESULT (WINAPI *QueryInterface)(void *, REFIID, void **);
	ULONG (WINAPI *AddRef)(void *);
	ULONG (WINAPI *Release)(void *);
	HRESULT (WINAPI *GetIids)(void *, ULONG *, IID **);
	HRESULT (WINAPI *GetRuntimeClassName)(void *, HSTRING *);
	HRESULT (WINAPI *GetTrustLevel)(void *, my_trust *);
	HRESULT (WINAPI *CreateFromIdAsync)(void *, HSTRING, void **);
	HRESULT (WINAPI *CreateFromIdAsyncAdditionalProperties)(void *, HSTRING, void *, void **);
	HRESULT (WINAPI *FindAllAsync)(void *, void **);
	HRESULT (WINAPI *FindAllAsyncDeviceClass)(void *, UINT32, void **);
	HRESULT (WINAPI *FindAllAsyncAqsFilter)(void *, HSTRING, void **);
	HRESULT (WINAPI *FindAllAsyncAqsFilterAndAdditionalProperties)(void *, HSTRING, void *, void **);
	HRESULT (WINAPI *CreateWatcher)(void *, void **);
	HRESULT (WINAPI *CreateWatcherDeviceClass)(void *, UINT32, void **);
	HRESULT (WINAPI *CreateWatcherAqsFilter)(void *, HSTRING, void **);
	HRESULT (WINAPI *CreateWatcherAqsFilterAndAdditionalProperties)(void *, HSTRING, void *, void **);
} device_statics_vtbl;
typedef struct device_statics_i { CONST_VTBL device_statics_vtbl *lpVtbl; } device_statics_i;
typedef struct device_op_vtbl {
	HRESULT (WINAPI *QueryInterface)(void *, REFIID, void **);
	ULONG (WINAPI *AddRef)(void *);
	ULONG (WINAPI *Release)(void *);
	HRESULT (WINAPI *GetIids)(void *, ULONG *, IID **);
	HRESULT (WINAPI *GetRuntimeClassName)(void *, HSTRING *);
	HRESULT (WINAPI *GetTrustLevel)(void *, my_trust *);
	HRESULT (WINAPI *put_Completed)(void *, void *);
	HRESULT (WINAPI *get_Completed)(void *, void **);
	HRESULT (WINAPI *GetResults)(void *, void **);
} device_op_vtbl;
typedef struct device_vector_vtbl {
	HRESULT (WINAPI *QueryInterface)(void *, REFIID, void **);
	ULONG (WINAPI *AddRef)(void *);
	ULONG (WINAPI *Release)(void *);
	HRESULT (WINAPI *GetIids)(void *, ULONG *, IID **);
	HRESULT (WINAPI *GetRuntimeClassName)(void *, HSTRING *);
	HRESULT (WINAPI *GetTrustLevel)(void *, my_trust *);
	HRESULT (WINAPI *GetAt)(void *, UINT32, void **);
	HRESULT (WINAPI *get_Size)(void *, UINT32 *);
	HRESULT (WINAPI *IndexOf)(void *, void *, UINT32 *, boolean *);
	HRESULT (WINAPI *GetMany)(void *, UINT32, UINT32, void **, UINT32 *);
} device_vector_vtbl;
typedef struct device_info_vtbl {
	HRESULT (WINAPI *QueryInterface)(void *, REFIID, void **);
	ULONG (WINAPI *AddRef)(void *);
	ULONG (WINAPI *Release)(void *);
	HRESULT (WINAPI *GetIids)(void *, ULONG *, IID **);
	HRESULT (WINAPI *GetRuntimeClassName)(void *, HSTRING *);
	HRESULT (WINAPI *GetTrustLevel)(void *, my_trust *);
	HRESULT (WINAPI *get_Id)(void *, UINT32 *);
	HRESULT (WINAPI *get_Status)(void *, AsyncStatus *);
	HRESULT (WINAPI *get_ErrorCode)(void *, HRESULT *);
	HRESULT (WINAPI *Cancel)(void *);
	HRESULT (WINAPI *Close)(void *);
} device_info_vtbl;

/* The statics face, on record: the identifier the traced launch's refused RoGetActivationFactory named. */
static const IID device_statics_iid = {0xc17f100e, 0x3a46, 0x4a78, {0x80, 0x13, 0x76, 0x9d, 0xc9, 0xb9, 0x73, 0x90}};
/* The parameterized faces, on record: the SHA-1 instance identifiers over the generic signatures. */
static const IID device_vector_iid = {0xe170688f, 0x3495, 0x5bf6, {0xaa, 0xb5, 0x9c, 0xac, 0x17, 0xe0, 0xf1, 0x0f}};
static const IID device_operation_iid = {0xf02f08f2, 0xf30f, 0x5b83, {0xac, 0xc5, 0x57, 0xe1, 0xf9, 0xda, 0xa0, 0xdd}};
/* A face nothing here carries: the refusal contract check. */
static const IID stranger_iid = {0xc17f100e, 0x3a46, 0x4a78, {0x80, 0x13, 0x76, 0x9d, 0xc9, 0xb9, 0x73, 0x91}};

static const WCHAR class_text[] = L"Windows.Devices.Enumeration.DeviceInformation";
static const WCHAR operation_text[] =
	L"Windows.Foundation.IAsyncOperation`1<Windows.Foundation.Collections.IVectorView`1<Windows.Devices.Enumeration.DeviceInformation>>";

static HSTRING class_string(const WCHAR *text)
{
	HSTRING value = NULL;
	if (WindowsCreateString(text, lstrlenW(text), &value) != S_OK)
		return NULL;
	return value;
}

static BOOL string_is(HSTRING value, const WCHAR *expected)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(value, &length);
	return (UINT32)lstrlenW(expected) == length &&
		CompareStringOrdinal(text, (int)length, expected, -1, FALSE) == CSTR_EQUAL;
}

/* the handler shape the operations invoke: query, addref, release, invoke */
struct handler_vtbl {
	HRESULT (WINAPI *QueryInterface)(void *, REFIID, void **);
	ULONG (WINAPI *AddRef)(void *);
	ULONG (WINAPI *Release)(void *);
	HRESULT (WINAPI *Invoke)(void *, void *, AsyncStatus);
};
static int handler_calls;
static AsyncStatus handler_status;
static HRESULT WINAPI handler_qi(void *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IAgileObject)) { *out = iface; return S_OK; }
	return E_NOINTERFACE;
}
static ULONG WINAPI handler_addref(void *iface) { (void)iface; return 2; }
static ULONG WINAPI handler_release(void *iface) { (void)iface; return 1; }
static HRESULT WINAPI handler_invoke(void *iface, void *operation, AsyncStatus status)
{
	(void)iface; (void)operation;
	handler_calls++;
	handler_status = status;
	return S_OK;
}
static const struct handler_vtbl handler_table = {handler_qi, handler_addref, handler_release, handler_invoke};
static struct { const struct handler_vtbl *lpVtbl; } handler_instance = {&handler_table};

/* One shape fits the operations the fixture awaits: put_Completed rides slot 6 and GetResults slot 8, and the
 * operation's GetRuntimeClassName names the parameterized instance the await casts for. */
static void *await_operation(void *operation, AsyncStatus *status, HSTRING *type_name)
{
	device_op_vtbl *table = *(device_op_vtbl **)operation;
	void *result = NULL;
	handler_calls = 0;
	handler_status = (AsyncStatus)0;
	if (table->put_Completed(operation, &handler_instance) != S_OK)
		return NULL;
	if (handler_calls != 1)
		return NULL;
	if (status)
		*status = handler_status;
	if (type_name) {
		*type_name = NULL;
		if (table->GetRuntimeClassName(operation, type_name) != S_OK || !*type_name)
			return NULL;
	}
	if (table->GetResults(operation, &result) != S_OK)
		return NULL;
	return result;
}

int raven_entry(void)
{
	HMODULE module = LoadLibraryW(L"raven-xaml.dll");
	factory_function get_factory;
	HSTRING class_key = NULL, filter_key = NULL, foreign_key = NULL;
	IActivationFactory *factory = NULL;
	device_statics_i *statics = NULL;
	void *operation = NULL, *vector = NULL;
	void *unknown = NULL;
	AsyncStatus status = (AsyncStatus)0;
	UINT32 size = 0;
	HRESULT op_error = S_OK;

	if (!module)
		return 1;
	get_factory = (factory_function)(void *)GetProcAddress(module, "DllGetActivationFactory");
	if (!get_factory)
		return 2;
	if (!(class_key = class_string(class_text)))
		return 3;
	if (get_factory(class_key, &factory) != S_OK || !factory)
		return 4;
	/* the statics face the traced launch's activation named */
	if (factory->lpVtbl->QueryInterface(factory, &device_statics_iid, &unknown) != S_OK || !unknown)
		return 5;
	statics = (device_statics_i *)unknown;
	unknown = NULL;
	/* the factory refuses a face neither the activation nor the statics carry */
	if (factory->lpVtbl->QueryInterface(factory, &stranger_iid, &unknown) != E_NOINTERFACE || unknown)
		return 6;
	/* the FindAllAsync await: the handler fires with Completed and the results are the empty set */
	if (statics->lpVtbl->FindAllAsync(statics, &operation) != S_OK || !operation)
		return 7;
	if (!(vector = await_operation(operation, &status, NULL)) || status != Completed)
		return 8;
	{
		device_vector_vtbl *vector_table = *(device_vector_vtbl **)vector;
		void *element = NULL;
		boolean found = 0;
		UINT32 index = 0;
		if (vector_table->get_Size(vector, &size) != S_OK || size != 0)
			return 9;
		if (vector_table->GetAt(vector, 0, &element) != E_BOUNDS)
			return 10;
		if (vector_table->IndexOf(vector, NULL, &index, &found) != S_OK || found)
			return 11;
		/* the vector's parameterized face, on record */
		if (((IUnknown *)vector)->lpVtbl->QueryInterface((IUnknown *)vector, &device_vector_iid, &unknown) != S_OK || !unknown)
			return 12;
		((IUnknown *)unknown)->lpVtbl->Release((IUnknown *)unknown);
		unknown = NULL;
	}
	/* the operation names the parameterized instance the await casts for */
	{
		device_op_vtbl *op_table = *(device_op_vtbl **)operation;
		HSTRING name = NULL;
		if (op_table->GetRuntimeClassName(operation, &name) != S_OK || !name)
			return 13;
		if (!string_is(name, operation_text)) {
			WindowsDeleteString(name);
			return 14;
		}
		WindowsDeleteString(name);
		if (op_table->QueryInterface(operation, &device_operation_iid, &unknown) != S_OK || !unknown)
			return 15;
		((IUnknown *)unknown)->lpVtbl->Release((IUnknown *)unknown);
		unknown = NULL;
	}
	{
		device_op_vtbl *op_table = *(device_op_vtbl **)operation;
		device_info_vtbl *info;
		if (op_table->QueryInterface(operation, &IID_IAsyncInfo, &unknown) != S_OK || !unknown)
			return 16;
		info = (device_info_vtbl *)unknown;
		if (info->get_Status(unknown, &status) != S_OK || status != Completed)
			return 17;
		((IUnknown *)unknown)->lpVtbl->Release((IUnknown *)unknown);
		unknown = NULL;
	}
	((IUnknown *)operation)->lpVtbl->Release((IUnknown *)operation);
	operation = NULL;
	/* the filter overload answers the same empty set */
	if (!(filter_key = class_string(L"System.Devices.AepProtocol:All")))
		return 18;
	if (statics->lpVtbl->FindAllAsyncAqsFilter(statics, filter_key, &operation) != S_OK || !operation)
		return 19;
	if (!(vector = await_operation(operation, &status, NULL)) || status != Completed)
		return 20;
	{
		device_vector_vtbl *vector_table = *(device_vector_vtbl **)vector;
		if (vector_table->get_Size(vector, &size) != S_OK || size != 0)
			return 21;
	}
	((IUnknown *)vector)->lpVtbl->Release((IUnknown *)vector);
	vector = NULL;
	((IUnknown *)operation)->lpVtbl->Release((IUnknown *)operation);
	operation = NULL;
	/* a device id raven's registry does not carry completes with the documented not-found, through IAsyncInfo */
	if (!(foreign_key = class_string(L"USB#VID_0000&PID_0000#0000")))
		return 22;
	if (statics->lpVtbl->CreateFromIdAsync(statics, foreign_key, &operation) != S_OK || !operation)
		return 23;
	{
		device_op_vtbl *op_table = *(device_op_vtbl **)operation;
		device_info_vtbl *info;
		handler_calls = 0;
		if (op_table->put_Completed(operation, &handler_instance) != S_OK || handler_calls != 1)
			return 24;
		if (handler_status != Error)
			return 25;
		if (op_table->QueryInterface(operation, &IID_IAsyncInfo, &unknown) != S_OK || !unknown)
			return 26;
		info = (device_info_vtbl *)unknown;
		if (info->get_ErrorCode(unknown, &op_error) != S_OK || op_error != (HRESULT)0x80070490)
			return 27;
		((IUnknown *)unknown)->lpVtbl->Release((IUnknown *)unknown);
		unknown = NULL;
	}
	((IUnknown *)operation)->lpVtbl->Release((IUnknown *)operation);
	operation = NULL;
	/* the DeviceWatcher class is not served yet: the member refuses honestly */
	if (statics->lpVtbl->CreateWatcher(statics, &unknown) != E_NOTIMPL || unknown)
		return 28;
	/* the statics face names the runtime class */
	{
		HSTRING name = NULL;
		if (statics->lpVtbl->GetRuntimeClassName(statics, &name) != S_OK || !name)
			return 29;
		if (!string_is(name, class_text)) {
			WindowsDeleteString(name);
			return 30;
		}
		WindowsDeleteString(name);
	}
	statics->lpVtbl->Release(statics);
	factory->lpVtbl->Release(factory);
	WindowsDeleteString(filter_key);
	WindowsDeleteString(foreign_key);
	WindowsDeleteString(class_key);
	FreeLibrary(module);
	return 0;
}
