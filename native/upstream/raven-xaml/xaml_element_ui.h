/* SPDX-License-Identifier: MIT */
#ifndef RAVEN_XAML_ELEMENT_UI_H
#define RAVEN_XAML_ELEMENT_UI_H
#include "windows_ui_xaml_uielement.h"
#include "xaml_input.h"
#include "events.h"
typedef HRESULT (*element_query_function)(void *owner, REFIID iid, void **out);
typedef ULONG (*element_reference_function)(void *owner);
struct routed_registration;
struct element_ui {
	const __x_ABI_CWindows_CUI_CXaml_CIUIElementVtbl *lpVtbl;
	void *owner;
	HRESULT (*owner_query)(void *owner, REFIID iid, void **out);
	ULONG (*owner_addref)(void *owner);
	ULONG (*owner_release)(void *owner);
	const WCHAR *class_name;
	/* The element-specific pointer behavior (the button's IsPressed and Click); NULL where the engine has none. */
	void (*behavior)(void *owner, IInspectable *element, enum raven_element_pointer kind,
		__x_ABI_CWindows_CUI_CCore_CIPointerEventArgs *args);
	struct raven_event_list events[RAVEN_ELEMENT_POINTER_COUNT];
	struct routed_registration *routed_handlers;
	boolean hit_test_visible;
};

void element_ui_init(struct element_ui *, void *, element_query_function, element_reference_function, element_reference_function, const WCHAR *);
void element_ui_release(struct element_ui *);
#endif
