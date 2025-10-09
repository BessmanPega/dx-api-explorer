#ifndef DX_API_MODEL_PROCS
#define DX_API_MODEL_PROCS

#include <cassert>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include "dx_api_app_types.h"
#include "dx_api_constants.h"
#include "dx_api_helper_procs.h"
#include "dx_api_model_types.h"
#include "nlohmann/json.hpp"

namespace dx_api_explorer
{
// Maps component type to string.
auto to_c_str(component_type_t type) -> const char*
{
    return component_type_strings[type];
}

// Returns the component type which corresponds to the provided string.
auto to_component_type(std::string_view str) -> component_type_t
{
    return to_enum<component_type_t>(str, component_type_strings);
}

// Produces a debug string for the component type and name.
auto to_string(component_type_t type, std::string_view name) -> std::string
{
    return std::format("{}: {}", to_c_str(type), name);
}

// Produces a debug string for a component type, name, and referenced component type.
auto to_string(component_type_t type, std::string_view name, component_type_t ref_type) -> std::string
{
    return std::format("{}: {} [{}]", to_c_str(type), name, to_c_str(ref_type));
}

// Returns true if the provided field should be rendered with an editable interface.
auto is_editable(const component_t& component, const field_t& field) -> bool
{
    if (component.is_readonly)  return false;
    if (component.is_disabled)  return false;
    if (field.is_special)       return false;
    if (field.is_class_key)     return false;

    return true;
}

// Creates a key from a class id and a name such as: "The-Class-ID.TheName"
auto make_key(std::string_view class_id, std::string_view name) -> std::string
{
    return std::format("{}.{}", class_id, name);
}

// Takes a DX API label property like "@L Blah", "@FL .BlahBlah", or "Blah Blah Blah"
// and returns correct response:
//  "@L Blah"           -> "Blah".
//  "@FL .BlahBlah"     -> The value of the label property for field .BlahBlah.
//  "Blah Blah Blah"    -> "Blah Blah Blah"
auto resolve_label(std::string_view raw_label, const field_map_t& fields, std::string_view class_id) -> std::string
{
    assert(!class_id.empty());
    std::string result{ raw_label };

    if (!raw_label.empty())
    {
        if (raw_label[0] == '@')
        {
            if (raw_label[1] == 'L')
            {
                // "@L Blah"
                //  0123456
                //     ^
                //     Start here.
                result = raw_label.substr(3);
            }
            else if (raw_label[1] == 'F' && raw_label[2] == 'L')
            {
                // "@FL .Blah"
                //  012345678
                //       ^
                //       Start here.
                auto field_id = raw_label.substr(5);
                auto field_key = make_key(class_id, field_id);
                result = fields.at(field_key).label;
            }
        }
    }

    return result;
};

// Returns the value of the provided name in the content map. In strict mode, throws an exception if the name can't be
// found or the content doesn't match.
auto get_content(const content_map_t& content, std::string_view class_id, const char* name, bool strict_mode = true) -> std::string
{
    std::string result;
    if (content.contains("classID"))
    {
        if (content.at("classID") == class_id)
        {
            if (content.contains(name))
            {
                result = content.at(name);
            }
            else if (strict_mode)
            {
                std::string e = std::format("Could not resolve name: {}\nName not found in content.", name);
                throw std::runtime_error(e);
            }
        }
        else if (strict_mode)
        {
            std::string e = std::format("Could not resolve name: {}\ncontent['classID'] = {}\nclass_id = {}", name, content.at("classID"), class_id);
            throw std::runtime_error(e);
        }
    }
    else if (strict_mode)
    {
        std::string e = std::format("Could not resolve name: {}\ncontent does not contain 'classID'", name);
        throw std::runtime_error(e);
    }
    return result;
}

// Takes a DX API name property like "@P .Blah" or "Blah Blah Blah" and returns correct response:
//  "@P .Blah"  -> "Blah" if dereferencing isn't required, otherwise the value of the content for "Blah".
//  "Blah Blah" -> "Blah Blah"
auto resolve_name(std::string_view raw_name, const content_map_t& content, std::string_view class_id, bool dereference_property_name = true) -> std::string
{
    assert(!class_id.empty());
    std::string result{ raw_name };

    if (raw_name.length() >= 2 && raw_name.substr(0, 2) == "@P")
    {
        // "@P .Blah"
        //  01234567
        //      ^
        //      Start here.
        std::string name{ raw_name.substr(4) };
        if (dereference_property_name)
        {
            result = get_content(content, class_id, name.c_str());
        }
        else
        {
            result = name;
        }
    }

    return result;
}

// Defined in main.cpp
auto validate_component_r(const component_t& component, const component_map_t& components, const field_map_t& fields) -> bool;

// Defined in main.cpp
auto make_component_r(const nlohmann::json& component_json, app_context_t& app, std::string_view parent_class_id = "") -> component_t;

// https://docs.pega.com/bundle/dx-api/page/platform/dx-api/understand-dx-api-response.html#d33668e350
auto parse_dx_response(app_context_t& app, std::string_view response_body) -> void
{
    auto j = nlohmann::json::parse(response_body);
    auto& info = j["data"]["caseInfo"];

    // Case info.
    {
        app.case_info.id = info["ID"];
        app.case_info.business_id = info["businessID"];
        app.case_info.type.id = info["caseTypeID"];
        app.case_info.type.name = info["caseTypeName"];
        app.case_info.create_time = info["createTime"];
        app.case_info.created_by = info["createdBy"];
        app.case_info.last_update_time = info["lastUpdateTime"];
        app.case_info.last_updated_by = info["lastUpdatedBy"];
        app.case_info.name = info["name"];
        app.case_info.owner = info["owner"];
        app.case_info.status = info["status"];

        app.case_info.assignments.clear();
        for (const auto& assignment : info["assignments"])
        {
            assignment_t new_assignment;
            new_assignment.id = assignment["ID"];
            new_assignment.name = assignment["name"];
            new_assignment.can_perform = to_bool(assignment["canPerform"]);

            for (const auto& action : assignment["actions"])
            {
                action_t new_action;
                new_action.id = action["ID"];
                new_action.name = action["name"];
                new_action.type = action["type"];

                new_assignment.actions[new_action.id] = new_action;
            }

            app.case_info.assignments[new_assignment.id] = new_assignment;
        }

        app.case_info.content.clear();
        for (const auto& content : info["content"].items())
        {
            std::string k = content.key();
            const auto& content_value = content.value();

            std::string v;
            if (content_value.is_string())
            {
                v = content_value.get<std::string>();
            }
            else if (content_value.is_number_integer())
            {
                v = std::to_string(content_value.get<int>());
            }
            else if (content_value.is_number_float())
            {
                v = std::to_string(content_value.get<double>());

                // Trim trailing zeros:
                auto end = v.find_last_not_of('0') + 1;
                v.erase(end);
            }
            else if (content_value.is_boolean())
            {
                v = std::to_string(content_value.get<bool>());
            }

            app.case_info.content[k] = v;
        }
    } // End of case info parsing.

    // UI resources.
    if (j.contains("uiResources"))
    {
        auto& ui_resources_json = j["uiResources"];
        auto& resources_json = ui_resources_json["resources"];
        auto& fields_json = resources_json["fields"];
        auto& views_json = resources_json["views"];

        // Fields:
        {
            app.resources.fields.clear();
            for (const auto& field_array : fields_json.items())
            {
                for (const auto& field : field_array.value().items())
                {
                    auto& value = field.value();

                    field_t new_field;
                    new_field.id = field_array.key();
                    new_field.json = value.dump(json_indent);
                    new_field.type = value["type"];

                    // Sometimes we get an "Unknown" type which is completely malformed,
                    // in which case we just skip it entirely.
                    if (to_lower(new_field.type) == "unknown") continue;

                    new_field.class_id = value["classID"];

                    if (value.contains("label")) new_field.label = value["label"];
                    if (value.contains("isSpecial")) new_field.is_special = to_bool(value["isSpecial"]);
                    if (value.contains("isClassKey")) new_field.is_class_key = to_bool(value["isClassKey"]);

                    new_field.data = get_content(app.case_info.content, new_field.class_id, new_field.id.c_str(), false);

                    auto new_field_key = make_key(new_field.class_id, new_field.id);
                    app.resources.fields[new_field_key] = new_field;
                }
            }
        }

        // Views (components):
        {
            app.resources.components.clear();
            for (const auto& view_array : views_json.items())
            {
                for (const auto& view : view_array.value().items())
                {
                    const auto& value = view.value();
                    component_t new_component = make_component_r(value, app);
                    app.resources.components[new_component.key] = new_component;
                }
            }
        }

        // Root:
        {
            const auto& config_json = ui_resources_json["root"]["config"];
            std::string context = config_json["context"];

            if (context == "caseInfo.content")
            {
                std::string name = config_json["name"];
                std::string type = config_json["type"];

                if (type == "view")
                {
                    std::string& class_id = app.case_info.content.at("classID");
                    app.root_component_key = make_key(class_id, name);
                }
                else
                {
                    std::string e = std::format("Root component uses unsupported type: {}", type);
                    throw std::runtime_error(e);
                }
            }
            else
            {
                std::string e = std::format("Root component uses unsupported context: ", context);
                throw std::runtime_error(e);
            }
        }
    } // End of UI resources parsing.
}
}

#endif // DX_API_MODEL_PROCS