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


    class Device
    {
    public:
        MicStatus status = MicStatus::Closed;


        u64 handle = 0;
    };


    bool init(Device& state);

    void start(Device& state);

    void pause(Device& state);

    void close(Device& state);
}