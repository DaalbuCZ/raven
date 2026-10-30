/* Generated from Windows.UI.Xaml.winmd (SHA-256 c55875b03b9829c2e6ed6ad8b2d1ba33bd80c31208ba3bcda1a12ac46eddc41d) by scripts/generate-winrt-header.py. Do not edit. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CIUIElement __x_ABI_CWindows_CUI_CXaml_CIUIElement;
typedef struct __x_ABI_CWindows_CUI_CXaml_CIUIElementVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_DesiredSize)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_AllowDrop)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_AllowDrop)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Opacity)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Opacity)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Clip)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Clip)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_RenderTransform)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_RenderTransform)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Projection)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Projection)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_RenderTransformOrigin)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_RenderTransformOrigin)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsHitTestVisible)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_IsHitTestVisible)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Visibility)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Visibility)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_RenderSize)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_UseLayoutRounding)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_UseLayoutRounding)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Transitions)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_Transitions)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_CacheMode)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_CacheMode)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsTapEnabled)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_IsTapEnabled)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsDoubleTapEnabled)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_IsDoubleTapEnabled)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsRightTapEnabled)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_IsRightTapEnabled)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsHoldingEnabled)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_IsHoldingEnabled)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_ManipulationMode)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_ManipulationMode)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_PointerCaptures)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_KeyUp)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_KeyUp)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_KeyDown)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_KeyDown)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_GotFocus)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_GotFocus)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_LostFocus)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_LostFocus)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_DragEnter)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_DragEnter)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_DragLeave)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_DragLeave)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_DragOver)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_DragOver)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_Drop)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_Drop)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_PointerPressed)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_PointerPressed)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_PointerMoved)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_PointerMoved)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_PointerReleased)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_PointerReleased)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_PointerEntered)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_PointerEntered)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_PointerExited)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_PointerExited)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_PointerCaptureLost)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_PointerCaptureLost)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_PointerCanceled)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_PointerCanceled)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_PointerWheelChanged)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_PointerWheelChanged)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_Tapped)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_Tapped)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_DoubleTapped)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_DoubleTapped)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_Holding)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_Holding)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_RightTapped)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_RightTapped)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_ManipulationStarting)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_ManipulationStarting)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_ManipulationInertiaStarting)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_ManipulationInertiaStarting)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_ManipulationStarted)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_ManipulationStarted)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_ManipulationDelta)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_ManipulationDelta)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *add_ManipulationCompleted)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *remove_ManipulationCompleted)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *Measure)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *Arrange)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *CapturePointer)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *ReleasePointerCapture)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *ReleasePointerCaptures)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This);
	HRESULT (STDMETHODCALLTYPE *AddHandler)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2, uint64_t a3);
	HRESULT (STDMETHODCALLTYPE *RemoveHandler)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *TransformToVisual)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *InvalidateMeasure)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This);
	HRESULT (STDMETHODCALLTYPE *InvalidateArrange)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This);
	HRESULT (STDMETHODCALLTYPE *UpdateLayout)(__x_ABI_CWindows_CUI_CXaml_CIUIElement *This);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CIUIElementVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CIUIElement {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CIUIElementVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CIUIElement, 0x676d0be9, 0xb65c, 0x41c6, 0xba, 0x40, 0x58, 0xcf, 0x87, 0xf2, 0x01, 0xc1);
