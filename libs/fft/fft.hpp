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


    void init(u32 n, f64* buffer, i32* ip, f64* w);

}
}


namespace fft
{   
    template <u32 N>
    class FFT
    {
    public:

        static constexpr u32 FFT_2_EXP = 10;
        static constexpr u32 FFT_SIZE = (u32)num::cxpr::pow(2.0f, FFT_2_EXP); // Power of 2 for Ooura
        static constexpr u32 FFT_IP_SIZE = 2 + internal::sqrt_approx(FFT_SIZE / 2);
        static constexpr u32 FFT_W_SIZE = FFT_SIZE / 2;

        i32 ip[FFT_IP_SIZE];
        f64 w[FFT_W_SIZE];

        f64 buffer[FFT_SIZE];
    };


    template <u32 N>
    inline void init(FFT<N>& fft)
    {
        internal::init(fft.FFT_SIZE, fft.buffer, fft.ip, fft.w);
    }
}