#include "fft.hpp"

namespace fft
{
namespace internal
{
    // fftsg_f32.cpp
    //void rdft(int n, int isgn, f32* a, int* ip, f32* w);

    void rdft_ip_w(int n, int* ip, f32* w);

    void rdft_forward(int n, f32* a, int* ip, f32* w);

    void rdft_inverse(int n, f32 *a, int *ip, f32 *w);


    void init_ip_w(u32 n, i32* ip, f32* w)
    {
        rdft_ip_w((int)n, ip, w);
    }


    void forward(u32 n, f32* buffer, i32* ip, f32* w, f32* bins)
    {
        rdft_forward((int)n, buffer, ip, w);

        u32 b = 0;
        for (u32 i = 2; i < n; i += 2)
        {
            bins[b++] = num::hypot(buffer[i], buffer[i + 1]);
        }
    }


    void inverse(u32 n, f32* buffer, i32* ip, f32* w)
    {
        rdft_inverse((int)n, buffer, ip, w);
    }

    #include "fftsg_f32.cpp"
}
}