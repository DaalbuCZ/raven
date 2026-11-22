/* SPDX-License-Identifier: MIT
 * Windows.Media.Core.CodecQuery: the codec enumeration the app's media path asks for. The activation and the
 * instance exist first; the faces the projection casts for (the instance's ICodecQuery and the factory's
 * ICodecQueryStatics) are served once their IIDs are on record — the refused QIs are traced with full GUIDs so
 * the casts land in the launch log. */
#include "xaml_internal.h"
#include <winstring.h>
#include <string.h>

static const WCHAR codec_query_class[] = L"Windows.Media.Core.CodecQuery";
static const WCHAR codec_subtypes_class[] = L"Windows.Media.Core.CodecSubtypes";

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

static int equal_iid(REFIID left, const IID *right)
{
	return left->Data1 == right->Data1 && left->Data2 == right->Data2 &&
		left->Data3 == right->Data3 && !memcmp(left->Data4, right->Data4, 8);
}
static HRESULT inspectable_iids(ULONG *count, IID **iids)
{
	if (!count || !iids)
		return E_POINTER;
	*iids = (IID *)HeapAlloc(GetProcessHeap(), 0, 2 * sizeof(IID));
	if (!*iids)
		return E_OUTOFMEMORY;
	**iids = IID_IUnknown;
	(*iids)[1] = IID_IInspectable;
	*count = 2;
	return S_OK;
}
static HRESULT inspectable_trust(TrustLevel *level)
{
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}
/* TEMPORARY diagnosis: the refused casts name the faces still to serve, with the object kind that refused
 * them. Removed after the round. */
static void trace_refused_iid(const char *who, REFIID iid)
{
	char note[112];
	wsprintfA(note, "raven-xaml: codec %s has no interface %08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
		who, (unsigned long)iid->Data1, iid->Data2, iid->Data3, iid->Data4[0], iid->Data4[1], iid->Data4[2],
		iid->Data4[3], iid->Data4[4], iid->Data4[5], iid->Data4[6], iid->Data4[7]);
	raven_trace_note(note);
}

/* The statics face the projection casts the factory for (the IID the refused-QI trace recorded):
 * ICodecQueryStatics.FindAllAsync(kind, category) over the empty codec list. The enumeration itself is honest
 * here in exactly one way — raven ships no codec registry, so the result is an empty set; the async-shaped
 * object answers identity and traces the cast IIDs still to serve. */
static const IID codec_statics_iid = {0xecc8691b, 0xc1db, 0x4dc0, {0x85, 0x5e, 0x65, 0xf6, 0xc5, 0x51, 0xaf, 0x49}};
/* the IAsyncOperation`1<IVectorView`1<CodecInfo>> face the .NET await casts the FindAllAsync result for
 * (the IID from the traced vector QI refusal) */
static const IID codec_instance_findall_iid = {0x222a953a, 0xaf61, 0x4e04, {0x80, 0x8a, 0xa4, 0x63, 0x4e, 0x2f, 0x3a, 0xc4}};
/* the parameterized IAsyncOperation`1<IVectorView`1<CodecInfo>> face the .NET await casts the FindAllAsync
 * return for (the IID the traced launch shows the operation QI refused, the refusal the caller's
 * NullReferenceException followed) */
static const IID codec_operation_parameterized_iid = {0xc1c59efc, 0x71df, 0x5534, {0xb0, 0x3f, 0xe1, 0x8a, 0x8f, 0xbe, 0x78, 0xbb}};
static const IID codec_subtypes_statics_iid = {0xa66ac4f2, 0x888b, 0x4224, {0x8c, 0xf6, 0x2a, 0x8d, 0x4e, 0xb0, 0x23, 0x82}};
static const IID codec_subtypes_statics2_iid = {0x222a953a, 0xaf61, 0x4e04, {0x80, 0x8a, 0xa4, 0x63, 0x4e, 0x2f, 0x3a, 0xc4}};
static const IID codec_subtypes_statics_ecc_iid = {0xecc8691b, 0xc1db, 0x4dc0, {0x85, 0x5e, 0x65, 0xf6, 0xc5, 0x51, 0xaf, 0x49}};
static const WCHAR codec_subtypes_class_name[] = L"Windows.Media.Core.CodecSubtypes";

/* The statics face: ICodecQueryStatics with its one FindAllAsync over (kind, category). The enumeration is
 * honest in exactly one way — raven ships no codec registry, so the result set is empty; the async-shaped
 * result answers identity and traces the cast IIDs still to serve. */
typedef struct codec_statics_i codec_statics_i;
typedef struct codec_statics_vtbl codec_statics_vtbl;
struct codec_statics_vtbl {
	BEGIN_INTERFACE
	HRESULT (WINAPI *QueryInterface)(codec_statics_i *This, REFIID riid, void **object);
	ULONG (WINAPI *AddRef)(codec_statics_i *This);
	ULONG (WINAPI *Release)(codec_statics_i *This);
	HRESULT (WINAPI *GetIids)(codec_statics_i *This, ULONG *count, IID **iids);
	HRESULT (WINAPI *GetRuntimeClassName)(codec_statics_i *This, HSTRING *name);
	HRESULT (WINAPI *GetTrustLevel)(codec_statics_i *This, TrustLevel *level);
	HRESULT (WINAPI *FindAllAsync)(codec_statics_i *This, uint64_t kind, uint64_t category, IInspectable **operation);
	END_INTERFACE
};
interface __x_ABI_CWindows_CMedia_CCore_CICodecQueryStatics { CONST_VTBL codec_statics_vtbl *lpVtbl; };

static const WCHAR codec_operation_class[] = L"Windows.Foundation.IAsyncOperation`1<Windows.Media.Core.CodecInfo>";



/* the empty result set: the parameterized IVectorView`1<CodecInfo> face the await's continuation casts the
 * GetResults return for. The IID is the one the traced launch shows the vector QI refusing — read out of the
 * app's compiled-in constant, and a proper sha-1 version-5/variant-10 parameterized GUID. The face follows
 * the generic IVectorView`1 definition's slot order after IInspectable — GetAt 6, get_Size 7, IndexOf 8,
 * GetMany 9 — without the inherited IIterable`1.First, the same shape mingw's generated vector vtables carry.
 * The set is empty, so get_Size answers 0, GetAt answers E_BOUNDS, IndexOf answers not-found, GetMany answers
 * zero elements. */
typedef struct codec_vector_view_vtbl codec_vector_view_vtbl;
struct codec_vector_view_vtbl {
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
static const IID codec_vector_parameterized_iid = {0x1fd24a93, 0xc3da, 0x513d, {0xb2, 0x16, 0x0c, 0x4c, 0x7f, 0x41, 0x2b, 0x88}};
typedef struct codec_vector codec_vector;
typedef struct codec_vector_part { const void *lpVtbl; codec_vector *owner; } codec_vector_part;
struct codec_vector {
	codec_vector_part inspectable;
	const WCHAR *class_name;
	LONG refs;
};
static LONG codec_vectors;
static HRESULT WINAPI codec_vector_query(IInspectable *iface, REFIID iid, void **out)
{
	codec_vector *self = ((codec_vector_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IAgileObject) ||
		equal_iid(iid, &codec_vector_parameterized_iid))
		*out = &self->inspectable;
	else {
		trace_refused_iid("vector", iid);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
	return S_OK;
}
static ULONG WINAPI codec_vector_addref(IInspectable *iface)
{ return (ULONG)InterlockedIncrement(&((codec_vector_part *)iface)->owner->refs); }
static ULONG WINAPI codec_vector_release(IInspectable *iface)
{
	codec_vector *self = ((codec_vector_part *)iface)->owner;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		FREE(self);
		InterlockedDecrement(&codec_vectors);
	}
	return (ULONG)refs;
}
static HRESULT WINAPI codec_vector_iids(IInspectable *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI codec_vector_name(IInspectable *iface, HSTRING *out)
{
	codec_vector *self = ((codec_vector_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	return WindowsCreateString(self->class_name, (UINT32)lstrlenW(self->class_name), out);
}
static HRESULT WINAPI codec_vector_trust(IInspectable *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static HRESULT WINAPI codec_vector_get_at(IInspectable *iface, UINT32 index, IInspectable **value)
{
	(void)iface; (void)index;
	if (!value)
		return E_POINTER;
	*value = NULL;
	return E_BOUNDS;
}
static HRESULT WINAPI codec_vector_get_size(IInspectable *iface, UINT32 *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = 0;
	return S_OK;
}
static HRESULT WINAPI codec_vector_index_of(IInspectable *iface, IInspectable *element, UINT32 *index, boolean *found)
{
	(void)iface; (void)element;
	if (!index || !found)
		return E_POINTER;
	*index = 0;
	*found = 0;
	return S_OK;
}
static HRESULT WINAPI codec_vector_get_many(IInspectable *iface, UINT32 start_index, UINT32 items_size, IInspectable **items, UINT32 *value)
{
	(void)iface; (void)start_index; (void)items; (void)items_size;
	if (!value)
		return E_POINTER;
	*value = 0;
	return S_OK;
}
static const codec_vector_view_vtbl codec_vector_table = {
	codec_vector_query, codec_vector_addref, codec_vector_release,
	codec_vector_iids, codec_vector_name, codec_vector_trust,
	codec_vector_get_at, codec_vector_get_size, codec_vector_index_of, codec_vector_get_many,
};
static IInspectable *codec_vector_new(const WCHAR *class_name)
{
	codec_vector *self = ALLOC(sizeof *self);
	if (!self)
		return NULL;
	self->inspectable.lpVtbl = &codec_vector_table;
	self->inspectable.owner = self;
	self->class_name = class_name;
	self->refs = 1;
	InterlockedIncrement(&codec_vectors);
	return (IInspectable *)&self->inspectable;
}

/* ICodecSubtypesStatics2 (the IID the traced launch shows the projection casting next): one member,
 * get_VideoFormatAv1 — the same empty-string answer. */
typedef struct codec_subtypes_statics2_i codec_subtypes_statics2_i;
typedef struct codec_subtypes_statics2_vtbl codec_subtypes_statics2_vtbl;
struct codec_subtypes_statics2_vtbl {
	BEGIN_INTERFACE
	HRESULT (WINAPI *QueryInterface)(codec_subtypes_statics2_i *This, REFIID riid, void **object);
	ULONG (WINAPI *AddRef)(codec_subtypes_statics2_i *This);
	ULONG (WINAPI *Release)(codec_subtypes_statics2_i *This);
	HRESULT (WINAPI *GetIids)(codec_subtypes_statics2_i *This, ULONG *count, IID **iids);
	HRESULT (WINAPI *GetRuntimeClassName)(codec_subtypes_statics2_i *This, HSTRING *name);
	HRESULT (WINAPI *GetTrustLevel)(codec_subtypes_statics2_i *This, TrustLevel *level);
	HRESULT (WINAPI *get_VideoFormatAv1)(codec_subtypes_statics2_i *This, HSTRING *value);
	END_INTERFACE
};
interface __x_ABI_CWindows_CMedia_CCore_CICodecSubtypesStatics2 { CONST_VTBL codec_subtypes_statics2_vtbl *lpVtbl; };
typedef struct codec_subtypes_statics2_object { const codec_subtypes_statics2_vtbl *lpVtbl; } codec_subtypes_statics2_object;
static HRESULT WINAPI codec_subtypes2_query(codec_subtypes_statics2_i *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &codec_subtypes_statics2_iid))
		*out = iface;
	else {
		trace_refused_iid("subtypes2 statics", iid);
		return E_NOINTERFACE;
	}
	return S_OK;
}
static ULONG WINAPI codec_subtypes2_addref(codec_subtypes_statics2_i *iface) { (void)iface; return 2; }
static ULONG WINAPI codec_subtypes2_release(codec_subtypes_statics2_i *iface) { (void)iface; return 1; }
static HRESULT WINAPI codec_subtypes2_iids(codec_subtypes_statics2_i *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI codec_subtypes2_name(codec_subtypes_statics2_i *iface, HSTRING *out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	return WindowsCreateString(codec_subtypes_class_name, (UINT32)lstrlenW(codec_subtypes_class_name), out);
}
static HRESULT WINAPI codec_subtypes2_trust(codec_subtypes_statics2_i *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static HRESULT WINAPI codec_subtypes2_get_av1(codec_subtypes_statics2_i *iface, HSTRING *value)
{ (void)iface; if (!value) return E_POINTER; *value = NULL; return S_OK; }
static const codec_subtypes_statics2_vtbl codec_subtypes_statics2_table = {
	codec_subtypes2_query, codec_subtypes2_addref, codec_subtypes2_release,
	codec_subtypes2_iids, codec_subtypes2_name, codec_subtypes2_trust, codec_subtypes2_get_av1,
};
static codec_subtypes_statics2_object codec_subtypes_statics2_instance = {&codec_subtypes_statics2_table};

/* The CodecSubtypes statics face: its IID came from the refused RoGetActivationFactory cast in the traced
 * launch, and its member list comes from the system Windows.Media.winmd (parsed from the WinSxS metadata): the
 * interface is 51 string properties — the FOURCC/GUID name of every video and audio format the platform names —
 * in metadata order, plus get_VideoFormatAv1 on ICodecSubtypesStatics2. raven ships no codec registry, so every
 * property answers an empty string: the honest value when the format has no codec behind it. The vtable order is
 * the metadata order, which is the projection's slot order. */

typedef struct codec_subtypes_statics_i codec_subtypes_statics_i;
typedef struct codec_subtypes_statics_vtbl codec_subtypes_statics_vtbl;
struct codec_subtypes_statics_vtbl {
	BEGIN_INTERFACE
	HRESULT (WINAPI *QueryInterface)(codec_subtypes_statics_i *This, REFIID riid, void **object);
	ULONG (WINAPI *AddRef)(codec_subtypes_statics_i *This);
	ULONG (WINAPI *Release)(codec_subtypes_statics_i *This);
	HRESULT (WINAPI *GetIids)(codec_subtypes_statics_i *This, ULONG *count, IID **iids);
	HRESULT (WINAPI *GetRuntimeClassName)(codec_subtypes_statics_i *This, HSTRING *name);
	HRESULT (WINAPI *GetTrustLevel)(codec_subtypes_statics_i *This, TrustLevel *level);
	HRESULT (WINAPI *get_VideoFormatDV25)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatDV50)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatDvc)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatDvh1)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatDvhD)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatDvsd)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatDvsl)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatH263)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatH264)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatH265)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatH264ES)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatHevc)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatHevcES)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatM4S2)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatMjpg)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatMP43)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatMP4S)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatMP4V)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatMpeg2)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatVP80)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatVP90)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatMpg1)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatMss1)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatMss2)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatWmv1)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatWmv2)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatWmv3)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormatWvc1)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_VideoFormat420O)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatAac)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatAdts)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatAlac)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatAmrNB)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatAmrWB)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatAmrWP)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatDolbyAC3)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatDolbyAC3Spdif)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatDolbyDDPlus)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatDrm)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatDts)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatFlac)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatFloat)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatMP3)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatMPeg)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatMsp1)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatOpus)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatPcm)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatWmaSpdif)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatWMAudioLossless)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatWMAudioV8)(codec_subtypes_statics_i *This, HSTRING *value);
	HRESULT (WINAPI *get_AudioFormatWMAudioV9)(codec_subtypes_statics_i *This, HSTRING *value);
	END_INTERFACE
};
interface __x_ABI_CWindows_CMedia_CCore_CICodecSubtypesStatics { CONST_VTBL codec_subtypes_statics_vtbl *lpVtbl; };

typedef struct codec_subtypes_statics_object { const codec_subtypes_statics_vtbl *lpVtbl; } codec_subtypes_statics_object;
static HRESULT WINAPI codec_subtypes_query(codec_subtypes_statics_i *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &codec_subtypes_statics_iid))
		*out = iface;
	else if (equal_iid(iid, &codec_subtypes_statics2_iid))
		*out = (codec_subtypes_statics2_i *)&codec_subtypes_statics2_instance;
	else if (equal_iid(iid, &codec_subtypes_statics_ecc_iid))
		*out = (codec_subtypes_statics2_i *)&codec_subtypes_statics2_instance;
	else {
		trace_refused_iid("subtypes statics", iid);
		return E_NOINTERFACE;
	}
	return S_OK;
}
static ULONG WINAPI codec_subtypes_addref(codec_subtypes_statics_i *iface) { (void)iface; return 2; }
static ULONG WINAPI codec_subtypes_release(codec_subtypes_statics_i *iface) { (void)iface; return 1; }
static HRESULT WINAPI codec_subtypes_iids(codec_subtypes_statics_i *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI codec_subtypes_name(codec_subtypes_statics_i *iface, HSTRING *out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	return WindowsCreateString(codec_subtypes_class_name, (UINT32)lstrlenW(codec_subtypes_class_name), out);
}
static HRESULT WINAPI codec_subtypes_trust(codec_subtypes_statics_i *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }

/* Every format property: no codec registry means no format GUID to hand out — the empty string. */
#define CODEC_FORMAT_GETTER(name) \
	static HRESULT WINAPI codec_subtypes_##name(codec_subtypes_statics_i *iface, HSTRING *value) \
	{ (void)iface; if (!value) return E_POINTER; *value = NULL; return S_OK; }
CODEC_FORMAT_GETTER(get_VideoFormatDV25)
CODEC_FORMAT_GETTER(get_VideoFormatDV50)
CODEC_FORMAT_GETTER(get_VideoFormatDvc)
CODEC_FORMAT_GETTER(get_VideoFormatDvh1)
CODEC_FORMAT_GETTER(get_VideoFormatDvhD)
CODEC_FORMAT_GETTER(get_VideoFormatDvsd)
CODEC_FORMAT_GETTER(get_VideoFormatDvsl)
CODEC_FORMAT_GETTER(get_VideoFormatH263)
CODEC_FORMAT_GETTER(get_VideoFormatH264)
CODEC_FORMAT_GETTER(get_VideoFormatH265)
CODEC_FORMAT_GETTER(get_VideoFormatH264ES)
CODEC_FORMAT_GETTER(get_VideoFormatHevc)
CODEC_FORMAT_GETTER(get_VideoFormatHevcES)
CODEC_FORMAT_GETTER(get_VideoFormatM4S2)
CODEC_FORMAT_GETTER(get_VideoFormatMjpg)
CODEC_FORMAT_GETTER(get_VideoFormatMP43)
CODEC_FORMAT_GETTER(get_VideoFormatMP4S)
CODEC_FORMAT_GETTER(get_VideoFormatMP4V)
CODEC_FORMAT_GETTER(get_VideoFormatMpeg2)
CODEC_FORMAT_GETTER(get_VideoFormatVP80)
CODEC_FORMAT_GETTER(get_VideoFormatVP90)
CODEC_FORMAT_GETTER(get_VideoFormatMpg1)
CODEC_FORMAT_GETTER(get_VideoFormatMss1)
CODEC_FORMAT_GETTER(get_VideoFormatMss2)
CODEC_FORMAT_GETTER(get_VideoFormatWmv1)
CODEC_FORMAT_GETTER(get_VideoFormatWmv2)
CODEC_FORMAT_GETTER(get_VideoFormatWmv3)
CODEC_FORMAT_GETTER(get_VideoFormatWvc1)
CODEC_FORMAT_GETTER(get_VideoFormat420O)
CODEC_FORMAT_GETTER(get_AudioFormatAac)
CODEC_FORMAT_GETTER(get_AudioFormatAdts)
CODEC_FORMAT_GETTER(get_AudioFormatAlac)
CODEC_FORMAT_GETTER(get_AudioFormatAmrNB)
CODEC_FORMAT_GETTER(get_AudioFormatAmrWB)
CODEC_FORMAT_GETTER(get_AudioFormatAmrWP)
CODEC_FORMAT_GETTER(get_AudioFormatDolbyAC3)
CODEC_FORMAT_GETTER(get_AudioFormatDolbyAC3Spdif)
CODEC_FORMAT_GETTER(get_AudioFormatDolbyDDPlus)
CODEC_FORMAT_GETTER(get_AudioFormatDrm)
CODEC_FORMAT_GETTER(get_AudioFormatDts)
CODEC_FORMAT_GETTER(get_AudioFormatFlac)
CODEC_FORMAT_GETTER(get_AudioFormatFloat)
CODEC_FORMAT_GETTER(get_AudioFormatMP3)
CODEC_FORMAT_GETTER(get_AudioFormatMPeg)
CODEC_FORMAT_GETTER(get_AudioFormatMsp1)
CODEC_FORMAT_GETTER(get_AudioFormatOpus)
CODEC_FORMAT_GETTER(get_AudioFormatPcm)
CODEC_FORMAT_GETTER(get_AudioFormatWmaSpdif)
CODEC_FORMAT_GETTER(get_AudioFormatWMAudioLossless)
CODEC_FORMAT_GETTER(get_AudioFormatWMAudioV8)
CODEC_FORMAT_GETTER(get_AudioFormatWMAudioV9)
#undef CODEC_FORMAT_GETTER
static const codec_subtypes_statics_vtbl codec_subtypes_statics_table = {
	codec_subtypes_query, codec_subtypes_addref, codec_subtypes_release,
	codec_subtypes_iids, codec_subtypes_name, codec_subtypes_trust,
	codec_subtypes_get_VideoFormatDV25, codec_subtypes_get_VideoFormatDV50,
	codec_subtypes_get_VideoFormatDvc, codec_subtypes_get_VideoFormatDvh1,
	codec_subtypes_get_VideoFormatDvhD, codec_subtypes_get_VideoFormatDvsd,
	codec_subtypes_get_VideoFormatDvsl, codec_subtypes_get_VideoFormatH263,
	codec_subtypes_get_VideoFormatH264, codec_subtypes_get_VideoFormatH265,
	codec_subtypes_get_VideoFormatH264ES, codec_subtypes_get_VideoFormatHevc,
	codec_subtypes_get_VideoFormatHevcES, codec_subtypes_get_VideoFormatM4S2,
	codec_subtypes_get_VideoFormatMjpg, codec_subtypes_get_VideoFormatMP43,
	codec_subtypes_get_VideoFormatMP4S, codec_subtypes_get_VideoFormatMP4V,
	codec_subtypes_get_VideoFormatMpeg2, codec_subtypes_get_VideoFormatVP80,
	codec_subtypes_get_VideoFormatVP90, codec_subtypes_get_VideoFormatMpg1,
	codec_subtypes_get_VideoFormatMss1, codec_subtypes_get_VideoFormatMss2,
	codec_subtypes_get_VideoFormatWmv1, codec_subtypes_get_VideoFormatWmv2,
	codec_subtypes_get_VideoFormatWmv3, codec_subtypes_get_VideoFormatWvc1,
	codec_subtypes_get_VideoFormat420O, codec_subtypes_get_AudioFormatAac,
	codec_subtypes_get_AudioFormatAdts, codec_subtypes_get_AudioFormatAlac,
	codec_subtypes_get_AudioFormatAmrNB, codec_subtypes_get_AudioFormatAmrWB,
	codec_subtypes_get_AudioFormatAmrWP, codec_subtypes_get_AudioFormatDolbyAC3,
	codec_subtypes_get_AudioFormatDolbyAC3Spdif, codec_subtypes_get_AudioFormatDolbyDDPlus,
	codec_subtypes_get_AudioFormatDrm, codec_subtypes_get_AudioFormatDts,
	codec_subtypes_get_AudioFormatFlac, codec_subtypes_get_AudioFormatFloat,
	codec_subtypes_get_AudioFormatMP3, codec_subtypes_get_AudioFormatMPeg,
	codec_subtypes_get_AudioFormatMsp1, codec_subtypes_get_AudioFormatOpus,
	codec_subtypes_get_AudioFormatPcm, codec_subtypes_get_AudioFormatWmaSpdif,
	codec_subtypes_get_AudioFormatWMAudioLossless, codec_subtypes_get_AudioFormatWMAudioV8,
	codec_subtypes_get_AudioFormatWMAudioV9,
};
static codec_subtypes_statics_object codec_subtypes_statics_instance = {&codec_subtypes_statics_table};


typedef HRESULT (WINAPI *QueryInterface_t)(void *, REFIID, void **);
typedef ULONG (WINAPI *AddRef_t)(void *);
typedef ULONG (WINAPI *Release_t)(void *);
typedef HRESULT (WINAPI *CompletedPut_t)(void *, void *);
typedef HRESULT (WINAPI *CompletedGet_t)(void *, void **);
typedef struct codec_operation_handler codec_operation_handler;
static void codec_operation_handler_release(codec_operation_handler *handler);
typedef struct codec_operation_operation_vtbl codec_operation_operation_vtbl;
struct codec_operation_operation_vtbl {
	BEGIN_INTERFACE
	HRESULT (WINAPI *QueryInterface)(void *This, REFIID riid, void **object);
	ULONG (WINAPI *AddRef)(void *This);
	ULONG (WINAPI *Release)(void *This);
	HRESULT (WINAPI *GetIids)(void *This, ULONG *count, IID **iids);
	HRESULT (WINAPI *GetRuntimeClassName)(void *This, HSTRING *name);
	HRESULT (WINAPI *GetTrustLevel)(void *This, TrustLevel *level);
	HRESULT (WINAPI *put_Completed)(void *This, void *handler);
	HRESULT (WINAPI *get_Completed)(void *This, void **handler);
	HRESULT (WINAPI *GetResults)(void *This, IInspectable **results);
	END_INTERFACE
};
typedef struct codec_operation_info_vtbl codec_operation_info_vtbl;
struct codec_operation_info_vtbl {
	BEGIN_INTERFACE
	HRESULT (WINAPI *QueryInterface)(void *This, REFIID riid, void **object);
	ULONG (WINAPI *AddRef)(void *This);
	ULONG (WINAPI *Release)(void *This);
	HRESULT (WINAPI *GetIids)(void *This, ULONG *count, IID **iids);
	HRESULT (WINAPI *GetRuntimeClassName)(void *This, HSTRING *name);
	HRESULT (WINAPI *GetTrustLevel)(void *This, TrustLevel *level);
	HRESULT (WINAPI *get_Id)(void *This, UINT32 *value);
	HRESULT (WINAPI *get_Status)(void *This, AsyncStatus *value);
	HRESULT (WINAPI *get_ErrorCode)(void *This, HRESULT *value);
	HRESULT (WINAPI *Cancel)(void *This);
	HRESULT (WINAPI *Close)(void *This);
	END_INTERFACE
};

/* The completed operation, two faces over one object. The parameterized IAsyncOperation`1
 * <IVectorView`1<CodecInfo>> face the .NET await drives carries ONLY its own three members after
 * IInspectable — put_Completed 6, get_Completed 7, GetResults 8; IAsyncInfo is a separate face
 * (get_Id 6, get_Status 7, get_ErrorCode 8, Cancel 9, Close 10) the projection reaches by QI, not by
 * slot. Serving both casts with one flattened table put the info members where the await reads the
 * completed handler, so the await's GetResults landed on get_ErrorCode and the awaited result read
 * back null — the app's NullReferenceException. GetResults hands the empty IVectorView`1<string>; the
 * vector's parameterized face joins when its cast IID lands in the trace. */
typedef struct codec_operation_handler codec_operation_handler;
typedef struct codec_operation codec_operation;
typedef struct codec_operation_part { const void *lpVtbl; codec_operation *owner; } codec_operation_part;
struct codec_operation {
	codec_operation_part parameterized;
	codec_operation_part asyncinfo;
	LONG refs;
	IInspectable *result;
	codec_operation_handler *completed;
};
static LONG codec_operations;
static codec_operation *codec_operation_owner(void *iface) { return ((codec_operation_part *)iface)->owner; }
static HRESULT WINAPI codec_operation_query(void *iface, REFIID iid, void **out)
{
	codec_operation *self = codec_operation_owner(iface);
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IAgileObject) ||
		equal_iid(iid, &codec_instance_findall_iid) || equal_iid(iid, &codec_operation_parameterized_iid)) {
		*out = &self->parameterized;
	} else if (equal_iid(iid, &IID_IAsyncInfo)) {
		*out = &self->asyncinfo;
	} else {
		trace_refused_iid("operation", iid);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
	return S_OK;
}
static ULONG WINAPI codec_operation_addref(void *iface)
{ return (ULONG)InterlockedIncrement(&codec_operation_owner(iface)->refs); }
static ULONG WINAPI codec_operation_release(void *iface)
{
	codec_operation *self = codec_operation_owner(iface);
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->result) self->result->lpVtbl->Release(self->result);
		if (self->completed) codec_operation_handler_release(self->completed);
		FREE(self);
		InterlockedDecrement(&codec_operations);
	}
	return (ULONG)refs;
}
static HRESULT WINAPI codec_operation_iids(void *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI codec_operation_name(void *iface, HSTRING *out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	return WindowsCreateString(codec_operation_class, (UINT32)lstrlenW(codec_operation_class), out);
}
static HRESULT WINAPI codec_operation_trust(void *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static HRESULT WINAPI codec_operation_get_id(void *iface, UINT32 *out)
{ (void)iface; if (!out) return E_POINTER; *out = 1; return S_OK; }
static HRESULT WINAPI codec_operation_get_status(void *iface, AsyncStatus *out)
{ (void)iface; if (!out) return E_POINTER; *out = Completed; return S_OK; }
static HRESULT WINAPI codec_operation_get_error(void *iface, HRESULT *out)
{ (void)iface; if (!out) return E_POINTER; *out = S_OK; return S_OK; }
static HRESULT WINAPI codec_operation_cancel(void *iface)
{ (void)iface; raven_trace_note("raven-xaml: codec operation Cancel\n"); return S_OK; }
static HRESULT WINAPI codec_operation_close(void *iface)
{ (void)iface; return S_OK; }
/* the completed-handler convention: the handler is an IUnknown whose fourth slot is Invoke(asyncOp, status);
 * the operation is already terminal, so put_Completed invokes it synchronously */
static void codec_operation_handler_release(codec_operation_handler *handler)
{ ((IUnknown *)handler)->lpVtbl->Release((IUnknown *)handler); }
static HRESULT WINAPI codec_operation_put_completed(void *iface, codec_operation_handler *handler)
{
	codec_operation *self = codec_operation_owner(iface);
	char note[96];
	wsprintfA(note, "raven-xaml: codec operation put_completed handler=%p\n", (void *)handler);
	raven_trace_note(note);
	if (handler) {
		void **vtable = *(void ***)(uintptr_t)handler;
		void (*invoke)(codec_operation_handler *, IUnknown *, AsyncStatus) =
			(void (*)(codec_operation_handler *, IUnknown *, AsyncStatus))vtable[3];
		invoke(handler, iface, Completed);
	}
	return S_OK;
}
static HRESULT WINAPI codec_operation_get_completed(void *iface, codec_operation_handler **handler)
{
	codec_operation *self = codec_operation_owner(iface);
	(void)iface;
	if (!handler)
		return E_POINTER;
	*handler = self->completed;
	raven_trace_note("raven-xaml: codec operation get_completed answered\n");
	return S_OK;
}
static HRESULT WINAPI codec_operation_get_results(void *iface, IInspectable **results)
{
	codec_operation *self = codec_operation_owner(iface);
	if (!results)
		return E_POINTER;
	*results = NULL;
	if (!self->result) {
		self->result = codec_vector_new(L"Windows.Foundation.Collections.IVectorView`1<Windows.Media.Core.CodecInfo>");
		if (!self->result)
			return E_OUTOFMEMORY;
	}
	*results = self->result;
	(*results)->lpVtbl->AddRef(*results);
	raven_trace_note("raven-xaml: codec operation GetResults answered the empty vector\n");
	return S_OK;
}
static const codec_operation_operation_vtbl codec_operation_table = {
	(QueryInterface_t)codec_operation_query, (AddRef_t)codec_operation_addref, (Release_t)codec_operation_release,
	codec_operation_iids, codec_operation_name, codec_operation_trust,
	(CompletedPut_t)codec_operation_put_completed, (CompletedGet_t)codec_operation_get_completed,
	codec_operation_get_results,
};
static const codec_operation_info_vtbl codec_operation_info_table = {
	(QueryInterface_t)codec_operation_query, (AddRef_t)codec_operation_addref, (Release_t)codec_operation_release,
	codec_operation_iids, codec_operation_name, codec_operation_trust,
	codec_operation_get_id, codec_operation_get_status, codec_operation_get_error,
	codec_operation_cancel, codec_operation_close,
};

static codec_operation *codec_operation_new(void)
{
	codec_operation *self = ALLOC(sizeof *self);
	if (!self)
		return NULL;
	self->parameterized.lpVtbl = (const void *)&codec_operation_table;
	self->parameterized.owner = self;
	self->asyncinfo.lpVtbl = (const void *)&codec_operation_info_table;
	self->asyncinfo.owner = self;
	self->refs = 1;
	InterlockedIncrement(&codec_operations);
	return self;
}

typedef struct codec_statics_object { const codec_statics_vtbl *lpVtbl; } codec_statics_object;
static HRESULT WINAPI codec_statics_query(codec_statics_i *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &codec_statics_iid))
		*out = iface;
	else {
		trace_refused_iid("codecquery statics", iid);
		return E_NOINTERFACE;
	}
	return S_OK;
}
static ULONG WINAPI codec_statics_addref(codec_statics_i *iface) { (void)iface; return 2; }
static ULONG WINAPI codec_statics_release(codec_statics_i *iface) { (void)iface; return 1; }
static HRESULT WINAPI codec_statics_iids(codec_statics_i *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI codec_statics_name(codec_statics_i *iface, HSTRING *out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	return WindowsCreateString(codec_query_class, (UINT32)lstrlenW(codec_query_class), out);
}
static HRESULT WINAPI codec_statics_trust(codec_statics_i *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static HRESULT WINAPI codec_statics_find_all(codec_statics_i *iface, uint64_t kind, uint64_t category, IInspectable **operation)
{
	/* raven ships no codec registry: every query answers an empty result set. The async-shaped object carries
	 * the identity the projection casts for; its parameterized faces join when their IIDs are traced. */
	(void)iface; (void)kind; (void)category;
	if (!operation)
		return E_POINTER;
	codec_operation *operation_object = codec_operation_new();
	if (!operation_object)
		return E_OUTOFMEMORY;
	*operation = (IInspectable *)&operation_object->parameterized;
	{
		char note[96];
		wsprintfA(note, "raven-xaml: codec FindAllAsync kind=%llu category=%llu\n",
			(unsigned long long)kind, (unsigned long long)category);
		raven_trace_note(note);
	}
	return S_OK;
}
static const codec_statics_vtbl codec_statics_table = {
	codec_statics_query, codec_statics_addref, codec_statics_release,
	codec_statics_iids, codec_statics_name, codec_statics_trust, codec_statics_find_all,
};
static codec_statics_object codec_statics_instance = {&codec_statics_table};
typedef struct codec_query_object codec_query_object;
typedef struct codec_query_iids_part { const IInspectableVtbl *lpVtbl; codec_query_object *owner; } codec_query_iids_part;
struct codec_query_object {
	codec_query_iids_part inspectable;
	LONG refs;
	CRITICAL_SECTION lock;
};
static LONG codec_objects;

/* FindAllAsync's ABI, decoded from the system winmd (MethodDef row 2749, sig
 * "20 03 15 12 99b1 01 15 12 99c1 01 12 8825 11 8829 11 8821 0e"):
 * FindAllAsync(IIterable`1<CodecInfo>, CodecKind, CodecCategory) -> IVectorView`1<string>
 * — three parameters, so the vtable declares five slots: this + kind + category + subCategory
 * (HSTRING, may legitimately be NULL) + the retval out-pointer, which rides the STACK as the
 * fifth argument. The MCG calls the three-parameter overload with subCategory=NULL in r9; a null
 * subCategory is a value, never an E_POINTER condition. */
typedef struct codec_instance_findall_vtbl codec_instance_findall_vtbl;
struct codec_instance_findall_vtbl {
	BEGIN_INTERFACE
	HRESULT (WINAPI *QueryInterface)(codec_subtypes_statics2_i *This, REFIID riid, void **object);
	ULONG (WINAPI *AddRef)(codec_subtypes_statics2_i *This);
	ULONG (WINAPI *Release)(codec_subtypes_statics2_i *This);
	HRESULT (WINAPI *GetIids)(codec_subtypes_statics2_i *This, ULONG *count, IID **iids);
	HRESULT (WINAPI *GetRuntimeClassName)(codec_subtypes_statics2_i *This, HSTRING *name);
	HRESULT (WINAPI *GetTrustLevel)(codec_subtypes_statics2_i *This, TrustLevel *level);
	HRESULT (WINAPI *FindAllAsync)(codec_subtypes_statics2_i *This, uint64_t kind, uint64_t category, HSTRING sub_category, IInspectable **results);
	END_INTERFACE
};
static HRESULT WINAPI codec_instance_findall_query(codec_subtypes_statics2_i *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &codec_instance_findall_iid))
		*out = iface;
	else {
		trace_refused_iid("codecquery instance findall", iid);
		return E_NOINTERFACE;
	}
	return S_OK;
}
static ULONG WINAPI codec_instance_findall_addref(codec_subtypes_statics2_i *iface) { (void)iface; return 2; }
static ULONG WINAPI codec_instance_findall_release(codec_subtypes_statics2_i *iface) { (void)iface; return 1; }
static HRESULT WINAPI codec_instance_findall_iids(codec_subtypes_statics2_i *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI codec_instance_findall_name(codec_subtypes_statics2_i *iface, HSTRING *out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	return WindowsCreateString(codec_query_class, (UINT32)lstrlenW(codec_query_class), out);
}
static HRESULT WINAPI codec_instance_findall_trust(codec_subtypes_statics2_i *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static HRESULT WINAPI codec_instance_findall_async(codec_subtypes_statics2_i *iface, uint64_t kind, uint64_t category, HSTRING sub_category, IInspectable **results)
{
	/* FindAllAsync over the empty codec set. The caller probes the return for IAsyncInfo (the .NET await
	 * machinery) and casts it for c1c59efc-71df-5534-b03f-e18a8fbe78bb — the IAsyncOperation`1
	 * <IVectorView`1<CodecInfo>> parameterized face — so the return is the awaitable wrapper, whose
	 * GetResults hands the empty vector. The three-parameter overload arrives with subCategory in r9
	 * (legitimately NULL) and the retval out-pointer on the stack as the fifth argument; answering
	 * E_POINTER for the null subCategory was the observed NullReferenceException. */
	(void)iface; (void)kind; (void)category; (void)sub_category;
	if (!results)
		return E_POINTER;
	codec_operation *operation_object = codec_operation_new();
	if (!operation_object)
		return E_OUTOFMEMORY;
	*results = (IInspectable *)&operation_object->parameterized;
	raven_trace_note("raven-xaml: codec instance FindAllAsync answered the empty completed operation\n");
	return S_OK;
}
static const codec_instance_findall_vtbl codec_instance_findall_table = {
	codec_instance_findall_query, codec_instance_findall_addref, codec_instance_findall_release,
	codec_instance_findall_iids, codec_instance_findall_name, codec_instance_findall_trust,
	codec_instance_findall_async,
};
static codec_subtypes_statics2_object codec_instance_findall_instance = {
	(const codec_subtypes_statics2_vtbl *)&codec_instance_findall_table};
static HRESULT WINAPI codec_query_inspectable_query(IInspectable *iface, REFIID iid, void **out)
{
	codec_query_object *self = ((codec_query_iids_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IAgileObject))
		*out = &self->inspectable;
	else if (equal_iid(iid, &codec_instance_findall_iid))
		*out = (codec_subtypes_statics2_i *)&codec_instance_findall_instance;
	else {
		trace_refused_iid("codecquery instance", iid);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
	return S_OK;
}
static ULONG WINAPI codec_query_inspectable_addref(IInspectable *iface)
{ return (ULONG)InterlockedIncrement(&((codec_query_iids_part *)iface)->owner->refs); }
static ULONG WINAPI codec_query_inspectable_release(IInspectable *iface)
{
	codec_query_object *self = ((codec_query_iids_part *)iface)->owner;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		DeleteCriticalSection(&self->lock);
		FREE(self);
		InterlockedDecrement(&codec_objects);
	}
	return (ULONG)refs;
}
static HRESULT WINAPI codec_query_inspectable_iids(IInspectable *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI codec_query_inspectable_name(IInspectable *iface, HSTRING *out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	return WindowsCreateString(codec_query_class, (UINT32)lstrlenW(codec_query_class), out);
}
static HRESULT WINAPI codec_query_inspectable_trust(IInspectable *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static const IInspectableVtbl codec_query_inspectable_table = {
	codec_query_inspectable_query, codec_query_inspectable_addref, codec_query_inspectable_release,
	codec_query_inspectable_iids, codec_query_inspectable_name, codec_query_inspectable_trust,
};

static codec_query_object *codec_query_new(void)
{
	codec_query_object *self = ALLOC(sizeof *self);
	if (!self)
		return NULL;
	self->inspectable.lpVtbl = &codec_query_inspectable_table;
	self->inspectable.owner = self;
	self->refs = 1;
	InitializeCriticalSection(&self->lock);
	InterlockedIncrement(&codec_objects);
	return self;
}

static HRESULT WINAPI codec_factory_query(IActivationFactory *iface, REFIID iid, void **out);
static ULONG WINAPI codec_factory_addref(IActivationFactory *iface);
static ULONG WINAPI codec_factory_release(IActivationFactory *iface);
static HRESULT WINAPI codec_factory_iids(IActivationFactory *iface, ULONG *count, IID **iids);
static HRESULT WINAPI codec_factory_name(IActivationFactory *iface, HSTRING *name);
static HRESULT WINAPI codec_factory_trust(IActivationFactory *iface, TrustLevel *level);
static HRESULT WINAPI codec_factory_activate(IActivationFactory *iface, IInspectable **out);

static LONG codec_factory_refs;

static const IActivationFactoryVtbl codec_factory_table = {
	codec_factory_query, codec_factory_addref, codec_factory_release,
	codec_factory_iids, codec_factory_name, codec_factory_trust, codec_factory_activate,
};

/* The factory object: a struct whose first member is the vtable pointer, the shape the C callers dereference. */
typedef struct codec_factory_object { const IActivationFactoryVtbl *lpVtbl; } codec_factory_object;
static codec_factory_object codec_factory_instance = {&codec_factory_table};

/* The CodecSubtypes factory: statics-only class — the factory answers its statics face directly. */
typedef struct codec_subtypes_factory_object { const IActivationFactoryVtbl *lpVtbl; } codec_subtypes_factory_object;
static HRESULT WINAPI codec_subtypes_factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory))
		*out = iface;
	else if (equal_iid(iid, &codec_subtypes_statics_iid))
		*out = (codec_subtypes_statics_i *)&codec_subtypes_statics_instance;
	else if (equal_iid(iid, &codec_subtypes_statics2_iid))
		*out = (codec_subtypes_statics2_i *)&codec_subtypes_statics2_instance;
	else {
		trace_refused_iid("subtypes factory", iid);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&codec_factory_refs);
	return S_OK;
}
static const IActivationFactoryVtbl codec_subtypes_factory_table = {
	codec_subtypes_factory_query, codec_factory_addref, codec_factory_release,
	codec_factory_iids, codec_factory_name, codec_factory_trust, codec_factory_activate,
};
static codec_subtypes_factory_object codec_subtypes_instance = {&codec_subtypes_factory_table};

static HRESULT WINAPI codec_factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID_IActivationFactory))
		*out = iface;
	else if (equal_iid(iid, &codec_statics_iid))
		*out = (codec_statics_i *)&codec_statics_instance;
	else {
		trace_refused_iid("codecquery factory", iid);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&codec_factory_refs);
	return S_OK;
}

static ULONG WINAPI codec_factory_addref(IActivationFactory *iface)
{ (void)iface; return (ULONG)InterlockedIncrement(&codec_factory_refs); }
static ULONG WINAPI codec_factory_release(IActivationFactory *iface)
{ (void)iface; return (ULONG)InterlockedDecrement(&codec_factory_refs); }
static HRESULT WINAPI codec_factory_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{ (void)iface; return inspectable_iids(count, iids); }
static HRESULT WINAPI codec_factory_name(IActivationFactory *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(codec_query_class, (UINT32)lstrlenW(codec_query_class), name);
}
static HRESULT WINAPI codec_factory_trust(IActivationFactory *iface, TrustLevel *level)
{ (void)iface; return inspectable_trust(level); }
static HRESULT WINAPI codec_factory_activate(IActivationFactory *iface, IInspectable **out)
{
	/* The caller constructs the query instance (new CodecQuery()) and enumerates through its face. */
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	codec_query_object *instance = codec_query_new();
	if (!instance)
		return E_OUTOFMEMORY;
	*out = (IInspectable *)&instance->inspectable;
	return S_OK;
}

HRESULT raven_codec_query_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (length == ARRAYSIZE(codec_query_class) - 1 &&
		CompareStringOrdinal(text, (int)length, codec_query_class, -1, FALSE) == CSTR_EQUAL) {
		InterlockedIncrement(&codec_factory_refs);
		*out = (IActivationFactory *)&codec_factory_instance;
		return S_OK;
	}
	if (length == ARRAYSIZE(codec_subtypes_class) - 1 &&
		CompareStringOrdinal(text, (int)length, codec_subtypes_class, -1, FALSE) == CSTR_EQUAL) {
		InterlockedIncrement(&codec_factory_refs);
		*out = (IActivationFactory *)&codec_subtypes_instance;
		return S_OK;
	}
	return CLASS_E_CLASSNOTAVAILABLE;
}
