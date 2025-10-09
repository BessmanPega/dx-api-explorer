#ifndef DX_API_HELPER_PROCS_H
#define DX_API_HELPER_PROCS_H

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <string>
#include <string_view>
#include "dx_api_app_types.h"
#include "dx_api_constants.h"
#include "dx_api_helper_types.h"
#include "nlohmann/json.hpp"
#include "SDL.h"

namespace dx_api_explorer
{
// Returns the number of milliseconds that have passed since the high resolution clock epoch.
auto get_ticks() -> std::chrono::milliseconds
{
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto ticks = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    return ticks;
}

// Return lowercase string. "Good enough is good enough."
auto to_lower(std::string_view str) -> std::string
{
    std::string result{ str };
    std::transform(str.begin(), str.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return result;
}

// This converts a string to an enum type identifier. It assumes that:
//  - The first enum is 0 and corresponds to "unspecified."
//  - The second enum is 1 and corresponds to "unknown."
//  - The provided enum strings correspond 1:1 to the count of the provided enum type.
//
// Usage looks like:
//  enum_type_t result = to_enum<enum_type_t>(str, enum_type_strings);
//
// You shouldn't have to specifiy the enum count because that should be inferred
// from the enum strings parameter.
template<typename enum_t, int enum_count>
auto to_enum(std::string_view str, enum_c_strs_t<enum_count> enum_strings) -> enum_t
{
    auto str_lower = to_lower(str);
    auto result = static_cast<enum_t>(1);
    auto count = enum_count;
    for (int i = result + 1; i < count; ++i)
    {
        auto enum_str = enum_strings[i];
        auto enum_str_lower = to_lower(enum_str);
        if (str_lower == enum_str_lower)
        {
            result = static_cast<enum_t>(i);
            break;
        }
    };

    return result;
}

// Robustly extracts a boolean value from the provided JSON.
auto to_bool(const nlohmann::json& j) -> bool
{
    if (j.is_boolean()) return j.get<bool>();

    if (j.is_string())
    {
        auto str = to_lower(j.get<std::string>());
        if (str == "true") return true;
    }

    return false;
}

// Read applicaton configuration.
auto read_config(app_context_t& app) -> void
{
    try
    {
        std::ifstream i(config_file_name);
        nlohmann::json j;
        i >> j;

        app.user_id = j["user_id"];
        app.password = j["password"];
        app.server = j["server"];
        app.dx_api_path = j["dx_api_path"];
        app.token_endpoint = j["token_endpoint"];
        app.client_id = j["client_id"];
        app.client_secret = j["client_secret"];
        app.font_index = j["font_index"];
    }
    catch (const std::exception& e)
    {
        SDL_Log("Error reading settings file: %s", config_file_name);
        SDL_Log("Reason: %s", e.what());
    }
}

// Write application configuration.
auto write_config(app_context_t& app) -> void
{
    std::ofstream o(config_file_name);
    nlohmann::json j;
    j["user_id"] = app.user_id;
    j["password"] = app.password;
    j["server"] = app.server;
    j["dx_api_path"] = app.dx_api_path;
    j["token_endpoint"] = app.token_endpoint;
    j["client_id"] = app.client_id;
    j["client_secret"] = app.client_secret;
    j["font_index"] = app.font_index;
    o << std::setw(json_indent) << j << std::endl;
}
}

#endif // DX_API_HELPER_PROCS_H
