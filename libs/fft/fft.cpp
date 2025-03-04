#include "fft.hpp"

namespace fft
{
namespace internal
{
    // fftsg.c
    // TODO: f32 fft
    void rdft(int n, int isgn, f32 *a, int *ip, f32 *w);


    void init_ip_w(u32 n, f32* buffer, i32* ip, f32* w)
    {
        ip[0] = 0;
        rdft((int)n, 1, buffer, ip, w);
    }


    void forward(u32 n, f32* buffer, i32* ip, f32* w, f32* bins)
    {
        rdft((int)n, 1, buffer, ip, w);

        u32 b = 0;
        for (u32 i = 2; i < n; i += 2)
        {
            bins[b++] = num::hypot(buffer[i], buffer[i + 1]);
        }
    }

    // TODO: f32 fft

    #include "fftsg_f32.cpp"
}
}