/* SPDX-License-Identifier: MIT */
#include "internal.h"
#include <limits.h>

/* shortcut: only English (US) plural and conjunction patterns are available; add CLDR data for other locales. */
static BOOL english_us(const char *locale)
{
	if (!locale || !*locale) {
		WCHAR name[LOCALE_NAME_MAX_LENGTH];
		if (!GetUserDefaultLocaleName(name, ARRAYSIZE(name))) return FALSE;
		return !lstrcmpiW(name, L"en-US") || !lstrcmpiW(name, L"en");
	}
	return !lstrcmpiA(locale, "en") || !lstrcmpiA(locale, "en_US") || !lstrcmpiA(locale, "en-US");
}

typedef struct { int ordinal; } plural_rules;
typedef struct { int unused; } list_formatter;

plural_rules *__cdecl uplrules_openForType(const char *locale, int type, UErrorCode *status)
{
	plural_rules *rules;
	if (!status || U_FAILED(*status)) return NULL;
	if (type < 0 || type > 1) { *status = U_ILLEGAL_ARGUMENT_ERROR; return NULL; }
	if (!english_us(locale)) { *status = U_UNSUPPORTED_ERROR; return NULL; }
	rules = icu_alloc(sizeof *rules);
	if (!rules) { *status = U_MEMORY_ALLOCATION_ERROR; return NULL; }
	rules->ordinal = type;
	return rules;
}
void __cdecl uplrules_close(plural_rules *rules) { icu_free(rules); }

/* x87 remainder handles large finite values without an integer conversion overflow. */
static double modulo(double x, double y)
{
	unsigned short status;
	do { __asm__ volatile("fprem; fnstsw %0" : "=a"(status), "+t"(x) : "u"(y) : "cc"); } while (status & 0x400);
	return x;
}

static INT copy_result(const UChar *text, INT length, UChar *out, INT capacity, UErrorCode *status)
{
	if (capacity < 0 || (!out && capacity)) { *status = U_ILLEGAL_ARGUMENT_ERROR; return 0; }
	for (INT i = 0; i < length && i < capacity; ++i) out[i] = text[i];
	if (length < capacity) out[length] = 0;
	return icu_finish(length, capacity, status);
}

INT __cdecl uplrules_select(const plural_rules *rules, double number, UChar *out, INT capacity, UErrorCode *status)
{
	const UChar *word = L"other";
	if (!status || U_FAILED(*status)) return 0;
	if (!rules) { *status = U_ILLEGAL_ARGUMENT_ERROR; return 0; }
	if (number < 0) number = -number;
	if (!rules->ordinal) {
		if (number == 1.0) word = L"one";
	} else if (number == number && number < 1.0 / 0.0 && modulo(number, 1.0) == 0) {
		int last = (int)modulo(number, 10.0), last_two = (int)modulo(number, 100.0);
		if (last == 1 && last_two != 11) word = L"one";
		else if (last == 2 && last_two != 12) word = L"two";
		else if (last == 3 && last_two != 13) word = L"few";
	}
	return copy_result(word, lstrlenW(word), out, capacity, status);
}

list_formatter *__cdecl ulistfmt_open(const char *locale, UErrorCode *status)
{
	list_formatter *formatter;
	if (!status || U_FAILED(*status)) return NULL;
	if (!english_us(locale)) { *status = U_UNSUPPORTED_ERROR; return NULL; }
	formatter = icu_alloc(sizeof *formatter);
	if (!formatter) *status = U_MEMORY_ALLOCATION_ERROR;
	return formatter;
}
void __cdecl ulistfmt_close(list_formatter *formatter) { icu_free(formatter); }

INT __cdecl ulistfmt_format(const list_formatter *formatter, const UChar *const *strings, const INT *lengths,
	INT count, UChar *out, INT capacity, UErrorCode *status)
{
	INT total = 0;
	if (!status || U_FAILED(*status)) return 0;
	if (!formatter || count < 0 || (count && !strings) || capacity < 0 || (!out && capacity)) {
		*status = U_ILLEGAL_ARGUMENT_ERROR; return 0;
	}
	for (INT i = 0; i < count; ++i) {
		if (!strings[i] || (lengths && lengths[i] < -1)) { *status = U_ILLEGAL_ARGUMENT_ERROR; return 0; }
		INT length = icu_length(strings[i], lengths ? lengths[i] : -1);
		const UChar *separator = i == 0 ? L"" : i == count - 1 ? (count == 2 ? L" and " : L", and ") : L", ";
		INT sep_length = lstrlenW(separator);
		if (length > INT_MAX - total - sep_length) { *status = U_INDEX_OUTOFBOUNDS_ERROR; return 0; }
		for (INT j = 0; j < sep_length; ++j) { if (total < capacity) out[total] = separator[j]; ++total; }
		for (INT j = 0; j < length; ++j) { if (total < capacity) out[total] = strings[i][j]; ++total; }
	}
	if (total < capacity) out[total] = 0;
	return icu_finish(total, capacity, status);
}
