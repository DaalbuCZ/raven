/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* The automation fixture: a compiled page hosts a grid whose three children are a named TextBlock, a
 * nameless one and a nameless grid, and a Button named Go, and the client works through the
 * FrameworkElementAutomationPeer statics the winmd declares: a second CreatePeerForElement for an
 * element returns the same peer, the Owner the peer reports is the element, GetName answers the
 * loader-assigned name and falls back to the Text of a nameless TextBlock and to the runtime class
 * name of a nameless grid, GetPattern answers E_NOTIMPL for every PatternInterface on a text peer,
 * the button's peer serves the Invoke provider whose Invoke raises the Click a handler on IButtonBase
 * receives, the peer reports itself a content and a control element, and two elements get different
 * peers. The registry holds one reference to each peer for the process, so the fixture releases
 * everything it borrowed and lets the registry entries stay. */
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_Foundation
#include <windows.h>
#include <winstring.h>
#include <activation.h>
#include <roapi.h>
#include <windows.ui.core.h>
#include "../../native/upstream/raven-xaml/corewindow_interop.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_elements.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_window.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_uielement.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_automation_peers.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_automation_provider.h"
#include "../../native/upstream/raven-xaml/windows_ui_xaml_buttonbase.h"

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);
typedef HRESULT (WINAPI *load_function)(const void *, uint32_t, IInspectable **);

typedef __x_ABI_CWindows_CUI_CXaml_CControls_CIPanel panel1;
typedef __x_ABI_CWindows_CUI_CXaml_CIUIElement ui_element1;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindow window1;
typedef __x_ABI_CWindows_CUI_CXaml_CIWindowStatics window_statics;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer automation1;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeer framework_peer1;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics peer_statics1;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerFactory peer_factory1;

/* The IVector<UIElement> the panel's Children returns: the parameterized ABI has no mingw header, so
 * the slots the fixture reads are declared locally in metadata order after the six IInspectable methods. */
struct fixture_vector_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(void *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(void *This);
	ULONG (STDMETHODCALLTYPE *Release)(void *This);
	HRESULT (STDMETHODCALLTYPE *GetIids)(void *This, ULONG *count, IID **iids);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(void *This, HSTRING *name);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(void *This, TrustLevel *level);
	HRESULT (STDMETHODCALLTYPE *GetAt)(void *This, UINT32 index, ui_element1 **value);
	HRESULT (STDMETHODCALLTYPE *get_Size)(void *This, UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *GetView)(void *This, void **value);
	HRESULT (STDMETHODCALLTYPE *IndexOf)(void *This, ui_element1 *element, UINT32 *index, boolean *value);
	HRESULT (STDMETHODCALLTYPE *SetAt)(void *This, UINT32 index, ui_element1 *value);
	HRESULT (STDMETHODCALLTYPE *InsertAt)(void *This, UINT32 index, ui_element1 *value);
	HRESULT (STDMETHODCALLTYPE *RemoveAt)(void *This, UINT32 index);
	HRESULT (STDMETHODCALLTYPE *Append)(void *This, ui_element1 *value);
	HRESULT (STDMETHODCALLTYPE *RemoveAtEnd)(void *This);
	HRESULT (STDMETHODCALLTYPE *Clear)(void *This);
	HRESULT (STDMETHODCALLTYPE *GetMany)(void *This, UINT32 start_index, UINT32 items_size, ui_element1 **items,
		UINT32 *value);
	HRESULT (STDMETHODCALLTYPE *ReplaceAll)(void *This, UINT32 count, ui_element1 **items);
};

struct fixture_vector {
	const struct fixture_vector_vtbl *lpVtbl;
};

static void put16(unsigned char *doc, size_t at, UINT16 value)
{
	doc[at] = (unsigned char)(value & 0xff);
	doc[at + 1] = (unsigned char)(value >> 8);
}

static void put32(unsigned char *doc, size_t at, UINT32 value)
{
	doc[at] = (unsigned char)value;
	doc[at + 1] = (unsigned char)(value >> 8);
	doc[at + 2] = (unsigned char)(value >> 16);
	doc[at + 3] = (unsigned char)(value >> 24);
}

static void put64(unsigned char *doc, size_t at, UINT64 value)
{
	for (int index = 0; index < 8; ++index)
		doc[at + index] = (unsigned char)(value >> (index * 8));
}

/* The empty metadata section every fixture shares, with the substream table for one node stream. */
static size_t begin_document(unsigned char *doc)
{
	for (size_t index = 0; index < 1024; ++index)
		doc[index] = 0;
	doc[0] = 'X'; doc[1] = 'B'; doc[2] = 'F';
	put32(doc, 4, 144);
	put32(doc, 12, 2);
	put32(doc, 16, 1);
	put64(doc, 20, 0x78);
	put64(doc, 28, 0x7c);
	put64(doc, 36, 0x80);
	put64(doc, 44, 0x84);
	put64(doc, 52, 0x88);
	put64(doc, 60, 0x8c);
	put32(doc, 156, 1);
	put32(doc, 160, 0);
	return 168;
}

static size_t create_type(unsigned char *doc, size_t at, UINT16 stable)
{
	doc[at++] = 0x14;
	put16(doc, at, (UINT16)(stable | 0x8000));
	return at + 2;
}

static size_t set_property(unsigned char *doc, size_t at, UINT16 stable)
{
	doc[at++] = 0x07;
	put16(doc, at, (UINT16)(stable | 0x8000));
	return at + 2;
}

static size_t set_string(unsigned char *doc, size_t at, UINT16 stable, const char *text)
{
	doc[at++] = 0x1a;
	put16(doc, at, (UINT16)(stable | 0x8000));
	at += 2;
	doc[at++] = 0x09;
	UINT32 length = 0;
	while (text[length])
		++length;
	put32(doc, at, length);
	at += 4;
	for (UINT32 index = 0; index < length; ++index) {
		doc[at++] = (unsigned char)text[index];
		doc[at++] = 0;
	}
	return at;
}

/* The Name node the loader applies to the element on the top of the stack: the constant 0x0d carries a
 * string (kind 0x09, length, UTF-16LE), which becomes the element's x:Name. */
static size_t set_name(unsigned char *doc, size_t at, const char *text)
{
	UINT32 length = 0;
	while (text[length])
		++length;
	doc[at++] = 0x0d;
	doc[at++] = 0x09;
	put32(doc, at, length);
	at += 4;
	for (UINT32 index = 0; index < length; ++index) {
		doc[at++] = (unsigned char)text[index];
		doc[at++] = 0;
	}
	return at;
}

/* A page whose content is a grid with four children: a TextBlock named Caption carrying text, a
 * nameless TextBlock carrying text, a nameless grid and a Button named Go — the one element whose
 * peer serves the Invoke provider the button checks exercise. */
static size_t build_automation_document(unsigned char *doc)
{
	size_t at = begin_document(doc);
	at = create_type(doc, at, 525);                       /* Page */
	at = set_property(doc, at, 999);                      /* Content */
	at = create_type(doc, at, 442);                       /* root grid */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 396);                       /* the named text block */
	at = set_name(doc, at, "Caption");                    /* x:Name */
	at = set_string(doc, at, 800, "Hello");               /* Text */
	doc[at++] = 0x02;                                     /* pop the named text block */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 396);                       /* the nameless text block */
	at = set_string(doc, at, 800, "Second text");         /* Text */
	doc[at++] = 0x02;                                     /* pop the nameless text block */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 442);                       /* the nameless grid */
	doc[at++] = 0x02;                                     /* pop the nameless grid */
	at = set_property(doc, at, 648);                      /* Children */
	at = create_type(doc, at, 540);                       /* the button */
	at = set_name(doc, at, "Go");                         /* x:Name */
	doc[at++] = 0x02;                                     /* pop the button */
	doc[at++] = 0x02;                                     /* pop the root grid */
	const UINT32 nodes = (UINT32)(at - 168);
	put32(doc, 8, nodes + 12);
	put32(doc, 164, nodes);
	return at;
}

/* The Click handler the invoke checks count: the engine stores handlers through IUnknown and invokes
 * the fourth slot with the element as sender, so the fixture declares the delegate shape locally. */
struct fixture_click_vtbl {
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(void *This, REFIID riid, void **object);
	ULONG (STDMETHODCALLTYPE *AddRef)(void *This);
	ULONG (STDMETHODCALLTYPE *Release)(void *This);
	HRESULT (STDMETHODCALLTYPE *Invoke)(void *This, IInspectable *sender, void *args);
};

struct fixture_click {
	const struct fixture_click_vtbl *lpVtbl;
	LONG refs;
	ULONG calls;
};

static HRESULT WINAPI fixture_click_query(void *This, REFIID riid, void **object)
{
	if (!object)
		return E_POINTER;
	*object = NULL;
	(void)riid;
	InterlockedIncrement(&((struct fixture_click *)This)->refs);
	*object = This;
	return S_OK;
}

static ULONG WINAPI fixture_click_addref(void *This)
{
	return (ULONG)InterlockedIncrement(&((struct fixture_click *)This)->refs);
}

static ULONG WINAPI fixture_click_release(void *This)
{
	return (ULONG)InterlockedDecrement(&((struct fixture_click *)This)->refs);
}

static HRESULT WINAPI fixture_click_invoke(void *This, IInspectable *sender, void *args)
{
	(void)sender;
	(void)args;
	((struct fixture_click *)This)->calls++;
	return S_OK;
}

static const struct fixture_click_vtbl fixture_click_table = {
	fixture_click_query, fixture_click_addref, fixture_click_release, fixture_click_invoke,
};

/* The name a GetName answer carries, compared against the fixture's ASCII expectations. */
static int same_text(HSTRING value, const char *expected)
{
	UINT32 length = 0;
	const WCHAR *text = value ? WindowsGetStringRawBuffer(value, &length) : NULL;
	if (!text)
		return 0;
	for (UINT32 index = 0; index < length; ++index)
		if (expected[index] != (char)text[index] || !expected[index])
			return 0;
	return expected[length] == '\0';
}

int raven_entry(void)
{
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function RoInitialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function RoGetActivationFactory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	if (!RoInitialize || !RoGetActivationFactory) return 60;
	if (FAILED(RoInitialize(1))) return 1;

	HSTRING core_key = NULL;
	static const WCHAR core_name[] = L"Windows.UI.Core.CoreWindow";
	if (WindowsCreateString(core_name, ARRAYSIZE(core_name) - 1, &core_key) != S_OK) return 61;
	__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic *core_statics = NULL;
	if (RoGetActivationFactory(core_key, &IID___x_ABI_CWindows_CUI_CCore_CICoreWindowStatic, (void **)&core_statics) != S_OK ||
		!core_statics) return 2;
	__x_ABI_CWindows_CUI_CCore_CICoreWindow *core_window = NULL;
	if (__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_GetForCurrentThread(core_statics, &core_window) != S_OK || !core_window)
		return 3;
	raven_core_window_interop *interop = NULL;
	if (core_window->lpVtbl->QueryInterface(core_window, &raven_core_window_interop_iid, (void **)&interop) != S_OK ||
		!interop) return 4;
	HWND hwnd = NULL;
	if (interop->lpVtbl->get_WindowHandle(interop, &hwnd) != S_OK || !hwnd) return 5;
	(void)hwnd;
	ICoreWindow_Activate(core_window);

	/* The compiled page the fixture hosts. */
	HMODULE component = LoadLibraryW(L"raven-xaml.dll");
	load_function load = component ? (load_function)GetProcAddress(component, "RavenXamlLoad") : NULL;
	if (!load) return 6;
	static unsigned char document[1024];
	IInspectable *page = NULL;
	const size_t size = build_automation_document(document);
	if (load(document, (uint32_t)size, &page) != S_OK || !page) return 7;

	/* The root grid is the page's content; its three children carry the automation cases. */
	IInspectable *root = NULL;
	{
		__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl *content_control = NULL;
		if (page->lpVtbl->QueryInterface(page, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIContentControl,
			(void **)&content_control) != S_OK || !content_control) return 8;
		content_control->lpVtbl->get_Content(content_control, (uint64_t)(uintptr_t)&root);
		content_control->lpVtbl->Release(content_control);
	}
	if (!root) return 9;
	panel1 *panel = NULL;
	if (root->lpVtbl->QueryInterface(root, &IID__x_ABI_CWindows_CUI_CXaml_CControls_CIPanel, (void **)&panel) != S_OK ||
		!panel) return 10;
	void *children_collection = NULL;
	if (panel->lpVtbl->get_Children(panel, (uint64_t)(uintptr_t)&children_collection) != S_OK || !children_collection)
		return 11;
	struct fixture_vector *children = (struct fixture_vector *)children_collection;
	UINT32 child_count = 0;
	if (children->lpVtbl->get_Size(children, &child_count) != S_OK || child_count != 4) return 12;
	/* The element pointers the engine hands out: the Children vector's IUIElement entries, which are
	 * the element objects themselves. The statics take them as the owners to bind peers to. */
	ui_element1 *named_ui = NULL, *plain_ui = NULL, *inner_ui = NULL, *button_ui = NULL;
	{
		if (children->lpVtbl->GetAt(children, 0, &named_ui) != S_OK || !named_ui) return 13;
		if (children->lpVtbl->GetAt(children, 1, &plain_ui) != S_OK || !plain_ui) return 14;
		if (children->lpVtbl->GetAt(children, 2, &inner_ui) != S_OK || !inner_ui) return 15;
		if (children->lpVtbl->GetAt(children, 3, &button_ui) != S_OK || !button_ui) return 64;
	}


	/* Hosting the page on the window, the way the engine presents a page; the peers need no pixels. */
	HSTRING window_key = NULL;
	static const WCHAR window_name[] = L"Windows.UI.Xaml.Window";
	if (WindowsCreateString(window_name, ARRAYSIZE(window_name) - 1, &window_key) != S_OK) return 62;
	IActivationFactory *window_activation = NULL;
	if (RoGetActivationFactory(window_key, &IID_IActivationFactory, (void **)&window_activation) != S_OK ||
		!window_activation) return 18;
	window_statics *window_current = NULL;
	if (window_activation->lpVtbl->QueryInterface(window_activation, &IID__x_ABI_CWindows_CUI_CXaml_CIWindowStatics,
		(void **)&window_current) != S_OK || !window_current) return 19;
	window1 *xaml_window = NULL;
	if (window_current->lpVtbl->get_Current(window_current, (uint64_t)(uintptr_t)&xaml_window) != S_OK || !xaml_window)
		return 20;
	if (xaml_window->lpVtbl->put_Content(xaml_window, (uint64_t)(uintptr_t)page) != S_OK) return 21;

	/* The statics of FrameworkElementAutomationPeer, reached the way the projection reaches them: the
	 * activation factory of the class QIs to the statics interface. */
	HSTRING peer_key = NULL;
	static const WCHAR peer_name[] = L"Windows.UI.Xaml.Controls.Primitives.FrameworkElementAutomationPeer";
	if (WindowsCreateString(peer_name, ARRAYSIZE(peer_name) - 1, &peer_key) != S_OK) return 63;
	peer_statics1 *statics = NULL;
	if (RoGetActivationFactory(peer_key, &IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerStatics,
		(void **)&statics) != S_OK || !statics) return 22;

	/* The identity: a second CreatePeerForElement for the same element returns the same peer. */
	framework_peer1 *named_peer = NULL, *named_peer_again = NULL;
	if (statics->lpVtbl->CreatePeerForElement(statics, (uint64_t)(uintptr_t)named_ui,
		(uint64_t)(uintptr_t)&named_peer) != S_OK || !named_peer) return 23;
	if (statics->lpVtbl->CreatePeerForElement(statics, (uint64_t)(uintptr_t)named_ui,
		(uint64_t)(uintptr_t)&named_peer_again) != S_OK || !named_peer_again) return 24;
	if (named_peer != named_peer_again) return 25;
	named_peer_again->lpVtbl->Release(named_peer_again);

	/* The peer's Owner is the element it was created for. */
	framework_peer1 *framework = NULL;
	if (named_peer->lpVtbl->QueryInterface(named_peer,
		&IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeer, (void **)&framework) != S_OK ||
		!framework) return 26;
	ui_element1 *owner = NULL;
	if (framework->lpVtbl->get_Owner(framework, (uint64_t)(uintptr_t)&owner) != S_OK || !owner) return 27;
	if (owner != named_ui) return 28;
	owner->lpVtbl->Release(owner);
	framework->lpVtbl->Release(framework);

	/* GetName answers the loader-assigned name. */
	automation1 *automation = NULL;
	if (named_peer->lpVtbl->QueryInterface(named_peer,
		&IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer, (void **)&automation) != S_OK || !automation)
		return 29;
	HSTRING name = NULL;
	EventRegistrationToken token = {0};
	if (automation->lpVtbl->GetName(automation, (uint64_t)(uintptr_t)&name) != S_OK || !name) return 30;
	if (!same_text(name, "Caption")) return 31;
	WindowsDeleteString(name);

	/* A nameless TextBlock falls back to its Text, a nameless grid to the runtime class name. An
	 * element without a peer has none: FromElement answers null without an error before one is created. */
	framework_peer1 *plain_peer = NULL, *inner_peer = NULL;
	if (statics->lpVtbl->FromElement(statics, (uint64_t)(uintptr_t)plain_ui,
		(uint64_t)(uintptr_t)&plain_peer) != S_OK) return 70;
	if (plain_peer) return 71;
	if (statics->lpVtbl->CreatePeerForElement(statics, (uint64_t)(uintptr_t)plain_ui,
		(uint64_t)(uintptr_t)&plain_peer) != S_OK || !plain_peer) return 72;
	{
		automation1 *plain_automation = NULL;
		if (plain_peer->lpVtbl->QueryInterface(plain_peer,
			&IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer, (void **)&plain_automation) != S_OK ||
			!plain_automation) return 33;
		if (plain_automation->lpVtbl->GetName(plain_automation, (uint64_t)(uintptr_t)&name) != S_OK || !name) return 34;
		if (!same_text(name, "Second text")) return 35;
		WindowsDeleteString(name);
		plain_automation->lpVtbl->Release(plain_automation);
	}
	if (statics->lpVtbl->CreatePeerForElement(statics, (uint64_t)(uintptr_t)inner_ui,
		(uint64_t)(uintptr_t)&inner_peer) != S_OK || !inner_peer) return 36;
	{
		automation1 *inner_automation = NULL;
		if (inner_peer->lpVtbl->QueryInterface(inner_peer,
			&IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer, (void **)&inner_automation) != S_OK ||
			!inner_automation) return 37;
		if (inner_automation->lpVtbl->GetName(inner_automation, (uint64_t)(uintptr_t)&name) != S_OK || !name) return 38;
		if (!same_text(name, "Windows.UI.Xaml.Controls.Grid")) return 39;
		WindowsDeleteString(name);
		inner_automation->lpVtbl->Release(inner_automation);
	}

	/* A second element gets a different peer, and FromElement answers the peer an element already has. */
	framework_peer1 *root_peer = NULL, *from_element_peer = NULL;
	if (statics->lpVtbl->CreatePeerForElement(statics, (uint64_t)(uintptr_t)root,
		(uint64_t)(uintptr_t)&root_peer) != S_OK || !root_peer) return 40;
	if (root_peer == named_peer || root_peer == inner_peer) return 41;
	if (statics->lpVtbl->FromElement(statics, (uint64_t)(uintptr_t)named_ui,
		(uint64_t)(uintptr_t)&from_element_peer) != S_OK || from_element_peer != named_peer) return 42;
	if (from_element_peer)
		from_element_peer->lpVtbl->Release(from_element_peer);

	/* No pattern provider is implemented: every PatternInterface answers E_NOTIMPL. The winmd declares
	 * 34 values, Invoke through CustomNavigation. */
	for (UINT32 pattern = 0; pattern < 34; ++pattern) {
		void *result = (void *)0x1;
		if (automation->lpVtbl->GetPattern(automation, pattern, (uint64_t)(uintptr_t)&result) != E_NOTIMPL) return 45;
		if (result) return 46;
	}

	/* The element kinds the engine builds peers for are content and control elements. */
	boolean flag = 0;
	if (automation->lpVtbl->IsContentElement(automation, (uint64_t)(uintptr_t)&flag) != S_OK) return 47;
	if (!flag) return 48;
	if (automation->lpVtbl->IsControlElement(automation, (uint64_t)(uintptr_t)&flag) != S_OK) return 49;
	if (!flag) return 50;

	/* The composable factory the class carries answers the one peer of the element too, and rejects the
	 * aggregation outer nothing here consumes. */
	IActivationFactory *factory = NULL;
	if (statics->lpVtbl->QueryInterface(statics, &IID_IActivationFactory, (void **)&factory) != S_OK || !factory)
		return 51;
	peer_factory1 *composable = NULL;
	if (factory->lpVtbl->QueryInterface(factory,
		&IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIFrameworkElementAutomationPeerFactory,
		(void **)&composable) != S_OK || !composable) return 52;
	{
		void *inner = (void *)0x1, *created = NULL;
		if (composable->lpVtbl->CreateInstanceWithOwner(composable, (uint64_t)(uintptr_t)root, 0,
			(uint64_t)(uintptr_t)&inner, (uint64_t)(uintptr_t)&created) != S_OK || !created) return 53;
		if (created != (void *)root_peer || inner) return 54;
		((IUnknown *)created)->lpVtbl->Release((IUnknown *)created);
		if (composable->lpVtbl->CreateInstanceWithOwner(composable, (uint64_t)(uintptr_t)root, 1,
			(uint64_t)(uintptr_t)&inner, (uint64_t)(uintptr_t)&created) != E_NOTIMPL) return 55;
	}

	/* The default construction of the class: an ownerless peer, which has no name to answer. */
	{
		IInspectable *ownerless = NULL;
		if (factory->lpVtbl->ActivateInstance(factory, &ownerless) != S_OK || !ownerless) return 56;
		automation1 *ownerless_automation = NULL;
		if (ownerless->lpVtbl->QueryInterface(ownerless,
			&IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer, (void **)&ownerless_automation) != S_OK ||
			!ownerless_automation) return 57;
		if (ownerless_automation->lpVtbl->GetName(ownerless_automation, (uint64_t)(uintptr_t)&name) == S_OK) return 58;
		ownerless_automation->lpVtbl->Release(ownerless_automation);
		ownerless->lpVtbl->Release(ownerless);
	}

	/* The button's peer: the one element kind whose peer serves a pattern provider. Its Click goes
	 * through the handler the fixture registers on IButtonBase, and the Invoke provider raises it
	 * synchronously — the count answers inside the call that invoked. */
	{
		__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase *button_base = NULL;
		struct fixture_click click = { &fixture_click_table, 1, 0 };
		framework_peer1 *button_peer = NULL;
		automation1 *button_automation = NULL;
		void *provider = NULL;
		if (button_ui->lpVtbl->QueryInterface(button_ui,
			&IID__x_ABI_CWindows_CUI_CXaml_CControls_CPrimitives_CIButtonBase, (void **)&button_base) != S_OK ||
			!button_base) return 73;
		if (button_base->lpVtbl->add_Click(button_base, (uint64_t)(uintptr_t)&click,
			(uint64_t)(uintptr_t)&token) != S_OK) return 74;
		button_base->lpVtbl->Release(button_base);
		if (statics->lpVtbl->CreatePeerForElement(statics, (uint64_t)(uintptr_t)button_ui,
			(uint64_t)(uintptr_t)&button_peer) != S_OK || !button_peer) return 75;
		if (button_peer->lpVtbl->QueryInterface(button_peer,
			&IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CPeers_CIAutomationPeer,
			(void **)&button_automation) != S_OK || !button_automation) return 76;
		if (button_automation->lpVtbl->GetName(button_automation, (uint64_t)(uintptr_t)&name) != S_OK || !name)
			return 77;
		if (!same_text(name, "Go")) return 78;
		WindowsDeleteString(name);
		name = NULL;
		/* Invoke is pattern 0; the button's peer serves it and nothing else. */
		if (button_automation->lpVtbl->GetPattern(button_automation, 0,
			(uint64_t)(uintptr_t)&provider) != S_OK || !provider) return 79;
		__x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProvider *invoke =
			(__x_ABI_CWindows_CUI_CXaml_CAutomation_CProvider_CIInvokeProvider *)provider;
		if (invoke->lpVtbl->Invoke(invoke) != S_OK) return 80;
		if (click.calls != 1) return 81;
		if (invoke->lpVtbl->Invoke(invoke) != S_OK) return 82;
		if (click.calls != 2) return 83;
		invoke->lpVtbl->Release(invoke);
		if (button_automation->lpVtbl->GetPattern(button_automation, 1,
			(uint64_t)(uintptr_t)&provider) != E_NOTIMPL || provider) return 84;
		button_automation->lpVtbl->Release(button_automation);
		button_peer->lpVtbl->Release(button_peer);
	}

	factory->lpVtbl->Release(factory);
	composable->lpVtbl->Release(composable);
	statics->lpVtbl->Release(statics);
	WindowsDeleteString(peer_key);
	root_peer->lpVtbl->Release(root_peer);
	inner_peer->lpVtbl->Release(inner_peer);
	plain_peer->lpVtbl->Release(plain_peer);
	named_peer->lpVtbl->Release(named_peer);
	automation->lpVtbl->Release(automation);
	root->lpVtbl->Release(root);
	inner_ui->lpVtbl->Release(inner_ui);
	plain_ui->lpVtbl->Release(plain_ui);
	named_ui->lpVtbl->Release(named_ui);
	button_ui->lpVtbl->Release(button_ui);
	children->lpVtbl->Release(children);
	panel->lpVtbl->Release(panel);
	root->lpVtbl->Release(root);
	xaml_window->lpVtbl->Release(xaml_window);
	window_current->lpVtbl->Release(window_current);
	window_activation->lpVtbl->Release(window_activation);
	WindowsDeleteString(window_key);
	page->lpVtbl->Release(page);
	ICoreWindow_Release(core_window);
	__x_ABI_CWindows_CUI_CCore_CICoreWindowStatic_Release(core_statics);
	WindowsDeleteString(core_key);
	interop->lpVtbl->Release(interop);
	return 0;
}
