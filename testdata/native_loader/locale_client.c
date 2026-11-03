#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Locale information, whether the runtime or the program answers it: lengths, texts, numbers and the failures. */

static int same(const WCHAR *left, const WCHAR *right)
{
	while (*left && *left == *right)
		++left, ++right;
	return *left == *right;
}

static unsigned enumerated;
static BOOL CALLBACK locale_found(LPWSTR name, DWORD flags, LPARAM parameter)
{
	if (!same(name, L"en-US") || flags != (LOCALE_WINDOWS | LOCALE_SPECIFICDATA) || parameter != (LPARAM)&enumerated)
		enumerated = 100;
	++enumerated;
	return FALSE;
}

int raven_entry(void)
{
	WCHAR text[64];
	for (int round = 0; round < 3; ++round) {
		if (GetLocaleInfoEx(NULL, LOCALE_SDECIMAL, NULL, 0) != 2 || GetLocaleInfoEx(L"en-US", LOCALE_SDECIMAL, text, 64) != 2 || !same(text, L"."))
			return 1;
		if (GetLocaleInfoEx(NULL, LOCALE_SMONTHNAME1, text, 64) != 8 || !same(text, L"January"))
			return 2;
		if (GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, text, 64) != 9 || !same(text, L"M/d/yyyy"))
			return 3;
		if (GetLocaleInfoEx(NULL, LOCALE_STIMEFORMAT, text, 64) != 11 || !same(text, L"h:mm:ss tt"))
			return 4;
		DWORD number = 0;
		if (GetLocaleInfoEx(NULL, LOCALE_IFIRSTDAYOFWEEK | LOCALE_RETURN_NUMBER, (LPWSTR)&number, 2) != 2 || number != 6)
			return 5;
		if (GetLocaleInfoEx(NULL, LOCALE_SGROUPING | LOCALE_NOUSEROVERRIDE, text, 64) != 4 || !same(text, L"3;0"))
			return 6;
		SetLastError(0);
		if (GetLocaleInfoEx(NULL, LOCALE_SMONTHNAME1, text, 3) || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			return 7;
		SetLastError(0);
		if (GetLocaleInfoEx(NULL, 0x7f, text, 64) || GetLastError() != ERROR_INVALID_PARAMETER)
			return 8;
		/* The text ends at its terminator and leaves the rest of the buffer alone. */
		text[5] = 0x1234;
		if (GetLocaleInfoEx(NULL, LOCALE_SCURRENCY, text, 64) != 2 || !same(text, L"$") || text[5] != 0x1234)
			return 9;
	}
	if (!EnumSystemLocalesEx(locale_found, LOCALE_ALL, (LPARAM)&enumerated, NULL) || enumerated != 1)
		return 10;
	if (!EnumSystemLocalesEx(locale_found, LOCALE_WINDOWS | LOCALE_SPECIFICDATA, (LPARAM)&enumerated, NULL) || enumerated != 2)
		return 11;
	if (!EnumSystemLocalesEx(locale_found, LOCALE_SUPPLEMENTAL, (LPARAM)&enumerated, NULL) || enumerated != 2)
		return 12;
	if (!EnumSystemLocalesEx(locale_found, LOCALE_NEUTRALDATA, (LPARAM)&enumerated, NULL) || enumerated != 2)
		return 13;
	SetLastError(0);
	if (EnumSystemLocalesEx(NULL, 0, 0, NULL) || GetLastError() != ERROR_INVALID_PARAMETER)
		return 14;
	SetLastError(0);
	if (EnumSystemLocalesEx(locale_found, 0, 0, &enumerated) || GetLastError() != ERROR_INVALID_PARAMETER)
		return 15;
	SetLastError(0);
	if (EnumSystemLocalesEx(locale_found, 0x80000000, 0, NULL) || GetLastError() != ERROR_INVALID_FLAGS || enumerated != 2)
		return 16;
	if (!IsValidLocaleName(L"en-US") || !IsValidLocaleName(L"EN-us") || IsValidLocaleName(L"invalid-locale") || IsValidLocaleName(L"en-US-extra"))
		return 17;
	/* Names convert to identifiers and back; the culture-neutral "x-none" is no locale. */
	WCHAR name[LOCALE_NAME_MAX_LENGTH];
	if (LocaleNameToLCID(L"it-IT", 0) != 0x410 || LocaleNameToLCID(L"EN-us", 0) != 0x409 || LocaleNameToLCID(L"", 0) != 0x7f) return 18;
	SetLastError(0);
	if (LocaleNameToLCID(L"x-none", 0) || GetLastError() != ERROR_INVALID_PARAMETER) return 19;
	if (LCIDToLocaleName(0x407, name, LOCALE_NAME_MAX_LENGTH, 0) != 6 || name[0] != 'd' || name[3] != 'D') return 20;
	if (LCIDToLocaleName(0x1234, name, LOCALE_NAME_MAX_LENGTH, 0)) return 21;
	/* The neutral culture "en" keeps what identifies it: its own canonical name, the invariant culture as its parent, its own language
	 * identifier, and its neutrality, with the number flag near-variant of the last answering as a number. */
	DWORD number = 0, neutral_number = 0;
	if (GetLocaleInfoEx(L"en", LOCALE_SNAME, NULL, 0) != 3) return 22;
	if (GetLocaleInfoEx(L"en", LOCALE_SNAME, text, 64) != 3 || !same(text, L"en")) return 23;
	if (GetLocaleInfoEx(L"en", LOCALE_SPARENT, text, 64) != 1 || text[0]) return 24;
	if (GetLocaleInfoEx(L"en", LOCALE_ILANGUAGE | LOCALE_RETURN_NUMBER, (LPWSTR)&number, 2) != 2 || number != 9) return 25;
	if (GetLocaleInfoEx(L"en", LOCALE_ILANGUAGE, text, 64) != 5 || !same(text, L"0009")) return 26;
	if (GetLocaleInfoEx(L"en", LOCALE_INEUTRAL | LOCALE_RETURN_NUMBER, (LPWSTR)&number, 2) != 2 || number != 1) return 27;
	if (GetLocaleInfoEx(L"en-US", LOCALE_INEUTRAL | LOCALE_RETURN_NUMBER, (LPWSTR)&number, 2) != 2 || number != 0) return 28;
	if (GetLocaleInfoEx(L"en", LOCALE_INEGNUMBER | LOCALE_RETURN_NUMBER, (LPWSTR)&neutral_number, 2) != 2 || neutral_number != 1) return 29;
	/* Its other data is its specific form's, as Windows serves the country and abbreviations of the default specific culture. */
	if (GetLocaleInfoEx(L"en", LOCALE_SDECIMAL, text, 64) != 2 || !same(text, L".")) return 30;
	if (GetLocaleInfoEx(L"en", LOCALE_SMONTHNAME1, text, 64) != 8 || !same(text, L"January")) return 31;
	/* The canonical name comes back whatever letter case asked for it. */
	if (GetLocaleInfoEx(L"EN-us", LOCALE_SNAME, text, 64) != 6 || !same(text, L"en-US")) return 32;
	/* The invariant culture answers its own empty name and parent and nothing else. */
	if (GetLocaleInfoEx(L"", LOCALE_SNAME, text, 64) != 1 || text[0]) return 33;
	if (GetLocaleInfoEx(L"", LOCALE_SPARENT, text, 64) != 1 || text[0]) return 34;
	SetLastError(0);
	if (GetLocaleInfoEx(L"", LOCALE_SSORTNAME, text, 64) || GetLastError() != ERROR_INVALID_PARAMETER) return 35;
	/* Names the tables do not know are no locale, for the neutral form as for the specific one. */
	SetLastError(0);
	if (GetLocaleInfoEx(L"zz-ZZ", LOCALE_SNAME, text, 64) || GetLastError() != ERROR_INVALID_PARAMETER) return 36;
	SetLastError(0);
	if (GetLocaleInfoEx(L"zz", LOCALE_SNAME, text, 64) || GetLastError() != ERROR_INVALID_PARAMETER) return 37;
	/* And every name the tables know is a locale, the empty one the invariant. */
	if (!IsValidLocaleName(L"en") || !IsValidLocaleName(L"it-IT") || !IsValidLocaleName(L"zh-Hans") || !IsValidLocaleName(L""))
		return 38;
	if (IsValidLocaleName(L"zzz-ZZZ")) return 39;
	/* A name resolves to the specific locale it stands for: itself, the language's default specific culture for a neutral one, the
	 * user's default when the name is missing, and its own empty name for the invariant. */
	if (ResolveLocaleName(L"en", name, LOCALE_NAME_MAX_LENGTH) != 6 || !same(name, L"en-US")) return 40;
	if (ResolveLocaleName(L"EN", name, LOCALE_NAME_MAX_LENGTH) != 6 || !same(name, L"en-US")) return 41;
	if (ResolveLocaleName(L"en-US", name, LOCALE_NAME_MAX_LENGTH) != 6 || !same(name, L"en-US")) return 42;
	if (ResolveLocaleName(L"zh-Hans", name, LOCALE_NAME_MAX_LENGTH) != 6 || !same(name, L"zh-CN")) return 43;
	if (ResolveLocaleName(NULL, name, LOCALE_NAME_MAX_LENGTH) != 6 || !same(name, L"en-US")) return 44;
	if (ResolveLocaleName(L"", name, LOCALE_NAME_MAX_LENGTH) != 1 || name[0]) return 45;
	SetLastError(0);
	if (ResolveLocaleName(L"zz", name, LOCALE_NAME_MAX_LENGTH) || GetLastError() != ERROR_INVALID_PARAMETER) return 46;
	SetLastError(0);
	if (ResolveLocaleName(L"en", name, 3) || GetLastError() != ERROR_INSUFFICIENT_BUFFER) return 47;
	/* Native digits are a ten-character sequence, not the number-substitution mode. The null-buffer length and short-buffer
	 * checks reach the host handler; the full buffers exercise the guest-side fast path for both locale APIs. */
	if (GetLocaleInfoEx(L"en-US", LOCALE_SNATIVEDIGITS, NULL, 0) != 11) return 48;
	if (GetLocaleInfoEx(L"en-US", LOCALE_SNATIVEDIGITS, text, 64) != 11 || !same(text, L"0123456789")) return 49;
	if (GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SNATIVEDIGITS, text, 64) != 11 || !same(text, L"0123456789")) return 50;
	SetLastError(0);
	if (GetLocaleInfoEx(L"en-US", LOCALE_SNATIVEDIGITS, text, 10) || GetLastError() != ERROR_INSUFFICIENT_BUFFER) return 51;
	if (LCIDToLocaleName(0x459, name, LOCALE_NAME_MAX_LENGTH, 0) != 11 || lstrcmpW(name, L"sd-Deva-IN")) return 52;
	if (LocaleNameToLCID(L"ar-EG", 0) != 0xc01) return 53;
	return 0;
}
