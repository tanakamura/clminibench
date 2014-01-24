#include "bench.h"
#include "../port.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ITER4(a) a a a a
#define ITER16(a)                               \
    ITER4(a) ITER4(a) ITER4(a) ITER4(a) 

#define ITER32(a)                               \
    ITER4(a) ITER4(a) ITER4(a) ITER4(a)         \
    ITER4(a) ITER4(a) ITER4(a) ITER4(a) 

#define ITER64(a)                               \
    ITER32(a) ITER32(a)

#define HAS_LMEM (1<<0)
#define INIT_INDEX (1<<1)
#define THROUGHPUT (1<<2)
#define CONSTANT_MEM (1<<3)

#define  LOG_TAG    "libclminibench"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
static struct clinst_bench benches[BENCH_NUM];

static const char * const eval_message[] = {
    "�͂₢��",
    "�߂�����͂₢��",
    "���������͂₢��",
    "���Ȃ�͂₢��",
    "�߂��ۂ��͂₢��",
    "�ƂĂ��͂₢��",
    "������",
};


#undef __kernel

#define ASIZE(A) (sizeof(A) / sizeof(A[0]))

int
clinst_bench_init_context(struct clinst_bench_context *ctxt,
                          cl_device_id dev)
{
    cl_platform_id plat;
    size_t sz;
    cl_context_properties cps[3];

    clGetDeviceInfo(dev, CL_DEVICE_PLATFORM, sizeof(plat), &plat, &sz);

    cps[0] = CL_CONTEXT_PLATFORM;
    cps[1] = (cl_context_properties)plat;
    cps[2] = 0;

    ctxt->dev = dev;
    ctxt->ctxt = clCreateContext(cps, 1, &dev, NULL, NULL, NULL);
    ctxt->queue = clCreateCommandQueue(ctxt->ctxt, dev, 0, NULL);

    return 0;
}

void
clinst_bench_fini_context(struct clinst_bench_context *ctxt)
{
    clReleaseCommandQueue(ctxt->queue);
    clReleaseContext(ctxt->ctxt);
}

static void
set_pref_ndrange(cl_device_id dev,
                 cl_kernel ker,
                 size_t *gw,
                 size_t *lw)
{
    cl_int val;
    size_t sz;
    size_t pref_wgs;
    cl_device_type dev_type;

    // clGetKernelWorkGroupInfo(ker, dev, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
    //                          sizeof(pref_wgs), &pref_wgs, &sz);
    // lw[0] = 16;

    clGetDeviceInfo(dev, CL_DEVICE_TYPE, sizeof(dev_type), &dev_type, &sz);
    if (dev_type == CL_DEVICE_TYPE_CPU) {
        lw[0] = 1;
    } else {
        size_t wsz[3];
        size_t sz;
        cl_int r = clGetDeviceInfo(dev, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(wsz), &wsz, &sz);
        if (r == CL_SUCCESS) {
            lw[0] = wsz[0]/2;
        } else {
            lw[0] = 128;
        }
    }

    clGetDeviceInfo(dev, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(val), &val, &sz);

    val *= 32;

    gw[0] = lw[0] * val;
}

static void
get_score(char *buf)
{
    strcpy(buf, eval_message[rand()%ASIZE(eval_message)]);
}

static int
build_kernel(struct bench_result *r,
             struct clinst_bench_context *ctxt,
             cl_kernel *ker,
             cl_program *prog,
             const char *code)
{
    size_t sz[1];
    cl_int st;
    sz[0] = strlen(code);
    *prog = clCreateProgramWithSource(ctxt->ctxt, 1, &code, sz, NULL);
    if (*prog == NULL) {
        r->code = BENCH_BUILD_ERROR;
        strcpy(r->error_message, "clCreateProgramWithSource failed");
        return -1;
    }

    st = clBuildProgram(*prog, 1, &ctxt->dev, "-cl-fast-relaxed-math", NULL, NULL);
    if (st != CL_SUCCESS) {
        size_t s;
        r->code = BENCH_BUILD_ERROR;
        clGetProgramBuildInfo(*prog, ctxt->dev, CL_PROGRAM_BUILD_LOG, sizeof(r->error_message), r->error_message, &s);
        clReleaseProgram(*prog);
        return -1;
    }

    *ker = clCreateKernel(*prog, "f", &st);
    if (*ker == NULL) {
        r->code = BENCH_BUILD_ERROR;
        clReleaseProgram(*prog);
        strcpy(r->error_message, "clCreateKernel failed");
        return -1;
    }

    return 0;
}

#define DECL_PARAM()                            \
             cl_kernel ker;                     \
             cl_program prog;                   \
             size_t gw[3], lw[3];               \
             cl_uint dim;                       \
             int st;

#define KERNEL(code) if (build_kernel(r, ctxt, &ker, &prog, code) < 0) { return; }

#define KERNEL_ARG_GMEM(n, v) if (clSetKernelArg(ker, n, sizeof(cl_mem), &v) != CL_SUCCESS) { return; }
#define KERNEL_ARG_LMEM(n, sz) if ((st=clSetKernelArg(ker, n, sz, NULL)) != CL_SUCCESS) { printf("%d\n", st);return; }
#define KERNEL_ARG_VAL(n, v) if (clSetKernelArg(ker, n, sizeof(v), (void*)&v) != CL_SUCCESS) { return; }
#define KERNEL_ARG_INT(n, v) {cl_int v_=(v); if (clSetKernelArg(ker, n, sizeof(v_), (void*)&v_) != CL_SUCCESS) { return; }; }
#define KERNEL_ARG_FLOAT(n, v) {cl_float v_=(v); if (clSetKernelArg(ker, n, sizeof(v_), (void*)&v_) != CL_SUCCESS) { return; }; }

#define RELEASE_ALL() clReleaseKernel(ker); clReleaseProgram(prog); 

#define WORK_DIM1(g,l) dim=1; gw[0]=g; lw[0]=l;

#define WORK_DIM_PREF() dim=1; set_pref_ndrange(ctxt->dev, ker, gw, lw);

#define RUN() if (run(r, ctxt, ker, dim, gw, lw, 1) < 0) { return; }
#define RUN_N(N) if (run(r, ctxt, ker, dim, gw, lw, N) < 0) { return; }

static int
run(struct bench_result *r,
    struct clinst_bench_context *ctxt,
    cl_kernel ker,
    int dim,
    size_t *gw,
    size_t *lw,
    int num_enq)
{
    int qi;
    for (qi=0; qi<num_enq; qi++) {
        cl_int st = clEnqueueNDRangeKernel(ctxt->queue, ker, dim, NULL, gw, lw, 0, NULL, NULL);
        if (st != CL_SUCCESS) {
            r->code = BENCH_ENQUEUE_KERNEL_ERROR;
            sprintf(r->error_message, "ndrange error : %d\n", (int)st);
            puts(r->error_message);
            return -1;
        }
    }
    clFinish(ctxt->queue);

    return 0;
}

#define K_(k) #k
#define K(k) K_(k)

static const char kernel_latency_kernel[] = 
    K(void __kernel f(void) { });

static void
kernel_latency_run(struct bench_result *r,
                   struct clinst_bench_context *ctxt)
{
    int i;
    timeval_t t0, t1;
    double usec;
    int nloop = 100;

    DECL_PARAM();
    KERNEL(kernel_latency_kernel);

    WORK_DIM1(1,1);

    RUN();

    timeval_get(&t0);

    for (i=0; i<nloop; i++) {
        RUN();
    }

    timeval_get(&t1);

    RELEASE_ALL();

    usec = timeval_diff_usec(&t0, &t1);
    usec /= nloop;

    r->code = BENCH_OK;
    r->fval = usec;
    get_score(r->score);
}

static void
kernel_latency_run16(struct bench_result *r,
                     struct clinst_bench_context *ctxt)
{
    int i;
    timeval_t t0, t1;
    double usec;
    int nloop = 100;

    DECL_PARAM();
    KERNEL(kernel_latency_kernel);

    WORK_DIM1(1,1);

    RUN_N(16);

    timeval_get(&t0);

    for (i=0; i<nloop; i++) {
        RUN_N(16);
    }

    timeval_get(&t1);

    RELEASE_ALL();

    usec = timeval_diff_usec(&t0, &t1);
    usec /= (nloop*16);

    r->code = BENCH_OK;
    r->fval = usec;
    get_score(r->score);
}


#include "mem.h"
#include "inst.h"

#define INIT_BENCH(code, type_, name_, desc_, unit_, run_, valid_, cl_code_) \
    benches[code].result_type = type_;          \
    benches[code].name = name_;                 \
    benches[code].desc = desc_;                 \
    benches[code].unit_str = unit_;             \
    benches[code].run = run_;                   \
    benches[code].is_valid = valid_;            \
    benches[code].cl_code = cl_code_;

static int
valid(cl_device_id dev, const char **reason)
{
    return 1;
}

static int
invalid(cl_device_id dev, const char **reason)
{
    *reason = "always invalid";
    return 0;
}

struct clinst_bench *
clinst_bench_init(void)
{
    INIT_BENCH(BENCH_ENQUEUE_KERNEL_LATENCY,
               RESULT_TYPE_FLOAT,
               "enqueue kernel latency",
               "���[�N�A�C�e����N�����鎞�Ԃ͂���܂��B�݂������قǂ悢�ł�",
               "usec",
               kernel_latency_run,
               valid,
               kernel_latency_kernel);

    INIT_BENCH(BENCH_ENQUEUE16_KERNEL_LATENCY,
               RESULT_TYPE_FLOAT,
               "enqueue kernel latency x16",
               "���[�N�A�C�e�����x16��N�����鎞��(�̕���)���͂���܂��B��̂��Z���ꍇ�͂�������enqueu���܂��傤�B",
               "usec",
               kernel_latency_run16,
               valid,
               kernel_latency_kernel);

    INIT_BENCH(BENCH_ENQUEUE_MEMREAD_LATENCY,
               RESULT_TYPE_FLOAT,
               "enqueue memread latency",
               "1byte clEnqueueReadBuffer���鎞�Ԃ͂���܂��B�݂������قǂ悢�ł���",
               "usec",
               memread_latency_run,
               valid,
               "");

    INIT_BENCH(BENCH_ENQUEUE_MEMWRITE_LATENCY,
               RESULT_TYPE_FLOAT,
               "enqueue memwrite latency",
               "1byte clEnqueueWriteBuffer���鎞�Ԃ͂���܂��B�݂������قǂ悢�ł���",
               "usec",
               memwrite_latency_run,
               valid,
               "");

//    INIT_BENCH(BENCH_INVALID_TEST,
//               RESULT_TYPE_INT,
//               "invalid_test",
//               "test",
//               "usec",
//               kernel_latency_run,
//               invalid);


    INIT_BENCH(BENCH_ENQUEUE_MEMREAD_BANDWIDTH,
               RESULT_TYPE_FLOAT,
               "memread bandwidth",
               "clEnqueueReadMemory��bandwidth���͂���܂��BCL_ALLOC_HOSTMEMORY���g������ł͂������g���ĂˁB",
               "MB/s",
               memread_bandwidth_run,
               valid,
               "");

    INIT_BENCH(BENCH_ENQUEUE_MEMWRITE_BANDWIDTH,
               RESULT_TYPE_FLOAT,
               "memwrite bandwidth",
               "clEnqueueWriteMemory��bandwidth���͂���܂��BCL_ALLOC_HOSTMEMORY���g������ł͂������g���ĂˁB",
               "MB/s",
               memwrite_bandwidth_run,
               valid,
               "");

    INIT_BENCH(BENCH_FLOAT1_MEMREAD_BANDWIDTH,
               RESULT_TYPE_FLOAT,
               "global float1 read bandwidth",
               "global memory ���� float1 �ǂ�bandwidth���͂���܂��B",
               "GB/s",
               float1_read_run,
               valid,
               float1_read_kernel);

    INIT_BENCH(BENCH_FLOAT4_MEMREAD_BANDWIDTH,
               RESULT_TYPE_FLOAT,
               "global float4 read bandwidth",
               "global memory ���� float4 �ǂ�bandwidth���͂���܂��B",
               "GB/s",
               float4_read_run,
               valid,
               float4_read_kernel);


    INIT_BENCH(BENCH_FLOAT1_ADD_LATENCY,
               RESULT_TYPE_FLOAT,
               "float1 add latency",
               "float1 �̉��Z���C�e���V�����܂��傤",
               "clk",
               float1_add_latency_run,
               valid,
               float1_add_latency_kernel);


    INIT_BENCH(BENCH_FLOAT2_ADD_LATENCY,
               RESULT_TYPE_FLOAT,
               "float2 add latency",
               "float2 �̉��Z���C�e���V�����܂��傤",
               "clk",
               float2_add_latency_run,
               valid,
               float2_add_latency_kernel);


    INIT_BENCH(BENCH_FLOAT4_ADD_LATENCY,
               RESULT_TYPE_FLOAT,
               "float4 add latency",
               "float4 �̉��Z���C�e���V�����܂��傤",
               "clk",
               float4_add_latency_run,
               valid,
               float4_add_latency_kernel);


    INIT_BENCH(BENCH_INT1_ADD_LATENCY,
               RESULT_TYPE_FLOAT,
               "int1 add latency",
               "int1 �̉��Z���C�e���V�����܂��傤",
               "clk",
               int1_add_latency_run,
               valid,
               int1_add_latency_kernel);


    INIT_BENCH(BENCH_INT4_ADD_LATENCY,
               RESULT_TYPE_FLOAT,
               "int4 add latency",
               "int4 �̉��Z���C�e���V�����܂��傤",
               "clk",
               int4_add_latency_run,
               valid,
               int4_add_latency_kernel);


    INIT_BENCH(BENCH_GMEM_LOAD_LATENCY,
               RESULT_TYPE_FLOAT,
               "gmem load latency",
               "gmem �̃��[�h��ɂ����鎞��",
               "clk",
               gmem_load_latency_run,
               valid,
               gmem_load_latency_kernel);

    INIT_BENCH(BENCH_CONSTANT_LOAD_LATENCY,
               RESULT_TYPE_FLOAT,
               "constant load latency",
               "constant �̃��[�h��ɂ����鎞��",
               "clk",
               constant_load_latency_run,
               valid,
               constant_load_latency_kernel);

    INIT_BENCH(BENCH_GMEM_LOAD_LATENCY_LARGE,
               RESULT_TYPE_FLOAT,
               "gmem load latency uc",
               "gmem �̃��[�h��ɂ����鎞��(�L���b�V���ɓ���Ȃ�)",
               "clk",
               gmem_load_latency_uc_run,
               valid,
               gmem_load_latency_kernel);

    INIT_BENCH(BENCH_LMEM_LOAD_LATENCY,
               RESULT_TYPE_FLOAT,
               "lmem load latency",
               "lmem �̃��[�h��ɂ����鎞��",
               "clk",
               lmem_load_latency_run,
               valid,
               lmem_load_latency_kernel);


    INIT_BENCH(BENCH_MAD1_THROUGHPUT,
               RESULT_TYPE_FLOAT,
               "mad1 throughput",
               "�R���s���[�^�̐��\���v�����܂��B�͂₢�قǂ͂₢�ł��B",
               "GFLOPS",
               mad1_throughput_run,
               valid,
               mad1_throughput_kernel);

    INIT_BENCH(BENCH_MAD1DEP_THROUGHPUT,
               RESULT_TYPE_FLOAT,
               "mad1dep throughput",
               "�R���s���[�^�̐��\���v�����܂��B�͂₢�قǂ͂₢�ł��B",
               "GFLOPS",
               mad1dep_throughput_run,
               valid,
               mad1dep_throughput_kernel);

    INIT_BENCH(BENCH_MAD4_THROUGHPUT,
               RESULT_TYPE_FLOAT,
               "mad4 throughput",
               "�R���s���[�^�̐��\���v�����܂��B�͂₢�قǂ͂₢�ł��B",
               "GFLOPS",
               mad4_throughput_run,
               valid,
               mad4_throughput_kernel);


    return benches;
}
