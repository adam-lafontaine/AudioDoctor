#include "mic.hpp"

#include <SDL2/SDL.h>
#include <cstdlib>





namespace mic
{
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr Uint8 CHANNELS = 1;    // Mono

    static constexpr int AUDIO_CAPTURE = 1;
    static constexpr int AUDIO_PLAYBACK = 0;

    static constexpr int DEVICE_RUN = 0;
    static constexpr int DEVICE_PAUSE = 1;

    static constexpr int FFT_SIZE = 1024; // Power of 2 for Ooura
    


    class StateData
    {
    public:
        SDL_AudioSpec spec;
        SDL_AudioDeviceID device;



        static StateData* create() { return (StateData*)std::malloc(sizeof(StateData)); }

        static void destroy(StateData* s) { std::free(s); }
    };


    static bool create_data(MicDevice& state)
    {
        auto data = StateData::create();
        if (!data)
        {
            return false;
        }

        state.handle = (u64)data;

        return true;
    }


    static StateData& get_data(MicDevice& state)
    {
        return *(StateData*)state.handle;
    }


    static void mic_audio_cb(void* userdata, Uint8* stream, int len)
    {
        auto& state = *(MicDevice*)userdata;
        f32* audio_data = (f32*)stream;
        len /= sizeof(f32);

        for (u32 i = 0; i < len; i++)
        {
            state.sample = audio_data[i];
        }
    }
}


namespace mic
{
    bool init(MicDevice& state)
    {
        state.status = MicStatus::Closed;

        if (SDL_Init(SDL_INIT_AUDIO) < 0)
        {
            return false;
        }

        if (!create_data(state))
        {
            return false;
        }

        auto& data = get_data(state);

        SDL_AudioSpec desired;

        SDL_zero(desired);
        desired.freq = SAMPLE_RATE;
        desired.format = AUDIO_F32SYS; // 32-bit float, system endianness
        desired.channels = 1;
        desired.samples = 256; // Buffer size per callback
        desired.callback = mic_audio_cb;
        desired.userdata = &state;

        cstr device_name = 0;

        auto device = SDL_OpenAudioDevice(device_name, AUDIO_CAPTURE, &desired, &data.spec, 0);
        if (!device)
        {
            return false;
        }

        data.device = device;
        state.status = MicStatus::Open;

        return true;
    }


    void start(MicDevice& state)
    {
        if (state.status != MicStatus::Open)
        {
            return;
        }

        auto& data = get_data(state);

        SDL_PauseAudioDevice(data.device, DEVICE_RUN);
        state.status = MicStatus::Running;
    }


    void pause(MicDevice& state)
    {
        if (state.status != MicStatus::Running)
        {
            return;
        }

        auto& data = get_data(state);

        SDL_PauseAudioDevice(data.device, DEVICE_PAUSE);
        state.status = MicStatus::Open;
    }


    void close(MicDevice& state)
    {
        pause(state);

        if (state.status != MicStatus::Open)
        {
            return;
        }

        auto& data = get_data(state);

        SDL_CloseAudioDevice(data.device);
        StateData::destroy(&data);
        state.status = MicStatus::Closed;
    }
}