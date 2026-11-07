/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The style fixture: activates Windows.UI.Xaml.Style through the runtime catalog, adds setters carrying colour-name
 * strings and boxed numbers through the engine-internal helper, and verifies that putting a style on a hosted element
 * APPLIES the setters to the element's own storage — a Background "Red" setter paints a hosted grid red through the same
 * named-colour path the compiled markup uses, a FontSize setter lands in the text block's real font size (read back
 * through the public getter), and the pixel follows the setter the way it follows a direct put. The application is
 * atomic: a wrong-kind setter value (a number where a colour is wanted) fails put_Style with E_INVALIDARG and the element
 * keeps its previous state. put_Style(NULL) clears the reference but the engine re-applies on put and does not revert, so
 * the applied values stay. BasedOn applies the base chain's setters before the style's own, so a derived style overriding
 * only FontSize over a base that sets Background takes both, the derived FontSize winning. Read back through the same
 * emulated GDI surface the rendering fixture uses; the text re-measure asserts bounds instead of pixels, the way the text
 * fixture does, because the face the host font service picks decides the exact extent. A second grid without the style
 * keeps its own background, the style round-trips through a ResourceDictionary under a string key and still applies,
 * TargetType round-trips the interop TypeName and the Setters Size reflects the stored pairs. The authored default styles
 * round last: a fresh control the engine attached its default to answers get_Style with an engine Style whose setter
 * pairs carry the authored values (the button's transparent Background, the text block's FontSize 14 and black
 * Foreground), the defaults render through the same pipeline a put drives, an explicit put_Style overrides the default
 * and get_Style answers the app's style, and put_Style(NULL) clears as before without reverting. The DependencyProperty
 * stage runs after those: the statics come through the engine-internal export and through RoGetActivationFactory (the
 * cataloged class name resolves the same singleton), two DPs register with defaults carried on hand-rolled
 * IPropertyMetadata objects, the metadata readers round-trip name, TypeName, OwnerType and the default value's identity,
 * the duplicate (owner, name) contract refuses, the same name under another owner registers, RegisterAttached registers
 * and rejects duplicates, UnsetValue is one identity-stable sentinel, and a Setter carrying a DP applies through put_Style to the same
 * pixels the name-string setters reach — the DP's registered name flowing into the same (name, value) pair store, read
 * back through setter_count/setter_at. The ControlTemplate stage runs after those: a Style setter named "Template" carries
 * the ControlTemplate object itself through the same (name, value) store, the button routes it to the store put_Template
 * serves, and the applied template's root tree replaces the control's rendered visuals — a template whose root is a red
 * grid paints red where the unstyled control painted nothing, the replace changes the pixel, a wrong-kind Template value
 * fails put_Style with E_INVALIDARG leaving the previous template, and a direct put_Template(NULL) clears. Freestanding C
 * like the
 * other guests: hand-rolled COM, no CRT beyond the freestanding one the build links. */
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <roapi.h>
#include <activation.h>
#include <windows.foundation.h>
#include <windows.ui.core.h>
#include <windows.ui.composition.h>
#include "../../native/upstream/raven-xaml/corewindow_interop.h"
#include "windows_ui_xaml_dictionary.h"
#include "windows_ui_xaml_elements.h"
#include "windows_ui_xaml_style.h"
#include "xaml_styles.h" /* the DependencyProperty interfaces, IIDs and the engine-internal interop part */
#include "windows_ui_xaml_window.h"

typedef __x_ABI_CWindows_CUI_CXaml_CIStyle style_i;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindow window_i;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindowStatics window_statics_i;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPanel panel_i;
typedef __x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock textblock_i;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositor compositor_i;
typedef __x_ABI_CWindows_CUI_CComposition_CICompositionColorBrush brush_i;
typedef __x_ABI_CWindows_CUI_CColor color_s;
typedef __FIMap_2_IInspectable_IInspectable map_i;
typedef __FIReference_1_HSTRING boxed_string_i;
typedef __FIReference_1_HSTRINGVtbl boxed_string_i_vtbl;
typedef __FIReference_1_DOUBLE boxed_number_i;
typedef __FIReference_1_DOUBLEVtbl boxed_number_i_vtbl;
typedef __FIVector_1_Windows__CUI__CXaml__CSetterBase setter_vector_i;
typedef __x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName type_name_s;
typedef __x_ABI_CWindows_CUI_CXaml_CIDependencyProperty dp_i;
typedef __x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics dp_statics_i;
typedef __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata metadata_i;
typedef __x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataVtbl metadata_i_vtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CISetter setter_i;
typedef raven_dependency_property_interop dp_interop_i;

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);

/* The engine-internal exports, reached through GetProcAddress the way the loader fixtures reach RavenXamlLoad; the
 * signatures follow xaml_styles.h. */
typedef HRESULT (*add_setter_function)(IInspectable *, const WCHAR *, UINT32, IInspectable *);
typedef HRESULT (*put_style_function)(IInspectable *, IInspectable *);
typedef HRESULT (*get_style_function)(IInspectable *, IInspectable **);
typedef HRESULT (*default_attach_function)(IInspectable *);
typedef HRESULT (*setter_count_function)(IInspectable *, UINT32 *);
typedef HRESULT (*setter_at_function)(IInspectable *, UINT32, const WCHAR **, UINT32 *, IInspectable **);
typedef HRESULT (*dependency_statics_function)(dp_statics_i **);
typedef HRESULT (*setter_create_function)(IInspectable **);
typedef HRESULT (*template_factory_function)(IActivationFactory **);
typedef HRESULT (*template_root_function)(IInspectable *, IInspectable *);
typedef HRESULT (*put_template_function)(IInspectable *, IInspectable *);
typedef HRESULT (*get_template_function)(IInspectable *, IInspectable **);

static int same_text(HSTRING value, const WCHAR *expected)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(value, &length);
	return length == lstrlenW(expected) && CompareStringOrdinal(text, (int)length, expected, -1, FALSE) == CSTR_EQUAL;
}

/* The property name setter_at hands out: a raw buffer with its own length, not an HSTRING. */
static int same_raw(const WCHAR *text, UINT32 length, const WCHAR *expected)
{
	return text && length == (UINT32)lstrlenW(expected) &&
		CompareStringOrdinal(text, (int)length, expected, -1, FALSE) == CSTR_EQUAL;
}

static int same_unknown(void *left, void *right)
{
	IUnknown *one = NULL, *other = NULL;
	const int same = left && right &&
		SUCCEEDED(((IUnknown *)left)->lpVtbl->QueryInterface((IUnknown *)left, &IID_IUnknown, (void **)&one)) && one &&
		SUCCEEDED(((IUnknown *)right)->lpVtbl->QueryInterface((IUnknown *)right, &IID_IUnknown, (void **)&other)) &&
		other && one == other;
	if (one)
		one->lpVtbl->Release(one);
	if (other)
		other->lpVtbl->Release(other);
	return same;
}

/* A boxed string: what a WinRT caller passes as a dictionary key or a colour-name setter value. The inspectable part
 * carries the identity, the box part answers IReference<HSTRING>, and QI reaches one from the other as COM identity
 * requires — the same object the resource-dictionary fixture builds. */
struct fixture_box;
struct fixture_box_part {
	const boxed_string_i_vtbl *lpVtbl;
	struct fixture_box *owner;
};
struct fixture_box {
	const IInspectableVtbl *lpVtbl;
	struct fixture_box_part box;
	LONG refs;
	HSTRING value;
};

static ULONG fixture_box_addref(struct fixture_box *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG fixture_box_release(struct fixture_box *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->value)
			WindowsDeleteString(self->value);
		HeapFree(GetProcessHeap(), 0, self);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI box_query(IInspectable *iface, REFIID iid, void **object)
{
	struct fixture_box *self = (struct fixture_box *)iface;
	if (!object)
		return E_POINTER;
	*object = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable)) {
		*object = iface;
	} else if (IsEqualIID(iid, &IID___FIReference_1_HSTRING)) {
		*object = &self->box;
	} else
		return E_NOINTERFACE;
	fixture_box_addref(self);
	return S_OK;
}

static HRESULT WINAPI box_part_query(boxed_string_i *iface, REFIID iid, void **object)
{
	struct fixture_box *self = ((struct fixture_box_part *)iface)->owner;
	if (!object)
		return E_POINTER;
	*object = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable)) {
		*object = self;
	} else if (IsEqualIID(iid, &IID___FIReference_1_HSTRING)) {
		*object = iface;
	} else
		return E_NOINTERFACE;
	fixture_box_addref(self);
	return S_OK;
}

static ULONG WINAPI box_addref(IInspectable *iface) { return fixture_box_addref((struct fixture_box *)iface); }
static ULONG WINAPI box_release(IInspectable *iface) { return fixture_box_release((struct fixture_box *)iface); }
static ULONG WINAPI box_part_addref(boxed_string_i *iface)
{
	return fixture_box_addref(((struct fixture_box_part *)iface)->owner);
}
static ULONG WINAPI box_part_release(boxed_string_i *iface)
{
	return fixture_box_release(((struct fixture_box_part *)iface)->owner);
}

static HRESULT WINAPI box_iids(IInspectable *iface, ULONG *count, IID **iids)
{ (void)iface; if (!count || !iids) return E_POINTER; *count = 0; *iids = NULL; return S_OK; }
static HRESULT WINAPI box_part_iids(boxed_string_i *iface, ULONG *count, IID **iids)
{ (void)iface; if (!count || !iids) return E_POINTER; *count = 0; *iids = NULL; return S_OK; }
static HRESULT WINAPI box_trust(IInspectable *iface, TrustLevel *level)
{ (void)iface; if (!level) return E_POINTER; *level = BaseTrust; return S_OK; }
static HRESULT WINAPI box_part_trust(boxed_string_i *iface, TrustLevel *level)
{ (void)iface; if (!level) return E_POINTER; *level = BaseTrust; return S_OK; }

static HRESULT WINAPI box_name(IInspectable *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Raven.Fixture.BoxedString";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, ARRAYSIZE(text) - 1, name);
}

static HRESULT WINAPI box_part_name(boxed_string_i *iface, HSTRING *name)
{
	return box_name((IInspectable *)((struct fixture_box_part *)iface)->owner, name);
}

static HRESULT WINAPI box_part_value(boxed_string_i *iface, HSTRING *value)
{
	struct fixture_box *self = ((struct fixture_box_part *)iface)->owner;
	if (!value)
		return E_POINTER;
	*value = NULL;
	/* the caller deletes what it receives, so the stored string is handed out duplicated */
	return WindowsDuplicateString(self->value, value);
}

static const IInspectableVtbl box_inspectable_table = {
	box_query, box_addref, box_release, box_iids, box_name, box_trust,
};

static const boxed_string_i_vtbl box_part_table = {
	box_part_query, box_part_addref, box_part_release, box_part_iids, box_part_name, box_part_trust,
	box_part_value,
};

static struct fixture_box *make_box(const WCHAR *text)
{
	struct fixture_box *self = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *self);
	if (!self)
		return NULL;
	self->lpVtbl = &box_inspectable_table;
	self->box.lpVtbl = &box_part_table;
	self->box.owner = self;
	self->refs = 1;
	if (WindowsCreateString(text, (UINT32)lstrlenW(text), &self->value) != S_OK) {
		HeapFree(GetProcessHeap(), 0, self);
		return NULL;
	}
	return self;
}

/* A boxed number: what a WinRT caller passes for the numeric setter values (FontSize, Width, Height). The same shape as
 * the string box over IReference<double>. */
struct fixture_number;
struct fixture_number_part {
	const boxed_number_i_vtbl *lpVtbl;
	struct fixture_number *owner;
};
struct fixture_number {
	const IInspectableVtbl *lpVtbl;
	struct fixture_number_part box;
	LONG refs;
	double value;
};

static ULONG fixture_number_addref(struct fixture_number *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG fixture_number_release(struct fixture_number *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs)
		HeapFree(GetProcessHeap(), 0, self);
	return (ULONG)refs;
}

static HRESULT WINAPI number_query(IInspectable *iface, REFIID iid, void **object)
{
	struct fixture_number *self = (struct fixture_number *)iface;
	if (!object)
		return E_POINTER;
	*object = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable)) {
		*object = iface;
	} else if (IsEqualIID(iid, &IID___FIReference_1_DOUBLE)) {
		*object = &self->box;
	} else
		return E_NOINTERFACE;
	fixture_number_addref(self);
	return S_OK;
}

static HRESULT WINAPI number_part_query(boxed_number_i *iface, REFIID iid, void **object)
{
	struct fixture_number *self = ((struct fixture_number_part *)iface)->owner;
	if (!object)
		return E_POINTER;
	*object = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable)) {
		*object = self;
	} else if (IsEqualIID(iid, &IID___FIReference_1_DOUBLE)) {
		*object = iface;
	} else
		return E_NOINTERFACE;
	fixture_number_addref(self);
	return S_OK;
}

static ULONG WINAPI number_addref(IInspectable *iface) { return fixture_number_addref((struct fixture_number *)iface); }
static ULONG WINAPI number_release(IInspectable *iface) { return fixture_number_release((struct fixture_number *)iface); }
static ULONG WINAPI number_part_addref(boxed_number_i *iface)
{
	return fixture_number_addref(((struct fixture_number_part *)iface)->owner);
}
static ULONG WINAPI number_part_release(boxed_number_i *iface)
{
	return fixture_number_release(((struct fixture_number_part *)iface)->owner);
}

static HRESULT WINAPI number_iids(IInspectable *iface, ULONG *count, IID **iids)
{ (void)iface; if (!count || !iids) return E_POINTER; *count = 0; *iids = NULL; return S_OK; }
static HRESULT WINAPI number_part_iids(boxed_number_i *iface, ULONG *count, IID **iids)
{ (void)iface; if (!count || !iids) return E_POINTER; *count = 0; *iids = NULL; return S_OK; }
static HRESULT WINAPI number_trust(IInspectable *iface, TrustLevel *level)
{ (void)iface; if (!level) return E_POINTER; *level = BaseTrust; return S_OK; }
static HRESULT WINAPI number_part_trust(boxed_number_i *iface, TrustLevel *level)
{ (void)iface; if (!level) return E_POINTER; *level = BaseTrust; return S_OK; }

static HRESULT WINAPI number_name(IInspectable *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Raven.Fixture.BoxedNumber";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, ARRAYSIZE(text) - 1, name);
}

static HRESULT WINAPI number_part_name(boxed_number_i *iface, HSTRING *name)
{
	return number_name((IInspectable *)((struct fixture_number_part *)iface)->owner, name);
}

static HRESULT WINAPI number_part_value(boxed_number_i *iface, double *value)
{
	struct fixture_number *self = ((struct fixture_number_part *)iface)->owner;
	if (!value)
		return E_POINTER;
	*value = self->value;
	return S_OK;
}

static const IInspectableVtbl number_inspectable_table = {
	number_query, number_addref, number_release, number_iids, number_name, number_trust,
};

static const boxed_number_i_vtbl number_part_table = {
	number_part_query, number_part_addref, number_part_release, number_part_iids, number_part_name, number_part_trust,
	number_part_value,
};

static struct fixture_number *make_number(double value)
{
	struct fixture_number *self = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *self);
	if (!self)
		return NULL;
	self->lpVtbl = &number_inspectable_table;
	self->box.lpVtbl = &number_part_table;
	self->box.owner = self;
	self->refs = 1;
	self->value = value;
	return self;
}

/* A hand-rolled IPropertyMetadata: the object a DependencyProperty registration takes its default value from, built the
 * way the boxes above are hand-rolled IReference`1 — the increment implements DependencyProperty, not PropertyMetadata,
 * and the engine reads only get_DefaultValue through the real IID. */
struct fixture_metadata;
struct fixture_metadata_part {
	const metadata_i_vtbl *lpVtbl;
	struct fixture_metadata *owner;
};
struct fixture_metadata {
	const IInspectableVtbl *lpVtbl;
	struct fixture_metadata_part part;
	LONG refs;
	IInspectable *value;
};

static ULONG fixture_metadata_addref(struct fixture_metadata *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG fixture_metadata_release_any(struct fixture_metadata *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->value)
			self->value->lpVtbl->Release(self->value);
		HeapFree(GetProcessHeap(), 0, self);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI metadata_query(IInspectable *iface, REFIID iid, void **object)
{
	struct fixture_metadata *self = (struct fixture_metadata *)iface;
	if (!object)
		return E_POINTER;
	*object = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable))
		*object = iface;
	else if (IsEqualIID(iid, &IID___x_ABI_CWindows_CUI_CXaml_CIPropertyMetadata))
		*object = &self->part;
	else
		return E_NOINTERFACE;
	fixture_metadata_addref(self);
	return S_OK;
}

static HRESULT WINAPI metadata_part_query(metadata_i *iface, REFIID iid, void **object)
{
	return metadata_query((IInspectable *)((struct fixture_metadata_part *)iface)->owner, iid, object);
}

static ULONG WINAPI metadata_addref(IInspectable *iface) { return fixture_metadata_addref((struct fixture_metadata *)iface); }
static ULONG WINAPI metadata_release(IInspectable *iface) { return fixture_metadata_release_any((struct fixture_metadata *)iface); }
static ULONG WINAPI metadata_part_addref(metadata_i *iface)
{
	return fixture_metadata_addref(((struct fixture_metadata_part *)iface)->owner);
}
static ULONG WINAPI metadata_part_release(metadata_i *iface)
{
	return fixture_metadata_release_any(((struct fixture_metadata_part *)iface)->owner);
}

static HRESULT WINAPI metadata_iids(IInspectable *iface, ULONG *count, IID **iids)
{ (void)iface; if (!count || !iids) return E_POINTER; *count = 0; *iids = NULL; return S_OK; }
static HRESULT WINAPI metadata_part_iids(metadata_i *iface, ULONG *count, IID **iids)
{ (void)iface; if (!count || !iids) return E_POINTER; *count = 0; *iids = NULL; return S_OK; }
static HRESULT WINAPI metadata_trust(IInspectable *iface, TrustLevel *level)
{ (void)iface; if (!level) return E_POINTER; *level = BaseTrust; return S_OK; }
static HRESULT WINAPI metadata_part_trust(metadata_i *iface, TrustLevel *level)
{ (void)iface; if (!level) return E_POINTER; *level = BaseTrust; return S_OK; }

static HRESULT WINAPI metadata_name(IInspectable *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Raven.Fixture.PropertyMetadata";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, ARRAYSIZE(text) - 1, name);
}

static HRESULT WINAPI metadata_part_name(metadata_i *iface, HSTRING *name)
{
	return metadata_name((IInspectable *)((struct fixture_metadata_part *)iface)->owner, name);
}

static HRESULT WINAPI metadata_default_value(metadata_i *iface, IInspectable **value)
{
	struct fixture_metadata *self = ((struct fixture_metadata_part *)iface)->owner;
	if (!value)
		return E_POINTER;
	*value = self->value;
	if (*value)
		(*value)->lpVtbl->AddRef(*value);
	return S_OK;
}

static HRESULT WINAPI metadata_create_callback(metadata_i *iface,
	__x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback **value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = NULL; /* a metadata built with a value carries no factory callback */
	return S_OK;
}

static const IInspectableVtbl metadata_inspectable_table = {
	metadata_query, metadata_addref, metadata_release, metadata_iids, metadata_name, metadata_trust,
};

static const metadata_i_vtbl metadata_part_table = {
	metadata_part_query, metadata_part_addref, metadata_part_release, metadata_part_iids, metadata_part_name,
	metadata_part_trust, metadata_default_value, metadata_create_callback,
};

static struct fixture_metadata *make_metadata(IInspectable *value)
{
	struct fixture_metadata *self = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *self);
	if (!self)
		return NULL;
	self->lpVtbl = &metadata_inspectable_table;
	self->part.lpVtbl = &metadata_part_table;
	self->part.owner = self;
	self->refs = 1;
	if (value) {
		value->lpVtbl->AddRef(value);
		self->value = value;
	}
	return self;
}

static void fixture_metadata_release(struct fixture_metadata *self)
{
	if (self)
		fixture_metadata_release_any(self);
}

/* A hand-rolled ICreateDefaultValueCallback: the lazy default creator a factory-backed PropertyMetadata holds. The
 * delegate ABI is IUnknown plus one Invoke slot; the fixture answers the value it was built with and counts the calls,
 * so the engine invoking it exactly when a default is needed is provable. */
typedef __x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback create_cb_i;
struct fixture_create_cb {
	const __x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallbackVtbl *lpVtbl;
	LONG refs;
	IInspectable *value;
	LONG invoked;
};

static ULONG fixture_create_cb_addref(struct fixture_create_cb *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG WINAPI create_cb_addref(create_cb_i *iface)
{
	return fixture_create_cb_addref((struct fixture_create_cb *)iface);
}

static ULONG WINAPI create_cb_release(create_cb_i *iface)
{
	struct fixture_create_cb *self = (struct fixture_create_cb *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->value)
			self->value->lpVtbl->Release(self->value);
		HeapFree(GetProcessHeap(), 0, self);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI create_cb_query(create_cb_i *iface, REFIID iid, void **object)
{
	struct fixture_create_cb *self = (struct fixture_create_cb *)iface;
	if (!object)
		return E_POINTER;
	*object = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID___x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback))
		*object = iface;
	else
		return E_NOINTERFACE;
	fixture_create_cb_addref(self);
	return S_OK;
}

static HRESULT WINAPI create_cb_invoke(create_cb_i *iface, IInspectable **value)
{
	struct fixture_create_cb *self = (struct fixture_create_cb *)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	InterlockedIncrement(&self->invoked);
	if (self->value) {
		self->value->lpVtbl->AddRef(self->value);
		*value = self->value;
	}
	return S_OK;
}

static const __x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallbackVtbl create_cb_table = {
	create_cb_query, create_cb_addref, create_cb_release, create_cb_invoke,
};

static struct fixture_create_cb *make_create_cb(IInspectable *value)
{
	struct fixture_create_cb *self = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *self);
	if (!self)
		return NULL;
	self->lpVtbl = &create_cb_table;
	self->refs = 1;
	if (value) {
		value->lpVtbl->AddRef(value);
		self->value = value;
	}
	return self;
}

/* The parameterized IVector`1<UIElement> a panel's Children hands out: the GUID is the derivation the element layer
 * documents (SHA-1 of the signature over the parameterized-type namespace), the vtable the twelve IVector`1 slots after
 * the IInspectable ones. The engine's children vector answers this IID, so the fixture can append a child the way the
 * loader does. */
DEFINE_GUID(IID___FIVector_1_UIElement, 0x393d6210, 0xfa21, 0x573a, 0xa2, 0x38, 0xf0, 0xe8, 0x73, 0x3b, 0x3c, 0x94);

typedef struct fixture_children_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(void *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(void *This);
	ULONG (STDMETHODCALLTYPE *Release)(void *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(void *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(void *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(void *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetAt)(void *This, UINT32 index, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(void *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *GetView)(void *This, void **value);
	HRESULT (STDMETHODCALLTYPE *IndexOf)(void *This, IInspectable *element, UINT32 *index, boolean *found);
	HRESULT (STDMETHODCALLTYPE *SetAt)(void *This, UINT32 index, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *InsertAt)(void *This, UINT32 index, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *RemoveAt)(void *This, UINT32 index);
	HRESULT (STDMETHODCALLTYPE *Append)(void *This, IInspectable *value);
} fixture_children_vtbl;

/* The object the QI hands back: the vtable sits behind its lpVtbl, the way every COM interface here does. */
struct fixture_children {
	const fixture_children_vtbl *lpVtbl;
};

static HRESULT activate(const WCHAR *name, IInspectable **out)
{
	HSTRING key = NULL;
	IActivationFactory *factory = NULL;
	HRESULT result;
	if (WindowsCreateString(name, (UINT32)lstrlenW(name), &key) != S_OK)
		return E_FAIL;
	result = RoGetActivationFactory(key, &IID_IActivationFactory, (void **)&factory);
	WindowsDeleteString(key);
	if (FAILED(result))
		return result;
	result = factory->lpVtbl->ActivateInstance(factory, out);
	factory->lpVtbl->Release(factory);
	return result;
}

/* Append one child to a panel's Children through the parameterized vector the ABI serves — the same walk the inline
 * rounds do, factored for the rounds that add several children. */
static HRESULT append_child(panel_i *panel, IInspectable *child)
{
	void *children = NULL;
	struct fixture_children *vector = NULL;
	HRESULT result;
	if (panel->lpVtbl->get_Children(panel, (uint64_t)(uintptr_t)&children) != S_OK || !children)
		return E_FAIL;
	if (((IInspectable *)children)->lpVtbl->QueryInterface((IInspectable *)children,
		&IID___FIVector_1_UIElement, (void **)&vector) != S_OK) {
		((IInspectable *)children)->lpVtbl->Release((IInspectable *)children);
		return E_FAIL;
	}
	((IInspectable *)children)->lpVtbl->Release((IInspectable *)children);
	result = vector->lpVtbl->Append(vector, child);
	vector->lpVtbl->Release(vector);
	return result;
}

static int color_at(HWND hwnd, int x, int y, COLORREF expected)
{
	HDC dc = GetDC(hwnd);
	if (!dc)
		return 1;
	const COLORREF pixel = GetPixel(dc, x, y);
	ReleaseDC(hwnd, dc);
	return pixel == expected ? 0 : 1;
}

static int channel_delta(int left, int right)
{
	return left > right ? left - right : right - left;
}

/* The glyph pixels are deterministic the way the text fixture describes: the CPU fallback rasterizes monochrome coverage,
 * so a foreground leaves pixels of exactly its colour where the strokes are. A pixel counts as the foreground when every
 * channel stays within 55 of it, which the blue background never does. */
static int near_color(COLORREF pixel, COLORREF color)
{
	return channel_delta(GetRValue(pixel), GetRValue(color)) <= 55 &&
		channel_delta(GetGValue(pixel), GetGValue(color)) <= 55 &&
		channel_delta(GetBValue(pixel), GetBValue(color)) <= 55;
}

static UINT count_foreground(HWND hwnd, int x, int y, int width, int height, COLORREF color)
{
	HDC dc = GetDC(hwnd);
	UINT found = 0;
	if (!dc)
		return 0;
	for (int row = 0; row < height; ++row)
		for (int column = 0; column < width; ++column)
			if (near_color(GetPixel(dc, x + column, y + row), color))
				++found;
	ReleaseDC(hwnd, dc);
	return found;
}

/* One runtime object per check; the codes are unique so a failure names its step. The styled grid takes turns as the
 * hosted root, the way the rendering fixture swaps its content: it paints nothing (its own background is unset), red
 * under the first style, still red after put_Style(NULL) — the engine re-applies on put and does not revert — red again
 * through the dictionary round trip, blue under the replacement style (whose FontSize setter lands on the text block
 * child instead), red once more through the BasedOn chain, and it stays red through the wrong-kind rejections. The plain
 * grid paints its own blue before any style exists and again at the end. */
int raven_entry(void)
{
	static const WCHAR style_name[] = L"Windows.UI.Xaml.Style";
	static const WCHAR grid_name[] = L"Windows.UI.Xaml.Controls.Grid";
	static const WCHAR textblock_name[] = L"Windows.UI.Xaml.Controls.TextBlock";
	static const WCHAR button_name[] = L"Windows.UI.Xaml.Controls.Button";
	static const WCHAR dictionary_name[] = L"Windows.UI.Xaml.ResourceDictionary";
	static const WCHAR core_name[] = L"Windows.UI.Core.CoreWindow";
	static const WCHAR window_name[] = L"Windows.UI.Xaml.Window";
	static const WCHAR compositor_name[] = L"Windows.UI.Composition.Compositor";
	static const WCHAR grid_type[] = L"Windows.UI.Xaml.Controls.Grid";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function initialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function get_factory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	HSTRING core_key = NULL, window_key = NULL, compositor_key = NULL;
	__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic *core_statics = NULL;
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *core = NULL;
	raven_core_window_interop *interop = NULL;
	HWND hwnd = NULL;
	IActivationFactory *window_activation = NULL;
	window_statics_i *window_statics = NULL;
	window_i *xaml_window = NULL;
	IInspectable *grid_plain = NULL, *grid_styled = NULL, *style = NULL, *found = NULL, *compositor_object = NULL;
	IInspectable *style2 = NULL, *textblock = NULL, *base_style = NULL, *derived_style = NULL;
	IInspectable *bad_style = NULL, *bad_style2 = NULL, *skip_style = NULL, *roundtrip = NULL;
	IInspectable *default_button = NULL, *default_textblock = NULL, *override_style = NULL;
	style_i *style_iface = NULL, *derived_iface = NULL;
	textblock_i *text_iface = NULL, *default_text_iface = NULL;
	panel_i *plain_panel = NULL, *styled_panel = NULL;
	compositor_i *compositor = NULL;
	brush_i *brush = NULL;
	map_i *map = NULL;
	IInspectable *dictionary = NULL;
	void *children = NULL;
	struct fixture_children *child_vector = NULL;
	HMODULE component = NULL;
	add_setter_function add_setter = NULL;
	put_style_function put_style = NULL;
	get_style_function get_style = NULL;
	default_attach_function default_attach = NULL;
	setter_count_function setter_count = NULL;
	setter_at_function setter_at = NULL;
	struct fixture_box *red = NULL, *key = NULL, *probe = NULL, *blue = NULL, *red2 = NULL, *blue2 = NULL;
	struct fixture_number *size32 = NULL, *size14 = NULL, *seven = NULL, *three = NULL, *size8 = NULL;
	UINT count14 = 0, count32 = 0, count_default = 0, count_override = 0;
	float font_size = 0;
	dp_statics_i *dp_statics = NULL;
	dp_i *font_dp = NULL, *background_dp = NULL, *grid_dp = NULL, *property_read = NULL, *property_read2 = NULL;
	setter_i *dp_setter = NULL, *dp_setter2 = NULL;
	IInspectable *setter_object = NULL, *setter_object2 = NULL, *style3 = NULL, *style4 = NULL;
	IInspectable *dp_unset1 = NULL, *dp_unset2 = NULL;
	style_i *style3_iface = NULL, *style4_iface = NULL;
	setter_vector_i *style3_setters = NULL, *style4_setters = NULL;
	struct fixture_metadata *font_metadata = NULL, *bg_metadata = NULL;
	struct fixture_number *font_default = NULL, *forty = NULL;
	struct fixture_box *bg_default = NULL, *dp_blue = NULL;
	HSTRING font_key = NULL, bg_key = NULL;
	/* the ControlTemplate round */
	template_factory_function control_template_factory_fn = NULL;
	template_root_function template_put_root = NULL;
	put_template_function put_template = NULL;
	get_template_function get_template = NULL;
	IActivationFactory *control_template_factory = NULL;
	IInspectable *template_red = NULL, *template_blue = NULL, *template_style = NULL, *template_style2 = NULL;
	IInspectable *template_bad = NULL, *template_button = NULL, *template_root_grid = NULL, *template_root_grid2 = NULL;
	brush_i *red_brush = NULL, *blue_brush = NULL;
	if (!initialize || !get_factory)
		return 1;
	if (initialize(1) != S_OK)
		return 2;
	/* the interop window and its surface */
	if (WindowsCreateString(core_name, ARRAYSIZE(core_name) - 1, &core_key) != S_OK)
		return 3;
	if (get_factory(core_key, &IID___x_ABI_CWindows_CUI_CCore_CICoreWindowStatic, (void **)&core_statics) != S_OK ||
		!core_statics)
		return 4;
	if (__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_GetForCurrentThread(core_statics, &core) != S_OK || !core)
		return 5;
	if (core->lpVtbl->QueryInterface(core, &raven_core_window_interop_iid, (void **)&interop) != S_OK || !interop)
		return 6;
	if (interop->lpVtbl->get_WindowHandle(interop, &hwnd) != S_OK || !hwnd)
		return 7;
	core->lpVtbl->Activate(core);
	/* the XAML window the fixture hosts its grids on */
	if (WindowsCreateString(window_name, ARRAYSIZE(window_name) - 1, &window_key) != S_OK)
		return 8;
	if (get_factory(window_key, &IID_IActivationFactory, (void **)&window_activation) != S_OK || !window_activation)
		return 9;
	if (window_activation->lpVtbl->QueryInterface(window_activation, &IID__x_ABI_CWindows_CUI_CXaml_CIWindowStatics,
		(void **)&window_statics) != S_OK || !window_statics)
		return 10;
	if (window_statics->lpVtbl->get_Current(window_statics, (uint64_t)(uintptr_t)&xaml_window) != S_OK || !xaml_window)
		return 11;
	/* the engine-internal exports */
	component = LoadLibraryW(L"raven-xaml.dll");
	add_setter = component ? (add_setter_function)GetProcAddress(component, "raven_style_add_setter") : NULL;
	put_style = component ? (put_style_function)GetProcAddress(component, "raven_xaml_element_put_style") : NULL;
	get_style = component ? (get_style_function)GetProcAddress(component, "raven_xaml_element_get_style") : NULL;
	default_attach = component ? (default_attach_function)GetProcAddress(component, "raven_style_default_attach") : NULL;
	setter_count = component ? (setter_count_function)GetProcAddress(component, "raven_style_setter_count") : NULL;
	setter_at = component ? (setter_at_function)GetProcAddress(component, "raven_style_setter_at") : NULL;
	if (!add_setter || !put_style || !get_style || !default_attach || !setter_count || !setter_at)
		return 12;
	control_template_factory_fn = component ? (template_factory_function)GetProcAddress(component, "raven_xaml_control_template_factory") : NULL;
	template_put_root = component ? (template_root_function)GetProcAddress(component, "raven_xaml_control_template_put_root") : NULL;
	put_template = component ? (put_template_function)GetProcAddress(component, "raven_xaml_element_put_template") : NULL;
	get_template = component ? (get_template_function)GetProcAddress(component, "raven_xaml_element_get_template") : NULL;
	if (!control_template_factory_fn || !template_put_root || !put_template || !get_template)
		return 12;
	/* the plain grid: its own blue background through the real brush slot, set before any style exists */
	if (activate(grid_name, &grid_plain) != S_OK || !grid_plain)
		return 13;
	if (WindowsCreateString(compositor_name, ARRAYSIZE(compositor_name) - 1, &compositor_key) != S_OK)
		return 14;
	if (RoActivateInstance(compositor_key, &compositor_object) != S_OK || !compositor_object)
		return 15;
	if (compositor_object->lpVtbl->QueryInterface(compositor_object,
		&IID___x_ABI_CWindows_CUI_CComposition_CICompositor, (void **)&compositor) != S_OK || !compositor)
		return 16;
	{
		color_s blue;
		blue.A = 255;
		blue.R = 0;
		blue.G = 0;
		blue.B = 255;
		if (compositor->lpVtbl->CreateColorBrushWithColor(compositor, blue, &brush) != S_OK || !brush)
			return 17;
	}
	if (grid_plain->lpVtbl->QueryInterface(grid_plain, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
		(void **)&plain_panel) != S_OK || !plain_panel)
		return 18;
	if (plain_panel->lpVtbl->put_Background(plain_panel, (uint64_t)(uintptr_t)brush) != S_OK)
		return 19;
	if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)grid_plain) != S_OK)
		return 20;
	/* the unstyled grid paints its own background */
	if (color_at(hwnd, 5, 5, RGB(0, 0, 255)))
		return 21;
	if (color_at(hwnd, 300, 200, RGB(0, 0, 255)))
		return 22;
	/* the style: the fresh instance is not sealed and TargetType round-trips the interop TypeName */
	if (activate(style_name, &style) != S_OK || !style)
		return 23;
	if (style->lpVtbl->QueryInterface(style, &IID__x_ABI_CWindows_CUI_CXaml_CIStyle, (void **)&style_iface) != S_OK ||
		!style_iface)
		return 24;
	{
		type_name_s put, read;
		boolean sealed_flag = TRUE;
		if (WindowsCreateString(grid_type, ARRAYSIZE(grid_type) - 1, &put.Name) != S_OK)
			return 25;
		put.Kind = TypeKind_Metadata;
		read.Name = NULL;
		read.Kind = TypeKind_Primitive;
		if (style_iface->lpVtbl->put_TargetType(style_iface, put) != S_OK)
			return 26;
		if (style_iface->lpVtbl->get_TargetType(style_iface, &read) != S_OK || !read.Name)
			return 27;
		if (!same_text(read.Name, grid_type) || read.Kind != TypeKind_Metadata)
			return 28;
		WindowsDeleteString(read.Name);
		WindowsDeleteString(put.Name);
		if (style_iface->lpVtbl->get_IsSealed(style_iface, &sealed_flag) != S_OK || sealed_flag)
			return 29;
	}
	/* the Background setter carries a colour-name string the way the compiled markup spells one */
	if (!(red = make_box(L"Red")))
		return 30;
	if (add_setter(style, L"Background", (UINT32)lstrlenW(L"Background"), (IInspectable *)red) != S_OK)
		return 31;
	/* the Setters collection reflects the stored pair */
	{
		void *setters = NULL;
		setter_vector_i *vector = NULL;
		UINT32 size = 0;
		if (style_iface->lpVtbl->get_Setters(style_iface, (uint64_t)(uintptr_t)&setters) != S_OK || !setters)
			return 32;
		vector = (setter_vector_i *)setters;
		if (vector->lpVtbl->get_Size(vector, &size) != S_OK || size != 1)
			return 33;
		vector->lpVtbl->Release(vector);
	}
	/* the styled grid: its own background is unset, so hosting it first paints nothing */
	if (activate(grid_name, &grid_styled) != S_OK || !grid_styled)
		return 34;
	if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)grid_styled) != S_OK)
		return 35;
	if (color_at(hwnd, 5, 5, RGB(0, 0, 0)))
		return 36;
	/* putting the style on the hosted grid applies the setter to the grid's own storage and re-renders it red */
	if (put_style(grid_styled, style) != S_OK)
		return 37;
	{
		if (get_style(grid_styled, &roundtrip) != S_OK || !roundtrip)
			return 38;
		if (!same_unknown(roundtrip, style))
			return 39;
		roundtrip->lpVtbl->Release(roundtrip);
		roundtrip = NULL;
	}
	if (color_at(hwnd, 5, 5, RGB(255, 0, 0)))
		return 40;
	if (color_at(hwnd, 300, 200, RGB(255, 0, 0)))
		return 41;
	/* put_Style(NULL) clears the reference; the engine re-applies on put and does not revert, so the applied red stays */
	if (put_style(grid_styled, NULL) != S_OK)
		return 42;
	if (color_at(hwnd, 5, 5, RGB(255, 0, 0)))
		return 43;
	if (get_style(grid_styled, &roundtrip) != S_OK || roundtrip)
		return 44;
	/* the style round-trips through a ResourceDictionary under a string key and still applies */
	if (activate(dictionary_name, &dictionary) != S_OK || !dictionary)
		return 45;
	if (dictionary->lpVtbl->QueryInterface(dictionary, &IID___FIMap_2_IInspectable_IInspectable, (void **)&map) != S_OK ||
		!map)
		return 46;
	if (!(key = make_box(L"style.key")) || !(probe = make_box(L"style.key")))
		return 47;
	{
		boolean replaced = TRUE;
		if (map->lpVtbl->Insert(map, (IInspectable *)key, style, &replaced) != S_OK || replaced)
			return 48;
	}
	if (map->lpVtbl->Lookup(map, (IInspectable *)probe, &found) != S_OK || !found)
		return 49;
	if (!same_unknown(found, style))
		return 50;
	/* the looked-up style applies to the element the same way */
	if (put_style(grid_styled, found) != S_OK)
		return 51;
	if (color_at(hwnd, 5, 5, RGB(255, 0, 0)))
		return 52;
	/* the replacement style carries Background "Blue" AND FontSize 32: the grid takes the Background, and once a text
	 * block child exists the same style's FontSize reaches the block's real font size */
	if (activate(style_name, &style2) != S_OK || !style2)
		return 53;
	if (!(blue = make_box(L"Blue")))
		return 54;
	if (add_setter(style2, L"Background", (UINT32)lstrlenW(L"Background"), (IInspectable *)blue) != S_OK)
		return 55;
	if (!(size32 = make_number(32.0)))
		return 56;
	if (add_setter(style2, L"FontSize", (UINT32)lstrlenW(L"FontSize"), (IInspectable *)size32) != S_OK)
		return 57;
	/* the text block child: its own Text through the real slot, appended to the hosted grid the way the loader does */
	if (activate(textblock_name, &textblock) != S_OK || !textblock)
		return 58;
	if (textblock->lpVtbl->QueryInterface(textblock, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock,
		(void **)&text_iface) != S_OK || !text_iface)
		return 59;
	{
		HSTRING text_value = NULL;
		static const WCHAR text_w[] = L"Hello";
		if (WindowsCreateString(text_w, ARRAYSIZE(text_w) - 1, &text_value) != S_OK)
			return 60;
		if (text_iface->lpVtbl->put_Text(text_iface, (uint64_t)(uintptr_t)text_value) != S_OK) {
			WindowsDeleteString(text_value);
			return 60;
		}
		WindowsDeleteString(text_value);
	}
	if (grid_styled->lpVtbl->QueryInterface(grid_styled, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
		(void **)&styled_panel) != S_OK || !styled_panel)
		return 61;
	if (styled_panel->lpVtbl->get_Children(styled_panel, (uint64_t)(uintptr_t)&children) != S_OK || !children)
		return 62;
	if (((IInspectable *)children)->lpVtbl->QueryInterface((IInspectable *)children, &IID___FIVector_1_UIElement,
		(void **)&child_vector) != S_OK || !child_vector)
		return 63;
	if (child_vector->lpVtbl->Append(child_vector, textblock) != S_OK)
		return 64;
	/* the get_Children reference is dropped here; the QI'd vector's own reference releases at the cleanup */
	((IInspectable *)children)->lpVtbl->Release((IInspectable *)children);
	children = NULL;
	/* the replacement style on the grid: Background blue (the FontSize setter has no grid storage and is skipped) */
	if (put_style(grid_styled, style2) != S_OK)
		return 65;
	/* the pixel checks stay clear of the text block's run at the grid origin */
	if (color_at(hwnd, 300, 200, RGB(0, 0, 255)))
		return 66;
	if (color_at(hwnd, 600, 500, RGB(0, 0, 255)))
		return 67;
	/* the child's text renders at its default size over the blue */
	count14 = count_foreground(hwnd, 0, 0, 220, 40, RGB(0, 0, 0));
	if (!count14)
		return 68;
	/* the same style on the text block: FontSize 32 lands in the real font size, and the re-render re-measures the run */
	if (put_style(textblock, style2) != S_OK)
		return 69;
	if (text_iface->lpVtbl->get_FontSize(text_iface, (uint64_t)(uintptr_t)&font_size) != S_OK || font_size != 32.0f)
		return 70;
	/* bounded, the way the text fixture asserts its extent: the face the host font service picks decides the exact run,
	 * so the re-measure is a growth assertion, not a pixel */
	count32 = count_foreground(hwnd, 0, 0, 220, 40, RGB(0, 0, 0));
	if (count32 <= count14)
		return 71;
	/* BasedOn: a base that sets Background "Red" and FontSize 14, a derived style overriding only the FontSize with 32.
	 * Both apply — the base's Background reaches the grid through the chain — and the derived FontSize wins. */
	if (activate(style_name, &base_style) != S_OK || !base_style)
		return 72;
	if (!(red2 = make_box(L"Red")))
		return 73;
	if (add_setter(base_style, L"Background", (UINT32)lstrlenW(L"Background"), (IInspectable *)red2) != S_OK)
		return 74;
	if (!(size14 = make_number(14.0)))
		return 75;
	if (add_setter(base_style, L"FontSize", (UINT32)lstrlenW(L"FontSize"), (IInspectable *)size14) != S_OK)
		return 76;
	if (activate(style_name, &derived_style) != S_OK || !derived_style)
		return 77;
	if (derived_style->lpVtbl->QueryInterface(derived_style, &IID__x_ABI_CWindows_CUI_CXaml_CIStyle,
		(void **)&derived_iface) != S_OK || !derived_iface)
		return 78;
	if (derived_iface->lpVtbl->put_BasedOn(derived_iface, (uint64_t)(uintptr_t)base_style) != S_OK)
		return 79;
	if (add_setter(derived_style, L"FontSize", (UINT32)lstrlenW(L"FontSize"), (IInspectable *)size32) != S_OK)
		return 80;
	/* the derived style on the grid: the base's Background applies through the chain */
	if (put_style(grid_styled, derived_style) != S_OK)
		return 81;
	if (color_at(hwnd, 300, 200, RGB(255, 0, 0)))
		return 82;
	/* the derived style on the text block: the base's 14 applies first, the derived 32 lands after and wins */
	if (put_style(textblock, derived_style) != S_OK)
		return 83;
	if (text_iface->lpVtbl->get_FontSize(text_iface, (uint64_t)(uintptr_t)&font_size) != S_OK || font_size != 32.0f)
		return 84;
	/* a wrong-kind value fails the application honestly: a NUMBER where a colour is wanted makes put_Style answer
	 * E_INVALIDARG, and nothing lands — the pixel keeps the style it had and get_Style still answers it */
	if (activate(style_name, &bad_style) != S_OK || !bad_style)
		return 85;
	if (!(seven = make_number(7.0)))
		return 86;
	if (add_setter(bad_style, L"Background", (UINT32)lstrlenW(L"Background"), (IInspectable *)seven) != S_OK)
		return 87;
	if (put_style(grid_styled, bad_style) != E_INVALIDARG)
		return 88;
	if (color_at(hwnd, 300, 200, RGB(255, 0, 0)))
		return 89;
	if (get_style(grid_styled, &roundtrip) != S_OK || !roundtrip)
		return 90;
	if (!same_unknown(roundtrip, derived_style))
		return 91;
	roundtrip->lpVtbl->Release(roundtrip);
	roundtrip = NULL;
	/* the mirror case: a STRING where a number is wanted fails the text block's put the same way */
	if (activate(style_name, &bad_style2) != S_OK || !bad_style2)
		return 92;
	if (!(blue2 = make_box(L"Blue")))
		return 93;
	if (add_setter(bad_style2, L"FontSize", (UINT32)lstrlenW(L"FontSize"), (IInspectable *)blue2) != S_OK)
		return 94;
	if (put_style(textblock, bad_style2) != E_INVALIDARG)
		return 95;
	if (text_iface->lpVtbl->get_FontSize(text_iface, (uint64_t)(uintptr_t)&font_size) != S_OK || font_size != 32.0f)
		return 96;
	/* a property the element has no storage for is skipped, not failed: a style of only such setters applies with no
	 * effect on the pixels */
	if (activate(style_name, &skip_style) != S_OK || !skip_style)
		return 97;
	if (!(three = make_number(3.0)))
		return 98;
	if (add_setter(skip_style, L"Opacity", (UINT32)lstrlenW(L"Opacity"), (IInspectable *)three) != S_OK)
		return 99;
	if (put_style(grid_styled, skip_style) != S_OK)
		return 100;
	if (color_at(hwnd, 300, 200, RGB(255, 0, 0)))
		return 101;
	/* the plain grid kept its own background the whole time: hosting it again paints blue, unstyled */
	if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)grid_plain) != S_OK)
		return 102;
	if (color_at(hwnd, 5, 5, RGB(0, 0, 255)))
		return 103;
	if (color_at(hwnd, 300, 200, RGB(0, 0, 255)))
		return 104;
	/* The authored default styles. The engine authors one sealed Style per control class from the same setter path the
	 * rounds above use, and the constructor attaches it: a fresh control already answers get_Style with the engine's
	 * own object and a later put_Style replaces it. The button first: the pickup is automatic — no explicit attach —
	 * and the default carries the authored Background pair; the explicit attach entry point re-puts the same default. */
	if (activate(button_name, &default_button) != S_OK || !default_button)
		return 106;
	if (get_style(default_button, &roundtrip) != S_OK || !roundtrip)
		return 107;
	if (default_attach(default_button) != S_OK)
		return 108;
	{
		IInspectable *reput = NULL;
		if (get_style(default_button, &reput) != S_OK || reput != roundtrip)
			return 109;
		reput->lpVtbl->Release(reput);
	}
	{
		UINT32 pairs = 0;
		const WCHAR *property = NULL;
		UINT32 length = 0;
		IInspectable *value = NULL;
		boxed_string_i *string_box = NULL;
		HSTRING text_value = NULL;
		if (setter_count(roundtrip, &pairs) != S_OK || pairs != 1)
			return 110;
		if (setter_at(roundtrip, 0, &property, &length, &value) != S_OK || !property || !value)
			return 111;
		if (!same_raw(property, length, L"Background"))
			return 112;
		if (value->lpVtbl->QueryInterface(value, &IID___FIReference_1_HSTRING, (void **)&string_box) != S_OK ||
			!string_box)
			return 113;
		if (string_box->lpVtbl->get_Value(string_box, &text_value) != S_OK || !text_value)
			return 114;
		if (!same_text(text_value, L"Transparent"))
			return 115;
		WindowsDeleteString(text_value);
		string_box->lpVtbl->Release(string_box);
		value->lpVtbl->Release(value);
	}
	roundtrip->lpVtbl->Release(roundtrip);
	roundtrip = NULL;
	/* the text block default: the constructor's own FontSize 14 and black foreground, authored as setter pairs and
	 * attached by the constructor the same way */
	if (activate(textblock_name, &default_textblock) != S_OK || !default_textblock)
		return 116;
	if (default_textblock->lpVtbl->QueryInterface(default_textblock,
		&IID__x_ABI_CWindows_CUI_CXaml_CControls_CITextBlock, (void **)&default_text_iface) != S_OK ||
		!default_text_iface)
		return 117;
	{
		HSTRING text_value = NULL;
		static const WCHAR text_w[] = L"Default";
		if (WindowsCreateString(text_w, ARRAYSIZE(text_w) - 1, &text_value) != S_OK)
			return 118;
		if (default_text_iface->lpVtbl->put_Text(default_text_iface, (uint64_t)(uintptr_t)text_value) != S_OK) {
			WindowsDeleteString(text_value);
			return 118;
		}
		WindowsDeleteString(text_value);
	}
	if (get_style(default_textblock, &roundtrip) != S_OK || !roundtrip)
		return 120;
	{
		UINT32 pairs = 0;
		int font_size_seen = 0, foreground_seen = 0;
		if (setter_count(roundtrip, &pairs) != S_OK || pairs != 2)
			return 121;
		for (UINT32 index = 0; index < pairs; ++index) {
			const WCHAR *property = NULL;
			UINT32 length = 0;
			IInspectable *value = NULL;
			if (setter_at(roundtrip, index, &property, &length, &value) != S_OK || !property || !value)
				return 122;
			if (same_raw(property, length, L"FontSize")) {
				boxed_number_i *number_box = NULL;
				double read = 0;
				if (value->lpVtbl->QueryInterface(value, &IID___FIReference_1_DOUBLE, (void **)&number_box) != S_OK ||
					!number_box)
					return 123;
				if (number_box->lpVtbl->get_Value(number_box, &read) != S_OK || read != 14.0)
					return 124;
				number_box->lpVtbl->Release(number_box);
				font_size_seen = 1;
			} else if (same_raw(property, length, L"Foreground")) {
				boxed_string_i *string_box = NULL;
				HSTRING text_value = NULL;
				if (value->lpVtbl->QueryInterface(value, &IID___FIReference_1_HSTRING, (void **)&string_box) != S_OK ||
					!string_box)
					return 125;
				if (string_box->lpVtbl->get_Value(string_box, &text_value) != S_OK || !text_value)
					return 126;
				if (!same_text(text_value, L"Black"))
					return 127;
				WindowsDeleteString(text_value);
				string_box->lpVtbl->Release(string_box);
				foreground_seen = 1;
			}
			value->lpVtbl->Release(value);
		}
		if (!font_size_seen || !foreground_seen)
			return 128;
	}
	/* the authored defaults render: the default text block appended to the hosted blue grid draws its black 14pt run */
	{
		void *children = NULL;
		struct fixture_children *vector = NULL;
		if (plain_panel->lpVtbl->get_Children(plain_panel, (uint64_t)(uintptr_t)&children) != S_OK || !children)
			return 129;
		if (((IInspectable *)children)->lpVtbl->QueryInterface((IInspectable *)children, &IID___FIVector_1_UIElement,
			(void **)&vector) != S_OK || !vector) {
			((IInspectable *)children)->lpVtbl->Release((IInspectable *)children);
			return 130;
		}
		((IInspectable *)children)->lpVtbl->Release((IInspectable *)children);
		if (vector->lpVtbl->Append(vector, default_textblock) != S_OK) {
			vector->lpVtbl->Release(vector);
			return 131;
		}
		vector->lpVtbl->Release(vector);
		/* the children append does not invalidate by itself — only the definition vectors re-render on append — so the
		 * re-host presents the new child, the way the engine's own puts re-render */
		if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)grid_plain) != S_OK)
			return 131;
	}
	count_default = count_foreground(hwnd, 0, 0, 220, 40, RGB(0, 0, 0));
	if (!count_default)
		return 132;
	/* an explicit put_Style overrides the default: get_Style answers the app's style and the value follows it */
	if (activate(style_name, &override_style) != S_OK || !override_style)
		return 133;
	if (!(size8 = make_number(8.0)))
		return 134;
	if (add_setter(override_style, L"FontSize", (UINT32)lstrlenW(L"FontSize"), (IInspectable *)size8) != S_OK)
		return 135;
	if (put_style(default_textblock, override_style) != S_OK)
		return 136;
	if (get_style(default_textblock, &roundtrip) != S_OK || !roundtrip)
		return 137;
	if (!same_unknown(roundtrip, override_style))
		return 138;
	roundtrip->lpVtbl->Release(roundtrip);
	roundtrip = NULL;
	if (default_text_iface->lpVtbl->get_FontSize(default_text_iface, (uint64_t)(uintptr_t)&font_size) != S_OK ||
		font_size != 8.0f)
		return 139;
	count_override = count_foreground(hwnd, 0, 0, 220, 40, RGB(0, 0, 0));
	if (!count_override || count_override >= count_default)
		return 140;
	/* put_Style(NULL) clears as before: no style answers, the engine does not revert, and the run re-renders unchanged */
	if (put_style(default_textblock, NULL) != S_OK)
		return 141;
	if (get_style(default_textblock, &roundtrip) != S_OK || roundtrip)
		return 142;
	if (default_text_iface->lpVtbl->get_FontSize(default_text_iface, (uint64_t)(uintptr_t)&font_size) != S_OK ||
		font_size != 8.0f)
		return 143;
	/* The DependencyProperty stage. The statics come through the engine-internal export and — now that the class is
	 * cataloged — through RoGetActivationFactory: both hands resolve the same singleton the activation chain reaches. */
	dependency_statics_function dependency_statics = component ?
		(dependency_statics_function)GetProcAddress(component, "raven_xaml_dependency_statics") : NULL;
	setter_create_function setter_create = component ?
		(setter_create_function)GetProcAddress(component, "raven_xaml_setter_create") : NULL;
	if (!dependency_statics || !setter_create)
		return 144;
	if (dependency_statics(&dp_statics) != S_OK || !dp_statics)
		return 145;
	{
		HSTRING dp_key = NULL;
		static const WCHAR dp_class[] = L"Windows.UI.Xaml.DependencyProperty";
		dp_statics_i *chained = NULL;
		if (WindowsCreateString(dp_class, ARRAYSIZE(dp_class) - 1, &dp_key) != S_OK)
			return 145;
		if (get_factory(dp_key, &IID___x_ABI_CWindows_CUI_CXaml_CIDependencyPropertyStatics, (void **)&chained) != S_OK ||
			chained != dp_statics) {
			WindowsDeleteString(dp_key);
			return 145;
		}
		WindowsDeleteString(dp_key);
		dp_statics->lpVtbl->Release(chained);
	}
	/* the metadata objects the registrations take their default values from: hand-rolled IPropertyMetadata the way the
	 * boxes above are hand-rolled IReference`1, because this increment implements DependencyProperty, not
	 * PropertyMetadata */
	if (!(font_default = make_number(24.0)))
		return 146;
	if (!(font_metadata = make_metadata((IInspectable *)font_default)))
		return 147;
	if (!(bg_default = make_box(L"Green")))
		return 148;
	if (!(bg_metadata = make_metadata((IInspectable *)bg_default)))
		return 149;
	if (WindowsCreateString(L"FontSize", (UINT32)lstrlenW(L"FontSize"), &font_key) != S_OK)
		return 150;
	if (WindowsCreateString(L"Background", (UINT32)lstrlenW(L"Background"), &bg_key) != S_OK)
		return 151;
	/* Register: the TextBlock-owned FontSize, whose default the metadata's real DefaultValue slot carries */
	{
		type_name_s property_type, owner_type;
		HRESULT result;
		if (WindowsCreateString(L"Double", (UINT32)lstrlenW(L"Double"), &property_type.Name) != S_OK)
			return 152;
		property_type.Kind = TypeKind_Primitive;
		if (WindowsCreateString(textblock_name, ARRAYSIZE(textblock_name) - 1, &owner_type.Name) != S_OK) {
			WindowsDeleteString(property_type.Name);
			return 152;
		}
		owner_type.Kind = TypeKind_Metadata;
		result = dp_statics->lpVtbl->Register(dp_statics, font_key, property_type, owner_type,
			(metadata_i *)&font_metadata->part, &font_dp);
		WindowsDeleteString(property_type.Name);
		WindowsDeleteString(owner_type.Name);
		if (result != S_OK || !font_dp)
			return 153;
	}
	/* the metadata readers round-trip the registration through the engine-internal interop part the engine object
	 * answers next to the real IDependencyProperty */
	{
		dp_interop_i *interop = NULL;
		HSTRING read = NULL;
		type_name_s type;
		IInspectable *value = NULL;
		if (((IInspectable *)font_dp)->lpVtbl->QueryInterface((IInspectable *)font_dp,
			&raven_dependency_property_interop_iid, (void **)&interop) != S_OK || !interop)
			return 154;
		if (interop->lpVtbl->get_Name(interop, &read) != S_OK || !read || !same_text(read, L"FontSize"))
			return 155;
		WindowsDeleteString(read);
		if (interop->lpVtbl->get_TypeName(interop, &type) != S_OK || !type.Name)
			return 156;
		if (!same_text(type.Name, L"Double") || type.Kind != TypeKind_Primitive)
			return 157;
		WindowsDeleteString(type.Name);
		if (interop->lpVtbl->get_OwnerType(interop, &type) != S_OK || !type.Name)
			return 158;
		if (!same_text(type.Name, textblock_name) || type.Kind != TypeKind_Metadata)
			return 159;
		WindowsDeleteString(type.Name);
		if (interop->lpVtbl->get_DefaultValue(interop, &value) != S_OK || !value)
			return 160;
		if (!same_unknown(value, (IInspectable *)font_default))
			return 161;
		value->lpVtbl->Release(value);
		interop->lpVtbl->Release(interop);
	}
	/* the button-owned Background with the string default */
	{
		type_name_s property_type, owner_type;
		HRESULT result;
		if (WindowsCreateString(L"String", (UINT32)lstrlenW(L"String"), &property_type.Name) != S_OK)
			return 162;
		property_type.Kind = TypeKind_Primitive;
		if (WindowsCreateString(button_name, ARRAYSIZE(button_name) - 1, &owner_type.Name) != S_OK) {
			WindowsDeleteString(property_type.Name);
			return 162;
		}
		owner_type.Kind = TypeKind_Metadata;
		result = dp_statics->lpVtbl->Register(dp_statics, bg_key, property_type, owner_type,
			(metadata_i *)&bg_metadata->part, &background_dp);
		WindowsDeleteString(property_type.Name);
		WindowsDeleteString(owner_type.Name);
		if (result != S_OK || !background_dp)
			return 163;
	}
	/* the duplicate contract: the same (owner, name) refuses, the same name under a different owner registers (no
	 * metadata: the default stays NULL) */
	{
		type_name_s property_type, owner_type;
		HRESULT result;
		if (WindowsCreateString(L"Double", (UINT32)lstrlenW(L"Double"), &property_type.Name) != S_OK)
			return 164;
		property_type.Kind = TypeKind_Primitive;
		if (WindowsCreateString(textblock_name, ARRAYSIZE(textblock_name) - 1, &owner_type.Name) != S_OK) {
			WindowsDeleteString(property_type.Name);
			return 164;
		}
		owner_type.Kind = TypeKind_Metadata;
		result = dp_statics->lpVtbl->Register(dp_statics, font_key, property_type, owner_type, NULL, &grid_dp);
		WindowsDeleteString(property_type.Name);
		WindowsDeleteString(owner_type.Name);
		if (result != E_INVALIDARG)
			return 165;
		if (WindowsCreateString(L"Double", (UINT32)lstrlenW(L"Double"), &property_type.Name) != S_OK ||
			WindowsCreateString(grid_name, ARRAYSIZE(grid_name) - 1, &owner_type.Name) != S_OK)
			return 166;
		owner_type.Kind = TypeKind_Metadata;
		result = dp_statics->lpVtbl->Register(dp_statics, font_key, property_type, owner_type, NULL, &grid_dp);
		WindowsDeleteString(property_type.Name);
		WindowsDeleteString(owner_type.Name);
		if (result != S_OK || !grid_dp)
			return 167;
	}
	/* Attached registrations retain a distinct name and obey the same duplicate contract. */
	{
		type_name_s property_type, owner_type;
		HSTRING attached_name = NULL;
		dp_i *attached_dp = NULL;
		HRESULT result;
		if (WindowsCreateString(L"FixtureAttached", 15, &attached_name) != S_OK) return 168;
		if (WindowsCreateString(L"Double", (UINT32)lstrlenW(L"Double"), &property_type.Name) != S_OK)
			return 168;
		property_type.Kind = TypeKind_Primitive;
		if (WindowsCreateString(grid_name, ARRAYSIZE(grid_name) - 1, &owner_type.Name) != S_OK) {
			WindowsDeleteString(property_type.Name);
			return 168;
		}
		owner_type.Kind = TypeKind_Metadata;
		result = dp_statics->lpVtbl->RegisterAttached(dp_statics, attached_name, property_type, owner_type, NULL, &attached_dp);
		if (result != S_OK || !attached_dp) return 169;
		attached_dp->lpVtbl->Release(attached_dp);
		attached_dp = NULL;
		result = dp_statics->lpVtbl->RegisterAttached(dp_statics, attached_name, property_type, owner_type, NULL, &attached_dp);
		WindowsDeleteString(attached_name);
		WindowsDeleteString(property_type.Name);
		WindowsDeleteString(owner_type.Name);
		if (result != E_INVALIDARG || attached_dp)
			return 169;
	}
	/* UnsetValue: one identity-stable sentinel across the calls */
	if (dp_statics->lpVtbl->get_UnsetValue(dp_statics, &dp_unset1) != S_OK || !dp_unset1)
		return 170;
	if (dp_statics->lpVtbl->get_UnsetValue(dp_statics, &dp_unset2) != S_OK || !dp_unset2)
		return 171;
	if (dp_statics->lpVtbl->get_UnsetValue(dp_statics, &dp_unset2) != S_OK || !dp_unset2)
		return 171;
	if (!same_unknown(dp_unset1, dp_unset2))
		return 172;
	dp_unset1->lpVtbl->Release(dp_unset1);
	dp_unset2->lpVtbl->Release(dp_unset2);
	dp_unset1 = NULL;
	dp_unset2 = NULL;
	/* the Setter carries the DP: put_Property takes the engine object and copies its registered name into the stored
	 * pair, so the application walks a DP setter through the very same (name, value) store the name-string setters
	 * fill, and put_Style applies it the way it applied every round above */
	if (setter_create(&setter_object) != S_OK || !setter_object)
		return 173;
	if (setter_object->lpVtbl->QueryInterface(setter_object, &IID__x_ABI_CWindows_CUI_CXaml_CISetter,
		(void **)&dp_setter) != S_OK || !dp_setter)
		return 174;
	if (dp_setter->lpVtbl->put_Property(dp_setter, (uint64_t)(uintptr_t)background_dp) != S_OK)
		return 175;
	if (dp_setter->lpVtbl->get_Property(dp_setter, (uint64_t)(uintptr_t)&property_read) != S_OK || !property_read)
		return 176;
	if (!same_unknown(property_read, background_dp))
		return 177;
	property_read->lpVtbl->Release(property_read);
	property_read = NULL;
	/* a foreign "property" — an object that is no engine DependencyProperty — is refused: it carries no registration
	 * the engine could read a property name from */
	if (dp_setter->lpVtbl->put_Property(dp_setter, (uint64_t)(uintptr_t)style) != E_INVALIDARG)
		return 178;
	if (!(dp_blue = make_box(L"Blue")))
		return 179;
	if (dp_setter->lpVtbl->put_Value(dp_setter, (uint64_t)(uintptr_t)dp_blue) != S_OK)
		return 180;
	/* the DP setter appended through the real Setters vector of a fresh style */
	if (activate(style_name, &style3) != S_OK || !style3)
		return 181;
	if (style3->lpVtbl->QueryInterface(style3, &IID__x_ABI_CWindows_CUI_CXaml_CIStyle, (void **)&style3_iface) != S_OK ||
		!style3_iface)
		return 182;
	if (style3_iface->lpVtbl->get_Setters(style3_iface, (uint64_t)(uintptr_t)&style3_setters) != S_OK ||
		!style3_setters)
		return 183;
	if (style3_setters->lpVtbl->Append(style3_setters, (__x_ABI_CWindows_CUI_CXaml_CISetterBase *)dp_setter) != S_OK)
		return 184;
	/* hosting the styled grid again and putting the DP style: the pixel follows the DP setter the way it follows the
	 * name-string round at the top, and the pair store hands the setter out under its registered name */
	if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)grid_styled) != S_OK)
		return 185;
	if (put_style(grid_styled, style3) != S_OK)
		return 186;
	if (color_at(hwnd, 300, 200, RGB(0, 0, 255)))
		return 187;
	if (color_at(hwnd, 600, 500, RGB(0, 0, 255)))
		return 188;
	{
		UINT32 pairs = 0;
		const WCHAR *property = NULL;
		UINT32 length = 0;
		IInspectable *value = NULL;
		if (setter_count(style3, &pairs) != S_OK || pairs != 1)
			return 189;
		if (setter_at(style3, 0, &property, &length, &value) != S_OK || !property || !value)
			return 190;
		if (!same_raw(property, length, L"Background"))
			return 191;
		value->lpVtbl->Release(value);
	}
	/* the text block round: the TextBlock-owned FontSize DP setter lands in the real font size through the same
	 * application, the owner recorded at registration NOT being checked (the name drives the setter) */
	if (activate(style_name, &style4) != S_OK || !style4)
		return 192;
	if (setter_create(&setter_object2) != S_OK || !setter_object2)
		return 193;
	if (setter_object2->lpVtbl->QueryInterface(setter_object2, &IID__x_ABI_CWindows_CUI_CXaml_CISetter,
		(void **)&dp_setter2) != S_OK || !dp_setter2)
		return 194;
	if (dp_setter2->lpVtbl->put_Property(dp_setter2, (uint64_t)(uintptr_t)font_dp) != S_OK)
		return 195;
	if (!(forty = make_number(40.0)))
		return 196;
	if (dp_setter2->lpVtbl->put_Value(dp_setter2, (uint64_t)(uintptr_t)forty) != S_OK)
		return 197;
	if (dp_setter2->lpVtbl->get_Property(dp_setter2, (uint64_t)(uintptr_t)&property_read2) != S_OK || !property_read2)
		return 198;
	if (!same_unknown(property_read2, font_dp))
		return 199;
	property_read2->lpVtbl->Release(property_read2);
	property_read2 = NULL;
	if (style4->lpVtbl->QueryInterface(style4, &IID__x_ABI_CWindows_CUI_CXaml_CIStyle, (void **)&style4_iface) != S_OK ||
		!style4_iface)
		return 200;
	if (style4_iface->lpVtbl->get_Setters(style4_iface, (uint64_t)(uintptr_t)&style4_setters) != S_OK ||
		!style4_setters)
		return 201;
	if (style4_setters->lpVtbl->Append(style4_setters, (__x_ABI_CWindows_CUI_CXaml_CISetterBase *)dp_setter2) != S_OK)
		return 202;
	if (put_style(textblock, style4) != S_OK)
		return 203;
	if (text_iface->lpVtbl->get_FontSize(text_iface, (uint64_t)(uintptr_t)&font_size) != S_OK || font_size != 40.0f)
		return 204;
	/* The PropertyMetadata stage: the class DependencyProperty.Register takes its metadata from is itself activatable,
	 * and its factory is the one WinUI 2.8 asks for first. The composable factory carries both constructor forms, the
	 * statics carry PropertyMetadata.Create, the factory-backed form holds a create-default-value delegate the engine
	 * invokes through its internal reader, the agile marker is answered on the factory and metadata identities, and
	 * the documented refusals stay refusals. */
	{
		static const WCHAR metadata_class[] = L"Windows.UI.Xaml.PropertyMetadata";
		HSTRING metadata_key = NULL;
		__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactory *metadata_factory = NULL;
		__x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics *metadata_statics = NULL;
		IActivationFactory *metadata_activation = NULL;
		IInspectable *agile = NULL;
		IInspectable *number = (IInspectable *)make_number(7.5);
		IInspectable *read_default = NULL;
		metadata_i *meta = NULL, *meta2 = NULL, *meta_factory_backed = NULL;
		raven_property_metadata_interop *meta_interop = NULL;
		__x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback *read_factory = NULL;
		struct fixture_create_cb *callback = NULL;
		if (WindowsCreateString(metadata_class, ARRAYSIZE(metadata_class) - 1, &metadata_key) != S_OK)
			return 210;
		if (get_factory(metadata_key, &IID___x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataFactory,
			(void **)&metadata_factory) != S_OK || !metadata_factory)
			return 211;
		WindowsDeleteString(metadata_key);
		if (metadata_factory->lpVtbl->QueryInterface(metadata_factory, &IID_IAgileObject, (void **)&agile) != S_OK ||
			!agile)
			return 212;
		agile->lpVtbl->Release(agile);
		agile = NULL;
		/* the class has no parameterless constructor, so activation has nothing to run */
		if (metadata_factory->lpVtbl->QueryInterface(metadata_factory, &IID_IActivationFactory,
			(void **)&metadata_activation) != S_OK || !metadata_activation)
			return 213;
		if (metadata_activation->lpVtbl->ActivateInstance(metadata_activation, &agile) != E_NOTIMPL || agile)
			return 214;
		metadata_activation->lpVtbl->Release(metadata_activation);
		metadata_activation = NULL;
		if (!number)
			return 215;
		/* PropertyMetadata(Object): the fixed default round-trips by identity, no factory is carried */
		if (metadata_factory->lpVtbl->CreateInstanceWithDefaultValue(metadata_factory, number, NULL, NULL, &meta) != S_OK ||
			!meta)
			return 216;
		if (meta->lpVtbl->get_DefaultValue(meta, &read_default) != S_OK || read_default != number)
			return 217;
		read_default->lpVtbl->Release(read_default);
		read_default = NULL;
		if (meta->lpVtbl->get_CreateDefaultValueCallback(meta, &read_factory) != S_OK || read_factory)
			return 218;
		if (meta->lpVtbl->QueryInterface(meta, &IID_IAgileObject, (void **)&agile) != S_OK || !agile)
			return 219;
		agile->lpVtbl->Release(agile);
		agile = NULL;
		/* PropertyMetadata.Create statics: the NULL default is a legal fixed default */
		if (metadata_factory->lpVtbl->QueryInterface(metadata_factory,
			&IID___x_ABI_CWindows_CUI_CXaml_CIPropertyMetadataStatics, (void **)&metadata_statics) != S_OK ||
			!metadata_statics)
			return 220;
		if (metadata_statics->lpVtbl->CreateWithDefaultValue(metadata_statics, NULL, &meta2) != S_OK || !meta2)
			return 221;
		if (meta2->lpVtbl->get_DefaultValue(meta2, &read_default) != S_OK || read_default)
			return 222;
		/* the factory-backed form: only the create-default-value reading is set, and the engine's internal reader
		 * invokes the delegate exactly then (the engine's registration store keeps one default per property, so the
		 * real per-instance lazy default is the one documented approximation) */
		callback = make_create_cb(number);
		if (!callback)
			return 223;
		if (metadata_statics->lpVtbl->CreateWithFactory(metadata_statics,
			(__x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback *)callback, &meta_factory_backed) != S_OK ||
			!meta_factory_backed)
			return 224;
		if (meta_factory_backed->lpVtbl->get_DefaultValue(meta_factory_backed, &read_default) != S_OK || read_default)
			return 225;
		if (meta_factory_backed->lpVtbl->get_CreateDefaultValueCallback(meta_factory_backed, &read_factory) != S_OK ||
			(struct fixture_create_cb *)read_factory != callback)
			return 226;
		read_factory->lpVtbl->Release(read_factory);
		read_factory = NULL;
		if (meta_factory_backed->lpVtbl->QueryInterface(meta_factory_backed, &raven_property_metadata_interop_iid,
			(void **)&meta_interop) != S_OK || !meta_interop)
			return 227;
		if (meta_interop->lpVtbl->CreateDefaultValue(meta_interop, &read_default) != S_OK || read_default != number ||
			callback->invoked != 1)
			return 228;
		read_default->lpVtbl->Release(read_default);
		read_default = NULL;
		meta_interop->lpVtbl->Release(meta_interop);
		meta_interop = NULL;
		/* the refusals: no factory to hold, a callback that is not the delegate, a base interface needing
		 * aggregation — each out pointer is its own variable so the successful meta2 above stays referenced */
		{
			metadata_i *refused = (metadata_i *)1;
			if (metadata_statics->lpVtbl->CreateWithFactory(metadata_statics, NULL, &refused) != E_INVALIDARG ||
				refused)
				return 229;
			refused = (metadata_i *)1;
			if (metadata_factory->lpVtbl->CreateInstanceWithDefaultValueAndCallback(metadata_factory, number,
				(__x_ABI_CWindows_CUI_CXaml_CIPropertyChangedCallback *)number, NULL, NULL, &refused) != E_INVALIDARG ||
				refused)
				return 230;
			refused = (metadata_i *)1;
			if (metadata_factory->lpVtbl->CreateInstanceWithDefaultValue(metadata_factory, number, number, NULL,
				&refused) != E_NOTIMPL || refused)
				return 231;
		}
		/* the refcount ledger: the metadata objects drop to zero, the singleton keeps its base reference */
		if (meta_factory_backed->lpVtbl->Release(meta_factory_backed) != 0)
			return 232;
		meta_factory_backed = NULL;
		if (callback->lpVtbl->Release((__x_ABI_CWindows_CUI_CXaml_CICreateDefaultValueCallback *)callback) != 0)
			return 233;
		callback = NULL;
		if (meta->lpVtbl->Release(meta) != 0)
			return 234;
		meta = NULL;
		if (meta2->lpVtbl->Release(meta2) != 0)
			return 235;
		meta2 = NULL;
		if (metadata_statics->lpVtbl->Release(metadata_statics) != 2)
			return 236;
		metadata_statics = NULL;
		if (metadata_factory->lpVtbl->Release(metadata_factory) != 1)
			return 237;
		metadata_factory = NULL;
		((IInspectable *)number)->lpVtbl->Release(number);
		number = NULL;
	}
	/* The ControlTemplate stage: a Style setter named "Template" carries the ControlTemplate object itself, the button
	 * routes it to the same store put_Template serves, and the applied template's root tree replaces the control's
	 * rendered visuals — a template whose root is a red grid paints red where the unstyled button painted nothing. The
	 * wrong-kind value (a string where a template is wanted) fails put_Style with E_INVALIDARG and the element keeps
	 * its previous template; the replace and the clear follow the pixels. */
	if (control_template_factory_fn(&control_template_factory) != S_OK || !control_template_factory)
		return 300;
	if (control_template_factory->lpVtbl->ActivateInstance(control_template_factory, &template_red) != S_OK ||
		!template_red)
		return 301;
	{
		color_s red;
		color_s blue;
		red.A = 255;
		red.R = 255;
		red.G = 0;
		red.B = 0;
		blue.A = 255;
		blue.R = 0;
		blue.G = 0;
		blue.B = 255;
		if (compositor->lpVtbl->CreateColorBrushWithColor(compositor, red, &red_brush) != S_OK || !red_brush)
			return 302;
		if (compositor->lpVtbl->CreateColorBrushWithColor(compositor, blue, &blue_brush) != S_OK || !blue_brush)
			return 303;
	}
	if (activate(grid_name, &template_root_grid) != S_OK || !template_root_grid)
		return 304;
	{
		panel_i *root_panel = NULL;
		if (template_root_grid->lpVtbl->QueryInterface(template_root_grid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
			(void **)&root_panel) != S_OK || !root_panel)
			return 305;
		if (root_panel->lpVtbl->put_Background(root_panel, (uint64_t)(uintptr_t)red_brush) != S_OK) {
			root_panel->lpVtbl->Release(root_panel);
			return 306;
		}
		root_panel->lpVtbl->Release(root_panel);
	}
	if (template_put_root(template_red, template_root_grid) != S_OK)
		return 307;
	/* the template style: the extent setters size the button inside its cell, the Template setter carries the object */
	if (activate(style_name, &template_style) != S_OK || !template_style)
		return 308;
	{
		struct fixture_number *width400 = make_number(400.0);
		struct fixture_number *height300 = make_number(300.0);
		if (!width400 || !height300)
			return 309;
		if (add_setter(template_style, L"Width", (UINT32)lstrlenW(L"Width"), (IInspectable *)width400) != S_OK ||
			add_setter(template_style, L"Height", (UINT32)lstrlenW(L"Height"), (IInspectable *)height300) != S_OK ||
			add_setter(template_style, L"Template", (UINT32)lstrlenW(L"Template"), template_red) != S_OK) {
			fixture_number_release(width400);
			fixture_number_release(height300);
			return 310;
		}
		fixture_number_release(width400);
		fixture_number_release(height300);
	}
	/* the button on the hosted plain grid: without the template it paints nothing (the engine's control chrome does not
	 * render), the unstyled pixel is the plain grid's blue */
	if (activate(button_name, &template_button) != S_OK || !template_button)
		return 311;
	{
		void *children = NULL;
		struct fixture_children *vector = NULL;
		if (plain_panel->lpVtbl->get_Children(plain_panel, (uint64_t)(uintptr_t)&children) != S_OK || !children)
			return 312;
		if (((IInspectable *)children)->lpVtbl->QueryInterface((IInspectable *)children, &IID___FIVector_1_UIElement,
			(void **)&vector) != S_OK) {
			((IInspectable *)children)->lpVtbl->Release((IInspectable *)children);
			return 313;
		}
		((IInspectable *)children)->lpVtbl->Release((IInspectable *)children);
		const HRESULT appended = vector->lpVtbl->Append(vector, template_button);
		vector->lpVtbl->Release(vector);
		if (appended != S_OK)
			return 314;
		if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)grid_plain) != S_OK)
			return 315;
	}
	if (color_at(hwnd, 100, 100, RGB(0, 0, 255)))
		return 316;
	if (put_style(template_button, template_style) != S_OK)
		return 317;
	/* the template root's red fills the button's rectangle */
	if (color_at(hwnd, 100, 100, RGB(255, 0, 0)))
		return 318;
	if (color_at(hwnd, 200, 150, RGB(255, 0, 0)))
		return 319;
	/* outside the 400x300 button the plain grid's blue shows */
	if (color_at(hwnd, 600, 500, RGB(0, 0, 255)))
		return 320;
	/* the template round-trips through the element export */
	{
		IInspectable *read = NULL;
		if (get_template(template_button, &read) != S_OK || !read)
			return 321;
		if (!same_unknown(read, template_red)) {
			read->lpVtbl->Release(read);
			return 322;
		}
		read->lpVtbl->Release(read);
	}
	/* the wrong-kind Template setter fails the whole put: the red template stays */
	if (activate(style_name, &template_bad) != S_OK || !template_bad)
		return 323;
	if (add_setter(template_bad, L"Template", (UINT32)lstrlenW(L"Template"), (IInspectable *)red) != S_OK)
		return 324;
	if (put_style(template_button, template_bad) != E_INVALIDARG)
		return 325;
	if (color_at(hwnd, 100, 100, RGB(255, 0, 0)))
		return 326;
	if (get_style(template_button, &roundtrip) != S_OK || !roundtrip)
		return 327;
	if (!same_unknown(roundtrip, template_style)) {
		roundtrip->lpVtbl->Release(roundtrip);
		return 328;
	}
	roundtrip->lpVtbl->Release(roundtrip);
	roundtrip = NULL;
	/* the replace: a second template whose root paints blue, carried by a second style */
	if (control_template_factory->lpVtbl->ActivateInstance(control_template_factory, &template_blue) != S_OK ||
		!template_blue)
		return 329;
	if (activate(grid_name, &template_root_grid2) != S_OK || !template_root_grid2)
		return 330;
	{
		panel_i *root_panel = NULL;
		if (template_root_grid2->lpVtbl->QueryInterface(template_root_grid2,
			&IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel, (void **)&root_panel) != S_OK || !root_panel)
			return 331;
		if (root_panel->lpVtbl->put_Background(root_panel, (uint64_t)(uintptr_t)blue_brush) != S_OK) {
			root_panel->lpVtbl->Release(root_panel);
			return 332;
		}
		root_panel->lpVtbl->Release(root_panel);
	}
	if (template_put_root(template_blue, template_root_grid2) != S_OK)
		return 333;
	if (activate(style_name, &template_style2) != S_OK || !template_style2)
		return 334;
	if (add_setter(template_style2, L"Template", (UINT32)lstrlenW(L"Template"), template_blue) != S_OK)
		return 335;
	if (put_style(template_button, template_style2) != S_OK)
		return 336;
	if (color_at(hwnd, 100, 100, RGB(0, 0, 255)))
		return 337;
	/* get_Template answers the replacement */
	{
		IInspectable *read = NULL;
		if (get_template(template_button, &read) != S_OK || !read)
			return 338;
		if (!same_unknown(read, template_blue)) {
			read->lpVtbl->Release(read);
			return 339;
		}
		read->lpVtbl->Release(read);
	}
	/* the ContentPresenter round: a third template whose root grid is red and whose ContentPresenter shows the button's
	 * Content — a green 200x150 grid — so the content appears inside the template tree (green over the red root at the
	 * top-left, the root's red where the content does not reach). The presenter was bound to the button when the
	 * template was applied; the engine's presenter resolves the content through the same hook the render walk reads
	 * first, so both the measure and the paint pick it up without a render-path change. */
	{
		typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl content_control_i;
		typedef HRESULT (*presenter_factory_function)(IActivationFactory **);
		presenter_factory_function presenter_factory_fn = NULL;
		IActivationFactory *presenter_factory = NULL;
		IInspectable *template_presenter = NULL, *template_root_grid3 = NULL, *template_style3 = NULL;
		IInspectable *presenter_object = NULL, *content_grid = NULL, *content_style = NULL;
		brush_i *green_brush = NULL;
		color_s green;
		struct fixture_number *content_width = make_number(200.0);
		struct fixture_number *content_height = make_number(150.0);
		presenter_factory_fn = component ? (presenter_factory_function)GetProcAddress(component,
			"raven_xaml_content_presenter_factory") : NULL;
		if (!presenter_factory_fn)
			return 343;
		green.A = 255;
		green.R = 0;
		green.G = 255;
		green.B = 0;
		if (compositor->lpVtbl->CreateColorBrushWithColor(compositor, green, &green_brush) != S_OK || !green_brush)
			return 344;
		/* the content: a green 200x150 grid with its extent from the Width/Height setters, the button's Content */
		if (activate(grid_name, &content_grid) != S_OK || !content_grid)
			return 345;
		{
			panel_i *content_panel = NULL;
			if (content_grid->lpVtbl->QueryInterface(content_grid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
				(void **)&content_panel) != S_OK || !content_panel)
				return 346;
			if (content_panel->lpVtbl->put_Background(content_panel, (uint64_t)(uintptr_t)green_brush) != S_OK) {
				content_panel->lpVtbl->Release(content_panel);
				return 347;
			}
			content_panel->lpVtbl->Release(content_panel);
		}
		if (!content_width || !content_height)
			return 348;
		if (activate(style_name, &content_style) != S_OK || !content_style)
			return 349;
		if (add_setter(content_style, L"Width", (UINT32)lstrlenW(L"Width"), (IInspectable *)content_width) != S_OK ||
			add_setter(content_style, L"Height", (UINT32)lstrlenW(L"Height"), (IInspectable *)content_height) != S_OK) {
			fixture_number_release(content_width);
			fixture_number_release(content_height);
			return 350;
		}
		fixture_number_release(content_width);
		fixture_number_release(content_height);
		if (put_style(content_grid, content_style) != S_OK)
			return 351;
		{
			content_control_i *button_content = NULL;
			if (template_button->lpVtbl->QueryInterface(template_button,
				&IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl, (void **)&button_content) != S_OK ||
				!button_content)
				return 352;
			const HRESULT stored = button_content->lpVtbl->put_Content(button_content, (uint64_t)(uintptr_t)content_grid);
			button_content->lpVtbl->Release(button_content);
			if (stored != S_OK)
				return 353;
		}
		/* the presenter first: the template root grid takes it as its child below */
		if (presenter_factory_fn(&presenter_factory) != S_OK || !presenter_factory)
			return 361;
		if (presenter_factory->lpVtbl->ActivateInstance(presenter_factory, &presenter_object) != S_OK || !presenter_object)
			return 362;
		presenter_factory->lpVtbl->Release(presenter_factory);
		/* the template: a red root grid with the presenter as its child */
		if (control_template_factory->lpVtbl->ActivateInstance(control_template_factory, &template_presenter) != S_OK ||
			!template_presenter)
			return 354;
		if (activate(grid_name, &template_root_grid3) != S_OK || !template_root_grid3)
			return 355;
		{
			panel_i *root_panel = NULL;
			if (template_root_grid3->lpVtbl->QueryInterface(template_root_grid3,
				&IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel, (void **)&root_panel) != S_OK || !root_panel)
				return 356;
			if (root_panel->lpVtbl->put_Background(root_panel, (uint64_t)(uintptr_t)red_brush) != S_OK) {
				root_panel->lpVtbl->Release(root_panel);
				return 357;
			}
			{
				void *children = NULL;
				struct fixture_children *vector = NULL;
				if (root_panel->lpVtbl->get_Children(root_panel, (uint64_t)(uintptr_t)&children) != S_OK || !children) {
					root_panel->lpVtbl->Release(root_panel);
					return 358;
				}
				if (((IInspectable *)children)->lpVtbl->QueryInterface((IInspectable *)children,
					&IID___FIVector_1_UIElement, (void **)&vector) != S_OK) {
					((IInspectable *)children)->lpVtbl->Release((IInspectable *)children);
					root_panel->lpVtbl->Release(root_panel);
					return 359;
				}
				((IInspectable *)children)->lpVtbl->Release((IInspectable *)children);
				const HRESULT appended = vector->lpVtbl->Append(vector, presenter_object);
				vector->lpVtbl->Release(vector);
				if (appended != S_OK) {
					root_panel->lpVtbl->Release(root_panel);
					return 360;
				}
			}
			root_panel->lpVtbl->Release(root_panel);
		}
		if (template_put_root(template_presenter, template_root_grid3) != S_OK)
			return 370;
		if (activate(style_name, &template_style3) != S_OK || !template_style3)
			return 363;
		if (add_setter(template_style3, L"Template", (UINT32)lstrlenW(L"Template"), template_presenter) != S_OK)
			return 364;
		if (put_style(template_button, template_style3) != S_OK)
			return 365;
		/* the content grid shows inside the template (200x150 green at the top-left), the root red beyond it */
		if (color_at(hwnd, 100, 100, RGB(0, 255, 0)))
			return 366;
		if (color_at(hwnd, 350, 250, RGB(255, 0, 0)))
			return 367;
		{
			IInspectable *read = NULL;
			if (get_template(template_button, &read) != S_OK || !read)
				return 368;
			if (!same_unknown(read, template_presenter)) {
				read->lpVtbl->Release(read);
				return 369;
			}
			read->lpVtbl->Release(read);
		}
		if (template_root_grid3)
			template_root_grid3->lpVtbl->Release(template_root_grid3);
		if (template_style3)
			template_style3->lpVtbl->Release(template_style3);
		if (template_presenter)
			template_presenter->lpVtbl->Release(template_presenter);
		if (presenter_object)
			presenter_object->lpVtbl->Release(presenter_object);
		if (content_style)
			content_style->lpVtbl->Release(content_style);
		if (content_grid)
			content_grid->lpVtbl->Release(content_grid);
		if (green_brush)
			green_brush->lpVtbl->Release(green_brush);
	}
	/* the ContentTemplate round: the applied content template rides the same template-child hook — the outer presenter's
	 * render child becomes the content template's root (a red grid) once the store lands, and the content template's
	 * inner presenter binds to the button's Content OBJECT (a content button holding the green grid, not the templated
	 * control), so the green still shows at the top-left through the content template's tree while the red fills the
	 * rest. The clear then unbinds the cascade: the outer presenter and the content template's inner presenter both
	 * answer nothing, and the control falls back to rendering its content directly. */
	{
		typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl content_control_i;
		typedef HRESULT (*presenter_factory_function)(IActivationFactory **);
		typedef HRESULT (*put_content_template_function)(IInspectable *, IInspectable *);
		typedef HRESULT (*get_content_template_function)(IInspectable *, IInspectable **);
		typedef HRESULT (*template_child_function)(IInspectable *, IInspectable **);
		presenter_factory_function presenter_factory_fn = NULL;
		put_content_template_function put_content_template = NULL;
		get_content_template_function get_content_template = NULL;
		template_child_function template_child = NULL;
		IActivationFactory *presenter_factory = NULL;
		IInspectable *template_outer = NULL, *outer_root_grid = NULL, *outer_presenter = NULL;
		IInspectable *content_template = NULL, *content_root_grid = NULL, *inner_presenter = NULL;
		IInspectable *content_button = NULL, *content_style4 = NULL, *content_object = NULL, *green_grid = NULL;
		brush_i *green_brush2 = NULL;
		color_s green2;
		content_control_i *button_content = NULL, *inner_content = NULL;
		green2.A = 255;
		green2.R = 0;
		green2.G = 255;
		green2.B = 0;
		if (compositor->lpVtbl->CreateColorBrushWithColor(compositor, green2, &green_brush2) != S_OK || !green_brush2)
			return 371;
		put_content_template = component ?
			(put_content_template_function)GetProcAddress(component, "raven_xaml_element_put_content_template") : NULL;
		get_content_template = component ?
			(get_content_template_function)GetProcAddress(component, "raven_xaml_element_get_content_template") : NULL;
		template_child = component ?
			(template_child_function)GetProcAddress(component, "raven_xaml_element_template_child") : NULL;
		presenter_factory_fn = component ?
			(presenter_factory_function)GetProcAddress(component, "raven_xaml_content_presenter_factory") : NULL;
		if (!put_content_template || !get_content_template || !template_child || !presenter_factory_fn)
			return 372;
		/* the content object: a button whose own Content is the green 200x150 grid — a ContentControl, so the inner
		 * presenter has something to resolve. The grid is this round's own: the previous round's grid released with
		 * its block. */
		if (activate(grid_name, &green_grid) != S_OK || !green_grid)
			return 373;
		{
			panel_i *panel = NULL;
			if (green_grid->lpVtbl->QueryInterface(green_grid, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel,
				(void **)&panel) != S_OK || !panel)
				return 374;
			const HRESULT painted = panel->lpVtbl->put_Background(panel, (uint64_t)(uintptr_t)green_brush2);
			panel->lpVtbl->Release(panel);
			if (painted != S_OK)
				return 375;
		}
		{
			struct fixture_number *grid_width = make_number(200.0);
			struct fixture_number *grid_height = make_number(150.0);
			IInspectable *grid_style = NULL;
			if (!grid_width || !grid_height) {
				fixture_number_release(grid_width);
				fixture_number_release(grid_height);
				return 408;
			}
			if (activate(style_name, &grid_style) != S_OK) {
				fixture_number_release(grid_width);
				fixture_number_release(grid_height);
				return 409;
			}
			if (add_setter(grid_style, L"Width", (UINT32)lstrlenW(L"Width"), (IInspectable *)grid_width) != S_OK ||
				add_setter(grid_style, L"Height", (UINT32)lstrlenW(L"Height"), (IInspectable *)grid_height) != S_OK ||
				put_style(green_grid, grid_style) != S_OK) {
				((IInspectable *)grid_style)->lpVtbl->Release((IInspectable *)grid_style);
				fixture_number_release(grid_width);
				fixture_number_release(grid_height);
				return 410;
			}
			((IInspectable *)grid_style)->lpVtbl->Release((IInspectable *)grid_style);
			fixture_number_release(grid_width);
			fixture_number_release(grid_height);
		}
		if (activate(button_name, &content_button) != S_OK || !content_button)
			return 376;
		if (content_button->lpVtbl->QueryInterface(content_button,
			&IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl, (void **)&inner_content) != S_OK || !inner_content)
			return 377;
		if (inner_content->lpVtbl->put_Content(inner_content, (uint64_t)(uintptr_t)green_grid) != S_OK) {
			inner_content->lpVtbl->Release(inner_content);
			return 378;
		}
		inner_content->lpVtbl->Release(inner_content);
		inner_content = NULL;
		if (template_button->lpVtbl->QueryInterface(template_button,
			&IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl, (void **)&button_content) != S_OK || !button_content)
			return 411;
		if (button_content->lpVtbl->put_Content(button_content, (uint64_t)(uintptr_t)content_button) != S_OK) {
			button_content->lpVtbl->Release(button_content);
			return 412;
		}
		button_content->lpVtbl->Release(button_content);
		button_content = NULL;
		/* the content template: a red root grid holding one presenter */
		if (presenter_factory_fn(&presenter_factory) != S_OK || !presenter_factory)
			return 413;
		if (presenter_factory->lpVtbl->ActivateInstance(presenter_factory, &inner_presenter) != S_OK || !inner_presenter)
			return 379;
		presenter_factory->lpVtbl->Release(presenter_factory);
		presenter_factory = NULL;
		if (control_template_factory->lpVtbl->ActivateInstance(control_template_factory, &content_template) != S_OK ||
			!content_template)
			return 380;
		if (activate(grid_name, &content_root_grid) != S_OK || !content_root_grid)
			return 381;
		if (template_put_root(content_template, content_root_grid) != S_OK)
			return 382;
		{
			panel_i *panel = NULL;
			if (content_root_grid->lpVtbl->QueryInterface(content_root_grid,
				&IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel, (void **)&panel) != S_OK || !panel)
				return 383;
			if (panel->lpVtbl->put_Background(panel, (uint64_t)(uintptr_t)red_brush) != S_OK) {
				panel->lpVtbl->Release(panel);
				return 384;
			}
			if (append_child(panel, inner_presenter) != S_OK) {
				panel->lpVtbl->Release(panel);
				return 385;
			}
			panel->lpVtbl->Release(panel);
		}
		/* the outer template: a blue root grid holding the presenter that carries the content template */
		if (presenter_factory_fn(&presenter_factory) != S_OK || !presenter_factory)
			return 386;
		if (presenter_factory->lpVtbl->ActivateInstance(presenter_factory, &outer_presenter) != S_OK || !outer_presenter)
			return 387;
		presenter_factory->lpVtbl->Release(presenter_factory);
		presenter_factory = NULL;
		if (control_template_factory->lpVtbl->ActivateInstance(control_template_factory, &template_outer) != S_OK ||
			!template_outer)
			return 388;
		if (activate(grid_name, &outer_root_grid) != S_OK || !outer_root_grid)
			return 389;
		if (template_put_root(template_outer, outer_root_grid) != S_OK)
			return 390;
		{
			panel_i *panel = NULL;
			if (outer_root_grid->lpVtbl->QueryInterface(outer_root_grid,
				&IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel, (void **)&panel) != S_OK || !panel)
				return 391;
			if (panel->lpVtbl->put_Background(panel, (uint64_t)(uintptr_t)blue_brush) != S_OK) {
				panel->lpVtbl->Release(panel);
				return 392;
			}
			if (append_child(panel, outer_presenter) != S_OK) {
				panel->lpVtbl->Release(panel);
				return 393;
			}
			panel->lpVtbl->Release(panel);
		}
		if (activate(style_name, &content_style4) != S_OK || !content_style4)
			return 394;
		if (add_setter(content_style4, L"Template", (UINT32)lstrlenW(L"Template"), template_outer) != S_OK)
			return 395;
		if (put_style(template_button, content_style4) != S_OK)
			return 396;
		/* without the content template the presenter shows the content button directly: green over the blue root */
		if (color_at(hwnd, 100, 100, RGB(0, 255, 0)))
			return 397;
		if (color_at(hwnd, 350, 250, RGB(0, 0, 255)))
			return 398;
		/* the store: the content template applies at the template-child hook, red replaces the direct rendering and
		 * the green flows through the inner presenter bound to the content button */
		if (put_content_template(outer_presenter, content_template) != S_OK)
			return 399;
		if (color_at(hwnd, 100, 100, RGB(0, 255, 0)))
			return 400;
		if (color_at(hwnd, 350, 250, RGB(255, 0, 0)))
			return 401;
		{
			IInspectable *read = NULL;
			if (get_content_template(outer_presenter, &read) != S_OK || !read)
				return 402;
			if (!same_unknown(read, content_template)) {
				read->lpVtbl->Release(read);
				return 403;
			}
			read->lpVtbl->Release(read);
		}
		/* the clear unbinds the cascade: both presenters answer nothing, and the content renders directly again */
		if (put_template(template_button, NULL) != S_OK)
			return 404;
		if (template_child(outer_presenter, &content_object) != S_OK || content_object)
			return 405;
		if (template_child(inner_presenter, &content_object) != S_OK || content_object)
			return 406;
		content_object = NULL;
		if (color_at(hwnd, 100, 100, RGB(0, 255, 0)))
			return 407;
		if (outer_root_grid)
			outer_root_grid->lpVtbl->Release(outer_root_grid);
		if (content_root_grid)
			content_root_grid->lpVtbl->Release(content_root_grid);
		if (content_style4)
			content_style4->lpVtbl->Release(content_style4);
		if (template_outer)
			template_outer->lpVtbl->Release(template_outer);
		if (content_template)
			content_template->lpVtbl->Release(content_template);
		if (outer_presenter)
			outer_presenter->lpVtbl->Release(outer_presenter);
		if (inner_presenter)
			inner_presenter->lpVtbl->Release(inner_presenter);
		if (content_button)
			content_button->lpVtbl->Release(content_button);
		if (green_grid)
			green_grid->lpVtbl->Release(green_grid);
		if (green_brush2)
			green_brush2->lpVtbl->Release(green_brush2);
	}
	/* the clear: a direct put_Template(NULL) removes the template and the control's visuals fall back to its content
	 * slot — the green content grid this round gave the button, so the content the template was showing renders
	 * directly */
	if (put_template(template_button, NULL) != S_OK)
		return 340;
	if (get_template(template_button, &roundtrip) != S_OK || roundtrip)
		return 341;
	if (color_at(hwnd, 100, 100, RGB(0, 255, 0)))
		return 342;
	if (control_template_factory)
		control_template_factory->lpVtbl->Release(control_template_factory);
	/* release everything: the window's own reference goes with Close, and clearing the content detaches the hosted tree
	 * from the render and input roots. The component cannot answer DllCanUnloadNow with S_OK after a render — the
	 * composition target keeps the presented visual tree until the process ends, which is why the rendering fixture never
	 * checks it — so unload stays untested here and every style object above releases through the refs it owns. */
	if (child_vector)
		((IInspectable *)child_vector)->lpVtbl->Release((IInspectable *)child_vector);
	if (xaml_window->lpVtbl->put_Content(xaml_window, 0) != S_OK)
		return 105;
	xaml_window->lpVtbl->Close(xaml_window);
	if (text_iface)
		text_iface->lpVtbl->Release(text_iface);
	if (styled_panel)
		styled_panel->lpVtbl->Release(styled_panel);
	if (plain_panel)
		plain_panel->lpVtbl->Release(plain_panel);
	if (brush)
		brush->lpVtbl->Release(brush);
	if (compositor)
		compositor->lpVtbl->Release(compositor);
	if (compositor_object)
		compositor_object->lpVtbl->Release(compositor_object);
	if (map)
		map->lpVtbl->Release(map);
	if (dictionary)
		dictionary->lpVtbl->Release(dictionary);
	fixture_box_release(red);
	fixture_box_release(key);
	fixture_box_release(probe);
	fixture_box_release(blue);
	fixture_box_release(red2);
	fixture_box_release(blue2);
	fixture_number_release(size32);
	fixture_number_release(size14);
	fixture_number_release(seven);
	fixture_number_release(three);
	fixture_number_release(size8);
	if (derived_iface)
		derived_iface->lpVtbl->Release(derived_iface);
	if (default_text_iface)
		default_text_iface->lpVtbl->Release(default_text_iface);
	if (style_iface)
		style_iface->lpVtbl->Release(style_iface);
	if (style)
		style->lpVtbl->Release(style);
	if (style2)
		style2->lpVtbl->Release(style2);
	if (base_style)
		base_style->lpVtbl->Release(base_style);
	if (derived_style)
		derived_style->lpVtbl->Release(derived_style);
	if (bad_style)
		bad_style->lpVtbl->Release(bad_style);
	if (bad_style2)
		bad_style2->lpVtbl->Release(bad_style2);
	if (skip_style)
		skip_style->lpVtbl->Release(skip_style);
	if (override_style)
		override_style->lpVtbl->Release(override_style);
	if (default_textblock)
		default_textblock->lpVtbl->Release(default_textblock);
	if (default_button)
		default_button->lpVtbl->Release(default_button);
	if (found)
		found->lpVtbl->Release(found);
	if (textblock)
		textblock->lpVtbl->Release(textblock);
	if (grid_styled)
		grid_styled->lpVtbl->Release(grid_styled);
	if (grid_plain)
		grid_plain->lpVtbl->Release(grid_plain);
	/* the DependencyProperty round: the DPs themselves live in the engine's registration store (component lifetime, the
	 * way the authored defaults are), so what releases here is what the fixture owns */
	if (property_read)
		property_read->lpVtbl->Release(property_read);
	if (property_read2)
		property_read2->lpVtbl->Release(property_read2);
	if (dp_setter)
		dp_setter->lpVtbl->Release(dp_setter);
	if (dp_setter2)
		dp_setter2->lpVtbl->Release(dp_setter2);
	if (setter_object)
		setter_object->lpVtbl->Release(setter_object);
	if (setter_object2)
		setter_object2->lpVtbl->Release(setter_object2);
	if (style3_iface)
		style3_iface->lpVtbl->Release(style3_iface);
	if (style4_iface)
		style4_iface->lpVtbl->Release(style4_iface);
	if (style3_setters)
		style3_setters->lpVtbl->Release(style3_setters);
	if (style4_setters)
		style4_setters->lpVtbl->Release(style4_setters);
	if (style3)
		style3->lpVtbl->Release(style3);
	if (style4)
		style4->lpVtbl->Release(style4);
	if (font_dp)
		font_dp->lpVtbl->Release(font_dp);
	if (background_dp)
		background_dp->lpVtbl->Release(background_dp);
	if (grid_dp)
		grid_dp->lpVtbl->Release(grid_dp);
	if (dp_unset1)
		dp_unset1->lpVtbl->Release(dp_unset1);
	if (dp_unset2)
		dp_unset2->lpVtbl->Release(dp_unset2);
	if (dp_statics)
		dp_statics->lpVtbl->Release(dp_statics);
	/* the ControlTemplate round: the templates own their roots, so what releases here is the fixture's own references */
	if (red_brush)
		red_brush->lpVtbl->Release(red_brush);
	if (blue_brush)
		blue_brush->lpVtbl->Release(blue_brush);
	if (template_button)
		template_button->lpVtbl->Release(template_button);
	if (template_bad)
		template_bad->lpVtbl->Release(template_bad);
	if (template_style2)
		template_style2->lpVtbl->Release(template_style2);
	if (template_style)
		template_style->lpVtbl->Release(template_style);
	if (template_root_grid2)
		template_root_grid2->lpVtbl->Release(template_root_grid2);
	if (template_root_grid)
		template_root_grid->lpVtbl->Release(template_root_grid);
	if (template_blue)
		template_blue->lpVtbl->Release(template_blue);
	if (template_red)
		template_red->lpVtbl->Release(template_red);
	fixture_metadata_release(font_metadata);
	fixture_metadata_release(bg_metadata);
	fixture_number_release(font_default);
	fixture_number_release(forty);
	fixture_box_release(bg_default);
	fixture_box_release(dp_blue);
	if (font_key)
		WindowsDeleteString(font_key);
	if (bg_key)
		WindowsDeleteString(bg_key);
	xaml_window->lpVtbl->Release(xaml_window);
	window_statics->lpVtbl->Release(window_statics);
	window_activation->lpVtbl->Release(window_activation);
	WindowsDeleteString(window_key);
	WindowsDeleteString(compositor_key);
	interop->lpVtbl->Release(interop);
	core->lpVtbl->Release(core);
	__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_Release(core_statics);
	WindowsDeleteString(core_key);
	return 0;
}
