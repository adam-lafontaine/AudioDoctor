#include "fft.hpp"

namespace fft
{
namespace internal
{
    // fftsg.c
    // TODO: f32 fft
    void rdft(int n, int isgn, double *a, int *ip, double *w);


    void init_ip_w(u32 n, f64* buffer, i32* ip, f64* w)
    {
        ip[0] = 0;
        rdft((int)n, 1, buffer, ip, w);
    }


    void forward(u32 n, f64* buffer, i32* ip, f64* w, f32* bins)
    {
        rdft((int)n, 1, buffer, ip, w);

        u32 b = 0;
        for (u32 i = 2; i < n; i += 2)
        {
            bins[b++] = num::hypot((f32)buffer[i], (f32)buffer[i + 1]);
        }
    }

    // TODO: f32 fft

    #include "fftsg_f32.cpp"
}
}