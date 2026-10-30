/* Generated from Windows.ApplicationModel.winmd (SHA-256 25c17722258acd8f3370612d6c34c6464578741f28bacbfa2e7fe56c1d1536f2) by scripts/generate-winrt-header.py. Do not edit. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource __x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource;
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResourceVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Uri)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Candidates)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *Resolve)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *Resolve2)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *ResolveAll)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *ResolveAll2)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource *This, uint64_t a1, uint64_t a2);
	END_INTERFACE
} __x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResourceVtbl;

interface __x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource {
	CONST_VTBL __x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResourceVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource, 0x1c98c219, 0x0b13, 0x4240, 0x89, 0xa5, 0xd4, 0x95, 0xdc, 0x18, 0x9a, 0x00);
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate;
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidateVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Qualifiers)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsMatch)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsMatchAsDefault)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsDefault)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_ValueAsString)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetValueAsFileAsync)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetQualifierValue)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate *This, uint64_t a1, uint64_t a2);
	END_INTERFACE
} __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidateVtbl;

interface __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate {
	CONST_VTBL __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidateVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate, 0xaf5207d9, 0xc433, 0x4764, 0xb3, 0xfd, 0x8f, 0xa6, 0xbf, 0xbc, 0xba, 0xdc);
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2 __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2;
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *GetValueAsStreamAsync)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2Vtbl;

interface __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2 {
	CONST_VTBL __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2, 0x69e5b468, 0xf6fc, 0x4013, 0xaa, 0xa2, 0xd5, 0x3f, 0x17, 0x57, 0xd3, 0xb5);
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3 __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3;
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Kind)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3 *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3Vtbl;

interface __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3 {
	CONST_VTBL __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3, 0x08ae97f8, 0x517a, 0x4674, 0x95, 0x8c, 0x4a, 0x3c, 0x7c, 0xd2, 0xcc, 0x6b);
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager;
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_MainResourceMap)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_AllResourceMaps)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_DefaultContext)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *LoadPriFiles)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *UnloadPriFiles)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerVtbl;

interface __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager {
	CONST_VTBL __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager, 0xf744d97b, 0x9988, 0x44fb, 0xab, 0xd6, 0x53, 0x78, 0x84, 0x4c, 0xfa, 0x8b);
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2 __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2;
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2 *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2 *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2 *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2 *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2 *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2 *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *GetAllNamedResourcesForPackage)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2 *This, uint64_t a1, uint64_t a2, uint64_t a3);
	HRESULT (STDMETHODCALLTYPE *GetAllSubtreesForPackage)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2 *This, uint64_t a1, uint64_t a2, uint64_t a3);
	END_INTERFACE
} __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2Vtbl;

interface __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2 {
	CONST_VTBL __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2Vtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2, 0x9d66fe6c, 0xa4d7, 0x4c23, 0x9e, 0x85, 0x67, 0x5f, 0x30, 0x4c, 0x25, 0x2d);
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics;
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Current)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *IsResourceReference)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics *This, uint64_t a1, uint64_t a2);
	END_INTERFACE
} __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStaticsVtbl;

interface __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics {
	CONST_VTBL __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStaticsVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics, 0x1cc0fdfc, 0x69ee, 0x4e43, 0x99, 0x01, 0x47, 0xf1, 0x26, 0x87, 0xba, 0xf7);
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap;
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMapVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Uri)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *GetValue)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *GetValue2)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap *This, uint64_t a1, uint64_t a2, uint64_t a3);
	HRESULT (STDMETHODCALLTYPE *GetSubtree)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap *This, uint64_t a1, uint64_t a2);
	END_INTERFACE
} __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMapVtbl;

interface __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap {
	CONST_VTBL __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMapVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap, 0x72284824, 0xdb8c, 0x42f8, 0xb0, 0x8c, 0x53, 0xff, 0x35, 0x7d, 0xad, 0x82);
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifier __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifier;
typedef struct __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifierVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifier *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifier *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifier *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifier *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifier *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifier *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_QualifierName)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifier *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_QualifierValue)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifier *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsDefault)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifier *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_IsMatch)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifier *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_Score)(__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifier *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifierVtbl;

interface __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifier {
	CONST_VTBL __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifierVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceQualifier, 0x785da5b2, 0x4afd, 0x4376, 0xa8, 0x88, 0xc5, 0xf9, 0xa6, 0xb7, 0xa0, 0x5c);
