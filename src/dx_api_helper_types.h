#ifndef DX_API_HELPER_TYPES_H
#define DX_API_HELPER_TYPES_H

#include <array>
#include <memory>

namespace dx_api_explorer
{
// This function object lets us have default-constructible unique_ptr's with custom deleters.
template<typename data_t, void (*delete_data)(data_t*)>
class deleter_t
{
public:
    void operator()(data_t* data)
    {
        delete_data(data);
    }
};

// This alias is much nicer to use than the preceding.
template<typename data_t, void (*delete_data)(data_t*)>
using unique_ptr_t = std::unique_ptr<data_t, deleter_t<data_t, delete_data>>;

// A la: https://en.cppreference.com/w/cpp/experimental/scope_exit
template<typename exit_func>
class scope_exit
{
private:
    exit_func func;
public:
    scope_exit(exit_func _func) : func(_func) {}
    ~scope_exit() { func(); }
};

// For enum type specifier strings.
template<size_t enum_count>
using enum_c_strs_t = std::array<const char*, enum_count>;
}

#endif // DX_API_HELPER_TYPES_H