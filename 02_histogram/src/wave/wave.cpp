#include "wave.hpp"

#include "../../../libs/fft/fft.hpp"
#include "../../../libs/util/stopwatch.hpp"

#include <thread>
#include <cstdlib>


namespace wave
{

    class WaveData
    {
    public:
        
        FFT fft;

        f32 sample_data[FFT::size];

        bool running;



        static WaveData* create() { return (WaveData*)std::malloc(sizeof(WaveData)); }

        static void destroy(WaveData* w) { std::free(w); }
    };


    static bool create_data(WaveContext& ctx)
    {
        auto data = WaveData::create();
        if (!data)
        {
            return false;
        }

        ctx.handle = (u64)data;

        return true;
    }


    static WaveData& get_data(WaveContext& ctx)
    {
        return *(WaveData*)ctx.handle;
    }
}


namespace wave
{
    static constexpr u32 FFT_EXP = 8;


    using FFT = fft::FFT<FFT_EXP>;


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


    static void generate_square_wave_fft(WaveContext& ctx, u32 freq)
    {  
        auto& fft = get_data(ctx).fft;
        
        for (u32 i = 0; i < fft.size; i++)
        {
            auto s = (i / freq) % 2 ? 1.0 : -1.0;

            ctx.samples.data[i] = s;
            fft.buffer[i] = s;
        }

        fft.forward(fft.bins);
    }


    static void wave_cb(WaveContext& ctx)
    {
        constexpr auto N = FFT::size;

        static Stopwatch sw;
        static auto w = WaveForm::None;

        auto& data = get_data(ctx);

        u32 min = 2u;
        u32 max = N / 2;

        auto f = ctx.freq_ratio;

        u32 freq = num::round_to_unsigned<u32>(min + f * (max - min));

        auto const reset = [&]()
        {
            w = WaveForm::None;
            sw.start();
        };

        if (w != ctx.wave)
        {
            reset();
        }

        while (data.running)
        {
            switch (w)
            {
            case WaveForm::Square:
                generate_square_wave_fft(ctx, freq);
                break;

            default:
                break;
            }

            cap_thread_ns(sw, 500.0);            
        }
    }
}


namespace wave
{
    bool init(WaveContext& ctx)
    {
        if (!create_data(ctx))
        {
            return false;
        }

        auto& data = get_data(ctx);

        data.fft.init();

        data.running = false;

        ctx.fft_bins.data = data.fft.bins;
        ctx.fft_bins.length = data.fft.n_bins;

        ctx.samples.data = data.sample_data;
        ctx.samples.length = data.fft.size;

        return true;
    }


    void start(WaveContext& ctx)
    {
        auto const proc = [&]()
        {
            wave_cb(ctx);
        };

        get_data(ctx).running = true;

        std::thread th(proc);
        th.detach();
    }


    void pause(WaveContext& ctx)
    {
        ctx.wave = WaveForm::None;
    }


    void close(WaveContext& ctx)
    {
        pause(ctx);

        auto& data = get_data(ctx);
        data.running = false;

        WaveData::destroy(&data);
        
    }
}