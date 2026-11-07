/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.ApplicationModel.Resources.Core.ResourceManager class of the Windows Runtime.
 *
 * MRT Core reads resources.pri directly: the index beside the running executable is the package's resource registration,
 * the way package.c reads the AppxManifest beside it. This file parses the mrm_pri2 subset a string lookup needs — the
 * file header and section directory, the PRI descriptor, the hierarchical schema (scope and item names), the decision
 * info (each item's candidate count), the resource map (item info and candidates) and the data item sections (UTF-16
 * string storage) — the layout scripts/pri-extract.py documents. Two field conventions of the format carry over from
 * that reference: a schema entry's name offset counts 4-byte units and is doubled into a character index, and a data
 * item string's length counts bytes while its offset counts UTF-16 units. The parse is fail-closed: a file that is
 * absent, truncated or shaped differently leaves an empty index whose lookups answer the miss, never a crash. The
 * extended [def_hnamesx] schema form stores ASCII names separately; they are expanded to UTF-16 for lookup.
 * Compiled resources can also be resolved as bounded binary blobs for the XAML loader.
 *
 * The classes: the statics hand out the per-process ResourceManager singleton, whose MainResourceMap is the root of the
 * schema; ResourceMap.GetValue answers a NamedResource, whose Resolve answers the item's first string candidate and
 * whose Candidates and ResolveAll answer the item's string candidates as a vector view. The GetValue(name, context)
 * overload accepts the caller's ResourceContext — a NULL context is the default one — but does not match candidates
 * against its qualifier values yet: the first string candidate answers, which for a single-candidate item is the same
 * answer. A lookup that misses answers E_BOUNDS with a NULL out parameter, the miss a collection lookup answers
 * elsewhere in this component. GetSubtree narrows the map to a schema scope. The candidate's qualifier view is the
 * empty one — the fixture candidates are unqualified defaults — and the file and stream value accessors answer
 * E_NOTIMPL.
 *
 * The candidates vector view is the parameterized interface
 * Windows.Foundation.Collections.IVectorView`1<ResourceCandidate>; no mingw header declares it, so the vtable follows
 * the generic definition's documented slot order and the IID is the SHA-1 of the interface signature over the
 * parameterized-interface namespace, the derivation validated against mingw's precomputed IVectorView<HSTRING>
 * (2f13c006-a03a-5f69-b090-75a4-3e33423e) and Microsoft's published projection. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <winstring.h>
#include "windows_applicationmodel_resources.h"
#include "windows_applicationmodel_resources_core.h"

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))
#ifndef E_ILLEGAL_METHOD_CALL
#define E_ILLEGAL_METHOD_CALL ((HRESULT)0x8000000e)
#endif
#ifndef E_BOUNDS
#define E_BOUNDS ((HRESULT)0x8000000b)
#endif

typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager manager_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2 manager2_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics manager_statics_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap map_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource named_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate candidate_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2 candidate2_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3 candidate3_i;
typedef __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2 context_statics2_i;

static const WCHAR class_name[] = L"Windows.ApplicationModel.Resources.Core.ResourceManager";
static const WCHAR context_class_name[] = L"Windows.ApplicationModel.Resources.Core.ResourceContext";
static const WCHAR map_class_name[] = L"Windows.ApplicationModel.Resources.Core.ResourceMap";
static const WCHAR named_class_name[] = L"Windows.ApplicationModel.Resources.Core.NamedResource";
static const WCHAR candidate_class_name[] = L"Windows.ApplicationModel.Resources.Core.ResourceCandidate";
static const WCHAR view_class_name[] =
	L"Windows.Foundation.Collections.IVectorView`1<Windows.ApplicationModel.Resources.Core.ResourceCandidate>";
static const WCHAR qualifier_view_class_name[] =
	L"Windows.Foundation.Collections.IVectorView`1<Windows.ApplicationModel.Resources.Core.ResourceQualifier>";

/* The resource reference scheme IsResourceReference answers for, compared without case like a scheme name. */
static const WCHAR reference_scheme[] = L"ms-resource:";
#define REFERENCE_SCHEME_LENGTH 12

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

static HRESULT create_string(const WCHAR *text, UINT32 length, HSTRING *out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	return WindowsCreateString(text ? text : L"", text ? length : 0, out);
}

static HRESULT create_class_string(const WCHAR *name, HSTRING *out)
{
	return create_string(name, (UINT32)lstrlenW(name), out);
}

/* The candidates vector view of the parameterized interface, declared by hand after mingw's IVectorView`1 vtables: the
 * generic definition's slots without the inherited IIterable`1.First. The element is the candidate's default interface,
 * which the candidate objects answer at their base. */
typedef struct __FIVectorView_1_Windows__CApplicationModel__CResources__CCore__CResourceCandidate candidate_view_i;
typedef struct __FIVectorView_1_Windows__CApplicationModel__CResources__CCore__CResourceCandidateVtbl candidate_view_vtbl;
typedef struct __FIVectorView_1_Windows__CApplicationModel__CResources__CCore__CResourceQualifier qualifier_view_i;
typedef struct __FIVectorView_1_Windows__CApplicationModel__CResources__CCore__CResourceQualifierVtbl qualifier_view_vtbl;

struct __FIVectorView_1_Windows__CApplicationModel__CResources__CCore__CResourceCandidateVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(candidate_view_i *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(candidate_view_i *This);
	ULONG (STDMETHODCALLTYPE *Release)(candidate_view_i *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(candidate_view_i *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(candidate_view_i *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(candidate_view_i *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetAt)(candidate_view_i *This, UINT32 index, candidate_i **value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(candidate_view_i *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *IndexOf)(candidate_view_i *This, candidate_i *element, UINT32 *index, boolean *found);
	HRESULT (STDMETHODCALLTYPE *GetMany)(candidate_view_i *This, UINT32 start_index, UINT32 items_size,
		candidate_i **items, UINT32 *value);
	END_INTERFACE
};

struct __FIVectorView_1_Windows__CApplicationModel__CResources__CCore__CResourceQualifierVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(qualifier_view_i *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(qualifier_view_i *This);
	ULONG (STDMETHODCALLTYPE *Release)(qualifier_view_i *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(qualifier_view_i *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(qualifier_view_i *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(qualifier_view_i *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetAt)(qualifier_view_i *This, UINT32 index, void **value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(qualifier_view_i *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *IndexOf)(qualifier_view_i *This, void *element, UINT32 *index, boolean *found);
	HRESULT (STDMETHODCALLTYPE *GetMany)(qualifier_view_i *This, UINT32 start_index, UINT32 items_size,
		void **items, UINT32 *value);
	END_INTERFACE
};

/* IVectorView`1<ResourceCandidate>: SHA-1 of pinterface({bbe1fa4c-b0e3-4583-baef-1f1b2e483e56};
 * rc(Windows.ApplicationModel.Resources.Core.ResourceCandidate;{af5207d9-c433-4764-b3fd-8fa6bfbcbadc})). */
static const IID view_iid = {0xe28e92f0, 0x9ffb, 0x5ea7, {0x9f, 0xc9, 0xa7, 0x3b, 0xda, 0x47, 0x18, 0x86}};
/* IVectorView`1<ResourceQualifier>, the same derivation for the empty qualifier view. */
static const IID qualifier_view_iid =
	{0xb82c58dc, 0x1cc0, 0x53f0, {0xb0, 0xf4, 0x66, 0xef, 0x39, 0xa8, 0x1c, 0xd6}};

/* The resources.pri index as this component reads it: the schema entries, the resource map tables and the data item
 * sections, all pointing into the file buffer that stays loaded for the process. Every count is capped and
 * bounds-checked against the file, so a truncated or hostile index parses into an empty state instead of a crash. */
#define PRI_MAX_SECTIONS 4096
#define PRI_MAX_LIST 1024
#define PRI_MAX_TABLE (1u << 20)
#define PRI_MAX_DATA_SECTIONS 1024
#define PRI_MAX_NAME_UNITS 1024
#define PRI_MAX_PATH_UNITS 1024
#define PRI_MAX_VALUE_UNITS 65536
#define PRI_CHAIN_MAX 64
#define PRI_FOOTER_MAGIC 0xDEF5FADEU

struct pri_schema_entry {
	UINT16 parent;
	UINT16 flags; /* 0x10 marks a scope; the items carry values; the low two
	               * bits hold the name offset's bits 16-17 */
	UINT32 offset; /* name offset in the unicode block, in 4-byte units, with
	                * the flags' low two bits as its bits 16-17 */
	UINT16 name_len; /* name length in UTF-16 units, 0 reads to the first terminator */
	UINT16 index; /* the item's index into the resource map's item info */
};

struct pri_data_section {
	UINT32 string_count, blob_count;
	const BYTE *table, *blob_table; /* string_count (u16 byte offset, u16 byte length) pairs */
	const BYTE *data; /* the stored strings */
	UINT32 data_bytes;
};

struct pri_state {
	BYTE *file;
	UINT32 file_length;
	UINT32 entry_count;
	struct pri_schema_entry *entries;
	const WCHAR *block;
	UINT32 block_units;
	WCHAR *ascii_block;
	UINT32 ascii_units;
	UINT32 iteminfo_count; /* (u16 decision, u16 first candidate) per item */
	const BYTE *iteminfo;
	UINT32 decision_count; /* (u16 first set, u16 set count) per decision */
	const BYTE *decisions;
	UINT32 candidate_count; /* 8 bytes per candidate: kind, value type, source, item, section */
	const BYTE *candidates;
	struct pri_data_section data[PRI_MAX_DATA_SECTIONS];
	UINT16 data_section_index[PRI_MAX_DATA_SECTIONS]; /* directory index per data section */
	UINT32 data_count;
};

static struct pri_state pri_state;

/* A package dependency's own index: a framework package such as Microsoft.UI.Xaml.2.8 ships its resources.pri, whose
 * theme documents an ms-appx URI with that package as the host names. The maps load on demand beside the main one; a
 * lookup activates the map it searches for the duration of the call (the loaders here are single-threaded callers). */
#define PRI_MAX_EXTRA_MAPS 4
#define PRI_MAX_PACKAGE_UNITS 128
struct pri_extra_map {
	WCHAR package[PRI_MAX_PACKAGE_UNITS];
	UINT32 package_length;
	BOOL tried;
	struct pri_state state;
};
static struct pri_extra_map extra_maps[PRI_MAX_EXTRA_MAPS];
static struct pri_state *active_pri = &pri_state;

static UINT32 read_u16(const BYTE *data, UINT32 offset)
{
	return (UINT32)data[offset] | ((UINT32)data[offset + 1] << 8);
}

static UINT32 read_u32(const BYTE *data, UINT32 offset)
{
	return (UINT32)data[offset] | ((UINT32)data[offset + 1] << 8) | ((UINT32)data[offset + 2] << 16) |
		((UINT32)data[offset + 3] << 24);
}

/* A cursor walks a section body with a bounds check per read; a read past the end fails the parse. */
struct pri_cursor {
	const BYTE *data;
	UINT32 length;
	UINT32 at;
};

static BOOL take_u8(struct pri_cursor *cursor, UINT32 *value)
{
	if (cursor->length < 1 || cursor->at >= cursor->length)
		return FALSE;
	*value = cursor->data[cursor->at];
	cursor->at += 1;
	return TRUE;
}

static BOOL take_u16(struct pri_cursor *cursor, UINT32 *value)
{
	if (cursor->length < 2 || cursor->at > cursor->length - 2)
		return FALSE;
	*value = read_u16(cursor->data, cursor->at);
	cursor->at += 2;
	return TRUE;
}

static BOOL take_u32(struct pri_cursor *cursor, UINT32 *value)
{
	if (cursor->length < 4 || cursor->at > cursor->length - 4)
		return FALSE;
	*value = read_u32(cursor->data, cursor->at);
	cursor->at += 4;
	return TRUE;
}

static BOOL skip_bytes(struct pri_cursor *cursor, UINT32 count)
{
	if (count > cursor->length || cursor->at > cursor->length - count)
		return FALSE;
	cursor->at += count;
	return TRUE;
}

/* One section's body: between its 32-byte header and its footer magic. The directory entry is the 16-byte identifier,
 * two 16-bit flags, a 32-bit qualifier, then the 32-bit offset from the first section and the 32-bit length. */
struct pri_section_body {
	UINT32 start;
	UINT32 length;
};

static BOOL section_body(UINT32 index, struct pri_section_body *body)
{
	struct pri_state *state = active_pri;
	UINT32 toc, first, count, offset, length;
	if (!state->file || index >= PRI_MAX_SECTIONS)
		return FALSE;
	toc = read_u32(state->file, 16);
	first = read_u32(state->file, 20);
	count = read_u16(state->file, 24);
	if (index >= count)
		return FALSE;
	offset = read_u32(state->file, toc + index * 32 + 24);
	length = read_u32(state->file, toc + index * 32 + 28);
	if (length < 40 || offset > state->file_length || length > state->file_length - offset)
		return FALSE;
	body->start = first + offset;
	body->length = length;
	return body->start <= state->file_length && body->length <= state->file_length - body->start &&
		read_u32(state->file, body->start + body->length - 8) == PRI_FOOTER_MAGIC;
}

static BOOL body_matches(struct pri_section_body *body, const char *identifier)
{
	UINT32 i;
	for (i = 0; i < 16 && identifier[i]; ++i)
		if (active_pri->file[body->start + i] != (BYTE)identifier[i])
			return FALSE;
	return !identifier[i];
}

static BOOL parse_data_section(UINT32 index, struct pri_section_body *body)
{
	struct pri_state *state = active_pri;
	struct pri_cursor cursor = {state->file + body->start + 32, body->length - 40, 0};
	UINT32 string_count, blob_count, stored;
	struct pri_data_section *section;
	if (state->data_count >= PRI_MAX_DATA_SECTIONS)
		return TRUE; /* the extra sections stay unread; the ones recorded still resolve */
	if (!take_u32(&cursor, &stored) || !take_u16(&cursor, &string_count) || !take_u16(&cursor, &blob_count) ||
		!take_u32(&cursor, &stored))
		return FALSE;
	if (string_count > (cursor.length - cursor.at) / 4)
		return FALSE;
	stored = 12 + string_count * 4 + blob_count * 8;
	if (stored > cursor.length)
		return FALSE;
	section = &state->data[state->data_count];
	section->string_count = string_count;
	section->blob_count = blob_count;
	section->table = cursor.data + 12;
	section->blob_table = section->table + string_count * 4;
	section->data = cursor.data + stored;
	section->data_bytes = cursor.length - stored;
	state->data_section_index[state->data_count] = (UINT16)index;
	++state->data_count;
	return TRUE;
}

static BOOL parse_index_into(struct pri_state *state, BYTE *data, UINT32 length)
{
	struct pri_cursor cursor;
	struct pri_section_body body;
	UINT32 toc, count, i;
	UINT32 resmap_index, schema_index = 0xFFFF, decision_index = 0xFFFF;
	UINT32 data_indices[PRI_MAX_DATA_SECTIONS];
	UINT32 data_indices_count = 0;
	UINT32 env_length, env_count, hschema_length, value_types, item_groups, groups;
	UINT32 iteminfo_count, candidate_count, extension_length;
	UINT32 iteminfo_at, candidates_at;
	UINT32 entry_count, scope_count, item_count, unicode_units;
	UINT32 unique_length, name_length, unknown, ascii_units = 0;
	BOOL extended = FALSE, ascii_names = FALSE;

	if (length < 32 || memcmp(data, "mrm_pri2", 8) != 0)
		return FALSE;
	toc = read_u32(data, 16);
	count = read_u16(data, 24);
	if (count > PRI_MAX_SECTIONS || toc > length || count > (length - toc) / 32)
		return FALSE;
	state->file = data;
	state->file_length = length;

	/* The descriptor can appear anywhere in the section directory. */
	for (i = 0; i < count; ++i)
		if (section_body(i, &body) && body_matches(&body, "[mrm_pridescex]")) break;
	if (i == count) return FALSE;
	cursor.data = data + body.start + 32;
	cursor.length = body.length - 40;
	cursor.at = 0;
	{
		UINT32 header[10];
		for (i = 0; i < 10; ++i)
			if (!take_u16(&cursor, &header[i]))
				return FALSE;
		if (header[3] > PRI_MAX_LIST || header[4] > PRI_MAX_LIST || header[5] > PRI_MAX_LIST ||
			header[7] > PRI_MAX_LIST || header[8] > PRI_MAX_LIST)
			return FALSE;
		resmap_index = header[6] != 0xFFFF ? header[6] : 0xFFFF;
		/* the index lists follow the header: schemas, decisions, resource maps, referenced files, data items */
		for (i = 0; i < 5; ++i) {
			UINT32 list_count = i == 0 ? header[3] : i == 1 ? header[4] : i == 2 ? header[5] :
				i == 3 ? header[7] : header[8];
			UINT32 j;
			if (list_count > (cursor.length - cursor.at) / 2)
				return FALSE;
			for (j = 0; j < list_count; ++j) {
				UINT32 value;
				if (!take_u16(&cursor, &value))
					return FALSE;
				if (i == 0 && !j)
					schema_index = value;
				else if (i == 1 && !j)
					decision_index = value;
				else if (i == 2 && resmap_index == 0xFFFF && !j)
					resmap_index = value;
				else if (i == 4 && j < PRI_MAX_DATA_SECTIONS)
					data_indices[j] = value;
			}
			if (i == 4)
				data_indices_count = list_count < PRI_MAX_DATA_SECTIONS ? list_count : PRI_MAX_DATA_SECTIONS;
		}
	}
	if (resmap_index == 0xFFFF || schema_index == 0xFFFF || decision_index == 0xFFFF)
		return FALSE;
	for (i = 0; i < data_indices_count; ++i) {
		struct pri_section_body data_body;
		UINT32 section_index = data_indices[i];
		if (section_index == 0xFFFF || !section_body(section_index, &data_body))
			return FALSE;
		if (!parse_data_section(section_index, &data_body))
			return FALSE;
	}

	/* the resource map: its header locates the tables, the item info and candidate lists carry the lookups */
	if (!section_body(resmap_index, &body) || body.length < 72)
		return FALSE;
	cursor.data = data + body.start + 32;
	cursor.length = body.length - 40;
	cursor.at = 0;
	if (!take_u16(&cursor, &env_length) || !take_u16(&cursor, &env_count) || !take_u16(&cursor, &schema_index) ||
		!take_u16(&cursor, &hschema_length) || !take_u16(&cursor, &decision_index) ||
		!take_u16(&cursor, &value_types) || !take_u16(&cursor, &item_groups) || !take_u16(&cursor, &groups) ||
		!take_u32(&cursor, &iteminfo_count) || !take_u32(&cursor, &candidate_count) ||
		!take_u32(&cursor, &extension_length) || !take_u32(&cursor, &unknown))
		return FALSE;
	if (env_count > PRI_MAX_LIST || value_types > PRI_MAX_LIST || item_groups > PRI_MAX_LIST ||
		groups > PRI_MAX_LIST || iteminfo_count > PRI_MAX_TABLE || candidate_count > PRI_MAX_TABLE)
		return FALSE;
	if (!skip_bytes(&cursor, env_count * 0x22C) || !skip_bytes(&cursor, hschema_length) ||
		!skip_bytes(&cursor, value_types * 8) || !skip_bytes(&cursor, item_groups * 4) ||
		!skip_bytes(&cursor, groups * 4))
		return FALSE;
	if (iteminfo_count > (cursor.length - cursor.at) / 4)
		return FALSE;
	iteminfo_at = cursor.at;
	if (!skip_bytes(&cursor, iteminfo_count * 4) || !skip_bytes(&cursor, extension_length))
		return FALSE;
	if (candidate_count > (cursor.length - cursor.at) / 8)
		return FALSE;
	candidates_at = cursor.at;
	state->iteminfo = data + body.start + 32 + iteminfo_at;
	state->iteminfo_count = iteminfo_count;
	state->candidates = data + body.start + 32 + candidates_at;
	state->candidate_count = candidate_count;

	/* the decision info: one entry per decision, each naming its first candidate set and the set's candidate count */
	if (!section_body(decision_index, &body))
		return FALSE;
	cursor.data = data + body.start + 32;
	cursor.length = body.length - 40;
	cursor.at = 0;
	{
		UINT32 distinct, qualifiers, sets, decision_count, index_count, value_length;
		if (!take_u16(&cursor, &distinct) || !take_u16(&cursor, &qualifiers) || !take_u16(&cursor, &sets) ||
			!take_u16(&cursor, &decision_count) || !take_u16(&cursor, &index_count) ||
			!take_u16(&cursor, &value_length))
			return FALSE;
		if (decision_count > PRI_MAX_TABLE || decision_count > (cursor.length - cursor.at) / 4)
			return FALSE;
		state->decisions = cursor.data + cursor.at;
		state->decision_count = decision_count;
	}

	/* The extended schema can carry both UTF-16 and compact ASCII names. */
	if (!section_body(schema_index, &body))
		return FALSE;
	cursor.data = data + body.start + 32;
	cursor.length = body.length - 40;
	cursor.at = 0;
	if (!take_u16(&cursor, &unknown) || !take_u16(&cursor, &unique_length) ||
		!take_u16(&cursor, &name_length) || !take_u16(&cursor, &unknown))
		return FALSE;
	if (cursor.length >= 24 && !memcmp(cursor.data + 8, "[def_hnames", 11)) {
		extended = TRUE;
		ascii_names = !memcmp(cursor.data + 8, "[def_hnamesx]", 13);
		if (!skip_bytes(&cursor, 16)) return FALSE;
	}
	if (!skip_bytes(&cursor, 20)) /* the schema version info, unread */
		return FALSE;
	if (unique_length > (cursor.length - cursor.at) / 2 || name_length > (cursor.length - cursor.at) / 2)
		return FALSE;
	if (!skip_bytes(&cursor, unique_length * 2 + name_length * 2 + 6))
		return FALSE;
	if (!take_u32(&cursor, &entry_count) || !take_u32(&cursor, &scope_count) || !take_u32(&cursor, &item_count) ||
		!take_u32(&cursor, &unicode_units) || !take_u32(&cursor, &unknown))
		return FALSE;
	if (extended && ascii_names && !take_u32(&cursor, &ascii_units)) return FALSE;
	if (ascii_units > (1u << 24)) return FALSE;
	if (entry_count > PRI_MAX_TABLE ||
		entry_count > (cursor.length - cursor.at) / 12 ||
		scope_count > (cursor.length - cursor.at - entry_count * 12) / 8 ||
		item_count * 2 > cursor.length - cursor.at - entry_count * 12 - scope_count * 8 ||
		unicode_units > (cursor.length - cursor.at - entry_count * 12 - scope_count * 8 - item_count * 2) / 2 ||
		unicode_units > (1u << 24))
		return FALSE;
	state->entries = ALLOC(sizeof(struct pri_schema_entry) * entry_count);
	if (!state->entries)
		return FALSE;
	for (i = 0; i < entry_count; ++i) {
		UINT32 parent, path_length, first_child, name_len, flags, offset, index;
		if (!take_u16(&cursor, &parent) || !take_u16(&cursor, &path_length) || !take_u16(&cursor, &first_child) ||
			!take_u8(&cursor, &name_len) || !take_u8(&cursor, &flags) || !take_u16(&cursor, &offset) ||
			!take_u16(&cursor, &index))
			return FALSE;
		state->entries[i].parent = (UINT16)parent;
		state->entries[i].flags = (UINT16)flags;
		/* the on-disk 16-bit offset wraps for name blocks larger than 64 Ki
		 * units; the entry flags' low two bits carry the offset's bits 16-17 */
		state->entries[i].offset = offset | ((flags & 3u) << 16);
		state->entries[i].name_len = (UINT16)name_len;
		state->entries[i].index = (UINT16)index;
	}
	if (!skip_bytes(&cursor, scope_count * 8 + item_count * 2))
		return FALSE;
	if (unicode_units > (cursor.length - cursor.at) / 2)
		return FALSE;
	state->block = (const WCHAR *)(cursor.data + cursor.at);
	state->block_units = unicode_units;
	if (!skip_bytes(&cursor, unicode_units * 2) || ascii_units > cursor.length - cursor.at) return FALSE;
	if (ascii_units) {
		state->ascii_block = ALLOC(sizeof(WCHAR) * (SIZE_T)ascii_units);
		if (!state->ascii_block) return FALSE;
		for (i = 0; i < ascii_units; ++i) {
			if (cursor.data[cursor.at + i] > 0x7f) return FALSE;
			state->ascii_block[i] = cursor.data[cursor.at + i];
		}
		state->ascii_units = ascii_units;
	}
	state->entry_count = entry_count;
	return TRUE;
}

/* The index beside the running executable: <exe-dir>\resources.pri, else the directory named after the executable, the
 * layout the build's fixtures and package.c's manifest resolution follow. */
static BOOL find_pri(WCHAR *path /* MAX_PATH */)
{
	WCHAR directory[MAX_PATH];
	WCHAR candidate[MAX_PATH];
	const WCHAR *base;
	const UINT32 suffix = (UINT32)lstrlenW(L"\\resources.pri");
	DWORD length = GetModuleFileNameW(NULL, directory, MAX_PATH);
	UINT32 cut = 0;
	UINT32 i;
	HANDLE file;
	if (!length || length >= MAX_PATH)
		return FALSE;
	for (i = 0; i < length; ++i)
		if (directory[i] == L'\\' || directory[i] == L'/')
			cut = i;
	directory[cut] = 0;
	/* beside the executable */
	if ((UINT32)lstrlenW(directory) + suffix + 1 < MAX_PATH) {
		lstrcpyW(candidate, directory);
		lstrcatW(candidate, L"\\resources.pri");
		file = CreateFileW(candidate, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (file != INVALID_HANDLE_VALUE) {
			CloseHandle(file);
			lstrcpyW(path, candidate);
			return TRUE;
		}
	}
	/* the directory named after the executable without its extension, as the fixtures lay it out */
	base = directory + cut + 1;
	{
		UINT32 base_length = (UINT32)lstrlenW(base);
		if (base_length > 4 && base[base_length - 4] == L'.' && base[base_length - 3] == L'e' &&
			base[base_length - 2] == L'x' && base[base_length - 1] == L'e')
			base_length -= 4;
		if ((UINT32)lstrlenW(directory) + 1 + base_length + suffix + 1 < MAX_PATH) {
			UINT32 stem_length = (UINT32)lstrlenW(directory) + 1;
			lstrcpyW(candidate, directory);
			lstrcatW(candidate, L"\\");
			memcpy(candidate + stem_length, base, base_length * sizeof(WCHAR));
			candidate[stem_length + base_length] = 0;
			lstrcatW(candidate, L"\\resources.pri");
			file = CreateFileW(candidate, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			if (file != INVALID_HANDLE_VALUE) {
				CloseHandle(file);
				lstrcpyW(path, candidate);
				return TRUE;
			}
		}
	}
	return FALSE;
}

static BOOL read_file(const WCHAR *path, BYTE **data, UINT32 *length)
{
	LARGE_INTEGER size;
	BYTE *buffer;
	DWORD read = 0;
	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	*data = NULL;
	*length = 0;
	if (file == INVALID_HANDLE_VALUE)
		return FALSE;
	if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 64 * 1024 * 1024) {
		CloseHandle(file);
		return FALSE;
	}
	if (!(buffer = ALLOC(sizeof(BYTE) * (SIZE_T)size.QuadPart))) {
		CloseHandle(file);
		return FALSE;
	}
	if (!ReadFile(file, buffer, (DWORD)size.QuadPart, &read, NULL) || read != (DWORD)size.QuadPart) {
		FREE(buffer);
		CloseHandle(file);
		return FALSE;
	}
	CloseHandle(file);
	*data = buffer;
	*length = (UINT32)read;
	return TRUE;
}

/* The index is read once per process: the first asker parses it, later askers wait for the result. A file that does not
 * parse leaves the state empty, and every lookup answers the miss. */
static void ensure_pri(void)
{
	static LONG state;
	if (InterlockedCompareExchange(&state, 1, 0) == 0) {
		WCHAR path[MAX_PATH];
		BYTE *data = NULL;
		UINT32 length = 0;
		if (find_pri(path) && read_file(path, &data, &length)) {
			if (!parse_index_into(&pri_state, data, length)) {
				FREE(pri_state.entries);
				FREE(pri_state.ascii_block);
				memset(&pri_state, 0, sizeof pri_state);
				FREE(data);
			}
		}
		InterlockedExchange(&state, 2);
	} else {
		while (InterlockedCompareExchange(&state, 2, 2) != 2)
			Sleep(0);
	}
}

/* A schema entry's name: the unicode block holds every name nul-terminated, the entry's offset counting 4-byte units
 * which double into a character index, the way the reference parser reads them. */
static const WCHAR *entry_name(UINT32 index, UINT32 *length)
{
	struct pri_state *state = active_pri;
	const WCHAR *name;
	UINT32 start, limit, units;
	if (index >= state->entry_count)
		return NULL;
	if (state->entries[index].flags & 0x20) {
		start = state->entries[index].offset;
		if (!state->ascii_block || start >= state->ascii_units) return NULL;
		name = state->ascii_block + start;
		limit = state->ascii_units - start;
	} else {
		start = (UINT32)state->entries[index].offset * 2;
		if (!state->block_units && index == 0 && !state->entries[index].name_len &&
			state->entries[index].parent == 0) { *length = 0; return L""; }
		if (!state->block || start >= state->block_units) return NULL;
		name = state->block + start;
		limit = state->block_units - start;
	}
	units = state->entries[index].name_len ? state->entries[index].name_len : limit;
	if (units > limit)
		units = limit;
	if (units > PRI_MAX_NAME_UNITS)
		units = PRI_MAX_NAME_UNITS;
	for (start = 0; start < units && name[start] != 0; ++start); /* the name ends at its first terminator */
	*length = start;
	return name;
}

/* The entry's full path: the names from the root down, joined by "/" — the root's empty name leaves the leading slash. */
static BOOL entry_path(UINT32 index, WCHAR *out, UINT32 capacity, UINT32 *length)
{
	const WCHAR *spans[PRI_CHAIN_MAX];
	UINT32 lengths[PRI_CHAIN_MAX];
	UINT32 depth = 0, position = 0, level, i = index;
	struct pri_state *state = active_pri;
	if (index >= state->entry_count)
		return FALSE;
	while (depth < PRI_CHAIN_MAX) {
		const WCHAR *name = entry_name(i, &lengths[depth]);
		if (!name)
			return FALSE;
		spans[depth++] = name;
		if (state->entries[i].parent == i)
			break; /* the root parents itself */
		i = state->entries[i].parent;
		if (i >= state->entry_count)
			return FALSE;
	}
	if (depth >= PRI_CHAIN_MAX)
		return FALSE;
	for (level = 0; level < depth; ++level) {
		UINT32 from = depth - 1 - level;
		if (level) {
			if (position + 1 >= capacity)
				return FALSE;
			out[position++] = L'/';
		}
		if (position + lengths[from] >= capacity)
			return FALSE;
		memcpy(out + position, spans[from], lengths[from] * sizeof(WCHAR));
		position += lengths[from];
	}
	out[position] = 0;
	*length = position;
	return TRUE;
}

/* The string a candidate names: kind 1 candidates point at a data item section's string table, the offset and the
 * length both counting bytes — the way the Shapr3D package's index stores them. */
static BOOL candidate_string(UINT32 index, const WCHAR **text, UINT32 *units)
{
	struct pri_state *state = active_pri;
	const BYTE *candidate;
	UINT32 kind, item, section, i, offset, length_bytes;
	if (index >= state->candidate_count)
		return FALSE;
	candidate = state->candidates + index * 8;
	kind = candidate[0];
	item = read_u16(candidate, 4);
	section = read_u16(candidate, 6);
	if (kind != 1)
		return FALSE;
	for (i = 0; i < state->data_count; ++i)
		if (state->data_section_index[i] == section)
			break;
	if (i >= state->data_count)
		return FALSE;
	if (item >= state->data[i].string_count)
		return FALSE;
	offset = read_u16(state->data[i].table, item * 4);
	length_bytes = read_u16(state->data[i].table, item * 4 + 2);
	if (offset > state->data[i].data_bytes || length_bytes > state->data[i].data_bytes - offset)
		return FALSE;
	*text = (const WCHAR *)(state->data[i].data + offset);
	*units = length_bytes / 2;
	if (!*units || *units > PRI_MAX_VALUE_UNITS)
		return FALSE;
	return TRUE;
}

/* The candidates of an item: the item info names the first candidate and the decision's set count says how many. */
static BOOL candidate_range(UINT32 index, UINT32 *first, UINT32 *count)
{
	struct pri_state *state = active_pri;
	UINT32 iteminfo, decision_index, first_candidate, first_set, set_count;
	if (index >= state->entry_count || !state->iteminfo)
		return FALSE;
	iteminfo = state->entries[index].index;
	if (iteminfo >= state->iteminfo_count)
		return FALSE;
	decision_index = read_u16(state->iteminfo, iteminfo * 4);
	first_candidate = read_u16(state->iteminfo, iteminfo * 4 + 2);
	if (!state->decisions || decision_index >= state->decision_count)
		return FALSE;
	first_set = read_u16(state->decisions, decision_index * 4);
	set_count = read_u16(state->decisions, decision_index * 4 + 2);
	(void)first_set;
	if (first_candidate >= state->candidate_count || set_count > state->candidate_count - first_candidate)
		return FALSE;
	*first = first_candidate;
	*count = set_count;
	return TRUE;
}

/* Resolve a package resource to a bounded blob in the process-lifetime PRI buffer. */
/* The blob lookup itself, against the active map. */
static HRESULT pri_lookup_blob(const WCHAR *name, UINT32 name_length, const BYTE **data, UINT32 *length)
{
	WCHAR path[PRI_MAX_PATH_UNITS];
	UINT32 path_length, first, count;
	for (UINT32 i = 0; i < active_pri->entry_count; ++i) {
		if ((active_pri->entries[i].flags & 0x10) || !entry_path(i, path, ARRAYSIZE(path), &path_length)) continue;
		if (CompareStringOrdinal(name, (int)name_length, path, (int)path_length, TRUE) != CSTR_EQUAL) continue;
		if (!candidate_range(i, &first, &count) || !count) return E_BOUNDS;
		const BYTE *candidate = active_pri->candidates + first * 8;
		if (candidate[0] != 1) { raven_trace_not_implemented(__func__); return E_NOTIMPL; }
		UINT32 item = read_u16(candidate, 4), section = read_u16(candidate, 6), index;
		for (index = 0; index < active_pri->data_count; ++index)
			if (active_pri->data_section_index[index] == section) break;
		if (index == active_pri->data_count) return E_BOUNDS;
		struct pri_data_section *items = &active_pri->data[index];
		UINT32 offset, bytes;
		/* Both item tables store byte ranges; small binary resources can use the compact table too. */
		if (item < items->string_count) {
			offset = read_u16(items->table, item * 4);
			bytes = read_u16(items->table, item * 4 + 2);
		} else {
			item -= items->string_count;
			if (item >= items->blob_count) return E_INVALIDARG;
			offset = read_u32(items->blob_table, item * 8);
			bytes = read_u32(items->blob_table, item * 8 + 4);
		}
		if (offset > items->data_bytes || bytes > items->data_bytes - offset) return E_INVALIDARG;
		*data = items->data + offset; *length = bytes;
		return S_OK;
	}
	return E_BOUNDS;
}

HRESULT raven_pri_get_blob(const WCHAR *name, UINT32 name_length, const BYTE **data, UINT32 *length)
{
	HRESULT result;
	if (!name || !data || !length) return E_POINTER;
	*data = NULL; *length = 0;
	ensure_pri();
	result = pri_lookup_blob(name, name_length, data, length);
	return result;
}

/* A dependency package's index: parsed once per package name into an extra map, so an ms-appx URI whose host names the
 * package resolves its resources there. A file that is absent or does not parse marks the map tried with an empty
 * state, whose lookups miss the way the main map's do. */
HRESULT raven_pri_open_map(const WCHAR *package, UINT32 package_length, const WCHAR *path)
{
	BYTE *data = NULL;
	UINT32 length = 0;
	struct pri_extra_map *map = NULL;
	if (!package || !package_length || !path) return E_POINTER;
	if (package_length >= PRI_MAX_PACKAGE_UNITS) return E_INVALIDARG;
	ensure_pri();
	for (UINT32 i = 0; i < PRI_MAX_EXTRA_MAPS; ++i) {
		struct pri_extra_map *candidate = &extra_maps[i];
		if (candidate->package_length == package_length &&
			CompareStringOrdinal(candidate->package, (int)candidate->package_length, package, (int)package_length, TRUE) == CSTR_EQUAL)
			return S_OK; /* already registered, whatever its read found; the caller does not re-read */
		if (!map && !candidate->tried)
			map = candidate;
	}
	if (!map)
		return E_OUTOFMEMORY; /* more framework packages than this engine registers; the URIs stay unresolved */
	memcpy(map->package, package, (SIZE_T)package_length * sizeof(WCHAR));
	map->package_length = package_length;
	map->tried = TRUE;
	if (!read_file(path, &data, &length))
		return S_FALSE;
	/* parse_index_into reads the section directory through the active map, so the parse runs against the new state */
	active_pri = &map->state;
	const BOOL parsed = parse_index_into(&map->state, data, length);
	active_pri = &pri_state;
	if (!parsed) {
		FREE(map->state.entries);
		FREE(map->state.ascii_block);
		memset(&map->state, 0, sizeof map->state);
		FREE(data);
		return S_FALSE;
	}
	return S_OK;
}

/* The map a package-qualified URI reads: the package names one of the extra maps, NULL the main one. */
static struct pri_extra_map *pri_find_map(const WCHAR *package, UINT32 package_length)
{
	for (UINT32 i = 0; i < PRI_MAX_EXTRA_MAPS; ++i) {
		struct pri_extra_map *candidate = &extra_maps[i];
		if (candidate->package_length == package_length &&
			CompareStringOrdinal(candidate->package, (int)candidate->package_length, package, (int)package_length, TRUE) == CSTR_EQUAL)
			return candidate;
	}
	return NULL;
}

HRESULT raven_pri_get_blob_from(const WCHAR *package, UINT32 package_length, const WCHAR *name, UINT32 name_length,
	const BYTE **data, UINT32 *length)
{
	struct pri_extra_map *map;
	HRESULT result;
	if (!data || !length) return E_POINTER;
	*data = NULL; *length = 0;
	if (!name) return E_POINTER;
	if (!package || !package_length)
		return raven_pri_get_blob(name, name_length, data, length);
	ensure_pri();
	if (!(map = pri_find_map(package, package_length)))
		return E_BOUNDS;
	active_pri = &map->state;
	result = pri_lookup_blob(name, name_length, data, length);
	active_pri = &pri_state;
	return result;
}

/* The ResourceCandidate: the string value is copied out of the index so the object owns its answer; the fixture
 * candidates are unqualified defaults, so the qualifier view is the shared empty one. */
struct candidate2_part {
	const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2Vtbl *lpVtbl;
	struct candidate_object *owner;
};

struct candidate3_part {
	const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3Vtbl *lpVtbl;
	struct candidate_object *owner;
};

struct candidate_object {
	const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidateVtbl *lpVtbl;
	LONG refs;
	struct candidate2_part second;
	struct candidate3_part third;
	WCHAR *value;
};

static HRESULT WINAPI candidate_query(candidate_i *iface, REFIID iid, void **out);
static ULONG WINAPI candidate_addref(candidate_i *iface);
static ULONG WINAPI candidate_release(candidate_i *iface);
static HRESULT WINAPI candidate_iids(candidate_i *iface, ULONG *count, IID **iids);
static HRESULT WINAPI candidate_name(candidate_i *iface, HSTRING *name);
static HRESULT WINAPI candidate_trust(candidate_i *iface, TrustLevel *level);
static HRESULT WINAPI candidate_qualifiers(candidate_i *iface, uint64_t out);
static HRESULT WINAPI candidate_is_match(candidate_i *iface, uint64_t out);
static HRESULT WINAPI candidate_is_match_as_default(candidate_i *iface, uint64_t out);
static HRESULT WINAPI candidate_is_default(candidate_i *iface, uint64_t out);
static HRESULT WINAPI candidate_value_as_string(candidate_i *iface, uint64_t out);
static HRESULT WINAPI candidate_value_as_file_async(candidate_i *iface, uint64_t out);
static HRESULT WINAPI candidate_qualifier_value(candidate_i *iface, uint64_t name, uint64_t out);
static HRESULT WINAPI candidate2_get_value_as_stream_async(candidate2_i *iface, uint64_t out);
static HRESULT WINAPI candidate3_get_kind(candidate3_i *iface, uint64_t out);

/* The parts forward their lifetime to the object, whose base their owner pointer reaches. */
#define CANDIDATE_PART_LIFETIME(part, iface_type) \
	static HRESULT WINAPI part##_query(iface_type *iface, REFIID iid, void **out) \
	{ return candidate_query((candidate_i *)((struct part##_part *)iface)->owner, iid, out); } \
	static ULONG WINAPI part##_addref(iface_type *iface) \
	{ return candidate_addref((candidate_i *)((struct part##_part *)iface)->owner); } \
	static ULONG WINAPI part##_release(iface_type *iface) \
	{ return candidate_release((candidate_i *)((struct part##_part *)iface)->owner); } \
	static HRESULT WINAPI part##_iids(iface_type *iface, ULONG *count, IID **iids) \
	{ (void)iface; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI part##_name(iface_type *iface, HSTRING *name) \
	{ (void)iface; return create_class_string(candidate_class_name, name); } \
	static HRESULT WINAPI part##_trust(iface_type *iface, TrustLevel *level) \
	{ (void)iface; return inspectable_trust(level); }

CANDIDATE_PART_LIFETIME(candidate2, candidate2_i)
CANDIDATE_PART_LIFETIME(candidate3, candidate3_i)

static const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidateVtbl candidate_table = {
	.QueryInterface = candidate_query,
	.AddRef = candidate_addref,
	.Release = candidate_release,
	.GetIids = candidate_iids,
	.GetRuntimeClassName = candidate_name,
	.GetTrustLevel = candidate_trust,
	.get_Qualifiers = candidate_qualifiers,
	.get_IsMatch = candidate_is_match,
	.get_IsMatchAsDefault = candidate_is_match_as_default,
	.get_IsDefault = candidate_is_default,
	.get_ValueAsString = candidate_value_as_string,
	.GetValueAsFileAsync = candidate_value_as_file_async,
	.GetQualifierValue = candidate_qualifier_value,
};

static const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2Vtbl candidate2_table = {
	.QueryInterface = candidate2_query,
	.AddRef = candidate2_addref,
	.Release = candidate2_release,
	.GetIids = candidate2_iids,
	.GetRuntimeClassName = candidate2_name,
	.GetTrustLevel = candidate2_trust,
	.GetValueAsStreamAsync = candidate2_get_value_as_stream_async,
};

static const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3Vtbl candidate3_table = {
	.QueryInterface = candidate3_query,
	.AddRef = candidate3_addref,
	.Release = candidate3_release,
	.GetIids = candidate3_iids,
	.GetRuntimeClassName = candidate3_name,
	.GetTrustLevel = candidate3_trust,
	.get_Kind = candidate3_get_kind,
};

/* The qualifier view: the candidates this index reads carry no qualifiers (they are unqualified defaults), so one
 * shared empty view answers every ask; the count is zero and GetAt answers the bounds miss. The instance sits after
 * its table below. */
static HRESULT WINAPI qualifier_view_query(qualifier_view_i *iface, REFIID iid, void **out);
static ULONG WINAPI qualifier_view_addref(qualifier_view_i *iface);
static ULONG WINAPI qualifier_view_release(qualifier_view_i *iface);
static HRESULT WINAPI qualifier_view_iids(qualifier_view_i *iface, ULONG *count, IID **iids);
static HRESULT WINAPI qualifier_view_name(qualifier_view_i *iface, HSTRING *name);
static HRESULT WINAPI qualifier_view_trust(qualifier_view_i *iface, TrustLevel *level);
static HRESULT WINAPI qualifier_view_get_at(qualifier_view_i *iface, UINT32 index, void **value);
static HRESULT WINAPI qualifier_view_size(qualifier_view_i *iface, UINT32 *value);
static HRESULT WINAPI qualifier_view_index_of(qualifier_view_i *iface, void *element, UINT32 *index, boolean *found);
static HRESULT WINAPI qualifier_view_get_many(qualifier_view_i *iface, UINT32 start_index, UINT32 items_size,
	void **items, UINT32 *value);

static const qualifier_view_vtbl qualifier_view_table = {
	.QueryInterface = qualifier_view_query,
	.AddRef = qualifier_view_addref,
	.Release = qualifier_view_release,
	.GetIids = qualifier_view_iids,
	.GetRuntimeClassName = qualifier_view_name,
	.GetTrustLevel = qualifier_view_trust,
	.GetAt = qualifier_view_get_at,
	.get_Size = qualifier_view_size,
	.IndexOf = qualifier_view_index_of,
	.GetMany = qualifier_view_get_many,
};

static struct {
	const qualifier_view_vtbl *lpVtbl;
	LONG refs;
} qualifier_view_instance = {&qualifier_view_table, 1};

static HRESULT WINAPI qualifier_view_query(qualifier_view_i *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &qualifier_view_iid))
		return E_NOINTERFACE;
	qualifier_view_addref(iface);
	*out = iface;
	return S_OK;
}

/* The empty view lives as long as the process: the count follows the guests' references and the object is never freed. */
static ULONG WINAPI qualifier_view_addref(qualifier_view_i *iface)
{
	return (ULONG)InterlockedIncrement(&qualifier_view_instance.refs);
}

static ULONG WINAPI qualifier_view_release(qualifier_view_i *iface)
{
	return (ULONG)InterlockedDecrement(&qualifier_view_instance.refs);
}

static HRESULT WINAPI qualifier_view_iids(qualifier_view_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI qualifier_view_name(qualifier_view_i *iface, HSTRING *name)
{
	(void)iface;
	return create_class_string(qualifier_view_class_name, name);
}

static HRESULT WINAPI qualifier_view_trust(qualifier_view_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI qualifier_view_get_at(qualifier_view_i *iface, UINT32 index, void **value)
{
	(void)iface;
	(void)index;
	if (!value)
		return E_POINTER;
	*value = NULL;
	return E_BOUNDS;
}

static HRESULT WINAPI qualifier_view_size(qualifier_view_i *iface, UINT32 *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = 0;
	return S_OK;
}

static HRESULT WINAPI qualifier_view_index_of(qualifier_view_i *iface, void *element, UINT32 *index, boolean *found)
{
	(void)iface;
	(void)element;
	if (!index || !found)
		return E_POINTER;
	*index = 0;
	*found = 0;
	return S_OK;
}

static HRESULT WINAPI qualifier_view_get_many(qualifier_view_i *iface, UINT32 start_index, UINT32 items_size,
	void **items, UINT32 *value)
{
	(void)iface;
	(void)start_index;
	(void)items_size;
	(void)items;
	if (!value)
		return E_POINTER;
	*value = 0;
	return S_OK;
}

static HRESULT WINAPI candidate_query(candidate_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate))
		*out = iface;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate2))
		*out = &((struct candidate_object *)iface)->second;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceCandidate3))
		*out = &((struct candidate_object *)iface)->third;
	else
		return E_NOINTERFACE;
	candidate_addref(iface);
	return S_OK;
}

static ULONG WINAPI candidate_addref(candidate_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct candidate_object *)iface)->refs);
}

static ULONG WINAPI candidate_release(candidate_i *iface)
{
	struct candidate_object *self = (struct candidate_object *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		FREE(self->value);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI candidate_iids(candidate_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI candidate_name(candidate_i *iface, HSTRING *name)
{
	(void)iface;
	return create_class_string(candidate_class_name, name);
}

static HRESULT WINAPI candidate_trust(candidate_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI candidate_qualifiers(candidate_i *iface, uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	InterlockedIncrement(&qualifier_view_instance.refs);
	*(void **)(uintptr_t)out = (void *)&qualifier_view_instance;
	return S_OK;
}

/* A single default candidate matches and is the default; matching against a context's qualifiers is not implemented,
 * and the index this component reads carries no decision qualifiers to match against anyway. */
static HRESULT WINAPI candidate_is_match(candidate_i *iface, uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*(boolean *)(uintptr_t)out = 1;
	return S_OK;
}

static HRESULT WINAPI candidate_is_match_as_default(candidate_i *iface, uint64_t out)
{
	return candidate_is_match(iface, out);
}

static HRESULT WINAPI candidate_is_default(candidate_i *iface, uint64_t out)
{
	return candidate_is_match(iface, out);
}

static HRESULT WINAPI candidate_value_as_string(candidate_i *iface, uint64_t out)
{
	struct candidate_object *self = (struct candidate_object *)iface;
	if (!out)
		return E_POINTER;
	return create_string(self->value, self->value ? (UINT32)lstrlenW(self->value) : 0, (HSTRING *)(uintptr_t)out);
}

static HRESULT WINAPI candidate_value_as_file_async(candidate_i *iface, uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI candidate_qualifier_value(candidate_i *iface, uint64_t name, uint64_t out)
{
	(void)iface;
	(void)name;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	return E_BOUNDS; /* the candidates carry no qualifiers to look up */
}

static HRESULT WINAPI candidate2_get_value_as_stream_async(candidate2_i *iface, uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI candidate3_get_kind(candidate3_i *iface, uint64_t out)
{
	/* ResourceCandidateKind.String, the only kind this index carries */
	(void)iface;
	if (!out)
		return E_POINTER;
	*(int32_t *)(uintptr_t)out = 0;
	return S_OK;
}

static struct candidate_object *create_candidate(const WCHAR *text, UINT32 units)
{
	struct candidate_object *candidate = ALLOC(sizeof *candidate);
	if (!candidate)
		return NULL;
	candidate->value = ALLOC(sizeof(WCHAR) * (SIZE_T)(units + 1));
	if (!candidate->value) {
		FREE(candidate);
		return NULL;
	}
	memcpy(candidate->value, text, units * sizeof(WCHAR));
	candidate->value[units] = 0;
	candidate->lpVtbl = &candidate_table;
	candidate->second.lpVtbl = &candidate2_table;
	candidate->second.owner = candidate;
	candidate->third.lpVtbl = &candidate3_table;
	candidate->third.owner = candidate;
	candidate->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	return candidate;
}

/* The candidates vector view: a snapshot of the item's resolvable string candidates, each held by reference. */
struct candidates_view {
	const candidate_view_vtbl *lpVtbl;
	LONG refs;
	UINT32 count;
	struct candidate_object **items;
};

static HRESULT WINAPI view_query(candidate_view_i *iface, REFIID iid, void **out);
static ULONG WINAPI view_addref(candidate_view_i *iface);
static ULONG WINAPI view_release(candidate_view_i *iface);
static HRESULT WINAPI view_iids(candidate_view_i *iface, ULONG *count, IID **iids);
static HRESULT WINAPI view_name(candidate_view_i *iface, HSTRING *name);
static HRESULT WINAPI view_trust(candidate_view_i *iface, TrustLevel *level);
static HRESULT WINAPI view_get_at(candidate_view_i *iface, UINT32 index, candidate_i **value);
static HRESULT WINAPI view_size(candidate_view_i *iface, UINT32 *value);
static HRESULT WINAPI view_index_of(candidate_view_i *iface, candidate_i *element, UINT32 *index, boolean *found);
static HRESULT WINAPI view_get_many(candidate_view_i *iface, UINT32 start_index, UINT32 items_size,
	candidate_i **items, UINT32 *value);

static const candidate_view_vtbl view_table = {
	.QueryInterface = view_query,
	.AddRef = view_addref,
	.Release = view_release,
	.GetIids = view_iids,
	.GetRuntimeClassName = view_name,
	.GetTrustLevel = view_trust,
	.GetAt = view_get_at,
	.get_Size = view_size,
	.IndexOf = view_index_of,
	.GetMany = view_get_many,
};

static HRESULT WINAPI view_query(candidate_view_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) && !equal_iid(iid, &view_iid))
		return E_NOINTERFACE;
	view_addref(iface);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI view_addref(candidate_view_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct candidates_view *)iface)->refs);
}

static ULONG WINAPI view_release(candidate_view_i *iface)
{
	struct candidates_view *self = (struct candidates_view *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		UINT32 i;
		for (i = 0; i < self->count; ++i)
			candidate_release((candidate_i *)self->items[i]);
		FREE(self->items);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI view_iids(candidate_view_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI view_name(candidate_view_i *iface, HSTRING *name)
{
	(void)iface;
	return create_class_string(view_class_name, name);
}

static HRESULT WINAPI view_trust(candidate_view_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI view_get_at(candidate_view_i *iface, UINT32 index, candidate_i **value)
{
	struct candidates_view *self = (struct candidates_view *)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (index >= self->count)
		return E_BOUNDS;
	*value = (candidate_i *)self->items[index];
	candidate_addref((candidate_i *)self->items[index]);
	return S_OK;
}

static HRESULT WINAPI view_size(candidate_view_i *iface, UINT32 *value)
{
	struct candidates_view *self = (struct candidates_view *)iface;
	if (!value)
		return E_POINTER;
	*value = self->count;
	return S_OK;
}

/* The elements are compared by interface pointer, the identity a guest holds them by. */
static HRESULT WINAPI view_index_of(candidate_view_i *iface, candidate_i *element, UINT32 *index, boolean *found)
{
	struct candidates_view *self = (struct candidates_view *)iface;
	UINT32 i;
	if (!index || !found)
		return E_POINTER;
	*index = 0;
	*found = 0;
	for (i = 0; i < self->count; ++i) {
		if ((candidate_i *)self->items[i] == element) {
			*index = i;
			*found = 1;
			return S_OK;
		}
	}
	return S_OK;
}

static HRESULT WINAPI view_get_many(candidate_view_i *iface, UINT32 start_index, UINT32 items_size,
	candidate_i **items, UINT32 *value)
{
	struct candidates_view *self = (struct candidates_view *)iface;
	UINT32 copied = 0, i;
	if (!items || !value)
		return E_POINTER;
	*value = 0;
	if (start_index > self->count)
		return E_BOUNDS;
	for (i = start_index; i < self->count && copied < items_size; ++i, ++copied) {
		items[copied] = (candidate_i *)self->items[i];
		candidate_addref((candidate_i *)self->items[i]);
	}
	*value = copied;
	return S_OK;
}

static struct candidates_view *create_view(UINT32 first, UINT32 count)
{
	struct candidates_view *view;
	UINT32 i, kept = 0;
	if (count > PRI_MAX_TABLE)
		return NULL;
	view = ALLOC(sizeof *view);
	if (!view)
		return NULL;
	view->items = ALLOC(sizeof(struct candidate_object *) * (count ? count : 1));
	if (!view->items) {
		FREE(view);
		return NULL;
	}
	for (i = 0; i < count; ++i) {
		const WCHAR *text;
		UINT32 units;
		struct candidate_object *candidate;
		if (!candidate_string(first + i, &text, &units))
			continue; /* a non-string or unreadable candidate is not offered */
		candidate = create_candidate(text, units);
		if (!candidate)
			continue;
		view->items[kept++] = candidate;
	}
	if (!kept) {
		FREE(view->items);
		FREE(view);
		return NULL;
	}
	view->lpVtbl = &view_table;
	view->refs = 1;
	view->count = kept;
	InterlockedIncrement(&raven_xaml_objects);
	return view;
}

/* The NamedResource: one schema item, resolving to its candidates on demand. */
struct named_resource {
	const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResourceVtbl *lpVtbl;
	LONG refs;
	UINT32 entry;
};

static HRESULT WINAPI named_query(named_i *iface, REFIID iid, void **out);
static ULONG WINAPI named_addref(named_i *iface);
static ULONG WINAPI named_release(named_i *iface);
static HRESULT WINAPI named_iids(named_i *iface, ULONG *count, IID **iids);
static HRESULT WINAPI named_name(named_i *iface, HSTRING *name);
static HRESULT WINAPI named_trust(named_i *iface, TrustLevel *level);
static HRESULT WINAPI named_uri(named_i *iface, uint64_t out);
static HRESULT WINAPI named_candidates(named_i *iface, uint64_t out);
static HRESULT WINAPI named_resolve(named_i *iface, uint64_t out);
static HRESULT WINAPI named_resolve_context(named_i *iface, uint64_t context, uint64_t out);
static HRESULT WINAPI named_resolve_all(named_i *iface, uint64_t out);
static HRESULT WINAPI named_resolve_all_context(named_i *iface, uint64_t context, uint64_t out);

static const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResourceVtbl named_table = {
	.QueryInterface = named_query,
	.AddRef = named_addref,
	.Release = named_release,
	.GetIids = named_iids,
	.GetRuntimeClassName = named_name,
	.GetTrustLevel = named_trust,
	.get_Uri = named_uri,
	.get_Candidates = named_candidates,
	.Resolve = named_resolve,
	.Resolve2 = named_resolve_context,
	.ResolveAll = named_resolve_all,
	.ResolveAll2 = named_resolve_all_context,
};

static HRESULT WINAPI named_query(named_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CINamedResource))
		return E_NOINTERFACE;
	named_addref(iface);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI named_addref(named_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct named_resource *)iface)->refs);
}

static ULONG WINAPI named_release(named_i *iface)
{
	struct named_resource *self = (struct named_resource *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI named_iids(named_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI named_name(named_i *iface, HSTRING *name)
{
	(void)iface;
	return create_class_string(named_class_name, name);
}

static HRESULT WINAPI named_trust(named_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI named_uri(named_i *iface, uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI named_candidates(named_i *iface, uint64_t out)
{
	struct named_resource *self = (struct named_resource *)iface;
	struct candidates_view *view;
	UINT32 first, count;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	if (!candidate_range(self->entry, &first, &count))
		return E_BOUNDS;
	view = create_view(first, count);
	if (!view)
		return E_OUTOFMEMORY;
	*(void **)(uintptr_t)out = view;
	return S_OK;
}

/* Resolve answers the item's first string candidate; the context parameter is accepted — a NULL context is the default
 * one — but is not matched against the candidate qualifiers yet. */
static HRESULT WINAPI named_resolve_context(named_i *iface, uint64_t context, uint64_t out)
{
	struct named_resource *self = (struct named_resource *)iface;
	struct candidate_object *candidate;
	UINT32 first, count, i;
	(void)context;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	if (!candidate_range(self->entry, &first, &count))
		return E_BOUNDS;
	for (i = 0; i < count; ++i) {
		const WCHAR *text;
		UINT32 units;
		if (candidate_string(first + i, &text, &units) && (candidate = create_candidate(text, units))) {
			*(void **)(uintptr_t)out = candidate;
			return S_OK;
		}
	}
	return E_BOUNDS;
}

static HRESULT WINAPI named_resolve(named_i *iface, uint64_t out)
{
	return named_resolve_context(iface, 0, out);
}

static HRESULT WINAPI named_resolve_all(named_i *iface, uint64_t out)
{
	return named_candidates(iface, out);
}

static HRESULT WINAPI named_resolve_all_context(named_i *iface, uint64_t context, uint64_t out)
{
	(void)context;
	return named_candidates(iface, out);
}

static struct named_resource *create_named(UINT32 entry)
{
	struct named_resource *named = ALLOC(sizeof *named);
	if (!named)
		return NULL;
	named->lpVtbl = &named_table;
	named->refs = 1;
	named->entry = entry;
	InterlockedIncrement(&raven_xaml_objects);
	return named;
}

/* The ResourceMap: the root map is the manager's MainResourceMap, subtrees narrow it by schema scope. */
struct map_object {
	const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMapVtbl *lpVtbl;
	LONG refs;
	WCHAR prefix[PRI_MAX_PATH_UNITS]; /* with the leading slash; empty for the root */
};

static HRESULT WINAPI map_query(map_i *iface, REFIID iid, void **out);
static ULONG WINAPI map_addref(map_i *iface);
static ULONG WINAPI map_release(map_i *iface);
static HRESULT WINAPI map_iids(map_i *iface, ULONG *count, IID **iids);
static HRESULT WINAPI map_name(map_i *iface, HSTRING *name);
static HRESULT WINAPI map_trust(map_i *iface, TrustLevel *level);
static HRESULT WINAPI map_uri(map_i *iface, uint64_t out);
static HRESULT WINAPI map_get_value(map_i *iface, uint64_t name, uint64_t out);
static HRESULT WINAPI map_get_value_context(map_i *iface, uint64_t name, uint64_t context, uint64_t out);
static HRESULT WINAPI map_get_subtree(map_i *iface, uint64_t name, uint64_t out);

static const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMapVtbl map_table = {
	.QueryInterface = map_query,
	.AddRef = map_addref,
	.Release = map_release,
	.GetIids = map_iids,
	.GetRuntimeClassName = map_name,
	.GetTrustLevel = map_trust,
	.get_Uri = map_uri,
	.GetValue = map_get_value,
	.GetValue2 = map_get_value_context,
	.GetSubtree = map_get_subtree,
};

static struct map_object map_instance = {&map_table, 1, {0}}; /* the root, never freed */

static HRESULT WINAPI map_query(map_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceMap))
		return E_NOINTERFACE;
	map_addref(iface);
	*out = iface;
	return S_OK;
}

/* The root map lives as long as the process, like the manager; subtree maps free at zero. */
static ULONG WINAPI map_addref(map_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct map_object *)iface)->refs);
}

static ULONG WINAPI map_release(map_i *iface)
{
	struct map_object *self = (struct map_object *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs && iface != (map_i *)&map_instance) {
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI map_iids(map_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI map_name(map_i *iface, HSTRING *name)
{
	(void)iface;
	return create_class_string(map_class_name, name);
}

static HRESULT WINAPI map_trust(map_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI map_uri(map_i *iface, uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

/* The requested name resolves against the map's prefix: the full path is prefix + "/" + name, matched against the
 * schema's item paths without case, the way resource names look up. */
static BOOL map_find_entry(struct map_object *self, const WCHAR *name, UINT32 name_length, BOOL scope,
	UINT32 *entry)
{
	WCHAR full[PRI_MAX_PATH_UNITS];
	WCHAR path[PRI_MAX_PATH_UNITS];
	UINT32 prefix_length = (UINT32)lstrlenW(self->prefix);
	UINT32 full_length, path_length, i;
	if (prefix_length + 1 + name_length + 1 > PRI_MAX_PATH_UNITS)
		return FALSE;
	memcpy(full, self->prefix, prefix_length * sizeof(WCHAR));
	full_length = prefix_length;
	full[full_length++] = L'/';
	memcpy(full + full_length, name, name_length * sizeof(WCHAR));
	full_length += name_length;
	for (i = 0; i < pri_state.entry_count; ++i) {
		if (!!(pri_state.entries[i].flags & 0x10) != !!scope)
			continue;
		if (!entry_path(i, path, PRI_MAX_PATH_UNITS, &path_length))
			continue;
		if (CompareStringOrdinal(full, (int)full_length, path, (int)path_length, TRUE) == CSTR_EQUAL) {
			*entry = i;
			return TRUE;
		}
	}
	return FALSE;
}

static HRESULT map_lookup_entry(struct map_object *self, HSTRING name, BOOL scope, UINT32 *entry)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	if (!text)
		return E_POINTER;
	if (!pri_state.entry_count)
		return E_BOUNDS; /* no index: no resources */
	if (!map_find_entry(self, text, length, scope, entry)) {
		WCHAR message[512] = L"raven-mrt: missing resource key: ";
		UINT32 offset = (UINT32)ARRAYSIZE(L"raven-mrt: missing resource key: ") - 1;
		for (UINT32 index = 0; index < length && offset < ARRAYSIZE(message) - 1; ++index)
			message[offset++] = text[index] >= 0x20 && text[index] < 0x7f ? text[index] : L'?';
		message[offset] = 0;
		if (GetEnvironmentVariableW(L"RAVEN_TRACE_DEBUG", NULL, 0)) OutputDebugStringW(message);
		return E_BOUNDS;
	}
	return S_OK;
}

static HRESULT WINAPI map_get_value(map_i *iface, uint64_t name, uint64_t out)
{
	return map_get_value_context(iface, name, 0, out);
}

/* The context overload accepts the caller's ResourceContext or NULL for the default one; the candidates are not matched
 * against the context's qualifier values yet, so the answer is the one-argument lookup's. */
static HRESULT WINAPI map_get_value_context(map_i *iface, uint64_t name, uint64_t context, uint64_t out)
{
	struct map_object *self = (struct map_object *)iface;
	struct named_resource *named;
	UINT32 entry;
	HRESULT result;
	(void)context;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	result = map_lookup_entry(self, (HSTRING)(uintptr_t)name, FALSE, &entry);
	if (result != S_OK)
		return result;
	if (!(named = create_named(entry)))
		return E_OUTOFMEMORY;
	*(void **)(uintptr_t)out = named;
	return S_OK;
}

static HRESULT WINAPI map_get_subtree(map_i *iface, uint64_t name, uint64_t out)
{
	struct map_object *self = (struct map_object *)iface;
	struct map_object *subtree;
	UINT32 length = 0, entry, prefix_length;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	text = WindowsGetStringRawBuffer((HSTRING)(uintptr_t)name, &length);
	if (!text)
		return E_POINTER;
	if (!pri_state.entry_count)
		return E_BOUNDS;
	if (!map_find_entry(self, text, length, TRUE, &entry))
		return E_BOUNDS;
	subtree = ALLOC(sizeof *subtree);
	if (!subtree)
		return E_OUTOFMEMORY;
	prefix_length = (UINT32)lstrlenW(self->prefix);
	if (prefix_length + 1 + length + 1 > PRI_MAX_PATH_UNITS) {
		FREE(subtree);
		return E_BOUNDS;
	}
	subtree->lpVtbl = &map_table;
	subtree->refs = 1;
	memcpy(subtree->prefix, self->prefix, prefix_length * sizeof(WCHAR));
	subtree->prefix[prefix_length] = L'/';
	memcpy(subtree->prefix + prefix_length + 1, text, length * sizeof(WCHAR));
	subtree->prefix[prefix_length + 1 + length] = 0;
	InterlockedIncrement(&raven_xaml_objects);
	*(void **)(uintptr_t)out = subtree;
	return S_OK;
}

/* The ResourceManager: the process's one manager, asked for through its statics and never constructed by name. */
struct manager2_part {
	const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2Vtbl *lpVtbl;
	struct manager_object *owner;
};

struct manager_object {
	const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerVtbl *lpVtbl;
	LONG refs;
	struct manager2_part second;
};

static HRESULT WINAPI manager_query(manager_i *iface, REFIID iid, void **out);
static ULONG WINAPI manager_addref(manager_i *iface);
static ULONG WINAPI manager_release(manager_i *iface);
static HRESULT WINAPI manager_iids(manager_i *iface, ULONG *count, IID **iids);
static HRESULT WINAPI manager_name(manager_i *iface, HSTRING *name);
static HRESULT WINAPI manager_trust(manager_i *iface, TrustLevel *level);
static HRESULT WINAPI manager_main_map(manager_i *iface, uint64_t out);
static HRESULT WINAPI manager_all_maps(manager_i *iface, uint64_t out);
static HRESULT WINAPI manager_default_context(manager_i *iface, uint64_t out);
static HRESULT WINAPI manager_load_pri_files(manager_i *iface, uint64_t files);
static HRESULT WINAPI manager_unload_pri_files(manager_i *iface, uint64_t files);
static HRESULT WINAPI manager2_all_named(manager2_i *iface, uint64_t a1, uint64_t a2, uint64_t a3);
static HRESULT WINAPI manager2_all_subtrees(manager2_i *iface, uint64_t a1, uint64_t a2, uint64_t a3);

/* The second interface's part forwards its lifetime to the object, whose base its owner pointer reaches. */
#define MANAGER_PART_LIFETIME(part, iface_type) \
	static HRESULT WINAPI part##_query(iface_type *iface, REFIID iid, void **out) \
	{ return manager_query((manager_i *)((struct part##_part *)iface)->owner, iid, out); } \
	static ULONG WINAPI part##_addref(iface_type *iface) \
	{ return manager_addref((manager_i *)((struct part##_part *)iface)->owner); } \
	static ULONG WINAPI part##_release(iface_type *iface) \
	{ return manager_release((manager_i *)((struct part##_part *)iface)->owner); } \
	static HRESULT WINAPI part##_iids(iface_type *iface, ULONG *count, IID **iids) \
	{ (void)iface; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI part##_name(iface_type *iface, HSTRING *name) \
	{ (void)iface; return create_class_string(class_name, name); } \
	static HRESULT WINAPI part##_trust(iface_type *iface, TrustLevel *level) \
	{ (void)iface; return inspectable_trust(level); }

MANAGER_PART_LIFETIME(manager2, manager2_i)

static const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerVtbl manager_table = {
	.QueryInterface = manager_query,
	.AddRef = manager_addref,
	.Release = manager_release,
	.GetIids = manager_iids,
	.GetRuntimeClassName = manager_name,
	.GetTrustLevel = manager_trust,
	.get_MainResourceMap = manager_main_map,
	.get_AllResourceMaps = manager_all_maps,
	.get_DefaultContext = manager_default_context,
	.LoadPriFiles = manager_load_pri_files,
	.UnloadPriFiles = manager_unload_pri_files,
};

static const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2Vtbl manager2_table = {
	.QueryInterface = manager2_query,
	.AddRef = manager2_addref,
	.Release = manager2_release,
	.GetIids = manager2_iids,
	.GetRuntimeClassName = manager2_name,
	.GetTrustLevel = manager2_trust,
	.GetAllNamedResourcesForPackage = manager2_all_named,
	.GetAllSubtreesForPackage = manager2_all_subtrees,
};

static struct manager_object manager_instance = {
	&manager_table,
	0, /* the singleton is never freed; the count follows the references the guests hold */
	{&manager2_table, &manager_instance},
};

static HRESULT WINAPI manager_query(manager_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager))
		*out = iface;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManager2))
		*out = &((struct manager_object *)iface)->second;
	else
		return E_NOINTERFACE;
	manager_addref(iface);
	return S_OK;
}

static ULONG WINAPI manager_addref(manager_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct manager_object *)iface)->refs);
}

static ULONG WINAPI manager_release(manager_i *iface)
{
	return (ULONG)InterlockedDecrement(&((struct manager_object *)iface)->refs);
}

static HRESULT WINAPI manager_iids(manager_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI manager_name(manager_i *iface, HSTRING *name)
{
	(void)iface;
	return create_class_string(class_name, name);
}

static HRESULT WINAPI manager_trust(manager_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI manager_main_map(manager_i *iface, uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	ensure_pri();
	map_addref((map_i *)&map_instance);
	*(void **)(uintptr_t)out = &map_instance;
	return S_OK;
}

/* The per-package map view is not built; the main map is the one index this component reads. */
static HRESULT WINAPI manager_all_maps(manager_i *iface, uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

/* The default context is the ResourceContext class this component implements: its view-independent context carries the
 * default qualifier values, reached through that class's own factory. */
static HRESULT WINAPI manager_default_context(manager_i *iface, uint64_t out)
{
	IActivationFactory *factory = NULL;
	context_statics2_i *statics2 = NULL;
	HSTRING key = NULL;
	HRESULT result;
	(void)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	result = WindowsCreateString(context_class_name, (UINT32)lstrlenW(context_class_name), &key);
	if (result != S_OK)
		return result;
	result = raven_xaml_resource_context_factory(key, &factory);
	WindowsDeleteString(key);
	if (result != S_OK || !factory)
		return result == S_OK ? E_NOINTERFACE : result;
	result = factory->lpVtbl->QueryInterface(factory,
		&IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceContextStatics2, (void **)&statics2);
	factory->lpVtbl->Release(factory);
	if (result != S_OK || !statics2)
		return result == S_OK ? E_NOINTERFACE : result;
	result = statics2->lpVtbl->GetForViewIndependentUse(statics2, out);
	statics2->lpVtbl->Release(statics2);
	return result;
}

static HRESULT WINAPI manager_load_pri_files(manager_i *iface, uint64_t files)
{
	(void)iface;
	(void)files;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI manager_unload_pri_files(manager_i *iface, uint64_t files)
{
	(void)iface;
	(void)files;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI manager2_all_named(manager2_i *iface, uint64_t a1, uint64_t a2, uint64_t a3)
{
	(void)iface;
	(void)a1;
	(void)a2;
	if (!a3)
		return E_POINTER;
	*(void **)(uintptr_t)a3 = NULL;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

static HRESULT WINAPI manager2_all_subtrees(manager2_i *iface, uint64_t a1, uint64_t a2, uint64_t a3)
{
	(void)iface;
	(void)a1;
	(void)a2;
	if (!a3)
		return E_POINTER;
	*(void **)(uintptr_t)a3 = NULL;
	{ raven_trace_not_implemented(__func__); return E_NOTIMPL; }
}

/* The class factory and its statics */
struct factory_activation { const IActivationFactoryVtbl *lpVtbl; struct mrt_factory *owner; };
struct factory_statics { const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStaticsVtbl *lpVtbl; struct mrt_factory *owner; };

struct mrt_factory {
	struct factory_activation activation;
	struct factory_statics statics;
	LONG refs;
};

static HRESULT WINAPI factory_query(IActivationFactory *iface, REFIID iid, void **out);
static ULONG WINAPI factory_addref(IActivationFactory *iface);
static ULONG WINAPI factory_release(IActivationFactory *iface);
static HRESULT WINAPI factory_iids(IActivationFactory *iface, ULONG *count, IID **iids);
static HRESULT WINAPI factory_class_name(IActivationFactory *iface, HSTRING *name);
static HRESULT WINAPI factory_trust(IActivationFactory *iface, TrustLevel *level);
static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out);
static HRESULT WINAPI statics_get_current(manager_statics_i *iface, uint64_t out);
static HRESULT WINAPI statics_is_resource_reference(manager_statics_i *iface, uint64_t value, uint64_t out);

static HRESULT WINAPI statics_query(manager_statics_i *iface, REFIID iid, void **out)
{
	return factory_query((IActivationFactory *)((struct factory_statics *)iface)->owner, iid, out);
}

static ULONG WINAPI statics_addref(manager_statics_i *iface)
{
	return factory_addref((IActivationFactory *)((struct factory_statics *)iface)->owner);
}

static ULONG WINAPI statics_release(manager_statics_i *iface)
{
	return factory_release((IActivationFactory *)((struct factory_statics *)iface)->owner);
}

static HRESULT WINAPI statics_iids(manager_statics_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI statics_name(manager_statics_i *iface, HSTRING *name)
{
	(void)iface;
	return create_class_string(class_name, name);
}

static HRESULT WINAPI statics_trust(manager_statics_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

/* IsResourceReference: the documented ms-resource: prefix, compared without case like a scheme name. */
static HRESULT WINAPI statics_is_resource_reference(manager_statics_i *iface, uint64_t value, uint64_t out)
{
	UINT32 length = 0;
	const WCHAR *text;
	(void)iface;
	if (!out)
		return E_POINTER;
	text = WindowsGetStringRawBuffer((HSTRING)(uintptr_t)value, &length);
	*(boolean *)(uintptr_t)out = (boolean)(text && length >= REFERENCE_SCHEME_LENGTH &&
		CompareStringOrdinal(text, REFERENCE_SCHEME_LENGTH, reference_scheme, REFERENCE_SCHEME_LENGTH, TRUE)
			== CSTR_EQUAL);
	return S_OK;
}

static const __x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStaticsVtbl statics_table = {
	.QueryInterface = statics_query,
	.AddRef = statics_addref,
	.Release = statics_release,
	.GetIids = statics_iids,
	.GetRuntimeClassName = statics_name,
	.GetTrustLevel = statics_trust,
	.get_Current = statics_get_current,
	.IsResourceReference = statics_is_resource_reference,
};

static const IActivationFactoryVtbl factory_activation_table = {
	.QueryInterface = factory_query,
	.AddRef = factory_addref,
	.Release = factory_release,
	.GetIids = factory_iids,
	.GetRuntimeClassName = factory_class_name,
	.GetTrustLevel = factory_trust,
	.ActivateInstance = factory_activate,
};

static struct mrt_factory factory_instance = {
	{&factory_activation_table, &factory_instance},
	{&statics_table, &factory_instance},
	1,
};

static HRESULT WINAPI factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
	struct mrt_factory *self = ((struct factory_activation *)iface)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory))
		*out = &self->activation;
	else if (equal_iid(iid, &IID__x_ABI_CWindows_CApplicationModel_CResources_CCore_CIResourceManagerStatics))
		*out = &self->statics;
	else
		return E_NOINTERFACE;
	InterlockedIncrement(&self->refs);
	return S_OK;
}

static ULONG WINAPI factory_addref(IActivationFactory *iface)
{
	return (ULONG)InterlockedIncrement(&((struct factory_activation *)iface)->owner->refs);
}

static ULONG WINAPI factory_release(IActivationFactory *iface)
{
	return (ULONG)InterlockedDecrement(&((struct factory_activation *)iface)->owner->refs);
}

static HRESULT WINAPI factory_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI factory_class_name(IActivationFactory *iface, HSTRING *name)
{
	(void)iface;
	return create_class_string(class_name, name);
}

static HRESULT WINAPI factory_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

/* The class has no public constructor: the manager of a process is asked for, never constructed by name. */
static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return E_ILLEGAL_METHOD_CALL;
}

static HRESULT WINAPI statics_get_current(manager_statics_i *iface, uint64_t out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*(void **)(uintptr_t)out = NULL;
	ensure_pri();
	manager_addref((manager_i *)&manager_instance);
	*(void **)(uintptr_t)out = &manager_instance;
	return S_OK;
}

HRESULT raven_mrt_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (length != ARRAYSIZE(class_name) - 1 || CompareStringOrdinal(text, (int)length, class_name, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	InterlockedIncrement(&factory_instance.refs);
	*out = (IActivationFactory *)&factory_instance.activation;
	return S_OK;
}
