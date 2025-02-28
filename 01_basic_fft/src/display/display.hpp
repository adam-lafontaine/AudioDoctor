#pragma once

#include "../../../libs/imgui/imgui.h"
#include "../../../libs/stb_libs/qsprintf.hpp"
#include "../mic/mic.hpp"


namespace display
{
    class DisplayState
    {
    public:

        mic::MicDevice mic;
        
    };
}


/* internal */

namespace display
{
namespace internal
{
    class PlotProps
    {
    public:
        static constexpr int count = 256;
        f32 data[count];
        u8 index = 255;
    };


    static void plot_samples(PlotProps& props, mic::MicDevice const& mic)
    {
        ++props.index;
        props.data[props.index] = mic.sample;

        auto plot_data = props.data;
        int data_count = props.count;
        int data_offset = (int)props.index;

        constexpr auto plot_min = -1.0f;
        constexpr auto plot_max = 1.0f;
        constexpr auto plot_size = ImVec2(0, 80.0f);
        constexpr auto data_stride = sizeof(f32);

        char overlay[32] = { 0 };
        //stb::qsnprintf(overlay, 32, "%3.1f", mic.sample);

        ImGui::PlotLines("Samples", 
            plot_data, 
            data_count, 
            data_offset, 
            overlay,
            plot_min, plot_max, 
            plot_size, 
            data_stride);
    }
}
}

namespace display
{
    inline void mic_window(DisplayState& state)
    {
        using MS = mic::MicStatus;

        auto start_disabled = state.mic.status != MS::Open;
        auto pause_disabled = state.mic.status != MS::Running;

        ImGui::Begin("Mic");

        if (start_disabled) { ImGui::BeginDisabled(); }

        if (ImGui::Button("Start"))
        {
            mic::start(state.mic);
        }

        if (start_disabled) { ImGui::EndDisabled(); }

        ImGui::SameLine();

        if (pause_disabled) { ImGui::BeginDisabled(); }

        if (ImGui::Button("Pause"))
        {
            mic::pause(state.mic);
        }

        if (pause_disabled) { ImGui::EndDisabled(); }

        static internal::PlotProps sample_props{};

        internal::plot_samples(sample_props, state.mic);

        ImGui::End();
    }


    inline void open(DisplayState& state)
    {
        mic::init(state.mic);
    }


    inline void close(DisplayState& state)
    {
        mic::close(state.mic);
    }
}