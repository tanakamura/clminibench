#define CLLIB_EXTERN
#include "CLlib.h"
#include <dlfcn.h>
#include <android/log.h>


#define  LOG_TAG    "libclminibench"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static void *handle;

int
cllib_init(void)
{
    /* g620s */
    handle = dlopen("/system/vendor/lib/libOpenCL.so", RTLD_LAZY);
    if (!handle) {
        /* nexus 10 */
        handle = dlopen("/system/vendor/lib/egl/libGLES_mali.so", RTLD_LAZY);
    }

    if (!handle) {
        return -1;
    }

    p_clGetDeviceInfo = dlsym(handle, "clGetDeviceInfo");
    p_clGetPlatformIDs = dlsym(handle, "clGetPlatformIDs");

    LOGI("%p %p\n",
         p_clGetPlatformIDs, p_clGetPlatformIDs);

    return 0;
}