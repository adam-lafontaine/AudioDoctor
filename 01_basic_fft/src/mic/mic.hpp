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

    class MicDevice
    {
    public:
        f32 sample = 0.0f;
        MicStatus status = MicStatus::Closed;

        u64 handle = 0;
    };


    bool init(MicDevice& state);

    void start(MicDevice& state);

    void pause(MicDevice& state);

    void close(MicDevice& state);
}