#pragma once

#include "../../../libs/util/types.hpp"


namespace mic
{
    enum class MicStatus : int
    {
        Closed = 0,
        Open,
        Running
    };


    enum class AudioProc : int
    {
        FFT = 0,
        InfoChunk,
        InfoBuffer,
        InfoFFT
    };


    class MicDevice
    {
    public:
        MicStatus status = MicStatus::Closed;
        AudioProc audio_proc = AudioProc::FFT;

        f32 sample = 0.0f;

        u32 chunk_samples = 0;
        f64 chunk_ms = 0.0;

        f64 fill_buffer_ms = 0;
        f64 fft_ms = 0.0;

        f64 cb_ms;

        u64 handle = 0;
    };


    bool init(MicDevice& state);

    void start(MicDevice& state);

    void pause(MicDevice& state);

    void close(MicDevice& state);
}