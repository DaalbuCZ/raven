/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Windows.UI.Colors: the standard named ARGB values. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <winstring.h>
#include <windows.ui.h>
typedef __x_ABI_CWindows_CUI_CIColorsStatics colors_i;
typedef __x_ABI_CWindows_CUI_CColor color;
static const WCHAR class_name[] = L"Windows.UI.Colors";
static IActivationFactory activation;
static colors_i statics;
static HRESULT WINAPI query(IActivationFactory *self, REFIID iid, void **out)
{
	(void)self;
	if (!out) return E_POINTER;
	*out=NULL;
	if (IsEqualIID(iid,&IID_IUnknown) || IsEqualIID(iid,&IID_IInspectable) || IsEqualIID(iid,&IID_IActivationFactory) || IsEqualIID(iid,&IID_IAgileObject)) *out=&activation;
	else if (IsEqualIID(iid,&IID___x_ABI_CWindows_CUI_CIColorsStatics)) *out=&statics;
	else return E_NOINTERFACE;
	return S_OK;
}
static ULONG WINAPI addref(IActivationFactory *self) { (void)self; return 2; }
static ULONG WINAPI release(IActivationFactory *self) { (void)self; return 1; }
static HRESULT WINAPI iids(IActivationFactory *self, ULONG *count, IID **out) { (void)self; if (!count || !out) return E_POINTER; *count=0; *out=NULL; return S_OK; }
static HRESULT WINAPI name(IActivationFactory *self,HSTRING *out) { (void)self; if (!out) return E_POINTER; return WindowsCreateString(class_name,ARRAYSIZE(class_name)-1,out); }
static HRESULT WINAPI trust(IActivationFactory *self,TrustLevel *out) { (void)self; if (!out) return E_POINTER; *out=BaseTrust; return S_OK; }
static HRESULT WINAPI activate(IActivationFactory *self,IInspectable **out) { (void)self; if (!out) return E_POINTER; *out=NULL; return E_NOTIMPL; }
#define COLOR(n,a,r,g,b) static HRESULT WINAPI color_##n(colors_i *self,color *out) { (void)self; if (!out) return E_POINTER; out->A=a; out->R=r; out->G=g; out->B=b; return S_OK; }
#include "colors.inc"
#undef COLOR
static const __x_ABI_CWindows_CUI_CIColorsStaticsVtbl statics_table = {
	(void *)query,(void *)addref,(void *)release,(void *)iids,(void *)name,(void *)trust,
#define COLOR(n,a,r,g,b) color_##n,
#include "colors.inc"
#undef COLOR
};
static colors_i statics = {&statics_table};
static const IActivationFactoryVtbl activation_table = {query,addref,release,iids,name,trust,activate};
static IActivationFactory activation = {&activation_table};
HRESULT raven_colors_factory(HSTRING key,IActivationFactory **out)
{
	UINT32 length=0; const WCHAR *text=WindowsGetStringRawBuffer(key,&length);
	if (!out) return E_POINTER;
	*out=NULL;
	if (length != ARRAYSIZE(class_name)-1 || CompareStringOrdinal(text,length,class_name,-1,FALSE)!=CSTR_EQUAL) return CLASS_E_CLASSNOTAVAILABLE;
	*out=&activation;
	return S_OK;
}
