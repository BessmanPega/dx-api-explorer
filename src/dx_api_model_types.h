#ifndef DX_API_MODEL_TYPES_H
#define DX_API_MODEL_TYPES_H

#include <string>
#include <unordered_map>
#include <vector>
#include "dx_api_helper_types.h"

namespace dx_api_explorer
{
// https://docs.pega.com/bundle/dx-api/page/platform/dx-api/understand-dx-api-response.html#d33668e1783
struct field_t
{
    std::string id;
    std::string class_id;
    std::string label;
    std::string type;
    std::string data;
    std::string json;

    bool is_special = false;
    bool is_class_key = false;
    bool is_dirty = false;
};
using field_map_t = std::unordered_map<std::string, field_t>;

// Types of components: https://docs.pega.com/bundle/constellation-sdk/page/constellation-sdks/sdks/using-dx-component-builder.html#d15866e82
enum component_type_t
{
    component_type_unspecified,
    component_type_unknown,
    // ...always first.

    // Design system extensions:
    // ...nothing yet!

    // Infrastructure:
    component_type_reference,
    component_type_region,
    component_type_view,

    // Fields:
    component_type_currency,
    component_type_text_area,
    component_type_text_input,

    // Templates:
    component_type_default_form,

    // Widgets:
    // ...nothing yet!

    // Always last:
    component_type_count
};

// Component strings as we'll see them in DX API responses, should be in same order as enum.
constexpr std::array<const char*, component_type_count> component_type_strings =
{
    "Unspecified",
    "Unknown",
    "Reference",
    "Region",
    "View",
    "Currency",
    "TextArea",
    "TextInput",
    "DefaultForm"
};

static_assert(decltype(component_type_strings)().size() == component_type_count,
    "Must have the same number of component type enums and strings.");

// Component megastruct:
struct component_t;
using component_list_t = std::vector<component_t>;
struct component_t
{
    component_type_t type = component_type_t::component_type_unspecified;
    std::string name;
    std::string class_id;
    std::string key; // Identifies this rule, or the referenced rule in the case of references/fields.
    // ...required for all components!

    std::string label;
    std::string json;
    std::string debug_string;
    std::string broken_string;

    bool is_readonly = false;
    bool is_required = false;
    bool is_disabled = false;
    bool is_broken = false;
    bool is_selected = false;

    component_type_t ref_type = component_type_t::component_type_unspecified; // Referenced component / type of template.
    component_list_t children;
};
using component_map_t = std::unordered_map<std::string, component_t>;

// https://docs.pega.com/bundle/dx-api/page/platform/dx-api/understand-dx-api-response.html#d33668e2455
struct action_t
{
    std::string id;
    std::string name;
    std::string type;
};
using action_map_t = std::unordered_map<std::string, action_t>;

// https://docs.pega.com/bundle/dx-api/page/platform/dx-api/understand-dx-api-response.html#d33668e1053
struct assignment_t
{
    std::string id;
    std::string name;
    bool can_perform = false;
    action_map_t actions;
};
using assignment_map_t = std::unordered_map<std::string, assignment_t>;

// https://docs.pega.com/bundle/dx-api/page/platform/dx-api/understand-dx-api-response.html#d33668e800
using content_map_t = std::unordered_map<std::string, std::string>;

// https://docs.pega.com/bundle/dx-api/page/platform/dx-api/endpoint-get-casetypes.html
struct case_type_t
{
    std::string id;
    std::string name;
};

// https://docs.pega.com/bundle/dx-api/page/platform/dx-api/understand-dx-api-response.html#d33668e350
struct case_info_t
{
    case_type_t	type;
    std::string id;
    std::string business_id;
    std::string create_time;
    std::string created_by;
    std::string last_update_time;
    std::string last_updated_by;
    std::string name;
    std::string owner;
    std::string status;

    assignment_map_t	assignments;
    content_map_t       content;
};

// https://docs.pega.com/bundle/dx-api/page/platform/dx-api/understand-dx-api-response.html#d33668e1783
struct resources_t
{
    field_map_t fields;
    component_map_t components;
};
}

#endif // DX_API_MODEL_TYPES_H