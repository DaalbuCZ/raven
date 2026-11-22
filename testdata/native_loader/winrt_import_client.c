#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <roapi.h>

/* A program that imports only the Windows Runtime's activation and strings, through the API sets runtimeobject's import library names,
 * and nothing of COM, the shell or user32: it activates the package manager by name and reads back its class name. */
int raven_entry(void)
{
	static const WCHAR name[] = L"Windows.Management.Deployment.PackageManager";
	HSTRING class_name = NULL, runtime_name = NULL;
	IInspectable *object = NULL;
	UINT32 length = 0;
	if (FAILED(RoInitialize(RO_INIT_MULTITHREADED))) return 1;
	if (WindowsCreateString(name, ARRAYSIZE(name) - 1, &class_name) != S_OK) return 2;
	if (RoActivateInstance(class_name, &object) != S_OK || !object) return 3;
	if (IInspectable_GetRuntimeClassName(object, &runtime_name) != S_OK) return 4;
	const WCHAR *text = WindowsGetStringRawBuffer(runtime_name, &length);
	if (length != ARRAYSIZE(name) - 1 || CompareStringOrdinal(text, (int)length, name, -1, FALSE) != CSTR_EQUAL) return 5;
	WindowsDeleteString(runtime_name);
	IInspectable_Release(object);
	WindowsDeleteString(class_name);
	/* The shape class the compiled ConfirmDialog names by its stable index (470): the catalog serves the activation,
	 * the element names the Shapes class, and the engine's apply seam stores a Fill the way the Panel.Background
	 * store does — a brush or a colour name lands, and a value that is neither fails the application. */
	{
		static const WCHAR shape_name[] = L"Windows.UI.Xaml.Shapes.Rectangle";
		HSTRING shape = NULL, runtime_name = NULL;
		IInspectable *rectangle = NULL, *brush = NULL, *name_value = NULL, *wrong_value = NULL;
		HMODULE component = NULL;
		typedef HRESULT (*apply_fn)(IInspectable *, const WCHAR *, UINT32, IInspectable *);
		typedef HRESULT (*brush_create_fn)(IInspectable **);
		typedef IInspectable *(*boxed_string_fn)(const WCHAR *, UINT32);
		apply_fn apply = NULL;
		brush_create_fn brush_create = NULL;
		boxed_string_fn boxed = NULL;
		if (WindowsCreateString(shape_name, ARRAYSIZE(shape_name) - 1, &shape) != S_OK) return 6;
		if (RoActivateInstance(shape, &rectangle) != S_OK || !rectangle) return 7;
		if (IInspectable_GetRuntimeClassName(rectangle, &runtime_name) != S_OK) return 8;
		{
			UINT32 shape_length = 0;
			const WCHAR *text = WindowsGetStringRawBuffer(runtime_name, &shape_length);
			if (shape_length != ARRAYSIZE(shape_name) - 1 ||
				CompareStringOrdinal(text, (int)shape_length, shape_name, -1, FALSE) != CSTR_EQUAL) return 9;
		}
		WindowsDeleteString(runtime_name);
		component = LoadLibraryW(L"raven-xaml.dll");
		apply = component ? (apply_fn)(void *)GetProcAddress(component, "raven_xaml_element_apply_value") : NULL;
		brush_create = component ? (brush_create_fn)(void *)GetProcAddress(component, "raven_xaml_solid_brush_create") : NULL;
		boxed = component ? (boxed_string_fn)(void *)GetProcAddress(component, "raven_xaml_boxed_string_create") : NULL;
		if (!apply || !brush_create || !boxed) return 10;
		if (brush_create(&brush) != S_OK || !brush) return 11;
		if (apply(rectangle, L"Fill", 4, brush) != S_OK) return 12;
		name_value = boxed(L"Red", 3);
		if (!name_value || apply(rectangle, L"Fill", 4, name_value) != S_OK) return 13;
		wrong_value = boxed(L"Septarine", 9);
		if (!wrong_value || apply(rectangle, L"Fill", 4, wrong_value) != E_INVALIDARG) return 14;
		IInspectable_Release(wrong_value); IInspectable_Release(name_value);
		IInspectable_Release(brush); IInspectable_Release(rectangle);
		WindowsDeleteString(shape);
		FreeLibrary(component);
	}
	RoUninitialize();
	return 0;
}
