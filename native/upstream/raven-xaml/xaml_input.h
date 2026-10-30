/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
#ifndef RAVEN_XAML_INPUT_H
#define RAVEN_XAML_INPUT_H

/* Pointer routing between the CoreWindow and the hosted element tree.
 *
 * The render pass records the DIP rectangle of every element it projects (raven_input_record, in paint order), the routing
 * subscribes once to the hosting CoreWindow's pointer events (raven_input_hosted) and hit-tests each event's DIP position
 * against the recorded geometry, and delivery (raven_xaml_element_pointer, implemented with the element classes) invokes the
 * handlers the application stored on the element's IUIElement. The plumbing is declared here rather than in xaml_internal.h so
 * the guest clients can build handlers against the same shapes the engine dispatches. */

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>
#include <windows.foundation.h>
#include <windows.ui.core.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CIUIElement __x_ABI_CWindows_CUI_CXaml_CIUIElement;

/* The routed event arguments an element handler receives. PointerRoutedEventArgs is a Windows.UI.Xaml.Input type the engine has
 * no C header for; the routing hands the CoreWindow's PointerEventArgs snapshot across (one immutable snapshot per desktop
 * message, built by the CoreWindow's own event path), and the ABI is the word-sized receiver either way. */
typedef struct __x_ABI_CWindows_CUI_CXaml_CInput_CIPointerRoutedEventArgs __x_ABI_CWindows_CUI_CXaml_CInput_CIPointerRoutedEventArgs;
typedef struct __x_ABI_CWindows_CUI_CXaml_CRoutedEventArgs __x_ABI_CWindows_CUI_CXaml_CRoutedEventArgs;

/* TypedEventHandler<Windows.UI.Xaml.UIElement, Windows.UI.Xaml.Input.PointerRoutedEventArgs>, the delegate type of the
 * UIElement pointer events. The engine stores and invokes handlers through IUnknown, so the instantiation's IID is not
 * referenced by the engine and is not derived here. */
typedef struct __FITypedEventHandler_2_Windows__CUI__CXaml__CUIElement_Windows__CUI__CXaml__CInput__CPointerRoutedEventArgs __FITypedEventHandler_2_Windows__CUI__CXaml__CUIElement_Windows__CUI__CXaml__CInput__CPointerRoutedEventArgs;
typedef struct __FITypedEventHandler_2_Windows__CUI__CXaml__CUIElement_Windows__CUI__CXaml__CInput__CPointerRoutedEventArgsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__FITypedEventHandler_2_Windows__CUI__CXaml__CUIElement_Windows__CUI__CXaml__CInput__CPointerRoutedEventArgs *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__FITypedEventHandler_2_Windows__CUI__CXaml__CUIElement_Windows__CUI__CXaml__CInput__CPointerRoutedEventArgs *This);
	ULONG (STDMETHODCALLTYPE *Release)(__FITypedEventHandler_2_Windows__CUI__CXaml__CUIElement_Windows__CUI__CXaml__CInput__CPointerRoutedEventArgs *This);
	HRESULT (STDMETHODCALLTYPE *Invoke)(__FITypedEventHandler_2_Windows__CUI__CXaml__CUIElement_Windows__CUI__CXaml__CInput__CPointerRoutedEventArgs *This,
		__x_ABI_CWindows_CUI_CXaml_CIUIElement *sender, __x_ABI_CWindows_CUI_CXaml_CInput_CIPointerRoutedEventArgs *args);
	END_INTERFACE
} __FITypedEventHandler_2_Windows__CUI__CXaml__CUIElement_Windows__CUI__CXaml__CInput__CPointerRoutedEventArgsVtbl;
struct __FITypedEventHandler_2_Windows__CUI__CXaml__CUIElement_Windows__CUI__CXaml__CInput__CPointerRoutedEventArgs {
	CONST_VTBL __FITypedEventHandler_2_Windows__CUI__CXaml__CUIElement_Windows__CUI__CXaml__CInput__CPointerRoutedEventArgsVtbl *lpVtbl;
};

/* TypedEventHandler<Windows.Foundation.Object (IInspectable), Windows.UI.Xaml.RoutedEventArgs>, the delegate type of
 * ButtonBase.Click. The engine raises Click with a minimal IInspectable-shaped RoutedEventArgs snapshot. */
typedef struct __FITypedEventHandler_2_Windows__CObject_Windows__CUI__CXaml__CRoutedEventArgs __FITypedEventHandler_2_Windows__CObject_Windows__CUI__CXaml__CRoutedEventArgs;
typedef struct __FITypedEventHandler_2_Windows__CObject_Windows__CUI__CXaml__CRoutedEventArgsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__FITypedEventHandler_2_Windows__CObject_Windows__CUI__CXaml__CRoutedEventArgs *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__FITypedEventHandler_2_Windows__CObject_Windows__CUI__CXaml__CRoutedEventArgs *This);
	ULONG (STDMETHODCALLTYPE *Release)(__FITypedEventHandler_2_Windows__CObject_Windows__CUI__CXaml__CRoutedEventArgs *This);
	HRESULT (STDMETHODCALLTYPE *Invoke)(__FITypedEventHandler_2_Windows__CObject_Windows__CUI__CXaml__CRoutedEventArgs *This,
		IInspectable *sender, __x_ABI_CWindows_CUI_CXaml_CRoutedEventArgs *args);
	END_INTERFACE
} __FITypedEventHandler_2_Windows__CObject_Windows__CUI__CXaml__CRoutedEventArgsVtbl;
struct __FITypedEventHandler_2_Windows__CObject_Windows__CUI__CXaml__CRoutedEventArgs {
	CONST_VTBL __FITypedEventHandler_2_Windows__CObject_Windows__CUI__CXaml__CRoutedEventArgsVtbl *lpVtbl;
};

/* The pointer events an element receives from the routing. */
enum raven_element_pointer {
	RAVEN_ELEMENT_POINTER_ENTERED, RAVEN_ELEMENT_POINTER_EXITED, RAVEN_ELEMENT_POINTER_MOVED,
	RAVEN_ELEMENT_POINTER_PRESSED, RAVEN_ELEMENT_POINTER_RELEASED, RAVEN_ELEMENT_POINTER_WHEEL,
	RAVEN_ELEMENT_POINTER_CAPTURE_LOST, RAVEN_ELEMENT_POINTER_COUNT
};

/* Windows.UI.Xaml.Controls.ClickMode; the values follow the winmd enumeration (Release, Press, Hover). */
enum raven_xaml_click_mode {
	raven_xaml_click_release = 0,
	raven_xaml_click_press = 1,
	raven_xaml_click_hover = 2
};

/* The render pass tells the routing what geometry it projected. Begin clears the window's recorded rectangles, record appends
 * one element's rect in paint order (ancestors before descendants, so a reverse walk finds the topmost element first). The
 * pass and the routed events run on the hosting UI thread; there is no lock between them. */
void raven_input_begin(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window);
void raven_input_record(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window, IInspectable *element,
	float x, float y, float width, float height);

/* A tree was hosted on the window (or replaced by another, or removed with a null element): subscribe once to the CoreWindow's
 * pointer events for this tree, dropping the previous subscription and geometry. The subscription ends when the CoreWindow
 * closes, which the routing observes through the window's Closed event. */
HRESULT raven_input_hosted(__x_ABI_CWindows_CUI_CCore_CICoreWindow *window, IInspectable *element);

/* Whether the position falls inside the most recently recorded rectangle of the element. ButtonBase uses this for the
 * ClickMode.Release contract, where the release must happen within the button's bounds. Unknown elements contain nothing. */
BOOL raven_input_element_contains(IInspectable *element, const __x_ABI_CWindows_CFoundation_CPoint *position);

/* Deliver one pointer event to an element: the element's own behavior first (a button tracks IsPressed and raises Click), then
 * the handlers the application stored on its IUIElement, each with the CoreWindow's event args and the element as sender.
 * Handler failures never propagate; the routing stays alive whatever an application handler returns. */
void raven_xaml_element_pointer(IInspectable *element, enum raven_element_pointer kind,
	__x_ABI_CWindows_CUI_CCore_CIPointerEventArgs *args);

#endif
