#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* The millisecond clocks are the performance counter in milliseconds, whether the runtime or the program reads them. */

static LONGLONG distance(ULONGLONG left, ULONGLONG right)
{
	return left > right ? (LONGLONG)(left - right) : (LONGLONG)(right - left);
}

int raven_entry(void)
{
	/* Long enough for the runtime to have calibrated the time stamp counter against the performance counter. */
	Sleep(400);
	LARGE_INTEGER frequency, counter;
	if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart != 10000000 || !QueryPerformanceCounter(&counter))
		return 1;
	const ULONGLONG ticks = GetTickCount64();
	if (distance(ticks, (ULONGLONG)counter.QuadPart / 10000) > 5)
		return 2;
	if (distance(GetTickCount(), (DWORD)GetTickCount64()) > 5)
		return 3;
	Sleep(50);
	const ULONGLONG later = GetTickCount64();
	if (later < ticks + 45 || later > ticks + 250)
		return 4;
	HMODULE zone_api = LoadLibraryW(L"api-ms-win-core-timezone-l1-1-0.dll");
	typedef DWORD (WINAPI *enum_zones_fn)(DWORD, PDYNAMIC_TIME_ZONE_INFORMATION);
	typedef DWORD (WINAPI *zone_years_fn)(PDYNAMIC_TIME_ZONE_INFORMATION, PDWORD, PDWORD);
	enum_zones_fn enum_zones = (enum_zones_fn)GetProcAddress(zone_api, "EnumDynamicTimeZoneInformation");
	zone_years_fn zone_years = (zone_years_fn)GetProcAddress(zone_api, "GetDynamicTimeZoneInformationEffectiveYears");
	DYNAMIC_TIME_ZONE_INFORMATION zone;
	DWORD first = 123, last = 456;
	if (!enum_zones || !zone_years || enum_zones(0, NULL) != ERROR_INVALID_PARAMETER ||
	    enum_zones(0, &zone) != ERROR_NO_MORE_ITEMS || zone_years(NULL, &first, &last) != ERROR_INVALID_PARAMETER)
		return 5;
	GetDynamicTimeZoneInformation(&zone);
	if (zone_years(&zone, &first, &last) != ERROR_FILE_NOT_FOUND || first != 123 || last != 456)
		return 6;
	HMODULE type_api = LoadLibraryW(L"api-ms-win-ro-typeresolution-l1-1-0.dll");
	if (!type_api || !GetProcAddress(type_api, "RoParseTypeName"))
		return 7;
	return 0;
}
