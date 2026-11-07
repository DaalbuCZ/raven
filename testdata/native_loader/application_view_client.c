#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <roapi.h>
#include <winstring.h>
#include <weakreference.h>
#include <windows.ui.viewmanagement.h>
typedef __x_ABI_CWindows_CUI_CViewManagement_CIApplicationView view_i;
typedef __x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewStatics2 statics_i;
typedef __x_ABI_CWindows_CUI_CViewManagement_CIApplicationView3 view3_i;
static LONG handler_refs=1;
static HRESULT WINAPI handler_query(IUnknown *self,REFIID iid,void **out) { (void)iid; if (!out) return E_POINTER; *out=self; InterlockedIncrement(&handler_refs); return S_OK; }
static ULONG WINAPI handler_addref(IUnknown *self) { (void)self; return InterlockedIncrement(&handler_refs); }
static ULONG WINAPI handler_release(IUnknown *self) { (void)self; return InterlockedDecrement(&handler_refs); }
static const IUnknownVtbl handler_table={handler_query,handler_addref,handler_release};
static IUnknown handler={&handler_table};
struct focus_table {
	void *inspectable[6];
	HRESULT (WINAPI *add)(void *,IUnknown *,EventRegistrationToken *);
	HRESULT (WINAPI *remove)(void *,EventRegistrationToken);
};
struct focus_statics { const struct focus_table *vtbl; };
int raven_entry(void)
{
	static const WCHAR name[] = L"Windows.UI.ViewManagement.ApplicationView", title[] = L"View fixture";
	static const WCHAR colors_name[] = L"Windows.UI.Colors";
	HSTRING key=NULL, text=NULL, received=NULL;
	statics_i *statics=NULL; view_i *view=NULL, *again=NULL; IUnknown *identity=NULL;
	__x_ABI_CWindows_CUI_CIColorsStatics *colors=NULL;
	__x_ABI_CWindows_CUI_CColor color_value;
	static const IID focus_iid={0x3546a1b6,0x20bf,0x5007,{0x92,0x9d,0xe6,0xd3,0x2e,0x16,0xaf,0xe4}};
	static const WCHAR focus_name[]=L"Windows.UI.Xaml.Input.FocusManager";
	struct focus_statics *focus=NULL; EventRegistrationToken token;
	if (WindowsCreateString(focus_name,ARRAYSIZE(focus_name)-1,&key)!=S_OK ||
		RoGetActivationFactory(key,&focus_iid,(void **)&focus)!=S_OK || !focus) return 14;
	WindowsDeleteString(key); key=NULL;
	if (focus->vtbl->add(focus,&handler,&token)!=S_OK || !token.value || handler_refs!=2 ||
		focus->vtbl->remove(focus,token)!=S_OK || handler_refs!=1) return 15;
	((IUnknown *)focus)->lpVtbl->Release((IUnknown *)focus);
	if (WindowsCreateString(colors_name, ARRAYSIZE(colors_name)-1, &key)!=S_OK ||
		RoGetActivationFactory(key,&IID___x_ABI_CWindows_CUI_CIColorsStatics,(void **)&colors)!=S_OK || !colors) return 12;
	WindowsDeleteString(key); key=NULL;
	if (colors->lpVtbl->get_Red(colors,&color_value)!=S_OK || color_value.A!=255 || color_value.R!=255 || color_value.G || color_value.B ||
		colors->lpVtbl->get_Transparent(colors,&color_value)!=S_OK || color_value.A || color_value.R!=255 || color_value.G!=255 || color_value.B!=255 ||
		colors->lpVtbl->get_YellowGreen(colors,&color_value)!=S_OK || color_value.A!=255 || color_value.R!=154 || color_value.G!=205 || color_value.B!=50) return 13;
	colors->lpVtbl->Release(colors);
	if (WindowsCreateString(name, ARRAYSIZE(name)-1, &key)!=S_OK ||
		RoGetActivationFactory(key,&IID___x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewStatics2,(void **)&statics)!=S_OK || !statics) return 1;
	WindowsDeleteString(key);
	__x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewStatics3 *launch=NULL;
	if (statics->lpVtbl->QueryInterface(statics,&IID___x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewStatics3,(void **)&launch)!=S_OK || !launch ||
		launch->lpVtbl->put_PreferredLaunchWindowingMode(launch,ApplicationViewWindowingMode_Auto)!=S_OK) return 11;
	launch->lpVtbl->Release(launch);
	if (statics->lpVtbl->GetForCurrentView(statics,&view)!=S_OK || !view) return 2;
	if (statics->lpVtbl->GetForCurrentView(statics,&again)!=S_OK || again!=view) return 5;
	again->lpVtbl->Release(again);
	if (view->lpVtbl->QueryInterface(view,&IID_IWeakReferenceSource,(void **)&identity)!=S_OK || !identity) return 3;
	identity->lpVtbl->Release(identity);
	if (view->lpVtbl->QueryInterface(view,&IID___x_ABI_CWindows_CUI_CViewManagement_CIApplicationView3,(void **)&identity)!=S_OK || !identity) return 6;
	__x_ABI_CWindows_CFoundation_CSize minimum = {400, 300};
	if (((view3_i *)identity)->lpVtbl->SetPreferredMinSize((view3_i *)identity, minimum)!=S_OK) return 7;
	__x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewTitleBar *bar=NULL;
	__FIReference_1_Color *color=(void *)1;
	if (((view3_i *)identity)->lpVtbl->get_TitleBar((view3_i *)identity,&bar)!=S_OK || !bar ||
		bar->lpVtbl->put_BackgroundColor(bar,NULL)!=S_OK || bar->lpVtbl->get_BackgroundColor(bar,&color)!=S_OK || color) return 9;
	ULONG count=0; IID *interfaces=NULL;
	if (bar->lpVtbl->GetIids(bar,&count,&interfaces)!=S_OK || count!=1 || !interfaces ||
		!IsEqualIID(&interfaces[0],&IID___x_ABI_CWindows_CUI_CViewManagement_CIApplicationViewTitleBar)) return 10;
	HeapFree(GetProcessHeap(),0,interfaces);
	bar->lpVtbl->Release(bar);
	identity->lpVtbl->Release(identity);
	if (WindowsCreateString(title,ARRAYSIZE(title)-1,&text)!=S_OK || view->lpVtbl->put_Title(view,text)!=S_OK ||
		view->lpVtbl->get_Title(view,&received)!=S_OK || lstrcmpW(WindowsGetStringRawBuffer(received,NULL),title)) return 4;
	HWND window=FindWindowW(L"Raven.Xaml.CoreWindow", title);
	RECT bounds;
	if (!window || !SetWindowPos(window,NULL,0,0,100,100,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE) ||
		!GetClientRect(window,&bounds) || bounds.right < 400 || bounds.bottom < 300) return 8;
	WindowsDeleteString(text); WindowsDeleteString(received);
	view->lpVtbl->Release(view); statics->lpVtbl->Release(statics);
	return 0;
}
