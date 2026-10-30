/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: the Windows.ApplicationModel.Package class of the Windows Runtime.
 *
 * A guest process carries its package identity in the AppxManifest.xml beside its executable: Raven keeps no package
 * database, so the manifest found on disk is the registration. get_Current answers the one package of the process, a
 * per-process singleton whose IPackageId follows the documented identity construction — the family and full names end
 * in the publisher identifier, the first eight bytes of the SHA-256 of the UTF-16 publisher string, Crockford base 32.
 *
 * A manifest that is missing or carries no usable Identity answers the failure a process without package identity gets:
 * get_Current still succeeds and hands out the package with an empty Id (empty Name and Publisher, version 0.0.0.0),
 * which a guest can tell apart from an identified package without catching. The collection-valued properties
 * (Dependencies' vector view, InstalledLocation's folder, Logo's Uri) answer E_NOTIMPL: no observed startup path has
 * asked for them yet. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <winstring.h>
#include <windows.applicationmodel.h>

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))
#ifndef E_ILLEGAL_METHOD_CALL
#define E_ILLEGAL_METHOD_CALL ((HRESULT)0x8000000e)
#endif

typedef __x_ABI_CWindows_CApplicationModel_CIPackage package_i;
typedef __x_ABI_CWindows_CApplicationModel_CIPackage2 package2_i;
typedef __x_ABI_CWindows_CApplicationModel_CIPackage3 package3_i;
typedef __x_ABI_CWindows_CApplicationModel_CIPackageId package_id_i;
typedef __x_ABI_CWindows_CApplicationModel_CIPackageStatus package_status_i;
typedef __x_ABI_CWindows_CApplicationModel_CIPackageStatics package_statics_i;
typedef __x_ABI_CWindows_CApplicationModel_CPackageVersion package_version;
typedef __x_ABI_CWindows_CSystem_CProcessorArchitecture package_architecture;

static const WCHAR class_name[] = L"Windows.ApplicationModel.Package";
static const WCHAR id_type_name[] = L"Windows.ApplicationModel.PackageId";
static const WCHAR status_type_name[] = L"Windows.ApplicationModel.PackageStatus";

/* The package is the process: its architecture is the width of the component serving it, which is the width of the
 * guest the loader picked this build of the component for. */
#ifdef _WIN64
static const package_architecture package_architecture_value = ProcessorArchitecture_X64;
static const WCHAR package_architecture_text[] = L"x64";
#else
static const package_architecture package_architecture_value = ProcessorArchitecture_X86;
static const WCHAR package_architecture_text[] = L"x86";
#endif

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

static HRESULT create_string(const WCHAR *text, HSTRING *out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!text)
		text = L"";
	return WindowsCreateString(text, (UINT32)lstrlenW(text), out);
}

/* SHA-256, for the publisher identifier. The component is freestanding, so the digest is computed here rather than
 * through a cryptography interface. */
static UINT32 rotate_right(UINT32 value, UINT32 bits)
{
	return (value >> bits) | (value << (32 - bits));
}

static void sha256_block(UINT32 *state, const BYTE *block)
{
	static const UINT32 rounds[64] = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
	UINT32 schedule[64];
	UINT32 a, b, c, d, e, f, g, h;
	UINT32 i;
	for (i = 0; i < 16; ++i)
		schedule[i] = ((UINT32)block[i * 4] << 24) | ((UINT32)block[i * 4 + 1] << 16) |
			((UINT32)block[i * 4 + 2] << 8) | (UINT32)block[i * 4 + 3];
	for (i = 16; i < 64; ++i) {
		const UINT32 s0 = rotate_right(schedule[i - 15], 7) ^ rotate_right(schedule[i - 15], 18) ^ (schedule[i - 15] >> 3);
		const UINT32 s1 = rotate_right(schedule[i - 2], 17) ^ rotate_right(schedule[i - 2], 19) ^ (schedule[i - 2] >> 10);
		schedule[i] = schedule[i - 16] + s0 + schedule[i - 7] + s1;
	}
	a = state[0]; b = state[1]; c = state[2]; d = state[3];
	e = state[4]; f = state[5]; g = state[6]; h = state[7];
	for (i = 0; i < 64; ++i) {
		const UINT32 s1 = rotate_right(e, 6) ^ rotate_right(e, 11) ^ rotate_right(e, 25);
		const UINT32 ch = (e & f) ^ (~e & g);
		const UINT32 temp1 = h + s1 + ch + rounds[i] + schedule[i];
		const UINT32 s0 = rotate_right(a, 2) ^ rotate_right(a, 13) ^ rotate_right(a, 22);
		const UINT32 maj = (a & b) ^ (a & c) ^ (b & c);
		const UINT32 temp2 = s0 + maj;
		h = g; g = f; f = e; e = d + temp1;
		d = c; c = b; b = a; a = temp1 + temp2;
	}
	state[0] += a; state[1] += b; state[2] += c; state[3] += d;
	state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

static void sha256(const BYTE *data, UINT32 length, BYTE *digest)
{
	UINT32 state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
	BYTE tail[128];
	UINT32 tail_length = length % 64;
	UINT64 bit_length = (UINT64)length * 8;
	UINT32 i;
	while (length >= 64) {
		sha256_block(state, data);
		data += 64;
		length -= 64;
	}
	for (i = 0; i < length; ++i)
		tail[i] = data[i];
	tail[tail_length++] = 0x80;
	while (tail_length % 64 != 56)
		tail[tail_length++] = 0;
	for (i = 0; i < 8; ++i)
		tail[tail_length++] = (BYTE)(bit_length >> (56 - i * 8));
	for (i = 0; i < tail_length / 64; ++i)
		sha256_block(state, tail + i * 64);
	for (i = 0; i < 8; ++i) {
		digest[i * 4] = (BYTE)(state[i] >> 24);
		digest[i * 4 + 1] = (BYTE)(state[i] >> 16);
		digest[i * 4 + 2] = (BYTE)(state[i] >> 8);
		digest[i * 4 + 3] = (BYTE)state[i];
	}
}

/* The publisher identifier that ends family and full names: the first eight bytes of the SHA-256 of the publisher as
 * UTF-16 little endian, in the Crockford base 32 alphabet Windows writes lowercase, thirteen characters. */
static void publisher_identifier(const WCHAR *publisher, WCHAR *out /* fourteen including the terminator */)
{
	static const WCHAR alphabet[] = L"0123456789abcdefghjkmnpqrstvwxyz";
	BYTE digest[32];
	UINT64 bits = 0;
	UINT32 i;
	sha256((const BYTE *)publisher, (UINT32)lstrlenW(publisher) * (UINT32)sizeof(WCHAR), digest);
	for (i = 0; i < 8; ++i)
		bits = (bits << 8) | digest[i];
	for (i = 0; i < 12; ++i)
		out[i] = alphabet[(UINT32)((bits >> (59 - i * 5)) & 31)];
	/* the last character carries the remaining four bits, shifted up */
	out[12] = alphabet[(UINT32)((bits << 1) & 31)];
	out[13] = 0;
}

/* The manifest as this component reads it: the identity, the display properties, and the names of the dependencies'
 * elements. Everything is fixed width so the parsed result lives in the static package object; a value longer than its
 * field truncates, the scanner's tolerance for long values matching its tolerance for everything else. */
#define MANIFEST_TEXT_MAX 256
#define MANIFEST_DESCRIPTION_MAX 1024
#define MANIFEST_DEPENDENCY_MAX 8

struct package_manifest {
	WCHAR name[MANIFEST_TEXT_MAX];
	WCHAR publisher[MANIFEST_TEXT_MAX];
	WCHAR resource_id[MANIFEST_TEXT_MAX];
	WCHAR version_text[64];
	WCHAR display_name[MANIFEST_TEXT_MAX];
	WCHAR publisher_display[MANIFEST_TEXT_MAX];
	WCHAR description[MANIFEST_DESCRIPTION_MAX];
	WCHAR logo[MANIFEST_TEXT_MAX];
	WCHAR dependencies[MANIFEST_DEPENDENCY_MAX][64];
	UINT32 dependency_count;
	BOOL framework;
	BOOL has_identity; /* an Identity element was read; its Name attribute decides whether it identifies */
};

/* A decoded value collects into a UTF-8 staging buffer before it becomes UTF-16. */
struct utf8_value {
	BYTE text[MANIFEST_DESCRIPTION_MAX * 3];
	UINT32 length;
};

static void utf8_clear(struct utf8_value *value)
{
	value->length = 0;
}

static void utf8_code_point(struct utf8_value *value, UINT32 code)
{
	BYTE *out = value->text;
	if (value->length + 4 > sizeof value->text)
		return;
	if (code < 0x80) {
		out[value->length++] = (BYTE)code;
	} else if (code < 0x800) {
		out[value->length++] = (BYTE)(0xc0 | (code >> 6));
		out[value->length++] = (BYTE)(0x80 | (code & 0x3f));
	} else if (code < 0x10000) {
		out[value->length++] = (BYTE)(0xe0 | (code >> 12));
		out[value->length++] = (BYTE)(0x80 | ((code >> 6) & 0x3f));
		out[value->length++] = (BYTE)(0x80 | (code & 0x3f));
	} else if (code < 0x110000) {
		out[value->length++] = (BYTE)(0xf0 | (code >> 18));
		out[value->length++] = (BYTE)(0x80 | ((code >> 12) & 0x3f));
		out[value->length++] = (BYTE)(0x80 | ((code >> 6) & 0x3f));
		out[value->length++] = (BYTE)(0x80 | (code & 0x3f));
	}
}

static void utf8_text(struct utf8_value *value, const BYTE *text, UINT32 length)
{
	if (value->length + length > sizeof value->text)
		length = (UINT32)(sizeof value->text - value->length);
	memcpy(value->text + value->length, text, length);
	value->length += length;
}

/* Chardata and attribute values: the five named entities and the decimal and hexadecimal character references decode,
 * anything else passes through byte for byte. */
static void utf8_markup(struct utf8_value *value, const BYTE *data, UINT32 length)
{
	UINT32 i = 0;
	while (i < length) {
		if (data[i] != '&') {
			utf8_text(value, data + i, 1);
			++i;
		} else if (i + 5 <= length && !memcmp(data + i, "&amp;", 5)) {
			utf8_text(value, (const BYTE *)"&", 1);
			i += 5;
		} else if (i + 4 <= length && !memcmp(data + i, "&lt;", 4)) {
			utf8_text(value, (const BYTE *)"<", 1);
			i += 4;
		} else if (i + 4 <= length && !memcmp(data + i, "&gt;", 4)) {
			utf8_text(value, (const BYTE *)">", 1);
			i += 4;
		} else if (i + 6 <= length && !memcmp(data + i, "&quot;", 6)) {
			utf8_text(value, (const BYTE *)"\"", 1);
			i += 6;
		} else if (i + 6 <= length && !memcmp(data + i, "&apos;", 6)) {
			utf8_text(value, (const BYTE *)"'", 1);
			i += 6;
		} else if (i + 3 <= length && data[i + 1] == '#') {
			const BOOL hex = data[i + 2] == 'x' || data[i + 2] == 'X';
			UINT32 code = 0;
			UINT32 j = i + (hex ? 3 : 2);
			BOOL digits = FALSE;
			while (j < length && data[j] != ';') {
				UINT32 digit;
				if (data[j] >= '0' && data[j] <= '9')
					digit = (UINT32)(data[j] - '0');
				else if (hex && data[j] >= 'a' && data[j] <= 'f')
					digit = (UINT32)(data[j] - 'a' + 10);
				else if (hex && data[j] >= 'A' && data[j] <= 'F')
					digit = (UINT32)(data[j] - 'A' + 10);
				else
					break;
				code = code * (hex ? 16u : 10u) + digit;
				digits = TRUE;
				++j;
			}
			if (digits && j < length && data[j] == ';') {
				utf8_code_point(value, code);
				i = j + 1;
			} else {
				utf8_text(value, data + i, 1);
				++i;
			}
		} else {
			utf8_text(value, data + i, 1);
			++i;
		}
	}
}

/* The staging value becomes UTF-16 in the destination, trimmed of the whitespace around an element's text. */
static void store_utf8(WCHAR *destination, UINT32 capacity, const struct utf8_value *value)
{
	WCHAR wide[MANIFEST_DESCRIPTION_MAX];
	INT converted;
	destination[0] = 0;
	if (!value->length)
		return;
	converted = MultiByteToWideChar(CP_UTF8, 0, (const char *)value->text, (INT)value->length,
		wide, (INT)(sizeof wide / sizeof wide[0]));
	if (!converted)
		return;
	if (converted > (INT)capacity - 1)
		converted = (INT)capacity - 1;
	while (converted && (wide[0] == L' ' || wide[0] == L'\t' || wide[0] == L'\r' || wide[0] == L'\n')) {
		memmove(wide, wide + 1, (SIZE_T)(converted - 1) * sizeof(WCHAR));
		--converted;
	}
	while (converted && (wide[converted - 1] == L' ' || wide[converted - 1] == L'\t' || wide[converted - 1] == L'\r' ||
		wide[converted - 1] == L'\n'))
		--converted;
	memcpy(destination, wide, (SIZE_T)converted * sizeof(WCHAR));
	destination[converted] = 0;
}

static BOOL bytes_equal(const BYTE *data, UINT32 position, UINT32 count, const char *name)
{
	UINT32 i;
	for (i = 0; i < count; ++i) {
		if (!name[i] || data[position + i] != (BYTE)name[i])
			return FALSE;
	}
	return !name[count];
}

/* The stack keeps element names without a terminator, so the comparison carries the length. */
static BOOL stack_is(const BYTE *name, UINT32 length, const char *text)
{
	UINT32 i;
	for (i = 0; i < length; ++i) {
		if (!text[i] || name[i] != (BYTE)text[i])
			return FALSE;
	}
	return !text[length];
}

static void append_wide(WCHAR *target, UINT32 capacity, const WCHAR *text)
{
	UINT32 length = (UINT32)lstrlenW(target);
	while (*text && length < capacity - 1)
		target[length++] = *text++;
	target[length] = 0;
}

static void append_char(WCHAR *target, UINT32 capacity, WCHAR character)
{
	UINT32 length = (UINT32)lstrlenW(target);
	if (length < capacity - 1) {
		target[length] = character;
		target[length + 1] = 0;
	}
}

static void append_number(WCHAR *target, UINT32 capacity, UINT32 number)
{
	WCHAR digits[12];
	UINT32 count = 0;
	do {
		digits[count++] = (WCHAR)(L'0' + number % 10);
		number /= 10;
	} while (number);
	while (count)
		append_char(target, capacity, digits[--count]);
}

/* The version attribute: four dot-separated fields, each at most 65535, missing fields zero. A field that is not a
 * number, a value past 65535 or more than four fields leaves the whole version zero, the way a malformed identity
 * answers nothing rather than a made-up number. */
static void parse_version(const WCHAR *text, package_version *version)
{
	UINT32 values[4] = {0, 0, 0, 0};
	UINT32 field = 0;
	UINT32 index = 0;
	BOOL digits = FALSE, valid = TRUE;
	version->Major = version->Minor = version->Build = version->Revision = 0;
	for (; text[index]; ++index) {
		if (text[index] >= L'0' && text[index] <= L'9') {
			values[field] = values[field] * 10 + (UINT32)(text[index] - L'0');
			if (values[field] > 65535)
				valid = FALSE;
			digits = TRUE;
		} else if (text[index] == L'.') {
			if (field == 3 || !digits) {
				valid = FALSE;
				break;
			}
			++field;
			digits = FALSE;
		} else {
			valid = FALSE;
			break;
		}
	}
	if (!valid || !digits)
		return;
	version->Major = (UINT16)values[0];
	version->Minor = (UINT16)values[1];
	version->Build = (UINT16)values[2];
	version->Revision = (UINT16)values[3];
}

/* The documented identity construction: the family name is Name_PublisherId, the full name is
 * Name_Version_Architecture__PublisherId with the version as its four parts. Without a name and publisher there is
 * nothing to construct. */
static void build_identity_names(const struct package_manifest *manifest, const package_version *version,
	WCHAR *full_name, UINT32 full_capacity, WCHAR *family_name, UINT32 family_capacity)
{
	WCHAR publisher_id[14];
	full_name[0] = 0;
	family_name[0] = 0;
	if (!manifest->name[0] || !manifest->publisher[0])
		return;
	publisher_identifier(manifest->publisher, publisher_id);
	append_wide(family_name, family_capacity, manifest->name);
	append_char(family_name, family_capacity, L'_');
	append_wide(family_name, family_capacity, publisher_id);
	append_wide(full_name, full_capacity, manifest->name);
	append_char(full_name, full_capacity, L'_');
	append_number(full_name, full_capacity, version->Major);
	append_char(full_name, full_capacity, L'.');
	append_number(full_name, full_capacity, version->Minor);
	append_char(full_name, full_capacity, L'.');
	append_number(full_name, full_capacity, version->Build);
	append_char(full_name, full_capacity, L'.');
	append_number(full_name, full_capacity, version->Revision);
	append_char(full_name, full_capacity, L'_');
	append_wide(full_name, full_capacity, package_architecture_text);
	append_wide(full_name, full_capacity, L"__");
	append_wide(full_name, full_capacity, publisher_id);
}

/* The package root is where the running executable lives: GetModuleFileNameW of the running image answers the path and
 * the manifest is expected beside it. The build lays the per-architecture package directory out beside the fixture
 * executable (package-client64\AppxManifest.xml next to package-client64.exe), so that directory is the second
 * candidate; a real packaged layout answers at the first. */
static BOOL find_manifest(WCHAR *path /* MAX_PATH */)
{
	WCHAR directory[MAX_PATH];
	WCHAR candidate[MAX_PATH];
	const WCHAR *base;
	const UINT32 manifest_suffix = (UINT32)lstrlenW(L"\\AppxManifest.xml");
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
	if ((UINT32)lstrlenW(directory) + manifest_suffix + 1 < MAX_PATH) {
		lstrcpyW(candidate, directory);
		lstrcatW(candidate, L"\\AppxManifest.xml");
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
		if ((UINT32)lstrlenW(directory) + 1 + base_length + manifest_suffix + 1 < MAX_PATH) {
			UINT32 stem_length = (UINT32)lstrlenW(directory) + 1;
			lstrcpyW(candidate, directory);
			lstrcatW(candidate, L"\\");
			memcpy(candidate + stem_length, base, base_length * sizeof(WCHAR));
			candidate[stem_length + base_length] = 0;
			lstrcatW(candidate, L"\\AppxManifest.xml");
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
	if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 4 * 1024 * 1024) {
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

enum capture_kind {
	capture_none = 0,
	capture_display_name,
	capture_publisher_display,
	capture_description,
	capture_logo
};

/* The scanner walks tags and text with a tolerant hand: only the elements the identity needs are recognized, matched by
 * the local name the way an Appx manifest spells them, and everything else — comments, processing instructions, other
 * elements — is skipped or copied through. A manifest with sections this component does not model still parses. */
static void parse_manifest(const BYTE *data, UINT32 length, struct package_manifest *manifest)
{
	/* the element stack, only to know whose child the current element is */
	struct { BYTE name[64]; UINT32 length; } stack[16];
	UINT32 depth = 0;
	UINT32 position = 0;
	enum capture_kind capture = capture_none;
	struct utf8_value value;
	utf8_clear(&value);
	if (length >= 3 && data[0] == 0xef && data[1] == 0xbb && data[2] == 0xbf)
		position = 3; /* a UTF-8 byte order mark precedes the document */
	while (position < length) {
		if (data[position] != '<') {
			UINT32 start = position;
			while (position < length && data[position] != '<')
				++position;
			if (capture != capture_none)
				utf8_markup(&value, data + start, position - start);
			continue;
		}
		if (position + 4 <= length && !memcmp(data + position, "<!--", 4)) {
			position += 4;
			while (position + 3 <= length && memcmp(data + position, "-->", 3))
				++position;
			position = position + 3 <= length ? position + 3 : length;
			continue;
		}
		if (position + 9 <= length && !memcmp(data + position, "<![CDATA[", 9)) {
			position += 9;
			while (position + 3 <= length && memcmp(data + position, "]]>", 3)) {
				if (capture != capture_none)
					utf8_markup(&value, data + position, 1);
				++position;
			}
			position = position + 3 <= length ? position + 3 : length;
			continue;
		}
		if (position + 2 <= length && data[position + 1] == '?') {
			position += 2;
			while (position + 2 <= length && memcmp(data + position, "?>", 2))
				++position;
			position = position + 2 <= length ? position + 2 : length;
			continue;
		}
		if (position + 2 <= length && data[position + 1] == '!') {
			position += 2;
			while (position < length && data[position] != '>')
				++position;
			++position;
			continue;
		}
		if (position + 2 <= length && data[position + 1] == '/') {
			position += 2;
			while (position < length && data[position] != '>')
				++position;
			++position;
			if (capture != capture_none) {
				/* the first end tag under a capture element closes it; a display property holds text only */
				switch (capture) {
				case capture_display_name:
					store_utf8(manifest->display_name, MANIFEST_TEXT_MAX, &value);
					break;
				case capture_publisher_display:
					store_utf8(manifest->publisher_display, MANIFEST_TEXT_MAX, &value);
					break;
				case capture_description:
					store_utf8(manifest->description, MANIFEST_DESCRIPTION_MAX, &value);
					break;
				case capture_logo:
					store_utf8(manifest->logo, MANIFEST_TEXT_MAX, &value);
					break;
				default:
					break;
				}
				capture = capture_none;
			}
			if (depth)
				--depth;
			continue;
		}
		/* start tag: name, then attributes, then the push unless it closes itself */
		{
			const UINT32 name_start = position + 1;
			UINT32 name_length;
			BOOL self_closing = FALSE;
			BOOL is_identity = FALSE;
			BOOL in_dependencies = FALSE;
			enum capture_kind property = capture_none;
			position = name_start;
			while (position < length && data[position] != ' ' && data[position] != '\t' && data[position] != '\r' &&
				data[position] != '\n' && data[position] != '>' && data[position] != '/')
				++position;
			name_length = position - name_start;
			if (!name_length || position >= length)
				break;
			is_identity = bytes_equal(data, name_start, name_length, "Identity");
			in_dependencies = depth && stack_is(stack[depth - 1].name, stack[depth - 1].length, "Dependencies");
			if (is_identity)
				manifest->has_identity = TRUE;
			if (in_dependencies) {
				/* the dependency elements are recorded by name, for what reads the package graph later */
				if (manifest->dependency_count < MANIFEST_DEPENDENCY_MAX && name_length < 64) {
					memcpy(manifest->dependencies[manifest->dependency_count], data + name_start, name_length);
					manifest->dependencies[manifest->dependency_count][name_length] = 0;
					++manifest->dependency_count;
				}
			} else if (depth && stack_is(stack[depth - 1].name, stack[depth - 1].length, "Properties")) {
				if (bytes_equal(data, name_start, name_length, "DisplayName"))
					property = capture_display_name;
				else if (bytes_equal(data, name_start, name_length, "PublisherDisplayName"))
					property = capture_publisher_display;
				else if (bytes_equal(data, name_start, name_length, "Description"))
					property = capture_description;
				else if (bytes_equal(data, name_start, name_length, "Logo"))
					property = capture_logo;
				if (property != capture_none) {
					capture = property;
					utf8_clear(&value);
				}
			}
			/* attributes, dispatched only inside Identity */
			while (position < length && data[position] != '>' && data[position] != '/') {
				UINT32 attribute_name, attribute_length, value_start, value_length;
				BYTE quote;
				while (position < length && (data[position] == ' ' || data[position] == '\t' || data[position] == '\r' ||
					data[position] == '\n'))
					++position;
				if (position >= length || data[position] == '>' || data[position] == '/')
					break;
				attribute_name = position;
				while (position < length && data[position] != '=' && data[position] != ' ' && data[position] != '\t' &&
					data[position] != '\r' && data[position] != '\n' && data[position] != '>' && data[position] != '/')
					++position;
				attribute_length = position - attribute_name;
				while (position < length && (data[position] == ' ' || data[position] == '\t' || data[position] == '\r' ||
					data[position] == '\n'))
					++position;
				if (position >= length || data[position] != '=')
					continue;
				++position;
				while (position < length && (data[position] == ' ' || data[position] == '\t' || data[position] == '\r' ||
					data[position] == '\n'))
					++position;
				if (position >= length || (data[position] != '"' && data[position] != '\''))
					continue;
				quote = data[position++];
				value_start = position;
				while (position < length && data[position] != quote)
					++position;
				if (position >= length)
					break;
				value_length = position - value_start;
				++position;
				if (is_identity) {
					if (bytes_equal(data, attribute_name, attribute_length, "Name")) {
						utf8_clear(&value);
						utf8_markup(&value, data + value_start, value_length);
						store_utf8(manifest->name, MANIFEST_TEXT_MAX, &value);
					} else if (bytes_equal(data, attribute_name, attribute_length, "Version")) {
						utf8_clear(&value);
						utf8_markup(&value, data + value_start, value_length);
						store_utf8(manifest->version_text,
							(UINT32)(sizeof manifest->version_text / sizeof manifest->version_text[0]), &value);
					} else if (bytes_equal(data, attribute_name, attribute_length, "Publisher")) {
						utf8_clear(&value);
						utf8_markup(&value, data + value_start, value_length);
						store_utf8(manifest->publisher, MANIFEST_TEXT_MAX, &value);
					} else if (bytes_equal(data, attribute_name, attribute_length, "ResourceId")) {
						utf8_clear(&value);
						utf8_markup(&value, data + value_start, value_length);
						store_utf8(manifest->resource_id, MANIFEST_TEXT_MAX, &value);
					} else if (bytes_equal(data, attribute_name, attribute_length, "Framework")) {
						utf8_clear(&value);
						utf8_markup(&value, data + value_start, value_length);
						manifest->framework = manifest->framework ||
							(value.length == 4 && !memcmp(value.text, "true", 4)) ||
							(value.length == 1 && value.text[0] == '1');
					}
				}
			}
			if (position < length && data[position] == '/') {
				self_closing = TRUE;
				++position;
			}
			if (position >= length || data[position] != '>')
				break;
			++position;
			if (self_closing)
				continue;
			if (depth < sizeof stack / sizeof stack[0]) {
				stack[depth].length = name_length < sizeof stack[depth].name ? name_length : sizeof stack[depth].name;
				memcpy(stack[depth].name, data + name_start, stack[depth].length);
				++depth;
			}
		}
	}
}

/* The package object: the process's one package, with the interfaces the mingw header spells for the class as parts.
 * Each part forwards its lifetime to the object, the way the other classes' part structs do. */
struct package_id_part {
	const __x_ABI_CWindows_CApplicationModel_CIPackageIdVtbl *lpVtbl;
	struct package_object *owner;
};

struct package2_part {
	const __x_ABI_CWindows_CApplicationModel_CIPackage2Vtbl *lpVtbl;
	struct package_object *owner;
};

struct package3_part {
	const __x_ABI_CWindows_CApplicationModel_CIPackage3Vtbl *lpVtbl;
	struct package_object *owner;
};

struct package_object {
	const __x_ABI_CWindows_CApplicationModel_CIPackageVtbl *lpVtbl;
	LONG refs;
	struct package_id_part id;
	struct package2_part second;
	struct package3_part third;
	/* the identity, read once from the manifest beside the running image */
	struct package_manifest manifest;
	package_version version;
	const package_architecture *architecture;
	WCHAR full_name[MANIFEST_TEXT_MAX + 48];
	WCHAR family_name[MANIFEST_TEXT_MAX + 16];
};

static HRESULT WINAPI package_query(package_i *iface, REFIID iid, void **out);
static ULONG WINAPI package_addref(package_i *iface);
static ULONG WINAPI package_release(package_i *iface);
static HRESULT WINAPI package_iids(package_i *iface, ULONG *count, IID **iids);
static HRESULT WINAPI package_class_name(package_i *iface, HSTRING *name);
static HRESULT WINAPI package_trust(package_i *iface, TrustLevel *level);

/* The parts forward their lifetime to the object, whose base their owner pointer reaches. */
#define PACKAGE_PART_LIFETIME(part, iface_type) \
	static HRESULT WINAPI part##_query(iface_type *iface, REFIID iid, void **out) \
	{ return package_query((package_i *)((struct part##_part *)iface)->owner, iid, out); } \
	static ULONG WINAPI part##_addref(iface_type *iface) \
	{ return package_addref((package_i *)((struct part##_part *)iface)->owner); } \
	static ULONG WINAPI part##_release(iface_type *iface) \
	{ return package_release((package_i *)((struct part##_part *)iface)->owner); } \
	static HRESULT WINAPI part##_iids(iface_type *iface, ULONG *count, IID **iids) \
	{ (void)iface; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI part##_trust(iface_type *iface, TrustLevel *level) \
	{ (void)iface; return inspectable_trust(level); }

PACKAGE_PART_LIFETIME(package_id, package_id_i)
PACKAGE_PART_LIFETIME(package2, package2_i)
PACKAGE_PART_LIFETIME(package3, package3_i)

/* The parts answer with the object's identity; the Id part is a runtime class of its own and answers with its name. */
static HRESULT WINAPI package_id_class_name(package_id_i *iface, HSTRING *name)
{
	(void)iface;
	return create_string(id_type_name, name);
}

static HRESULT WINAPI package2_class_name(package2_i *iface, HSTRING *name)
{
	(void)iface;
	return create_string(class_name, name);
}

static HRESULT WINAPI package3_class_name(package3_i *iface, HSTRING *name)
{
	(void)iface;
	return create_string(class_name, name);
}

static HRESULT WINAPI package_get_id(package_i *iface, package_id_i **value);
static HRESULT WINAPI package_get_installed_location(package_i *iface, __x_ABI_CWindows_CStorage_CIStorageFolder **value);
static HRESULT WINAPI package_get_is_framework(package_i *iface, boolean *value);
static HRESULT WINAPI package_get_dependencies(package_i *iface, __FIVectorView_1_Windows__CApplicationModel__CPackage **value);
static HRESULT WINAPI package2_get_display_name(package2_i *iface, HSTRING *value);
static HRESULT WINAPI package2_get_publisher_display_name(package2_i *iface, HSTRING *value);
static HRESULT WINAPI package2_get_description(package2_i *iface, HSTRING *value);
static HRESULT WINAPI package2_get_logo(package2_i *iface, __x_ABI_CWindows_CFoundation_CIUriRuntimeClass **value);
static HRESULT WINAPI package2_get_is_resource_package(package2_i *iface, boolean *value);
static HRESULT WINAPI package2_get_is_bundle(package2_i *iface, boolean *value);
static HRESULT WINAPI package2_get_is_development_mode(package2_i *iface, boolean *value);
static HRESULT WINAPI package3_get_status(package3_i *iface, package_status_i **value);
static HRESULT WINAPI package3_get_installed_date(package3_i *iface, __x_ABI_CWindows_CFoundation_CDateTime *value);
static HRESULT WINAPI package3_get_app_list_entries_async(package3_i *iface,
	__FIAsyncOperation_1___FIVectorView_1_Windows__CApplicationModel__CCore__CAppListEntry **operation);
static HRESULT WINAPI package_id_get_name(package_id_i *iface, HSTRING *value);
static HRESULT WINAPI package_id_get_version(package_id_i *iface, package_version *value);
static HRESULT WINAPI package_id_get_architecture(package_id_i *iface, package_architecture *value);
static HRESULT WINAPI package_id_get_resource_id(package_id_i *iface, HSTRING *value);
static HRESULT WINAPI package_id_get_publisher(package_id_i *iface, HSTRING *value);
static HRESULT WINAPI package_id_get_publisher_id(package_id_i *iface, HSTRING *value);
static HRESULT WINAPI package_id_get_full_name(package_id_i *iface, HSTRING *value);
static HRESULT WINAPI package_id_get_family_name(package_id_i *iface, HSTRING *value);

static const __x_ABI_CWindows_CApplicationModel_CIPackageVtbl package_table = {
	.QueryInterface = package_query,
	.AddRef = package_addref,
	.Release = package_release,
	.GetIids = package_iids,
	.GetRuntimeClassName = package_class_name,
	.GetTrustLevel = package_trust,
	.get_Id = package_get_id,
	.get_InstalledLocation = package_get_installed_location,
	.get_IsFramework = package_get_is_framework,
	.get_Dependencies = package_get_dependencies,
};

static const __x_ABI_CWindows_CApplicationModel_CIPackage2Vtbl package2_table = {
	.QueryInterface = package2_query,
	.AddRef = package2_addref,
	.Release = package2_release,
	.GetIids = package2_iids,
	.GetRuntimeClassName = package2_class_name,
	.GetTrustLevel = package2_trust,
	.get_DisplayName = package2_get_display_name,
	.get_PublisherDisplayName = package2_get_publisher_display_name,
	.get_Description = package2_get_description,
	.get_Logo = package2_get_logo,
	.get_IsResourcePackage = package2_get_is_resource_package,
	.get_IsBundle = package2_get_is_bundle,
	.get_IsDevelopmentMode = package2_get_is_development_mode,
};

static const __x_ABI_CWindows_CApplicationModel_CIPackage3Vtbl package3_table = {
	.QueryInterface = package3_query,
	.AddRef = package3_addref,
	.Release = package3_release,
	.GetIids = package3_iids,
	.GetRuntimeClassName = package3_class_name,
	.GetTrustLevel = package3_trust,
	.get_Status = package3_get_status,
	.get_InstalledDate = package3_get_installed_date,
	.GetAppListEntriesAsync = package3_get_app_list_entries_async,
};

static const __x_ABI_CWindows_CApplicationModel_CIPackageIdVtbl package_id_table = {
	.QueryInterface = package_id_query,
	.AddRef = package_id_addref,
	.Release = package_id_release,
	.GetIids = package_id_iids,
	.GetRuntimeClassName = package_id_class_name,
	.GetTrustLevel = package_id_trust,
	.get_Name = package_id_get_name,
	.get_Version = package_id_get_version,
	.get_Architecture = package_id_get_architecture,
	.get_ResourceId = package_id_get_resource_id,
	.get_Publisher = package_id_get_publisher,
	.get_PublisherId = package_id_get_publisher_id,
	.get_FullName = package_id_get_full_name,
	.get_FamilyName = package_id_get_family_name,
};

static struct package_object package_instance = {
	&package_table,
	0, /* the singleton is never freed; the count follows the references the guests hold */
	{&package_id_table, &package_instance},
	{&package2_table, &package_instance},
	{&package3_table, &package_instance},
	{{0}},
	{0, 0, 0, 0},
	&package_architecture_value,
	L"",
	L"",
};

static HRESULT WINAPI package_query(package_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) ||
		equal_iid(iid, &IID___x_ABI_CWindows_CApplicationModel_CIPackage))
		*out = iface;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CApplicationModel_CIPackage2))
		*out = &((struct package_object *)iface)->second;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CApplicationModel_CIPackage3))
		*out = &((struct package_object *)iface)->third;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CApplicationModel_CIPackageId))
		*out = &((struct package_object *)iface)->id;
	else
		return E_NOINTERFACE;
	package_addref(iface);
	return S_OK;
}

/* The package lives as long as the process: the count follows the guests' references and the object is never freed. */
static ULONG WINAPI package_addref(package_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct package_object *)iface)->refs);
}

static ULONG WINAPI package_release(package_i *iface)
{
	return (ULONG)InterlockedDecrement(&((struct package_object *)iface)->refs);
}

static HRESULT WINAPI package_iids(package_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI package_class_name(package_i *iface, HSTRING *name)
{
	(void)iface;
	return create_string(class_name, name);
}

static HRESULT WINAPI package_trust(package_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI package_get_id(package_i *iface, package_id_i **value)
{
	struct package_object *self = (struct package_object *)iface;
	if (!value)
		return E_POINTER;
	*value = (package_id_i *)&self->id;
	package_addref(iface);
	return S_OK;
}

/* The folder object the real Windows answers with is not built; no observed startup path has asked for it yet. */
static HRESULT WINAPI package_get_installed_location(package_i *iface, __x_ABI_CWindows_CStorage_CIStorageFolder **value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI package_get_is_framework(package_i *iface, boolean *value)
{
	struct package_object *self = (struct package_object *)iface;
	if (!value)
		return E_POINTER;
	*value = (boolean)(self->manifest.framework ? 1 : 0);
	return S_OK;
}

/* The vector view of the package graph is not built; the manifest's dependency elements are recorded for it. */
static HRESULT WINAPI package_get_dependencies(package_i *iface, __FIVectorView_1_Windows__CApplicationModel__CPackage **value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI package2_get_display_name(package2_i *iface, HSTRING *value)
{
	struct package_object *self = ((struct package2_part *)iface)->owner;
	return create_string(self->manifest.display_name, value);
}

static HRESULT WINAPI package2_get_publisher_display_name(package2_i *iface, HSTRING *value)
{
	struct package_object *self = ((struct package2_part *)iface)->owner;
	return create_string(self->manifest.publisher_display, value);
}

static HRESULT WINAPI package2_get_description(package2_i *iface, HSTRING *value)
{
	struct package_object *self = ((struct package2_part *)iface)->owner;
	return create_string(self->manifest.description, value);
}

/* The Uri object the real Windows answers with is not built; the manifest's Logo element is read and kept. */
static HRESULT WINAPI package2_get_logo(package2_i *iface, __x_ABI_CWindows_CFoundation_CIUriRuntimeClass **value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI package2_get_is_resource_package(package2_i *iface, boolean *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = 0;
	return S_OK;
}

static HRESULT WINAPI package2_get_is_bundle(package2_i *iface, boolean *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = 0;
	return S_OK;
}

static HRESULT WINAPI package2_get_is_development_mode(package2_i *iface, boolean *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = 0;
	return S_OK;
}

/* The status object: a package read from a manifest beside its image is present and healthy, so every condition is
 * clear and VerifyIsOK answers true. */
struct package_status_object {
	const __x_ABI_CWindows_CApplicationModel_CIPackageStatusVtbl *lpVtbl;
	LONG refs;
};

static HRESULT WINAPI status_query(package_status_i *iface, REFIID iid, void **out);
static ULONG WINAPI status_addref(package_status_i *iface);
static ULONG WINAPI status_release(package_status_i *iface);
static HRESULT WINAPI status_iids(package_status_i *iface, ULONG *count, IID **iids);
static HRESULT WINAPI status_class_name_impl(package_status_i *iface, HSTRING *name);
static HRESULT WINAPI status_trust(package_status_i *iface, TrustLevel *level);
static HRESULT WINAPI status_verify_is_ok(package_status_i *iface, boolean *value);
static HRESULT WINAPI status_boolean_false(package_status_i *iface, boolean *value);

static const __x_ABI_CWindows_CApplicationModel_CIPackageStatusVtbl status_table = {
	.QueryInterface = status_query,
	.AddRef = status_addref,
	.Release = status_release,
	.GetIids = status_iids,
	.GetRuntimeClassName = status_class_name_impl,
	.GetTrustLevel = status_trust,
	.VerifyIsOK = status_verify_is_ok,
	.get_NotAvailable = status_boolean_false,
	.get_PackageOffline = status_boolean_false,
	.get_DataOffline = status_boolean_false,
	.get_Disabled = status_boolean_false,
	.get_NeedsRemediation = status_boolean_false,
	.get_LicenseIssue = status_boolean_false,
	.get_Modified = status_boolean_false,
	.get_Tampered = status_boolean_false,
	.get_DependencyIssue = status_boolean_false,
	.get_Servicing = status_boolean_false,
	.get_DeploymentInProgress = status_boolean_false,
};

static HRESULT WINAPI status_query(package_status_i *iface, REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (!equal_iid(iid, &IID_IUnknown) && !equal_iid(iid, &IID_IInspectable) &&
		!equal_iid(iid, &IID___x_ABI_CWindows_CApplicationModel_CIPackageStatus))
		return E_NOINTERFACE;
	status_addref(iface);
	*out = iface;
	return S_OK;
}

static ULONG WINAPI status_addref(package_status_i *iface)
{
	return (ULONG)InterlockedIncrement(&((struct package_status_object *)iface)->refs);
}

static ULONG WINAPI status_release(package_status_i *iface)
{
	struct package_status_object *self = (struct package_status_object *)iface;
	const LONG refs = InterlockedDecrement(&self->refs);
	if (!refs) {
		FREE(self);
		InterlockedDecrement(&raven_xaml_objects);
	}
	return (ULONG)refs;
}

static HRESULT WINAPI status_iids(package_status_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI status_class_name_impl(package_status_i *iface, HSTRING *name)
{
	(void)iface;
	return create_string(status_type_name, name);
}

static HRESULT WINAPI status_trust(package_status_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI status_verify_is_ok(package_status_i *iface, boolean *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = 1;
	return S_OK;
}

static HRESULT WINAPI status_boolean_false(package_status_i *iface, boolean *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = 0;
	return S_OK;
}

static HRESULT WINAPI package3_get_status(package3_i *iface, package_status_i **value)
{
	struct package_status_object *status;
	(void)iface;
	if (!value)
		return E_POINTER;
	*value = NULL;
	if (!(status = ALLOC(sizeof *status)))
		return E_OUTOFMEMORY;
	status->lpVtbl = &status_table;
	status->refs = 1;
	InterlockedIncrement(&raven_xaml_objects);
	*value = (package_status_i *)status;
	return S_OK;
}

static HRESULT WINAPI package3_get_installed_date(package3_i *iface, __x_ABI_CWindows_CFoundation_CDateTime *value)
{
	(void)iface;
	if (!value)
		return E_POINTER;
	ZeroMemory(value, sizeof *value);
	return E_NOTIMPL;
}

static HRESULT WINAPI package3_get_app_list_entries_async(package3_i *iface,
	__FIAsyncOperation_1___FIVectorView_1_Windows__CApplicationModel__CCore__CAppListEntry **operation)
{
	(void)iface;
	if (!operation)
		return E_POINTER;
	*operation = NULL;
	return E_NOTIMPL;
}

static HRESULT WINAPI package_id_get_name(package_id_i *iface, HSTRING *value)
{
	struct package_object *self = ((struct package_id_part *)iface)->owner;
	return create_string(self->manifest.name, value);
}

static HRESULT WINAPI package_id_get_version(package_id_i *iface, package_version *value)
{
	struct package_object *self = ((struct package_id_part *)iface)->owner;
	if (!value)
		return E_POINTER;
	*value = self->version;
	return S_OK;
}

static HRESULT WINAPI package_id_get_architecture(package_id_i *iface, package_architecture *value)
{
	struct package_object *self = ((struct package_id_part *)iface)->owner;
	if (!value)
		return E_POINTER;
	*value = *self->architecture;
	return S_OK;
}

static HRESULT WINAPI package_id_get_resource_id(package_id_i *iface, HSTRING *value)
{
	struct package_object *self = ((struct package_id_part *)iface)->owner;
	return create_string(self->manifest.resource_id, value);
}

static HRESULT WINAPI package_id_get_publisher(package_id_i *iface, HSTRING *value)
{
	struct package_object *self = ((struct package_id_part *)iface)->owner;
	return create_string(self->manifest.publisher, value);
}

/* The publisher identifier, the thirteen character base 32 of the publisher hash, as the family and full names carry it. */
static HRESULT WINAPI package_id_get_publisher_id(package_id_i *iface, HSTRING *value)
{
	struct package_object *self = ((struct package_id_part *)iface)->owner;
	WCHAR publisher_id[14];
	if (!value)
		return E_POINTER;
	publisher_identifier(self->manifest.publisher, publisher_id);
	return create_string(publisher_id, value);
}

static HRESULT WINAPI package_id_get_full_name(package_id_i *iface, HSTRING *value)
{
	struct package_object *self = ((struct package_id_part *)iface)->owner;
	return create_string(self->full_name, value);
}

static HRESULT WINAPI package_id_get_family_name(package_id_i *iface, HSTRING *value)
{
	struct package_object *self = ((struct package_id_part *)iface)->owner;
	return create_string(self->family_name, value);
}

/* The manifest is read once per process: the first asker parses it, later askers wait for the result. */
static void ensure_manifest(void)
{
	static LONG state;
	if (InterlockedCompareExchange(&state, 1, 0) == 0) {
		struct package_object *self = &package_instance;
		WCHAR path[MAX_PATH];
		BYTE *data = NULL;
		UINT32 length = 0;
		if (find_manifest(path) && read_file(path, &data, &length)) {
			parse_manifest(data, length, &self->manifest);
			FREE(data);
		}
		if (!self->manifest.has_identity || !self->manifest.name[0]) {
			/* a process without package identity: the manifest is missing or carries no usable Identity, and the
			 * package answers with an empty Id */
			memset(&self->manifest, 0, sizeof self->manifest);
		}
		parse_version(self->manifest.version_text, &self->version);
		build_identity_names(&self->manifest, &self->version, self->full_name,
			(UINT32)(sizeof self->full_name / sizeof self->full_name[0]), self->family_name,
			(UINT32)(sizeof self->family_name / sizeof self->family_name[0]));
		InterlockedExchange(&state, 2);
	} else {
		while (InterlockedCompareExchange(&state, 2, 2) != 2)
			Sleep(0);
	}
}

/* The class factory and its statics */

struct factory_activation { const IActivationFactoryVtbl *lpVtbl; struct package_factory *owner; };
struct factory_statics { const __x_ABI_CWindows_CApplicationModel_CIPackageStaticsVtbl *lpVtbl; struct package_factory *owner; };

struct package_factory {
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

/* The statics part forwards its lifetime to the factory, whose activation part answers for both. */
static HRESULT WINAPI statics_query(package_statics_i *iface, REFIID iid, void **out)
{
	return factory_query((IActivationFactory *)((struct factory_statics *)iface)->owner, iid, out);
}

static ULONG WINAPI statics_addref(package_statics_i *iface)
{
	return factory_addref((IActivationFactory *)((struct factory_statics *)iface)->owner);
}

static ULONG WINAPI statics_release(package_statics_i *iface)
{
	return factory_release((IActivationFactory *)((struct factory_statics *)iface)->owner);
}

static HRESULT WINAPI statics_iids(package_statics_i *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}

static HRESULT WINAPI statics_class_name(package_statics_i *iface, HSTRING *name)
{
	(void)iface;
	return create_string(class_name, name);
}

static HRESULT WINAPI statics_trust(package_statics_i *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static HRESULT WINAPI statics_get_current(package_statics_i *iface, package_i **out);

static const IActivationFactoryVtbl factory_activation_table = {
	.QueryInterface = factory_query,
	.AddRef = factory_addref,
	.Release = factory_release,
	.GetIids = factory_iids,
	.GetRuntimeClassName = factory_class_name,
	.GetTrustLevel = factory_trust,
	.ActivateInstance = factory_activate,
};

static const __x_ABI_CWindows_CApplicationModel_CIPackageStaticsVtbl factory_statics_table = {
	.QueryInterface = statics_query,
	.AddRef = statics_addref,
	.Release = statics_release,
	.GetIids = statics_iids,
	.GetRuntimeClassName = statics_class_name,
	.GetTrustLevel = statics_trust,
	.get_Current = statics_get_current,
};

static struct package_factory factory_instance = {
	{&factory_activation_table, &factory_instance},
	{&factory_statics_table, &factory_instance},
	1,
};

static HRESULT WINAPI factory_query(IActivationFactory *iface, REFIID iid, void **out)
{
	struct package_factory *self = ((struct factory_activation *)iface)->owner;
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (equal_iid(iid, &IID_IUnknown) || equal_iid(iid, &IID_IInspectable) || equal_iid(iid, &IID_IActivationFactory))
		*out = &self->activation;
	else if (equal_iid(iid, &IID___x_ABI_CWindows_CApplicationModel_CIPackageStatics))
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
	return create_string(class_name, name);
}

static HRESULT WINAPI factory_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

/* The class has no public constructor: the package of a process is asked for, never constructed by name. */
static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	return E_ILLEGAL_METHOD_CALL;
}

static HRESULT WINAPI statics_get_current(package_statics_i *iface, package_i **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	ensure_manifest();
	package_addref((package_i *)&package_instance);
	*out = (package_i *)&package_instance;
	return S_OK;
}

HRESULT raven_package_factory(HSTRING name, IActivationFactory **out)
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
