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
#include <stdint.h>
#include <winstring.h>

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
	uint32_t count;
};

enum value_kind { value_none, value_string, value_bool, value_integer, value_float };
struct value {
	enum value_kind kind;
	const WCHAR *text;
	UINT32 length;
	int64_t integer;
	double real;
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
		return E_INVALIDARG;
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
		return E_INVALIDARG;
	for (int index = 0; index < 6; ++index) {
		offsets[index] = read_u64(data + start + 8 + index * 8);
		if (offsets[index] > size - start || size - start - (size_t)offsets[index] < 4)
			return E_INVALIDARG;
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
		return E_INVALIDARG;
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
	return E_INVALIDARG;
}

static HRESULT metadata_string(const struct metadata *m, uint32_t id, const WCHAR **text, UINT32 *length)
{
	size_t position = m->strings + 4;
	if (id >= m->string_count)
		return E_INVALIDARG;
	for (uint32_t index = 0; index < id; ++index) {
		int32_t size;
		if (position + 4 > m->size)
			return E_INVALIDARG;
		size = (int32_t)read_u32(m->data + position);
		if (size < 0)
			return E_INVALIDARG;
		position += 4 + (size_t)size * 2 + (m->null_terminated ? 2 : 0);
	}
	if (position + 4 > m->size)
		return E_INVALIDARG;
	*length = read_u32(m->data + position);
	if ((size_t)*length * 2 > m->size - (position + 4))
		return E_INVALIDARG;
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
		return E_INVALIDARG;
	position = m->types + 4 + (size_t)id * 12;
	namespace_kind = read_u32(m->data + position);
	namespace_id = read_u32(m->data + position + 4);
	name_id = read_u32(m->data + position + 8);
	if (FAILED(metadata_string(m, name_id, &name_text, &name_length)))
		return E_INVALIDARG;
	if (namespace_kind == 2) {
		if (namespace_id >= m->xmlns_count)
			return E_INVALIDARG;
		namespace_name_id = read_u32(m->data + m->xmlns + 4 + (size_t)namespace_id * 4);
		if (FAILED(metadata_string(m, namespace_name_id, &namespace_text, &namespace_length)) ||
			namespace_length < ARRAYSIZE(using_prefix) - 1 ||
			CompareStringOrdinal(namespace_text, ARRAYSIZE(using_prefix) - 1, using_prefix, -1, TRUE) != CSTR_EQUAL)
			return E_INVALIDARG;
		namespace_text += ARRAYSIZE(using_prefix) - 1;
		namespace_length -= ARRAYSIZE(using_prefix) - 1;
		if (!namespace_length)
			return E_INVALIDARG;
		for (UINT32 index = 0; index < namespace_length; ++index)
			if (namespace_text[index] == L';')
				return E_INVALIDARG;
	} else {
		if (namespace_id >= m->namespace_count)
			return E_INVALIDARG;
		namespace_name_id = read_u32(m->data + m->namespaces + 4 + (size_t)namespace_id * 8 + 4);
		if (FAILED(metadata_string(m, namespace_name_id, &namespace_text, &namespace_length)))
			return E_INVALIDARG;
	}
	if ((size_t)namespace_length + 1 + name_length + 1 > capacity)
		return E_INVALIDARG;
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
		return E_INVALIDARG;
	position = m->types + 4 + (size_t)id * 12;
	name_id = read_u32(m->data + position + 8);
	return metadata_string(m, name_id, name, length);
}

/* A compiled type reference in either form: a stable index into the class table or an index into the document's own type
 * table. The stable table names the full runtime class, so the short name is what follows its last dot. */
static HRESULT type_reference_name(const struct metadata *m, uint16_t reference, const WCHAR **name, UINT32 *length)
{
	if (reference & 0x8000) {
		const WCHAR *class_name = raven_xaml_class_by_stable(reference & 0x7FFF);
		const WCHAR *short_name;
		if (!class_name)
			return CLASS_E_CLASSNOTAVAILABLE;
		short_name = class_name;
		for (const WCHAR *at = class_name; *at; ++at)
			if (*at == L'.')
				short_name = at + 1;
		*name = short_name;
		*length = (UINT32)lstrlenW(short_name);
		return S_OK;
	}
	return metadata_type_short_name(m, reference, name, length);
}

static HRESULT metadata_property_name(const struct metadata *m, uint32_t id, const WCHAR **name, UINT32 *length)
{
	size_t position;
	uint32_t name_id;
	if (id >= m->property_count)
		return E_INVALIDARG;
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
		return E_INVALIDARG;
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
	IActivationFactory *factory = NULL;
	HRESULT result;
	*out = NULL;
	if (CompareStringOrdinal(name, (int)length, template_name, -1, FALSE) == CSTR_EQUAL) {
		result = raven_xaml_control_template_factory(&factory);
	} else if (CompareStringOrdinal(name, (int)length, presenter_name, -1, FALSE) == CSTR_EQUAL) {
		HSTRING key = NULL;
		result = WindowsCreateString(presenter_name, (UINT32)(ARRAYSIZE(presenter_name) - 1), &key);
		if (SUCCEEDED(result)) {
			result = raven_xaml_elements_factory(key, &factory);
			WindowsDeleteString(key);
		}
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
			return E_INVALIDARG;
		if (reference & 0x8000)
			return S_OK;
		v->kind = value_string;
		return metadata_string(m, reference, &v->text, &v->length);
	}
	case 6:
		if (r->end - r->at < 16)
			return E_INVALIDARG;
		r->at += 16;
		break;
	case 7:
		if (r->end - r->at < 8)
			return E_INVALIDARG;
		r->at += 8;
		break;
	case 8:
	case 11:
		if (kind == 11)
			next_u16(r);
		v->kind = value_integer;
		v->integer = next_i32(r);
		break;
	case 9:
		v->kind = value_string;
		return read_string_ref(r, &v->text, &v->length);
	case 10:
		return S_OK;
	default:
		return E_NOTIMPL;
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
		return raven_xaml_element_set_text(object, name, length, value->text, value->length);
	case value_bool:
	case value_integer:
		return raven_xaml_element_set_integer(object, name, length, value->integer);
	case value_float:
		return raven_xaml_elements_set_float(object, name, length, (float)value->real);
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
			if (!expected) return E_NOTIMPL;
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
 * resource, the theme stage. */
static HRESULT resolve_resource(const struct scope *stack, size_t depth, int theme_resource, const WCHAR *key,
	UINT32 length, IInspectable **out)
{
	const WCHAR *theme = NULL;
	UINT32 theme_length = 0;
	IInspectable *dictionary = NULL;
	HRESULT result = E_BOUNDS;
	*out = NULL;
	if (theme_resource)
		requested_theme_name(&theme, &theme_length);
	for (size_t index = depth; index; ) {
		dictionary = raven_xaml_element_resources_existing(stack[--index].object);
		if (!dictionary)
			continue;
		result = theme_resource ? raven_xaml_dictionary_lookup_theme(dictionary, key, length, theme, theme_length, out) :
			raven_xaml_dictionary_lookup(dictionary, key, length, out);
		if (SUCCEEDED(result))
			return result;
	}
	result = application_resources(&dictionary);
	if (FAILED(result) || !dictionary)
		return E_BOUNDS;
	result = theme_resource ? raven_xaml_dictionary_lookup_theme(dictionary, key, length, theme, theme_length, out) :
		raven_xaml_dictionary_lookup(dictionary, key, length, out);
	IInspectable_Release(dictionary);
	return result;
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
		return E_NOTIMPL;
	result = IInspectable_QueryInterface(object, &IID___FIMap_2_IInspectable_IInspectable, (void **)&map);
	if (FAILED(result))
		return result;
	key_object = raven_xaml_boxed_string(key->text, key->length);
	if (!key_object) { result = E_OUTOFMEMORY; goto done; }
	if (value)
		value_object = value;
	else if (constant && constant->kind == value_string && constant->text)
		value_object = raven_xaml_boxed_string(constant->text, constant->length);
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

/* The runtime-data payload of a SetCustomRuntimeData node, by the type index Microsoft's XBF reader dispatches on. The
 * ResourceDictionary versions are decoded to their end so the node stream can continue; a Style's or a deferred element's
 * payload is not decoded here, and the node refuses rather than guess its length. */
static HRESULT read_custom_runtime_data(struct reader *r, uint32_t *type_index)
{
	const uint32_t type = (uint32_t)next_vint(r);
	int complete = 0;
	switch (type) {
	case 371: /* ResourceDictionary_v1 */
		complete = skip_resource_map(r) && skip_token_vector(r) && skip_resource_map(r) &&
			skip_string_vector(r) && skip_string_vector(r);
		break;
	case 7: /* ResourceDictionary_v2 */
		complete = skip_resource_map(r) && skip_token_vector(r) && skip_resource_map(r) && skip_string_vector(r);
		break;
	case 10: /* ResourceDictionary_v3 */
		complete = skip_resource_map(r) && skip_token_vector(r) && skip_resource_map(r) &&
			skip_conditional_map(r) && skip_conditional_map(r);
		break;
	case 12: /* ResourceDictionary_v4 */
		complete = skip_key_map(r) && skip_token_vector(r) && skip_conditional_key_map(r);
		break;
	default:
		return E_NOTIMPL;
	}
	/* v3 and v4 also carry the objects declared under a conditional scope. */
	if (complete && (type == 10 || type == 12))
		complete = skip_predicates(r);
	if (!complete || r->invalid)
		return E_INVALIDARG;
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

static HRESULT load_nodes(const struct metadata *m, size_t begin, size_t end, IInspectable **root,
	IInspectable *target, BOOL *target_used, const struct load_context *context, int deferral_depth)
{
	struct reader r = {m->data, begin, end, 0};
	struct scope stack[LOADER_DEPTH];
	size_t depth = 0;
	const WCHAR *pending_name = NULL;
	IInspectable *last_object = NULL;
	struct value pending_value = {value_none, NULL, 0, 0, 0};
	BOOL pending_value_valid = FALSE;
	UINT32 pending_length = 0;
	HRESULT status = S_OK;
	while (r.at < r.end) {
		const uint8_t kind = next_byte(&r);
		/* A pushed constant is the value of the dictionary entry the next keyed insertion consumes; any other node in
		 * between leaves it unused, the way an ignored PushConstant always behaved. */
		if (kind != 4 && kind != 10)
			pending_value_valid = FALSE;
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
				}
				if (scope->object && depth && (stack[depth - 1].kind == 3 ||
					(scope->kind == 0 && is_map_object(stack[depth - 1].object)))) {
					/* A value a collection or map scope receives: the append or the keyed insertion takes it. */
					if (last_object) IInspectable_Release(last_object);
					last_object = scope->object;
				} else if (scope->object) IInspectable_Release(scope->object);
			}
			break;
		case 8:
			if (!depth || stack[depth - 1].kind != 3 || !last_object) { status = E_INVALIDARG; goto done; }
			status = append_dictionary(stack[depth - 1].object, last_object);
			IInspectable_Release(last_object); last_object = NULL;
			if (FAILED(status)) goto done;
			break;
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
			} else status = create_reference(m, reference, &object);
			if (target && (FAILED(status) || !object)) { if (SUCCEEDED(status)) status = E_UNEXPECTED; goto done; }
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
			if (target && (FAILED(status) || !object)) { if (SUCCEEDED(status)) status = E_UNEXPECTED; goto done; }
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
						if (target && applied != S_OK) {
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
				IInspectable_Release(object);
			}
			break;
		}
		case 3: case 18:
			next_u16(&r);
			status = skip_string(&r);
			if (FAILED(status)) goto done;
			if (kind == 18) {
				if (depth == LOADER_DEPTH) { status = E_INVALIDARG; goto done; }
				memset(&stack[depth++], 0, sizeof stack[0]);
			}
			break;
		case 13: {
			struct value value = {value_none, NULL, 0, 0, 0};
			status = read_constant(&r, m, &value);
			if (FAILED(status))
				goto done;
			if (depth && value.kind == value_string) {
				HRESULT applied = raven_xaml_element_set_name(stack[depth - 1].object, value.text, value.length);
				if (target && applied != S_OK) { status = FAILED(applied) ? applied : E_NOTIMPL; goto done; }
			} else if (target) { status = E_NOTIMPL; goto done; }
			break;
		}
		case 7: case 19: case 32: {
			const uint16_t reference = next_u16(&r);
			const WCHAR *name = NULL;
			UINT32 length = 0;
			pending_name = NULL;
			status = property_reference(m, reference, &name, &length);
			if (FAILED(status)) { if (target) goto done; status = S_OK; break; }
			if (kind == 19) {
				IInspectable *collection = NULL;
				if (!depth || depth == LOADER_DEPTH) {
					status = E_NOTIMPL; goto done;
				}
				if (length == 18 && !memcmp(name, L"MergedDictionaries", 18 * sizeof(WCHAR))) {
					status = get_merged_dictionaries(stack[depth - 1].object, &collection);
					if (FAILED(status)) goto done;
					memset(&stack[depth], 0, sizeof stack[depth]);
					stack[depth].object = collection; stack[depth].kind = 3; ++depth;
				} else if (length == 17 && !memcmp(name, L"ThemeDictionaries", 17 * sizeof(WCHAR))) {
					status = get_theme_dictionaries(stack[depth - 1].object, &collection);
					if (FAILED(status)) goto done;
					memset(&stack[depth], 0, sizeof stack[depth]);
					stack[depth].object = collection; stack[depth].kind = 5; ++depth;
				} else {
					status = E_NOTIMPL; goto done;
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
				break;
			}
			if (target) { status = E_NOTIMPL; goto done; }
			break;
		}
		case 10: {
			struct value key = {value_none, NULL, 0, 0, 0};
			status = read_constant(&r, m, &key);
			if (FAILED(status)) goto done;
			status = add_to_dictionary(depth ? stack[depth - 1].object : NULL, &key, last_object,
				pending_value_valid ? &pending_value : NULL);
			if (last_object) IInspectable_Release(last_object);
			last_object = NULL;
			pending_value_valid = FALSE;
			if (FAILED(status)) goto done;
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
			if (r.invalid || statics > 0x10000 || themes > 0x10000) { status = E_INVALIDARG; goto done; }
			for (uint32_t index = 0; index < statics + themes; ++index)
				next_u16(&r);
			status = read_custom_runtime_data(&r, &runtime_type);
			if (FAILED(status))
				goto done;
			/* A dictionary's deferred content is the substream itself: load it and let it take the placeholder's place,
			 * so the enclosing map receives the finished dictionary. A substream that cannot be loaded leaves the
			 * placeholder in place, which is the skip the loader gives any unapplied value. */
			if (depth && context && deferral_depth < 2) {
				size_t deferred_begin = 0, deferred_end = 0;
				if (substream_range(context, stream_index, &deferred_begin, &deferred_end) &&
					deferred_begin < deferred_end) {
					IInspectable *deferred = NULL;
					const HRESULT loaded = load_nodes(m, deferred_begin, deferred_end, &deferred, NULL, NULL, context,
						deferral_depth + 1);
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
					if (deferred)
						IInspectable_Release(deferred);
					if (target && FAILED(loaded)) {
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
			if (target) { status = E_NOTIMPL; goto done; }
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
					applied = apply_value(stack[depth - 1].object, name, length, &value);
				} else if (value.kind == value_string && value.text) {
					IInspectable *resolved = NULL;
					if (SUCCEEDED(resolve_resource(stack, depth, kind == 36, value.text, value.length, &resolved)) &&
						resolved) {
						applied = raven_xaml_element_apply_value(stack[depth - 1].object, name, length, resolved);
						IInspectable_Release(resolved);
					}
				}
				if (target && applied != S_OK) { status = FAILED(applied) ? applied : E_NOTIMPL; goto done; }
			} else if (target) { status = E_NOTIMPL; goto done; }
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
				if (target && applied != S_OK) { status = FAILED(applied) ? applied : E_NOTIMPL; goto done; }
			} else if (target) { status = E_NOTIMPL; goto done; }
			break;
		}
		case 28: case 31: case 38:
			next_u16(&r);
			next_u16(&r);
			if (target) { status = r.invalid ? E_INVALIDARG : E_NOTIMPL; goto done; }
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
	}
done:
	if (SUCCEEDED(status) && target && (pending_name || last_object)) status = E_INVALIDARG;
	if (last_object) IInspectable_Release(last_object);
	/* The scopes unwind without their deferred applications: a failed document never half-applies the templates it
	 * declared, and the objects the unwind releases own the only references the deferred targets held through them. */
	while (depth) {
		IInspectable *object = stack[--depth].object;
		if (object)
			IInspectable_Release(object);
	}
	return status;
}

static HRESULT load_document(const void *data, uint32_t size, IInspectable **root, IInspectable *target)
{
	const uint8_t *bytes = (const uint8_t *)data;
	uint32_t metadata_size, node_size, count;
	size_t table, node_end, substreams;
	struct metadata metadata;
	struct load_context context;
	HRESULT status;
	BOOL target_used = FALSE;
	if (!root)
		return E_POINTER;
	*root = NULL;
	if (!bytes || size < 12 || bytes[0] != 'X' || bytes[1] != 'B' || bytes[2] != 'F' || bytes[3] != 0)
		return E_INVALIDARG;
	metadata_size = read_u32(bytes + 4);
	node_size = read_u32(bytes + 8);
	if (metadata_size > size - 12)
		return E_INVALIDARG;
	table = (size_t)12 + metadata_size;
	if (node_size < 4 || (size_t)node_size > size - table)
		return E_INVALIDARG;
	node_end = table + node_size;
	status = metadata_parse(&metadata, bytes, table);
	if (FAILED(status))
		return status;
	count = read_u32(bytes + table);
	if ((size_t)count > (node_end - table - 4) / 8)
		return E_INVALIDARG;
	substreams = table + 4 + (size_t)count * 8;
	context.m = &metadata;
	context.table = table;
	context.nodes = substreams;
	context.count = count;
	/* Validate every extent before creating any objects; a later invalid stream cannot leak the first root. */
	for (uint32_t index = 0; index < count; ++index) {
		const uint32_t node_offset = read_u32(bytes + table + 4 + (size_t)index * 8);
		const uint32_t line_offset = read_u32(bytes + table + 4 + (size_t)index * 8 + 4);
		if (node_offset > line_offset || line_offset > node_end - substreams)
			return E_INVALIDARG;
	}
	for (uint32_t index = 0; index < count && (!target || index == 0); ++index) {
		const uint32_t node_offset = read_u32(bytes + table + 4 + (size_t)index * 8);
		const uint32_t line_offset = read_u32(bytes + table + 4 + (size_t)index * 8 + 4);
		const size_t begin = substreams + node_offset, end = substreams + line_offset;
		status = load_nodes(&metadata, begin, end, root, target, &target_used, &context, 0);
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
__declspec(dllexport) HRESULT raven_xaml_load_into(const void *data, uint32_t size, IInspectable *target)
{
	IInspectable *root = NULL;
	if (!target) return E_POINTER;
	HRESULT result = load_document(data, size, &root, target);
	if (root) IInspectable_Release(root);
	return result;
}
