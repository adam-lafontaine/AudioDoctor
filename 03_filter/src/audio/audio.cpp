#include "audio.hpp"
#include "../../../libs/util/stopwatch.hpp"
#include "../../../libs/util/numeric.hpp"
#include "../../../libs/fft/fft.hpp"

// sudo apt-get install ffmpeg libavformat-dev libavcodec-dev libavutil-dev libswscale-dev
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <SDL2/SDL.h>
#include <thread>
#include <cstdlib>
#include <cassert>

#include <cstdio>


/* constants */

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

    using FFT = fft::FFT_Light<SAMPLE_BASE2_EXP>;

    static constexpr u32 SAMPLE_SIZE = FFT::size;

    constexpr auto AUDIO_FILE_PATH = "/home/adam/Repos/GameEngine/game_asteroids/assets/music/intro_00.ogg";


}


/* sdl */

namespace audio
{
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


    static inline void pause(i64 ns)
    {
        std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
    }


    static void cap_thread_ns(Stopwatch& sw, f64 target_ns)
    {
        constexpr f64 fudge = 0.9;

        auto sleep_ns = target_ns - sw.get_time_nano();
        if (sleep_ns > 0)
        {
            pause((i64)(sleep_ns * fudge));
        }

        sw.start();
    }
}


/* audio file */

namespace audio
{
    class AudioFileContext
    {
    public:
        AVFormatContext* fmt_ctx = 0;
        AVCodecParameters* codecpar = 0;
        AVCodec* codec = 0;
        AVCodecContext* codec_ctx = 0;
        int audio_stream_idx = -1;

        int n_channels = 0;
    };


    void destroy(AudioFileContext& ctx)
    {
        if (ctx.codec_ctx)
        {
            avcodec_free_context(&ctx.codec_ctx);
        }

        if (ctx.fmt_ctx)
        {
            avformat_close_input(&ctx.fmt_ctx);
        }

        ctx.fmt_ctx = 0;
        ctx.codecpar = 0;
        ctx.codec = 0;
        ctx.codec_ctx = 0;
        ctx.audio_stream_idx = -1;
        ctx.n_channels = 0;
    }


    bool load_audio_file(AudioFileContext& ctx, cstr file_path)
    {        
        //avformat_network_init();

        // Open OGG file
        if (avformat_open_input(&ctx.fmt_ctx, file_path, NULL, NULL) < 0) 
        {
            //fprintf(stderr, "Could not open input file\n");
            return false;
        }

        // Find stream info
        if (avformat_find_stream_info(ctx.fmt_ctx, NULL) < 0) 
        {
            //fprintf(stderr, "Could not find stream info\n");
            avformat_close_input(&ctx.fmt_ctx);
            return false;
        }

        // Find audio stream        
        for (unsigned int i = 0; i < ctx.fmt_ctx->nb_streams; i++) 
        {
            if (ctx.fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) 
            {
                ctx.audio_stream_idx = i;
                break;
            }
        }

        if (ctx.audio_stream_idx == -1) 
        {
            //fprintf(stderr, "No audio stream found\n");
            avformat_close_input(&ctx.fmt_ctx);
            return false;
        }

        // Get codec parameters
        ctx.codecpar = ctx.fmt_ctx->streams[ctx.audio_stream_idx]->codecpar;
        ctx.codec = avcodec_find_decoder(ctx.codecpar->codec_id);
        if (!ctx.codec) 
        {
            //fprintf(stderr, "Codec not found\n");
            avformat_close_input(&ctx.fmt_ctx);
            return false;
        }

        // Initialize codec context
        ctx.codec_ctx = avcodec_alloc_context3(ctx.codec);
        if (avcodec_parameters_to_context(ctx.codec_ctx, ctx.codecpar) < 0) 
        {
            //fprintf(stderr, "Failed to copy codec parameters\n");
            avcodec_free_context(&ctx.codec_ctx);
            avformat_close_input(&ctx.fmt_ctx);
            return false;
        }

        if (avcodec_open2(ctx.codec_ctx, ctx.codec, NULL) < 0) 
        {
            //fprintf(stderr, "Could not open codec\n");
            avcodec_free_context(&ctx.codec_ctx);
            avformat_close_input(&ctx.fmt_ctx);
            return false;
        }
        
        ctx.n_channels = ctx.codec_ctx->channels;

        return true;
    }    


    class AudioSamples
    {
        u32 rc = 0;
    public:
        f32* data = 0;
        u32 count = 0;

        b8 ok = 0;


        void read(SpanView<f32> const& dst)
        {
            auto len = num::min(dst.length, (count - rc));
            auto src = span::make_view(data + rc, len);

            span::copy(src, dst);
            
            rc += len;
            if (rc >= count)
            {
                rc = 0;
                auto dst_view = span::make_view(dst.data + len, dst.length - len);
                span::fill(dst_view, 0.0f);
            }
        }
    };


    void destroy(AudioSamples& s)
    {
        if (s.data)
        {
            std::free(s.data);
        }
    }


    AudioSamples convert_file(AudioFileContext& ctx)
    {
        auto bq = (AVRational){1, AV_TIME_BASE};
        auto cq = (AVRational){1, ctx.codec_ctx->sample_rate};

        int64_t duration_samples = av_rescale_q(ctx.fmt_ctx->duration, bq, cq);
        //u32 total_samples = duration_samples * ctx.n_channels;

        AudioSamples res;
        res.ok = 0;

        auto data = (f32*)std::malloc(duration_samples * sizeof(f32));
        if (!data)
        {
            return res;
        }

        // Packet and frame for decoding
        AVPacket packet;
        AVFrame *frame = av_frame_alloc();
        av_init_packet(&packet);
        packet.data = NULL;
        packet.size = 0;
        
        u32 buffer_idx = 0;
        f32 norm_f = ctx.n_channels * 32768.0f;

        // Decode and process
        while (av_read_frame(ctx.fmt_ctx, &packet) >= 0) 
        {
            if (packet.stream_index == ctx.audio_stream_idx) 
            {
                if (avcodec_send_packet(ctx.codec_ctx, &packet) < 0)
                {
                    break;
                }

                while (avcodec_receive_frame(ctx.codec_ctx, frame) >= 0) 
                {
                    i16* samples = (i16*)frame->data[0];
                    int nb_samples = frame->nb_samples;
                    int n_channels = ctx.n_channels;

                    u32 i = 0;
                    for (int s = 0; s < nb_samples; s++)
                    {
                        if (buffer_idx >= duration_samples)
                        {
                            break;
                        }

                        f32 out_sample = 0.0f;
                        for (int c = 0; c < n_channels; c++)
                        {
                            out_sample += (f32)samples[i++];
                        }
                        data[buffer_idx++] = out_sample / norm_f;
                    }
                }
            }

            av_packet_unref(&packet);
        }

        assert(buffer_idx == duration_samples);

        res.data = data;
        res.count = duration_samples;// total_samples;
        res.ok = 1;

        return res;
    }
}


/* sample queue */

namespace audio
{
    enum class AudioFilter : int
    {
        None = 0,
        FFT
    };


    class SampleQueue
    {
    private:
        u8 rc;
        u8 wc;
        b8 stop;

        FFT fft;

        static constexpr u32 N = 4;
        static constexpr u32 SZ = SAMPLE_SIZE;

        union
        {
            f32 memory[N * SZ];

            f32 slots[N][SZ];
        } data;

        AudioFilter data_filters[N];


    public:

        void reset() { wc = 1; rc = stop = 0; SDL_zero(data); }

        void init() { reset(); fft.init(); static_assert(num::is_power_of_2(N)); }

        void disable() { stop = 1; }


        void push(SpanView<f32> const& src, AudioFilter filter = AudioFilter::None)
        {
            if (stop || src.length != SZ)
            {
                return;
            }

            while ((u8)(rc - wc) == 1)
            {
                pause(10);
                if (stop)
                {
                    return;
                }
            }

            ++wc;
            auto w = wc & (N - 1u);
            
            auto dst = span::make_view(data.slots[w], SZ);
            data_filters[w] = filter;

            span::copy(src, dst);

            switch(filter)
            {
            case AudioFilter::None:
                break;
            
            case AudioFilter::FFT:
                fft.forward(dst.data);
                break;

            default:
                break;
            }
        }


        void pop(SpanView<f32> const& dst)
        {     
            assert(dst.length >= SZ);

            auto r = rc & (N - 1u);
            
            auto src = span::make_view(data.slots[r], SZ);
            auto filter = data_filters[r];

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

            switch(filter)
            {
            case AudioFilter::None:
                break;
            
            case AudioFilter::FFT:
                fft.inverse(src.data);
                break;

            default:
                break;
            }
            
            ++rc;
        }
        
    };
}


namespace audio
{
    class AudioData
    {
    public:
        AudioSDL capture;
        AudioSamples file_samples;

        AudioSDL playback;
        
        SampleQueue queue;

        SpanView<f32> in_samples;
        SpanView<f32> out_samples;

        f32 buffer32[SAMPLE_SIZE * 2];


        static AudioData* create() { return (AudioData*)std::malloc(sizeof(AudioData)); }

        static void destroy(AudioDevice& device) { std::free((AudioData*)(device.handle)); }

        static AudioData& get(AudioDevice const& device) { return *(AudioData*)(device.handle); }
    };


    static bool create_data(AudioDevice& state)
    {
        AudioFileContext ctx{};
        if (!load_audio_file(ctx, AUDIO_FILE_PATH))
        {
            return false;
        }

        auto file = convert_file(ctx);       

        destroy(ctx);

        if (!file.ok)
        {
            return false;
        }

        auto data = AudioData::create();
        if (!data)
        {
            destroy(file);
            return false;
        }

        SDL_zero(data->buffer32);

        data->file_samples = file;
        data->in_samples = span::make_view(data->buffer32, SAMPLE_SIZE);
        data->out_samples = span::make_view(data->buffer32 + SAMPLE_SIZE, SAMPLE_SIZE);

        state.handle = (u64)data;

        return true;
    }
}


namespace audio
{
    static void capture_cb(void* userdata, Uint8* stream, int len_8)
    {        
        auto& data = *(AudioData*)userdata;

        auto span = span::make_view((f32*)stream, len_8 / sizeof(f32));

        // temp
        //data.file_samples.read(span);


        span::copy(span, data.in_samples);
        //data.queue.push(span);
    }


    static void playback_cb(void* userdata, Uint8* stream, int len_8)
    {
        auto& data = *(AudioData*)userdata;
        auto span = span::make_view((f32*)stream, len_8 / sizeof(f32));
        
        data.queue.pop(span);
        span::copy(span, data.out_samples);
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

        data.queue.init();

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
        
        data.queue.reset();

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

        destroy(data.file_samples);
        
        state.status = AudioStatus::Closed;
        AudioData::destroy(state);
    }


    SpanView<f32> in_samples(AudioDevice const& device)
    {
        return AudioData::get(device).in_samples;
    }


    SpanView<f32> out_samples(AudioDevice const& device)
    {
        return AudioData::get(device).out_samples;
    }
}
