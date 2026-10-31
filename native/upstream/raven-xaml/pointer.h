/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
#ifndef RAVEN_XAML_POINTER_H
#define RAVEN_XAML_POINTER_H
#include <activation.h>
#include <windows.ui.input.h>
struct raven_mouse_sample {
	__x_ABI_CWindows_CFoundation_CPoint position;
	UINT32 buttons, frame;
	UINT64 timestamp;
	__x_ABI_CWindows_CSystem_CVirtualKeyModifiers modifiers;
	__x_ABI_CWindows_CUI_CInput_CPointerUpdateKind update;
	INT32 wheel;
	boolean horizontal;
	/* Which pointer the sample belongs to. The desktop mouse pipeline fills in
	 * PointerDeviceType_Mouse and pointer id 1; a WM_POINTER contact fills in its
	 * own device and the id its message carried. The boolean state the mouse
	 * derives from its button bits (contact, the primary pointer, hover range)
	 * is what a touch contact reports directly, so the layout carries it once. */
	__x_ABI_CWindows_CDevices_CInput_CPointerDeviceType device;
	UINT32 pointer_id;
	boolean in_contact, primary, in_range;
};
HRESULT raven_mouse_point_create(const struct raven_mouse_sample *sample, __x_ABI_CWindows_CUI_CInput_CIPointerPoint **out);
/* Records the pointer id a WM_POINTER contact arrived under, so GetPointerDevice can
 * resolve the ids the engine actually delivered; the engine mouse's id 1 needs no record. */
void raven_pointer_track(UINT32 pointer_id, __x_ABI_CWindows_CDevices_CInput_CPointerDeviceType type);
/* Builds an IVector<PointerPoint> snapshot owning one reference per element, so a history handed to
 * event args outlives the sample ring that produced it. windows.ui.input.h declares the
 * __FIVector_1_Windows__CUI__CInput__CPointerPoint instantiation (IID dfa655cf-fde7-5048-b4bf-c909231b7edb)
 * and the implementation uses it directly. */
__declspec(dllexport) HRESULT raven_pointer_history_create(const struct raven_mouse_sample *samples, UINT32 count,
	__FIVector_1_Windows__CUI__CInput__CPointerPoint **out);
/* The Windows.Devices.Input.PointerDevice statics: GetPointerDevice and GetPointerDevices over the
 * host's own device enumeration. The class has no default activation on real Windows, so its
 * factory part faults ActivateInstance and answers only the statics interface. */
__declspec(dllexport) HRESULT raven_pointer_device_statics(__x_ABI_CWindows_CDevices_CInput_CIPointerDeviceStatics **out);
/* The activation-chain entry for "Windows.Devices.Input.PointerDevice": CLASS_E_CLASSNOTAVAILABLE for any other name. */
HRESULT raven_pointer_device_factory(HSTRING name, IActivationFactory **out);
#endif
