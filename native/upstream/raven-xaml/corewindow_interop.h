/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
#ifndef RAVEN_COREWINDOW_INTEROP_H
#define RAVEN_COREWINDOW_INTEROP_H
/* ABI from the public SDK CoreWindow.h (missing in mingw):
 * https://github.com/microsoft/win32metadata/blob/main/generation/WinSDK/RecompiledIdlHeaders/winrt/CoreWindow.h */
#include <windows.h>
#include <inspectable.h>
static const IID raven_core_window_interop_iid = {0x45d64a29,0xa63e,0x4cb6,{0xb4,0x98,0x57,0x81,0xd2,0x98,0xcb,0x4f}};
typedef struct raven_core_window_interop raven_core_window_interop;
struct raven_core_window_interop_vtable {
	HRESULT (WINAPI *QueryInterface)(raven_core_window_interop *, REFIID, void **);
	ULONG (WINAPI *AddRef)(raven_core_window_interop *);
	ULONG (WINAPI *Release)(raven_core_window_interop *);
	HRESULT (WINAPI *get_WindowHandle)(raven_core_window_interop *, HWND *);
	HRESULT (WINAPI *put_MessageHandled)(raven_core_window_interop *, boolean);
};
struct raven_core_window_interop { const struct raven_core_window_interop_vtable *lpVtbl; };
#endif
