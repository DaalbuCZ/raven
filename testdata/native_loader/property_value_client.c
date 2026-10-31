/* SPDX-License-Identifier: MIT */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <roapi.h>
#include <windows.foundation.h>
#include "windows_ui_xaml_dictionary.h"

typedef __x_ABI_CWindows_CFoundation_CIPropertyValueStatics statics_i;
typedef __x_ABI_CWindows_CFoundation_CIPropertyValue value_i;

static BOOL same(HSTRING value, const WCHAR *text)
{
 UINT32 length=0; const WCHAR *p=WindowsGetStringRawBuffer(value,&length);
 return length==(UINT32)lstrlenW(text) && CompareStringOrdinal(p,length,text,-1,FALSE)==CSTR_EQUAL;
}
#define CHECK_SCALAR(Name, Ctype, Reference, expected, wrong) do { \
 IInspectable *object=NULL; value_i *value=NULL; Reference *reference=NULL; Ctype x=0; \
 if (statics->lpVtbl->Create##Name(statics,(Ctype)(expected),&object)!=S_OK || !object) return wrong; \
 if (IInspectable_QueryInterface(object,&IID___x_ABI_CWindows_CFoundation_CIPropertyValue,(void **)&value)!=S_OK || !value) return wrong; \
 if (value->lpVtbl->Get##Name(value,&x)!=S_OK || x!=(Ctype)(expected)) return wrong; \
 if (IInspectable_QueryInterface(object,&IID_##Reference,(void **)&reference)!=S_OK || !reference) return wrong; \
 x=0; if (reference->lpVtbl->get_Value(reference,&x)!=S_OK || x!=(Ctype)(expected)) return wrong; \
 reference->lpVtbl->Release(reference); value->lpVtbl->Release(value); IInspectable_Release(object); \
} while(0)
int raven_entry(void)
{
 HSTRING class_key=NULL, text=NULL, actual=NULL; IActivationFactory *activation=NULL;
 statics_i *statics=NULL; IInspectable *object=NULL; value_i *value=NULL; __FIReference_1_HSTRING *reference=NULL;
 __x_ABI_CWindows_CFoundation_CPropertyType type; boolean numeric=0; INT32 number=0;
 if (FAILED(RoInitialize(1))) return 1;
 if (WindowsCreateString(L"Windows.Foundation.PropertyValue",32,&class_key)!=S_OK) return 2;
 if (RoGetActivationFactory(class_key,&IID_IActivationFactory,(void **)&activation)!=S_OK || !activation) return 3;
 if (activation->lpVtbl->QueryInterface(activation,&IID___x_ABI_CWindows_CFoundation_CIPropertyValueStatics,(void **)&statics)!=S_OK || !statics) return 4;
 if (WindowsCreateString(L"hello",5,&text)!=S_OK) return 5;
 if (statics->lpVtbl->CreateString(statics,text,&object)!=S_OK || !object) return 6;
 WindowsDeleteString(text); text=NULL;
 if (IInspectable_QueryInterface(object,&IID___x_ABI_CWindows_CFoundation_CIPropertyValue,(void **)&value)!=S_OK || !value) return 7;
 if (value->lpVtbl->get_Type(value,&type)!=S_OK || (int)type!=12) return 8;
 if (value->lpVtbl->get_IsNumericScalar(value,&numeric)!=S_OK || numeric) return 9;
 if (value->lpVtbl->GetString(value,&actual)!=S_OK || !same(actual,L"hello")) return 10;
 WindowsDeleteString(actual); actual=NULL;
 if (value->lpVtbl->GetInt32(value,&number)!=TYPE_E_TYPEMISMATCH) return 11;
 if (IInspectable_QueryInterface(object,&IID___FIReference_1_HSTRING,(void **)&reference)!=S_OK || !reference) return 12;
 if (reference->lpVtbl->get_Value(reference,&actual)!=S_OK || !same(actual,L"hello")) return 13;
 WindowsDeleteString(actual); reference->lpVtbl->Release(reference); value->lpVtbl->Release(value); IInspectable_Release(object);
 CHECK_SCALAR(Int32,INT32,__FIReference_1_INT32,-123,14);
 CHECK_SCALAR(UInt32,UINT32,__FIReference_1_UINT32,0xf1234567U,15);
 CHECK_SCALAR(Int64,INT64,__FIReference_1_INT64,-1234567890123LL,16);
 CHECK_SCALAR(UInt64,UINT64,__FIReference_1_UINT64,0xf123456789123456ULL,17);
 CHECK_SCALAR(Single,FLOAT,__FIReference_1_FLOAT,2.5f,18);
 CHECK_SCALAR(Double,DOUBLE,__FIReference_1_DOUBLE,-9.25,19);
 CHECK_SCALAR(Boolean,boolean,__FIReference_1_boolean,1,20);
 object=(void *)1; if (statics->lpVtbl->CreateUInt8(statics,3,&object)!=E_NOTIMPL || object) return 21;
 if (statics->lpVtbl->CreateInt32(statics,1,NULL)!=E_POINTER) return 22;
 statics->lpVtbl->Release(statics); activation->lpVtbl->Release(activation); WindowsDeleteString(class_key);
 return 0;
}
