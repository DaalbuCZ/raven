/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* SystemSupportInfo's local device view. Hardware values come from the Windows
 * system-information registry; an unavailable value is an empty string.
 * shortcut: only LocalDeviceInfo is exposed, add the other statics when needed. */
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <winstring.h>

static const IID iid_statics2 = {0x33f349a4,0x3fa1,0x4986,{0xaa,0x4b,0x05,0x74,0x20,0x45,0x5e,0x6d}};
static const IID iid_device = {0x05880b99,0x8247,0x441b,{0xa9,0x96,0xa1,0x78,0x4b,0xab,0x79,0xa8}};
static const WCHAR class_name[] = L"Windows.System.Profile.SystemManufacturers.SystemSupportInfo";
static const WCHAR device_name[] = L"Windows.System.Profile.SystemManufacturers.SystemSupportDeviceInfo";

typedef struct part { const void *vtbl; } part;
typedef struct statics_vtbl { IInspectableVtbl base; HRESULT (WINAPI *local_device)(part *, void **); } statics_vtbl;
typedef struct device_vtbl { IInspectableVtbl base; HRESULT (WINAPI *os)(part *, HSTRING *); HRESULT (WINAPI *friendly)(part *, HSTRING *); HRESULT (WINAPI *manufacturer)(part *, HSTRING *); HRESULT (WINAPI *product)(part *, HSTRING *); HRESULT (WINAPI *sku)(part *, HSTRING *); HRESULT (WINAPI *hardware)(part *, HSTRING *); HRESULT (WINAPI *firmware)(part *, HSTRING *); } device_vtbl;
static part activation, statics, device;
static int eq(REFIID a, const IID *b) { return !memcmp(a,b,sizeof(*b)); }
static HRESULT WINAPI query(IInspectable *self, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (self == (IInspectable *)&device) {
		if (eq(iid,&IID_IUnknown) || eq(iid,&IID_IInspectable) || eq(iid,&IID_IAgileObject) || eq(iid,&iid_device)) *out = &device;
	} else {
		if (eq(iid,&IID_IUnknown) || eq(iid,&IID_IInspectable) || eq(iid,&IID_IAgileObject) || eq(iid,&IID_IActivationFactory)) *out = &activation;
		else if (eq(iid,&iid_statics2)) *out = &statics;
	}
	return *out ? S_OK : E_NOINTERFACE;
}
static ULONG WINAPI addref(IInspectable *self) { (void)self; return 2; }
static ULONG WINAPI release(IInspectable *self) { (void)self; return 1; }
static HRESULT WINAPI iids(IInspectable *self, ULONG *count, IID **out) { (void)self; if (!count || !out) return E_POINTER; *count=0; *out=NULL; return S_OK; }
static HRESULT WINAPI name(IInspectable *self, HSTRING *out) { const WCHAR *value = self == (IInspectable *)&device ? device_name : class_name; if (!out) return E_POINTER; return WindowsCreateString(value,lstrlenW(value),out); }
static HRESULT WINAPI trust(IInspectable *self, TrustLevel *out) { (void)self; if (!out) return E_POINTER; *out=BaseTrust; return S_OK; }
static HRESULT WINAPI activate(IActivationFactory *self, IInspectable **out) { (void)self; if (!out) return E_POINTER; *out=NULL; return E_NOTIMPL; }
static HRESULT WINAPI local_device(part *self, void **out) { (void)self; if (!out) return E_POINTER; *out=&device; return S_OK; }
static HRESULT make(HSTRING *out, const WCHAR *value) { if (!out) return E_POINTER; return WindowsCreateString(value,lstrlenW(value),out); }
static HRESULT WINAPI os(part *self,HSTRING *out) { (void)self; return make(out,L"WINDOWS"); }
static HRESULT WINAPI friendly(part *self,HSTRING *out) { WCHAR value[MAX_COMPUTERNAME_LENGTH+1]; DWORD size=ARRAYSIZE(value); (void)self; if (!GetComputerNameW(value,&size)) value[0]=0; return make(out,value); }
static HRESULT registry_string(const WCHAR *field,HSTRING *out) { WCHAR value[512]={0}; DWORD size=sizeof(value); if (RegGetValueW(HKEY_LOCAL_MACHINE,L"HARDWARE\\DESCRIPTION\\System\\BIOS",field,RRF_RT_REG_SZ,NULL,value,&size)!=ERROR_SUCCESS) value[0]=0; return make(out,value); }
#define REG_GETTER(fn, field) static HRESULT WINAPI fn(part *self,HSTRING *out) { (void)self; return registry_string(L##field,out); }
REG_GETTER(manufacturer,"SystemManufacturer")
REG_GETTER(product,"SystemProductName")
REG_GETTER(sku,"SystemSKU")
REG_GETTER(hardware,"SystemVersion")
REG_GETTER(firmware,"BIOSVersion")
static const IActivationFactoryVtbl activation_vtbl = {(void *)query,(void *)addref,(void *)release,(void *)iids,(void *)name,(void *)trust,activate};
static const statics_vtbl statics_table = {{query,addref,release,iids,name,trust},local_device};
static const device_vtbl device_table = {{query,addref,release,iids,name,trust},os,friendly,manufacturer,product,sku,hardware,firmware};
static part activation = {&activation_vtbl}, statics = {&statics_table}, device = {&device_table};
HRESULT raven_system_support_factory(HSTRING key, IActivationFactory **out)
{
	UINT32 length=0; const WCHAR *text=WindowsGetStringRawBuffer(key,&length);
	if (!out) return E_POINTER;
	*out=NULL;
	if (length != ARRAYSIZE(class_name)-1 || CompareStringOrdinal(text,length,class_name,-1,FALSE)!=CSTR_EQUAL) return CLASS_E_CLASSNOTAVAILABLE;
	*out=(IActivationFactory *)&activation;
	return S_OK;
}
