/* Generated from Windows.Storage.winmd (SHA-256 21f4e900ef68c7d40ce64af23277adcb8297b2e77514bcfd848fc81fa163b345) by scripts/generate-winrt-header.py. Do not edit. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CStorage_CIPathIOStatics __x_ABI_CWindows_CStorage_CIPathIOStatics;
typedef struct __x_ABI_CWindows_CStorage_CIPathIOStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *ReadTextAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *ReadTextAsync2)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, uint64_t a1, uint64_t a2, uint64_t a3);
	HRESULT (STDMETHODCALLTYPE *WriteTextAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, uint64_t a1, uint64_t a2, uint64_t a3);
	HRESULT (STDMETHODCALLTYPE *WriteTextAsync2)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);
	HRESULT (STDMETHODCALLTYPE *AppendTextAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, uint64_t a1, uint64_t a2, uint64_t a3);
	HRESULT (STDMETHODCALLTYPE *AppendTextAsync2)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);
	HRESULT (STDMETHODCALLTYPE *ReadLinesAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *ReadLinesAsync2)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, uint64_t a1, uint64_t a2, uint64_t a3);
	HRESULT (STDMETHODCALLTYPE *WriteLinesAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, uint64_t a1, uint64_t a2, uint64_t a3);
	HRESULT (STDMETHODCALLTYPE *WriteLinesAsync2)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);
	HRESULT (STDMETHODCALLTYPE *AppendLinesAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, uint64_t a1, uint64_t a2, uint64_t a3);
	HRESULT (STDMETHODCALLTYPE *AppendLinesAsync2)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);
	HRESULT (STDMETHODCALLTYPE *ReadBufferAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, uint64_t a1, uint64_t a2);
	HRESULT (STDMETHODCALLTYPE *WriteBufferAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, uint64_t a1, uint64_t a2, uint64_t a3);
	HRESULT (STDMETHODCALLTYPE *WriteBytesAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, uint64_t a1, uint64_t a2, uint64_t a3);
	END_INTERFACE
} __x_ABI_CWindows_CStorage_CIPathIOStaticsVtbl;

interface __x_ABI_CWindows_CStorage_CIPathIOStatics {
	CONST_VTBL __x_ABI_CWindows_CStorage_CIPathIOStaticsVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CStorage_CIPathIOStatics, 0x0f2f3758, 0x8ec7, 0x4381, 0x92, 0x2b, 0x8f, 0x6c, 0x07, 0xd2, 0x88, 0xf3);
