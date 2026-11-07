/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Mirko Brombin
 * See LICENSES/MIT.txt.
 */
/* Raven XAML component: Windows.UI.Xaml.Automation.AutomationProperties.
 *
 * AutomationProperties is a statics-only class: the framework never activates it, it takes the nine
 * IAutomationPropertiesStatics interfaces from the activation factory, and through them reads and
 * writes the attached automation properties of elements — the automation id a peer reports, the
 * name a screen reader announces, the heading level and set positions a tree structure is told
 * about. This file answers the nine statics over an attached-property store keyed by the element
 * pointer, the same keying the peers use (the IUIElement entry the engine hands out is the element
 * object itself, so the pointer identifies the object across callers); entries live for the
 * process, as the peer registry does.
 *
 * The Get/Set pairs are real: a string property answers its stored value and NULL when unset, a
 * value property answers its stored number or the property's unset default, and Set of a null
 * string clears. The DependencyProperty getters (get_NameProperty and kin) are not answered yet —
 * the engine has no DependencyProperty objects for the attached properties, and the callers that
 * ask will name themselves in the trace through raven_trace_not_implemented. The same holds for
 * the collection-valued members (ControlledPeers, Annotations, DescribedBy, FlowsTo, FlowsFrom),
 * whose IVector backing the engine does not keep. */
#define INITGUID
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "xaml_internal.h"
#include <string.h>
#include "windows_ui_xaml_automation_properties.h"
#include "xaml_automation.h"

#define ALLOC(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size))
#define FREE(block) HeapFree(GetProcessHeap(), 0, (block))

typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics statics1;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStaticsVtbl statics1_vtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics2 statics2;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics2Vtbl statics2_vtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics3 statics3;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics3Vtbl statics3_vtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics4 statics4;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics4Vtbl statics4_vtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics5 statics5;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics5Vtbl statics5_vtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics6 statics6;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics6Vtbl statics6_vtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics7 statics7;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics7Vtbl statics7_vtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics8 statics8;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics8Vtbl statics8_vtbl;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics9 statics9;
typedef __x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics9Vtbl statics9_vtbl;

/* The runtime class name the winmd declares; the activation key is the same spelling. */
static const WCHAR properties_class[] = L"Windows.UI.Xaml.Automation.AutomationProperties";

/* The string-valued attached properties, in their storage slots. */
enum attached_text { attached_accelerator = 0, attached_access_key, attached_automation_id,
	attached_help_text, attached_item_status, attached_item_type, attached_name,
	attached_localized_landmark, attached_full_description, attached_localized_control_type,
	attached_text_count };
/* The value-valued attached properties, in their storage slots with the answers an unset property gives. */
enum attached_number { attached_required_for_form = 0, attached_live_setting, attached_accessibility_view,
	attached_position_in_set, attached_size_of_set, attached_level, attached_landmark_type,
	attached_is_peripheral, attached_is_data_valid, attached_culture, attached_heading_level,
	attached_is_dialog, attached_control_type, attached_number_count };

struct attached {
	IInspectable *element;
	HSTRING strings[attached_text_count];
	int64_t numbers[attached_number_count];
	IInspectable *labeled_by;
};

/* The entries never leave: an element keeps its attached properties for the process lifetime, as the
 * peer registry keeps its peers. */
static struct attached *attached_vector;
static size_t attached_count, attached_capacity;

static struct attached *attached_find(IInspectable *element)
{
	for (size_t at = 0; at < attached_count; ++at)
		if (attached_vector[at].element == element)
			return &attached_vector[at];
	if (attached_count == attached_capacity) {
		size_t capacity = attached_capacity ? attached_capacity * 2 : 8;
		/* HeapReAlloc refuses a null block the way Windows does (the XML DOM and the DataWriter growth
		 * sites learned this before); the store's first allocation is a HeapAlloc. */
		struct attached *grown = attached_vector
			? HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, attached_vector, capacity * sizeof *grown)
			: HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, capacity * sizeof *grown);
		if (!grown)
			return NULL;
		attached_vector = grown;
		attached_capacity = capacity;
	}
	struct attached *fresh = &attached_vector[attached_count++];
	fresh->element = element;
	/* An unset IsDataValidForForm answers TRUE, the property's documented default. */
	fresh->numbers[attached_is_data_valid] = 1;
	return fresh;
}

static HRESULT inspectable_iids(ULONG *count, IID **iids)
{
	if (!count || !iids)
		return E_POINTER;
	*count = 0;
	*iids = NULL;
	return S_OK;
}

static HRESULT inspectable_trust(TrustLevel *level)
{
	if (!level)
		return E_POINTER;
	*level = BaseTrust;
	return S_OK;
}

static LONG factory_refs;

struct properties_factory {
	const IActivationFactoryVtbl *activation;
	struct { const statics1_vtbl *lpVtbl; } first;
	struct { const statics2_vtbl *lpVtbl; } second;
	struct { const statics3_vtbl *lpVtbl; } third;
	struct { const statics4_vtbl *lpVtbl; } fourth;
	struct { const statics5_vtbl *lpVtbl; } fifth;
	struct { const statics6_vtbl *lpVtbl; } sixth;
	struct { const statics7_vtbl *lpVtbl; } seventh;
	struct { const statics8_vtbl *lpVtbl; } eighth;
	struct { const statics9_vtbl *lpVtbl; } ninth;
};

static struct properties_factory properties_factory_instance;

static void factory_addref(void)
{
	InterlockedIncrement(&factory_refs);
}

static void factory_release(void)
{
	InterlockedDecrement(&factory_refs);
}

static HRESULT factory_name(HSTRING *name)
{
	if (!name)
		return E_POINTER;
	return WindowsCreateString(properties_class, (UINT32)lstrlenW(properties_class), name);
}

static HRESULT factory_query(REFIID iid, void **out)
{
	if (!out)
		return E_POINTER;
	*out = NULL;
	if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IInspectable) ||
		IsEqualGUID(iid, &IID_IActivationFactory))
		*out = (void *)&properties_factory_instance.activation;
	else if (IsEqualGUID(iid, &IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics))
		*out = (void *)&properties_factory_instance.first;
	else if (IsEqualGUID(iid, &IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics2))
		*out = (void *)&properties_factory_instance.second;
	else if (IsEqualGUID(iid, &IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics3))
		*out = (void *)&properties_factory_instance.third;
	else if (IsEqualGUID(iid, &IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics4))
		*out = (void *)&properties_factory_instance.fourth;
	else if (IsEqualGUID(iid, &IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics5))
		*out = (void *)&properties_factory_instance.fifth;
	else if (IsEqualGUID(iid, &IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics6))
		*out = (void *)&properties_factory_instance.sixth;
	else if (IsEqualGUID(iid, &IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics7))
		*out = (void *)&properties_factory_instance.seventh;
	else if (IsEqualGUID(iid, &IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics8))
		*out = (void *)&properties_factory_instance.eighth;
	else if (IsEqualGUID(iid, &IID__x_ABI_CWindows_CUI_CXaml_CAutomation_CIAutomationPropertiesStatics9))
		*out = (void *)&properties_factory_instance.ninth;
	else
		return E_NOINTERFACE;
	factory_addref();
	return S_OK;
}

/* The one per-part boilerplate: the parts only route back to the common forms. */
#define PROPERTIES_PART(iface) \
	static HRESULT WINAPI part##iface##_query(iface *This, REFIID iid, void **out) \
	{ (void)This; return factory_query(iid, out); } \
	static ULONG WINAPI part##iface##_addref(iface *This) \
	{ (void)This; factory_addref(); return (ULONG)factory_refs; } \
	static ULONG WINAPI part##iface##_release(iface *This) \
	{ (void)This; factory_release(); return (ULONG)factory_refs; } \
	static HRESULT WINAPI part##iface##_iids(iface *This, ULONG *count, IID **iids) \
	{ (void)This; return inspectable_iids(count, iids); } \
	static HRESULT WINAPI part##iface##_name(iface *This, HSTRING *name) \
	{ (void)This; return factory_name(name); } \
	static HRESULT WINAPI part##iface##_trust(iface *This, TrustLevel *level) \
	{ (void)This; return inspectable_trust(level); }

PROPERTIES_PART(statics1)
PROPERTIES_PART(statics2)
PROPERTIES_PART(statics3)
PROPERTIES_PART(statics4)
PROPERTIES_PART(statics5)
PROPERTIES_PART(statics6)
PROPERTIES_PART(statics7)
PROPERTIES_PART(statics8)
PROPERTIES_PART(statics9)

static HRESULT WINAPI factory_activate(IActivationFactory *iface, IInspectable **out)
{
	(void)iface;
	if (!out)
		return E_POINTER;
	*out = NULL;
	/* The class is statics-only; nothing constructs it. */
	raven_trace_not_implemented(__func__);
	return E_NOTIMPL;
}

static HRESULT WINAPI activation_query(IActivationFactory *iface, REFIID iid, void **out)
{
	(void)iface;
	return factory_query(iid, out);
}
static ULONG WINAPI activation_addref(IActivationFactory *iface)
{
	(void)iface;
	factory_addref();
	return (ULONG)factory_refs;
}
static ULONG WINAPI activation_release(IActivationFactory *iface)
{
	(void)iface;
	factory_release();
	return (ULONG)factory_refs;
}
static HRESULT WINAPI activation_iids(IActivationFactory *iface, ULONG *count, IID **iids)
{
	(void)iface;
	return inspectable_iids(count, iids);
}
static HRESULT WINAPI activation_name(IActivationFactory *iface, HSTRING *name)
{
	(void)iface;
	return factory_name(name);
}
static HRESULT WINAPI activation_trust(IActivationFactory *iface, TrustLevel *level)
{
	(void)iface;
	return inspectable_trust(level);
}

static const IActivationFactoryVtbl activation_table = {
	.QueryInterface = activation_query,
	.AddRef = activation_addref,
	.Release = activation_release,
	.GetIids = activation_iids,
	.GetRuntimeClassName = activation_name,
	.GetTrustLevel = activation_trust,
	.ActivateInstance = factory_activate,
};

/* The attached-property store answers a statics method through the element its first argument hands over. */
static struct attached *attached_of(uint64_t element)
{
	if (!element)
		return NULL;
	return attached_find((IInspectable *)(uintptr_t)element);
}

/* A string property: Get answers the stored value, NULL unset; Set of NULL clears. */
static HRESULT text_get(struct attached *slot, enum attached_text which, uint64_t out)
{
	if (!out)
		return E_POINTER;
	if (!slot)
		return E_POINTER;
	if (!slot->strings[which]) {
		*(HSTRING *)(uintptr_t)out = NULL;
		return S_OK;
	}
	return WindowsDuplicateString(slot->strings[which], (HSTRING *)(uintptr_t)out);
}

static HRESULT text_set(struct attached *slot, enum attached_text which, uint64_t value)
{
	HSTRING copy = NULL;
	HRESULT result;
	if (!slot)
		return E_POINTER;
	if (value) {
		result = WindowsDuplicateString((HSTRING)(uintptr_t)value, &copy);
		if (FAILED(result))
			return result;
	}
	if (slot->strings[which])
		WindowsDeleteString(slot->strings[which]);
	slot->strings[which] = copy;
	return S_OK;
}

/* A value property: Get answers the stored number or the default, Set stores it. */
static HRESULT number_get(struct attached *slot, enum attached_number which, uint64_t out)
{
	if (!out)
		return E_POINTER;
	if (!slot) {
		/* Reading an unset attached property answers its default, the way Windows does — the traced
		 * Shapr3D run reads GetLevel/GetIsPeripheral of a fresh Frame and treats the refusal as a
		 * failed projection, dying on a null reference right after naming the frame RootFrame. */
		*(int32_t *)(uintptr_t)out = 0;
		return S_OK;
	}
	*(int32_t *)(uintptr_t)out = (int32_t)slot->numbers[which];
	return S_OK;
}

static HRESULT number_set(struct attached *slot, enum attached_number which, uint64_t value)
{
	if (!slot)
		return E_POINTER;
	slot->numbers[which] = (int64_t)(int32_t)value;
	return S_OK;
}

/* The DependencyProperty getters: the engine keeps no DependencyProperty objects for the attached
 * properties yet; the asking caller names itself in the trace. */
static HRESULT property_boxed(void)
{
	raven_trace_not_implemented(__func__);
	return E_NOTIMPL;
}

#define TEXT_PROPERTY(suffix, slot) \
	static HRESULT WINAPI get_##suffix(statics1 *This, uint64_t out) \
	{ (void)This; (void)out; return property_boxed(); } \
	static HRESULT WINAPI Get_##suffix(statics1 *This, uint64_t element, uint64_t out) \
	{ (void)This; return text_get(attached_of(element), slot, out); } \
	static HRESULT WINAPI Set_##suffix(statics1 *This, uint64_t element, uint64_t value) \
	{ \
		(void)This; \
		char note[80]; \
		HRESULT result = text_set(attached_of(element), slot, value); \
		wsprintfA(note, "raven-autoprops: Set" #suffix " element %llx value %llx -> %lx\n", \
			(unsigned long long)element, (unsigned long long)value, (unsigned long)result); \
		OutputDebugStringA(note); \
		return result; \
	}

TEXT_PROPERTY(AcceleratorKey, attached_accelerator)
TEXT_PROPERTY(AccessKey, attached_access_key)
TEXT_PROPERTY(AutomationId, attached_automation_id)
TEXT_PROPERTY(HelpText, attached_help_text)
TEXT_PROPERTY(ItemStatus, attached_item_status)
TEXT_PROPERTY(ItemType, attached_item_type)
TEXT_PROPERTY(Name, attached_name)

static HRESULT WINAPI get_IsRequiredForFormProperty(statics1 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetIsRequiredForForm(statics1 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return number_get(attached_of(element), attached_required_for_form, out);
}
static HRESULT WINAPI SetIsRequiredForForm(statics1 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return number_set(attached_of(element), attached_required_for_form, value);
}
static HRESULT WINAPI get_ItemStatusProperty(statics1 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetItemStatus(statics1 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return text_get(attached_of(element), attached_item_status, out);
}
static HRESULT WINAPI SetItemStatus(statics1 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return text_set(attached_of(element), attached_item_status, value);
}
static HRESULT WINAPI get_ItemTypeProperty(statics1 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetItemType(statics1 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return text_get(attached_of(element), attached_item_type, out);
}
static HRESULT WINAPI SetItemType(statics1 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return text_set(attached_of(element), attached_item_type, value);
}
static HRESULT WINAPI get_LabeledByProperty(statics1 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetLabeledBy(statics1 *This, uint64_t element, uint64_t out)
{
	struct attached *slot;
	(void)This;
	if (!out)
		return E_POINTER;
	if (!element)
		return E_POINTER;
	slot = attached_find((IInspectable *)(uintptr_t)element);
	*(void **)(uintptr_t)out = slot && slot->labeled_by ? slot->labeled_by : NULL;
	return S_OK;
}
static HRESULT WINAPI SetLabeledBy(statics1 *This, uint64_t element, uint64_t value)
{
	struct attached *slot;
	(void)This;
	if (!element)
		return E_POINTER;
	slot = attached_find((IInspectable *)(uintptr_t)element);
	if (!slot)
		return E_OUTOFMEMORY;
	slot->labeled_by = (IInspectable *)(uintptr_t)value;
	return S_OK;
}
static HRESULT WINAPI get_LiveSettingProperty(statics1 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetLiveSetting(statics1 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return number_get(attached_of(element), attached_live_setting, out);
}
static HRESULT WINAPI SetLiveSetting(statics1 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return number_set(attached_of(element), attached_live_setting, value);
}

static const statics1_vtbl statics1_table = {
	.QueryInterface = partstatics1_query,
	.AddRef = partstatics1_addref,
	.Release = partstatics1_release,
	.GetIids = partstatics1_iids,
	.GetRuntimeClassName = partstatics1_name,
	.GetTrustLevel = partstatics1_trust,
	.get_AcceleratorKeyProperty = get_AcceleratorKey,
	.GetAcceleratorKey = Get_AcceleratorKey,
	.SetAcceleratorKey = Set_AcceleratorKey,
	.get_AccessKeyProperty = get_AccessKey,
	.GetAccessKey = Get_AccessKey,
	.SetAccessKey = Set_AccessKey,
	.get_AutomationIdProperty = get_AutomationId,
	.GetAutomationId = Get_AutomationId,
	.SetAutomationId = Set_AutomationId,
	.get_HelpTextProperty = get_HelpText,
	.GetHelpText = Get_HelpText,
	.SetHelpText = Set_HelpText,
	.get_IsRequiredForFormProperty = get_IsRequiredForFormProperty,
	.GetIsRequiredForForm = GetIsRequiredForForm,
	.SetIsRequiredForForm = SetIsRequiredForForm,
	.get_ItemStatusProperty = get_ItemStatusProperty,
	.GetItemStatus = GetItemStatus,
	.SetItemStatus = SetItemStatus,
	.get_ItemTypeProperty = get_ItemTypeProperty,
	.GetItemType = GetItemType,
	.SetItemType = SetItemType,
	.get_LabeledByProperty = get_LabeledByProperty,
	.GetLabeledBy = GetLabeledBy,
	.SetLabeledBy = SetLabeledBy,
	.get_NameProperty = get_Name,
	.GetName = Get_Name,
	.SetName = Set_Name,
	.get_LiveSettingProperty = get_LiveSettingProperty,
	.GetLiveSetting = GetLiveSetting,
	.SetLiveSetting = SetLiveSetting,
};

/* The second statics adds the accessibility view and the controlled peers. The view stores; the
 * controlled-peers collection has no backing yet. */
static HRESULT WINAPI get_AccessibilityViewProperty(statics2 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetAccessibilityView(statics2 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return number_get(attached_of(element), attached_accessibility_view, out);
}
static HRESULT WINAPI SetAccessibilityView(statics2 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return number_set(attached_of(element), attached_accessibility_view, value);
}
static HRESULT WINAPI get_ControlledPeersProperty(statics2 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetControlledPeers(statics2 *This, uint64_t element, uint64_t out)
{
	(void)This; (void)element; (void)out;
	raven_trace_not_implemented(__func__);
	return E_NOTIMPL;
}

static const statics2_vtbl statics2_table = {
	.QueryInterface = partstatics2_query,
	.AddRef = partstatics2_addref,
	.Release = partstatics2_release,
	.GetIids = partstatics2_iids,
	.GetRuntimeClassName = partstatics2_name,
	.GetTrustLevel = partstatics2_trust,
	.get_AccessibilityViewProperty = get_AccessibilityViewProperty,
	.GetAccessibilityView = GetAccessibilityView,
	.SetAccessibilityView = SetAccessibilityView,
	.get_ControlledPeersProperty = get_ControlledPeersProperty,
	.GetControlledPeers = GetControlledPeers,
};

/* The third statics adds the set positions and the annotations. */
static HRESULT WINAPI get_PositionInSetProperty(statics3 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetPositionInSet(statics3 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return number_get(attached_of(element), attached_position_in_set, out);
}
static HRESULT WINAPI SetPositionInSet(statics3 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return number_set(attached_of(element), attached_position_in_set, value);
}
static HRESULT WINAPI get_SizeOfSetProperty(statics3 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetSizeOfSet(statics3 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return number_get(attached_of(element), attached_size_of_set, out);
}
static HRESULT WINAPI SetSizeOfSet(statics3 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return number_set(attached_of(element), attached_size_of_set, value);
}
static HRESULT WINAPI get_LevelProperty(statics3 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetLevel(statics3 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return number_get(attached_of(element), attached_level, out);
}
static HRESULT WINAPI SetLevel(statics3 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return number_set(attached_of(element), attached_level, value);
}
static HRESULT WINAPI get_AnnotationsProperty(statics3 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetAnnotations(statics3 *This, uint64_t element, uint64_t out)
{
	(void)This; (void)element; (void)out;
	raven_trace_not_implemented(__func__);
	return E_NOTIMPL;
}

static const statics3_vtbl statics3_table = {
	.QueryInterface = partstatics3_query,
	.AddRef = partstatics3_addref,
	.Release = partstatics3_release,
	.GetIids = partstatics3_iids,
	.GetRuntimeClassName = partstatics3_name,
	.GetTrustLevel = partstatics3_trust,
	.get_PositionInSetProperty = get_PositionInSetProperty,
	.GetPositionInSet = GetPositionInSet,
	.SetPositionInSet = SetPositionInSet,
	.get_SizeOfSetProperty = get_SizeOfSetProperty,
	.GetSizeOfSet = GetSizeOfSet,
	.SetSizeOfSet = SetSizeOfSet,
	.get_LevelProperty = get_LevelProperty,
	.GetLevel = GetLevel,
	.SetLevel = SetLevel,
	.get_AnnotationsProperty = get_AnnotationsProperty,
	.GetAnnotations = GetAnnotations,
};

/* The fourth statics adds the landmark type and its localized form. */
static HRESULT WINAPI get_LandmarkTypeProperty(statics4 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetLandmarkType(statics4 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return number_get(attached_of(element), attached_landmark_type, out);
}
static HRESULT WINAPI SetLandmarkType(statics4 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return number_set(attached_of(element), attached_landmark_type, value);
}
static HRESULT WINAPI get_LocalizedLandmarkTypeProperty(statics4 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetLocalizedLandmarkType(statics4 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return text_get(attached_of(element), attached_localized_landmark, out);
}
static HRESULT WINAPI SetLocalizedLandmarkType(statics4 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return text_set(attached_of(element), attached_localized_landmark, value);
}

static const statics4_vtbl statics4_table = {
	.QueryInterface = partstatics4_query,
	.AddRef = partstatics4_addref,
	.Release = partstatics4_release,
	.GetIids = partstatics4_iids,
	.GetRuntimeClassName = partstatics4_name,
	.GetTrustLevel = partstatics4_trust,
	.get_LandmarkTypeProperty = get_LandmarkTypeProperty,
	.GetLandmarkType = GetLandmarkType,
	.SetLandmarkType = SetLandmarkType,
	.get_LocalizedLandmarkTypeProperty = get_LocalizedLandmarkTypeProperty,
	.GetLocalizedLandmarkType = GetLocalizedLandmarkType,
	.SetLocalizedLandmarkType = SetLocalizedLandmarkType,
};

/* The fifth statics adds the peripheral and form-data flags, the description strings and the
 * element relations. The relations' collections have no backing yet. */
static HRESULT WINAPI get_IsPeripheralProperty(statics5 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetIsPeripheral(statics5 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return number_get(attached_of(element), attached_is_peripheral, out);
}
static HRESULT WINAPI SetIsPeripheral(statics5 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return number_set(attached_of(element), attached_is_peripheral, value);
}
static HRESULT WINAPI get_IsDataValidForFormProperty(statics5 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetIsDataValidForForm(statics5 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return number_get(attached_of(element), attached_is_data_valid, out);
}
static HRESULT WINAPI SetIsDataValidForForm(statics5 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return number_set(attached_of(element), attached_is_data_valid, value);
}
static HRESULT WINAPI get_FullDescriptionProperty(statics5 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetFullDescription(statics5 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return text_get(attached_of(element), attached_full_description, out);
}
static HRESULT WINAPI SetFullDescription(statics5 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return text_set(attached_of(element), attached_full_description, value);
}
static HRESULT WINAPI get_LocalizedControlTypeProperty(statics5 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetLocalizedControlType(statics5 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return text_get(attached_of(element), attached_localized_control_type, out);
}
static HRESULT WINAPI SetLocalizedControlType(statics5 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return text_set(attached_of(element), attached_localized_control_type, value);
}
static HRESULT WINAPI get_DescribedByProperty(statics5 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetDescribedBy(statics5 *This, uint64_t element, uint64_t out)
{
	(void)This; (void)element; (void)out;
	raven_trace_not_implemented(__func__);
	return E_NOTIMPL;
}
static HRESULT WINAPI get_FlowsToProperty(statics5 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetFlowsTo(statics5 *This, uint64_t element, uint64_t out)
{
	(void)This; (void)element; (void)out;
	raven_trace_not_implemented(__func__);
	return E_NOTIMPL;
}
static HRESULT WINAPI get_FlowsFromProperty(statics5 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetFlowsFrom(statics5 *This, uint64_t element, uint64_t out)
{
	(void)This; (void)element; (void)out;
	raven_trace_not_implemented(__func__);
	return E_NOTIMPL;
}

static const statics5_vtbl statics5_table = {
	.QueryInterface = partstatics5_query,
	.AddRef = partstatics5_addref,
	.Release = partstatics5_release,
	.GetIids = partstatics5_iids,
	.GetRuntimeClassName = partstatics5_name,
	.GetTrustLevel = partstatics5_trust,
	.get_IsPeripheralProperty = get_IsPeripheralProperty,
	.GetIsPeripheral = GetIsPeripheral,
	.SetIsPeripheral = SetIsPeripheral,
	.get_IsDataValidForFormProperty = get_IsDataValidForFormProperty,
	.GetIsDataValidForForm = GetIsDataValidForForm,
	.SetIsDataValidForForm = SetIsDataValidForForm,
	.get_FullDescriptionProperty = get_FullDescriptionProperty,
	.GetFullDescription = GetFullDescription,
	.SetFullDescription = SetFullDescription,
	.get_LocalizedControlTypeProperty = get_LocalizedControlTypeProperty,
	.GetLocalizedControlType = GetLocalizedControlType,
	.SetLocalizedControlType = SetLocalizedControlType,
	.get_DescribedByProperty = get_DescribedByProperty,
	.GetDescribedBy = GetDescribedBy,
	.get_FlowsToProperty = get_FlowsToProperty,
	.GetFlowsTo = GetFlowsTo,
	.get_FlowsFromProperty = get_FlowsFromProperty,
	.GetFlowsFrom = GetFlowsFrom,
};

/* The sixth statics adds the culture. */
static HRESULT WINAPI get_CultureProperty(statics6 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetCulture(statics6 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return number_get(attached_of(element), attached_culture, out);
}
static HRESULT WINAPI SetCulture(statics6 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return number_set(attached_of(element), attached_culture, value);
}

static const statics6_vtbl statics6_table = {
	.QueryInterface = partstatics6_query,
	.AddRef = partstatics6_addref,
	.Release = partstatics6_release,
	.GetIids = partstatics6_iids,
	.GetRuntimeClassName = partstatics6_name,
	.GetTrustLevel = partstatics6_trust,
	.get_CultureProperty = get_CultureProperty,
	.GetCulture = GetCulture,
	.SetCulture = SetCulture,
};

/* The seventh statics adds the heading level. */
static HRESULT WINAPI get_HeadingLevelProperty(statics7 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetHeadingLevel(statics7 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return number_get(attached_of(element), attached_heading_level, out);
}
static HRESULT WINAPI SetHeadingLevel(statics7 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return number_set(attached_of(element), attached_heading_level, value);
}

static const statics7_vtbl statics7_table = {
	.QueryInterface = partstatics7_query,
	.AddRef = partstatics7_addref,
	.Release = partstatics7_release,
	.GetIids = partstatics7_iids,
	.GetRuntimeClassName = partstatics7_name,
	.GetTrustLevel = partstatics7_trust,
	.get_HeadingLevelProperty = get_HeadingLevelProperty,
	.GetHeadingLevel = GetHeadingLevel,
	.SetHeadingLevel = SetHeadingLevel,
};

/* The eighth statics adds the dialog flag. */
static HRESULT WINAPI get_IsDialogProperty(statics8 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetIsDialog(statics8 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return number_get(attached_of(element), attached_is_dialog, out);
}
static HRESULT WINAPI SetIsDialog(statics8 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return number_set(attached_of(element), attached_is_dialog, value);
}

static const statics8_vtbl statics8_table = {
	.QueryInterface = partstatics8_query,
	.AddRef = partstatics8_addref,
	.Release = partstatics8_release,
	.GetIids = partstatics8_iids,
	.GetRuntimeClassName = partstatics8_name,
	.GetTrustLevel = partstatics8_trust,
	.get_IsDialogProperty = get_IsDialogProperty,
	.GetIsDialog = GetIsDialog,
	.SetIsDialog = SetIsDialog,
};

/* The ninth statics adds the control type. */
static HRESULT WINAPI get_AutomationControlTypeProperty(statics9 *This, uint64_t out)
{
	(void)This; (void)out;
	return property_boxed();
}
static HRESULT WINAPI GetAutomationControlType(statics9 *This, uint64_t element, uint64_t out)
{
	(void)This;
	return number_get(attached_of(element), attached_control_type, out);
}
static HRESULT WINAPI SetAutomationControlType(statics9 *This, uint64_t element, uint64_t value)
{
	(void)This;
	return number_set(attached_of(element), attached_control_type, value);
}

static const statics9_vtbl statics9_table = {
	.QueryInterface = partstatics9_query,
	.AddRef = partstatics9_addref,
	.Release = partstatics9_release,
	.GetIids = partstatics9_iids,
	.GetRuntimeClassName = partstatics9_name,
	.GetTrustLevel = partstatics9_trust,
	.get_AutomationControlTypeProperty = get_AutomationControlTypeProperty,
	.GetAutomationControlType = GetAutomationControlType,
	.SetAutomationControlType = SetAutomationControlType,
};

static struct properties_factory properties_factory_instance = {
	&activation_table,
	{&statics1_table},
	{&statics2_table},
	{&statics3_table},
	{&statics4_table},
	{&statics5_table},
	{&statics6_table},
	{&statics7_table},
	{&statics8_table},
	{&statics9_table},
};

/* The activation factory entry the chain in xaml.c reaches: only the class the catalog registers. */
HRESULT raven_automation_properties_factory(HSTRING name, IActivationFactory **out)
{
	UINT32 length = 0;
	const WCHAR *text;
	if (!out)
		return E_POINTER;
	*out = NULL;
	text = WindowsGetStringRawBuffer(name, &length);
	if (!text || length != ARRAYSIZE(properties_class) - 1 ||
		CompareStringOrdinal(text, (int)length, properties_class, -1, FALSE) != CSTR_EQUAL)
		return CLASS_E_CLASSNOTAVAILABLE;
	factory_addref();
	*out = (IActivationFactory *)&properties_factory_instance;
	return S_OK;
}
