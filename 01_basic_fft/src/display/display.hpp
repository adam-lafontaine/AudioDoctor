#pragma once

#include "../../../libs/imgui/imgui.h"
#include "../mic/mic.hpp"


namespace display
{
    class DisplayState
    {
    public:
        
    };
}


/* internal */

namespace display
{
namespace internal
{



}
}

namespace display
{
    inline void mic_window(DisplayState& state)
    {
        ImGui::Begin("Mic");

        ImGui::Text("Microphon!");

        ImGui::End();
    }
}