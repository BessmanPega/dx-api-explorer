#ifndef DX_API_DRAW_PROCS_H
#define DX_API_DRAW_PROCS_H

#include <algorithm>
#include <cassert>
#include <string>
#include "dx_api_app_types.h"
#include "dx_api_constants.h"
#include "dx_api_helper_procs.h"
#include "dx_api_model_procs.h"
#include "dx_api_model_types.h"
#include "dx_api_network_procs.h"
#include "imgui.h"
#include "imgui_stdlib.h"

namespace dx_api_explorer
{
// Defined in main.cpp
auto deselect_component_r(component_t& component, component_map_t& component_map) -> void;

// Defined in main.cpp
auto draw_component_debug_r(component_t& component, component_map_t& component_map, std::string& component_debug_json) -> void;

// Defined in main.cpp
auto draw_component_r(component_t& component, resources_t& resources, int& id, std::string& component_debug_json, bool show_xray) -> ImVec2;

// Draws a spinner/throbber/whatchamacallit to indicate waiting for an action to complete.
auto draw_spinner() -> void
{
    static std::string spinner = "|/-\\";
    static int index = 0;
    static auto last_ticks = get_ticks();

    ImGui::Text("Loading %c", spinner[index]);

    auto current_ticks = get_ticks();
    auto delta_ticks = current_ticks - last_ticks;
    if (delta_ticks > spinner_period_ticks)
    {
        if (++index >= spinner.length()) index = 0;
        last_ticks = current_ticks;
    }
}

// Draws the main menu.
auto draw_main_menu(app_context_t& app) -> void
{
    if (ImGui::BeginMenuBar())
    {
        // Logged-in menu
        if (app.status != app_status_t::logged_out)
        {
            // User menu
            if (ImGui::BeginMenu(app.user_id.c_str()))
            {
                if (ImGui::MenuItem("Logout"))
                {
                    app.case_types.clear();
                    app.status = app_status_t::logged_out;
                }
                ImGui::EndMenu();
            }

            // Create menu
            if (ImGui::BeginMenu("Create"))
            {
                if (ImGui::MenuItem("Refresh Case Types"))
                {
                    refresh_case_types(app);
                }

                // Case types
                if (!app.case_types.empty())
                {
                    ImGui::Separator();
                    for (auto& work_type : app.case_types)
                    {
                        if (ImGui::MenuItem(work_type.name.c_str()))
                        {
                            create_case(app, work_type.id);
                        }
                        ImGui::SetItemTooltip(work_type.id.c_str());
                    }
                }

                ImGui::EndMenu();
            }
        }

        // View menu
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Reset layout"))
            {
                app.requested_events.push_back(app_event_type_t::reset_window_layout);
            }
            ImGui::MenuItem("Show debug window", nullptr, &app.show_debug_window);
            ImGui::MenuItem("Show XRay", nullptr, &app.show_xray);
            ImGui::MenuItem("Show Dear ImGui demo", nullptr, &app.show_demo_window);
            if (ImGui::BeginMenu("Font size"))
            {
                for (int i = 0; i < font_sizes.size(); ++i)
                {
                    auto& io = ImGui::GetIO();
                    bool selected = (io.FontDefault->FontSize == font_sizes[i].first);

                    if (ImGui::MenuItem(font_sizes[i].second, nullptr, &selected))
                    {
                        app.font_index = i;
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

// Draws the login form.
auto draw_login_form(app_context_t& app) -> void
{
    ImGui::InputText("Server", &app.server);
    ImGui::InputText("DX API Path", &app.dx_api_path);
    ImGui::InputText("Token Endpoint", &app.token_endpoint);
    ImGui::InputText("Client ID", &app.client_id);
    ImGui::InputText("Client Secret", &app.client_secret);
    ImGui::InputText("User ID", &app.user_id);
    ImGui::InputText("Password", &app.password, ImGuiInputTextFlags_Password);

    if (ImGui::Button("Login"))
    {
        login(app);
    }
}

// Draws the currently open case.
auto draw_open_case(app_context_t& app) -> void
{
    auto& work = app.case_info;

    if (ImGui::CollapsingHeader("Case", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SeparatorText("Info");
        ImGui::LabelText("Case ID", work.business_id.c_str());
        ImGui::SetItemTooltip(work.id.c_str());
        ImGui::LabelText("Name", work.name.c_str());
        ImGui::SetItemTooltip("%s: %s", work.type.id.c_str(), work.type.name.c_str());
        ImGui::LabelText("Status", work.status.c_str());
        ImGui::LabelText("Owner", work.owner.c_str());
        ImGui::LabelText("Created on", work.create_time.c_str());
        ImGui::LabelText("Created by", work.created_by.c_str());
        ImGui::LabelText("Updated on", work.last_update_time.c_str());
        ImGui::LabelText("Updated by", work.last_updated_by.c_str());

        if (!work.assignments.empty())
        {
            ImGui::SeparatorText("Assignments");
            for (auto& assignment_pair : work.assignments)
            {
                auto& assignment = assignment_pair.second;

                ImGui::PushID(&assignment);
                if (assignment.can_perform)
                {
                    if (ImGui::Button(assignment.name.c_str()))
                    {
                        open_assignment(app, assignment.id);
                    }
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, { 1, 0, 0, 1 });
                    ImGui::Button(assignment.name.c_str());
                    ImGui::PopStyleColor();
                    ImGui::SetItemTooltip("You cannot perform this assignment.");
                }
                ImGui::PopID();
            }
        }
    }
}

// Draws the currently open assignment.
auto draw_open_assignment(app_context_t& app)
{
    assert(!app.open_assignment_id.empty());
    auto& assignment = app.case_info.assignments[app.open_assignment_id];

    if (ImGui::CollapsingHeader("Assignment", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SeparatorText("Info");
        ImGui::LabelText("Name", assignment.name.c_str());

        ImGui::SeparatorText("Actions");
        for (auto& pair : assignment.actions)
        {
            auto& action = pair.second;
            ImGui::PushID(&action);
            if (ImGui::Button(action.name.c_str()))
            {
                open_assignment_action(app, action.id);
            }
            ImGui::PopID();
        }
    }
}

// Draws the currently open assignment action.
auto draw_open_assignment_action(app_context_t& app) -> void
{
    assert(!app.open_assignment_id.empty());
    assert(!app.open_action_id.empty());
    auto& action = app.case_info.assignments[app.open_assignment_id].actions[app.open_action_id];

    if (ImGui::CollapsingHeader("Action", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SeparatorText("Info");
        ImGui::LabelText("Name", action.name.c_str());

        ImGui::SeparatorText("UI");
        int component_id = 0;
        draw_component_r(app.resources.components[app.root_component_key], app.resources, component_id, app.component_debug_json, app.show_xray);

        if (ImGui::Button("Submit"))
        {
            const auto& root_component = app.resources.components[app.root_component_key];
            auto are_components_valid = validate_component_r(root_component, app.resources.components, app.resources.fields);
            if (are_components_valid)
            {
                submit_open_assignment_action(app);
            }
            else
            {
                app.flash = "Validation failed. Did you fill out all required fields?";
            }
        }
    }
}

// Draws the main user interface.
auto draw_main_window(app_context_t& app) -> void
{
    static bool first_call = true;

    if (first_call)
    {
        first_call = false;

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        auto font_size = ImGui::GetFontSize();
        ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + font_size, main_viewport->WorkPos.y + font_size), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(main_viewport->WorkSize.x / 2.0f - font_size * 1.5f, main_viewport->WorkSize.y - font_size * 2.0f), ImGuiCond_FirstUseEver);
    }

    if (std::ranges::contains(app.active_events, app_event_type_t::reset_window_layout))
    {
        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        auto font_size = ImGui::GetFontSize();
        ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + font_size, main_viewport->WorkPos.y + font_size));
        ImGui::SetNextWindowSize(ImVec2(main_viewport->WorkSize.x / 2.0f - font_size * 1.5f, main_viewport->WorkSize.y - font_size * 2.0f));
    }

    ImGui::Begin("Main", nullptr, ImGuiWindowFlags_MenuBar);

    // If we have net ops pending, make a note and show a spinner. We
    // do things this way to minimize the amount of time we're 
    // holding the queue mutex, and to not have to remember to call
    // ImGui::End() in multiple places.
    bool have_pending_requests = false;
    {
        std::scoped_lock lock(app.dx_request_mutex);
        if (!app.dx_request_queue.empty())
        {
            have_pending_requests = true;
            draw_spinner();
        }
    }

    // Don't render any more interactive UI if we have a pending network
    // operation. That means  UI interaction is effectively blocked by net ops, no
    // different from if we ran on a single thread. But we can at least render
    // a spinner/progress bar/something better than just freezing up.
    // 
    // Even if we were single threaded, we would still want to work in this manner —
    // queue up network operations to take some action and refresh our cache,
    // then render the updated cache — and its not much harder to do that in multithreaded
    // way.
    if (!have_pending_requests)
    {
        draw_main_menu(app);

        // Show login form.
        if (app.status == app_status_t::logged_out)
        {
            draw_login_form(app);
        }

        // Show open work object.
        else if (app.status == app_status_t::open_case || app.status == app_status_t::open_assignment || app.status == app_status_t::open_action)
        {
            draw_open_case(app);

            // Show open assignment.
            if (app.status == app_status_t::open_assignment || app.status == app_status_t::open_action)
            {
                draw_open_assignment(app);
            }

            // Show open action.
            if (app.status == app_status_t::open_action)
            {
                draw_open_assignment_action(app);
            }
        }
    }

    ImGui::End();
}

// Draws information about network calls and responses.
auto draw_debug_calls(app_context_t& app) -> void
{
    auto font_size = ImGui::GetFontSize();

    ImGui::PushItemWidth(font_size * -10); // In practice this seems to work out to a little less than 20 characters of space for labels.

    ImGui::InputText("Endpoint", &app.endpoint, ImGuiInputTextFlags_ReadOnly);

    ImGui::InputTextMultiline("Request headers", &app.request_headers, ImVec2(0, 3 * font_size), ImGuiInputTextFlags_ReadOnly);
    ImGui::InputTextMultiline("Request body", &app.request_body, ImVec2(0, 5 * font_size), ImGuiInputTextFlags_ReadOnly);
    ImGui::InputTextMultiline("Response headers", &app.response_headers, ImVec2(0, 10 * font_size), ImGuiInputTextFlags_ReadOnly);
    ImGui::InputTextMultiline("Response body", &app.response_body, ImVec2(0, 20 * font_size), ImGuiInputTextFlags_ReadOnly);

    ImGui::PopItemWidth();
}

// Draws a tree view of components in use starting with root.
auto draw_debug_components(app_context_t& app) -> void
{
    auto font_size = ImGui::GetFontSize();

    ImGui::BeginGroup();
    draw_component_debug_r(app.resources.components[app.root_component_key], app.resources.components, app.component_debug_json);
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::InputTextMultiline("##ComponentJSON", &app.component_debug_json, ImVec2(-font_size, -font_size), ImGuiInputTextFlags_ReadOnly);
}

// Draws fields currently in use.
auto draw_debug_fields(app_context_t& app) -> void
{
    auto font_size = ImGui::GetFontSize();

    ImGui::BeginGroup();
    for (const auto& field : app.resources.fields)
    {
        ImGui::Text(field.first.c_str());
        if (ImGui::IsItemClicked())
        {
            app.field_debug_json = field.second.json;
        }
    }
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::InputTextMultiline("##FieldJSON", &app.field_debug_json, ImVec2(-font_size, -font_size), ImGuiInputTextFlags_ReadOnly);
}

// Draws content currently in use.
auto draw_debug_content(app_context_t& app) -> void
{
    for (const auto& content : app.case_info.content)
    {
        ImGui::Text("%s: %s", content.first.c_str(), content.second.c_str());
    }
}

// Draws the debug user interface.
auto draw_debug_window(app_context_t& app) -> void
{
    static bool first_call = true;

    if (first_call)
    {
        first_call = false;

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        auto font_size = ImGui::GetFontSize();
        float next_pos_x = main_viewport->WorkSize.x / 2.0f + font_size / 2.0f;
        ImGui::SetNextWindowPos(ImVec2(next_pos_x, main_viewport->WorkPos.y + font_size), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(main_viewport->WorkSize.x - next_pos_x - font_size, main_viewport->WorkSize.y - font_size * 2.0f), ImGuiCond_FirstUseEver);
    }

    if (std::ranges::contains(app.active_events, app_event_type_t::reset_window_layout))
    {
        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        auto font_size = ImGui::GetFontSize();
        float next_pos_x = main_viewport->WorkSize.x / 2.0f + font_size / 2.0f;
        ImGui::SetNextWindowPos(ImVec2(next_pos_x, main_viewport->WorkPos.y + font_size));
        ImGui::SetNextWindowSize(ImVec2(main_viewport->WorkSize.x - next_pos_x - font_size, main_viewport->WorkSize.y - font_size * 2.0f));
    }

    ImGui::Begin("Debug", &app.show_debug_window);

    if (ImGui::BeginTabBar("DebugTabBar"))
    {
        if (ImGui::BeginTabItem("Calls"))
        {
            draw_debug_calls(app);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Structure"))
        {
            if (!app.resources.components.empty())
            {
                draw_debug_components(app);
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Fields"))
        {
            if (!app.resources.fields.empty())
            {
                draw_debug_fields(app);
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Content"))
        {
            if (!app.case_info.content.empty())
            {
                draw_debug_content(app);
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}

// Draws a "modal" that displays the flash message with a button to clear it.
auto draw_flash_window(app_context_t& app) -> void
{
    static bool first_call = true;

    if (first_call)
    {
        first_call = false;

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkSize.x * 0.4f, main_viewport->WorkSize.y * 0.4f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(main_viewport->WorkSize.x * 0.2f, main_viewport->WorkSize.y * 0.2f), ImGuiCond_FirstUseEver);
    }

    ImGui::Begin("Alert");
    ImGui::TextWrapped(app.flash.c_str());
    if (ImGui::Button("OK"))
    {
        app.flash.clear();
    }
    ImGui::End();
}
}

#endif // DX_API_DRAW_PROCS_H