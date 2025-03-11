#pragma once

#include "../../../libs/imgui/imgui.h"
#include "../../../libs/stb_libs/qsprintf.hpp"
#include "../../../libs/util/numeric.hpp"
#include "../audio/audio.hpp"


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

        ImGui::End();
    }
}
