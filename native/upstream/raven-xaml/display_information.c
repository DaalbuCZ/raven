/* SPDX-License-Identifier: MIT */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include "corewindow_interop.h"
#include "events.h"
#include <windows.graphics.display.h>
#include <winstring.h>
#include <stddef.h>

typedef __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation display_i;
typedef __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation2 display2_i;
typedef __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation3 display3_i;
typedef __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation4 display4_i;
typedef __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformationStatics statics_i;
typedef __x_ABI_CWindows_CGraphics_CDisplay_CDisplayOrientations orientation_t;
typedef __x_ABI_CWindows_CGraphics_CDisplay_CResolutionScale scale_t;
static const WCHAR display_name[] = L"Windows.Graphics.Display.DisplayInformation";
struct display { display_i first; display2_i second; display3_i third; display4_i fourth; LONG refs; __x_ABI_CWindows_CUI_CCore_CICoreWindow *window; struct raven_event_list events[4]; };
#define OWNER2(iface) ((struct display *)((char *)(iface)-offsetof(struct display, second)))
#define OWNER3(iface) ((struct display *)((char *)(iface)-offsetof(struct display, third)))
#define OWNER4(iface) ((struct display *)((char *)(iface)-offsetof(struct display, fourth)))
static HRESULT display_query(struct display *self, REFIID iid, void **out)
{
 if (!out) return E_POINTER;
 *out = NULL;
 if (IsEqualIID(iid,&IID_IUnknown) || IsEqualIID(iid,&IID_IInspectable) || IsEqualIID(iid,&IID___x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation)) *out=&self->first;
 else if (IsEqualIID(iid,&IID___x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation2)) *out=&self->second;
 else if (IsEqualIID(iid,&IID___x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation3)) *out=&self->third;
 else if (IsEqualIID(iid,&IID___x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation4)) *out=&self->fourth;
 else { char line[120]; wsprintfA(line,"raven-display: no interface %08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n", (unsigned long)iid->Data1,iid->Data2,iid->Data3,iid->Data4[0],iid->Data4[1],iid->Data4[2],iid->Data4[3],iid->Data4[4],iid->Data4[5],iid->Data4[6],iid->Data4[7]); OutputDebugStringA(line); return E_NOINTERFACE; }
 InterlockedIncrement(&self->refs); return S_OK;
}
static HRESULT WINAPI display_qi(display_i *iface, REFIID iid, void **out) { return display_query((struct display *)iface,iid,out); }
static ULONG WINAPI display_addref(display_i *iface) { return InterlockedIncrement(&((struct display *)iface)->refs); }
static ULONG WINAPI display_release(display_i *iface)
{
 struct display *self=(struct display *)iface; LONG refs=InterlockedDecrement(&self->refs);
 if (!refs) { for (int i=0;i<4;++i) { raven_event_close(&self->events[i]); DeleteCriticalSection(&self->events[i].lock); } self->window->lpVtbl->Release(self->window); HeapFree(GetProcessHeap(),0,self); InterlockedDecrement(&raven_xaml_objects); }
 return refs;
}
static HRESULT WINAPI display2_qi(display2_i *iface,REFIID iid,void **out) { return display_query(OWNER2(iface),iid,out); }
static ULONG WINAPI display2_addref(display2_i *iface) { return display_addref(&OWNER2(iface)->first); }
static ULONG WINAPI display2_release(display2_i *iface) { return display_release(&OWNER2(iface)->first); }
static HRESULT WINAPI display3_qi(display3_i *iface,REFIID iid,void **out) { return display_query(OWNER3(iface),iid,out); }
static ULONG WINAPI display3_addref(display3_i *iface) { return display_addref(&OWNER3(iface)->first); }
static ULONG WINAPI display3_release(display3_i *iface) { return display_release(&OWNER3(iface)->first); }
static HRESULT WINAPI display4_qi(display4_i *iface,REFIID iid,void **out) { return display_query(OWNER4(iface),iid,out); }
static ULONG WINAPI display4_addref(display4_i *iface) { return display_addref(&OWNER4(iface)->first); }
static ULONG WINAPI display4_release(display4_i *iface) { return display_release(&OWNER4(iface)->first); }
static HRESULT WINAPI iids(void *iface,ULONG *count,IID **out) { (void)iface; if (!count || !out) return E_POINTER; *count=0; *out=NULL; return S_OK; }
static HRESULT WINAPI name(void *iface,HSTRING *out) { (void)iface; return out ? WindowsCreateString(display_name,ARRAYSIZE(display_name)-1,out) : E_POINTER; }
static HRESULT WINAPI trust(void *iface,TrustLevel *out) { (void)iface; if (!out) return E_POINTER; *out=BaseTrust; return S_OK; }
static UINT display_dpi(struct display *self)
{
 raven_core_window_interop *interop=NULL; HWND hwnd=NULL; UINT dpi=0;
 if (SUCCEEDED(self->window->lpVtbl->QueryInterface(self->window,&raven_core_window_interop_iid,(void **)&interop)) && interop) {
  if (SUCCEEDED(interop->lpVtbl->get_WindowHandle(interop,&hwnd)) && hwnd) dpi=GetDpiForWindow(hwnd);
  interop->lpVtbl->Release(interop);
 }
 if (!dpi) dpi=GetDpiForSystem();
 return dpi ? dpi : 96;
}
static HRESULT WINAPI get_orientation(display_i *iface,orientation_t *out)
{ (void)iface; if (!out) return E_POINTER; *out=GetSystemMetrics(SM_CXSCREEN)>=GetSystemMetrics(SM_CYSCREEN) ? 1 : 2; return S_OK; }
static HRESULT WINAPI get_scale(display_i *iface,scale_t *out)
{ if (!out) return E_POINTER; *out=(scale_t)((display_dpi((struct display *)iface)*100+48)/96); return S_OK; }
/* shortcut: raw DPI currently follows logical DPI until physical monitor data is available. */
static HRESULT WINAPI get_dpi(display_i *iface,FLOAT *out)
{ if (!out) return E_POINTER; *out=(FLOAT)display_dpi((struct display *)iface); return S_OK; }
static HRESULT WINAPI get_ratio(display2_i *iface,DOUBLE *out)
{ if (!out) return E_POINTER; *out=(DOUBLE)display_dpi(OWNER2(iface))/96.0; return S_OK; }
/* shortcut: physical diagonal and monitor-specific dimensions need host EDID and per-monitor metrics. */
static HRESULT WINAPI get_diagonal(display3_i *iface,__FIReference_1_DOUBLE **out)
{ (void)iface; if (!out) return E_POINTER; *out=NULL; return S_OK; }
static HRESULT WINAPI get_width(display4_i *iface,UINT32 *out)
{ (void)iface; if (!out) return E_POINTER; *out=(UINT32)GetSystemMetrics(SM_CXSCREEN); return S_OK; }
static HRESULT WINAPI get_height(display4_i *iface,UINT32 *out)
{ (void)iface; if (!out) return E_POINTER; *out=(UINT32)GetSystemMetrics(SM_CYSCREEN); return S_OK; }
static HRESULT WINAPI get_stereo(display_i *iface,boolean *out) { (void)iface; if (!out) return E_POINTER; *out=FALSE; return S_OK; }
/* shortcut: display-change registrations are retained but notifications require native monitor change tracking. */
#define EVENT(n,i) static HRESULT WINAPI add_##n(display_i *iface,void *handler,EventRegistrationToken *token) { return raven_event_add(&((struct display *)iface)->events[i],(IUnknown *)handler,token); } \
 static HRESULT WINAPI remove_##n(display_i *iface,EventRegistrationToken token) { return raven_event_remove(&((struct display *)iface)->events[i],token); }
EVENT(orientation,0) EVENT(dpi,1) EVENT(stereo,2) EVENT(color,3)
static HRESULT WINAPI color_profile(display_i *iface,void **out) { (void)iface; if (!out) return E_POINTER; *out=NULL; return E_NOTIMPL; }
static const __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformationVtbl display_table = {
 display_qi,display_addref,display_release,(void *)iids,(void *)name,(void *)trust,
 get_orientation,get_orientation,(void *)add_orientation,remove_orientation,get_scale,get_dpi,get_dpi,get_dpi,
 (void *)add_dpi,remove_dpi,get_stereo,(void *)add_stereo,remove_stereo,(void *)color_profile,(void *)add_color,remove_color
};
static const __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation2Vtbl display2_table = {display2_qi,display2_addref,display2_release,(void *)iids,(void *)name,(void *)trust,get_ratio};
static const __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation3Vtbl display3_table = {display3_qi,display3_addref,display3_release,(void *)iids,(void *)name,(void *)trust,get_diagonal};
static const __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation4Vtbl display4_table = {display4_qi,display4_addref,display4_release,(void *)iids,(void *)name,(void *)trust,get_width,get_height};
static IActivationFactory activation;
static statics_i statics;
static HRESULT WINAPI factory_qi(void *iface,REFIID iid,void **out)
{ (void)iface; if (!out) return E_POINTER; *out=NULL; if (IsEqualIID(iid,&IID_IUnknown)||IsEqualIID(iid,&IID_IInspectable)||IsEqualIID(iid,&IID_IActivationFactory)||IsEqualIID(iid,&IID_IAgileObject)) *out=&activation; else if (IsEqualIID(iid,&IID___x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformationStatics)) *out=&statics; else return E_NOINTERFACE; return S_OK; }
static ULONG WINAPI factory_addref(void *iface) { (void)iface; return 2; }
static ULONG WINAPI factory_release(void *iface) { (void)iface; return 1; }
static HRESULT WINAPI get_current(statics_i *iface,display_i **out)
{
 (void)iface; if (!out) return E_POINTER; *out=NULL;
 __x_ABI_CWindows_CUI_CCore_CICoreWindow *window=NULL; HRESULT hr=raven_current_window(&window);
 if (FAILED(hr)) return hr;
 struct display *self=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof *self);
 if (!self) { window->lpVtbl->Release(window); return E_OUTOFMEMORY; }
 self->first.lpVtbl=(void *)&display_table; self->second.lpVtbl=(void *)&display2_table; self->third.lpVtbl=(void *)&display3_table; self->fourth.lpVtbl=(void *)&display4_table; self->window=window; self->refs=1;
 for (int i=0;i<4;++i) InitializeCriticalSection(&self->events[i].lock);
 InterlockedIncrement(&raven_xaml_objects); *out=&self->first; return S_OK;
}
static orientation_t preferences;
static HRESULT WINAPI get_preferences(statics_i *iface,orientation_t *out) { (void)iface; if (!out) return E_POINTER; *out=preferences; return S_OK; }
static HRESULT WINAPI put_preferences(statics_i *iface,orientation_t value) { (void)iface; preferences=value; return S_OK; }
static HRESULT WINAPI statics_event(statics_i *iface,void *handler,EventRegistrationToken *token) { (void)iface;(void)handler;(void)token; return E_NOTIMPL; }
static HRESULT WINAPI statics_remove(statics_i *iface,EventRegistrationToken token) { (void)iface;(void)token; return E_NOTIMPL; }
static HRESULT WINAPI activate(IActivationFactory *iface,IInspectable **out) { (void)iface; if (!out) return E_POINTER; *out=NULL; return E_NOTIMPL; }
static const __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformationStaticsVtbl statics_table={(void *)factory_qi,(void *)factory_addref,(void *)factory_release,(void *)iids,(void *)name,(void *)trust,get_current,get_preferences,put_preferences,(void *)statics_event,statics_remove};
static const IActivationFactoryVtbl activation_table={(void *)factory_qi,(void *)factory_addref,(void *)factory_release,(void *)iids,(void *)name,(void *)trust,activate};
static IActivationFactory activation={(IActivationFactoryVtbl *)&activation_table};
static statics_i statics={(void *)&statics_table};
HRESULT raven_display_information_factory(HSTRING key,IActivationFactory **out)
{
 UINT32 length=0; const WCHAR *text=WindowsGetStringRawBuffer(key,&length);
 if (!out) return E_POINTER;
 *out=NULL;
 if (length!=ARRAYSIZE(display_name)-1 || CompareStringOrdinal(text,length,display_name,-1,FALSE)!=CSTR_EQUAL) return CLASS_E_CLASSNOTAVAILABLE;
 *out=&activation; return S_OK;
}
