/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven shell component: Windows Runtime classes registered in the registry, as the runtime finds in-process classes on Windows.
 *
 * A class is looked up under HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\WindowsRuntime\ActivatableClassId\<class>, whose DllPath value names
 * the library that serves it; the library is loaded and its DllGetActivationFactory asked for the factory of the class. A class the
 * catalog does not carry falls to the guest package's own declarations, which the XAML component reads from the manifest beside the
 * executable. */
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <roapi.h>
#include <winstring.h>
#include <activation.h>

typedef HRESULT (WINAPI *factory_function)(HSTRING, IActivationFactory **);
typedef HRESULT (WINAPI *package_server_function)(HSTRING, HSTRING *);

static const IID activation_factory = {0x00000035, 0x0000, 0x0000, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

/* Package-server diagnostics are intentionally separate from the normal activation trace. */
static void trace_package_server(const char *text)
{
	static void (WINAPI *raven_trace)(const char *);
	static int state = -1;
	if (state < 0)
		state = GetEnvironmentVariableW(L"RAVEN_TRACE_PACKAGE_SERVER", NULL, 0) != 0 ? 1 : 0;
	if (!state)
		return;
	if (!raven_trace) {
		raven_trace = (void (WINAPI *)(const char *))GetProcAddress(LoadLibraryW(L"ravenhost.dll"), "RavenTrace");
		if (!raven_trace)
			state = 0;
	}
	if (raven_trace)
		raven_trace(text);
}

static void trace_package_wide(const char *prefix, const WCHAR *text, UINT32 length)
{
	char line[260];
	size_t at = 0;
	while (at < ARRAYSIZE(line) - 1 && prefix[at])
		line[at] = prefix[at], ++at;
	for (UINT32 index = 0; index < length && at < ARRAYSIZE(line) - 1; ++index)
		line[at++] = text[index] < 0x80 ? (char)text[index] : '?';
	line[at] = 0;
	trace_package_server(line);
}

static void trace_package_hresult(HRESULT result)
{
	char line[64];
	wsprintfA(line, "winrt: package server lookup HRESULT 0x%08lx", (unsigned long)result);
	trace_package_server(line);
}

/* A class nobody registered is named in Raven's trace, so the classes a program misses can be found. */
/* The activation diagnosis: what was asked and what came of it, under RAVEN_TRACE_DWRITE. */
static void trace_activation(HSTRING name, const WCHAR *outcome)
{
	static void (WINAPI *raven_trace)(const char *);
	static BOOL looked;
	char line[220] = "winrt: activation ";
	size_t at = 18;
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	if (!looked) {
		raven_trace = (void (WINAPI *)(const char *))GetProcAddress(LoadLibraryW(L"ravenhost.dll"), "RavenTrace");
		looked = TRUE;
	}
	if (!raven_trace)
		return;
	for (UINT32 index = 0; index < length && at < ARRAYSIZE(line) - 64; ++index)
		line[at++] = text[index] < 0x80 ? (char)text[index] : '?';
	for (const WCHAR *scan = outcome; *scan && at < ARRAYSIZE(line) - 2; ++scan)
		line[at++] = *scan < 0x80 ? (char)*scan : '?';
	line[at] = 0;
	raven_trace(line);
}

/* The interface id the activation asked for, under RAVEN_TRACE_DWRITE. */
static void trace_iid(REFIID iid)
{
	static void (WINAPI *raven_trace)(const char *);
	static BOOL looked;
	char guid[64];
	if (!looked) {
		raven_trace = (void (WINAPI *)(const char *))GetProcAddress(LoadLibraryW(L"ravenhost.dll"), "RavenTrace");
		looked = TRUE;
	}
	if (!raven_trace)
		return;
	wsprintfA(guid, "winrt: activation iid %08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		(unsigned long)iid->Data1, iid->Data2, iid->Data3, iid->Data4[0], iid->Data4[1], iid->Data4[2],
		iid->Data4[3], iid->Data4[4], iid->Data4[5], iid->Data4[6], iid->Data4[7]);
	raven_trace(guid);
}

static void trace_unregistered(HSTRING name, REFIID iid)
{
	static void (WINAPI *raven_trace)(const char *);
	static BOOL looked;
	char line[200] = "winrt: no class ";
	size_t at = 16;
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	if (!looked) {
		raven_trace = (void (WINAPI *)(const char *))GetProcAddress(LoadLibraryW(L"ravenhost.dll"), "RavenTrace");
		looked = TRUE;
	}
	if (!raven_trace)
		return;
	for (UINT32 index = 0; index < length && at < ARRAYSIZE(line) - 1; ++index)
		line[at++] = text[index] < 0x80 ? (char)text[index] : '?';
	line[at] = 0;
	raven_trace(line);
	{
		char guid[64];
		wsprintfA(guid, "winrt: requested %08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			(unsigned long)iid->Data1, iid->Data2, iid->Data3, iid->Data4[0], iid->Data4[1], iid->Data4[2],
			iid->Data4[3], iid->Data4[4], iid->Data4[5], iid->Data4[6], iid->Data4[7]);
		raven_trace(guid);
	}
}

/* The library registered for a class, with environment variables expanded; FALSE when the class is not registered. */
static BOOL class_library(HSTRING name, WCHAR *library, DWORD capacity)
{
	static const WCHAR root[] = L"SOFTWARE\\Microsoft\\WindowsRuntime\\ActivatableClassId\\";
	WCHAR key[MAX_PATH], raw[MAX_PATH];
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(name, &length);
	if (!length || length >= MAX_PATH - ARRAYSIZE(root))
		return FALSE;
	lstrcpyW(key, root);
	lstrcpynW(key + ARRAYSIZE(root) - 1, text, (int)length + 1);
	DWORD type = 0, size = sizeof raw;
	if (RegGetValueW(HKEY_LOCAL_MACHINE, key, L"DllPath", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, &type, raw, &size))
		return FALSE;
	if (type == REG_EXPAND_SZ)
		return ExpandEnvironmentStringsW(raw, library, capacity) != 0;
	lstrcpynW(library, raw, (int)capacity);
	return TRUE;
}

HRESULT WINAPI RoGetActivationFactory(HSTRING name, REFIID iid, void **out)
{
	WCHAR library[MAX_PATH];
	IActivationFactory *factory;
	BOOL registered;
	if (!out)
		return E_INVALIDARG;
	*out = NULL;
	if (!name)
		return E_INVALIDARG;
	registered = class_library(name, library, MAX_PATH);
	trace_package_server(registered ? "winrt: package server class_library matched" : "winrt: package server class_library did not match");
	if (!registered) {
		HMODULE xaml = LoadLibraryW(L"raven-xaml.dll");
		factory_function xaml_get = xaml ? (factory_function)GetProcAddress(xaml, "DllGetActivationFactory") : NULL;
		IActivationFactory *xaml_factory = NULL;
		HRESULT result = xaml_get ? xaml_get(name, &xaml_factory) : CLASS_E_CLASSNOTAVAILABLE;
		trace_package_hresult(result);
		if (result != CLASS_E_CLASSNOTAVAILABLE) {
			if (FAILED(result))
				return result;
			if (!xaml_factory)
				return E_UNEXPECTED;
			result = IActivationFactory_QueryInterface(xaml_factory, iid, out);
			IActivationFactory_Release(xaml_factory);
			return result;
		}
		/* A class the XAML component does not serve may be the guest package's own: the manifest beside the executable declares
		 * its in-process servers, and the component that reads it names the server DLL for the class. */
		HSTRING package_path = NULL;
		package_server_function lookup = xaml ? (package_server_function)GetProcAddress(xaml, "raven_package_winrt_server") : NULL;
		trace_package_server(xaml ? "winrt: package server raven-xaml LoadLibraryW succeeded" : "winrt: package server raven-xaml LoadLibraryW failed");
		trace_package_server(lookup ? "winrt: package server GetProcAddress succeeded" : "winrt: package server GetProcAddress failed");
		HRESULT found = lookup ? lookup(name, &package_path) : S_FALSE;
		trace_package_hresult(found);
		if (package_path) {
			UINT32 path_length = 0;
			const WCHAR *path_text = WindowsGetStringRawBuffer(package_path, &path_length);
			trace_package_wide("winrt: package server returned package path ", path_text, path_length);
		} else {
			trace_package_server("winrt: package server returned package path (null)");
		}
		if (xaml)
			FreeLibrary(xaml);
		if (FAILED(found))
			return found;
		if (found != S_OK || !package_path) {
			trace_package_server(lookup ? "winrt: package server REGDB_E_CLASSNOTREG branch: lookup returned no package" : "winrt: package server REGDB_E_CLASSNOTREG branch: no package server");
			trace_unregistered(name, iid);
			return REGDB_E_CLASSNOTREG;
		}
		trace_activation(name, L"package");
		UINT32 length = 0;
		const WCHAR *path = WindowsGetStringRawBuffer(package_path, &length);
		if (length >= MAX_PATH) {
			WindowsDeleteString(package_path);
			return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
		}
		lstrcpynW(library, path, (int)length + 1);
		WindowsDeleteString(package_path);
	}
	/* The library stays loaded for the objects it makes, as the runtime keeps it until the apartment ends. */
	HMODULE module = LoadLibraryExW(library, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!module)
		return CO_E_DLLNOTFOUND;
	factory_function get = (factory_function)GetProcAddress(module, "DllGetActivationFactory");
	if (!get)
		return CO_E_ERRORINDLL;
	HRESULT result = get(name, &factory);
	if (FAILED(result)) {
		trace_activation(name, L"dll factory");
		return result;
	}
	trace_iid(iid);
	result = IActivationFactory_QueryInterface(factory, iid, out);
	IActivationFactory_Release(factory);
	trace_activation(name, FAILED(result) ? L"query failed" : (*out ? L"served" : L"null factory"));
	return result;
}

HRESULT WINAPI RoActivateInstance(HSTRING name, IInspectable **out)
{
	IActivationFactory *factory;
	if (!out)
		return E_INVALIDARG;
	*out = NULL;
	HRESULT result = RoGetActivationFactory(name, &activation_factory, (void **)&factory);
	if (FAILED(result))
		return result;
	result = IActivationFactory_ActivateInstance(factory, out);
	IActivationFactory_Release(factory);
	return result;
}
