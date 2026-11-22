#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <weakreference.h>

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *activate_function)(HSTRING, IInspectable **);
typedef void (WINAPI *uninitialize_function)(void);

/* The string header programs built with C++/WinRT write themselves for a reference: flags, length, two unused words, the characters. */
struct reference {
	UINT32 flags, length, unused[2];
	const WCHAR *text;
};

/* mingw's weakreference.h models both interfaces as IUnknown-only, but the Windows ABI derives them from IInspectable with
 * the one method at vtable slot 6, which is the layout the component implements; the fixture calls through that layout. */
struct weak_source_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(IUnknown *, REFIID, void **);
	ULONG (STDMETHODCALLTYPE *AddRef)(IUnknown *);
	ULONG (STDMETHODCALLTYPE *Release)(IUnknown *);
	HRESULT (STDMETHODCALLTYPE *GetIids)(IUnknown *, ULONG *, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(IUnknown *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(IUnknown *, TrustLevel *);
	HRESULT (STDMETHODCALLTYPE *GetWeakReference)(IUnknown *, IWeakReference **);
};

struct weak_reference_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(IUnknown *, REFIID, void **);
	ULONG (STDMETHODCALLTYPE *AddRef)(IUnknown *);
	ULONG (STDMETHODCALLTYPE *Release)(IUnknown *);
	HRESULT (STDMETHODCALLTYPE *GetIids)(IUnknown *, ULONG *, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(IUnknown *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(IUnknown *, TrustLevel *);
	HRESULT (STDMETHODCALLTYPE *Resolve)(IUnknown *, REFIID, IInspectable **);
};

/* The navigation transition token the traced Shapr3D launch builds for Frame.Navigate: activated by name through the
 * runtime catalog, the instance answers its IInspectable identity, the agility marker and the weak source the managed
 * projections take, and refuses the class's own interface casts while their walk's contract is unpinned. */
int raven_entry(void)
{
	static const WCHAR name[] = L"Windows.UI.Xaml.Media.Animation.SuppressNavigationTransitionInfo";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	activate_function RoActivateInstance = runtime ? (activate_function)GetProcAddress(runtime, "RoActivateInstance") : NULL;
	uninitialize_function RoUninitialize = runtime ? (uninitialize_function)GetProcAddress(runtime, "RoUninitialize") : NULL;
	if (!RoInitialize || !RoActivateInstance || !RoUninitialize) return 20;
	if (FAILED(RoInitialize(1))) return 1;
	struct reference class_name = {1, ARRAYSIZE(name) - 1, {0, 0}, name};
	IInspectable *object = NULL;
	if (RoActivateInstance((HSTRING)&class_name, &object) != S_OK || !object) return 2;
	IUnknown *unknown = NULL;
	if (IInspectable_QueryInterface(object, &IID_IUnknown, (void **)&unknown) != S_OK || !unknown) return 3;
	unknown->lpVtbl->Release(unknown);
	struct weak_source_vtbl *source_table;
	struct weak_reference_vtbl *reference_table;
	IWeakReferenceSource *source = NULL; IWeakReference *weak = NULL; IInspectable *resolved = NULL;
	if (IInspectable_QueryInterface(object, &IID_IWeakReferenceSource, (void **)&source) != S_OK || !source) return 41;
	source_table = (struct weak_source_vtbl *)(uintptr_t)source->lpVtbl;
	if (source_table->GetWeakReference((IUnknown *)source, &weak) != S_OK || !weak) return 42;
	reference_table = (struct weak_reference_vtbl *)(uintptr_t)weak->lpVtbl;
	if (reference_table->Resolve((IUnknown *)weak, &IID_IInspectable, &resolved) != S_OK || resolved != object) return 43;
	IInspectable_Release(resolved);
	reference_table->Release((IUnknown *)weak);
	source_table->Release((IUnknown *)source);
	/* The class's interface ids from the traced launch: the marker of the class and the second face its cast walk asks.
	 * Both are refused for now; the fixture records that contract. */
	static const IID marker_interface = {0x11d3b13a, 0x180e, 0x4789, {0xa8, 0xbe, 0x77, 0x12, 0x88, 0x28, 0x93, 0xe6}};
	static const IID second_interface = {0xa9b05091, 0xae4a, 0x4372, {0x86, 0x25, 0x21, 0xb7, 0xa8, 0xb9, 0x8c, 0xa4}};
	void *face = (void *)1;
	if (IInspectable_QueryInterface(object, &marker_interface, &face) != E_NOINTERFACE || face) return 5;
	face = (void *)1;
	if (IInspectable_QueryInterface(object, &second_interface, &face) != E_NOINTERFACE || face) return 6;
	IUnknown_Release((IUnknown *)object);
	RoUninitialize();
	return 0;
}
