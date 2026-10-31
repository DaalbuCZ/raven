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

/* The real IControlTemplate and IControl IIDs (Windows.UI.Xaml.winmd, the WDK IDL): the engine's ControlTemplate answers
 * the first, the engine's button answers the second. The vtables here carry the exact member order — TargetType get/put on
 * the template, the Template pair among the 41 control members — with the slots the fixture does not call left declared
 * but unused. */
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplate, 0xefd2418e, 0x41e0, 0x48bb, 0x8b, 0x82, 0x91,
	0xed, 0xa1, 0xba, 0x3f, 0xe2);
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControl, 0xa8912263, 0x2951, 0x4f58, 0xa9, 0xc5, 0x5a, 0x13,
	0x4e, 0xaa, 0x7f, 0x07);

struct fixture_template_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(void *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(void *This);
	ULONG (STDMETHODCALLTYPE *Release)(void *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(void *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(void *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(void *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_TargetType)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_TargetType)(void *This, uint64_t value);
};

struct fixture_control_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(void *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(void *This);
	ULONG (STDMETHODCALLTYPE *Release)(void *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(void *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(void *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(void *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_FontSize)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_FontSize)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_FontFamily)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_FontFamily)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_FontWeight)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_FontWeight)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_FontStyle)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_FontStyle)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_FontStretch)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_FontStretch)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_CharacterSpacing)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_CharacterSpacing)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_Foreground)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_Foreground)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_IsTabStop)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_IsTabStop)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_IsEnabled)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_IsEnabled)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_TabIndex)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_TabIndex)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_TabNavigation)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_TabNavigation)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_Template)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_Template)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_Padding)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_Padding)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_HorizontalContentAlignment)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_HorizontalContentAlignment)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_VerticalContentAlignment)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_VerticalContentAlignment)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_Background)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_Background)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_BorderThickness)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_BorderThickness)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_BorderBrush)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *put_BorderBrush)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *get_FocusState)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *add_IsEnabledChanged)(void *This, uint64_t handler, uint64_t token);
	HRESULT (STDMETHODCALLTYPE *remove_IsEnabledChanged)(void *This, uint64_t token);
	HRESULT (STDMETHODCALLTYPE *ApplyTemplate)(void *This, uint64_t value);
	HRESULT (STDMETHODCALLTYPE *Focus)(void *This, uint64_t state, uint64_t value);
};

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
	put32(doc, 8, 47);
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

/* A TextBlock whose text properties all arrive by stable index: TextElement.FontSize (251) as a float constant and
 * TextElement.Foreground (255) as the named-colour string, beside the established TextBlock.Text (800). The empty
 * metadata section forces every reference through the stable table. The engine's float and text dispatches apply
 * "FontSize" and "Foreground" by simple name, so the loader resolves both on the TextBlock. */
static size_t build_text_document(unsigned char *doc)
{
	for (size_t index = 0; index < 206; ++index)
		doc[index] = 0;
	doc[0] = 'X'; doc[1] = 'B'; doc[2] = 'F';
	put32(doc, 4, 144);
	put32(doc, 8, 50);
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
	put32(doc, 164, 38);
	/* create TextBlock (stable 396) */
	doc[168] = 0x14;
	doc[169] = (unsigned char)(396 & 0xff);
	doc[170] = (unsigned char)((396 >> 8) | 0x80);
	/* TextElement.FontSize (stable 251) = 21.5f */
	doc[171] = 0x1a;
	doc[172] = (unsigned char)(251 & 0xff);
	doc[173] = (unsigned char)((251 >> 8) | 0x80);
	doc[174] = 0x03;
	put32(doc, 175, 0x41ac0000); /* the IEEE-754 bits of 21.5f, the float constant kind */
	/* TextElement.Foreground (stable 255) = "Red", a colour the engine's named-colour path knows */
	doc[179] = 0x1a;
	doc[180] = (unsigned char)(255 & 0xff);
	doc[181] = (unsigned char)((255 >> 8) | 0x80);
	doc[182] = 0x09;
	put32(doc, 183, 3);
	doc[187] = 'R'; doc[189] = 'e'; doc[191] = 'd';
	/* TextBlock.Text (stable 800) = "Hi" */
	doc[193] = 0x1a;
	doc[194] = (unsigned char)(800 & 0xff);
	doc[195] = (unsigned char)((800 >> 8) | 0x80);
	doc[196] = 0x09;
	put32(doc, 197, 2);
	doc[201] = 'H'; doc[203] = 'i';
	doc[205] = 0x02;
	return 206;
}

/* A presenter carrying its content template by stable index alone: a Grid (442) holds one ContentPresenter (296) under
 * Panel.Children (648), and the presenter's ContentControl.ContentTemplate (833 — the content-control owner of the
 * member the presenter store accepts) names a ControlTemplate (191) whose root is a TextBlock (396) with Text (800).
 * The loader defers the template application to the template scope's pop, so the presenter stores the finished tree. */
static size_t build_content_template_document(unsigned char *doc)
{
	for (size_t index = 0; index < 200; ++index)
		doc[index] = 0;
	doc[0] = 'X'; doc[1] = 'B'; doc[2] = 'F';
	put32(doc, 4, 144);
	put32(doc, 8, 44);
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
	put32(doc, 164, 32);
	/* create Grid (stable 442) */
	doc[168] = 0x14;
	doc[169] = (unsigned char)(442 & 0xff);
	doc[170] = (unsigned char)((442 >> 8) | 0x80);
	/* Panel.Children (stable 648) pending */
	doc[171] = 0x07;
	doc[172] = (unsigned char)(648 & 0xff);
	doc[173] = (unsigned char)((648 >> 8) | 0x80);
	/* create ContentPresenter (stable 296) as the child */
	doc[174] = 0x14;
	doc[175] = (unsigned char)(296 & 0xff);
	doc[176] = (unsigned char)((296 >> 8) | 0x80);
	/* ContentControl.ContentTemplate (stable 833) pending */
	doc[177] = 0x07;
	doc[178] = (unsigned char)(833 & 0xff);
	doc[179] = (unsigned char)((833 >> 8) | 0x80);
	/* create ControlTemplate (stable 191): the template scope opens with the deferred application to the presenter */
	doc[180] = 0x14;
	doc[181] = (unsigned char)(191 & 0xff);
	doc[182] = (unsigned char)((191 >> 8) | 0x80);
	/* create TextBlock (stable 396): the template root */
	doc[183] = 0x14;
	doc[184] = (unsigned char)(396 & 0xff);
	doc[185] = (unsigned char)((396 >> 8) | 0x80);
	/* TextBlock.Text (stable 800) = "T" */
	doc[186] = 0x1a;
	doc[187] = (unsigned char)(800 & 0xff);
	doc[188] = (unsigned char)((800 >> 8) | 0x80);
	doc[189] = 0x09;
	put32(doc, 190, 1);
	doc[194] = 'T';
	/* close the TextBlock scope, the template scope (the deferred application fires here), the presenter and the grid */
	doc[196] = 0x02;
	doc[197] = 0x02;
	doc[198] = 0x02;
	doc[199] = 0x02;
	return 200;
}

/* A document whose own metadata table names an application page: the loader resolves it through the application's metadata
 * provider, not through a framework table. */
static size_t build_provider_document(unsigned char *doc)
{
	for (size_t index = 0; index < 243; ++index)
		doc[index] = 0;
	doc[0] = 'X'; doc[1] = 'B'; doc[2] = 'F';
	put32(doc, 4, 216);
	put32(doc, 8, 15);
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

/* A document that declares a ControlTemplate through a property node: a Button (stable 540) carries Content (stable 999,
 * a TextBlock "Hi", stable 396 with Text 800) and a "Template" property (the document's own property table) whose value
 * is a ControlTemplate (the document's own type table — the stable type table does not carry the class) whose root is a
 * Grid (stable 442) holding one ContentPresenter (the document's type table again). The loader defers the template's
 * application to the template scope's pop, so the button stores the template with its root and presenter attached. */
static size_t build_template_document(unsigned char *doc)
{
	size_t at;
	for (size_t index = 0; index < 400; ++index)
		doc[index] = 0;
	doc[0] = 'X'; doc[1] = 'B'; doc[2] = 'F';
	put32(doc, 4, 332);
	put32(doc, 8, 52);
	put32(doc, 12, 2);
	put32(doc, 16, 1);
	put64(doc, 20, 120);
	put64(doc, 28, 266);
	put64(doc, 36, 270);
	put64(doc, 44, 282);
	put64(doc, 52, 310);
	put64(doc, 60, 328);
	put32(doc, 132, 4);
	at = put_text(doc, 136, "Windows.UI.Xaml.Controls");
	at = put_text(doc, at, "ControlTemplate");
	at = put_text(doc, at, "ContentPresenter");
	at = put_text(doc, at, "Template");
	put32(doc, 278, 0);
	put32(doc, 282, 1);
	put32(doc, 286, 0);
	put32(doc, 290, 0);
	put32(doc, 294, 2);
	put32(doc, 298, 0);
	put32(doc, 302, 0);
	put32(doc, 306, 1);
	put32(doc, 310, 0);
	put32(doc, 314, 0);
	put32(doc, 318, 2);
	put32(doc, 322, 1);
	put32(doc, 326, 0);
	put32(doc, 330, 0);
	put32(doc, 334, 3);
	put32(doc, 340, 0);
	put32(doc, 344, 1);
	put32(doc, 348, 0);
	put32(doc, 352, 40);
	doc[356] = 0x14;
	doc[357] = (unsigned char)(540 & 0xff);
	doc[358] = (unsigned char)((540 >> 8) | 0x80);
	doc[359] = 0x07;
	doc[360] = (unsigned char)(999 & 0xff);
	doc[361] = (unsigned char)((999 >> 8) | 0x80);
	doc[362] = 0x14;
	doc[363] = (unsigned char)(396 & 0xff);
	doc[364] = (unsigned char)((396 >> 8) | 0x80);
	doc[365] = 0x1a;
	doc[366] = (unsigned char)(800 & 0xff);
	doc[367] = (unsigned char)((800 >> 8) | 0x80);
	doc[368] = 0x09;
	put32(doc, 369, 2);
	doc[373] = 'H'; doc[375] = 'i';
	doc[377] = 0x02;
	doc[378] = 0x07;
	doc[379] = 0x00;
	doc[380] = 0x00;
	doc[381] = 0x14;
	doc[382] = 0x00;
	doc[383] = 0x00;
	doc[384] = 0x14;
	doc[385] = (unsigned char)(442 & 0xff);
	doc[386] = (unsigned char)((442 >> 8) | 0x80);
	doc[387] = 0x07;
	doc[388] = (unsigned char)(648 & 0xff);
	doc[389] = (unsigned char)((648 >> 8) | 0x80);
	doc[390] = 0x14;
	doc[391] = 0x01;
	doc[392] = 0x00;
	doc[393] = 0x02;
	doc[394] = 0x02;
	doc[395] = 0x02;
	return 396;
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
	IInspectable *inner;
};
static ULONG fixture_inner_remaining = (ULONG)-1;

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
	struct fixture_provider *self = (struct fixture_provider *)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!IsEqualGUID(iid, &IID_IUnknown) && !IsEqualGUID(iid, &IID_IInspectable) &&
		!IsEqualGUID(iid, &IID__x_ABI_CWindows_CUI_CXaml_CMarkup_CIXamlMetadataProvider)) {
		if (self->inner)
			return self->inner->lpVtbl->QueryInterface(self->inner, iid, out);
		return E_NOINTERFACE;
	}
	InterlockedIncrement(&self->refs);
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
		if (self->inner)
			fixture_inner_remaining = self->inner->lpVtbl->Release(self->inner);
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
	/* Malformed XBF must fail without returning a partial tree. */
	unsigned char bad_fixture[256];
	IInspectable *bad_root = NULL;
	build_fixture(bad_fixture, 509);
	put32(bad_fixture, 164, 2); /* incomplete CreateType reference */
	if (load(bad_fixture, 172, &bad_root) != E_INVALIDARG || bad_root) return 700;
	build_fixture(bad_fixture, 509);
	bad_fixture[168] = 13; bad_fixture[169] = 9; bad_fixture[170] = 1; bad_fixture[171] = 0;
	if (load(bad_fixture, 172, &bad_root) != E_INVALIDARG || bad_root) return 701;
	build_fixture(bad_fixture, 509);
	bad_fixture[168] = 17; bad_fixture[169] = 0; bad_fixture[170] = 0; bad_fixture[171] = 0x80;
	if (load(bad_fixture, 172, &bad_root) != E_INVALIDARG || bad_root) return 702;
	build_fixture(bad_fixture, 509);
	put64(bad_fixture, 20, UINT64_MAX); /* metadata offset wraps on 32- and 64-bit hosts */
	if (load(bad_fixture, 172, &bad_root) != E_INVALIDARG || bad_root) return 703;
	build_fixture(bad_fixture, 509);
	put32(bad_fixture, 8, 77); put32(bad_fixture, 164, 65);
	for (size_t index = 0; index < 65; ++index) bad_fixture[168 + index] = 1;
	if (load(bad_fixture, 233, &bad_root) != E_INVALIDARG || bad_root) return 704;
	build_fixture(bad_fixture, 509);
	put32(bad_fixture, 8, 24); put32(bad_fixture, 156, 2);
	put32(bad_fixture, 160, 0); put32(bad_fixture, 164, 4);
	put32(bad_fixture, 168, 4); put32(bad_fixture, 172, 100);
	bad_fixture[176] = 20; bad_fixture[177] = 0xfd; bad_fixture[178] = 0x81; bad_fixture[179] = 2;
	if (load(bad_fixture, 180, &bad_root) != E_INVALIDARG || bad_root) return 705;
	build_fixture(bad_fixture, 509);
	put32(bad_fixture, 8, 12); /* bytes beyond the declared node section are not node data */
	if (load(bad_fixture, 172, &bad_root) != E_INVALIDARG || bad_root) return 706;
	build_fixture(bad_fixture, 509);
	put32(bad_fixture, 8, 20); put32(bad_fixture, 164, 8);
	bad_fixture[168] = 17; bad_fixture[169] = 0; bad_fixture[170] = 0;
	for (size_t index = 171; index < 176; ++index) bad_fixture[index] = 0xff;
	if (load(bad_fixture, 176, &bad_root) != E_INVALIDARG || bad_root) return 707;
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

	/* ControlTemplate through the engine-internal factory (the runtime catalog does not list the class yet): the
	 * TargetType round-trips the interop TypeName over the real IControlTemplate, put_Template assigns through the real
	 * IControl, get_Template round-trips, and the content precedence is the documented one — the template root replaces
	 * the control's rendered visuals while get_Content keeps answering the stored Content, because the engine's control
	 * has no chrome ContentPresenter to host it (the pixel proof of the replace lives in the style fixture, which hosts
	 * the templated control on a rendered grid). A value that is no engine ControlTemplate answers E_INVALIDARG and the
	 * previous template stays; put_Template(NULL) clears. */
	{
		typedef HRESULT (*template_factory_function)(IActivationFactory **);
		typedef HRESULT (*template_root_function)(IInspectable *, IInspectable *);
		typedef HRESULT (*put_template_function)(IInspectable *, IInspectable *);
		typedef HRESULT (*get_template_function)(IInspectable *, IInspectable **);
		static const WCHAR template_class[] = L"Windows.UI.Xaml.Controls.ControlTemplate";
		static const WCHAR button_name[] = L"Windows.UI.Xaml.Controls.Button";
		template_factory_function get_template_factory = NULL;
		template_root_function put_template_root = NULL;
		put_template_function element_put_template = NULL;
		get_template_function element_get_template = NULL;
		HMODULE engine = LoadLibraryW(L"raven-xaml.dll");
		IActivationFactory *control_template_factory = NULL;
		IInspectable *template_object = NULL, *template_root_child = NULL, *button_object = NULL;
		IActivationFactory *button_factory = NULL, *child_factory = NULL;
		HSTRING button_key = NULL;
		content_control1 *button_content = NULL;
		void *template_iface = NULL, *control_iface = NULL;
		struct fixture_template_vtbl *template_vtbl = NULL;
		struct fixture_control_vtbl *control = NULL;
		IInspectable *read = NULL;
		IUnknown *one = NULL, *other = NULL;
		HSTRING read_name = NULL;
		xaml_type_name put_type, got_type;
		if (!engine) return 700;
		get_template_factory = (template_factory_function)GetProcAddress(engine, "raven_xaml_control_template_factory");
		put_template_root = (template_root_function)GetProcAddress(engine, "raven_xaml_control_template_put_root");
		element_put_template = (put_template_function)GetProcAddress(engine, "raven_xaml_element_put_template");
		element_get_template = (get_template_function)GetProcAddress(engine, "raven_xaml_element_get_template");
		if (!get_template_factory || !put_template_root || !element_put_template || !element_get_template) return 701;
		if (get_template_factory(&control_template_factory) != S_OK || !control_template_factory) return 702;
		if (control_template_factory->lpVtbl->ActivateInstance(control_template_factory, &template_object) != S_OK ||
			!template_object) return 703;
		if (template_object->lpVtbl->QueryInterface(template_object,
			&IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplate, &template_iface) != S_OK || !template_iface)
			return 704;
		template_vtbl = *(struct fixture_template_vtbl **)template_iface;
		if (WindowsCreateString(button_name, ARRAYSIZE(button_name) - 1, &put_type.Name) != S_OK) return 705;
		put_type.Kind = TypeKind_Metadata;
		if (template_vtbl->put_TargetType(template_iface, (uint64_t)(uintptr_t)&put_type) != S_OK) return 706;
		got_type.Name = NULL;
		got_type.Kind = TypeKind_Primitive;
		if (template_vtbl->get_TargetType(template_iface, (uint64_t)(uintptr_t)&got_type) != S_OK || !got_type.Name)
			return 707;
		if (!same(got_type.Name, button_name, ARRAYSIZE(button_name) - 1) || got_type.Kind != TypeKind_Metadata) {
			WindowsDeleteString(got_type.Name);
			return 708;
		}
		WindowsDeleteString(got_type.Name);
		WindowsDeleteString(put_type.Name);
		/* the template content: the engine-internal attach stores the root tree the real interface has no setter for */
		if (WindowsCreateString(L"Windows.UI.Xaml.Controls.TextBlock", 34, &textblock_key) != S_OK) return 709;
		if (RoGetActivationFactory(textblock_key, &IID_IActivationFactory, (void **)&child_factory) != S_OK ||
			!child_factory) return 710;
		if (child_factory->lpVtbl->ActivateInstance(child_factory, &template_root_child) != S_OK ||
			!template_root_child) return 711;
		if (put_template_root(template_object, template_root_child) != S_OK) return 712;
		/* the button: content child first, then the template assignment through the real IControl */
		if (WindowsCreateString(button_name, ARRAYSIZE(button_name) - 1, &button_key) != S_OK) return 713;
		if (RoGetActivationFactory(button_key, &IID_IActivationFactory, (void **)&button_factory) != S_OK ||
			!button_factory) return 713;
		WindowsDeleteString(button_key);
		if (button_factory->lpVtbl->ActivateInstance(button_factory, &button_object) != S_OK || !button_object)
			return 714;
		if (button_object->lpVtbl->QueryInterface(button_object, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
			(void **)&button_content) != S_OK || !button_content) return 715;
		if (button_content->lpVtbl->put_Content(button_content, (uint64_t)(uintptr_t)template_root_child) != S_OK)
			return 716;
		if (button_object->lpVtbl->QueryInterface(button_object, &IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControl,
			&control_iface) != S_OK || !control_iface) return 717;
		control = *(struct fixture_control_vtbl **)control_iface;
		if (control->put_Template(control_iface, (uint64_t)(uintptr_t)template_object) != S_OK) return 718;
		if (control->get_Template(control_iface, (uint64_t)(uintptr_t)&read) != S_OK || !read) return 719;
		if (read->lpVtbl->QueryInterface(read, &IID_IUnknown, (void **)&one) != S_OK || !one) return 720;
		if (template_object->lpVtbl->QueryInterface(template_object, &IID_IUnknown, (void **)&other) != S_OK || !other)
			return 721;
		if (one != other) return 722;
		other->lpVtbl->Release(other);
		other = NULL;
		one->lpVtbl->Release(one);
		one = NULL;
		read->lpVtbl->Release(read);
		read = NULL;
		/* the content precedence: get_Content still answers the stored Content while the template root is the visual child */
		if (button_content->lpVtbl->get_Content(button_content, (uint64_t)(uintptr_t)&read) != S_OK || !read) return 723;
		if (read->lpVtbl->QueryInterface(read, &IID_IUnknown, (void **)&one) != S_OK || !one) return 724;
		if (template_root_child->lpVtbl->QueryInterface(template_root_child, &IID_IUnknown, (void **)&other) != S_OK ||
			!other) return 725;
		if (one != other) return 726;
		other->lpVtbl->Release(other);
		other = NULL;
		one->lpVtbl->Release(one);
		one = NULL;
		read->lpVtbl->Release(read);
		read = NULL;
		/* the wrong-kind value: a TextBlock where a ControlTemplate is wanted answers E_INVALIDARG and the template stays */
		if (element_put_template(button_object, template_root_child) != E_INVALIDARG) return 727;
		if (element_get_template(button_object, &read) != S_OK || !read) return 728;
		if (read->lpVtbl->QueryInterface(read, &IID_IUnknown, (void **)&one) != S_OK || !one) return 729;
		if (template_object->lpVtbl->QueryInterface(template_object, &IID_IUnknown, (void **)&other) != S_OK || !other)
			return 730;
		if (one != other) return 731;
		other->lpVtbl->Release(other);
		other = NULL;
		one->lpVtbl->Release(one);
		one = NULL;
		read->lpVtbl->Release(read);
		read = NULL;
		/* the clear and the re-put: the control answers NULL after the NULL put and the template again after the re-put */
		if (control->put_Template(control_iface, 0) != S_OK) return 732;
		if (element_get_template(button_object, &read) != S_OK || read) return 733;
		if (control->put_Template(control_iface, (uint64_t)(uintptr_t)template_object) != S_OK) return 734;
		if (element_get_template(button_object, &read) != S_OK || !read) return 735;
		read->lpVtbl->Release(read);
		read = NULL;
		/* the runtime class name answers the ControlTemplate class */
		if (template_object->lpVtbl->GetRuntimeClassName(template_object, &read_name) != S_OK || !read_name) return 736;
		if (!same(read_name, template_class, ARRAYSIZE(template_class) - 1)) {
			WindowsDeleteString(read_name);
			return 737;
		}
		WindowsDeleteString(read_name);
		((IUnknown *)control_iface)->lpVtbl->Release((IUnknown *)control_iface);
		((IUnknown *)template_iface)->lpVtbl->Release((IUnknown *)template_iface);
		button_content->lpVtbl->Release(button_content);
		button_object->lpVtbl->Release(button_object);
		template_object->lpVtbl->Release(template_object);
		template_root_child->lpVtbl->Release(template_root_child);
		button_factory->lpVtbl->Release(button_factory);
		child_factory->lpVtbl->Release(child_factory);
		control_template_factory->lpVtbl->Release(control_template_factory);
		WindowsDeleteString(textblock_key);
		textblock_key = NULL;
	}

	/* XBF-declared templates: the loader path. The document declares the template under the button's "Template" property
	 * (the document's own tables — the stable type table does not carry ControlTemplate or ContentPresenter), the loader
	 * defers the application until the template's root is attached, and the button answers the applied template with its
	 * root grid and the presenter inside it. The presenter resolves the button's Content through the same hook the render
	 * walk reads, so the compiled document's template shows the control's content. */
	{
		typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIButton button1;
		typedef HRESULT (*template_root_read_function)(IInspectable *, IInspectable **);
		typedef HRESULT (*template_child_function)(IInspectable *, IInspectable **);
		typedef HRESULT (*get_template_function2)(IInspectable *, IInspectable **);
		template_root_read_function read_template_root = NULL;
		template_child_function template_child = NULL;
		get_template_function2 element_get_template2 = NULL;
		HMODULE engine = LoadLibraryW(L"raven-xaml.dll");
		unsigned char template_fixture[400];
		IInspectable *loaded_button = NULL, *read_template = NULL, *root_grid = NULL, *presenter = NULL, *content_child = NULL;
		button1 *loaded_button_iface = NULL;
		void *loaded_template_iface = NULL;
		panel1 *root_panel = NULL;
		content_control1 *loaded_content = NULL;
		textblock1 *content_text = NULL;
		void *children_object = NULL;
		struct fixture_vector *children = NULL;
		HSTRING presenter_name = NULL, content_text_value = NULL;
		UINT32 child_count = 0;
		static const WCHAR presenter_class[] = L"Windows.UI.Xaml.Controls.ContentPresenter";
		if (!engine) return 740;
		read_template_root = (template_root_read_function)GetProcAddress(engine, "raven_xaml_control_template_root");
		template_child = (template_child_function)GetProcAddress(engine, "raven_xaml_element_template_child");
		element_get_template2 = (get_template_function2)GetProcAddress(engine, "raven_xaml_element_get_template");
		if (!read_template_root || !template_child || !element_get_template2) return 741;
		if (load(template_fixture, (uint32_t)build_template_document(template_fixture), &loaded_button) != S_OK ||
			!loaded_button) return 742;
		if (loaded_button->lpVtbl->QueryInterface(loaded_button, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton,
			(void **)&loaded_button_iface) != S_OK || !loaded_button_iface) return 743;
		loaded_button_iface->lpVtbl->Release(loaded_button_iface);
		if (element_get_template2(loaded_button, &read_template) != S_OK || !read_template) return 744;
		if (read_template->lpVtbl->QueryInterface(read_template,
			&IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplate, &loaded_template_iface) != S_OK ||
			!loaded_template_iface) return 745;
		((IUnknown *)loaded_template_iface)->lpVtbl->Release((IUnknown *)loaded_template_iface);
		/* the template root: the grid the document declared inside the template */
		if (read_template_root(read_template, &root_grid) != S_OK || !root_grid) return 746;
		if (root_grid->lpVtbl->QueryInterface(root_grid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
			(void **)&root_panel) != S_OK || !root_panel) return 747;
		if (root_panel->lpVtbl->get_Children(root_panel, (uint64_t)(uintptr_t)&children_object) != S_OK || !children_object)
			return 748;
		children = (struct fixture_vector *)children_object;
		if (children->lpVtbl->get_Size(children, &child_count) != S_OK || child_count != 1) return 749;
		if (children->lpVtbl->GetAt(children, 0, &presenter) != S_OK || !presenter) return 750;
		if (presenter->lpVtbl->GetRuntimeClassName(presenter, &presenter_name) != S_OK || !presenter_name) return 751;
		if (!same(presenter_name, presenter_class, ARRAYSIZE(presenter_class) - 1)) {
			WindowsDeleteString(presenter_name);
			return 752;
		}
		WindowsDeleteString(presenter_name);
		children->lpVtbl->Release(children);
		root_panel->lpVtbl->Release(root_panel);
		root_grid->lpVtbl->Release(root_grid);
		read_template->lpVtbl->Release(read_template);
		read_template = NULL;
		/* the presenter resolves the button's Content through the render walk's own hook */
		if (template_child(presenter, &content_child) != S_OK || !content_child) return 753;
		if (content_child->lpVtbl->QueryInterface(content_child, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
			(void **)&content_text) != S_OK || !content_text) return 754;
		if (content_text->lpVtbl->get_Text(content_text, (uint64_t)(uintptr_t)&content_text_value) != S_OK || !content_text_value ||
			!same(content_text_value, L"Hi", 2)) return 755;
		WindowsDeleteString(content_text_value);
		content_text->lpVtbl->Release(content_text);
		content_child->lpVtbl->Release(content_child);
		content_child = NULL;
		/* the grid root is neither a button nor a presenter: the hook answers S_FALSE for it */
		if (template_child(root_grid, &content_child) != S_FALSE) return 756;
		/* the button's stored content still answers logically while the template is applied */
		if (loaded_button->lpVtbl->QueryInterface(loaded_button, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
			(void **)&loaded_content) != S_OK || !loaded_content) return 757;
		if (loaded_content->lpVtbl->get_Content(loaded_content, (uint64_t)(uintptr_t)&content_child) != S_OK || !content_child)
			return 758;
		loaded_content->lpVtbl->Release(loaded_content);
		content_child->lpVtbl->Release(content_child);
		content_child = NULL;
		presenter->lpVtbl->Release(presenter);
		loaded_button->lpVtbl->Release(loaded_button);
	}

	/* Stable-index text properties: a TextBlock built entirely through the stable table — the type (396) and the
	 * TextElement.FontSize (251) and TextElement.Foreground (255) members a real document carries on its text
	 * elements. The font size readback proves the float dispatch applied the stable-named "FontSize"; the text
	 * readback proves the document came through; the Foreground colour lands in the engine's text storage the render
	 * walk reads (the same path the registered TextBlock.Foreground 788 uses — the colour has no public readback). */
	{
		unsigned char text_fixture[206];
		IInspectable *text_root = NULL;
		textblock1 *fixture_textblock = NULL;
		HSTRING fixture_text_value = NULL;
		float fixture_font_size = 0.0f;
		HRESULT text_doc_status = load(text_fixture, (uint32_t)build_text_document(text_fixture), &text_root);
		if (text_doc_status != S_OK || !text_root)
			return 4000 + (int)(text_doc_status & 0xfff);
		if (text_root->lpVtbl->QueryInterface(text_root, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
			(void **)&fixture_textblock) != S_OK || !fixture_textblock) return 4001;
		if (fixture_textblock->lpVtbl->get_FontSize(fixture_textblock, (uint64_t)(uintptr_t)&fixture_font_size) != S_OK)
			return 4002;
		if (fixture_font_size != 21.5f) return 4003;
		if (fixture_textblock->lpVtbl->get_Text(fixture_textblock, (uint64_t)(uintptr_t)&fixture_text_value) != S_OK ||
			!fixture_text_value || !same(fixture_text_value, L"Hi", 2)) return 4004;
		WindowsDeleteString(fixture_text_value);
		fixture_textblock->lpVtbl->Release(fixture_textblock);
		text_root->lpVtbl->Release(text_root);
	}

	/* Stable-index content template: the presenter inside the grid carries ContentControl.ContentTemplate (833) naming
	 * a ControlTemplate (191) whose root TextBlock the presenter store accepted at the template scope's pop. The
	 * readback seam proves the stored template's identity and root, and a non-presenter element answers the skip
	 * convention the registration comment states. */
	{
		typedef HRESULT (*get_content_template_function)(IInspectable *, IInspectable **);
		typedef HRESULT (*template_root_read_function2)(IInspectable *, IInspectable **);
		get_content_template_function get_content_template = NULL;
		template_root_read_function2 read_template_root2 = NULL;
		HMODULE engine = LoadLibraryW(L"raven-xaml.dll");
		unsigned char presenter_fixture[200];
		IInspectable *presenter_root = NULL, *stored_template = NULL, *stored_root_child = NULL, *grid_object2 = NULL;
		panel1 *presenter_panel = NULL;
		void *presenter_children = NULL;
		struct fixture_vector *presenter_vector = NULL;
		IInspectable *presenter_object = NULL;
		HSTRING presenter_class_name = NULL;
		textblock1 *stored_textblock = NULL;
		UINT32 presenter_child_count = 0;
		static const WCHAR presenter_class2[] = L"Windows.UI.Xaml.Controls.ContentPresenter";
		if (!engine) return 4100;
		get_content_template = (get_content_template_function)GetProcAddress(engine, "raven_xaml_element_get_content_template");
		read_template_root2 = (template_root_read_function2)GetProcAddress(engine, "raven_xaml_control_template_root");
		if (!get_content_template || !read_template_root2) return 4101;
		HRESULT presenter_doc_status = load(presenter_fixture, (uint32_t)build_content_template_document(presenter_fixture),
			&presenter_root);
		if (presenter_doc_status != S_OK || !presenter_root)
			return 4102 + (int)(presenter_doc_status & 0xfff);
		/* the root is itself the grid (no page wraps it) and the presenter is its single child */
		if (presenter_root->lpVtbl->QueryInterface(presenter_root, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
			(void **)&presenter_panel) != S_OK || !presenter_panel) return 4103;
		grid_object2 = presenter_root;
		grid_object2->lpVtbl->AddRef(grid_object2);
		if (presenter_panel->lpVtbl->get_Children(presenter_panel, (uint64_t)(uintptr_t)&presenter_children) != S_OK ||
			!presenter_children) return 4106;
		presenter_vector = (struct fixture_vector *)presenter_children;
		if (presenter_vector->lpVtbl->get_Size(presenter_vector, &presenter_child_count) != S_OK ||
			presenter_child_count != 1) return 4107;
		if (presenter_vector->lpVtbl->GetAt(presenter_vector, 0, &presenter_object) != S_OK || !presenter_object)
			return 4108;
		if (presenter_object->lpVtbl->GetRuntimeClassName(presenter_object, &presenter_class_name) != S_OK ||
			!presenter_class_name) return 4109;
		if (!same(presenter_class_name, presenter_class2, ARRAYSIZE(presenter_class2) - 1)) {
			WindowsDeleteString(presenter_class_name);
			return 4110;
		}
		WindowsDeleteString(presenter_class_name);
		/* the stored template: identity and root the deferred application left on the presenter */
		if (get_content_template(presenter_object, &stored_template) != S_OK || !stored_template) return 4111;
		{
			void *template_iface2 = NULL;
			if (stored_template->lpVtbl->QueryInterface(stored_template,
				&IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplate, &template_iface2) != S_OK ||
				!template_iface2) return 4112;
			((IUnknown *)template_iface2)->lpVtbl->Release((IUnknown *)template_iface2);
		}
		if (read_template_root2(stored_template, &stored_root_child) != S_OK || !stored_root_child) return 4113;
		if (stored_root_child->lpVtbl->QueryInterface(stored_root_child,
			&IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock, (void **)&stored_textblock) != S_OK ||
			!stored_textblock) return 4114;
		{
			HSTRING stored_text_value = NULL;
			if (stored_textblock->lpVtbl->get_Text(stored_textblock, (uint64_t)(uintptr_t)&stored_text_value) != S_OK ||
				!stored_text_value || !same(stored_text_value, L"T", 1)) return 4115;
			WindowsDeleteString(stored_text_value);
		}
		stored_textblock->lpVtbl->Release(stored_textblock);
		stored_root_child->lpVtbl->Release(stored_root_child);
		stored_template->lpVtbl->Release(stored_template);
		/* the skip convention: a grid is not a presenter, so the store answers S_FALSE for it */
		if (get_content_template(grid_object2, &stored_template) != S_FALSE) return 4116;
		presenter_vector->lpVtbl->Release(presenter_vector);
		presenter_panel->lpVtbl->Release(presenter_panel);
		grid_object2->lpVtbl->Release(grid_object2);
		presenter_object->lpVtbl->Release(presenter_object);
		presenter_root->lpVtbl->Release(presenter_root);
	}

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
	IInspectable *invalid_instance = (IInspectable *)1;
	if (app_factory->lpVtbl->CreateInstance(app_factory, (uint64_t)(uintptr_t)provider, 0,
		(uint64_t)(uintptr_t)&invalid_instance) != E_POINTER || invalid_instance) return 691;
	if (app_factory->lpVtbl->CreateInstance(app_factory, (uint64_t)(uintptr_t)provider,
		(uint64_t)(uintptr_t)&app_inner, (uint64_t)(uintptr_t)&app_instance) != S_OK || !app_instance || !app_inner) return 677;
	provider->inner = app_inner;
	IUnknown *inner_identity = NULL;
	if (((IInspectable *)app_inner)->lpVtbl->QueryInterface((IInspectable *)app_inner, &IID_IUnknown,
		(void **)&inner_identity) != S_OK || (void *)inner_identity != app_inner || (void *)inner_identity == (void *)provider) return 690;
	inner_identity->lpVtbl->Release(inner_identity);
	IInspectable *inner_application = NULL;
	if (((IInspectable *)app_inner)->lpVtbl->QueryInterface((IInspectable *)app_inner,
		&IID__x_ABI_CWindows_CUI_CXaml_CIApplication, (void **)&inner_application) != S_OK || !inner_application) return 686;
	IUnknown *identity = NULL;
	if (inner_application->lpVtbl->QueryInterface(inner_application, &IID_IUnknown, (void **)&identity) != S_OK ||
		(void *)identity != (void *)provider) return 687;
	identity->lpVtbl->Release(identity);
	inner_application->lpVtbl->Release(inner_application);
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
	/* Replacing Current releases the component's reference to the outer. Its last release must also free its inner. */
	IInspectable *replacement = NULL;
	if (app_activation->lpVtbl->ActivateInstance(app_activation, &replacement) != S_OK || !replacement) return 688;
	replacement->lpVtbl->Release(replacement);
	if (provider->lpVtbl->Release((markup_provider *)provider) != 0 || fixture_inner_remaining != 0) return 689;
	app_factory->lpVtbl->Release(app_factory);
	app_activation->lpVtbl->Release(app_activation);
	WindowsDeleteString(app_key);
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
