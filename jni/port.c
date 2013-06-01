#include "port.h"

#ifdef _WIN32
void
timeval_get(timeval_t *t)
{
    QueryPerformanceCounter(t);
}

double
timeval_diff_usec(timeval_t *t0, timeval_t *t1)
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    return ((t1->QuadPart - t0->QuadPart)*1000000.0) / (double)(freq.QuadPart);
}

double
timeval_diff_msec(timeval_t *t0, timeval_t *t1)
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    return ((t1->QuadPart - t0->QuadPart)*1000.0) / (double)(freq.QuadPart);
}

void *
aligned_malloc(size_t size, size_t align)
{
    return _aligned_malloc(size, align);
}

void
aligned_free(void *p)
{
    _aligned_free(p);
}

#else
#include <unistd.h>


void
timeval_get(timeval_t *t)
{
    gettimeofday(t, NULL);
}

double
timeval_diff_usec(timeval_t *t0, timeval_t *t1)
{
    double t0d = t0->tv_sec*1000000.0+t0->tv_usec;
    double t1d = t1->tv_sec*1000000.0+t1->tv_usec;

    return t1d-t0d;
}

double
timeval_diff_msec(timeval_t *t0, timeval_t *t1)
{
    double t0d = t0->tv_sec*1000.0+t0->tv_usec/1000.0;
    double t1d = t1->tv_sec*1000.0+t1->tv_usec/1000.0;

    return t1d-t0d;
}

void *
aligned_malloc(size_t size, size_t align)
{
    return memalign(align, size);
}

void
aligned_free(void *p)
{
    free(p);
}


#endif

