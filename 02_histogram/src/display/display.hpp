#pragma once

#include "../../../libs/imgui/imgui.h"
#include "../../../libs/stb_libs/qsprintf.hpp"
#include "../wave/wave.hpp"


namespace display
{
namespace internal
{
    static void select_wave(wave::WaveContext& ctx)
    {
        using WF = wave::WaveForm;

        constexpr auto square = (int)WF::Square;
        constexpr auto sine = (int)WF::Sine;
        constexpr auto none = (int)WF::None;

        static int option = none;

        ImGui::RadioButton("Square", &option, square);
        ImGui::SameLine();
        ImGui::RadioButton("Sine", &option, sine);
        ImGui::SameLine();
        ImGui::RadioButton("None", &option, none);

        int wave = (int)ctx.wave;

        if (option == wave)
        {
            return;
        }

        ctx.wave = (WF)option;
    }


    static void plot_samples(wave::WaveContext& ctx)
    {
        auto plot_data = ctx.samples.data;
        int data_count = ctx.samples.length;
        int data_offset = 0;

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
    class DisplayState
    {
    public:
        wave::WaveContext wave;
    };


    bool open(DisplayState& state)
    {
        if (!wave::init(state.wave))
        {
            return false;
        }

        return true;
    }


    void close(DisplayState& state)
    {
        wave::close(state.wave);
    }


    void show_window(DisplayState& state)
    {
        using WS = wave::WaveStatus;

        auto start_disabled = state.wave.status != WS::Open;
        auto pause_disabled = state.wave.status != WS::Running;

        ImGui::Begin("Wave");

        if (start_disabled) { ImGui::BeginDisabled(); }

        if (ImGui::Button("Start"))
        {
            wave::start(state.wave);
        }

        if (start_disabled) { ImGui::EndDisabled(); }

        ImGui::SameLine();

        if (pause_disabled) { ImGui::BeginDisabled(); }

        if (ImGui::Button("Pause"))
        {
            wave::pause(state.wave);
        }

        if (pause_disabled) { ImGui::EndDisabled(); }

        internal::select_wave(state.wave);
        internal::plot_samples(state.wave);

        ImGui::End();
    }
}