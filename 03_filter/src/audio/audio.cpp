#include "audio.hpp"
#include "../../../libs/util/stopwatch.hpp"
#include "../../../libs/util/numeric.hpp"
#include "../../../libs/span/span.hpp"
#include "../../../libs/fft/fft.hpp"

#include <SDL2/SDL.h>
#include <thread>
#include <cstdlib>
#include <cassert>

#include <cstdio>


namespace audio
{
    namespace num = numeric;


    static constexpr int SAMPLE_RATE = 44100;
    static constexpr Uint8 CHANNELS = 1;    // Mono

    static constexpr int AUDIO_CAPTURE = 1;
    static constexpr int AUDIO_PLAYBACK = 0;

    static constexpr int DEVICE_RUN = 0;
    static constexpr int DEVICE_PAUSE = 1;

    static constexpr u32 SAMPLE_BASE2_EXP = 11;

    static constexpr u32 SAMPLE_SIZE = num::cxpr::pow(2, SAMPLE_BASE2_EXP);


    class AudioSDL
    {
    public:
        SDL_AudioSpec spec;
        SDL_AudioDeviceID device;
    };


    static bool open_capture_device(AudioSDL& data, SDL_AudioCallback cb, void* cb_data)
    {
        SDL_AudioSpec desired;

        SDL_zero(desired);
        desired.freq = SAMPLE_RATE;
        desired.format = AUDIO_F32SYS; // 32-bit float, system endianness
        desired.channels = CHANNELS;
        desired.samples = (Uint16)SAMPLE_SIZE;
        desired.callback = cb;
        desired.userdata = cb_data;

        data.device = SDL_OpenAudioDevice(0, AUDIO_CAPTURE, &desired, &data.spec, 0);
        if (!data.device)
        {
            return false;
        }

        return true;
    }


    static bool open_playback_device(AudioSDL& data, SDL_AudioCallback cb, void* cb_data)
    {
        SDL_AudioSpec desired;

        SDL_zero(desired);
        desired.freq = SAMPLE_RATE;
        desired.format = AUDIO_F32SYS; // 32-bit float, system endianness
        desired.channels = CHANNELS;
        desired.samples = (Uint16)SAMPLE_SIZE;
        desired.callback = cb;
        desired.userdata = cb_data;

        data.device = SDL_OpenAudioDevice(0, AUDIO_PLAYBACK, &desired, &data.spec, 0);
        if (!data.device)
        {
            return false;
        }

        return true;
    }


    static void pause(i64 ns)
    {
        std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
    }


    static void cap_thread_ns(Stopwatch& sw, f64 target_ns)
    {
        constexpr f64 fudge = 0.9;

        auto sleep_ns = target_ns - sw.get_time_nano();
        if (sleep_ns > 0)
        {
            std::this_thread::sleep_for(std::chrono::nanoseconds((i64)(sleep_ns * fudge)));
        }

        sw.start();
    }
}


namespace audio
{
    class SampleQueue
    {
    private:
        u8 rc;
        u8 wc;
        b8 stop;

        static constexpr u32 N = 4;
        static constexpr u32 SZ = SAMPLE_SIZE;

        union
        {
            f32 memory[N * SZ];

            f32 slots[N][SZ];
        } data;


    public:
        void init() { rc = wc = stop = 0; SDL_memset(data.memory, 0, N * SZ * sizeof(f32)); }

        void disable() { stop = 1; }


        void push_write(SpanView<f32> const& src)
        {
            if (stop)
            {
                return;
            }

            static_assert(num::is_power_of_2(N));
            wc = (wc + 1) & (N - 1u);

            assert(src.length <= SAMPLE_SIZE);

            auto len = num::min(src.length, SAMPLE_SIZE);
            auto dst = span::make_view(data.slots[wc], len);

            span::copy(src, dst);
        }


        void pop_read(SpanView<f32> const& dst)
        {     
            assert(dst.length <= SAMPLE_SIZE);

            auto len = num::min(dst.length, SAMPLE_SIZE);
            auto src = span::make_view(data.slots[rc], len);

            if (stop)
            {
                return;
            }

            while (rc == wc)
            {
                pause(10);
                if (stop)
                {
                    return;
                }
            }

            span::copy(src, dst);

            static_assert(num::is_power_of_2(N));
            rc = (rc + 1) & (N - 1u);
        }
        
    };
}


namespace audio
{
    using FFT = fft::FFT<SAMPLE_BASE2_EXP>;


    class AudioData
    {
    public:
        AudioSDL capture;
        AudioSDL playback;
        
        SampleQueue queue;

        FFT fft;

        static AudioData* create() { return (AudioData*)std::malloc(sizeof(AudioData)); }

        static void destroy(AudioDevice& device) { std::free((AudioData*)(device.handle)); }

        static AudioData& get(AudioDevice& device) { return *(AudioData*)(device.handle); }
    };


    static bool create_data(AudioDevice& state)
    {
        auto data = AudioData::create();
        if (!data)
        {
            return false;
        }

        state.handle = (u64)data;

        return true;
    }
}


namespace audio
{
    static void capture_cb(void* userdata, Uint8* stream, int len_8)
    {        
        auto& data = *(AudioData*)userdata;
        
        data.queue.push_write(span::make_view((f32*)stream, len_8 / sizeof(f32)));
    }


    static void playback_cb(void* userdata, Uint8* stream, int len_8)
    {
        auto& data = *(AudioData*)userdata;
        
        data.queue.pop_read(span::make_view((f32*)stream, len_8 / sizeof(f32)));
    }
}


namespace audio
{
    bool init(AudioDevice& state)
    {
        state.status = AudioStatus::Closed;

        if (SDL_Init(SDL_INIT_AUDIO) < 0)
        {
            return false;
        }

        if (!create_data(state))
        {
            return false;
        }

        auto& data = AudioData::get(state);

        if (!open_capture_device(data.capture, capture_cb, &data))
        {
            return false;
        }

        if (!open_playback_device(data.playback, playback_cb, &data))
        {
            return false;
        }

        state.status = AudioStatus::Open;

        return true;
    }


    void start(AudioDevice& state)
    {
        if (state.status != AudioStatus::Open)
        {
            return;
        }

        auto& data = AudioData::get(state);
        
        data.queue.init();

        SDL_PauseAudioDevice(data.capture.device, DEVICE_RUN);
        SDL_PauseAudioDevice(data.playback.device, DEVICE_RUN);

        state.status = AudioStatus::Running;
    }


    void pause(AudioDevice& state)
    {
        if (state.status != AudioStatus::Running)
        {
            return;
        }

        auto& data = AudioData::get(state);
        data.queue.disable();

        SDL_PauseAudioDevice(data.capture.device, DEVICE_PAUSE);
        SDL_PauseAudioDevice(data.playback.device, DEVICE_PAUSE);

        state.status = AudioStatus::Open;
    }


    void close(AudioDevice& state)
    {
        auto& data = AudioData::get(state);
        data.queue.disable();
        
        SDL_PauseAudioDevice(data.capture.device, DEVICE_PAUSE);
        SDL_PauseAudioDevice(data.playback.device, DEVICE_PAUSE);

        pause(100);
        
        SDL_CloseAudioDevice(data.capture.device);
        SDL_CloseAudioDevice(data.playback.device);
        
        state.status = AudioStatus::Closed;
        AudioData::destroy(state);
    }
}
