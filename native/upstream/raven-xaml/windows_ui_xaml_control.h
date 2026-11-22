/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 */
#pragma once

/* The real IControl ABI (Windows.UI.Xaml.winmd, uuid A8912263-2951-4F58-A9C5-5A134EAA7F07): the members in the interface's
 * declaration order after the six IInspectable methods, Template among them. The hand headers cannot carry this interface
 * (they list only the classes earlier stages needed), so it is declared here once and the IID is defined in every TU that
 * includes the declaration (DEFINE_GUID is selectany under INITGUID). The engine stores only Template; the other members
 * answer E_NOTIMPL the way the control chrome itself waits for its stage. */
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIControl control1;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIControlVtbl control1_vtbl;
struct __x_ABI_CWindows_CUI_CXaml_CControls_CIControlVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(control1 *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(control1 *This);
	ULONG (STDMETHODCALLTYPE *Release)(control1 *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(control1 *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(control1 *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(control1 *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_FontSize)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_FontSize)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_FontFamily)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_FontFamily)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_FontWeight)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_FontWeight)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_FontStyle)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_FontStyle)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_FontStretch)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_FontStretch)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_CharacterSpacing)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_CharacterSpacing)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_Foreground)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_Foreground)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_IsTabStop)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_IsTabStop)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_IsEnabled)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_IsEnabled)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_TabIndex)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_TabIndex)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_TabNavigation)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_TabNavigation)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_Template)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_Template)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_Padding)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_Padding)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_HorizontalContentAlignment)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_HorizontalContentAlignment)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_VerticalContentAlignment)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_VerticalContentAlignment)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_Background)(control1 *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *put_Background)(control1 *This, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *get_BorderThickness)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_BorderThickness)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_BorderBrush)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_BorderBrush)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_FocusState)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *add_IsEnabledChanged)(control1 *This, uint64_t handler, uint64_t token);
	HRESULT (STDMETHODCALLTYPE *remove_IsEnabledChanged)(control1 *This, uint64_t token);
	HRESULT (STDMETHODCALLTYPE *ApplyTemplate)(control1 *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *Focus)(control1 *This, uint64_t state, uint64_t value);
	END_INTERFACE
};
interface __x_ABI_CWindows_CUI_CXaml_CControls_CIControl { CONST_VTBL control1_vtbl *lpVtbl; };
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControl, 0xa8912263, 0x2951, 0x4f58, 0xa9, 0xc5, 0x5a, 0x13,
	0x4e, 0xaa, 0x7f, 0x07);

