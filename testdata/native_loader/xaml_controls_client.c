#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <windows.ui.xaml.interop.h>
#include "windows_ui_xaml_controls.h"
#include "windows_ui_xaml_elements.h"
#include "windows_ui_xaml_markup.h"
#include "windows_ui_xaml_application.h"
#include "windows_ui_xaml_window.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);
typedef HRESULT (WINAPI *load_function)(const void *, uint32_t, IInspectable **);

typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame frame1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2 frame2;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3 frame3;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4 frame4;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5 frame5;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPage page1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPanel panel1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock textblock1;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl content_control1;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindow window1;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindowStatics window_statics;
typedef __x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName xaml_type_name;
typedef __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider markup_provider;
typedef __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProviderVtbl markup_providerVtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType markup_type;
typedef __x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlTypeVtbl markup_typeVtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CIApplicationFactory application_factory1;

/* The IVector<UIElement> the panel's Children returns: the parameterized ABI has no mingw header, so the slots the fixture
 * reads are declared locally in metadata order after the six IInspectable methods. */
struct fixture_vector_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(void *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(void *This);
	ULONG (STDMETHODCALLTYPE *Release)(void *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(void *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(void *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(void *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetAt)(void *This, UINT32 index, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(void *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *GetView)(void *This, void **value);
	HRESULT (STDMETHODCALLTYPE *IndexOf)(void *This, IInspectable *element, UINT32 *index, boolean *value);
	HRESULT (STDMETHODCALLTYPE *SetAt)(void *This, UINT32 index, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *InsertAt)(void *This, UINT32 index, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *RemoveAt)(void *This, UINT32 index);
	HRESULT (STDMETHODCALLTYPE *Append)(void *This, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *RemoveAtEnd)(void *This);
	HRESULT (STDMETHODCALLTYPE *Clear)(void *This);
	HRESULT (STDMETHODCALLTYPE *GetMany)(void *This, UINT32 start_index, UINT32 items_size, IInspectable **items, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *ReplaceAll)(void *This, UINT32 count, IInspectable **items);
};

struct fixture_vector {
	const struct fixture_vector_vtbl *lpVtbl;
};

static void put32(unsigned char *bytes, size_t at, uint32_t value)
{
	bytes[at] = (unsigned char)value;
	bytes[at + 1] = (unsigned char)(value >> 8);
	bytes[at + 2] = (unsigned char)(value >> 16);
	bytes[at + 3] = (unsigned char)(value >> 24);
}

static void put64(unsigned char *bytes, size_t at, uint64_t value)
{
	for (int index = 0; index < 8; ++index)
		bytes[at + index] = (unsigned char)(value >> (index * 8));
}

static void put_text_at(unsigned char *doc, size_t at, const char *text);

/* A minimal XBF v2.1 document: an empty metadata section and one substream whose nodes create the type named by its stable index
 * and pop the scope. This is the smallest document the loader can instantiate. */
static size_t build_fixture(unsigned char *doc, uint16_t stable)
{
	for (size_t index = 0; index < 172; ++index)
		doc[index] = 0;
	doc[0] = 'X'; doc[1] = 'B'; doc[2] = 'F';
	put32(doc, 4, 144);
	put32(doc, 8, 16);
	put32(doc, 12, 2);
	put32(doc, 16, 1);
	put64(doc, 20, 0x78);
	put64(doc, 28, 0x7c);
	put64(doc, 36, 0x80);
	put64(doc, 44, 0x84);
	put64(doc, 52, 0x88);
	put64(doc, 60, 0x8c);
	put32(doc, 132, 0);
	put32(doc, 136, 0);
	put32(doc, 140, 0);
	put32(doc, 144, 0);
	put32(doc, 148, 0);
	put32(doc, 152, 0);
	put32(doc, 156, 1);
	put32(doc, 160, 0);
	put32(doc, 164, 4);
	doc[168] = 0x14;
	doc[169] = (unsigned char)(stable & 0xff);
	doc[170] = (unsigned char)((stable >> 8) | 0x80);
	doc[171] = 0x02;
	return 172;
}

static size_t put_text(unsigned char *doc, size_t at, const char *text)
{
	size_t length = 0;
	while (text[length])
		++length;
	put32(doc, at, (uint32_t)length);
	at += 4;
	for (size_t index = 0; index < length; ++index) {
		doc[at++] = (unsigned char)text[index];
		doc[at++] = 0;
	}
	return at;
}

/* A document that names Frame and CacheSize through its own metadata tables, so the type and property references are untrusted
 * and resolved by the document rather than by their stable index. */
static size_t build_metadata_document(unsigned char *doc)
{
	size_t at;
	for (size_t index = 0; index < 308; ++index)
		doc[index] = 0;
	doc[0] = 'X'; doc[1] = 'B'; doc[2] = 'F';
	put32(doc, 4, 272);
	put32(doc, 8, 24);
	put32(doc, 12, 2);
	put32(doc, 16, 1);
	put64(doc, 20, 120);
	put64(doc, 28, 212);
	put64(doc, 36, 224);
	put64(doc, 44, 236);
	put64(doc, 52, 252);
	put64(doc, 60, 268);
	put32(doc, 132, 3);
	at = put_text(doc, 136, "Windows.UI.Xaml.Controls");
	at = put_text(doc, at, "Frame");
	at = put_text(doc, at, "CacheSize");
	put32(doc, 224, 1);
	put32(doc, 228, 0);
	put32(doc, 232, 0);
	put32(doc, 236, 1);
	put32(doc, 240, 0);
	put32(doc, 244, 0);
	put32(doc, 248, 1);
	put32(doc, 252, 0);
	put32(doc, 256, 0);
	put32(doc, 260, 1);
	put32(doc, 264, 1);
	put32(doc, 268, 0);
	put32(doc, 272, 0);
	put32(doc, 276, 2);
	put32(doc, 280, 0);
	put32(doc, 284, 1);
	put32(doc, 288, 0);
	put32(doc, 292, 12);
	doc[296] = 0x14;
	doc[297] = 0x00;
	doc[298] = 0x00;
	doc[299] = 0x1a;
	doc[300] = 0x00;
	doc[301] = 0x00;
	doc[302] = 0x04;
	put32(doc, 303, 7);
	doc[307] = 0x02;
	return 308;
}

/* A Frame document with two property nodes: a name and a cache size. */
static size_t build_frame_properties(unsigned char *doc)
{
	for (size_t index = 0; index < 194; ++index)
		doc[index] = 0;
	doc[0] = 'X'; doc[1] = 'B'; doc[2] = 'F';
	put32(doc, 4, 144);
	put32(doc, 8, 38);
	put32(doc, 12, 2);
	put32(doc, 16, 1);
	put64(doc, 20, 0x78);
	put64(doc, 28, 0x7c);
	put64(doc, 36, 0x80);
	put64(doc, 44, 0x84);
	put64(doc, 52, 0x88);
	put64(doc, 60, 0x8c);
	put32(doc, 156, 1);
	put32(doc, 160, 0);
	put32(doc, 164, 26);
	doc[168] = 0x14;
	doc[169] = (unsigned char)(509 & 0xff);
	doc[170] = (unsigned char)((509 >> 8) | 0x80);
	doc[171] = 0x0d;
	doc[172] = 0x09;
	put32(doc, 173, 4);
	doc[177] = 'r'; doc[179] = 'o'; doc[181] = 'o'; doc[183] = 't';
	doc[185] = 0x1a;
	doc[186] = 0x17;
	doc[187] = 0x84;
	doc[188] = 0x04;
	put32(doc, 189, 5);
	doc[193] = 0x02;
	return 194;
}

/* A page that hosts a tree the element classes build: the stable property indices name Page.Content (999), Panel.Children (648)
 * and TextBlock.Text (800); the stable type indices create Page (525), Grid (442) and TextBlock (396). The loader applies the
 * object properties as the scopes open, so the compiled nodes read: create page, create grid as its content, create text block
 * as a grid child, give the text block its text, close both scopes. */
static size_t build_tree_document(unsigned char *doc)
{
	for (size_t index = 0; index < 203; ++index)
		doc[index] = 0;
	doc[0] = 'X'; doc[1] = 'B'; doc[2] = 'F';
	put32(doc, 4, 144);
	put32(doc, 8, 35);
	put32(doc, 12, 2);
	put32(doc, 16, 1);
	put64(doc, 20, 0x78);
	put64(doc, 28, 0x7c);
	put64(doc, 36, 0x80);
	put64(doc, 44, 0x84);
	put64(doc, 52, 0x88);
	put64(doc, 60, 0x8c);
	put32(doc, 156, 1);
	put32(doc, 160, 0);
	put32(doc, 164, 35);
	doc[168] = 0x14;
	doc[169] = (unsigned char)(525 & 0xff);
	doc[170] = (unsigned char)((525 >> 8) | 0x80);
	doc[171] = 0x07;
	doc[172] = (unsigned char)(999 & 0xff);
	doc[173] = (unsigned char)((999 >> 8) | 0x80);
	doc[174] = 0x14;
	doc[175] = (unsigned char)(442 & 0xff);
	doc[176] = (unsigned char)((442 >> 8) | 0x80);
	doc[177] = 0x07;
	doc[178] = (unsigned char)(648 & 0xff);
	doc[179] = (unsigned char)((648 >> 8) | 0x80);
	doc[180] = 0x14;
	doc[181] = (unsigned char)(396 & 0xff);
	doc[182] = (unsigned char)((396 >> 8) | 0x80);
	doc[183] = 0x1a;
	doc[184] = (unsigned char)(800 & 0xff);
	doc[185] = (unsigned char)((800 >> 8) | 0x80);
	doc[186] = 0x09;
	put32(doc, 187, 5);
	doc[191] = 'H'; doc[193] = 'e'; doc[195] = 'l'; doc[197] = 'l'; doc[199] = 'o';
	doc[201] = 0x02;
	doc[202] = 0x02;
	return 203;
}

/* A document whose own metadata table names an application page: the loader resolves it through the application's metadata
 * provider, not through a framework table. */
static size_t build_provider_document(unsigned char *doc)
{
	for (size_t index = 0; index < 243; ++index)
		doc[index] = 0;
	doc[0] = 'X'; doc[1] = 'B'; doc[2] = 'F';
	put32(doc, 4, 216);
	put32(doc, 8, 3);
	put32(doc, 12, 2);
	put32(doc, 16, 1);
	put64(doc, 20, 120);
	put64(doc, 28, 170);
	put64(doc, 36, 174);
	put64(doc, 44, 186);
	put64(doc, 52, 202);
	put64(doc, 60, 206);
	put32(doc, 132, 2);
	put_text_at(doc, 136, "Shapr3D.App");
	put_text_at(doc, 162, "HomePage");
	put32(doc, 182, 0);
	put32(doc, 186, 1);
	put32(doc, 190, 0);
	put32(doc, 194, 0);
	put32(doc, 198, 1);
	put32(doc, 202, 0);
	put32(doc, 206, 0);
	put32(doc, 210, 1);
	put32(doc, 214, 0);
	put32(doc, 218, 0);
	put32(doc, 228, 1);
	put32(doc, 232, 0);
	put32(doc, 236, 3);
	doc[240] = 0x14;
	doc[241] = 0x00;
	doc[242] = 0x00;
	return 243;
}

static void put_text_at(unsigned char *doc, size_t at, const char *text)
{
	size_t length = 0;
	while (text[length])
		++length;
	put32(doc, at, (uint32_t)length);
	at += 4;
	for (size_t index = 0; index < length; ++index) {
		doc[at++] = (unsigned char)text[index];
		doc[at++] = 0;
	}
}

static int same(HSTRING value, const WCHAR *text, UINT32 expected)
{
	UINT32 length = 0;
	const WCHAR *buffer = WindowsGetStringRawBuffer(value, &length);
	return buffer && length == expected && CompareStringOrdinal(buffer, (int)length, text, -1, FALSE) == CSTR_EQUAL;
}

/* The application metadata provider the fixture composes onto the XAML Application object: this is what a XAML compiler
 * generates on a real application class. The provider serves one application page type, whose ActivateInstance hands out the
 * marker object the loader must return as the document root. One vtable serves IInspectable and IXamlMetadataProvider, the
 * latter being a strict superset of the former. */
static const WCHAR app_page_name[] = L"Shapr3D.App.HomePage";

struct fixture_marker {
	const IInspectableVtbl *lpVtbl;
	LONG refs;
};

struct fixture_type {
	const markup_typeVtbl *lpVtbl;
	LONG refs;
	HSTRING name;
	IInspectable *instance;
};

struct fixture_provider {
	const markup_providerVtbl *lpVtbl;
	LONG refs;
	struct fixture_type *type;
};

static HRESULT WINAPI fixture_marker_query(IInspectable *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!IsEqualGUID(iid, &IID_IUnknown) && !IsEqualGUID(iid, &IID_IInspectable))
		return E_NOINTERFACE;
	InterlockedIncrement(&((struct fixture_marker *)iface)->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI fixture_marker_addref(IInspectable *iface)
{
	return (ULONG)InterlockedIncrement(&((struct fixture_marker *)iface)->refs);
}

static ULONG WINAPI fixture_marker_release(IInspectable *iface)
{
	struct fixture_marker *self = (struct fixture_marker *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs)
		HeapFree(GetProcessHeap(), 0, self);
	return (ULONG)refs;
}

static HRESULT WINAPI fixture_marker_iids(IInspectable *iface, ULONG *count, IID **iids)
{
	(void)iface;
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}

static HRESULT WINAPI fixture_marker_name(IInspectable *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(app_page_name, ARRAYSIZE(app_page_name) - 1, name);
}

static HRESULT WINAPI fixture_marker_trust(IInspectable *iface, TrustLevel *level)
{
	(void)iface;
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

static const IInspectableVtbl fixture_marker_table = {
	.QueryInterface = fixture_marker_query,
	.AddRef = fixture_marker_addref,
	.Release = fixture_marker_release,
	.GetIids = fixture_marker_iids,
	.GetRuntimeClassName = fixture_marker_name,
	.GetTrustLevel = fixture_marker_trust,
};

static IInspectable *fixture_marker_create(void)
{
	struct fixture_marker *marker = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *marker);
	if (!marker)
		return NULL;
	marker->lpVtbl = &fixture_marker_table;
	marker->refs = 1;
	return (IInspectable *)marker;
}

static HRESULT WINAPI fixture_type_query(markup_type *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!IsEqualGUID(iid, &IID_IUnknown) && !IsEqualGUID(iid, &IID_IInspectable) &&
		!IsEqualGUID(iid, &IID__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlType))
		return E_NOINTERFACE;
	InterlockedIncrement(&((struct fixture_type *)iface)->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI fixture_type_addref(markup_type *iface)
{
	return (ULONG)InterlockedIncrement(&((struct fixture_type *)iface)->refs);
}

static ULONG WINAPI fixture_type_release(markup_type *iface)
{
	struct fixture_type *self = (struct fixture_type *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->name)
			WindowsDeleteString(self->name);
		if (self->instance)
			self->instance->lpVtbl->Release(self->instance);
		HeapFree(GetProcessHeap(), 0, self);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI fixture_type_iids(markup_type *iface, ULONG *count, IID **iids)
{
	(void)iface;
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}

static HRESULT WINAPI fixture_type_class(markup_type *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(app_page_name, ARRAYSIZE(app_page_name) - 1, name);
}

static HRESULT WINAPI fixture_type_trust(markup_type *iface, TrustLevel *level)
{
	(void)iface;
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

#define TYPE_STUB1(name) static HRESULT WINAPI name(markup_type *This, uint64_t a1) \
	{ (void)This; if (a1) *(void **)(uintptr_t)a1 = NULL; return E_NOTIMPL; }

TYPE_STUB1(fixture_type_base)
TYPE_STUB1(fixture_type_content_property)
static HRESULT WINAPI fixture_type_full_name(markup_type *iface, uint64_t out)
{
	if (!out)
		return E_POINTER;
	*(HSTRING *)(uintptr_t)out = ((struct fixture_type *)iface)->name;
	if (((struct fixture_type *)iface)->name)
		WindowsDuplicateString(((struct fixture_type *)iface)->name, (HSTRING *)(uintptr_t)out);
	return S_OK;
}
TYPE_STUB1(fixture_type_is_array)
TYPE_STUB1(fixture_type_is_collection)
TYPE_STUB1(fixture_type_is_constructible)
TYPE_STUB1(fixture_type_is_dictionary)
TYPE_STUB1(fixture_type_is_markup)
TYPE_STUB1(fixture_type_is_bindable)
TYPE_STUB1(fixture_type_item_type)
TYPE_STUB1(fixture_type_key_type)
TYPE_STUB1(fixture_type_underlying)
static HRESULT WINAPI fixture_type_activate(markup_type *iface, uint64_t out)
{
	struct fixture_type *self = (struct fixture_type *)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = self->instance;
	if (self->instance)
		self->instance->lpVtbl->AddRef(self->instance);
	return S_OK;
}
static HRESULT WINAPI fixture_type_create_from_string(markup_type *This, uint64_t a1, uint64_t a2)
{
	(void)This; (void)a1; if (a2) *(void **)(uintptr_t)a2 = NULL; return E_NOTIMPL;
}
static HRESULT WINAPI fixture_type_get_member(markup_type *This, uint64_t a1, uint64_t a2)
{
	(void)This; (void)a1; if (a2) *(void **)(uintptr_t)a2 = NULL; return E_NOTIMPL;
}
static HRESULT WINAPI fixture_type_add_to_vector(markup_type *This, uint64_t a1, uint64_t a2)
{ (void)This; (void)a1; (void)a2; return E_NOTIMPL; }
static HRESULT WINAPI fixture_type_add_to_map(markup_type *This, uint64_t a1, uint64_t a2, uint64_t a3)
{ (void)This; (void)a1; (void)a2; (void)a3; return E_NOTIMPL; }
static HRESULT WINAPI fixture_type_run_initializer(markup_type *This)
{ (void)This; return E_NOTIMPL; }

static const markup_typeVtbl fixture_type_table = {
	.QueryInterface = fixture_type_query,
	.AddRef = fixture_type_addref,
	.Release = fixture_type_release,
	.GetIids = fixture_type_iids,
	.GetRuntimeClassName = fixture_type_class,
	.GetTrustLevel = fixture_type_trust,
	.get_BaseType = fixture_type_base,
	.get_ContentProperty = fixture_type_content_property,
	.get_FullName = fixture_type_full_name,
	.get_IsArray = fixture_type_is_array,
	.get_IsCollection = fixture_type_is_collection,
	.get_IsConstructible = fixture_type_is_constructible,
	.get_IsDictionary = fixture_type_is_dictionary,
	.get_IsMarkupExtension = fixture_type_is_markup,
	.get_IsBindable = fixture_type_is_bindable,
	.get_ItemType = fixture_type_item_type,
	.get_KeyType = fixture_type_key_type,
	.get_UnderlyingType = fixture_type_underlying,
	.ActivateInstance = fixture_type_activate,
	.CreateFromString = fixture_type_create_from_string,
	.GetMember = fixture_type_get_member,
	.AddToVector = fixture_type_add_to_vector,
	.AddToMap = fixture_type_add_to_map,
	.RunInitializer = fixture_type_run_initializer,
};

static struct fixture_type *fixture_type_create(const WCHAR *name, IInspectable *instance)
{
	struct fixture_type *type = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *type);
	if (!type)
		return NULL;
	type->lpVtbl = &fixture_type_table;
	type->refs = 1;
	if (WindowsCreateString(name, (UINT32)lstrlenW(name), &type->name) != S_OK) {
		HeapFree(GetProcessHeap(), 0, type);
		return NULL;
	}
	type->instance = instance;
	instance->lpVtbl->AddRef(instance);
	return type;
}

static HRESULT WINAPI fixture_provider_query(markup_provider *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!IsEqualGUID(iid, &IID_IUnknown) && !IsEqualGUID(iid, &IID_IInspectable) &&
		!IsEqualGUID(iid, &IID__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider))
		return E_NOINTERFACE;
	InterlockedIncrement(&((struct fixture_provider *)iface)->refs);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI fixture_provider_addref(markup_provider *iface)
{
	return (ULONG)InterlockedIncrement(&((struct fixture_provider *)iface)->refs);
}

static ULONG WINAPI fixture_provider_release(markup_provider *iface)
{
	struct fixture_provider *self = (struct fixture_provider *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->type)
			self->type->lpVtbl->Release((markup_type *)self->type);
		HeapFree(GetProcessHeap(), 0, self);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI fixture_provider_iids(markup_provider *iface, ULONG *count, IID **iids)
{
	(void)iface;
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}

static HRESULT WINAPI fixture_provider_class(markup_provider *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(app_page_name, ARRAYSIZE(app_page_name) - 1, name);
}

static HRESULT WINAPI fixture_provider_trust(markup_provider *iface, TrustLevel *level)
{
	(void)iface;
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

static HRESULT WINAPI fixture_provider_by_type(markup_provider *This, uint64_t type, uint64_t out)
{
	(void)This;
	(void)type;
	if (out)
		*(void **)(uintptr_t)out = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI fixture_provider_by_name(markup_provider *iface, uint64_t name, uint64_t out)
{
	struct fixture_provider *self = (struct fixture_provider *)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	if (!name || !same((HSTRING)(uintptr_t)name, app_page_name, ARRAYSIZE(app_page_name) - 1))
		return S_OK;
	self->type->lpVtbl->AddRef((markup_type *)self->type);
	*(void **)(uintptr_t)out = self->type;
	return S_OK;
}

static HRESULT WINAPI fixture_provider_xmlns(markup_provider *This, uint64_t out)
{
	(void)This;
	if (out)
		*(void **)(uintptr_t)out = NULL;
	return E_NOTIMPL;
}

static const markup_providerVtbl fixture_provider_table = {
	.QueryInterface = fixture_provider_query,
	.AddRef = fixture_provider_addref,
	.Release = fixture_provider_release,
	.GetIids = fixture_provider_iids,
	.GetRuntimeClassName = fixture_provider_class,
	.GetTrustLevel = fixture_provider_trust,
	.GetXamlType = fixture_provider_by_type,
	.GetXamlType2 = fixture_provider_by_name,
	.GetXmlnsDefinitions = fixture_provider_xmlns,
};

static struct fixture_provider *fixture_provider_create(struct fixture_type *type)
{
	struct fixture_provider *provider = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *provider);
	if (!provider)
		return NULL;
	provider->lpVtbl = &fixture_provider_table;
	provider->refs = 1;
	provider->type = type;
	type->lpVtbl->AddRef((markup_type *)type);
	return provider;
}

/* Frame and Page through the metadata provider and the XBF loader: Navigate resolves a type name and creates the page, the frame
 * reports its source type, and a compiled document instantiates its root type. */
int raven_entry(void)
{
	static const WCHAR frame_name[] = L"Windows.UI.Xaml.Controls.Frame";
	static const WCHAR page_name[] = L"Windows.UI.Xaml.Controls.Page";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function RoGetActivationFactory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	if (!RoInitialize || !RoGetActivationFactory) return 20;
	if (FAILED(RoInitialize(1))) return 1;

	HSTRING frame_key = NULL, page_key = NULL;
	if (WindowsCreateString(frame_name, ARRAYSIZE(frame_name) - 1, &frame_key) != S_OK ||
		WindowsCreateString(page_name, ARRAYSIZE(page_name) - 1, &page_key) != S_OK) return 21;
	IActivationFactory *frame_factory = NULL, *page_factory = NULL;
	if (RoGetActivationFactory(frame_key, &IID_IActivationFactory, (void **)&frame_factory) != S_OK || !frame_factory) return 2;
	if (RoGetActivationFactory(page_key, &IID_IActivationFactory, (void **)&page_factory) != S_OK || !page_factory) return 3;
	IInspectable *frame_object = NULL, *page_object = NULL;
	if (frame_factory->lpVtbl->ActivateInstance(frame_factory, &frame_object) != S_OK || !frame_object) return 4;
	if (page_factory->lpVtbl->ActivateInstance(page_factory, &page_object) != S_OK || !page_object) return 5;
	frame1 *frame = NULL;
	if (frame_object->lpVtbl->QueryInterface(frame_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame, (void **)&frame) != S_OK ||
		!frame) return 6;
	page1 *page = NULL;
	if (page_object->lpVtbl->QueryInterface(page_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPage, (void **)&page) != S_OK ||
		!page) return 7;

	xaml_type_name navigate_to;
	navigate_to.Name = page_key;
	navigate_to.Kind = TypeKind_Metadata;
	boolean navigated = FALSE;
	if (frame->lpVtbl->Navigate(frame, (uint64_t)(uintptr_t)&navigate_to, 0, (uint64_t)(uintptr_t)&navigated) != S_OK ||
		!navigated) return 8;
	xaml_type_name current = {NULL, TypeKind_Metadata};
	if (frame->lpVtbl->get_CurrentSourcePageType(frame, (uint64_t)(uintptr_t)&current) != S_OK ||
		!current.Name || !same(current.Name, page_name, ARRAYSIZE(page_name) - 1)) return 9;
	WindowsDeleteString(current.Name);
	boolean can_back = TRUE;
	if (frame->lpVtbl->get_CanGoBack(frame, (uint64_t)(uintptr_t)&can_back) != S_OK || can_back) return 10;
	frame2 *frame_later = NULL;
	frame3 *frame_third = NULL;
	frame4 *frame_fourth = NULL;
	frame5 *frame_fifth = NULL;
	if (frame->lpVtbl->QueryInterface(frame, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame2, (void **)&frame_later) != S_OK ||
		!frame_later) return 11;
	if (frame->lpVtbl->QueryInterface(frame, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame3, (void **)&frame_third) != S_OK ||
		!frame_third) return 12;
	if (frame->lpVtbl->QueryInterface(frame, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame4, (void **)&frame_fourth) != S_OK ||
		!frame_fourth) return 13;
	if (frame->lpVtbl->QueryInterface(frame, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame5, (void **)&frame_fifth) != S_OK ||
		!frame_fifth) return 14;
	if (page->lpVtbl->put_NavigationCacheMode(page, 2) != S_OK) return 15;
	int32_t mode = -1;
	if (page->lpVtbl->get_NavigationCacheMode(page, (uint64_t)(uintptr_t)&mode) != S_OK || mode != 2) return 16;
	HSTRING class_name = NULL;
	if (frame_object->lpVtbl->GetRuntimeClassName(frame_object, &class_name) != S_OK ||
		!same(class_name, frame_name, ARRAYSIZE(frame_name) - 1)) return 17;
	WindowsDeleteString(class_name);

	HMODULE component = LoadLibraryW(L"raven-xaml.dll");
	load_function load = component ? (load_function)GetProcAddress(component, "RavenXamlLoad") : NULL;
	if (!load) return 18;
	unsigned char fixture[172];
	IInspectable *loaded = NULL;
	if (load(fixture, (uint32_t)build_fixture(fixture, 509), &loaded) != S_OK || !loaded) return 19;
	frame1 *loaded_frame = NULL;
	if (loaded->lpVtbl->QueryInterface(loaded, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame, (void **)&loaded_frame) != S_OK ||
		!loaded_frame) return 22;
	loaded_frame->lpVtbl->Release(loaded_frame);
	loaded->lpVtbl->Release(loaded);
	if (load(fixture, (uint32_t)build_fixture(fixture, 525), &loaded) != S_OK || !loaded) return 23;
	page1 *loaded_page = NULL;
	if (loaded->lpVtbl->QueryInterface(loaded, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPage, (void **)&loaded_page) != S_OK ||
		!loaded_page) return 24;
	loaded_page->lpVtbl->Release(loaded_page);
	loaded->lpVtbl->Release(loaded);

	unsigned char property_fixture[194];
	IInspectable *property_root = NULL;
	if (load(property_fixture, (uint32_t)build_frame_properties(property_fixture), &property_root) != S_OK || !property_root) return 25;
	frame1 *configured = NULL;
	if (property_root->lpVtbl->QueryInterface(property_root, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame, (void **)&configured) != S_OK ||
		!configured) return 26;
	int32_t cache = 0;
	if (configured->lpVtbl->get_CacheSize(configured, (uint64_t)(uintptr_t)&cache) != S_OK || cache != 5) return 27;
	configured->lpVtbl->Release(configured);
	property_root->lpVtbl->Release(property_root);

	unsigned char metadata_fixture[308];
	IInspectable *metadata_root = NULL;
	HRESULT metadata_status = load(metadata_fixture, (uint32_t)build_metadata_document(metadata_fixture), &metadata_root);
	if (metadata_status != S_OK || !metadata_root)
		return 100 + (int)(metadata_status & 0xfff);
	frame1 *metadata_frame = NULL;
	if (metadata_root->lpVtbl->QueryInterface(metadata_root, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIFrame,
		(void **)&metadata_frame) != S_OK || !metadata_frame) return 36;
	int32_t metadata_cache = 0;
	if (metadata_frame->lpVtbl->get_CacheSize(metadata_frame, (uint64_t)(uintptr_t)&metadata_cache) != S_OK ||
		metadata_cache != 7) return 37;
	metadata_frame->lpVtbl->Release(metadata_frame);
	metadata_root->lpVtbl->Release(metadata_root);

	IInspectable *hosted_root = NULL;
	if (load(fixture, (uint32_t)build_fixture(fixture, 525), &hosted_root) != S_OK || !hosted_root) return 28;
	HSTRING window_key = NULL;
	if (WindowsCreateString(L"Windows.UI.Xaml.Window", 22, &window_key) != S_OK) return 29;
	IActivationFactory *window_factory = NULL;
	if (RoGetActivationFactory(window_key, &IID_IActivationFactory, (void **)&window_factory) != S_OK || !window_factory) return 30;
	window_statics *window_current = NULL;
	if (window_factory->lpVtbl->QueryInterface(window_factory, &IID__x_ABI_CWindows_CUI_CXaml_CIWindowStatics,
		(void **)&window_current) != S_OK || !window_current) return 31;
	window1 *window = NULL;
	if (window_current->lpVtbl->get_Current(window_current, (uint64_t)(uintptr_t)&window) != S_OK || !window) return 32;
	if (window->lpVtbl->put_Content(window, (uint64_t)(uintptr_t)hosted_root) != S_OK) return 33;
	void *content = NULL;
	if (window->lpVtbl->get_Content(window, (uint64_t)(uintptr_t)&content) != S_OK || content != hosted_root) return 34;
	IInspectable *hosted_check = (IInspectable *)content;
	hosted_check->lpVtbl->Release(hosted_check);
	window->lpVtbl->Release(window);
	window_current->lpVtbl->Release(window_current);
	window_factory->lpVtbl->Release(window_factory);
	WindowsDeleteString(window_key);
	hosted_root->lpVtbl->Release(hosted_root);

	/* Direct round-trip on the Page content part before the loader path. */
	HSTRING direct_page_key = NULL;
	if (WindowsCreateString(L"Windows.UI.Xaml.Controls.Page", 29, &direct_page_key) != S_OK) return 901;
	IActivationFactory *direct_page_factory = NULL;
	if (RoGetActivationFactory(direct_page_key, &IID_IActivationFactory, (void **)&direct_page_factory) != S_OK || !direct_page_factory) return 902;
	IInspectable *direct_page = NULL;
	if (direct_page_factory->lpVtbl->ActivateInstance(direct_page_factory, &direct_page) != S_OK || !direct_page) return 903;
	content_control1 *direct_content = NULL;
	if (direct_page->lpVtbl->QueryInterface(direct_page, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
		(void **)&direct_content) != S_OK || !direct_content) return 904;
	HSTRING probe_key = NULL;
	if (WindowsCreateString(L"Windows.UI.Xaml.Controls.TextBlock", 34, &probe_key) != S_OK) return 905;
	IActivationFactory *probe_factory = NULL;
	if (RoGetActivationFactory(probe_key, &IID_IActivationFactory, (void **)&probe_factory) != S_OK || !probe_factory) return 906;
	IInspectable *probe_child = NULL;
	if (probe_factory->lpVtbl->ActivateInstance(probe_factory, &probe_child) != S_OK || !probe_child) return 907;
	if (direct_content->lpVtbl->put_Content(direct_content, (uint64_t)(uintptr_t)probe_child) != S_OK) return 908;
	IInspectable *direct_stored = NULL;
	if (direct_content->lpVtbl->get_Content(direct_content, (uint64_t)(uintptr_t)&direct_stored) != S_OK || !direct_stored) return 909;
	direct_stored->lpVtbl->Release(direct_stored);
	direct_content->lpVtbl->Release(direct_content);
	direct_page->lpVtbl->Release(direct_page);
	direct_page_factory->lpVtbl->Release(direct_page_factory);
	probe_child->lpVtbl->Release(probe_child);
	probe_factory->lpVtbl->Release(probe_factory);
	WindowsDeleteString(probe_key);
	WindowsDeleteString(direct_page_key);

	/* The element tree: a compiled page hosts a Grid through its Content, the Grid reports the TextBlock among its children,
	 * and the TextBlock carries the text the document gave it. */
	unsigned char tree_fixture[203];
	IInspectable *tree_root = NULL;
	HRESULT tree_status = load(tree_fixture, (uint32_t)build_tree_document(tree_fixture), &tree_root);
	if (tree_status != S_OK || !tree_root)
		return 200 + (int)(tree_status & 0xfff);
	content_control1 *page_content = NULL;
	if (tree_root->lpVtbl->QueryInterface(tree_root, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
		(void **)&page_content) != S_OK || !page_content) return 52;
	IInspectable *grid_object = NULL;
	if (page_content->lpVtbl->get_Content(page_content, (uint64_t)(uintptr_t)&grid_object) != S_OK || !grid_object) return 53;
	panel1 *grid_panel = NULL;
	if (grid_object->lpVtbl->QueryInterface(grid_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
		(void **)&grid_panel) != S_OK || !grid_panel) return 54;
	void *children_object = NULL;
	if (grid_panel->lpVtbl->get_Children(grid_panel, (uint64_t)(uintptr_t)&children_object) != S_OK || !children_object) return 55;
	struct fixture_vector *children = (struct fixture_vector *)children_object;
	UINT32 child_count = 0;
	if (children->lpVtbl->get_Size(children, &child_count) != S_OK || child_count != 1) return 56;
	IInspectable *text_object = NULL;
	if (children->lpVtbl->GetAt(children, 0, &text_object) != S_OK || !text_object) return 57;
	textblock1 *textblock = NULL;
	if (text_object->lpVtbl->QueryInterface(text_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
		(void **)&textblock) != S_OK || !textblock) return 58;
	HSTRING text = NULL;
	if (textblock->lpVtbl->get_Text(textblock, (uint64_t)(uintptr_t)&text) != S_OK || !text ||
		!same(text, L"Hello", 5)) return 59;
	WindowsDeleteString(text);
	textblock->lpVtbl->Release(textblock);
	children->lpVtbl->Release(children);
	grid_panel->lpVtbl->Release(grid_panel);
	grid_object->lpVtbl->Release(grid_object);
	page_content->lpVtbl->Release(page_content);
	tree_root->lpVtbl->Release(tree_root);

	/* Button: the content control slot of the class stores and returns the element it is given. */
	HSTRING button_key = NULL;
	if (WindowsCreateString(L"Windows.UI.Xaml.Controls.Button", 31, &button_key) != S_OK) return 61;
	IActivationFactory *button_factory = NULL;
	if (RoGetActivationFactory(button_key, &IID_IActivationFactory, (void **)&button_factory) != S_OK || !button_factory) return 62;
	IInspectable *button_object = NULL;
	if (button_factory->lpVtbl->ActivateInstance(button_factory, &button_object) != S_OK || !button_object) return 63;
	content_control1 *button_content = NULL;
	if (button_object->lpVtbl->QueryInterface(button_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
		(void **)&button_content) != S_OK || !button_content) return 64;
	HSTRING textblock_key = NULL;
	if (WindowsCreateString(L"Windows.UI.Xaml.Controls.TextBlock", 34, &textblock_key) != S_OK) return 641;
	IActivationFactory *textblock_factory = NULL;
	if (RoGetActivationFactory(textblock_key, &IID_IActivationFactory, (void **)&textblock_factory) != S_OK || !textblock_factory) return 642;
	IInspectable *button_child = NULL;
	if (textblock_factory->lpVtbl->ActivateInstance(textblock_factory, &button_child) != S_OK || !button_child) return 643;
	if (button_content->lpVtbl->put_Content(button_content, (uint64_t)(uintptr_t)button_child) != S_OK) return 65;
	IInspectable *stored = NULL;
	if (button_content->lpVtbl->get_Content(button_content, (uint64_t)(uintptr_t)&stored) != S_OK || !stored) return 66;
	IUnknown *stored_unknown = NULL, *child_unknown = NULL;
	if (stored->lpVtbl->QueryInterface(stored, &IID_IUnknown, (void **)&stored_unknown) != S_OK || !stored_unknown) return 67;
	if (button_child->lpVtbl->QueryInterface(button_child, &IID_IUnknown, (void **)&child_unknown) != S_OK || !child_unknown) return 68;
	if (stored_unknown != child_unknown) return 69;
	child_unknown->lpVtbl->Release(child_unknown);
	stored_unknown->lpVtbl->Release(stored_unknown);
	stored->lpVtbl->Release(stored);
	button_content->lpVtbl->Release(button_content);
	button_object->lpVtbl->Release(button_object);
	button_factory->lpVtbl->Release(button_factory);
	textblock_factory->lpVtbl->Release(textblock_factory);
	button_child->lpVtbl->Release(button_child);
	WindowsDeleteString(textblock_key);
	WindowsDeleteString(button_key);

	/* The application metadata provider: the fixture composes its provider onto the XAML Application object, the loader
	 * reaches it through Application.Current, and an application page type resolves to the object the provider creates. */
	IInspectable *marker = fixture_marker_create();
	if (!marker) return 671;
	struct fixture_type *fixture_type = fixture_type_create(app_page_name, marker);
	if (!fixture_type) return 672;
	struct fixture_provider *provider = fixture_provider_create(fixture_type);
	if (!provider) return 673;
	HSTRING app_key = NULL;
	if (WindowsCreateString(L"Windows.UI.Xaml.Application", 27, &app_key) != S_OK) return 674;
	IActivationFactory *app_activation = NULL;
	if (RoGetActivationFactory(app_key, &IID_IActivationFactory, (void **)&app_activation) != S_OK || !app_activation) return 675;
	application_factory1 *app_factory = NULL;
	if (app_activation->lpVtbl->QueryInterface(app_activation, &IID__x_ABI_CWindows_CUI_CXaml_CIApplicationFactory,
		(void **)&app_factory) != S_OK || !app_factory) return 676;
	void *app_inner = NULL;
	IInspectable *app_instance = NULL;
	if (app_factory->lpVtbl->CreateInstance(app_factory, (uint64_t)(uintptr_t)provider,
		(uint64_t)(uintptr_t)&app_inner, (uint64_t)(uintptr_t)&app_instance) != S_OK || !app_instance) return 677;
	markup_provider *resolved = NULL;
	if (app_instance->lpVtbl->QueryInterface(app_instance, &IID__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider,
		(void **)&resolved) != S_OK || !resolved) return 678;
	if ((void *)resolved != (void *)provider) return 679;
	resolved->lpVtbl->Release(resolved);
	unsigned char provider_fixture[243];
	IInspectable *provider_root = NULL;
	HRESULT provider_status = load(provider_fixture, (uint32_t)build_provider_document(provider_fixture), &provider_root);
	if (provider_status != S_OK || !provider_root)
		return 3000 + (int)(provider_status & 0xffff);
	IUnknown *root_unknown = NULL, *marker_unknown = NULL;
	if (provider_root->lpVtbl->QueryInterface(provider_root, &IID_IUnknown, (void **)&root_unknown) != S_OK || !root_unknown)
		return 682;
	if (marker->lpVtbl->QueryInterface(marker, &IID_IUnknown, (void **)&marker_unknown) != S_OK || !marker_unknown) return 683;
	if (root_unknown != marker_unknown) return 684;
	marker_unknown->lpVtbl->Release(marker_unknown);
	root_unknown->lpVtbl->Release(root_unknown);
	provider_root->lpVtbl->Release(provider_root);
	/* Frame navigation resolves the same application type through the provider. */
	HSTRING app_page_key = NULL;
	if (WindowsCreateString(app_page_name, ARRAYSIZE(app_page_name) - 1, &app_page_key) != S_OK) return 685;
	xaml_type_name page_type;
	page_type.Name = app_page_key;
	page_type.Kind = TypeKind_Metadata;
	boolean navigated_app = FALSE;
	if (frame->lpVtbl->Navigate(frame, (uint64_t)(uintptr_t)&page_type, 0, (uint64_t)(uintptr_t)&navigated_app) != S_OK ||
		!navigated_app) return 686;
	WindowsDeleteString(app_page_key);
	app_instance->lpVtbl->Release(app_instance);
	app_factory->lpVtbl->Release(app_factory);
	app_activation->lpVtbl->Release(app_activation);
	WindowsDeleteString(app_key);
	provider->lpVtbl->Release((markup_provider *)provider);
	fixture_type->lpVtbl->Release((markup_type *)fixture_type);
	marker->lpVtbl->Release(marker);

	frame_fifth->lpVtbl->Release(frame_fifth);
	frame_fourth->lpVtbl->Release(frame_fourth);
	frame_third->lpVtbl->Release(frame_third);
	frame_later->lpVtbl->Release(frame_later);
	page->lpVtbl->Release(page);
	frame->lpVtbl->Release(frame);
	page_object->lpVtbl->Release(page_object);
	frame_object->lpVtbl->Release(frame_object);
	page_factory->lpVtbl->Release(page_factory);
	frame_factory->lpVtbl->Release(frame_factory);
	WindowsDeleteString(page_key);
	WindowsDeleteString(frame_key);
	return 0;
}
