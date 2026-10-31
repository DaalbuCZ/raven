/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
#ifndef RAVEN_XAML_AUTOMATION_H
#define RAVEN_XAML_AUTOMATION_H
/* The automation-peer surface the engine exports to the peer classes: the name a screen reader would announce for an
 * element, read back from the element's own state. Precedence mirrors FrameworkElementAutomationPeer.GetName: the element's
 * Name; for a text block an empty Name falls back to its Text; S_FALSE means the element carries nothing and the peer uses
 * its runtime class name. */
#include <windows.h>
#include <winstring.h>

HRESULT raven_xaml_element_automation_name(IInspectable *element, HSTRING *out);

/* The automation Invoke: raises the button's Click through the button's own raise, independent of the click mode, the way
 * the real ButtonAutomationPeer.Invoke calls OnClick. A non-button answers nothing. */
void raven_xaml_button_invoke(IInspectable *element);

#endif
