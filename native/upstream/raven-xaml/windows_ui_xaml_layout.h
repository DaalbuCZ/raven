/* Generated from Windows.UI.Xaml.winmd (SHA-256 c55875b03b9829c2e6ed6ad8b2d1ba33bd80c31208ba3bcda1a12ac46eddc41d) by scripts/generate-winrt-header.py. Do not edit.
 *
 * The generator passes word-sized slots; a GridLength is a 16-byte struct the ABI hands over by value (a pointer to a temporary
 * on x64, four stack words on x86), so the Height and Width slots are the hand declarations the generator's rules call for: the
 * compiler applies the per-architecture passing the way mingw's own headers do for Vector2 and Vector3. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>

typedef enum __x_ABI_CWindows_CUI_CXaml_CGridUnitType {
	GridUnitType_Auto = 0,
	GridUnitType_Pixel = 1,
	GridUnitType_Star = 2
} __x_ABI_CWindows_CUI_CXaml_CGridUnitType;

typedef struct __x_ABI_CWindows_CUI_CXaml_CGridLength {
	double Value;
	__x_ABI_CWindows_CUI_CXaml_CGridUnitType GridUnitType;
} __x_ABI_CWindows_CUI_CXaml_CGridLength;

typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition __x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinitionVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Height)(__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition *This, __x_ABI_CWindows_CUI_CXaml_CGridLength *value);
	HRESULT (STDMETHODCALLTYPE *put_Height)(__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition *This, __x_ABI_CWindows_CUI_CXaml_CGridLength value);
	HRESULT (STDMETHODCALLTYPE *get_MaxHeight)(__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_MaxHeight)(__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_MinHeight)(__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_MinHeight)(__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_ActualHeight)(__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinitionVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinitionVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIRowDefinition, 0x4abae829, 0xd80c, 0x4a5e, 0xa4, 0x8c, 0xf8, 0xb3, 0xd3, 0xb6, 0x53, 0x3d);
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition __x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinitionVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_Width)(__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition *This, __x_ABI_CWindows_CUI_CXaml_CGridLength *value);
	HRESULT (STDMETHODCALLTYPE *put_Width)(__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition *This, __x_ABI_CWindows_CUI_CXaml_CGridLength value);
	HRESULT (STDMETHODCALLTYPE *get_MaxWidth)(__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_MaxWidth)(__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_MinWidth)(__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *put_MinWidth)(__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition *This, uint64_t a1);
	HRESULT (STDMETHODCALLTYPE *get_ActualWidth)(__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition *This, uint64_t a1);
	END_INTERFACE
} __x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinitionVtbl;

interface __x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition {
	CONST_VTBL __x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinitionVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CUI_CXaml_CControls_CIColumnDefinition, 0xf7f1b229, 0xf024, 0x467f, 0x97, 0x0a, 0x7e, 0x70, 0x56, 0x15, 0xdb, 0x7b);
