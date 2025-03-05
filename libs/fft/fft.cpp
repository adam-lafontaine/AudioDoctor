#include "fft.hpp"

namespace fft
{
namespace internal
{
    // fftsg_f32.cpp
    void rdft(int n, int isgn, f64 *a, int *ip, f64 *w);

    void rdft_ip_w(int n, int *ip, f64 *w);

    void rdft_forward(int n, f64 *a, int *ip, f64 *w);


    void init_ip_w(u32 n, f64 *a, i32* ip, f64* w)
    {
        //rdft_ip_w((int)n, ip, w);

        ip[0] = 0;
        rdft((int)n, 1, a, ip, w);
    }


    void forward(u32 n, f64* buffer, i32* ip, f64* w, f32* bins)
    {
        //rdft_forward((int)n, buffer, ip, w);

        rdft((int)n, 1, buffer, ip, w);

        u32 b = 0;
        for (u32 i = 2; i < n; i += 2)
        {
            bins[b++] = num::hypot(buffer[i], buffer[i + 1]);
        }
    }

    #include "fftsg_f32.cpp"
}
}