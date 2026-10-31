/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Persistent local application settings. Files are written in the same directory as LocalState, but are never placed
 * inside LocalFolder: application code can enumerate and delete its own files without affecting its settings. */
typedef __x_ABI_CWindows_CStorage_CIApplicationDataContainer settings_container_i;
typedef __x_ABI_CWindows_CFoundation_CCollections_CIPropertySet settings_set_i;
typedef __x_ABI_CWindows_CFoundation_CIPropertyValue property_value_i;
typedef __x_ABI_CWindows_CFoundation_CIPropertyValueStatics property_statics_i;
typedef __x_ABI_CWindows_CFoundation_CPropertyType property_type;
extern HRESULT raven_property_value_statics(property_statics_i **out);

#define SETTINGS_LIMIT 8192U
#define SETTINGS_MAGIC 0x31535652U
struct settings_record {
	UINT32 magic, key_length, type, length;
	WCHAR key[256];
	BYTE data[SETTINGS_LIMIT];
};
/* One process-wide store backs the container and values. It does not claim agility: concurrent mutations and
 * cross-process updates are not serialized yet. */
struct settings_handler {
	LONGLONG token;
	__FIMapChangedEventHandler_2_HSTRING_IInspectable *handler;
};
struct settings_object {
	settings_container_i container;
	settings_set_i set;
	__FIMap_2_HSTRING_IInspectable map;
	__FIMapView_2_HSTRING_IInspectable view;
	__FIObservableMap_2_HSTRING_IInspectable observable;
	__FIMapView_2_HSTRING_Windows__CStorage__CApplicationDataContainer children;
	LONG refs;
	SRWLOCK lock;
	struct settings_handler *handlers;
	UINT32 handler_count, handler_capacity;
	LONGLONG next_token;
	struct raven_weak_source container_weak, set_weak, children_weak;
	BOOL dynamic;
	UINT64 id;
	UINT64 parent_id;
	UINT64 generation_time, generation_id;
	UINT32 depth;
	HSTRING name;
};
static const __x_ABI_CWindows_CStorage_CIApplicationDataContainerVtbl settings_container_table;
static const __x_ABI_CWindows_CFoundation_CCollections_CIPropertySetVtbl settings_set_table;
static const __FIMap_2_HSTRING_IInspectableVtbl settings_map_vtbl;
static const __FIMapView_2_HSTRING_IInspectableVtbl settings_view_vtbl;
static const __FIObservableMap_2_HSTRING_IInspectableVtbl settings_observable_vtbl;
static const __FIMapView_2_HSTRING_Windows__CStorage__CApplicationDataContainerVtbl settings_children_vtbl;
static struct settings_object settings = {
	.container = {(void *)&settings_container_table}, .set = {(void *)&settings_set_table},
	.map = {(void *)&settings_map_vtbl}, .view = {(void *)&settings_view_vtbl},
	.observable = {(void *)&settings_observable_vtbl}, .children = {(void *)&settings_children_vtbl}, .lock = SRWLOCK_INIT,
};
static LONG settings_sequence;
static const WCHAR hex_digits[] = L"0123456789abcdef";

/* Container identifiers are flat: even a 32-level hierarchy cannot exhaust MAX_PATH merely by nesting. Each
 * directory has a manifest containing its parent and exact name, so a hash collision never aliases settings. */
#define CONTAINER_MAGIC 0x31435652U
struct container_record {
	UINT32 magic, depth, name_length, reserved;
	UINT64 parent, generation_time, generation_id, parent_generation_time, parent_generation_id;
	WCHAR name[256];
};

static HRESULT settings_root_path(WCHAR *path, BOOL create)
{
	DWORD attributes;
	if (!executable_directory(path)) return last_error();
	if (lstrlenW(path) + 14 >= MAX_PATH) return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	lstrcatW(path, L"\\LocalSettings");
	attributes = GetFileAttributesW(path);
	if (attributes == INVALID_FILE_ATTRIBUTES && create) {
		if (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) return last_error();
		attributes = GetFileAttributesW(path);
	}
	if (attributes == INVALID_FILE_ATTRIBUTES) return last_error();
	return (attributes & FILE_ATTRIBUTE_DIRECTORY) && !(attributes & FILE_ATTRIBUTE_REPARSE_POINT) ? S_OK : HRESULT_FROM_WIN32(ERROR_DIRECTORY);
}

static HRESULT settings_containers_path(WCHAR *path, BOOL create)
{
	HRESULT result = settings_root_path(path, create);
	DWORD attributes;
	if (FAILED(result)) return result;
	if (lstrlenW(path) + 12 + 1 + 16 + 1 + 16 + 4 + 26 >= MAX_PATH) return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	lstrcatW(path, L"\\containers");
	attributes = GetFileAttributesW(path);
	if (attributes == INVALID_FILE_ATTRIBUTES && create) {
		if (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) return last_error();
		attributes = GetFileAttributesW(path);
	}
	if (attributes == INVALID_FILE_ATTRIBUTES) return last_error();
	return (attributes & FILE_ATTRIBUTE_DIRECTORY) && !(attributes & FILE_ATTRIBUTE_REPARSE_POINT) ? S_OK : HRESULT_FROM_WIN32(ERROR_DIRECTORY);
}

static HRESULT settings_id_path(UINT64 id, WCHAR *path, BOOL create_base)
{
	HRESULT result = settings_containers_path(path, create_base);
	if (FAILED(result)) return result;
	SIZE_T offset = (SIZE_T)lstrlenW(path);
	path[offset++] = L'\\';
	for (int i = 15; i >= 0; --i) path[offset++] = hex_digits[(id >> (i * 4)) & 15];
	path[offset] = 0;
	return S_OK;
}

static UINT64 settings_child_id(UINT64 parent, const WCHAR *name, UINT32 length)
{
	UINT64 hash = 14695981039346656037ULL;
	for (int i = 0; i < 8; ++i) hash = (hash ^ ((parent >> (i * 8)) & 255)) * 1099511628211ULL;
	for (UINT32 i = 0; i < length; ++i) {
		hash = (hash ^ (name[i] & 255)) * 1099511628211ULL;
		hash = (hash ^ (name[i] >> 8)) * 1099511628211ULL;
	}
	return hash;
}

static HRESULT settings_read_container(UINT64 id, struct container_record *record)
{
	WCHAR path[MAX_PATH];
	DWORD got = 0;
	LARGE_INTEGER size;
	HRESULT result = settings_id_path(id, path, FALSE);
	if (FAILED(result)) return result;
	DWORD attributes = GetFileAttributesW(path);
	if (attributes == INVALID_FILE_ATTRIBUTES) return last_error();
	if (!(attributes & FILE_ATTRIBUTE_DIRECTORY) || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	lstrcatW(path, L"\\container.rvc");
	attributes = GetFileAttributesW(path);
	if (attributes == INVALID_FILE_ATTRIBUTES) return last_error();
	if (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) return last_error();
	if (!GetFileSizeEx(file, &size)) { result = last_error(); CloseHandle(file); return result; }
	if (size.QuadPart != sizeof *record) { CloseHandle(file); return HRESULT_FROM_WIN32(ERROR_INVALID_DATA); }
	if (!ReadFile(file, record, sizeof *record, &got, NULL)) { result = last_error(); CloseHandle(file); return result; }
	CloseHandle(file);
	if (got != sizeof *record || record->magic != CONTAINER_MAGIC || !record->depth || record->depth > 32 ||
		record->name_length > 255 || !record->name_length ||
		settings_child_id(record->parent, record->name, record->name_length) != id)
		return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	return S_OK;
}

static HRESULT settings_directory(struct settings_object *self, WCHAR *path)
{
	if (!self->dynamic) return settings_root_path(path, TRUE);
	struct container_record record;
	HRESULT result = settings_read_container(self->id, &record);
	if (FAILED(result)) return result;
	UINT32 length = 0;
	const WCHAR *name = WindowsGetStringRawBuffer(self->name, &length);
	if (record.parent != self->parent_id || record.depth != self->depth || record.name_length != length ||
		record.generation_time != self->generation_time || record.generation_id != self->generation_id ||
		memcmp(record.name, name, length * sizeof(WCHAR))) return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	UINT32 depth = record.depth;
	UINT64 parent = record.parent;
	UINT64 parent_time = record.parent_generation_time, parent_generation = record.parent_generation_id;
	while (depth > 1) {
		if (!parent) return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		result = settings_read_container(parent, &record);
		if (FAILED(result)) return result;
		if (record.depth != --depth || record.generation_time != parent_time || record.generation_id != parent_generation)
			return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		parent = record.parent;
		parent_time = record.parent_generation_time; parent_generation = record.parent_generation_id;
	}
	if (parent || parent_time || parent_generation) return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	return settings_id_path(self->id, path, FALSE);
}

/* A hash bounds the path length for Windows' 255-character keys. The stored key is checked on every access, so a
 * collision is an error rather than silently reading or replacing a different setting. */
static HRESULT settings_directory(struct settings_object *self, WCHAR *path);

static HRESULT settings_path(struct settings_object *self, HSTRING key, WCHAR *path, WCHAR *temporary)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(key, &length);
	UINT64 hash = 14695981039346656037ULL;
	if (length > 255) return E_INVALIDARG;
	HRESULT result = settings_directory(self, path);
	if (FAILED(result)) return result;
	if (lstrlenW(path) + 1 + 16 + 4 + 26 >= MAX_PATH) return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	for (UINT32 i = 0; i < length; ++i) {
		hash = (hash ^ (text[i] & 255)) * 1099511628211ULL;
		hash = (hash ^ (text[i] >> 8)) * 1099511628211ULL;
	}
	SIZE_T offset = (SIZE_T)lstrlenW(path);
	path[offset++] = L'\\';
	for (int i = 15; i >= 0; --i) path[offset++] = hex_digits[(hash >> (i * 4)) & 15];
	path[offset] = 0;
	lstrcatW(path, L".rvs");
	if (temporary) {
		lstrcpyW(temporary, path);
		offset = (SIZE_T)lstrlenW(temporary);
		temporary[offset++] = L'.';
		DWORD ids[3] = {GetCurrentProcessId(), GetCurrentThreadId(), (DWORD)InterlockedIncrement(&settings_sequence)};
		for (int part = 0; part < 3; ++part)
			for (int i = 7; i >= 0; --i) temporary[offset++] = hex_digits[(ids[part] >> (i * 4)) & 15];
		temporary[offset] = 0;
	}
	return S_OK;
}

static HRESULT settings_read(struct settings_object *self, HSTRING key, struct settings_record *record, WCHAR *path)
{
	DWORD got = 0;
	LARGE_INTEGER size;
	UINT32 key_length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(key, &key_length);
	HRESULT result = settings_path(self, key, path, NULL);
	if (FAILED(result)) return result;
	DWORD attributes = GetFileAttributesW(path);
	if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
		return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
			WCHAR message[320] = L"raven-settings: missing value key: ";
			UINT32 offset = (UINT32)ARRAYSIZE(L"raven-settings: missing value key: ") - 1;
			for (UINT32 index = 0; index < key_length && offset < ARRAYSIZE(message) - 1; ++index)
				message[offset++] = text[index] >= 0x20 && text[index] < 0x7f ? text[index] : L'?';
			message[offset] = 0;
			if (GetEnvironmentVariableW(L"RAVEN_TRACE_DEBUG", NULL, 0)) OutputDebugStringW(message);
			return E_BOUNDS;
		}
		return last_error();
	}
	if (!GetFileSizeEx(file, &size)) {
		result = last_error();
		CloseHandle(file);
		return result;
	}
	if (size.QuadPart < 16 || size.QuadPart > sizeof *record) {
		CloseHandle(file);
		return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	}
	if (!ReadFile(file, record, (DWORD)size.QuadPart, &got, NULL)) {
		result = last_error();
		CloseHandle(file);
		return result;
	}
	CloseHandle(file);
	if (got != (DWORD)size.QuadPart) return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	if (record->magic != SETTINGS_MAGIC || record->key_length > 255 || record->length > SETTINGS_LIMIT ||
		size.QuadPart != (LONGLONG)(16 + sizeof record->key + record->length)) return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	if (record->key_length != key_length || (key_length && memcmp(record->key, text, key_length * sizeof(WCHAR))))
		return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
	return S_OK;
}

static HRESULT settings_encode(IInspectable *object, struct settings_record *record)
{
	property_value_i *value = NULL;
	property_type type;
	HRESULT result;
	if (!object || FAILED(IInspectable_QueryInterface(object, &IID___x_ABI_CWindows_CFoundation_CIPropertyValue, (void **)&value)))
		return E_INVALIDARG;
	result = value->lpVtbl->get_Type(value, &type);
	if (SUCCEEDED(result)) {
		record->type = (UINT32)type;
		switch (type) {
		case PropertyType_Boolean: record->length = sizeof(boolean); result = value->lpVtbl->GetBoolean(value, (boolean *)record->data); break;
		case PropertyType_Int32: record->length = sizeof(INT32); result = value->lpVtbl->GetInt32(value, (INT32 *)record->data); break;
		case PropertyType_UInt32: record->length = sizeof(UINT32); result = value->lpVtbl->GetUInt32(value, (UINT32 *)record->data); break;
		case PropertyType_Int64: record->length = sizeof(INT64); result = value->lpVtbl->GetInt64(value, (INT64 *)record->data); break;
		case PropertyType_UInt64: record->length = sizeof(UINT64); result = value->lpVtbl->GetUInt64(value, (UINT64 *)record->data); break;
		case PropertyType_Single: record->length = sizeof(FLOAT); result = value->lpVtbl->GetSingle(value, (FLOAT *)record->data); break;
		case PropertyType_Double: record->length = sizeof(DOUBLE); result = value->lpVtbl->GetDouble(value, (DOUBLE *)record->data); break;
		case PropertyType_String: {
			HSTRING string = NULL;
			result = value->lpVtbl->GetString(value, &string);
			if (SUCCEEDED(result)) {
				UINT32 length = 0;
				const WCHAR *text = WindowsGetStringRawBuffer(string, &length);
				if (length > SETTINGS_LIMIT / sizeof(WCHAR)) result = E_INVALIDARG;
				else { record->length = length * sizeof(WCHAR); if (length) memcpy(record->data, text, record->length); }
				WindowsDeleteString(string);
			}
			break;
		}
		default: result = E_INVALIDARG; break;
		}
	}
	value->lpVtbl->Release(value);
	return result;
}

static HRESULT settings_decode(const struct settings_record *record, IInspectable **out)
{
	property_statics_i *factory = NULL;
	HRESULT result = raven_property_value_statics(&factory);
	if (FAILED(result)) return result;
	switch (record->type) {
#define DECODE(TYPE, CODE, METHOD) \
	case CODE: { TYPE value; if (record->length != sizeof value) { result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); break; } \
		memcpy(&value, record->data, sizeof value); result = factory->lpVtbl->METHOD(factory, value, out); break; }
	DECODE(boolean, PropertyType_Boolean, CreateBoolean)
	DECODE(INT32, PropertyType_Int32, CreateInt32)
	DECODE(UINT32, PropertyType_UInt32, CreateUInt32)
	DECODE(INT64, PropertyType_Int64, CreateInt64)
	DECODE(UINT64, PropertyType_UInt64, CreateUInt64)
	DECODE(FLOAT, PropertyType_Single, CreateSingle)
	DECODE(DOUBLE, PropertyType_Double, CreateDouble)
#undef DECODE
	case PropertyType_String: {
		HSTRING text = NULL;
		if (record->length % sizeof(WCHAR)) { result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); break; }
		result = WindowsCreateString((const WCHAR *)record->data, record->length / sizeof(WCHAR), &text);
		if (SUCCEEDED(result)) result = factory->lpVtbl->CreateString(factory, text, out);
		WindowsDeleteString(text);
		break;
	}
	default: result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); break;
	}
	factory->lpVtbl->Release(factory);
	return result;
}

static HRESULT settings_lookup(struct settings_object *self, HSTRING key, IInspectable **value)
{
	struct settings_record record;
	WCHAR path[MAX_PATH];
	HRESULT result;
	if (!value) return E_POINTER;
	*value = NULL;
	result = settings_read(self, key, &record, path);
	return SUCCEEDED(result) ? settings_decode(&record, value) : result;
}

static HRESULT settings_insert(struct settings_object *self, HSTRING key, IInspectable *value, boolean *replaced)
{
	struct settings_record record = {0}, previous;
	WCHAR path[MAX_PATH], temporary[MAX_PATH], old_path[MAX_PATH];
	UINT32 length = 0;
	DWORD written = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(key, &length);
	HRESULT result;
	HANDLE file;
	if (!replaced) return E_POINTER;
	*replaced = FALSE;
	if (length > 255) return E_INVALIDARG;
	result = settings_encode(value, &record);
	if (FAILED(result)) return result;
	result = settings_read(self, key, &previous, old_path);
	if (FAILED(result) && result != E_BOUNDS) return result;
	if (SUCCEEDED(result)) {
		IInspectable *check = NULL;
		result = settings_decode(&previous, &check);
		if (FAILED(result)) return result;
		IInspectable_Release(check);
		*replaced = TRUE;
	}
	result = settings_path(self, key, path, temporary);
	if (FAILED(result)) return result;
	record.magic = SETTINGS_MAGIC;
	record.key_length = length;
	if (length) memcpy(record.key, text, length * sizeof(WCHAR));
	file = CreateFileW(temporary, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) return last_error();
	DWORD bytes = (DWORD)(16 + sizeof record.key + record.length);
	if (!WriteFile(file, &record, bytes, &written, NULL)) result = last_error();
	else if (written != bytes) result = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
	else if (!FlushFileBuffers(file)) result = last_error();
	if (!CloseHandle(file) && SUCCEEDED(result)) result = last_error();
	if (SUCCEEDED(result) && !MoveFileExW(temporary, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) result = last_error();
	if (FAILED(result)) { DeleteFileW(temporary); *replaced = FALSE; }
	return result;
}

static HRESULT settings_remove(struct settings_object *self, HSTRING key)
{
	struct settings_record record;
	WCHAR path[MAX_PATH];
	IInspectable *check = NULL;
	HRESULT result = settings_read(self, key, &record, path);
	if (FAILED(result)) return result;
	result = settings_decode(&record, &check);
	if (FAILED(result)) return result;
	IInspectable_Release(check);
	return DeleteFileW(path) ? S_OK : last_error();
}

/* ---- the container object ----
 *
 * get_LocalSettings hands out this object; the container part is the public face and its QueryInterface routes the values
 * PropertySet and every collection interface the values imply. Windows.Storage.winmd (SHA-256 21f4e900…) and mingw's
 * windows.storage.h agree that IApplicationDataContainer carries exactly six methods — Name, Locality, Values, Containers,
 * CreateContainer and DeleteContainer; Path, HasKey and DeleteContainerAsync are absent from both, so the object answers
 * them at no slot. Nested containers have no storage scheme below this file store (the record files carry no parent), so
 * the two container-creation methods and the Containers view refuse. */

#define SETTINGS_OWNER(iface, field) ((struct settings_object *)((char *)(iface) - offsetof(struct settings_object, field)))
#define SETTINGS_ARGS(iface) ((struct settings_change *)((char *)(iface) - offsetof(struct settings_change, iface)))

static void settings_dispatch(struct settings_object *self, __x_ABI_CWindows_CFoundation_CCollections_CCollectionChange change, HSTRING key);

static const IID *settings_iids_list[] = {
	&IID___x_ABI_CWindows_CFoundation_CCollections_CIPropertySet,
	&IID___FIMap_2_HSTRING_IInspectable,
	&IID___FIMapView_2_HSTRING_IInspectable,
	&IID___FIObservableMap_2_HSTRING_IInspectable,
};

static HRESULT WINAPI settings_copied_iids(const IID *list, ULONG size, ULONG *count, IID **iids)
{
	IID *copy;
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	copy = ALLOC(size * sizeof *copy);
	if (!copy)
		return E_OUTOFMEMORY;
	for (ULONG index = 0; index < size; ++index)
		copy[index] = list[index];
	*count = size;
	*iids = copy;
	return S_OK;
}

static ULONG settings_addref(struct settings_object *self) { return (ULONG)InterlockedIncrement(&self->refs); }

static ULONG settings_release(struct settings_object *self)
{
	LONG refs = InterlockedDecrement(&self->refs);
	if (!refs && self->dynamic) {
		raven_weak_source_disconnect(&self->container_weak);
		raven_weak_source_disconnect(&self->set_weak);
		raven_weak_source_disconnect(&self->children_weak);
		for (UINT32 i = 0; i < self->handler_count; ++i) self->handlers[i].handler->lpVtbl->Release(self->handlers[i].handler);
		FREE(self->handlers);
		WindowsDeleteString(self->name);
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

/* GetIids answers the interfaces the object really implements, never an empty list; the buffer answers with the same
 * HeapAlloc the other GetIids paths of the component hand out (see resources.c static_iids). */
static HRESULT settings_owned_iids(struct settings_object *self, ULONG *count, IID **iids)
{
	IID *list;
	(void)self;
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	list = ALLOC(sizeof settings_iids_list);
	if (!list)
		return E_OUTOFMEMORY;
	for (ULONG index = 0; index < ARRAYSIZE(settings_iids_list); ++index)
		list[index] = *settings_iids_list[index];
	*count = (ULONG)ARRAYSIZE(settings_iids_list);
	*iids = list;
	return S_OK;
}

static HRESULT WINAPI settings_owner_query(struct settings_object *self, REFIID iid, void **out)
{
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) ||
		IsEqualIID(iid, &IID___x_ABI_CWindows_CFoundation_CCollections_CIPropertySet))
		*out = &self->set;
	else if (IsEqualIID(iid, &IID___FIMap_2_HSTRING_IInspectable))
		*out = &self->map;
	else if (IsEqualIID(iid, &IID___FIMapView_2_HSTRING_IInspectable))
		*out = &self->view;
	else if (IsEqualIID(iid, &IID___FIObservableMap_2_HSTRING_IInspectable))
		*out = &self->observable;
	else if (IsEqualIID(iid, &IID_IWeakReferenceSource))
		*out = &self->set_weak;
	else
		return E_NOINTERFACE;
	settings_addref(self);
	return S_OK;
}

static HRESULT WINAPI settings_container_query(settings_container_i *iface, REFIID iid, void **out)
{
	struct settings_object *self = SETTINGS_OWNER(iface, container);
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) ||
		IsEqualIID(iid, &IID___x_ABI_CWindows_CStorage_CIApplicationDataContainer))
		*out = &self->container;
	else if (IsEqualIID(iid, &IID_IWeakReferenceSource))
		*out = &self->container_weak;
	else
		return E_NOINTERFACE;
	settings_addref(self);
	return S_OK;
}

static HRESULT settings_set_name(HSTRING *name)
{
	static const WCHAR text[] = L"Windows.Foundation.Collections.PropertySet";	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)ARRAYSIZE(text) - 1, name);
}

/* The number of stored settings equals the number of record files in LocalSettings. */
static HRESULT settings_walk(struct settings_object *self, UINT32 *count)
{
	WCHAR directory[MAX_PATH], pattern[MAX_PATH];
	WIN32_FIND_DATAW found;
	HANDLE handle;
	UINT32 entries = 0;
	if (!count)
		return E_POINTER;
	*count = 0;
	HRESULT result = settings_directory(self, directory);
	if (FAILED(result)) return result;
	if ((UINT32)lstrlenW(directory) + 1 + 5 >= MAX_PATH)
		return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	lstrcpyW(pattern, directory);
	lstrcatW(pattern, L"\\*.rvs");
	handle = FindFirstFileW(pattern, &found);
	if (handle == INVALID_HANDLE_VALUE) {
		const DWORD error = GetLastError();
		if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
			return S_OK;
		return HRESULT_FROM_WIN32(error);
	}
	do {
		if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		++entries;
	} while (FindNextFileW(handle, &found));
	if (!FindClose(handle))
		return last_error();
	*count = entries;
	return S_OK;
}

static HRESULT WINAPI settings_map_lookup(__FIMap_2_HSTRING_IInspectable *iface, HSTRING key, IInspectable **value)
{
	HRESULT result = settings_lookup(SETTINGS_OWNER(iface, map), key, value);
	/* ApplicationDataContainerSettings projects absent values as null, unlike a general PropertySet. */
	return result == E_BOUNDS ? S_OK : result;
}

static HRESULT WINAPI settings_map_size(__FIMap_2_HSTRING_IInspectable *iface, UINT32 *size)
{
	return settings_walk(SETTINGS_OWNER(iface, map), size);
}

static HRESULT WINAPI settings_map_has_key(__FIMap_2_HSTRING_IInspectable *iface, HSTRING key, boolean *found)
{
	struct settings_record record;
	WCHAR path[MAX_PATH];
	HRESULT result;
	if (!found)
		return E_POINTER;
	result = settings_read(SETTINGS_OWNER(iface, map), key, &record, path);
	if (SUCCEEDED(result)) {
		IInspectable *value = NULL;
		result = settings_decode(&record, &value);
		if (value) IInspectable_Release(value);
	}
	*found = (boolean)SUCCEEDED(result);
	return result == E_BOUNDS ? S_OK : result;
}

static HRESULT WINAPI settings_map_get_view(__FIMap_2_HSTRING_IInspectable *iface, __FIMapView_2_HSTRING_IInspectable **view)
{
	struct settings_object *self = SETTINGS_OWNER(iface, map);
	if (!view)
		return E_POINTER;
	settings_addref(self);
	*view = &self->view;
	return S_OK;
}

static HRESULT WINAPI settings_map_insert(__FIMap_2_HSTRING_IInspectable *iface, HSTRING key, IInspectable *value,
	boolean *replaced)
{
	HRESULT result;
	struct settings_object *self = SETTINGS_OWNER(iface, map);
	result = settings_insert(self, key, value, replaced);
	if (SUCCEEDED(result)) {
		HSTRING change_key = NULL;
		if (WindowsDuplicateString(key, &change_key) == S_OK) {
			settings_dispatch(self, *replaced ? CollectionChange_ItemChanged : CollectionChange_ItemInserted, change_key);
			WindowsDeleteString(change_key);
		}
	}
	return result;
}

static HRESULT WINAPI settings_map_remove(__FIMap_2_HSTRING_IInspectable *iface, HSTRING key)
{
	HRESULT result;
	struct settings_object *self = SETTINGS_OWNER(iface, map);
	result = settings_remove(self, key);
	if (SUCCEEDED(result)) {
		HSTRING change_key = NULL;
		if (WindowsDuplicateString(key, &change_key) == S_OK) {
			settings_dispatch(self, CollectionChange_ItemRemoved, change_key);
			WindowsDeleteString(change_key);
		}
	}
	return result;
}

static HRESULT settings_clear_pass(struct settings_object *self, BOOL remove)
{
	WCHAR directory[MAX_PATH], pattern[MAX_PATH], path[MAX_PATH], expected[MAX_PATH];
	WIN32_FIND_DATAW found;
	HRESULT result = settings_directory(self, directory);
	if (FAILED(result)) return result;
	if (lstrlenW(directory) + 22 >= MAX_PATH) return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	lstrcpyW(pattern, directory); lstrcatW(pattern, L"\\*.rvs");
	HANDLE search = FindFirstFileW(pattern, &found);
	if (search == INVALID_HANDLE_VALUE)
		return GetLastError() == ERROR_FILE_NOT_FOUND ? S_OK : last_error();
	do {
		struct settings_record record, verified;
		DWORD got = 0;
		LARGE_INTEGER size;
		HSTRING key = NULL;
		IInspectable *value = NULL;
		if ((found.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) ||
			lstrlenW(found.cFileName) != 20 || lstrcmpW(found.cFileName + 16, L".rvs")) {
			result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); break;
		}
		lstrcpyW(path, directory); lstrcatW(path, L"\\"); lstrcatW(path, found.cFileName);
		HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE) { result = last_error(); break; }
		if (!GetFileSizeEx(file, &size) || size.QuadPart < 16 + sizeof record.key || size.QuadPart > sizeof record ||
			!ReadFile(file, &record, (DWORD)size.QuadPart, &got, NULL) || got != (DWORD)size.QuadPart ||
			record.magic != SETTINGS_MAGIC || record.key_length > 255 || record.length > SETTINGS_LIMIT ||
			size.QuadPart != (LONGLONG)(16 + sizeof record.key + record.length))
			result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		CloseHandle(file);
		if (FAILED(result)) break;
		result = WindowsCreateString(record.key, record.key_length, &key);
		if (SUCCEEDED(result)) result = settings_read(self, key, &verified, expected);
		if (SUCCEEDED(result) && lstrcmpW(expected, path)) result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		if (SUCCEEDED(result)) result = settings_decode(&verified, &value);
		if (value) IInspectable_Release(value);
		WindowsDeleteString(key);
		if (FAILED(result)) break;
		if (remove && !DeleteFileW(path)) { result = last_error(); break; }
	} while (FindNextFileW(search, &found));
	if (SUCCEEDED(result) && GetLastError() != ERROR_NO_MORE_FILES) result = last_error();
	FindClose(search);
	return result;
}

static HRESULT WINAPI settings_map_clear(__FIMap_2_HSTRING_IInspectable *iface)
{
	struct settings_object *self = SETTINGS_OWNER(iface, map);
	HRESULT result = settings_clear_pass(self, FALSE);
	if (SUCCEEDED(result)) result = settings_clear_pass(self, TRUE);
	if (SUCCEEDED(result)) settings_dispatch(self, CollectionChange_Reset, NULL);
	return result;
}

static HRESULT WINAPI settings_view_lookup(__FIMapView_2_HSTRING_IInspectable *iface, HSTRING key, IInspectable **value)
{
	return settings_lookup(SETTINGS_OWNER(iface, view), key, value);
}

static HRESULT WINAPI settings_view_size(__FIMapView_2_HSTRING_IInspectable *iface, UINT32 *size)
{
	return settings_walk(SETTINGS_OWNER(iface, view), size);
}

static HRESULT WINAPI settings_view_has_key(__FIMapView_2_HSTRING_IInspectable *iface, HSTRING key, boolean *found)
{
	return settings_map_has_key(&SETTINGS_OWNER(iface, view)->map, key, found);
}

static HRESULT WINAPI settings_view_split(__FIMapView_2_HSTRING_IInspectable *iface, __FIMapView_2_HSTRING_IInspectable **first,
	__FIMapView_2_HSTRING_IInspectable **second)
{
	(void)iface;
	if (!first || !second)
		return E_POINTER;
	/* This live view cannot be partitioned. */
	*first = NULL;
	*second = NULL;
	return S_OK;
}

static HRESULT WINAPI settings_observed_add(__FIObservableMap_2_HSTRING_IInspectable *iface,
	__FIMapChangedEventHandler_2_HSTRING_IInspectable *handler, EventRegistrationToken *token)
{
	struct settings_object *self = SETTINGS_OWNER(iface, observable);
	if (!token)
		return E_POINTER;
	token->value = 0;
	if (!handler)
		return E_INVALIDARG;
	AcquireSRWLockExclusive(&self->lock);
	if (self->handler_count == self->handler_capacity) {
		const UINT32 capacity = self->handler_capacity ? self->handler_capacity * 2 : 4;
		struct settings_handler *grown = self->handlers
			? (struct settings_handler *)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, self->handlers,
				capacity * sizeof *grown)
			: (struct settings_handler *)ALLOC(capacity * sizeof *grown);
		if (!grown) {
			ReleaseSRWLockExclusive(&self->lock);
			return E_OUTOFMEMORY;
		}
		self->handlers = grown;
		self->handler_capacity = capacity;
	}
	handler->lpVtbl->AddRef(handler);
	self->handlers[self->handler_count].token = ++self->next_token;
	self->handlers[self->handler_count].handler = handler;
	++self->handler_count;
	token->value = self->next_token;
	ReleaseSRWLockExclusive(&self->lock);
	return S_OK;
}

static HRESULT WINAPI settings_observed_remove(__FIObservableMap_2_HSTRING_IInspectable *iface, EventRegistrationToken token)
{
	struct settings_object *self = SETTINGS_OWNER(iface, observable);
	__FIMapChangedEventHandler_2_HSTRING_IInspectable *handler = NULL;
	if (token.value == 0)
		return E_INVALIDARG;
	AcquireSRWLockExclusive(&self->lock);
	for (UINT32 index = 0; index < self->handler_count; ++index) {
		if (self->handlers[index].token == token.value) {
			handler = self->handlers[index].handler;
			memmove(&self->handlers[index], &self->handlers[index + 1],
				(self->handler_count - index - 1) * sizeof *self->handlers);
			--self->handler_count;
			break;
		}
	}
	ReleaseSRWLockExclusive(&self->lock);
	if (!handler)
		return E_BOUNDS;
	handler->lpVtbl->Release(handler);
	return S_OK;
}

/* The MapChanged event carries a fresh IMapChangedEventArgs<String> per change; the object loses its last internal
 * reference after the last handler's answer. */
struct settings_change {
	__FIMapChangedEventArgs_1_HSTRING iface;
	HSTRING key;
	__x_ABI_CWindows_CFoundation_CCollections_CCollectionChange change;
	LONG refs;
	struct raven_weak_source weak;
};

static ULONG WINAPI settings_change_addref(__FIMapChangedEventArgs_1_HSTRING *iface)
{
	return (ULONG)InterlockedIncrement(&SETTINGS_ARGS(iface)->refs);
}

static ULONG WINAPI settings_change_release(__FIMapChangedEventArgs_1_HSTRING *iface)
{
	struct settings_change *self = SETTINGS_ARGS(iface);
	const LONG refs = InterlockedDecrement(&self->refs);
	if (refs)
		return (ULONG)refs;
	raven_weak_source_disconnect(&self->weak);
	if (self->key)
		WindowsDeleteString(self->key);
	FREE(self);
	return 0;
}

static HRESULT WINAPI settings_change_query(__FIMapChangedEventArgs_1_HSTRING *iface, REFIID iid, void **out)
{
	struct settings_change *self = SETTINGS_ARGS(iface);
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) ||
		IsEqualIID(iid, &IID___FIMapChangedEventArgs_1_HSTRING))
		*out = iface;
	else if (IsEqualIID(iid, &IID_IWeakReferenceSource))
		*out = &self->weak;
	else if (IsEqualIID(iid, &IID_IAgileObject))
		/* agile: the arguments are immutable snapshots, the reference count is interlocked and nothing is shared */
		*out = iface;
	else
		return E_NOINTERFACE;
	settings_change_addref(iface);
	return S_OK;
}

static HRESULT WINAPI settings_change_iids(__FIMapChangedEventArgs_1_HSTRING *iface, ULONG *count, IID **iids)
{
	static const IID list[] = {IID___FIMapChangedEventArgs_1_HSTRING};
	(void)iface;
	if (!count || !iids)
		return E_POINTER;
	return settings_copied_iids(list, (ULONG)ARRAYSIZE(list), count, iids);
}

static HRESULT WINAPI settings_change_name(__FIMapChangedEventArgs_1_HSTRING *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Windows.Foundation.Collections.IMapChangedEventArgs`1<String>";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)ARRAYSIZE(text) - 1, name);
}

static HRESULT WINAPI settings_change_trust(__FIMapChangedEventArgs_1_HSTRING *iface, TrustLevel *level)
{
	(void)iface;
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

static HRESULT WINAPI settings_change_kind(__FIMapChangedEventArgs_1_HSTRING *iface,
	__x_ABI_CWindows_CFoundation_CCollections_CCollectionChange *value)
{
	if (!value)
		return E_POINTER;
	*value = SETTINGS_ARGS(iface)->change;
	return S_OK;
}

static HRESULT WINAPI settings_change_key(__FIMapChangedEventArgs_1_HSTRING *iface, HSTRING *key)
{
	struct settings_change *self = SETTINGS_ARGS(iface);
	if (!key)
		return E_POINTER;
	/* a reset carries the empty key */
	if (self->key)
		return WindowsDuplicateString(self->key, key);
	*key = NULL;
	return S_OK;
}

static const __FIMapChangedEventArgs_1_HSTRINGVtbl settings_change_table = {
	.QueryInterface = settings_change_query, .AddRef = settings_change_addref, .Release = settings_change_release,
	.GetIids = settings_change_iids, .GetRuntimeClassName = settings_change_name, .GetTrustLevel = settings_change_trust,
	.get_CollectionChanged = settings_change_kind, .get_Key = settings_change_key,
};

static HRESULT settings_change_create(__x_ABI_CWindows_CFoundation_CCollections_CCollectionChange change,
	HSTRING key /* borrowed */, __FIMapChangedEventArgs_1_HSTRING **out)
{
	struct settings_change *args;
	if (!out)
		return E_POINTER;
	*out = NULL;
	args = ALLOC(sizeof *args);
	if (!args)
		return E_OUTOFMEMORY;
	args->iface.lpVtbl = (void *)&settings_change_table;
	if (key) {
		HRESULT result = WindowsDuplicateString(key, &args->key);
		if (FAILED(result)) {
			FREE(args);
			return result;
		}
	}
	args->change = change;
	args->refs = 1;
	raven_weak_source_init(&args->weak, args, &args->refs, (raven_weak_query_fn)settings_change_query);
	*out = &args->iface;
	return S_OK;
}

/* Every handler raised so far receives exactly one event, and no later registration or removal changes the catch: the list
 * is snapshotted. A failing or re-entering handler never stops the pass. */
static void settings_dispatch(struct settings_object *self, __x_ABI_CWindows_CFoundation_CCollections_CCollectionChange change, HSTRING key)
{
	__FIMapChangedEventHandler_2_HSTRING_IInspectable **askers = NULL;
	__FIMapChangedEventArgs_1_HSTRING *event_args = NULL;
	UINT32 count = 0;
	AcquireSRWLockExclusive(&self->lock);
	if (self->handler_count) {
		askers = ALLOC(self->handler_count * sizeof *askers);
		if (askers)
			for (UINT32 index = 0; index < self->handler_count; ++index) {
				askers[count] = self->handlers[index].handler;
				askers[count]->lpVtbl->AddRef(askers[count]);
				++count;
			}
	}
	ReleaseSRWLockExclusive(&self->lock);
	if (!count)
		return;
	if (settings_change_create(change, key, &event_args) == S_OK && event_args) {
		for (UINT32 index = 0; index < count; ++index)
			askers[index]->lpVtbl->Invoke(askers[index], &self->observable, event_args);
		event_args->lpVtbl->Release(event_args);
	}
	for (UINT32 index = 0; index < count; ++index)
		askers[index]->lpVtbl->Release(askers[index]);
	FREE(askers);
}

static HRESULT WINAPI settings_container_name(settings_container_i *iface, HSTRING *value)
{
	static const WCHAR text[] = L"Local";
	struct settings_object *self = SETTINGS_OWNER(iface, container);
	if (!value)
		return E_POINTER;
	return self->dynamic ? WindowsDuplicateString(self->name, value) : WindowsCreateString(text, (UINT32)ARRAYSIZE(text) - 1, value);
}

static HRESULT WINAPI settings_container_locality(settings_container_i *iface,
	__x_ABI_CWindows_CStorage_CApplicationDataLocality *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = ApplicationDataLocality_Local;
	return S_OK;
}

static HRESULT WINAPI settings_container_values(settings_container_i *iface, settings_set_i **value)
{
	WCHAR path[MAX_PATH];
	if (!value)
		return E_POINTER;
	struct settings_object *self = SETTINGS_OWNER(iface, container);
	*value = NULL;
	HRESULT result = settings_directory(self, path);
	if (FAILED(result)) return result;
	settings_addref(self);
	*value = &self->set;
	return S_OK;
}

static HRESULT WINAPI settings_container_weak_query(void *owner, REFIID iid, void **out);
static HRESULT WINAPI settings_set_weak_query(void *owner, REFIID iid, void **out);
static HRESULT WINAPI settings_children_weak_query(void *owner, REFIID iid, void **out);

static HRESULT settings_open_child(struct settings_object *parent, HSTRING name,
	__x_ABI_CWindows_CStorage_CApplicationDataCreateDisposition disposition, settings_container_i **out)
{
	WCHAR path[MAX_PATH], manifest[MAX_PATH], parent_path[MAX_PATH];
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	struct container_record record = {0}, existing;
	struct settings_object *child;
	DWORD written = 0;
	BOOL created = FALSE;
	HRESULT result;
	if (!out) return E_POINTER;
	*out = NULL;
	if (!length || length > 255 || (disposition != ApplicationDataCreateDisposition_Always &&
		disposition != ApplicationDataCreateDisposition_Existing)) return E_INVALIDARG;
	if (parent->depth >= 32) return HRESULT_FROM_WIN32(ERROR_DIRECTORY);
	result = settings_directory(parent, parent_path);
	if (FAILED(result)) return result;
	UINT64 id = settings_child_id(parent->id, text, length);
	if (!id) return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
	result = settings_id_path(id, path, disposition == ApplicationDataCreateDisposition_Always);
	if (FAILED(result)) return result;
	child = ALLOC(sizeof *child);
	if (!child) return E_OUTOFMEMORY;
	result = WindowsDuplicateString(name, &child->name);
	if (FAILED(result)) { FREE(child); return result; }
	if (disposition == ApplicationDataCreateDisposition_Always) {
		if (CreateDirectoryW(path, NULL)) created = TRUE;
		else if (GetLastError() != ERROR_ALREADY_EXISTS) { result = last_error(); goto fail; }
	}
	if (created) {
		record.magic = CONTAINER_MAGIC; record.parent = parent->id; record.depth = parent->depth + 1; record.name_length = length;
		FILETIME now;
		GetSystemTimeAsFileTime(&now);
		record.generation_time = ((UINT64)now.dwHighDateTime << 32) | now.dwLowDateTime;
		record.generation_id = ((UINT64)GetCurrentProcessId() << 32) | (UINT32)InterlockedIncrement(&settings_sequence);
		record.parent_generation_time = parent->generation_time; record.parent_generation_id = parent->generation_id;
		memcpy(record.name, text, length * sizeof(WCHAR));
		lstrcpyW(manifest, path); lstrcatW(manifest, L"\\container.rvc");
		HANDLE file = CreateFileW(manifest, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE) { result = last_error(); RemoveDirectoryW(path); goto fail; }
		if (!WriteFile(file, &record, sizeof record, &written, NULL)) result = last_error();
		else if (written != sizeof record) result = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
		else if (!FlushFileBuffers(file)) result = last_error();
		else result = S_OK;
		if (!CloseHandle(file) && SUCCEEDED(result)) result = last_error();
		if (FAILED(result)) { DeleteFileW(manifest); RemoveDirectoryW(path); goto fail; }
	} else {
		result = settings_read_container(id, &existing);
		if (FAILED(result)) goto fail;
		if (existing.parent != parent->id || existing.depth != parent->depth + 1 || existing.name_length != length ||
			existing.parent_generation_time != parent->generation_time || existing.parent_generation_id != parent->generation_id ||
			memcmp(existing.name, text, length * sizeof(WCHAR))) { result = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS); goto fail; }
		record = existing;
	}
	child->container.lpVtbl = (void *)&settings_container_table;
	child->set.lpVtbl = (void *)&settings_set_table;
	child->map.lpVtbl = (void *)&settings_map_vtbl;
	child->view.lpVtbl = (void *)&settings_view_vtbl;
	child->observable.lpVtbl = (void *)&settings_observable_vtbl;
	child->children.lpVtbl = (void *)&settings_children_vtbl;
	InitializeSRWLock(&child->lock);
	child->refs = 1; child->dynamic = TRUE; child->id = id; child->parent_id = parent->id; child->depth = parent->depth + 1;
	child->generation_time = record.generation_time; child->generation_id = record.generation_id;
	raven_weak_source_init(&child->container_weak, child, &child->refs, settings_container_weak_query);
	raven_weak_source_init(&child->set_weak, child, &child->refs, settings_set_weak_query);
	raven_weak_source_init(&child->children_weak, child, &child->refs, settings_children_weak_query);
	InterlockedIncrement(&raven_xaml_objects);
	*out = &child->container;
	return S_OK;
fail:
	WindowsDeleteString(child->name);
	FREE(child);
	return result;
}

static HRESULT settings_delete_child(struct settings_object *parent, HSTRING name);

static HRESULT WINAPI settings_container_view(settings_container_i *iface, __FIMapView_2_HSTRING_Windows__CStorage__CApplicationDataContainer **value)
{
	struct settings_object *self = SETTINGS_OWNER(iface, container);
	if (!value)
		return E_POINTER;
	WCHAR path[MAX_PATH];
	HRESULT result = settings_directory(self, path);
	if (FAILED(result)) { *value = NULL; return result; }
	settings_addref(self);
	*value = &self->children;
	return S_OK;
}

static HRESULT WINAPI settings_container_create_container(settings_container_i *iface, HSTRING name,
	__x_ABI_CWindows_CStorage_CApplicationDataCreateDisposition disposition, settings_container_i **container)
{
	return settings_open_child(SETTINGS_OWNER(iface, container), name, disposition, container);
}

static HRESULT WINAPI settings_container_delete_container(settings_container_i *iface, HSTRING name)
{
	return settings_delete_child(SETTINGS_OWNER(iface, container), name);
}

static HRESULT WINAPI settings_container_class(settings_container_i *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Windows.Storage.ApplicationDataContainer";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)ARRAYSIZE(text) - 1, name);
}

static HRESULT WINAPI settings_container_trust(settings_container_i *iface, TrustLevel *level)
{
	(void)iface;
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

static HRESULT WINAPI settings_container_iids(settings_container_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return settings_copied_iids(&IID___x_ABI_CWindows_CStorage_CIApplicationDataContainer, 1, count, iids);
}

static ULONG WINAPI settings_container_addref_v(settings_container_i *iface)
{
	return settings_addref(SETTINGS_OWNER(iface, container));
}

static ULONG WINAPI settings_container_release_v(settings_container_i *iface)
{
	return settings_release(SETTINGS_OWNER(iface, container));
}

static HRESULT WINAPI settings_set_iids(settings_set_i *iface, ULONG *count, IID **iids)
{
	return settings_owned_iids(SETTINGS_OWNER(iface, set), count, iids);
}

static ULONG WINAPI settings_map_addref(__FIMap_2_HSTRING_IInspectable *iface)
{
	return settings_addref(SETTINGS_OWNER(iface, map));
}

static ULONG WINAPI settings_map_release(__FIMap_2_HSTRING_IInspectable *iface)
{
	return settings_release(SETTINGS_OWNER(iface, map));
}

static HRESULT WINAPI settings_map_iids(__FIMap_2_HSTRING_IInspectable *iface, ULONG *count, IID **iids)
{
	return settings_owned_iids(SETTINGS_OWNER(iface, map), count, iids);
}

static HRESULT WINAPI settings_map_name(__FIMap_2_HSTRING_IInspectable *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Windows.Foundation.Collections.IMap`2<String,IInspectable>";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)ARRAYSIZE(text) - 1, name);
}

static HRESULT WINAPI settings_map_trust(__FIMap_2_HSTRING_IInspectable *iface, TrustLevel *level)
{
	(void)iface;
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

static ULONG WINAPI settings_view_addref(__FIMapView_2_HSTRING_IInspectable *iface)
{
	return settings_addref(SETTINGS_OWNER(iface, view));
}

static ULONG WINAPI settings_view_release(__FIMapView_2_HSTRING_IInspectable *iface)
{
	return settings_release(SETTINGS_OWNER(iface, view));
}

static HRESULT WINAPI settings_view_iids(__FIMapView_2_HSTRING_IInspectable *iface, ULONG *count, IID **iids)
{
	return settings_owned_iids(SETTINGS_OWNER(iface, view), count, iids);
}

static HRESULT WINAPI settings_view_name(__FIMapView_2_HSTRING_IInspectable *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Windows.Foundation.Collections.IMapView`2<String,IInspectable>";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)ARRAYSIZE(text) - 1, name);
}

static HRESULT WINAPI settings_view_trust(__FIMapView_2_HSTRING_IInspectable *iface, TrustLevel *level)
{
	(void)iface;
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

static ULONG WINAPI settings_observed_addref(__FIObservableMap_2_HSTRING_IInspectable *iface)
{
	return settings_addref(SETTINGS_OWNER(iface, observable));
}

static ULONG WINAPI settings_observed_release(__FIObservableMap_2_HSTRING_IInspectable *iface)
{
	return settings_release(SETTINGS_OWNER(iface, observable));
}

static HRESULT WINAPI settings_observed_iids(__FIObservableMap_2_HSTRING_IInspectable *iface, ULONG *count, IID **iids)
{
	return settings_owned_iids(SETTINGS_OWNER(iface, observable), count, iids);
}

static HRESULT WINAPI settings_observed_name(__FIObservableMap_2_HSTRING_IInspectable *iface, HSTRING *name)
{
	static const WCHAR text[] = L"Windows.Foundation.Collections.IObservableMap`2<String,IInspectable>";
	(void)iface;
	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)ARRAYSIZE(text) - 1, name);
}

static HRESULT WINAPI settings_observed_trust(__FIObservableMap_2_HSTRING_IInspectable *iface, TrustLevel *level)
{
	(void)iface;
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

/* Every part's QueryInterface routes through one dispatch so an ask against any part answers every part; the AddRef and
 * Release life each part inherits from the shared object count. */
static HRESULT WINAPI settings_set_query_v(settings_set_i *iface, REFIID iid, void **out);
static ULONG WINAPI settings_set_addref_v(settings_set_i *iface);
static ULONG WINAPI settings_set_release_v(settings_set_i *iface);
static HRESULT WINAPI settings_set_class(settings_set_i *iface, HSTRING *name);
static HRESULT WINAPI settings_set_trust(settings_set_i *iface, TrustLevel *level);

static HRESULT WINAPI settings_map_query(__FIMap_2_HSTRING_IInspectable *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	return settings_owner_query(SETTINGS_OWNER(iface, map), iid, out);
}

static HRESULT WINAPI settings_view_query(__FIMapView_2_HSTRING_IInspectable *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	return settings_owner_query(SETTINGS_OWNER(iface, view), iid, out);
}

static HRESULT WINAPI settings_observed_query(__FIObservableMap_2_HSTRING_IInspectable *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	return settings_owner_query(SETTINGS_OWNER(iface, observable), iid, out);
}

static const __FIMap_2_HSTRING_IInspectableVtbl settings_map_vtbl = {
	.QueryInterface = settings_map_query, .AddRef = settings_map_addref, .Release = settings_map_release,
	.GetIids = settings_map_iids, .GetRuntimeClassName = settings_map_name, .GetTrustLevel = settings_map_trust,
	.Lookup = settings_map_lookup, .get_Size = settings_map_size, .HasKey = settings_map_has_key,
	.GetView = settings_map_get_view, .Insert = settings_map_insert, .Remove = settings_map_remove,
	.Clear = settings_map_clear,
};

static const __FIMapView_2_HSTRING_IInspectableVtbl settings_view_vtbl = {
	.QueryInterface = settings_view_query, .AddRef = settings_view_addref, .Release = settings_view_release,
	.GetIids = settings_view_iids, .GetRuntimeClassName = settings_view_name, .GetTrustLevel = settings_view_trust,
	.Lookup = settings_view_lookup, .get_Size = settings_view_size, .HasKey = settings_view_has_key,
	.Split = settings_view_split,
};

static const __FIObservableMap_2_HSTRING_IInspectableVtbl settings_observable_vtbl = {
	.QueryInterface = settings_observed_query, .AddRef = settings_observed_addref, .Release = settings_observed_release,
	.GetIids = settings_observed_iids, .GetRuntimeClassName = settings_observed_name, .GetTrustLevel = settings_observed_trust,
	.add_MapChanged = settings_observed_add, .remove_MapChanged = settings_observed_remove,
};

static const __x_ABI_CWindows_CFoundation_CCollections_CIPropertySetVtbl settings_set_table = {
	.QueryInterface = settings_set_query_v, .AddRef = settings_set_addref_v, .Release = settings_set_release_v,
	.GetIids = settings_set_iids, .GetRuntimeClassName = settings_set_class, .GetTrustLevel = settings_set_trust,
};

static const __x_ABI_CWindows_CStorage_CIApplicationDataContainerVtbl settings_container_table = {
	.QueryInterface = settings_container_query, .AddRef = settings_container_addref_v, .Release = settings_container_release_v,
	.GetIids = settings_container_iids, .GetRuntimeClassName = settings_container_class, .GetTrustLevel = settings_container_trust,
	.get_Name = settings_container_name, .get_Locality = settings_container_locality, .get_Values = settings_container_values,
	.get_Containers = settings_container_view, .CreateContainer = settings_container_create_container,
	.DeleteContainer = settings_container_delete_container,
};

static ULONG WINAPI settings_set_addref_v(settings_set_i *iface)
{
	return settings_addref(SETTINGS_OWNER(iface, set));
}

static ULONG WINAPI settings_set_release_v(settings_set_i *iface)
{
	return settings_release(SETTINGS_OWNER(iface, set));
}

static HRESULT WINAPI settings_set_query_v(settings_set_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	return settings_owner_query(SETTINGS_OWNER(iface, set), iid, out);
}

static HRESULT WINAPI settings_set_class(settings_set_i *iface, HSTRING *name) { return settings_set_name(name); }

static HRESULT WINAPI settings_set_trust(settings_set_i *iface, TrustLevel *level)
{
	(void)iface;
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

/* Walk the flat container directory. A corrupt manifest blocks enumeration and deletion rather than risking a
 * different namespace. The two-pass delete validates the entire subtree before changing any file. */
static HRESULT settings_walk_children(UINT64 parent, UINT32 parent_depth, UINT64 parent_time, UINT64 parent_generation,
	BOOL remove, BOOL validate_only, UINT32 *count);

static HRESULT settings_delete_tree(UINT64 id, const struct container_record *manifest, BOOL validate_only)
{
	WCHAR path[MAX_PATH], pattern[MAX_PATH], expected[MAX_PATH], file_path[MAX_PATH];
	WIN32_FIND_DATAW found;
	HANDLE search;
	HRESULT result;
	UINT32 ignored = 0;
	struct settings_object temporary = {0};
	HSTRING name = NULL;
	result = settings_walk_children(id, manifest->depth, manifest->generation_time, manifest->generation_id, TRUE, validate_only, &ignored);
	if (FAILED(result)) return result;
	result = WindowsCreateString(manifest->name, manifest->name_length, &name);
	if (FAILED(result)) return result;
	temporary.dynamic = TRUE; temporary.id = id; temporary.parent_id = manifest->parent;
	temporary.depth = manifest->depth; temporary.name = name;
	temporary.generation_time = manifest->generation_time; temporary.generation_id = manifest->generation_id;
	result = settings_id_path(id, path, FALSE);
	if (FAILED(result)) goto done;
	lstrcpyW(pattern, path); lstrcatW(pattern, L"\\*");
	search = FindFirstFileW(pattern, &found);
	if (search == INVALID_HANDLE_VALUE) { result = last_error(); goto done; }
	do {
		struct settings_record record;
		HSTRING key = NULL;
		IInspectable *value = NULL;
		DWORD got = 0;
		LARGE_INTEGER size;
		if (!lstrcmpW(found.cFileName, L".") || !lstrcmpW(found.cFileName, L"..") ||
			!lstrcmpW(found.cFileName, L"container.rvc")) continue;
		if ((found.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) || lstrlenW(found.cFileName) != 20 ||
			lstrcmpW(found.cFileName + 16, L".rvs")) { result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); break; }
		lstrcpyW(file_path, path); lstrcatW(file_path, L"\\"); lstrcatW(file_path, found.cFileName);
		HANDLE file = CreateFileW(file_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE) { result = last_error(); break; }
		if (!GetFileSizeEx(file, &size) || size.QuadPart < 16 + sizeof record.key || size.QuadPart > sizeof record ||
			!ReadFile(file, &record, (DWORD)size.QuadPart, &got, NULL)) {
			CloseHandle(file); result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); break;
		}
		CloseHandle(file);
		if (got != (DWORD)size.QuadPart || record.magic != SETTINGS_MAGIC || record.key_length > 255 ||
			record.length > SETTINGS_LIMIT || size.QuadPart != (LONGLONG)(16 + sizeof record.key + record.length)) {
			result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); break;
		}
		result = WindowsCreateString(record.key, record.key_length, &key);
		if (SUCCEEDED(result)) result = settings_path(&temporary, key, expected, NULL);
		if (SUCCEEDED(result) && lstrcmpW(expected, file_path)) result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		if (SUCCEEDED(result)) result = settings_decode(&record, &value);
		if (value) IInspectable_Release(value);
		WindowsDeleteString(key);
		if (FAILED(result)) break;
		if (!validate_only && !DeleteFileW(file_path)) { result = last_error(); break; }
	} while (FindNextFileW(search, &found));
	if (SUCCEEDED(result) && GetLastError() != ERROR_NO_MORE_FILES) result = last_error();
	FindClose(search);
	if (SUCCEEDED(result) && !validate_only) {
		lstrcpyW(file_path, path); lstrcatW(file_path, L"\\container.rvc");
		if (!DeleteFileW(file_path) || !RemoveDirectoryW(path)) result = last_error();
	}
done:
	WindowsDeleteString(name);
	return result;
}

static HRESULT settings_walk_children(UINT64 parent, UINT32 parent_depth, UINT64 parent_time, UINT64 parent_generation,
	BOOL remove, BOOL validate_only, UINT32 *count)
{
	WCHAR path[MAX_PATH], pattern[MAX_PATH];
	WIN32_FIND_DATAW found;
	HRESULT result = settings_containers_path(path, FALSE);
	*count = 0;
	if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || result == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) return S_OK;
	if (FAILED(result)) return result;
	lstrcpyW(pattern, path); lstrcatW(pattern, L"\\*");
	HANDLE search = FindFirstFileW(pattern, &found);
	if (search == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		return error == ERROR_FILE_NOT_FOUND ? S_OK : HRESULT_FROM_WIN32(error);
	}
	do {
		UINT64 id = 0;
		struct container_record record;
		if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || lstrlenW(found.cFileName) != 16) continue;
		if (found.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) { result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); break; }
		BOOL valid = TRUE;
		for (int i = 0; i < 16; ++i) {
			int digit;
			for (digit = 0; digit < 16 && hex_digits[digit] != found.cFileName[i]; ++digit) {}
			if (digit == 16) { valid = FALSE; break; }
			id = (id << 4) | (UINT64)digit;
		}
		if (!valid || !id) continue;
		result = settings_read_container(id, &record);
		/* FindFirstFile snapshots the flat directory; descendants removed by this same walk may still appear. */
		if (remove && !validate_only && (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
			result == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))) { result = S_OK; continue; }
		if (FAILED(result)) break;
		if (record.parent != parent) continue;
		if (parent_depth >= 32 || record.depth != parent_depth + 1 || record.parent_generation_time != parent_time ||
			record.parent_generation_id != parent_generation) { result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); break; }
		++*count;
		if (remove) {
			result = settings_delete_tree(id, &record, validate_only);
			if (FAILED(result)) break;
		}
	} while (FindNextFileW(search, &found));
	if (SUCCEEDED(result) && GetLastError() != ERROR_NO_MORE_FILES) result = last_error();
	FindClose(search);
	return result;
}

static HRESULT settings_delete_child(struct settings_object *parent, HSTRING name)
{
	settings_container_i *child = NULL;
	struct container_record record;
	HRESULT result = settings_open_child(parent, name, ApplicationDataCreateDisposition_Existing, &child);
	if (FAILED(result)) return result;
	struct settings_object *self = SETTINGS_OWNER(child, container);
	result = settings_read_container(self->id, &record);
	if (SUCCEEDED(result)) result = settings_delete_tree(self->id, &record, TRUE);
	if (SUCCEEDED(result)) result = settings_delete_tree(self->id, &record, FALSE);
	child->lpVtbl->Release(child);
	return result;
}

typedef __FIMapView_2_HSTRING_Windows__CStorage__CApplicationDataContainer children_i;
static HRESULT WINAPI children_query(children_i *iface, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	struct settings_object *self = SETTINGS_OWNER(iface, children);
	if (IsEqualIID(iid, &IID_IWeakReferenceSource)) *out = &self->children_weak;
	else if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) ||
		IsEqualIID(iid, &IID___FIMapView_2_HSTRING_Windows__CStorage__CApplicationDataContainer)) *out = iface;
	else return E_NOINTERFACE;
	settings_addref(self);
	return S_OK;
}
static ULONG WINAPI children_addref(children_i *iface) { return settings_addref(SETTINGS_OWNER(iface, children)); }
static ULONG WINAPI children_release(children_i *iface) { return settings_release(SETTINGS_OWNER(iface, children)); }
static HRESULT WINAPI children_iids(children_i *iface, ULONG *count, IID **iids)
{ (void)iface; return settings_copied_iids(&IID___FIMapView_2_HSTRING_Windows__CStorage__CApplicationDataContainer, 1, count, iids); }
static HRESULT WINAPI children_name(children_i *iface, HSTRING *name)
{ static const WCHAR text[] = L"Windows.Foundation.Collections.IMapView`2<String,Windows.Storage.ApplicationDataContainer>";
  (void)iface; return name ? WindowsCreateString(text, ARRAYSIZE(text) - 1, name) : E_POINTER; }
static HRESULT WINAPI children_trust(children_i *iface, TrustLevel *level)
{ (void)iface; if (!level) return E_POINTER; *level = BaseTrust; return S_OK; }
static HRESULT WINAPI children_lookup(children_i *iface, HSTRING key, settings_container_i **out)
{
	struct settings_object *self = SETTINGS_OWNER(iface, children);
	WCHAR path[MAX_PATH];
	if (!out) return E_POINTER;
	*out = NULL;
	HRESULT result = settings_directory(self, path);
	if (FAILED(result)) return result;
	result = settings_open_child(self, key, ApplicationDataCreateDisposition_Existing, out);
	return result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || result == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) ? E_BOUNDS : result;
}
static HRESULT WINAPI children_size(children_i *iface, UINT32 *out)
{
	struct settings_object *self = SETTINGS_OWNER(iface, children);
	WCHAR path[MAX_PATH];
	if (!out) return E_POINTER;
	*out = 0;
	HRESULT result = settings_directory(self, path);
	return FAILED(result) ? result : settings_walk_children(self->id, self->depth, self->generation_time,
		self->generation_id, FALSE, TRUE, out);
}
static HRESULT WINAPI children_has_key(children_i *iface, HSTRING key, boolean *out)
{
	settings_container_i *child = NULL;
	if (!out) return E_POINTER;
	HRESULT result = children_lookup(iface, key, &child);
	*out = SUCCEEDED(result);
	if (child) child->lpVtbl->Release(child);
	return result == E_BOUNDS ? S_OK : result;
}
static HRESULT WINAPI children_split(children_i *iface, children_i **first, children_i **second)
{ (void)iface; if (!first || !second) return E_POINTER; *first = NULL; *second = NULL; return S_OK; }
static const __FIMapView_2_HSTRING_Windows__CStorage__CApplicationDataContainerVtbl settings_children_vtbl = {
	children_query, children_addref, children_release, children_iids, children_name, children_trust,
	children_lookup, children_size, children_has_key, children_split
};

static HRESULT WINAPI settings_container_weak_query(void *owner, REFIID iid, void **out)
{
	struct settings_object *self = owner;
	return settings_container_query(&self->container, iid, out);
}

static HRESULT WINAPI settings_set_weak_query(void *owner, REFIID iid, void **out)
{
	struct settings_object *self = owner;
	if (!out) return E_POINTER;
	*out = NULL;
	return settings_owner_query(self, iid, out);
}

static HRESULT WINAPI settings_children_weak_query(void *owner, REFIID iid, void **out)
{
	struct settings_object *self = owner;
	return children_query(&self->children, iid, out);
}

static void settings_init_weak(void)
{
	static LONG once;
	if (InterlockedCompareExchange(&once, 1, 0) == 0) {
		raven_weak_source_init(&settings.container_weak, &settings, &settings.refs, settings_container_weak_query);
		raven_weak_source_init(&settings.set_weak, &settings, &settings.refs, settings_set_weak_query);
		raven_weak_source_init(&settings.children_weak, &settings, &settings.refs, settings_children_weak_query);
		InterlockedExchange(&once, 2);
	} else {
		while (InterlockedCompareExchange(&once, 2, 2) != 2) Sleep(0);
	}
}

/* The single settings entry of ApplicationData: the process hands out one Local container, real Windows replies. */
static HRESULT storage_local_settings(settings_container_i **value)
{
	if (!value)
		return E_POINTER;
	settings_init_weak();
	settings_addref(&settings);
	*value = &settings.container;
	return S_OK;
}
