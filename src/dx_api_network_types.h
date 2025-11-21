#ifndef DX_API_NETWORK_TYPES_H
#define DX_API_NETWORK_TYPES_H

#include <queue>
#include <string>
#include "dx_api_helper_types.h"

namespace dx_api_explorer
{
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods
// Using a plain enum so it can index into an array without casting.
enum http_method_t
{
    http_method_unspecified,
    http_method_unknown,
    // ...always first.

    http_method_get,
    http_method_head,
    http_method_options,
    http_method_trace,
    http_method_put,
    http_method_delete,
    http_method_post,
    http_method_patch,
    http_method_connect,

    // Always last:
    http_method_count
};

// Strings that correspond to http method enums.
constexpr std::array<const char*, http_method_count> http_method_strings =
{
    "Unspecified",
    "Unknown",
    "Get",
    "Head",
    "Options",
    "Trace",
    "Put",
    "Delete",
    "Post",
    "Patch",
    "Connect"
};

static_assert(decltype(http_method_strings)().size() == http_method_count,
    "Must have the same number of http method type enums and strings.");

// https://docs.pega.com/bundle/dx-api/page/platform/dx-api/constellacion-dx-api-endponts.html
enum struct net_call_type_t
{
    none,
    login,
    refresh_case_types,
    create_case,
    open_assignment,
    open_assignment_action,
    submit_assignment_action
};

// Megastruct representation of a DX API call. Main thread provides call type and appropriate
// input variables. Network thread executes call and fills out output variables.
struct net_call_t
{
    // Input:
    net_call_type_t type = net_call_type_t::none;
    std::string client_id;
    std::string client_secret;
    std::string dx_api_path;
    std::string id1; // pzInsKey such as: "MYORG-MYCO-WORK-MYCASE C-123" or "ASSIGN-WORKLIST MYORG-MYCO-WORK-MYCASE C-123!MY_FLOW"
    std::string id2; // pyID such as: "MyFlowAction"
    std::string password;
    std::string server;
    std::string user_id;
    std::string work_type_id;

    // Input/Output:
    std::string access_token;
    std::string endpoint;

    // Output:
    bool        succeeded = false; // Important that this is always initialized to false!
    std::string method;
    std::string error_message;
    std::string etag;
    std::string request_headers;
    std::string request_body;
    std::string response_headers;
    std::string response_body;
};
using net_call_queue_t = std::queue<net_call_t>;
}

#endif // DX_API_NETWORK_TYPES_H