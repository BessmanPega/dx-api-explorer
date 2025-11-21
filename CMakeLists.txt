cmake_minimum_required(VERSION 3.15)

# Use GCC instead of Clang to avoid compiler crashes (must be set before project())
set(CMAKE_CXX_COMPILER "/opt/homebrew/bin/g++-15")
set(CMAKE_C_COMPILER "/opt/homebrew/bin/gcc-15")

project(dx_api_explorer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find required packages
find_package(SDL2 REQUIRED)
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/lib/cpp-httplib
    ${CMAKE_SOURCE_DIR}/lib/imgui
    ${CMAKE_SOURCE_DIR}/lib/imgui/backends
    ${CMAKE_SOURCE_DIR}/lib/imgui/misc/cpp
    ${CMAKE_SOURCE_DIR}/lib/json/single_include
    ${CMAKE_SOURCE_DIR}/lib
)

# Source files - Use unity build as designed, GCC handles it better than Clang
set(SOURCES
    src/dx_api_build.cpp
    lib/imgui/imgui.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/backends/imgui_impl_sdl2.cpp
    lib/imgui/backends/imgui_impl_sdlrenderer2.cpp
    lib/imgui/misc/cpp/imgui_stdlib.cpp
)

# Create executable
add_executable(dx_api_explorer ${SOURCES})

# Compiler definitions
target_compile_definitions(dx_api_explorer PRIVATE
    CPPHTTPLIB_OPENSSL_SUPPORT
)

# Remove WIN32_LEAN_AND_MEAN for non-Windows platforms
if(NOT WIN32)
    target_compile_definitions(dx_api_explorer PRIVATE
        # No Windows-specific definitions needed
    )
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dx_api_explorer PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        # Disable specific warnings that may not apply on macOS/Linux
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        # Add -O0 to prevent compiler optimization issues
        -O0
        # Increase compiler memory limit
        -ftemplate-depth=1024
    )
endif()

# Link libraries
target_link_libraries(dx_api_explorer
    ${SDL2_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Copy font file to output directory
add_custom_command(TARGET dx_api_explorer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/Cousine-Regular.ttf
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Cousine-Regular.ttf
)

# Installation rules
install(TARGETS dx_api_explorer RUNTIME DESTINATION bin)
install(FILES Cousine-Regular.ttf DESTINATION bin)
