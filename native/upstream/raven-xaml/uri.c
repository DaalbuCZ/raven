/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Windows.Foundation.Uri: absolute hierarchical URI construction.
 * shortcut: canonicalization, relative resolution and complex authorities remain unsupported; extend with a complete URI parser. */
#define INITGUID
#define CONST_VTABLE
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <windows.foundation.h>
#include <winstring.h>
#include <stddef.h>

#define ALLOC(n) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (n))
#define FREE(p) HeapFree(GetProcessHeap(), 0, (p))
typedef __x_ABI_CWindows_CFoundation_CIUriRuntimeClass uri_i;
typedef __x_ABI_CWindows_CFoundation_CIUriRuntimeClassFactory uri_factory_i;
typedef __x_ABI_CWindows_CFoundation_CIStringable stringable_i;
static const WCHAR class_name[] = L"Windows.Foundation.Uri";
struct uri {
	uri_i iface;
	stringable_i stringable;
	LONG refs;
	HSTRING raw;
	UINT32 scheme_end, path_start, path_end, query_end, length;
	struct raven_weak_source weak;
};
static const __x_ABI_CWindows_CFoundation_CIUriRuntimeClassVtbl uri_table;
static const __x_ABI_CWindows_CFoundation_CIStringableVtbl stringable_table;
static HRESULT WINAPI uri_query(uri_i *iface, REFIID iid, void **out)
{
	struct uri *self = (void *)iface;
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) ||
		IsEqualIID(iid, &IID___x_ABI_CWindows_CFoundation_CIUriRuntimeClass) || IsEqualIID(iid, &IID_IAgileObject)) *out = iface;
	else if (IsEqualIID(iid, &IID___x_ABI_CWindows_CFoundation_CIStringable)) *out = &self->stringable;
	else if (IsEqualIID(iid, &IID_IWeakReferenceSource)) *out = &self->weak;
	else return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}
static ULONG WINAPI uri_addref(uri_i *iface) { return InterlockedIncrement(&((struct uri *)iface)->refs); }
static ULONG WINAPI uri_release(uri_i *iface)
{
	struct uri *self = (void *)iface;
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) { raven_weak_source_disconnect(&self->weak); WindowsDeleteString(self->raw); FREE(self); InterlockedDecrement(&raven_xaml_objects); }
	return refs;
}
static HRESULT WINAPI uri_iids(uri_i *iface, ULONG *count, IID **iids)
{
	if (!count || !iids) return E_POINTER;
	*count = 0; *iids = NULL;
	IID *list = ALLOC(2 * sizeof *list);
	if (!list) return E_OUTOFMEMORY;
	list[0] = IID___x_ABI_CWindows_CFoundation_CIUriRuntimeClass;
	list[1] = IID___x_ABI_CWindows_CFoundation_CIStringable;
	*count = 2; *iids = list; return S_OK;
}
static HRESULT WINAPI uri_name(uri_i *iface, HSTRING *out)
{ if (!out) return E_POINTER; return WindowsCreateString(class_name, ARRAYSIZE(class_name)-1, out); }
static HRESULT WINAPI uri_trust(uri_i *iface, TrustLevel *out)
{ if (!out) return E_POINTER; *out = BaseTrust; return S_OK; }
static HRESULT uri_slice(struct uri *self, UINT32 begin, UINT32 end, HSTRING *out)
{
	if (!out) return E_POINTER;
	const WCHAR *text = WindowsGetStringRawBuffer(self->raw, NULL);
	return WindowsCreateString(text + begin, end - begin, out);
}
static HRESULT WINAPI uri_raw(uri_i *iface, HSTRING *out)
{ if (!out) return E_POINTER; return WindowsDuplicateString(((struct uri *)iface)->raw, out); }
static HRESULT WINAPI uri_absolute(uri_i *iface, HSTRING *out) { return uri_raw(iface, out); }
static HRESULT WINAPI uri_scheme(uri_i *iface, HSTRING *out)
{ struct uri *s = (void *)iface; return uri_slice(s, 0, s->scheme_end, out); }
static HRESULT WINAPI uri_path(uri_i *iface, HSTRING *out)
{ struct uri *s = (void *)iface; return uri_slice(s, s->path_start, s->path_end, out); }
static HRESULT WINAPI uri_query_string(uri_i *iface, HSTRING *out)
{ struct uri *s = (void *)iface; return uri_slice(s, s->path_end, s->query_end, out); }
static HRESULT WINAPI uri_fragment(uri_i *iface, HSTRING *out)
{ struct uri *s = (void *)iface; return uri_slice(s, s->query_end, s->length, out); }
static HRESULT WINAPI uri_notimpl_string(uri_i *iface, HSTRING *out)
{ if (!out) return E_POINTER;
	*out = NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI uri_notimpl_decoder(uri_i *iface, __x_ABI_CWindows_CFoundation_CIWwwFormUrlDecoderRuntimeClass **out)
{ if (!out) return E_POINTER;
	*out = NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI uri_notimpl_port(uri_i *iface, INT32 *out)
{ if (!out) return E_POINTER; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI uri_notimpl_bool(uri_i *iface, boolean *out)
{ if (!out) return E_POINTER; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI uri_notimpl_equal(uri_i *iface, uri_i *other, boolean *out)
{ if (!out || !other) return E_POINTER; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI uri_notimpl_combine(uri_i *iface, HSTRING relative, uri_i **out)
{ if (!out) return E_POINTER;
	*out = NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static const __x_ABI_CWindows_CFoundation_CIUriRuntimeClassVtbl uri_table = {
	uri_query, uri_addref, uri_release, uri_iids, uri_name, uri_trust,
	uri_absolute, uri_absolute, uri_notimpl_string, uri_notimpl_string, uri_fragment,
	uri_notimpl_string, uri_notimpl_string, uri_path, uri_query_string, uri_notimpl_decoder,
	uri_raw, uri_scheme, uri_notimpl_string, uri_notimpl_port, uri_notimpl_bool, uri_notimpl_equal, uri_notimpl_combine
};
#define STRING_OWNER(p) CONTAINING_RECORD(p, struct uri, stringable)
static HRESULT WINAPI str_query(stringable_i *p, REFIID iid, void **out) { return uri_query(&STRING_OWNER(p)->iface, iid, out); }
static ULONG WINAPI str_addref(stringable_i *p) { return uri_addref(&STRING_OWNER(p)->iface); }
static ULONG WINAPI str_release(stringable_i *p) { return uri_release(&STRING_OWNER(p)->iface); }
static HRESULT WINAPI str_iids(stringable_i *p, ULONG *count, IID **iids) { return uri_iids(&STRING_OWNER(p)->iface, count, iids); }
static HRESULT WINAPI str_name(stringable_i *p, HSTRING *out) { return uri_name(&STRING_OWNER(p)->iface, out); }
static HRESULT WINAPI str_trust(stringable_i *p, TrustLevel *out) { return uri_trust(&STRING_OWNER(p)->iface, out); }
static HRESULT WINAPI str_string(stringable_i *p, HSTRING *out) { return uri_absolute(&STRING_OWNER(p)->iface, out); }
static const __x_ABI_CWindows_CFoundation_CIStringableVtbl stringable_table = {
	str_query, str_addref, str_release, str_iids, str_name, str_trust, str_string
};
static HRESULT create_uri(HSTRING input, uri_i **out)
{
	UINT32 length = 0, scheme = 0, authority, path, end, query;
	const WCHAR *text = WindowsGetStringRawBuffer(input, &length);
	struct uri *self;
	if (!out) return E_POINTER;
	*out = NULL;
	if (!length || !((text[0] >= L'a' && text[0] <= L'z') || (text[0] >= L'A' && text[0] <= L'Z'))) { OutputDebugStringA("raven-xaml: uri scheme refused\n"); return E_INVALIDARG; }
	while (scheme < length && ((text[scheme] >= L'a' && text[scheme] <= L'z') ||
		(text[scheme] >= L'0' && text[scheme] <= L'9') || text[scheme] == L'+' || text[scheme] == L'-' || text[scheme] == L'.')) ++scheme;
	if (scheme < length && text[scheme] >= L'A' && text[scheme] <= L'Z') return 0x80004011; /* PROBE */
	if (scheme >= length || text[scheme] != L':') { OutputDebugStringA("raven-xaml: uri colon refused\n"); return E_INVALIDARG; }
	if (scheme + 2 >= length || text[scheme+1] != L'/' || text[scheme+2] != L'/') return 0x80004012; /* PROBE */
	authority = scheme + 3; path = authority;
	while (path < length && text[path] != L'/' && text[path] != L'?' && text[path] != L'#') ++path;
	/* An authority that names a package — ms-appx://Microsoft.UI.Xaml.2.8/... — carries the dots and upper case a host
	 * form keeps, so the host form lifts the lower-case-only rule for the authority segment. */
	const UINT32 host_form = path > authority && text[authority] != L'/';
	/* Only canonical ASCII without escapes or dot segments is represented until full IRI support exists. */
	for (UINT32 i = authority; i < length; ++i)
		if (text[i] <= 0x20 || text[i] >= 0x7f || text[i] == L'%' || text[i] == L'\\' || text[i] == L'@' ||
			(!host_form && i < path && ((text[i] >= L'A' && text[i] <= L'Z') || text[i] == L':'))) return 0x80004013; /* PROBE */
	for (UINT32 i = path; i < length && text[i] != L'?' && text[i] != L'#'; ++i)
		if (text[i] == L'/' && i+1 < length && text[i+1] == L'.' &&
			(i+2 == length || text[i+2] == L'/' || text[i+2] == L'?' || text[i+2] == L'#' ||
			(text[i+2] == L'.' && (i+3 == length || text[i+3] == L'/' || text[i+3] == L'?' || text[i+3] == L'#')))) return 0x80004014; /* PROBE */
	end = path; while (end < length && text[end] != L'?' && text[end] != L'#') ++end;
	query = end; if (query < length && text[query] == L'?') { while (query < length && text[query] != L'#') ++query; }
	if (path == length || text[path] != L'/') return 0x80004015; /* PROBE */
	self = ALLOC(sizeof *self); if (!self) return E_OUTOFMEMORY;
	self->iface.lpVtbl = &uri_table; self->stringable.lpVtbl = &stringable_table; self->refs = 1;
	self->scheme_end = scheme; self->path_start = path; self->path_end = end; self->query_end = query; self->length = length;
	HRESULT result = WindowsDuplicateString(input, &self->raw);
	if (FAILED(result)) { FREE(self); return result; }
	raven_weak_source_init(&self->weak, self, &self->refs, (raven_weak_query_fn)uri_query);
	InterlockedIncrement(&raven_xaml_objects); *out = &self->iface; return S_OK;
}
struct factory { IActivationFactory activation; uri_factory_i factory; LONG refs; };
static struct factory factory;
static HRESULT factory_query(REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) || IsEqualIID(iid, &IID_IActivationFactory)) *out = &factory.activation;
	else if (IsEqualIID(iid, &IID___x_ABI_CWindows_CFoundation_CIUriRuntimeClassFactory)) *out = &factory.factory;
	else return E_NOINTERFACE;
	InterlockedIncrement(&factory.refs); return S_OK;
}
static HRESULT WINAPI activation_query(IActivationFactory *p, REFIID i, void **out) { return factory_query(i, out); }
static ULONG WINAPI activation_addref(IActivationFactory *p) { return InterlockedIncrement(&factory.refs); }
static ULONG WINAPI activation_release(IActivationFactory *p) { return InterlockedDecrement(&factory.refs); }
static HRESULT WINAPI activation_iids(IActivationFactory *p, ULONG *n, IID **i) { if (!n || !i) return E_POINTER; *n=0; *i=NULL; return S_OK; }
static HRESULT WINAPI activation_name(IActivationFactory *p, HSTRING *n) { return uri_name(NULL,n); }
static HRESULT WINAPI activation_trust(IActivationFactory *p, TrustLevel *t) { return uri_trust(NULL,t); }
static HRESULT WINAPI activation_create(IActivationFactory *p, IInspectable **out) { if (!out) return E_POINTER; *out=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static HRESULT WINAPI factory_part_query(uri_factory_i *p, REFIID i, void **out) { return factory_query(i,out); }
static ULONG WINAPI factory_addref(uri_factory_i *p) { return activation_addref(NULL); }
static ULONG WINAPI factory_release(uri_factory_i *p) { return activation_release(NULL); }
static HRESULT WINAPI factory_iids(uri_factory_i *p, ULONG *n, IID **i) { return activation_iids(NULL,n,i); }
static HRESULT WINAPI factory_name(uri_factory_i *p, HSTRING *n) { return uri_name(NULL,n); }
static HRESULT WINAPI factory_trust(uri_factory_i *p, TrustLevel *t) { return uri_trust(NULL,t); }
static HRESULT WINAPI factory_create(uri_factory_i *p, HSTRING value, uri_i **out) { return create_uri(value,out); }
static HRESULT WINAPI factory_relative(uri_factory_i *p, HSTRING base, HSTRING relative, uri_i **out)
{ if (!out) return E_POINTER; *out=NULL; { raven_trace_not_implemented(__func__); return E_NOTIMPL; } }
static const IActivationFactoryVtbl activation_table = {
	activation_query, activation_addref, activation_release, activation_iids, activation_name, activation_trust, activation_create
};
static const __x_ABI_CWindows_CFoundation_CIUriRuntimeClassFactoryVtbl factory_table = {
	factory_part_query, factory_addref, factory_release, factory_iids, factory_name, factory_trust, factory_create, factory_relative
};
static struct factory factory = {{&activation_table}, {&factory_table}, 1};

/* The engine-internal creation a compiled document or a dictionary's Source uses: the factory's parser and object, reached
 * without the catalog. */
__declspec(dllexport) HRESULT raven_xaml_uri_create(const WCHAR *text, UINT32 length, IInspectable **out)
{
	HSTRING input = NULL;
	uri_i *uri = NULL;
	HRESULT result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!text && length)
		return E_POINTER;
	result = WindowsCreateString(text, length, &input);
	if (FAILED(result))
		return result;
	result = create_uri(input, &uri);
	WindowsDeleteString(input);
	if (SUCCEEDED(result))
		*out = (IInspectable *)uri;
	return result;
}

HRESULT raven_uri_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0; const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	if (!out) return E_POINTER;
	*out = NULL;
	if (length != ARRAYSIZE(class_name)-1 || CompareStringOrdinal(text, length, class_name, -1, FALSE) != CSTR_EQUAL) return CLASS_E_CLASSNOTAVAILABLE;
	*out = &factory.activation; InterlockedIncrement(&factory.refs); return S_OK;
}
