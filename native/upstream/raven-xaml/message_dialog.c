#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include "windows_ui_popups.h"
#include <winstring.h>
#include <stddef.h>

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))
#ifndef E_ILLEGAL_DELEGATE_ASSIGNMENT
#define E_ILLEGAL_DELEGATE_ASSIGNMENT ((HRESULT)0x80000018)
#endif
#ifndef RO_E_CLOSED
#define RO_E_CLOSED ((HRESULT)0x80000013)
#endif
static const WCHAR class_name[] = L"Windows.UI.Popups.MessageDialog";
static const WCHAR operation_name[] = L"Windows.Foundation.IAsyncOperation`1<Windows.UI.Popups.IUICommand>";
/* IID for the parameterized IAsyncOperation<Windows.UI.Popups.IUICommand> interface.
 * The old value was unrelated to this instantiation, so await's cast rejected the
 * returned operation and crashed in the managed native runtime. */
static const IID operation_iid = {0xb8770535, 0x6a4b, 0x52b1, {0xb5, 0x78, 0xf3, 0xcd, 0xc5, 0x00, 0x7a, 0x1f}};

typedef struct message_operation message_operation;
static int same_iid(REFIID, const IID *);
static HRESULT empty_iids(ULONG *, IID **);
static HRESULT trust(TrustLevel *);
static HRESULT runtime_name(HSTRING *, const WCHAR *);
static void handler_release(IUnknown *);
static void handler_addref(IUnknown *);
typedef struct ui_command {
	raven_ui_command_i iface;
	LONG refs;
	HSTRING label;
	IInspectable *id;
	IUnknown *invoked;
} ui_command;

static ui_command *command_from(raven_ui_command_i *i) { return CONTAINING_RECORD(i, ui_command, iface); }
static HRESULT WINAPI command_query(raven_ui_command_i *i, REFIID iid, void **out) {
	ui_command *self = command_from(i); if (!out) return E_POINTER; *out = NULL;
	if (same_iid(iid, &IID_IUnknown) || same_iid(iid, &IID_IInspectable) || same_iid(iid, &IID_IAgileObject) || same_iid(iid, &IID_RavenIUICommand)) *out = i;
	else return E_NOINTERFACE;
	InterlockedIncrement(&self->refs); return S_OK;
}
static ULONG WINAPI command_addref(raven_ui_command_i *i) { return (ULONG)InterlockedIncrement(&command_from(i)->refs); }
static ULONG WINAPI command_release(raven_ui_command_i *i) {
	ui_command *self = command_from(i); ULONG refs = (ULONG)InterlockedDecrement(&self->refs);
	if (!refs) { WindowsDeleteString(self->label); if (self->id) self->id->lpVtbl->Release(self->id); handler_release(self->invoked); FREE(self); }
	return refs;
}
static HRESULT WINAPI command_iids(raven_ui_command_i *i, ULONG *c, IID **ids) { (void)i; return empty_iids(c, ids); }
static HRESULT WINAPI command_name(raven_ui_command_i *i, HSTRING *out) { (void)i; return runtime_name(out, L"Windows.UI.Popups.UICommand"); }
static HRESULT WINAPI command_trust(raven_ui_command_i *i, TrustLevel *out) { (void)i; return trust(out); }
static HRESULT WINAPI command_get_label(raven_ui_command_i *i, HSTRING *out) { if (!out) return E_POINTER; *out = NULL; return WindowsDuplicateString(command_from(i)->label, out); }
static HRESULT WINAPI command_put_label(raven_ui_command_i *i, HSTRING value) { ui_command *self = command_from(i); WindowsDeleteString(self->label); return WindowsDuplicateString(value, &self->label); }
static HRESULT WINAPI command_get_invoked(raven_ui_command_i *i, IUnknown **out) { if (!out) return E_POINTER; *out = command_from(i)->invoked; handler_addref(*out); return S_OK; }
static HRESULT WINAPI command_put_invoked(raven_ui_command_i *i, IUnknown *value) { ui_command *self = command_from(i); handler_release(self->invoked); self->invoked = value; handler_addref(value); return S_OK; }
static HRESULT WINAPI command_get_id(raven_ui_command_i *i, IInspectable **out) { if (!out) return E_POINTER; *out = command_from(i)->id; if (*out) (*out)->lpVtbl->AddRef(*out); return S_OK; }
static HRESULT WINAPI command_put_id(raven_ui_command_i *i, IInspectable *value) { ui_command *self = command_from(i); if (self->id) self->id->lpVtbl->Release(self->id); self->id = value; if (self->id) self->id->lpVtbl->AddRef(self->id); return S_OK; }
static const raven_ui_command_vtbl command_table = { command_query, command_addref, command_release, command_iids, command_name, command_trust, command_get_label, command_put_label, command_get_invoked, command_put_invoked, command_get_id, command_put_id };
static ui_command *new_command(void) {
	ui_command *command = ALLOC(sizeof *command); if (!command) return NULL;
	command->iface.lpVtbl = &command_table; command->refs = 1;
	if (FAILED(WindowsCreateString(L"Close", 5, &command->label))) { FREE(command); return NULL; }
	return command;
}
typedef struct message_operation_vtbl {
	IInspectableVtbl base;
	HRESULT (STDMETHODCALLTYPE *put_Completed)(message_operation *, IUnknown *);
	HRESULT (STDMETHODCALLTYPE *get_Completed)(message_operation *, IUnknown **);
	HRESULT (STDMETHODCALLTYPE *GetResults)(message_operation *, IInspectable **);
} message_operation_vtbl;
struct message_operation { const message_operation_vtbl *lpVtbl; IAsyncInfo info; LONG refs; IUnknown *completed; IInspectable *result; BOOL closed; };

static int same_iid(REFIID a, const IID *b) { return IsEqualIID(a, b); }
static HRESULT empty_iids(ULONG *count, IID **iids) { if (!count || !iids) return E_POINTER; *count = 0; *iids = NULL; return S_OK; }
static HRESULT trust(TrustLevel *level) { if (!level) return E_POINTER; *level = BaseTrust; return S_OK; }
static HRESULT runtime_name(HSTRING *out, const WCHAR *text) { if (!out) return E_POINTER; return WindowsCreateString(text, (UINT32)lstrlenW(text), out); }
static void handler_release(IUnknown *handler) { if (handler) handler->lpVtbl->Release(handler); }
static void handler_addref(IUnknown *handler) { if (handler) handler->lpVtbl->AddRef(handler); }
static void trace_dialog_text(const char *label, HSTRING text) {
	char line[520];
	const WCHAR *wide;
	UINT32 length = 0, i;
	int at = wsprintfA(line, "raven-xaml: MessageDialog %s: ", label);
	wide = text ? WindowsGetStringRawBuffer(text, &length) : L"";
	for (i = 0; i < length && at < (int)sizeof(line) - 3; ++i)
		line[at++] = wide[i] >= 0x20 && wide[i] < 0x7f ? (char)wide[i] : '?';
	line[at++] = '\n'; line[at] = 0;
	raven_trace_note(line);
}

static message_operation *op_from_info(IAsyncInfo *info) { return CONTAINING_RECORD(info, message_operation, info); }
static HRESULT WINAPI op_query(void *iface, REFIID iid, void **out) {
	message_operation *self = (message_operation *)iface; if (!out) return E_POINTER; *out = NULL;
	raven_trace_note("raven-xaml: MessageDialog operation QueryInterface\n");
	if (same_iid(iid, &IID_IUnknown) || same_iid(iid, &IID_IInspectable) || same_iid(iid, &IID_IAgileObject) || same_iid(iid, &operation_iid)) *out = (IInspectable *)self;
	else if (same_iid(iid, &IID_IAsyncInfo)) *out = &self->info;
	else return E_NOINTERFACE;
	InterlockedIncrement(&self->refs); return S_OK;
}
static ULONG WINAPI op_addref(void *iface) { return (ULONG)InterlockedIncrement(&((message_operation *)iface)->refs); }
static ULONG WINAPI op_release(void *iface) { message_operation *self = (message_operation *)iface; ULONG refs = (ULONG)InterlockedDecrement(&self->refs); if (!refs) { handler_release(self->completed); if (self->result) self->result->lpVtbl->Release(self->result); FREE(self); } return refs; }
static HRESULT WINAPI op_iids(void *iface, ULONG *c, IID **i) { (void)iface; return empty_iids(c, i); }
static HRESULT WINAPI op_name(void *iface, HSTRING *out) { (void)iface; return runtime_name(out, operation_name); }
static HRESULT WINAPI op_trust(void *iface, TrustLevel *out) { (void)iface; return trust(out); }
static HRESULT WINAPI info_query(IAsyncInfo *i, REFIID id, void **out) { return op_query(op_from_info(i), id, out); }
static ULONG WINAPI info_addref(IAsyncInfo *i) { return op_addref(op_from_info(i)); }
static ULONG WINAPI info_release(IAsyncInfo *i) { return op_release(op_from_info(i)); }
static HRESULT WINAPI info_iids(IAsyncInfo *i, ULONG *c, IID **ids) { return op_iids(i, c, ids); }
static HRESULT WINAPI info_name(IAsyncInfo *i, HSTRING *out) { return op_name(i, out); }
static HRESULT WINAPI info_trust(IAsyncInfo *i, TrustLevel *out) { return op_trust(i, out); }
static HRESULT WINAPI info_id(IAsyncInfo *i, UINT32 *out) { if (!out) return E_POINTER; *out = 1; return S_OK; }
static HRESULT WINAPI info_status(IAsyncInfo *i, AsyncStatus *out) { raven_trace_note("raven-xaml: MessageDialog operation Status\n"); if (!out) return E_POINTER; *out = op_from_info(i)->closed ? Canceled : Completed; return S_OK; }
static HRESULT WINAPI info_error(IAsyncInfo *i, HRESULT *out) { if (!out) return E_POINTER; *out = S_OK; return S_OK; }
static HRESULT WINAPI info_cancel(IAsyncInfo *i) { return S_OK; }
static HRESULT WINAPI info_close(IAsyncInfo *i) { op_from_info(i)->closed = TRUE; return S_OK; }
static const IAsyncInfoVtbl info_table = { info_query, info_addref, info_release, info_iids, info_name, info_trust, info_id, info_status, info_error, info_cancel, info_close };
static HRESULT WINAPI op_put_completed(message_operation *self, IUnknown *handler) { raven_trace_note("raven-xaml: MessageDialog operation put_Completed\n"); if (self->completed) return E_ILLEGAL_DELEGATE_ASSIGNMENT; self->completed = handler; handler_addref(handler); if (handler) { void **v = *(void ***)handler; ((HRESULT (WINAPI *)(void *, void *, AsyncStatus))v[3])(handler, self, Completed); } return S_OK; }
static HRESULT WINAPI op_get_completed(message_operation *self, IUnknown **handler) { if (!handler) return E_POINTER; *handler = self->completed; handler_addref(*handler); return S_OK; }
static HRESULT WINAPI op_results(message_operation *self, IInspectable **result) { raven_trace_note("raven-xaml: MessageDialog operation GetResults\n"); if (!result) return E_POINTER; *result = NULL; if (self->closed) return RO_E_CLOSED; if (self->result) { *result = self->result; self->result->lpVtbl->AddRef(self->result); } return S_OK; }
static const message_operation_vtbl op_table = { { (void *)op_query, (void *)op_addref, (void *)op_release,
	(void *)op_iids, (void *)op_name, (void *)op_trust }, op_put_completed, op_get_completed, op_results };
static message_operation *new_operation(ui_command *command) { message_operation *op = ALLOC(sizeof *op); if (!op) return NULL; op->lpVtbl = &op_table; op->info.lpVtbl = &info_table; op->refs = 1; if (command) { op->result = (IInspectable *)&command->iface; command->iface.lpVtbl->AddRef(&command->iface); } return op; }

typedef struct message_dialog { raven_message_dialog_i iface; LONG refs; HSTRING content, title; UINT32 default_index, cancel_index, options; } message_dialog;
static message_dialog *dialog_from(raven_message_dialog_i *i) { return CONTAINING_RECORD(i, message_dialog, iface); }
static HRESULT WINAPI dialog_query(raven_message_dialog_i *i, REFIID id, void **out) { message_dialog *d = dialog_from(i); if (!out) return E_POINTER; *out = NULL; if (same_iid(id, &IID_IUnknown) || same_iid(id, &IID_IInspectable) || same_iid(id, &IID_RavenIMessageDialog)) *out = i; else return E_NOINTERFACE; InterlockedIncrement(&d->refs); return S_OK; }
static ULONG WINAPI dialog_addref(raven_message_dialog_i *i) { return (ULONG)InterlockedIncrement(&dialog_from(i)->refs); }
static ULONG WINAPI dialog_release(raven_message_dialog_i *i) { message_dialog *d = dialog_from(i); ULONG refs = (ULONG)InterlockedDecrement(&d->refs); if (!refs) { WindowsDeleteString(d->content); WindowsDeleteString(d->title); FREE(d); } return refs; }
static HRESULT WINAPI dialog_iids(raven_message_dialog_i *i, ULONG *c, IID **ids) { return empty_iids(c, ids); }
static HRESULT WINAPI dialog_name(raven_message_dialog_i *i, HSTRING *out) { return runtime_name(out, class_name); }
static HRESULT WINAPI dialog_trust(raven_message_dialog_i *i, TrustLevel *out) { return trust(out); }
#define TEXT_GET(field) static HRESULT WINAPI dialog_get_##field(raven_message_dialog_i *i, HSTRING *out) { if (!out) return E_POINTER; *out = dialog_from(i)->field; if (*out) WindowsDuplicateString(*out, out); return S_OK; }
#define TEXT_PUT(field) static HRESULT WINAPI dialog_put_##field(raven_message_dialog_i *i, HSTRING value) { message_dialog *d = dialog_from(i); WindowsDeleteString(d->field); return WindowsDuplicateString(value, &d->field); }
TEXT_GET(title) TEXT_PUT(title) TEXT_GET(content) TEXT_PUT(content)
static HRESULT WINAPI dialog_commands(raven_message_dialog_i *i, IInspectable **out) { if (!out) return E_POINTER; *out = NULL; return E_NOTIMPL; }
static HRESULT WINAPI dialog_get_default(raven_message_dialog_i *i, UINT32 *out) { if (!out) return E_POINTER; *out = dialog_from(i)->default_index; return S_OK; }
static HRESULT WINAPI dialog_put_default(raven_message_dialog_i *i, UINT32 v) { dialog_from(i)->default_index = v; return S_OK; }
static HRESULT WINAPI dialog_get_cancel(raven_message_dialog_i *i, UINT32 *out) { if (!out) return E_POINTER; *out = dialog_from(i)->cancel_index; return S_OK; }
static HRESULT WINAPI dialog_put_cancel(raven_message_dialog_i *i, UINT32 v) { dialog_from(i)->cancel_index = v; return S_OK; }
static HRESULT WINAPI dialog_show(raven_message_dialog_i *i, IInspectable **out) { message_operation *op; message_dialog *dialog = dialog_from(i); if (!out) return E_POINTER; *out = NULL; trace_dialog_text("Title", dialog->title); trace_dialog_text("Content", dialog->content); op = new_operation(NULL); if (!op) return E_OUTOFMEMORY; *out = (IInspectable *)op; raven_trace_note("raven-xaml: MessageDialog.ShowAsync -> completed without selected command (headless)\n"); return S_OK; }
static HRESULT WINAPI dialog_get_options(raven_message_dialog_i *i, UINT32 *out) { if (!out) return E_POINTER; *out = dialog_from(i)->options; return S_OK; }
static HRESULT WINAPI dialog_put_options(raven_message_dialog_i *i, UINT32 v) { dialog_from(i)->options = v; return S_OK; }
static const raven_message_dialog_vtbl dialog_table = { dialog_query, dialog_addref, dialog_release, dialog_iids, dialog_name, dialog_trust, dialog_get_title, dialog_put_title, dialog_commands, dialog_get_default, dialog_put_default, dialog_get_cancel, dialog_put_cancel, dialog_get_content, dialog_put_content, dialog_show, dialog_get_options, dialog_put_options };
static HRESULT new_dialog(HSTRING content, HSTRING title, IInspectable **out) { message_dialog *d; if (!out) return E_POINTER; *out = NULL; d = ALLOC(sizeof *d); if (!d) return E_OUTOFMEMORY; d->iface.lpVtbl = &dialog_table; d->refs = 1; WindowsDuplicateString(content, &d->content); WindowsDuplicateString(title, &d->title); *out = (IInspectable *)&d->iface; return S_OK; }
static ULONG WINAPI factory_addref(raven_message_dialog_factory_i *i) { return 2; }
static ULONG WINAPI factory_release(raven_message_dialog_factory_i *i) { return 1; }
static HRESULT WINAPI factory_iids(raven_message_dialog_factory_i *i, ULONG *c, IID **ids) { return empty_iids(c, ids); }
static HRESULT WINAPI factory_name(raven_message_dialog_factory_i *i, HSTRING *out) { return runtime_name(out, class_name); }
static HRESULT WINAPI factory_trust(raven_message_dialog_factory_i *i, TrustLevel *out) { return trust(out); }
static HRESULT WINAPI factory_create(raven_message_dialog_factory_i *i, HSTRING content, IInspectable **out) { return new_dialog(content, NULL, out); }
static HRESULT WINAPI factory_create_title(raven_message_dialog_factory_i *i, HSTRING content, HSTRING title, IInspectable **out) { return new_dialog(content, title, out); }
typedef struct message_factory { IActivationFactory activation; raven_message_dialog_factory_i factory; } message_factory;
static HRESULT WINAPI factory_query(raven_message_dialog_factory_i *i, REFIID id, void **out) { message_factory *f = CONTAINING_RECORD(i, message_factory, factory); if (!out) return E_POINTER; *out = NULL; if (same_iid(id, &IID_IUnknown) || same_iid(id, &IID_IInspectable) || same_iid(id, &IID_RavenIMessageDialogFactory)) *out = i; else if (same_iid(id, &IID_IActivationFactory)) *out = &f->activation; else return E_NOINTERFACE; ((IUnknown *)*out)->lpVtbl->AddRef((IUnknown *)*out); return S_OK; }
static const raven_message_dialog_factory_vtbl factory_table = { factory_query, factory_addref, factory_release, factory_iids, factory_name, factory_trust, factory_create, factory_create_title };
static HRESULT WINAPI activation_query(IActivationFactory *i, REFIID id, void **out) { return factory_query(&CONTAINING_RECORD(i, message_factory, activation)->factory, id, out); }
static ULONG WINAPI activation_addref(IActivationFactory *i) { return factory_addref(&CONTAINING_RECORD(i, message_factory, activation)->factory); }
static ULONG WINAPI activation_release(IActivationFactory *i) { return factory_release(&CONTAINING_RECORD(i, message_factory, activation)->factory); }
static HRESULT WINAPI activation_iids(IActivationFactory *i, ULONG *c, IID **ids) { return empty_iids(c, ids); }
static HRESULT WINAPI activation_name(IActivationFactory *i, HSTRING *out) { return runtime_name(out, class_name); }
static HRESULT WINAPI activation_trust(IActivationFactory *i, TrustLevel *out) { return trust(out); }
static HRESULT WINAPI activation_activate(IActivationFactory *i, IInspectable **out) { return factory_create(&CONTAINING_RECORD(i, message_factory, activation)->factory, NULL, out); }
static const IActivationFactoryVtbl activation_table = { activation_query, activation_addref, activation_release, activation_iids, activation_name, activation_trust, activation_activate };
static message_factory factory_instance = { { &activation_table }, { &factory_table } };
HRESULT raven_message_dialog_factory(HSTRING name, IActivationFactory **out) { UINT32 n; const WCHAR *s; if (!out) return E_POINTER; *out = NULL; s = WindowsGetStringRawBuffer(name, &n); if (n != ARRAYSIZE(class_name)-1 || CompareStringOrdinal(s, (int)n, class_name, (int)(ARRAYSIZE(class_name)-1), FALSE) != CSTR_EQUAL) return CLASS_E_CLASSNOTAVAILABLE; *out = &factory_instance.activation; return S_OK; }
