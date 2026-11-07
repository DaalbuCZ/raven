/* SPDX-License-Identifier: MIT */
/* Hand-typed from the Windows SDK header windows.ui.xaml.hosting.referencetracker.h (Windows 8+; the
 * interface IIDs and vtable orders match that header, verified against the published SDK sources).
 * MinGW carries no declarations for these, and they are absent from Windows.UI.Xaml.winmd because
 * they are classic COM, not projected WinRT. Every XAML framework object implements IReferenceTracker
 * and the .NET runtime's SharedLibrary asks for it when a XAML object is handed to managed code, so
 * the raven component declares the five interfaces it may be handed or asked for. */
#ifndef RAVEN_XAML_WINDOWS_UI_XAML_HOSTING_REFERENCETRACKER_H
#define RAVEN_XAML_WINDOWS_UI_XAML_HOSTING_REFERENCETRACKER_H

#include <windows.h>
#include <inspectable.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTarget __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTarget;
typedef struct __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTargetVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTarget *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTarget *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTarget *This);
	/* A target the tracker hands the garbage collector: the tracker-side reference count is distinct
	 * from the COM count and Peg/Unpeg protect the object across a collection. */
	ULONG (STDMETHODCALLTYPE *AddRefFromReferenceTracker)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTarget *This);
	ULONG (STDMETHODCALLTYPE *ReleaseFromReferenceTracker)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTarget *This);
	HRESULT (STDMETHODCALLTYPE *Peg)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTarget *This);
	HRESULT (STDMETHODCALLTYPE *Unpeg)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTarget *This);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTargetVtbl;
interface __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTarget {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTargetVtbl *lpVtbl;
};

typedef struct __x_ABI_CWindows_CUI_CXaml_CHosting_CIFindReferenceTargetsCallback __x_ABI_CWindows_CUI_CXaml_CHosting_CIFindReferenceTargetsCallback;
typedef struct __x_ABI_CWindows_CUI_CXaml_CHosting_CIFindReferenceTargetsCallbackVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIFindReferenceTargetsCallback *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIFindReferenceTargetsCallback *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIFindReferenceTargetsCallback *This);
	/* The collector's per-object callback: FindTrackerTargets invokes it once per reachable target. */
	HRESULT (STDMETHODCALLTYPE *FoundTrackerTarget)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIFindReferenceTargetsCallback *This,
		__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTarget *target);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CHosting_CIFindReferenceTargetsCallbackVtbl;
interface __x_ABI_CWindows_CUI_CXaml_CHosting_CIFindReferenceTargetsCallback {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CHosting_CIFindReferenceTargetsCallbackVtbl *lpVtbl;
};

typedef struct __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost;
typedef struct __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager;
typedef struct __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManagerVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager *This);
	/* The per-collection round handshake the runtime drives against the framework's manager. */
	HRESULT (STDMETHODCALLTYPE *ReferenceTrackingStarted)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager *This);
	HRESULT (STDMETHODCALLTYPE *FindTrackerTargetsCompleted)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager *This, boolean findFailed);
	HRESULT (STDMETHODCALLTYPE *ReferenceTrackingCompleted)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager *This);
	HRESULT (STDMETHODCALLTYPE *SetReferenceTrackerHost)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager *This,
		__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost *host);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManagerVtbl;
interface __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManagerVtbl *lpVtbl;
};

typedef struct __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost;
typedef struct __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHostVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost *This);
	/* The runtime's host, given to the manager once: disconnect decisions, thread teardown and memory pressure. */
	HRESULT (STDMETHODCALLTYPE *DisconnectUnusedReferenceSources)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost *This, uint32_t options);
	HRESULT (STDMETHODCALLTYPE *ReleaseDisconnectedReferenceSources)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost *This);
	HRESULT (STDMETHODCALLTYPE *NotifyEndOfReferenceTrackingOnThread)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost *This);
	HRESULT (STDMETHODCALLTYPE *GetTrackerTarget)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost *This,
		IUnknown *unknown, __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTarget **newReference);
	HRESULT (STDMETHODCALLTYPE *AddMemoryPressure)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost *This, uint64_t bytesAllocated);
	HRESULT (STDMETHODCALLTYPE *RemoveMemoryPressure)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost *This, uint64_t bytesAllocated);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHostVtbl;
interface __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHostVtbl *lpVtbl;
};

typedef struct __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker;
typedef struct __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker *This);
	/* The per-object face the runtime negotiates with: connect/disconnect bookkeeping, the collection
	 * walk, the framework manager, and the AddRef/Release/Peg notifications around tracker-source
	 * references. The slot order is the SDK header's. */
	HRESULT (STDMETHODCALLTYPE *ConnectFromTrackerSource)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker *This);
	HRESULT (STDMETHODCALLTYPE *DisconnectFromTrackerSource)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker *This);
	HRESULT (STDMETHODCALLTYPE *FindTrackerTargets)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker *This,
		__x_ABI_CWindows_CUI_CXaml_CHosting_CIFindReferenceTargetsCallback *callback);
	HRESULT (STDMETHODCALLTYPE *GetReferenceTrackerManager)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker *This,
		__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager **value);
	HRESULT (STDMETHODCALLTYPE *AddRefFromTrackerSource)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker *This);
	HRESULT (STDMETHODCALLTYPE *ReleaseFromTrackerSource)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker *This);
	HRESULT (STDMETHODCALLTYPE *PegFromTrackerSource)(__x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker *This);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerVtbl;
interface __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerVtbl *lpVtbl;
};

DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTracker, 0x11d3b13a, 0x180e, 0x4789, 0xa8, 0xbe, 0x77, 0x12, 0x88, 0x28, 0x93, 0xe6);
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerTarget, 0x64bd43f8, 0xbfee, 0x4ec4, 0xb7, 0xeb, 0x29, 0x35, 0x15, 0x8d, 0xae, 0x21);
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerManager, 0x3cf184b4, 0x7ccb, 0x4dda, 0x84, 0x55, 0x7e, 0x6c, 0xe9, 0x9a, 0x32, 0x98);
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CHosting_CIFindReferenceTargetsCallback, 0x04b3486c, 0x4687, 0x4229, 0x8d, 0x14, 0x50, 0x5a, 0xb5, 0x84, 0xdd, 0x88);
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CHosting_CIReferenceTrackerHost, 0x29a71c6a, 0x3c42, 0x4416, 0xa3, 0x9d, 0xe2, 0x82, 0x5a, 0x07, 0xa7, 0x77);

#endif
