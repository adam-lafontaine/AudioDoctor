#include "fft.hpp"

namespace fft
{
namespace internal
{
    // fftsg.c
    void rdft(int n, int isgn, double *a, int *ip, double *w);


    void init(u32 n, f64* buffer, i32* ip, f64* w)
    {
        ip[0] = 0;
        rdft((int)n, 1, buffer, ip, w);
    }



    #include "fftsg.c"
}
}