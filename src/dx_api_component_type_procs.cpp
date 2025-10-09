/* This file contains all of the functions that switch (literally or
figuratively) on the component type. It broadly consists of routines for
creating and drawing components, and in effect, it's where you go when you want
to understand how a component works, or implement support for a new component.
*/

#include "dx_api_app_types.h"
#include "dx_api_draw_procs.h"
#include "dx_api_model_types.h"
#include "dx_api_model_procs.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "nlohmann/json.hpp"

// We use this to ensure we always explicitly account for all component types.
// See https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4061
#pragma warning(default : 4061)

namespace dx_api_explorer
{
// Recursively makes component and its children from DX API JSON response data.
auto make_component_r(const nlohmann::json& component_json, app_context_t& app, std::string_view parent_class_id) -> component_t
{
    component_t new_component;
    new_component.json = component_json.dump(json_indent);
    new_component.type = to_component_type(component_json["type"]);

    switch (new_component.type)
    {
    case component_type_unknown:
    {
        new_component.class_id = parent_class_id;
        // ...always first.

        new_component.name = component_json["type"];

        // Always last:
        new_component.debug_string = to_string(new_component.type, new_component.name);
    } break;
    case component_type_reference:
    {
        new_component.class_id = parent_class_id;
        // ...always first.

        auto& config_json = component_json["config"];
        new_component.name = resolve_name(config_json["name"], app.case_info.content, new_component.class_id);
        new_component.ref_type = to_component_type(config_json["type"]);

        // References might specify a context. If that context exists, we use it if we support it. If it exists
        // and we don't support it, we mark this reference as broken.
        if (config_json.contains("context"))
        {
            std::string context = config_json["context"];

            if (context.substr(0, 6) == "@CLASS")
            {
                // "@CLASS The-Class-Name"
                //  0123456789...
                //         ^
                //         Start here.
                new_component.class_id = context.substr(7);
            }
            else
            {
                new_component.is_broken = true;
                new_component.broken_string = std::format("Unsupported context: {}", context);
            }
        }

        // Always last:
        new_component.debug_string = to_string(new_component.type, new_component.name, new_component.ref_type);
    } break;
    case component_type_region:
    {
        new_component.class_id = parent_class_id;
        // ...always first.

        new_component.name = resolve_name(component_json["name"], app.case_info.content, new_component.class_id);

        // Always last:
        new_component.debug_string = to_string(new_component.type, new_component.name);
    } break;
    case component_type_view:
    {
        new_component.class_id = component_json["classID"];
        // ...always first.

        new_component.name = resolve_name(component_json["name"], app.case_info.content, new_component.class_id);

        // Views usually, but not always, specify a template in the config.
        auto& config_json = component_json["config"];
        if (config_json.contains("template")) new_component.ref_type = to_component_type(config_json["template"]);

        // Always last:
        new_component.debug_string = to_string(new_component.type, new_component.name, new_component.ref_type);
    } break;
    case component_type_currency:
    case component_type_text_area:
    case component_type_text_input:
    {
        new_component.class_id = parent_class_id;
        // ...always first.

        auto& config_json = component_json["config"];
        new_component.name = resolve_name(config_json["value"], app.case_info.content, new_component.class_id, false);
        new_component.label = resolve_label(config_json["label"], app.resources.fields, new_component.class_id);

        // Check for optional attributes.
        if (config_json.contains("disabled")) new_component.is_disabled = to_bool(config_json["disabled"]);
        if (config_json.contains("readOnly")) new_component.is_readonly = to_bool(config_json["readOnly"]);
        if (config_json.contains("required")) new_component.is_required = to_bool(config_json["required"]);

        // Always last:
        new_component.debug_string = to_string(new_component.type, new_component.label);
    } break;
    // Ignore these:
    case component_type_count:
    case component_type_default_form:
    case component_type_unspecified:
    default: break;
    }

    // Validate the component and finalize it.
    if (new_component.name.empty()
        || new_component.class_id.empty()
        || new_component.type == component_type_unspecified)
    {
        std::string error_message = std::format("Failed to make component from JSON:\n{}", new_component.json);
        throw std::runtime_error(error_message);
    }
    else
    {
        new_component.key = make_key(new_component.class_id, new_component.name);
    }

    // Process children:
    if (component_json.contains("children"))
    {
        for (const auto& child : component_json["children"])
        {
            //component_t new_child_component = make_component_r(child, app, parent_class_id);
            component_t new_child_component = make_component_r(child, app, new_component.class_id);
            new_component.children.push_back(new_child_component);
        }
    }

    return new_component;
};

// Recursively validates that component and all of its children are in a valid
// state for submission. Only applies to field components.
auto validate_component_r(const component_t& component, const component_map_t& components, const field_map_t& fields) -> bool
{
    bool is_valid = true;

    switch (component.type)
    {
    case component_type_currency:
    case component_type_text_input:
    case component_type_text_area:
    {
        if (component.is_required)
        {
            const auto& field = fields.at(component.key);
            if (field.data.empty())
            {
                is_valid = false;
            }
        }
    } break;
    // Ignore these:
    case component_type_count:
    case component_type_default_form:
    case component_type_reference:
    case component_type_region:
    case component_type_unknown:
    case component_type_unspecified:
    case component_type_view:
    default: break;
    }

    // Process children.
    if (!component.children.empty())
    {
        for (auto& child : component.children)
        {
            if (!is_valid) break;
            is_valid = validate_component_r(child, components, fields);
        }
    }

    return is_valid;
}

// Recursively draws components, returns the coordinates of the lower-right corner of the bounding box for the component and its children.
auto draw_component_r(component_t& component, resources_t& resources, int& id, std::string& component_debug_json, bool show_xray) -> ImVec2
{
    ImVec2 bbul{}, bblr{}; // Bounding box upper-left and lower-right corners.

    if (show_xray)
    {
        ImGui::TreePush(&id);
    }

    ImGui::PushID(id++);
    switch (component.type)
    {
    case component_type_reference:
    {
        if (!component.is_broken)
        {
            if (show_xray)
            {
                ImGui::Text(component.debug_string.c_str());
                bbul = ImGui::GetItemRectMin();
                bblr = ImGui::GetItemRectMax();
            }

            auto& reference = resources.components.at(component.key);
            ImVec2 ref_bblr = draw_component_r(reference, resources, id, component_debug_json, show_xray);

            bblr.x = std::max(bblr.x, ref_bblr.x);
            bblr.y = std::max(bblr.y, ref_bblr.y);
        }
    } break;
    case component_type_currency:
    case component_type_text_area:
    case component_type_text_input:
    {
        auto& field = resources.fields.at(component.key);
        if (is_editable(component, field))
        {
            field.is_dirty = true;

            if (component.type == component_type_text_area)
            {
                ImGui::InputTextMultiline(component.label.c_str(), &field.data);
            }
            else
            {
                ImGui::InputText(component.label.c_str(), &field.data);
            }

            bbul = ImGui::GetItemRectMin();
            bblr = ImGui::GetItemRectMax();

            if (component.is_required)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "*");
            }
        }
        else
        {
            ImGui::LabelText(component.label.c_str(), field.data.c_str());
            bbul = ImGui::GetItemRectMin();
            bblr = ImGui::GetItemRectMax();
        }

        ImGui::SameLine();

        auto was_component_selected = component.is_selected;
        if (component.is_selected)
        {
            ImGui::PushStyleColor(ImGuiCol_TextDisabled, selected_text_color);
        }

        ImGui::TextDisabled("(?)");

        // Adjust bounding box width to account for any appended widgets.
        bblr.x = ImGui::GetItemRectMax().x;

        if (ImGui::IsItemClicked() && !component.is_selected)
        {
            // Deselect all components, then select this one. It will render as selected on the next frame.
            for (auto& pair : resources.components)
            {
                deselect_component_r(pair.second, resources.components);
            }
            component.is_selected = true;
            component_debug_json = component.json;
        }
        if (was_component_selected)
        {
            ImGui::PopStyleColor();
        }

        ImGui::SetItemTooltip(component.key.c_str());

    } break;
    case component_type_count:
    case component_type_default_form:
    case component_type_region:
    case component_type_unknown:
    case component_type_unspecified:
    case component_type_view:
    default:
    {
        if (show_xray)
        {
            ImGui::Text(component.debug_string.c_str());
            bbul = ImGui::GetItemRectMin();
            bblr = ImGui::GetItemRectMax();
        }

        // If this is a view with an unsupported template, bail.
        bool should_process_children = !component.children.empty();
        if (component.type == component_type_view)
        {
            if (component.ref_type == component_type_unspecified ||
                component.ref_type == component_type_unknown)
            {
                should_process_children = false;
            }
        }

        // Process children.
        if (should_process_children)
        {
            for (auto& child : component.children)
            {
                ImVec2 child_bblr = draw_component_r(child, resources, id, component_debug_json, show_xray);

                bblr.x = std::max(bblr.x, child_bblr.x);
                bblr.y = std::max(bblr.y, child_bblr.y);
            }
        }
    } break;
    }
    ImGui::PopID();

    if (show_xray)
    {
        ImGui::TreePop();

        // Draw a bounding box around this component and its children.
        ImGui::GetWindowDrawList()->AddRect(bbul, bblr, IM_COL32(255, 0, 0, 255)); // Red color
    }

    return bblr;
}

// Recursively draws debug component information.
auto draw_component_debug_r(component_t& component, component_map_t& component_map, std::string& component_debug_json) -> void
{
    ImGui::TreePush(&component);

    auto text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    if (component.is_selected)
    {
        text_color = selected_text_color;
    }
    ImGui::TextColored(text_color, component.debug_string.c_str());
    if (ImGui::IsItemClicked() && !component.is_selected)
    {
        // Deselect all components, then select this one. It will render as selected on the next frame.
        for (auto& pair : component_map)
        {
            deselect_component_r(pair.second, component_map);
        }
        component.is_selected = true;
        component_debug_json = component.json;
    }

    if (component.is_broken)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0, 0.0, 0.0, 1.0), "(!)");
        ImGui::SetItemTooltip(component.broken_string.c_str());
    }
    else if (component.type == component_type_reference)
    {
        auto& reference = component_map.at(component.key);
        draw_component_debug_r(reference, component_map, component_debug_json);
    }

    if (!component.children.empty())
    {
        for (auto& child : component.children)
        {
            draw_component_debug_r(child, component_map, component_debug_json);
        }
    }

    ImGui::TreePop();
}

// Recursively marks all components as not selected.
auto deselect_component_r(component_t& component, component_map_t& component_map) -> void
{
    component.is_selected = false;

    if (component.type == component_type_reference && !component.is_broken)
    {
        auto& reference = component_map.at(component.key);
        deselect_component_r(reference, component_map);
    }

    if (!component.children.empty())
    {
        for (auto& child : component.children)
        {
            deselect_component_r(child, component_map);
        }
    }
}
}