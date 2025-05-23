#pragma once

#include "../util/numeric.hpp"

namespace num = numeric;


namespace fft
{
namespace internal
{
    static constexpr u32 sqrt_approx(u32 val)
    {
        for (u32 i = 0; i < 50; i++)
        {
            if (i * i > val)
            {
                return i - 1;
            }
        }

        return 0;
    }


    static constexpr u32 fft_size(u32 base2_exp)
    {
        return (u32)num::cxpr::pow(2.0f, base2_exp); // Power of 2 for Ooura
    }


    static constexpr u32 fft_ip_size(u32 size)
    {
        return 2 + sqrt_approx(size / 2);
    }


    static constexpr u32 fft_w_size(u32 size)
    {
        return size / 2;
    }


    static constexpr u32 fft_bin_size(u32 size)
    {
        return (size - 2) / 2;
    }


    void init_ip_w(u32 n, i32* ip, f32* w);

    void forward(u32 n, f32* buffer, i32* ip, f32* w, f32* bins);

    void inverse(u32 n, f32* buffer, i32* ip, f32* w);

    void forward(u32 n, f32* buffer, i32* ip, f32* w);

}
}


namespace fft
{ 
    template <u32 B2EXP>
    class FFT
    {
    public:

        static constexpr u32 exp = B2EXP;
        static constexpr u32 size = internal::fft_size(exp);
        static constexpr u32 n_bins = internal::fft_bin_size(size);

        f32 buffer[size];

        i32 ip[internal::fft_ip_size(size)];
        f32 w[internal::fft_w_size(size)];

        f32 bins[n_bins];


        void init() 
        { 
            static_assert(num::is_power_of_2(size));
            static_assert(size >= 4);

            internal::init_ip_w(size, ip, w); 
            for (u32 i = 0; i < n_bins; i++) { bins[i] = 0.0f; } 
        }

        void forward(f32* bins) { internal::forward(size, buffer, ip, w, bins);  }

        void inverse() { internal::inverse(size, buffer, ip, w); }
    };
}