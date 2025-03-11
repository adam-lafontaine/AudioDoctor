#pragma once

#include "../../../libs/util/types.hpp"


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


    bool init(AudioDevice& state);

    void start(AudioDevice& state);

    void pause(AudioDevice& state);

    void close(AudioDevice& state);
}