#ifndef PORT_H
#define PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <windows.h>
typedef LARGE_INTEGER timeval_t;
#else
#include <unistd.h>
typedef struct timeval timeval_t;
#endif

void timeval_get(timeval_t *t);
double timeval_diff_usec(timeval_t *t0, timeval_t *t1);
double timeval_diff_msec(timeval_t *t0, timeval_t *t1);

void *aligned_malloc(size_t size, size_t align);
void aligned_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
