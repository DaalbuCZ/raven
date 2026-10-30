/* Generated from Windows.UI.Xaml.winmd (SHA-256 c55875b03b9829c2e6ed6ad8b2d1ba33bd80c31208ba3bcda1a12ac46eddc41d) by scripts/generate-winrt-header.py. Do not edit. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlockVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_FontSize)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_FontSize)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_FontFamily)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_FontFamily)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_FontWeight)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_FontWeight)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_FontStyle)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_FontStyle)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_FontStretch)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_FontStretch)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_CharacterSpacing)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_CharacterSpacing)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Foreground)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Foreground)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_TextWrapping)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_TextWrapping)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_TextTrimming)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_TextTrimming)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_TextAlignment)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_TextAlignment)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Text)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Text)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Inlines)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Padding)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Padding)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_LineHeight)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_LineHeight)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_LineStackingStrategy)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_LineStackingStrategy)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsTextSelectionEnabled)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_IsTextSelectionEnabled)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_SelectedText)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_ContentStart)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_ContentEnd)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_SelectionStart)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_SelectionEnd)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_BaselineOffset)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_SelectionChanged)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_SelectionChanged)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_ContextMenuOpening)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_ContextMenuOpening)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *SelectAll)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This);
	HRESULT (STDMETHODCALLTYPE *Select)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *Focus)(__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock *This, uint64_t a1, uint64_t a2);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlockVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlockVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock, 0xae2d9271, 0x3b4a, 0x45fc, 0x84, 0x68, 0xf7, 0x94, 0x95, 0x48, 0xf4, 0xd5);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIPanel __x_ABI_CWindows_CUI_CXaml_CControls_CIPanel;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIPanelVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Children)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Background)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Background)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsItemsHost)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_ChildrenTransitions)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_ChildrenTransitions)(__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIPanelVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIPanel {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIPanelVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel, 0xa50a4bbd, 0x8361, 0x469c, 0x90, 0xda, 0xe9, 0xa4, 0x0c, 0x74, 0x74, 0xdf);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIGrid __x_ABI_CWindows_CUI_CXaml_CControls_CIGrid;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIGridVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_RowDefinitions)(__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_ColumnDefinitions)(__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIGridVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIGrid {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIGridVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIGrid, 0xfd104460, 0x2e15, 0x4ba3, 0x8b, 0x8f, 0xfa, 0x69, 0x3a, 0x41, 0x61, 0xe9);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIButton __x_ABI_CWindows_CUI_CXaml_CControls_CIButton;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIButtonVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIButton *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIButton *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIButton *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIButton *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIButton *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIButton *This, TrustLevel *level);

	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIButtonVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIButton {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIButtonVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton, 0x280335ae, 0x5570, 0x46c7, 0x8e, 0x0b, 0x60, 0x2b, 0xe7, 0x12, 0x29, 0xa2);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControlVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Content)(__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Content)(__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_ContentTemplate)(__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_ContentTemplate)(__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_ContentTemplateSelector)(__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_ContentTemplateSelector)(__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_ContentTransitions)(__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_ContentTransitions)(__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControlVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControlVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl, 0xa26dd1dc, 0xcd44, 0x435c, 0xbe, 0x94, 0x01, 0xd6, 0x24, 0x1c, 0x23, 0x1c);
