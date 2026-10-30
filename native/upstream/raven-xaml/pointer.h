/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
#ifndef RAVEN_XAML_POINTER_H
#define RAVEN_XAML_POINTER_H
#include <windows.ui.input.h>
struct raven_mouse_sample {
	__x_ABI_CWindows_CFoundation_CPoint position;
	UINT32 buttons, frame;
	UINT64 timestamp;
	__x_ABI_CWindows_CSystem_CVirtualKeyModifiers modifiers;
	__x_ABI_CWindows_CUI_CInput_CPointerUpdateKind update;
	INT32 wheel;
	boolean horizontal;
};
HRESULT raven_mouse_point_create(const struct raven_mouse_sample *sample, __x_ABI_CWindows_CUI_CInput_CIPointerPoint **out);
#endif
