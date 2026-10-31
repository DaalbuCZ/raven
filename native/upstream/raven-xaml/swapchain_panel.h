/* SPDX-License-Identifier: MIT */
#ifndef RAVEN_SWAPCHAIN_PANEL_H
#define RAVEN_SWAPCHAIN_PANEL_H
#include <inspectable.h>
#include <windows.ui.core.h>
#include <dxgi.h>

/* Older mingw omits this interface and CoreInputDeviceTypes (an INT32 flags enum). */
typedef struct __x_ABI_CWindows_CUI_CCore_CICoreInputSourceBase __x_ABI_CWindows_CUI_CCore_CICoreInputSourceBase;

/* SDK 10.0.16299.0, windows.ui.xaml.controls.h and um/windows.ui.xaml.media.dxinterop.h:
 * https://github.com/tpn/winsdk-10/tree/master/Include/10.0.16299.0
 * ISwapChainPanel has no SwapChain or CompositeMode property. Keep native attachment separate.
 * Integrator: move these verified declarations into the generated/public interop headers. */
typedef struct raven_swapchain_panel raven_swapchain_panel;
typedef struct raven_swapchain_panel_vtbl {
	HRESULT (WINAPI *QueryInterface)(raven_swapchain_panel *, REFIID, void **);
	ULONG (WINAPI *AddRef)(raven_swapchain_panel *);
	ULONG (WINAPI *Release)(raven_swapchain_panel *);
	HRESULT (WINAPI *GetIids)(raven_swapchain_panel *, ULONG *, IID **);
	HRESULT (WINAPI *GetRuntimeClassName)(raven_swapchain_panel *, HSTRING *);
	HRESULT (WINAPI *GetTrustLevel)(raven_swapchain_panel *, TrustLevel *);
	HRESULT (WINAPI *get_CompositionScaleX)(raven_swapchain_panel *, FLOAT *);
	HRESULT (WINAPI *get_CompositionScaleY)(raven_swapchain_panel *, FLOAT *);
	HRESULT (WINAPI *add_CompositionScaleChanged)(raven_swapchain_panel *, IUnknown *, EventRegistrationToken *);
	HRESULT (WINAPI *remove_CompositionScaleChanged)(raven_swapchain_panel *, EventRegistrationToken);
	HRESULT (WINAPI *CreateCoreIndependentInputSource)(raven_swapchain_panel *,
		INT32, __x_ABI_CWindows_CUI_CCore_CICoreInputSourceBase **);
} raven_swapchain_panel_vtbl;
struct raven_swapchain_panel { const raven_swapchain_panel_vtbl *lpVtbl; };
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CControls_CISwapChainPanel,
	0xc589644f, 0xeba8, 0x427a, 0xb7, 0x5a, 0x9f, 0x1f, 0x93, 0xa1, 0x1a, 0xe9);

typedef struct raven_swapchain_native raven_swapchain_native;
typedef struct raven_swapchain_native_vtbl {
	HRESULT (WINAPI *QueryInterface)(raven_swapchain_native *, REFIID, void **);
	ULONG (WINAPI *AddRef)(raven_swapchain_native *);
	ULONG (WINAPI *Release)(raven_swapchain_native *);
	HRESULT (WINAPI *SetSwapChain)(raven_swapchain_native *, IDXGISwapChain *);
} raven_swapchain_native_vtbl;
struct raven_swapchain_native { const raven_swapchain_native_vtbl *lpVtbl; };
DEFINE_GUID(IID_ISwapChainPanelNative, 0xf92f19d2, 0x3ade, 0x45a6, 0xa2, 0x0c, 0xf6, 0xf1, 0xea, 0x90, 0x55, 0x4b);
#endif
