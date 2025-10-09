#ifndef DX_API_NETWORK_PROCS_H
#define DX_API_NETWORK_PROCS_H

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "dx_api_app_types.h"
#include "dx_api_constants.h"
#include "dx_api_model_procs.h"
#include "dx_api_model_types.h"
#include "dx_api_network_types.h"
#include "httplib.h"
#include "imgui.h"
#include "nlohmann/json.hpp"

namespace dx_api_explorer
{
// Helper to convert httplib headers into a human-friendly string.
auto to_string(const httplib::Headers& headers) -> std::string
{
    std::string result;

    for (auto& header : headers)
    {
        result += std::format("{}: {}\n", header.first, header.second);
    }

    return result;
}

// Returns an httplib client with the most common initialization and an endpoint built from the provided arg strings.
template<typename... strings>
auto init_call_and_create_client(net_call_t& call, http_method_t method, strings... args) -> httplib::Client
{
    httplib::Client client(call.server);
    client.set_bearer_token_auth(call.access_token);
    call.method = http_method_strings[method];
    std::vector<std::string_view> params = { args... };

    call.endpoint = call.dx_api_path;
    for (std::string_view sv : params)
    {
        call.endpoint += sv;
    }

    return client;
}

// Maps an httplib result into the output params of a dx call struct.
auto set_call_output(net_call_t& call, const httplib::Result& result) -> void
{
    auto err = result.error();
    if (err != httplib::Error::Success)
    {
        call.error_message = httplib::to_string(err);
        return;
    }

    call.etag = result->get_header_value("eTag");
    call.response_headers = to_string(result->headers);
    call.response_body = result->body;
    call.succeeded = true;
}

// Should be called from the network thread. Executes the specified network call and stores the response.
auto handle_request(net_call_t& call) -> void
{
    switch (call.type)
    {
    case net_call_type_t::login:
    {
        httplib::Client cli(call.server);
        cli.set_basic_auth(call.client_id, call.client_secret);
        call.method = "POST";

        httplib::Headers request_headers =
        {
            { "Accept", "application/json" }
        };
        call.request_headers = to_string(request_headers);

        call.request_body = std::format("grant_type=password&username={}&password={}", call.user_id, call.password);

        auto result = cli.Post(call.endpoint, request_headers, call.request_body, "application/x-www-form-urlencoded");
        auto err = result.error();
        if (err != httplib::Error::Success)
        {
            call.error_message = httplib::to_string(err);
            return;
        }

        call.response_headers = to_string(result->headers);
        call.response_body = result->body;
        call.succeeded = true;
    } break;
    case net_call_type_t::refresh_case_types:
    {
        auto client = init_call_and_create_client(call, http_method_get, "/casetypes");
        auto result = client.Get(call.endpoint);
        set_call_output(call, result);
    } break;
    case net_call_type_t::create_case:
    {
        auto client = init_call_and_create_client(call, http_method_post, "/cases");
        nlohmann::json request_body_json =
        {
            { "caseTypeID", call.work_type_id }
        };
        call.request_body = request_body_json.dump(json_indent);
        auto result = client.Post(call.endpoint, call.request_body, "application/json");
        set_call_output(call, result);
    } break;
    case net_call_type_t::open_assignment:
    {
        auto client = init_call_and_create_client(call, http_method_get, "/assignments/", call.id1);
        auto result = client.Get(call.endpoint);
        set_call_output(call, result);
    } break;
    case net_call_type_t::open_assignment_action:
    {
        auto client = init_call_and_create_client(call, http_method_get, "/assignments/", call.id1, "/actions/", call.id2);
        auto result = client.Get(call.endpoint);
        set_call_output(call, result);
    } break;
    case net_call_type_t::submit_assignment_action:
    {
        auto client = init_call_and_create_client(call, http_method_patch, "/assignments/", call.id1, "/actions/", call.id2);
        httplib::Headers request_headers =
        {
            //{ "if-match", call.etag }
            { "If-Match", call.etag }
        };
        call.request_headers = to_string(request_headers);
        auto result = client.Patch(call.endpoint, request_headers, call.request_body, "application/json");
        set_call_output(call, result);
    } break;
    }
}

// Should be called from the main thread. Processes a response from a network call and maps the response into
// the application context.
auto handle_response(net_call_t& call, app_context_t& app) -> void
{
    using json = nlohmann::json;

    app.flash = "";
    app.endpoint = call.method + " - " + app.server + call.endpoint;
    app.request_headers = call.request_headers;
    app.response_headers = call.response_headers;
    app.request_body = "";
    app.response_body = "";

    // Format JSON bodies if applicable.
    // Not super efficient, but not happening in a tight inner loop so how much does it matter?
    app.request_body = call.request_body;
    try
    {
        if (!call.response_body.empty())
        {
            // Parsing the result twice (here and below) makes me twitch. But in practice,
            // there is no perceptible performance impact, and this is for debugging anyway.
            // We could disable this (and all debugging facilities) for a release build if
            // the need ever arose.
            app.response_body = json::parse(call.response_body).dump(json_indent);
        }
    }
    catch (const std::exception& e)
    {
        IM_UNUSED(e);
        app.response_body = call.response_body;
    }

    if (!call.succeeded)
    {
        app.flash = call.error_message;
        return;
    }

    switch (call.type)
    {
    case net_call_type_t::login:
    {
        app.access_token = call.access_token;
        try
        {
            json j = json::parse(call.response_body);
            if (j.contains("access_token"))
            {
                app.access_token = j["access_token"];
                app.status = app_status_t::logged_in;
            }
            else
            {
                throw std::runtime_error("No access token received.");
            }
        }
        catch (const std::exception& ex)
        {
            app.flash = app.flash + "Failed to login: " + ex.what();
        }
    } break;
    case net_call_type_t::refresh_case_types:
    {
        app.case_types.clear();
        try
        {
            json j = json::parse(call.response_body);
            if (j.contains("applicationIsConstellationCompatible") &&
                j["applicationIsConstellationCompatible"].get<bool>() &&
                j.contains("caseTypes"))
            {
                for (auto& item : j["caseTypes"].items())
                {
                    auto& case_type_json = item.value();
                    case_type_t case_type;
                    case_type.id = case_type_json["ID"].get<std::string>();
                    case_type.name = case_type_json["name"].get<std::string>();
                    app.case_types.push_back(case_type);
                }
            }
            else
            {
                app.flash = "Not constellation compatible and/or no case types defined.";
            }
        }
        catch (const std::exception& e)
        {
            app.flash = std::format("Failed to refresh cases: {}", e.what());
        }
    } break;
    case net_call_type_t::create_case:
    {
        try
        {
            parse_dx_response(app, call.response_body);
            app.status = app_status_t::open_case;
        }
        catch (const std::exception& e)
        {
            app.flash = std::format("Failed to create case: {}", e.what());
        }
    } break;
    case net_call_type_t::open_assignment:
    {
        try
        {
            parse_dx_response(app, call.response_body);
            app.etag = call.etag;
            app.open_assignment_id = call.id1;
            app.status = app_status_t::open_assignment;
        }
        catch (const std::exception& e)
        {
            app.flash = std::format("Failed to open assignment: {}", e.what());
        }
    } break;
    case net_call_type_t::open_assignment_action:
    {
        try
        {
            parse_dx_response(app, call.response_body);
            app.etag = call.etag;
            app.open_assignment_id = call.id1;
            app.open_action_id = call.id2;
            app.status = app_status_t::open_action;
        }
        catch (const std::exception& e)
        {
            app.flash = std::format("Failed to open assignment action: {}", +e.what());
        }
    } break;
    case net_call_type_t::submit_assignment_action:
    {
        try
        {
            parse_dx_response(app, call.response_body);
            app.open_assignment_id = call.id1;
            app.status = app_status_t::open_case;
        }
        catch (const std::exception& e)
        {
            app.flash = std::format("Failed to submit assignment action: {}", e.what());
        }
    } break;
    }
}

// Returns a net call struct with the most common initialization.
auto make_net_call(const app_context_t& app, const net_call_type_t type) -> net_call_t
{
    net_call_t call;
    call.type = type;
    call.server = app.server;
    call.access_token = app.access_token;
    call.dx_api_path = app.dx_api_path;

    return call;
}

// Pushes a network call to login to the Pega instance.
auto login(app_context_t& app) -> void
{
    auto call = make_net_call(app, net_call_type_t::login);
    call.client_id = app.client_id;
    call.client_secret = app.client_secret;
    call.user_id = app.user_id;
    call.password = app.password;
    call.endpoint = app.token_endpoint;

    std::scoped_lock lock(app.dx_request_mutex);
    app.dx_request_queue.push(call);
}

// Pushes a network call to refresh the case types defined in the Pega app.
auto refresh_case_types(app_context_t& app) -> void
{
    auto call = make_net_call(app, net_call_type_t::refresh_case_types);
    std::scoped_lock lock(app.dx_request_mutex);
    app.dx_request_queue.push(call);
}

// Pushes a network call to create a new case of the specified type.
auto create_case(app_context_t& app, std::string_view work_type_id) -> void
{
    auto call = make_net_call(app, net_call_type_t::create_case);
    call.work_type_id = work_type_id;

    std::scoped_lock lock(app.dx_request_mutex);
    app.dx_request_queue.push(call);
}

// Pushes a network call to open the specified assignment.
auto open_assignment(app_context_t& app, std::string_view assignment_id) -> void
{
    auto call = make_net_call(app, net_call_type_t::open_assignment);
    call.id1 = assignment_id;
    std::scoped_lock lock(app.dx_request_mutex);
    app.dx_request_queue.push(call);
}

// Pushes a network call to open the specified action.
auto open_assignment_action(app_context_t& app, std::string_view action_id) -> void
{
    auto call = make_net_call(app, net_call_type_t::open_assignment_action);
    call.id1 = app.open_assignment_id;
    call.id2 = action_id;
    std::scoped_lock lock(app.dx_request_mutex);
    app.dx_request_queue.push(call);
}

// Pushes a network call to submit the currently open assignment action. Assumes that
// validation has already passed.
auto submit_open_assignment_action(app_context_t& app) -> void
{
    using json = nlohmann::json;

    auto call = make_net_call(app, net_call_type_t::submit_assignment_action);

    json content;
    bool have_content = false;
    for (const auto& field_pair : app.resources.fields)
    {
        const auto& field = field_pair.second;
        if (field.is_special || field.is_class_key) continue;
        if (field.is_dirty)
        {
            content[field.id] = field.data;
            have_content = true;
        }
    }

    json body;
    body["content"] = content;

    call.id1 = app.open_assignment_id;
    call.id2 = app.open_action_id;
    call.etag = app.etag;

    if (have_content)
    {
        call.request_body = body.dump(json_indent);
    }

    std::scoped_lock lock(app.dx_request_mutex);
    app.dx_request_queue.push(call);
}
}

#endif // DX_API_NETWORK_PROCS_H