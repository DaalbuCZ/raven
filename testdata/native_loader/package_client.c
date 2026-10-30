#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <windows.applicationmodel.h>

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);

typedef __x_ABI_CWindows_CApplicationModel_CIPackage package_i;
typedef __x_ABI_CWindows_CApplicationModel_CIPackage2 package2_i;
typedef __x_ABI_CWindows_CApplicationModel_CIPackageId package_id_i;
typedef __x_ABI_CWindows_CApplicationModel_CIPackageStatics package_statics_i;
typedef __x_ABI_CWindows_CApplicationModel_CPackageVersion package_version;
typedef __x_ABI_CWindows_CSystem_CProcessorArchitecture package_architecture;

/* The class the catalog routes to raven-xaml.dll; the manifest beside the executable carries the identity the package
   object answers with. */
static int same_text(HSTRING value, const WCHAR *expected)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(value, &length);
	return length == lstrlenW(expected) && CompareStringOrdinal(text, (int)length, expected, -1, FALSE) == CSTR_EQUAL;
}

int raven_entry(void)
{
	static const WCHAR name[] = L"Windows.ApplicationModel.Package";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function RoGetActivationFactory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	HSTRING class_key = NULL;
	IActivationFactory *activation = NULL, *activation_again = NULL;
	package_statics_i *statics = NULL;
	package_i *current = NULL, *current_again = NULL;
	package_id_i *id = NULL;
	package2_i *display = NULL;
	HSTRING text = NULL;
	package_version version = {0, 0, 0, 0};
	package_architecture architecture = ProcessorArchitecture_Unknown;
	boolean flag = 0;
	if (!RoInitialize || !RoGetActivationFactory) return 20;
	if (FAILED(RoInitialize(1))) return 1;
	if (WindowsCreateString(name, ARRAYSIZE(name) - 1, &class_key) != S_OK || !class_key) return 21;
	if (RoGetActivationFactory(class_key, &IID_IActivationFactory, (void **)&activation) != S_OK || !activation) return 2;
	/* a second activation of the class hands out the same factory object */
	if (RoGetActivationFactory(class_key, &IID_IActivationFactory, (void **)&activation_again) != S_OK ||
		activation_again != activation) return 3;
	if (activation->lpVtbl->QueryInterface(activation, &IID___x_ABI_CWindows_CApplicationModel_CIPackageStatics,
		(void **)&statics) != S_OK || !statics) return 4;
	/* the package of the process is a singleton: two asks answer the same object */
	if (statics->lpVtbl->get_Current(statics, &current) != S_OK || !current) return 5;
	if (statics->lpVtbl->get_Current(statics, &current_again) != S_OK || current_again != current) return 6;
	if (current->lpVtbl->GetRuntimeClassName(current, &text) != S_OK ||
		!same_text(text, L"Windows.ApplicationModel.Package")) return 7;
	WindowsDeleteString(text);
	text = NULL;
	if (current->lpVtbl->QueryInterface(current, &IID___x_ABI_CWindows_CApplicationModel_CIPackageId, (void **)&id) != S_OK ||
		!id) return 8;
	if (id->lpVtbl->get_Name(id, &text) != S_OK || !same_text(text, L"Raven.Fixture")) return 9;
	WindowsDeleteString(text);
	text = NULL;
	if (id->lpVtbl->get_Publisher(id, &text) != S_OK || !same_text(text, L"CN=Raven")) return 10;
	WindowsDeleteString(text);
	text = NULL;
	if (id->lpVtbl->get_Version(id, &version) != S_OK) return 11;
	if (version.Major != 1 || version.Minor != 2 || version.Build != 3 || version.Revision != 4) return 12;
	if (id->lpVtbl->get_Architecture(id, &architecture) != S_OK) return 13;
#ifdef _WIN64
	if (architecture != ProcessorArchitecture_X64) return 13;
#else
	if (architecture != ProcessorArchitecture_X86) return 13;
#endif
	/* the display properties come from the manifest's Properties element */
	if (current->lpVtbl->QueryInterface(current, &IID___x_ABI_CWindows_CApplicationModel_CIPackage2, (void **)&display) != S_OK ||
		!display) return 14;
	if (display->lpVtbl->get_DisplayName(display, &text) != S_OK || !same_text(text, L"Raven package fixture")) return 15;
	WindowsDeleteString(text);
	text = NULL;
	if (display->lpVtbl->get_PublisherDisplayName(display, &text) != S_OK || !same_text(text, L"Raven")) return 16;
	WindowsDeleteString(text);
	text = NULL;
	if (display->lpVtbl->get_IsResourcePackage(display, &flag) != S_OK || flag) return 17;
	if (current->lpVtbl->get_IsFramework(current, &flag) != S_OK || flag) return 18;
	/* the family and full names end in the publisher identifier and carry the version and the architecture */
	{
#ifdef _WIN64
		static const WCHAR arch[] = L"x64";
#else
		static const WCHAR arch[] = L"x86";
#endif
		HSTRING family = NULL, full = NULL, publisher_id = NULL;
		UINT32 family_length = 0, full_length = 0, publisher_length = 0;
		const WCHAR *family_text, *full_text, *publisher_text;
		const UINT32 full_prefix = lstrlenW(L"Raven.Fixture_1.2.3.4_");
		if (id->lpVtbl->get_FamilyName(id, &family) != S_OK || !family) return 19;
		if (id->lpVtbl->get_PublisherId(id, &publisher_id) != S_OK || !publisher_id) return 22;
		if (id->lpVtbl->get_FullName(id, &full) != S_OK || !full) return 23;
		family_text = WindowsGetStringRawBuffer(family, &family_length);
		publisher_text = WindowsGetStringRawBuffer(publisher_id, &publisher_length);
		full_text = WindowsGetStringRawBuffer(full, &full_length);
		/* the publisher identifier of CN=Raven, the documented SHA-256 over the UTF-16 publisher in Crockford base 32 */
		if (publisher_length != 13 ||
			CompareStringOrdinal(publisher_text, 13, L"0v3e6ww4q1hm4", -1, FALSE) != CSTR_EQUAL) return 27;
		if (family_length != lstrlenW(L"Raven.Fixture_") + publisher_length ||
			CompareStringOrdinal(family_text, (int)lstrlenW(L"Raven.Fixture_"), L"Raven.Fixture_", -1, FALSE) != CSTR_EQUAL ||
			CompareStringOrdinal(family_text + lstrlenW(L"Raven.Fixture_"), (int)publisher_length,
				publisher_text, (int)publisher_length, FALSE) != CSTR_EQUAL) return 24;
		if (full_length != full_prefix + lstrlenW(arch) + 2 + publisher_length ||
			CompareStringOrdinal(full_text, (int)full_prefix, L"Raven.Fixture_1.2.3.4_", -1, FALSE) != CSTR_EQUAL ||
			CompareStringOrdinal(full_text + full_length - publisher_length, (int)publisher_length,
				publisher_text, (int)publisher_length, FALSE) != CSTR_EQUAL) return 25;
		if (CompareStringOrdinal(full_text + full_prefix, (int)lstrlenW(arch), arch, -1, FALSE) != CSTR_EQUAL ||
			CompareStringOrdinal(full_text + full_prefix + lstrlenW(arch), 2, L"__", -1, FALSE) != CSTR_EQUAL) return 26;
		WindowsDeleteString(full);
		WindowsDeleteString(publisher_id);
		WindowsDeleteString(family);
	}
	display->lpVtbl->Release(display);
	id->lpVtbl->Release(id);
	current_again->lpVtbl->Release(current_again);
	current->lpVtbl->Release(current);
	statics->lpVtbl->Release(statics);
	activation_again->lpVtbl->Release(activation_again);
	activation->lpVtbl->Release(activation);
	WindowsDeleteString(class_key);
	return 0;
}
