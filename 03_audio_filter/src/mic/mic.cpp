#include "mic.hpp"
#include "../../../libs/fft/fft.hpp"
#include "../../../libs/util/stopwatch.hpp"
#include "../../../libs/span/span.hpp"

#include <SDL2/SDL.h>
#include <cstdlib>
#include <cassert>


namespace mic
{
    


    static constexpr int AUDIO_SAMPLE_RATE = 44100;
    static constexpr Uint8 AUDIO_CHANNELS = 1;    // Mono
    static constexpr int AUDIO_SAMPLES = 256;

    static constexpr int CMD_AUDIO_CAPTURE = 1;
    static constexpr int CMD_AUDIO_PLAYBACK = 0;

    static constexpr int CMD_DEVICE_RUN = 0;
    static constexpr int CMD_DEVICE_PAUSE = 1;

    static constexpr u32 FFT_EXP = 10;

    using FFT = fft::FFT2<FFT_EXP>;

    static constexpr u32 FFT_SAMPLES = FFT::size;
    static constexpr u32 N_FFT_CHUNKS = FFT_SAMPLES / AUDIO_SAMPLES;

    
    class FFTBuffer
    {
    public:
        f32 buffer[2 * FFT_SAMPLES];
        
    };

    


    class MicData
    {
    public:
        SDL_AudioSpec spec;
        SDL_AudioDeviceID device;

        FFT fft;

        f32 fft_buffer[2 * FFT_SAMPLES];

        

        static MicData* create() { return (MicData*)std::malloc(sizeof(MicData)); }

        static void destroy(MicData* s) { std::free(s); }
    };


    static bool create_data(Device& state)
    {
        auto data = MicData::create();
        if (!data)
        {
            return false;
        }

        state.handle = (u64)data;

        return true;
    }


    static MicData& get_data(Device& state)
    {
        return *(MicData*)state.handle;
    }
}


namespace mic
{
    static void push_audio_chunk(f32* src, u32 len, MicData& data)
    {
        switch (len)
        {
        case 256:

            break;

        default:

            break;
        }
    }


    static void mic_audio_cb(void* userdata, Uint8* stream, int len_8)
    {
        auto& device = *(Device*)userdata;
        auto& data = get_data(device);

        auto src = (f32*)stream;
        auto len = (u32)(len_8 / sizeof(f32));

        push_audio_chunk(src, len, data);
    }
}


namespace mic
{
    bool init(Device& state)
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
        desired.freq = AUDIO_SAMPLE_RATE;
        desired.format = AUDIO_F32SYS; // 32-bit float, system endianness
        desired.channels = AUDIO_CHANNELS;
        desired.samples = AUDIO_SAMPLES; // Buffer size per callback
        desired.callback = mic_audio_cb;
        desired.userdata = &state;

        cstr device_name = 0;

        auto device = SDL_OpenAudioDevice(device_name, CMD_AUDIO_CAPTURE, &desired, &data.spec, 0);
        if (!device)
        {
            return false;
        }

        data.device = device;
        state.status = MicStatus::Open;

        data.fft.init();
    }


    void start(Device& state);

    void pause(Device& state);

    void close(Device& state);
}