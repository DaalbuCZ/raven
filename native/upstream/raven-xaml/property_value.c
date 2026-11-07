/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Windows.Foundation.PropertyValue: the boxed scalar subset used by local settings.
 * shortcut: numeric coercion and unlisted types are unsupported, extend when callers need them. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <windows.foundation.h>
#include <winstring.h>
#include "windows_ui_xaml_dictionary.h"
#include <stddef.h>

typedef __x_ABI_CWindows_CFoundation_CIPropertyValue value_i;
typedef __x_ABI_CWindows_CFoundation_CIPropertyValueStatics statics_i;
typedef __x_ABI_CWindows_CFoundation_CPropertyType property_type;
#define ALLOC(n) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (n))
#define FREE(p) HeapFree(GetProcessHeap(), 0, (p))
struct reference_part { const void *lpVtbl; };
struct boxed_value { value_i value; struct reference_part reference; LONG refs; property_type type;
 union { INT32 i32; UINT32 u32; INT64 i64; UINT64 u64; FLOAT f32; DOUBLE f64; boolean b; HSTRING s; } data;
 struct raven_weak_source weak; };
static const __x_ABI_CWindows_CFoundation_CIPropertyValueVtbl value_table;
static const WCHAR class_name[] = L"Windows.Foundation.PropertyValue";
static const IID *reference_iid(property_type type) { switch ((int)type) {
case 4: return &IID___FIReference_1_INT32;
case 5: return &IID___FIReference_1_UINT32;
case 6: return &IID___FIReference_1_INT64;
case 7: return &IID___FIReference_1_UINT64;
case 8: return &IID___FIReference_1_FLOAT;
case 9: return &IID___FIReference_1_DOUBLE;
case 11: return &IID___FIReference_1_boolean;
case 12: return &IID___FIReference_1_HSTRING;
default: return NULL; } }
static HRESULT WINAPI boxed_query(struct boxed_value *self, REFIID iid, void **out) {
 if (!out) return E_POINTER;
 *out = NULL;
 if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID___x_ABI_CWindows_CFoundation_CIPropertyValue) || IsEqualIID(iid, &IID_IAgileObject)) *out = &self->value;
 else if (IsEqualIID(iid, &IID_IWeakReferenceSource)) *out = &self->weak;
 else if (reference_iid(self->type) && IsEqualIID(iid, reference_iid(self->type))) *out = &self->reference;
 else return E_NOINTERFACE;
 InterlockedIncrement(&self->refs); return S_OK;
}
static ULONG boxed_release(struct boxed_value *self) { LONG refs = InterlockedDecrement(&self->refs); if (!refs) {
 raven_weak_source_disconnect(&self->weak); if ((int)self->type == 12) WindowsDeleteString(self->data.s); FREE(self); InterlockedDecrement(&raven_xaml_objects); } return (ULONG)refs; }
static HRESULT WINAPI value_QueryInterface(value_i *This, REFIID iid, void **out) { return boxed_query((struct boxed_value *)This, iid, out); }
static ULONG WINAPI value_AddRef(value_i *This) { return InterlockedIncrement(&((struct boxed_value *)This)->refs); }
static ULONG WINAPI value_Release(value_i *This) { return boxed_release((struct boxed_value *)This); }
static HRESULT WINAPI value_GetIids(value_i *This, ULONG *count, IID **iids) {
 struct boxed_value *self=(struct boxed_value *)This; if (!count || !iids) return E_POINTER; *count=0; *iids=NULL;
 IID *list=ALLOC(2*sizeof(IID)); if (!list) return E_OUTOFMEMORY;
 list[0]=IID___x_ABI_CWindows_CFoundation_CIPropertyValue; list[1]=*reference_iid(self->type); *count=2; *iids=list; return S_OK; }
static HRESULT WINAPI value_GetRuntimeClassName(value_i *This,HSTRING *out) { (void)This; if (!out) return E_POINTER; return WindowsCreateString(class_name, ARRAYSIZE(class_name)-1, out); }
static HRESULT WINAPI value_GetTrustLevel(value_i *This,TrustLevel *out) { (void)This; if (!out) return E_POINTER; *out=BaseTrust; return S_OK; }
static HRESULT WINAPI value_get_Type(value_i *This,property_type *out) { if(!out)return E_POINTER; *out=((struct boxed_value*)This)->type; return S_OK; }
static HRESULT WINAPI value_get_IsNumericScalar(value_i *This,boolean *out) { int type=(int)((struct boxed_value*)This)->type; if(!out)return E_POINTER; *out=(type>=4 && type<=9); return S_OK; }
static HRESULT WINAPI value_GetInt32(value_i *This,INT32 *value) { struct boxed_value *self=(struct boxed_value *)This; if (!value) return E_POINTER; if ((int)self->type != 4) return TYPE_E_TYPEMISMATCH; *value=self->data.i32; return S_OK; }
static HRESULT WINAPI value_GetUInt32(value_i *This,UINT32 *value) { struct boxed_value *self=(struct boxed_value *)This; if (!value) return E_POINTER; if ((int)self->type != 5) return TYPE_E_TYPEMISMATCH; *value=self->data.u32; return S_OK; }
static HRESULT WINAPI value_GetInt64(value_i *This,INT64 *value) { struct boxed_value *self=(struct boxed_value *)This; if (!value) return E_POINTER; if ((int)self->type != 6) return TYPE_E_TYPEMISMATCH; *value=self->data.i64; return S_OK; }
static HRESULT WINAPI value_GetUInt64(value_i *This,UINT64 *value) { struct boxed_value *self=(struct boxed_value *)This; if (!value) return E_POINTER; if ((int)self->type != 7) return TYPE_E_TYPEMISMATCH; *value=self->data.u64; return S_OK; }
static HRESULT WINAPI value_GetSingle(value_i *This,FLOAT *value) { struct boxed_value *self=(struct boxed_value *)This; if (!value) return E_POINTER; if ((int)self->type != 8) return TYPE_E_TYPEMISMATCH; *value=self->data.f32; return S_OK; }
static HRESULT WINAPI value_GetDouble(value_i *This,DOUBLE *value) { struct boxed_value *self=(struct boxed_value *)This; if (!value) return E_POINTER; if ((int)self->type != 9) return TYPE_E_TYPEMISMATCH; *value=self->data.f64; return S_OK; }
static HRESULT WINAPI value_GetBoolean(value_i *This,boolean *value) { struct boxed_value *self=(struct boxed_value *)This; if (!value) return E_POINTER; if ((int)self->type != 11) return TYPE_E_TYPEMISMATCH; *value=self->data.b; return S_OK; }
static HRESULT WINAPI value_GetString(value_i *This,HSTRING *value) { struct boxed_value *self=(struct boxed_value *)This; if (!value) return E_POINTER; if ((int)self->type != 12) return TYPE_E_TYPEMISMATCH; return WindowsDuplicateString(self->data.s, value); }
static HRESULT WINAPI value_GetUInt8(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, BYTE *value) { (void)This; if (!value) return E_POINTER; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetInt16(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, INT16 *value) { (void)This; if (!value) return E_POINTER; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetUInt16(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT16 *value) { (void)This; if (!value) return E_POINTER; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetChar16(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, WCHAR *value) { (void)This; if (!value) return E_POINTER; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetGuid(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, GUID *value) { (void)This; if (!value) return E_POINTER; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetDateTime(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, __x_ABI_CWindows_CFoundation_CDateTime *value) { (void)This; if (!value) return E_POINTER; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetTimeSpan(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, __x_ABI_CWindows_CFoundation_CTimeSpan *value) { (void)This; if (!value) return E_POINTER; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetPoint(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, __x_ABI_CWindows_CFoundation_CPoint *value) { (void)This; if (!value) return E_POINTER; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetSize(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, __x_ABI_CWindows_CFoundation_CSize *value) { (void)This; if (!value) return E_POINTER; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetRect(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, __x_ABI_CWindows_CFoundation_CRect *value) { (void)This; if (!value) return E_POINTER; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetUInt8Array(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, BYTE **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetInt16Array(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, INT16 **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetUInt16Array(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, UINT16 **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetInt32Array(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, INT32 **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetUInt32Array(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, UINT32 **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetInt64Array(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, INT64 **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetUInt64Array(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, UINT64 **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetSingleArray(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, FLOAT **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetDoubleArray(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, DOUBLE **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetChar16Array(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, WCHAR **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetBooleanArray(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, boolean **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetStringArray(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, HSTRING **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetInspectableArray(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, IInspectable ***value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetGuidArray(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, GUID **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetDateTimeArray(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, __x_ABI_CWindows_CFoundation_CDateTime **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetTimeSpanArray(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, __x_ABI_CWindows_CFoundation_CTimeSpan **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetPointArray(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, __x_ABI_CWindows_CFoundation_CPoint **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetSizeArray(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, __x_ABI_CWindows_CFoundation_CSize **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI value_GetRectArray(__x_ABI_CWindows_CFoundation_CIPropertyValue *This, UINT32 *value_size, __x_ABI_CWindows_CFoundation_CRect **value) { (void)This; if (!value_size || !value) return E_POINTER; *value_size=0; *value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
struct reference_vtbl { HRESULT (WINAPI *QueryInterface)(struct reference_part *,REFIID,void **); ULONG (WINAPI *AddRef)(struct reference_part *); ULONG (WINAPI *Release)(struct reference_part *); HRESULT (WINAPI *GetIids)(struct reference_part *,ULONG *,IID **); HRESULT (WINAPI *GetRuntimeClassName)(struct reference_part *,HSTRING *); HRESULT (WINAPI *GetTrustLevel)(struct reference_part *,TrustLevel *); HRESULT (WINAPI *get_Value)(struct reference_part *,void *); };
#define OWNER(p) ((struct boxed_value *)((char *)(p)-offsetof(struct boxed_value,reference)))
static HRESULT WINAPI ref_query(struct reference_part *p,REFIID iid,void **out) {return boxed_query(OWNER(p),iid,out);}
static ULONG WINAPI ref_addref(struct reference_part *p) {return value_AddRef(&OWNER(p)->value);}
static ULONG WINAPI ref_release(struct reference_part *p) {return boxed_release(OWNER(p));}
static HRESULT WINAPI ref_iids(struct reference_part *p,ULONG *n,IID **i) {return value_GetIids(&OWNER(p)->value,n,i);}
static HRESULT WINAPI ref_name(struct reference_part *p,HSTRING *n) {return value_GetRuntimeClassName(&OWNER(p)->value,n);}
static HRESULT WINAPI ref_trust(struct reference_part *p,TrustLevel *t) {return value_GetTrustLevel(&OWNER(p)->value,t);}
static HRESULT WINAPI ref_get(struct reference_part *p,void *out) { struct boxed_value *v=OWNER(p); if(!out)return E_POINTER; switch((int)v->type) {
case 4: return value_GetInt32(&v->value,(INT32 *)out);
case 5: return value_GetUInt32(&v->value,(UINT32 *)out);
case 6: return value_GetInt64(&v->value,(INT64 *)out);
case 7: return value_GetUInt64(&v->value,(UINT64 *)out);
case 8: return value_GetSingle(&v->value,(FLOAT *)out);
case 9: return value_GetDouble(&v->value,(DOUBLE *)out);
case 11: return value_GetBoolean(&v->value,(boolean *)out);
case 12: return value_GetString(&v->value,(HSTRING *)out);
default:{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }} }
static const struct reference_vtbl reference_table={ref_query,ref_addref,ref_release,ref_iids,ref_name,ref_trust,ref_get};
static HRESULT boxed_create(property_type type,const void *data,size_t size,IInspectable **out) {
 if(!out)return E_POINTER;
 *out=NULL;
 struct boxed_value *v=ALLOC(sizeof *v); if(!v)return E_OUTOFMEMORY;
 v->value.lpVtbl=(void *)&value_table; v->reference.lpVtbl=(void *)&reference_table; v->refs=1; v->type=type;
 if((int)type==12) { HRESULT hr=WindowsDuplicateString(*(HSTRING const *)data,&v->data.s); if(FAILED(hr)){FREE(v);return hr;} }
 else memcpy(&v->data,data,size);
 raven_weak_source_init(&v->weak,v,&v->refs,(raven_weak_query_fn)boxed_query); InterlockedIncrement(&raven_xaml_objects); *out=(IInspectable *)&v->value; return S_OK;
}
HRESULT raven_double_value_create(DOUBLE value, IInspectable **out) {
 return boxed_create((property_type)9,&value,sizeof value,out);
}
static HRESULT WINAPI statics_CreateInt32(statics_i *This,INT32 value,IInspectable **property_value) { (void)This; return boxed_create((property_type)4,&value,sizeof value,property_value); }
static HRESULT WINAPI statics_CreateUInt32(statics_i *This,UINT32 value,IInspectable **property_value) { (void)This; return boxed_create((property_type)5,&value,sizeof value,property_value); }
static HRESULT WINAPI statics_CreateInt64(statics_i *This,INT64 value,IInspectable **property_value) { (void)This; return boxed_create((property_type)6,&value,sizeof value,property_value); }
static HRESULT WINAPI statics_CreateUInt64(statics_i *This,UINT64 value,IInspectable **property_value) { (void)This; return boxed_create((property_type)7,&value,sizeof value,property_value); }
static HRESULT WINAPI statics_CreateSingle(statics_i *This,FLOAT value,IInspectable **property_value) { (void)This; return boxed_create((property_type)8,&value,sizeof value,property_value); }
static HRESULT WINAPI statics_CreateDouble(statics_i *This,DOUBLE value,IInspectable **property_value) { (void)This; return raven_double_value_create(value,property_value); }
static HRESULT WINAPI statics_CreateBoolean(statics_i *This,boolean value,IInspectable **property_value) { (void)This; return boxed_create((property_type)11,&value,sizeof value,property_value); }
static HRESULT WINAPI statics_CreateString(statics_i *This,HSTRING value,IInspectable **property_value) { (void)This; return boxed_create((property_type)12,&value,sizeof value,property_value); }
static HRESULT WINAPI statics_CreateEmpty(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, IInspectable **property_value) { (void)This; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateUInt8(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, BYTE value, IInspectable **property_value) { (void)This; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateInt16(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, INT16 value, IInspectable **property_value) { (void)This; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateUInt16(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT16 value, IInspectable **property_value) { (void)This; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateChar16(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, WCHAR value, IInspectable **property_value) { (void)This; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateInspectable(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, IInspectable *value, IInspectable **property_value) { (void)This; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateGuid(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, GUID value, IInspectable **property_value) { (void)This; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateDateTime(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, __x_ABI_CWindows_CFoundation_CDateTime value, IInspectable **property_value) { (void)This; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateTimeSpan(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, __x_ABI_CWindows_CFoundation_CTimeSpan value, IInspectable **property_value) { (void)This; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreatePoint(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, __x_ABI_CWindows_CFoundation_CPoint value, IInspectable **property_value) { (void)This; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateSize(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, __x_ABI_CWindows_CFoundation_CSize value, IInspectable **property_value) { (void)This; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateRect(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, __x_ABI_CWindows_CFoundation_CRect value, IInspectable **property_value) { (void)This; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateUInt8Array(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, BYTE *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateInt16Array(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, INT16 *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateUInt16Array(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, UINT16 *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateInt32Array(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, INT32 *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateUInt32Array(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, UINT32 *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateInt64Array(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, INT64 *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateUInt64Array(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, UINT64 *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateSingleArray(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, FLOAT *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateDoubleArray(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, DOUBLE *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateChar16Array(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, WCHAR *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateBooleanArray(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, boolean *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateStringArray(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, HSTRING *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateInspectableArray(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, IInspectable **value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateGuidArray(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, GUID *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateDateTimeArray(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, __x_ABI_CWindows_CFoundation_CDateTime *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateTimeSpanArray(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, __x_ABI_CWindows_CFoundation_CTimeSpan *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreatePointArray(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, __x_ABI_CWindows_CFoundation_CPoint *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateSizeArray(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, __x_ABI_CWindows_CFoundation_CSize *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI statics_CreateRectArray(__x_ABI_CWindows_CFoundation_CIPropertyValueStatics *This, UINT32 value_size, __x_ABI_CWindows_CFoundation_CRect *value, IInspectable **property_value) { (void)This; (void)value_size; (void)value; if (!property_value) return E_POINTER; *property_value=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static const __x_ABI_CWindows_CFoundation_CIPropertyValueVtbl value_table={
 .QueryInterface=value_QueryInterface,.AddRef=value_AddRef,.Release=value_Release,.GetIids=value_GetIids,.GetRuntimeClassName=value_GetRuntimeClassName,.GetTrustLevel=value_GetTrustLevel,
.get_Type=value_get_Type,
.get_IsNumericScalar=value_get_IsNumericScalar,
.GetUInt8=value_GetUInt8,
.GetInt16=value_GetInt16,
.GetUInt16=value_GetUInt16,
.GetInt32=value_GetInt32,
.GetUInt32=value_GetUInt32,
.GetInt64=value_GetInt64,
.GetUInt64=value_GetUInt64,
.GetSingle=value_GetSingle,
.GetDouble=value_GetDouble,
.GetChar16=value_GetChar16,
.GetBoolean=value_GetBoolean,
.GetString=value_GetString,
.GetGuid=value_GetGuid,
.GetDateTime=value_GetDateTime,
.GetTimeSpan=value_GetTimeSpan,
.GetPoint=value_GetPoint,
.GetSize=value_GetSize,
.GetRect=value_GetRect,
.GetUInt8Array=value_GetUInt8Array,
.GetInt16Array=value_GetInt16Array,
.GetUInt16Array=value_GetUInt16Array,
.GetInt32Array=value_GetInt32Array,
.GetUInt32Array=value_GetUInt32Array,
.GetInt64Array=value_GetInt64Array,
.GetUInt64Array=value_GetUInt64Array,
.GetSingleArray=value_GetSingleArray,
.GetDoubleArray=value_GetDoubleArray,
.GetChar16Array=value_GetChar16Array,
.GetBooleanArray=value_GetBooleanArray,
.GetStringArray=value_GetStringArray,
.GetInspectableArray=value_GetInspectableArray,
.GetGuidArray=value_GetGuidArray,
.GetDateTimeArray=value_GetDateTimeArray,
.GetTimeSpanArray=value_GetTimeSpanArray,
.GetPointArray=value_GetPointArray,
.GetSizeArray=value_GetSizeArray,
.GetRectArray=value_GetRectArray,
};
struct property_factory { IActivationFactory activation; statics_i statics; LONG refs; };
static struct property_factory factory;
static HRESULT factory_query(REFIID iid,void **out) {if(!out)return E_POINTER; *out=NULL; if(IsEqualIID(iid,&IID_IUnknown)||IsEqualIID(iid,&IID_IInspectable)||IsEqualIID(iid,&IID_IActivationFactory)) *out=&factory.activation; else if(IsEqualIID(iid,&IID___x_ABI_CWindows_CFoundation_CIPropertyValueStatics)) *out=&factory.statics; else return E_NOINTERFACE; InterlockedIncrement(&factory.refs); return S_OK;}
static HRESULT WINAPI activation_query(IActivationFactory *p,REFIID i,void **o){(void)p;return factory_query(i,o);}
static ULONG WINAPI activation_addref(IActivationFactory *p){(void)p;return InterlockedIncrement(&factory.refs);}
static ULONG WINAPI activation_release(IActivationFactory *p){(void)p;return InterlockedDecrement(&factory.refs);}
static HRESULT WINAPI activation_iids(IActivationFactory *p,ULONG *n,IID **i){(void)p;if(!n||!i)return E_POINTER;*n=0;*i=NULL;return S_OK;}
static HRESULT WINAPI activation_name(IActivationFactory *p,HSTRING *n){(void)p;if(!n)return E_POINTER;return WindowsCreateString(class_name,ARRAYSIZE(class_name)-1,n);}
static HRESULT WINAPI activation_trust(IActivationFactory *p,TrustLevel *t){(void)p;if(!t)return E_POINTER;*t=BaseTrust;return S_OK;}
static HRESULT WINAPI activation_activate(IActivationFactory *p,IInspectable **out){(void)p;if(!out)return E_POINTER;*out=NULL;{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }}
static HRESULT WINAPI statics_query(statics_i *p,REFIID i,void **o){(void)p;return factory_query(i,o);}
static ULONG WINAPI statics_addref(statics_i *p){(void)p;return InterlockedIncrement(&factory.refs);}
static ULONG WINAPI statics_release(statics_i *p){(void)p;return InterlockedDecrement(&factory.refs);}
static HRESULT WINAPI statics_iids(statics_i *p,ULONG *n,IID **i){(void)p;if(!n||!i)return E_POINTER;*n=0;*i=NULL;return S_OK;}
static HRESULT WINAPI statics_name(statics_i *p,HSTRING *n){(void)p;if(!n)return E_POINTER;return WindowsCreateString(class_name,ARRAYSIZE(class_name)-1,n);}
static HRESULT WINAPI statics_trust(statics_i *p,TrustLevel *t){(void)p;if(!t)return E_POINTER;*t=BaseTrust;return S_OK;}
static const IActivationFactoryVtbl activation_table={activation_query,activation_addref,activation_release,activation_iids,activation_name,activation_trust,activation_activate};
static const __x_ABI_CWindows_CFoundation_CIPropertyValueStaticsVtbl statics_table={
 .QueryInterface=statics_query,.AddRef=statics_addref,.Release=statics_release,.GetIids=statics_iids,.GetRuntimeClassName=statics_name,.GetTrustLevel=statics_trust,
.CreateEmpty=statics_CreateEmpty,
.CreateUInt8=statics_CreateUInt8,
.CreateInt16=statics_CreateInt16,
.CreateUInt16=statics_CreateUInt16,
.CreateInt32=statics_CreateInt32,
.CreateUInt32=statics_CreateUInt32,
.CreateInt64=statics_CreateInt64,
.CreateUInt64=statics_CreateUInt64,
.CreateSingle=statics_CreateSingle,
.CreateDouble=statics_CreateDouble,
.CreateChar16=statics_CreateChar16,
.CreateBoolean=statics_CreateBoolean,
.CreateString=statics_CreateString,
.CreateInspectable=statics_CreateInspectable,
.CreateGuid=statics_CreateGuid,
.CreateDateTime=statics_CreateDateTime,
.CreateTimeSpan=statics_CreateTimeSpan,
.CreatePoint=statics_CreatePoint,
.CreateSize=statics_CreateSize,
.CreateRect=statics_CreateRect,
.CreateUInt8Array=statics_CreateUInt8Array,
.CreateInt16Array=statics_CreateInt16Array,
.CreateUInt16Array=statics_CreateUInt16Array,
.CreateInt32Array=statics_CreateInt32Array,
.CreateUInt32Array=statics_CreateUInt32Array,
.CreateInt64Array=statics_CreateInt64Array,
.CreateUInt64Array=statics_CreateUInt64Array,
.CreateSingleArray=statics_CreateSingleArray,
.CreateDoubleArray=statics_CreateDoubleArray,
.CreateChar16Array=statics_CreateChar16Array,
.CreateBooleanArray=statics_CreateBooleanArray,
.CreateStringArray=statics_CreateStringArray,
.CreateInspectableArray=statics_CreateInspectableArray,
.CreateGuidArray=statics_CreateGuidArray,
.CreateDateTimeArray=statics_CreateDateTimeArray,
.CreateTimeSpanArray=statics_CreateTimeSpanArray,
.CreatePointArray=statics_CreatePointArray,
.CreateSizeArray=statics_CreateSizeArray,
.CreateRectArray=statics_CreateRectArray,
};
static struct property_factory factory={{(void *)&activation_table},{(void *)&statics_table},1};
HRESULT raven_property_value_statics(statics_i **out) { if(!out)return E_POINTER; *out=&factory.statics; InterlockedIncrement(&factory.refs); return S_OK; }
__declspec(dllexport) HRESULT raven_property_value_factory(HSTRING name,IActivationFactory **out) { UINT32 length=0; const WCHAR *text; if(!out)return E_POINTER; *out=NULL; text=WindowsGetStringRawBuffer(name,&length); if(length!=ARRAYSIZE(class_name)-1 || CompareStringOrdinal(text,length,class_name,-1,FALSE)!=CSTR_EQUAL)return CLASS_E_CLASSNOTAVAILABLE; *out=&factory.activation; InterlockedIncrement(&factory.refs); return S_OK; }

/* Windows.UI.Color as a value object. The struct is not activatable — a compiled theme dictionary reaches its colours
 * through the loader's kind-8 constants — so the engine boxes the four bytes into an IInspectable whose runtime class is
 * "Windows.UI.Color" and whose readers are the engine's own interop interface, the convention the DependencyProperty
 * interop uses where the real ABI has no getter to call and no winmd in tree carries the projection IID. */
static const WCHAR color_value_class_name[] = L"Windows.UI.Color";
static const IID raven_color_value_interop_iid = {0x8f4d2c63, 0x51a7, 0x4be0, {0x92, 0xb4, 0x6e, 0x21, 0x0c, 0x55, 0x7d, 0xa1}};
typedef struct raven_color_value_interop raven_color_value_interop;
struct raven_color_value_interop_vtbl {
	HRESULT (WINAPI *QueryInterface)(raven_color_value_interop *, REFIID, void **);
	ULONG (WINAPI *AddRef)(raven_color_value_interop *);
	ULONG (WINAPI *Release)(raven_color_value_interop *);
	HRESULT (WINAPI *get_Color)(raven_color_value_interop *, __x_ABI_CWindows_CUI_CColor *);
};
struct raven_color_value_interop { const struct raven_color_value_interop_vtbl *lpVtbl; };
struct color_value { IInspectable inspectable; raven_color_value_interop interop; LONG refs; __x_ABI_CWindows_CUI_CColor color; };
#define COLOR_OWNER(part) ((struct color_value *)((char *)(part)-offsetof(struct color_value, interop)))

static HRESULT WINAPI color_value_query(IInspectable *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID_IAgileObject))
		*out = self;
	else if (IsEqualIID(iid, &raven_color_value_interop_iid))
		*out = &((struct color_value *)self)->interop;
	else
		return E_NOINTERFACE;
	IInspectable_AddRef(self);
	return S_OK;
}
static ULONG WINAPI color_value_addref(IInspectable *self) { return InterlockedIncrement(&((struct color_value *)self)->refs); }
static ULONG WINAPI color_value_release(IInspectable *self)
{
	ULONG refs = InterlockedDecrement(&((struct color_value *)self)->refs);
	if (!refs)
		HeapFree(GetProcessHeap(), 0, self);
	return refs;
}
static HRESULT WINAPI color_value_iids(IInspectable *self, ULONG *count, IID **iids) { (void)self; if (!count || !iids) return E_POINTER; *count = 0; *iids = NULL; return S_OK; }
static HRESULT WINAPI color_value_name(IInspectable *self, HSTRING *out)
{
	(void)self;
	if (!out)
		return E_POINTER;
	return WindowsCreateString(color_value_class_name, (UINT32)(ARRAYSIZE(color_value_class_name) - 1), out);
}
static HRESULT WINAPI color_value_trust(IInspectable *self, TrustLevel *out) { (void)self; if (!out) return E_POINTER; *out = BaseTrust; return S_OK; }
static const IInspectableVtbl color_value_table = {
	color_value_query, color_value_addref, color_value_release, color_value_iids, color_value_name, color_value_trust,
};
static HRESULT WINAPI color_interop_query(raven_color_value_interop *self, REFIID iid, void **out) { return color_value_query(&COLOR_OWNER(self)->inspectable, iid, out); }
static ULONG WINAPI color_interop_addref(raven_color_value_interop *self) { return color_value_addref(&COLOR_OWNER(self)->inspectable); }
static ULONG WINAPI color_interop_release(raven_color_value_interop *self) { return color_value_release(&COLOR_OWNER(self)->inspectable); }
static HRESULT WINAPI color_interop_get(raven_color_value_interop *self, __x_ABI_CWindows_CUI_CColor *out)
{
	if (!out)
		return E_POINTER;
	*out = COLOR_OWNER(self)->color;
	return S_OK;
}
static const struct raven_color_value_interop_vtbl color_interop_table = {
	color_interop_query, color_interop_addref, color_interop_release, color_interop_get,
};

HRESULT raven_color_value_create(const __x_ABI_CWindows_CUI_CColor *color, IInspectable **out)
{
	struct color_value *value;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!color)
		return E_POINTER;
	value = ALLOC(sizeof *value);
	if (!value)
		return E_OUTOFMEMORY;
	value->inspectable.lpVtbl = &color_value_table;
	value->interop.lpVtbl = &color_interop_table;
	value->refs = 1;
	value->color = *color;
	*out = &value->inspectable;
	return S_OK;
}

HRESULT raven_color_value_of(IInspectable *object, __x_ABI_CWindows_CUI_CColor *color)
{
	raven_color_value_interop *interop = NULL;
	HRESULT result;
	if (!object || !color)
		return E_POINTER;
	if (FAILED(object->lpVtbl->QueryInterface(object, &raven_color_value_interop_iid, (void **)&interop)))
		return S_FALSE;
	result = interop->lpVtbl->get_Color(interop, color);
	interop->lpVtbl->Release(interop);
	return result;
}

__declspec(dllexport) HRESULT raven_xaml_color_value(IInspectable *object, __x_ABI_CWindows_CUI_CColor *color)
{
	return raven_color_value_of(object, color);
}

/* A XAML enum as a value object. An enum is a struct-like projected type, not activatable, so a compiled dictionary that
 * stores one (Colors.xbf's "InverseTheme" = ElementTheme.Dark) arrives as a CreateTypeWithConstant node whose type the
 * catalog cannot create. The engine boxes the constant into an IInspectable whose runtime class name is the enum's full
 * name and whose reader is the engine's own interop interface — the convention the colour value above established. The
 * integer is what the property setters already take, so an enum reaching a property applies like one. */
static const IID raven_enum_value_interop_iid = {0x2b6a4a7e, 0x9d31, 0x4c1b, {0x85, 0x3f, 0x0a, 0x7e, 0x2d, 0x61, 0x9c, 0x44}};
typedef struct raven_enum_value_interop raven_enum_value_interop;
struct raven_enum_value_interop_vtbl {
	HRESULT (WINAPI *QueryInterface)(raven_enum_value_interop *, REFIID, void **);
	ULONG (WINAPI *AddRef)(raven_enum_value_interop *);
	ULONG (WINAPI *Release)(raven_enum_value_interop *);
	HRESULT (WINAPI *get_Value)(raven_enum_value_interop *, INT32 *);
};
struct raven_enum_value_interop { const struct raven_enum_value_interop_vtbl *lpVtbl; };
struct enum_value {
	IInspectable inspectable;
	raven_enum_value_interop interop;
	LONG refs;
	WCHAR *type_name;
	UINT32 type_length;
	INT32 value;
};
#define ENUM_OWNER(part) ((struct enum_value *)((char *)(part)-offsetof(struct enum_value, interop)))

static HRESULT WINAPI enum_value_query(IInspectable *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID_IAgileObject))
		*out = self;
	else if (IsEqualIID(iid, &raven_enum_value_interop_iid))
		*out = &((struct enum_value *)self)->interop;
	else
		return E_NOINTERFACE;
	IInspectable_AddRef(self);
	return S_OK;
}
static ULONG WINAPI enum_value_addref(IInspectable *self) { return InterlockedIncrement(&((struct enum_value *)self)->refs); }
static ULONG WINAPI enum_value_release(IInspectable *self)
{
	struct enum_value *value = (struct enum_value *)self;
	ULONG refs = InterlockedDecrement(&value->refs);
	if (!refs) {
		if (value->type_name)
			HeapFree(GetProcessHeap(), 0, value->type_name);
		HeapFree(GetProcessHeap(), 0, value);
	}
	return refs;
}
static HRESULT WINAPI enum_value_iids(IInspectable *self, ULONG *count, IID **iids) { (void)self; if (!count || !iids) return E_POINTER; *count = 0; *iids = NULL; return S_OK; }
static HRESULT WINAPI enum_value_name(IInspectable *self, HSTRING *out)
{
	struct enum_value *value = (struct enum_value *)self;
	if (!out)
		return E_POINTER;
	return WindowsCreateString(value->type_name, value->type_length, out);
}
static HRESULT WINAPI enum_value_trust(IInspectable *self, TrustLevel *out) { (void)self; if (!out) return E_POINTER; *out = BaseTrust; return S_OK; }
static const IInspectableVtbl enum_value_table = {
	enum_value_query, enum_value_addref, enum_value_release, enum_value_iids, enum_value_name, enum_value_trust,
};
static HRESULT WINAPI enum_interop_query(raven_enum_value_interop *self, REFIID iid, void **out) { return enum_value_query(&ENUM_OWNER(self)->inspectable, iid, out); }
static ULONG WINAPI enum_interop_addref(raven_enum_value_interop *self) { return enum_value_addref(&ENUM_OWNER(self)->inspectable); }
static ULONG WINAPI enum_interop_release(raven_enum_value_interop *self) { return enum_value_release(&ENUM_OWNER(self)->inspectable); }
static HRESULT WINAPI enum_interop_get(raven_enum_value_interop *self, INT32 *out)
{
	if (!out)
		return E_POINTER;
	*out = ENUM_OWNER(self)->value;
	return S_OK;
}
static const struct raven_enum_value_interop_vtbl enum_interop_table = {
	enum_interop_query, enum_interop_addref, enum_interop_release, enum_interop_get,
};

HRESULT raven_enum_value_create(const WCHAR *type_name, UINT32 type_length, INT32 value, IInspectable **out)
{
	struct enum_value *boxed;
	if (!out || !type_name || !type_length)
		return E_POINTER;
	*out = NULL;
	boxed = ALLOC(sizeof *boxed);
	if (!boxed)
		return E_OUTOFMEMORY;
	boxed->type_name = HeapAlloc(GetProcessHeap(), 0, (SIZE_T)type_length * sizeof(WCHAR));
	if (!boxed->type_name) {
		HeapFree(GetProcessHeap(), 0, boxed);
		return E_OUTOFMEMORY;
	}
	memcpy(boxed->type_name, type_name, (SIZE_T)type_length * sizeof(WCHAR));
	boxed->type_length = type_length;
	boxed->value = value;
	boxed->refs = 1;
	boxed->inspectable.lpVtbl = &enum_value_table;
	boxed->interop.lpVtbl = &enum_interop_table;
	*out = &boxed->inspectable;
	return S_OK;
}

BOOL raven_enum_value_of(IInspectable *object, INT32 *value)
{
	raven_enum_value_interop *interop = NULL;
	HRESULT result;
	if (!object || !value)
		return FALSE;
	if (FAILED(object->lpVtbl->QueryInterface(object, &raven_enum_value_interop_iid, (void **)&interop)) || !interop)
		return FALSE;
	result = interop->lpVtbl->get_Value(interop, value);
	interop->lpVtbl->Release(interop);
	return SUCCEEDED(result);
}

__declspec(dllexport) HRESULT raven_xaml_enum_value(IInspectable *object, INT32 *value)
{
	if (!object || !value) return E_POINTER;
	return raven_enum_value_of(object, value) ? S_OK : E_NOINTERFACE;
}

/* shortcut: colors use compiled palette defaults until live system theme settings are served. */
HRESULT raven_xaml_system_color(const WCHAR *key, UINT32 length, const WCHAR *theme, UINT32 theme_length, IInspectable **out)
{
	static const struct { const WCHAR *name; BYTE alpha; BOOL base; } colors[] = {
		{L"SystemAltLowColor", 0x33, FALSE},
		{L"SystemAltHighColor", 0xff, FALSE},
		{L"SystemBaseLowColor", 0x33, TRUE},
	};
	static const struct { const WCHAR *name; BYTE slot; } accent_names[] = {
		{L"SystemAccentColorLight3", 0}, {L"SystemAccentColorLight2", 1},
		{L"SystemAccentColorLight1", 2}, {L"SystemAccentColor", 3},
		{L"SystemAccentColorDark1", 4}, {L"SystemAccentColorDark2", 5}, {L"SystemAccentColorDark3", 6},
	};
	/* shortcut: fixed blue fallback when the user has no valid AccentPalette; replace with the shell's palette generator. */
	static const BYTE default_accent[7][3] = {
		{0x99, 0xeb, 0xff}, {0x4c, 0xc2, 0xff}, {0x1a, 0x98, 0xed}, {0x00, 0x78, 0xd4},
		{0x00, 0x5f, 0xb8}, {0x00, 0x3e, 0x92}, {0x00, 0x21, 0x5e},
	};
	BOOL dark;
	if (!key || !theme || !out) return E_POINTER;
	*out = NULL;
	for (size_t i = 0; i < ARRAYSIZE(accent_names); ++i) {
		if (CompareStringOrdinal(key, (int)length, accent_names[i].name, -1, FALSE) == CSTR_EQUAL) {
			/* AccentPalette is eight four-byte RGB slots, lightest first; the last byte is reserved, not opacity. */
			typedef LSTATUS (WINAPI *reg_get_value_fn)(HKEY, LPCWSTR, LPCWSTR, DWORD, LPDWORD, PVOID, LPDWORD);
			BYTE palette[32];
			DWORD bytes = sizeof palette;
			const BYTE *rgb = default_accent[accent_names[i].slot];
			HMODULE registry = LoadLibraryW(L"advapi32.dll");
			reg_get_value_fn get_value = registry ? (reg_get_value_fn)(void *)GetProcAddress(registry, "RegGetValueW") : NULL;
			if (get_value && get_value(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent",
				L"AccentPalette", RRF_RT_REG_BINARY, NULL, palette, &bytes) == ERROR_SUCCESS && bytes == sizeof palette)
				rgb = palette + accent_names[i].slot * 4;
			__x_ABI_CWindows_CUI_CColor color = {0xff, rgb[0], rgb[1], rgb[2]};
			if (registry) FreeLibrary(registry);
			return raven_color_value_create(&color, out);
		}
	}
	if (theme_length == 4 && CompareStringOrdinal(theme, 4, L"Dark", -1, FALSE) == CSTR_EQUAL) dark = TRUE;
	else if (theme_length == 12 && CompareStringOrdinal(theme, 12, L"HighContrast", -1, FALSE) == CSTR_EQUAL) dark = TRUE;
	else if (theme_length == 5 && CompareStringOrdinal(theme, 5, L"Light", -1, FALSE) == CSTR_EQUAL) dark = FALSE;
	else return S_FALSE;
	for (size_t i = 0; i < ARRAYSIZE(colors); ++i) {
		if (CompareStringOrdinal(key, (int)length, colors[i].name, -1, FALSE) == CSTR_EQUAL) {
			BYTE rgb = (dark == colors[i].base) ? 0xff : 0;
			__x_ABI_CWindows_CUI_CColor color = {colors[i].alpha, rgb, rgb, rgb};
			return raven_color_value_create(&color, out);
		}
	}
	return S_FALSE;
}
