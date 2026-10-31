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
 * releases; both answer S_FALSE for an object that is not an engine Style. */
HRESULT raven_style_setter_count(IInspectable *style, UINT32 *count);
HRESULT raven_style_setter_at(IInspectable *style, UINT32 index, const WCHAR **property, UINT32 *length,
	IInspectable **value);

/* The per-element Style property. Both store and clear an owned reference on the element classes the engine implements (the
 * grid, the text block and the button); other classes answer S_FALSE. put_Style re-renders, whether it attaches or clears. */
HRESULT raven_xaml_element_put_style(IInspectable *element, IInspectable *style);
HRESULT raven_xaml_element_get_style(IInspectable *element, IInspectable **style);

#endif
