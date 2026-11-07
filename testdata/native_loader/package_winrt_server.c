#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <activation.h>
#include <winstring.h>

static HRESULT WINAPI query(IActivationFactory *self, REFIID iid, void **out)
{
	if (!out) return E_POINTER;
	*out = NULL;
	if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IInspectable) && !IsEqualIID(iid, &IID_IActivationFactory)) return E_NOINTERFACE;
	*out = self;
	return S_OK;
}
static ULONG WINAPI ref(IActivationFactory *self) { (void)self; return 1; }
static HRESULT WINAPI iids(IActivationFactory *self, ULONG *count, IID **out) { (void)self; if (!count || !out) return E_POINTER; *count = 0; *out = NULL; return S_OK; }
static HRESULT WINAPI name(IActivationFactory *self, HSTRING *out) { (void)self; return WindowsCreateString(L"Fixture.PackagedClass", 21, out); }
static HRESULT WINAPI trust(IActivationFactory *self, TrustLevel *out) { (void)self; if (!out) return E_POINTER; *out = BaseTrust; return S_OK; }
static HRESULT WINAPI activate(IActivationFactory *self, IInspectable **out) { if (!out) return E_POINTER; *out = (IInspectable *)self; return S_OK; }
static const IActivationFactoryVtbl table = { query, ref, ref, iids, name, trust, activate };
static IActivationFactory factory = { &table };
__declspec(dllexport) HRESULT WINAPI DllGetActivationFactory(HSTRING class_id, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(class_id, &length);
	if (!out) return E_POINTER;
	*out = NULL;
	if (length != 21 || CompareStringOrdinal(text, length, L"Fixture.PackagedClass", -1, FALSE) != CSTR_EQUAL) return CLASS_E_CLASSNOTAVAILABLE;
	*out = &factory;
	return S_OK;
}
