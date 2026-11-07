/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: Windows.Foundation.Metadata.ApiInformation, the class a runtime and its
 * compiled code use to ask which Windows APIs are present. Raven's answer is its own catalog: a type
 * is present when the runtime's activation catalog serves it (the hive the guest's RoGetActivationFactory
 * reads), and a member of a served type answers as its type does — the component carries whole classes,
 * not per-member records. Contracts answer by name for the ones the served surface was built against,
 * with the version the mounted metadata records; an unknown contract is absent. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <winstring.h>
#include <windows.foundation.metadata.h>
#include <winreg.h>

static const WCHAR class_name[] = L"Windows.Foundation.Metadata.ApiInformation";

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

typedef __x_ABI_CWindows_CFoundation_CMetadata_CIApiInformationStatics statics_i;
typedef __x_ABI_CWindows_CFoundation_CMetadata_CIApiInformationStaticsVtbl statics_iVtbl;

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

static int equal_iid(REFIID left, const IID *right)
{
	return left->Data1 == right->Data1 && left->Data2 == right->Data2 && left->Data3 == right->Data3 &&
		left->Data4[0] == right->Data4[0] && left->Data4[1] == right->Data4[1] &&
		left->Data4[2] == right->Data4[2] && left->Data4[3] == right->Data4[3] &&
		left->Data4[4] == right->Data4[4] && left->Data4[5] == right->Data4[5] &&
		left->Data4[6] == right->Data4[6] && left->Data4[7] == right->Data4[7];
}

static HRESULT create_string(const WCHAR *text, HSTRING *out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!text)
		return S_OK;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), out);
}

/* The type question is the catalog question: the hive key the guest's activation reads. */
static boolean type_present(const WCHAR *text)
{
	WCHAR key[256];
	static const WCHAR root[] = L"SOFTWARE\\Microsoft\\WindowsRuntime\\ActivatableClassId\\";
	static const WCHAR value[] = L"DllPath";
	BYTE raw[MAX_PATH];
	DWORD type = 0, size = sizeof raw;
	lstrcpyW(key, root);
	if (lstrlenW(text) >= MAX_PATH - ARRAYSIZE(root))
		return FALSE;
	lstrcpynW(key + ARRAYSIZE(root) - 1, text, (int)lstrlenW(text) + 1);
	if (RegGetValueW(HKEY_LOCAL_MACHINE, key, value, RRF_RT_REG_SZ, &type, raw, &size) != ERROR_SUCCESS)
		return FALSE;
	return TRUE;
}

static HRESULT text_of(HSTRING value, const WCHAR **text)
{
	if (!text)
		return E_POINTER;
	*text = WindowsGetStringRawBuffer(value, NULL);
	return S_OK;
}

static const statics_iVtbl statics_table;

static statics_i statics_instance; /* the object the statics interface serves; defined after its table */

static HRESULT WINAPI statics_query(statics_i *self, REFIID iid, void **out)
{
	(void)self;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID___x_ABI_CWindows_CFoundation_CMetadata_CIApiInformationStatics) &&
		!equal_iid(iid, &IID_IAgileObject))
		return E_NOINTERFACE;
	*out = (void *)&statics_instance;
	return S_OK;
}

static ULONG WINAPI statics_addref(statics_i *self) { (void)self; return 2; }
static ULONG WINAPI statics_release(statics_i *self) { (void)self; return 1; }

static HRESULT WINAPI statics_iids(statics_i *self, ULONG *count, IID **iids)
{
	(void)self;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI statics_class(statics_i *self, HSTRING *out)
{
	(void)self;
	return create_string(L"Windows.Foundation.Metadata.IApiInformationStatics", out);
}

static HRESULT WINAPI statics_trust(statics_i *self, TrustLevel *level)
{
	(void)self;
	return inspectable_trust(level);
}

static HRESULT WINAPI statics_type_present(statics_i *self, HSTRING type_name, boolean *result)
{
	const WCHAR *text;
	HRESULT status = text_of(type_name, &text);
	(void)self;
	if (FAILED(status))
		return status;
	if (!result)
		return E_POINTER;
	*result = type_present(text);
	return S_OK;
}

/* A member of a served type answers as its type: the components carry whole classes. */
static HRESULT WINAPI statics_method_present(statics_i *self, HSTRING type_name, HSTRING method_name, boolean *result)
{
	const WCHAR *text;
	HRESULT status = text_of(type_name, &text);
	(void)self;
	(void)method_name;
	if (FAILED(status))
		return status;
	if (!result)
		return E_POINTER;
	*result = type_present(text);
	return S_OK;
}

static HRESULT WINAPI statics_method_present_arity(statics_i *self, HSTRING type_name, HSTRING method_name,
	UINT32 parameter_count, boolean *result)
{
	(void)parameter_count;
	return statics_method_present(self, type_name, method_name, result);
}

static HRESULT WINAPI statics_event_present(statics_i *self, HSTRING type_name, HSTRING event_name, boolean *result)
{
	(void)event_name;
	return statics_method_present(self, type_name, NULL, result);
}

static HRESULT WINAPI statics_property_present(statics_i *self, HSTRING type_name, HSTRING property_name, boolean *result)
{
	(void)property_name;
	return statics_method_present(self, type_name, NULL, result);
}

static HRESULT WINAPI statics_read_only_property_present(statics_i *self, HSTRING type_name, HSTRING property_name,
	boolean *result)
{
	(void)property_name;
	return statics_method_present(self, type_name, NULL, result);
}

static HRESULT WINAPI statics_writeable_property_present(statics_i *self, HSTRING type_name, HSTRING property_name,
	boolean *result)
{
	(void)property_name;
	return statics_method_present(self, type_name, NULL, result);
}

static HRESULT WINAPI statics_enum_named_value_present(statics_i *self, HSTRING enum_type_name, HSTRING value_name,
	boolean *result)
{
	(void)value_name;
	/* the enum's type is served whole, like any other */
	return statics_method_present(self, enum_type_name, NULL, result);
}

/* The contracts the served surface was built against, with the versions the mounted Windows metadata records.
 * The UniversalApiContract of the Windows this runtime's metadata describes reaches its recorded major; a
 * request for a contract the surface does not name is absent. The mounted metadata is Windows 10.0.26100
 * (24H2), whose UniversalApiContract is v15 — answering the versions above it steers feature-gated callers
 * onto code paths a real Windows never takes, which for the traced Shapr3D run meant an interop round
 * whose marshalling data the toolchain never compiled. */
static HRESULT WINAPI statics_contract_present_major(statics_i *self, HSTRING contract_name, UINT16 major, boolean *result)
{
	const WCHAR *text;
	HRESULT status = text_of(contract_name, &text);
	(void)self;
	if (FAILED(status))
		return status;
	if (!result)
		return E_POINTER;
	*result = FALSE;
	if (!lstrcmpW(text, L"Windows.Foundation.UniversalApiContract"))
		*result = major > 0 && major <= 15;
	return S_OK;
}

static HRESULT WINAPI statics_contract_present_major_minor(statics_i *self, HSTRING contract_name, UINT16 major,
	UINT16 minor, boolean *result)
{
	(void)minor;
	return statics_contract_present_major(self, contract_name, major, result);
}

static const statics_iVtbl statics_table = {
	statics_query, statics_addref, statics_release, statics_iids, statics_class, statics_trust,
	statics_type_present, statics_method_present, statics_method_present_arity, statics_event_present,
	statics_property_present, statics_read_only_property_present, statics_writeable_property_present,
	statics_enum_named_value_present, statics_contract_present_major, statics_contract_present_major_minor,
};

static statics_i statics_instance = {&statics_table};

static const IActivationFactoryVtbl factory_table;

static struct {
	const IActivationFactoryVtbl *activation;
	const statics_iVtbl *statics;
	LONG refs;
} api_information = {&factory_table, &statics_table, 1};

static HRESULT WINAPI factory_query(IActivationFactory *self, REFIID iid, void **out)
{
	(void)self;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID_IActivationFactory) || equal_iid(iid, &IID_IAgileObject))
		*out = (void *)&api_information.activation;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CFoundation_CMetadata_CIApiInformationStatics))
		*out = (void *)&statics_instance;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&api_information.refs);
	return S_OK;
}

static ULONG WINAPI factory_addref(IActivationFactory *self)
{
	(void)self;
	return (ULONG)InterlockedIncrement(&api_information.refs);
}

static ULONG WINAPI factory_release(IActivationFactory *self)
{
	(void)self;
	return (ULONG)InterlockedDecrement(&api_information.refs);
}

static HRESULT WINAPI factory_iids(IActivationFactory *self, ULONG *count, IID **iids)
{
	(void)self;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI factory_class(IActivationFactory *self, HSTRING *out)
{
	(void)self;
	return create_string(class_name, out);
}

static HRESULT WINAPI factory_trust(IActivationFactory *self, TrustLevel *out)
{
	(void)self;
	return inspectable_trust(out);
}

static HRESULT WINAPI factory_activate(IActivationFactory *self, IInspectable **out)
{
	(void)self;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return E_NOTIMPL; /* the class carries only statics */
}

static const IActivationFactoryVtbl factory_table = {
	factory_query, factory_addref, factory_release, factory_iids, factory_class, factory_trust, factory_activate,
};

HRESULT raven_api_information_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (length != ARRAYSIZE(class_name) - 1 ||
		CompareStringOrdinal(text, (int)length, class_name, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	InterlockedIncrement(&api_information.refs);
	*out = (IActivationFactory *)&api_information.activation;
	return S_OK;
}
