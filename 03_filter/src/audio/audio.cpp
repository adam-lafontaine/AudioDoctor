#include "audio.hpp"
#include "../../../libs/util/stopwatch.hpp"
#include "../../../libs/util/numeric.hpp"
#include "../../../libs/span/span.hpp"

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

    static constexpr u32 SAMPLE_SIZE = 1024;


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
    class RingBuffer
    {
    private:
        static constexpr u16 SZ = 4096;
        u16 rc;
        u16 wc;

        f32 data[SZ];
    public:

        void init()
        {
            wc = 0;
            rc = SZ / 2;
            SDL_memset(data, 0, SZ);
        }


        void read(SpanView<f32> const& dst)
        {       
            static_assert(num::is_power_of_2(SZ));

            auto w = wc;

            u32 i = 0;
            auto const min_len = [&]()
            {
                auto a = (u32)(w - rc - 1);
                auto b = (u32)(SZ - wc);
                auto c = dst.length - i;
                return num::min(num::min(a, b), c);
            };

            auto len = min_len();

            int c = 0;

            while (i < dst.length)
            {
                auto s = (u8*)(data + rc);
                auto d = (u8*)(dst.data + i);
                auto bytes = len * sizeof(f32);

                span::copy_u8(s, d, bytes);

                i += len;
                rc = (rc + len) & (SZ - 1u);
                len = min_len();

                c++;
            }
        }


        void write(SpanView<f32> const& src)
        {
            static_assert(num::is_power_of_2(SZ));

            auto r = rc;

            u32 i = 0;
            auto const min_len = [&]()
            {
                auto a = (u32)(r - wc - 1);
                auto b = (u32)(SZ - wc);
                auto c = src.length - i;
                return num::min(num::min(a, b), c);
            };

            auto len = min_len();

            while (i < src.length)
            {
                auto s = (u8*)(src.data + i);
                auto d = (u8*)(data + wc);
                auto bytes = len * sizeof(f32);

                span::copy_u8(s, d, bytes);

                i += len;
                wc = (wc + len) & (SZ - 1u);
                len = min_len();
            }
        }
    };
}


namespace audio
{
    using AudioBuffer = RingBuffer;


    class AudioData
    {
    public:
        AudioSDL capture;
        AudioSDL playpack;

        AudioBuffer buffer;

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
        auto src = span::make_view((f32*)stream, len_8 / sizeof(f32));

        auto& buffer = *(AudioBuffer*)userdata;
        
        buffer.write(src);
    }


    static void playback_cb(void* userdata, Uint8* stream, int len_8)
    {
        auto dst = span::make_view((f32*)stream, len_8 / sizeof(f32));

        auto& buffer = *(AudioBuffer*)userdata;
        
        buffer.read(dst);

        /*static u32 phase = 0;

        auto len = len_8;
        auto freq = 440;
        auto amp = 0.8f;

        float* buffer = (float*)stream;
        int samples = len / sizeof(float); // Number of float samples
        
        for (int i = 0; i < samples; i++) {
            // Calculate if we're in the high or low part of the wave
            Uint32 samplesPerCycle = SAMPLE_RATE / freq;
            Uint32 halfCycle = samplesPerCycle / 2;
            float sampleValue = (phase < halfCycle) ? amp : -amp;
            
            // Write to buffer (mono)
            buffer[i] = sampleValue;
            
            // Increment phase and wrap around
            phase = (phase + 1) % samplesPerCycle;
        }*/
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

        if (!open_capture_device(data.capture, capture_cb, &data.buffer))
        {
            return false;
        }

        if (!open_playback_device(data.playpack, playback_cb, &data.buffer))
        {
            return false;
        }

        data.buffer.init();

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

        SDL_PauseAudioDevice(data.capture.device, DEVICE_RUN);
        SDL_PauseAudioDevice(data.playpack.device, DEVICE_RUN);

        state.status = AudioStatus::Running;
    }


    void pause(AudioDevice& state)
    {
        if (state.status != AudioStatus::Running)
        {
            return;
        }

        auto& data = AudioData::get(state);

        SDL_PauseAudioDevice(data.capture.device, DEVICE_PAUSE);
        SDL_PauseAudioDevice(data.playpack.device, DEVICE_PAUSE);

        state.status = AudioStatus::Open;
    }


    void close(AudioDevice& state)
    {
        auto& data = AudioData::get(state);

        SDL_PauseAudioDevice(data.capture.device, DEVICE_PAUSE);
        SDL_PauseAudioDevice(data.playpack.device, DEVICE_PAUSE);
        SDL_CloseAudioDevice(data.capture.device);
        SDL_CloseAudioDevice(data.playpack.device);
        state.status = AudioStatus::Closed;
        AudioData::destroy(state);
    }
}
