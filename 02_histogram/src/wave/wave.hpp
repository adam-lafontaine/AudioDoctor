#pragma once

#include "../../../libs/util/types.hpp"


namespace wave
{
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
    };


    class WaveContext
    {
    public:

        WaveForm wave;

        f32 freq_ratio;

        Span fft_bins;
        Span samples;

        u64 handle;
    };


    bool init(WaveContext& ctx);

    void start(WaveContext& ctx);

    void pause(WaveContext& ctx);

    void close(WaveContext& ctx);
}