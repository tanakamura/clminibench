#ifndef CLLIB_H
#define CLLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <CL/cl.h>

#ifndef CLLIB_EXTERN
#define CLLIB_EXTERN extern
#endif

CLLIB_EXTERN CL_API_ENTRY cl_int CL_API_CALL
(*p_clGetDeviceInfo)(cl_device_id    /* device */,
                     cl_device_info  /* param_name */, 
                     size_t          /* param_value_size */, 
                     void *          /* param_value */,
                     size_t *        /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

//#define clGetDeviceInfo p_clGetDeviceInfo

CLLIB_EXTERN CL_API_ENTRY cl_int CL_API_CALL
(*p_clGetPlatformIDs)(cl_uint          /* num_entries */,
                      cl_platform_id * /* platforms */,
                      cl_uint *        /* num_platforms */) CL_API_SUFFIX__VERSION_1_0;

//#define clGetPlatformIDs p_clGetPlatformIDs

extern int cllib_init(void);



#ifdef __cplusplus
}
#endif


#endif