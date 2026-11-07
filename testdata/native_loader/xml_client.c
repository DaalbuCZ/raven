/* The XML DOM fixture: activates Windows.Data.Xml.Dom.XmlDocument through the runtime catalog, parses a document
 * the way the guests observed do — LoadXml, the document element, tag-name searches, an XPath select, attribute
 * reads and writes, the InnerText and GetXml serializations, node value boxes, tree edits, clone and the load
 * settings' defaults. Freestanding C like the other guests: hand-rolled COM, no CRT beyond the freestanding one
 * the build links. */
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <winstring.h>
#include <roapi.h>
#include <activation.h>
#include <windows.foundation.h>
#include <windows.data.xml.dom.h>
#include "windows_ui_xaml_dictionary.h" /* the IReference<HSTRING> box the NodeValue getters serve */

typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocument document_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentIO documentio_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlElement element_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlNode node_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeList nodelist_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeSelector selector_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeSerializer serializer_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlNamedNodeMap namedmap_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlAttribute attribute_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlLoadSettings settings_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlDomImplementation domimpl_i;
typedef __x_ABI_CWindows_CData_CXml_CDom_CIXmlText text_i;
typedef __FIReference_1_HSTRING reference_i;

typedef HRESULT (WINAPI *initialize_function)(int);
typedef HRESULT (WINAPI *factory_function)(HSTRING, REFIID, void **);

static int same_text(HSTRING value, const WCHAR *expected)
{
	UINT32 length = 0;
	const WCHAR *text = WindowsGetStringRawBuffer(value, &length);
	INT expected_length = lstrlenW(expected);
	return (INT)length == expected_length &&
		CompareStringOrdinal(text, (int)length, expected, expected_length, FALSE) == CSTR_EQUAL;
}

static int text_of(HSTRING value, const WCHAR *expected)
{
	int same = same_text(value, expected);
	WindowsDeleteString(value);
	return same;
}

static HRESULT activate(const WCHAR *name, IInspectable **out)
{
	HSTRING key = NULL;
	IActivationFactory *factory = NULL;
	HRESULT result;
	if (WindowsCreateString(name, (UINT32)lstrlenW(name), &key) != S_OK)
		return E_FAIL;
	result = RoGetActivationFactory(key, &IID_IActivationFactory, (void **)&factory);
	WindowsDeleteString(key);
	if (FAILED(result))
		return result;
	result = factory->lpVtbl->ActivateInstance(factory, out);
	factory->lpVtbl->Release(factory);
	return result;
}

static HRESULT make_string(const WCHAR *text, HSTRING *out)
{
	return WindowsCreateString(text, (UINT32)lstrlenW(text), out);
}

/* The document the checks walk: a manifest-shaped root with attributes, nested elements,
 * text and entity references, a comment and a processing instruction. */
static const WCHAR sample_xml[] =
	L"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
	L"<Package Version=\"1.2\" xmlns=\"http://schemas.example.com/manifest\">"
	L"<!-- the identity -->"
	L"<Identity Name=\"Shapr.Fixture\"/>"
	L"<Applications><Application Id=\"App\" EntryPoint=\"Fixture.App\">"
	L"<DisplayName>Fixture &amp; Tools</DisplayName>"
	L"</Application></Applications>"
	L"<Tools><Tool Name=\"alpha\"/><Tool Name=\"beta\"/><Tool Name=\"gamma\"/></Tools>"
	L"</Package>";

int raven_entry(void)
{
	static const WCHAR document_name[] = L"Windows.Data.Xml.Dom.XmlDocument";
	static const WCHAR settings_name[] = L"Windows.Data.Xml.Dom.XmlLoadSettings";
	static const WCHAR sample_root[] = L"Package";
	HMODULE runtime = LoadLibraryW(L"combase.dll");
	initialize_function initialize = runtime ? (initialize_function)GetProcAddress(runtime, "RoInitialize") : NULL;
	factory_function get_factory = runtime ? (factory_function)GetProcAddress(runtime, "RoGetActivationFactory") : NULL;
	IInspectable *instance = NULL, *settings_object = NULL;
	document_i *document = NULL;
	documentio_i *io = NULL;
	selector_i *selector = NULL;
	serializer_i *serializer = NULL;
	HSTRING xml = NULL, text = NULL;
	element_i *root = NULL;
	HSTRING tag = NULL, attribute = NULL, path = NULL;
	nodelist_i *tools = NULL;
	node_i *node = NULL, *selected = NULL, *text_node = NULL, *clone = NULL, *unknown = NULL;
	element_i *created = NULL;
	attribute_i *created_attribute = NULL;
	node_i *created_node = NULL, *root_node = NULL;
	UINT32 count = 0;
	boolean flag = 0;
	HRESULT result;
	(void)get_factory;
	if (!initialize || !get_factory)
		return 1;
	if (initialize(1) != S_OK)
		return 2;

	/* the document activates and serves its default and required interfaces */
	if (activate(document_name, &instance) != S_OK || !instance)
		return 10;
	{
		/* the statics the way a .NET Native program takes them: RoGetActivationFactory with the statics IID */
		HSTRING key = NULL;
		IActivationFactory *factory = NULL;
		IInspectable *statics = NULL;
		if (WindowsCreateString(document_name, lstrlenW(document_name), &key) != S_OK)
			return 17;
		if (RoGetActivationFactory(key, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentStatics, (void **)&statics) != S_OK || !statics)
			return 18;
		statics->lpVtbl->Release(statics);
		WindowsDeleteString(key);
	}
	if (instance->lpVtbl->QueryInterface(instance, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlDocument, (void **)&document) != S_OK || !document)
		return 11;
	if (instance->lpVtbl->QueryInterface(instance, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentIO, (void **)&io) != S_OK || !io)
		return 12;
	if (instance->lpVtbl->QueryInterface(instance, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeSelector, (void **)&selector) != S_OK || !selector)
		return 13;
	if (instance->lpVtbl->QueryInterface(instance, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeSerializer, (void **)&serializer) != S_OK || !serializer)
		return 14;
	if (instance->lpVtbl->QueryInterface(instance, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlNamedNodeMap, (void **)&unknown) == S_OK)
		return 16; /* the map is the element's, not the document's */
	if (instance->lpVtbl->QueryInterface(instance, &IID_IUnknown, (void **)&unknown) != S_OK || !unknown)
		return 15;
	unknown->lpVtbl->Release(unknown);
	unknown = NULL;

	/* an empty document has no root; LoadXml parses the sample */
	if (document->lpVtbl->get_DocumentElement(document, &root) != S_OK || root)
		return 20;
	if (make_string(sample_xml, &xml) != S_OK)
		return 21;
	if (io->lpVtbl->LoadXml(io, xml) != S_OK)
		return 22;
	WindowsDeleteString(xml);
	xml = NULL;
	if (document->lpVtbl->get_DocumentElement(document, &root) != S_OK || !root)
		return 23;
	if (root->lpVtbl->get_TagName(root, &tag) != S_OK || !text_of(tag, sample_root))
		return 24;
	if (make_string(L"Version", &attribute) != S_OK)
		return 25;
	if (root->lpVtbl->GetAttribute(root, attribute, &text) != S_OK || !text_of(text, L"1.2"))
		return 26;
	WindowsDeleteString(attribute);
	attribute = NULL;

	/* the declaration and comment are not elements; the tag search finds the nested ones */
	if (make_string(L"Tool", &tag) != S_OK)
		return 30;
	if (document->lpVtbl->GetElementsByTagName(document, tag, &tools) != S_OK || !tools)
		return 31;
	WindowsDeleteString(tag);
	tag = NULL;
	if (tools->lpVtbl->get_Length(tools, &count) != S_OK || count != 3)
		return 32;
	if (tools->lpVtbl->Item(tools, 1, &selected) != S_OK || !selected)
		return 33;
	tools->lpVtbl->Release(tools);
	tools = NULL;
	if (selected->lpVtbl->QueryInterface(selected, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlElement, (void **)&created) != S_OK || !created)
		return 34;
	if (make_string(L"Name", &attribute) != S_OK)
		return 35;
	if (created->lpVtbl->GetAttribute(created, attribute, &text) != S_OK || !text_of(text, L"beta"))
		return 36;

	/* the attribute map: one entry, the named item, and the boxed node value */
	{
		namedmap_i *map = NULL;
		reference_i *box = NULL;
		HSTRING value = NULL;
		if (selected->lpVtbl->get_Attributes(selected, &map) != S_OK || !map)
			return 40;
		if (map->lpVtbl->get_Length(map, &count) != S_OK || count != 1)
			return 41;
		if (map->lpVtbl->GetNamedItem(map, attribute, &node) != S_OK || !node)
			return 42;
		if (node->lpVtbl->get_NodeValue(node, &instance) != S_OK || !instance)
			return 43;
		if (instance->lpVtbl->QueryInterface(instance, &IID___FIReference_1_HSTRING, (void **)&box) != S_OK || !box)
			return 44;
		if (box->lpVtbl->get_Value(box, &value) != S_OK || !text_of(value, L"beta"))
			return 45;
		box->lpVtbl->Release(box);
		WindowsDeleteString(value);
		instance->lpVtbl->Release(instance);
		instance = NULL;
		node->lpVtbl->Release(node);
		node = NULL;
		map->lpVtbl->Release(map);
		map = NULL;
	}

	/* an XPath select: the absolute path, a descendant step with a positional predicate */
	if (make_string(L"/Package/Applications/Application", &path) != S_OK)
		return 50;
	result = selector->lpVtbl->SelectSingleNode(selector, path, &selected);
	WindowsDeleteString(path);
	path = NULL;
	if (result != S_OK || !selected)
		return 51;
	selected->lpVtbl->Release(selected);
	selected = NULL;
	if (make_string(L"//Tool", &path) != S_OK)
		return 52;
	result = selector->lpVtbl->SelectSingleNode(selector, path, &selected);
	WindowsDeleteString(path);
	path = NULL;
	if (result != S_OK || !selected)
		return 53;
	selected->lpVtbl->Release(selected);
	selected = NULL;
	if (make_string(L"//Tool[3]", &path) != S_OK)
		return 52;
	result = selector->lpVtbl->SelectSingleNode(selector, path, &selected);
	WindowsDeleteString(path);
	path = NULL;
	if (result != S_OK || !selected)
		return 54;
	if (!selected)
		return 533;
	{
		nodelist_i *many = NULL;
		if (make_string(L"/Package/Tools/Tool", &path) != S_OK)
			return 54;
		result = selector->lpVtbl->SelectNodes(selector, path, &many);
		WindowsDeleteString(path);
		path = NULL;
		if (result != S_OK || !many)
			return 55;
		if (many->lpVtbl->get_Length(many, &count) != S_OK || count != 3)
			return 56;
		many->lpVtbl->Release(many);
	}
	selected->lpVtbl->Release(selected);
	selected = NULL;
	created->lpVtbl->Release(created);
	created = NULL;
	WindowsDeleteString(attribute);
	attribute = NULL;

	/* InnerText keeps the entity-decoded text; GetXml re-encodes it */
	if (make_string(L"DisplayName", &tag) != S_OK)
		return 60;
	if (document->lpVtbl->GetElementsByTagName(document, tag, &tools) != S_OK || !tools)
		return 61;
	WindowsDeleteString(tag);
	tag = NULL;
	if (tools->lpVtbl->get_Length(tools, &count) != S_OK || count != 1)
		return 62;
	if (tools->lpVtbl->Item(tools, 0, &node) != S_OK || !node)
		return 63;
	tools->lpVtbl->Release(tools);
	tools = NULL;
	if (node->lpVtbl->QueryInterface(node, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeSerializer, (void **)&serializer) != S_OK)
		return 64;
	if (serializer->lpVtbl->get_InnerText(serializer, &text) != S_OK || !text_of(text, L"Fixture & Tools"))
		return 65;
	if (serializer->lpVtbl->GetXml(serializer, &text) != S_OK || !text)
		return 66;
	WindowsDeleteString(text);
	node->lpVtbl->Release(node);
	node = NULL;

	/* attribute writes: replace, read back, remove */
	if (make_string(L"Version", &attribute) != S_OK)
		return 70;
	if (make_string(L"3.4", &text) != S_OK)
		return 71;
	if (root->lpVtbl->SetAttribute(root, attribute, text) != S_OK)
		return 72;
	WindowsDeleteString(text);
	text = NULL;
	if (root->lpVtbl->GetAttribute(root, attribute, &text) != S_OK || !text_of(text, L"3.4"))
		return 73;
	if (root->lpVtbl->RemoveAttribute(root, attribute) != S_OK)
		return 74;
	if (root->lpVtbl->GetAttribute(root, attribute, &text) != S_OK)
		return 75;
	{
		/* CompareStringOrdinal over two empty strings is not the equality the fixture needs here; the length is */
		UINT32 length = 0;
		WindowsGetStringRawBuffer(text, &length);
		WindowsDeleteString(text);
		if (length)
			return 76; /* the removal left the attribute */
	}
	WindowsDeleteString(attribute);
	attribute = NULL;

	/* tree edits: create, append, read back through the selector, remove */
	if (make_string(L"New", &tag) != S_OK)
		return 80;
	if (make_string(L"appended", &text) != S_OK)
		return 81;
	if (document->lpVtbl->CreateElement(document, tag, &created) != S_OK || !created)
		return 82;
	{
		text_i *created_text = NULL;
		if (document->lpVtbl->CreateTextNode(document, text, &created_text) != S_OK || !created_text)
			return 83;
		if (created_text->lpVtbl->QueryInterface(created_text, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlNode, (void **)&text_node) != S_OK)
			return 84;
		created_text->lpVtbl->Release(created_text);
	}
	WindowsDeleteString(tag);
	tag = NULL;
	WindowsDeleteString(text);
	text = NULL;
	if (created->lpVtbl->QueryInterface(created, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlNode, (void **)&created_node) != S_OK || !created_node)
		return 85;
	if (root->lpVtbl->QueryInterface(root, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlNode, (void **)&root_node) != S_OK || !root_node)
		return 85;
	if (created_node->lpVtbl->AppendChild(created_node, text_node, &node) != S_OK || !node)
		return 85;
	node->lpVtbl->Release(node);
	node = NULL;
	if (root_node->lpVtbl->AppendChild(root_node, created_node, &node) != S_OK || !node)
		return 86;
	node->lpVtbl->Release(node);
	node = NULL;
	if (make_string(L"/Package/New", &path) != S_OK)
		return 87;
	result = selector->lpVtbl->SelectSingleNode(selector, path, &selected);
	WindowsDeleteString(path);
	path = NULL;
	if (result != S_OK || !selected)
		return 88;
	selected->lpVtbl->Release(selected);
	selected = NULL;
	if (root_node->lpVtbl->RemoveChild(root_node, created_node, &node) != S_OK || !node)
		return 89;
	node->lpVtbl->Release(node);
	node = NULL;
	created_node->lpVtbl->Release(created_node);
	created_node = NULL;
	root_node->lpVtbl->Release(root_node);
	root_node = NULL;
	created->lpVtbl->Release(created);
	created = NULL;
	text_node->lpVtbl->Release(text_node);
	text_node = NULL;

	/* a deep clone copies the attributes and children */
	if (make_string(L"//Application", &path) != S_OK)
		return 90;
	result = selector->lpVtbl->SelectSingleNode(selector, path, &selected);
	WindowsDeleteString(path);
	path = NULL;
	if (result != S_OK || !selected)
		return 91;
	if (selected->lpVtbl->CloneNode(selected, TRUE, &clone) != S_OK || !clone)
		return 92;
	selected->lpVtbl->Release(selected);
	selected = NULL;
	if (clone->lpVtbl->get_FirstChild(clone, &node) != S_OK || !node)
		return 93;
	node->lpVtbl->Release(node);
	node = NULL;
	clone->lpVtbl->Release(clone);
	clone = NULL;

	/* the implementation object and the load settings' defaults */
	if (document->lpVtbl->get_Implementation(document, (domimpl_i **)&unknown) != S_OK || !unknown)
		return 100;
	{
		domimpl_i *implementation = (domimpl_i *)unknown;
		HSTRING feature = NULL;
		if (make_string(L"XML", &feature) != S_OK)
			return 101;
		if (implementation->lpVtbl->HasFeature(implementation, feature, NULL, &flag) != S_OK || !flag)
			return 102;
		WindowsDeleteString(feature);
		unknown->lpVtbl->Release(unknown);
		unknown = NULL;
	}
	if (activate(settings_name, &settings_object) != S_OK || !settings_object)
		return 103;
	{
		settings_i *settings = NULL;
		UINT32 depth = 1;
		if (settings_object->lpVtbl->QueryInterface(settings_object, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlLoadSettings, (void **)&settings) != S_OK || !settings)
			return 104;
		if (settings->lpVtbl->get_ProhibitDtd(settings, &flag) != S_OK || !flag)
			return 105;
		if (settings->lpVtbl->get_ResolveExternals(settings, &flag) != S_OK || flag)
			return 106;
		if (settings->lpVtbl->get_MaxElementDepth(settings, &depth) != S_OK || depth)
			return 107;
		settings->lpVtbl->Release(settings);
	}
	settings_object->lpVtbl->Release(settings_object);

	/* the serialization parses again: the document's GetXml feeds LoadXml without a change */
	{
		serializer_i *document_serializer = NULL;
		if (((IInspectable *)document)->lpVtbl->QueryInterface((IInspectable *)document, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlNodeSerializer, (void **)&document_serializer) != S_OK)
			return 109;
		result = document_serializer->lpVtbl->GetXml(document_serializer, &text);
		document_serializer->lpVtbl->Release(document_serializer);
		if (result != S_OK)
			return 110;
	}
	{
		IInspectable *second = NULL;
		documentio_i *second_io = NULL;
		document_i *second_document = NULL;
		element_i *second_root = NULL;
		if (activate(document_name, &second) != S_OK || !second)
			return 111;
		if (second->lpVtbl->QueryInterface(second, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentIO, (void **)&second_io) != S_OK)
			return 112;
		if (second_io->lpVtbl->LoadXml(second_io, text) != S_OK)
			return 113;
		WindowsDeleteString(text);
		if (second->lpVtbl->QueryInterface(second, &IID___x_ABI_CWindows_CData_CXml_CDom_CIXmlDocument, (void **)&second_document) != S_OK)
			return 114;
		if (second_document->lpVtbl->get_DocumentElement(second_document, &second_root) != S_OK || !second_root)
			return 115;
		if (second_root->lpVtbl->get_TagName(second_root, &tag) != S_OK || !text_of(tag, sample_root))
			return 116;
		second_root->lpVtbl->Release(second_root);
		second_document->lpVtbl->Release(second_document);
		second_io->lpVtbl->Release(second_io);
		second->lpVtbl->Release(second);
	}

	serializer->lpVtbl->Release(serializer);
	serializer = NULL;
	root->lpVtbl->Release(root);
	root = NULL;
	io->lpVtbl->Release(io);
	io = NULL;
	selector->lpVtbl->Release(selector);
	selector = NULL;
	document->lpVtbl->Release(document);
	document = NULL;
	return 0;
}
