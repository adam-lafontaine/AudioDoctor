#include "wave.hpp"

#include "../../../libs/fft/fft.hpp"
#include "../../../libs/util/stopwatch.hpp"

#include <thread>
#include <cstdlib>


namespace wave
{    
    static constexpr u32 FFT_EXP = 8;


    using FFT = fft::FFT<FFT_EXP>;


    enum class CBStatus : int
    {
        Off = 0,
        On
    };


    class WaveData
    {
    public:
        
        FFT fft;

        f32 sample_data[FFT::size];
        f32 inverse_data[FFT::size];

        CBStatus cb_status;


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

        data->cb_status = CBStatus::Off;

        ctx.handle = (u64)data;

        return true;
    }


    static void destroy_data(WaveContext& ctx)
    {
        WaveData::destroy((WaveData*)ctx.handle);
    }


    static WaveData& get_data(WaveContext& ctx)
    {
        return *(WaveData*)ctx.handle;
    }
}


namespace wave
{
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


    static void generate_square_wave_fft(WaveContext& ctx, u32 wavelength)
    {  
        auto& fft = get_data(ctx).fft;
        
        for (u32 i = 0; i < fft.size; i++)
        {
            auto s = (i / wavelength) % 2 ? 1.0 : -1.0;

            ctx.samples.data[i] = s;
            fft.buffer[i] = s;
        }

        fft.forward(fft.bins);
    }


    static void generate_sine_wave_fft(WaveContext& ctx, u32 wavelength)
    {
        auto& fft = get_data(ctx).fft;

        f32 f = 2.0f * num::PI / wavelength;
        
        for (u32 i = 0; i < fft.size; i++)
        {
            auto s = num::sin(i * f);

            ctx.samples.data[i] = s;
            fft.buffer[i] = s;
        }

        fft.forward(fft.bins);
    }


    static void generate_zero_wave_fft(WaveContext& ctx)
    {
        auto& fft = get_data(ctx).fft;
        
        for (u32 i = 0; i < fft.size; i++)
        {
            auto s = 0.0f;

            ctx.samples.data[i] = s;
            fft.buffer[i] = s;
        }

        fft.forward(fft.bins);
    }


    static void inverse_fft(WaveContext& ctx)
    {
        auto& fft = get_data(ctx).fft;

        fft.inverse();

        for (u32 i = 0; i < fft.size; i++)
        {
            ctx.fft_inverted.data[i] = fft.buffer[i];
        }
    }


    static void wave_cb(WaveContext& ctx)
    {
        constexpr auto N = FFT::size;

        static Stopwatch sw;
        static auto w = WaveForm::None;

        constexpr u32 min = 2u;
        constexpr u32 max = N / 2;

        u32 wavelength = 0;
        f32 wl = 0.0f;

        auto& data = get_data(ctx);

        data.cb_status = CBStatus::On;

        auto const reset = [&]()
        {
            w = WaveForm::None;
            sw.start();
        };        

        while (ctx.status == WaveStatus::Running)
        {
            wl = 1.0f - ctx.freq_ratio;
            wavelength = num::round_to_unsigned<u32>(min + wl * (max - min));

            if (w != ctx.wave)
            {
                reset();
            }

            w = ctx.wave;

            switch (ctx.wave)
            {
            case WaveForm::Square:
                generate_square_wave_fft(ctx, wavelength);
                break;

            case WaveForm::Sine:
                generate_sine_wave_fft(ctx, wavelength);
                break;

            case WaveForm::None:
                generate_zero_wave_fft(ctx);
                break;

            default:
                break;
            }

            inverse_fft(ctx);

            cap_thread_ns(sw, 500.0);
        }

        data.cb_status = CBStatus::Off;
    }
}


namespace wave
{
    bool init(WaveContext& ctx)
    {
        ctx.status = WaveStatus::Closed;
        ctx.wave = WaveForm::None;

        if (!create_data(ctx))
        {
            return false;
        }

        auto& data = get_data(ctx);

        data.fft.init();

        ctx.fft_bins.data = data.fft.bins;
        ctx.fft_bins.length = data.fft.n_bins;

        ctx.samples.data = data.sample_data;
        ctx.samples.length = data.fft.size;
        ctx.samples.zero();

        ctx.fft_inverted.data = data.inverse_data;
        ctx.fft_inverted.length = data.fft.size;
        ctx.fft_inverted.zero();

        ctx.status = WaveStatus::Open;

        ctx.freq_ratio = 0.5f;

        return true;
    }


    void start(WaveContext& ctx)
    {
        if (ctx.status != WaveStatus::Open)
        {
            return;
        }
        
        auto const proc = [&]()
        {
            wave_cb(ctx);
        };

        ctx.status = WaveStatus::Running;

        std::thread th(proc);
        th.detach();
    }


    void pause(WaveContext& ctx)
    {
        if (ctx.status == WaveStatus::Closed)
        {
            return;
        }

        ctx.status = WaveStatus::Open;
    }


    void close(WaveContext& ctx)
    {
        pause(ctx);

        if (ctx.status != WaveStatus::Open)
        {
            return;
        }

        auto& data = get_data(ctx);

        Stopwatch sw;
        sw.start();

        while (data.cb_status == CBStatus::On)
        {
            cap_thread_ns(sw, 20);
        }

        destroy_data(ctx);
        ctx.status = WaveStatus::Closed;
    }
}