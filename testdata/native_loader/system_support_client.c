#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <roapi.h>
#include <activation.h>
static const IID iid_statics2 = {0x33f349a4,0x3fa1,0x4986,{0xaa,0x4b,0x05,0x74,0x20,0x45,0x5e,0x6d}};
static const IID iid_device = {0x05880b99,0x8247,0x441b,{0xa9,0x96,0xa1,0x78,0x4b,0xab,0x79,0xa8}};
typedef struct part { const void *vtbl; } part;
typedef struct statics_vtbl { IInspectableVtbl base; HRESULT (WINAPI *device)(part *, void **); } statics_vtbl;
typedef struct device_vtbl { IInspectableVtbl base; HRESULT (WINAPI *get[7])(part *, HSTRING *); } device_vtbl;
int raven_entry(void)
{
	static const WCHAR class_name[] = L"Windows.System.Profile.SystemManufacturers.SystemSupportInfo";
	HSTRING key=NULL, value=NULL; part *statics=NULL, *device=NULL; IUnknown *unknown=NULL; HRESULT hr;
	if (WindowsCreateString(class_name, ARRAYSIZE(class_name)-1, &key)!=S_OK) return 1;
	hr=RoGetActivationFactory(key,&iid_statics2,(void **)&statics); WindowsDeleteString(key);
	if (hr!=S_OK || !statics) return 2;
	if (((const statics_vtbl *)statics->vtbl)->device(statics,(void **)&device)!=S_OK || !device) return 3;
	if (((const device_vtbl *)device->vtbl)->base.QueryInterface((IInspectable *)device,&iid_device,(void **)&unknown)!=S_OK || !unknown) return 4;
	unknown->lpVtbl->Release(unknown);
	for (int i=0; i<7; ++i) {
		value=NULL;
		if (((const device_vtbl *)device->vtbl)->get[i](device,&value)!=S_OK) return 5+i;
		if (i==0 && lstrcmpW(WindowsGetStringRawBuffer(value,NULL),L"WINDOWS")) return 12;
		WindowsDeleteString(value);
	}
	((const device_vtbl *)device->vtbl)->base.Release((IInspectable *)device);
	((const statics_vtbl *)statics->vtbl)->base.Release((IInspectable *)statics);
	return 0;
}
