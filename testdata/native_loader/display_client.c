/* Checks the display enumeration answers and returns 0, or the number of the first failing check. */
#include <windows.h>

static int wide_equals(const wchar_t *text, const wchar_t *expected)
{
	for (unsigned index = 0;; ++index) {
		if (text[index] != expected[index])
			return 0;
		if (!expected[index])
			return 1;
	}
}

static int wide_starts(const wchar_t *text, const wchar_t *prefix)
{
	for (unsigned index = 0; prefix[index]; ++index)
		if (text[index] != prefix[index])
			return 0;
	return 1;
}

unsigned int raven_entry(void)
{
	DEVMODEW mode;
	for (unsigned index = 0; index < sizeof mode; ++index)
		((char *)&mode)[index] = 0;
	mode.dmSize = sizeof mode;
	if (!EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &mode))
		return 1;
	if (mode.dmBitsPerPel != 32 || mode.dmPelsWidth == 0 || mode.dmPelsHeight == 0 || mode.dmDisplayFrequency == 0)
		return 2;
	if ((mode.dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY)) != (DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY))
		return 3;
	if (mode.dmPelsWidth != (DWORD)GetSystemMetrics(SM_CXSCREEN) || mode.dmPelsHeight != (DWORD)GetSystemMetrics(SM_CYSCREEN))
		return 4;
	DEVMODEW listed = mode;
	if (!EnumDisplaySettingsW(L"\\\\.\\DISPLAY1", 0, &listed) || listed.dmPelsWidth != mode.dmPelsWidth)
		return 5;
	if (EnumDisplaySettingsW(NULL, 1, &listed))
		return 6;
	if (EnumDisplaySettingsW(L"\\\\.\\DISPLAY9", 0, &listed))
		return 7;
	/* The display configuration reports the one active path over one source and one target, with the desktop's mode
	 * described as a source mode and a target mode each, and names the display and its monitor on the way to them. */
	UINT32 paths = 0, modes = 0;
	if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &paths, &modes) != ERROR_SUCCESS || paths != 1 || modes != 2)
		return 8;
	DISPLAYCONFIG_PATH_INFO path[1];
	DISPLAYCONFIG_MODE_INFO mode_info[2];
	paths = 1;
	modes = 2;
	if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &paths, path, &modes, mode_info, NULL) != ERROR_SUCCESS || paths != 1 || modes != 2)
		return 9;
	if (!(path[0].flags & DISPLAYCONFIG_PATH_ACTIVE) || path[0].sourceInfo.adapterId.LowPart != 1 ||
		path[0].targetInfo.adapterId.LowPart != 1 || mode_info[0].infoType != DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE ||
		mode_info[1].infoType != DISPLAYCONFIG_MODE_INFO_TYPE_TARGET ||
		mode_info[0].sourceMode.width != (UINT32)GetSystemMetrics(SM_CXSCREEN))
		return 10;
	DISPLAYCONFIG_SOURCE_DEVICE_NAME source;
	source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
	source.header.size = sizeof(source);
	source.header.adapterId = path[0].sourceInfo.adapterId;
	source.header.id = path[0].sourceInfo.id;
	if (DisplayConfigGetDeviceInfo(&source.header) != ERROR_SUCCESS || !wide_equals(source.viewGdiDeviceName, L"\\\\.\\DISPLAY1"))
		return 11;
	DISPLAYCONFIG_TARGET_DEVICE_NAME target;
	target.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
	target.header.size = sizeof(target);
	target.header.adapterId = path[0].targetInfo.adapterId;
	target.header.id = path[0].targetInfo.id;
	if (DisplayConfigGetDeviceInfo(&target.header) != ERROR_SUCCESS ||
		!wide_starts(target.monitorDevicePath, L"\\\\?\\DISPLAY#RAVEN0000") || !target.monitorFriendlyDeviceName[0])
		return 12;
	DISPLAYCONFIG_SDR_WHITE_LEVEL sdr;
	sdr.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
	sdr.header.size = sizeof(sdr);
	sdr.header.adapterId = path[0].targetInfo.adapterId;
	sdr.header.id = path[0].targetInfo.id;
	if (DisplayConfigGetDeviceInfo(&sdr.header) != ERROR_SUCCESS || sdr.SDRWhiteLevel != 1000)
		return 13;
	/* A struct version the API does not know answers ERROR_INVALID_PARAMETER, as it does on Windows. */
	DISPLAYCONFIG_SOURCE_DEVICE_NAME wrong = source;
	wrong.header.size = sizeof(wrong) + 4;
	if (DisplayConfigGetDeviceInfo(&wrong.header) != ERROR_INVALID_PARAMETER)
		return 14;
	return 0;
}
