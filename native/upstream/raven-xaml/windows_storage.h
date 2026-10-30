/* Generated from Windows.Storage.winmd (SHA-256 21f4e900ef68c7d40ce64af23277adcb8297b2e77514bcfd848fc81fa163b345) by
 * scripts/generate-winrt-header.py. Do not edit.
 *
 * The generator passes word-sized slots; every IPathIOStatics parameter is a word (HSTRING handles, object and
 * operation pointers), so the slots below are the hand spellings of the same ABI: the exact signatures the winmd
 * declares, which the word slots compile to on x64 and preserve on x86. WriteBytesAsync carries a BYTE array, and a
 * Windows Runtime array crosses the ABI as a length and a pointer, one slot more than the generator's parameter count.
 * Include <windows.storage.h> (which brings windows.foundation.h) before this header. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef struct __x_ABI_CWindows_CStorage_CIPathIOStatics __x_ABI_CWindows_CStorage_CIPathIOStatics;
/* mingw declares no IAsyncOperation<IVectorView<HSTRING>> anywhere; the type only appears here as an out pointer the
 * stub answers E_NOTIMPL with, so the declaration stays opaque. */
typedef struct __FIAsyncOperation_1___FIVectorView_1_HSTRING __FIAsyncOperation_1___FIVectorView_1_HSTRING;
typedef struct __x_ABI_CWindows_CStorage_CIPathIOStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *ReadTextAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING absolutePath,
		__FIAsyncOperation_1_HSTRING **operation);
	HRESULT (STDMETHODCALLTYPE *ReadTextAsync2)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING absolutePath,
		__x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding encoding, __FIAsyncOperation_1_HSTRING **operation);
	HRESULT (STDMETHODCALLTYPE *WriteTextAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING absolutePath,
		HSTRING contents, __x_ABI_CWindows_CFoundation_CIAsyncAction **operation);
	HRESULT (STDMETHODCALLTYPE *WriteTextAsync2)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING absolutePath,
		HSTRING contents, __x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding encoding,
		__x_ABI_CWindows_CFoundation_CIAsyncAction **operation);
	HRESULT (STDMETHODCALLTYPE *AppendTextAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING absolutePath,
		HSTRING contents, __x_ABI_CWindows_CFoundation_CIAsyncAction **operation);
	HRESULT (STDMETHODCALLTYPE *AppendTextAsync2)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING absolutePath,
		HSTRING contents, __x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding encoding,
		__x_ABI_CWindows_CFoundation_CIAsyncAction **operation);
	HRESULT (STDMETHODCALLTYPE *ReadLinesAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING absolutePath,
		__FIAsyncOperation_1___FIVectorView_1_HSTRING **operation);
	HRESULT (STDMETHODCALLTYPE *ReadLinesAsync2)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING absolutePath,
		__x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding encoding, __FIAsyncOperation_1___FIVectorView_1_HSTRING **operation);
	HRESULT (STDMETHODCALLTYPE *WriteLinesAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING absolutePath,
		__FIIterable_1_HSTRING *lines, __x_ABI_CWindows_CFoundation_CIAsyncAction **operation);
	HRESULT (STDMETHODCALLTYPE *WriteLinesAsync2)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING absolutePath,
		__FIIterable_1_HSTRING *lines, __x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding encoding,
		__x_ABI_CWindows_CFoundation_CIAsyncAction **operation);
	HRESULT (STDMETHODCALLTYPE *AppendLinesAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING absolutePath,
		__FIIterable_1_HSTRING *lines, __x_ABI_CWindows_CFoundation_CIAsyncAction **operation);
	HRESULT (STDMETHODCALLTYPE *AppendLinesAsync2)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING absolutePath,
		__FIIterable_1_HSTRING *lines, __x_ABI_CWindows_CStorage_CStreams_CUnicodeEncoding encoding,
		__x_ABI_CWindows_CFoundation_CIAsyncAction **operation);
	HRESULT (STDMETHODCALLTYPE *ReadBufferAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING absolutePath,
		__FIAsyncOperation_1_Windows__CStorage__CStreams__CIBuffer **operation);
	HRESULT (STDMETHODCALLTYPE *WriteBufferAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING absolutePath,
		__x_ABI_CWindows_CStorage_CStreams_CIBuffer *buffer, __x_ABI_CWindows_CFoundation_CIAsyncAction **operation);
	HRESULT (STDMETHODCALLTYPE *WriteBytesAsync)(__x_ABI_CWindows_CStorage_CIPathIOStatics *This, HSTRING absolutePath,
		UINT32 value_count, const BYTE *value, __x_ABI_CWindows_CFoundation_CIAsyncAction **operation);
	END_INTERFACE
} __x_ABI_CWindows_CStorage_CIPathIOStaticsVtbl;

interface __x_ABI_CWindows_CStorage_CIPathIOStatics {
	CONST_VTBL __x_ABI_CWindows_CStorage_CIPathIOStaticsVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CStorage_CIPathIOStatics, 0x0f2f3758, 0x8ec7, 0x4381, 0x92, 0x2b, 0x8f, 0x6c, 0x07, 0xd2, 0x88, 0xf3);
