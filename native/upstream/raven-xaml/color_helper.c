/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Windows.UI.ColorHelper: the static color helpers per the metadata (both the mingw windows.ui.h and
 * the shipped Windows.UI.winmd agree): IColorHelperStatics answers FromArgb, the derived
 * IColorHelperStatics2 answers ToDisplayName (the palette name of a color, or the empty string for one
 * no palette name covers). The guest's material pipeline reaches for the class when it builds colors. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <winstring.h>
#include <windows.ui.h>
typedef __x_ABI_CWindows_CUI_CIColorHelperStatics helper_i;
typedef __x_ABI_CWindows_CUI_CIColorHelperStatics2 helper2_i;
typedef __x_ABI_CWindows_CUI_CColor color;
static const WCHAR class_name[] = L"Windows.UI.ColorHelper";
static IActivationFactory activation;
static helper_i statics;
static helper2_i statics2;
static HRESULT WINAPI query(IActivationFactory *self, REFIID iid, void **out)
{
	(void)self;
	if (!out) return E_POINTER;
	*out=NULL;
	if (IsEqualIID(iid,&IID_IUnknown) || IsEqualIID(iid,&IID_IInspectable) || IsEqualIID(iid,&IID_IActivationFactory) || IsEqualIID(iid,&IID_IAgileObject)) *out=&activation;
	else if (IsEqualIID(iid,&IID___x_ABI_CWindows_CUI_CIColorHelperStatics)) *out=&statics;
	else if (IsEqualIID(iid,&IID___x_ABI_CWindows_CUI_CIColorHelperStatics2)) *out=&statics2;
	else return E_NOINTERFACE;
	return S_OK;
}
static ULONG WINAPI addref(IActivationFactory *self) { (void)self; return 2; }
static ULONG WINAPI release(IActivationFactory *self) { (void)self; return 1; }
static HRESULT WINAPI iids(IActivationFactory *self, ULONG *count, IID **out) { (void)self; if (!count || !out) return E_POINTER; *count=0; *out=NULL; return S_OK; }
static HRESULT WINAPI name(IActivationFactory *self,HSTRING *out) { (void)self; if (!out) return E_POINTER; return WindowsCreateString(class_name,ARRAYSIZE(class_name)-1,out); }
static HRESULT WINAPI trust(IActivationFactory *self,TrustLevel *out) { (void)self; if (!out) return E_POINTER; *out=BaseTrust; return S_OK; }
static HRESULT WINAPI activate(IActivationFactory *self,IInspectable **out) { (void)self; if (!out) return E_POINTER; *out=NULL; return E_NOTIMPL; }
static HRESULT WINAPI from_argb(helper_i *self, BYTE a, BYTE r, BYTE g, BYTE b, color *out)
{
	(void)self;
	if (!out) return E_POINTER;
	out->A=a; out->R=r; out->G=g; out->B=b;
	return S_OK;
}
#define WIDE(text) L##text
#define WNAME(n) WIDE(#n)
static const struct { const WCHAR *name; BYTE a, r, g, b; } palette[] = {
#define COLOR(n,a,r,g,b) { WNAME(n), a, r, g, b },
#include "colors.inc"
#undef COLOR
};
#undef WNAME
#undef WIDE
static UINT32 palette_len(const WCHAR *text) { UINT32 n = 0; while (text[n]) ++n; return n; }
static HRESULT WINAPI to_display_name(helper2_i *self, color value, HSTRING *out)
{
	UINT32 i, count = ARRAYSIZE(palette);
	const WCHAR *text = NULL;
	(void)self;
	/* the standard palette's names; a color outside it has no display name and the empty string answers */
	if (!out) return E_POINTER;
	*out = NULL;
	for (i = 0; i < count; ++i) {
		if (palette[i].a == value.A && palette[i].r == value.R && palette[i].g == value.G && palette[i].b == value.B) {
			text = palette[i].name;
			break;
		}
	}
	return WindowsCreateString(text ? text : L"", text ? palette_len(text) : 0, out);
}
static const __x_ABI_CWindows_CUI_CIColorHelperStaticsVtbl statics_table = {
	(void *)query,(void *)addref,(void *)release,(void *)iids,(void *)name,(void *)trust,
	from_argb
};
static const __x_ABI_CWindows_CUI_CIColorHelperStatics2Vtbl statics2_table = {
	(void *)query,(void *)addref,(void *)release,(void *)iids,(void *)name,(void *)trust,
	to_display_name
};
static helper_i statics = {&statics_table};
static helper2_i statics2 = {&statics2_table};
static const IActivationFactoryVtbl activation_table = {query,addref,release,iids,name,trust,activate};
static IActivationFactory activation = {&activation_table};
HRESULT raven_color_helper_factory(HSTRING key,IActivationFactory **out)
{
	UINT32 length=0; const WCHAR *text=WindowsGetStringRawBuffer(key,&length);
	if (!out) return E_POINTER;
	*out=NULL;
	if (length != ARRAYSIZE(class_name)-1 || CompareStringOrdinal(text,length,class_name,-1,FALSE)!=CSTR_EQUAL) return CLASS_E_CLASSNOTAVAILABLE;
	*out=&activation;
	return S_OK;
}
