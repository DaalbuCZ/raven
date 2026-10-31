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
	LONG refs;
	SRWLOCK lock;
	struct settings_handler *handlers;
	UINT32 handler_count, handler_capacity;
	LONGLONG next_token;
	struct raven_weak_source container_weak, set_weak;
};
static const __x_ABI_CWindows_CStorage_CIApplicationDataContainerVtbl settings_container_table;
static const __x_ABI_CWindows_CFoundation_CCollections_CIPropertySetVtbl settings_set_table;
static const __FIMap_2_HSTRING_IInspectableVtbl settings_map_vtbl;
static const __FIMapView_2_HSTRING_IInspectableVtbl settings_view_vtbl;
static const __FIObservableMap_2_HSTRING_IInspectableVtbl settings_observable_vtbl;
static struct settings_object settings = {
	{(void *)&settings_container_table},
	{(void *)&settings_set_table},
	{(void *)&settings_map_vtbl},
	{(void *)&settings_view_vtbl},
	{(void *)&settings_observable_vtbl},
	0, /* the singleton is never freed; the count follows the references the guests hold, like Package.Current */
	SRWLOCK_INIT,
	NULL, 0, 0, 0,
	{NULL}, {NULL},
};
static LONG settings_sequence;
static const WCHAR hex_digits[] = L"0123456789abcdef";

/* A hash bounds the path length for Windows' 255-character keys. The stored key is checked on every access, so a
 * collision is an error rather than silently reading or replacing a different setting. */
static HRESULT settings_path(HSTRING key, WCHAR *path, WCHAR *temporary)
{
	WCHAR directory[MAX_PATH];
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(key, &length);
	UINT64 hash = 14695981039346656037ULL;
	DWORD attributes;
	if (length > 255) return E_INVALIDARG;
	if (!executable_directory(directory)) return last_error();
	if (lstrlenW(directory) + 1 + 13 + 1 + 16 + 4 + 26 >= MAX_PATH) return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	lstrcpyW(path, directory);
	lstrcatW(path, L"\\LocalSettings");
	attributes = GetFileAttributesW(path);
	if (attributes == INVALID_FILE_ATTRIBUTES) {
		if (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) return last_error();
		attributes = GetFileAttributesW(path);
	}
	if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY)) return HRESULT_FROM_WIN32(ERROR_DIRECTORY);
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

static HRESULT settings_read(HSTRING key, struct settings_record *record, WCHAR *path)
{
	DWORD got = 0;
	LARGE_INTEGER size;
	UINT32 key_length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(key, &key_length);
	HRESULT result = settings_path(key, path, NULL);
	if (FAILED(result)) return result;
	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? E_BOUNDS : last_error();
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

static HRESULT settings_lookup(HSTRING key, IInspectable **value)
{
	struct settings_record record;
	WCHAR path[MAX_PATH];
	HRESULT result;
	if (!value) return E_POINTER;
	*value = NULL;
	result = settings_read(key, &record, path);
	return SUCCEEDED(result) ? settings_decode(&record, value) : result;
}

static HRESULT settings_insert(HSTRING key, IInspectable *value, boolean *replaced)
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
	result = settings_read(key, &previous, old_path);
	if (FAILED(result) && result != E_BOUNDS) return result;
	if (SUCCEEDED(result)) {
		IInspectable *check = NULL;
		result = settings_decode(&previous, &check);
		if (FAILED(result)) return result;
		IInspectable_Release(check);
		*replaced = TRUE;
	}
	result = settings_path(key, path, temporary);
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

static HRESULT settings_remove(HSTRING key)
{
	struct settings_record record;
	WCHAR path[MAX_PATH];
	IInspectable *check = NULL;
	HRESULT result = settings_read(key, &record, path);
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

static void settings_dispatch(__x_ABI_CWindows_CFoundation_CCollections_CCollectionChange change, HSTRING key);

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

static ULONG settings_release(struct settings_object *self) { return (ULONG)InterlockedDecrement(&self->refs); }

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

static HRESULT WINAPI settings_owner_query(REFIID iid, void **out)
{
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) ||
		IsEqualIID(iid, &IID___x_ABI_CWindows_CFoundation_CCollections_CIPropertySet))
		*out = &settings.set;
	else if (IsEqualIID(iid, &IID___FIMap_2_HSTRING_IInspectable))
		*out = &settings.map;
	else if (IsEqualIID(iid, &IID___FIMapView_2_HSTRING_IInspectable))
		*out = &settings.view;
	else if (IsEqualIID(iid, &IID___FIObservableMap_2_HSTRING_IInspectable))
		*out = &settings.observable;
	else if (IsEqualIID(iid, &IID_IWeakReferenceSource))
		*out = &settings.set_weak;
	else
		return E_NOINTERFACE;
	settings_addref(&settings);
	return S_OK;
}

static HRESULT WINAPI settings_container_query(settings_container_i *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IInspectable) ||
		IsEqualIID(iid, &IID___x_ABI_CWindows_CStorage_CIApplicationDataContainer))
		*out = &settings.container;
	else if (IsEqualIID(iid, &IID_IWeakReferenceSource))
		*out = &settings.container_weak;
	else
		return E_NOINTERFACE;
	settings_addref(&settings);
	return S_OK;
}

static HRESULT settings_set_name(HSTRING *name)
{
	static const WCHAR text[] = L"Windows.Foundation.Collections.PropertySet";	if (!name)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)ARRAYSIZE(text) - 1, name);
}

/* The number of stored settings equals the number of record files in LocalSettings. */
static HRESULT settings_walk(UINT32 *count)
{
	WCHAR directory[MAX_PATH], pattern[MAX_PATH];
	WIN32_FIND_DATAW found;
	HANDLE handle;
	UINT32 entries = 0;
	if (!count)
		return E_POINTER;
	*count = 0;
	if (!executable_directory(directory))
		return last_error();
	if ((UINT32)lstrlenW(directory) + 1 + 13 + 1 + 5 >= MAX_PATH)
		return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	lstrcatW(directory, L"\\LocalSettings");
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
	(void)iface;
	return settings_lookup(key, value);
}

static HRESULT WINAPI settings_map_size(__FIMap_2_HSTRING_IInspectable *iface, UINT32 *size)
{
	(void)iface;
	return settings_walk(size);
}

static HRESULT WINAPI settings_map_has_key(__FIMap_2_HSTRING_IInspectable *iface, HSTRING key, boolean *found)
{
	struct settings_record record;
	WCHAR path[MAX_PATH];
	HRESULT result;
	(void)iface;
	if (!found)
		return E_POINTER;
	result = settings_read(key, &record, path);
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
	(void)iface;
	result = settings_insert(key, value, replaced);
	if (SUCCEEDED(result)) {
		HSTRING change_key = NULL;
		if (WindowsDuplicateString(key, &change_key) == S_OK) {
			settings_dispatch(*replaced ? CollectionChange_ItemChanged : CollectionChange_ItemInserted, change_key);
			WindowsDeleteString(change_key);
		}
	}
	return result;
}

static HRESULT WINAPI settings_map_remove(__FIMap_2_HSTRING_IInspectable *iface, HSTRING key)
{
	HRESULT result;
	(void)iface;
	result = settings_remove(key);
	if (SUCCEEDED(result)) {
		HSTRING change_key = NULL;
		if (WindowsDuplicateString(key, &change_key) == S_OK) {
			settings_dispatch(CollectionChange_ItemRemoved, change_key);
			WindowsDeleteString(change_key);
		}
	}
	return result;
}

static HRESULT WINAPI settings_map_clear(__FIMap_2_HSTRING_IInspectable *iface)
{
	(void)iface;
	/* A directory scan cannot safely identify every record yet. */
	return E_NOTIMPL;
}

static HRESULT WINAPI settings_view_lookup(__FIMapView_2_HSTRING_IInspectable *iface, HSTRING key, IInspectable **value)
{
	(void)iface;
	return settings_lookup(key, value);
}

static HRESULT WINAPI settings_view_size(__FIMapView_2_HSTRING_IInspectable *iface, UINT32 *size)
{
	(void)iface;
	return settings_walk(size);
}

static HRESULT WINAPI settings_view_has_key(__FIMapView_2_HSTRING_IInspectable *iface, HSTRING key, boolean *found)
{
	(void)iface;
	return settings_map_has_key(&settings.map, key, found);
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
static void settings_dispatch(__x_ABI_CWindows_CFoundation_CCollections_CCollectionChange change, HSTRING key)
{
	struct settings_object *self = &settings;
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
	(void)iface;
	if (!value)
		return E_POINTER;
	return WindowsCreateString(text, (UINT32)ARRAYSIZE(text) - 1, value);
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
	if (!value)
		return E_POINTER;
	settings_addref(&settings);
	*value = &settings.set;
	return S_OK;
}

/* shortcut: nested containers are unsupported, add isolated persistent namespaces before enabling them. */
static HRESULT WINAPI settings_container_view(settings_container_i *iface, __FIMapView_2_HSTRING_Windows__CStorage__CApplicationDataContainer **value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI settings_container_create_container(settings_container_i *iface, HSTRING name,
	__x_ABI_CWindows_CStorage_CApplicationDataCreateDisposition disposition, settings_container_i **container)
{
	(void)iface;
	(void)name;
	(void)disposition;
	if (!container)
		return E_POINTER;
	*container = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI settings_container_delete_container(settings_container_i *iface, HSTRING name)
{
	(void)iface;
	(void)name;
	return E_NOTIMPL;
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
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return settings_owner_query(iid, out);
}

static HRESULT WINAPI settings_view_query(__FIMapView_2_HSTRING_IInspectable *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return settings_owner_query(iid, out);
}

static HRESULT WINAPI settings_observed_query(__FIObservableMap_2_HSTRING_IInspectable *iface, REFIID iid, void **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return settings_owner_query(iid, out);
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
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return settings_owner_query(iid, out);
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

static HRESULT WINAPI settings_weak_query(void *owner, REFIID iid, void **out)
{
	if (owner == &settings.container)
		return settings_container_query(&settings.container, iid, out);
	if (!out) return E_POINTER;
	*out = NULL;
	return settings_owner_query(iid, out);
}

static void settings_init_weak(void)
{
	static LONG once;
	if (InterlockedCompareExchange(&once, 1, 0) == 0) {
		raven_weak_source_init(&settings.container_weak, &settings.container, &settings.refs, settings_weak_query);
		raven_weak_source_init(&settings.set_weak, &settings.set, &settings.refs, settings_weak_query);
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
