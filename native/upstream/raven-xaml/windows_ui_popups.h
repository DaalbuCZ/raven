/* Minimal Windows.UI.Popups ABI declarations used by the headless MessageDialog shim. */
#ifndef RAVEN_WINDOWS_UI_POPUPS_H
#define RAVEN_WINDOWS_UI_POPUPS_H
#include <inspectable.h>
#include <asyncinfo.h>

typedef struct raven_message_dialog_factory raven_message_dialog_factory_i;
typedef struct raven_message_dialog_factory_vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(raven_message_dialog_factory_i *, REFIID, void **);
	ULONG (STDMETHODCALLTYPE *AddRef)(raven_message_dialog_factory_i *);
	ULONG (STDMETHODCALLTYPE *Release)(raven_message_dialog_factory_i *);
	HRESULT (STDMETHODCALLTYPE *GetIids)(raven_message_dialog_factory_i *, ULONG *, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(raven_message_dialog_factory_i *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(raven_message_dialog_factory_i *, TrustLevel *);
	HRESULT (STDMETHODCALLTYPE *Create)(raven_message_dialog_factory_i *, HSTRING, IInspectable **);
	HRESULT (STDMETHODCALLTYPE *CreateWithTitle)(raven_message_dialog_factory_i *, HSTRING, HSTRING, IInspectable **);
	END_INTERFACE
} raven_message_dialog_factory_vtbl;
struct raven_message_dialog_factory { const raven_message_dialog_factory_vtbl *lpVtbl; };
DEFINE_GUID(IID_RavenIMessageDialogFactory, 0x2d161777, 0xa66f, 0x4ea5, 0xbb, 0x87, 0x79, 0x3f, 0xfa, 0x49, 0x41, 0xf2);

typedef struct raven_message_dialog raven_message_dialog_i;
typedef struct raven_message_dialog_vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(raven_message_dialog_i *, REFIID, void **);
	ULONG (STDMETHODCALLTYPE *AddRef)(raven_message_dialog_i *);
	ULONG (STDMETHODCALLTYPE *Release)(raven_message_dialog_i *);
	HRESULT (STDMETHODCALLTYPE *GetIids)(raven_message_dialog_i *, ULONG *, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(raven_message_dialog_i *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(raven_message_dialog_i *, TrustLevel *);
	HRESULT (STDMETHODCALLTYPE *get_Title)(raven_message_dialog_i *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *put_Title)(raven_message_dialog_i *, HSTRING);
	HRESULT (STDMETHODCALLTYPE *get_Commands)(raven_message_dialog_i *, IInspectable **);
	HRESULT (STDMETHODCALLTYPE *get_DefaultCommandIndex)(raven_message_dialog_i *, UINT32 *);
	HRESULT (STDMETHODCALLTYPE *put_DefaultCommandIndex)(raven_message_dialog_i *, UINT32);
	HRESULT (STDMETHODCALLTYPE *get_CancelCommandIndex)(raven_message_dialog_i *, UINT32 *);
	HRESULT (STDMETHODCALLTYPE *put_CancelCommandIndex)(raven_message_dialog_i *, UINT32);
	HRESULT (STDMETHODCALLTYPE *get_Content)(raven_message_dialog_i *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *put_Content)(raven_message_dialog_i *, HSTRING);
	HRESULT (STDMETHODCALLTYPE *ShowAsync)(raven_message_dialog_i *, IInspectable **);
	HRESULT (STDMETHODCALLTYPE *get_Options)(raven_message_dialog_i *, UINT32 *);
	HRESULT (STDMETHODCALLTYPE *put_Options)(raven_message_dialog_i *, UINT32);
	END_INTERFACE
} raven_message_dialog_vtbl;
struct raven_message_dialog { const raven_message_dialog_vtbl *lpVtbl; };
DEFINE_GUID(IID_RavenIMessageDialog, 0x33f59b01, 0x5325, 0x43ab, 0x9a, 0xb3, 0xbd, 0xae, 0x44, 0x0e, 0x41, 0x21);

typedef struct raven_ui_command raven_ui_command_i;
typedef struct raven_ui_command_vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(raven_ui_command_i *, REFIID, void **);
	ULONG (STDMETHODCALLTYPE *AddRef)(raven_ui_command_i *);
	ULONG (STDMETHODCALLTYPE *Release)(raven_ui_command_i *);
	HRESULT (STDMETHODCALLTYPE *GetIids)(raven_ui_command_i *, ULONG *, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(raven_ui_command_i *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(raven_ui_command_i *, TrustLevel *);
	HRESULT (STDMETHODCALLTYPE *get_Label)(raven_ui_command_i *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *put_Label)(raven_ui_command_i *, HSTRING);
	HRESULT (STDMETHODCALLTYPE *get_Invoked)(raven_ui_command_i *, IUnknown **);
	HRESULT (STDMETHODCALLTYPE *put_Invoked)(raven_ui_command_i *, IUnknown *);
	HRESULT (STDMETHODCALLTYPE *get_Id)(raven_ui_command_i *, IInspectable **);
	HRESULT (STDMETHODCALLTYPE *put_Id)(raven_ui_command_i *, IInspectable *);
	END_INTERFACE
} raven_ui_command_vtbl;
struct raven_ui_command { const raven_ui_command_vtbl *lpVtbl; };
DEFINE_GUID(IID_RavenIUICommand, 0x4ff93a75, 0x4145, 0x47ff, 0xac, 0x7f, 0xdf, 0xf1, 0xc1, 0xfa, 0x5b, 0x0f);

HRESULT raven_message_dialog_factory(HSTRING name, IActivationFactory **out);
#endif
