/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.Data.Xml.Dom namespace of the Windows Runtime.
 *
 * Shapr3D's native library activates XmlDocument during startup, so the DOM is
 * here in full for the surfaces a document consumer reaches: an XML parser over
 * UTF-16 text building a node tree, the IXmlNode/IXmlNodeSelector/IXmlNodeSerializer
 * triple every node serves, the kind-specific interfaces (element, attribute,
 * character data, text, comment, CDATA, processing instruction, fragment), the
 * document interfaces (IXmlDocument, IXmlDocumentIO, IXmlDocumentIO2), XmlNodeList,
 * XmlNamedNodeMap, XmlDomImplementation, XmlLoadSettings and the statics' file
 * loads. The node vtables come from MinGW's windows.data.xml.dom.h, which carries
 * the real ABI for both widths.
 *
 * Subsets, recorded honestly:
 *  - The XPath of SelectSingleNode/SelectNodes covers child and attribute steps,
 *    wildcards, '//' descendant steps and the [n]/[last()] predicates; no name
 *    tests on other axes, no functions, no unions. The NS variants ignore the
 *    namespace map and match qualified names literally.
 *  - Namespace URIs are not tracked: get_NamespaceUri answers an empty string,
 *    the NS attribute lookups match local names only.
 *  - DTDs are skipped, not validated; LoadXmlFromBuffer is not served (the guests
 *    observed so far load text); SaveToFileAsync writes UTF-8.
 */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <winstring.h>
#include <windows.foundation.h>
#include <windows.data.xml.dom.h>
#include "windows_ui_xaml_dictionary.h" /* the IReference<HSTRING> box the NodeValue getters serve */

#include <stddef.h>

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))
#ifndef E_ILLEGAL_METHOD_CALL
#define E_ILLEGAL_METHOD_CALL ((HRESULT)0x8000000e)
#endif
#ifndef E_ILLEGAL_DELEGATE_ASSIGNMENT
#define E_ILLEGAL_DELEGATE_ASSIGNMENT ((HRESULT)0x80000018)
#endif
#ifndef RO_E_CLOSED
#define RO_E_CLOSED ((HRESULT)0x80000013)
#endif

/* Short aliases of the vtable types the MinGW header spells out in full. */
#define node_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeVtbl
#define selector_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeSelectorVtbl
#define serializer_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeSerializerVtbl
#define element_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlElementVtbl
#define attribute_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlAttributeVtbl
#define chardata_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlCharacterDataVtbl
#define text_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlTextVtbl
#define comment_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlCommentVtbl
#define cdata_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlCDataSectionVtbl
#define pi_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlProcessingInstructionVtbl
#define fragment_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentFragmentVtbl
#define document_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentVtbl
#define documentio_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentIOVtbl
#define documentio2_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentIO2Vtbl
#define nodelist_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeListVtbl
#define namedmap_iVtbl __x_ABI_CWindows_CData_CXml_CDom_CIXmlNamedNodeMapVtbl

typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlNode node_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeSelector selector_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeSerializer serializer_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlElement element_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlAttribute attribute_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlCharacterData chardata_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlText text_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlComment comment_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlCDataSection cdata_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlProcessingInstruction pi_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentFragment fragment_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocument document_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentIO documentio_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentIO2 documentio2_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeList nodelist_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlNamedNodeMap namedmap_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlLoadSettings loadsettings_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentStatics docstatics_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CNodeType node_type;

/* The generic instances MinGW's windows.data.xml.dom.h declares for the document
 * loads; the pinterface identifiers there match the SHA-1 names the type system
 * computes (verified against the IAsyncOperation<StorageFile> identifier
 * windows.storage.h carries before this component relied on them). */
typedef __FIAsyncOperation_1_Windows__CData__CXml__CDom__CXmlDocument xml_doc_op_i;
typedef __FIAsyncOperationCompletedHandler_1_Windows__CData__CXml__CDom__CXmlDocument xml_doc_done_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlDomImplementation domimpl_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentType doctype_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlEntityReference entityref_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentStaticsVtbl docstatics_iVtbl;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlLoadSettingsVtbl loadsettings_iVtbl;

static const WCHAR document_class_name[] = L"Windows.Data.Xml.Dom.XmlDocument";
static const WCHAR settings_class_name[] = L"Windows.Data.Xml.Dom.XmlLoadSettings";

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

static int equal_iid(REFIID left, const IID *right)
{
	return left->Data1 == right->Data1 && left->Data2 == right->Data2 && left->Data3 == right->Data3 &&
		left->Data4[0] == right->Data4[0] && left->Data4[1] == right->Data4[1] &&
		left->Data4[2] == right->Data4[2] && left->Data4[3] == right->Data4[3] &&
		left->Data4[4] == right->Data4[4] && left->Data4[5] == right->Data4[5] &&
		left->Data4[6] == right->Data4[6] && left->Data4[7] == right->Data4[7];
}

static HRESULT create_string(const WCHAR *text, HSTRING *out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!text)
		return S_OK;
	return WindowsCreateString(text, (UINT32)lstrlenW(text), out);
}

static HRESULT hstring_text(HSTRING value, const WCHAR **text)
{
	if (!text)
		return E_POINTER;
	*text = WindowsGetStringRawBuffer(value, NULL);
	return S_OK;
}

/* The one-box HSTRING box: what NodeValue and the namespace getters carry. */
struct xml_string_box {
	const __FIReference_1_HSTRINGVtbl *lpVtbl;
	LONG refs;
	HSTRING value;
};

static const __FIReference_1_HSTRINGVtbl string_box_table;

static HRESULT WINAPI string_box_query(__FIReference_1_HSTRING *iface, REFIID iid, void **out)
{
	struct xml_string_box *self = (void *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID___FIReference_1_HSTRING) && !equal_iid(iid, &IID_IAgileObject))
		return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI string_box_addref(__FIReference_1_HSTRING *iface)
{
	return (ULONG)InterlockedIncrement(&((struct xml_string_box *)iface)->refs);
}

static ULONG WINAPI string_box_release(__FIReference_1_HSTRING *iface)
{
	struct xml_string_box *self = (void *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->value)
			WindowsDeleteString(self->value);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI string_box_iids(__FIReference_1_HSTRING *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI string_box_class(__FIReference_1_HSTRING *iface, HSTRING *out)
{
	(void)iface;
	return create_string(L"Windows.Foundation.IReference`1<String>", out);
}

static HRESULT WINAPI string_box_trust(__FIReference_1_HSTRING *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI string_box_value(__FIReference_1_HSTRING *iface, HSTRING *out)
{
	struct xml_string_box *self = (void *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (self->value)
		WindowsDuplicateString(self->value, out);
	return S_OK;
}

static const __FIReference_1_HSTRINGVtbl string_box_table = {
	string_box_query, string_box_addref, string_box_release, string_box_iids, string_box_class,
	string_box_trust, string_box_value,
};

static HRESULT box_string(const WCHAR *text, IInspectable **out)
{
	struct xml_string_box *box;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!text)
		return S_OK;
	box = ALLOC(sizeof *box);
	if (!box)
		return E_OUTOFMEMORY;
	box->lpVtbl = &string_box_table;
	box->refs = 1;
	if (FAILED(WindowsCreateString(text, (UINT32)lstrlenW(text), &box->value))) {
		FREE(box);
		return E_OUTOFMEMORY;
	}
	InterlockedIncrement(&raven_xaml_objects);
	*out = (IInspectable *)box;
	return S_OK;
}

/* An IReference<HSTRING> the guest passed as a node value. */
static HRESULT unbox_string(IInspectable *value, const WCHAR **text)
{
	__FIReference_1_HSTRING *reference = NULL;
	HSTRING string = NULL;
	const WCHAR *raw;
	HRESULT result;
	if (!text)
		return E_POINTER;
	*text = NULL;
	if (!value)
		return S_OK;
	if (FAILED(((IInspectable *)value)->lpVtbl->QueryInterface(value, &IID___FIReference_1_HSTRING, (void **)&reference)))
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:258\n"); return E_INVALIDARG; }
	result = reference->lpVtbl->get_Value(reference, &string);
	reference->lpVtbl->Release(reference);
	if (FAILED(result))
		return result;
	raw = WindowsGetStringRawBuffer(string, NULL);
	{
		SIZE_T length = lstrlenW(raw);
		WCHAR *copy = ALLOC((length + 1) * sizeof *copy);
		if (!copy) {
			WindowsDeleteString(string);
			return E_OUTOFMEMORY;
		}
		lstrcpyW(copy, raw);
		*text = copy;
	}
	WindowsDeleteString(string);
	return S_OK;
}

/* ---------------------------------------------------------------- growth buffer */

struct xml_buf {
	WCHAR *data;
	SIZE_T len, cap;
	BOOL ok;
};

static void buf_reserve(struct xml_buf *b, SIZE_T extra)
{
	SIZE_T want = b->len + extra + 1;
	if (!b->ok)
		return;
	if (want <= b->cap)
		return;
	SIZE_T cap = b->cap ? b->cap : 128;
	while (cap < want)
		cap *= 2;
	/* HeapReAlloc refuses a null block, unlike realloc; the first reserve allocates */
	WCHAR *grown = b->data ?
		HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, b->data, cap * sizeof *grown) :
		HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cap * sizeof *grown);
	if (!grown) {
		b->ok = FALSE;
		return;
	}
	b->data = grown;
	b->cap = cap;
}

static void buf_putn(struct xml_buf *b, const WCHAR *text, SIZE_T count)
{
	if (!count)
		return;
	buf_reserve(b, count);
	if (!b->ok)
		return;
	memcpy(b->data + b->len, text, count * sizeof *text);
	b->len += count;
	b->data[b->len] = 0;
}

static void buf_put(struct xml_buf *b, const WCHAR *text)
{
	if (text)
		buf_putn(b, text, lstrlenW(text));
}

static void buf_putc(struct xml_buf *b, WCHAR c)
{
	buf_putn(b, &c, 1);
}

/* Serialization escaping: text keeps entities for ampersand and angle brackets, attribute values also quote. */
static void buf_put_escaped(struct xml_buf *b, const WCHAR *text, BOOL attribute)
{
	for (; *text; ++text) {
		switch (*text) {
		case L'&': buf_put(b, L"&amp;"); break;
		case L'<': buf_put(b, L"&lt;"); break;
		case L'>': buf_put(b, L"&gt;"); break;
		case L'"': if (attribute) buf_put(b, L"&quot;"); else buf_putc(b, L'"'); break;
		case L'\'': if (attribute) buf_put(b, L"&apos;"); else buf_putc(b, L'\''); break;
		default: buf_putc(b, *text); break;
		}
	}
}

/* ---------------------------------------------------------------- nodes */

enum xml_kind {
	K_DOC, K_ELEMENT, K_ATTR, K_TEXT, K_CDATA, K_COMMENT, K_PI, K_FRAGMENT,
};

#define XML_PART_LIFETIME(part, iface) \
	static HRESULT WINAPI part##_query(iface *p, REFIID iid, void **out) \
	{ return xml_node_query(((struct part##_part *)p)->owner, iid, out); } \
	static ULONG WINAPI part##_addref(iface *p) \
	{ return xml_node_addref(((struct part##_part *)p)->owner); } \
	static ULONG WINAPI part##_release(iface *p) \
	{ return xml_node_release(((struct part##_part *)p)->owner); } \
	static HRESULT WINAPI part##_iids(iface *p, ULONG *count, IID **iids) \
	{ (void)p; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI part##_trust(iface *p, TrustLevel *level) \
	{ (void)p; return inspectable_trust(level); }

struct node_part { const node_iVtbl *lpVtbl; struct xml_node *owner; };
struct selector_part { const selector_iVtbl *lpVtbl; struct xml_node *owner; };
struct serializer_part { const serializer_iVtbl *lpVtbl; struct xml_node *owner; };
struct element_part { const element_iVtbl *lpVtbl; struct xml_node *owner; };
struct attribute_part { const attribute_iVtbl *lpVtbl; struct xml_node *owner; };
struct chardata_part { const chardata_iVtbl *lpVtbl; struct xml_node *owner; };
struct textp_part { const text_iVtbl *lpVtbl; struct xml_node *owner; };
struct comment_part { const comment_iVtbl *lpVtbl; struct xml_node *owner; };
struct cdata_part { const cdata_iVtbl *lpVtbl; struct xml_node *owner; };
struct pi_part { const pi_iVtbl *lpVtbl; struct xml_node *owner; };
struct fragment_part { const fragment_iVtbl *lpVtbl; struct xml_node *owner; };
struct document_part { const document_iVtbl *lpVtbl; struct xml_node *owner; };
struct documentio_part { const documentio_iVtbl *lpVtbl; struct xml_node *owner; };
struct documentio2_part { const documentio2_iVtbl *lpVtbl; struct xml_node *owner; };
/* The map's vtable is declared here: MinGW's IXmlNamedNodeMap drops the namespace and
 * local name from GetNamedItemNS, so the ABI-correct order comes from the winmd instead. */
struct xml_map_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(namedmap_i *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(namedmap_i *This);
	ULONG (STDMETHODCALLTYPE *Release)(namedmap_i *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(namedmap_i *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(namedmap_i *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(namedmap_i *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_Length)(namedmap_i *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *Item)(namedmap_i *This, UINT32 index, node_i **node);
	HRESULT (STDMETHODCALLTYPE *GetNamedItem)(namedmap_i *This, HSTRING name, node_i **node);
	HRESULT (STDMETHODCALLTYPE *SetNamedItem)(namedmap_i *This, node_i *node, node_i **previous);
	HRESULT (STDMETHODCALLTYPE *RemoveNamedItem)(namedmap_i *This, HSTRING name, node_i **removed);
	HRESULT (STDMETHODCALLTYPE *GetNamedItemNS)(namedmap_i *This, IInspectable *namespace_uri, HSTRING local_name, node_i **node);
	HRESULT (STDMETHODCALLTYPE *RemoveNamedItemNS)(namedmap_i *This, IInspectable *namespace_uri, HSTRING local_name, node_i **removed);
	HRESULT (STDMETHODCALLTYPE *SetNamedItemNS)(namedmap_i *This, node_i *node, node_i **previous);
};
struct map_part { const struct xml_map_vtbl *lpVtbl; struct xml_node *owner; };

struct xml_node {
	struct node_part node; /* the identity */
	struct selector_part selector;
	struct serializer_part serializer;
	struct element_part element;
	struct attribute_part attribute;
	struct chardata_part chardata;
	struct textp_part textp;
	struct comment_part comment;
	struct cdata_part cdata;
	struct pi_part pi;
	struct fragment_part fragment;
	struct document_part document;
	struct documentio_part documentio;
	struct documentio2_part documentio2;
	struct map_part map;
	LONG refs;
	int kind;
	struct xml_node *doc; /* addref'd; the document points at itself without a self-ref */
	struct xml_node *parent, *first, *last, *prev, *next;
	WCHAR *name;  /* the qualified name of an element or attribute, the PI's target */
	WCHAR *local; /* the name after the first colon, or the name itself */
	WCHAR *prefix;
	WCHAR *value; /* text, comment, CDATA and PI data, attribute values */
	WCHAR *uri;   /* the document's source URI */
	struct xml_node **attrs;
	SIZE_T attr_count, attr_cap;
};

static HRESULT WINAPI xml_node_query(struct xml_node *self, REFIID iid, void **out);
static ULONG xml_node_addref(struct xml_node *self);
static ULONG xml_node_release(struct xml_node *self);
struct xml_parser;
static void xml_node_unlink(struct xml_node *child);
static BOOL p_text_quoted(struct xml_parser *p, struct xml_buf *out, WCHAR quote);

static const node_iVtbl node_table;
static const selector_iVtbl selector_table;
static const serializer_iVtbl serializer_table;
static const element_iVtbl element_table;
static const attribute_iVtbl attribute_table;
static const chardata_iVtbl chardata_table;
static const text_iVtbl text_table;
static const comment_iVtbl comment_table;
static const cdata_iVtbl cdata_table;
static const pi_iVtbl pi_table;
static const fragment_iVtbl fragment_table;
static const document_iVtbl document_table;
static const documentio_iVtbl documentio_table;
static const documentio2_iVtbl documentio2_table;
static const struct xml_map_vtbl map_table;

XML_PART_LIFETIME(node, node_i)
XML_PART_LIFETIME(selector, selector_i)
XML_PART_LIFETIME(serializer, serializer_i)
XML_PART_LIFETIME(element, element_i)
XML_PART_LIFETIME(attribute, attribute_i)
XML_PART_LIFETIME(chardata, chardata_i)
XML_PART_LIFETIME(textp, text_i)
XML_PART_LIFETIME(comment, comment_i)
XML_PART_LIFETIME(cdata, cdata_i)
XML_PART_LIFETIME(pi, pi_i)
XML_PART_LIFETIME(fragment, fragment_i)
XML_PART_LIFETIME(document, document_i)
XML_PART_LIFETIME(documentio, documentio_i)
XML_PART_LIFETIME(documentio2, documentio2_i)
XML_PART_LIFETIME(map, namedmap_i)

/* The class name the kind's default interface reports. */
static const WCHAR *xml_kind_class(int kind)
{
	switch (kind) {
	case K_DOC: return document_class_name;
	case K_ELEMENT: return L"Windows.Data.Xml.Dom.XmlElement";
	case K_ATTR: return L"Windows.Data.Xml.Dom.XmlAttribute";
	case K_TEXT: return L"Windows.Data.Xml.Dom.XmlText";
	case K_CDATA: return L"Windows.Data.Xml.Dom.XmlCDataSection";
	case K_COMMENT: return L"Windows.Data.Xml.Dom.XmlComment";
	case K_PI: return L"Windows.Data.Xml.Dom.XmlProcessingInstruction";
	case K_FRAGMENT: return L"Windows.Data.Xml.Dom.XmlDocumentFragment";
	}
	return document_class_name;
}

static HRESULT WINAPI node_class(node_i *p, HSTRING *out)
{
	(void)p;
	return create_string(xml_kind_class(((struct node_part *)p)->owner->kind), out);
}

static HRESULT WINAPI selector_class(selector_i *p, HSTRING *out)
{
	(void)p;
	return create_string(L"Windows.Data.Xml.Dom.IXmlNodeSelector", out);
}

static HRESULT WINAPI serializer_class(serializer_i *p, HSTRING *out)
{
	(void)p;
	return create_string(L"Windows.Data.Xml.Dom.IXmlNodeSerializer", out);
}

/* Kind parts are the default interfaces of the kind's class; the map is XmlNamedNodeMap's. */
static HRESULT kind_part_class(int kind, HSTRING *out) { return create_string(xml_kind_class(kind), out); }

static HRESULT WINAPI element_class(element_i *p, HSTRING *out) { return kind_part_class(((struct element_part *)p)->owner->kind, out); }
static HRESULT WINAPI attribute_class(attribute_i *p, HSTRING *out) { return kind_part_class(((struct attribute_part *)p)->owner->kind, out); }
static HRESULT WINAPI chardata_class(chardata_i *p, HSTRING *out) { return kind_part_class(((struct chardata_part *)p)->owner->kind, out); }
static HRESULT WINAPI text_class(text_i *p, HSTRING *out) { return kind_part_class(((struct textp_part *)p)->owner->kind, out); }
static HRESULT WINAPI comment_class(comment_i *p, HSTRING *out) { return kind_part_class(((struct comment_part *)p)->owner->kind, out); }
static HRESULT WINAPI cdata_class(cdata_i *p, HSTRING *out) { return kind_part_class(((struct cdata_part *)p)->owner->kind, out); }
static HRESULT WINAPI pi_class(pi_i *p, HSTRING *out) { return kind_part_class(((struct pi_part *)p)->owner->kind, out); }
static HRESULT WINAPI fragment_class(fragment_i *p, HSTRING *out) { return kind_part_class(((struct fragment_part *)p)->owner->kind, out); }
static HRESULT WINAPI document_class(document_i *p, HSTRING *out) { return kind_part_class(((struct document_part *)p)->owner->kind, out); }
static HRESULT WINAPI documentio_class(documentio_i *p, HSTRING *out)
{
	(void)p;
	return create_string(L"Windows.Data.Xml.Dom.IXmlDocumentIO", out);
}
static HRESULT WINAPI documentio2_class(documentio2_i *p, HSTRING *out)
{
	(void)p;
	return create_string(L"Windows.Data.Xml.Dom.IXmlDocumentIO2", out);
}
static HRESULT WINAPI map_class(namedmap_i *p, HSTRING *out)
{
	(void)p;
	return create_string(L"Windows.Data.Xml.Dom.XmlNamedNodeMap", out);
}

static node_type xml_kind_type(int kind)
{
	switch (kind) {
	case K_DOC: return NodeType_DocumentNode;
	case K_ELEMENT: return NodeType_ElementNode;
	case K_ATTR: return NodeType_AttributeNode;
	case K_TEXT: return NodeType_TextNode;
	case K_CDATA: return NodeType_DataSectionNode;
	case K_COMMENT: return NodeType_CommentNode;
	case K_PI: return NodeType_ProcessingInstructionNode;
	case K_FRAGMENT: return NodeType_DocumentFragmentNode;
	}
	return NodeType_Invalid;
}

static HRESULT xml_node_query(struct xml_node *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IAgileObject))
		*out = &self->node;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlNode))
		*out = &self->node;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeSelector))
		*out = &self->selector;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeSerializer))
		*out = &self->serializer;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlElement)) {
		if (self->kind != K_ELEMENT) return E_NOINTERFACE;
		*out = &self->element;
	} else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlAttribute)) {
		if (self->kind != K_ATTR) return E_NOINTERFACE;
		*out = &self->attribute;
	} else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlCharacterData)) {
		if (self->kind != K_TEXT && self->kind != K_CDATA && self->kind != K_COMMENT) return E_NOINTERFACE;
		*out = &self->chardata;
	} else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlText)) {
		if (self->kind != K_TEXT && self->kind != K_CDATA) return E_NOINTERFACE;
		*out = &self->textp;
	} else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlComment)) {
		if (self->kind != K_COMMENT) return E_NOINTERFACE;
		*out = &self->comment;
	} else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlCDataSection)) {
		if (self->kind != K_CDATA) return E_NOINTERFACE;
		*out = &self->cdata;
	} else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlProcessingInstruction)) {
		if (self->kind != K_PI) return E_NOINTERFACE;
		*out = &self->pi;
	} else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentFragment)) {
		if (self->kind != K_FRAGMENT) return E_NOINTERFACE;
		*out = &self->fragment;
	} else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlDocument)) {
		if (self->kind != K_DOC) return E_NOINTERFACE;
		*out = &self->document;
	} else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentIO)) {
		if (self->kind != K_DOC) return E_NOINTERFACE;
		*out = &self->documentio;
	} else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentIO2)) {
		if (self->kind != K_DOC) return E_NOINTERFACE;
		*out = &self->documentio2;
	} else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlNamedNodeMap)) {
		if (self->kind != K_ELEMENT) return E_NOINTERFACE;
		*out = &self->map;
	} else
		return E_NOINTERFACE;
	xml_node_addref(self);
	return S_OK;
}

static ULONG xml_node_addref(struct xml_node *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG xml_node_release(struct xml_node *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (refs)
		return (ULONG)refs;
	if (self->doc && self->doc != self)
		xml_node_release(self->doc);
	while (self->first) {
		/* unlink drops the tree's own reference, which frees the child when no one else holds it */
		xml_node_unlink(self->first);
	}
	while (self->attr_count) {
		struct xml_node *attr = self->attrs[--self->attr_count];
		attr->parent = NULL;
		xml_node_release(attr);
	}
	if (self->attrs)
		FREE(self->attrs);
	if (self->name) FREE(self->name);
	if (self->local && self->local != self->name) FREE(self->local);
	if (self->prefix) FREE(self->prefix);
	if (self->value) FREE(self->value);
	if (self->uri) FREE(self->uri);
	FREE(self);
	InterlockedDecrement(&raven_xaml_objects);
	return 0;
}

/* The children and attributes the tree owns hold references; unlink drops the tree's one. */
static void xml_node_unlink(struct xml_node *child)
{
	struct xml_node *parent = child->parent;
	if (!parent)
		return;
	if (child->prev) child->prev->next = child->next;
	else parent->first = child->next;
	if (child->next) child->next->prev = child->prev;
	else parent->last = child->prev;
	child->parent = child->prev = child->next = NULL;
	xml_node_release(child);
}

static void xml_attr_unlink(struct xml_node *element, struct xml_node *attr)
{
	SIZE_T index;
	if (attr->parent != element)
		return;
	for (index = 0; index < element->attr_count; ++index)
		if (element->attrs[index] == attr)
			break;
	if (index == element->attr_count)
		return;
	memmove(element->attrs + index, element->attrs + index + 1,
		(element->attr_count - index - 1) * sizeof *element->attrs);
	--element->attr_count;
	attr->parent = NULL;
	xml_node_release(attr);
}

static int xml_name_split(struct xml_node *node, const WCHAR *text)
{
	SIZE_T length = lstrlenW(text);
	SIZE_T head = 0;
	if (!length)
		return 0;
	while (head < length && text[head] != L':')
		++head;
	node->name = ALLOC((length + 1) * sizeof *node->name);
	if (!node->name)
		return 0;
	lstrcpyW(node->name, text);
	if (head && head < length - 1) {
		SIZE_T tail = length - head - 1;
		node->prefix = ALLOC((head + 1) * sizeof *node->prefix);
		node->local = ALLOC((tail + 1) * sizeof *node->local);
		if (!node->prefix || !node->local)
			return 0;
		memcpy(node->prefix, text, head * sizeof *text);
		node->prefix[head] = 0;
		lstrcpyW(node->local, text + head + 1);
	} else {
		node->local = node->name;
	}
	return 1;
}

static struct xml_node *xml_node_create(struct xml_node *doc, int kind, const WCHAR *name, const WCHAR *value)
{
	struct xml_node *self = ALLOC(sizeof *self);
	if (!self)
		return NULL;
	self->node.lpVtbl = &node_table;
	self->selector.lpVtbl = &selector_table;
	self->serializer.lpVtbl = &serializer_table;
	self->element.lpVtbl = &element_table;
	self->attribute.lpVtbl = &attribute_table;
	self->chardata.lpVtbl = &chardata_table;
	self->textp.lpVtbl = &text_table;
	self->comment.lpVtbl = &comment_table;
	self->cdata.lpVtbl = &cdata_table;
	self->pi.lpVtbl = &pi_table;
	self->fragment.lpVtbl = &fragment_table;
	self->document.lpVtbl = &document_table;
	self->documentio.lpVtbl = &documentio_table;
	self->documentio2.lpVtbl = &documentio2_table;
	self->map.lpVtbl = &map_table;
#define XML_OWNER(part) self->part.owner = self
	XML_OWNER(node); XML_OWNER(selector); XML_OWNER(serializer); XML_OWNER(element);
	XML_OWNER(attribute); XML_OWNER(chardata); XML_OWNER(textp); XML_OWNER(comment);
	XML_OWNER(cdata); XML_OWNER(pi); XML_OWNER(fragment); XML_OWNER(document);
	XML_OWNER(documentio); XML_OWNER(documentio2); XML_OWNER(map);
#undef XML_OWNER
	self->refs = 1;
	self->kind = kind;
	self->doc = doc;
	if (kind == K_DOC) {
		self->doc = self;
	} else {
		xml_node_addref(doc);
		if (name && !xml_name_split(self, name)) {
			xml_node_release(self);
			return NULL;
		}
	}
	if (value) {
		self->value = ALLOC((lstrlenW(value) + 1) * sizeof *self->value);
		if (!self->value) {
			xml_node_release(self);
			return NULL;
		}
		lstrcpyW(self->value, value);
	}
	InterlockedIncrement(&raven_xaml_objects);
	return self;
}

static void xml_node_append(struct xml_node *parent, struct xml_node *child)
{
	if (child->parent)
		xml_node_unlink(child);
	child->parent = parent;
	child->prev = parent->last;
	child->next = NULL;
	if (parent->last) parent->last->next = child;
	else parent->first = child;
	parent->last = child;
	xml_node_addref(child);
}

static int xml_attr_append(struct xml_node *element, struct xml_node *attr)
{
	if (attr->parent && attr->parent != element)
		xml_attr_unlink(attr->parent, attr);
	if (element->attr_count == element->attr_cap) {
		SIZE_T cap = element->attr_cap ? element->attr_cap * 2 : 4;
		struct xml_node **grown = element->attrs ?
			HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, element->attrs, cap * sizeof *grown) :
			HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cap * sizeof *grown);
		if (!grown)
			return 0;
		element->attrs = grown;
		element->attr_cap = cap;
	}
	element->attrs[element->attr_count++] = attr;
	attr->parent = element;
	xml_node_addref(attr);
	return 1;
}

static struct xml_node *xml_node_clone(struct xml_node *doc, struct xml_node *source, int deep)
{
	struct xml_node *copy = xml_node_create(doc ? doc : source->doc, source->kind, source->name, source->value);
	struct xml_node *child;
	SIZE_T index;
	if (!copy)
		return NULL;
	if (source->prefix && copy->prefix) {
		FREE(copy->prefix);
		copy->prefix = ALLOC((lstrlenW(source->prefix) + 1) * sizeof *copy->prefix);
		if (copy->prefix)
			lstrcpyW(copy->prefix, source->prefix);
	}
	if (source->kind == K_ELEMENT)
		for (index = 0; index < source->attr_count; ++index) {
			struct xml_node *attr = xml_node_clone(doc, source->attrs[index], 0);
			if (!attr || !xml_attr_append(copy, attr))
				xml_node_release(attr);
		}
	if (deep)
		for (child = source->first; child; child = child->next) {
			struct xml_node *copy_child = xml_node_clone(doc, child, deep);
			if (copy_child) {
				xml_node_append(copy, copy_child);
				xml_node_release(copy_child);
			}
		}
	return copy;
}

static struct xml_node *xml_element_child(struct xml_node *parent)
{
	struct xml_node *child;
	for (child = parent->first; child; child = child->next)
		if (child->kind == K_ELEMENT)
			return child;
	return NULL;
}

/* The concatenated character data of the subtree, the way IXmlNodeSerializer.InnerText reads. */
static void xml_inner_text(struct xml_node *node, struct xml_buf *out)
{
	struct xml_node *child;
	if (node->kind == K_TEXT || node->kind == K_CDATA) {
		buf_put(out, node->value ? node->value : L"");
		return;
	}
	for (child = node->first; child; child = child->next)
		xml_inner_text(child, out);
}

static void xml_serialize(struct xml_node *node, struct xml_buf *out)
{
	struct xml_node *child;
	SIZE_T index;
	switch (node->kind) {
	case K_DOC:
		for (child = node->first; child; child = child->next)
			xml_serialize(child, out);
		break;
	case K_ELEMENT:
		buf_putc(out, L'<');
		buf_put(out, node->name);
		for (index = 0; index < node->attr_count; ++index) {
			buf_putc(out, L' ');
			buf_put(out, node->attrs[index]->name);
			buf_put(out, L"=\"");
			buf_put_escaped(out, node->attrs[index]->value ? node->attrs[index]->value : L"", TRUE);
			buf_putc(out, L'"');
		}
		if (!node->first) {
			buf_put(out, L"/>");
			break;
		}
		buf_putc(out, L'>');
		for (child = node->first; child; child = child->next)
			xml_serialize(child, out);
		buf_put(out, L"</");
		buf_put(out, node->name);
		buf_putc(out, L'>');
		break;
	case K_ATTR:
		buf_put(out, node->name);
		buf_put(out, L"=\"");
		buf_put_escaped(out, node->value ? node->value : L"", TRUE);
		buf_putc(out, L'"');
		break;
	case K_TEXT:
		buf_put_escaped(out, node->value ? node->value : L"", FALSE);
		break;
	case K_CDATA:
		buf_put(out, L"<![CDATA[");
		buf_put(out, node->value ? node->value : L"");
		buf_put(out, L"]]>");
		break;
	case K_COMMENT:
		buf_put(out, L"<!--");
		buf_put(out, node->value ? node->value : L"");
		buf_put(out, L"-->");
		break;
	case K_PI:
		buf_put(out, L"<?");
		buf_put(out, node->name ? node->name : L"");
		if (node->value && *node->value) {
			buf_putc(out, L' ');
			buf_put(out, node->value);
		}
		buf_put(out, L"?>");
		break;
	case K_FRAGMENT:
		for (child = node->first; child; child = child->next)
			xml_serialize(child, out);
		break;
	}
}

static HRESULT xml_get_xml(struct xml_node *self, HSTRING *out)
{
	struct xml_buf buf = {0};
	HRESULT result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	buf.ok = TRUE;
	xml_serialize(self, &buf);
	if (!buf.ok) {
		if (buf.data) HeapFree(GetProcessHeap(), 0, buf.data);
		return E_OUTOFMEMORY;
	}
	result = buf.len ? WindowsCreateString(buf.data, (UINT32)buf.len, out) : S_OK;
	if (buf.data)
		HeapFree(GetProcessHeap(), 0, buf.data);
	return result;
}

/* ---------------------------------------------------------------- the parser */

struct xml_parser {
	const WCHAR *text, *cur, *end;
};

static int xml_last_stage;

static void trace_at(const char *where, struct xml_parser *parser)
{
	(void)where;
	(void)parser;
#if 0
	WCHAR message[160];
	const WCHAR *scan = L"raven-xml: ";
	UINT32 offset = 11;
	while (*scan)
		message[offset++] = *scan++;
	while (*where && offset < ARRAYSIZE(message) - 14)
		message[offset++] = (WCHAR)*where++;
	message[offset++] = L'@';
	{
		UINT32 value = parser->text ? (UINT32)(parser->cur - parser->text) : 0;
		WCHAR digits[12];
		UINT32 count = 0;
		do {
			digits[count++] = (WCHAR)(L'0' + value % 10);
			value /= 10;
		} while (value);
		while (count && offset < ARRAYSIZE(message) - 1)
			message[offset++] = digits[--count];
	}
	message[offset] = 0;
	OutputDebugStringW(message);
#endif
}


static BOOL p_lit(struct xml_parser *p, const WCHAR *literal)
{
	SIZE_T length = lstrlenW(literal);
	if ((SIZE_T)(p->end - p->cur) < length || memcmp(p->cur, literal, length * sizeof *literal) != 0)
		return FALSE;
	p->cur += length;
	return TRUE;
}

static void p_ws(struct xml_parser *p)
{
	while (p->cur < p->end && (*p->cur == L' ' || *p->cur == L'\t' || *p->cur == L'\r' || *p->cur == L'\n'))
		++p->cur;
}

static BOOL p_name(struct xml_parser *p, WCHAR **out)
{
	const WCHAR *start = p->cur;
	SIZE_T length;
	*out = NULL;
	while (p->cur < p->end) {
		WCHAR c = *p->cur;
		if (c == L':' || c == L'_' || (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || c >= 0x80)
			++p->cur;
		else if (p->cur > start && (c == L'-' || c == L'.' || (c >= L'0' && c <= L'9')))
			++p->cur;
		else
			break;
	}
	if (p->cur == start)
		return FALSE;
	length = (SIZE_T)(p->cur - start);
	*out = ALLOC((length + 1) * sizeof **out);
	if (!*out)
		return FALSE;
	memcpy(*out, start, length * sizeof **out);
	(*out)[length] = 0;
	return TRUE;
}

/* Entities: the five named ones and the numeric forms; anything else is an error. */
static BOOL p_entity(struct xml_parser *p, struct xml_buf *out)
{
	const WCHAR *start = ++p->cur; /* past '&' */
	const WCHAR *scan = start;
	unsigned long value = 0;
	int hex = 0;
	while (scan < p->end && *scan != L';') {
		if (scan - start > 10)
			return FALSE;
		++scan;
	}
	if (scan >= p->end)
		return FALSE;
	if (*start == L'#') {
		const WCHAR *digits = start + 1;
		if (digits < scan && (*digits == L'x' || *digits == L'X')) {
			hex = 1;
			++digits;
		}
		for (; digits < scan; ++digits) {
			unsigned long digit;
			if (*digits >= L'0' && *digits <= L'9') digit = (unsigned long)(*digits - L'0');
			else if (hex && *digits >= L'a' && *digits <= L'f') digit = (unsigned long)(*digits - L'a' + 10);
			else if (hex && *digits >= L'A' && *digits <= L'F') digit = (unsigned long)(*digits - L'A' + 10);
			else return FALSE;
			value = value * (hex ? 16 : 10) + digit;
			if (value > 0x10FFFF)
				return FALSE;
		}
		buf_reserve(out, 2);
		if (!out->ok)
			return FALSE;
		if (value >= 0x10000) {
			value -= 0x10000;
			buf_putc(out, (WCHAR)(0xD800 + (value >> 10)));
			buf_putc(out, (WCHAR)(0xDC00 + (value & 0x3FF)));
		} else {
			buf_putc(out, (WCHAR)value);
		}
	} else {
		SIZE_T length = (SIZE_T)(scan - start);
		if (length == 2 && !memcmp(start, L"lt", 2 * sizeof(WCHAR))) buf_putc(out, L'<');
		else if (length == 2 && !memcmp(start, L"gt", 2 * sizeof(WCHAR))) buf_putc(out, L'>');
		else if (length == 3 && !memcmp(start, L"amp", 3 * sizeof(WCHAR))) buf_putc(out, L'&');
		else if (length == 4 && !memcmp(start, L"quot", 4 * sizeof(WCHAR))) buf_putc(out, L'"');
		else if (length == 4 && !memcmp(start, L"apos", 4 * sizeof(WCHAR))) buf_putc(out, L'\'');
		else return FALSE;
	}
	p->cur = scan + 1;
	return TRUE;
}

/* Text until the next '<', entities decoded. */
static BOOL p_text(struct xml_parser *p, struct xml_buf *out)
{
	const WCHAR *start = p->cur;
	while (p->cur < p->end && *p->cur != L'<') {
		if (*p->cur == L'&') {
			buf_putn(out, start, (SIZE_T)(p->cur - start));
			if (!p_entity(p, out))
				return FALSE;
			start = p->cur;
		} else {
			++p->cur;
		}
	}
	buf_putn(out, start, (SIZE_T)(p->cur - start));
	return TRUE;
}

static BOOL p_skip_pi(struct xml_parser *p)
{
	p->cur += 2; /* '<?' */
	while (p->cur < p->end) {
		if (p->cur + 1 < p->end && p->cur[0] == L'?' && p->cur[1] == L'>') {
			p->cur += 2;
			return TRUE;
		}
		++p->cur;
	}
	return FALSE;
}

static BOOL p_skip_comment(struct xml_parser *p)
{
	p->cur += 4; /* '<!--' */
	while (p->cur < p->end) {
		if (p->cur + 2 < p->end && p->cur[0] == L'-' && p->cur[1] == L'-' && p->cur[2] == L'-')
			return FALSE; /* a bare '--' inside a comment */
		if (p->cur + 2 < p->end && p->cur[0] == L'-' && p->cur[1] == L'-' && p->cur[2] == L'>') {
			p->cur += 3;
			return TRUE;
		}
		++p->cur;
	}
	return FALSE;
}

static BOOL p_skip_doctype(struct xml_parser *p)
{
	int depth = 0;
	p->cur += 9; /* '<!DOCTYPE' */
	while (p->cur < p->end) {
		if (*p->cur == L'[')
			++depth;
		else if (*p->cur == L']')
			--depth;
		else if (*p->cur == L'>' && depth <= 0) {
			++p->cur; /* the doctype's own close */
			return TRUE;
		}
		++p->cur;
	}
	return FALSE;
}

static BOOL p_ws_only(const WCHAR *text, SIZE_T length)
{
	SIZE_T index;
	for (index = 0; index < length; ++index)
		if (text[index] != L' ' && text[index] != L'\t' && text[index] != L'\r' && text[index] != L'\n')
			return FALSE;
	return TRUE;
}

static BOOL p_element(struct xml_parser *p, struct xml_node *doc, struct xml_node *parent, int depth);

static BOOL p_content(struct xml_parser *p, struct xml_node *doc, struct xml_node *parent, int depth)
{
	for (;;) {
		if (p->cur >= p->end)
			return FALSE; /* an unclosed parent element */
		if (*p->cur == L'<') {
			if (p->cur + 1 < p->end && p->cur[1] == L'/') {
				return TRUE; /* the parent's close tag; the caller consumes it */
			}
			if (p_lit(p, L"<!--")) {
				p->cur -= 4;
				if (!p_skip_comment(p)) {
					trace_at("comment", p);
					return FALSE;
				}
				continue;
			}
			if (p_lit(p, L"<![CDATA[")) {
				const WCHAR *start = p->cur;
				while (p->cur + 2 < p->end && !(p->cur[0] == L']' && p->cur[1] == L']' && p->cur[2] == L'>'))
					++p->cur;
				if (p->cur + 2 >= p->end) {
					trace_at("cdata", p);
					return FALSE;
				}
				struct xml_node *cdata = xml_node_create(doc, K_CDATA, NULL, NULL);
				if (!cdata)
					return FALSE;
				cdata->value = ALLOC(((SIZE_T)(p->cur - start) + 1) * sizeof *cdata->value);
				if (!cdata->value) {
					xml_node_release(cdata);
					return FALSE;
				}
				memcpy(cdata->value, start, (SIZE_T)(p->cur - start) * sizeof *start);
				cdata->value[p->cur - start] = 0;
				xml_node_append(parent, cdata);
				xml_node_release(cdata);
				p->cur += 3;
				continue;
			}
			if (p_lit(p, L"<?")) {
				p->cur -= 2;
				if (!p_skip_pi(p))
					return FALSE;
				continue;
			}
			if (p_lit(p, L"<!")) {
				trace_at("bang", p);
				return FALSE; /* a doctype or an unrecognized construct inside element content */
			}
			++p->cur; /* p_element reads from the tag name, the way p_document's caller enters it */
			if (!p_element(p, doc, parent, depth))
				return FALSE;
			continue;
		}
		/* character data */
		{
			struct xml_buf text = {0};
			text.ok = TRUE;
			if (!p_text(p, &text)) {
				trace_at("text", p);
				if (text.data) HeapFree(GetProcessHeap(), 0, text.data);
				return FALSE;
			}
			if (text.len && !p_ws_only(text.data, text.len)) {
				struct xml_node *node = xml_node_create(doc, K_TEXT, NULL, NULL);
				if (!node) {
					HeapFree(GetProcessHeap(), 0, text.data);
					return FALSE;
				}
				node->value = text.data;
				text.data = NULL;
				xml_node_append(parent, node);
				xml_node_release(node);
			}
			if (text.data)
				HeapFree(GetProcessHeap(), 0, text.data);
		}
	}
}

static BOOL p_element(struct xml_parser *p, struct xml_node *doc, struct xml_node *parent, int depth)
{
	WCHAR *name = NULL;
	struct xml_node *element;
	if (depth > 256) {
		trace_at("depth", p);
		return FALSE;
	}
	trace_at("elem", p);
	if (!p_name(p, &name)) {
		trace_at("name", p);
		return FALSE;
	}
	element = xml_node_create(doc, K_ELEMENT, name, NULL);
	FREE(name);
	if (!element) {
		trace_at("create", p);
		return FALSE;
	}
	/* attributes */
	for (;;) {
		WCHAR *attr_name = NULL;
		p_ws(p);
		if (p->cur < p->end && *p->cur == L'>') {
			++p->cur;
			break;
		}
		if (p->cur + 1 < p->end && p->cur[0] == L'/' && p->cur[1] == L'>') {
			p->cur += 2;
			goto done; /* an empty element */
		}
		if (!p_name(p, &attr_name)) {
			trace_at("an", p);
			goto fail;
		}
		p_ws(p);
		if (p->cur >= p->end || *p->cur != L'=') {
			trace_at("ae", p);
			FREE(attr_name);
			goto fail;
		}
		++p->cur;
		p_ws(p);
		if (p->cur >= p->end || (*p->cur != L'"' && *p->cur != L'\'')) {
			trace_at("aq", p);
			FREE(attr_name);
			goto fail;
		}
		{
			WCHAR quote = *p->cur++;
			struct xml_buf value = {0};
			BOOL ok;
			value.ok = TRUE;
			ok = p_text_quoted(p, &value, quote);
			if (ok) {
				struct xml_node *attr = xml_node_create(doc, K_ATTR, attr_name, NULL);
				if (!attr) ok = FALSE;
				else {
					attr->value = value.data;
					value.data = NULL;
					if (!xml_attr_append(element, attr)) ok = FALSE;
					xml_node_release(attr);
				}
			}
			if (value.data) HeapFree(GetProcessHeap(), 0, value.data);
			FREE(attr_name);
			if (!ok) {
				trace_at("av", p);
				goto fail;
			}
		}
	}
	if (!p_content(p, doc, element, depth + 1)) {
		trace_at("content", p);
		goto fail;
	}
	/* the close tag */
	if (!p_lit(p, L"</")) {
		trace_at("close-open", p);
		goto fail;
	}
	{
		WCHAR *close = NULL;
		BOOL ok = p_name(p, &close);
		if (ok) {
			ok = !lstrcmpW(close, element->name);
			FREE(close);
		}
		if (!ok) {
			trace_at("close-name", p);
			goto fail;
		}
	}
	p_ws(p);
	if (p->cur >= p->end || *p->cur != L'>') {
		trace_at("close-gt", p);
		goto fail;
	}
	++p->cur;
done:
	xml_node_append(parent, element);
	xml_node_release(element);
	return TRUE;
fail:
	xml_node_release(element);
	return FALSE;
}

/* Quoted attribute value: like p_text but bounded by the quote, entities decoded. */
static BOOL p_text_quoted(struct xml_parser *p, struct xml_buf *out, WCHAR quote)
{
	const WCHAR *start = p->cur;
	while (p->cur < p->end && *p->cur != quote) {
		if (*p->cur == L'<')
			return FALSE;
		if (*p->cur == L'&') {
			buf_putn(out, start, (SIZE_T)(p->cur - start));
			if (!p_entity(p, out))
				return FALSE;
			start = p->cur;
		} else {
			++p->cur;
		}
	}
	if (p->cur >= p->end)
		return FALSE;
	buf_putn(out, start, (SIZE_T)(p->cur - start));
	++p->cur; /* the quote */
	return TRUE;
}

static BOOL p_prolog(struct xml_parser *p)
{
	if (p->cur < p->end && *p->cur == 0xFEFF)
		++p->cur; /* a byte-order mark the text form carried along */
	for (;;) {
		p_ws(p);
		if (p->cur + 1 < p->end && p->cur[0] == L'<' && p->cur[1] == L'?') {
			if (!p_skip_pi(p))
				return FALSE;
		} else if (p->cur + 3 < p->end && p->cur[0] == L'<' && p->cur[1] == L'!' && p->cur[2] == L'-' && p->cur[3] == L'-') {
			if (!p_skip_comment(p))
				return FALSE;
		} else if ((SIZE_T)(p->end - p->cur) >= 9 && !memcmp(p->cur, L"<!DOCTYPE", 9 * sizeof(WCHAR))) {
			if (!p_skip_doctype(p))
				return FALSE;
		} else {
			return TRUE;
		}
	}
}

/* Parse a whole document: the prolog, one root element, trailing whitespace. */
static BOOL p_document(struct xml_parser *p, struct xml_node *doc)
{
	trace_at("document", p);
	if (!p_prolog(p))
		return FALSE;
	p_ws(p);
	if (p->cur >= p->end || *p->cur != L'<')
		return FALSE;
	++p->cur;
	if (!p_element(p, doc, doc, 0))
		return FALSE;
	p_ws(p);
	if (p->cur < p->end)
		trace_at("tail", p);
	return p->cur >= p->end;
}

static HRESULT xml_doc_load(struct xml_node *doc, const WCHAR *text)
{
	struct xml_parser parser = {text, text, text + lstrlenW(text)};
	while (doc->first)
		xml_node_unlink(doc->first);
	while (doc->attr_count)
		xml_attr_unlink(doc, doc->attrs[doc->attr_count - 1]);
	if (doc->uri) {
		FREE(doc->uri);
		doc->uri = NULL;
	}
	if (!p_document(&parser, doc)) {
		/* the document around the failure, while the app's XML is under diagnosis */
		SIZE_T length = lstrlenW(text);
		SIZE_T at = (SIZE_T)(parser.cur - text);
		SIZE_T begin = at > 100 ? at - 100 : 0;
		SIZE_T stop = at + 100 < length ? at + 100 : length;
		WCHAR window[210];
		UINT32 out = 0;
		window[out++] = L'@';
		{
			WCHAR digits[12];
			UINT32 count = 0;
			UINT32 value = (UINT32)at;
			do {
				digits[count++] = (WCHAR)(L'0' + value % 10);
				value /= 10;
			} while (value);
			while (count && out < ARRAYSIZE(window) - 2)
				window[out++] = digits[--count];
			window[out++] = L':';
		}
		for (SIZE_T walk = begin; walk < stop && out < ARRAYSIZE(window) - 2; ++walk) {
			WCHAR c = text[walk];
			window[out++] = (c >= 32 && c < 127) || c == L'\n' ? c : L'?';
		}
		if (at < length) {
			/* the failing unit, as its code point */
			WCHAR c = text[at];
			static const WCHAR digits[] = L"0123456789abcdef";
			window[out++] = L'|';
			for (int shift = 12; shift >= 0 && out < ARRAYSIZE(window) - 5; shift -= 4)
				window[out++] = digits[(c >> shift) & 0xF];
		}
		window[out] = 0;
		OutputDebugStringW(window);
		return (HRESULT)(0x8000DD00 | (at & 0xFF)); /* the position rides the code while the app's document is under diagnosis */
	}
	return S_OK;
}

/* ---------------------------------------------------------------- XmlNodeList */

struct xml_list {
	const nodelist_iVtbl *lpVtbl;
	LONG refs;
	SIZE_T count;
	struct xml_node **items;
};

static const nodelist_iVtbl list_table;

static HRESULT WINAPI list_query(nodelist_i *iface, REFIID iid, void **out)
{
	struct xml_list *self = (void *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeList) && !equal_iid(iid, &IID_IAgileObject))
		return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI list_addref(nodelist_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct xml_list *)iface)->refs);
}

static ULONG WINAPI list_release(nodelist_i *iface)
{
	struct xml_list *self = (void *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		SIZE_T index;
		for (index = 0; index < self->count; ++index)
			xml_node_release(self->items[index]);
		if (self->items)
			FREE(self->items);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI list_iids(nodelist_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI list_class(nodelist_i *iface, HSTRING *out)
{
	(void)iface;
	return create_string(L"Windows.Data.Xml.Dom.XmlNodeList", out);
}

static HRESULT WINAPI list_trust(nodelist_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI list_length(nodelist_i *iface, UINT32 *out)
{
	struct xml_list *self = (void *)iface;
	if (!out)
		return E_POINTER;
	*out = (UINT32)self->count;
	return S_OK;
}

static HRESULT WINAPI list_item(nodelist_i *iface, UINT32 index, node_i **out)
{
	struct xml_list *self = (void *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if ((SIZE_T)index >= self->count)
		return E_BOUNDS;
	*out = (node_i *)self->items[index];
	xml_node_addref(self->items[index]);
	return S_OK;
}

static const nodelist_iVtbl list_table = {
	list_query, list_addref, list_release, list_iids, list_class, list_trust, list_length, list_item,
};

struct xml_vec {
	struct xml_node **items;
	SIZE_T count, cap;
	BOOL ok;
};

static void vec_push(struct xml_vec *v, struct xml_node *node)
{
	if (!v->ok)
		return;
	if (v->count == v->cap) {
		SIZE_T cap = v->cap ? v->cap * 2 : 8;
		struct xml_node **grown = v->items ?
			HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, v->items, cap * sizeof *grown) :
			HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cap * sizeof *grown);
		if (!grown) {
			v->ok = FALSE;
			return;
		}
		v->items = grown;
		v->cap = cap;
	}
	v->items[v->count++] = node;
	xml_node_addref(node);
}

static void vec_free(struct xml_vec *v)
{
	SIZE_T index;
	for (index = 0; index < v->count; ++index)
		xml_node_release(v->items[index]);
	if (v->items)
		FREE(v->items);
	v->items = NULL;
	v->count = v->cap = 0;
}

static HRESULT list_from_vec(struct xml_vec *v, nodelist_i **out)
{
	struct xml_list *list;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!v->ok)
		return E_OUTOFMEMORY;
	list = ALLOC(sizeof *list);
	if (!list) {
		vec_free(v);
		return E_OUTOFMEMORY;
	}
	list->lpVtbl = &list_table;
	list->refs = 1;
	list->count = v->count;
	list->items = v->items;
	v->items = NULL;
	v->count = v->cap = 0;
	InterlockedIncrement(&raven_xaml_objects);
	*out = (nodelist_i *)list;
	return S_OK;
}

static void vec_descendants(struct xml_node *node, struct xml_vec *v)
{
	struct xml_node *child;
	for (child = node->first; child; child = child->next) {
		vec_push(v, child);
		vec_descendants(child, v);
	}
}

/* ---------------------------------------------------------------- XmlNamedNodeMap */

static HRESULT WINAPI map_length(namedmap_i *iface, UINT32 *out)
{
	struct xml_node *element = ((struct map_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	*out = (UINT32)element->attr_count;
	return S_OK;
}

static HRESULT WINAPI map_item(namedmap_i *iface, UINT32 index, node_i **out)
{
	struct xml_node *element = ((struct map_part *)iface)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if ((SIZE_T)index >= element->attr_count)
		return E_BOUNDS;
	*out = (node_i *)element->attrs[index];
	xml_node_addref(element->attrs[index]);
	return S_OK;
}

static struct xml_node *map_find(struct xml_node *element, const WCHAR *name)
{
	SIZE_T index;
	for (index = 0; index < element->attr_count; ++index)
		if (!lstrcmpW(element->attrs[index]->name, name))
			return element->attrs[index];
	return NULL;
}

static HRESULT WINAPI map_get(namedmap_i *iface, HSTRING name, node_i **out)
{
	struct xml_node *element = ((struct map_part *)iface)->owner;
	const WCHAR *text;
	HRESULT result = hstring_text(name, &text);
	if (FAILED(result))
		return result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	{
		struct xml_node *attr = map_find(element, text);
		if (attr) {
			*out = (node_i *)attr;
			xml_node_addref(attr);
		}
	}
	return S_OK;
}

static HRESULT WINAPI map_set(namedmap_i *iface, node_i *node, node_i **out)
{
	struct xml_node *element = ((struct map_part *)iface)->owner;
	struct xml_node *attr = (struct xml_node *)node;
	struct xml_node *previous;
	if (!out || !node)
		return E_POINTER;
	*out = NULL;
	if (attr->kind != K_ATTR)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:1637\n"); return E_INVALIDARG; }
	previous = map_find(element, attr->name);
	if (previous) {
		*out = (node_i *)previous;
		xml_node_addref(previous);
		xml_attr_unlink(element, previous);
	}
	if (!xml_attr_append(element, attr)) {
		if (*out) xml_node_release((struct xml_node *)*out);
		*out = NULL;
		return E_OUTOFMEMORY;
	}
	return S_OK;
}

static HRESULT WINAPI map_remove(namedmap_i *iface, HSTRING name, node_i **out)
{
	struct xml_node *element = ((struct map_part *)iface)->owner;
	const WCHAR *text;
	HRESULT result = hstring_text(name, &text);
	if (FAILED(result))
		return result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	{
		struct xml_node *attr = map_find(element, text);
		if (attr) {
			*out = (node_i *)attr;
			xml_node_addref(attr);
			xml_attr_unlink(element, attr);
		}
	}
	return S_OK;
}

/* The NS forms match local names: the DOM tracks no namespace URIs (see the header comment). */
static HRESULT WINAPI map_get_ns(namedmap_i *iface, IInspectable *ns, HSTRING local, node_i **out)
{
	(void)ns;
	return map_get(iface, local, out);
}

static HRESULT WINAPI map_remove_ns(namedmap_i *iface, IInspectable *ns, HSTRING local, node_i **out)
{
	(void)ns;
	return map_remove(iface, local, out);
}

static const struct xml_map_vtbl map_table = {
	map_query, map_addref, map_release, map_iids, map_class, map_trust,
	map_length, map_item, map_get, map_set, map_remove, map_get_ns, map_remove_ns, map_set,
};

/* ---------------------------------------------------------------- XPath subset */

struct xstep {
	int desc;   /* preceded by '//' */
	int attr;   /* '@name' */
	int any;    /* '*' */
	int index;  /* [n], 1-based, 0 when absent */
	int last;   /* [last()] */
	WCHAR *name;
};

static void xsteps_free(struct xstep *steps, int count)
{
	int index;
	for (index = 0; index < count; ++index)
		if (steps[index].name)
			FREE(steps[index].name);
	if (steps)
		FREE(steps);
}

static BOOL xpath_step(struct xml_parser *p, struct xstep *step)
{
	WCHAR *text = NULL;
	SIZE_T length;
	const WCHAR *bracket = NULL;
	if (*p->cur == L'@') {
		++p->cur;
		step->attr = 1;
	}
	{
		const WCHAR *start = p->cur;
		while (p->cur < p->end && *p->cur != L'/' && *p->cur != L'[')
			++p->cur;
		length = (SIZE_T)(p->cur - start);
		if (!length)
			return FALSE;
		text = ALLOC((length + 1) * sizeof *text);
		if (!text)
			return FALSE;
		memcpy(text, start, length * sizeof *text);
		text[length] = 0;
	}
	const WCHAR *bracket_end = NULL;
	if (*p->cur == L'[') {
		bracket = ++p->cur;
		while (p->cur < p->end && *p->cur != L']')
			++p->cur;
		if (p->cur >= p->end) {
			FREE(text);
			return FALSE;
		}
		bracket_end = p->cur; /* the ']' the digits run to */
		++p->cur; /* ']' */
	}
	if (!step->attr && !lstrcmpW(text, L"*")) {
		step->any = 1;
		FREE(text);
	} else
		step->name = text;
	if (bracket) {
		if (bracket[0] == L'l' && (SIZE_T)(p->cur - bracket) >= 7 && !memcmp(bracket, L"last()", 6 * sizeof(WCHAR)))
			step->last = 1;
		else {
			unsigned long value = 0;
			const WCHAR *digits = bracket;
			while (digits < bracket_end && *digits >= L'0' && *digits <= L'9') {
				value = value * 10 + (unsigned long)(*digits - L'0');
				if (value > 1000000)
					return FALSE;
				++digits;
			}
			if (digits != bracket_end || !value)
				return FALSE;
			step->index = (int)value;
		}
	}
	return TRUE;
}

static BOOL xpath_parse(const WCHAR *path, struct xstep **out, int *out_count)
{
	struct xml_parser p = {NULL, path, path + lstrlenW(path)};
	struct xstep *steps = NULL;
	int count = 0, cap = 0;
	BOOL desc = 0;
	*out = NULL;
	*out_count = 0;
	for (;;) {
		struct xstep step = {0};
		if (p.cur < p.end && *p.cur == L'/') {
			++p.cur;
			if (p.cur < p.end && *p.cur == L'/') {
				++p.cur;
				desc = 1;
			}
			continue;
		}
		if (p.cur >= p.end)
			break;
		step.desc = desc;
		desc = 0;
		if (count == cap) {
			int grown_cap = cap ? cap * 2 : 8;
			struct xstep *grown = steps ?
				HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, steps, grown_cap * sizeof *grown) :
				HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, grown_cap * sizeof *grown);
			if (!grown) {
				xsteps_free(steps, count);
				return FALSE;
			}
			steps = grown;
			cap = grown_cap;
		}
		if (!xpath_step(&p, &step)) {
			if (step.name) FREE(step.name);
			xsteps_free(steps, count);
			return FALSE;
		}
		steps[count++] = step;
	}
	if (!count || desc) {
		xsteps_free(steps, count);
		return FALSE;
	}
	*out = steps;
	*out_count = count;
	return TRUE;
}

static BOOL name_matches(const struct xstep *step, struct xml_node *node)
{
	if (step->any)
		return TRUE;
	if (!step->name)
		return FALSE;
	return node->name && !lstrcmpW(node->name, step->name);
}

static void xpath_apply(struct xml_node *node, const struct xstep *step, struct xml_vec *out)
{
	if (step->attr) {
		if (node->kind != K_ELEMENT)
			return;
		if (step->any) {
			SIZE_T index;
			for (index = 0; index < node->attr_count; ++index)
				vec_push(out, node->attrs[index]);
		} else {
			struct xml_node *attr = map_find(node, step->name);
			if (attr)
				vec_push(out, attr);
		}
		return;
	}
	{
		struct xml_node *child;
		for (child = node->first; child; child = child->next)
			if (child->kind == K_ELEMENT && name_matches(step, child))
				vec_push(out, child);
	}
}

/* One path evaluation; the vector holds addref'd nodes and passes to the caller. */
static BOOL xpath_run(struct xml_node *ctx, const struct xstep *steps, int count, struct xml_vec *out)
{
	struct xml_vec cur = {0};
	int index;
	cur.ok = TRUE;
	if (steps[0].desc) {
		/* '//' from the context: the context and its whole subtree */
		vec_push(&cur, ctx);
		vec_descendants(ctx, &cur);
	} else
		vec_push(&cur, ctx);
	if (steps[0].attr && ctx->kind == K_DOC) {
		/* attributes select nothing from a document; the empty result is honest */
	}
	for (index = 0; index < count; ++index) {
		struct xml_vec next = {0};
		SIZE_T item;
		next.ok = TRUE;
		if (steps[index].desc) {
			struct xml_vec subtree = {0};
			subtree.ok = TRUE;
			for (item = 0; item < cur.count; ++item) {
				vec_push(&subtree, cur.items[item]);
				vec_descendants(cur.items[item], &subtree);
			}
			for (item = 0; item < subtree.count; ++item)
				xpath_apply(subtree.items[item], &steps[index], &next);
			vec_free(&subtree);
		} else {
			for (item = 0; item < cur.count; ++item)
				xpath_apply(cur.items[item], &steps[index], &next);
		}
		if (steps[index].index && next.count > (SIZE_T)steps[index].index) {
			/* keep only the n-th */
			struct xml_node *keep = next.items[steps[index].index - 1];
			SIZE_T position;
			for (position = 0; position < next.count; ++position)
				if (position != (SIZE_T)steps[index].index - 1)
					xml_node_release(next.items[position]);
			next.items[0] = keep;
			next.count = 1;
		} else if (steps[index].last && next.count > 1) {
			struct xml_node *keep = next.items[next.count - 1];
			SIZE_T position;
			for (position = 0; position < next.count - 1; ++position)
				xml_node_release(next.items[position]);
			next.items[0] = keep;
			next.count = 1;
		}
		vec_free(&cur);
		cur = next;
	}
	*out = cur;
	return cur.ok;
}

static HRESULT xml_select(struct xml_node *ctx, HSTRING path, nodelist_i **list_out, node_i **single_out)
{
	const WCHAR *text;
	struct xstep *steps = NULL;
	int count = 0;
	struct xml_vec result = {0};
	HRESULT result_hresult;
	if ((result_hresult = hstring_text(path, &text)) != S_OK)
		return result_hresult;
	if (!list_out && !single_out)
		return E_POINTER;
	if (!xpath_parse(text, &steps, &count)) {
		if (list_out) *list_out = NULL;
		if (single_out) *single_out = NULL;
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:1925\n"); return E_INVALIDARG; }
	}
	/* an absolute path selects from the document */
	if (ctx->kind != K_DOC && ctx->doc && text[0] == L'/')
		ctx = ctx->doc;
	if (!xpath_run(ctx, steps, count, &result)) {
		xsteps_free(steps, count);
		vec_free(&result);
		if (list_out) *list_out = NULL;
		if (single_out) *single_out = NULL;
		return E_OUTOFMEMORY;
	}
	xsteps_free(steps, count);
	if (single_out) {
		*single_out = NULL;
		if (result.count) {
			*single_out = (node_i *)result.items[0];
			xml_node_addref(result.items[0]);
		}
		vec_free(&result);
		return S_OK;
	}
	return list_from_vec(&result, list_out);
}

/* ---------------------------------------------------------------- IXmlNode */

static node_type kind_of_part(struct node_part *part) { return xml_kind_type(part->owner->kind); }

static HRESULT WINAPI node_get_value(node_i *p, IInspectable **out)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	switch (self->kind) {
	case K_TEXT:
	case K_COMMENT:
	case K_CDATA:
		return box_string(self->value ? self->value : L"", out);
	case K_ATTR:
		return box_string(self->value ? self->value : L"", out);
	case K_PI:
		return box_string(self->value ? self->value : L"", out);
	}
	return S_OK; /* element, document and fragment values are null */
}

static HRESULT WINAPI node_put_value(node_i *p, IInspectable *value)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	const WCHAR *text;
	HRESULT result;
	if (self->kind != K_TEXT && self->kind != K_CDATA && self->kind != K_COMMENT && self->kind != K_PI &&
		self->kind != K_ATTR)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:1980\n"); return E_INVALIDARG; }
	result = unbox_string(value, &text);
	if (FAILED(result))
		return result;
	{
		SIZE_T length = text ? lstrlenW(text) : 0;
		WCHAR *copy = ALLOC((length + 1) * sizeof *copy);
		if (!copy)
			return E_OUTOFMEMORY;
		if (text)
			lstrcpyW(copy, text);
		if (self->value)
			FREE(self->value);
		self->value = copy;
	}
	return S_OK;
}

static HRESULT WINAPI node_get_type(node_i *p, node_type *out)
{
	if (!out)
		return E_POINTER;
	*out = kind_of_part((struct node_part *)p);
	return S_OK;
}

static HRESULT WINAPI node_get_name(node_i *p, HSTRING *out)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	switch (self->kind) {
	case K_ELEMENT:
	case K_ATTR:
	case K_PI:
		return create_string(self->name, out);
	case K_DOC:
		return create_string(L"#document", out);
	case K_FRAGMENT:
		return create_string(L"#document-fragment", out);
	case K_TEXT:
		return create_string(L"#text", out);
	case K_CDATA:
		return create_string(L"#cdata-section", out);
	case K_COMMENT:
		return create_string(L"#comment", out);
	}
	return create_string(L"#invalid", out);
}

static HRESULT WINAPI node_get_parent(node_i *p, node_i **out)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (self->parent) {
		*out = (node_i *)self->parent;
		xml_node_addref(self->parent);
	}
	return S_OK;
}

static HRESULT WINAPI node_get_children(node_i *p, nodelist_i **out)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	struct xml_vec vec = {0};
	if (!out)
		return E_POINTER;
	*out = NULL;
	vec.ok = TRUE;
	{
		struct xml_node *child;
		for (child = self->first; child; child = child->next)
			vec_push(&vec, child);
	}
	return list_from_vec(&vec, out);
}

static HRESULT WINAPI node_get_first(node_i *p, node_i **out)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (self->first) {
		*out = (node_i *)self->first;
		xml_node_addref(self->first);
	}
	return S_OK;
}

static HRESULT WINAPI node_get_last(node_i *p, node_i **out)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (self->last) {
		*out = (node_i *)self->last;
		xml_node_addref(self->last);
	}
	return S_OK;
}

static HRESULT WINAPI node_get_previous(node_i *p, node_i **out)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (self->prev) {
		*out = (node_i *)self->prev;
		xml_node_addref(self->prev);
	}
	return S_OK;
}

static HRESULT WINAPI node_get_next(node_i *p, node_i **out)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (self->next) {
		*out = (node_i *)self->next;
		xml_node_addref(self->next);
	}
	return S_OK;
}

static HRESULT WINAPI node_get_attributes(node_i *p, namedmap_i **out)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (self->kind != K_ELEMENT)
		return S_OK;
	*out = (namedmap_i *)&self->map;
	xml_node_addref(self);
	return S_OK;
}

static HRESULT WINAPI node_has_children(node_i *p, boolean *out)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = self->first ? TRUE : FALSE;
	return S_OK;
}

static HRESULT WINAPI node_get_owner(node_i *p, document_i **out)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = (document_i *)&self->doc->document;
	xml_node_addref(self->doc);
	return S_OK;
}

static HRESULT WINAPI node_insert_before(node_i *p, node_i *child, node_i *reference, node_i **inserted)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	struct xml_node *new_child = (struct xml_node *)child;
	struct xml_node *ref = (struct xml_node *)reference;
	struct xml_node *before;
	if (!inserted)
		return E_POINTER;
	*inserted = NULL;
	if (!new_child || new_child == self || new_child->kind == K_DOC)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:2154\n"); return E_INVALIDARG; }
	if (ref && ref->parent != self)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:2156\n"); return E_INVALIDARG; }
	before = ref;
	if (new_child->parent)
		xml_node_unlink(new_child);
	if (before) {
		new_child->parent = self;
		new_child->prev = before->prev;
		new_child->next = before;
		if (before->prev) before->prev->next = new_child;
		else self->first = new_child;
		before->prev = new_child;
	} else {
		new_child->parent = self;
		new_child->prev = self->last;
		new_child->next = NULL;
		if (self->last) self->last->next = new_child;
		else self->first = new_child;
		self->last = new_child;
	}
	xml_node_addref(new_child);
	*inserted = child;
	xml_node_addref(new_child);
	return S_OK;
}

static HRESULT WINAPI node_replace_child(node_i *p, node_i *child, node_i *reference, node_i **previous)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	struct xml_node *old = (struct xml_node *)reference;
	HRESULT result;
	if (!previous)
		return E_POINTER;
	*previous = NULL;
	if (!old || old->parent != self)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:2190\n"); return E_INVALIDARG; }
	result = node_insert_before(p, child, reference, NULL);
	if (FAILED(result))
		return result;
	*previous = reference;
	xml_node_addref(old);
	xml_node_unlink(old);
	return S_OK;
}

static HRESULT WINAPI node_remove_child(node_i *p, node_i *child, node_i **removed)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	struct xml_node *old = (struct xml_node *)child;
	if (!removed)
		return E_POINTER;
	*removed = NULL;
	if (!old || old->parent != self)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:2208\n"); return E_INVALIDARG; }
	xml_node_addref(old); /* the tree's reference dies in the unlink */
	xml_node_unlink(old);
	*removed = child;
	return S_OK;
}

static HRESULT WINAPI node_append_child(node_i *p, node_i *child, node_i **appended)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	struct xml_node *new_child = (struct xml_node *)child;
	if (!appended)
		return E_POINTER;
	*appended = NULL;
	if (!new_child || new_child == self || new_child->kind == K_DOC)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:2223\n"); return E_INVALIDARG; }
	xml_node_append(self, new_child);
	*appended = child;
	xml_node_addref(new_child);
	return S_OK;
}

static HRESULT WINAPI node_clone(node_i *p, boolean deep, node_i **out)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	struct xml_node *copy;
	if (!out)
		return E_POINTER;
	*out = NULL;
	copy = xml_node_clone(NULL, self, deep ? 1 : 0);
	if (!copy)
		return E_OUTOFMEMORY;
	*out = (node_i *)copy;
	return S_OK;
}

/* The namespace getters answer boxed strings; without namespace tracking the URI is empty. */
static HRESULT WINAPI node_get_ns_uri(node_i *p, IInspectable **out)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	(void)self;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return box_string(L"", out);
}

static HRESULT WINAPI node_get_local(node_i *p, IInspectable **out)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (self->kind != K_ELEMENT && self->kind != K_ATTR)
		return box_string(L"", out);
	return box_string(self->local ? self->local : L"", out);
}

static HRESULT WINAPI node_get_prefix(node_i *p, IInspectable **out)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (self->kind != K_ELEMENT && self->kind != K_ATTR)
		return box_string(L"", out);
	return box_string(self->prefix ? self->prefix : L"", out);
}

static HRESULT WINAPI node_normalize(node_i *p)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	struct xml_node *child = self->first;
	while (child) {
		struct xml_node *next = child->next;
		if (child->kind == K_TEXT && next && next->kind == K_TEXT) {
			SIZE_T length = lstrlenW(child->value ? child->value : L"") + lstrlenW(next->value ? next->value : L"");
			WCHAR *merged = ALLOC((length + 1) * sizeof *merged);
			if (merged) {
				merged[0] = 0;
				if (child->value) lstrcpyW(merged, child->value);
				if (next->value) lstrcatW(merged, next->value);
				if (child->value) FREE(child->value);
				child->value = merged;
			}
			xml_node_unlink(next);
			continue;
		}
		child = next;
	}
	return S_OK;
}

static HRESULT WINAPI node_put_prefix(node_i *p, IInspectable *value)
{
	struct xml_node *self = ((struct node_part *)p)->owner;
	const WCHAR *text;
	HRESULT result;
	if (self->kind != K_ELEMENT && self->kind != K_ATTR)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:2307\n"); return E_INVALIDARG; }
	result = unbox_string(value, &text);
	if (FAILED(result))
		return result;
	/* renaming through the prefix alone would desynchronize the qualified name; refuse it */
	(void)text;
	return E_NOTIMPL;
}

static const node_iVtbl node_table = {
	node_query, node_addref, node_release, node_iids, node_class, node_trust,
	node_get_value, node_put_value, node_get_type, node_get_name, node_get_parent, node_get_children,
	node_get_first, node_get_last, node_get_previous, node_get_next, node_get_attributes,
	node_has_children, node_get_owner, node_insert_before, node_replace_child, node_remove_child,
	node_append_child, node_clone, node_get_ns_uri, node_get_local, node_get_prefix, node_normalize,
	node_put_prefix,
};

/* ---------------------------------------------------------------- selector & serializer */

static HRESULT WINAPI selector_single(selector_i *p, HSTRING path, node_i **out)
{
	return xml_select(((struct selector_part *)p)->owner, path, NULL, out);
}

static HRESULT WINAPI selector_many(selector_i *p, HSTRING path, nodelist_i **out)
{
	return xml_select(((struct selector_part *)p)->owner, path, out, NULL);
}

static HRESULT WINAPI selector_single_ns(selector_i *p, HSTRING path, IInspectable *namespaces, node_i **out)
{
	(void)namespaces; /* qualified names match literally; see the header comment */
	return xml_select(((struct selector_part *)p)->owner, path, NULL, out);
}

static HRESULT WINAPI selector_many_ns(selector_i *p, HSTRING path, IInspectable *namespaces, nodelist_i **out)
{
	(void)namespaces;
	return xml_select(((struct selector_part *)p)->owner, path, out, NULL);
}

static const selector_iVtbl selector_table = {
	selector_query, selector_addref, selector_release, selector_iids, selector_class, selector_trust,
	selector_single, selector_many, selector_single_ns, selector_many_ns,
};

static HRESULT WINAPI serializer_get_xml(serializer_i *p, HSTRING *out)
{
	return xml_get_xml(((struct serializer_part *)p)->owner, out);
}

static HRESULT WINAPI serializer_get_text(serializer_i *p, HSTRING *out)
{
	struct xml_buf buf = {0};
	HRESULT result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	buf.ok = TRUE;
	xml_inner_text(((struct serializer_part *)p)->owner, &buf);
	if (!buf.ok) {
		if (buf.data) HeapFree(GetProcessHeap(), 0, buf.data);
		return E_OUTOFMEMORY;
	}
	result = buf.len ? WindowsCreateString(buf.data, (UINT32)buf.len, out) : S_OK;
	if (buf.data)
		HeapFree(GetProcessHeap(), 0, buf.data);
	return result;
}

static HRESULT WINAPI serializer_put_text(serializer_i *p, HSTRING value)
{
	struct xml_node *self = ((struct serializer_part *)p)->owner;
	const WCHAR *text;
	HRESULT result = hstring_text(value, &text);
	if (FAILED(result))
		return result;
	while (self->first)
		xml_node_unlink(self->first);
	{
		struct xml_node *node = xml_node_create(self->doc, K_TEXT, NULL, text);
		if (!node)
			return E_OUTOFMEMORY;
		xml_node_append(self, node);
		xml_node_release(node);
	}
	return S_OK;
}

static const serializer_iVtbl serializer_table = {
	serializer_query, serializer_addref, serializer_release, serializer_iids, serializer_class, serializer_trust,
	serializer_get_xml, serializer_get_text, serializer_put_text,
};

/* ---------------------------------------------------------------- IXmlElement */

static HRESULT WINAPI element_tag(element_i *p, HSTRING *out)
{
	struct xml_node *self = ((struct element_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return create_string(self->name, out);
}

static HRESULT WINAPI element_get_attribute(element_i *p, HSTRING name, HSTRING *out)
{
	struct xml_node *self = ((struct element_part *)p)->owner;
	const WCHAR *text;
	struct xml_node *attr;
	HRESULT result = hstring_text(name, &text);
	if (FAILED(result))
		return result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	attr = map_find(self, text);
	return create_string(attr && attr->value ? attr->value : L"", out);
}

static HRESULT WINAPI element_set_attribute(element_i *p, HSTRING name, HSTRING value)
{
	struct xml_node *self = ((struct element_part *)p)->owner;
	const WCHAR *name_text, *value_text;
	struct xml_node *attr;
	HRESULT result;
	if (FAILED((result = hstring_text(name, &name_text))) || FAILED((result = hstring_text(value, &value_text))))
		return result;
	if (!*name_text)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:2437\n"); return E_INVALIDARG; }
	attr = map_find(self, name_text);
	if (!attr) {
		attr = xml_node_create(self->doc, K_ATTR, name_text, NULL);
		if (!attr)
			return E_OUTOFMEMORY;
		if (!xml_attr_append(self, attr)) {
			xml_node_release(attr);
			return E_OUTOFMEMORY;
		}
		xml_node_release(attr); /* the element's reference keeps it */
	}
	{
		SIZE_T length = lstrlenW(value_text);
		WCHAR *copy = ALLOC((length + 1) * sizeof *copy);
		if (!copy)
			return E_OUTOFMEMORY;
		lstrcpyW(copy, value_text);
		if (attr->value)
			FREE(attr->value);
		attr->value = copy;
	}
	return S_OK;
}

static HRESULT WINAPI element_remove_attribute(element_i *p, HSTRING name)
{
	struct xml_node *self = ((struct element_part *)p)->owner;
	const WCHAR *text;
	struct xml_node *attr;
	HRESULT result = hstring_text(name, &text);
	if (FAILED(result))
		return result;
	attr = map_find(self, text);
	if (attr)
		xml_attr_unlink(self, attr);
	return S_OK;
}

static HRESULT WINAPI element_get_attribute_node(element_i *p, HSTRING name, attribute_i **out)
{
	struct xml_node *self = ((struct element_part *)p)->owner;
	const WCHAR *text;
	struct xml_node *attr;
	HRESULT result = hstring_text(name, &text);
	if (FAILED(result))
		return result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	attr = map_find(self, text);
	if (attr) {
		*out = (attribute_i *)&attr->attribute;
		xml_node_addref(attr);
	}
	return S_OK;
}

static HRESULT WINAPI element_set_attribute_node(element_i *p, attribute_i *attribute, attribute_i **previous)
{
	struct xml_node *self = ((struct element_part *)p)->owner;
	struct xml_node *attr = (struct xml_node *)attribute;
	struct xml_node *old;
	if (!previous)
		return E_POINTER;
	*previous = NULL;
	if (!attr || attr->kind != K_ATTR)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:2504\n"); return E_INVALIDARG; }
	old = map_find(self, attr->name);
	if (old) {
		*previous = (attribute_i *)&old->attribute;
		xml_node_addref(old);
		xml_attr_unlink(self, old);
	}
	if (!xml_attr_append(self, attr))
		return E_OUTOFMEMORY;
	return S_OK;
}

static HRESULT WINAPI element_remove_attribute_node(element_i *p, attribute_i *attribute, attribute_i **removed)
{
	struct xml_node *self = ((struct element_part *)p)->owner;
	struct xml_node *attr = (struct xml_node *)attribute;
	if (!removed)
		return E_POINTER;
	*removed = NULL;
	if (!attr || attr->kind != K_ATTR || attr->parent != self)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:2524\n"); return E_INVALIDARG; }
	*removed = attribute;
	xml_node_addref(attr);
	xml_attr_unlink(self, attr);
	return S_OK;
}

static HRESULT WINAPI element_get_elements(element_i *p, HSTRING tag, nodelist_i **out)
{
	struct xml_node *self = ((struct element_part *)p)->owner;
	const WCHAR *text;
	struct xml_vec vec = {0};
	SIZE_T index;
	HRESULT result = hstring_text(tag, &text);
	if (FAILED(result))
		return result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	vec.ok = TRUE;
	vec_descendants(self, &vec);
	for (index = 0; index < vec.count;) {
		struct xml_node *node = vec.items[index];
		if (node->kind != K_ELEMENT || (lstrcmpW(text, L"*") && (!node->name || lstrcmpW(node->name, text)))) {
			xml_node_release(node);
			vec.items[index] = vec.items[--vec.count];
		} else
			++index;
	}
	return list_from_vec(&vec, out);
}

static HRESULT WINAPI element_set_attribute_ns(element_i *p, IInspectable *ns, HSTRING name, HSTRING value)
{
	(void)ns;
	return element_set_attribute(p, name, value);
}

static HRESULT WINAPI element_get_attribute_ns(element_i *p, IInspectable *ns, HSTRING name, HSTRING *out)
{
	(void)ns;
	return element_get_attribute(p, name, out);
}

static HRESULT WINAPI element_remove_attribute_ns(element_i *p, IInspectable *ns, HSTRING name)
{
	(void)ns;
	return element_remove_attribute(p, name);
}

static HRESULT WINAPI element_set_attribute_node_ns(element_i *p, attribute_i *attribute, attribute_i **previous)
{
	return element_set_attribute_node(p, attribute, previous);
}

static HRESULT WINAPI element_get_attribute_node_ns(element_i *p, IInspectable *ns, HSTRING local, attribute_i **out)
{
	(void)ns;
	return element_get_attribute_node(p, local, out);
}

static const element_iVtbl element_table = {
	element_query, element_addref, element_release, element_iids, element_class, element_trust,
	element_tag, element_get_attribute, element_set_attribute, element_remove_attribute,
	element_get_attribute_node, element_set_attribute_node, element_remove_attribute_node,
	element_get_elements, element_set_attribute_ns, element_get_attribute_ns, element_remove_attribute_ns,
	element_set_attribute_node_ns, element_get_attribute_node_ns,
};


/* ---------------------------------------------------------------- IXmlAttribute */

static HRESULT WINAPI attribute_name(attribute_i *p, HSTRING *out)
{
	struct xml_node *self = ((struct attribute_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return create_string(self->name, out);
}

static HRESULT WINAPI attribute_specified(attribute_i *p, boolean *out)
{
	(void)p;
	if (!out)
		return E_POINTER;
	*out = TRUE; /* every attribute this DOM builds was written by the document or the guest */
	return S_OK;
}

static HRESULT WINAPI attribute_get_value(attribute_i *p, HSTRING *out)
{
	struct xml_node *self = ((struct attribute_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return create_string(self->value ? self->value : L"", out);
}

static HRESULT WINAPI attribute_put_value(attribute_i *p, HSTRING value)
{
	struct xml_node *self = ((struct attribute_part *)p)->owner;
	const WCHAR *text;
	SIZE_T length;
	WCHAR *copy;
	HRESULT result = hstring_text(value, &text);
	if (FAILED(result))
		return result;
	length = lstrlenW(text);
	copy = ALLOC((length + 1) * sizeof *copy);
	if (!copy)
		return E_OUTOFMEMORY;
	lstrcpyW(copy, text);
	if (self->value)
		FREE(self->value);
	self->value = copy;
	return S_OK;
}

static const attribute_iVtbl attribute_table = {
	attribute_query, attribute_addref, attribute_release, attribute_iids, attribute_class, attribute_trust,
	attribute_name, attribute_specified, attribute_get_value, attribute_put_value,
};

/* ---------------------------------------------------------------- IXmlCharacterData */

static HRESULT chardata_self(chardata_i *p, struct xml_node **self)
{
	*self = ((struct chardata_part *)p)->owner;
	if ((*self)->kind != K_TEXT && (*self)->kind != K_CDATA && (*self)->kind != K_COMMENT)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:2654\n"); return E_INVALIDARG; }
	return S_OK;
}

static HRESULT WINAPI chardata_get(chardata_i *p, HSTRING *out)
{
	struct xml_node *self;
	HRESULT result = chardata_self(p, &self);
	if (FAILED(result))
		return result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return create_string(self->value ? self->value : L"", out);
}

static HRESULT WINAPI chardata_put(chardata_i *p, HSTRING value)
{
	struct xml_node *self;
	const WCHAR *text;
	SIZE_T length;
	WCHAR *copy;
	HRESULT result = chardata_self(p, &self);
	if (FAILED(result))
		return result;
	if (FAILED((result = hstring_text(value, &text))))
		return result;
	length = lstrlenW(text);
	copy = ALLOC((length + 1) * sizeof *copy);
	if (!copy)
		return E_OUTOFMEMORY;
	lstrcpyW(copy, text);
	if (self->value)
		FREE(self->value);
	self->value = copy;
	return S_OK;
}

static HRESULT WINAPI chardata_length(chardata_i *p, UINT32 *out)
{
	struct xml_node *self;
	HRESULT result = chardata_self(p, &self);
	if (FAILED(result))
		return result;
	if (!out)
		return E_POINTER;
	*out = (UINT32)(self->value ? lstrlenW(self->value) : 0);
	return S_OK;
}

static HRESULT WINAPI chardata_substring(chardata_i *p, UINT32 offset, UINT32 count, HSTRING *out)
{
	struct xml_node *self;
	const WCHAR *text;
	UINT32 length, index;
	WCHAR *copy;
	HRESULT result = chardata_self(p, &self);
	if (FAILED(result))
		return result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = self->value ? self->value : L"";
	length = (UINT32)lstrlenW(text);
	if (offset > length)
		return E_BOUNDS;
	if (count > length - offset)
		count = length - offset;
	copy = ALLOC(((SIZE_T)count + 1) * sizeof *copy);
	if (!copy)
		return E_OUTOFMEMORY;
	for (index = 0; index < count; ++index)
		copy[index] = text[offset + index];
	copy[count] = 0;
	result = WindowsCreateString(copy, count, out);
	FREE(copy);
	return result;
}

static HRESULT WINAPI chardata_append(chardata_i *p, HSTRING value)
{
	struct xml_node *self;
	const WCHAR *text;
	HRESULT result = chardata_self(p, &self);
	if (FAILED(result))
		return result;
	if (FAILED((result = hstring_text(value, &text))))
		return result;
	{
		SIZE_T old = self->value ? lstrlenW(self->value) : 0;
		SIZE_T add = lstrlenW(text);
		WCHAR *grown = ALLOC((old + add + 1) * sizeof *grown);
		if (!grown)
			return E_OUTOFMEMORY;
		grown[0] = 0;
		if (self->value) lstrcpyW(grown, self->value);
		lstrcatW(grown, text);
		if (self->value) FREE(self->value);
		self->value = grown;
	}
	return S_OK;
}

static HRESULT WINAPI chardata_insert(chardata_i *p, UINT32 offset, HSTRING value)
{
	struct xml_node *self;
	const WCHAR *text;
	HRESULT result = chardata_self(p, &self);
	if (FAILED(result))
		return result;
	if (FAILED((result = hstring_text(value, &text))))
		return result;
	{
		UINT32 length = (UINT32)(self->value ? lstrlenW(self->value) : 0);
		SIZE_T add = lstrlenW(text);
		WCHAR *grown;
		if (offset > length)
			return E_BOUNDS;
		grown = ALLOC((length + add + 1) * sizeof *grown);
		if (!grown)
			return E_OUTOFMEMORY;
		memcpy(grown, self->value ? self->value : L"", offset * sizeof *grown);
		memcpy(grown + offset, text, add * sizeof *grown);
		memcpy(grown + offset + add, (self->value ? self->value : L"") + offset, (length - offset) * sizeof *grown);
		grown[length + add] = 0;
		if (self->value) FREE(self->value);
		self->value = grown;
	}
	return S_OK;
}

static HRESULT WINAPI chardata_delete(chardata_i *p, UINT32 offset, UINT32 count)
{
	struct xml_node *self;
	UINT32 length;
	HRESULT result = chardata_self(p, &self);
	if (FAILED(result))
		return result;
	length = (UINT32)(self->value ? lstrlenW(self->value) : 0);
	if (offset > length)
		return E_BOUNDS;
	if (count > length - offset)
		count = length - offset;
	memmove(self->value + offset, self->value + offset + count, (length - offset - count) * sizeof *self->value);
	self->value[length - count] = 0;
	return S_OK;
}

static HRESULT WINAPI chardata_replace(chardata_i *p, UINT32 offset, UINT32 count, HSTRING value)
{
	struct xml_node *self;
	const WCHAR *text;
	UINT32 length;
	HRESULT result = chardata_self(p, &self);
	if (FAILED(result))
		return result;
	if (FAILED((result = hstring_text(value, &text))))
		return result;
	length = (UINT32)(self->value ? lstrlenW(self->value) : 0);
	if (offset > length)
		return E_BOUNDS;
	if (count > length - offset)
		count = length - offset;
	{
		SIZE_T add = lstrlenW(text);
		WCHAR *grown = ALLOC((length - count + add + 1) * sizeof *grown);
		if (!grown)
			return E_OUTOFMEMORY;
		memcpy(grown, self->value, offset * sizeof *grown);
		memcpy(grown + offset, text, add * sizeof *grown);
		memcpy(grown + offset + add, self->value + offset + count, (length - offset - count) * sizeof *grown);
		grown[length - count + add] = 0;
		FREE(self->value);
		self->value = grown;
	}
	return S_OK;
}

static const chardata_iVtbl chardata_table = {
	chardata_query, chardata_addref, chardata_release, chardata_iids, chardata_class, chardata_trust,
	chardata_get, chardata_put, chardata_length, chardata_substring, chardata_append, chardata_insert,
	chardata_delete, chardata_replace,
};

static HRESULT WINAPI text_split(text_i *p, UINT32 offset, text_i **out)
{
	struct xml_node *self = ((struct textp_part *)p)->owner;
	UINT32 length;
	struct xml_node *tail;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (self->kind != K_TEXT && self->kind != K_CDATA)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:2847\n"); return E_INVALIDARG; }
	length = (UINT32)(self->value ? lstrlenW(self->value) : 0);
	if (offset > length)
		return E_BOUNDS;
	tail = xml_node_create(self->doc, self->kind, NULL, self->value ? self->value + offset : L"");
	if (!tail)
		return E_OUTOFMEMORY;
	if (self->value)
		self->value[offset] = 0;
	if (self->parent) {
		node_insert_before((node_i *)self->parent, (node_i *)tail, (node_i *)self, NULL);
	}
	*out = (text_i *)&tail->textp;
	return S_OK;
}

static const text_iVtbl text_table = {
	textp_query, textp_addref, textp_release, textp_iids, text_class, textp_trust, text_split,
};

static const comment_iVtbl comment_table = {
	comment_query, comment_addref, comment_release, comment_iids, comment_class, comment_trust,
};

static const cdata_iVtbl cdata_table = {
	cdata_query, cdata_addref, cdata_release, cdata_iids, cdata_class, cdata_trust,
};

static HRESULT WINAPI pi_target(pi_i *p, HSTRING *out)
{
	struct xml_node *self = ((struct pi_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return create_string(self->name, out);
}

static HRESULT WINAPI pi_get_data(pi_i *p, HSTRING *out)
{
	struct xml_node *self = ((struct pi_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return create_string(self->value ? self->value : L"", out);
}

static HRESULT WINAPI pi_put_data(pi_i *p, HSTRING value)
{
	struct xml_node *self = ((struct pi_part *)p)->owner;
	const WCHAR *text;
	SIZE_T length;
	WCHAR *copy;
	HRESULT result = hstring_text(value, &text);
	if (FAILED(result))
		return result;
	length = lstrlenW(text);
	copy = ALLOC((length + 1) * sizeof *copy);
	if (!copy)
		return E_OUTOFMEMORY;
	lstrcpyW(copy, text);
	if (self->value)
		FREE(self->value);
	self->value = copy;
	return S_OK;
}

static const pi_iVtbl pi_table = {
	pi_query, pi_addref, pi_release, pi_iids, pi_class, pi_trust, pi_target, pi_get_data, pi_put_data,
};

static const fragment_iVtbl fragment_table = {
	fragment_query, fragment_addref, fragment_release, fragment_iids, fragment_class, fragment_trust,
};

/* ---------------------------------------------------------------- IXmlDocumentIO */

/* The storage file's path and the package root live in storage.c and package.c. */
HRESULT raven_storage_file_path(__x_ABI_CWindows_CStorage_CIStorageFile *file, WCHAR *path, UINT32 capacity);
BOOL raven_package_root(WCHAR *path, UINT32 capacity);

/* The save action and the file writer live below, with the async operations. */
static struct xml_save_action *xml_save_action_new(void);
static void xml_save_action_finish(__x_ABI_CWindows_CFoundation_CIAsyncAction *action, HRESULT error, AsyncStatus status);
static HRESULT write_text_file(const WCHAR *path, const WCHAR *text);

static HRESULT WINAPI documentio_load_xml(documentio_i *p, HSTRING xml)
{
	struct xml_node *self = ((struct documentio_part *)p)->owner;
	const WCHAR *text;
	HRESULT result = hstring_text(xml, &text);
	if (FAILED(result))
		return result;
	return xml_doc_load(self, text);
}

static HRESULT WINAPI documentio_load_xml_settings(documentio_i *p, HSTRING xml, loadsettings_i *settings)
{
	(void)settings; /* the parser applies the documented defaults already */
	return documentio_load_xml(p, xml);
}

static HRESULT WINAPI documentio_save_to_file(documentio_i *p, __x_ABI_CWindows_CStorage_CIStorageFile *file,
	__x_ABI_CWindows_CFoundation_CIAsyncAction **operation)
{
	struct xml_node *self = ((struct documentio_part *)p)->owner;
	struct xml_save_action *action;
	HSTRING xml = NULL;
	const WCHAR *text;
	WCHAR path[MAX_PATH];
	HRESULT result;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	result = raven_storage_file_path(file, path, MAX_PATH);
	if (FAILED(result))
		return result;
	result = xml_get_xml(self, &xml);
	if (FAILED(result))
		return result;
	text = WindowsGetStringRawBuffer(xml, NULL);
	result = write_text_file(path, text);
	WindowsDeleteString(xml);
	if (FAILED(result))
		return result; /* nothing started, so nothing completes */
	action = xml_save_action_new();
	if (!action)
		return E_OUTOFMEMORY;
	xml_save_action_finish((__x_ABI_CWindows_CFoundation_CIAsyncAction *)action, S_OK, Completed);
	*operation = (__x_ABI_CWindows_CFoundation_CIAsyncAction *)action;
	return S_OK;
}

static const documentio_iVtbl documentio_table = {
	documentio_query, documentio_addref, documentio_release, documentio_iids, documentio_class,
	documentio_trust, documentio_load_xml, documentio_load_xml_settings, documentio_save_to_file,
};

/* ---------------------------------------------------------------- IXmlDocumentIO2 */

static HRESULT WINAPI documentio2_load_xml_from_buffer(documentio2_i *p,
	__x_ABI_CWindows_CStorage_CStreams_CIBuffer *buffer)
{
	(void)p;
	(void)buffer; /* the guests observed so far load text; extend with IBuffer reads when one does not */
	return E_NOTIMPL;
}

static HRESULT WINAPI documentio2_load_xml_from_buffer_settings(documentio2_i *p,
	__x_ABI_CWindows_CStorage_CStreams_CIBuffer *buffer, loadsettings_i *settings)
{
	(void)settings;
	return documentio2_load_xml_from_buffer(p, buffer);
}

static const documentio2_iVtbl documentio2_table = {
	documentio2_query, documentio2_addref, documentio2_release, documentio2_iids, documentio2_class,
	documentio2_trust, documentio2_load_xml_from_buffer, documentio2_load_xml_from_buffer_settings,
};

/* ---------------------------------------------------------------- IXmlDocument */

static HRESULT WINAPI document_get_doctype(document_i *p, doctype_i **out)
{
	(void)p;
	if (!out)
		return E_POINTER;
	*out = NULL; /* DTDs are skipped, no doctype node exists */
	return S_OK;
}

/* The implementation object below; one static instance serves every document. */
static HRESULT WINAPI domimpl_query(domimpl_i *iface, REFIID iid, void **out);
static ULONG WINAPI domimpl_addref(domimpl_i *iface);
static ULONG WINAPI domimpl_release(domimpl_i *iface);
static const __x_ABI_CWindows_CData_CXml_CDom_CIXmlDomImplementationVtbl domimpl_table;
static __x_ABI_CWindows_CData_CXml_CDom_CIXmlDomImplementation domimpl_instance = {&domimpl_table};

static HRESULT WINAPI document_get_implementation(document_i *p, __x_ABI_CWindows_CData_CXml_CDom_CIXmlDomImplementation **out)
{
	(void)p;
	if (!out)
		return E_POINTER;
	*out = &domimpl_instance; /* the static instance never dies */
	return S_OK;
}

static HRESULT WINAPI document_get_element(document_i *p, element_i **out)
{
	struct xml_node *self = ((struct document_part *)p)->owner;
	struct xml_node *element = xml_element_child(self);
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (element) {
		*out = (element_i *)&element->element;
		xml_node_addref(element);
	}
	return S_OK;
}

static HRESULT WINAPI document_create_element(document_i *p, HSTRING tag, element_i **out)
{
	struct xml_node *self = ((struct document_part *)p)->owner;
	const WCHAR *text;
	struct xml_node *element;
	HRESULT result = hstring_text(tag, &text);
	if (FAILED(result))
		return result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!*text)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:3059\n"); return E_INVALIDARG; }
	element = xml_node_create(self, K_ELEMENT, text, NULL);
	if (!element)
		return E_OUTOFMEMORY;
	*out = (element_i *)&element->element;
	return S_OK;
}

static HRESULT WINAPI document_create_fragment(document_i *p, fragment_i **out)
{
	struct xml_node *self = ((struct document_part *)p)->owner;
	struct xml_node *node;
	if (!out)
		return E_POINTER;
	*out = NULL;
	node = xml_node_create(self, K_FRAGMENT, NULL, NULL);
	if (!node)
		return E_OUTOFMEMORY;
	*out = (fragment_i *)&node->fragment;
	return S_OK;
}

static HRESULT WINAPI document_create_text(document_i *p, HSTRING data, text_i **out)
{
	struct xml_node *self = ((struct document_part *)p)->owner;
	const WCHAR *text;
	struct xml_node *node;
	HRESULT result = hstring_text(data, &text);
	if (FAILED(result))
		return result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	node = xml_node_create(self, K_TEXT, NULL, text);
	if (!node)
		return E_OUTOFMEMORY;
	*out = (text_i *)&node->textp;
	return S_OK;
}

static HRESULT WINAPI document_create_comment(document_i *p, HSTRING data, comment_i **out)
{
	struct xml_node *self = ((struct document_part *)p)->owner;
	const WCHAR *text;
	struct xml_node *node;
	HRESULT result = hstring_text(data, &text);
	if (FAILED(result))
		return result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	node = xml_node_create(self, K_COMMENT, NULL, text);
	if (!node)
		return E_OUTOFMEMORY;
	*out = (comment_i *)&node->comment;
	return S_OK;
}

static HRESULT WINAPI document_create_pi(document_i *p, HSTRING target, HSTRING data, pi_i **out)
{
	struct xml_node *self = ((struct document_part *)p)->owner;
	const WCHAR *target_text, *data_text;
	struct xml_node *node;
	HRESULT result;
	if (FAILED((result = hstring_text(target, &target_text))) || FAILED((result = hstring_text(data, &data_text))))
		return result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!*target_text)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:3129\n"); return E_INVALIDARG; }
	node = xml_node_create(self, K_PI, target_text, data_text);
	if (!node)
		return E_OUTOFMEMORY;
	*out = (pi_i *)&node->pi;
	return S_OK;
}

static HRESULT WINAPI document_create_attribute(document_i *p, HSTRING name, attribute_i **out)
{
	struct xml_node *self = ((struct document_part *)p)->owner;
	const WCHAR *text;
	struct xml_node *node;
	HRESULT result = hstring_text(name, &text);
	if (FAILED(result))
		return result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!*text)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xml_dom.c:3149\n"); return E_INVALIDARG; }
	node = xml_node_create(self, K_ATTR, text, NULL);
	if (!node)
		return E_OUTOFMEMORY;
	*out = (attribute_i *)&node->attribute;
	return S_OK;
}

static HRESULT WINAPI document_create_entity_reference(document_i *p, HSTRING name, entityref_i **out)
{
	(void)p;
	(void)name;
	if (!out)
		return E_POINTER;
	*out = NULL; /* no entity tables exist to reference */
	return E_NOTIMPL;
}

static HRESULT WINAPI document_get_elements(document_i *p, HSTRING tag, nodelist_i **out)
{
	struct xml_node *self = ((struct document_part *)p)->owner;
	const WCHAR *text;
	struct xml_vec vec = {0};
	SIZE_T index;
	HRESULT result = hstring_text(tag, &text);
	if (FAILED(result))
		return result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	vec.ok = TRUE;
	vec_descendants(self, &vec);
	for (index = 0; index < vec.count;) {
		struct xml_node *node = vec.items[index];
		if (node->kind != K_ELEMENT || (lstrcmpW(text, L"*") && (!node->name || lstrcmpW(node->name, text)))) {
			xml_node_release(node);
			vec.items[index] = vec.items[--vec.count];
		} else
			++index;
	}
	return list_from_vec(&vec, out);
}

static HRESULT WINAPI document_create_cdata(document_i *p, HSTRING data, cdata_i **out)
{
	struct xml_node *self = ((struct document_part *)p)->owner;
	const WCHAR *text;
	struct xml_node *node;
	HRESULT result = hstring_text(data, &text);
	if (FAILED(result))
		return result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	node = xml_node_create(self, K_CDATA, NULL, text);
	if (!node)
		return E_OUTOFMEMORY;
	*out = (cdata_i *)&node->cdata;
	return S_OK;
}

static HRESULT WINAPI document_get_uri(document_i *p, HSTRING *out)
{
	struct xml_node *self = ((struct document_part *)p)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return create_string(self->uri, out);
}

static HRESULT WINAPI document_create_attribute_ns(document_i *p, IInspectable *ns, HSTRING name, attribute_i **out)
{
	(void)ns;
	return document_create_attribute(p, name, out);
}

static HRESULT WINAPI document_create_element_ns(document_i *p, IInspectable *ns, HSTRING name, element_i **out)
{
	(void)ns;
	return document_create_element(p, name, out);
}

static BOOL element_id_matches(struct xml_node *element, const WCHAR *id)
{
	struct xml_node *attr = map_find(element, L"id");
	if (attr && attr->value && !lstrcmpW(attr->value, id))
		return TRUE;
	attr = map_find(element, L"xml:id");
	return attr && attr->value && !lstrcmpW(attr->value, id);
}

static HRESULT id_search(struct xml_node *node, const WCHAR *id, struct xml_node **found)
{
	struct xml_node *child;
	for (child = node->first; child; child = child->next) {
		if (child->kind == K_ELEMENT && element_id_matches(child, id)) {
			*found = child;
			return S_OK;
		}
		if (id_search(child, id, found) == S_OK)
			return S_OK;
	}
	return S_FALSE;
}

static HRESULT WINAPI document_get_by_id(document_i *p, HSTRING id, element_i **out)
{
	struct xml_node *self = ((struct document_part *)p)->owner;
	const WCHAR *text;
	struct xml_node *found = NULL;
	HRESULT result = hstring_text(id, &text);
	if (FAILED(result))
		return result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (*text && id_search(self, text, &found) == S_OK) {
		*out = (element_i *)&found->element;
		xml_node_addref(found);
	}
	return S_OK;
}

static HRESULT WINAPI document_import(document_i *p, node_i *node, boolean deep, node_i **out)
{
	struct xml_node *self = ((struct document_part *)p)->owner;
	struct xml_node *copy;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!node)
		return E_POINTER;
	copy = xml_node_clone(self, (struct xml_node *)node, deep ? 1 : 0);
	if (!copy)
		return E_OUTOFMEMORY;
	*out = (node_i *)copy;
	return S_OK;
}

static const document_iVtbl document_table = {
	document_query, document_addref, document_release, document_iids, document_class, document_trust,
	document_get_doctype, document_get_implementation, document_get_element, document_create_element,
	document_create_fragment, document_create_text, document_create_comment, document_create_pi,
	document_create_attribute, document_create_entity_reference, document_get_elements,
	document_create_cdata, document_get_uri, document_create_attribute_ns, document_create_element_ns,
	document_get_by_id, document_import,
};

/* ---------------------------------------------------------------- loading and saving */

/* The whole file as UTF-16 text: BOM-marked UTF-8 and UTF-16, else UTF-8. */
static HRESULT read_text_file(const WCHAR *path, WCHAR **out)
{
	HANDLE handle;
	LARGE_INTEGER size;
	BYTE *data = NULL;
	DWORD read = 0;
	if (!out)
		return E_POINTER;
	*out = NULL;
	handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return HRESULT_FROM_WIN32(GetLastError());
	if (!GetFileSizeEx(handle, &size) || size.QuadPart > 0x4000000) {
		CloseHandle(handle);
		return E_FAIL;
	}
	data = ALLOC(size.QuadPart ? (SIZE_T)size.QuadPart : 1);
	if (!data) {
		CloseHandle(handle);
		return E_OUTOFMEMORY;
	}
	if (!ReadFile(handle, data, (DWORD)size.QuadPart, &read, NULL)) {
		CloseHandle(handle);
		FREE(data);
		return HRESULT_FROM_WIN32(GetLastError());
	}
	CloseHandle(handle);
	{
		const BYTE *text = data;
		SIZE_T bytes = read;
		BOOL utf16 = FALSE, bom = FALSE;
		if (bytes >= 2 && data[0] == 0xFF && data[1] == 0xFE) {
			utf16 = TRUE;
			bom = TRUE;
		} else if (bytes >= 2 && data[0] == 0x3C && data[1] == 0x00) {
			utf16 = TRUE;
		} else if (bytes >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
			text += 3;
			bytes -= 3;
			bom = TRUE;
		}
		if (utf16) {
			SIZE_T length = (bytes - (bom ? 2 : 0)) / sizeof(WCHAR);
			WCHAR *copy = ALLOC((length + 1) * sizeof *copy);
			if (!copy) {
				FREE(data);
				return E_OUTOFMEMORY;
			}
			memcpy(copy, text + (bom ? 2 : 0), length * sizeof *copy);
			copy[length] = 0;
			FREE(data);
			*out = copy;
			return S_OK;
		}
		{
			int wide = MultiByteToWideChar(CP_UTF8, 0, (const char *)text, (int)bytes, NULL, 0);
			WCHAR *copy;
			if (!wide) {
				FREE(data);
				return HRESULT_FROM_WIN32(GetLastError());
			}
			copy = ALLOC(((SIZE_T)wide + 1) * sizeof *copy);
			if (!copy) {
				FREE(data);
				return E_OUTOFMEMORY;
			}
			MultiByteToWideChar(CP_UTF8, 0, (const char *)text, (int)bytes, copy, wide);
			copy[wide] = 0;
			FREE(data);
			*out = copy;
			return S_OK;
		}
	}
}

static unsigned long hex_value(WCHAR c)
{
	if (c >= L'0' && c <= L'9')
		return (unsigned long)(c - L'0');
	if (c >= L'a' && c <= L'f')
		return (unsigned long)(c - L'a' + 10);
	if (c >= L'A' && c <= L'F')
		return (unsigned long)(c - L'A' + 10);
	return 16; /* not a digit; the caller sees an out-of-range sum */
}

static BOOL text_starts(const WCHAR *text, const WCHAR *prefix)
{
	SIZE_T index = 0;
	for (; prefix[index]; ++index)
		if (text[index] != prefix[index])
			return FALSE;
	return TRUE;
}

static void percent_decode(WCHAR *text)
{
	WCHAR *read = text, *write = text;
	while (*read) {
		if (read[0] == L'%' && read[1] && read[2] && hex_value(read[1]) < 16 && hex_value(read[2]) < 16) {
			*write++ = (WCHAR)(hex_value(read[1]) * 16 + hex_value(read[2]));
			read += 3;
		} else {
			*write++ = *read++;
		}
	}
	*write = 0;
}

/* file:// and ms-appx:/// URIs reach files; other schemes are not served. */
static HRESULT uri_to_path(const WCHAR *uri, WCHAR *path, UINT32 capacity)
{
	const WCHAR *rest = NULL;
	if (text_starts(uri, L"file://")) {
		rest = uri + 7;
		while (*rest == L'/')
			++rest;
	} else if (text_starts(uri, L"ms-appx:///")) {
		WCHAR root[MAX_PATH];
		if (!raven_package_root(root, MAX_PATH))
			return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
		rest = uri + 12;
		while (*rest && *rest != L'/')
			++rest; /* the package name; the current package's root serves every host */
		if (lstrlenW(root) + 1 + lstrlenW(rest) >= capacity)
			return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
		lstrcpyW(path, root);
		lstrcatW(path, rest ? rest : L"");
		percent_decode(path + lstrlenW(root));
		return S_OK;
	} else
		return E_NOTIMPL;
	if (lstrlenW(rest) + 1 > capacity)
		return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	lstrcpyW(path, rest);
	percent_decode(path);
	return S_OK;
}

static struct xml_node *xml_doc_new(void)
{
	return xml_node_create(NULL, K_DOC, NULL, NULL);
}

static HRESULT load_document_text(struct xml_node *doc, const WCHAR *text, const WCHAR *uri)
{
	HRESULT result = xml_doc_load(doc, text);
	if (SUCCEEDED(result) && uri) {
		doc->uri = ALLOC((lstrlenW(uri) + 1) * sizeof *doc->uri);
		if (doc->uri)
			lstrcpyW(doc->uri, uri);
	}
	return result;
}


/* ---------------------------------------------------------------- async operations */

/* The operation and action shapes follow the storage component's: the terminal state is
 * always reached before the object escapes its creating call, so nothing is ever in flight
 * and a completed handler assigned afterwards is invoked immediately. */
struct xml_op_core {
	LONG refs;
	CRITICAL_SECTION lock;
	AsyncStatus status;
	HRESULT error;
	UINT32 id;
	BOOL closed, assigned;
	void *completed;
};

static LONG xml_next_async_id;

#define XML_ASYNC_LIFETIME(name, iface_type, own_iid, handler_type) \
	static HRESULT WINAPI name##_query(iface_type *iface, REFIID iid, void **out) \
	{ \
		struct name *self = (struct name *)iface; \
		if (!out) \
			return E_POINTER; \
		*out = NULL; \
		if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || \
			equal_iid(iid, own_iid) || equal_iid(iid, &IID_IAgileObject)) \
			*out = iface; \
		else if (equal_iid(iid, &IID_IAsyncInfo)) \
			*out = &self->info; \
		else \
			return E_NOINTERFACE; \
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
				((handler_type *)self->core.completed)->lpVtbl->Release((handler_type *)self->core.completed); \
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
	static HRESULT WINAPI name##_trust(iface_type *iface, TrustLevel *level) \
	{ \
		(void)iface; \
		return inspectable_trust(level); \
	} \
	static HRESULT WINAPI name##_info_query(IAsyncInfo *info, REFIID iid, void **out) \
	{ \
		return name##_query((iface_type *)CONTAINING_RECORD(info, struct name, info), iid, out); \
	} \
	static ULONG WINAPI name##_info_addref(IAsyncInfo *info) \
	{ \
		return name##_addref((iface_type *)CONTAINING_RECORD(info, struct name, info)); \
	} \
	static ULONG WINAPI name##_info_release(IAsyncInfo *info) \
	{ \
		return name##_release((iface_type *)CONTAINING_RECORD(info, struct name, info)); \
	} \
	static HRESULT WINAPI name##_info_iids(IAsyncInfo *info, ULONG *count, IID **iids) \
	{ \
		(void)info; \
		return inspectable_iids(count, iids); \
	} \
	static HRESULT WINAPI name##_info_trust(IAsyncInfo *info, TrustLevel *level) \
	{ \
		(void)info; \
		return inspectable_trust(level); \
	} \
	static HRESULT WINAPI name##_info_id(IAsyncInfo *info, UINT32 *out) \
	{ \
		struct name *self = CONTAINING_RECORD(info, struct name, info); \
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
		struct name *self = CONTAINING_RECORD(info, struct name, info); \
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
		struct name *self = CONTAINING_RECORD(info, struct name, info); \
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
		return S_OK; /* terminal before it ever escaped; nothing is in flight */ \
	} \
	static HRESULT WINAPI name##_info_close(IAsyncInfo *info) \
	{ \
		struct name *self = CONTAINING_RECORD(info, struct name, info); \
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
			((handler_type *)handler)->lpVtbl->Release((handler_type *)handler); \
		return result; \
	} \
	static const IAsyncInfoVtbl name##_info_table; /* filled after the class slot below */ \
	static HRESULT WINAPI name##_info_class(IAsyncInfo *info, HSTRING *out); \
	static const IAsyncInfoVtbl name##_info_table = { \
		name##_info_query, name##_info_addref, name##_info_release, name##_info_iids, \
		name##_info_class, name##_info_trust, name##_info_id, name##_info_status, name##_info_error, \
		name##_info_cancel, name##_info_close, \
	};

/* GetRuntimeClassName differs per instantiation; the body comes from this macro. */
#define XML_ASYNC_CLASS(name, iface_type, type_name) \
	static HRESULT WINAPI name##_info_class(IAsyncInfo *info, HSTRING *out) \
	{ \
		(void)info; \
		return create_string(type_name, out); \
	} \
	static HRESULT WINAPI name##_class(iface_type *iface, HSTRING *out) \
	{ \
		(void)iface; \
		return create_string(type_name, out); \
	}

#define XML_OP_COMPLETED(name, iface_type, handler_type) \
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
				((handler_type *)handler)->lpVtbl->AddRef((handler_type *)handler); \
				if (status != Started) ((handler_type *)handler)->lpVtbl->AddRef((handler_type *)handler); \
			} \
		} \
		LeaveCriticalSection(&self->core.lock); \
		if (SUCCEEDED(result) && handler && status != Started) { \
			((handler_type *)handler)->lpVtbl->Invoke((handler_type *)handler, iface, status); \
			((handler_type *)handler)->lpVtbl->Release((handler_type *)handler); \
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
		else if ((*handler = (handler_type *)self->core.completed)) ((handler_type *)*handler)->lpVtbl->AddRef((handler_type *)*handler); \
		LeaveCriticalSection(&self->core.lock); \
		return result; \
	} \
	static void name##_finish(iface_type *iface, HRESULT error, AsyncStatus status) \
	{ \
		struct name *self = (struct name *)iface; \
		handler_type *handler = NULL; \
		EnterCriticalSection(&self->core.lock); \
		if (self->core.status == Started) { \
			self->core.status = status; \
			self->core.error = error; \
			handler = (handler_type *)self->core.completed; \
			if (handler) ((handler_type *)handler)->lpVtbl->AddRef((handler_type *)handler); \
		} \
		LeaveCriticalSection(&self->core.lock); \
		if (handler) { \
			((handler_type *)handler)->lpVtbl->Invoke((handler_type *)handler, iface, status); \
			((handler_type *)handler)->lpVtbl->Release((handler_type *)handler); \
		} \
	}

#define XML_ASYNC_NEW(name, iface_type, table) \
	static struct name *name##_new(void) \
	{ \
		struct name *self = ALLOC(sizeof *self); \
		if (!self) \
			return NULL; \
		self->iface.lpVtbl = (void *)&table; \
		self->info.lpVtbl = (void *)&name##_info_table; \
		self->core.refs = 1; \
		self->core.status = Started; \
		self->core.error = S_OK; \
		self->core.id = (UINT32)InterlockedIncrement(&xml_next_async_id); \
		InitializeCriticalSection(&self->core.lock); \
		InterlockedIncrement(&raven_xaml_objects); \
		return self; \
	}

static const WCHAR op_type_name[] = L"Windows.Foundation.IAsyncOperation`1<Windows.Data.Xml.Dom.XmlDocument>";

struct xml_doc_op {
	xml_doc_op_i iface;
	IAsyncInfo info;
	struct xml_op_core core;
	document_i *result;
};

static void xml_doc_op_free(struct xml_doc_op *self)
{
	if (self->result)
		((node_i *)self->result)->lpVtbl->Release((node_i *)self->result);
}

XML_ASYNC_LIFETIME(xml_doc_op, xml_doc_op_i, &IID___FIAsyncOperation_1_Windows__CData__CXml__CDom__CXmlDocument, xml_doc_done_i)
XML_ASYNC_CLASS(xml_doc_op, xml_doc_op_i, op_type_name)
XML_OP_COMPLETED(xml_doc_op, xml_doc_op_i, xml_doc_done_i)

static HRESULT WINAPI xml_doc_op_results(xml_doc_op_i *iface, document_i **results)
{
	struct xml_doc_op *self = (struct xml_doc_op *)iface;
	HRESULT result;
	if (!results)
		return E_POINTER;
	*results = NULL;
	EnterCriticalSection(&self->core.lock);
	result = self->core.closed ? RO_E_CLOSED : self->core.status == Started ? E_ILLEGAL_METHOD_CALL : self->core.error;
	LeaveCriticalSection(&self->core.lock);
	if (SUCCEEDED(result) && self->result) {
		*results = self->result;
		xml_node_addref((struct xml_node *)((struct document_part *)self->result)->owner);
	}
	return result;
}

static const __FIAsyncOperation_1_Windows__CData__CXml__CDom__CXmlDocumentVtbl xml_doc_op_table = {
	xml_doc_op_query, xml_doc_op_addref, xml_doc_op_release, xml_doc_op_iids, xml_doc_op_class,
	xml_doc_op_trust, xml_doc_op_put_completed, xml_doc_op_get_completed, xml_doc_op_results,
};

XML_ASYNC_NEW(xml_doc_op, xml_doc_op_i, xml_doc_op_table)

static const WCHAR action_type_name[] = L"Windows.Foundation.IAsyncAction";

struct xml_save_action {
	__x_ABI_CWindows_CFoundation_CIAsyncAction iface;
	IAsyncInfo info;
	struct xml_op_core core;
};

static void xml_save_action_free(struct xml_save_action *self)
{
	(void)self;
}

XML_ASYNC_LIFETIME(xml_save_action, __x_ABI_CWindows_CFoundation_CIAsyncAction,
	&IID___x_ABI_CWindows_CFoundation_CIAsyncAction, __x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler)
XML_ASYNC_CLASS(xml_save_action, __x_ABI_CWindows_CFoundation_CIAsyncAction, action_type_name)
XML_OP_COMPLETED(xml_save_action, __x_ABI_CWindows_CFoundation_CIAsyncAction,
	__x_ABI_CWindows_CFoundation_CIAsyncActionCompletedHandler)

static HRESULT WINAPI xml_save_action_results(__x_ABI_CWindows_CFoundation_CIAsyncAction *iface)
{
	(void)iface; /* the action is terminal; nothing to await */
	return S_OK;
}

static const __x_ABI_CWindows_CFoundation_CIAsyncActionVtbl xml_save_action_table = {
	xml_save_action_query, xml_save_action_addref, xml_save_action_release, xml_save_action_iids,
	xml_save_action_class, xml_save_action_trust, xml_save_action_put_completed, xml_save_action_get_completed,
	xml_save_action_results,
};

XML_ASYNC_NEW(xml_save_action, __x_ABI_CWindows_CFoundation_CIAsyncAction, xml_save_action_table)

/* ---------------------------------------------------------------- file loads and the statics */

static HRESULT write_text_file(const WCHAR *path, const WCHAR *text)
{
	HANDLE handle;
	int bytes = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
	char *data;
	DWORD written = 0;
	BOOL ok;
	if (!bytes)
		return HRESULT_FROM_WIN32(GetLastError());
	bytes -= 1; /* the terminator WideCharToMultiByte counted is not written */
	data = ALLOC(bytes ? (SIZE_T)bytes : 1);
	if (!data)
		return E_OUTOFMEMORY;
	WideCharToMultiByte(CP_UTF8, 0, text, -1, data, bytes, NULL, NULL);
	handle = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
		FREE(data);
		return HRESULT_FROM_WIN32(GetLastError());
	}
	ok = WriteFile(handle, data, (DWORD)bytes, &written, NULL);
	CloseHandle(handle);
	FREE(data);
	if (!ok)
		return HRESULT_FROM_WIN32(GetLastError());
	return written == (DWORD)bytes ? S_OK : E_FAIL;
}

/* LoadFromUriAsync over a Uri the guest built: the absolute text decides the scheme. */
static HRESULT load_uri(const WCHAR *uri, struct xml_node **out)
{
	WCHAR path[MAX_PATH];
	WCHAR *text = NULL;
	struct xml_node *doc;
	HRESULT result = uri_to_path(uri, path, MAX_PATH);
	if (FAILED(result))
		return result;
	result = read_text_file(path, &text);
	if (FAILED(result))
		return result;
	doc = xml_doc_new();
	if (!doc) {
		FREE(text);
		return E_OUTOFMEMORY;
	}
	result = load_document_text(doc, text, uri);
	FREE(text);
	if (FAILED(result)) {
		xml_node_release(doc);
		return result;
	}
	*out = doc;
	return S_OK;
}

static HRESULT load_file_object(__x_ABI_CWindows_CStorage_CIStorageFile *file, struct xml_node **out)
{
	WCHAR path[MAX_PATH];
	WCHAR *text = NULL;
	struct xml_node *doc;
	HRESULT result;
	if (!out)
		return E_POINTER;
	*out = NULL;
	result = raven_storage_file_path(file, path, MAX_PATH);
	if (FAILED(result))
		return result;
	result = read_text_file(path, &text);
	if (FAILED(result))
		return result;
	doc = xml_doc_new();
	if (!doc) {
		FREE(text);
		return E_OUTOFMEMORY;
	}
	result = load_document_text(doc, text, path);
	FREE(text);
	if (FAILED(result)) {
		xml_node_release(doc);
		return result;
	}
	*out = doc;
	return S_OK;
}

static HRESULT WINAPI docstatics_load_from_uri(docstatics_i *self, __x_ABI_CWindows_CFoundation_CIUriRuntimeClass *uri,
	xml_doc_op_i **operation);
static HRESULT WINAPI docstatics_load_from_uri_settings(docstatics_i *self,
	__x_ABI_CWindows_CFoundation_CIUriRuntimeClass *uri, loadsettings_i *settings, xml_doc_op_i **operation);
static HRESULT WINAPI docstatics_load_from_file(docstatics_i *self, __x_ABI_CWindows_CStorage_CIStorageFile *file,
	xml_doc_op_i **operation);
static HRESULT WINAPI docstatics_load_from_file_settings(docstatics_i *self,
	__x_ABI_CWindows_CStorage_CIStorageFile *file, loadsettings_i *settings, xml_doc_op_i **operation);

/* The statics share one implementation; the settings variants only ignore their settings. */
static HRESULT WINAPI docstatics_load_from_uri(docstatics_i *self, __x_ABI_CWindows_CFoundation_CIUriRuntimeClass *uri,
	xml_doc_op_i **operation)
{
	struct xml_doc_op *op;
	HSTRING text = NULL;
	const WCHAR *raw;
	struct xml_node *doc = NULL;
	HRESULT result;
	(void)self;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	if (!uri)
		return E_POINTER;
	result = uri->lpVtbl->get_AbsoluteUri(uri, &text);
	if (FAILED(result))
		return result;
	raw = WindowsGetStringRawBuffer(text, NULL);
	result = load_uri(raw, &doc);
	WindowsDeleteString(text);
	if (FAILED(result))
		return result;
	op = xml_doc_op_new();
	if (!op) {
		xml_node_release(doc);
		return E_OUTOFMEMORY;
	}
	op->result = (document_i *)&doc->document; /* the operation holds the document's reference */
	xml_doc_op_finish((xml_doc_op_i *)op, S_OK, Completed);
	*operation = (xml_doc_op_i *)op;
	return S_OK;
}

static HRESULT WINAPI docstatics_load_from_uri_settings(docstatics_i *self,
	__x_ABI_CWindows_CFoundation_CIUriRuntimeClass *uri, loadsettings_i *settings, xml_doc_op_i **operation)
{
	(void)settings;
	return docstatics_load_from_uri(self, uri, operation);
}

static HRESULT WINAPI docstatics_load_from_file(docstatics_i *self, __x_ABI_CWindows_CStorage_CIStorageFile *file,
	xml_doc_op_i **operation)
{
	struct xml_doc_op *op;
	struct xml_node *doc = NULL;
	HRESULT result;
	(void)self;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	result = load_file_object(file, &doc);
	if (FAILED(result))
		return result;
	op = xml_doc_op_new();
	if (!op) {
		xml_node_release(doc);
		return E_OUTOFMEMORY;
	}
	op->result = (document_i *)&doc->document;
	xml_doc_op_finish((xml_doc_op_i *)op, S_OK, Completed);
	*operation = (xml_doc_op_i *)op;
	return S_OK;
}

static HRESULT WINAPI docstatics_load_from_file_settings(docstatics_i *self,
	__x_ABI_CWindows_CStorage_CIStorageFile *file, loadsettings_i *settings, xml_doc_op_i **operation)
{
	(void)settings;
	return docstatics_load_from_file(self, file, operation);
}

static const WCHAR docstatics_name[] = L"Windows.Data.Xml.Dom.IXmlDocumentStatics";

static HRESULT WINAPI docstatics_query(docstatics_i *self, REFIID iid, void **out);
static ULONG WINAPI docstatics_addref(docstatics_i *self);
static ULONG WINAPI docstatics_release(docstatics_i *self);
static docstatics_i docstatics_instance; /* defined after its table below */

static HRESULT WINAPI docstatics_query(docstatics_i *self, REFIID iid, void **out)
{
	(void)self;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentStatics) &&
		!equal_iid(iid, &IID_IAgileObject))
		return E_NOINTERFACE;
	*out = (void *)&docstatics_instance;
	return S_OK;
}

static ULONG WINAPI docstatics_addref(docstatics_i *self) { (void)self; return 2; }
static ULONG WINAPI docstatics_release(docstatics_i *self) { (void)self; return 1; }

static HRESULT WINAPI docstatics_iids(docstatics_i *self, ULONG *count, IID **iids)
{
	(void)self;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI docstatics_class(docstatics_i *self, HSTRING *out)
{
	(void)self;
	return create_string(docstatics_name, out);
}

static HRESULT WINAPI docstatics_trust(docstatics_i *self, TrustLevel *level)
{
	(void)self;
	return inspectable_trust(level);
}

static const docstatics_iVtbl docstatics_table = {
	docstatics_query, docstatics_addref, docstatics_release, docstatics_iids, docstatics_class,
	docstatics_trust, docstatics_load_from_uri, docstatics_load_from_uri_settings,
	docstatics_load_from_file, docstatics_load_from_file_settings,
};

static docstatics_i docstatics_instance = {&docstatics_table};

/* ---------------------------------------------------------------- IXmlDomImplementation */

static HRESULT WINAPI domimpl_query(domimpl_i *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlDomImplementation) &&
		!equal_iid(iid, &IID_IAgileObject))
		return E_NOINTERFACE;
	*out = (void *)&domimpl_table;
	return S_OK;
}

static ULONG WINAPI domimpl_addref(domimpl_i *iface) { (void)iface; return 2; }
static ULONG WINAPI domimpl_release(domimpl_i *iface) { (void)iface; return 1; }

static HRESULT WINAPI domimpl_iids(domimpl_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI domimpl_class(domimpl_i *iface, HSTRING *out)
{
	(void)iface;
	return create_string(L"Windows.Data.Xml.Dom.XmlDomImplementation", out);
}

static HRESULT WINAPI domimpl_trust(domimpl_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI domimpl_has_feature(domimpl_i *iface, HSTRING feature, IInspectable *version, boolean *out)
{
	const WCHAR *feature_text = NULL, *version_text = NULL;
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = FALSE;
	feature_text = WindowsGetStringRawBuffer(feature, NULL);
	{
		__FIReference_1_HSTRING *boxed = NULL;
		if (version && SUCCEEDED(((IInspectable *)version)->lpVtbl->QueryInterface(version,
			&IID___FIReference_1_HSTRING, (void **)&boxed))) {
			HSTRING text = NULL;
			if (SUCCEEDED(boxed->lpVtbl->get_Value(boxed, &text))) {
				version_text = WindowsGetStringRawBuffer(text, NULL);
				WindowsDeleteString(text);
			}
			boxed->lpVtbl->Release(boxed);
		}
	}
	if (!feature_text)
		return S_OK;
	if (lstrcmpW(feature_text, L"XML"))
		return S_OK;
	*out = !version_text || !*version_text || !lstrcmpW(version_text, L"1.0") || !lstrcmpW(version_text, L"2.0");
	return S_OK;
}

static const __x_ABI_CWindows_CData_CXml_CDom_CIXmlDomImplementationVtbl domimpl_table = {
	domimpl_query, domimpl_addref, domimpl_release, domimpl_iids, domimpl_class, domimpl_trust,
	domimpl_has_feature,
};

/* ---------------------------------------------------------------- IXmlLoadSettings */

struct xml_settings {
	const loadsettings_iVtbl *lpVtbl;
	LONG refs;
	UINT32 max_depth;
	BOOL prohibit_dtd, resolve_externals, validate, keep_white_space;
};

static const loadsettings_iVtbl settings_table;

static HRESULT WINAPI settings_query(loadsettings_i *iface, REFIID iid, void **out)
{
	struct xml_settings *self = (void *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlLoadSettings) && !equal_iid(iid, &IID_IAgileObject))
		return E_NOINTERFACE;
	*out = iface;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI settings_addref(loadsettings_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct xml_settings *)iface)->refs);
}

static ULONG WINAPI settings_release(loadsettings_i *iface)
{
	struct xml_settings *self = (void *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI settings_iids(loadsettings_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI settings_class(loadsettings_i *iface, HSTRING *out)
{
	(void)iface;
	return create_string(settings_class_name, out);
}

static HRESULT WINAPI settings_trust(loadsettings_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI settings_get_depth(loadsettings_i *iface, UINT32 *out)
{
	if (!out)
		return E_POINTER;
	*out = ((struct xml_settings *)iface)->max_depth;
	return S_OK;
}

static HRESULT WINAPI settings_put_depth(loadsettings_i *iface, UINT32 value)
{
	((struct xml_settings *)iface)->max_depth = value;
	return S_OK;
}

static HRESULT WINAPI settings_get_prohibit(loadsettings_i *iface, boolean *out)
{
	if (!out)
		return E_POINTER;
	*out = ((struct xml_settings *)iface)->prohibit_dtd;
	return S_OK;
}

static HRESULT WINAPI settings_put_prohibit(loadsettings_i *iface, boolean value)
{
	((struct xml_settings *)iface)->prohibit_dtd = value;
	return S_OK;
}

static HRESULT WINAPI settings_get_resolve(loadsettings_i *iface, boolean *out)
{
	if (!out)
		return E_POINTER;
	*out = ((struct xml_settings *)iface)->resolve_externals;
	return S_OK;
}

static HRESULT WINAPI settings_put_resolve(loadsettings_i *iface, boolean value)
{
	((struct xml_settings *)iface)->resolve_externals = value;
	return S_OK;
}

static HRESULT WINAPI settings_get_validate(loadsettings_i *iface, boolean *out)
{
	if (!out)
		return E_POINTER;
	*out = ((struct xml_settings *)iface)->validate;
	return S_OK;
}

static HRESULT WINAPI settings_put_validate(loadsettings_i *iface, boolean value)
{
	((struct xml_settings *)iface)->validate = value;
	return S_OK;
}

static HRESULT WINAPI settings_get_ws(loadsettings_i *iface, boolean *out)
{
	if (!out)
		return E_POINTER;
	*out = ((struct xml_settings *)iface)->keep_white_space;
	return S_OK;
}

static HRESULT WINAPI settings_put_ws(loadsettings_i *iface, boolean value)
{
	((struct xml_settings *)iface)->keep_white_space = value;
	return S_OK;
}

static const loadsettings_iVtbl settings_table = {
	settings_query, settings_addref, settings_release, settings_iids, settings_class, settings_trust,
	settings_get_depth, settings_put_depth, settings_get_prohibit, settings_put_prohibit,
	settings_get_resolve, settings_put_resolve, settings_get_validate, settings_put_validate,
	settings_get_ws, settings_put_ws,
};

static struct xml_settings *settings_new(void)
{
	struct xml_settings *self = ALLOC(sizeof *self);
	if (!self)
		return NULL;
	self->lpVtbl = &settings_table;
	self->refs = 1;
	/* the documented defaults: depth 0 (no limit), DTDs prohibited, no resolution, no validation */
	self->max_depth = 0;
	self->prohibit_dtd = TRUE;
	self->resolve_externals = FALSE;
	self->validate = FALSE;
	self->keep_white_space = FALSE;
	InterlockedIncrement(&raven_xaml_objects);
	return self;
}

/* ---------------------------------------------------------------- activation */

static const IActivationFactoryVtbl doc_factory_table;
static const IActivationFactoryVtbl settings_factory_table;
static struct { const IActivationFactoryVtbl *activation; LONG refs; } doc_factory = {&doc_factory_table, 1};
static struct { const IActivationFactoryVtbl *activation; LONG refs; } settings_factory = {&settings_factory_table, 1};

#define XML_FACTORY_COMMON(prefix, factory, class_text) \
	static HRESULT WINAPI prefix##_query(IActivationFactory *self, REFIID iid, void **out); \
	static ULONG WINAPI prefix##_addref(IActivationFactory *self) \
	{ \
		(void)self; \
		return (ULONG)InterlockedIncrement(&(factory).refs); \
	} \
	static ULONG WINAPI prefix##_release(IActivationFactory *self) \
	{ \
		(void)self; \
		return (ULONG)InterlockedDecrement(&(factory).refs); \
	} \
	static HRESULT WINAPI prefix##_iids(IActivationFactory *self, ULONG *count, IID **iids) \
	{ \
		(void)self; \
		return inspectable_iids(count, iids); \
	} \
	static HRESULT WINAPI prefix##_name(IActivationFactory *self, HSTRING *out) \
	{ \
		(void)self; \
		return create_string(class_text, out); \
	} \
	static HRESULT WINAPI prefix##_trust(IActivationFactory *self, TrustLevel *level) \
	{ \
		(void)self; \
		return inspectable_trust(level); \
	}

XML_FACTORY_COMMON(doc_factory, doc_factory, document_class_name)
XML_FACTORY_COMMON(settings_factory, settings_factory, settings_class_name)

static HRESULT WINAPI doc_factory_activate(IActivationFactory *self, IInspectable **out)
{
	struct xml_node *doc;
	HRESULT result;
	(void)self;
	if (!out)
		return E_POINTER;
	*out = NULL;
	doc = xml_doc_new();
	if (!doc)
		return E_OUTOFMEMORY;
	result = ((IInspectable *)&doc->node)->lpVtbl->QueryInterface((IInspectable *)&doc->node, &IID_IInspectable, (void **)out);
	xml_node_release(doc); /* the query holds the guest's reference */
	return result;
}

/* The activation factory answers the statics interface too: a .NET Native program takes
 * IXmlDocumentStatics from RoGetActivationFactory the way it takes IActivationFactory. */
static HRESULT WINAPI doc_factory_query(IActivationFactory *self, REFIID iid, void **out)
{
	(void)self;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID_IActivationFactory) || equal_iid(iid, &IID_IAgileObject))
		*out = (void *)&doc_factory.activation;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentStatics))
		*out = (void *)&docstatics_instance;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&doc_factory.refs);
	return S_OK;
}

static HRESULT WINAPI settings_factory_query(IActivationFactory *self, REFIID iid, void **out)
{
	(void)self;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID_IActivationFactory) || equal_iid(iid, &IID_IAgileObject))
		*out = (void *)&settings_factory.activation;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlLoadSettings)) {
		/* the factory serves the class's default interface, the way a guest that takes it from the activation reads */
		static loadsettings_i *served;
		if (!served) {
			struct xml_settings *settings = settings_new();
			if (!settings)
				return E_OUTOFMEMORY;
			if (InterlockedCompareExchangePointer((void **)&served, (loadsettings_i *)settings, NULL))
				settings_release((loadsettings_i *)settings); /* another thread won the race */
		}
		*out = (void *)served;
		settings_addref((loadsettings_i *)served);
		return S_OK;
	} else
		return E_NOINTERFACE;
	InterlockedIncrement(&settings_factory.refs);
	return S_OK;
}

static HRESULT WINAPI settings_factory_activate(IActivationFactory *self, IInspectable **out)
{
	struct xml_settings *settings;
	HRESULT result;
	(void)self;
	if (!out)
		return E_POINTER;
	*out = NULL;
	settings = settings_new();
	if (!settings)
		return E_OUTOFMEMORY;
	result = ((IInspectable *)settings)->lpVtbl->QueryInterface((IInspectable *)settings, &IID_IInspectable, (void **)out);
	settings_release((loadsettings_i *)settings);
	return result;
}

static const IActivationFactoryVtbl doc_factory_table = {
	doc_factory_query, doc_factory_addref, doc_factory_release, doc_factory_iids, doc_factory_name,
	doc_factory_trust, doc_factory_activate,
};

static const IActivationFactoryVtbl settings_factory_table = {
	settings_factory_query, settings_factory_addref, settings_factory_release, settings_factory_iids,
	settings_factory_name, settings_factory_trust, settings_factory_activate,
};

/* The classes the component's DllGetActivationFactory chain routes here. */
HRESULT raven_xaml_xml_dom_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (length == ARRAYSIZE(document_class_name) - 1 &&
		CompareStringOrdinal(text, (int)length, document_class_name, -1, FALSE) == CSTR_EQUAL) {
		InterlockedIncrement(&doc_factory.refs);
		*out = (IActivationFactory *)&doc_factory.activation;
		return S_OK;
	}
	if (length == ARRAYSIZE(settings_class_name) - 1 &&
		CompareStringOrdinal(text, (int)length, settings_class_name, -1, FALSE) == CSTR_EQUAL) {
		InterlockedIncrement(&settings_factory.refs);
		*out = (IActivationFactory *)&settings_factory.activation;
		return S_OK;
	}
	return CLASS_E_CLASSNOTAVAILABLE;
}
