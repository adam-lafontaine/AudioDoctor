#pragma once

#include "../../../libs/util/types.hpp"


namespace wave
{
    enum class WaveStatus : int
    {
        Closed = 0,
        Open,
        Running
    };


    enum class WaveForm : int
    {
        Square = 0,
        Sine,
        None
    };


    class Span
    {
    public:
        f32* data = 0;
        u32 length = 0;

        void zero() { for (u32 i = 0; i < length; i++) { data[i] = 0; } }
    };


    class WaveContext
    {
    public:

        WaveStatus status;
        WaveForm wave;

        f32 freq_ratio;

        Span fft_bins;
        Span samples;
        Span fft_inverted;

        u64 handle;
    };


    bool init(WaveContext& ctx);

    void start(WaveContext& ctx);

    void pause(WaveContext& ctx);

    void close(WaveContext& ctx);
}