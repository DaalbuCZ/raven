/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* A view belongs to its CoreWindow; unsupported view-management operations fail. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include "corewindow_interop.h"
#include <windows.ui.viewmanagement.h>
#include <winstring.h>
#include <stddef.h>

typedef __x_ABI_CWindows_CUI_CViewManagement_CIApplicationView view_i;
typedef __x_ABI_CWindows_CUI_CViewManagement_CIApplicationView3 view3_i;
typedef __x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewTitleBar bar_i;
typedef __FIReference_1_Color color_i;
typedef __x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewStatics2 statics_i;
typedef __x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewStatics3 statics3_i;
typedef __x_ABI_CWindows_CUI_CCore_CICoreWindow core_i;
static const WCHAR class_name[] = L"Windows.UI.ViewManagement.ApplicationView";
struct view3_part { const __x_ABI_CWindows_CUI_CViewManagement_CIApplicationView3Vtbl *lpVtbl; struct view *owner; };
struct bar_part { const __x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewTitleBarVtbl *lpVtbl; struct view *owner; };
struct view { view_i iface; LONG refs; struct raven_weak_source weak, bar_weak; core_i *core; HSTRING title; struct view3_part third; struct bar_part bar; color_i *colors[12]; };
static LONG view_tls = -1;
static const __x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewVtbl view_table;
static const __x_ABI_CWindows_CUI_CViewManagement_CIApplicationView3Vtbl view3_table;
static const __x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewTitleBarVtbl bar_table;
static HRESULT WINAPI view_query(struct view *self, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid,&IID_IUnknown) || IsEqualIID(iid,&IID_IInspectable) || IsEqualIID(iid,&IID___x_ABI_CWindows_CUI_CViewManagement_CIApplicationView)) *out=&self->iface;
	else if (IsEqualIID(iid,&IID___x_ABI_CWindows_CUI_CViewManagement_CIApplicationView3)) *out=&self->third;
	else if (IsEqualIID(iid,&IID_IWeakReferenceSource)) *out=&self->weak;
	else {
		char line[100];
		wsprintfA(line,"raven-xaml: ApplicationView has no interface %08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
			(unsigned long)iid->Data1,iid->Data2,iid->Data3,iid->Data4[0],iid->Data4[1],iid->Data4[2],iid->Data4[3],iid->Data4[4],iid->Data4[5],iid->Data4[6],iid->Data4[7]);
		OutputDebugStringA(line);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs); return S_OK;
}
static HRESULT WINAPI view_qi(view_i *iface,REFIID iid,void **out) { return view_query((struct view *)iface,iid,out); }
static ULONG WINAPI view_addref(view_i *iface) { return InterlockedIncrement(&((struct view *)iface)->refs); }
static ULONG WINAPI view_release(view_i *iface) { struct view *self=(struct view *)iface; ULONG refs=InterlockedDecrement(&self->refs); if (!refs) { raven_weak_source_disconnect(&self->weak); raven_weak_source_disconnect(&self->bar_weak); for (unsigned i=0;i<ARRAYSIZE(self->colors);++i) if (self->colors[i]) self->colors[i]->lpVtbl->Release(self->colors[i]); WindowsDeleteString(self->title); self->core->lpVtbl->Release(self->core); HeapFree(GetProcessHeap(),0,self); } return refs; }
static HRESULT WINAPI no_iids(void *iface,ULONG *count,IID **out) { (void)iface; if (!count || !out) return E_POINTER; *count=0; *out=NULL; return S_OK; }
static HRESULT list_iids(const IID *first, const IID *second, ULONG *count, IID **out)
{
	if (!count || !out) return E_POINTER;
	*count = 0; *out = NULL;
	IID *list = HeapAlloc(GetProcessHeap(),0,(second ? 2 : 1) * sizeof *list);
	if (!list) return E_OUTOFMEMORY;
	list[0] = *first;
	if (second) list[1] = *second;
	*count = second ? 2 : 1; *out = list;
	return S_OK;
}
static HRESULT WINAPI view_iids(void *iface,ULONG *count,IID **out) { (void)iface; return list_iids(&IID___x_ABI_CWindows_CUI_CViewManagement_CIApplicationView,&IID___x_ABI_CWindows_CUI_CViewManagement_CIApplicationView3,count,out); }
static HRESULT WINAPI bar_iids(void *iface,ULONG *count,IID **out) { (void)iface; return list_iids(&IID___x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewTitleBar,NULL,count,out); }
static HRESULT WINAPI class_string(void *iface,HSTRING *out) { (void)iface; if (!out) return E_POINTER; return WindowsCreateString(class_name,ARRAYSIZE(class_name)-1,out); }
static HRESULT WINAPI trust(void *iface,TrustLevel *out) { (void)iface; if (!out) return E_POINTER; *out=BaseTrust; return S_OK; }
static HRESULT view_hwnd(struct view *self,HWND *out) { raven_core_window_interop *interop=NULL; HRESULT hr=self->core->lpVtbl->QueryInterface(self->core,&raven_core_window_interop_iid,(void **)&interop); if (SUCCEEDED(hr)) { hr=interop->lpVtbl->get_WindowHandle(interop,out); interop->lpVtbl->Release(interop); } return hr; }
static HRESULT WINAPI orientation(view_i *iface,__x_ABI_CWindows_CUI_CViewManagement_CApplicationViewOrientation *out) { RECT rect; HWND hwnd=NULL; if (!out) return E_POINTER; HRESULT hr=view_hwnd((struct view *)iface,&hwnd); if (FAILED(hr)) return hr; if (!GetClientRect(hwnd,&rect)) return HRESULT_FROM_WIN32(GetLastError()); *out=(rect.right-rect.left >= rect.bottom-rect.top) ? 0 : 1; return S_OK; }
static HRESULT WINAPI no_flag(view_i *iface,boolean *out) { (void)iface; if (!out) return E_POINTER; *out=FALSE; return S_OK; }
static HRESULT WINAPI capture(view_i *iface,boolean *out) { (void)iface; if (!out) return E_POINTER; *out=TRUE; return S_OK; }
static HRESULT WINAPI set_capture(view_i *iface,boolean value) { (void)iface; (void)value; return E_NOTIMPL; }
static HRESULT WINAPI set_title(view_i *iface,HSTRING value) { struct view *self=(struct view *)iface; HSTRING copy=NULL; HWND hwnd=NULL; HRESULT hr=WindowsDuplicateString(value,&copy); if (FAILED(hr)) return hr; hr=view_hwnd(self,&hwnd); if (SUCCEEDED(hr) && !SetWindowTextW(hwnd,WindowsGetStringRawBuffer(value,NULL))) hr=HRESULT_FROM_WIN32(GetLastError()); if (SUCCEEDED(hr)) { WindowsDeleteString(self->title); self->title=copy; } else WindowsDeleteString(copy); return hr; }
static HRESULT WINAPI get_title(view_i *iface,HSTRING *out) { if (!out) return E_POINTER; return WindowsDuplicateString(((struct view *)iface)->title,out); }
static HRESULT WINAPI get_id(view_i *iface,INT32 *out) { (void)iface; if (!out) return E_POINTER; *out=(INT32)GetCurrentThreadId(); return S_OK; }
static HRESULT WINAPI add_consolidated(view_i *iface,void *handler,EventRegistrationToken *token) { (void)iface; (void)handler; (void)token; return E_NOTIMPL; }
static HRESULT WINAPI remove_consolidated(view_i *iface,EventRegistrationToken token) { (void)iface; (void)token; return E_NOTIMPL; }
static const __x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewVtbl view_table = {view_qi,view_addref,view_release,(void *)view_iids,(void *)class_string,(void *)trust,orientation,no_flag,no_flag,no_flag,no_flag,capture,set_capture,set_title,get_title,get_id,(void *)add_consolidated,remove_consolidated};
static struct view *third_owner(view3_i *iface) { return ((struct view3_part *)iface)->owner; }
static HRESULT WINAPI third_qi(view3_i *iface,REFIID iid,void **out) { return view_query(third_owner(iface),iid,out); }
static ULONG WINAPI third_addref(view3_i *iface) { return view_addref(&third_owner(iface)->iface); }
static ULONG WINAPI third_release(view3_i *iface) { return view_release(&third_owner(iface)->iface); }
static HRESULT WINAPI third_titlebar(view3_i *iface,bar_i **out) { if (!out) return E_POINTER; struct view *self=third_owner(iface); view_addref(&self->iface); *out=(bar_i *)&self->bar; return S_OK; }
static HRESULT WINAPI third_overlay(view3_i *iface,__x_ABI_CWindows_CUI_CViewManagement_CFullScreenSystemOverlayMode *out) { (void)iface; if (!out) return E_POINTER; *out=0; return S_OK; }
static HRESULT WINAPI third_set_overlay(view3_i *iface,__x_ABI_CWindows_CUI_CViewManagement_CFullScreenSystemOverlayMode mode) { (void)iface; (void)mode; raven_trace_not_implemented(__func__); return E_NOTIMPL; }
static HRESULT WINAPI third_fullscreen(view3_i *iface,boolean *out) { (void)iface; if (!out) return E_POINTER; *out=FALSE; return S_OK; }
static HRESULT WINAPI third_enter_fullscreen(view3_i *iface,boolean *out) { (void)iface; if (!out) return E_POINTER; *out=FALSE; return S_OK; }
static HRESULT WINAPI third_exit_fullscreen(view3_i *iface) { (void)iface; return S_OK; }
static HRESULT WINAPI third_overlays(view3_i *iface) { (void)iface; raven_trace_not_implemented(__func__); return E_NOTIMPL; }
static HRESULT WINAPI third_resize(view3_i *iface,__x_ABI_CWindows_CFoundation_CSize size,boolean *out) { (void)iface; (void)size; if (!out) return E_POINTER; *out=FALSE; return S_OK; }
static HRESULT WINAPI third_min_size(view3_i *iface,__x_ABI_CWindows_CFoundation_CSize size) { return raven_window_minimum_size(third_owner(iface)->core,size); }
static const __x_ABI_CWindows_CUI_CViewManagement_CIApplicationView3Vtbl view3_table = {third_qi,third_addref,third_release,(void *)view_iids,(void *)class_string,(void *)trust,third_titlebar,third_overlay,third_set_overlay,third_fullscreen,third_enter_fullscreen,third_exit_fullscreen,third_overlays,third_resize,third_min_size};
static struct view *bar_owner(bar_i *iface) { return ((struct bar_part *)iface)->owner; }
static HRESULT WINAPI bar_query_owner(struct view *self, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out=NULL;
	if (IsEqualIID(iid,&IID_IUnknown) || IsEqualIID(iid,&IID_IInspectable) || IsEqualIID(iid,&IID___x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewTitleBar)) *out=&self->bar;
	else if (IsEqualIID(iid,&IID_IWeakReferenceSource)) *out=&self->bar_weak;
	else {
		char line[104];
		wsprintfA(line,"raven-xaml: ApplicationViewTitleBar has no interface %08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
			(unsigned long)iid->Data1,iid->Data2,iid->Data3,iid->Data4[0],iid->Data4[1],iid->Data4[2],iid->Data4[3],iid->Data4[4],iid->Data4[5],iid->Data4[6],iid->Data4[7]);
		OutputDebugStringA(line);
		return E_NOINTERFACE;
	}
	view_addref(&self->iface); return S_OK;
}
static HRESULT WINAPI bar_qi(bar_i *iface,REFIID iid,void **out) { return bar_query_owner(bar_owner(iface),iid,out); }
static ULONG WINAPI bar_addref(bar_i *iface) { return view_addref(&bar_owner(iface)->iface); }
static ULONG WINAPI bar_release(bar_i *iface) { return view_release(&bar_owner(iface)->iface); }
static HRESULT WINAPI bar_class(bar_i *iface,HSTRING *out) { static const WCHAR name[]=L"Windows.UI.ViewManagement.ApplicationViewTitleBar"; (void)iface; if (!out) return E_POINTER; return WindowsCreateString(name,ARRAYSIZE(name)-1,out); }
/* shortcut: requested colors are retained but native window decorations do not yet consume them. */
static HRESULT set_color(bar_i *iface,unsigned index,color_i *color) { struct view *self=bar_owner(iface); if (color) color->lpVtbl->AddRef(color); if (self->colors[index]) self->colors[index]->lpVtbl->Release(self->colors[index]); self->colors[index]=color; return S_OK; }
static HRESULT get_color(bar_i *iface,unsigned index,color_i **out) { if (!out) return E_POINTER; *out=bar_owner(iface)->colors[index]; if (*out) (*out)->lpVtbl->AddRef(*out); return S_OK; }
#define COLOR_ACCESSORS(n) static HRESULT WINAPI put_color##n(bar_i *iface,color_i *color) { return set_color(iface,n,color); } static HRESULT WINAPI get_color##n(bar_i *iface,color_i **out) { return get_color(iface,n,out); }
COLOR_ACCESSORS(0) COLOR_ACCESSORS(1) COLOR_ACCESSORS(2) COLOR_ACCESSORS(3) COLOR_ACCESSORS(4) COLOR_ACCESSORS(5)
COLOR_ACCESSORS(6) COLOR_ACCESSORS(7) COLOR_ACCESSORS(8) COLOR_ACCESSORS(9) COLOR_ACCESSORS(10) COLOR_ACCESSORS(11)
static const __x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewTitleBarVtbl bar_table = {bar_qi,bar_addref,bar_release,(void *)bar_iids,bar_class,(void *)trust,put_color0,get_color0,put_color1,get_color1,put_color2,get_color2,put_color3,get_color3,put_color4,get_color4,put_color5,get_color5,put_color6,get_color6,put_color7,get_color7,put_color8,get_color8,put_color9,get_color9,put_color10,get_color10,put_color11,get_color11};
static IActivationFactory activation;
static statics_i statics;
static statics3_i statics3;
static HRESULT WINAPI factory_qi(statics_i *iface,REFIID iid,void **out) { (void)iface; if (!out) return E_POINTER; *out=NULL; if (IsEqualIID(iid,&IID_IUnknown) || IsEqualIID(iid,&IID_IInspectable) || IsEqualIID(iid,&IID_IActivationFactory) || IsEqualIID(iid,&IID_IAgileObject)) *out=&activation; else if (IsEqualIID(iid,&IID___x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewStatics2)) *out=&statics; else if (IsEqualIID(iid,&IID___x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewStatics3)) *out=&statics3; else return E_NOINTERFACE; return S_OK; }
static ULONG WINAPI factory_addref(statics_i *iface) { (void)iface; return 2; }
static ULONG WINAPI factory_release(statics_i *iface) { (void)iface; return 1; }
static HRESULT WINAPI get_current(statics_i *iface,view_i **out)
{
	(void)iface;
	if (!out) return E_POINTER;
	*out=NULL;
	if (view_tls == -1) {
		DWORD slot = TlsAlloc();
		if (slot == TLS_OUT_OF_INDEXES) return E_OUTOFMEMORY;
		if (InterlockedCompareExchange(&view_tls,(LONG)slot,-1) != -1) TlsFree(slot);
	}
	struct view *self = TlsGetValue((DWORD)view_tls);
	if (!self) {
		self=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof *self);
		if (!self) return E_OUTOFMEMORY;
		self->iface.lpVtbl=&view_table; self->refs=1;
		self->third.lpVtbl=&view3_table; self->third.owner=self;
		self->bar.lpVtbl=&bar_table; self->bar.owner=self;
		HRESULT hr=raven_current_window(&self->core);
		if (FAILED(hr)) { HeapFree(GetProcessHeap(),0,self); return hr; }
		raven_weak_source_init(&self->weak,self,&self->refs,(raven_weak_query_fn)view_query);
		raven_weak_source_init(&self->bar_weak,self,&self->refs,(raven_weak_query_fn)bar_query_owner);
		if (!TlsSetValue((DWORD)view_tls,self)) { view_release(&self->iface); return E_OUTOFMEMORY; }
	}
	view_addref(&self->iface);
	*out=&self->iface;
	return S_OK;
}
static HRESULT WINAPI get_terminate(statics_i *iface,boolean *out) { (void)iface; if (!out) return E_POINTER; *out=TRUE; return S_OK; }
static HRESULT WINAPI set_terminate(statics_i *iface,boolean value) { (void)iface; return value ? S_OK : E_NOTIMPL; }
static const __x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewStatics2Vtbl statics_table = {factory_qi,factory_addref,factory_release,(void *)no_iids,(void *)class_string,(void *)trust,get_current,get_terminate,set_terminate};
static statics_i statics = {&statics_table};
/* shortcut: only the system-default launch mode is applied; other launch preferences require window creation support. */
static HRESULT WINAPI get_launch_mode(statics3_i *iface,__x_ABI_CWindows_CUI_CViewManagement_CApplicationViewWindowingMode *out) { (void)iface; if (!out) return E_POINTER; *out=ApplicationViewWindowingMode_Auto; return S_OK; }
static HRESULT WINAPI set_launch_mode(statics3_i *iface,__x_ABI_CWindows_CUI_CViewManagement_CApplicationViewWindowingMode mode) { (void)iface; return mode==ApplicationViewWindowingMode_Auto ? S_OK : E_NOTIMPL; }
static HRESULT WINAPI get_launch_size(statics3_i *iface,__x_ABI_CWindows_CFoundation_CSize *out) { (void)iface; if (!out) return E_POINTER; out->Width=0; out->Height=0; return S_OK; }
static HRESULT WINAPI set_launch_size(statics3_i *iface,__x_ABI_CWindows_CFoundation_CSize size) { (void)iface; (void)size; return E_NOTIMPL; }
static const __x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewStatics3Vtbl statics3_table = {(void *)factory_qi,(void *)factory_addref,(void *)factory_release,(void *)no_iids,(void *)class_string,(void *)trust,get_launch_mode,set_launch_mode,get_launch_size,set_launch_size};
static statics3_i statics3 = {&statics3_table};
static HRESULT WINAPI cannot_activate(IActivationFactory *self,IInspectable **out) { (void)self; if (!out) return E_POINTER; *out=NULL; return E_NOTIMPL; }
static const IActivationFactoryVtbl activation_table = {(void *)factory_qi,(void *)factory_addref,(void *)factory_release,(void *)no_iids,(void *)class_string,(void *)trust,cannot_activate};
static IActivationFactory activation = {&activation_table};
/* shortcut: only current-view and default-launch statics are served; add later interfaces as callers need them. */
HRESULT raven_application_view_factory(HSTRING key,IActivationFactory **out) { UINT32 length=0; const WCHAR *text=WindowsGetStringRawBuffer(key,&length); if (!out) return E_POINTER; *out=NULL; if (length!=ARRAYSIZE(class_name)-1 || CompareStringOrdinal(text,length,class_name,-1,FALSE)!=CSTR_EQUAL) return CLASS_E_CLASSNOTAVAILABLE; *out=&activation; return S_OK; }
