/* Generated from Windows.UI.Xaml.winmd (SHA-256 c55875b03b9829c2e6ed6ad8b2d1ba33bd80c31208ba3bcda1a12ac46eddc41d) by scripts/generate-winrt-header.py. Do not edit. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer;
typedef struct __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeerVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_EventsSource)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_EventsSource)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetPattern)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *RaiseAutomationEvent)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *RaisePropertyChangedEvent)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1, uint64_t a2, uint64_t a3);
	HRESULT (STDMETHODCALLTYPE *GetAcceleratorKey)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetAccessKey)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetAutomationControlType)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetAutomationId)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetBoundingRectangle)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetChildren)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetClassName)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetClickablePoint)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetHelpText)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetItemStatus)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetItemType)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetLabeledBy)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetLocalizedControlType)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetName)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetOrientation)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *HasKeyboardFocus)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *IsContentElement)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *IsControlElement)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *IsEnabled)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *IsKeyboardFocusable)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *IsOffscreen)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *IsPassword)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *IsRequiredForForm)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *SetFocus)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This);
	HRESULT (STDMETHODCALLTYPE *GetParent)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *InvalidatePeer)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This);
	HRESULT (STDMETHODCALLTYPE *GetPeerFromPoint)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *GetLiveSetting)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeerVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeerVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer, 0x35aac87a, 0x62ee, 0x4d3e, 0xa2, 0x4c, 0x2b, 0xc8, 0x43, 0x2d, 0x68, 0xb7);
typedef struct __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeer __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeer;
typedef struct __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeer *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeer *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeer *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeer *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeer *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeer *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Owner)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeer *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeer {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeer, 0xb90ad781, 0xbfeb, 0x4451, 0xbd, 0x47, 0x9f, 0x3a, 0x63, 0xeb, 0xd2, 0x4a);
typedef struct __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics;
typedef struct __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *FromElement)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *CreatePeerForElement)(__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics *This, uint64_t a1, uint64_t a2);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStaticsVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStaticsVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics, 0xb9c0b997, 0x2820, 0x44a1, 0xa5, 0xa8, 0x9b, 0x80, 0x1e, 0xdc, 0x26, 0x9e);
