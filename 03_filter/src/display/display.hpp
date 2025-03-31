#pragma once

#include "../../../libs/imgui/imgui.h"
#include "../../../libs/stb_libs/qsprintf.hpp"
#include "../../../libs/util/numeric.hpp"
#include "../audio/audio.hpp"


namespace display
{
namespace internal
{
    static void plot_in_samples(audio::AudioDevice const& device)
    {
        auto samples = audio::in_samples(device);

        auto plot_data = samples.data;
        int data_count = samples.length;
        int data_offset = 0;

        constexpr auto plot_min = -1.0f;
        constexpr auto plot_max = 1.0f;
        constexpr auto plot_size = ImVec2(0, 80.0f);
        constexpr auto data_stride = sizeof(f32);

        char overlay[32] = { 0 };
        //stb::qsnprintf(overlay, 32, "%3.1f", mic.sample);

        ImGui::PlotLines("In Samples", 
            plot_data, 
            data_count, 
            data_offset, 
            overlay,
            plot_min, plot_max, 
            plot_size, 
            data_stride);
    }


    static void plot_out_samples(audio::AudioDevice const& device)
    {
        auto samples = audio::out_samples(device);

        auto plot_data = samples.data;
        int data_count = samples.length;
        int data_offset = 0;

        constexpr auto plot_min = -1.0f;
        constexpr auto plot_max = 1.0f;
        constexpr auto plot_size = ImVec2(0, 80.0f);
        constexpr auto data_stride = sizeof(f32);

        char overlay[32] = { 0 };
        //stb::qsnprintf(overlay, 32, "%3.1f", mic.sample);

        ImGui::PlotLines("Out Samples", 
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
        audio::AudioDevice device;
    };


    bool open(DisplayState& state)
    {
        if (!audio::init(state.device))
        {
            return false;
        }

        return true;
    }


    void close(DisplayState& state)
    {
        audio::close(state.device);
    }


    void show_window(DisplayState& state)
    {
        using AS = audio::AudioStatus;

        auto start_disabled = state.device.status != AS::Open;
        auto pause_disabled = state.device.status != AS::Running;

        ImGui::Begin("Audio Filter");

        if (start_disabled) { ImGui::BeginDisabled(); }

        if (ImGui::Button("Start"))
        {
            audio::start(state.device);
        }

        if (start_disabled) { ImGui::EndDisabled(); }

        ImGui::SameLine();

        if (pause_disabled) { ImGui::BeginDisabled(); }

        if (ImGui::Button("Pause"))
        {
            audio::pause(state.device);
        }

        if (pause_disabled) { ImGui::EndDisabled(); }

        internal::plot_in_samples(state.device);
        internal::plot_out_samples(state.device);

        ImGui::End();
    }
}
