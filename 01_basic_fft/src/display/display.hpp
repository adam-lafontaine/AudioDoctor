#pragma once

#include "../../../libs/imgui/imgui.h"
#include "../../../libs/stb_libs/qsprintf.hpp"
#include "../../../libs/util/numeric.hpp"
#include "../mic/mic.hpp"


namespace num = numeric;


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


    static void select_process(mic::MicDevice& mic)
    {   
        using AP = mic::AudioProc;

        constexpr auto fft = (int)AP::FFT;
        constexpr auto ichunk = (int)AP::InfoChunk;
        constexpr auto ibuffer = (int)AP::InfoBuffer;
        constexpr auto ifft = (int)AP::InfoFFT;

        static int option = (int)AP::FFT;

        ImGui::RadioButton("FFT", &option, fft);
        ImGui::SameLine();
        ImGui::RadioButton("Chunk info", &option, ichunk);
        ImGui::SameLine();
        ImGui::RadioButton("Buffer info", &option, ibuffer);
        ImGui::SameLine();
        ImGui::RadioButton("FFT info", &option, ifft);

        int proc = (int)mic.audio_proc;

        if (option == proc)
        {
            return;
        }

        mic.audio_proc = (AP)option;
    }


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
        stb::qsnprintf(overlay, 32, "%3.1f", mic.sample);

        ImGui::PlotLines("Samples", 
            plot_data, 
            data_count, 
            data_offset, 
            overlay,
            plot_min, plot_max, 
            plot_size, 
            data_stride);
    }


    static void plot_chunk_samples(PlotProps& props, mic::MicDevice const& mic)
    {
        static u32 chunk_max = 0;

        ++props.index;
        props.data[props.index] = (f32)mic.chunk_samples;

        chunk_max = num::max(mic.chunk_samples, chunk_max);

        auto plot_data = props.data;
        int data_count = props.count;
        int data_offset = (int)props.index;

        constexpr auto plot_min = 0.0f;
        constexpr auto plot_size = ImVec2(0, 80.0f);
        constexpr auto data_stride = sizeof(f32);

        auto plot_max = (f32)chunk_max;

        char overlay[32] = { 0 };
        stb::qsnprintf(overlay, 32, "%u", mic.chunk_samples);

        ImGui::PlotLines("Chunk sizes", 
            plot_data, 
            data_count, 
            data_offset, 
            overlay,
            plot_min, plot_max, 
            plot_size, 
            data_stride);        
    }


    static void plot_chunk_times(PlotProps& props, mic::MicDevice const& mic)
    {
        static f32 ms_max = 0.0f;

        ++props.index;
        props.data[props.index] = (f32)mic.chunk_ms;

        ms_max = num::max((f32)mic.chunk_ms, ms_max);

        auto plot_data = props.data;
        int data_count = props.count;
        int data_offset = (int)props.index;

        constexpr auto plot_min = 0.0f;
        constexpr auto plot_size = ImVec2(0, 80.0f);
        constexpr auto data_stride = sizeof(f32);

        auto plot_max = ms_max;

        char overlay[32] = { 0 };
        stb::qsnprintf(overlay, 32, "%f", mic.chunk_ms);

        ImGui::PlotLines("Chunk times", 
            plot_data, 
            data_count, 
            data_offset, 
            overlay,
            plot_min, plot_max, 
            plot_size, 
            data_stride);
    }


    static void plot_buffer_times(PlotProps& props, mic::MicDevice const& mic)
    {
        static f32 ms_max = 0.0f;

        ++props.index;
        props.data[props.index] = (f32)mic.fill_buffer_ms;

        ms_max = num::max((f32)mic.fill_buffer_ms, ms_max);

        auto plot_data = props.data;
        int data_count = props.count;
        int data_offset = (int)props.index;

        constexpr auto plot_min = 0.0f;
        constexpr auto plot_size = ImVec2(0, 80.0f);
        constexpr auto data_stride = sizeof(f32);

        auto plot_max = ms_max;

        char overlay[32] = { 0 };
        stb::qsnprintf(overlay, 32, "%f", mic.fill_buffer_ms);

        ImGui::PlotLines("Buffer times", 
            plot_data, 
            data_count, 
            data_offset, 
            overlay,
            plot_min, plot_max, 
            plot_size, 
            data_stride);
    }


    static void plot_fft_times(PlotProps& props, mic::MicDevice const& mic)
    {
        static f32 ms_max = 0.0f;

        ++props.index;
        props.data[props.index] = (f32)mic.fft_ms;

        ms_max = num::max((f32)mic.fft_ms, ms_max);

        auto plot_data = props.data;
        int data_count = props.count;
        int data_offset = (int)props.index;

        constexpr auto plot_min = 0.0f;
        constexpr auto plot_size = ImVec2(0, 80.0f);
        constexpr auto data_stride = sizeof(f32);

        auto plot_max = ms_max;

        char overlay[32] = { 0 };
        stb::qsnprintf(overlay, 32, "%f", mic.fft_ms);

        ImGui::PlotLines("FFT times", 
            plot_data, 
            data_count, 
            data_offset, 
            overlay,
            plot_min, plot_max, 
            plot_size, 
            data_stride);
    }


    static void plot_fft_bin(PlotProps& props, mic::MicDevice const& mic, u32 bin)
    {
        auto value = mic.fft_bins.data[bin];

        static f32 value_max = 0.0f;

        value_max = num::max(value, value_max);

        ++props.index;
        props.data[props.index] = (f32)mic.fft_ms;

        auto plot_data = props.data;
        int data_count = props.count;
        int data_offset = (int)props.index;

        constexpr auto plot_min = 0.0f;
        constexpr auto plot_size = ImVec2(0, 20.0f);
        constexpr auto data_stride = sizeof(f32);

        auto plot_max = value_max;

        char title[32] = { 0 };
        stb::qsnprintf(title, 32, "Bin #%u", bin);

        char overlay[32] = { 0 };
        stb::qsnprintf(overlay, 32, "%f", value);

        ImGui::PlotLines(title, 
            plot_data, 
            data_count, 
            data_offset, 
            overlay,
            plot_min, plot_max, 
            plot_size, 
            data_stride);
    }


    static void show_mic_info(DisplayState& state)
    {
        if (!ImGui::CollapsingHeader("Info"))
        {
            return;
        }

        static PlotProps sample_props{};
        plot_samples(sample_props, state.mic);

        static PlotProps chunk_sample_props{};
        plot_chunk_samples(chunk_sample_props, state.mic);

        static PlotProps chunk_time_props{};
        plot_chunk_times(chunk_time_props, state.mic);

        static PlotProps buffer_time_props{};
        plot_buffer_times(buffer_time_props, state.mic);

        static PlotProps fft_time_props{};
        plot_fft_times(fft_time_props, state.mic);
    }


    static void show_fft_bins(DisplayState& state)
    {
        if (!ImGui::CollapsingHeader("Bins"))
        {
            return;
        }

        constexpr u32 N = 1024;

        auto& bins = state.mic.fft_bins;
        assert(bins.length <= N);

        static PlotProps props[N];

        for (u32 i = 0; i <bins.length; i++)
        {
            plot_fft_bin(props[i], state.mic, i);
        }
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

        internal::select_process(state.mic);

        internal::show_mic_info(state);
        internal::show_fft_bins(state);

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