#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <windows.storage.h>
#include <windows.storage.streams.h>
#include <asyncinfo.h>
#include "windows_graphics_imaging.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);

typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics decoder_statics;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoder decoder1;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrame frame1;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap frame_soft1;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation codec_info1;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIPixelDataProvider provider1;
typedef __x_ABI_CWindows_CGraphics_CImaging_CISoftwareBitmap software1;
typedef __x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform transform1;
typedef __FIAsyncOperation_1_Windows__CGraphics__CImaging__CBitmapDecoder decoder_op_i;
typedef __FIAsyncOperation_1_Windows__CGraphics__CImaging__CBitmapFrame frame_op_i;
typedef __FIAsyncOperation_1_Windows__CGraphics__CImaging__CPixelDataProvider pixel_op_i;
typedef __FIAsyncOperation_1_Windows__CGraphics__CImaging__CSoftwareBitmap software_op_i;
typedef __x_ABI_CWindows_CStorage_CIStorageFile file_i;
typedef __FIAsyncOperation_1_Windows__CStorage__CStorageFile file_op_i;
/* mingw declares no IStorageFileStatics vtable: the one member the fixture uses is hand-spelled at its vtable slot,
 * the way the storage fixture does it. */
static const IID storage_file_statics_iid = {0x5984c710, 0xdaf2, 0x43c8, {0x8b, 0xb4, 0xa4, 0xd3, 0xea, 0xcf, 0xd0, 0x3f}};
struct file_statics_table { void *base[6]; HRESULT (WINAPI *get_path)(void *, HSTRING, file_op_i **); };
struct file_statics { const struct file_statics_table *lpVtbl; };
typedef __FIAsyncOperation_1_Windows__CStorage__CStreams__CIRandomAccessStreamWithContentType ras_op_i;
typedef __x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStreamWithContentType with_type_i;

/* The decoders' identifiers, as wincodec.h gives the WIC decoder classes the runtime's statics answer. */
DEFINE_GUID(expected_bmp_id, 0x6b462062, 0x7cbf, 0x400d, 0x9f, 0xdb, 0x81, 0x3d, 0xd1, 0x0f, 0x27, 0x78);
DEFINE_GUID(expected_jpeg_id, 0x9456a480, 0xe88b, 0x43ea, 0x9e, 0x73, 0x0b, 0x2d, 0x9b, 0x71, 0xb1, 0xca);
DEFINE_GUID(expected_png_id, 0x389ea17b, 0x5078, 0x4cde, 0xb6, 0xef, 0x25, 0xc1, 0x51, 0x75, 0xc7, 0x51);
DEFINE_GUID(expected_tiff_id, 0xb54e85d9, 0xfe23, 0x499f, 0x8b, 0x88, 0x6a, 0xce, 0xa7, 0x13, 0x75, 0x2b);
DEFINE_GUID(expected_gif_id, 0x381dda3c, 0x9ce9, 0x4834, 0xa2, 0x3e, 0x1f, 0x98, 0xf8, 0xfc, 0x52, 0xbe);
DEFINE_GUID(expected_jpeg_xr_id, 0xa26cec36, 0x234c, 0x4950, 0xae, 0x16, 0xe3, 0x4a, 0xac, 0xe7, 0x1d, 0x0d);
DEFINE_GUID(expected_ico_id, 0xc61bfcdf, 0x2e0f, 0x4aad, 0xa8, 0xd7, 0xe0, 0x6b, 0xaf, 0xeb, 0xcd, 0xfe);

/* The parameterized IVectorView`1<BitmapCodecInformation> face the enumerator's result carries: the RFC 4122
 * version-5 UUID (SHA-1) of "pinterface({bbe1fa4c-b0e3-4583-baef-1f1b2e483e56};
 * rc(Windows.Graphics.Imaging.BitmapCodecInformation;{400caaf2-c4b0-4392-a3b0-6f6f9ba95cb4}))" under the WinRT
 * namespace 11f47ad5-7b73-42c0-abae-878b1e16adee. */
DEFINE_GUID(expected_view_iid, 0x97dfde96, 0xff1d, 0x5aa1, 0x86, 0x3a, 0x90, 0x11, 0x6a, 0x31, 0xb8, 0x6b);

/* A 4x2 RGBA PNG the fixture writes to the guest's temp directory and decodes through the storage stream face:
 * pixel (0,0) is RGBA(0,0,16,255) and pixel (3,1) is RGBA(180,120,31,255), so the decoded BGRA bytes identify
 * themselves. */
static const BYTE fixture_png[] = {
	0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
	0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02,
	0x08, 0x06, 0x00, 0x00, 0x00, 0x7f, 0xa8, 0x7d, 0x63, 0x00, 0x00, 0x00,
	0x29, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x05, 0xc1, 0x31, 0x01, 0x00,
	0x30, 0x08, 0xc4, 0xc0, 0x48, 0x60, 0xe9, 0xc2, 0x82, 0x11, 0x84, 0xbd,
	0x30, 0xfc, 0xa5, 0x77, 0x40, 0xb9, 0x3c, 0x43, 0x7b, 0x8c, 0x90, 0x72,
	0xf3, 0x4c, 0xda, 0xcb, 0xf8, 0x01, 0xc2, 0x84, 0x0d, 0x65, 0xfd, 0xf7,
	0x81, 0x19, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42,
	0x60, 0x82,
};

static int same_guid(const GUID *left, const GUID *right)
{
	return memcmp(left, right, sizeof(GUID)) == 0;
}

static HSTRING class_string(const WCHAR *text)
{
	HSTRING value = NULL;
	if (WindowsCreateString(text, lstrlenW(text), &value) != S_OK)
		return NULL;
	return value;
}

static HSTRING transform_key(void)
{
	return class_string(L"Windows.Graphics.Imaging.BitmapTransform");
}

/* The completed handler the operations invoke: the operations finish inside the call, so the handler assigned after
 * creation fires at once; the fixture counts the invocations to prove both. The handler shape is the delegate ABI —
 * query, addref, release, invoke — and raven never queries one. */
struct handler_vtbl {
	HRESULT (WINAPI *query)(void *, REFIID, void **);
	ULONG (WINAPI *addref)(void *);
	ULONG (WINAPI *release)(void *);
	HRESULT (WINAPI *invoke)(void *, void *, AsyncStatus);
};

struct handler {
	const struct handler_vtbl *lpVtbl;
	LONG refs;
	ULONG calls;
	AsyncStatus status;
};

static ULONG WINAPI handler_addref(void *iface)
{
	return (ULONG)InterlockedIncrement(&((struct handler *)iface)->refs);
}

static ULONG WINAPI handler_release(void *iface)
{
	return (ULONG)InterlockedDecrement(&((struct handler *)iface)->refs);
}

static HRESULT WINAPI handler_invoke(void *iface, void *operation, AsyncStatus status)
{
	struct handler *self = (struct handler *)iface;
	(void)operation;
	self->calls++;
	self->status = status;
	return S_OK;
}

static const struct handler_vtbl handler_table = {
	NULL,
	handler_addref,
	handler_release,
	handler_invoke,
};

static struct handler handler_instance = {&handler_table, 1, 0, (AsyncStatus)0};

/* One shape fits every operation the fixture awaits: the IAsyncOperation layout puts the completed slot seventh and
 * GetResults ninth, whatever the result type is. */
static void *await_operation(IUnknown *operation, AsyncStatus *status)
{
	void **table = (void **)*(void **)operation;
	HRESULT (WINAPI *put)(void *, void *) = (HRESULT (WINAPI *)(void *, void *))table[6];
	HRESULT (WINAPI *get_results)(void *, void **) = (HRESULT (WINAPI *)(void *, void **))table[8];
	void *result = NULL;
	handler_instance.calls = 0;
	handler_instance.status = (AsyncStatus)0;
	if (put(operation, &handler_instance) != S_OK)
		return NULL;
	if (handler_instance.calls != 1)
		return NULL;
	if (status)
		*status = handler_instance.status;
	if (get_results(operation, &result) != S_OK)
		return NULL;
	return result;
}

static BOOL write_fixture_png(WCHAR *path, DWORD capacity)
{
	WCHAR temp[MAX_PATH];
	HANDLE file;
	DWORD written = 0;
	UINT32 used = GetTempPathW(MAX_PATH, temp);
	if (!used || used + 40 > MAX_PATH)
		return FALSE;
	lstrcpynW(path, temp, capacity);
	lstrcatW(path, L"raven-bitmap-decode-fixture.png");
	file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return FALSE;
	if (!WriteFile(file, fixture_png, sizeof(fixture_png), &written, NULL) || written != sizeof(fixture_png)) {
		CloseHandle(file);
		return FALSE;
	}
	CloseHandle(file);
	return TRUE;
}

/* The bitmap decoder: the activation hands the class's factory and the statics face the query names; the identifiers
 * are the codecs' real ones, the codec-information enumeration answers the empty set raven's codec state supports,
 * and the stream decoders decode for real through the guest's WIC — the null stream fails the operation's await,
 * and a real stream decodes the fixture PNG into its frames, pixels and software bitmap. */
int raven_entry(void)
{
	static const WCHAR name[] = L"Windows.Graphics.Imaging.BitmapDecoder";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function RoGetActivationFactory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	HSTRING class_key = NULL;
	decoder_statics *statics = NULL;
	GUID id;
	IInspectable *view = NULL;
	void *parameterized = NULL;
	UINT32 size = 0xffffffff;
	IUnknown *operation = NULL;
	AsyncStatus status = (AsyncStatus)0;
	IAsyncInfo *async_info = NULL;
	HRESULT op_error = S_OK;
	if (!RoInitialize || !RoGetActivationFactory) return 20;
	if (FAILED(RoInitialize(1))) return 1;
	if (WindowsCreateString(name, ARRAYSIZE(name) - 1, &class_key) != S_OK || !class_key) return 21;
	if (RoGetActivationFactory(class_key, &IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapDecoderStatics,
		(void **)&statics) != S_OK || !statics) return 2;
	/* the identifiers are the WIC decoder classes the runtime's own statics answer */
	if (statics->lpVtbl->get_BmpDecoderId(statics, &id) != S_OK || !same_guid(&id, &expected_bmp_id)) return 3;
	if (statics->lpVtbl->get_JpegDecoderId(statics, &id) != S_OK || !same_guid(&id, &expected_jpeg_id)) return 4;
	if (statics->lpVtbl->get_PngDecoderId(statics, &id) != S_OK || !same_guid(&id, &expected_png_id)) return 5;
	if (statics->lpVtbl->get_TiffDecoderId(statics, &id) != S_OK || !same_guid(&id, &expected_tiff_id)) return 6;
	if (statics->lpVtbl->get_GifDecoderId(statics, &id) != S_OK || !same_guid(&id, &expected_gif_id)) return 7;
	if (statics->lpVtbl->get_JpegXRDecoderId(statics, &id) != S_OK || !same_guid(&id, &expected_jpeg_xr_id)) return 8;
	if (statics->lpVtbl->get_IcoDecoderId(statics, &id) != S_OK || !same_guid(&id, &expected_ico_id)) return 9;
	/* the codec-information enumeration answers the empty set: no codec registry exists to enumerate */
	if (statics->lpVtbl->GetDecoderInformationEnumerator(statics, &view) != S_OK || !view) return 10;
	/* the parameterized face the projection casts the result for joins on the view */
	if (view->lpVtbl->QueryInterface(view, &expected_view_iid, &parameterized) != S_OK || !parameterized) return 12;
	{
		typedef HRESULT (WINAPI *get_at_function)(IInspectable *, UINT32, IInspectable **);
		typedef HRESULT (WINAPI *get_size_function)(IInspectable *, UINT32 *);
		get_at_function get_at = (get_at_function)(*(void ***)parameterized)[6];
		get_size_function get_size = (get_size_function)(*(void ***)parameterized)[7];
		IInspectable *element = (IInspectable *)1;
		if (get_size((IInspectable *)parameterized, &size) != S_OK || size != 0) return 13;
		if (get_at((IInspectable *)parameterized, 0, &element) != E_BOUNDS || element) return 14;
	}
	((IUnknown *)parameterized)->lpVtbl->Release((IUnknown *)parameterized);
	/* the stream decoders: a null stream still answers an operation, and the operation's await carries the failure
	 * through IAsyncInfo */
	if (statics->lpVtbl->CreateAsync(statics, NULL, (IInspectable **)&operation) != S_OK || !operation) return 15;
	if (await_operation(operation, &status) != NULL || status != Error) return 16;
	if (operation->lpVtbl->QueryInterface(operation, &IID_IAsyncInfo, (void **)&async_info) != S_OK || !async_info) return 17;
	if (async_info->lpVtbl->get_ErrorCode(async_info, &op_error) != S_OK || op_error == S_OK) return 18;
	async_info->lpVtbl->Release(async_info);
	async_info = NULL;
	operation->lpVtbl->Release(operation);
	if (statics->lpVtbl->CreateWithIdAsync(statics, expected_png_id, NULL, (IInspectable **)&operation) != S_OK || !operation) return 22;
	if (await_operation(operation, &status) != NULL || status != Error) return 23;
	operation->lpVtbl->Release(operation);
	/* the runtime class the statics names is the class the activation asked for */
	{
		HSTRING runtime_name = NULL;
		UINT32 length = 0;
		const WCHAR *text;
		if (statics->lpVtbl->GetRuntimeClassName(statics, &runtime_name) != S_OK) return 19;
		text = WindowsGetStringRawBuffer(runtime_name, &length);
		if (length != ARRAYSIZE(name) - 1 || CompareStringOrdinal(text, (int)length, name, -1, FALSE) != CSTR_EQUAL) return 24;
		WindowsDeleteString(runtime_name);
	}

	/* ---- the real decode: a PNG through the storage stream face ---- */
	{
		WCHAR png_path[MAX_PATH];
		HSTRING path_key = NULL;
		file_op_i *file_operation = NULL;
		file_i *file = NULL;
		ras_op_i *ras_operation = NULL;
		with_type_i *stream = NULL;
		decoder1 *decoder = NULL;
		UINT32 frame_count = 0;
		IInspectable *information = NULL;
		frame_op_i *frame_operation = NULL;
		frame1 *frame = NULL;
		UINT32 width = 0, height = 0;
		double dpi_x = 0, dpi_y = 0;
		int32_t pixel_format = 0, alpha_mode = 0;
		pixel_op_i *pixel_operation = NULL;
		provider1 *provider = NULL;
		UINT32 data_length = 0;
		BYTE *pixels = NULL;
		IActivationFactory *transform_factory = NULL;
		IInspectable *transform_inspectable = NULL;
		transform1 *transform = NULL;
		pixel_op_i *transformed_operation = NULL;
		provider1 *transformed_provider = NULL;
		frame_soft1 *soft = NULL;
		software_op_i *software_operation = NULL;
		software1 *bitmap = NULL;
		int32_t software_format = 0, software_alpha = 0;
		int32_t software_width = 0, software_height = 0;

		if (!write_fixture_png(png_path, MAX_PATH)) return 30;
		if (!(path_key = class_string(png_path))) return 31;
		{
			struct file_statics *statics_face = NULL;
			HSTRING file_key = class_string(L"Windows.Storage.StorageFile");
			if (!file_key) return 32;
			if (RoGetActivationFactory(file_key, &storage_file_statics_iid, (void **)&statics_face) != S_OK || !statics_face) return 32;
			WindowsDeleteString(file_key);
			if (statics_face->lpVtbl->get_path(statics_face, path_key, &file_operation) != S_OK || !file_operation) return 33;
		}
		if (!(file = (file_i *)await_operation((IUnknown *)file_operation, &status)) || status != Completed) return 34;
		file_operation->lpVtbl->Release(file_operation);
		/* the stream the decode reads: the stream reference's open read, over raven's file-backed stream */
		{
			__x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStreamReference *reference = NULL;
			if (file->lpVtbl->QueryInterface(file,
				&IID___x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStreamReference, (void **)&reference) != S_OK || !reference) return 35;
			if (reference->lpVtbl->OpenReadAsync(reference, &ras_operation) != S_OK || !ras_operation) return 35;
			reference->lpVtbl->Release(reference);
		}
		if (!(stream = (with_type_i *)await_operation((IUnknown *)ras_operation, &status)) || status != Completed) return 36;
		ras_operation->lpVtbl->Release(ras_operation);
		/* the decode: the operation answers complete, the decoder answers the PNG's one frame */
		if (statics->lpVtbl->CreateAsync(statics, (IUnknown *)stream, (IInspectable **)&operation) != S_OK || !operation) return 37;
		if (!(decoder = (decoder1 *)await_operation(operation, &status)) || status != Completed) return 38;
		operation->lpVtbl->Release(operation);
		if (decoder->lpVtbl->get_FrameCount(decoder, &frame_count) != S_OK || frame_count != 1) return 39;
		/* the decoder information the codec's WIC info answers */
		if (decoder->lpVtbl->get_DecoderInformation(decoder, &information) != S_OK || !information) return 40;
		{
			codec_info1 *info_face = NULL;
			GUID container;
			HSTRING friendly = NULL;
			if (information->lpVtbl->QueryInterface(information,
				&IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapCodecInformation, (void **)&info_face) != S_OK || !info_face) return 41;
			if (info_face->lpVtbl->get_CodecId(info_face, &container) != S_OK) return 42;
			if (info_face->lpVtbl->get_FriendlyName(info_face, &friendly) != S_OK || !friendly) return 43;
			WindowsDeleteString(friendly);
			info_face->lpVtbl->Release(info_face);
		}
		information->lpVtbl->Release(information);
		/* the frame: the PNG's sizes, resolution and pixel format */
		if (decoder->lpVtbl->GetFrameAsync(decoder, 0, (IUnknown **)&frame_operation) != S_OK || !frame_operation) return 44;
		if (!(frame = (frame1 *)await_operation((IUnknown *)frame_operation, &status)) || status != Completed) {
			/* the failure's shape: the WIC frame itself refused, or the operation completed without a result */
			if (status == Error) {
				IAsyncInfo *frame_info = NULL;
				HRESULT frame_error = S_OK;
				if (frame_operation->lpVtbl->QueryInterface(frame_operation, &IID_IAsyncInfo, (void **)&frame_info) == S_OK && frame_info) {
					frame_info->lpVtbl->get_ErrorCode(frame_info, &frame_error);
					frame_info->lpVtbl->Release(frame_info);
				}
				return frame_error == (HRESULT)0x88982f0c ? 145 : frame_error == (HRESULT)0x88982f62 ? 147 : 146; /* NOTINITIALIZED, FRAMEMISSING, other */
			}
			return 45;
		}
		frame_operation->lpVtbl->Release(frame_operation);
		if (frame->lpVtbl->get_OrientedPixelWidth(frame, &width) != S_OK || width != 4) return 46;
		if (frame->lpVtbl->get_OrientedPixelHeight(frame, &height) != S_OK || height != 2) return 47;
		if (frame->lpVtbl->get_PixelWidth(frame, &width) != S_OK || width != 4) return 48;
		if (frame->lpVtbl->get_PixelHeight(frame, &height) != S_OK || height != 2) return 49;
		if (frame->lpVtbl->get_DpiX(frame, &dpi_x) != S_OK || dpi_x != 96.0) return 50;
		if (frame->lpVtbl->get_DpiY(frame, &dpi_y) != S_OK || dpi_y != 96.0) return 51;
		if (frame->lpVtbl->get_BitmapPixelFormat(frame, &pixel_format) != S_OK || pixel_format != BitmapPixelFormat_Bgra8) return 52;
		if (frame->lpVtbl->get_BitmapAlphaMode(frame, &alpha_mode) != S_OK || alpha_mode != BitmapAlphaMode_Straight) return 53;
		/* the pixels: the 4x2 BGRA bytes, first and last identified */
		if (frame->lpVtbl->GetPixelDataAsync(frame, (IUnknown **)&pixel_operation) != S_OK || !pixel_operation) return 54;
		if (!(provider = (provider1 *)await_operation((IUnknown *)pixel_operation, &status)) || status != Completed) return 55;
		pixel_operation->lpVtbl->Release(pixel_operation);
		if (provider->lpVtbl->DetachPixelData(provider, &data_length, &pixels) != S_OK || !pixels) return 56;
		if (data_length != 4 * 2 * 4) return 57;
		if (pixels[0] != 0x10 || pixels[1] != 0x00 || pixels[2] != 0x00 || pixels[3] != 0xff) return 58;
		if (pixels[28] != 0x1f || pixels[29] != 120 || pixels[30] != 180 || pixels[31] != 0xff) return 59;
		if (provider->lpVtbl->DetachPixelData(provider, &data_length, &pixels) == S_OK) return 60;
		provider->lpVtbl->Release(provider);
		/* the transformed pixels: the BitmapTransform's flip and scale reach the decode */
		if (RoGetActivationFactory(transform_key(), &IID_IActivationFactory, (void **)&transform_factory) != S_OK || !transform_factory) return 61;
		if (transform_factory->lpVtbl->ActivateInstance(transform_factory, &transform_inspectable) != S_OK || !transform_inspectable) return 62;
		transform_factory->lpVtbl->Release(transform_factory);
		if (transform_inspectable->lpVtbl->QueryInterface(transform_inspectable,
			&IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapTransform, (void **)&transform) != S_OK || !transform) return 63;
		transform_inspectable->lpVtbl->Release(transform_inspectable);
		if (transform->lpVtbl->put_Flip(transform, 1) != S_OK) return 64;
		if (transform->lpVtbl->put_ScaledWidth(transform, 2) != S_OK) return 65;
		if (transform->lpVtbl->put_ScaledHeight(transform, 1) != S_OK) return 66;
		if (frame->lpVtbl->GetPixelDataTransformedAsync(frame, BitmapPixelFormat_Bgra8, BitmapAlphaMode_Straight,
			(IUnknown *)transform, 0, 0, (IUnknown **)&transformed_operation) != S_OK || !transformed_operation) return 67;
		if (!(transformed_provider = (provider1 *)await_operation((IUnknown *)transformed_operation, &status)) || status != Completed) return 68;
		transformed_operation->lpVtbl->Release(transformed_operation);
		pixels = NULL;
		if (transformed_provider->lpVtbl->DetachPixelData(transformed_provider, &data_length, &pixels) != S_OK || !pixels) return 69;
		if (data_length != 2 * 1 * 4) return 70;
		transformed_provider->lpVtbl->Release(transformed_provider);
		transform->lpVtbl->Release(transform);
		/* the software bitmap: the Bgra8 premultiplied decode the no-argument call answers */
		if (frame->lpVtbl->QueryInterface(frame, &IID__x_ABI_CWindows_CGraphics_CImaging_CIBitmapFrameWithSoftwareBitmap,
			(void **)&soft) != S_OK || !soft) return 71;
		if (soft->lpVtbl->GetSoftwareBitmapAsync(soft, (IUnknown **)&software_operation) != S_OK || !software_operation) return 72;
		if (!(bitmap = (software1 *)await_operation((IUnknown *)software_operation, &status)) || status != Completed) return 73;
		software_operation->lpVtbl->Release(software_operation);
		if (bitmap->lpVtbl->get_BitmapPixelFormat(bitmap, &software_format) != S_OK || software_format != BitmapPixelFormat_Bgra8) return 74;
		if (bitmap->lpVtbl->get_BitmapAlphaMode(bitmap, &software_alpha) != S_OK || software_alpha != BitmapAlphaMode_Premultiplied) return 75;
		if (bitmap->lpVtbl->get_PixelWidth(bitmap, &software_width) != S_OK || software_width != 4) return 76;
		if (bitmap->lpVtbl->get_PixelHeight(bitmap, &software_height) != S_OK || software_height != 2) return 77;
		bitmap->lpVtbl->Release(bitmap);
		soft->lpVtbl->Release(soft);
		frame->lpVtbl->Release(frame);
		decoder->lpVtbl->Release(decoder);
		stream->lpVtbl->Release(stream);
		file->lpVtbl->Release(file);
		DeleteFileW(png_path);
		WindowsDeleteString(path_key);
	}
	WindowsDeleteString(class_key);
	statics->lpVtbl->Release(statics);
	return 0;
}
