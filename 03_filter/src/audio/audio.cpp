#include "audio.hpp"
#include "../../../libs/util/stopwatch.hpp"
#include "../../../libs/span/span.hpp"

#include <SDL2/SDL.h>
#include <thread>
#include <cstdlib>
#include <cassert>


namespace audio
{
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
    
    template <typename T, u32 SZ>
    class RWBuffer
    {
    private:
        u8 r = 0;
        u8 w = 1;

        u8 rc = 0;

    public:
        static constexpr u32 size = SZ;

        T data[2][size];

        SpanView<T> wait_get_read() { while (r == rc) { pause(10); } rc = r; return { data[r], size }; }

        SpanView<T> get_write() { return { data[w], size }; }

        void write_complete() { r = (r + 1) & 1; w = !r; }        
    };
}


namespace audio
{
    using AudioBuffer = RWBuffer<f32, SAMPLE_SIZE>;


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
        auto dst = buffer.get_write();

        assert(src.length == dst.length);
        span::copy(src, dst);

        buffer.write_complete();
    }


    static void playback_cb(void* userdata, Uint8* stream, int len_8)
    {
        auto dst = span::make_view((f32*)stream, len_8 / sizeof(f32));

        auto& buffer = *(AudioBuffer*)userdata;
        auto src = buffer.wait_get_read();

        assert(src.length == dst.length);
        span::copy(src, dst);
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
        state.status = AudioStatus::Closed;
        AudioData::destroy(state);
    }
}
