/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.UI.Xaml.Style class with the Setter and SetterBaseCollection it owns.
 *
 * A Style is the object a resource dictionary hands out and an element's Style property takes: it carries the TargetType
 * TypeName, the BasedOn reference and the Setters collection, and answers IStyle, whose members the winmd orders as
 * IsSealed, Setters, TargetType, BasedOn and Seal. The Setters collection is the SetterBaseCollection class, whose default
 * interface is the parameterized IVector`1<SetterBase> and whose ISetterBaseCollection part carries IsSealed — the same
 * part-struct shape the ResourceDictionary uses for its map. A Setter answers ISetter (its default), ISetter2 and
 * ISetterBase; the object is one COM identity and every part answers for the others.
 *
 * DependencyProperty is not implemented yet, so a Setter cannot carry a real property: ISetter.put_Property answers
 * E_NOTIMPL at its exact signature, and the engine addresses setters by their property NAME through
 * raven_style_add_setter, which stores (property-name string, value IInspectable) pairs the render readback in
 * xaml_elements.c walks. The Value property is real: put_Value takes the value as an IInspectable and get_Value hands it
 * back, so a Setter created through the factory already round-trips its value.
 *
 * The interface declarations come from Windows.UI.Xaml.winmd by scripts/generate-winrt-header.py into
 * windows_ui_xaml_style.h; the TargetType slots use the real TypeName struct mingw declares and the IVector`1<SetterBase>
 * instantiation is the hand derivation the header documents. Members the engine does not implement yet answer E_NOTIMPL,
 * and every stub keeps the exact signature of its slot so a call never depends on an argument count the metadata did not
 * give. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <string.h>
#include "windows_ui_xaml_style.h"
#include "xaml_styles.h"

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

typedef __FIVector_1_Windows__CUI__CXaml__CSetterBase setter_vector1;
typedef __FIVector_1_Windows__CUI__CXaml__CSetterBaseVtbl setter_vector1_vtbl;

static const WCHAR style_class[] = L"Windows.UI.Xaml.Style";
static const WCHAR setter_class[] = L"Windows.UI.Xaml.Setter";
static const WCHAR setter_base_collection_class[] = L"Windows.UI.Xaml.SetterBaseCollection";
static const WCHAR vector_class_name[] =
	L"Windows.Foundation.Collections.IVector`1<Windows.UI.Xaml.SetterBase>";

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

static int same_text(const WCHAR *text, UINT32 length, const WCHAR *name)
{
	return text && CompareStringOrdinal(text, (int)length, name, -1, FALSE) == CSTR_EQUAL;
}

/* The stored name: an HSTRING slot replaced in place, the way the element classes store theirs. */
static void store_name(HSTRING *slot, const WCHAR *text, UINT32 length)
{
	if (*slot) {
		WindowsDeleteString(*slot);
		*slot = NULL;
	}
	if (text && length)
		WindowsCreateString(text, length, slot);
}

#define STUB1(iface, name) static HRESULT WINAPI name(iface *This, uint64_t a1) \
	{ (void)This; if (a1) *(void **)(uintptr_t)a1 = NULL; return E_NOTIMPL; }
#define STUB1_SET(iface, name) static HRESULT WINAPI name(iface *This, uint64_t a1) \
	{ (void)This; (void)a1; return E_NOTIMPL; }

/* The Setter */

struct setter_object;

struct setter2_part { const __x_ABI_CWindows_CUI_CXaml_CISetter2Vtbl *lpVtbl; struct setter_object *owner; };
struct setterbase_part { const __x_ABI_CWindows_CUI_CXaml_CISetterBaseVtbl *lpVtbl; struct setter_object *owner; };

struct setter_object {
	const __x_ABI_CWindows_CUI_CXaml_CISetterVtbl *lpVtbl;
	struct setter2_part setter2;
	struct setterbase_part base;
	LONG refs;
	BOOL sealed;
	/* The property NAME the engine addresses the setter by; a real DependencyProperty reference arrives with the
	 * dependency-property stage, and until then ISetter.put_Property answers E_NOTIMPL. */
	HSTRING property;
	IInspectable *value;
};

static ULONG setter_addref_any(struct setter_object *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG setter_release_any(struct setter_object *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->property)
			WindowsDeleteString(self->property);
		if (self->value)
			self->value->lpVtbl->Release(self->value);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT setter_query_any(struct setter_object *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CISetter))
		*out = self;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CISetter2))
		*out = &self->setter2;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CISetterBase))
		*out = &self->base;
	else
		return E_NOINTERFACE;
	setter_addref_any(self);
	return S_OK;
}

/* Every part of the setter answers as one COM object: the QI, the refs and the class name live on the owner, the way the
 * element and dictionary parts do. The default part is the object itself. */
#define SETTER_PART(part, iface, prefix) \
	static HRESULT WINAPI prefix##_query(iface *This, REFIID iid, void **out) \
	{ return setter_query_any(((struct part *)This)->owner, iid, out); } \
	static ULONG WINAPI prefix##_addref(iface *This) { return setter_addref_any(((struct part *)This)->owner); } \
	static ULONG WINAPI prefix##_release(iface *This) { return setter_release_any(((struct part *)This)->owner); } \
	static HRESULT WINAPI prefix##_iids(iface *This, ULONG *count, IID **iids) \
	{ (void)This; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI prefix##_name(iface *This, HSTRING *name) \
	{ (void)This; if (!name) return E_POINTER; return WindowsCreateString(setter_class, (UINT32)lstrlenW(setter_class), name); } \
	static HRESULT WINAPI prefix##_trust(iface *This, TrustLevel *level) \
	{ (void)This; return inspectable_trust(level); }

SETTER_PART(setter2_part, __x_ABI_CWindows_CUI_CXaml_CISetter2, setter2p)
SETTER_PART(setterbase_part, __x_ABI_CWindows_CUI_CXaml_CISetterBase, setterbasep)

static HRESULT WINAPI setter_query(__x_ABI_CWindows_CUI_CXaml_CISetter *This, REFIID iid, void **out)
{
	return setter_query_any((struct setter_object *)This, iid, out);
}
static ULONG WINAPI setter_addref(__x_ABI_CWindows_CUI_CXaml_CISetter *This)
{
	return setter_addref_any((struct setter_object *)This);
}
static ULONG WINAPI setter_release(__x_ABI_CWindows_CUI_CXaml_CISetter *This)
{
	return setter_release_any((struct setter_object *)This);
}

static HRESULT WINAPI setter_iids(__x_ABI_CWindows_CUI_CXaml_CISetter *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI setter_name(__x_ABI_CWindows_CUI_CXaml_CISetter *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(setter_class, (UINT32)lstrlenW(setter_class), name);
}

static HRESULT WINAPI setter_trust(__x_ABI_CWindows_CUI_CXaml_CISetter *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

/* Property takes a DependencyProperty, which the engine has not yet; the name path is raven_style_add_setter. */
STUB1(__x_ABI_CWindows_CUI_CXaml_CISetter, setter_property)
STUB1_SET(__x_ABI_CWindows_CUI_CXaml_CISetter, setter_put_property)

static HRESULT WINAPI setter_value(__x_ABI_CWindows_CUI_CXaml_CISetter *iface, uint64_t out)
{
	struct setter_object *self = (struct setter_object *)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = self->value;
	if (self->value)
		self->value->lpVtbl->AddRef(self->value);
	return S_OK;
}

static HRESULT WINAPI setter_put_value(__x_ABI_CWindows_CUI_CXaml_CISetter *iface, uint64_t value)
{
	struct setter_object *self = (struct setter_object *)iface;
	IInspectable *object = (IInspectable *)(uintptr_t)value;
	if (self->sealed)
		return E_INVALIDARG;
	if (object)
		object->lpVtbl->AddRef(object);
	if (self->value)
		self->value->lpVtbl->Release(self->value);
	self->value = object;
	raven_render_changed();
	return S_OK;
}

static const __x_ABI_CWindows_CUI_CXaml_CISetterVtbl setter_table = {
	.QueryInterface = setter_query,
	.AddRef = setter_addref,
	.Release = setter_release,
	.GetIids = setter_iids,
	.GetRuntimeClassName = setter_name,
	.GetTrustLevel = setter_trust,
	.get_Property = setter_property,
	.put_Property = setter_put_property,
	.get_Value = setter_value,
	.put_Value = setter_put_value,
};

/* ISetter2.Target selects a named element; the target stage wires it. */
STUB1(__x_ABI_CWindows_CUI_CXaml_CISetter2, setter2_target)
STUB1_SET(__x_ABI_CWindows_CUI_CXaml_CISetter2, setter2_put_target)

static const __x_ABI_CWindows_CUI_CXaml_CISetter2Vtbl setter2_table = {
	.QueryInterface = setter2p_query,
	.AddRef = setter2p_addref,
	.Release = setter2p_release,
	.GetIids = setter2p_iids,
	.GetRuntimeClassName = setter2p_name,
	.GetTrustLevel = setter2p_trust,
	.get_Target = setter2_target,
	.put_Target = setter2_put_target,
};

static HRESULT WINAPI setterbase_is_sealed(__x_ABI_CWindows_CUI_CXaml_CISetterBase *iface, boolean *value)
{
	if (!value)
		return E_POINTER;
	*value = ((struct setter_object *)iface)->sealed;
	return S_OK;
}

static const __x_ABI_CWindows_CUI_CXaml_CISetterBaseVtbl setterbase_table = {
	.QueryInterface = setterbasep_query,
	.AddRef = setterbasep_addref,
	.Release = setterbasep_release,
	.GetIids = setterbasep_iids,
	.GetRuntimeClassName = setterbasep_name,
	.GetTrustLevel = setterbasep_trust,
	.get_IsSealed = setterbase_is_sealed,
};

static struct setter_object *create_setter(const WCHAR *property, UINT32 length, IInspectable *value)
{
	struct setter_object *setter = ALLOC(sizeof *setter);
	if (!setter)
		return NULL;
	setter->lpVtbl = &setter_table;
	setter->setter2.lpVtbl = &setter2_table;
	setter->setter2.owner = setter;
	setter->base.lpVtbl = &setterbase_table;
	setter->base.owner = setter;
	setter->refs = 1;
	if (property && length)
		WindowsCreateString(property, length, &setter->property);
	if (value) {
		value->lpVtbl->AddRef(value);
		setter->value = value;
	}
	InterlockedIncrement(&raven_xaml_objects);
	return setter;
}

/* The SetterBaseCollection: the collection IStyle.Setters hands out. Its default interface is the parameterized
 * IVector`1<SetterBase>, and the ISetterBaseCollection part carries IsSealed; the same subset the children vector
 * implements is real, and the slots the engine does not need yet answer E_NOTIMPL at their exact signatures. */

struct setters_vector;

struct collection_part { const __x_ABI_CWindows_CUI_CXaml_CISetterBaseCollectionVtbl *lpVtbl; struct setters_vector *owner; };

struct setters_vector {
	const setter_vector1_vtbl *lpVtbl;
	struct collection_part collection;
	LONG refs;
	/* Sealed with the style: the real collection faults mutation once the style sealed, and so does this. */
	BOOL sealed;
	struct setter_object **items;
	UINT32 count, capacity;
};

static ULONG vector_addref_any(struct setters_vector *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG vector_release_any(struct setters_vector *self)
{
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		for (UINT32 index = 0; index < self->count; ++index)
			if (self->items[index])
				setter_release_any(self->items[index]);
		if (self->items)
			FREE(self->items);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT vector_query_any(struct setters_vector *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___FIVector_1_Windows__CUI__CXaml__CSetterBase))
		*out = self;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CISetterBaseCollection))
		*out = &self->collection;
	else
		return E_NOINTERFACE;
	vector_addref_any(self);
	return S_OK;
}

/* The default part is the object itself; the ISetterBaseCollection part answers for it as one COM object. */
#define COLLECTION_PART(part, iface, prefix) \
	static HRESULT WINAPI prefix##_query(iface *This, REFIID iid, void **out) \
	{ return vector_query_any(((struct part *)This)->owner, iid, out); } \
	static ULONG WINAPI prefix##_addref(iface *This) { return vector_addref_any(((struct part *)This)->owner); } \
	static ULONG WINAPI prefix##_release(iface *This) { return vector_release_any(((struct part *)This)->owner); } \
	static HRESULT WINAPI prefix##_iids(iface *This, ULONG *count, IID **iids) \
	{ (void)This; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI prefix##_name(iface *This, HSTRING *name) \
	{ (void)This; if (!name) return E_POINTER; return WindowsCreateString(setter_base_collection_class, (UINT32)lstrlenW(setter_base_collection_class), name); } \
	static HRESULT WINAPI prefix##_trust(iface *This, TrustLevel *level) \
	{ (void)This; return inspectable_trust(level); }

COLLECTION_PART(collection_part, __x_ABI_CWindows_CUI_CXaml_CISetterBaseCollection, collectionp)

static HRESULT WINAPI vector_query(setter_vector1 *This, REFIID iid, void **out)
{
	return vector_query_any((struct setters_vector *)This, iid, out);
}
static ULONG WINAPI vector_addref(setter_vector1 *This)
{
	return vector_addref_any((struct setters_vector *)This);
}
static ULONG WINAPI vector_release(setter_vector1 *This)
{
	return vector_release_any((struct setters_vector *)This);
}

static HRESULT WINAPI vector_iids(setter_vector1 *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI vector_name(setter_vector1 *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(vector_class_name, (UINT32)lstrlenW(vector_class_name), name);
}

static HRESULT WINAPI vector_trust(setter_vector1 *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI vector_get_at(setter_vector1 *iface, UINT32 index, __x_ABI_CWindows_CUI_CXaml_CISetterBase **value)
{
	struct setters_vector *self = (struct setters_vector *)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (index >= self->count)
		return E_BOUNDS;
	*value = (__x_ABI_CWindows_CUI_CXaml_CISetterBase *)self->items[index];
	setter_addref_any(self->items[index]);
	return S_OK;
}

static HRESULT WINAPI vector_size(setter_vector1 *iface, UINT32 *value)
{
	struct setters_vector *self = (struct setters_vector *)iface;
	if (!value)
		return E_POINTER;
	*value = self->count;
	return S_OK;
}

static HRESULT WINAPI vector_index_of(setter_vector1 *iface, __x_ABI_CWindows_CUI_CXaml_CISetterBase *element,
	UINT32 *index, boolean *value)
{
	struct setters_vector *self = (struct setters_vector *)iface;
	if (!index || !value)
		return E_POINTER;
	*value = FALSE;
	*index = 0;
	/* Collection identity is IUnknown identity: two references to one setter can arrive through different interfaces. */
	IUnknown *unknown = NULL;
	if (!element || FAILED(((IUnknown *)element)->lpVtbl->QueryInterface((IUnknown *)element, &IID_IUnknown,
		(void **)&unknown)))
		return S_OK;
	for (UINT32 position = 0; position < self->count; ++position) {
		IUnknown *other = NULL;
		if (FAILED(((IUnknown *)self->items[position])->lpVtbl->QueryInterface((IUnknown *)self->items[position],
			&IID_IUnknown, (void **)&other)))
			continue;
		const int same = other == unknown;
		other->lpVtbl->Release(other);
		if (same) {
			*index = position;
			*value = TRUE;
			break;
		}
	}
	unknown->lpVtbl->Release(unknown);
	return S_OK;
}

static HRESULT WINAPI vector_append(setter_vector1 *iface, __x_ABI_CWindows_CUI_CXaml_CISetterBase *value)
{
	struct setters_vector *self = (struct setters_vector *)iface;
	if (!value)
		return E_POINTER;
	if (self->sealed)
		return E_INVALIDARG;
	if (self->count == self->capacity) {
		const UINT32 capacity = self->capacity ? self->capacity * 2 : 4;
		struct setter_object **items = ALLOC((size_t)capacity * sizeof *items);
		if (!items)
			return E_OUTOFMEMORY;
		if (self->items) {
			for (UINT32 index = 0; index < self->count; ++index)
				items[index] = self->items[index];
			FREE(self->items);
		}
		self->items = items;
		self->capacity = capacity;
	}
	self->items[self->count] = (struct setter_object *)value;
	setter_addref_any((struct setter_object *)value);
	++self->count;
	return S_OK;
}

static HRESULT WINAPI vector_clear(setter_vector1 *iface)
{
	struct setters_vector *self = (struct setters_vector *)iface;
	if (self->sealed)
		return E_INVALIDARG;
	for (UINT32 index = 0; index < self->count; ++index)
		if (self->items[index])
			setter_release_any(self->items[index]);
	self->count = 0;
	return S_OK;
}

STUB1(setter_vector1, vector_get_view)
static HRESULT WINAPI vector_set_at(setter_vector1 *iface, UINT32 index, __x_ABI_CWindows_CUI_CXaml_CISetterBase *value)
{ (void)iface; (void)index; (void)value; return E_NOTIMPL; }
static HRESULT WINAPI vector_insert_at(setter_vector1 *iface, UINT32 index, __x_ABI_CWindows_CUI_CXaml_CISetterBase *value)
{ (void)iface; (void)index; (void)value; return E_NOTIMPL; }
static HRESULT WINAPI vector_remove_at(setter_vector1 *iface, UINT32 index)
{ (void)iface; (void)index; return E_NOTIMPL; }
static HRESULT WINAPI vector_remove_at_end(setter_vector1 *iface)
{ (void)iface; return E_NOTIMPL; }
static HRESULT WINAPI vector_get_many(setter_vector1 *iface, UINT32 start_index, UINT32 items_size,
	__x_ABI_CWindows_CUI_CXaml_CISetterBase **items, UINT32 *value)
{
	(void)iface; (void)start_index; (void)items_size; (void)items;
	if (value)
		*value = 0;
	return E_NOTIMPL;
}
static HRESULT WINAPI vector_replace_all(setter_vector1 *iface, UINT32 count, __x_ABI_CWindows_CUI_CXaml_CISetterBase **items)
{ (void)iface; (void)count; (void)items; return E_NOTIMPL; }

static const setter_vector1_vtbl vector_table = {
	.QueryInterface = vector_query,
	.AddRef = vector_addref,
	.Release = vector_release,
	.GetIids = vector_iids,
	.GetRuntimeClassName = vector_name,
	.GetTrustLevel = vector_trust,
	.GetAt = vector_get_at,
	.get_Size = vector_size,
	.GetView = vector_get_view,
	.IndexOf = vector_index_of,
	.SetAt = vector_set_at,
	.InsertAt = vector_insert_at,
	.RemoveAt = vector_remove_at,
	.Append = vector_append,
	.RemoveAtEnd = vector_remove_at_end,
	.Clear = vector_clear,
	.GetMany = vector_get_many,
	.ReplaceAll = vector_replace_all,
};

static HRESULT WINAPI collection_is_sealed(__x_ABI_CWindows_CUI_CXaml_CISetterBaseCollection *iface, boolean *value)
{
	if (!value)
		return E_POINTER;
	*value = ((struct collection_part *)iface)->owner->sealed;
	return S_OK;
}

static const __x_ABI_CWindows_CUI_CXaml_CISetterBaseCollectionVtbl collection_table = {
	.QueryInterface = collectionp_query,
	.AddRef = collectionp_addref,
	.Release = collectionp_release,
	.GetIids = collectionp_iids,
	.GetRuntimeClassName = collectionp_name,
	.GetTrustLevel = collectionp_trust,
	.get_IsSealed = collection_is_sealed,
};

static struct setters_vector *create_setters_vector(void)
{
	struct setters_vector *vector = ALLOC(sizeof *vector);
	if (!vector)
		return NULL;
	vector->lpVtbl = &vector_table;
	vector->collection.lpVtbl = &collection_table;
	vector->collection.owner = vector;
	vector->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	return vector;
}

/* Seal walks the collection: the real Seal seals the style and every setter it owns. */
static void setters_seal(struct setters_vector *vector)
{
	vector->sealed = TRUE;
	for (UINT32 index = 0; index < vector->count; ++index)
		if (vector->items[index])
			vector->items[index]->sealed = TRUE;
}

/* The Style */

struct style_object {
	const __x_ABI_CWindows_CUI_CXaml_CIStyleVtbl *lpVtbl;
	LONG refs;
	BOOL sealed;
	/* The TargetType the interop struct carried: the name string when the struct carried one, and the kind as it
	 * arrived, so the getter hands both back the way the setter stored them. */
	HSTRING target_type;
	INT32 target_kind;
	IInspectable *based_on;
	struct setters_vector *setters;
};

static ULONG WINAPI style_addref(__x_ABI_CWindows_CUI_CXaml_CIStyle *iface)
{
	return (ULONG)InterlockedIncrement(&((struct style_object *)iface)->refs);
}

static ULONG WINAPI style_release(__x_ABI_CWindows_CUI_CXaml_CIStyle *iface)
{
	struct style_object *self = (struct style_object *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		if (self->target_type)
			WindowsDeleteString(self->target_type);
		if (self->based_on)
			self->based_on->lpVtbl->Release(self->based_on);
		if (self->setters)
			vector_release_any(self->setters);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI style_query(__x_ABI_CWindows_CUI_CXaml_CIStyle *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIStyle))
		return E_NOINTERFACE;
	style_addref(iface);
	*out = iface;
	return S_OK;
}

static HRESULT WINAPI style_iids(__x_ABI_CWindows_CUI_CXaml_CIStyle *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI style_name(__x_ABI_CWindows_CUI_CXaml_CIStyle *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(style_class, (UINT32)lstrlenW(style_class), name);
}

static HRESULT WINAPI style_trust(__x_ABI_CWindows_CUI_CXaml_CIStyle *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI style_is_sealed(__x_ABI_CWindows_CUI_CXaml_CIStyle *iface, boolean *value)
{
	if (!value)
		return E_POINTER;
	*value = ((struct style_object *)iface)->sealed;
	return S_OK;
}

static HRESULT WINAPI style_get_setters(__x_ABI_CWindows_CUI_CXaml_CIStyle *iface, uint64_t out)
{
	struct style_object *self = (struct style_object *)iface;
	if (!out)
		return E_POINTER;
	InterlockedIncrement(&self->setters->refs);
	*(void **)(uintptr_t)out = self->setters;
	return S_OK;
}

static HRESULT WINAPI style_get_target_type(__x_ABI_CWindows_CUI_CXaml_CIStyle *iface,
	__x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName *value)
{
	struct style_object *self = (struct style_object *)iface;
	if (!value)
		return E_POINTER;
	value->Name = NULL;
	if (self->target_type)
		WindowsDuplicateString(self->target_type, &value->Name);
	value->Kind = (__x_ABI_CWindows_CUI_CXaml_CInterop_CTypeKind)self->target_kind;
	return S_OK;
}

static HRESULT WINAPI style_put_target_type(__x_ABI_CWindows_CUI_CXaml_CIStyle *iface,
	__x_ABI_CWindows_CUI_CXaml_CInterop_CTypeName value)
{
	struct style_object *self = (struct style_object *)iface;
	UINT32 length = 0;
	const WCHAR *text = NULL;
	if (self->sealed)
		return E_INVALIDARG;
	/* The name is stored when the struct carries one; a nameless struct resets the TargetType, so an empty TypeName
	 * hands back from get_TargetType. */
	if (value.Name)
		text = WindowsGetStringRawBuffer(value.Name, &length);
	store_name(&self->target_type, text, length);
	self->target_kind = (INT32)value.Kind;
	return S_OK;
}

static HRESULT WINAPI style_get_based_on(__x_ABI_CWindows_CUI_CXaml_CIStyle *iface, uint64_t out)
{
	struct style_object *self = (struct style_object *)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = self->based_on;
	if (self->based_on)
		self->based_on->lpVtbl->AddRef(self->based_on);
	return S_OK;
}

static HRESULT WINAPI style_put_based_on(__x_ABI_CWindows_CUI_CXaml_CIStyle *iface, uint64_t value)
{
	struct style_object *self = (struct style_object *)iface;
	IInspectable *based_on = (IInspectable *)(uintptr_t)value;
	if (self->sealed)
		return E_INVALIDARG;
	if (based_on)
		based_on->lpVtbl->AddRef(based_on);
	if (self->based_on)
		self->based_on->lpVtbl->Release(self->based_on);
	self->based_on = based_on;
	return S_OK;
}

static HRESULT WINAPI style_seal(__x_ABI_CWindows_CUI_CXaml_CIStyle *iface)
{
	struct style_object *self = (struct style_object *)iface;
	self->sealed = TRUE;
	setters_seal(self->setters);
	return S_OK;
}

static const __x_ABI_CWindows_CUI_CXaml_CIStyleVtbl style_table = {
	.QueryInterface = style_query,
	.AddRef = style_addref,
	.Release = style_release,
	.GetIids = style_iids,
	.GetRuntimeClassName = style_name,
	.GetTrustLevel = style_trust,
	.get_IsSealed = style_is_sealed,
	.get_Setters = style_get_setters,
	.get_TargetType = style_get_target_type,
	.put_TargetType = style_put_target_type,
	.get_BasedOn = style_get_based_on,
	.put_BasedOn = style_put_based_on,
	.Seal = style_seal,
};

static struct style_object *create_style(void)
{
	struct style_object *style = ALLOC(sizeof *style);
	if (!style)
		return NULL;
	style->lpVtbl = &style_table;
	style->refs = 1;
	style->setters = create_setters_vector();
	if (!style->setters) {
		FREE(style);
		return NULL;
	}
	InterlockedIncrement(&raven_xaml_objects);
	return style;
}

/* The factory: the activation entry the catalog reaches, answering IActivationFactory and IStyleFactory from one object.
 * Style is not composable, so CreateInstance only takes a null base interface. */

struct style_factory {
	const IActivationFactoryVtbl *lpVtbl;
	const __x_ABI_CWindows_CUI_CXaml_CIStyleFactoryVtbl *style;
	LONG refs;
};

static HRESULT factory_query_any(struct style_factory *self, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory))
		*out = &self->lpVtbl;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CUI_CXaml_CIStyleFactory))
		*out = &self->style;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG factory_addref_any(struct style_factory *self)
{
	return (ULONG)InterlockedIncrement(&self->refs);
}

static ULONG factory_release_any(struct style_factory *self)
{
	return (ULONG)InterlockedDecrement(&self->refs);
}

static HRESULT WINAPI factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
	return factory_query_any((struct style_factory *)iface, iid, out);
}

static ULONG WINAPI factory_addref(IActivationFactory *iface)
{
	return factory_addref_any((struct style_factory *)iface);
}

static ULONG WINAPI factory_release(IActivationFactory *iface)
{
	return factory_release_any((struct style_factory *)iface);
}

static HRESULT WINAPI factory_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI factory_name(IActivationFactory *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(style_class, (UINT32)lstrlenW(style_class), name);
}

static HRESULT WINAPI factory_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	struct style_object *style;
	if (!out)
		return E_POINTER;
	*out = NULL;
	style = create_style();
	if (!style)
		return E_OUTOFMEMORY;
	*out = (IInspectable *)style;
	return S_OK;
}

static const IActivationFactoryVtbl activation_table = {
	.QueryInterface = factory_query,
	.AddRef = factory_addref,
	.Release = factory_release,
	.GetIids = factory_iids,
	.GetRuntimeClassName = factory_name,
	.GetTrustLevel = factory_trust,
	.ActivateInstance = factory_activate,
};

static HRESULT WINAPI factory_create(__x_ABI_CWindows_CUI_CXaml_CIStyleFactory *iface, uint64_t base_interface,
	uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	if (base_interface)
		return E_INVALIDARG;
	return factory_activate(NULL, (IInspectable **)(uintptr_t)out);
}

/* The IStyleFactory projection of the same factory object; every slot answers for both parts. */
static HRESULT WINAPI style_factory_query(__x_ABI_CWindows_CUI_CXaml_CIStyleFactory *iface, REFIID iid, void **out)
{
	return factory_query_any((struct style_factory *)iface, iid, out);
}
static ULONG WINAPI style_factory_addref(__x_ABI_CWindows_CUI_CXaml_CIStyleFactory *iface)
{
	return factory_addref_any((struct style_factory *)iface);
}
static ULONG WINAPI style_factory_release(__x_ABI_CWindows_CUI_CXaml_CIStyleFactory *iface)
{
	return factory_release_any((struct style_factory *)iface);
}
static HRESULT WINAPI style_factory_iids(__x_ABI_CWindows_CUI_CXaml_CIStyleFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}
static HRESULT WINAPI style_factory_name(__x_ABI_CWindows_CUI_CXaml_CIStyleFactory *iface, HSTRING *name)
{
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(style_class, (UINT32)lstrlenW(style_class), name);
}
static HRESULT WINAPI style_factory_trust(__x_ABI_CWindows_CUI_CXaml_CIStyleFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static const __x_ABI_CWindows_CUI_CXaml_CIStyleFactoryVtbl style_factory_table = {
	.QueryInterface = style_factory_query,
	.AddRef = style_factory_addref,
	.Release = style_factory_release,
	.GetIids = style_factory_iids,
	.GetRuntimeClassName = style_factory_name,
	.GetTrustLevel = style_factory_trust,
	.CreateInstance = factory_create,
};

static struct style_factory factory_instance = {&activation_table, &style_factory_table, 1};

HRESULT raven_style_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (!same_text(text, length, style_class))
		return CLASS_E_CLASSNOTAVAILABLE;
	InterlockedIncrement(&factory_instance.refs);
	*out = (IActivationFactory *)&factory_instance;
	return S_OK;
}

/* The engine-internal surface xaml_styles.h declares. */

/* The object behind an IInspectable reference, when it is an engine Style: the QI reaches the one part, and the vtable
 * check tells the engine's own class from a caller's implementation. */
static struct style_object *style_of(IInspectable *object)
{
	__x_ABI_CWindows_CUI_CXaml_CIStyle *style = NULL;
	if (!object || FAILED(object->lpVtbl->QueryInterface(object, &IID__x_ABI_CWindows_CUI_CXaml_CIStyle,
		(void **)&style)) || !style)
		return NULL;
	struct style_object *self = NULL;
	if (style->lpVtbl == &style_table)
		self = (struct style_object *)style;
	style->lpVtbl->Release(style);
	return self;
}

__declspec(dllexport) HRESULT raven_style_add_setter(IInspectable *style, const WCHAR *property, UINT32 length,
	IInspectable *value)
{
	struct style_object *self = style_of(style);
	struct setter_object *setter;
	HRESULT result;
	if (!self)
		return S_FALSE;
	if (!property || !length)
		return E_INVALIDARG;
	setter = create_setter(property, length, value);
	if (!setter)
		return E_OUTOFMEMORY;
	result = vector_append((setter_vector1 *)self->setters, (__x_ABI_CWindows_CUI_CXaml_CISetterBase *)setter);
	setter_release_any(setter); /* the collection owns its own reference */
	if (SUCCEEDED(result))
		raven_render_changed();
	return result;
}

HRESULT raven_style_setter_count(IInspectable *style, UINT32 *count)
{
	struct style_object *self = style_of(style);
	if (!count)
		return E_POINTER;
	*count = 0;
	if (!self)
		return S_FALSE;
	/* The pairs raven_style_add_setter stored are the engine setters of the collection; a SetterBase appended through
	 * the vector carries no property name the engine can read and is skipped here. */
	UINT32 pairs = 0;
	for (UINT32 index = 0; index < self->setters->count; ++index)
		if (self->setters->items[index] &&
			self->setters->items[index]->lpVtbl == &setter_table &&
			self->setters->items[index]->property)
			++pairs;
	*count = pairs;
	return S_OK;
}

HRESULT raven_style_setter_at(IInspectable *style, UINT32 index, const WCHAR **property,
	UINT32 *length, IInspectable **value)
{
	struct style_object *self = style_of(style);
	if (!property || !length || !value)
		return E_POINTER;
	*property = NULL;
	*length = 0;
	*value = NULL;
	if (!self)
		return S_FALSE;
	/* The name points into the style's own storage: the element holds a reference on the style, and the readback copies
	 * synchronously while it is alive. */
	UINT32 position = 0;
	for (UINT32 scan = 0; scan < self->setters->count; ++scan) {
		struct setter_object *setter = self->setters->items[scan];
		if (!setter || setter->lpVtbl != &setter_table || !setter->property)
			continue;
		if (position == index) {
			*property = WindowsGetStringRawBuffer(setter->property, length);
			*value = setter->value;
			if (*value)
				(*value)->lpVtbl->AddRef(*value);
			return *property ? S_OK : S_FALSE;
		}
		++position;
	}
	return E_BOUNDS;
}
