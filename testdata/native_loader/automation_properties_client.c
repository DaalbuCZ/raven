#define WIN32_LEAN_AND_MEAN
#define CINTERFACE
#define COBJMACROS
#define INITGUID
#include <windows.h>
#include <inspectable.h>
#include <winstring.h>
#include <activation.h>
#include <stdint.h>

/* AutomationProperties, the attached-property surface the app drives over any element: the unset
 * getters answer their documented defaults (Level 0, IsPeripheral false, LabeledBy null), the string
 * setters create the element's store on demand (the first store growth is a HeapAlloc — HeapReAlloc
 * refuses a null block, the bug the traced Shapr3D run died on), and a value set through the grown
 * store reads back. The declarations are the component's generated header. */
#include "windows_ui_xaml_automation_properties.h"

typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics props1;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics3 props3;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics5 props5;
#define IID_props1 IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics
#define IID_props3 IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics3
#define IID_props5 IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics5

typedef HRESULT (WINAPI *activation_function)(HSTRING, REFIID, void **);

static int same_text(HSTRING value, const WCHAR *expected)
{
	UINT32 length = 0;
	const WCHAR *text = expected ? WindowsGetStringRawBuffer(value, &length) : NULL;
	return (INT)length == (expected ? (INT)lstrlenW(expected) : 0) &&
		(!expected || CompareStringOrdinal(text, (int)length, expected, -1, FALSE) == CSTR_EQUAL);
}

int raven_entry(void)
{
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	activation_function activate_factory = runtime ? (activation_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	if (!activate_factory)
		return 1;
	HSTRING key = NULL, text = NULL, read_back = NULL;
	IActivationFactory *factory = NULL;
	props1 *statics1 = NULL;
	props3 *statics3 = NULL;
	props5 *statics5 = NULL;
	IInspectable *labeled_by = (IInspectable *)(uintptr_t)0x1;
	int32_t number = -1;
	HRESULT result;
	UINT32 index;
	/* Any non-null value keys the store; the statics treat the pointer as the element. */
	uint64_t element = (uint64_t)(uintptr_t)&factory;

	if (WindowsCreateString(L"Windows.UI.Xaml.Automation.AutomationProperties", 47, &key) != S_OK)
		return 2;
	if (activate_factory(key, &IID_IActivationFactory, (void **)&factory) != S_OK || !factory)
		return 3;
	if (factory->lpVtbl->QueryInterface(factory, &IID_props1, (void **)&statics1) != S_OK || !statics1)
		return 4;
	if (factory->lpVtbl->QueryInterface(factory, &IID_props3, (void **)&statics3) != S_OK || !statics3)
		return 5;
	if (factory->lpVtbl->QueryInterface(factory, &IID_props5, (void **)&statics5) != S_OK || !statics5)
		return 6;

	/* The unset getters answer their defaults, not a refusal. */
	if (statics3->lpVtbl->GetLevel(statics3, element, (uint64_t)(uintptr_t)&number) != S_OK)
		return 7;
	if (number != 0)
		return 8;
	if (statics5->lpVtbl->GetIsPeripheral(statics5, element, (uint64_t)(uintptr_t)&number) != S_OK)
		return 9;
	if (number != 0)
		return 10;
	if (statics1->lpVtbl->GetLabeledBy(statics1, element, (uint64_t)(uintptr_t)&labeled_by) != S_OK)
		return 11;
	if (labeled_by)
		return 12;

	/* Twenty sets force the store past its first growth; the last value reads back. */
	if (WindowsCreateString(L"fixture", 7, &text) != S_OK)
		return 13;
	for (index = 0; index < 20; ++index) {
		result = statics1->lpVtbl->SetAutomationId(statics1, element, (uint64_t)(uintptr_t)text);
		if (result != S_OK)
			return 20 + index;
	}
	if (statics1->lpVtbl->GetAutomationId(statics1, element, (uint64_t)(uintptr_t)&read_back) != S_OK)
		return 41;
	if (!same_text(read_back, L"fixture"))
		return 42;
	WindowsDeleteString(read_back);

	/* A second element keeps its own store. */
	{
		uint64_t other = element + 0x40;
		if (statics1->lpVtbl->SetAutomationId(statics1, other, (uint64_t)(uintptr_t)text) != S_OK)
			return 43;
		if (statics1->lpVtbl->GetAutomationId(statics1, element, (uint64_t)(uintptr_t)&read_back) != S_OK)
			return 44;
		if (!same_text(read_back, L"fixture"))
			return 45;
		WindowsDeleteString(read_back);
	}

	WindowsDeleteString(text);
	WindowsDeleteString(key);
	statics5->lpVtbl->Release(statics5);
	statics3->lpVtbl->Release(statics3);
	statics1->lpVtbl->Release(statics1);
	factory->lpVtbl->Release(factory);
	return 0;
}
