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
 * time through the element side's hook. Both classes build through the engine-internal factories — the stable type table
 * does not carry them.
 *
 * The entry point is exported as RavenXamlLoad so the engine and its tests can hand it a compiled page. */
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <stdint.h>
#include <winstring.h>

#define MAX_NAME 256

struct reader {
	const uint8_t *data;
	size_t at, end;
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

static uint8_t next_byte(struct reader *r) { return r->at < r->end ? r->data[r->at++] : 0; }
static uint16_t next_u16(struct reader *r)
{
	uint16_t value = r->at + 2 <= r->end ? read_u16(r->data + r->at) : 0;
	r->at += 2;
	return value;
}
static int32_t next_i32(struct reader *r)
{
	int32_t value = r->at + 4 <= r->end ? (int32_t)read_u32(r->data + r->at) : 0;
	r->at += 4;
	return value;
}
static double next_f32(struct reader *r)
{
	union { uint32_t bits; float number; } converter;
	converter.bits = r->at + 4 <= r->end ? read_u32(r->data + r->at) : 0;
	r->at += 4;
	return converter.number;
}
static int32_t next_vint(struct reader *r)
{
	int32_t value = 0;
	int shift = 0;
	for (;;) {
		const uint8_t byte = next_byte(r);
		value |= (int32_t)(byte & 0x7F) << shift;
		if (!(byte & 0x80))
			return value;
		shift += 7;
	}
}

static HRESULT read_string_ref(struct reader *r, const WCHAR **text, UINT32 *length)
{
	const int32_t size = next_i32(r);
	if (size < 0 || (size_t)size > (r->end - r->at) / 2)
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
	for (int index = 0; index < 6; ++index)
		offsets[index] = read_u64(data + start + 8 + index * 8);
	m->strings = start + (size_t)offsets[0];
	m->assemblies = start + (size_t)offsets[1];
	m->namespaces = start + (size_t)offsets[2];
	m->types = start + (size_t)offsets[3];
	m->properties = start + (size_t)offsets[4];
	m->xmlns = start + (size_t)offsets[5];
	if (m->strings + 4 > size || m->assemblies + 4 > size || m->namespaces + 4 > size ||
		m->types + 4 > size || m->properties + 4 > size || m->xmlns + 4 > size)
		return E_INVALIDARG;
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
	/* Walk the string table once to learn whether every entry carries a null terminator, as older metadata does. */
	for (int terminated = 0; terminated < 2; ++terminated) {
		position = m->strings + 4;
		for (uint32_t index = 0; index < m->string_count; ++index) {
			int32_t length;
			if (position + 4 > size)
				return E_INVALIDARG;
			length = (int32_t)read_u32(data + position);
			if (length < 0 || (size_t)length > (size - position) / 2)
				return E_INVALIDARG;
			position += 4 + (size_t)length * 2 + (terminated ? 2 : 0);
			if (position > size)
				return E_INVALIDARG;
		}
		if (position == m->assemblies) {
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

static HRESULT metadata_type_name(const struct metadata *m, uint32_t id, WCHAR *buffer, size_t capacity, UINT32 *length)
{
	size_t position;
	uint32_t namespace_id, name_id, namespace_name_id;
	const WCHAR *namespace_text = NULL, *name_text = NULL;
	UINT32 namespace_length = 0, name_length = 0;
	if (id >= m->type_count)
		return E_INVALIDARG;
	position = m->types + 4 + (size_t)id * 12;
	namespace_id = read_u32(m->data + position + 4);
	name_id = read_u32(m->data + position + 8);
	if (namespace_id >= m->namespace_count)
		return E_INVALIDARG;
	namespace_name_id = read_u32(m->data + m->namespaces + 4 + (size_t)namespace_id * 8 + 4);
	if (FAILED(metadata_string(m, namespace_name_id, &namespace_text, &namespace_length)) ||
		FAILED(metadata_string(m, name_id, &name_text, &name_length)))
		return E_INVALIDARG;
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
	if (reference & 0x8000)
		return raven_xaml_create_stable(reference & 0x7FFF, out);
	if (FAILED(metadata_type_name(m, reference, buffer, MAX_NAME, &length)))
		return E_INVALIDARG;
	{
		const HRESULT known = create_known(buffer, length, out);
		if (known != CLASS_E_CLASSNOTAVAILABLE)
			return known;
	}
	return raven_xaml_create_named(buffer, length, out);
}

/* The template-path classes a compiled document names that the stable type table does not carry: ControlTemplate (the
 * value a "Template" property declares) and ContentPresenter (the element inside a template that shows the control's
 * content). Both build through the engine-internal factories, so a document's own type table resolves them without a
 * running application's metadata provider; every other document-named type keeps the provider path. */
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
		return S_OK;
	case 4:
		v->kind = value_integer;
		v->integer = next_i32(r);
		return S_OK;
	case 5: {
		const uint16_t reference = next_u16(r);
		if (reference & 0x8000)
			return S_OK;
		v->kind = value_string;
		return metadata_string(m, reference, &v->text, &v->length);
	}
	case 6:
		r->at += 16;
		break;
	case 7:
		r->at += 8;
		break;
	case 8:
	case 11:
		if (kind == 11)
			next_u16(r);
		v->kind = value_integer;
		v->integer = (int32_t)(r->at + 4 <= r->end ? read_u32(r->data + r->at) : 0);
		r->at += 4;
		break;
	case 9:
		v->kind = value_string;
		return read_string_ref(r, &v->text, &v->length);
	case 10:
		return S_OK;
	default:
		return E_NOTIMPL;
	}
	return r->at <= r->end ? S_OK : E_INVALIDARG;
}

static HRESULT apply_value(IInspectable *object, const WCHAR *name, UINT32 length, const struct value *value)
{
	static const WCHAR name_property[] = L"Name";
	if (length == ARRAYSIZE(name_property) - 1 && CompareStringOrdinal(name, (int)length, name_property, -1, FALSE) == CSTR_EQUAL) {
		if (value->kind == value_string)
			return raven_xaml_element_set_name(object, value->text, value->length);
		return S_FALSE;
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

#define LOADER_DEPTH 64

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

static HRESULT load_nodes(const struct metadata *m, size_t begin, size_t end, IInspectable **root)
{
	struct reader r = {m->data, begin, end};
	struct scope stack[LOADER_DEPTH];
	size_t depth = 0;
	const WCHAR *pending_name = NULL;
	UINT32 pending_length = 0;
	HRESULT status = S_OK;
	while (r.at < r.end) {
		const uint8_t kind = next_byte(&r);
		switch (kind) {
		case 1:
			if (depth < LOADER_DEPTH) {
				stack[depth].object = NULL;
				stack[depth].kind = 0;
				stack[depth].deferred_target = NULL;
				stack[depth].deferred_name = NULL;
				stack[depth].deferred_length = 0;
				++depth;
			}
			break;
		case 2: case 33: case 39: case 40:
			if (depth) {
				struct scope *scope = &stack[--depth];
				/* The deferred template application: the template scope closes with its root attached, so the control
				 * stores the finished tree. A failed application fails the document the way a failed scoped property
				 * does — the caller is not handed a tree that silently lost its template. */
				if (scope->kind && scope->deferred_target && scope->object) {
					const HRESULT applied = raven_xaml_element_set_object(scope->deferred_target,
						scope->deferred_name, scope->deferred_length, scope->object);
					if (FAILED(applied)) {
						if (scope->object)
							IInspectable_Release(scope->object);
						status = applied;
						goto done;
					}
				}
				if (scope->object)
					IInspectable_Release(scope->object);
			}
			break;
		case 20: case 21: case 22: {
			const uint16_t reference = next_u16(&r);
			IInspectable *object = NULL;
			struct value value = {value_none, NULL, 0, 0, 0};
			if (kind != 20) {
				status = read_constant(&r, m, &value);
				if (FAILED(status))
					goto done;
			}
			status = create_reference(m, reference, &object);
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
					if (pending_name && depth) {
						const HRESULT applied = raven_xaml_element_set_object(stack[depth - 1].object, pending_name, pending_length, object);
						if (FAILED(applied)) {
							IInspectable_Release(object);
							status = applied;
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
						stack[depth].kind = 0;
						stack[depth].deferred_target = NULL;
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
			if (kind != 23) {
				status = read_constant(&r, m, &value);
				if (FAILED(status))
					goto done;
			}
			if (SUCCEEDED(create_reference(m, reference, &object)) && object) {
				if (depth && stack[depth - 1].kind == 1) {
					/* A non-scoped object under a template scope is the root the same way a scoped one is. */
					if (raven_xaml_control_template_put_root(stack[depth - 1].object, object) == S_OK)
						stack[depth - 1].kind = 2;
					pending_name = NULL;
				} else {
					if (pending_name && depth)
						(void)raven_xaml_element_set_object(stack[depth - 1].object, pending_name, pending_length, object);
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
			if (FAILED(status))
				goto done;
			break;
		case 13: {
			struct value value = {value_none, NULL, 0, 0, 0};
			status = read_constant(&r, m, &value);
			if (FAILED(status))
				goto done;
			if (depth && value.kind == value_string)
				(void)raven_xaml_element_set_name(stack[depth - 1].object, value.text, value.length);
			break;
		}
		case 7: case 19: case 32: {
			const uint16_t reference = next_u16(&r);
			const WCHAR *name = NULL;
			UINT32 length = 0;
			pending_name = NULL;
			if (SUCCEEDED(property_reference(m, reference, &name, &length))) {
				pending_name = name;
				pending_length = length;
			}
			break;
		}
		case 4: case 10: case 12: case 14: case 34: case 35: {
			struct value value = {value_none, NULL, 0, 0, 0};
			status = read_constant(&r, m, &value);
			if (FAILED(status))
				goto done;
			break;
		}
		case 11:
			status = skip_string(&r);
			if (FAILED(status))
				goto done;
			break;
		case 17: {
			const int32_t substream = (next_u16(&r), next_vint(&r));
			const int32_t statics = next_vint(&r);
			const int32_t themes = next_vint(&r);
			(void)substream;
			for (int32_t index = 0; index < statics + themes; ++index)
				next_u16(&r);
			break;
		}
		case 26: case 27: case 30: case 36: {
			const uint16_t reference = next_u16(&r);
			const WCHAR *name = NULL;
			UINT32 length = 0;
			struct value value = {value_none, NULL, 0, 0, 0};
			status = read_constant(&r, m, &value);
			if (FAILED(status))
				goto done;
			if (depth && SUCCEEDED(property_reference(m, reference, &name, &length)))
				(void)apply_value(stack[depth - 1].object, name, length, &value);
			break;
		}
		case 28: case 29: case 31: case 38:
			next_u16(&r);
			next_u16(&r);
			break;
		default:
			status = E_NOTIMPL;
			goto done;
		}
	}
done:
	/* The scopes unwind without their deferred applications: a failed document never half-applies the templates it
	 * declared, and the objects the unwind releases own the only references the deferred targets held through them. */
	while (depth) {
		IInspectable *object = stack[--depth].object;
		if (object)
			IInspectable_Release(object);
	}
	return status;
}

__declspec(dllexport) HRESULT WINAPI RavenXamlLoad(const void *data, uint32_t size, IInspectable **root)
{
	const uint8_t *bytes = (const uint8_t *)data;
	uint32_t metadata_size, node_size, count;
	size_t table, substreams;
	struct metadata metadata;
	HRESULT status;
	if (!root)
		return E_POINTER;
	*root = NULL;
	if (!bytes || size < 12 || bytes[0] != 'X' || bytes[1] != 'B' || bytes[2] != 'F' || bytes[3] != 0)
		return E_INVALIDARG;
	metadata_size = read_u32(bytes + 4);
	node_size = read_u32(bytes + 8);
	table = (size_t)12 + metadata_size;
	if (table + 4 > size || (size_t)node_size > size - table)
		return E_INVALIDARG;
	status = metadata_parse(&metadata, bytes, size);
	if (FAILED(status))
		return status;
	count = read_u32(bytes + table);
	substreams = table + 4 + (size_t)count * 8;
	if (substreams > size)
		return E_INVALIDARG;
	for (uint32_t index = 0; index < count; ++index) {
		const uint32_t node_offset = read_u32(bytes + table + 4 + index * 8);
		const uint32_t line_offset = read_u32(bytes + table + 4 + index * 8 + 4);
		const size_t begin = substreams + node_offset, end = substreams + line_offset;
		if (end > size || begin > end)
			return E_INVALIDARG;
		status = load_nodes(&metadata, begin, end, root);
		if (FAILED(status)) {
			if (*root) {
				IInspectable_Release(*root);
				*root = NULL;
			}
			return status;
		}
	}
	return S_OK;
}
