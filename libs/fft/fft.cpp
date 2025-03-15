#include "fft.hpp"
#include "../span/span.hpp"

namespace fft
{
namespace internal
{
    // fftsg_f32.cpp
    //void rdft(int n, int isgn, f32* a, int* ip, f32* w);

    void rdft_ip_w(int n, int* ip, f32* w);

    void rdft_forward(int n, f32* a, int* ip, f32* w);

    void rdft_inverse(int n, f32 *a, int *ip, f32 *w);


    static inline void normalize_n(SpanView<f32> const& view)
    {        
        span::mul(view, 1.0f / view.length, view);
    }


    static inline void normalize_max(SpanView<f32> const& view)
    {
        f32 max = 0.0f;
        for (u32 i = 0; i < view.length; i++)
        {
            max = num::max(max, num::abs(view.data[i]));
        }

        span::mul(view, 1.0f / max, view);
    }


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


    void forward(u32 n, f32* buffer, i32* ip, f32* w)
    {
        rdft_forward((int)n, buffer, ip, w);
    }


    void inverse(u32 n, f32* buffer, i32* ip, f32* w)
    {
        rdft_inverse((int)n, buffer, ip, w);
        
        normalize_max(span::make_view(buffer, n));
    }

    #include "fftsg_f32.cpp"
}
}