#ifndef BENCH_H
#define BENCH_H

#include <CL/cl.h>

#ifdef __cplusplus
extern "C" {
#endif

enum bench_result_code {
    BENCH_OK,
    BENCH_BUILD_ERROR,
    BENCH_ENQUEUE_KERNEL_ERROR
};

struct bench_result {
    enum bench_result_code code;

    double fval;
    int ival;
    char strval[4096];
    char score[4096];
    char error_message[4096];
};

struct clinst_bench_context {
    cl_context ctxt;
    cl_device_id dev;
    cl_command_queue queue;
};

int clinst_bench_init_context(struct clinst_bench_context *ctxt,
                              cl_device_id dev);
void clinst_bench_fini_context(struct clinst_bench_context *ctxt);

typedef void (*clinst_bench_run_t)(struct bench_result *r,
                                   struct clinst_bench_context *ctxt);

typedef int (*clinst_bench_is_valid_t)(cl_device_id dev, const char **reason);

enum clinst_bench_result_type {
    RESULT_TYPE_FLOAT,
    RESULT_TYPE_INT,
    RESULT_TYPE_STRING
};

struct clinst_bench {
    enum clinst_bench_result_type result_type;

    const char *name;
    const char *desc;
    const char *unit_str;
    const char *cl_code;

    clinst_bench_run_t run;
    clinst_bench_is_valid_t is_valid;
};

enum clinst_bench_code {
    BENCH_ENQUEUE_KERNEL_LATENCY,
    BENCH_ENQUEUE_MEMREAD_LATENCY,
    BENCH_ENQUEUE_MEMWRITE_LATENCY,
    BENCH_ENQUEUE_MEMREAD_BANDWIDTH,
    BENCH_ENQUEUE_MEMWRITE_BANDWIDTH,
    BENCH_FLOAT1_MEMREAD_BANDWIDTH,
    BENCH_FLOAT4_MEMREAD_BANDWIDTH,

    BENCH_FLOAT1_ADD_LATENCY,
    BENCH_FLOAT2_ADD_LATENCY,
    BENCH_FLOAT4_ADD_LATENCY,
    BENCH_INT1_ADD_LATENCY,
    BENCH_INT4_ADD_LATENCY,

    BENCH_GMEM_LOAD_LATENCY,
    BENCH_GMEM_LOAD_LATENCY_LARGE,
    BENCH_LMEM_LOAD_LATENCY,

    BENCH_FMA1_THROUGHPUT,
    BENCH_FMA1DEP_THROUGHPUT,
    BENCH_FMA4_THROUGHPUT,

//    BENCH_INVALID_TEST,

    BENCH_NUM
};

struct clinst_bench * clinst_bench_init(void);

#ifdef __cplusplus
}
#endif

#endif
