/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The Windows.Graphics.Imaging interface this component serves, in the shape scripts/generate-winrt-header.py's
 * committed headers take: vtable order and interface identifier hand-extracted from the windows-rs metadata that
 * generator pins (revision 58217a69, crates/libs/windows/src/Windows/Graphics/Imaging/mod.rs, which the generator
 * does not declare). The BitmapTransform class's default interface is the one a guest's activation query names, and
 * the members' order is the vtable order a guest binary is compiled against. */
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <stdint.h>
#include <asyncinfo.h>

/* Windows.Graphics.Imaging.BitmapBounds, as the metadata's struct(...;u4;u4;u4;u4) gives it. */
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CBitmapBounds {
	UINT32 X;
	UINT32 Y;
	UINT32 Width;
	UINT32 Height;
} __x_ABI_CWindows_CGraphics_CImaging_CBitmapBounds;

typedef struct __x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform __x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform;
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransformVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_ScaledWidth)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *put_ScaledWidth)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, UINT32 value);
	HRESULT (STDMETHODCALLTYPE *get_ScaledHeight)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *put_ScaledHeight)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, UINT32 value);
	HRESULT (STDMETHODCALLTYPE *get_InterpolationMode)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, int32_t *value);
	HRESULT (STDMETHODCALLTYPE *put_InterpolationMode)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, int32_t value);
	HRESULT (STDMETHODCALLTYPE *get_Flip)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, int32_t *value);
	HRESULT (STDMETHODCALLTYPE *put_Flip)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, int32_t value);
	HRESULT (STDMETHODCALLTYPE *get_Rotation)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, int32_t *value);
	HRESULT (STDMETHODCALLTYPE *put_Rotation)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, int32_t value);
	HRESULT (STDMETHODCALLTYPE *get_Bounds)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, __x_ABI_CWindows_CGraphics_CImaging_CBitmapBounds *value);
	HRESULT (STDMETHODCALLTYPE *put_Bounds)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform *This, __x_ABI_CWindows_CGraphics_CImaging_CBitmapBounds value);
	END_INTERFACE
} __x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransformVtbl;

interface __x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform {
	CONST_VTBL __x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransformVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform, 0xae755344, 0xe268, 0x4d35, 0xad, 0xcf, 0xe9, 0x95, 0xd3, 0x1a, 0x8d, 0x34);

/* Windows.Graphics.Imaging.IBitmapDecoderStatics, the statics face the BitmapDecoder activation names: the codecs'
 * identifiers, the codec-information enumeration and the stream decoders it creates. Vtable order and identifier
 * hand-extracted from the same pinned windows-rs metadata (crates/libs/windows/src/Windows/Graphics/Imaging/mod.rs,
 * define_interface!(IBitmapDecoderStatics, ..., 0x438ccb26_bcef_4e95_bad6_23a822e58d01)). */
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics;
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStaticsVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This, REFIID riid, void **object);

	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This);

	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This);

	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This, ULONG *count, IID **iids);

	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This, HSTRING *name);

	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This, TrustLevel *level);

	HRESULT (STDMETHODCALLTYPE *get_BmpDecoderId)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This, GUID *value);
	HRESULT (STDMETHODCALLTYPE *get_JpegDecoderId)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This, GUID *value);
	HRESULT (STDMETHODCALLTYPE *get_PngDecoderId)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This, GUID *value);
	HRESULT (STDMETHODCALLTYPE *get_TiffDecoderId)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This, GUID *value);
	HRESULT (STDMETHODCALLTYPE *get_GifDecoderId)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This, GUID *value);
	HRESULT (STDMETHODCALLTYPE *get_JpegXRDecoderId)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This, GUID *value);
	HRESULT (STDMETHODCALLTYPE *get_IcoDecoderId)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This, GUID *value);
	HRESULT (STDMETHODCALLTYPE *GetDecoderInformationEnumerator)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *CreateAsync)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This, IUnknown *stream, IInspectable **operation);
	HRESULT (STDMETHODCALLTYPE *CreateWithIdAsync)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics *This, GUID decoder_id, IUnknown *stream, IInspectable **operation);
	END_INTERFACE
} __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStaticsVtbl;

interface __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics {
	CONST_VTBL __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStaticsVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics, 0x438ccb26, 0xbcef, 0x4e95, 0xba, 0xd6, 0x23, 0xa8, 0x22, 0xe5, 0x8d, 0x01);

/* ---- The instance faces of the decode path, hand-extracted from the same pinned windows-rs metadata
 * (crates/libs/windows/src/Windows/Graphics/Imaging/mod.rs): IBitmapDecoder (0xacef22ba), IBitmapFrame
 * (0x72a49a1c), IBitmapFrameWithSoftwareBitmap (0xfe287c9a), IBitmapCodecInformation (0x400caaf2),
 * IPixelDataProvider (0xdd831f25) and ISoftwareBitmap (0x689e0708). The enumerations and the parameterized
 * IAsyncOperation`1 instances follow the Windows signature algorithm raven's own generator
 * (scripts/generate-winrt-header.py) computes: SHA-1 over the WinRT namespace bytes and the type's signature,
 * with the RFC 4122 version and variant bits set. ---- */

typedef enum __x_ABI_CWindows_CGraphics_CImaging_CBitmapPixelFormat {
	BitmapPixelFormat_Unknown = 0,
	BitmapPixelFormat_Rgba16 = 12,
	BitmapPixelFormat_Rgba8 = 30,
	BitmapPixelFormat_Rgb24 = 31,
	BitmapPixelFormat_Gray16 = 57,
	BitmapPixelFormat_Gray8 = 62,
	BitmapPixelFormat_Bgra8 = 87,
	BitmapPixelFormat_Nv12 = 103,
	BitmapPixelFormat_P010 = 104,
	BitmapPixelFormat_Yuy2 = 107
} __x_ABI_CWindows_CGraphics_CImaging_CBitmapPixelFormat;

typedef enum __x_ABI_CWindows_CGraphics_CImaging_CBitmapAlphaMode {
	BitmapAlphaMode_Premultiplied = 0,
	BitmapAlphaMode_Straight = 1,
	BitmapAlphaMode_Ignore = 2
} __x_ABI_CWindows_CGraphics_CImaging_CBitmapAlphaMode;

typedef enum __x_ABI_CWindows_CGraphics_CImaging_CExifOrientationMode {
	ExifOrientationMode_IgnoreExifOrientation = 0,
	ExifOrientationMode_RespectExifOrientation = 1
} __x_ABI_CWindows_CGraphics_CImaging_CExifOrientationMode;

typedef enum __x_ABI_CWindows_CGraphics_CImaging_CColorManagementMode {
	ColorManagementMode_DoNotColorManage = 0,
	ColorManagementMode_ColorManageToSRgb = 1
} __x_ABI_CWindows_CGraphics_CImaging_CColorManagementMode;

/* IBitmapCodecInformation: what the codec-information enumeration's elements answer. */
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation __x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation;
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformationVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_CodecId)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation *This, GUID *value);
	HRESULT (STDMETHODCALLTYPE *get_FileExtensions)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *get_FriendlyName)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation *This, HSTRING *value);
	HRESULT (STDMETHODCALLTYPE *get_MimeTypes)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation *This, IInspectable **value);
	END_INTERFACE
} __x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformationVtbl;

interface __x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation {
	CONST_VTBL __x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformationVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation, 0x400caaf2, 0xc4b0, 0x4392, 0xa3, 0xb0, 0x6f, 0x6f, 0x9b, 0xa9, 0x5c, 0xb4);

/* IBitmapDecoder: the stream decoder's instance face. */
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder;
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_BitmapContainerProperties)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *get_DecoderInformation)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *get_FrameCount)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *GetPreviewAsync)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder *This, IUnknown **operation);
	HRESULT (STDMETHODCALLTYPE *GetFrameAsync)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder *This, UINT32 frame_index, IUnknown **operation);
	END_INTERFACE
} __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderVtbl;

interface __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder {
	CONST_VTBL __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder, 0xacef22ba, 0x1d74, 0x4c91, 0x9d, 0xfc, 0x96, 0x20, 0x74, 0x52, 0x33, 0xe6);

/* IBitmapFrame: one decoded frame, in the vtable order the projection pins. */
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame;
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetThumbnailAsync)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, IUnknown **operation);
	HRESULT (STDMETHODCALLTYPE *get_BitmapProperties)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *get_BitmapPixelFormat)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, int32_t *value);
	HRESULT (STDMETHODCALLTYPE *get_BitmapAlphaMode)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, int32_t *value);
	HRESULT (STDMETHODCALLTYPE *get_DpiX)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, double *value);
	HRESULT (STDMETHODCALLTYPE *get_DpiY)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, double *value);
	HRESULT (STDMETHODCALLTYPE *get_PixelWidth)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *get_PixelHeight)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *get_OrientedPixelWidth)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *get_OrientedPixelHeight)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *GetPixelDataAsync)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, IUnknown **operation);
	HRESULT (STDMETHODCALLTYPE *GetPixelDataTransformedAsync)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame *This, int32_t pixel_format, int32_t alpha_mode,
		IUnknown *transform, int32_t exif_orientation_mode, int32_t color_management_mode, IUnknown **operation);
	END_INTERFACE
} __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameVtbl;

interface __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame {
	CONST_VTBL __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame, 0x72a49a1c, 0x8081, 0x438d, 0x91, 0xbc, 0x94, 0xec, 0xfc, 0x81, 0x85, 0xc6);

/* IBitmapFrameWithSoftwareBitmap: the SoftwareBitmap-producing additions the BitmapFrame class also serves. */
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap;
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmapVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetSoftwareBitmapAsync)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap *This, IUnknown **operation);
	HRESULT (STDMETHODCALLTYPE *GetSoftwareBitmapConvertedAsync)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap *This, int32_t pixel_format, int32_t alpha_mode, IUnknown **operation);
	HRESULT (STDMETHODCALLTYPE *GetSoftwareBitmapTransformedAsync)(__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap *This, int32_t pixel_format, int32_t alpha_mode,
		IUnknown *transform, int32_t exif_orientation_mode, int32_t color_management_mode, IUnknown **operation);
	END_INTERFACE
} __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmapVtbl;

interface __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap {
	CONST_VTBL __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmapVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap, 0xfe287c9a, 0x420c, 0x4963, 0x87, 0xad, 0x69, 0x14, 0x36, 0xe0, 0x83, 0x83);

/* IPixelDataProvider: the pixel bytes GetPixelDataAsync hands over, detached on demand. */
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProvider __x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProvider;
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProviderVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProvider *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProvider *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProvider *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProvider *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProvider *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProvider *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *DetachPixelData)(__x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProvider *This, UINT32 *length, BYTE **value);
	END_INTERFACE
} __x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProviderVtbl;

interface __x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProvider {
	CONST_VTBL __x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProviderVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProvider, 0xdd831f25, 0x185c, 0x4595, 0x9f, 0xb9, 0xcc, 0xbe, 0x6e, 0xc1, 0x8a, 0x6f);

/* ISoftwareBitmap: the bitmap object GetSoftwareBitmapAsync produces. */
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap __x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap;
typedef struct __x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmapVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This);
	ULONG (STDMETHODCALLTYPE *Release)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_BitmapPixelFormat)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, int32_t *value);
	HRESULT (STDMETHODCALLTYPE *get_BitmapAlphaMode)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, int32_t *value);
	HRESULT (STDMETHODCALLTYPE *get_PixelWidth)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, int32_t *value);
	HRESULT (STDMETHODCALLTYPE *get_PixelHeight)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, int32_t *value);
	HRESULT (STDMETHODCALLTYPE *get_IsReadOnly)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, boolean *value);
	HRESULT (STDMETHODCALLTYPE *put_DpiX)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, double value);
	HRESULT (STDMETHODCALLTYPE *get_DpiX)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, double *value);
	HRESULT (STDMETHODCALLTYPE *put_DpiY)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, double value);
	HRESULT (STDMETHODCALLTYPE *get_DpiY)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, double *value);
	HRESULT (STDMETHODCALLTYPE *LockBuffer)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, int32_t access_mode, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *CopyTo)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, __x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *other);
	HRESULT (STDMETHODCALLTYPE *CopyFromBuffer)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, IInspectable *buffer);
	HRESULT (STDMETHODCALLTYPE *CopyToBuffer)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, IInspectable *buffer);
	HRESULT (STDMETHODCALLTYPE *GetReadOnlyView)(__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap *This, IInspectable **value);
	END_INTERFACE
} __x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmapVtbl;

interface __x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap {
	CONST_VTBL __x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmapVtbl *lpVtbl;
};

DEFINE_GUID(IID__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap, 0x689e0708, 0x7eef, 0x483f, 0x96, 0x3f, 0xda, 0x93, 0x88, 0x18, 0xe0, 0x73);


/* ---- The parameterized IAsyncOperation`1 faces the decode path's async members answer, stamped from one shape.
 * The identifiers are the signature computation the header comment names, over the runtime classes' default
 * interfaces: BitmapDecoder 8eb6ba3f-1df3-59ad-987b-63c85923af3d, BitmapFrame 78375865-3078-52a9-9490-
 * 4426c44d6e52, PixelDataProvider 512f88d3-7570-5a10-885c-18b0917073ea, SoftwareBitmap 1a30d2f6-ffa7-5e48-
 * a034-7625fdda3d1d. The completed handlers are delegate objects the guest owns: the operation invokes and
 * releases them and no face of raven ever queries one, so only the shape matters. ---- */

#define RAVEN_IMAGING_ASYNC_OPERATION(name, result_type) \
	typedef struct __FIAsyncOperationCompletedHandler_1_##name __FIAsyncOperationCompletedHandler_1_##name; \
	typedef struct __FIAsyncOperation_1_##name __FIAsyncOperation_1_##name; \
	typedef struct __FIAsyncOperation_1_##name##Vtbl { \
		BEGIN_INTERFACE \
		HRESULT (STDMETHODCALLTYPE *QueryInterface)(__FIAsyncOperation_1_##name *This, REFIID riid, void **object); \
		ULONG (STDMETHODCALLTYPE *AddRef)(__FIAsyncOperation_1_##name *This); \
		ULONG (STDMETHODCALLTYPE *Release)(__FIAsyncOperation_1_##name *This); \
		HRESULT (STDMETHODCALLTYPE *GetIids)(__FIAsyncOperation_1_##name *This, ULONG *count, IID **iids); \
		HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__FIAsyncOperation_1_##name *This, HSTRING *type_name); \
		HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__FIAsyncOperation_1_##name *This, TrustLevel *level); \
		HRESULT (STDMETHODCALLTYPE *put_Completed)(__FIAsyncOperation_1_##name *This, __FIAsyncOperationCompletedHandler_1_##name *handler); \
		HRESULT (STDMETHODCALLTYPE *get_Completed)(__FIAsyncOperation_1_##name *This, __FIAsyncOperationCompletedHandler_1_##name **handler); \
		HRESULT (STDMETHODCALLTYPE *GetResults)(__FIAsyncOperation_1_##name *This, result_type results); \
		END_INTERFACE \
	} __FIAsyncOperation_1_##name##Vtbl; \
	interface __FIAsyncOperation_1_##name { \
		CONST_VTBL __FIAsyncOperation_1_##name##Vtbl *lpVtbl; \
	}; \
	typedef struct __FIAsyncOperationCompletedHandler_1_##name##Vtbl { \
		HRESULT (STDMETHODCALLTYPE *QueryInterface)(__FIAsyncOperationCompletedHandler_1_##name *This, REFIID riid, void **object); \
		ULONG (STDMETHODCALLTYPE *AddRef)(__FIAsyncOperationCompletedHandler_1_##name *This); \
		ULONG (STDMETHODCALLTYPE *Release)(__FIAsyncOperationCompletedHandler_1_##name *This); \
		HRESULT (STDMETHODCALLTYPE *Invoke)(__FIAsyncOperationCompletedHandler_1_##name *This, __FIAsyncOperation_1_##name *operation, AsyncStatus status); \
	} __FIAsyncOperationCompletedHandler_1_##name##Vtbl; \
	struct __FIAsyncOperationCompletedHandler_1_##name { \
		CONST_VTBL __FIAsyncOperationCompletedHandler_1_##name##Vtbl *lpVtbl; \
	}; \

RAVEN_IMAGING_ASYNC_OPERATION(Windows__CGraphics__CImaging__CBitmapDecoder, __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder **)
RAVEN_IMAGING_ASYNC_OPERATION(Windows__CGraphics__CImaging__CBitmapFrame, __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame **)
RAVEN_IMAGING_ASYNC_OPERATION(Windows__CGraphics__CImaging__CPixelDataProvider, __x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProvider **)
RAVEN_IMAGING_ASYNC_OPERATION(Windows__CGraphics__CImaging__CSoftwareBitmap, __x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap **)

DEFINE_GUID(IID___FIAsyncOperation_1_Windows__CGraphics__CImaging__CBitmapDecoder, 0x8eb6ba3f, 0x1df3, 0x59ad, 0x98, 0x7b, 0x63, 0xc8, 0x59, 0x23, 0xaf, 0x3d);
DEFINE_GUID(IID___FIAsyncOperation_1_Windows__CGraphics__CImaging__CBitmapFrame, 0x78375865, 0x3078, 0x52a9, 0x94, 0x90, 0x44, 0x26, 0xc4, 0x4d, 0x6e, 0x52);
DEFINE_GUID(IID___FIAsyncOperation_1_Windows__CGraphics__CImaging__CPixelDataProvider, 0x512f88d3, 0x7570, 0x5a10, 0x88, 0x5c, 0x18, 0xb0, 0x91, 0x70, 0x73, 0xea);
DEFINE_GUID(IID___FIAsyncOperation_1_Windows__CGraphics__CImaging__CSoftwareBitmap, 0x1a30d2f6, 0xffa7, 0x5e48, 0xa0, 0x34, 0x76, 0x25, 0xfd, 0xda, 0x3d, 0x1d);
