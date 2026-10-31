#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include "windows_ui_xaml_application.h"
#include "windows_ui_xaml_controls.h"
#include "windows_ui_xaml_elements.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);
typedef HRESULT (WINAPI *load_function)(const void *, uint32_t, IInspectable **);
HRESULT WINAPI GetRestrictedErrorInfo(void **info);

/* The real IControlTemplate IID (Windows.UI.Xaml.winmd, the WDK IDL): the engine's ControlTemplate answers it, and no
 * generated header here declares it, so the fixture carries it the way the controls fixture does. */
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplate, 0xefd2418e, 0x41e0, 0x48bb, 0x8b, 0x82, 0x91,
	0xed, 0xa1, 0xba, 0x3f, 0xe2);

typedef __x_ABI_CWindows_CUI_CXaml_CIApplication application1;
typedef __x_ABI_CWindows_CUI_CXaml_CIApplication2 application2;
typedef __x_ABI_CWindows_CUI_CXaml_CIApplication3 application3;
typedef __x_ABI_CWindows_CUI_CXaml_CIApplicationStatics application_statics;
typedef __x_ABI_CWindows_CUI_CXaml_CIApplicationFactory application_factory;

/* The initialization callback the runtime passes to Application.Start: a WinRT delegate is an IUnknown with one Invoke slot. */
struct callback;

struct callback_vtbl {
	HRESULT (WINAPI *query)(struct callback *self, REFIID iid, void **out);
	ULONG (WINAPI *addref)(struct callback *self);
	ULONG (WINAPI *release)(struct callback *self);
	HRESULT (WINAPI *invoke)(struct callback *self, void *parameters);
};

struct callback {
	const struct callback_vtbl *lpVtbl;
	LONG refs;
	LONG invoked;
	LONG parameters_ok;
};

static HRESULT WINAPI callback_query(struct callback *self, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown)) return E_NOINTERFACE;
	*out = self;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI callback_addref(struct callback *self) { return (ULONG)InterlockedIncrement(&self->refs); }
static ULONG WINAPI callback_release(struct callback *self) { return (ULONG)InterlockedDecrement(&self->refs); }

static HRESULT WINAPI callback_invoke(struct callback *self, void *parameters)
{
	IInspectable *params = (IInspectable *)parameters;
	IInspectable *queried = NULL;
	if (!params) return E_POINTER;
	if (params->lpVtbl->QueryInterface(params, &IID__x_ABI_CWindows_CUI_CXaml_CIApplicationInitializationCallbackParams,
		(void **)&queried) != S_OK || !queried) return E_NOINTERFACE;
	queried->lpVtbl->Release(queried);
	InterlockedIncrement(&self->parameters_ok);
	InterlockedIncrement(&self->invoked);
	return S_OK;
}

static const struct callback_vtbl callback_table = {callback_query, callback_addref, callback_release, callback_invoke};

static int same(HSTRING value, const WCHAR *text, UINT32 expected)
{
	UINT32 length = 0;
	const WCHAR *buffer = WindowsGetStringRawBuffer(value, &length);
	return buffer && length == expected && CompareStringOrdinal(buffer, (int)length, text, -1, FALSE) == CSTR_EQUAL;
}

/* The IVector<UIElement> the panel's Children returns: the parameterized ABI has no mingw header, so the two slots the
 * fixture reads are declared locally in metadata order after the six IInspectable methods, the way the controls fixture
 * declares the full vector. */
struct fixture_vector_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(void *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(void *This);
	ULONG (STDMETHODCALLTYPE *Release)(void *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(void *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(void *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(void *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetAt)(void *This, UINT32 index, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(void *This, UINT32 *value);
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

/* A real-shaped XBF v2.1 document: the same template declaration a compiled app page carries, but every framework type
 * and property rides the stable table — Button 540, ContentControl.Content 999, TextBlock 396 with TextBlock.Text 800,
 * Control.Template 560, ControlTemplate 191, Grid 442, Panel.Children 648 and ContentPresenter 296. The node stream
 * reads: create the button, its content text block, then the template whose root grid holds one presenter, closing the
 * three scopes; the loader defers the template's application until its root is attached. The metadata section is the
 * same empty one the minimal fixture uses, since nothing here resolves through the document's own tables. */
static size_t build_stable_template_document(unsigned char *doc)
{
	for (size_t index = 0; index < 208; ++index)
		doc[index] = 0;
	doc[0] = 'X'; doc[1] = 'B'; doc[2] = 'F';
	put32(doc, 4, 144);
	put32(doc, 8, 52);
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
	put32(doc, 164, 40);
	doc[168] = 0x14; /* create scoped: Button (stable 540) */
	doc[169] = 0x1c;
	doc[170] = 0x82;
	doc[171] = 0x07; /* property: ContentControl.Content (stable 999) */
	doc[172] = 0xe7;
	doc[173] = 0x83;
	doc[174] = 0x14; /* create scoped: TextBlock (stable 396) */
	doc[175] = 0x8c;
	doc[176] = 0x81;
	doc[177] = 0x1a; /* property value: TextBlock.Text (stable 800) */
	doc[178] = 0x20;
	doc[179] = 0x83;
	doc[180] = 0x09; /* string "Hi" */
	put32(doc, 181, 2);
	doc[185] = 'H'; doc[186] = 0;
	doc[187] = 'i'; doc[188] = 0;
	doc[189] = 0x02; /* pop the text block: the button stores its Content */
	doc[190] = 0x07; /* property: Control.Template (stable 560) */
	doc[191] = 0x30;
	doc[192] = 0x82;
	doc[193] = 0x14; /* create scoped: ControlTemplate (stable 191) — opens the template scope */
	doc[194] = 0xbf;
	doc[195] = 0x80;
	doc[196] = 0x14; /* create scoped: Grid (stable 442) — the template root */
	doc[197] = 0xba;
	doc[198] = 0x81;
	doc[199] = 0x07; /* property: Panel.Children (stable 648) */
	doc[200] = 0x88;
	doc[201] = 0x82;
	doc[202] = 0x14; /* create scoped: ContentPresenter (stable 296) */
	doc[203] = 0x28;
	doc[204] = 0x81;
	doc[205] = 0x02; /* pop the presenter */
	doc[206] = 0x02; /* pop the grid: the template root is attached */
	doc[207] = 0x02; /* pop the template: the loader applies it to the button */
	return 208;
}

/* The ContentTemplate round: the same stable-index shapes with ContentPresenter.ContentTemplate (stable 525) applied to
 * the presenter inside the button's template — under the presenter scope the node stream carries the property 525
 * reference and a second ControlTemplate whose root is one grid holding one presenter; the loader's template scope
 * defers THAT application to the presenter until its root attaches, and the presenter's engine-internal store keeps the
 * reference. The button's Content stays a TextBlock, which is no ContentControl, so the applied content template's inner
 * presenter stays unbound and answers nothing — the documented non-ContentControl answer. */
static size_t build_stable_content_template_document(unsigned char *doc)
{
	size_t at = 168;
	for (size_t index = 0; index < 214; ++index)
		doc[index] = 0;
	doc[0] = 'X'; doc[1] = 'B'; doc[2] = 'F';
	put32(doc, 4, 144);
	put32(doc, 8, 58);
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
	put32(doc, 164, 46);
	doc[at++] = 0x14; doc[at++] = 0x1c; doc[at++] = 0x82; /* create scoped: Button (stable 540) */
	doc[at++] = 0x07; doc[at++] = 0xe7; doc[at++] = 0x83; /* property: ContentControl.Content (stable 999) */
	doc[at++] = 0x14; doc[at++] = 0x8c; doc[at++] = 0x81; /* create scoped: TextBlock (stable 396) */
	doc[at++] = 0x02; /* pop the text block: the button stores its Content */
	doc[at++] = 0x07; doc[at++] = 0x30; doc[at++] = 0x82; /* property: Control.Template (stable 560) */
	doc[at++] = 0x14; doc[at++] = 0xbf; doc[at++] = 0x80; /* create scoped: ControlTemplate (stable 191) */
	doc[at++] = 0x14; doc[at++] = 0xba; doc[at++] = 0x81; /* create scoped: Grid (stable 442) — the template root */
	doc[at++] = 0x07; doc[at++] = 0x88; doc[at++] = 0x82; /* property: Panel.Children (stable 648) */
	doc[at++] = 0x14; doc[at++] = 0x28; doc[at++] = 0x81; /* create scoped: ContentPresenter (stable 296) */
	doc[at++] = 0x07; doc[at++] = 0x0d; doc[at++] = 0x82; /* property: ContentPresenter.ContentTemplate (stable 525) */
	doc[at++] = 0x14; doc[at++] = 0xbf; doc[at++] = 0x80; /* create scoped: ControlTemplate (stable 191) — the content template */
	doc[at++] = 0x14; doc[at++] = 0xba; doc[at++] = 0x81; /* create scoped: Grid (stable 442) — its root */
	doc[at++] = 0x07; doc[at++] = 0x88; doc[at++] = 0x82; /* property: Panel.Children (stable 648) */
	doc[at++] = 0x14; doc[at++] = 0x28; doc[at++] = 0x81; /* create scoped: ContentPresenter (stable 296) — the inner one */
	doc[at++] = 0x02; /* pop the inner presenter */
	doc[at++] = 0x02; /* pop the content-template root grid: the root is attached */
	doc[at++] = 0x02; /* pop the content template: the loader stores it on the presenter */
	doc[at++] = 0x02; /* pop the presenter */
	doc[at++] = 0x02; /* pop the outer template root grid */
	doc[at++] = 0x02; /* pop the outer template: the loader applies it to the button */
	return at;
}

/* The flow-through round: the content template's inner presenter bound to the CONTENT object. The button's Content is
 * itself a Button (stable 540) whose Content is the "Hi" text block, so the inner presenter — bound to the content
 * button, not the templated control — resolves the content button's Content through the same hook. A bound-to-the-
 * templated-control presenter would answer the inner button here; the text proves the binding answered the content. */
static size_t build_stable_flow_document(unsigned char *doc)
{
	size_t at = 168;
	for (size_t index = 0; index < 233; ++index)
		doc[index] = 0;
	doc[0] = 'X'; doc[1] = 'B'; doc[2] = 'F';
	put32(doc, 4, 144);
	put32(doc, 8, 77);
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
	put32(doc, 164, 65);
	doc[at++] = 0x14; doc[at++] = 0x1c; doc[at++] = 0x82; /* create scoped: Button (stable 540) */
	doc[at++] = 0x07; doc[at++] = 0xe7; doc[at++] = 0x83; /* property: ContentControl.Content (stable 999) */
	doc[at++] = 0x14; doc[at++] = 0x1c; doc[at++] = 0x82; /* create scoped: Button (stable 540) — the content object */
	doc[at++] = 0x07; doc[at++] = 0xe7; doc[at++] = 0x83; /* property: ContentControl.Content (stable 999) */
	doc[at++] = 0x14; doc[at++] = 0x8c; doc[at++] = 0x81; /* create scoped: TextBlock (stable 396) */
	doc[at++] = 0x1a; doc[at++] = 0x20; doc[at++] = 0x83; /* property value: TextBlock.Text (stable 800) */
	doc[at++] = 0x09; /* string "Hi" */
	put32(doc, at, 2);
	at += 4;
	doc[at++] = 'H'; doc[at++] = 0;
	doc[at++] = 'i'; doc[at++] = 0;
	doc[at++] = 0x02; /* pop the text block: the content button stores its Content */
	doc[at++] = 0x02; /* pop the content button: the outer button stores its Content */
	doc[at++] = 0x07; doc[at++] = 0x30; doc[at++] = 0x82; /* property: Control.Template (stable 560) */
	doc[at++] = 0x14; doc[at++] = 0xbf; doc[at++] = 0x80; /* create scoped: ControlTemplate (stable 191) */
	doc[at++] = 0x14; doc[at++] = 0xba; doc[at++] = 0x81; /* create scoped: Grid (stable 442) — the template root */
	doc[at++] = 0x07; doc[at++] = 0x88; doc[at++] = 0x82; /* property: Panel.Children (stable 648) */
	doc[at++] = 0x14; doc[at++] = 0x28; doc[at++] = 0x81; /* create scoped: ContentPresenter (stable 296) */
	doc[at++] = 0x07; doc[at++] = 0x0d; doc[at++] = 0x82; /* property: ContentPresenter.ContentTemplate (stable 525) */
	doc[at++] = 0x14; doc[at++] = 0xbf; doc[at++] = 0x80; /* create scoped: ControlTemplate (stable 191) — the content template */
	doc[at++] = 0x14; doc[at++] = 0xba; doc[at++] = 0x81; /* create scoped: Grid (stable 442) — its root */
	doc[at++] = 0x07; doc[at++] = 0x88; doc[at++] = 0x82; /* property: Panel.Children (stable 648) */
	doc[at++] = 0x14; doc[at++] = 0x28; doc[at++] = 0x81; /* create scoped: ContentPresenter (stable 296) — the inner one */
	doc[at++] = 0x02; /* pop the inner presenter */
	doc[at++] = 0x02; /* pop the content-template root grid */
	doc[at++] = 0x02; /* pop the content template: the loader stores it on the presenter */
	doc[at++] = 0x02; /* pop the presenter */
	doc[at++] = 0x02; /* pop the outer template root grid */
	doc[at++] = 0x02; /* pop the outer template: the loader applies it to the button */
	return at;
}

/* The Application class of the Windows Runtime: activation, the factory and statics, a created object with the three application
 * interfaces, Application.Start giving its callback initialization parameters, and Application.Current returning the application
 * the runtime created. */
int raven_entry(void)
{
	static const WCHAR name[] = L"Windows.UI.Xaml.Application";
	static const WCHAR missing[] = L"Windows.Raven.Missing";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function RoGetActivationFactory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	if (!RoInitialize || !RoGetActivationFactory) return 20;
	if (FAILED(RoInitialize(1))) return 1;
	HSTRING class_key = NULL;
	if (WindowsCreateString(name, ARRAYSIZE(name) - 1, &class_key) != S_OK || !class_key) return 21;
	IActivationFactory *activation = NULL;
	if (RoGetActivationFactory(class_key, &IID_IActivationFactory, (void **)&activation) != S_OK || !activation) return 2;
	application_statics *statics = NULL;
	if (activation->lpVtbl->QueryInterface(activation, &IID__x_ABI_CWindows_CUI_CXaml_CIApplicationStatics, (void **)&statics) != S_OK ||
		!statics) return 3;
	application_factory *factory = NULL;
	if (statics->lpVtbl->QueryInterface(statics, &IID__x_ABI_CWindows_CUI_CXaml_CIApplicationFactory, (void **)&factory) != S_OK ||
		!factory) return 4;
	void *inner = (void *)1, *instance = NULL;
	if (factory->lpVtbl->CreateInstance(factory, 0, (uint64_t)(uintptr_t)&inner, (uint64_t)(uintptr_t)&instance) != S_OK ||
		!instance || inner) return 5;
	application1 *application = NULL;
	if (((IUnknown *)instance)->lpVtbl->QueryInterface((IUnknown *)instance, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication,
		(void **)&application) != S_OK || !application) return 6;
	HSTRING runtime_name = NULL;
	UINT32 length = 0;
	{
		INT32 theme = -1;
		if (application->lpVtbl->get_RequestedTheme(application, 0) != E_POINTER ||
			application->lpVtbl->get_RequestedTheme(application, (uint64_t)(uintptr_t)&theme) != S_OK || theme != 0) return 105;
	}
	if (application->lpVtbl->GetRuntimeClassName(application, &runtime_name) != S_OK) return 7;
	const WCHAR *text = WindowsGetStringRawBuffer(runtime_name, &length);
	if (length != ARRAYSIZE(name) - 1 || CompareStringOrdinal(text, (int)length, name, -1, FALSE) != CSTR_EQUAL) return 8;
	WindowsDeleteString(runtime_name);
	application2 *later = NULL;
	if (application->lpVtbl->QueryInterface(application, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication2, (void **)&later) != S_OK ||
		!later) return 9;
	application3 *latest = NULL;
	if (later->lpVtbl->QueryInterface(later, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication3, (void **)&latest) != S_OK ||
		!latest) return 10;
	struct callback init = {&callback_table, 1, 0, 0};
	if (statics->lpVtbl->Start(statics, (uint64_t)(uintptr_t)&init) != S_OK || !init.invoked || !init.parameters_ok)
		return 12;
	application1 *current = NULL;
	if (statics->lpVtbl->get_Current(statics, (uint64_t)(uintptr_t)&current) != S_OK || !current) return 13;
	if (current != application) return 14;
	current->lpVtbl->Release(current);
	/* Both events retain each registration, remove only their own token, and release outstanding handlers on destruction. */
	struct callback lifecycle = {&callback_table, 1, 0, 0};
	uint64_t suspend_token = 0, resume_token = 0, other_token = 0;
	if (application->lpVtbl->add_Suspending(application, 0, (uint64_t)(uintptr_t)&suspend_token) != E_POINTER ||
		application->lpVtbl->add_Resuming(application, (uint64_t)(uintptr_t)&lifecycle, 0) != E_POINTER) return 101;
	if (application->lpVtbl->add_Suspending(application, (uint64_t)(uintptr_t)&lifecycle,
		(uint64_t)(uintptr_t)&suspend_token) != S_OK || !suspend_token || lifecycle.refs != 2) return 102;
	if (application->lpVtbl->add_Resuming(application, (uint64_t)(uintptr_t)&lifecycle,
		(uint64_t)(uintptr_t)&resume_token) != S_OK || !resume_token || resume_token == suspend_token || lifecycle.refs != 3) return 103;
	if (application->lpVtbl->remove_Resuming(application, suspend_token) != S_OK || lifecycle.refs != 3 ||
		application->lpVtbl->remove_Suspending(application, suspend_token) != S_OK || lifecycle.refs != 2 ||
		application->lpVtbl->remove_Suspending(application, suspend_token) != S_OK || lifecycle.refs != 2) return 104;
	if (application->lpVtbl->add_Suspending(application, (uint64_t)(uintptr_t)&lifecycle,
		(uint64_t)(uintptr_t)&other_token) != S_OK || !other_token || lifecycle.refs != 3) return 105;
	latest->lpVtbl->Release(latest);
	later->lpVtbl->Release(later);
	application->lpVtbl->Release(application);
	((IUnknown *)instance)->lpVtbl->Release((IUnknown *)instance);
	IInspectable *replacement = NULL;
	if (activation->lpVtbl->ActivateInstance(activation, &replacement) != S_OK || !replacement || lifecycle.refs != 1) return 106;
	replacement->lpVtbl->Release(replacement);
	factory->lpVtbl->Release(factory);
	statics->lpVtbl->Release(statics);
	activation->lpVtbl->Release(activation);
	HSTRING missing_key = NULL;
	if (WindowsCreateString(missing, ARRAYSIZE(missing) - 1, &missing_key) != S_OK) return 22;
	IInspectable *object = (void *)1;
	if (RoGetActivationFactory(missing_key, &IID_IActivationFactory, (void **)&object) != REGDB_E_CLASSNOTREG || object)
		return 11;
	void *restricted = (void *)1;
	if (GetRestrictedErrorInfo(&restricted) != S_FALSE || restricted) return 15;
	WindowsDeleteString(missing_key);
	WindowsDeleteString(class_key);

	/* The stable-index template round: the loader resolves Control.Template (560), ControlTemplate (191) and
	 * ContentPresenter (296) through the stable tables — the presenter has no runtime catalog entry, so its activation
	 * rides the engine-internal factory path behind the stable reference — and applies the template through the same
	 * deferred template-scope path the document-table round exercises. */
	{
		typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIButton button1;
		typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPanel panel1;
		typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl content_control1;
		typedef __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock textblock1;
		typedef HRESULT (*get_template_function)(IInspectable *, IInspectable **);
		typedef HRESULT (*template_root_function)(IInspectable *, IInspectable **);
		typedef HRESULT (*template_child_function)(IInspectable *, IInspectable **);
		static const WCHAR presenter_class[] = L"Windows.UI.Xaml.Controls.ContentPresenter";
		load_function load = NULL;
		get_template_function element_get_template = NULL;
		template_root_function read_template_root = NULL;
		template_child_function template_child = NULL;
		HMODULE engine = LoadLibraryW(L"raven-xaml.dll");
		unsigned char stable_fixture[208];
		IInspectable *loaded_button = NULL, *read_template = NULL, *root_grid = NULL, *presenter = NULL, *content_child = NULL;
		button1 *loaded_button_iface = NULL;
		panel1 *root_panel = NULL;
		content_control1 *loaded_content = NULL;
		textblock1 *content_text = NULL;
		void *loaded_template_iface = NULL, *children_object = NULL;
		struct fixture_vector *children = NULL;
		HSTRING presenter_name = NULL, content_text_value = NULL;
		UINT32 child_count = 0;
		if (!engine) return 30;
		load = (load_function)GetProcAddress(engine, "RavenXamlLoad");
		element_get_template = (get_template_function)GetProcAddress(engine, "raven_xaml_element_get_template");
		read_template_root = (template_root_function)GetProcAddress(engine, "raven_xaml_control_template_root");
		template_child = (template_child_function)GetProcAddress(engine, "raven_xaml_element_template_child");
		if (!load || !element_get_template || !read_template_root || !template_child) return 31;
		if (load(stable_fixture, (uint32_t)build_stable_template_document(stable_fixture), &loaded_button) != S_OK ||
			!loaded_button) return 32;
		if (loaded_button->lpVtbl->QueryInterface(loaded_button, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton,
			(void **)&loaded_button_iface) != S_OK || !loaded_button_iface) return 33;
		loaded_button_iface->lpVtbl->Release(loaded_button_iface);
		if (element_get_template(loaded_button, &read_template) != S_OK || !read_template) return 34;
		if (read_template->lpVtbl->QueryInterface(read_template,
			&IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplate, &loaded_template_iface) != S_OK ||
			!loaded_template_iface) return 35;
		((IUnknown *)loaded_template_iface)->lpVtbl->Release((IUnknown *)loaded_template_iface);
		/* the template root: the grid the document declared inside the template, holding the one presenter */
		if (read_template_root(read_template, &root_grid) != S_OK || !root_grid) return 36;
		if (root_grid->lpVtbl->QueryInterface(root_grid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
			(void **)&root_panel) != S_OK || !root_panel) return 37;
		if (root_panel->lpVtbl->get_Children(root_panel, (uint64_t)(uintptr_t)&children_object) != S_OK ||
			!children_object) return 38;
		children = (void *)children_object;
		if (children->lpVtbl->get_Size(children, &child_count) != S_OK || child_count != 1) return 39;
		if (children->lpVtbl->GetAt(children, 0, &presenter) != S_OK || !presenter) return 40;
		if (presenter->lpVtbl->GetRuntimeClassName(presenter, &presenter_name) != S_OK || !presenter_name) return 41;
		if (!same(presenter_name, presenter_class, ARRAYSIZE(presenter_class) - 1)) {
			WindowsDeleteString(presenter_name);
			return 42;
		}
		WindowsDeleteString(presenter_name);
		children->lpVtbl->Release(children);
		/* the presenter resolves the button's Content through the render walk's own hook */
		if (template_child(presenter, &content_child) != S_OK || !content_child) return 43;
		if (content_child->lpVtbl->QueryInterface(content_child, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
			(void **)&content_text) != S_OK || !content_text) return 44;
		if (content_text->lpVtbl->get_Text(content_text, (uint64_t)(uintptr_t)&content_text_value) != S_OK ||
			!content_text_value || !same(content_text_value, L"Hi", 2)) return 45;
		WindowsDeleteString(content_text_value);
		content_text->lpVtbl->Release(content_text);
		content_text = NULL;
		content_child->lpVtbl->Release(content_child);
		content_child = NULL;
		/* the button's stored content still answers logically while the template is applied */
		if (loaded_button->lpVtbl->QueryInterface(loaded_button, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
			(void **)&loaded_content) != S_OK || !loaded_content) return 46;
		if (loaded_content->lpVtbl->get_Content(loaded_content, (uint64_t)(uintptr_t)&content_child) != S_OK ||
			!content_child) return 47;
		loaded_content->lpVtbl->Release(loaded_content);
		content_child->lpVtbl->Release(content_child);
		content_child = NULL;
		presenter->lpVtbl->Release(presenter);
		presenter = NULL;
		root_panel->lpVtbl->Release(root_panel);
		root_grid->lpVtbl->Release(root_grid);
		read_template->lpVtbl->Release(read_template);
		read_template = NULL;
		loaded_button->lpVtbl->Release(loaded_button);
		loaded_button = NULL;
	}

	/* The ContentTemplate round: the document above, extended with ContentPresenter.ContentTemplate (525) under the
	 * presenter scope, whose template root grid carries one inner presenter. The loader resolves the stable property,
	 * opens the nested template scope, and the presenter's store keeps the second ControlTemplate. The applied
	 * template answers at the template-child hook: the presenter's render child is the content template's root — the
	 * button's Content (a TextBlock, no ContentControl) leaves the inner presenter unbound, and it answers nothing
	 * rather than something invented. */
	{
		typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPanel panel1;
		typedef HRESULT (*get_template_function)(IInspectable *, IInspectable **);
		typedef HRESULT (*template_root_function)(IInspectable *, IInspectable **);
		typedef HRESULT (*template_child_function)(IInspectable *, IInspectable **);
		typedef HRESULT (*get_content_template_function)(IInspectable *, IInspectable **);
		static const WCHAR presenter_class[] = L"Windows.UI.Xaml.Controls.ContentPresenter";
		load_function load = NULL;
		get_template_function element_get_template = NULL;
		template_root_function read_template_root = NULL;
		/* the export takes an IInspectable ** — the round-2 local typedef mirrors the ABI, not the block above */
		template_child_function template_child = NULL;
		get_content_template_function read_content_template = NULL;
		HMODULE engine = LoadLibraryW(L"raven-xaml.dll");
		unsigned char stable_fixture[214];
		IInspectable *loaded_button = NULL, *read_template = NULL, *root_grid = NULL;
		IInspectable *presenter = NULL, *stored_template = NULL, *stored_root = NULL, *content_child = NULL;
		IInspectable *inner_root = NULL, *inner_presenter = NULL;
		panel1 *root_panel = NULL, *inner_panel = NULL;
		void *children_object = NULL, *stored_template_iface = NULL, *inner_children_object = NULL;
		struct fixture_vector *children = NULL, *inner_children = NULL;
		HSTRING presenter_name = NULL;
		UINT32 child_count = 0, inner_child_count = 0;
		if (!engine) return 50;
		load = (load_function)GetProcAddress(engine, "RavenXamlLoad");
		element_get_template = (get_template_function)GetProcAddress(engine, "raven_xaml_element_get_template");
		read_template_root = (template_root_function)GetProcAddress(engine, "raven_xaml_control_template_root");
		template_child = (template_child_function)GetProcAddress(engine, "raven_xaml_element_template_child");
		read_content_template = (get_content_template_function)GetProcAddress(engine,
			"raven_xaml_element_get_content_template");
		if (!load || !element_get_template || !read_template_root || !template_child || !read_content_template) return 51;
		if (load(stable_fixture, (uint32_t)build_stable_content_template_document(stable_fixture), &loaded_button) != S_OK ||
			!loaded_button) return 52;
		if (element_get_template(loaded_button, &read_template) != S_OK || !read_template) return 53;
		if (read_template_root(read_template, &root_grid) != S_OK || !root_grid) return 54;
		if (root_grid->lpVtbl->QueryInterface(root_grid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
			(void **)&root_panel) != S_OK || !root_panel) return 55;
		if (root_panel->lpVtbl->get_Children(root_panel, (uint64_t)(uintptr_t)&children_object) != S_OK ||
			!children_object) return 56;
		children = (void *)children_object;
		if (children->lpVtbl->get_Size(children, &child_count) != S_OK || child_count != 1) return 57;
		if (children->lpVtbl->GetAt(children, 0, &presenter) != S_OK || !presenter) return 58;
		/* the stored content template: the second ControlTemplate the document declared, with its grid root */
		if (read_content_template(presenter, &stored_template) != S_OK || !stored_template) return 59;
		if (stored_template->lpVtbl->QueryInterface(stored_template,
			&IID___x_ABI_CWindows_CUI_CXaml_CControls_CIControlTemplate, &stored_template_iface) != S_OK ||
			!stored_template_iface) return 60;
		((IUnknown *)stored_template_iface)->lpVtbl->Release((IUnknown *)stored_template_iface);
		if (read_template_root(stored_template, &stored_root) != S_OK || !stored_root) return 61;
		stored_root->lpVtbl->Release(stored_root);
		/* the applied content template: the presenter's render child is the content template's root grid */
		if (template_child(presenter, &content_child) != S_OK || !content_child) return 62;
		if (content_child->lpVtbl->QueryInterface(content_child, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
			(void **)&inner_panel) != S_OK || !inner_panel) return 63;
		if (inner_panel->lpVtbl->get_Children(inner_panel, (uint64_t)(uintptr_t)&inner_children_object) != S_OK ||
			!inner_children_object) return 64;
		inner_children = (void *)inner_children_object;
		if (inner_children->lpVtbl->get_Size(inner_children, &inner_child_count) != S_OK || inner_child_count != 1)
			return 65;
		if (inner_children->lpVtbl->GetAt(inner_children, 0, &inner_presenter) != S_OK || !inner_presenter) return 66;
		if (inner_presenter->lpVtbl->GetRuntimeClassName(inner_presenter, &presenter_name) != S_OK || !presenter_name)
			return 67;
		if (!same(presenter_name, presenter_class, ARRAYSIZE(presenter_class) - 1)) {
			WindowsDeleteString(presenter_name);
			return 68;
		}
		WindowsDeleteString(presenter_name);
		/* the TextBlock content is no ContentControl: the inner presenter stays unbound and answers nothing */
		if (template_child(inner_presenter, &content_child) != S_OK || content_child) return 69;
		inner_presenter->lpVtbl->Release(inner_presenter);
		inner_presenter = NULL;
		inner_children->lpVtbl->Release(inner_children);
		inner_panel->lpVtbl->Release(inner_panel);
		content_child = NULL;
		inner_root = NULL;
		presenter->lpVtbl->Release(presenter);
		presenter = NULL;
		children->lpVtbl->Release(children);
		root_panel->lpVtbl->Release(root_panel);
		root_grid->lpVtbl->Release(root_grid);
		stored_template->lpVtbl->Release(stored_template);
		read_template->lpVtbl->Release(read_template);
		loaded_button->lpVtbl->Release(loaded_button);
	}

	/* The flow-through round: the button's Content is itself a Button holding the "Hi" text block, so the content
	 * template's inner presenter binds to the content button and resolves ITS content — the text — through the same
	 * hook. A presenter bound to the templated control would answer the inner button here; the text answer proves the
	 * binding followed the content object. */
	{
		typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPanel panel1;
		typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl content_control1;
		typedef __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock textblock1;
		typedef HRESULT (*template_child_function)(IInspectable *, IInspectable **);
		load_function load = NULL;
		template_child_function template_child = NULL;
		HMODULE engine = LoadLibraryW(L"raven-xaml.dll");
		unsigned char stable_fixture[233];
		IInspectable *loaded_button = NULL, *root_grid = NULL, *presenter = NULL, *inner_root = NULL;
		IInspectable *inner_presenter = NULL, *content_object = NULL, *content_child = NULL;
		panel1 *root_panel = NULL, *inner_panel = NULL;
		content_control1 *outer_content = NULL;
		textblock1 *content_text = NULL;
		void *children_object = NULL, *inner_children_object = NULL;
		struct fixture_vector *children = NULL, *inner_children = NULL;
		HSTRING content_text_value = NULL;
		UINT32 child_count = 0, inner_child_count = 0;
		if (!engine) return 70;
		load = (load_function)GetProcAddress(engine, "RavenXamlLoad");
		template_child = (template_child_function)GetProcAddress(engine, "raven_xaml_element_template_child");
		if (!load || !template_child) return 71;
		if (load(stable_fixture, (uint32_t)build_stable_flow_document(stable_fixture), &loaded_button) != S_OK ||
			!loaded_button) return 72;
		/* the logical content is the inner button */
		if (loaded_button->lpVtbl->QueryInterface(loaded_button, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
			(void **)&outer_content) != S_OK || !outer_content) return 73;
		if (outer_content->lpVtbl->get_Content(outer_content, (uint64_t)(uintptr_t)&content_object) != S_OK ||
			!content_object) return 74;
		outer_content->lpVtbl->Release(outer_content);
		outer_content = NULL;
		{
			void *content_button = NULL;
			if (content_object->lpVtbl->QueryInterface(content_object,
				&IID__x_ABI_CWindows_CUI_CXaml_CControls_CIButton, &content_button) != S_OK || !content_button)
				return 75;
			((IUnknown *)content_button)->lpVtbl->Release((IUnknown *)content_button);
		}
		/* the outer presenter: template root grid, one child, the presenter */
		{
			typedef HRESULT (*get_template_function)(IInspectable *, IInspectable **);
			typedef HRESULT (*template_root_function)(IInspectable *, IInspectable **);
			get_template_function element_get_template = (get_template_function)GetProcAddress(engine,
				"raven_xaml_element_get_template");
			template_root_function read_template_root = (template_root_function)GetProcAddress(engine,
				"raven_xaml_control_template_root");
			IInspectable *read_template = NULL;
			if (!element_get_template || !read_template_root) return 77;
			if (element_get_template(loaded_button, &read_template) != S_OK || !read_template) return 78;
			if (read_template_root(read_template, &root_grid) != S_OK || !root_grid) return 79;
			read_template->lpVtbl->Release(read_template);
		}
		if (root_grid->lpVtbl->QueryInterface(root_grid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
			(void **)&root_panel) != S_OK || !root_panel) return 80;
		if (root_panel->lpVtbl->get_Children(root_panel, (uint64_t)(uintptr_t)&children_object) != S_OK ||
			!children_object) return 81;
		children = (void *)children_object;
		if (children->lpVtbl->get_Size(children, &child_count) != S_OK || child_count != 1) return 82;
		if (children->lpVtbl->GetAt(children, 0, &presenter) != S_OK || !presenter) return 83;
		/* the applied content template root with the inner presenter */
		if (template_child(presenter, &inner_root) != S_OK || !inner_root) return 84;
		if (inner_root->lpVtbl->QueryInterface(inner_root, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
			(void **)&inner_panel) != S_OK || !inner_panel) return 85;
		if (inner_panel->lpVtbl->get_Children(inner_panel, (uint64_t)(uintptr_t)&inner_children_object) != S_OK ||
			!inner_children_object) return 86;
		inner_children = (void *)inner_children_object;
		if (inner_children->lpVtbl->get_Size(inner_children, &inner_child_count) != S_OK || inner_child_count != 1)
			return 87;
		if (inner_children->lpVtbl->GetAt(inner_children, 0, &inner_presenter) != S_OK || !inner_presenter) return 88;
		/* the flow-through: the inner presenter resolves the content button's Content, not the content itself */
		if (template_child(inner_presenter, &content_child) != S_OK || !content_child) return 89;
		if (content_child->lpVtbl->QueryInterface(content_child, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
			(void **)&content_text) != S_OK || !content_text) return 90;
		if (content_text->lpVtbl->get_Text(content_text, (uint64_t)(uintptr_t)&content_text_value) != S_OK ||
			!content_text_value || !same(content_text_value, L"Hi", 2)) return 91;
		WindowsDeleteString(content_text_value);
		content_text->lpVtbl->Release(content_text);
		content_child->lpVtbl->Release(content_child);
		inner_presenter->lpVtbl->Release(inner_presenter);
		inner_children->lpVtbl->Release(inner_children);
		inner_panel->lpVtbl->Release(inner_panel);
		inner_root->lpVtbl->Release(inner_root);
		presenter->lpVtbl->Release(presenter);
		children->lpVtbl->Release(children);
		root_panel->lpVtbl->Release(root_panel);
		root_grid->lpVtbl->Release(root_grid);
		content_object->lpVtbl->Release(content_object);
		loaded_button->lpVtbl->Release(loaded_button);
	}
	return 0;
}
