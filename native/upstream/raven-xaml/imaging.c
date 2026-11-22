/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.Graphics.Imaging.BitmapTransform and BitmapDecoder classes of the Windows
 * Runtime.
 *
 * A packaged application asks for the transform a bitmap decode or encode applies: the factory activates instances
 * (the activation query names IActivationFactory, as the launch trace shows), and each instance carries the transform
 * the guest stores in it — rotation, flip, interpolation, the crop bounds and the scaled size — answering its
 * properties from what was set, the way the real class keeps its options. Every member a guest reaches is named in
 * Raven's trace, so the next run tells which ones the application touches.
 *
 * BitmapDecoder the application reaches as its statics face (the activation query names IBitmapDecoderStatics): the
 * identifier of each codec's decoder, the codec-information enumeration and the stream decoders it creates. The
 * identifiers are the real ones — the WIC decoder classes the runtime's own answers name, as wincodec.h gives them.
 * The enumeration is honest in exactly one way — raven ships no codec registry, so the result is an empty set; the
 * stream decoders decode for real through the guest's windowscodecs.dll — raven's own component, whose factory parses
 * BMP, PNG, GIF, ICO, JPEG and TIFF into in-memory frames — and every member a guest reaches is named in Raven's
 * trace, so the next run tells which ones the application touches. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include "windows_graphics_imaging.h"

typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform transform1;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransformVtbl transform1Vtbl;
typedef __x_ABI_CWindows_CGraphics_CImaging_CBitmapBounds bitmap_bounds;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics decoder_statics1;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStaticsVtbl decoder_statics1Vtbl;

static const WCHAR class_name[] = L"Windows.Graphics.Imaging.BitmapTransform";
static const WCHAR decoder_class_name[] = L"Windows.Graphics.Imaging.BitmapDecoder";

/* The decoders' identifiers, as the runtime's own statics answer them: the WIC decoder classes, as wincodec.h gives
 * each CLSID. */
DEFINE_GUID(bmp_decoder_id, 0x6b462062, 0x7cbf, 0x400d, 0x9f, 0xdb, 0x81, 0x3d, 0xd1, 0x0f, 0x27, 0x78);
DEFINE_GUID(jpeg_decoder_id, 0x9456a480, 0xe88b, 0x43ea, 0x9e, 0x73, 0x0b, 0x2d, 0x9b, 0x71, 0xb1, 0xca);
DEFINE_GUID(png_decoder_id, 0x389ea17b, 0x5078, 0x4cde, 0xb6, 0xef, 0x25, 0xc1, 0x51, 0x75, 0xc7, 0x51);
DEFINE_GUID(tiff_decoder_id, 0xb54e85d9, 0xfe23, 0x499f, 0x8b, 0x88, 0x6a, 0xce, 0xa7, 0x13, 0x75, 0x2b);
DEFINE_GUID(gif_decoder_id, 0x381dda3c, 0x9ce9, 0x4834, 0xa2, 0x3e, 0x1f, 0x98, 0xf8, 0xfc, 0x52, 0xbe);
DEFINE_GUID(jpeg_xr_decoder_id, 0xa26cec36, 0x234c, 0x4950, 0xae, 0x16, 0xe3, 0x4a, 0xac, 0xe7, 0x1d, 0x0d);
DEFINE_GUID(ico_decoder_id, 0xc61bfcdf, 0x2e0f, 0x4aad, 0xa8, 0xd7, 0xe0, 0x6b, 0xaf, 0xeb, 0xcd, 0xfe);

/* The parameterized IVectorView`1<BitmapCodecInformation> face the enumerator's result is cast for: the RFC 4122
 * version-5 UUID (SHA-1) of the instance signature
 * "pinterface({bbe1fa4c-b0e3-4583-baef-1f1b2e483e56};rc(Windows.Graphics.Imaging.BitmapCodecInformation;
 * {400caaf2-c4b0-4392-a3b0-6f6f9ba95cb4}))" under the WinRT namespace 11f47ad5-7b73-42c0-abae-878b1e16adee, the same
 * computation that reproduces mingw's precomputed instance GUIDs. */
static const IID decoder_vector_parameterized_iid = {0x97dfde96, 0xff1d, 0x5aa1, {0x86, 0x3a, 0x90, 0x11, 0x6a, 0x31, 0xb8, 0x6b}};

/* Windows.Graphics.Imaging's enumerations, as the metadata gives them: rotation and flip share "none" as zero, the
 * interpolation modes run nearest-neighbour, linear, cubic, fant. */
enum { rotation_none = 0, rotation_clockwise_90 = 1, rotation_clockwise_180 = 2, rotation_clockwise_270 = 3 };
enum { flip_none = 0, flip_horizontal = 1, flip_vertical = 2 };
enum { interpolation_nearest_neighbor = 0, interpolation_linear = 1, interpolation_cubic = 2, interpolation_fant = 3 };

/* The one member-name trace ravenhost.dll carries to the host, as raven-shell's activation trace does; nothing is
 * emitted when the host is absent. */
static void trace_member(const char *member)
{
	static void (WINAPI *raven_trace)(const char *);
	static BOOL looked;
	char line[160] = "raven-xaml: BitmapTransform.";
	size_t at = 28, length;
	if (!looked) {
		raven_trace = (void (WINAPI *)(const char *))GetProcAddress(LoadLibraryW(L"ravenhost.dll"), "RavenTrace");
		looked = TRUE;
	}
	if (!raven_trace)
		return;
	for (length = 0; member[length] && at < ARRAYSIZE(line) - 1; ++length)
		line[at++] = member[length];
	line[at] = 0;
	raven_trace(line);
}

/* The same trace for the decoder's members. */
static void trace_decoder_member(const char *member)
{
	static void (WINAPI *raven_trace)(const char *);
	static BOOL looked;
	char line[160] = "raven-xaml: BitmapDecoder.";
	size_t at = 26, length;
	if (!looked) {
		raven_trace = (void (WINAPI *)(const char *))GetProcAddress(LoadLibraryW(L"ravenhost.dll"), "RavenTrace");
		looked = TRUE;
	}
	if (!raven_trace)
		return;
	for (length = 0; member[length] && at < ARRAYSIZE(line) - 1; ++length)
		line[at++] = member[length];
	line[at] = 0;
	raven_trace(line);
}

static HRESULT inspectable_iids(ULONG *count, IID **iids)
{
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}

static HRESULT inspectable_trust(TrustLevel *level)
{
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

static HRESULT runtime_class_name(HSTRING *name, const WCHAR *text)
{
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), name);
}

static int equal_iid(REFIID left, const IID *right)
{
	return left->Data1 == right->Data1 && left->Data2 == right->Data2 && left->Data3 == right->Data3 &&
		left->Data4[0] == right->Data4[0] && left->Data4[1] == right->Data4[1] &&
		left->Data4[2] == right->Data4[2] && left->Data4[3] == right->Data4[3] &&
		left->Data4[4] == right->Data4[4] && left->Data4[5] == right->Data4[5] &&
		left->Data4[6] == right->Data4[6] && left->Data4[7] == right->Data4[7];
}

/* The query, refcounting and release the transform objects share; each object's vtable sits first and its reference
 * count directly after, so the release frees the block the object opened. */
static HRESULT object_query(void *iface, REFIID iid, void **out, const IID *own, LONG *refs)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) && !equal_iid(iid, own))
		return E_NOINTERFACE;
	InterlockedIncrement(refs);
	*out = iface;
	return S_OK;
}

static ULONG object_addref(LONG *refs)
{
	return (ULONG)InterlockedIncrement(refs);
}

static ULONG object_release(LONG *refs, void *block)
{
	const LONG refs_left = InterlockedDecrement(refs);
	if (!refs_left) {
		HeapFree(GetProcessHeap(), 0, block);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs_left;
}

/* ---- The transform object: the options a guest stores, answered by the properties. ---- */

struct transform_object {
	const transform1Vtbl *lpVtbl;
	LONG refs;
	UINT32 scaled_width;
	UINT32 scaled_height;
	int32_t interpolation_mode;
	int32_t flip;
	int32_t rotation;
	bitmap_bounds bounds;
};

static HRESULT WINAPI transform_query(transform1 *iface, REFIID iid, void **out)
{
	return object_query(iface, iid, out, &IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform,
		&((struct transform_object *)iface)->refs);
}

static ULONG WINAPI transform_addref(transform1 *iface)
{
	return object_addref(&((struct transform_object *)iface)->refs);
}

static ULONG WINAPI transform_release(transform1 *iface)
{
	return object_release(&((struct transform_object *)iface)->refs, iface);
}

static HRESULT WINAPI transform_iids(transform1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI transform_name(transform1 *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, class_name);
}

static HRESULT WINAPI transform_trust(transform1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI transform_scaled_width_get(transform1 *iface, UINT32 *value)
{
	struct transform_object *self = (struct transform_object *)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_ScaledWidth");
	*value = self->scaled_width;
	return S_OK;
}

static HRESULT WINAPI transform_scaled_width_put(transform1 *iface, UINT32 value)
{
	struct transform_object *self = (struct transform_object *)iface;
	trace_member("put_ScaledWidth");
	self->scaled_width = value;
	return S_OK;
}

static HRESULT WINAPI transform_scaled_height_get(transform1 *iface, UINT32 *value)
{
	struct transform_object *self = (struct transform_object *)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_ScaledHeight");
	*value = self->scaled_height;
	return S_OK;
}

static HRESULT WINAPI transform_scaled_height_put(transform1 *iface, UINT32 value)
{
	struct transform_object *self = (struct transform_object *)iface;
	trace_member("put_ScaledHeight");
	self->scaled_height = value;
	return S_OK;
}

static HRESULT WINAPI transform_interpolation_get(transform1 *iface, int32_t *value)
{
	struct transform_object *self = (struct transform_object *)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_InterpolationMode");
	*value = self->interpolation_mode;
	return S_OK;
}

static HRESULT WINAPI transform_interpolation_put(transform1 *iface, int32_t value)
{
	struct transform_object *self = (struct transform_object *)iface;
	trace_member("put_InterpolationMode");
	self->interpolation_mode = value;
	return S_OK;
}

static HRESULT WINAPI transform_flip_get(transform1 *iface, int32_t *value)
{
	struct transform_object *self = (struct transform_object *)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_Flip");
	*value = self->flip;
	return S_OK;
}

static HRESULT WINAPI transform_flip_put(transform1 *iface, int32_t value)
{
	struct transform_object *self = (struct transform_object *)iface;
	trace_member("put_Flip");
	self->flip = value;
	return S_OK;
}

static HRESULT WINAPI transform_rotation_get(transform1 *iface, int32_t *value)
{
	struct transform_object *self = (struct transform_object *)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_Rotation");
	*value = self->rotation;
	return S_OK;
}

static HRESULT WINAPI transform_rotation_put(transform1 *iface, int32_t value)
{
	struct transform_object *self = (struct transform_object *)iface;
	trace_member("put_Rotation");
	self->rotation = value;
	return S_OK;
}

static HRESULT WINAPI transform_bounds_get(transform1 *iface, bitmap_bounds *value)
{
	struct transform_object *self = (struct transform_object *)iface;
	if (!value)
		return E_POINTER;
	trace_member("get_Bounds");
	*value = self->bounds;
	return S_OK;
}

static HRESULT WINAPI transform_bounds_put(transform1 *iface, bitmap_bounds value)
{
	struct transform_object *self = (struct transform_object *)iface;
	trace_member("put_Bounds");
	self->bounds = value;
	return S_OK;
}

static const transform1Vtbl transform_table = {
	.QueryInterface = transform_query,
	.AddRef = transform_addref,
	.Release = transform_release,
	.GetIids = transform_iids,
	.GetRuntimeClassName = transform_name,
	.GetTrustLevel = transform_trust,
	.get_ScaledWidth = transform_scaled_width_get,
	.put_ScaledWidth = transform_scaled_width_put,
	.get_ScaledHeight = transform_scaled_height_get,
	.put_ScaledHeight = transform_scaled_height_put,
	.get_InterpolationMode = transform_interpolation_get,
	.put_InterpolationMode = transform_interpolation_put,
	.get_Flip = transform_flip_get,
	.put_Flip = transform_flip_put,
	.get_Rotation = transform_rotation_get,
	.put_Rotation = transform_rotation_put,
	.get_Bounds = transform_bounds_get,
	.put_Bounds = transform_bounds_put,
};

/* The class the factory activates opens with the transform the real class starts from: no rotation, no flip, no
 * crop, linear interpolation and no scale, until the guest sets what it wants. */
static struct transform_object *create_transform(void)
{
	struct transform_object *object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *object);
	if (!object)
		return NULL;
	object->lpVtbl = &transform_table;
	object->refs = 1;
	object->interpolation_mode = interpolation_linear;
	InterlockedIncrement(&raven_xaml_objects);
	return object;
}

/* ---- The decode path. The pixel work is the guest's windowscodecs.dll — raven's own component, whose imaging
 * factory parses BMP, PNG, GIF, ICO, JPEG and TIFF into in-memory frames. BitmapDecoder.CreateAsync duplicates the
 * application stream's underlying file handle (the accessor storage.c serves), hands it to WIC's
 * CreateDecoderFromFileHandle, and wraps the decoder in the Windows.Runtime faces the metadata declares. ---- */

#include <wincodec.h>

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder decoder1;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderVtbl decoder1Vtbl;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame frame1;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameVtbl frame1Vtbl;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap frame_soft1;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmapVtbl frame_soft1Vtbl;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation codec_info1;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformationVtbl codec_info1Vtbl;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProvider pixel_provider1;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProviderVtbl pixel_provider1Vtbl;
typedef __x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap software1;
typedef __x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmapVtbl software1Vtbl;

typedef __FIAsyncOperation_1_Windows__CGraphics__CImaging__CBitmapDecoder decoder_op_i;
typedef __FIAsyncOperationCompletedHandler_1_Windows__CGraphics__CImaging__CBitmapDecoder decoder_done_i;
typedef __FIAsyncOperation_1_Windows__CGraphics__CImaging__CBitmapFrame frame_op_i;
typedef __FIAsyncOperationCompletedHandler_1_Windows__CGraphics__CImaging__CBitmapFrame frame_done_i;
typedef __FIAsyncOperation_1_Windows__CGraphics__CImaging__CPixelDataProvider pixel_op_i;
typedef __FIAsyncOperationCompletedHandler_1_Windows__CGraphics__CImaging__CPixelDataProvider pixel_done_i;
typedef __FIAsyncOperation_1_Windows__CGraphics__CImaging__CSoftwareBitmap software_op_i;
typedef __FIAsyncOperationCompletedHandler_1_Windows__CGraphics__CImaging__CSoftwareBitmap software_done_i;


static IWICImagingFactory *wic_factory(void)
{
	static IWICImagingFactory *factory;
	static BOOL looked;
	if (!looked) {
		HRESULT (WINAPI *create)(UINT, IWICImagingFactory **);
		HMODULE module;
		looked = TRUE;
		module = LoadLibraryW(L"windowscodecs.dll");
		if (!module) {
			raven_trace_note("raven-xaml: windowscodecs.dll absent\n");
			return NULL;
		}
		create = (HRESULT (WINAPI *)(UINT, IWICImagingFactory **))GetProcAddress(module, "WICCreateImagingFactory_Proxy");
		if (!create || FAILED(create(WINCODEC_SDK_VERSION, &factory)) || !factory)
			factory = NULL;
	}
	return factory;
}

/* The stream the application hands CreateAsync is raven's file-backed random-access stream: the accessor storage.c
 * serves duplicates its underlying handle, WIC's CreateDecoderFromFileHandle reads the whole file through it into
 * the decoder's in-memory frames, and the position the application's stream keeps is restored before the duplicate
 * closes. A stream that is not raven's file stream fails the decode, and the trace names it. */
static IWICBitmapDecoder *decode_stream(IUnknown *stream)
{
	IWICImagingFactory *factory = wic_factory();
	IWICBitmapDecoder *decoder = NULL;
	HANDLE handle;
	DWORD saved;
	HRESULT result;
	if (!factory || !stream)
		return NULL;
	if (FAILED(raven_xaml_storage_stream_handle(stream, &handle)) || handle == INVALID_HANDLE_VALUE)
		return NULL;
	saved = SetFilePointer(handle, 0, NULL, FILE_CURRENT);
	result = IWICImagingFactory_CreateDecoderFromFileHandle(factory, (ULONG_PTR)handle, NULL, WICDecodeMetadataCacheOnDemand, &decoder);
	SetFilePointer(handle, saved, NULL, FILE_BEGIN);
	CloseHandle(handle);
	if (FAILED(result)) {
		raven_trace_note("raven-xaml: WIC decoder refused the stream\n");
		return NULL;
	}
	return decoder;
}

#ifndef E_ILLEGAL_METHOD_CALL
#define E_ILLEGAL_METHOD_CALL ((HRESULT)0x8000000e)
#endif
#ifndef E_ILLEGAL_DELEGATE_ASSIGNMENT
#define E_ILLEGAL_DELEGATE_ASSIGNMENT ((HRESULT)0x80000018)
#endif
#ifndef RO_E_CLOSED
#define RO_E_CLOSED ((HRESULT)0x80000013)
#endif

/* The hand-declared interfaces have no COBJMACROS wrappers: addref, release and invoke through the vtable. The
 * completed handlers share the delegate shape, so one helper serves every operation's handler calls. */
static void imaging_release_iunknown(void *iface)
{
	((IUnknown *)iface)->lpVtbl->Release((IUnknown *)iface);
}

static void imaging_addref_iunknown(void *iface)
{
	((IUnknown *)iface)->lpVtbl->AddRef((IUnknown *)iface);
}

static void imaging_handler_release(void *handler)
{
	void **table = *(void ***)handler;
	((ULONG (WINAPI *)(void *))table[2])(handler);
}

static void imaging_handler_addref(void *handler)
{
	void **table = *(void ***)handler;
	((ULONG (WINAPI *)(void *))table[1])(handler);
}

static void imaging_handler_invoke(void *handler, void *operation, AsyncStatus status)
{
	void **table = *(void ***)handler;
	((HRESULT (WINAPI *)(void *, void *, AsyncStatus))table[3])(handler, operation, status);
}

/* ---- The async operation: the shape storage.c's completed operations take, instantiated for the four result types
 * the decode path's members answer. Each operation finishes before CreateAsync returns — WIC holds the whole decoded
 * image in memory — so the completed handler, if the guest set one, runs inside the creation call. ---- */

static void trace_query_refuse(const char *face, REFIID iid);

struct async_core {
	LONG refs;
	CRITICAL_SECTION lock;
	AsyncStatus status;
	HRESULT error;
	UINT32 id;
	BOOL closed, assigned;
	void *completed;
};

static LONG next_imaging_async_id;

#define IMAGING_ASYNC_LIFETIME(name, iface_type, own_iids, handler_type) \
	static HRESULT WINAPI name##_query(iface_type *iface, REFIID iid, void **out) \
	{ \
		struct name *self = (struct name *)iface; \
		UINT which; \
		if (!out) \
			return E_POINTER; \
		*out = NULL; \
		if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IAgileObject)) \
			*out = iface; \
		else if (equal_iid(iid, &IID_IWeakReferenceSource)) \
			*out = &self->weak; \
		else { \
			for (which = 0; which < 2 && !*out; ++which) \
				if (equal_iid(iid, &own_iids[which])) \
					*out = iface; \
			if (!*out) { \
				if (equal_iid(iid, &IID_IAsyncInfo)) \
					*out = &self->info; \
				else { \
					trace_query_refuse("op", iid); \
					return E_NOINTERFACE; \
				} \
			} \
		} \
		InterlockedIncrement(&self->core.refs); \
		return S_OK; \
	} \
	static ULONG WINAPI name##_addref(iface_type *iface) \
	{ \
		return (ULONG)InterlockedIncrement(&((struct name *)iface)->core.refs); \
	} \
	static ULONG WINAPI name##_release(iface_type *iface) \
	{ \
		struct name *self = (struct name *)iface; \
		const LONG refs = InterlockedDecrement(&self->core.refs); \
		if (!refs) { \
			if (self->core.completed) \
				imaging_handler_release(self->core.completed); \
			name##_free(self); \
			DeleteCriticalSection(&self->core.lock); \
			FREE(self); \
			InterlockedDecrement(&raven_xaml_objects); \
		} \
		return (ULONG)refs; \
	} \
	static HRESULT WINAPI name##_iids(iface_type *iface, ULONG *count, IID **iids) \
	{ \
		(void)iface; \
		return inspectable_iids(count, iids); \
	} \
	static HRESULT WINAPI name##_class(iface_type *iface, HSTRING *out) \
	{ \
		(void)iface; \
		return runtime_class_name(out, name##_type_name); \
	} \
	static HRESULT WINAPI name##_trust(iface_type *iface, TrustLevel *level) \
	{ \
		(void)iface; \
		return inspectable_trust(level); \
	} \
	static struct name *name##_owner(IAsyncInfo *info) \
	{ \
		return CONTAINING_RECORD(info, struct name, info); \
	} \
	static HRESULT WINAPI name##_info_query(IAsyncInfo *info, REFIID iid, void **out) \
	{ \
		return name##_query(&name##_owner(info)->iface, iid, out); \
	} \
	static ULONG WINAPI name##_info_addref(IAsyncInfo *info) \
	{ \
		return name##_addref(&name##_owner(info)->iface); \
	} \
	static ULONG WINAPI name##_info_release(IAsyncInfo *info) \
	{ \
		return name##_release(&name##_owner(info)->iface); \
	} \
	static HRESULT WINAPI name##_info_iids(IAsyncInfo *info, ULONG *count, IID **iids) \
	{ \
		(void)info; \
		return inspectable_iids(count, iids); \
	} \
	static HRESULT WINAPI name##_info_class(IAsyncInfo *info, HSTRING *out) \
	{ \
		(void)info; \
		return runtime_class_name(out, name##_type_name); \
	} \
	static HRESULT WINAPI name##_info_trust(IAsyncInfo *info, TrustLevel *level) \
	{ \
		(void)info; \
		return inspectable_trust(level); \
	} \
	static HRESULT WINAPI name##_info_id(IAsyncInfo *info, UINT32 *out) \
	{ \
		struct name *self = name##_owner(info); \
		HRESULT result; \
		if (!out) \
			return E_POINTER; \
		EnterCriticalSection(&self->core.lock); \
		result = self->core.closed ? RO_E_CLOSED : S_OK; \
		*out = self->core.id; \
		LeaveCriticalSection(&self->core.lock); \
		return result; \
	} \
	static HRESULT WINAPI name##_info_status(IAsyncInfo *info, AsyncStatus *out) \
	{ \
		struct name *self = name##_owner(info); \
		HRESULT result; \
		if (!out) \
			return E_POINTER; \
		EnterCriticalSection(&self->core.lock); \
		result = self->core.closed ? RO_E_CLOSED : S_OK; \
		*out = self->core.status; \
		LeaveCriticalSection(&self->core.lock); \
		return result; \
	} \
	static HRESULT WINAPI name##_info_error(IAsyncInfo *info, HRESULT *out) \
	{ \
		struct name *self = name##_owner(info); \
		HRESULT result; \
		if (!out) \
			return E_POINTER; \
		EnterCriticalSection(&self->core.lock); \
		result = self->core.closed ? RO_E_CLOSED : S_OK; \
		*out = self->core.error; \
		LeaveCriticalSection(&self->core.lock); \
		return result; \
	} \
	static HRESULT WINAPI name##_info_cancel(IAsyncInfo *info) \
	{ \
		(void)info; \
		return S_OK; \
	} \
	static HRESULT WINAPI name##_info_close(IAsyncInfo *info) \
	{ \
		struct name *self = name##_owner(info); \
		void *handler = NULL; \
		HRESULT result = S_OK; \
		EnterCriticalSection(&self->core.lock); \
		if (self->core.status == Started) result = E_ILLEGAL_METHOD_CALL; \
		else { \
			self->core.closed = TRUE; \
			handler = self->core.completed; \
			self->core.completed = NULL; \
		} \
		LeaveCriticalSection(&self->core.lock); \
		if (handler) \
			imaging_handler_release(handler); \
		return result; \
	} \
	static const IAsyncInfoVtbl name##_info_table = { \
		name##_info_query, name##_info_addref, name##_info_release, name##_info_iids, name##_info_class, \
		name##_info_trust, name##_info_id, name##_info_status, name##_info_error, name##_info_cancel, \
		name##_info_close, \
	};

#define IMAGING_ASYNC_COMPLETED(name, iface_type, handler_type) \
	static HRESULT WINAPI name##_put_completed(iface_type *iface, handler_type *handler) \
	{ \
		struct name *self = (struct name *)iface; \
		HRESULT result = S_OK; \
		AsyncStatus status; \
		EnterCriticalSection(&self->core.lock); \
		status = self->core.status; \
		if (self->core.closed) result = RO_E_CLOSED; \
		else if (self->core.assigned) result = E_ILLEGAL_DELEGATE_ASSIGNMENT; \
		else { \
			self->core.assigned = TRUE; \
			self->core.completed = handler; \
			if (handler) { \
				imaging_handler_addref(handler); \
				if (status != Started) imaging_handler_addref(handler); \
			} \
		} \
		LeaveCriticalSection(&self->core.lock); \
		if (SUCCEEDED(result) && handler && status != Started) { \
			imaging_handler_invoke(handler, iface, status); \
			imaging_handler_release(handler); \
		} \
		return result; \
	} \
	static HRESULT WINAPI name##_get_completed(iface_type *iface, handler_type **handler) \
	{ \
		struct name *self = (struct name *)iface; \
		HRESULT result = S_OK; \
		if (!handler) \
			return E_POINTER; \
		*handler = NULL; \
		EnterCriticalSection(&self->core.lock); \
		if (self->core.closed) result = RO_E_CLOSED; \
		else if ((*handler = (handler_type *)self->core.completed)) imaging_handler_addref(*handler); \
		LeaveCriticalSection(&self->core.lock); \
		return result; \
	}

#define IMAGING_ASYNC_FINISH(name, iface_type, handler_type) \
	static void name##_finish(iface_type *iface, HRESULT error, AsyncStatus status) \
	{ \
		struct name *self = (struct name *)iface; \
		handler_type *handler = NULL; \
		EnterCriticalSection(&self->core.lock); \
		if (self->core.status == Started) { \
			self->core.status = status; \
			self->core.error = error; \
			handler = (handler_type *)self->core.completed; \
			if (handler) imaging_handler_addref(handler); \
		} \
		LeaveCriticalSection(&self->core.lock); \
		if (handler) { \
			imaging_handler_invoke(handler, iface, status); \
			imaging_handler_release(handler); \
		} \
	}

#define IMAGING_ASYNC_NEW(name, iface_type) \
	static struct name *name##_new(void) \
	{ \
		struct name *self = ALLOC(sizeof *self); \
		if (!self) \
			return NULL; \
		self->iface.lpVtbl = (void *)&name##_table; \
		self->info.lpVtbl = (void *)&name##_info_table; \
		self->core.refs = 1; \
		self->core.status = Started; \
		self->core.error = S_OK; \
		self->core.id = (UINT32)InterlockedIncrement(&next_imaging_async_id); \
		InitializeCriticalSection(&self->core.lock); \
		raven_weak_source_init(&self->weak, self, &self->core.refs, (raven_weak_query_fn)name##_query); \
		InterlockedIncrement(&raven_xaml_objects); \
		return self; \
	}

/* The completed-handler faces: delegate objects the guest owns; raven never queries one, so only the shape matters. */

struct decoder_operation {
	decoder_op_i iface;
	IAsyncInfo info;
	struct async_core core;
	struct raven_weak_source weak;
	decoder1 *result;
};

static const WCHAR decoder_operation_type_name[] = L"Windows.Foundation.IAsyncOperation`1<Windows.Graphics.Imaging.BitmapDecoder>";

/* The parameterized operation identifiers: the real signature computation names the first, and mingw's
 * windows.foundation.collections.idl mis-states the IAsyncOperation template's own identifier, so guests built
 * against mingw's projection query the second. Both name the same face. */
static const IID decoder_op_iids[2] = {
	{0x8eb6ba3f, 0x1df3, 0x59ad, {0x98, 0x7b, 0x63, 0xc8, 0x59, 0x23, 0xaf, 0x3d}},
	{0xaa94d8e9, 0xcaef, 0x53f6, {0x82, 0x3d, 0x91, 0xb6, 0xe8, 0x34, 0x05, 0x10}},
};

static void decoder_operation_free(struct decoder_operation *self)
{
	raven_weak_source_disconnect(&self->weak);
	if (self->result)
		imaging_release_iunknown(self->result);
}

IMAGING_ASYNC_LIFETIME(decoder_operation, decoder_op_i, decoder_op_iids, decoder_done_i)
IMAGING_ASYNC_COMPLETED(decoder_operation, decoder_op_i, decoder_done_i)
IMAGING_ASYNC_FINISH(decoder_operation, decoder_op_i, decoder_done_i)

static HRESULT WINAPI decoder_operation_results(decoder_op_i *iface, decoder1 **results)
{
	struct decoder_operation *self = (struct decoder_operation *)iface;
	HRESULT result;
	if (!results)
		return E_POINTER;
	*results = NULL;
	EnterCriticalSection(&self->core.lock);
	result = self->core.closed ? RO_E_CLOSED : self->core.status == Started ? E_ILLEGAL_METHOD_CALL : self->core.error;
	LeaveCriticalSection(&self->core.lock);
	if (SUCCEEDED(result) && self->result) {
		*results = self->result;
		imaging_addref_iunknown(*results);
	}
	return result;
}

static const __FIAsyncOperation_1_Windows__CGraphics__CImaging__CBitmapDecoderVtbl decoder_operation_table = {
	.QueryInterface = decoder_operation_query,
	.AddRef = decoder_operation_addref,
	.Release = decoder_operation_release,
	.GetIids = decoder_operation_iids,
	.GetRuntimeClassName = decoder_operation_class,
	.GetTrustLevel = decoder_operation_trust,
	.put_Completed = decoder_operation_put_completed,
	.get_Completed = decoder_operation_get_completed,
	.GetResults = decoder_operation_results,
};

IMAGING_ASYNC_NEW(decoder_operation, decoder_op_i)

/* The creation ref of the result transfers to the operation; the operation hands the guest its own reference at
 * GetResults and releases the stored one when it dies. */
static void decoder_operation_ready(struct decoder_operation *self, decoder1 *result, HRESULT error)
{
	if (SUCCEEDED(error))
		self->result = result;
	decoder_operation_finish(&self->iface, error, SUCCEEDED(error) ? Completed : Error);
}

struct frame_operation {
	frame_op_i iface;
	IAsyncInfo info;
	struct async_core core;
	struct raven_weak_source weak;
	frame1 *result;
};

static const WCHAR frame_operation_type_name[] = L"Windows.Foundation.IAsyncOperation`1<Windows.Graphics.Imaging.BitmapFrame>";

static const IID frame_op_iids[2] = {
	{0x78375865, 0x3078, 0x52a9, {0x94, 0x90, 0x44, 0x26, 0xc4, 0x4d, 0x6e, 0x52}},
	{0xcb1483d1, 0x1464, 0x5bf9, {0x93, 0x46, 0xd5, 0x37, 0x73, 0x5d, 0xfb, 0xd6}},
};

static void frame_operation_free(struct frame_operation *self)
{
	raven_weak_source_disconnect(&self->weak);
	if (self->result)
		imaging_release_iunknown(self->result);
}

IMAGING_ASYNC_LIFETIME(frame_operation, frame_op_i, frame_op_iids, frame_done_i)
IMAGING_ASYNC_COMPLETED(frame_operation, frame_op_i, frame_done_i)
IMAGING_ASYNC_FINISH(frame_operation, frame_op_i, frame_done_i)

static HRESULT WINAPI frame_operation_results(frame_op_i *iface, frame1 **results)
{
	struct frame_operation *self = (struct frame_operation *)iface;
	HRESULT result;
	if (!results)
		return E_POINTER;
	*results = NULL;
	EnterCriticalSection(&self->core.lock);
	result = self->core.closed ? RO_E_CLOSED : self->core.status == Started ? E_ILLEGAL_METHOD_CALL : self->core.error;
	LeaveCriticalSection(&self->core.lock);
	if (SUCCEEDED(result) && self->result) {
		*results = self->result;
		imaging_addref_iunknown(*results);
	}
	return result;
}

static const __FIAsyncOperation_1_Windows__CGraphics__CImaging__CBitmapFrameVtbl frame_operation_table = {
	.QueryInterface = frame_operation_query,
	.AddRef = frame_operation_addref,
	.Release = frame_operation_release,
	.GetIids = frame_operation_iids,
	.GetRuntimeClassName = frame_operation_class,
	.GetTrustLevel = frame_operation_trust,
	.put_Completed = frame_operation_put_completed,
	.get_Completed = frame_operation_get_completed,
	.GetResults = frame_operation_results,
};

IMAGING_ASYNC_NEW(frame_operation, frame_op_i)

static void frame_operation_ready(struct frame_operation *self, frame1 *result, HRESULT error)
{
	if (SUCCEEDED(error))
		self->result = result;
	frame_operation_finish(&self->iface, error, SUCCEEDED(error) ? Completed : Error);
}

struct pixel_operation {
	pixel_op_i iface;
	IAsyncInfo info;
	struct async_core core;
	struct raven_weak_source weak;
	pixel_provider1 *result;
};

static const WCHAR pixel_operation_type_name[] = L"Windows.Foundation.IAsyncOperation`1<Windows.Graphics.Imaging.PixelDataProvider>";

static const IID pixel_op_iids[2] = {
	{0x512f88d3, 0x7570, 0x5a10, {0x88, 0x5c, 0x18, 0xb0, 0x91, 0x70, 0x73, 0xea}},
	{0x8c2dfeb0, 0x6c22, 0x5863, {0x88, 0xd8, 0x85, 0xc1, 0xfb, 0xc7, 0x56, 0x97}},
};

static void pixel_operation_free(struct pixel_operation *self)
{
	raven_weak_source_disconnect(&self->weak);
	if (self->result)
		imaging_release_iunknown(self->result);
}

IMAGING_ASYNC_LIFETIME(pixel_operation, pixel_op_i, pixel_op_iids, pixel_done_i)
IMAGING_ASYNC_COMPLETED(pixel_operation, pixel_op_i, pixel_done_i)
IMAGING_ASYNC_FINISH(pixel_operation, pixel_op_i, pixel_done_i)

static HRESULT WINAPI pixel_operation_results(pixel_op_i *iface, pixel_provider1 **results)
{
	struct pixel_operation *self = (struct pixel_operation *)iface;
	HRESULT result;
	if (!results)
		return E_POINTER;
	*results = NULL;
	EnterCriticalSection(&self->core.lock);
	result = self->core.closed ? RO_E_CLOSED : self->core.status == Started ? E_ILLEGAL_METHOD_CALL : self->core.error;
	LeaveCriticalSection(&self->core.lock);
	if (SUCCEEDED(result) && self->result) {
		*results = self->result;
		imaging_addref_iunknown(*results);
	}
	return result;
}

static const __FIAsyncOperation_1_Windows__CGraphics__CImaging__CPixelDataProviderVtbl pixel_operation_table = {
	.QueryInterface = pixel_operation_query,
	.AddRef = pixel_operation_addref,
	.Release = pixel_operation_release,
	.GetIids = pixel_operation_iids,
	.GetRuntimeClassName = pixel_operation_class,
	.GetTrustLevel = pixel_operation_trust,
	.put_Completed = pixel_operation_put_completed,
	.get_Completed = pixel_operation_get_completed,
	.GetResults = pixel_operation_results,
};

IMAGING_ASYNC_NEW(pixel_operation, pixel_op_i)

static void pixel_operation_ready(struct pixel_operation *self, pixel_provider1 *result, HRESULT error)
{
	if (SUCCEEDED(error))
		self->result = result;
	pixel_operation_finish(&self->iface, error, SUCCEEDED(error) ? Completed : Error);
}

struct software_operation {
	software_op_i iface;
	IAsyncInfo info;
	struct async_core core;
	struct raven_weak_source weak;
	software1 *result;
};

static const WCHAR software_operation_type_name[] = L"Windows.Foundation.IAsyncOperation`1<Windows.Graphics.Imaging.SoftwareBitmap>";

static const IID software_op_iids[2] = {
	{0x1a30d2f6, 0xffa7, 0x5e48, {0xa0, 0x34, 0x76, 0x25, 0xfd, 0xda, 0x3d, 0x1d}},
	{0xc4a10980, 0x714b, 0x5501, {0x8d, 0xa2, 0xdb, 0xda, 0xcc, 0xe7, 0x0f, 0x73}},
};

static void software_operation_free(struct software_operation *self)
{
	raven_weak_source_disconnect(&self->weak);
	if (self->result)
		imaging_release_iunknown(self->result);
}

IMAGING_ASYNC_LIFETIME(software_operation, software_op_i, software_op_iids, software_done_i)
IMAGING_ASYNC_COMPLETED(software_operation, software_op_i, software_done_i)
IMAGING_ASYNC_FINISH(software_operation, software_op_i, software_done_i)

static HRESULT WINAPI software_operation_results(software_op_i *iface, software1 **results)
{
	struct software_operation *self = (struct software_operation *)iface;
	HRESULT result;
	if (!results)
		return E_POINTER;
	*results = NULL;
	EnterCriticalSection(&self->core.lock);
	result = self->core.closed ? RO_E_CLOSED : self->core.status == Started ? E_ILLEGAL_METHOD_CALL : self->core.error;
	LeaveCriticalSection(&self->core.lock);
	if (SUCCEEDED(result) && self->result) {
		*results = self->result;
		imaging_addref_iunknown(*results);
	}
	return result;
}

static const __FIAsyncOperation_1_Windows__CGraphics__CImaging__CSoftwareBitmapVtbl software_operation_table = {
	.QueryInterface = software_operation_query,
	.AddRef = software_operation_addref,
	.Release = software_operation_release,
	.GetIids = software_operation_iids,
	.GetRuntimeClassName = software_operation_class,
	.GetTrustLevel = software_operation_trust,
	.put_Completed = software_operation_put_completed,
	.get_Completed = software_operation_get_completed,
	.GetResults = software_operation_results,
};

IMAGING_ASYNC_NEW(software_operation, software_op_i)

static void software_operation_ready(struct software_operation *self, software1 *result, HRESULT error)
{
	if (SUCCEEDED(error))
		self->result = result;
	software_operation_finish(&self->iface, error, SUCCEEDED(error) ? Completed : Error);
}

/* ---- The trace: every member a guest reaches names itself, one line per face. ---- */

static void trace_face_member(const char *face, const char *member)
{
	static void (WINAPI *raven_trace)(const char *);
	static BOOL looked;
	char line[160];
	size_t at = 0, length;
	if (!looked) {
		raven_trace = (void (WINAPI *)(const char *))GetProcAddress(LoadLibraryW(L"ravenhost.dll"), "RavenTrace");
		looked = TRUE;
	}
	if (!raven_trace)
		return;
	line[at++] = 'r'; line[at++] = 'a'; line[at++] = 'v'; line[at++] = 'e'; line[at++] = 'n';
	line[at++] = '-'; line[at++] = 'x'; line[at++] = 'a'; line[at++] = 'm'; line[at++] = 'l';
	line[at++] = ':'; line[at++] = ' ';
	for (length = 0; face[length] && at < ARRAYSIZE(line) - 2; ++length)
		line[at++] = face[length];
	line[at++] = '.';
	for (length = 0; member[length] && at < ARRAYSIZE(line) - 1; ++length)
		line[at++] = member[length];
	line[at] = 0;
	raven_trace(line);
}

static void trace_frame_member(const char *member) { trace_face_member("BitmapFrame", member); }
static void trace_software_member(const char *member) { trace_face_member("SoftwareBitmap", member); }
static void trace_provider_member(const char *member) { trace_face_member("PixelDataProvider", member); }
static void trace_codec_member(const char *member) { trace_face_member("BitmapCodecInformation", member); }

/* The decode path's failures: the member trace names the stage and the HRESULT the WIC layer answered, so the next
 * run tells what the decode refused. */
static void trace_decode_result(const char *what, HRESULT result)
{
	char line[96];
	wsprintfA(line, "%s hr=%08x", what, (unsigned int)result);
	trace_face_member("decode", line);
}

/* The interface identifier a query refused, in short form: the next run tells which face the guest wanted and the
 * object does not serve. */
static void trace_query_refuse(const char *face, REFIID iid)
{
	char line[96];
	wsprintfA(line, "refuse %08x-%04x-%04x", (unsigned int)iid->Data1, (unsigned int)iid->Data2, (unsigned int)iid->Data3);
	trace_face_member(face, line);
}

/* ---- The pixel format map: the metadata's BitmapPixelFormat values and the WIC pixel formats the decode answers.
 * Alpha-bearing formats report Straight alpha and opaque ones Ignore, the way a decoded frame's own storage reads;
 * premultiplied targets name WIC's P formats. ---- */

static int same_guid(REFGUID left, REFGUID right)
{
	return memcmp(left, right, sizeof(GUID)) == 0;
}

static int32_t pixel_format_from_wic(REFGUID format)
{
	if (same_guid(format, &GUID_WICPixelFormat32bppBGRA) || same_guid(format, &GUID_WICPixelFormat32bppPBGRA))
		return BitmapPixelFormat_Bgra8;
	if (same_guid(format, &GUID_WICPixelFormat32bppRGBA) || same_guid(format, &GUID_WICPixelFormat32bppPRGBA))
		return BitmapPixelFormat_Rgba8;
	if (same_guid(format, &GUID_WICPixelFormat24bppBGR))
		return BitmapPixelFormat_Rgb24;
	if (same_guid(format, &GUID_WICPixelFormat8bppGray))
		return BitmapPixelFormat_Gray8;
	return BitmapPixelFormat_Unknown;
}

static UINT pixel_format_bpp(REFGUID format)
{
	if (same_guid(format, &GUID_WICPixelFormat24bppBGR))
		return 3;
	if (same_guid(format, &GUID_WICPixelFormat8bppGray))
		return 1;
	return 4;
}

static int32_t alpha_mode_from_wic(REFGUID format)
{
	return pixel_format_bpp(format) == 4 && !same_guid(format, &GUID_WICPixelFormat24bppBGR)
		? BitmapAlphaMode_Straight : BitmapAlphaMode_Ignore;
}

static const GUID *alpha_target_format(int32_t pixel_format, int32_t alpha_mode)
{
	switch (pixel_format) {
	case BitmapPixelFormat_Bgra8:
		return alpha_mode == BitmapAlphaMode_Premultiplied ? &GUID_WICPixelFormat32bppPBGRA : &GUID_WICPixelFormat32bppBGRA;
	case BitmapPixelFormat_Rgba8:
		return alpha_mode == BitmapAlphaMode_Premultiplied ? &GUID_WICPixelFormat32bppPRGBA : &GUID_WICPixelFormat32bppRGBA;
	case BitmapPixelFormat_Rgb24:
		return &GUID_WICPixelFormat24bppBGR;
	case BitmapPixelFormat_Gray8:
		return &GUID_WICPixelFormat8bppGray;
	}
	return NULL;
}

static WICBitmapTransformOptions rotation_to_wic(int32_t rotation)
{
	switch (rotation) {
	case rotation_clockwise_90: return WICBitmapTransformRotate90;
	case rotation_clockwise_180: return WICBitmapTransformRotate180;
	case rotation_clockwise_270: return WICBitmapTransformRotate270;
	}
	return WICBitmapTransformRotate0;
}

static WICBitmapTransformOptions flip_to_wic(int32_t flip)
{
	if (flip == flip_horizontal)
		return WICBitmapTransformFlipHorizontal;
	if (flip == flip_vertical)
		return WICBitmapTransformFlipVertical;
	return WICBitmapTransformRotate0;
}

static WICBitmapInterpolationMode interpolation_to_wic(int32_t mode)
{
	switch (mode) {
	case interpolation_nearest_neighbor: return WICBitmapInterpolationModeNearestNeighbor;
	case interpolation_cubic: return WICBitmapInterpolationModeCubic;
	case interpolation_fant: return WICBitmapInterpolationModeFant;
	}
	return WICBitmapInterpolationModeLinear;
}

/* ---- The string view: the IVectorView`1<String> the codec information's file extensions and MIME types answer,
 * over the strings it is built from. ---- */

typedef __FIVectorView_1_HSTRING string_view_i;
typedef __FIVectorView_1_HSTRINGVtbl string_view_vtbl;

struct string_vector {
	const string_view_vtbl *lpVtbl;
	LONG refs;
	struct raven_weak_source weak;
	HSTRING *items;
	UINT32 count;
};

static HRESULT WINAPI string_vector_query(string_view_i *iface, REFIID iid, void **out)
{
	struct string_vector *self = (struct string_vector *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IAgileObject) ||
		equal_iid(iid, &IID___FIVectorView_1_HSTRING))
		*out = iface;
	else if (equal_iid(iid, &IID_IWeakReferenceSource))
		*out = &self->weak;
	else {
		trace_query_refuse("vector", iid);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI string_vector_addref(string_view_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct string_vector *)iface)->refs);
}

static ULONG WINAPI string_vector_release(string_view_i *iface)
{
	struct string_vector *self = (struct string_vector *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		UINT32 index;
		raven_weak_source_disconnect(&self->weak);
		for (index = 0; index < self->count; ++index)
			WindowsDeleteString(self->items[index]);
		FREE(self->items);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI string_vector_iids(string_view_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI string_vector_name(string_view_i *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, L"Windows.Foundation.Collections.IVectorView`1<String>");
}

static HRESULT WINAPI string_vector_trust(string_view_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI string_vector_get_at(string_view_i *iface, UINT32 index, HSTRING *value)
{
	struct string_vector *self = (struct string_vector *)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (index >= self->count)
		return E_BOUNDS;
	return WindowsDuplicateString(self->items[index], value);
}

static HRESULT WINAPI string_vector_get_size(string_view_i *iface, UINT32 *value)
{
	struct string_vector *self = (struct string_vector *)iface;
	if (!value)
		return E_POINTER;
	*value = self->count;
	return S_OK;
}

static HRESULT WINAPI string_vector_index_of(string_view_i *iface, HSTRING element, UINT32 *index, boolean *found)
{
	struct string_vector *self = (struct string_vector *)iface;
	UINT32 at;
	if (index)
		*index = 0;
	if (found)
		*found = FALSE;
	if (!index || !found)
		return E_POINTER;
	for (at = 0; at < self->count; ++at) {
		INT32 order = 0;
		if (SUCCEEDED(WindowsCompareStringOrdinal(self->items[at], element, &order)) && order == 0) {
			*index = at;
			*found = TRUE;
			return S_OK;
		}
	}
	return S_OK;
}

static HRESULT WINAPI string_vector_get_many(string_view_i *iface, UINT32 start, UINT32 items_size, HSTRING *items,
	UINT32 *value)
{
	struct string_vector *self = (struct string_vector *)iface;
	UINT32 copied = 0;
	HRESULT result = S_OK;
	if (!value)
		return E_POINTER;
	*value = 0;
	if (start > self->count)
		return E_BOUNDS;
	for (; copied < items_size && start + copied < self->count; ++copied)
		if (FAILED(result = WindowsDuplicateString(self->items[start + copied], &items[copied])))
			break;
	*value = copied;
	return result;
}

static const string_view_vtbl string_vector_table = {
	.QueryInterface = string_vector_query,
	.AddRef = string_vector_addref,
	.Release = string_vector_release,
	.GetIids = string_vector_iids,
	.GetRuntimeClassName = string_vector_name,
	.GetTrustLevel = string_vector_trust,
	.GetAt = string_vector_get_at,
	.get_Size = string_vector_get_size,
	.IndexOf = string_vector_index_of,
	.GetMany = string_vector_get_many,
};

static struct string_vector *string_vector_split(const WCHAR *text, UINT32 length)
{
	struct string_vector *self;
	UINT32 pieces = 1, at, piece, start;
	for (at = 0; at < length; ++at)
		if (text[at] == ';')
			++pieces;
	if (!(self = ALLOC(sizeof *self)) || !(self->items = ALLOC(pieces * sizeof *self->items))) {
		FREE(self);
		return NULL;
	}
	self->lpVtbl = &string_vector_table;
	self->refs = 1;
	raven_weak_source_init(&self->weak, self, &self->refs, (raven_weak_query_fn)string_vector_query);
	self->count = 0;
	start = 0;
	for (piece = 0; piece < pieces; ++piece) {
		UINT32 end = start;
		while (end < length && text[end] != ';')
			++end;
		if (end > start && SUCCEEDED(WindowsCreateString(text + start, end - start, &self->items[self->count])))
			++self->count;
		start = end + 1;
	}
	if (!self->count) {
		FREE(self->items);
		FREE(self);
		return NULL;
	}
	InterlockedIncrement(&raven_xaml_objects);
	return self;
}

/* ---- The codec information: what WIC's decoder info answers, in the metadata's shape. ---- */

struct codec_information {
	const codec_info1Vtbl *lpVtbl;
	LONG refs;
	struct raven_weak_source weak;
	GUID codec_id;
	WCHAR *friendly;
	UINT friendly_length;
	WCHAR *extensions;
	UINT extensions_length;
	WCHAR *mimes;
	UINT mimes_length;
};

static void codec_information_clear(struct codec_information *self)
{
	if (self->friendly) {
		FREE(self->friendly);
		self->friendly = NULL;
	}
	if (self->extensions) {
		FREE(self->extensions);
		self->extensions = NULL;
	}
	if (self->mimes) {
		FREE(self->mimes);
		self->mimes = NULL;
	}
}

static HRESULT WINAPI codec_information_query(codec_info1 *iface, REFIID iid, void **out)
{
	struct codec_information *self = (struct codec_information *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation))
		*out = iface;
	else if (equal_iid(iid, &IID_IWeakReferenceSource))
		*out = &self->weak;
	else {
		trace_query_refuse("BitmapCodecInformation", iid);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI codec_information_addref(codec_info1 *iface)
{
	return object_addref(&((struct codec_information *)iface)->refs);
}

static ULONG WINAPI codec_information_release(codec_info1 *iface)
{
	struct codec_information *self = (struct codec_information *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		raven_weak_source_disconnect(&self->weak);
		codec_information_clear(self);
		HeapFree(GetProcessHeap(), 0, iface);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI codec_information_iids(codec_info1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI codec_information_name(codec_info1 *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, L"Windows.Graphics.Imaging.BitmapCodecInformation");
}

static HRESULT WINAPI codec_information_trust(codec_info1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI codec_information_codec_id(codec_info1 *iface, GUID *value)
{
	struct codec_information *self = (struct codec_information *)iface;
	if (!value)
		return E_POINTER;
	*value = self->codec_id;
	return S_OK;
}

static HRESULT WINAPI codec_information_friendly(codec_info1 *iface, HSTRING *value)
{
	struct codec_information *self = (struct codec_information *)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	trace_codec_member("get_FriendlyName");
	return WindowsCreateString(self->friendly, self->friendly_length, value);
}

static HRESULT WINAPI codec_information_extensions(codec_info1 *iface, IInspectable **value)
{
	struct codec_information *self = (struct codec_information *)iface;
	struct string_vector *vector;
	if (!value)
		return E_POINTER;
	*value = NULL;
	trace_codec_member("get_FileExtensions");
	if (!(vector = string_vector_split(self->extensions, self->extensions_length)))
		return E_OUTOFMEMORY;
	*value = (IInspectable *)vector;
	return S_OK;
}

static HRESULT WINAPI codec_information_mimes(codec_info1 *iface, IInspectable **value)
{
	struct codec_information *self = (struct codec_information *)iface;
	struct string_vector *vector;
	if (!value)
		return E_POINTER;
	*value = NULL;
	trace_codec_member("get_MimeTypes");
	if (!(vector = string_vector_split(self->mimes, self->mimes_length)))
		return E_OUTOFMEMORY;
	*value = (IInspectable *)vector;
	return S_OK;
}

static const codec_info1Vtbl codec_information_table = {
	.QueryInterface = codec_information_query,
	.AddRef = codec_information_addref,
	.Release = codec_information_release,
	.GetIids = codec_information_iids,
	.GetRuntimeClassName = codec_information_name,
	.GetTrustLevel = codec_information_trust,
	.get_CodecId = codec_information_codec_id,
	.get_FileExtensions = codec_information_extensions,
	.get_FriendlyName = codec_information_friendly,
	.get_MimeTypes = codec_information_mimes,
};

/* WIC's info getters answer the string and its length through the two-call pattern. */
typedef HRESULT (WINAPI *wic_info_string_fn)(IWICBitmapDecoderInfo *, UINT, WCHAR *, UINT *);

static WCHAR *info_string(wic_info_string_fn getter, IWICBitmapDecoderInfo *info, UINT *length)
{
	UINT needed = 0;
	WCHAR *text;
	HRESULT first;
	if (FAILED(first = getter(info, 0, NULL, &needed)) || !needed)
		return NULL;
	if (!(text = ALLOC(needed * sizeof(WCHAR) + sizeof(WCHAR))))
		return NULL;
	if (FAILED(getter(info, needed, text, &needed))) {
		FREE(text);
		return NULL;
	}
	/* the length WIC answers includes the terminator the copy wrote */
	*length = needed ? needed - 1 : 0;
	return text;
}

static struct codec_information *create_codec_information(IWICBitmapDecoder *wic)
{
	struct codec_information *self;
	IWICBitmapDecoderInfo *info = NULL;
	if (!(self = ALLOC(sizeof *self)))
		return NULL;
	self->lpVtbl = &codec_information_table;
	self->refs = 1;
	raven_weak_source_init(&self->weak, self, &self->refs, (raven_weak_query_fn)codec_information_query);
	if (FAILED(IWICBitmapDecoder_GetDecoderInfo(wic, &info)) || !info) {
		FREE(self);
		return NULL;
	}
	IWICBitmapDecoderInfo_GetContainerFormat(info, &self->codec_id);
	self->friendly = info_string((wic_info_string_fn)info->lpVtbl->GetFriendlyName, info, &self->friendly_length);
	self->extensions = info_string((wic_info_string_fn)info->lpVtbl->GetFileExtensions, info, &self->extensions_length);
	self->mimes = info_string((wic_info_string_fn)info->lpVtbl->GetMimeTypes, info, &self->mimes_length);
	IWICBitmapDecoderInfo_Release(info);
	InterlockedIncrement(&raven_xaml_objects);
	return self;
}

/* ---- The frame: one decoded image, WIC's frame decode behind it. The object answers both faces the BitmapFrame
 * runtime class serves — IBitmapFrame and, at the same vtable dispatch, IBitmapFrameWithSoftwareBitmap. ---- */

struct frame_object {
	const frame1Vtbl *lpVtbl;
	struct { const frame_soft1Vtbl *lpVtbl; } soft;
	LONG refs;
	struct raven_weak_source weak;
	IWICBitmapFrameDecode *wic;
	UINT32 width, height;
	GUID format;
	double dpi_x, dpi_y;
};

static struct frame_object *frame_soft_owner(frame_soft1 *iface)
{
	return CONTAINING_RECORD(iface, struct frame_object, soft);
}

struct pixel_provider_object;
static struct pixel_provider_object *create_pixel_provider(BYTE *pixels, UINT32 length);
static software1 *create_software_bitmap(struct frame_object *frame, const transform1 *transform,
	int32_t pixel_format, int32_t alpha_mode);

static const transform1 *transform_face(IUnknown *unknown)
{
	transform1 *face = NULL;
	if (unknown && SUCCEEDED(unknown->lpVtbl->QueryInterface(unknown,
		&IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform, (void **)&face)))
		return face;
	return NULL;
}

/* The WIC chain the transform describes: the crop first, then the scale, then the rotation and the flip — each
 * stage a component the factory creates over the previous one — and finally the format the caller asked for. A
 * stage that WIC refuses is skipped: the answer stays honest in the dimensions the caller sees. */
static HRESULT transformed_source(struct frame_object *self, const transform1 *transform, const GUID *target_format,
	IWICBitmapSource **out)
{
	IWICImagingFactory *factory = wic_factory();
	IWICBitmapSource *source, *end;
	IWICBitmapClipper *clipper = NULL;
	IWICBitmapScaler *scaler = NULL;
	IWICBitmapFlipRotator *rotator = NULL;
	IWICBitmapFlipRotator *flipper = NULL;
	IWICFormatConverter *converter = NULL;
	UINT32 width = self->width, height = self->height;
	UINT32 scaled_width = 0, scaled_height = 0;
	bitmap_bounds bounds = {0, 0, 0, 0};
	int32_t interpolation = interpolation_linear, flip = flip_none, rotation = rotation_none;
	HRESULT result = S_OK;
	if (!factory)
		return E_NOTIMPL;
	if (transform) {
		if (FAILED(result = transform->lpVtbl->get_ScaledWidth(transform, &scaled_width))) return result;
		if (FAILED(result = transform->lpVtbl->get_ScaledHeight(transform, &scaled_height))) return result;
		if (FAILED(result = transform->lpVtbl->get_InterpolationMode(transform, &interpolation))) return result;
		if (FAILED(result = transform->lpVtbl->get_Flip(transform, &flip))) return result;
		if (FAILED(result = transform->lpVtbl->get_Rotation(transform, &rotation))) return result;
		if (FAILED(result = transform->lpVtbl->get_Bounds(transform, &bounds))) return result;
	}
	source = end = (IWICBitmapSource *)self->wic;
	if (bounds.Width && bounds.Height && SUCCEEDED(IWICImagingFactory_CreateBitmapClipper(factory, &clipper))) {
		WICRect rect = {(INT)bounds.X, (INT)bounds.Y, (INT)bounds.Width, (INT)bounds.Height};
		if (SUCCEEDED(IWICBitmapClipper_Initialize(clipper, source, &rect))) {
			end = (IWICBitmapSource *)clipper;
			IWICBitmapSource_GetSize(end, &width, &height);
		}
	}
	if ((scaled_width || scaled_height) && SUCCEEDED(IWICImagingFactory_CreateBitmapScaler(factory, &scaler))) {
		UINT32 target_width = scaled_width, target_height = scaled_height;
		if (!target_width && target_height && self->height)
			target_width = (UINT32)((UINT64)target_height * self->width / self->height);
		if (!target_height && target_width && self->width)
			target_height = (UINT32)((UINT64)target_width * self->height / self->width);
		if (target_width && target_height &&
			SUCCEEDED(IWICBitmapScaler_Initialize(scaler, end, target_width, target_height,
				interpolation_to_wic(interpolation)))) {
			end = (IWICBitmapSource *)scaler;
			width = target_width;
			height = target_height;
		}
	}
	if (rotation != rotation_none && SUCCEEDED(IWICImagingFactory_CreateBitmapFlipRotator(factory, &rotator))) {
		if (SUCCEEDED(IWICBitmapFlipRotator_Initialize(rotator, end, rotation_to_wic(rotation))))
			end = (IWICBitmapSource *)rotator;
	}
	if (flip != flip_none && SUCCEEDED(IWICImagingFactory_CreateBitmapFlipRotator(factory, &flipper))) {
		if (SUCCEEDED(IWICBitmapFlipRotator_Initialize(flipper, end, flip_to_wic(flip))))
			end = (IWICBitmapSource *)flipper;
	}
	if (target_format && !same_guid(target_format, &self->format) &&
		SUCCEEDED(IWICImagingFactory_CreateFormatConverter(factory, &converter))) {
		if (SUCCEEDED(IWICFormatConverter_Initialize(converter, end, target_format, WICBitmapDitherTypeNone, NULL,
			0.0, WICBitmapPaletteTypeCustom)))
			end = (IWICBitmapSource *)converter;
	}
	(void)width;
	(void)height;
	IWICBitmapSource_AddRef(end);
	if (clipper) IWICBitmapClipper_Release(clipper);
	if (scaler) IWICBitmapScaler_Release(scaler);
	if (rotator) IWICBitmapFlipRotator_Release(rotator);
	if (flipper) IWICBitmapFlipRotator_Release(flipper);
	if (converter) IWICFormatConverter_Release(converter);
	*out = end;
	return S_OK;
}

/* The pixels the GetPixelData family answers: the frame through the transform chain, copied into a buffer whose
 * stride the format's bytes per pixel decide. */
static HRESULT frame_pixel_data(struct frame_object *self, const transform1 *transform, int32_t pixel_format,
	int32_t alpha_mode, BYTE **out_pixels, UINT32 *out_length)
{
	IWICBitmapSource *source = NULL;
	const GUID *target;
	UINT bpp, stride, size, width, height;
	BYTE *pixels = NULL;
	HRESULT result;
	if (pixel_format == BitmapPixelFormat_Unknown && alpha_mode == BitmapAlphaMode_Ignore)
		target = &self->format;
	else if (!(target = alpha_target_format(pixel_format, alpha_mode)))
		target = &self->format;
	if (FAILED(result = transformed_source(self, transform, target, &source))) {
		trace_decode_result("transformed_source", result);
		return result;
	}
	bpp = pixel_format_bpp(target) * 8;
	IWICBitmapSource_GetSize(source, &width, &height);
	stride = ((width * bpp + 31) / 32) * 4;
	size = stride * height;
	if (size && (pixels = ALLOC(size)))
		result = IWICBitmapSource_CopyPixels(source, NULL, stride, size, pixels);
	else if (SUCCEEDED(result))
		result = E_OUTOFMEMORY;
	if (FAILED(result))
		trace_decode_result("CopyPixels", result);
	IWICBitmapSource_Release(source);
	if (SUCCEEDED(result)) {
		*out_pixels = pixels;
		*out_length = size;
	} else if (pixels) {
		FREE(pixels);
	}
	return result;
}

static HRESULT WINAPI frame_query(frame1 *iface, REFIID iid, void **out)
{
	struct frame_object *self = (struct frame_object *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IAgileObject) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame))
		*out = iface;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap))
		*out = &self->soft;
	else if (equal_iid(iid, &IID_IWeakReferenceSource))
		*out = &self->weak;
	else {
		trace_query_refuse("BitmapFrame", iid);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI frame_addref(frame1 *iface)
{
	return object_addref(&((struct frame_object *)iface)->refs);
}

static ULONG WINAPI frame_release(frame1 *iface)
{
	struct frame_object *self = (struct frame_object *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		raven_weak_source_disconnect(&self->weak);
		if (self->wic)
			IWICBitmapFrameDecode_Release(self->wic);
		HeapFree(GetProcessHeap(), 0, iface);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI frame_iids(frame1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI frame_name(frame1 *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, L"Windows.Graphics.Imaging.BitmapFrame");
}

static HRESULT WINAPI frame_trust(frame1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI frame_get_thumbnail_async(frame1 *iface, IUnknown **operation)
{
	(void)iface;
	if (operation)
		*operation = NULL;
	trace_frame_member("GetThumbnailAsync");
	return E_NOTIMPL;
}

static HRESULT WINAPI frame_get_properties(frame1 *iface, IInspectable **value)
{
	(void)iface;
	if (value)
		*value = NULL;
	trace_frame_member("get_BitmapProperties");
	return E_NOTIMPL;
}

static HRESULT WINAPI frame_get_pixel_format(frame1 *iface, int32_t *value)
{
	struct frame_object *self = (struct frame_object *)iface;
	if (!value)
		return E_POINTER;
	*value = pixel_format_from_wic(&self->format);
	return S_OK;
}

static HRESULT WINAPI frame_get_alpha_mode(frame1 *iface, int32_t *value)
{
	struct frame_object *self = (struct frame_object *)iface;
	if (!value)
		return E_POINTER;
	*value = alpha_mode_from_wic(&self->format);
	return S_OK;
}

static double frame_resolution(double value)
{
	/* WIC answers 96 for an image that carries none */
	return value ? value : 96.0;
}

static HRESULT WINAPI frame_get_dpi_x(frame1 *iface, double *value)
{
	struct frame_object *self = (struct frame_object *)iface;
	if (!value)
		return E_POINTER;
	*value = frame_resolution(self->dpi_x);
	return S_OK;
}

static HRESULT WINAPI frame_get_dpi_y(frame1 *iface, double *value)
{
	struct frame_object *self = (struct frame_object *)iface;
	if (!value)
		return E_POINTER;
	*value = frame_resolution(self->dpi_y);
	return S_OK;
}

static HRESULT WINAPI frame_get_pixel_width(frame1 *iface, UINT32 *value)
{
	struct frame_object *self = (struct frame_object *)iface;
	if (!value)
		return E_POINTER;
	*value = self->width;
	return S_OK;
}

static HRESULT WINAPI frame_get_pixel_height(frame1 *iface, UINT32 *value)
{
	struct frame_object *self = (struct frame_object *)iface;
	if (!value)
		return E_POINTER;
	*value = self->height;
	return S_OK;
}

static HRESULT WINAPI frame_get_oriented_width(frame1 *iface, UINT32 *value)
{
	struct frame_object *self = (struct frame_object *)iface;
	if (!value)
		return E_POINTER;
	trace_frame_member("get_OrientedPixelWidth");
	/* raven reads no Exif orientation, so the oriented size is the stored size */
	*value = self->width;
	return S_OK;
}

static HRESULT WINAPI frame_get_oriented_height(frame1 *iface, UINT32 *value)
{
	struct frame_object *self = (struct frame_object *)iface;
	if (!value)
		return E_POINTER;
	trace_frame_member("get_OrientedPixelHeight");
	*value = self->height;
	return S_OK;
}

static HRESULT WINAPI frame_get_pixel_data_async(frame1 *iface, IUnknown **operation)
{
	struct frame_object *self = (struct frame_object *)iface;
	struct pixel_operation *op;
	struct pixel_provider_object *provider;
	BYTE *pixels = NULL;
	UINT32 length = 0;
	HRESULT result;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	trace_frame_member("GetPixelDataAsync");
	if (!(op = pixel_operation_new()))
		return E_OUTOFMEMORY;
	result = frame_pixel_data(self, NULL, BitmapPixelFormat_Unknown, BitmapAlphaMode_Ignore, &pixels, &length);
	if (SUCCEEDED(result) && (provider = create_pixel_provider(pixels, length))) {
		pixel_operation_ready(op, (pixel_provider1 *)provider, S_OK);
	} else {
		if (pixels)
			FREE(pixels);
		pixel_operation_ready(op, NULL, SUCCEEDED(result) ? E_OUTOFMEMORY : result);
	}
	*operation = (IUnknown *)&op->iface;
	return S_OK;
}

static HRESULT WINAPI frame_get_pixel_data_transformed_async(frame1 *iface, int32_t pixel_format, int32_t alpha_mode,
	IUnknown *transform, int32_t exif_orientation_mode, int32_t color_management_mode, IUnknown **operation)
{
	struct frame_object *self = (struct frame_object *)iface;
	struct pixel_operation *op;
	struct pixel_provider_object *provider;
	const transform1 *face;
	BYTE *pixels = NULL;
	UINT32 length = 0;
	HRESULT result;
	(void)exif_orientation_mode;
	(void)color_management_mode;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	trace_frame_member("GetPixelDataTransformedAsync");
	if (!(op = pixel_operation_new()))
		return E_OUTOFMEMORY;
	face = transform_face(transform);
	result = frame_pixel_data(self, face, pixel_format, alpha_mode, &pixels, &length);
	if (face)
		imaging_release_iunknown(face);
	if (SUCCEEDED(result) && (provider = create_pixel_provider(pixels, length))) {
		pixel_operation_ready(op, (pixel_provider1 *)provider, S_OK);
	} else {
		if (pixels)
			FREE(pixels);
		pixel_operation_ready(op, NULL, SUCCEEDED(result) ? E_OUTOFMEMORY : result);
	}
	*operation = (IUnknown *)&op->iface;
	return S_OK;
}

static const frame1Vtbl frame_table = {
	.QueryInterface = frame_query,
	.AddRef = frame_addref,
	.Release = frame_release,
	.GetIids = frame_iids,
	.GetRuntimeClassName = frame_name,
	.GetTrustLevel = frame_trust,
	.GetThumbnailAsync = frame_get_thumbnail_async,
	.get_BitmapProperties = frame_get_properties,
	.get_BitmapPixelFormat = frame_get_pixel_format,
	.get_BitmapAlphaMode = frame_get_alpha_mode,
	.get_DpiX = frame_get_dpi_x,
	.get_DpiY = frame_get_dpi_y,
	.get_PixelWidth = frame_get_pixel_width,
	.get_PixelHeight = frame_get_pixel_height,
	.get_OrientedPixelWidth = frame_get_oriented_width,
	.get_OrientedPixelHeight = frame_get_oriented_height,
	.GetPixelDataAsync = frame_get_pixel_data_async,
	.GetPixelDataTransformedAsync = frame_get_pixel_data_transformed_async,
};

/* The SoftwareBitmap-producing slots of the same object; the IBitmapFrame face is the one the metadata makes
 * required, and these three answer through it. */
static HRESULT WINAPI frame_soft_query(frame_soft1 *iface, REFIID iid, void **out)
{
	return frame_query((frame1 *)iface, iid, out);
}

static ULONG WINAPI frame_soft_addref(frame_soft1 *iface)
{
	return frame_addref((frame1 *)iface);
}

static ULONG WINAPI frame_soft_release(frame_soft1 *iface)
{
	return frame_release((frame1 *)iface);
}

static HRESULT WINAPI frame_soft_iids(frame_soft1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI frame_soft_name(frame_soft1 *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, L"Windows.Graphics.Imaging.BitmapFrame");
}

static HRESULT WINAPI frame_soft_trust(frame_soft1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI frame_soft_get_bitmap_async(frame_soft1 *iface, IUnknown **operation)
{
	struct frame_object *self = (struct frame_object *)frame_soft_owner(iface);
	struct software_operation *op;
	software1 *bitmap;
	HRESULT result;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	trace_frame_member("GetSoftwareBitmapAsync");
	if (!(op = software_operation_new()))
		return E_OUTOFMEMORY;
	bitmap = create_software_bitmap(self, NULL, BitmapPixelFormat_Bgra8, BitmapAlphaMode_Premultiplied);
	result = bitmap ? S_OK : E_FAIL;
	software_operation_ready(op, bitmap, result);
	*operation = (IUnknown *)&op->iface;
	return S_OK;
}

static HRESULT WINAPI frame_soft_get_bitmap_converted_async(frame_soft1 *iface, int32_t pixel_format,
	int32_t alpha_mode, IUnknown **operation)
{
	struct frame_object *self = (struct frame_object *)frame_soft_owner(iface);
	struct software_operation *op;
	software1 *bitmap;
	HRESULT result;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	trace_frame_member("GetSoftwareBitmapConvertedAsync");
	if (!(op = software_operation_new()))
		return E_OUTOFMEMORY;
	bitmap = create_software_bitmap(self, NULL, pixel_format, alpha_mode);
	result = bitmap ? S_OK : E_FAIL;
	software_operation_ready(op, bitmap, result);
	*operation = (IUnknown *)&op->iface;
	return S_OK;
}

static HRESULT WINAPI frame_soft_get_bitmap_transformed_async(frame_soft1 *iface, int32_t pixel_format,
	int32_t alpha_mode, IUnknown *transform, int32_t exif_orientation_mode, int32_t color_management_mode,
	IUnknown **operation)
{
	struct frame_object *self = (struct frame_object *)frame_soft_owner(iface);
	struct software_operation *op;
	const transform1 *face;
	software1 *bitmap;
	HRESULT result;
	(void)exif_orientation_mode;
	(void)color_management_mode;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	trace_frame_member("GetSoftwareBitmapTransformedAsync");
	if (!(op = software_operation_new()))
		return E_OUTOFMEMORY;
	face = transform_face(transform);
	bitmap = create_software_bitmap(self, face, pixel_format, alpha_mode);
	if (face)
		imaging_release_iunknown(face);
	result = bitmap ? S_OK : E_FAIL;
	software_operation_ready(op, bitmap, result);
	*operation = (IUnknown *)&op->iface;
	return S_OK;
}

static const frame_soft1Vtbl frame_soft_table = {
	.QueryInterface = frame_soft_query,
	.AddRef = frame_soft_addref,
	.Release = frame_soft_release,
	.GetIids = frame_soft_iids,
	.GetRuntimeClassName = frame_soft_name,
	.GetTrustLevel = frame_soft_trust,
	.GetSoftwareBitmapAsync = frame_soft_get_bitmap_async,
	.GetSoftwareBitmapConvertedAsync = frame_soft_get_bitmap_converted_async,
	.GetSoftwareBitmapTransformedAsync = frame_soft_get_bitmap_transformed_async,
};

static struct frame_object *create_frame_object(IWICBitmapFrameDecode *wic)
{
	struct frame_object *self;
	if (!(self = ALLOC(sizeof *self)))
		return NULL;
	self->lpVtbl = &frame_table;
	self->soft.lpVtbl = &frame_soft_table;
	self->refs = 1;
	raven_weak_source_init(&self->weak, self, &self->refs, (raven_weak_query_fn)frame_query);
	self->wic = wic;
	if (FAILED(IWICBitmapFrameDecode_GetSize(wic, &self->width, &self->height))) {
		self->width = 0;
		self->height = 0;
	}
	if (FAILED(IWICBitmapFrameDecode_GetPixelFormat(wic, &self->format)))
		self->format = GUID_WICPixelFormat32bppBGRA;
	if (FAILED(IWICBitmapFrameDecode_GetResolution(wic, &self->dpi_x, &self->dpi_y))) {
		self->dpi_x = 0;
		self->dpi_y = 0;
	}
	InterlockedIncrement(&raven_xaml_objects);
	return self;
}

/* ---- The pixel data provider: the bytes GetPixelDataAsync produced, detached once. ---- */

struct pixel_provider_object {
	const pixel_provider1Vtbl *lpVtbl;
	LONG refs;
	struct raven_weak_source weak;
	BYTE *pixels;
	UINT32 length;
	BOOL detached;
};

static HRESULT WINAPI pixel_provider_query(pixel_provider1 *iface, REFIID iid, void **out)
{
	struct pixel_provider_object *self = (struct pixel_provider_object *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProvider))
		*out = iface;
	else if (equal_iid(iid, &IID_IWeakReferenceSource))
		*out = &self->weak;
	else {
		trace_query_refuse("PixelDataProvider", iid);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI pixel_provider_addref(pixel_provider1 *iface)
{
	return object_addref(&((struct pixel_provider_object *)iface)->refs);
}

static ULONG WINAPI pixel_provider_release(pixel_provider1 *iface)
{
	struct pixel_provider_object *self = (struct pixel_provider_object *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		raven_weak_source_disconnect(&self->weak);
		if (self->pixels)
			FREE(self->pixels);
		HeapFree(GetProcessHeap(), 0, iface);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI pixel_provider_iids(pixel_provider1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI pixel_provider_name(pixel_provider1 *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, L"Windows.Graphics.Imaging.PixelDataProvider");
}

static HRESULT WINAPI pixel_provider_trust(pixel_provider1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI pixel_provider_detach(pixel_provider1 *iface, UINT32 *length, BYTE **value)
{
	struct pixel_provider_object *self = (struct pixel_provider_object *)iface;
	if (!length || !value)
		return E_POINTER;
	*length = 0;
	*value = NULL;
	trace_provider_member("DetachPixelData");
	if (self->detached)
		return E_ILLEGAL_METHOD_CALL;
	self->detached = TRUE;
	*length = self->length;
	*value = self->pixels;
	self->pixels = NULL;
	return S_OK;
}

static const pixel_provider1Vtbl pixel_provider_table = {
	.QueryInterface = pixel_provider_query,
	.AddRef = pixel_provider_addref,
	.Release = pixel_provider_release,
	.GetIids = pixel_provider_iids,
	.GetRuntimeClassName = pixel_provider_name,
	.GetTrustLevel = pixel_provider_trust,
	.DetachPixelData = pixel_provider_detach,
};

static struct pixel_provider_object *create_pixel_provider(BYTE *pixels, UINT32 length)
{
	struct pixel_provider_object *self;
	if (!(self = ALLOC(sizeof *self)))
		return NULL;
	self->lpVtbl = &pixel_provider_table;
	self->refs = 1;
	raven_weak_source_init(&self->weak, self, &self->refs, (raven_weak_query_fn)pixel_provider_query);
	self->pixels = pixels;
	self->length = length;
	InterlockedIncrement(&raven_xaml_objects);
	return self;
}

/* ---- The software bitmap: the decoded pixels with the format, alpha and resolution the decode chose. The buffer
 * access the interface also declares — LockBuffer, the copies, the read-only view — is not answered yet; the trace
 * names the members a guest reaches. ---- */

struct software_object {
	const software1Vtbl *lpVtbl;
	LONG refs;
	struct raven_weak_source weak;
	int32_t pixel_format, alpha_mode;
	int32_t width, height;
	double dpi_x, dpi_y;
	BYTE *pixels;
	UINT stride;
};

static HRESULT WINAPI software_query(software1 *iface, REFIID iid, void **out)
{
	struct software_object *self = (struct software_object *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap))
		*out = iface;
	else if (equal_iid(iid, &IID_IWeakReferenceSource))
		*out = &self->weak;
	else {
		trace_query_refuse("SoftwareBitmap", iid);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI software_addref(software1 *iface)
{
	return object_addref(&((struct software_object *)iface)->refs);
}

static ULONG WINAPI software_release(software1 *iface)
{
	struct software_object *self = (struct software_object *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		raven_weak_source_disconnect(&self->weak);
		if (self->pixels)
			FREE(self->pixels);
		HeapFree(GetProcessHeap(), 0, iface);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI software_iids(software1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI software_name(software1 *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, L"Windows.Graphics.Imaging.SoftwareBitmap");
}

static HRESULT WINAPI software_trust(software1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI software_get_pixel_format(software1 *iface, int32_t *value)
{
	struct software_object *self = (struct software_object *)iface;
	if (!value)
		return E_POINTER;
	*value = self->pixel_format;
	return S_OK;
}

static HRESULT WINAPI software_get_alpha_mode(software1 *iface, int32_t *value)
{
	struct software_object *self = (struct software_object *)iface;
	if (!value)
		return E_POINTER;
	*value = self->alpha_mode;
	return S_OK;
}

static HRESULT WINAPI software_get_width(software1 *iface, int32_t *value)
{
	struct software_object *self = (struct software_object *)iface;
	if (!value)
		return E_POINTER;
	*value = self->width;
	return S_OK;
}

static HRESULT WINAPI software_get_height(software1 *iface, int32_t *value)
{
	struct software_object *self = (struct software_object *)iface;
	if (!value)
		return E_POINTER;
	*value = self->height;
	return S_OK;
}

static HRESULT WINAPI software_get_read_only(software1 *iface, boolean *value)
{
	struct software_object *self = (struct software_object *)iface;
	if (!value)
		return E_POINTER;
	/* the decode's own pixels: nothing raven serves mutates them */
	*value = TRUE;
	return S_OK;
}

static HRESULT WINAPI software_put_dpi_x(software1 *iface, double value)
{
	struct software_object *self = (struct software_object *)iface;
	trace_software_member("put_DpiX");
	self->dpi_x = value;
	return S_OK;
}

static HRESULT WINAPI software_get_dpi_x(software1 *iface, double *value)
{
	struct software_object *self = (struct software_object *)iface;
	if (!value)
		return E_POINTER;
	*value = self->dpi_x;
	return S_OK;
}

static HRESULT WINAPI software_put_dpi_y(software1 *iface, double value)
{
	struct software_object *self = (struct software_object *)iface;
	trace_software_member("put_DpiY");
	self->dpi_y = value;
	return S_OK;
}

static HRESULT WINAPI software_get_dpi_y(software1 *iface, double *value)
{
	struct software_object *self = (struct software_object *)iface;
	if (!value)
		return E_POINTER;
	*value = self->dpi_y;
	return S_OK;
}

static HRESULT WINAPI software_lock_buffer(software1 *iface, int32_t access_mode, IInspectable **value)
{
	(void)access_mode;
	if (value)
		*value = NULL;
	trace_software_member("LockBuffer");
	return E_NOTIMPL;
}

static HRESULT WINAPI software_copy_to(software1 *iface, software1 *other)
{
	(void)other;
	trace_software_member("CopyTo");
	return E_NOTIMPL;
}

static HRESULT WINAPI software_copy_from_buffer(software1 *iface, IInspectable *buffer)
{
	(void)buffer;
	trace_software_member("CopyFromBuffer");
	return E_NOTIMPL;
}

static HRESULT WINAPI software_copy_to_buffer(software1 *iface, IInspectable *buffer)
{
	(void)buffer;
	trace_software_member("CopyToBuffer");
	return E_NOTIMPL;
}

static HRESULT WINAPI software_get_read_only_view(software1 *iface, IInspectable **value)
{
	if (value)
		*value = NULL;
	trace_software_member("GetReadOnlyView");
	return E_NOTIMPL;
}

static const software1Vtbl software_table = {
	.QueryInterface = software_query,
	.AddRef = software_addref,
	.Release = software_release,
	.GetIids = software_iids,
	.GetRuntimeClassName = software_name,
	.GetTrustLevel = software_trust,
	.get_BitmapPixelFormat = software_get_pixel_format,
	.get_BitmapAlphaMode = software_get_alpha_mode,
	.get_PixelWidth = software_get_width,
	.get_PixelHeight = software_get_height,
	.get_IsReadOnly = software_get_read_only,
	.put_DpiX = software_put_dpi_x,
	.get_DpiX = software_get_dpi_x,
	.put_DpiY = software_put_dpi_y,
	.get_DpiY = software_get_dpi_y,
	.LockBuffer = software_lock_buffer,
	.CopyTo = software_copy_to,
	.CopyFromBuffer = software_copy_from_buffer,
	.CopyToBuffer = software_copy_to_buffer,
	.GetReadOnlyView = software_get_read_only_view,
};

static software1 *create_software_bitmap(struct frame_object *frame, const transform1 *transform,
	int32_t pixel_format, int32_t alpha_mode)
{
	struct software_object *self;
	const GUID *target;
	IWICBitmapSource *source = NULL;
	UINT width, height;
	HRESULT result;
	if (!frame)
		return NULL;
	if (pixel_format == BitmapPixelFormat_Unknown && alpha_mode == BitmapAlphaMode_Ignore)
		target = &frame->format;
	else if (!(target = alpha_target_format(pixel_format, alpha_mode)))
		target = &frame->format;
	if (!(self = ALLOC(sizeof *self)))
		return NULL;
	self->lpVtbl = &software_table;
	self->refs = 1;
	raven_weak_source_init(&self->weak, self, &self->refs, (raven_weak_query_fn)software_query);
	self->pixel_format = same_guid(target, &frame->format) ? pixel_format_from_wic(target) : pixel_format;
	self->alpha_mode = alpha_mode;
	result = transformed_source(frame, transform, target, &source);
	if (FAILED(result)) {
		FREE(self);
		return NULL;
	}
	IWICBitmapSource_GetSize(source, &width, &height);
	self->width = (int32_t)width;
	self->height = (int32_t)height;
	self->dpi_x = frame->dpi_x ? frame->dpi_x : 96.0;
	self->dpi_y = frame->dpi_y ? frame->dpi_y : 96.0;
	self->stride = ((width * pixel_format_bpp(target) * 8 + 31) / 32) * 4;
	if ((self->pixels = ALLOC(self->stride * height ? (SIZE_T)self->stride * height : 1)))
		IWICBitmapSource_CopyPixels(source, NULL, self->stride, self->stride * height, self->pixels);
	IWICBitmapSource_Release(source);
	InterlockedIncrement(&raven_xaml_objects);
	return (software1 *)self;
}

/* ---- The decoder: WIC's decoder with the frame count and codec information it answers. ---- */

struct decoder_object {
	const decoder1Vtbl *lpVtbl;
	LONG refs;
	struct raven_weak_source weak;
	IWICBitmapDecoder *wic;
	UINT32 frame_count;
	struct codec_information *information;
	/* The frame-0 face BitmapDecoder carries: the class implements IBitmapFrame and IBitmapFrameWithSoftwareBitmap
	 * over the first frame (Microsoft's own contract, "provides access to ... data from the first frame"), so the
	 * projection's cast of the CreateAsync result to IBitmapFrame serves from here. */
	struct frame_object *frame0;
};

static HRESULT WINAPI decoder_query(decoder1 *iface, REFIID iid, void **out)
{
	struct decoder_object *self = (struct decoder_object *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder))
		*out = iface;
	else if (equal_iid(iid, &IID_IWeakReferenceSource))
		*out = &self->weak;
	else if (self->frame0 && (equal_iid(iid, &IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap)))
		return frame_query((frame1 *)self->frame0, iid, out);
	else {
		trace_query_refuse("BitmapDecoder", iid);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI decoder_addref(decoder1 *iface)
{
	return object_addref(&((struct decoder_object *)iface)->refs);
}

static ULONG WINAPI decoder_release(decoder1 *iface)
{
	struct decoder_object *self = (struct decoder_object *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		raven_weak_source_disconnect(&self->weak);
		if (self->frame0)
			frame_release((frame1 *)self->frame0);
		if (self->wic)
			IWICBitmapDecoder_Release(self->wic);
		if (self->information)
			codec_information_release((codec_info1 *)self->information);
		HeapFree(GetProcessHeap(), 0, iface);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI decoder_iids(decoder1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI decoder_name(decoder1 *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, L"Windows.Graphics.Imaging.BitmapDecoder");
}

static HRESULT WINAPI decoder_trust(decoder1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI decoder_get_container_properties(decoder1 *iface, IInspectable **value)
{
	(void)iface;
	if (value)
		*value = NULL;
	trace_decoder_member("get_BitmapContainerProperties");
	return E_NOTIMPL;
}

static HRESULT WINAPI decoder_get_information(decoder1 *iface, IInspectable **value)
{
	struct decoder_object *self = (struct decoder_object *)iface;
	if (!value)
		return E_POINTER;
	trace_decoder_member("get_DecoderInformation");
	if (!self->information)
		return E_NOTIMPL;
	*value = (IInspectable *)self->information;
	codec_information_addref((codec_info1 *)self->information);
	return S_OK;
}

static HRESULT WINAPI decoder_get_frame_count(decoder1 *iface, UINT32 *value)
{
	struct decoder_object *self = (struct decoder_object *)iface;
	if (!value)
		return E_POINTER;
	*value = self->frame_count;
	return S_OK;
}

static HRESULT WINAPI decoder_get_preview_async(decoder1 *iface, IUnknown **operation)
{
	(void)iface;
	if (operation)
		*operation = NULL;
	trace_decoder_member("GetPreviewAsync");
	return E_NOTIMPL;
}

static HRESULT WINAPI decoder_get_frame_async(decoder1 *iface, UINT32 index, IUnknown **operation)
{
	struct decoder_object *self = (struct decoder_object *)iface;
	struct frame_operation *op;
	IWICBitmapFrameDecode *wic_frame = NULL;
	struct frame_object *frame = NULL;
	HRESULT result;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	trace_decoder_member("GetFrameAsync");
	if (!(op = frame_operation_new()))
		return E_OUTOFMEMORY;
	result = IWICBitmapDecoder_GetFrame(self->wic, index, &wic_frame);
	if (SUCCEEDED(result) && wic_frame && (frame = create_frame_object(wic_frame))) {
		frame_operation_ready(op, (frame1 *)frame, S_OK);
	} else {
		if (FAILED(result))
			trace_decode_result("GetFrame", result);
		if (wic_frame)
			IWICBitmapFrameDecode_Release(wic_frame);
		frame_operation_ready(op, NULL, SUCCEEDED(result) ? E_OUTOFMEMORY : result);
	}
	*operation = (IUnknown *)&op->iface;
	return S_OK;
}

static const decoder1Vtbl decoder_table = {
	.QueryInterface = decoder_query,
	.AddRef = decoder_addref,
	.Release = decoder_release,
	.GetIids = decoder_iids,
	.GetRuntimeClassName = decoder_name,
	.GetTrustLevel = decoder_trust,
	.get_BitmapContainerProperties = decoder_get_container_properties,
	.get_DecoderInformation = decoder_get_information,
	.get_FrameCount = decoder_get_frame_count,
	.GetPreviewAsync = decoder_get_preview_async,
	.GetFrameAsync = decoder_get_frame_async,
};

/* The creation reference of the WIC decoder transfers to the object; the caller must not release it. */
static struct decoder_object *create_decoder_object(IWICBitmapDecoder *wic)
{
	struct decoder_object *self;
	if (!(self = ALLOC(sizeof *self)))
		return NULL;
	self->lpVtbl = &decoder_table;
	self->refs = 1;
	raven_weak_source_init(&self->weak, self, &self->refs, (raven_weak_query_fn)decoder_query);
	self->wic = wic;
	self->information = create_codec_information(wic);
	if (FAILED(IWICBitmapDecoder_GetFrameCount(wic, &self->frame_count)))
		self->frame_count = 0;
	/* The frame-0 face the class contract carries: a real WIC frame, so every IBitmapFrame member the projection
	 * drives through the decoder answers the decoded image's own data. */
	self->frame0 = NULL;
	{
		IWICBitmapFrameDecode *wic_frame = NULL;
		if (SUCCEEDED(IWICBitmapDecoder_GetFrame(wic, 0, &wic_frame)) && wic_frame)
			self->frame0 = create_frame_object(wic_frame);
	}
	InterlockedIncrement(&raven_xaml_objects);
	return self;
}

/* CreateWithIdAsync's decoder identifier: the codec class the application asks for must be the codec the stream
 * decodes as, the way WIC's own registry-driven creation would refuse a mismatch. */
static BOOL decoder_id_matches(IWICBitmapDecoder *wic, REFGUID id)
{
	IWICBitmapDecoderInfo *info = NULL;
	CLSID clsid;
	BOOL match = FALSE;
	if (SUCCEEDED(IWICBitmapDecoder_GetDecoderInfo(wic, &info)) && info) {
		if (SUCCEEDED(IWICComponentInfo_GetCLSID((IWICComponentInfo *)info, &clsid)))
			match = same_guid(&clsid, id);
		IWICBitmapDecoderInfo_Release(info);
	}
	return match;
}

/* ---- The decoder's codec-information view: the empty set the enumeration hands back, over the parameterized
 * IVectorView`1<BitmapCodecInformation> face. ---- */

static const WCHAR decoder_vector_name[] =
	L"Windows.Foundation.Collections.IVectorView`1<Windows.Graphics.Imaging.BitmapCodecInformation>";

typedef struct decoder_vector decoder_vector;
typedef struct decoder_vector_vtbl decoder_vector_vtbl;
struct decoder_vector_vtbl {
	BEGIN_INTERFACE
	HRESULT (WINAPI *QueryInterface)(IInspectable *This, REFIID riid, void **object);
	ULONG (WINAPI *AddRef)(IInspectable *This);
	ULONG (WINAPI *Release)(IInspectable *This);
	HRESULT (WINAPI *GetIids)(IInspectable *This, ULONG *count, IID **iids);
	HRESULT (WINAPI *GetRuntimeClassName)(IInspectable *This, HSTRING *name);
	HRESULT (WINAPI *GetTrustLevel)(IInspectable *This, TrustLevel *level);
	HRESULT (WINAPI *GetAt)(IInspectable *This, UINT32 index, IInspectable **value);
	HRESULT (WINAPI *get_Size)(IInspectable *This, UINT32 *value);
	HRESULT (WINAPI *IndexOf)(IInspectable *This, IInspectable *element, UINT32 *index, boolean *found);
	HRESULT (WINAPI *GetMany)(IInspectable *This, UINT32 start_index, UINT32 items_size, IInspectable **items, UINT32 *value);
	END_INTERFACE
};

struct decoder_vector {
	const decoder_vector_vtbl *lpVtbl;
	LONG refs;
};

static LONG decoder_vectors;

static HRESULT WINAPI decoder_vector_query(IInspectable *iface, REFIID iid, void **out)
{
	decoder_vector *self = (decoder_vector *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) && !equal_iid(iid, &IID_IAgileObject) &&
		!equal_iid(iid, &decoder_vector_parameterized_iid))
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI decoder_vector_addref(IInspectable *iface)
{
	return (ULONG)InterlockedIncrement(&((decoder_vector *)iface)->refs);
}

static ULONG WINAPI decoder_vector_release(IInspectable *iface)
{
	decoder_vector *self = (decoder_vector *)iface;
	const LONG refs_left = InterlockedDecrement(&self->refs);
	if (!refs_left) {
		HeapFree(GetProcessHeap(), 0, self);
		InterlockedDecrement(&decoder_vectors);
	}
	return (ULONG)refs_left;
}

static HRESULT WINAPI decoder_vector_iids(IInspectable *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI decoder_vector_runtime_name(IInspectable *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, decoder_vector_name);
}

static HRESULT WINAPI decoder_vector_trust(IInspectable *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI decoder_vector_get_at(IInspectable *iface, UINT32 index, IInspectable **value)
{
	(void)iface;
	(void)index;
	if (!value)
		return E_POINTER;
	*value = NULL;
	trace_decoder_member("vector.GetAt");
	return E_BOUNDS;
}

static HRESULT WINAPI decoder_vector_get_size(IInspectable *iface, UINT32 *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = 0;
	return S_OK;
}

static HRESULT WINAPI decoder_vector_index_of(IInspectable *iface, IInspectable *element, UINT32 *index, boolean *found)
{
	(void)iface;
	(void)element;
	if (index)
		*index = 0;
	if (found)
		*found = FALSE;
	return S_OK;
}

static HRESULT WINAPI decoder_vector_get_many(IInspectable *iface, UINT32 start_index, UINT32 items_size,
	IInspectable **items, UINT32 *value)
{
	(void)iface;
	(void)start_index;
	if (value)
		*value = 0;
	(void)items_size;
	(void)items;
	return S_OK;
}

static const decoder_vector_vtbl decoder_vector_table = {
	.QueryInterface = decoder_vector_query,
	.AddRef = decoder_vector_addref,
	.Release = decoder_vector_release,
	.GetIids = decoder_vector_iids,
	.GetRuntimeClassName = decoder_vector_runtime_name,
	.GetTrustLevel = decoder_vector_trust,
	.GetAt = decoder_vector_get_at,
	.get_Size = decoder_vector_get_size,
	.IndexOf = decoder_vector_index_of,
	.GetMany = decoder_vector_get_many,
};

static decoder_vector *create_decoder_vector(void)
{
	decoder_vector *object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *object);
	if (!object)
		return NULL;
	object->lpVtbl = &decoder_vector_table;
	object->refs = 1;
	InterlockedIncrement(&decoder_vectors);
	return object;
}

/* ---- The activation factory: the transform class has no statics, so only the activation query is answered; the
 * decoder class is statics-driven, so its factory also carries the IBitmapDecoderStatics face the activation query
 * names. ---- */

struct factory;
struct part_activation { const IActivationFactoryVtbl *lpVtbl; struct factory *owner; };
struct part_statics { const decoder_statics1Vtbl *lpVtbl; struct factory *owner; };

struct factory {
	struct part_activation activation;
	struct part_statics statics;
	LONG refs;
	BOOL decoder;
};

static HRESULT factory_query(struct factory *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory)) {
		*out = &self->activation;
		InterlockedIncrement(&self->refs);
		return S_OK;
	}
	if (self->decoder && equal_iid(iid, &IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics)) {
		*out = &self->statics;
		InterlockedIncrement(&self->refs);
		return S_OK;
	}
	return E_NOINTERFACE;
}

#define PART_BOILERPLATE(part, iface) \
	static ULONG WINAPI part##_addref(iface *iface_unused) \
	{ return (ULONG)InterlockedIncrement(&((struct part *)iface_unused)->owner->refs); } \
	static ULONG WINAPI part##_release(iface *iface_unused) \
	{ return (ULONG)InterlockedDecrement(&((struct part *)iface_unused)->owner->refs); } \
	static HRESULT WINAPI part##_iids(iface *iface_unused, ULONG *count, IID **iids) \
	{ (void)iface_unused; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI part##_trust(iface *iface_unused, TrustLevel *level) \
	{ (void)iface_unused; return inspectable_trust(level); }

PART_BOILERPLATE(part_activation, IActivationFactory)

static HRESULT WINAPI part_activation_query(IActivationFactory *iface, REFIID iid, void **out)
{
	return factory_query(((struct part_activation *)iface)->owner, iid, out);
}

static HRESULT WINAPI part_activation_name(IActivationFactory *iface, HSTRING *name)
{
	struct factory *self = ((struct part_activation *)iface)->owner;
	(void)iface;
	return runtime_class_name(name, self->decoder ? decoder_class_name : class_name);
}

PART_BOILERPLATE(part_statics, decoder_statics1)

static HRESULT WINAPI part_statics_query(decoder_statics1 *iface, REFIID iid, void **out)
{
	return factory_query(((struct part_statics *)iface)->owner, iid, out);
}

static HRESULT WINAPI part_statics_name(decoder_statics1 *iface, HSTRING *name)
{
	(void)iface;
	return runtime_class_name(name, decoder_class_name);
}

static HRESULT WINAPI activation_activate(IActivationFactory *iface, IInspectable **out)
{
	struct factory *self = ((struct part_activation *)iface)->owner;
	struct transform_object *object;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (self->decoder) {
		/* the real BitmapDecoder has no default constructor: only the statics build instances */
		trace_decoder_member("ActivateInstance");
		return E_NOTIMPL;
	}
	trace_member("ActivateInstance");
	if (!(object = create_transform()))
		return E_OUTOFMEMORY;
	*out = (IInspectable *)(transform1 *)object;
	return S_OK;
}

static const IActivationFactoryVtbl activation_table = {
	.QueryInterface = part_activation_query,
	.AddRef = part_activation_addref,
	.Release = part_activation_release,
	.GetIids = part_activation_iids,
	.GetRuntimeClassName = part_activation_name,
	.GetTrustLevel = part_activation_trust,
	.ActivateInstance = activation_activate,
};

/* The decoder identifiers, answered from the constants the WIC decoder classes carry. */
static HRESULT WINAPI statics_bmp_id(decoder_statics1 *iface, GUID *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_decoder_member("get_BmpDecoderId");
	*value = bmp_decoder_id;
	return S_OK;
}

static HRESULT WINAPI statics_jpeg_id(decoder_statics1 *iface, GUID *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_decoder_member("get_JpegDecoderId");
	*value = jpeg_decoder_id;
	return S_OK;
}

static HRESULT WINAPI statics_png_id(decoder_statics1 *iface, GUID *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_decoder_member("get_PngDecoderId");
	*value = png_decoder_id;
	return S_OK;
}

static HRESULT WINAPI statics_tiff_id(decoder_statics1 *iface, GUID *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_decoder_member("get_TiffDecoderId");
	*value = tiff_decoder_id;
	return S_OK;
}

static HRESULT WINAPI statics_gif_id(decoder_statics1 *iface, GUID *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_decoder_member("get_GifDecoderId");
	*value = gif_decoder_id;
	return S_OK;
}

static HRESULT WINAPI statics_jpeg_xr_id(decoder_statics1 *iface, GUID *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_decoder_member("get_JpegXRDecoderId");
	*value = jpeg_xr_decoder_id;
	return S_OK;
}

static HRESULT WINAPI statics_ico_id(decoder_statics1 *iface, GUID *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	trace_decoder_member("get_IcoDecoderId");
	*value = ico_decoder_id;
	return S_OK;
}

static HRESULT WINAPI statics_enumerator(decoder_statics1 *iface, IInspectable **value)
{
	decoder_vector *vector;
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	trace_decoder_member("GetDecoderInformationEnumerator");
	if (!(vector = create_decoder_vector()))
		return E_OUTOFMEMORY;
	*value = (IInspectable *)vector;
	return S_OK;
}

/* The stream decoders: the decode happens through the guest's WIC component and completes inside the creation
 * call, the way storage.c's read operations do. */
static HRESULT WINAPI statics_create_async(decoder_statics1 *iface, IUnknown *stream, IInspectable **operation)
{
	struct decoder_operation *op;
	IWICBitmapDecoder *wic;
	struct decoder_object *decoder;
	(void)iface;
	if (operation)
		*operation = NULL;
	trace_decoder_member("CreateAsync");
	if (!operation)
		return E_POINTER;
	if (!(op = decoder_operation_new()))
		return E_OUTOFMEMORY;
	wic = stream ? decode_stream(stream) : NULL;
	decoder = wic ? create_decoder_object(wic) : NULL;
	decoder_operation_ready(op, (decoder1 *)decoder, decoder ? S_OK : E_FAIL);
	*operation = (IInspectable *)&op->iface;
	return S_OK;
}

static HRESULT WINAPI statics_create_with_id_async(decoder_statics1 *iface, GUID decoder_id, IUnknown *stream,
	IInspectable **operation)
{
	struct decoder_operation *op;
	IWICBitmapDecoder *wic;
	struct decoder_object *decoder;
	HRESULT error = S_OK;
	(void)iface;
	if (operation)
		*operation = NULL;
	trace_decoder_member("CreateWithIdAsync");
	if (!operation)
		return E_POINTER;
	if (!(op = decoder_operation_new()))
		return E_OUTOFMEMORY;
	wic = stream ? decode_stream(stream) : NULL;
	decoder = wic ? create_decoder_object(wic) : NULL;
	if (wic && decoder && !decoder_id_matches(wic, &decoder_id))
		error = WINCODEC_ERR_COMPONENTNOTFOUND;
	else if (!wic || !decoder)
		error = E_FAIL;
	decoder_operation_ready(op, (decoder1 *)decoder, SUCCEEDED(error) ? S_OK : error);
	*operation = (IInspectable *)&op->iface;
	return S_OK;
}

static const decoder_statics1Vtbl decoder_statics_table = {
	.QueryInterface = part_statics_query,
	.AddRef = part_statics_addref,
	.Release = part_statics_release,
	.GetIids = part_statics_iids,
	.GetRuntimeClassName = part_statics_name,
	.GetTrustLevel = part_statics_trust,
	.get_BmpDecoderId = statics_bmp_id,
	.get_JpegDecoderId = statics_jpeg_id,
	.get_PngDecoderId = statics_png_id,
	.get_TiffDecoderId = statics_tiff_id,
	.get_GifDecoderId = statics_gif_id,
	.get_JpegXRDecoderId = statics_jpeg_xr_id,
	.get_IcoDecoderId = statics_ico_id,
	.GetDecoderInformationEnumerator = statics_enumerator,
	.CreateAsync = statics_create_async,
	.CreateWithIdAsync = statics_create_with_id_async,
};

static struct factory transform_factory_instance = {
	{&activation_table, &transform_factory_instance},
	{&decoder_statics_table, &transform_factory_instance},
	1,
	FALSE,
};

static struct factory decoder_factory_instance = {
	{&activation_table, &decoder_factory_instance},
	{&decoder_statics_table, &decoder_factory_instance},
	1,
	TRUE,
};

HRESULT raven_xaml_imaging_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (length == ARRAYSIZE(class_name) - 1 &&
		CompareStringOrdinal(text, (int)length, class_name, -1, FALSE) == CSTR_EQUAL) {
		InterlockedIncrement(&transform_factory_instance.refs);
		*out = (IActivationFactory *)&transform_factory_instance.activation;
		return S_OK;
	}
	if (length == ARRAYSIZE(decoder_class_name) - 1 &&
		CompareStringOrdinal(text, (int)length, decoder_class_name, -1, FALSE) == CSTR_EQUAL) {
		InterlockedIncrement(&decoder_factory_instance.refs);
		*out = (IActivationFactory *)&decoder_factory_instance.activation;
		return S_OK;
	}
	return CLASS_E_CLASSNOTAVAILABLE;
}
