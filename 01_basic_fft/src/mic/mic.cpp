#include "mic.hpp"
#include "../../../libs/fft/fft.hpp"
#include "../../../libs/util/stopwatch.hpp"

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


    using FFT = fft::FFT<10>;    


    class StateData
    {
    public:
        SDL_AudioSpec spec;
        SDL_AudioDeviceID device;

        FFT fft;

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


    static void chunk_info(MicDevice& state, int len_8, Stopwatch& sw)
    {
        auto len = len_8 / sizeof(f32);

        state.chunk_samples = (u32)len;

        state.chunk_ms = sw.get_time_milli();
        sw.start();
    }


    static void buffer_info(MicDevice& state, Uint8* stream, int len_8, Stopwatch& sw)
    {                
        auto& data = get_data(state);

        static u32 b = 0;
        auto const push_sample = [&](f32 sample)
        {
            state.sample = sample;
            data.fft.buffer[b++] = (f64)sample;

            if (b >= data.fft.size)
            {
                b = 0;
                state.fill_buffer_ms = sw.get_time_milli();
                sw.start();
            }
        };

        f32* audio_data = (f32*)stream;
        auto len = len_8 / sizeof(f32);

        for (u32 i = 0; i < len; i++)
        {
            push_sample(audio_data[i]);
        }
    }


    static void fft_info(MicDevice& state, Uint8* stream, int len_8)
    {
        static Stopwatch sw;

        auto& data = get_data(state);

        static u32 b = 0;
        auto const push_sample = [&](f32 sample)
        {
            state.sample = sample;
            data.fft.buffer[b++] = (f64)sample;

            if (b >= data.fft.size)
            {
                sw.start();
                b = 0;
                data.fft.forward(data.fft.bins);
                state.fft_ms = sw.get_time_milli();                
            }
        };

        f32* audio_data = (f32*)stream;
        auto len = len_8 / sizeof(f32);

        for (u32 i = 0; i < len; i++)
        {
            push_sample(audio_data[i]);
        }
    }


    static void process_audio_fft(MicDevice& state, Uint8* stream, int len_8)
    {
        auto& data = get_data(state);

        static u32 b = 0;
        auto const push_sample = [&](f32 sample)
        {
            state.sample = sample;
            data.fft.buffer[b++] = (f64)sample;

            if (b >= data.fft.size)
            {
                b = 0;
                data.fft.forward(data.fft.bins);
            }
        };

        f32* audio_data = (f32*)stream;
        auto len = len_8 / sizeof(f32);

        for (u32 i = 0; i < len; i++)
        {
            push_sample(audio_data[i]);
        }
    }


    static void mic_audio_cb(void* userdata, Uint8* stream, int len_8)
    {     
        static Stopwatch sw;
        static Stopwatch cb_sw;

        static bool chunk = true;
        static bool buffer = true;
        static bool fft = true;

        using AP = AudioProc;

        cb_sw.start();

        auto& state = *(MicDevice*)userdata;

        switch (state.audio_proc)
        {
        case AP::FFT:
            chunk = buffer = fft = true;
            process_audio_fft(state, stream, len_8);
            break;

        case AP::InfoChunk:
            buffer = fft = true;
            if (chunk)
            {
                chunk = false;
                sw.start();
            }
            chunk_info(state, len_8, sw);
            break;

        case AP::InfoBuffer:
            chunk = fft = true;
            if (buffer)
            {
                buffer = false;
                sw.start();
            }
            buffer_info(state, stream, len_8, sw);
            break;

        case AP::InfoFFT:
            chunk = buffer = true;
            if (fft)
            {
                fft = false;
            }
            fft_info(state, stream, len_8);
            break;

        default: break;
        }

        state.cb_ms = cb_sw.get_time_milli();
    }
}


namespace mic
{
    bool init(MicDevice& state)
    {
        state.status = MicStatus::Closed;
        state.audio_proc = AudioProc::FFT;

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

        data.fft.init();

        state.fft_bins.data = data.fft.bins;
        state.fft_bins.length = data.fft.n_bins;

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