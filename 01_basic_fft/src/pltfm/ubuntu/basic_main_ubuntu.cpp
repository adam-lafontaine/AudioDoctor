#include "../imgui_sdl2_ogl3/imgui_include.hpp"
#include "../../display/display.hpp"

namespace dsp = display;


enum class RunState : int
{
    Begin,
    Run,
    End
};


namespace
{
    ui::UIState ui_state{};
    RunState run_state = RunState::Begin;
    dsp::DisplayState dsp_state{};
}


static void end_program()
{
    run_state = RunState::End;
}


static bool is_running()
{
    return run_state != RunState::End;
}


static void handle_window_event(SDL_Event const& event, SDL_Window* window)
{
    switch (event.type)
    {
    case SDL_QUIT:
        end_program();
        break;

    case SDL_WINDOWEVENT:
    {
        switch (event.window.event)
        {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        //case SDL_WINDOWEVENT_RESIZED:
            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            glViewport(0, 0, w, h);
            break;

        case SDL_WINDOWEVENT_CLOSE:
            end_program();
            break;
        
        default:
            break;
        }
    } break;

    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        auto key_code = event.key.keysym.sym;
        switch (key_code)
        {
            
    #ifndef NDEBUG
        case SDLK_ESCAPE:
            //sdl::print_message("ESC");
            end_program();
            break;
    #endif

        default:
            break;
        }

    } break;
    
    default:
        break;
    }
}


static void process_user_input()
{
    SDL_Event event;

    // Poll and handle events (inputs, window resize, etc.)
    while (SDL_PollEvent(&event))
    {
        handle_window_event(event, ui_state.window);
        ImGui_ImplSDL2_ProcessEvent(&event);
    }
}


static void render_imgui_frame()
{
    ui::new_frame();

#ifdef SHOW_IMGUI_DEMO
    ui::show_imgui_demo(ui_state);
#endif

    dsp::mic_window(dsp_state);

    ui::render(ui_state);
}


static bool main_init()
{
    ui_state.window_title = "FFT Basic";
    ui_state.window_width = 500;
    ui_state.window_height = 500;

    if (!ui::init(ui_state))
    {
        return false;
    }

    dsp::open(dsp_state);

    return true;
}


static void main_close()
{
    dsp::close(dsp_state);
    ui::close(ui_state);
}


static void main_loop()
{
    while(is_running())
    {
        process_user_input();

        //render_textures();

        render_imgui_frame();
    }
}


int main()
{
    if (!main_init())
    {
        return 1;
    }

    run_state = RunState::Run;

    main_loop();

    main_close();

    return 0;
}

#include "main_o.cpp"