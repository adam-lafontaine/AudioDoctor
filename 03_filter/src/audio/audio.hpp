#pragma once

#include "../../../libs/span/span.hpp"


namespace audio
{
    enum class AudioStatus : int
    {
        Closed = 0,
        Open,
        Running
    };


    class AudioDevice
    {
    public:
        AudioStatus status = AudioStatus::Closed;

        u64 handle = 0;
    };


    bool init(AudioDevice& device);

    void start(AudioDevice& device);

    void pause(AudioDevice& device);

    void close(AudioDevice& device);


    SpanView<f32> in_samples(AudioDevice const& device);

    SpanView<f32> out_samples(AudioDevice const& device);
}