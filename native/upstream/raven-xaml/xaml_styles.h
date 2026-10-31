/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Engine-internal surface of the style stage, shared between xaml_styles.c (the Style, Setter and SetterBaseCollection
 * classes) and xaml_elements.c (the per-element style storage and the put_Style application that runs a style's — and its
 * BasedOn chain's — setters against the element's real storage). The fixture-facing entry points are marked
 * __declspec(dllexport) at their definitions — the component exports only what it marks, and the style fixture reaches
 * these through GetProcAddress the way it reaches RavenXamlLoad. */
#ifndef RAVEN_XAML_STYLES_H
#define RAVEN_XAML_STYLES_H

#include <windows.h>

/* Engine-internal setter application. DependencyProperty is not implemented yet, so a setter is addressed by its property
 * NAME: the call stores a (property-name string, value IInspectable) pair on the style — the pair shape the put_Style
 * application walks. The value is owned; the style keeps its own reference. */
HRESULT raven_style_add_setter(IInspectable *style, const WCHAR *property, UINT32 length, IInspectable *value);

/* What the put_Style application walks: the stored pairs in application order (the order raven_style_add_setter appended
 * them). setter_at hands out the property name pointing into the style's own storage and a value reference the caller
 * releases; both answer S_FALSE for an object that is not an engine Style. Both are fixture-facing as well — the style
 * fixture reads the stored pairs back through them. */
HRESULT raven_style_setter_count(IInspectable *style, UINT32 *count);
HRESULT raven_style_setter_at(IInspectable *style, UINT32 index, const WCHAR **property, UINT32 *length,
	IInspectable **value);

/* The authored default styles: one sealed engine Style per control class the put_Style application supports, built lazily
 * on first use from the same raven_style_add_setter path the application code uses. raven_style_default_attach puts the
 * class's default on an element through raven_xaml_element_put_style — the constructor's initial assignment, so a control
 * that never sees an explicit put_Style still carries the documented default values, and a later put_Style replaces it the
 * way any assignment does. The default objects are shared between every instance of the class and SEALED at authoring: an
 * application holding get_Style's answer cannot mutate what all of them point at, because every mutation slot of a sealed
 * style (Setters append/clear, Setter put_Value, put_TargetType, put_BasedOn) rejects. The objects live for the component's
 * lifetime. Answers S_FALSE for an element class the engine has no default for. */
HRESULT __declspec(dllexport) raven_style_default_attach(IInspectable *element);

/* The per-element Style property. Both store and clear an owned reference on the element classes the engine implements (the
 * grid, the text block and the button); other classes answer S_FALSE. put_Style re-renders, whether it attaches or clears. */
HRESULT raven_xaml_element_put_style(IInspectable *element, IInspectable *style);
HRESULT raven_xaml_element_get_style(IInspectable *element, IInspectable **style);

#endif
