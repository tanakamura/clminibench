
static void
inst_latency_run(struct bench_result *r,
                 struct clinst_bench_context *ctxt,
                 const char *kern,
                 int nelem,
                 int flags)
{
    int i;
    timeval_t t0, t1;
    double usec;
    int nloop = 2;
    int kernel_nloop = 65536;
    int kernel_unroll = 64;
    cl_mem data;
    int size = 128*1024*1024;
    char *host = aligned_malloc(size, 128);

    DECL_PARAM();


    if (flags & CONSTANT_MEM) {
        size = 32*1024;
    }

    memset(host, 0, size);
    if (flags & INIT_INDEX) {
        int i;
        int *ptr = (int*)host;
        for (i=0; i<kernel_nloop; i++) {
            ptr[i] = i+64;
        }
    }

    KERNEL(kern);

    data = clCreateBuffer(ctxt->ctxt, CL_MEM_READ_WRITE, size, NULL, NULL);
    clEnqueueWriteBuffer(ctxt->queue, data, CL_TRUE, 0, size, host, 0, NULL, NULL);

    if (flags & THROUGHPUT) {
        WORK_DIM_PREF();
    } else {
        WORK_DIM1(1,1);
    }

    KERNEL_ARG_GMEM(0, data);
    KERNEL_ARG_FLOAT(1, 1.0f);
    KERNEL_ARG_FLOAT(2, 0.0f);
    KERNEL_ARG_INT(3, kernel_nloop);

    if (flags & HAS_LMEM) {
        KERNEL_ARG_LMEM(4, 4096);
        if (flags & CONSTANT_MEM) {
            KERNEL_ARG_GMEM(5, data);
        }
    } else {
        if (flags & CONSTANT_MEM) {
            KERNEL_ARG_GMEM(4, data);
        }
    }

    RUN();

    timeval_get(&t0);

    for (i=0; i<nloop; i++) {
        RUN();
    }

    timeval_get(&t1);

    RELEASE_ALL();

    usec = timeval_diff_usec(&t0, &t1);

    if (flags & THROUGHPUT) {
        double gsz = gw[0];
        double total_op = gsz * kernel_nloop * kernel_unroll * nloop * nelem;

        r->fval = total_op / (usec*1000.0);
    } else {
        cl_int freq;
        size_t sz;
        double sec_per_clock, sec, sec_per_op;
        double num_op = nloop * (double)kernel_nloop * kernel_unroll;
        clGetDeviceInfo(ctxt->dev, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(freq), &freq, &sz);

        sec_per_clock = 1.0/((double)freq * 1000 * 1000);
        sec = usec / (1000 * 1000.0);
        sec_per_op = sec / num_op;

        r->fval = sec_per_op * 1000 * 1000.0 * freq;
    }

    r->code = BENCH_OK;
    get_score(r->score);

    aligned_free(host);
    clReleaseMemObject(data);
}



static const char float1_add_latency_kernel[] = 
    K(void __kernel f(__global float *result, float v0, float v1, int nloop) {
            for (int i=0; i<nloop; i++) {
                ITER64(v0 += v1;)
            }

            *result = v0;

        });

static void
float1_add_latency_run(struct bench_result *r,
                struct clinst_bench_context *ctxt)
{
    inst_latency_run(r,ctxt,float1_add_latency_kernel,1, 0);
}

static const char float2_add_latency_kernel[] = 
    K(void __kernel f(__global float2 *result, float v0_, float v1_, int nloop) {
            float2 v0 = v0_;
            float2 v1 = v1_;

            for (int i=0; i<nloop; i++) {
                ITER64(v0 += v1;)
            }

            *result = v0;
        });

static void
float2_add_latency_run(struct bench_result *r,
                struct clinst_bench_context *ctxt)
{
    inst_latency_run(r,ctxt,float2_add_latency_kernel,1, 0);
}


static const char float4_add_latency_kernel[] = 
    K(void __kernel f(__global float4 *result, float v0_, float v1_, int nloop) {
            float4 v0 = v0_;
            float4 v1 = v1_;

            for (int i=0; i<nloop; i++) {
                ITER64(v0 += v1;)
            }

            *result = v0;
        });

static void
float4_add_latency_run(struct bench_result *r,
                struct clinst_bench_context *ctxt)
{
    inst_latency_run(r,ctxt,float4_add_latency_kernel,1, 0);
}





static const char int1_add_latency_kernel[] = 
    K(void __kernel f(__global int *result, int v0, int v1, int nloop) {
            for (int i=0; i<nloop; i++) {
                ITER64(v0 += v1;)
            }

            *result = v0;

        });

static void
int1_add_latency_run(struct bench_result *r,
                struct clinst_bench_context *ctxt)
{
    inst_latency_run(r,ctxt,int1_add_latency_kernel,1, 0);
}



static const char int4_add_latency_kernel[] = 
    K(void __kernel f(__global int4 *result, int v0_, int v1_, int nloop) {
            int4 v0 = v0_;
            int4 v1 = v1_;

            for (int i=0; i<nloop; i++) {
                ITER64(v0 += v1;);
            }

            *result = v0;
        });

static void
int4_add_latency_run(struct bench_result *r,
                struct clinst_bench_context *ctxt)
{
    inst_latency_run(r,ctxt,int4_add_latency_kernel,1, 0);
}




static const char gmem_load_latency_kernel[] = 
//    K(#pragma OPENCL EXTENSION cl_intel_printf )
//    "\n"
    K(void __kernel f(__global int *gmem, float v0_, float v1_, int nloop) {
            int off = 0;

            for (int i=0; i<nloop; i++) {
                ITER64(off = gmem[off];)
            }

            *gmem = off;
        });


static const char constant_load_latency_kernel[] = 
    K(void __kernel f(__global int *gmem,float v0_, float v1_, int nloop,  __constant int *cmem) {
            int off = 0;

            for (int i=0; i<nloop; i++) {
                ITER64(off = cmem[off];)
            }

            *gmem = off;
        });

static void
gmem_load_latency_run(struct bench_result *r,
                      struct clinst_bench_context *ctxt)
{
    inst_latency_run(r,ctxt,gmem_load_latency_kernel,1, 0);
}

static void
constant_load_latency_run(struct bench_result *r,
                          struct clinst_bench_context *ctxt)
{
    inst_latency_run(r,ctxt,constant_load_latency_kernel,1, CONSTANT_MEM);
}

static void
gmem_load_latency_uc_run(struct bench_result *r,
                         struct clinst_bench_context *ctxt)
{
    inst_latency_run(r,ctxt,gmem_load_latency_kernel,1, INIT_INDEX);
}

static const char lmem_load_latency_kernel[] = 
    K(void __kernel f(__global int *gmem, float v0_, float v1_, int nloop, __local int *lmem) {
            int off = 0;

            lmem[0] = gmem[0];

            for (int i=0; i<nloop; i++) {
                ITER64(off = lmem[off];)
            }

            *gmem = off;
        });

static void
lmem_load_latency_run(struct bench_result *r,
                      struct clinst_bench_context *ctxt)
{
    inst_latency_run(r,ctxt,lmem_load_latency_kernel,1, HAS_LMEM);
}


static const char mad1_throughput_kernel[] = 
    K(void __kernel f(__global float *gmem, float v0_, float v1_, int nloop) {
            float v00 = gmem[0];
            float v01 = gmem[1];
            float v02 = gmem[2];

            float v10 = gmem[3];
            float v11 = gmem[4];
            float v12 = gmem[5];

            float v20 = gmem[6];
            float v21 = gmem[7];
            float v22 = gmem[8];

            float v30 = gmem[9];
            float v31 = gmem[10];
            float v32 = gmem[11];

            for (int i=0; i<nloop; i++) {
                ITER16(v00 = v00 * v01 + v02;);
                ITER16(v10 = v10 * v11 + v12;);
                ITER16(v20 = v20 * v21 + v22;);
                ITER16(v30 = v30 * v31 + v32;);
            }

            *gmem = v00 + v10 + v20 + v30;
        });

static void
mad1_throughput_run(struct bench_result *r,
                    struct clinst_bench_context *ctxt)
{
    inst_latency_run(r,ctxt,mad1_throughput_kernel, 2, THROUGHPUT);
}

static const char double_mad1_throughput_kernel[] =
    "#pragma OPENCL EXTENSION cl_khr_fp64 : enable\n\n"

    K(void __kernel f(__global float *gmem, float v0_, float v1_, int nloop) {
            double v00 = gmem[0];
            double v01 = gmem[1];
            double v02 = gmem[2];

            double v10 = gmem[3];
            double v11 = gmem[4];
            double v12 = gmem[5];

            double v20 = gmem[6];
            double v21 = gmem[7];
            double v22 = gmem[8];

            double v30 = gmem[9];
            double v31 = gmem[10];
            double v32 = gmem[11];

            for (int i=0; i<nloop; i++) {
                ITER16(v00 = v00 * v01 + v02;);
                ITER16(v10 = v10 * v11 + v12;);
                ITER16(v20 = v20 * v21 + v22;);
                ITER16(v30 = v30 * v31 + v32;);
            }

            *gmem = v00 + v10 + v20 + v30;
        });

static void
double_mad1_throughput_run(struct bench_result *r,
                           struct clinst_bench_context *ctxt)
{
    inst_latency_run(r,ctxt,double_mad1_throughput_kernel, 2, THROUGHPUT);
}


static const char mad1dep_throughput_kernel[] = 
    K(void __kernel f(__global float *gmem, float v0_, float v1_, int nloop) {
            float v00 = gmem[0];
            float v01 = gmem[1];
            float v02 = gmem[2];

            for (int i=0; i<nloop; i++) {
                ITER64(v00 = v00 * v01 + v02;);
            }

            *gmem = v00;
        });

static void
mad1dep_throughput_run(struct bench_result *r,
                       struct clinst_bench_context *ctxt)
{
    inst_latency_run(r,ctxt,mad1dep_throughput_kernel, 2, THROUGHPUT);
}



static const char mad4_throughput_kernel[] = 
    K(void __kernel f(__global float4 *gmem, float v0_, float v1_, int nloop) {
            float4 v00 = gmem[0];
            float4 v01 = gmem[1];
            float4 v02 = gmem[2];

            float4 v10 = gmem[3];
            float4 v11 = gmem[4];
            float4 v12 = gmem[5];

            float4 v20 = gmem[6];
            float4 v21 = gmem[7];
            float4 v22 = gmem[8];

            float4 v30 = gmem[9];
            float4 v31 = gmem[10];
            float4 v32 = gmem[11];

            for (int i=0; i<nloop; i++) {
                ITER16(v00 = v00 * v01 + v02;);
                ITER16(v10 = v10 * v11 + v12;);
                ITER16(v20 = v20 * v21 + v22;);
                ITER16(v30 = v30 * v31 + v32;);
            }

            *gmem = v00 + v10 + v20 + v30;
        });

static void
mad4_throughput_run(struct bench_result *r,
                    struct clinst_bench_context *ctxt)
{
    inst_latency_run(r,ctxt,mad4_throughput_kernel, 8, THROUGHPUT);
}
