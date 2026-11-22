/* SPDX-License-Identifier: MIT
 * The IContentDialogFactory ABI (Windows.UI.Xaml.winmd): the composable creation the .NET Native projection
 * drives with RoGetActivationFactory(name, IContentDialogFactory) — the traced launch asks the factory for
 * exactly this IID. The dialog instance's own face joins when its cast IID is on record (the frame's refusal
 * trace names it in full). */
#ifndef RAVEN_CONTENTDIALOG_H
#define RAVEN_CONTENTDIALOG_H
#include <inspectable.h>
#include <asyncinfo.h>

typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialogFactory contentdialog_factory_i;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialogFactoryVtbl contentdialog_factory_vtbl;
struct __x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialogFactoryVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(contentdialog_factory_i *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(contentdialog_factory_i *This);
	ULONG (STDMETHODCALLTYPE *Release)(contentdialog_factory_i *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(contentdialog_factory_i *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(contentdialog_factory_i *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(contentdialog_factory_i *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *CreateInstance)(contentdialog_factory_i *This, uint64_t outer, uint64_t inner, uint64_t instance);
	END_INTERFACE
};
interface __x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialogFactory { CONST_VTBL contentdialog_factory_vtbl *lpVtbl; };
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialogFactory,
	0x05557178, 0x9d8e, 0x4315, 0xb3, 0x7d, 0x68, 0x0c, 0x14, 0x01, 0x2c, 0x35);

/* The dialog instance's own faces, the IIDs the traced launch's cast walk asked and the frame's refusal trace
 * named (38dc4404-... for IContentDialog — the unnamed EETypeRva the InvalidCastException wall carried):
 * IContentDialog is the projected base-class face every ContentDialog-derived app class's marshal composes,
 * IContentDialog2 the CloseButtonText/DefaultButton continuation. The member layout follows the winmd
 * declaration order; the event handlers arrive as CCWs and the tokens pack as the documented int64 slots. */
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialog contentdialog_i;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialogVtbl contentdialog_vtbl;
struct __x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialogVtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(contentdialog_i *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(contentdialog_i *This);
	ULONG (STDMETHODCALLTYPE *Release)(contentdialog_i *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(contentdialog_i *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(contentdialog_i *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(contentdialog_i *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_Title)(contentdialog_i *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *put_Title)(contentdialog_i *This, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *get_TitleTemplate)(contentdialog_i *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *put_TitleTemplate)(contentdialog_i *This, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *get_FullSizeDesired)(contentdialog_i *This, boolean *value);
	HRESULT (STDMETHODCALLTYPE *put_FullSizeDesired)(contentdialog_i *This, boolean value);
	HRESULT (STDMETHODCALLTYPE *get_PrimaryButtonText)(contentdialog_i *This, HSTRING *value);
	HRESULT (STDMETHODCALLTYPE *put_PrimaryButtonText)(contentdialog_i *This, HSTRING value);
	HRESULT (STDMETHODCALLTYPE *get_SecondaryButtonText)(contentdialog_i *This, HSTRING *value);
	HRESULT (STDMETHODCALLTYPE *put_SecondaryButtonText)(contentdialog_i *This, HSTRING value);
	HRESULT (STDMETHODCALLTYPE *get_PrimaryButtonCommand)(contentdialog_i *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *put_PrimaryButtonCommand)(contentdialog_i *This, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *get_SecondaryButtonCommand)(contentdialog_i *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *put_SecondaryButtonCommand)(contentdialog_i *This, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *get_PrimaryButtonCommandParameter)(contentdialog_i *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *put_PrimaryButtonCommandParameter)(contentdialog_i *This, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *get_SecondaryButtonCommandParameter)(contentdialog_i *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *put_SecondaryButtonCommandParameter)(contentdialog_i *This, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *get_IsPrimaryButtonEnabled)(contentdialog_i *This, boolean *value);
	HRESULT (STDMETHODCALLTYPE *put_IsPrimaryButtonEnabled)(contentdialog_i *This, boolean value);
	HRESULT (STDMETHODCALLTYPE *get_IsSecondaryButtonEnabled)(contentdialog_i *This, boolean *value);
	HRESULT (STDMETHODCALLTYPE *put_IsSecondaryButtonEnabled)(contentdialog_i *This, boolean value);
	HRESULT (STDMETHODCALLTYPE *add_Closing)(contentdialog_i *This, void *handler, EventRegistrationToken *token);
	HRESULT (STDMETHODCALLTYPE *remove_Closing)(contentdialog_i *This, EventRegistrationToken token);
	HRESULT (STDMETHODCALLTYPE *add_Closed)(contentdialog_i *This, void *handler, EventRegistrationToken *token);
	HRESULT (STDMETHODCALLTYPE *remove_Closed)(contentdialog_i *This, EventRegistrationToken token);
	HRESULT (STDMETHODCALLTYPE *add_Opened)(contentdialog_i *This, void *handler, EventRegistrationToken *token);
	HRESULT (STDMETHODCALLTYPE *remove_Opened)(contentdialog_i *This, EventRegistrationToken token);
	HRESULT (STDMETHODCALLTYPE *add_PrimaryButtonClick)(contentdialog_i *This, void *handler, EventRegistrationToken *token);
	HRESULT (STDMETHODCALLTYPE *remove_PrimaryButtonClick)(contentdialog_i *This, EventRegistrationToken token);
	HRESULT (STDMETHODCALLTYPE *add_SecondaryButtonClick)(contentdialog_i *This, void *handler, EventRegistrationToken *token);
	HRESULT (STDMETHODCALLTYPE *remove_SecondaryButtonClick)(contentdialog_i *This, EventRegistrationToken token);
	HRESULT (STDMETHODCALLTYPE *Hide)(contentdialog_i *This);
	HRESULT (STDMETHODCALLTYPE *ShowAsync)(contentdialog_i *This, IInspectable **operation);
	END_INTERFACE
};
interface __x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialog { CONST_VTBL contentdialog_vtbl *lpVtbl; };
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialog,
	0x38dc4404, 0xd24e, 0x40d8, 0x94, 0x15, 0x34, 0x94, 0x64, 0xc1, 0xaf, 0xdc);

typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialog2 contentdialog2_i;
typedef struct __x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialog2Vtbl contentdialog2_vtbl;
struct __x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialog2Vtbl {
	BEGIN_INTERFACE
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(contentdialog2_i *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(contentdialog2_i *This);
	ULONG (STDMETHODCALLTYPE *Release)(contentdialog2_i *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(contentdialog2_i *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(contentdialog2_i *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(contentdialog2_i *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *get_CloseButtonText)(contentdialog2_i *This, HSTRING *value);
	HRESULT (STDMETHODCALLTYPE *put_CloseButtonText)(contentdialog2_i *This, HSTRING value);
	HRESULT (STDMETHODCALLTYPE *get_CloseButtonCommand)(contentdialog2_i *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *put_CloseButtonCommand)(contentdialog2_i *This, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *get_CloseButtonCommandParameter)(contentdialog2_i *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *put_CloseButtonCommandParameter)(contentdialog2_i *This, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *get_PrimaryButtonStyle)(contentdialog2_i *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *put_PrimaryButtonStyle)(contentdialog2_i *This, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *get_SecondaryButtonStyle)(contentdialog2_i *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *put_SecondaryButtonStyle)(contentdialog2_i *This, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *get_CloseButtonStyle)(contentdialog2_i *This, IInspectable **value);
	HRESULT (STDMETHODCALLTYPE *put_CloseButtonStyle)(contentdialog2_i *This, IInspectable *value);
	HRESULT (STDMETHODCALLTYPE *get_DefaultButton)(contentdialog2_i *This, INT32 *value);
	HRESULT (STDMETHODCALLTYPE *put_DefaultButton)(contentdialog2_i *This, INT32 value);
	HRESULT (STDMETHODCALLTYPE *add_CloseButtonClick)(contentdialog2_i *This, void *handler, EventRegistrationToken *token);
	HRESULT (STDMETHODCALLTYPE *remove_CloseButtonClick)(contentdialog2_i *This, EventRegistrationToken token);
	END_INTERFACE
};
interface __x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialog2 { CONST_VTBL contentdialog2_vtbl *lpVtbl; };
DEFINE_GUID(IID___x_ABI_CWindows_CUI_CXaml_CControls_CIContentDialog2,
	0x2f93eb45, 0xee43, 0x4303, 0x9b, 0x38, 0x3f, 0xe1, 0xa1, 0x11, 0xec, 0xbf);

/* ContentDialogResult, the enum the ShowAsync operation completes with (Microsoft Learn winrt reference,
 * Windows.UI.Xaml.Controls.ContentDialogResult: None 0 "No button was tapped", Primary 1, Secondary 2). The
 * ABI value is the int32 the parameterized operation's GetResults hands out. */
enum {
	contentdialog_result_none = 0,
	contentdialog_result_primary = 1,
	contentdialog_result_secondary = 2,
};

/* ---- The parameterized IAsyncOperation`1<ContentDialogResult> the ShowAsync return projects, stamped from the
 * one shape the display header's RAVEN_DISPLAY_ASYNC_OPERATION takes. The completed handler is a delegate object
 * the guest owns: the operation invokes and releases it and no face of raven ever queries one, so only the shape
 * matters. GetResults carries the enum by value (an int32 out), not an interface pointer. ---- */
#define RAVEN_CONTENTDIALOG_ASYNC_OPERATION(name, result_type) \
	typedef struct __FIAsyncOperationCompletedHandler_1_##name __FIAsyncOperationCompletedHandler_1_##name; \
	typedef struct __FIAsyncOperation_1_##name __FIAsyncOperation_1_##name; \
	typedef struct __FIAsyncOperation_1_##name##Vtbl { \
		BEGIN_INTERFACE \
		HRESULT (STDMETHODCALLTYPE *QueryInterface)(__FIAsyncOperation_1_##name *This, REFIID riid, void **object); \
		ULONG (STDMETHODCALLTYPE *AddRef)(__FIAsyncOperation_1_##name *This); \
		ULONG (STDMETHODCALLTYPE *Release)(__FIAsyncOperation_1_##name *This); \
		HRESULT (STDMETHODCALLTYPE *GetIids)(__FIAsyncOperation_1_##name *This, ULONG *count, IID **iids); \
		HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(__FIAsyncOperation_1_##name *This, HSTRING *type_name); \
		HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(__FIAsyncOperation_1_##name *This, TrustLevel *level); \
		HRESULT (STDMETHODCALLTYPE *put_Completed)(__FIAsyncOperation_1_##name *This, __FIAsyncOperationCompletedHandler_1_##name *handler); \
		HRESULT (STDMETHODCALLTYPE *get_Completed)(__FIAsyncOperation_1_##name *This, __FIAsyncOperationCompletedHandler_1_##name **handler); \
		HRESULT (STDMETHODCALLTYPE *GetResults)(__FIAsyncOperation_1_##name *This, result_type results); \
		END_INTERFACE \
	} __FIAsyncOperation_1_##name##Vtbl; \
	interface __FIAsyncOperation_1_##name { \
		CONST_VTBL __FIAsyncOperation_1_##name##Vtbl *lpVtbl; \
	}; \
	typedef struct __FIAsyncOperationCompletedHandler_1_##name##Vtbl { \
		HRESULT (STDMETHODCALLTYPE *QueryInterface)(__FIAsyncOperationCompletedHandler_1_##name *This, REFIID riid, void **object); \
		ULONG (STDMETHODCALLTYPE *AddRef)(__FIAsyncOperationCompletedHandler_1_##name *This); \
		ULONG (STDMETHODCALLTYPE *Release)(__FIAsyncOperationCompletedHandler_1_##name *This); \
		HRESULT (STDMETHODCALLTYPE *Invoke)(__FIAsyncOperationCompletedHandler_1_##name *This, __FIAsyncOperation_1_##name *operation, AsyncStatus status); \
	} __FIAsyncOperationCompletedHandler_1_##name##Vtbl; \
	struct __FIAsyncOperationCompletedHandler_1_##name { \
		CONST_VTBL __FIAsyncOperationCompletedHandler_1_##name##Vtbl *lpVtbl; \
	}; \

RAVEN_CONTENTDIALOG_ASYNC_OPERATION(Windows__CUI__CXaml__CControls__CContentDialogResult, INT32 *)

/* The instance identifiers, the RFC 4122 version-5 UUID (SHA-1) of the pinterface signature under the namespace
 * 11f47ad5-7b73-42c0-abae-878b1e16adee over the IAsyncOperation`1 template 9fc2b0bb-e446-44e2-aa61-9cab-8f636af2
 * (the computation windows_devices_display.h documents and the imaging round reproduced against mingw's precomputed
 * instances). The argument encoding the metadata gives an enum is enum(<type>;i4), so the first identifier —
 * pinterface({9fc2b0bb-...};enum(Windows.UI.Xaml.Controls.ContentDialogResult;i4)) = 1f23bdd1-06dc-5be9-9a60-
 * 0b4d94d4d72c — is the face the await casts, and the guest's own image carries it: Shapr3D.dll embeds it as its
 * ShowAsync projection constant (verified by byte search), with the completed-handler instance of the same argument
 * (45c7a306-..., the same computation over the fcdcf02c-e5d8-4478-915a-4d90b74b83a5 handler template) adjacent.
 * The alternate is the rc-form a class-encoding projection would query — the metadata declares no IContentDialogResult
 * interface (an enum carries none, verified against the wine idl and the winmd shape), so its rc() argument takes the
 * instance identifier itself. Both name the same face; an unseen query traces and refuses by name. */
DEFINE_GUID(IID___FIAsyncOperation_1_Windows__CUI__CXaml__CControls__CContentDialogResult,
	0x1f23bdd1, 0x06dc, 0x5be9, 0x9a, 0x60, 0x0b, 0x4d, 0x94, 0xd4, 0xd7, 0x2c);
DEFINE_GUID(IID___FIAsyncOperation_1_Windows__CUI__CXaml__CControls__CContentDialogResult_rcvariant,
	0xd5476170, 0x5fa4, 0x5660, 0xab, 0xec, 0x08, 0xaf, 0xbc, 0x01, 0x68, 0x58);
#endif
