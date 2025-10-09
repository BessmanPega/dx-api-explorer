#include <cassert>
#include <functional>
#include <memory>
#include <thread>
#include "dx_api_app_types.h"
#include "dx_api_constants.h"
#include "dx_api_helper_procs.h"
#include "dx_api_helper_types.h"
#include "dx_api_thread_procs.h"
#include "imgui.h"
#include "imgui_impl_sdlrenderer2.h"
#include "imgui_impl_sdl2.h"
#include "SDL.h"

int main(int, char**)
{
    namespace dx = dx_api_explorer;
    auto app_context = std::make_unique<dx::app_context_t>();
    auto& app = *app_context;
    dx::read_config(app); // Todo: this seems to not require the dx:: prefix, why?

    int sdl_init_result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
    assert(sdl_init_result == 0);
    dx::scope_exit sdl_quit(&SDL_Quit);
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_MAXIMIZED);
    dx::unique_ptr_t<SDL_Window, &SDL_DestroyWindow> window_uptr
    (
        SDL_CreateWindow("DX API Explorer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags)
    );
    SDL_Window* window = window_uptr.get();
    assert(window);

    dx::unique_ptr_t<SDL_Renderer, &SDL_DestroyRenderer> renderer_uptr
    (
        SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED)
    );
    SDL_Renderer* renderer = renderer_uptr.get();
    assert(renderer);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    dx::scope_exit destroy_imgui_context([]() { ImGui::DestroyContext(); });
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigDebugIsDebuggerPresent = true;
    ImGui::StyleColorsLight();

    // If we didn't get a font size from our (potentially non-existant) config file, try
    // to select a reasonable default based on screen resolution as a proxy for DPI.
    // Todo: this mechanism recommends overly large fonts for big normal DPI monitors.
    if (app.font_index < 0)
    {
        int w, h;
        int err = SDL_GetRendererOutputSize(renderer, &w, &h);
        if (err != 0)
        {
            SDL_Log("Could not determine renderer output size: %s", SDL_GetError());
        }
        else
        {
            int calculated_index  = int(w * 1.0f / dx::hidpi_pixel_width_threshold);
            if (calculated_index < dx::font_sizes.size())
            {
                app.font_index = calculated_index;
            }

            SDL_Log("Renderer is %d pixels wide, using default font size of %f.", w, dx::font_sizes[app.font_index].first);
        }
        
    }

    for (const auto& font_size : dx::font_sizes)
    {
        io.Fonts->AddFontFromFileTTF(dx::font_file_name, font_size.first);
    }
    io.FontDefault = io.Fonts->Fonts[app.font_index];

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
    dx::scope_exit shutdown_imgui_renderer([]()
    {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
    });

    std::thread network_thread(dx::network_thread_main_loop, std::ref(app));
    dx::app_thread_main_loop(app, window, renderer);
    network_thread.join();

    dx::write_config(app);
    return 0;
}