/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML loader: reads a compiled XAML (XBF) document and builds the element tree its node stream describes.
 *
 * The parser follows the format documented by the XBF tooling: a header with the metadata and node-stream sizes, a metadata
 * section with the string, assembly, type-namespace, type and property tables, a substream table whose entries delimit node
 * streams and line-info streams, and node records. Types and properties are resolved either by their stable index (framework
 * members) or through the document's own tables (application types and members), and the values a node names are applied to the
 * element on the scope stack: names through the element, integer/enum/boolean/string properties through the interfaces the
 * element implements, and object values through a pending property that the next created object fills.
 *
 * Property values outside the implemented subset are skipped rather than guessed; a node whose payload cannot be measured is
 * refused so the caller is not handed a half-built tree.
 *
 * Templates ride the object-property path: a ControlTemplate node opens a template scope that defers its application to
 * the target until the scope pops (so the root element the document declares inside it attaches first, through the
 * engine-internal put_root), and a ContentPresenter inside the template resolves its templated parent's Content at render
 * time through the element side's hook. Both classes build through the engine-internal factories when a stable-index
 * reference names them — the runtime catalog does not list ContentPresenter — and a document's own type table resolves
 * them by name through the same factories.
 *
 * The entry point is exported as RavenXamlLoad so the engine and its tests can hand it a compiled page. */
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include "windows_ui_xaml_application.h"
#include "windows_ui_xaml_dictionary.h"
#include "windows_ui_xaml_style.h"
#include <stdint.h>
#include <winstring.h>
#include "xbf_type_names.h"
static const WCHAR *g_current_effective_theme;
static UINT32 g_current_effective_theme_length;
static const WCHAR *g_pending_effective_theme;
static UINT32 g_pending_effective_theme_length;

#define MAX_NAME 256

struct reader {
	const uint8_t *data;
	size_t at, end;
	int invalid;
};

struct metadata {
	const uint8_t *data;
	size_t size;
	int null_terminated;
	size_t strings, string_count;
	size_t assemblies, assembly_count;
	size_t namespaces, namespace_count;
	size_t types, type_count;
	size_t properties, property_count;
	size_t xmlns, xmlns_count;
};

/* The document's substream table. A SetCustomRuntimeData node names one substream that carries a deferred object's
 * content; the table entries are (node offset, line offset) pairs relative to the first substream's node bytes. */
struct load_context {
	const struct metadata *m;
	size_t table; /* absolute offset of the substream count */
	size_t nodes; /* absolute offset of the first substream's nodes */
	size_t size; /* complete document bytes retained by deferred dictionaries */
	uint32_t count;
	int tolerant; /* a framework dictionary's merge skips what the engine cannot store instead of refusing the document */
};

enum value_kind { value_none, value_string, value_bool, value_integer, value_float, value_color, value_enum };
struct value {
	enum value_kind kind;
	const WCHAR *text;
	UINT32 length;
	int64_t integer;
	double real;
	uint16_t enum_type;
};

static uint16_t read_u16(const uint8_t *data) { return (uint16_t)(data[0] | (data[1] << 8)); }
static uint32_t read_u32(const uint8_t *data)
{
	return (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}
static uint64_t read_u64(const uint8_t *data)
{
	uint64_t value = 0;
	for (int index = 0; index < 8; ++index)
		value |= (uint64_t)data[index] << (index * 8);
	return value;
}

static uint8_t next_byte(struct reader *r)
{
	if (r->at >= r->end) {
		r->invalid = 1;
		return 0;
	}
	return r->data[r->at++];
}
static uint16_t next_u16(struct reader *r)
{
	uint16_t value;
	if (r->end - r->at < 2) {
		r->invalid = 1;
		return 0;
	}
	value = read_u16(r->data + r->at);
	r->at += 2;
	return value;
}
static int32_t next_i32(struct reader *r)
{
	int32_t value;
	if (r->end - r->at < 4) {
		r->invalid = 1;
		return 0;
	}
	value = (int32_t)read_u32(r->data + r->at);
	r->at += 4;
	return value;
}
static double next_f32(struct reader *r)
{
	union { uint32_t bits; float number; } converter;
	converter.bits = (uint32_t)next_i32(r);
	return converter.number;
}
static int32_t next_vint(struct reader *r)
{
	uint32_t value = 0;
	for (int shift = 0; shift <= 28; shift += 7) {
		const uint8_t byte = next_byte(r);
		if (r->invalid || (shift == 28 && (byte & 0xf8))) {
			r->invalid = 1;
			return 0;
		}
		value |= (uint32_t)(byte & 0x7F) << shift;
		if (!(byte & 0x80))
			return (int32_t)value;
	}
	r->invalid = 1;
	return 0;
}

static HRESULT read_string_ref(struct reader *r, const WCHAR **text, UINT32 *length)
{
	const int32_t size = next_i32(r);
	if (r->invalid || size < 0 || (size_t)size > (r->end - r->at) / 2)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:151\n"); return E_INVALIDARG; }
	*text = (const WCHAR *)(const void *)(r->data + r->at);
	*length = (UINT32)size;
	r->at += (size_t)size * 2;
	return S_OK;
}

static HRESULT skip_string(struct reader *r)
{
	const WCHAR *text;
	UINT32 length;
	return read_string_ref(r, &text, &length);
}

static int metadata_bounds(const struct metadata *m, size_t position, size_t count, size_t width)
{
	return position <= m->size && count <= (m->size - position) / width;
}

static HRESULT metadata_parse(struct metadata *m, const uint8_t *data, size_t size)
{
	uint64_t offsets[6];
	size_t start = 12, position;
	for (size_t index = 0; index < sizeof *m; ++index)
		((uint8_t *)m)[index] = 0;
	m->data = data;
	m->size = size;
	if (start + 56 > size)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:179\n"); return E_INVALIDARG; }
	for (int index = 0; index < 6; ++index) {
		offsets[index] = read_u64(data + start + 8 + index * 8);
		if (offsets[index] > size - start || size - start - (size_t)offsets[index] < 4)
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:183\n"); return E_INVALIDARG; }
	}
	m->strings = start + (size_t)offsets[0];
	m->assemblies = start + (size_t)offsets[1];
	m->namespaces = start + (size_t)offsets[2];
	m->types = start + (size_t)offsets[3];
	m->properties = start + (size_t)offsets[4];
	m->xmlns = start + (size_t)offsets[5];
	m->string_count = read_u32(data + m->strings);
	m->assembly_count = read_u32(data + m->assemblies);
	m->namespace_count = read_u32(data + m->namespaces);
	m->type_count = read_u32(data + m->types);
	m->property_count = read_u32(data + m->properties);
	m->xmlns_count = read_u32(data + m->xmlns);
	if (!metadata_bounds(m, m->assemblies + 4, m->assembly_count, 8) ||
		!metadata_bounds(m, m->namespaces + 4, m->namespace_count, 8) ||
		!metadata_bounds(m, m->types + 4, m->type_count, 12) ||
		!metadata_bounds(m, m->properties + 4, m->property_count, 12) ||
		!metadata_bounds(m, m->xmlns + 4, m->xmlns_count, 4))
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:202\n"); return E_INVALIDARG; }
	/* Test both encodings independently: a bad plain-string interpretation must not reject a terminated table. */
	for (int terminated = 0; terminated < 2; ++terminated) {
		BOOL valid = TRUE;
		position = m->strings + 4;
		for (uint32_t index = 0; index < m->string_count; ++index) {
			int32_t length;
			if (position > m->assemblies || m->assemblies - position < 4) { valid = FALSE; break; }
			length = (int32_t)read_u32(data + position);
			if (length < 0 || (size_t)length > (m->assemblies - position - 4) / 2) { valid = FALSE; break; }
			position += 4 + (size_t)length * 2;
			if (terminated) {
				if (m->assemblies - position < 2 || read_u16(data + position) != 0) { valid = FALSE; break; }
				position += 2;
			}
		}
		if (valid && position == m->assemblies) {
			m->null_terminated = terminated;
			return S_OK;
		}
	}
	{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:223\n"); return E_INVALIDARG; }
}

static HRESULT metadata_string(const struct metadata *m, uint32_t id, const WCHAR **text, UINT32 *length)
{
	size_t position = m->strings + 4;
	if (id >= m->string_count)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:230\n"); return E_INVALIDARG; }
	for (uint32_t index = 0; index < id; ++index) {
		int32_t size;
		if (position + 4 > m->size)
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:234\n"); return E_INVALIDARG; }
		size = (int32_t)read_u32(m->data + position);
		if (size < 0)
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:237\n"); return E_INVALIDARG; }
		position += 4 + (size_t)size * 2 + (m->null_terminated ? 2 : 0);
	}
	if (position + 4 > m->size)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:241\n"); return E_INVALIDARG; }
	*length = read_u32(m->data + position);
	if ((size_t)*length * 2 > m->size - (position + 4))
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:244\n"); return E_INVALIDARG; }
	*text = (const WCHAR *)(const void *)(m->data + position + 4);
	return S_OK;
}

/* A type record's first field names the table its second field indexes. Value 2 is the XML namespace table: every type record
 * in Shapr3D's compiled pages uses it, and the entry it names is a compiler "using:<CLR namespace>" directive (verified across
 * all 235 type records in the package's 84 loadable pages), so the directive prefix is stripped before the name is handed to
 * the application's metadata provider. Other values keep the type-namespace table the older documents and the hand-built
 * fixtures use, where the second field indexes a namespace entry whose second half is the namespace string. A "using:" entry
 * carrying an assembly qualifier is refused rather than dropped: the provider is asked by type name only, so dropping it could
 * resolve a same-named type from the wrong assembly. */
static HRESULT metadata_type_name(const struct metadata *m, uint32_t id, WCHAR *buffer, size_t capacity, UINT32 *length)
{
	static const WCHAR using_prefix[] = L"using:";
	size_t position;
	uint32_t namespace_kind, namespace_id, name_id, namespace_name_id;
	const WCHAR *namespace_text = NULL, *name_text = NULL;
	UINT32 namespace_length = 0, name_length = 0;
	if (id >= m->type_count)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:264\n"); return E_INVALIDARG; }
	position = m->types + 4 + (size_t)id * 12;
	namespace_kind = read_u32(m->data + position);
	namespace_id = read_u32(m->data + position + 4);
	name_id = read_u32(m->data + position + 8);
	if (FAILED(metadata_string(m, name_id, &name_text, &name_length)))
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:270\n"); return E_INVALIDARG; }
	if (namespace_kind == 2) {
		if (namespace_id >= m->xmlns_count)
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:273\n"); return E_INVALIDARG; }
		namespace_name_id = read_u32(m->data + m->xmlns + 4 + (size_t)namespace_id * 4);
		if (FAILED(metadata_string(m, namespace_name_id, &namespace_text, &namespace_length)) ||
			namespace_length < ARRAYSIZE(using_prefix) - 1 ||
			CompareStringOrdinal(namespace_text, ARRAYSIZE(using_prefix) - 1, using_prefix, -1, TRUE) != CSTR_EQUAL)
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:278\n"); return E_INVALIDARG; }
		namespace_text += ARRAYSIZE(using_prefix) - 1;
		namespace_length -= ARRAYSIZE(using_prefix) - 1;
		if (!namespace_length)
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:282\n"); return E_INVALIDARG; }
		for (UINT32 index = 0; index < namespace_length; ++index)
			if (namespace_text[index] == L';')
				{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:285\n"); return E_INVALIDARG; }
	} else {
		if (namespace_id >= m->namespace_count)
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:288\n"); return E_INVALIDARG; }
		namespace_name_id = read_u32(m->data + m->namespaces + 4 + (size_t)namespace_id * 8 + 4);
		if (FAILED(metadata_string(m, namespace_name_id, &namespace_text, &namespace_length)))
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:291\n"); return E_INVALIDARG; }
	}
	if ((size_t)namespace_length + 1 + name_length + 1 > capacity)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:294\n"); return E_INVALIDARG; }
	for (UINT32 index = 0; index < namespace_length; ++index)
		buffer[index] = namespace_text[index];
	buffer[namespace_length] = L'.';
	for (UINT32 index = 0; index < name_length; ++index)
		buffer[namespace_length + 1 + index] = name_text[index];
	*length = namespace_length + 1 + name_length;
	return S_OK;
}

/* The authored short name of a type record, the form a XAML type property carries ("Button", not
 * "Windows.UI.Xaml.Controls.Button"): the record's name field alone, without its namespace. */
static HRESULT metadata_type_short_name(const struct metadata *m, uint32_t id, const WCHAR **name, UINT32 *length)
{
	size_t position;
	uint32_t name_id;
	if (id >= m->type_count)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:311\n"); return E_INVALIDARG; }
	position = m->types + 4 + (size_t)id * 12;
	name_id = read_u32(m->data + position + 8);
	return metadata_string(m, name_id, name, length);
}

/* A type-valued property can name a framework type even when the engine cannot instantiate that class. */
static HRESULT type_reference_name(const struct metadata *m, uint16_t reference, const WCHAR **name, UINT32 *length)
{
	if (reference & 0x8000) {
		const UINT32 index = reference & 0x7fff;
		if (index >= ARRAYSIZE(xbf_type_names) || !xbf_type_names[index])
			return CLASS_E_CLASSNOTAVAILABLE;
		*name = xbf_type_names[index];
		*length = (UINT32)lstrlenW(*name);
		return S_OK;
	}
	return metadata_type_short_name(m, reference, name, length);
}

static HRESULT metadata_property_name(const struct metadata *m, uint32_t id, const WCHAR **name, UINT32 *length)
{
	size_t position;
	uint32_t name_id;
	if (id >= m->property_count)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:336\n"); return E_INVALIDARG; }
	position = m->properties + 4 + (size_t)id * 12;
	name_id = read_u32(m->data + position + 8);
	return metadata_string(m, name_id, name, length);
}

static HRESULT property_reference(const struct metadata *m, uint16_t reference, const WCHAR **name, UINT32 *length)
{
	if (reference & 0x8000) {
		const WCHAR *stable = raven_xaml_property_by_stable(reference & 0x7FFF);
		if (!stable)
			return CLASS_E_CLASSNOTAVAILABLE;
		*name = stable;
		*length = (UINT32)lstrlenW(stable);
		return S_OK;
	}
	return metadata_property_name(m, reference, name, length);
}

static HRESULT create_known(const WCHAR *name, UINT32 length, IInspectable **out);

static HRESULT create_reference(const struct metadata *m, uint16_t reference, IInspectable **out)
{
	WCHAR buffer[MAX_NAME];
	UINT32 length = 0;
	if (reference & 0x8000) {
		const uint32_t stable = reference & 0x7FFF;
		const HRESULT made = raven_xaml_create_stable(stable, out);
		if (SUCCEEDED(made))
			return made;
		/* The catalog does not list every class the stable table carries (ContentPresenter), so a failed RoActivateInstance
		 * still falls to the engine-internal factories when the stable table names a class they serve. */
		const WCHAR *class_name = raven_xaml_class_by_stable(stable);
		if (!class_name)
			return made;
		const HRESULT known = create_known(class_name, (UINT32)lstrlenW(class_name), out);
		return known != CLASS_E_CLASSNOTAVAILABLE ? known : made;
	}
	if (FAILED(metadata_type_name(m, reference, buffer, MAX_NAME, &length)))
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:375\n"); return E_INVALIDARG; }
	{
		const HRESULT known = create_known(buffer, length, out);
		if (known != CLASS_E_CLASSNOTAVAILABLE)
			return known;
	}
	return raven_xaml_create_named(buffer, length, out);
}

/* The template-path classes a compiled document names by stable index or through its own tables: ControlTemplate (the
 * value a "Template" property declares) and ContentPresenter (the element inside a template that shows the control's
 * content). Both build through the engine-internal factories, so a document resolves them without a running
 * application's metadata provider and without a catalog entry for the presenter; every other document-named type keeps
 * the provider path. */
static HRESULT create_known(const WCHAR *name, UINT32 length, IInspectable **out)
{
	static const WCHAR template_name[] = L"Windows.UI.Xaml.Controls.ControlTemplate";
	static const WCHAR presenter_name[] = L"Windows.UI.Xaml.Controls.ContentPresenter";
	static const WCHAR controls_resources_name[] = L"Microsoft.UI.Xaml.Controls.XamlControlsResources";
	static const WCHAR solid_brush_name[] = L"Microsoft.UI.Xaml.Media.SolidColorBrush";
	static const WCHAR acrylic_brush_name[] = L"Microsoft.UI.Xaml.Media.AcrylicBrush";
	IActivationFactory *factory = NULL;
	HRESULT result;
	*out = NULL;
	if (CompareStringOrdinal(name, (int)length, acrylic_brush_name, -1, FALSE) == CSTR_EQUAL) {
		return raven_acrylic_brush_create(out);
	} else if (CompareStringOrdinal(name, (int)length, solid_brush_name, -1, FALSE) == CSTR_EQUAL) {
		/* The media brush the theme dictionaries build: a projected runtime class the runtime catalog does not carry,
		 * served by the engine (see xaml_styles.c). The Microsoft.UI namespace is what the documents' own xmlns
		 * declares ("using:Microsoft.UI.Xaml.Media"). */
		return raven_solid_brush_create(out);
	} else if (CompareStringOrdinal(name, (int)length, template_name, -1, FALSE) == CSTR_EQUAL) {
		result = raven_xaml_control_template_factory(&factory);
	} else if (CompareStringOrdinal(name, (int)length, presenter_name, -1, FALSE) == CSTR_EQUAL) {
		HSTRING key = NULL;
		result = WindowsCreateString(presenter_name, (UINT32)(ARRAYSIZE(presenter_name) - 1), &key);
		if (SUCCEEDED(result)) {
			result = raven_xaml_elements_factory(key, &factory);
			WindowsDeleteString(key);
		}
	} else if (CompareStringOrdinal(name, (int)length, controls_resources_name, -1, FALSE) == CSTR_EQUAL) {
		/* The WinUI 2.x controls dictionary: a real ResourceDictionary of the right runtime class whose WinUI default
		 * content this runtime does not ship (see xaml_dictionary.c). */
		return raven_xaml_dictionary_create_controls_resources(out);
	} else {
		return CLASS_E_CLASSNOTAVAILABLE;
	}
	if (FAILED(result))
		return result;
	if (!factory)
		return CLASS_E_CLASSNOTAVAILABLE;
	result = factory->lpVtbl->ActivateInstance(factory, out);
	factory->lpVtbl->Release(factory);
	return result;
}

static HRESULT read_constant(struct reader *r, const struct metadata *m, struct value *v)
{
	const uint8_t kind = next_byte(r);
	v->kind = value_none;
	v->text = NULL;
	v->length = 0;
	v->integer = 0;
	v->real = 0;
	v->enum_type = 0;
	switch (kind) {
	case 0:
		return S_OK;
	case 1:
		v->kind = value_bool;
		return S_OK;
	case 2:
		v->kind = value_bool;
		v->integer = 1;
		return S_OK;
	case 3:
		v->kind = value_float;
		v->real = next_f32(r);
		return r->invalid ? E_INVALIDARG : S_OK;
	case 4:
		v->kind = value_integer;
		v->integer = next_i32(r);
		return r->invalid ? E_INVALIDARG : S_OK;
	case 5: {
		const uint16_t reference = next_u16(r);
		if (r->invalid)
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:461\n"); return E_INVALIDARG; }
		if (reference & 0x8000)
			return S_OK;
		v->kind = value_string;
		return metadata_string(m, reference, &v->text, &v->length);
	}
	case 6:
		if (r->end - r->at < 16)
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:469\n"); return E_INVALIDARG; }
		r->at += 16;
		break;
	case 7:
		if (r->end - r->at < 8)
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:474\n"); return E_INVALIDARG; }
		r->at += 8;
		break;
	case 11:
		v->enum_type = next_u16(r);
		v->kind = value_enum;
		v->integer = next_i32(r);
		break;
	case 8:
		/* A packed Windows.UI.Color: the constant the theme dictionaries apply to SolidColorBrush.Color and the one the
		 * PushScopeCreateTypeWithConstant nodes build the colour structs from. The four bytes read ARGB, high byte
		 * first — the spelling xbf-dump prints (#aarrggbb). */
		v->kind = value_color;
		v->integer = (uint32_t)next_i32(r);
		break;
	case 9:
		v->kind = value_string;
		return read_string_ref(r, &v->text, &v->length);
	case 10:
		return S_OK;
	default:
		{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
	}
	return r->invalid ? E_INVALIDARG : S_OK;
}

static HRESULT apply_value(IInspectable *object, const WCHAR *name, UINT32 length, const struct value *value)
{
	static const WCHAR name_property[] = L"Name";
	static const WCHAR source_property[] = L"Source";
	__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *dictionary = NULL;

	if (length == ARRAYSIZE(name_property) - 1 && CompareStringOrdinal(name, (int)length, name_property, -1, FALSE) == CSTR_EQUAL) {
		if (value->kind == value_string)
			return raven_xaml_element_set_name(object, value->text, value->length);
		return S_FALSE;
	}
	/* A compiled ResourceDictionary carries its contents by reference: SetValueTypeConvertedConstant ResourceDictionary_Source
	 * names the dictionary document to load, and the setter does the PRI resolution and the nested load. */
	if (length == ARRAYSIZE(source_property) - 1 &&
		CompareStringOrdinal(name, (int)length, source_property, -1, FALSE) == CSTR_EQUAL &&
		SUCCEEDED(IInspectable_QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary, (void **)&dictionary))) {
		IInspectable *uri = NULL;
		HRESULT result;
		if (value->kind != value_string) {
			dictionary->lpVtbl->Release(dictionary);
			return value->kind == value_none ? S_FALSE : E_NOTIMPL;
		}
		result = raven_xaml_uri_create(value->text, value->length, &uri);
		if (SUCCEEDED(result)) {
			result = dictionary->lpVtbl->put_Source(dictionary, (uint64_t)(uintptr_t)uri);
			IInspectable_Release(uri);
		}
		dictionary->lpVtbl->Release(dictionary);
		return result;
	}
	switch (value->kind) {
	case value_string:
		{
			HRESULT result = raven_acrylic_brush_set_text(object, name, length, value->text, value->length);
			return result != S_FALSE ? result : raven_xaml_element_set_text(object, name, length, value->text, value->length);
		}
	case value_bool:
	case value_integer:
	case value_enum:
		return raven_xaml_element_set_integer(object, name, length, value->integer);
	case value_float:
		{
			HRESULT result = raven_acrylic_brush_set_float(object, name, length, (float)value->real);
			return result != S_FALSE ? result : raven_xaml_elements_set_float(object, name, length, (float)value->real);
		}
	case value_color:
		/* Apply packed colors to the supported brush properties. */
		if (!object)
			return S_FALSE;
		{
			__x_ABI_CWindows_CUI_CColor color;
			color.A = (BYTE)((value->integer >> 24) & 0xff);
			color.R = (BYTE)((value->integer >> 16) & 0xff);
			color.G = (BYTE)((value->integer >> 8) & 0xff);
			color.B = (BYTE)(value->integer & 0xff);
			HRESULT result = raven_acrylic_brush_set_color(object, name, length, &color);
			return result != S_FALSE ? result : raven_solid_brush_put_color(object, &color);
		}
	default:
		return S_FALSE;
	}
}

static HRESULT use_target(const struct metadata *m, uint16_t reference, IInspectable *target, IInspectable **out)
{
	WCHAR buffer[MAX_NAME];
	const WCHAR *expected;
	UINT32 length;
	HSTRING actual = NULL;
	HRESULT result;
	if (reference == (0x8000 | 31)) {
		IInspectable *application = NULL;
		result = IInspectable_QueryInterface(target, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication, (void **)&application);
		if (application) IInspectable_Release(application);
		if (FAILED(result)) return result;
	} else {
		if (reference & 0x8000) {
			expected = raven_xaml_class_by_stable(reference & 0x7fff);
			if (!expected) { raven_trace_not_implemented(__func__); return E_NOTIMPL; }
			length = (UINT32)lstrlenW(expected);
		} else {
			result = metadata_type_name(m, reference, buffer, ARRAYSIZE(buffer), &length);
			if (FAILED(result)) return result;
			expected = buffer;
		}
		result = IInspectable_GetRuntimeClassName(target, &actual);
		if (FAILED(result)) return result;
		UINT32 actual_length = 0;
		const WCHAR *actual_text = WindowsGetStringRawBuffer(actual, &actual_length);
		BOOL matches = actual_length == length && CompareStringOrdinal(actual_text, (int)actual_length, expected, (int)length, FALSE) == CSTR_EQUAL;
		WindowsDeleteString(actual);
		if (!matches) return E_INVALIDARG;
	}
	IInspectable_AddRef(target);
	*out = target;
	return S_OK;
}

typedef __x_ABI_CWindows_CUI_CXaml_CIApplication application_i;
typedef __x_ABI_CWindows_CUI_CXaml_CIResourceDictionary dictionary_i;
typedef __FIVector_1_Windows__CUI__CXaml__CResourceDictionary dictionary_vector_i;

static BOOL is_application_dictionary(IInspectable *parent, IInspectable *child)
{
	application_i *app = NULL;
	dictionary_i *dictionary = NULL;
	BOOL found = SUCCEEDED(IInspectable_QueryInterface(parent, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication, (void **)&app)) && app &&
		SUCCEEDED(IInspectable_QueryInterface(child, &IID__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary, (void **)&dictionary)) && dictionary;
	if (dictionary) dictionary->lpVtbl->Release(dictionary);
	if (app) app->lpVtbl->Release(app);
	return found;
}
static HRESULT set_application_resources(IInspectable *parent, IInspectable *child)
{
	application_i *app = NULL;
	HRESULT result = IInspectable_QueryInterface(parent, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication, (void **)&app);
	if (FAILED(result)) return result;
	result = app->lpVtbl->put_Resources(app, child);
	app->lpVtbl->Release(app);
	return result;
}
/* One open scope. kind 0 is an element scope; 1 a ControlTemplate whose root element has not arrived; 2 a
 * ControlTemplate whose root is attached. A template scope defers its application to the target the pending property
 * named until the scope pops — the compiled document creates the template object before the elements inside it, and the
 * control must store the template only once its root is attached, so the presenter binding (and every later read of the
 * template) sees the final tree. The deferred target is a borrowed pointer into the scope below. */
struct scope {
	IInspectable *object;
	unsigned char kind;
	IInspectable *deferred_target;
	const WCHAR *deferred_name;
	UINT32 deferred_length;
};

/* A Source setter can synchronously start another load before its parent's resources are published. */
struct ambient_frame {
	const struct scope *stack;
	size_t depth;
	struct ambient_frame *previous;
};
static DWORD ambient_tls = TLS_OUT_OF_INDEXES;
static DWORD deferred_tls = TLS_OUT_OF_INDEXES;

BOOL raven_xaml_loader_notify(DWORD reason, void *reserved)
{
	(void)reserved;
	if (reason == DLL_PROCESS_ATTACH) {
		ambient_tls = TlsAlloc();
		deferred_tls = TlsAlloc();
		return ambient_tls != TLS_OUT_OF_INDEXES && deferred_tls != TLS_OUT_OF_INDEXES;
	}
	if (reason == DLL_PROCESS_DETACH && ambient_tls != TLS_OUT_OF_INDEXES) {
		TlsFree(ambient_tls);
		if (deferred_tls != TLS_OUT_OF_INDEXES) TlsFree(deferred_tls);
		ambient_tls = TLS_OUT_OF_INDEXES;
	}
	return TRUE;
}

/* The application's resource dictionary: the last stage of a lookup. Owned by the caller. */
static HRESULT application_resources(IInspectable **out)
{
	IInspectable *current = NULL;
	application_i *app = NULL;
	HRESULT result;
	*out = NULL;
	result = raven_xaml_application_current(&current);
	if (FAILED(result) || !current)
		return E_BOUNDS;
	result = IInspectable_QueryInterface(current, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication, (void **)&app);
	IInspectable_Release(current);
	if (FAILED(result) || !app)
		return E_BOUNDS;
	result = app->lpVtbl->get_Resources(app, out);
	app->lpVtbl->Release(app);
	if (SUCCEEDED(result) && !*out)
		return E_BOUNDS;
	return result;
}

/* The theme a ThemeResource lookup selects: the application's requested theme, named the way ThemeDictionaries keys are.
 * Without an application (or with the default Light theme) the "Light" selector applies; "Default" is the fallback. */
static void requested_theme_name(const WCHAR **name, UINT32 *length)
{
	static const WCHAR light[] = L"Light";
	static const WCHAR dark[] = L"Dark";
	IInspectable *current = NULL;
	application_i *app = NULL;
	INT32 theme = 0;
	*name = light;
	*length = ARRAYSIZE(light) - 1;
	if (FAILED(raven_xaml_application_current(&current)) || !current)
		return;
	if (SUCCEEDED(IInspectable_QueryInterface(current, &IID__x_ABI_CWindows_CUI_CXaml_CIApplication, (void **)&app)) && app) {
		if (FAILED(app->lpVtbl->get_RequestedTheme(app, &theme)))
			theme = 0;
		app->lpVtbl->Release(app);
	}
	IInspectable_Release(current);
	/* ApplicationTheme: Light is 0, Dark is 1. */
	if (theme == 1) {
		*name = dark;
		*length = ARRAYSIZE(dark) - 1;
	}
}

/* The value a StaticResource or ThemeResource names: the resources of the innermost scope first, then each enclosing scope,
 * then Application.Resources. Within one dictionary the lookup handles merged dictionaries (newest-first) and, for a theme
 * resource, the theme stage. An entry may itself be a resource reference from ProvideStaticResourceValue/
 * ProvideThemeResourceValue; the reference is followed through the same lookup, bounded so a cycle ends the search. */
#define RESOURCE_HOPS 16

static BOOL is_map_object(IInspectable *object);
static HRESULT materialize_resource(IInspectable *dictionary, const WCHAR *key, UINT32 length);
static HRESULT lookup_resource(IInspectable *dictionary, const WCHAR *key, UINT32 length,
	const WCHAR *theme, UINT32 theme_length, IInspectable **out)
{
	HRESULT result = materialize_resource(dictionary, key, length);
	if (FAILED(result) && result != E_BOUNDS) return result;
	/* StaticResource shares initial theme selection; ThemeResource can also reevaluate after a theme change. */
	return raven_xaml_dictionary_lookup_theme(dictionary, key, length, theme, theme_length, out);
}

static HRESULT resolve_resource(const struct scope *stack, size_t depth, int theme_resource, const WCHAR *key,
	UINT32 length, IInspectable **out, int hops, const WCHAR *effective_theme, UINT32 effective_theme_length);

static HRESULT resolve_follow(const struct scope *stack, size_t depth, IInspectable *value, IInspectable **out, int hops,
	const WCHAR *effective_theme, UINT32 effective_theme_length)
{
	const WCHAR *key = NULL;
	UINT32 length = 0;
	BOOL theme = FALSE;
	if (!raven_xaml_resource_reference_key(value, &key, &length, &theme))
		return S_OK;
	value->lpVtbl->Release(value);
	*out = NULL;
	if (hops >= RESOURCE_HOPS)
		return E_BOUNDS;
	return resolve_resource(stack, depth, theme, key, length, out, hops + 1, effective_theme, effective_theme_length);
}

static HRESULT resolve_resource(const struct scope *stack, size_t depth, int theme_resource, const WCHAR *key,
	UINT32 length, IInspectable **out, int hops, const WCHAR *effective_theme, UINT32 effective_theme_length)
{
	const WCHAR *theme = NULL;
	UINT32 theme_length = 0;
	IInspectable *dictionary = NULL;
	HRESULT result = E_BOUNDS;
	*out = NULL;
	if (effective_theme) { theme = effective_theme; theme_length = effective_theme_length; }
	else requested_theme_name(&theme, &theme_length);
	for (size_t index = depth; index; ) {
		IInspectable *scope_object = stack[--index].object;
		dictionary = raven_xaml_element_resources_existing(scope_object);
		if (!dictionary && scope_object && is_map_object(scope_object)) dictionary = scope_object;
		if (!dictionary)
			continue;
		result = lookup_resource(dictionary, key, length, theme, theme_length, out);
		if (FAILED(result) && result != E_BOUNDS) return result;
		if (result == S_OK && *out)
			return resolve_follow(stack, depth, *out, out, hops, effective_theme, effective_theme_length);
	}
	for (struct ambient_frame *frame = TlsGetValue(ambient_tls); frame; frame = frame->previous) {
		for (size_t index = frame->depth; index; ) {
			IInspectable *scope_object = frame->stack[--index].object;
			dictionary = raven_xaml_element_resources_existing(scope_object);
			if (!dictionary && scope_object && is_map_object(scope_object)) dictionary = scope_object;
			if (!dictionary) continue;
			result = lookup_resource(dictionary, key, length, theme, theme_length, out);
			if (FAILED(result) && result != E_BOUNDS) return result;
			if (result == S_OK && *out)
				return resolve_follow(stack, depth, *out, out, hops, effective_theme, effective_theme_length);
		}
	}
	result = application_resources(&dictionary);
	if (SUCCEEDED(result) && dictionary) {
		result = lookup_resource(dictionary, key, length, theme, theme_length, out);
		IInspectable_Release(dictionary);
		if (FAILED(result) && result != E_BOUNDS) return result;
		if (result == S_OK && *out)
			return resolve_follow(stack, depth, *out, out, hops, effective_theme, effective_theme_length);
	}
	if (!effective_theme) requested_theme_name(&effective_theme, &effective_theme_length);
	result = raven_xaml_system_color(key, length, effective_theme, effective_theme_length, out);
	if (result != S_FALSE)
		return result;
	/* Below every dictionary: the platform's own default styles, which compiled documents base their styles on. */
	result = raven_xaml_platform_resource(key, length, out);
	return result == S_FALSE ? E_BOUNDS : result;
}

static void trace_resource_failure(const WCHAR *key, UINT32 length, IInspectable *value)
{
	HSTRING text = NULL;
	OutputDebugStringA("raven-xaml: resource ");
	if (SUCCEEDED(WindowsCreateString(key, length, &text))) {
		OutputDebugStringW(WindowsGetStringRawBuffer(text, NULL));
		WindowsDeleteString(text); text = NULL;
	}
	if (value && SUCCEEDED(value->lpVtbl->GetRuntimeClassName(value, &text))) {
		OutputDebugStringA(" resolved as ");
		OutputDebugStringW(WindowsGetStringRawBuffer(text, NULL));
		WindowsDeleteString(text);
	} else OutputDebugStringA(" was not resolved");
	OutputDebugStringA("\n");
}

static HRESULT get_merged_dictionaries(IInspectable *object, IInspectable **out)
{
	dictionary_i *dictionary = NULL;
	if (!object || !out) return E_POINTER;
	*out = NULL;
	HRESULT result = IInspectable_QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary, (void **)&dictionary);
	if (FAILED(result)) return result;
	result = dictionary->lpVtbl->get_MergedDictionaries(dictionary, (void **)out);
	dictionary->lpVtbl->Release(dictionary);
	return result;
}
static HRESULT get_theme_dictionaries(IInspectable *object, IInspectable **out)
{
	dictionary_i *dictionary = NULL;
	if (!object || !out) return E_POINTER;
	*out = NULL;
	HRESULT result = IInspectable_QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary, (void **)&dictionary);
	if (FAILED(result)) return result;
	result = dictionary->lpVtbl->get_ThemeDictionaries(dictionary, (uint64_t)(uintptr_t)out);
	dictionary->lpVtbl->Release(dictionary);
	return result;
}
static HRESULT append_dictionary(IInspectable *collection, IInspectable *item)
{
	dictionary_vector_i *vector = NULL;
	dictionary_i *dictionary = NULL;
	HRESULT result;
	if (!collection || !item) return E_INVALIDARG;
	result = IInspectable_QueryInterface(collection, &IID___FIVector_1_Windows__CUI__CXaml__CResourceDictionary, (void **)&vector);
	if (SUCCEEDED(result)) result = IInspectable_QueryInterface(item, &IID__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary, (void **)&dictionary);
	if (SUCCEEDED(result)) result = vector->lpVtbl->Append(vector, dictionary);
	if (dictionary) dictionary->lpVtbl->Release(dictionary);
	if (vector) vector->lpVtbl->Release(vector);
	return result;
}

/* A map scope is the ThemeDictionaries property or a dictionary document's own object: either way the node that closes a
 * value scope hands the value to a keyed insertion instead of a collection append. */
static BOOL is_map_object(IInspectable *object)
{
	__FIMap_2_IInspectable_IInspectable *map = NULL;
	BOOL found;
	if (!object) return FALSE;
	found = SUCCEEDED(IInspectable_QueryInterface(object, &IID___FIMap_2_IInspectable_IInspectable, (void **)&map)) && map;
	if (map) map->lpVtbl->Release(map);
	return found;
}

/* AddToDictionaryWithKey: the value is the object the just-closed scope attached, or the constant the PushConstant node before
 * the key carried (the encoding for a primitive entry such as a string resource). The key is an XBF string, boxed so the
 * dictionary's content comparison sees it exactly as the compiled markup's other string keys. Only string values have a box
 * the loader can build today; the numeric and color constants arrive with the resource-value boxing of the style stage and
 * are refused here rather than inserted as the wrong type. */
static HRESULT add_to_dictionary(IInspectable *object, const struct value *key, IInspectable *value,
	const struct value *constant)
{
	__FIMap_2_IInspectable_IInspectable *map = NULL;
	IInspectable *key_object = NULL, *value_object = NULL;
	boolean replaced = FALSE;
	HRESULT result;
	if (!object || !key || key->kind != value_string || !key->text)
		{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
	result = IInspectable_QueryInterface(object, &IID___FIMap_2_IInspectable_IInspectable, (void **)&map);
	if (FAILED(result))
		return result;
	key_object = raven_xaml_boxed_string(key->text, key->length);
	if (!key_object) { result = E_OUTOFMEMORY; goto done; }
	if (value)
		value_object = value;
	else if (constant && constant->kind == value_string && constant->text)
		value_object = raven_xaml_boxed_string(constant->text, constant->length);
	else if (constant && constant->kind == value_color) {
		/* The value constants the framework dictionaries key directly: a packed ARGB colour boxes into the engine's
		 * colour value, the same object the node-22 choreography builds for the app's own dictionaries. */
		__x_ABI_CWindows_CUI_CColor color;
		color.A = (BYTE)((constant->integer >> 24) & 0xff);
		color.R = (BYTE)((constant->integer >> 16) & 0xff);
		color.G = (BYTE)((constant->integer >> 8) & 0xff);
		color.B = (BYTE)(constant->integer & 0xff);
		if (FAILED(raven_color_value_create(&color, &value_object)))
			value_object = NULL;
	} else if (constant && constant->kind == value_enum) {
		/* An enum constant boxes through the engine's enum value, the runtime class the stable table names. */
		const WCHAR *enum_name = raven_xaml_class_by_stable(constant->enum_type);
		if (enum_name && FAILED(raven_enum_value_create(enum_name, (UINT32)lstrlenW(enum_name),
			(INT32)constant->integer, &value_object)))
			value_object = NULL;
	}

	if (!value_object) {
		result = E_NOTIMPL;
		goto done;
	}
	result = map->lpVtbl->Insert(map, key_object, value_object, &replaced);
done:
	if (value_object && value_object != value) IInspectable_Release(value_object);
	if (key_object) IInspectable_Release(key_object);
	if (map) map->lpVtbl->Release(map);
	return result;
}

/* AddToDictionary: the entry whose key the value carries itself. A Style enters under its TargetType — the implicit key
 * a compiled dictionary uses for the un-keyed style form — read back from the style's own TargetType slot and boxed like
 * every other string key. A value without a TargetType, or a target that is not a map, refuses. */
static HRESULT add_with_implicit_key(IInspectable *object, IInspectable *value)
{
	__FIMap_2_IInspectable_IInspectable *map = NULL;
	__x_ABI_CWindows_CUI_CXaml_CIStyle *style = NULL;
	__x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName target_type;
	IInspectable *key = NULL;
	HSTRING name = NULL;
	INT32 length = 0;
	const WCHAR *text;
	boolean replaced = FALSE;
	HRESULT result;
	if (!object || !value)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:923\n"); return E_INVALIDARG; }
	if (FAILED(value->lpVtbl->QueryInterface(value, &IID__x_ABI_CWindows_CUI_CXaml_CIStyle, (void **)&style)) || !style)
		{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
	target_type.Name = NULL;
	target_type.Kind = TypeKind_Metadata;
	result = style->lpVtbl->get_TargetType(style, &target_type);
	style->lpVtbl->Release(style);
	if (FAILED(result))
		return result;
	name = target_type.Name;
	if (!name) {
		WindowsDeleteString(name);
		{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
	}
	text = WindowsGetStringRawBuffer(name, &length);
	if (!text || length <= 0) {
		WindowsDeleteString(name);
		{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
	}
	result = IInspectable_QueryInterface(object, &IID___FIMap_2_IInspectable_IInspectable, (void **)&map);
	if (FAILED(result)) {
		WindowsDeleteString(name);
		return result;
	}
	key = raven_xaml_boxed_string(text, (UINT32)length);
	WindowsDeleteString(name);
	if (!key) {
		map->lpVtbl->Release(map);
		return E_OUTOFMEMORY;
	}
	result = map->lpVtbl->Insert(map, key, value, &replaced);
	map->lpVtbl->Release(map);
	key->lpVtbl->Release(key);
	return result;
}

#define LOADER_DEPTH 64
/* The containers a SetCustomRuntimeData payload is built from. Every reader answers whether the stream stayed inside its
 * declared extent; a payload that runs past the substream is a malformed document, not an unapplied feature. */
static int skip_token_vector(struct reader *r)
{
	const uint32_t count = (uint32_t)next_vint(r);
	if (r->invalid || count > 0x10000)
		return 0;
	for (uint32_t index = 0; index < count; ++index)
		next_vint(r);
	return !r->invalid;
}

/* A legacy resource map: xstring key to stream token. */
static int skip_resource_map(struct reader *r)
{
	const uint32_t count = (uint32_t)next_vint(r);
	if (r->invalid || count > 0x10000)
		return 0;
	for (uint32_t index = 0; index < count; ++index) {
		next_u16(r);
		next_vint(r);
	}
	return !r->invalid;
}

static int skip_string_vector(struct reader *r)
{
	const uint32_t count = (uint32_t)next_vint(r);
	if (r->invalid || count > 0x10000)
		return 0;
	for (uint32_t index = 0; index < count; ++index)
		next_u16(r);
	return !r->invalid;
}

/* A conditional resource map: xstring key to a vector of stream tokens (the objects the predicates guard). */
static int skip_conditional_map(struct reader *r)
{
	const uint32_t count = (uint32_t)next_vint(r);
	if (r->invalid || count > 0x10000)
		return 0;
	for (uint32_t index = 0; index < count; ++index) {
		next_u16(r);
		if (!skip_token_vector(r))
			return 0;
	}
	return !r->invalid;
}

/* The v4 key carries a fixed-width 64-bit hash beside the string. */
static int skip_key_pair(struct reader *r)
{
	next_u16(r);
	if (r->invalid || r->end - r->at < 8) {
		r->invalid = 1;
		return 0;
	}
	r->at += 8;
	return 1;
}

static int skip_key_map(struct reader *r)
{
	const uint32_t count = (uint32_t)next_vint(r);
	if (r->invalid || count > 0x10000)
		return 0;
	for (uint32_t index = 0; index < count; ++index)
		if (!skip_key_pair(r))
			return 0;
	return !r->invalid;
}

static int skip_conditional_key_map(struct reader *r)
{
	const uint32_t count = (uint32_t)next_vint(r);
	if (r->invalid || count > 0x10000)
		return 0;
	for (uint32_t index = 0; index < count; ++index)
		if (!skip_key_pair(r) || !skip_token_vector(r))
			return 0;
	return !r->invalid;
}

/* The conditionally declared objects: an object token and the predicates that guard it (a type reference and its
 * arguments string). */
static int skip_predicates(struct reader *r)
{
	const uint32_t count = (uint32_t)next_vint(r);
	if (r->invalid || count > 0x10000)
		return 0;
	for (uint32_t index = 0; index < count; ++index) {
		next_vint(r); /* guarded object token */
		const uint32_t guarded = (uint32_t)next_vint(r);
		if (r->invalid || guarded > 0x10000)
			return 0;
		for (uint32_t predicate = 0; predicate < guarded; ++predicate) {
			next_u16(r);
			next_u16(r);
		}
	}
	return !r->invalid;
}

/* Style_v2 setters carry a property and either a constant, string, or deferred stream offset. */
static int skip_style_v2_setters(struct reader *r, const struct metadata *m)
{
	const uint32_t setters = (uint32_t)next_vint(r);
	if (r->invalid || setters > 0x10000)
		return 0;
	for (uint32_t index = 0; index < setters; ++index) {
		const uint32_t flags = (uint32_t)next_vint(r);
		const uint32_t value_flags = flags & 0x6f;
		if (r->invalid || flags > 0xff || (value_flags && (value_flags & (value_flags - 1))))
			return 0;
		next_u16(r); /* resolved property, or property name */
		if (!(flags & 0x10))
			next_u16(r); /* owner type */
		if (value_flags == 0x20) {
			struct value ignored = {value_none, NULL, 0, 0, 0};
			if (FAILED(read_constant(r, m, &ignored)))
				return 0;
		} else if (value_flags == 0x04) {
			next_u16(r); /* shared string */
		} else if (value_flags) {
			next_vint(r); /* deferred value or setter offset */
		}
		if (r->invalid)
			return 0;
	}
	return 1;
}

/* The VisualStateGroupCollection custom runtime data (the CustomWriterRuntimeDataTypeIndex values 1-5, the shapes the
 * microsoft-ui-xaml reader source publishes: VisualStateGroupCollection_v2..v5, and 420 for v1). The collections the
 * engine does not model — the visual states of a template — are consumed to their end so the node stream continues;
 * the deferred storyboards they name load through their own nodes. The field ladder follows the reader's version
 * checks: the setter and trigger token vectors arrive from v2, the extendable and collection token vectors from v3,
 * the static-resource trigger tokens only in v5, and the seen-name directives in v4 and v5. */
static int skip_visual_state_group_collection(struct reader *r, const struct metadata *m, uint32_t type)
{
	uint32_t count, inner, i;
	(void)m;
	/* the state-to-group map: a vector of unsigned ints */
	count = (uint32_t)next_vint(r);
	for (i = 0; i < count; ++i) next_vint(r);
	/* the visual states: a name string, a storyboard token, a bool, and the token vectors the version ladder adds */
	count = (uint32_t)next_vint(r);
	for (i = 0; i < count; ++i) {
		struct value ignored = {value_none, NULL, 0, 0, 0};
		next_u16(r); /* the state's name, a shared string reference */
		next_vint(r); /* the deferred storyboard token */
		if (FAILED(read_constant(r, m, &ignored))) /* hasStoryboard, a CValue */
			return 0;
		if (type != 420) {
			uint32_t tokens = (uint32_t)next_vint(r);
			for (inner = 0; inner < tokens; ++inner) next_vint(r);
			tokens = (uint32_t)next_vint(r); /* the state trigger values: a vector of vectors of ints */
			for (inner = 0; inner < tokens; ++inner) {
				uint32_t values = (uint32_t)next_vint(r);
				uint32_t v;
				for (v = 0; v < values; ++v) next_vint(r);
			}
			if (type != 1) {
				tokens = (uint32_t)next_vint(r);
				for (inner = 0; inner < tokens; ++inner) next_vint(r);
				tokens = (uint32_t)next_vint(r);
				for (inner = 0; inner < tokens; ++inner) next_vint(r);
				if (type == 5) {
					tokens = (uint32_t)next_vint(r);
					for (inner = 0; inner < tokens; ++inner) next_vint(r);
				}
			}
		}
		if (r->invalid)
			return 0;
	}
	/* the groups: a name, a bool, a token */
	count = (uint32_t)next_vint(r);
	for (i = 0; i < count; ++i) {
		struct value ignored = {value_none, NULL, 0, 0, 0};
		next_u16(r);
		if (FAILED(read_constant(r, m, &ignored)))
			return 0;
		next_vint(r);
	}
	/* the transitions: two name strings and a token */
	count = (uint32_t)next_vint(r);
	for (i = 0; i < count; ++i) {
		next_u16(r);
		next_u16(r);
		next_vint(r);
	}
	/* unexpectedTokensDetected: a bool */
	{
		struct value ignored = {value_none, NULL, 0, 0, 0};
		if (FAILED(read_constant(r, m, &ignored)))
			return 0;
	}
	/* the transition lookup: a vector of (from, to, token) triples and a vector of ints */
	count = (uint32_t)next_vint(r);
	for (i = 0; i < count; ++i) {
		next_vint(r);
		next_vint(r);
		next_vint(r);
	}
	count = (uint32_t)next_vint(r);
	for (i = 0; i < count; ++i) next_vint(r);
	/* the entire-collection token */
	next_vint(r);
	/* the seen-name directives of v4 and v5: a vector of name strings */
	if (type == 4 || type == 5) {
		count = (uint32_t)next_vint(r);
		for (i = 0; i < count; ++i) next_u16(r);
	}
	return !r->invalid;
}

/* The DeferredElement custom runtime data (CustomWriterRuntimeDataTypeIndex 6 and 9, and 745 for v1, per the
 * microsoft-ui-xaml reader source): the element's name, the properties it carries as literal values, and — from v3 —
 * a realize flag. A deferred element's real content is the substream the node names; the engine consumes this payload
 * so the node stream continues. */
static int skip_deferred_element(struct reader *r, const struct metadata *m, uint32_t type)
{
	uint32_t count, i;
	next_u16(r); /* the element's name, a shared string reference */
	if (type != 745) {
		count = (uint32_t)next_vint(r);
		for (i = 0; i < count; ++i) {
			struct value ignored = {value_none, NULL, 0, 0, 0};
			next_u16(r); /* the property */
			if (FAILED(read_constant(r, m, &ignored))) /* the literal value, a CValue */
				return 0;
		}
		if (type != 6) {
			struct value realize = {value_none, NULL, 0, 0, 0};
			if (FAILED(read_constant(r, m, &realize))) /* the realize flag, a CValue */
				return 0;
		}
	}
	return !r->invalid;
}

/* The runtime-data payload of a SetCustomRuntimeData node, by the type index Microsoft's XBF reader dispatches on. The
 * ResourceDictionary versions and the Style_v2 setter table are decoded to their end so the node stream can continue; a
 * deferred element's payload is not decoded here, and the node refuses rather than guess its length. */
static HRESULT read_custom_runtime_data(struct reader *r, const struct metadata *m, uint32_t *type_index)
{
	const uint32_t type = (uint32_t)next_vint(r);
	int complete = 0;
	switch (type) {
	case 371: /* ResourceDictionary_v1 */
		complete = skip_resource_map(r) && skip_string_vector(r) && skip_resource_map(r) &&
			skip_string_vector(r) && skip_string_vector(r);
		break;
	case 7: /* ResourceDictionary_v2 */
		complete = skip_resource_map(r) && skip_string_vector(r) && skip_resource_map(r) && skip_string_vector(r);
		break;
	case 10: /* ResourceDictionary_v3 */
		complete = skip_resource_map(r) && skip_string_vector(r) && skip_resource_map(r) &&
			skip_conditional_map(r) && skip_conditional_map(r);
		break;
	case 12: /* ResourceDictionary_v4 */
		complete = skip_key_map(r) && skip_string_vector(r) && skip_conditional_key_map(r);
		break;
	case 8: /* Style_v2 */
		complete = skip_style_v2_setters(r, m);
		break;
	default:
		{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
	}
	if (complete && (type == 10 || type == 12))
		complete = skip_predicates(r);
	if (!complete || r->invalid)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:1233\n"); return E_INVALIDARG; }
	*type_index = type;
	return S_OK;
}

/* The substream a SetCustomRuntimeData node names, as a node range. Only substream 0 of a document is a page stream; the
 * rest carry deferred objects. */
static int substream_range(const struct load_context *context, uint32_t index, size_t *begin, size_t *end)
{
	const uint8_t *data;
	if (!context || !context->m || index >= context->count)
		return 0;
	data = context->m->data;
	*begin = context->nodes + read_u32(data + context->table + 4 + (size_t)index * 8);
	*end = context->nodes + read_u32(data + context->table + 8 + (size_t)index * 8);
	return *begin <= *end;
}

/* The last node a load refused: the offset is relative to the document start (the absolute position within the bytes
 * the caller passed), the status the HRESULT the load answered. Both zero-meaning: no failure recorded yet. */
static UINT32 last_failure_offset;
static HRESULT last_failure_status;

struct resource_map_entry { uint32_t token; uint32_t key; BOOL seen; BOOL applied; BOOL implicit; BOOL loading; size_t end; };
struct resource_map { struct resource_map_entry *entries; uint32_t capacity; uint32_t remaining; };

/* Active deferred dictionaries stay on the calling thread while a lookup realizes a later entry. */
struct deferred_frame {
	struct resource_map *map;
	const struct metadata *m;
	const struct load_context *context;
	const struct scope *stack;
	size_t *depth;
	size_t begin, end;
	int deferral_depth;
	const WCHAR *theme;
	UINT32 theme_length;
	IInspectable *dictionary;
	unsigned resource_depth;
	struct deferred_frame *previous;
};

static HRESULT load_nodes(const struct metadata *m, size_t begin, size_t end, IInspectable **root,
	IInspectable *target, BOOL *target_used, const struct load_context *context, int deferral_depth,
	const WCHAR *effective_theme, UINT32 effective_theme_length, size_t map_start, size_t map_limit,
	struct deferred_frame *entry_frame);

static HRESULT materialize_resource(IInspectable *dictionary, const WCHAR *key, UINT32 length)
{
	for (struct deferred_frame *frame = TlsGetValue(deferred_tls); frame; frame = frame->previous) {
		IInspectable *owner = frame->dictionary;
		for (size_t i = 0; !owner && i < *frame->depth; ++i)
			if (frame->stack[i].object && is_map_object(frame->stack[i].object)) owner = frame->stack[i].object;
		if (owner != dictionary) continue;
		for (uint32_t i = 0; i < frame->map->capacity; ++i) {
			struct resource_map_entry *entry = &frame->map->entries[i];
			const WCHAR *text;
			UINT32 text_length;
			IInspectable *root = NULL;
			HRESULT result;
			if (!entry->token || entry->applied) continue;
			result = metadata_string(frame->m, entry->key, &text, &text_length);
			if (FAILED(result)) return result;
			if (entry->implicit) {
				const WCHAR *short_name = text;
				for (UINT32 j = 0; j < text_length; ++j) if (text[j] == L'.') short_name = text + j + 1;
				text_length -= (UINT32)(short_name - text);
				text = short_name;
			}
			if (length != text_length || memcmp(key, text, length * sizeof(WCHAR))) continue;
			if (entry->loading || frame->resource_depth >= RESOURCE_HOPS) return E_INVALIDARG;
			for (uint32_t j = 0; j < frame->map->capacity; ++j)
				if (frame->map->entries[j].token == entry->token) frame->map->entries[j].loading = TRUE;
			struct deferred_frame request = *frame;
			request.dictionary = dictionary;
			BOOL used = TRUE;
			result = load_nodes(frame->m, frame->begin + entry->token - 1, frame->end, &root, dictionary, &used,
				frame->context, frame->deferral_depth, frame->theme, frame->theme_length, 0, 0, &request);
			if (root) IInspectable_Release(root);
			for (uint32_t j = 0; j < frame->map->capacity; ++j)
				if (frame->map->entries[j].token == entry->token) frame->map->entries[j].loading = FALSE;
			return FAILED(result) ? result : entry->applied ? S_OK : E_BOUNDS;
		}
	}
	return E_BOUNDS;
}

/* ResourceDictionary_v3 maps keys to offsets of value-creation nodes within its deferred stream. */
static HRESULT resource_map_init(struct resource_map *map, const struct metadata *m, size_t start, size_t limit,
	size_t stream_size)
{
	struct reader r = {m->data, start, limit, 0}, implicit;
	uint32_t count, explicit_count, implicit_count, capacity = 1;
	map->entries = NULL;
	map->capacity = 0;
	map->remaining = 0;
	if (!start) return S_OK;
	if (next_vint(&r) != 10) return E_INVALIDARG;
	implicit = r;
	if (!skip_resource_map(&implicit) || !skip_string_vector(&implicit)) return E_INVALIDARG;
	implicit_count = (uint32_t)next_vint(&implicit);
	explicit_count = (uint32_t)next_vint(&r);
	if (r.invalid || implicit.invalid || implicit_count > 0x10000 || explicit_count > 0x10000) return E_INVALIDARG;
	count = explicit_count + implicit_count;
	if (!count) return S_OK;
	while (capacity < count * 2) capacity <<= 1;
	map->entries = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)capacity * sizeof *map->entries);
	if (!map->entries) return E_OUTOFMEMORY;
	map->capacity = capacity;
	map->remaining = count;
	for (uint32_t i = 0; i < count; ++i) {
		if (i == explicit_count) r = implicit;
		const uint16_t key = next_u16(&r);
		const uint32_t offset = (uint32_t)next_vint(&r);
		const WCHAR *text;
		UINT32 length, slot;
		if (r.invalid || offset >= stream_size || FAILED(metadata_string(m, key, &text, &length))) {
			HeapFree(GetProcessHeap(), 0, map->entries);
			map->entries = NULL;
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:1352\n"); return E_INVALIDARG; }
		}
		slot = ((offset + 1) * 2654435761u) & (capacity - 1);
		while (map->entries[slot].token) slot = (slot + 1) & (capacity - 1);
		map->entries[slot].token = offset + 1;
		map->entries[slot].key = key;
		map->entries[slot].implicit = i >= explicit_count;
	}
	return S_OK;
}

struct deferred_stream {
	LONG refs;
	struct metadata metadata;
	struct load_context context;
	struct resource_map map;
	size_t begin, end;
	int deferral_depth;
	HSTRING theme;
};

static void deferred_stream_release(void *context)
{
	struct deferred_stream *stream = context;
	if (InterlockedDecrement(&stream->refs)) return;
	HeapFree(GetProcessHeap(), 0, stream->map.entries);
	HeapFree(GetProcessHeap(), 0, (void *)stream->metadata.data);
	WindowsDeleteString(stream->theme);
	HeapFree(GetProcessHeap(), 0, stream);
}

static HRESULT deferred_stream_realize(void *context, IInspectable *dictionary, UINT32 token)
{
	struct deferred_stream *stream = context;
	struct deferred_frame *previous = TlsGetValue(deferred_tls);
	struct deferred_frame request = {0};
	IInspectable *root = NULL;
	BOOL used = TRUE;
	UINT32 theme_length = 0;
	HRESULT result;
	const size_t at = stream->begin + token - 1;
	uint32_t slot = (token * 2654435761u) & (stream->map.capacity - 1);
	if (at >= stream->end || (previous && previous->resource_depth >= RESOURCE_HOPS)) return E_INVALIDARG;
	while (stream->map.entries[slot].token && stream->map.entries[slot].token != token)
		slot = (slot + 1) & (stream->map.capacity - 1);
	if (!stream->map.entries[slot].token || stream->map.entries[slot].loading) return E_INVALIDARG;
	const BYTE kind = stream->metadata.data[at];
	if (kind != 1 && kind != 4 && kind != 20 && kind != 21 && kind != 22 && kind != 34 && kind != 35)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:1400\n"); return E_INVALIDARG; }
	InterlockedIncrement(&stream->refs);
	for (uint32_t i = 0; i < stream->map.capacity; ++i)
		if (stream->map.entries[i].token == token) stream->map.entries[i].loading = TRUE;
	request.map = &stream->map;
	request.m = &stream->metadata; request.context = &stream->context;
	request.begin = stream->begin; request.end = stream->end;
	request.deferral_depth = stream->deferral_depth;
	request.dictionary = dictionary;
	request.resource_depth = previous ? previous->resource_depth : 0;
	const WCHAR *theme = stream->theme ? WindowsGetStringRawBuffer(stream->theme, &theme_length) : NULL;
	result = load_nodes(&stream->metadata, at, stream->end, &root, dictionary, &used, &stream->context,
		stream->deferral_depth, theme, theme_length, 0, 0, &request);
	if (root) IInspectable_Release(root);
	for (uint32_t i = 0; i < stream->map.capacity; ++i)
		if (stream->map.entries[i].token == token) stream->map.entries[i].loading = FALSE;
	if (SUCCEEDED(result) && !stream->map.entries[slot].applied) result = E_BOUNDS;
	deferred_stream_release(stream);
	return result;
}

/* Register compiled entries without constructing their values. Each entry owns a reference to the shared document. */
static HRESULT load_dictionary_content(const struct metadata *m, size_t begin, size_t end, IInspectable **root,
	IInspectable *target, BOOL *target_used, const struct load_context *context, int deferral_depth,
	const WCHAR *theme, UINT32 theme_length, size_t map_start, size_t map_limit)
{
	struct deferred_stream *stream;
	HRESULT result;
	size_t first = end;
	if (!map_start || !target)
		return load_nodes(m, begin, end, root, target, target_used, context, deferral_depth,
			theme, theme_length, map_start, map_limit, NULL);
	stream = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *stream);
	if (!stream) return E_OUTOFMEMORY;
	stream->refs = 1;
	stream->metadata = *m;
	stream->metadata.data = HeapAlloc(GetProcessHeap(), 0, context->size);
	if (!stream->metadata.data) { deferred_stream_release(stream); return E_OUTOFMEMORY; }
	memcpy((void *)stream->metadata.data, m->data, context->size);
	stream->context = *context; stream->context.m = &stream->metadata;
	stream->begin = begin; stream->end = end; stream->deferral_depth = deferral_depth;
	result = theme ? WindowsCreateString(theme, theme_length, &stream->theme) : S_OK;
	if (SUCCEEDED(result)) result = resource_map_init(&stream->map, &stream->metadata, map_start, map_limit, end - begin);
	if (FAILED(result)) { deferred_stream_release(stream); return result; }
	if (!stream->map.capacity) {
		deferred_stream_release(stream);
		return load_nodes(m, begin, end, root, target, target_used, context, deferral_depth,
			theme, theme_length, map_start, map_limit, NULL);
	}
	for (uint32_t i = 0; i < stream->map.capacity; ++i)
		if (stream->map.entries[i].token && begin + stream->map.entries[i].token - 1 < first)
			first = begin + stream->map.entries[i].token - 1;
	/* The prefix declares namespaces and the dictionary itself, before any deferred value starts. */
	result = load_nodes(&stream->metadata, begin, first, root, target, target_used, &stream->context,
		deferral_depth, theme, theme_length, 0, 0, NULL);
	for (uint32_t i = 0; SUCCEEDED(result) && i < stream->map.capacity; ++i) {
		struct resource_map_entry *entry = &stream->map.entries[i];
		const WCHAR *key;
		UINT32 length;
		if (!entry->token) continue;
		result = metadata_string(&stream->metadata, entry->key, &key, &length);
		if (FAILED(result)) break;
		if (entry->implicit) {
			const WCHAR *short_name = key;
			for (UINT32 j = 0; j < length; ++j) if (key[j] == L'.') short_name = key + j + 1;
			length -= (UINT32)(short_name - key); key = short_name;
		}
		InterlockedIncrement(&stream->refs);
		result = raven_xaml_dictionary_defer(target, key, length, stream, entry->token,
			deferred_stream_realize, deferred_stream_release);
		if (FAILED(result)) deferred_stream_release(stream);
	}
	/* Named objects marked by the compiler must be created even if nobody requests their resource key. */
	if (SUCCEEDED(result)) {
		struct reader auto_entries = {stream->metadata.data, map_start, map_limit, 0};
		next_vint(&auto_entries);
		if (!skip_resource_map(&auto_entries)) result = E_INVALIDARG;
		const uint32_t count = (uint32_t)next_vint(&auto_entries);
		for (uint32_t i = 0; SUCCEEDED(result) && i < count; ++i) {
			const uint16_t key_index = next_u16(&auto_entries);
			const WCHAR *key;
			UINT32 length;
			IInspectable *value = NULL;
			result = metadata_string(&stream->metadata, key_index, &key, &length);
			if (SUCCEEDED(result)) result = raven_xaml_dictionary_lookup(target, key, length, &value);
			if (value) IInspectable_Release(value);
		}
	}
	deferred_stream_release(stream);
	return result;
}

/* The diagnostic a probe or a test reads after a failed RavenXamlLoad: which node and why. Engine-internal, __cdecl. */
__declspec(dllexport) void __cdecl RavenXamlLastFailure(UINT32 *offset, HRESULT *status)
{
	if (offset) *offset = last_failure_offset;
	if (status) *status = last_failure_status;
}

static HRESULT load_nodes(const struct metadata *m, size_t begin, size_t end, IInspectable **root,
	IInspectable *target, BOOL *target_used, const struct load_context *context, int deferral_depth,
	const WCHAR *effective_theme, UINT32 effective_theme_length, size_t map_start, size_t map_limit,
	struct deferred_frame *entry_frame)
{
	struct reader r = {m->data, begin, end, 0};
	struct scope stack[LOADER_DEPTH];
	uint32_t scope_tokens[LOADER_DEPTH] = {0};
	struct resource_map local_map;
	struct resource_map *resource_map = entry_frame ? entry_frame->map : &local_map;
	struct deferred_frame frame;
	const size_t stream_begin = entry_frame ? entry_frame->begin : begin;
	size_t depth = 0;
	const WCHAR *pending_name = NULL;
	IInspectable *last_object = NULL;
	BOOL nested_failed = FALSE;
	IInspectable *completed_value = NULL;

	IInspectable *pending_object = NULL;
	uint32_t pending_token = 0;
	IInspectable *pending_theme_dictionary = NULL;
	size_t pending_theme_begin = 0, pending_theme_end = 0;
	size_t pending_map_start = 0, pending_map_limit = 0;
	struct value pending_value = {value_none, NULL, 0, 0, 0};
	BOOL pending_value_valid = FALSE;
	UINT32 pending_length = 0;
	HRESULT status = S_OK;
	const WCHAR *previous_theme = g_current_effective_theme;
	const UINT32 previous_theme_length = g_current_effective_theme_length;
	/* The byte offset of the node being decoded: the last failure records it so a probe can name the form that refused
	 * (RavenXamlLastFailure below); a nested Source load overwrites the outer document's record, which is the evidence
	 * the caller wants — the deepest failure is the one that propagates. */
	size_t node_at = begin;
	if (!entry_frame) {
		status = resource_map_init(resource_map, m, map_start, map_limit, end - begin);
		if (FAILED(status)) return status;
	} else {
		stack[0] = (struct scope){entry_frame->dictionary, 0, NULL, NULL, 0};
		IInspectable_AddRef(stack[0].object);
		depth = 1;
	}
	frame = (struct deferred_frame){resource_map, m, context, stack, &depth, stream_begin, end, deferral_depth,
		effective_theme, effective_theme_length, entry_frame ? entry_frame->dictionary : NULL, entry_frame ? entry_frame->resource_depth + 1 : 0,
		TlsGetValue(deferred_tls)};
	if (!TlsSetValue(deferred_tls, &frame)) {
		if (entry_frame) IInspectable_Release(stack[0].object);
		else HeapFree(GetProcessHeap(), 0, resource_map->entries);
		return E_OUTOFMEMORY;
	}
	g_current_effective_theme = effective_theme;
	g_current_effective_theme_length = effective_theme_length;
	while (r.at < r.end) {
		const uint8_t kind = next_byte(&r);
		node_at = r.at - 1;
		if (resource_map->capacity) {
			uint32_t token = (uint32_t)(node_at - stream_begin) + 1;
			uint32_t slot = (token * 2654435761u) & (resource_map->capacity - 1);
			while (resource_map->entries[slot].token) {
				if (resource_map->entries[slot].token == token) {
					/* A forward lookup already built this entry; keep its object identity. */
					if (resource_map->entries[slot].end) {
						r.at = resource_map->entries[slot].end;
						break;
					}
					resource_map->entries[slot].seen = TRUE;
				}
				slot = (slot + 1) & (resource_map->capacity - 1);
			}
		}
		if (r.at != node_at + 1) continue;
		if (depth < LOADER_DEPTH && (kind == 1 || kind == 18 || kind == 19 ||
			(kind >= 20 && kind <= 22) || (!depth && kind >= 23 && kind <= 25)))
			scope_tokens[depth] = kind >= 20 && kind <= 22 ? (uint32_t)(node_at - stream_begin) + 1 : 0;
		/* A pushed constant is the value of the dictionary entry the next keyed insertion consumes; any other node in
		 * between leaves it unused, the way an ignored PushConstant always behaved. A provided resource reference is
		 * consumed the same way. */
		if (kind != 4 && kind != 10)
			pending_value_valid = FALSE;
		if (kind != 4 && kind != 10 && kind != 34 && kind != 35) pending_token = 0;
		if (kind != 10 && kind != 34 && kind != 35 && pending_object) {
			IInspectable_Release(pending_object);
			pending_object = NULL;
		}
		switch (kind) {
		case 1:
			if (depth < LOADER_DEPTH) {
				stack[depth].object = NULL;
				stack[depth].kind = 0;
				stack[depth].deferred_target = NULL;
				stack[depth].deferred_name = NULL;
				stack[depth].deferred_length = 0;
				++depth;
			} else {
				status = E_INVALIDARG;
				goto done;
			}
			break;
		case 2: case 33: case 39: case 40:
			if (!depth && target) { status = E_INVALIDARG; goto done; }
			if (depth) {
				struct scope *scope = &stack[--depth];
				const uint32_t closed_token = scope_tokens[depth];
				/* A namespace scope carries no object: a value completed inside it rides over it to the collection or
				 * map scope beneath, the choreography the compiled App.xaml runs for every merged dictionary
				 * (GetValue MergedDictionaries, PushScopeAddNamespace, create, EndInitPopScope, AddToCollection). */
				if (depth >= 2 && stack[depth - 1].kind == 6 && !stack[depth - 1].object &&
					(stack[depth - 2].kind == 3 || stack[depth - 2].kind == 5))
					--depth;
				/* The deferred template application: the template scope closes with its root attached, so the control
				 * stores the finished tree. A failed application fails the document the way a failed scoped property
				 * does — the caller is not handed a tree that silently lost its template. */
				if (scope->kind && scope->deferred_target && scope->object) {
					const HRESULT applied = scope->kind == 4 ? set_application_resources(scope->deferred_target, scope->object) :
						raven_xaml_element_set_object(scope->deferred_target, scope->deferred_name, scope->deferred_length, scope->object);
					if (FAILED(applied)) {
						if (scope->object)
							IInspectable_Release(scope->object);
						status = applied;
						goto done;
					}
					/* The attached application dictionary is the value the following assignment node consumes: the
					 * compiled App.xaml names Application.Resources after the dictionary's scope closes. */
					if (scope->kind == 4) {
						if (completed_value) IInspectable_Release(completed_value);
						completed_value = scope->object;
						IInspectable_AddRef(completed_value);
					}
				}
				/* A pending property assignment that outlived the value's own scope (the compiled App.xaml ends
				 * SetValue Application.Resources, PopScope): the value is the one the previous scope completed, and the
				 * assignment lands on the scope the pop closes — the application dictionary through the application's
				 * own property when the pair is that shape. */
				if (pending_name && scope->object) {
					const HRESULT applied = !completed_value ? S_FALSE :
						is_application_dictionary(scope->object, completed_value) ?
						set_application_resources(scope->object, completed_value) :
						raven_xaml_element_apply_value(scope->object, pending_name, pending_length, completed_value);
					pending_name = NULL;
					if (target && applied != S_OK && !context->tolerant) {
						if (scope->object)
							IInspectable_Release(scope->object);
						status = FAILED(applied) ? applied : E_NOTIMPL;
						goto done;
					}
				}
				if (scope->object && depth && (stack[depth - 1].kind == 3 ||
					(scope->kind == 0 && is_map_object(stack[depth - 1].object)))) {
					/* A value a collection or map scope receives: the append or the keyed insertion takes it. */
					if (last_object) IInspectable_Release(last_object);
					last_object = scope->object;
					if (resource_map->capacity && closed_token) {
						uint32_t token = closed_token;
						uint32_t slot = (token * 2654435761u) & (resource_map->capacity - 1);
						BOOL inserted = FALSE;
						while (resource_map->entries[slot].token) {
							if (resource_map->entries[slot].token == token) {
								struct value key = {0};
								key.kind = value_string;
								status = metadata_string(m, resource_map->entries[slot].key, &key.text, &key.length);
								if (SUCCEEDED(status)) status = resource_map->entries[slot].implicit ?
									add_with_implicit_key(stack[depth - 1].object, last_object) :
									add_to_dictionary(stack[depth - 1].object, &key, last_object, NULL);
								if (FAILED(status)) goto done;
								if (!resource_map->entries[slot].applied) {
									resource_map->entries[slot].applied = TRUE;
									--resource_map->remaining;
								}
								inserted = TRUE;
							}
							slot = (slot + 1) & (resource_map->capacity - 1);
						}
						if (inserted && (r.at == r.end || (r.data[r.at] != 8 && r.data[r.at] != 9 && r.data[r.at] != 10))) {
							IInspectable_Release(last_object);
							last_object = NULL;
						}
					}
				} else if (scope->object) IInspectable_Release(scope->object);
			}
			break;
		case 8:
			if (!depth || stack[depth - 1].kind != 3 || !last_object) {
				/* With a target the document must hand the collection its value; without one the skip convention
				 * applies (an object whose creation was skipped leaves nothing to append, and a scope pushed
				 * object-less over an unmodeled or unbuilt collection has nothing to receive it). */
				if (target && !context->tolerant) { status = E_INVALIDARG; goto done; }
				break;
			}
			if (!stack[depth - 1].object && (!target || context->tolerant)) {
				IInspectable_Release(last_object); last_object = NULL;
				break;
			}
			status = append_dictionary(stack[depth - 1].object, last_object);
			IInspectable_Release(last_object); last_object = NULL;
			if (FAILED(status)) goto done;
			break;
		case 9: {
			/* AddToDictionary: the keyed insertion whose key the value carries itself — a Style enters the dictionary
			 * under its TargetType, the implicit key. The key is the type name the style stored, boxed like every
			 * other string key. */
			IInspectable *value_object = last_object;
			if (!depth || !value_object) {
				if (target) { status = E_INVALIDARG; goto done; }
				break;
			}
			status = add_with_implicit_key(stack[depth - 1].object, value_object);
			IInspectable_Release(last_object); last_object = NULL;
			if (FAILED(status) && target) goto done;
			if (FAILED(status)) status = S_OK;
			break;
		}
		case 20: case 21: case 22: {
			const uint16_t reference = next_u16(&r);
			IInspectable *object = NULL;
			struct value value = {value_none, NULL, 0, 0, 0};
			if (r.invalid || depth == LOADER_DEPTH) {
				status = E_INVALIDARG;
				goto done;
			}
			if (kind != 20) {
				status = read_constant(&r, m, &value);
				if (FAILED(status))
					goto done;
			}
			if (target && !*target_used) {
				status = use_target(m, reference, target, &object);
				if (FAILED(status)) goto done;
				*target_used = TRUE;
			} else {
				status = create_reference(m, reference, &object);
				/* A Windows.UI.Color struct (stable 42, the index the stable table names) is not activatable: the
				 * kind-8 constant beside the node is the colour, so it boxes into the engine's colour value — the
				 * same object the dictionary then keys it under. Any other failed create keeps the skip. */
				if (FAILED(status) && !object && reference == (0x8000 | 42) && value.kind == value_color) {
					__x_ABI_CWindows_CUI_CColor color;
					color.A = (BYTE)((value.integer >> 24) & 0xff);
					color.R = (BYTE)((value.integer >> 16) & 0xff);
					color.G = (BYTE)((value.integer >> 8) & 0xff);
					color.B = (BYTE)(value.integer & 0xff);
					status = raven_color_value_create(&color, &object);
					if (FAILED(status))
						object = NULL;
				}
				if (FAILED(status) && !object && reference == (0x8000 | 52) && value.kind == value_float)
					status = raven_double_value_create(value.real, &object);
				if (FAILED(status) && !object && reference == (0x8000 | 601) && value.kind == value_enum) {
					const WCHAR *enum_name = raven_xaml_class_by_stable(601);
					if (value.enum_type != 601) { status = E_INVALIDARG; goto done; }
					status = raven_enum_value_create(enum_name, (UINT32)lstrlenW(enum_name), (INT32)value.integer, &object);
				}
			}
			if (target && (FAILED(status) || !object) && !context->tolerant) { if (SUCCEEDED(status)) status = E_UNEXPECTED; goto done; }
			/* The skip convention: a type the runtime cannot build is an unapplied value when no target demands it, and
			 * a failed create must not poison the rest of the load. A malformed reference (E_INVALIDARG: the metadata
			 * does not name a type at all) is not a skip — it refuses the document. */
			if (FAILED(status) && status != E_INVALIDARG) status = S_OK;
			if (object) {
				if (raven_xaml_object_is_control_template(object)) {
					/* The template scope opens; the pending property name rides the scope as the deferred
					 * application. A template is never the visual root of the scope above, so no property applies
					 * it now. */
					if (depth < LOADER_DEPTH) {
						stack[depth].object = object;
						stack[depth].kind = 1;
						stack[depth].deferred_target = (pending_name && depth) ? stack[depth - 1].object : NULL;
						stack[depth].deferred_name = pending_name;
						stack[depth].deferred_length = pending_length;
						++depth;
					} else {
						IInspectable_Release(object);
					}
					pending_name = NULL;
					if (!*root) {
						*root = object;
						IInspectable_AddRef(object);
					}
				} else if (depth && stack[depth - 1].kind == 1) {
					/* The template scope above is waiting for its root: this object is it, whatever property name
					 * the node stream carried — the engine-internal attach owns the tree from here. */
					status = raven_xaml_control_template_put_root(stack[depth - 1].object, object);
					if (FAILED(status)) {
						IInspectable_Release(object);
						goto done;
					}
					stack[depth - 1].kind = 2;
					pending_name = NULL;
					if (!*root) {
						*root = object;
						IInspectable_AddRef(object);
					}
					if (depth < LOADER_DEPTH) {
						stack[depth].object = object;
						stack[depth].kind = 0;
						stack[depth].deferred_target = NULL;
						stack[depth].deferred_name = NULL;
						stack[depth].deferred_length = 0;
						++depth;
					} else {
						IInspectable_Release(object);
					}
				} else {
					BOOL application_resources = depth && stack[depth - 1].object &&
						(!pending_name || (pending_length == 9 && !memcmp(pending_name, L"Resources", 9 * sizeof(WCHAR)))) &&
						is_application_dictionary(stack[depth - 1].object, object);
					if (pending_name && depth && !application_resources) {
						const HRESULT applied = raven_xaml_element_set_object(stack[depth - 1].object, pending_name, pending_length, object);
						if (FAILED(applied) || (target && applied != S_OK)) {
							IInspectable_Release(object);
							status = FAILED(applied) ? applied : E_NOTIMPL;
							goto done;
						}
					}
					pending_name = NULL;
					if (!*root) {
						*root = object;
						IInspectable_AddRef(object);
					}
					if (depth < LOADER_DEPTH) {
						stack[depth].object = object;
						stack[depth].kind = application_resources ? 4 : 0;
						stack[depth].deferred_target = application_resources ? stack[depth - 1].object : NULL;
						stack[depth].deferred_name = NULL;
						stack[depth].deferred_length = 0;
						++depth;
					} else {
						IInspectable_Release(object);
					}
				}
			} else if (depth < LOADER_DEPTH) {
				stack[depth].object = NULL;
				stack[depth].kind = 0;
				stack[depth].deferred_target = NULL;
				stack[depth].deferred_name = NULL;
				stack[depth].deferred_length = 0;
				++depth;
			}
			break;
		}
		case 23: case 24: case 25: {
			const uint16_t reference = next_u16(&r);
			IInspectable *object = NULL;
			struct value value = {value_none, NULL, 0, 0, 0};
			if (r.invalid) {
				status = E_INVALIDARG;
				goto done;
			}
			if (kind != 23) {
				status = read_constant(&r, m, &value);
				if (FAILED(status))
					goto done;
			}
			if (target && !*target_used) {
				status = use_target(m, reference, target, &object);
				if (FAILED(status)) goto done;
				*target_used = TRUE;
				if (depth && !stack[depth - 1].object) {
					stack[depth - 1].object = object;
					IInspectable_AddRef(object);
				}
			} else status = create_reference(m, reference, &object);
			if (target && (FAILED(status) || !object) && !context->tolerant) { if (SUCCEEDED(status)) status = E_UNEXPECTED; goto done; }
			/* The skip convention: a type the runtime cannot build is an unapplied value when no target demands it, and
			 * a failed create must not poison the rest of the load. A malformed reference (E_INVALIDARG: the metadata
			 * does not name a type at all) is not a skip — it refuses the document. */
			if (FAILED(status) && status != E_INVALIDARG) status = S_OK;
			if (object) {
				if (depth && !stack[depth - 1].object) {
					stack[depth - 1].object = object;
					IInspectable_AddRef(object);
				}
				if (depth && stack[depth - 1].kind == 1) {
					/* A non-scoped object under a template scope is the root the same way a scoped one is. */
					if (raven_xaml_control_template_put_root(stack[depth - 1].object, object) == S_OK)
						stack[depth - 1].kind = 2;
					pending_name = NULL;
				} else {
					if (pending_name && depth) {
						HRESULT applied = raven_xaml_element_set_object(stack[depth - 1].object, pending_name, pending_length, object);
						if (target && applied != S_OK && !context->tolerant) {
							IInspectable_Release(object);
							status = FAILED(applied) ? applied : E_NOTIMPL;
							goto done;
						}
					}
					pending_name = NULL;
				}
				if (!*root) {
					*root = object;
					IInspectable_AddRef(object);
				}
				/* A root object (the Application a compiled App.xaml creates before any scope exists) opens the scope the
				 * rest of the document builds inside: the dictionary that follows must see it as the parent whose
				 * Resources it becomes. The scope owns its own reference; the stream's matching pop closes it. */
				if (!depth && depth < LOADER_DEPTH) {
					stack[depth].object = object;
					IInspectable_AddRef(object);
					stack[depth].kind = 0;
					stack[depth].deferred_target = NULL;
					stack[depth].deferred_name = NULL;
					stack[depth].deferred_length = 0;
					++depth;
				}
				IInspectable_Release(object);
			}
			break;
		}
		case 3:
			/* AddNamespace declares and moves on. */
			next_u16(&r);
			status = skip_string(&r);
			if (FAILED(status)) goto done;
			break;
		case 18:
			/* PushScopeAddNamespace carries the same payload as AddNamespace, but it is a scope: the real App.xbf
			 * balances its stream only if the EndInitPopScope after the object created inside the namespace closes
			 * the namespace, leaving the MergedDictionaries collection scope beneath it for the AddToCollection.
			 * The scope carries no object of its own; a type the create inside it built lands there (the create
			 * node's empty-scope slot) and rides over it when it closes — the pop handler collapses an empty
			 * namespace scope into the collection or map scope beneath it. The namespace itself stays bookkeeping
			 * the loader's resolution never consults: a document's types resolve through the document's own table. */
			next_u16(&r);
			status = skip_string(&r);
			if (FAILED(status)) goto done;
			if (depth < LOADER_DEPTH) {
				stack[depth].object = NULL;
				stack[depth].kind = 6;
				stack[depth].deferred_target = NULL;
				stack[depth].deferred_name = NULL;
				stack[depth].deferred_length = 0;
				++depth;
			} else {
				status = E_INVALIDARG;
				goto done;
			}
			break;
		case 13: {
			struct value value = {value_none, NULL, 0, 0, 0};
			status = read_constant(&r, m, &value);
			if (FAILED(status))
				goto done;
			if (depth && value.kind == value_string) {
				HRESULT applied = raven_xaml_element_set_name(stack[depth - 1].object, value.text, value.length);
				/* A mapped resource can carry its key as x:Name even when it is not a FrameworkElement. */
				if (applied == S_FALSE && resource_map->capacity && scope_tokens[depth - 1]) {
					const uint32_t token = scope_tokens[depth - 1];
					uint32_t slot = (token * 2654435761u) & (resource_map->capacity - 1);
					while (resource_map->entries[slot].token) {
						const WCHAR *key; UINT32 key_length;
						if (resource_map->entries[slot].token == token &&
							SUCCEEDED(metadata_string(m, resource_map->entries[slot].key, &key, &key_length)) &&
							key_length == value.length && !memcmp(key, value.text, key_length * sizeof(WCHAR))) {
							applied = S_OK;
							break;
						}
						slot = (slot + 1) & (resource_map->capacity - 1);
					}
				}
				if (target && applied != S_OK && !context->tolerant) { status = FAILED(applied) ? applied : E_NOTIMPL; goto done; }
			} else if (target && !context->tolerant) { status = E_NOTIMPL; goto done; }
			break;
		}
		case 7: case 19: case 32: {
			const uint16_t reference = next_u16(&r);
			const WCHAR *name = NULL;
			UINT32 length = 0;
			pending_name = NULL;
			status = property_reference(m, reference, &name, &length);
			if (FAILED(status)) {
				if (status != CLASS_E_CLASSNOTAVAILABLE || (target && !context->tolerant)) goto done;
				status = S_OK;
				if (kind != 19) break;
			}
			if (kind == 19) {
				IInspectable *collection = NULL;
				if (!depth || depth == LOADER_DEPTH) {
					status = E_NOTIMPL; goto done;
				}
				if (length == 18 && !memcmp(name, L"MergedDictionaries", 18 * sizeof(WCHAR))) {
					if (!stack[depth - 1].object) {
						/* a create the skip convention left unbuilt: the scope still pushes, object-less */
						if (depth == LOADER_DEPTH) { status = E_INVALIDARG; goto done; }
						memset(&stack[depth], 0, sizeof stack[depth]);
						stack[depth].kind = 3; ++depth;
						break;
					}
					status = get_merged_dictionaries(stack[depth - 1].object, &collection);
					if (FAILED(status)) goto done;
					memset(&stack[depth], 0, sizeof stack[depth]);
					stack[depth].object = collection; stack[depth].kind = 3; ++depth;
				} else if (length == 17 && !memcmp(name, L"ThemeDictionaries", 17 * sizeof(WCHAR))) {
					if (!stack[depth - 1].object) {
						if (depth == LOADER_DEPTH) { status = E_INVALIDARG; goto done; }
						memset(&stack[depth], 0, sizeof stack[depth]);
						stack[depth].kind = 5; ++depth;
						break;
					}
					status = get_theme_dictionaries(stack[depth - 1].object, &collection);
					if (FAILED(status)) goto done;
					memset(&stack[depth], 0, sizeof stack[depth]);
					stack[depth].object = collection; stack[depth].kind = 5; ++depth;
				} else {
					if (target && !context->tolerant) { status = E_NOTIMPL; goto done; }
					/* An unsupported collection still owns a scope; its children must not pop the dictionary. */
					memset(&stack[depth], 0, sizeof stack[depth]);
					stack[depth].kind = 3; ++depth;
				}
			} else {
				pending_name = name; pending_length = length;
			}
			break;
		}
		case 4: case 12: case 14: case 34: case 35: {
			struct value value = {value_none, NULL, 0, 0, 0};
			status = read_constant(&r, m, &value);
			if (FAILED(status)) goto done;
			if (kind == 4) {
				/* PushConstant: the value a keyed insertion consumes. A document that pushes a constant for any other
				 * purpose keeps the old shortcut, which ignored it. */
				pending_value = value;
				pending_value_valid = TRUE;
				pending_token = (uint32_t)(node_at - stream_begin) + 1;
				break;
			}
			if (kind == 34 || kind == 35) {
				/* ProvideStaticResourceValue/ProvideThemeResourceValue: the operand is the key of the resource this
				 * entry stands for, and the following keyed insertion stores the reference. The value is resolved
				 * when the entry is read, the way a compiled App.xaml names a resource another dictionary defines. */
				IInspectable *reference = NULL;
				if (value.kind != value_string || !value.text) {
					if (target && !context->tolerant) { status = E_NOTIMPL; goto done; }
					break;
				}
				status = raven_xaml_resource_reference_create(value.text, value.length, kind == 35, &reference);
				if (FAILED(status)) goto done;
				if (pending_object) IInspectable_Release(pending_object);
				pending_object = reference;
				pending_token = (uint32_t)(node_at - stream_begin) + 1;
				break;
			}
			if (target && !context->tolerant) { status = E_NOTIMPL; goto done; }
			break;
		}
		case 10: {
			struct value key = {value_none, NULL, 0, 0, 0};
			IInspectable *value_object;
			status = read_constant(&r, m, &key);
			if (FAILED(status)) goto done;
			/* The value the entry takes: the object the just-closed scope attached, else the reference a preceding
			 * Provide node installed (nodes 34/35 are always followed by the keyed insertion), else a pushed constant. */
			value_object = last_object ? last_object : pending_object;
			if (resource_map->capacity && pending_token && !last_object) {
				uint32_t slot = (pending_token * 2654435761u) & (resource_map->capacity - 1);
				while (resource_map->entries[slot].token) {
					if (resource_map->entries[slot].token == pending_token) {
						struct value mapped_key = {0};
						mapped_key.kind = value_string;
						status = metadata_string(m, resource_map->entries[slot].key, &mapped_key.text, &mapped_key.length);
						if (SUCCEEDED(status)) status = add_to_dictionary(depth ? stack[depth - 1].object : NULL, &mapped_key,
							value_object, pending_value_valid ? &pending_value : NULL);
						if (FAILED(status)) goto done;
						if (!resource_map->entries[slot].applied) {
							resource_map->entries[slot].applied = TRUE;
							--resource_map->remaining;
						}
					}
					slot = (slot + 1) & (resource_map->capacity - 1);
				}
			}
			if (pending_theme_dictionary && value_object == pending_theme_dictionary) {
				const WCHAR *child_theme = effective_theme;
				UINT32 child_theme_length = effective_theme_length;
				IInspectable *deferred = NULL;
				BOOL into_used = FALSE;
				HRESULT loaded;
				if (key.kind == value_string && key.text &&
					((key.length == 5 && !memcmp(key.text, L"Light", 5 * sizeof(WCHAR))) ||
					 (key.length == 4 && !memcmp(key.text, L"Dark", 4 * sizeof(WCHAR))) ||
					 (key.length == 12 && !memcmp(key.text, L"HighContrast", 12 * sizeof(WCHAR))))) {
					child_theme = key.text;
					child_theme_length = key.length;
				}
				loaded = load_dictionary_content(m, pending_theme_begin, pending_theme_end, &deferred, value_object,
					&into_used, context, deferral_depth + 1, child_theme, child_theme_length, pending_map_start, pending_map_limit);
				if (deferred) IInspectable_Release(deferred);
				IInspectable_Release(pending_theme_dictionary);
				pending_theme_dictionary = NULL;
				if (target && FAILED(loaded)) {
					/* the nested load recorded its own node; the outer record must not overwrite it */
					nested_failed = TRUE;
					status = loaded;
					goto done;
				}
			}
			status = add_to_dictionary(depth ? stack[depth - 1].object : NULL, &key, value_object,
				pending_value_valid ? &pending_value : NULL);
			if (last_object) IInspectable_Release(last_object);
			last_object = NULL;
			if (pending_object) IInspectable_Release(pending_object);
			pending_object = NULL;
			pending_value_valid = FALSE;
			pending_token = 0;
			/* An entry whose value never materialized (the object creation was skipped: a struct the runtime does not
			 * activate, an unresolvable type) leaves the dictionary without the key when no target demands the
			 * document complete; with a target the miss is refused. */
			if (FAILED(status)) {
				if (target && !context->tolerant) goto done;
				status = S_OK;
			}
			break;
		}
		case 11:
			status = skip_string(&r);
			if (FAILED(status))
				goto done;
			break;
		case 15: {
			/* SetCustomRuntimeData: the object's deferred content lives in a substream, and the payload names it together
			 * with the runtime-data form that describes what the substream holds. The counts before the type are the
			 * legacy resource tables a Threshold-era document carried; they are persisted nodes even when empty. */
			const uint32_t stream_index = (uint32_t)next_vint(&r);
			const uint32_t statics = (uint32_t)next_vint(&r);
			const uint32_t themes = (uint32_t)next_vint(&r);
			uint32_t runtime_type = 0;
			size_t runtime_data;
			if (r.invalid || statics > 0x10000 || themes > 0x10000) { status = E_INVALIDARG; goto done; }
			for (uint32_t index = 0; index < statics + themes; ++index)
				next_u16(&r);
			runtime_data = r.at;
			status = read_custom_runtime_data(&r, m, &runtime_type);
			if (FAILED(status))
				goto done;
			/* A dictionary's deferred content is the substream itself. When the current object is the dictionary the
			 * content belongs to — the compiled App.xaml keeps filling its application dictionary after the theme
			 * dictionaries — the substream loads with that dictionary as its target, so the entries join what the
			 * scope already holds. Otherwise the substream builds a fresh dictionary that takes the placeholder's
			 * place, the way a theme dictionary's deferred content replaces the empty one the scope pushed. A
			 * substream that cannot be loaded leaves the placeholder in place, which is the skip the loader gives
			 * any unapplied value. */
			if (depth && context && deferral_depth < 2) {
				size_t deferred_begin = 0, deferred_end = 0;
				if (substream_range(context, stream_index, &deferred_begin, &deferred_end) &&
					deferred_begin < deferred_end) {
					IInspectable *deferred = NULL;
					__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *current = NULL;
					HRESULT loaded = S_OK;
					const int dictionary_content = runtime_type == 371 || runtime_type == 7 ||
						runtime_type == 10 || runtime_type == 12;
					/* Theme dictionary content is keyed immediately after its scope closes; defer it until that key is known. */
					if (dictionary_content && depth >= 2 && stack[depth - 2].kind == 5 && stack[depth - 1].object) {
						if (pending_theme_dictionary) { status = E_INVALIDARG; goto done; }
						pending_theme_dictionary = stack[depth - 1].object;
						IInspectable_AddRef(pending_theme_dictionary);
						pending_theme_begin = deferred_begin;
						pending_theme_end = deferred_end;
						pending_map_start = runtime_type == 10 ? runtime_data : 0;
						pending_map_limit = r.end;
						break;
					}
					if (dictionary_content && stack[depth - 1].object &&
						SUCCEEDED(IInspectable_QueryInterface(stack[depth - 1].object,
							&IID__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary, (void **)&current))) {
						BOOL into_used = FALSE;
						current->lpVtbl->Release(current);
						loaded = load_dictionary_content(m, deferred_begin, deferred_end, &deferred, stack[depth - 1].object,
							&into_used, context, deferral_depth + 1, effective_theme, effective_theme_length,
							runtime_type == 10 ? runtime_data : 0, r.end);
					} else {
						loaded = load_nodes(m, deferred_begin, deferred_end, &deferred, NULL, NULL, context,
							deferral_depth + 1, effective_theme, effective_theme_length, runtime_type == 10 ? runtime_data : 0, r.end, NULL);
						if (SUCCEEDED(loaded) && deferred) {
							__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary *dictionary = NULL;
							if (SUCCEEDED(IInspectable_QueryInterface(deferred, &IID__x_ABI_CWindows_CUI_CXaml_CIResourceDictionary,
								(void **)&dictionary)) && dictionary) {
								dictionary->lpVtbl->Release(dictionary);
								if (stack[depth - 1].object)
									IInspectable_Release(stack[depth - 1].object);
								stack[depth - 1].object = deferred;
								deferred = NULL;
							}
						}
					}
					if (deferred)
						IInspectable_Release(deferred);
					if (target && FAILED(loaded) && !context->tolerant) {
						nested_failed = TRUE;
						status = loaded;
						goto done;
					}
				}
			}
			break;
		}
		case 17: {
			const int32_t substream = (next_u16(&r), next_vint(&r));
			const int32_t statics = next_vint(&r);
			const int32_t themes = next_vint(&r);
			(void)substream;
			if (r.invalid || (uint64_t)statics + (uint64_t)themes > (r.end - r.at) / 2) {
				status = E_INVALIDARG;
				goto done;
			}
			for (uint64_t index = 0; index < (uint64_t)statics + (uint64_t)themes; ++index)
				next_u16(&r);
			if (target && !context->tolerant) { status = E_NOTIMPL; goto done; }
			break;
		}
		case 26: case 27: case 30: case 36: {
			/* 26/27 carry the value as the constant that follows the property; 30 (StaticResource) and 36 (ThemeResource)
			 * carry a resource key instead and the value is the one the scope stack resolves it to. A StringReference
			 * constant is the key form a compiled document uses. */
			const uint16_t reference = next_u16(&r);
			const WCHAR *name = NULL;
			UINT32 length = 0;
			const int resource = kind == 30 || kind == 36;
			struct value value = {value_none, NULL, 0, 0, 0};
			status = read_constant(&r, m, &value);
			if (FAILED(status))
				goto done;
			if (depth && SUCCEEDED(property_reference(m, reference, &name, &length))) {
				HRESULT applied = S_FALSE;
				if (!resource) {
					if (length == 6 && !memcmp(name, L"Source", 6 * sizeof(WCHAR)) && value.kind == value_string) {
						struct ambient_frame frame = {stack, depth, TlsGetValue(ambient_tls)};
						if (!TlsSetValue(ambient_tls, &frame)) { status = E_OUTOFMEMORY; goto done; }
						applied = apply_value(stack[depth - 1].object, name, length, &value);
						if (!TlsSetValue(ambient_tls, frame.previous)) { status = E_UNEXPECTED; goto done; }
					} else applied = apply_value(stack[depth - 1].object, name, length, &value);
				} else if (value.kind == value_string && value.text) {
					IInspectable *resolved = NULL;
					HRESULT resolved_status = resolve_resource(stack, depth, kind == 36, value.text, value.length, &resolved, 0,
						effective_theme, effective_theme_length);
					if (FAILED(resolved_status) && resolved_status != E_BOUNDS) { status = resolved_status; goto done; }
					if (SUCCEEDED(resolved_status) && resolved) {
						applied = raven_xaml_element_apply_value(stack[depth - 1].object, name, length, resolved);
						if (target && !context->tolerant && applied != S_OK) trace_resource_failure(value.text, value.length, resolved);
						IInspectable_Release(resolved);
					} else if (target && !context->tolerant) trace_resource_failure(value.text, value.length, NULL);
				}
				if (target && applied != S_OK && !context->tolerant) { status = FAILED(applied) ? applied : E_NOTIMPL; goto done; }
			} else if (target && !context->tolerant) { status = E_NOTIMPL; goto done; }
			break;
		}
		case 29: {
			/* SetValueTypeConvertedResolvedType: a property and a compiled type, the form a Style's TargetType arrives
			 * in. The type resolves to the authored short name; a property whose value is not a type name is refused. */
			const uint16_t reference = next_u16(&r);
			const uint16_t type_reference = next_u16(&r);
			const WCHAR *name = NULL, *type_name = NULL;
			UINT32 length = 0, type_length = 0;
			if (r.invalid) { status = E_INVALIDARG; goto done; }
			if (depth && SUCCEEDED(property_reference(m, reference, &name, &length)) &&
				SUCCEEDED(type_reference_name(m, type_reference, &type_name, &type_length))) {
				HRESULT applied = raven_xaml_element_set_type(stack[depth - 1].object, name, length, type_name, type_length);
				if (target && applied != S_OK && !context->tolerant) { status = FAILED(applied) ? applied : E_NOTIMPL; goto done; }
			} else if (target && !context->tolerant) { status = E_NOTIMPL; goto done; }
			break;
		}
		case 28: case 31: case 38:
			next_u16(&r);
			next_u16(&r);
			/* TemplateBinding and the resolved-property forms apply when a template resolves them; a dictionary
			 * document tolerates their presence the way it tolerates any value it cannot store. */
			if (target && r.invalid) { status = E_INVALIDARG; goto done; }
			if (target && !context->tolerant) { status = E_NOTIMPL; goto done; }
			break;
		default: {
			char message[] = "raven-xaml: unsupported XBF node 000\n";
			const size_t digits = sizeof message - 5;
			message[digits] = (char)('0' + kind / 100);
			message[digits + 1] = (char)('0' + (kind / 10) % 10);
			message[digits + 2] = (char)('0' + kind % 10);
			OutputDebugStringA(message);
			status = E_NOTIMPL;
			goto done;
		}
		}
		if (r.invalid) {
			status = E_INVALIDARG;
			goto done;
		}
		if (entry_frame && depth == 1 && !last_object && !pending_object && !pending_value_valid) {
			const uint32_t token = (uint32_t)(begin - stream_begin) + 1;
			for (uint32_t i = 0; i < resource_map->capacity; ++i)
				if (resource_map->entries[i].token == token) resource_map->entries[i].end = r.at;
			break;
		}
	}
done:
	g_current_effective_theme = previous_theme;
	g_current_effective_theme_length = previous_theme_length;
	TlsSetValue(deferred_tls, frame.previous);
	/* The deepest failure is the evidence the caller wants: a nested load records its own node before the outer one
	 * would overwrite it with the outer node. */
	if (SUCCEEDED(status) && target && !entry_frame && resource_map->remaining && !context->tolerant) {
		status = E_NOTIMPL;
		for (uint32_t i = 0; i < resource_map->capacity; ++i)
			if (resource_map->entries[i].token && !resource_map->entries[i].seen) { status = E_INVALIDARG; break; }
	}
	if (SUCCEEDED(status) && target && (pending_name || last_object || pending_theme_dictionary)) status = E_INVALIDARG;
	if (FAILED(status) && !nested_failed) {
		char message[] = "raven-xaml: XBF failure at 00000000, status 00000000\n";
		static const char hex[] = "0123456789abcdef";
		last_failure_offset = (UINT32)node_at;
		last_failure_status = status;
		for (unsigned i = 0; i < 8; ++i) {
			message[27 + i] = hex[((UINT32)node_at >> ((7 - i) * 4)) & 15];
			message[44 + i] = hex[((UINT32)status >> ((7 - i) * 4)) & 15];
		}
		OutputDebugStringA(message);
	}
	if (last_object) IInspectable_Release(last_object);
	if (completed_value) IInspectable_Release(completed_value);
	if (pending_object) IInspectable_Release(pending_object);
	if (pending_theme_dictionary) IInspectable_Release(pending_theme_dictionary);
	if (!entry_frame && resource_map->entries) HeapFree(GetProcessHeap(), 0, resource_map->entries);
	/* The scopes unwind without their deferred applications: a failed document never half-applies the templates it
	 * declared, and the objects the unwind releases own the only references the deferred targets held through them. */
	while (depth) {
		IInspectable *object = stack[--depth].object;
		if (object)
			IInspectable_Release(object);
	}
	return status;
}

/* The tolerant load the framework dictionary's merge uses: a document whose properties and objects the engine cannot
 * all store still merges what it can. The loaders run on the calling thread only, so the flag rides in a static. The
 * shape of the document is still validated; what is skipped is the application of values and objects the engine
 * refuses, the skip convention the app-document walk already gives a load without a target. */
static int g_tolerant_load;

static HRESULT load_document(const void *data, uint32_t size, IInspectable **root, IInspectable *target)
{
	const uint8_t *bytes = (const uint8_t *)data;
	uint32_t metadata_size, node_size, count;
	size_t table, node_end, substreams;
	struct metadata metadata;
	struct load_context context;
	HRESULT status;
	BOOL target_used = FALSE;
	context.tolerant = g_tolerant_load;
	if (!root)
		return E_POINTER;
	*root = NULL;
	if (!bytes || size < 12 || bytes[0] != 'X' || bytes[1] != 'B' || bytes[2] != 'F' || bytes[3] != 0)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:2343\n"); return E_INVALIDARG; }
	metadata_size = read_u32(bytes + 4);
	node_size = read_u32(bytes + 8);
	if (metadata_size > size - 12)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:2347\n"); return E_INVALIDARG; }
	table = (size_t)12 + metadata_size;
	if (node_size < 4 || (size_t)node_size > size - table)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:2350\n"); return E_INVALIDARG; }
	node_end = table + node_size;
	status = metadata_parse(&metadata, bytes, table);
	if (FAILED(status))
		return status;
	count = read_u32(bytes + table);
	if ((size_t)count > (node_end - table - 4) / 8)
		{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:2357\n"); return E_INVALIDARG; }
	substreams = table + 4 + (size_t)count * 8;
	context.m = &metadata;
	context.table = table;
	context.nodes = substreams;
	context.count = count;
	context.size = size;
	/* Validate every extent before creating any objects; a later invalid stream cannot leak the first root. */
	for (uint32_t index = 0; index < count; ++index) {
		const uint32_t node_offset = read_u32(bytes + table + 4 + (size_t)index * 8);
		const uint32_t line_offset = read_u32(bytes + table + 4 + (size_t)index * 8 + 4);
		if (node_offset > line_offset || line_offset > node_end - substreams)
			{ raven_trace_note("raven-xaml: E_INVALIDARG at xaml_loader.c:2369\n"); return E_INVALIDARG; }
	}
	if (count) {
		const uint32_t node_offset = read_u32(bytes + table + 4);
		const uint32_t line_offset = read_u32(bytes + table + 4 + 4);
		const size_t begin = substreams + node_offset, end = substreams + line_offset;
		status = load_nodes(&metadata, begin, end, root, target, &target_used, &context, 0, g_pending_effective_theme, g_pending_effective_theme_length, 0, 0, NULL);
		if (FAILED(status)) {
			if (*root) {
				IInspectable_Release(*root);
				*root = NULL;
			}
			return status;
		}
	}
	return target && !target_used ? E_INVALIDARG : S_OK;
}

__declspec(dllexport) HRESULT WINAPI RavenXamlLoad(const void *data, uint32_t size, IInspectable **root)
{
	return load_document(data, size, root, NULL);
}

/* The target form: a document applied INTO an object the caller already owns (Application.LoadComponent and a dictionary's
 * Source both use it). Exported so a fixture can populate Application.Resources the way a compiled App.xaml does. */
__declspec(dllexport) const WCHAR *raven_xaml_get_current_theme(UINT32 *length)
{
	if (length) *length = g_current_effective_theme_length;
	return g_current_effective_theme;
}

__declspec(dllexport) void raven_xaml_set_pending_theme(const WCHAR *theme, UINT32 length)
{
	g_pending_effective_theme = theme;
	g_pending_effective_theme_length = length;
}

/* The target form: a document applied INTO an object the caller already owns (Application.LoadComponent and a dictionary's
 * Source both use it). Exported so a fixture can populate Application.Resources the way a compiled App.xaml does. */
__declspec(dllexport) HRESULT raven_xaml_load_into(const void *data, uint32_t size, IInspectable *target)
{
	IInspectable *root = NULL;
	if (!target) return E_POINTER;
	HRESULT result = load_document(data, size, &root, target);
	if (root) IInspectable_Release(root);
	return result;
}

__declspec(dllexport) HRESULT raven_xaml_load_into_tolerant(const void *data, uint32_t size, IInspectable *target)
{
	IInspectable *root = NULL;
	if (!target) return E_POINTER;
	g_tolerant_load = 1;
	HRESULT result = load_document(data, size, &root, target);
	g_tolerant_load = 0;
	if (root) IInspectable_Release(root);
	return result;
}
