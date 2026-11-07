/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Windows.Foundation.Uri: an absolute URI, parsed and canonicalized by urlmon's IUri, as on Windows. Its parts are IUri's properties:
 * the query and fragment with their '?' and '#', the port the scheme implies when none is given. A string that is no absolute URI is
 * refused with E_INVALIDARG. */
#define INITGUID
#include "web.h"
#include <urlmon.h>
#include <weakreference.h>

/* IUriRuntimeClassWithAbsoluteCanonicalUri, which the MinGW headers lack: identifier from Microsoft's published metadata. */
static const IID IID_uri_canonical = {0x758d9661, 0x221c, 0x480f, {0xa3, 0x39, 0x50, 0x65, 0x66, 0x73, 0xf4, 0x6f}};

typedef __x_ABI_CWindows_CFoundation_CIUriRuntimeClassFactory uri_factory_class;
typedef __x_ABI_CWindows_CFoundation_CIUriEscapeStatics uri_escape_class;

/* Weak references. The source part embeds in the object and its IUnknown is the object's; the reference it hands out is a
 * standalone object over a shared target block that holds no reference on the object: the object disconnects the part from
 * its destroy path, and a resolution after that answers a null interface — the documented weak contract. .NET Native takes
 * IWeakReferenceSource on every object it projects, so the part belongs on the classes programs project, the Uri among them. */
struct uri_weak_target {
	LONG refs; /* one for the object's embedded part, one per live weak reference */
	void *owner;
	LONG *owner_refs;
	HRESULT (WINAPI *owner_query)(void *owner, REFIID iid, void **out);
};

struct uri_weak_source {
	const IWeakReferenceSourceVtbl *lpVtbl;
	struct uri_weak_target *target;
};

static const IWeakReferenceVtbl uri_weak_ref_table;

struct uri_weak_ref {
	const IWeakReferenceVtbl *lpVtbl;
	struct uri_weak_target *target;
	LONG refs;
};

static HRESULT WINAPI uri_owner_query(void *owner, REFIID iid, void **out)
{
	*out = winrt_get((struct winrt_object *)owner, iid);
	return *out ? S_OK : E_NOINTERFACE;
}

static HRESULT WINAPI uri_weak_source_query(IWeakReferenceSource *iface, REFIID iid, void **out)
{
	struct uri_weak_target *target = ((struct uri_weak_source *)iface)->target;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!target || !target->owner)
		return E_NOINTERFACE; /* the part only answers while its object lives */
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID_IWeakReferenceSource)) {
		InterlockedIncrement(target->owner_refs);
		*out = iface;
		return S_OK;
	}
	if (IsEqualIID(iid, &IID_IAgileObject))
		return target->owner_query(target->owner, iid, out);
	return E_NOINTERFACE;
}

static ULONG WINAPI uri_weak_source_add_ref(IWeakReferenceSource *iface)
{
	return (ULONG)InterlockedIncrement(((struct uri_weak_target *)((struct uri_weak_source *)iface)->target)->owner_refs);
}

static ULONG WINAPI uri_weak_source_release(IWeakReferenceSource *iface)
{
	return (ULONG)InterlockedDecrement(((struct uri_weak_target *)((struct uri_weak_source *)iface)->target)->owner_refs);
}

static HRESULT WINAPI uri_weak_source_iids(IWeakReferenceSource *iface, ULONG *count, IID **iids)
{
	(void)iface;
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}

static HRESULT WINAPI uri_weak_source_name(IWeakReferenceSource *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return E_NOTIMPL; /* callers read the class name from the object, not from this part */
}

static HRESULT WINAPI uri_weak_source_trust(IWeakReferenceSource *iface, TrustLevel *level)
{
	(void)iface;
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

static HRESULT WINAPI uri_weak_source_get_weak_reference(IWeakReferenceSource *iface, IWeakReference **out)
{
	struct uri_weak_target *target = ((struct uri_weak_source *)iface)->target;
	struct uri_weak_ref *reference;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!target || !target->owner)
		return E_NOTIMPL; /* the object is gone: nothing left to reference weakly */
	if (!(reference = WINRT_ALLOC(sizeof *reference)))
		return E_OUTOFMEMORY;
	reference->lpVtbl = &uri_weak_ref_table;
	reference->target = target;
	reference->refs = 1;
	InterlockedIncrement(&target->refs);
	winrt_live_add();
	*out = (IWeakReference *)reference;
	return S_OK;
}

/* mingw's weakreference.h models both interfaces as IUnknown-only; the Windows ABI derives them from IInspectable, with the
 * one method at vtable slot 6, which is the layout .NET Native is compiled against. */
static const IWeakReferenceSourceVtbl uri_weak_source_table = {
	(void *)uri_weak_source_query, (void *)uri_weak_source_add_ref, (void *)uri_weak_source_release,
	(void *)uri_weak_source_iids, (void *)uri_weak_source_name, (void *)uri_weak_source_trust,
	(void *)uri_weak_source_get_weak_reference,
};

static HRESULT WINAPI uri_weak_ref_query(IWeakReference *iface, REFIID iid, void **out)
{
	struct uri_weak_ref *self = (struct uri_weak_ref *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) &&
		!IsEqualIID(iid, &IID_IWeakReference) && !IsEqualIID(iid, &IID_IAgileObject))
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI uri_weak_ref_add_ref(IWeakReference *iface) { return InterlockedIncrement(&((struct uri_weak_ref *)iface)->refs); }

static ULONG WINAPI uri_weak_ref_release(IWeakReference *iface)
{
	struct uri_weak_ref *self = (struct uri_weak_ref *)iface;
	ULONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (!InterlockedDecrement(&self->target->refs))
			WINRT_FREE(self->target);
		WINRT_FREE(self);
		winrt_live_release();
	}
	return refs;
}

static HRESULT WINAPI uri_weak_ref_iids(IWeakReference *iface, ULONG *count, IID **iids)
{
	(void)iface;
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}

static HRESULT WINAPI uri_weak_ref_name(IWeakReference *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return E_NOTIMPL;
}

static HRESULT WINAPI uri_weak_ref_trust(IWeakReference *iface, TrustLevel *level)
{
	(void)iface;
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

static HRESULT WINAPI uri_weak_ref_resolve(IWeakReference *iface, REFIID iid, IInspectable **out)
{
	struct uri_weak_target *target = ((struct uri_weak_ref *)iface)->target;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!target || !target->owner)
		return S_OK; /* the object is gone: resolution answers null, the documented weak contract */
	return target->owner_query(target->owner, iid, (void **)out);
}

static const IWeakReferenceVtbl uri_weak_ref_table = {
	(void *)uri_weak_ref_query, (void *)uri_weak_ref_add_ref, (void *)uri_weak_ref_release,
	(void *)uri_weak_ref_iids, (void *)uri_weak_ref_name, (void *)uri_weak_ref_trust,
	(void *)uri_weak_ref_resolve,
};

struct uri {
	struct winrt_object base;
	struct winrt_interface uri, canonical, stringable;
	struct uri_weak_source weak;
	IUri *parsed;
};

static struct uri *uri_of(void *iface) { return (struct uri *)winrt_object_of(iface); }

static void uri_destroy(struct winrt_object *object)
{
	struct uri *self = (struct uri *)object;
	/* a resolution after the destroy answers null, never a reach into the freed object */
	if (self->weak.target) {
		self->weak.target->owner = NULL;
		self->weak.target->owner_refs = NULL;
		self->weak.target->owner_query = NULL;
		if (!InterlockedDecrement(&self->weak.target->refs))
			WINRT_FREE(self->weak.target);
		self->weak.target = NULL;
	}
	self->parsed->lpVtbl->Release(self->parsed);
}

/* A string property of the parsed URI, as an HSTRING. */
static HRESULT property(void *iface, Uri_PROPERTY which, HSTRING *out)
{
	BSTR text = NULL;
	if (!out)
		return E_POINTER;
	HRESULT result = uri_of(iface)->parsed->lpVtbl->GetPropertyBSTR(uri_of(iface)->parsed, which, &text, 0);
	if (SUCCEEDED(result))
		result = winrt_string_n(text, SysStringLen(text), out);
	SysFreeString(text);
	return result;
}

static HRESULT STDMETHODCALLTYPE uri_absolute(void *iface, HSTRING *out) { return property(iface, Uri_PROPERTY_ABSOLUTE_URI, out); }
static HRESULT STDMETHODCALLTYPE uri_display(void *iface, HSTRING *out) { return property(iface, Uri_PROPERTY_DISPLAY_URI, out); }
static HRESULT STDMETHODCALLTYPE uri_domain(void *iface, HSTRING *out) { return property(iface, Uri_PROPERTY_DOMAIN, out); }
static HRESULT STDMETHODCALLTYPE uri_extension(void *iface, HSTRING *out) { return property(iface, Uri_PROPERTY_EXTENSION, out); }
static HRESULT STDMETHODCALLTYPE uri_fragment(void *iface, HSTRING *out) { return property(iface, Uri_PROPERTY_FRAGMENT, out); }
static HRESULT STDMETHODCALLTYPE uri_host(void *iface, HSTRING *out) { return property(iface, Uri_PROPERTY_HOST, out); }
static HRESULT STDMETHODCALLTYPE uri_password(void *iface, HSTRING *out) { return property(iface, Uri_PROPERTY_PASSWORD, out); }
static HRESULT STDMETHODCALLTYPE uri_path(void *iface, HSTRING *out) { return property(iface, Uri_PROPERTY_PATH, out); }
static HRESULT STDMETHODCALLTYPE uri_query(void *iface, HSTRING *out) { return property(iface, Uri_PROPERTY_QUERY, out); }
static HRESULT STDMETHODCALLTYPE uri_raw(void *iface, HSTRING *out) { return property(iface, Uri_PROPERTY_RAW_URI, out); }
static HRESULT STDMETHODCALLTYPE uri_scheme(void *iface, HSTRING *out) { return property(iface, Uri_PROPERTY_SCHEME_NAME, out); }
static HRESULT STDMETHODCALLTYPE uri_user(void *iface, HSTRING *out) { return property(iface, Uri_PROPERTY_USER_NAME, out); }

static HRESULT STDMETHODCALLTYPE uri_query_parsed(void *iface, void **out)
{
	/* WwwFormUrlDecoder is not provided. */
	if (!out)
		return E_POINTER;
	*out = NULL;
	return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_port(void *iface, INT32 *out)
{
	DWORD port = 0;
	if (!out)
		return E_POINTER;
	HRESULT result = uri_of(iface)->parsed->lpVtbl->GetPort(uri_of(iface)->parsed, &port);
	*out = SUCCEEDED(result) ? (INT32)port : 0;
	return result == S_FALSE ? S_OK : result;
}

static HRESULT STDMETHODCALLTYPE uri_suspicious(void *iface, boolean *out)
{
	if (!out)
		return E_POINTER;
	*out = FALSE;
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE uri_equals(void *iface, uri_class *other, boolean *out)
{
	HSTRING mine = NULL, theirs = NULL;
	INT32 order = 1;
	if (!out)
		return E_POINTER;
	*out = FALSE;
	if (!other)
		return S_OK;
	HRESULT result = uri_absolute(iface, &mine);
	if (SUCCEEDED(result))
		result = uri_text(other, &theirs);
	if (SUCCEEDED(result))
		result = WindowsCompareStringOrdinal(mine, theirs, &order);
	*out = SUCCEEDED(result) && !order;
	WindowsDeleteString(mine);
	WindowsDeleteString(theirs);
	return result;
}

static HRESULT wrap(IUri *parsed, uri_class **out);

static HRESULT STDMETHODCALLTYPE uri_combine(void *iface, HSTRING relative, uri_class **out)
{
	IUri *combined = NULL;
	if (!out)
		return E_POINTER;
	*out = NULL;
	HRESULT result = CoInternetCombineUrlEx(uri_of(iface)->parsed, winrt_text(relative, NULL), 0, &combined, 0);
	if (FAILED(result))
		return E_INVALIDARG;
	result = wrap(combined, out);
	combined->lpVtbl->Release(combined);
	return result;
}

static HRESULT STDMETHODCALLTYPE uri_canonical(void *iface, HSTRING *out) { return property(iface, Uri_PROPERTY_ABSOLUTE_URI, out); }
static HRESULT STDMETHODCALLTYPE uri_display_iri(void *iface, HSTRING *out) { return property(iface, Uri_PROPERTY_DISPLAY_URI, out); }
static HRESULT STDMETHODCALLTYPE uri_string(void *iface, HSTRING *out) { return property(iface, Uri_PROPERTY_ABSOLUTE_URI, out); }

static HRESULT STDMETHODCALLTYPE uri_object_query(void *iface, REFIID iid, void **out)
{
	struct uri *self = uri_of(iface);
	if (out && IsEqualIID(iid, &IID_IWeakReferenceSource) && self->weak.target) {
		InterlockedIncrement(&self->base.refs);
		*out = &self->weak;
		return S_OK;
	}
	return winrt_query(iface, iid, out);
}

#define URI_INSPECTABLE (void *)uri_object_query, (void *)winrt_add_ref, (void *)winrt_release, (void *)winrt_iids, \
	(void *)winrt_class_name, (void *)winrt_trust

static const __x_ABI_CWindows_CFoundation_CIUriRuntimeClassVtbl uri_table = {URI_INSPECTABLE, (void *)uri_absolute, (void *)uri_display,
	(void *)uri_domain, (void *)uri_extension, (void *)uri_fragment, (void *)uri_host, (void *)uri_password, (void *)uri_path,
	(void *)uri_query, (void *)uri_query_parsed, (void *)uri_raw, (void *)uri_scheme, (void *)uri_user, (void *)uri_port,
	(void *)uri_suspicious, (void *)uri_equals, (void *)uri_combine};
static const struct {
	void *query, *add_ref, *release, *iids, *name, *trust, *canonical, *display_iri;
} canonical_table = {URI_INSPECTABLE, uri_canonical, uri_display_iri};
static const __x_ABI_CWindows_CFoundation_CIStringableVtbl uri_stringable_table = {URI_INSPECTABLE, (void *)uri_string};
static const struct winrt_entry uri_entries[] = {
	WINRT_ENTRY(struct uri, uri, IID___x_ABI_CWindows_CFoundation_CIUriRuntimeClass), WINRT_ENTRY(struct uri, canonical, IID_uri_canonical),
	WINRT_ENTRY(struct uri, stringable, IID___x_ABI_CWindows_CFoundation_CIStringable), WINRT_END};
static const struct winrt_class uri_kind = {L"Windows.Foundation.Uri", uri_entries, uri_destroy};

static HRESULT wrap(IUri *parsed, uri_class **out)
{
	struct uri *self = winrt_new(&uri_kind, sizeof(*self), NULL);
	if (!self)
		return E_OUTOFMEMORY;
	self->uri.table = &uri_table;
	self->canonical.table = &canonical_table;
	self->stringable.table = &uri_stringable_table;
	self->weak.lpVtbl = &uri_weak_source_table;
	self->weak.target = WINRT_ALLOC(sizeof(struct uri_weak_target));
	if (self->weak.target) {
		self->weak.target->refs = 1;
		self->weak.target->owner = &self->base;
		self->weak.target->owner_refs = &self->base.refs;
		self->weak.target->owner_query = uri_owner_query;
	}
	parsed->lpVtbl->AddRef(parsed);
	self->parsed = parsed;
	*out = (void *)&self->uri;
	return S_OK;
}

HRESULT uri_create(const WCHAR *text, UINT32 length, uri_class **out)
{
	IUri *parsed = NULL;
	WCHAR *copy;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!length)
		return E_INVALIDARG;
	if (!(copy = WINRT_ALLOC(((size_t)length + 1) * sizeof(WCHAR))))
		return E_OUTOFMEMORY;
	memcpy(copy, text, length * sizeof(WCHAR));
	HRESULT result = 	/* The default decoding Windows' CreateUri applies: the properties read canonical text and CombineUri composes with
	 * it. The NO_DECODE_EXTRA_INFO form an earlier build asked for refuses an escaped path here (raven-urlmon's
	 * no-decode path), and nothing of this component needs it. */
	CreateUri(copy, Uri_CREATE_CANONICALIZE, 0, &parsed);
	WINRT_FREE(copy);
	if (FAILED(result))
		return E_INVALIDARG;
	result = wrap(parsed, out);
	parsed->lpVtbl->Release(parsed);
	return result;
}

HRESULT uri_text(uri_class *uri, HSTRING *out) { return uri->lpVtbl->get_AbsoluteUri(uri, out); }

/* The factory: IActivationFactory (which cannot make one without a string), IUriRuntimeClassFactory and IUriEscapeStatics. */
struct factory {
	struct winrt_object base;
	struct winrt_interface activation, create, escape;
};

static HRESULT STDMETHODCALLTYPE factory_activate(void *iface, IInspectable **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE factory_create(void *iface, HSTRING text, uri_class **out)
{
	UINT32 length;
	const WCHAR *characters = winrt_text(text, &length);
	return uri_create(characters, length, out);
}

static HRESULT STDMETHODCALLTYPE factory_relative(void *iface, HSTRING base, HSTRING relative, uri_class **out)
{
	uri_class *absolute = NULL;
	HRESULT result = factory_create(iface, base, &absolute);
	if (SUCCEEDED(result)) {
		result = absolute->lpVtbl->CombineUri(absolute, relative, out);
		absolute->lpVtbl->Release(absolute);
	}
	return result;
}

/* Percent-encoding of one component (RFC 3986, 2.1): what is not unreserved is escaped as UTF-8 octets. */
static HRESULT STDMETHODCALLTYPE escape_component(void *iface, HSTRING text, HSTRING *out)
{
	static const char digits[] = "0123456789ABCDEF";
	UINT32 length, bytes;
	const WCHAR *characters = winrt_text(text, &length);
	char *utf8 = winrt_utf8(characters, length, &bytes);
	WCHAR *escaped = utf8 ? WINRT_ALLOC(((size_t)bytes * 3 + 1) * sizeof(WCHAR)) : NULL;
	if (!escaped) {
		WINRT_FREE(utf8);
		return E_OUTOFMEMORY;
	}
	UINT32 used = 0;
	for (UINT32 index = 0; index < bytes; ++index) {
		const unsigned char octet = (unsigned char)utf8[index];
		if ((octet >= 'A' && octet <= 'Z') || (octet >= 'a' && octet <= 'z') || (octet >= '0' && octet <= '9') || octet == '-' ||
			octet == '.' || octet == '_' || octet == '~')
			escaped[used++] = octet;
		else {
			escaped[used++] = '%';
			escaped[used++] = digits[octet >> 4];
			escaped[used++] = digits[octet & 15];
		}
	}
	const HRESULT result = winrt_string_n(escaped, used, out);
	WINRT_FREE(escaped);
	WINRT_FREE(utf8);
	return result;
}

static int hex_value(WCHAR character)
{
	return character >= '0' && character <= '9' ? character - '0' : character >= 'a' && character <= 'f' ? character - 'a' + 10 :
		character >= 'A' && character <= 'F' ? character - 'A' + 10 : -1;
}

static HRESULT STDMETHODCALLTYPE unescape_component(void *iface, HSTRING text, HSTRING *out)
{
	UINT32 length, used = 0, wide;
	const WCHAR *characters = winrt_text(text, &length);
	/* Escapes are UTF-8 octets; the other characters are taken as they are. */
	char *octets = WINRT_ALLOC((size_t)length * 3 + 1);
	if (!octets)
		return E_OUTOFMEMORY;
	for (UINT32 index = 0; index < length; ++index) {
		if (characters[index] == '%' && index + 2 < length && hex_value(characters[index + 1]) >= 0 && hex_value(characters[index + 2]) >= 0) {
			octets[used++] = (char)(hex_value(characters[index + 1]) * 16 + hex_value(characters[index + 2]));
			index += 2;
		} else {
			UINT32 bytes;
			char *piece = winrt_utf8(characters + index, IS_HIGH_SURROGATE(characters[index]) && index + 1 < length ? 2 : 1, &bytes);
			if (!piece) {
				WINRT_FREE(octets);
				return E_OUTOFMEMORY;
			}
			memcpy(octets + used, piece, bytes);
			used += bytes;
			index += IS_HIGH_SURROGATE(characters[index]) && index + 1 < length;
			WINRT_FREE(piece);
		}
	}
	WCHAR *decoded = winrt_utf16(octets, used, &wide);
	WINRT_FREE(octets);
	if (!decoded)
		return E_OUTOFMEMORY;
	const HRESULT result = winrt_string_n(decoded, wide, out);
	WINRT_FREE(decoded);
	return result;
}

static const IActivationFactoryVtbl activation_table = {WINRT_INSPECTABLE, (void *)factory_activate};
static const __x_ABI_CWindows_CFoundation_CIUriRuntimeClassFactoryVtbl create_table = {WINRT_INSPECTABLE, (void *)factory_create,
	(void *)factory_relative};
static const __x_ABI_CWindows_CFoundation_CIUriEscapeStaticsVtbl escape_table = {WINRT_INSPECTABLE, (void *)unescape_component,
	(void *)escape_component};
static const struct winrt_entry factory_entries[] = {
	WINRT_ENTRY(struct factory, activation, IID_IActivationFactory),
	WINRT_ENTRY(struct factory, create, IID___x_ABI_CWindows_CFoundation_CIUriRuntimeClassFactory),
	WINRT_ENTRY(struct factory, escape, IID___x_ABI_CWindows_CFoundation_CIUriEscapeStatics), WINRT_END};
static const struct winrt_class factory_kind = {L"Windows.Foundation.Uri", factory_entries, NULL};

HRESULT uri_factory(REFIID iid, void **out)
{
	struct factory *self = winrt_new(&factory_kind, sizeof(*self), NULL);
	if (!self)
		return E_OUTOFMEMORY;
	self->activation.table = &activation_table;
	self->create.table = &create_table;
	self->escape.table = &escape_table;
	const HRESULT result = winrt_query(&self->activation, iid, out);
	winrt_release(&self->activation);
	return result;
}
