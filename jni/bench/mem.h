
static void
memread_latency_run(struct bench_result *r,
                    struct clinst_bench_context *ctxt)
{
    int i;
    timeval_t t0, t1;
    double usec;
    int nloop = 100;
    cl_mem m;
    char buffer[1];

    m = clCreateBuffer(ctxt->ctxt, CL_MEM_READ_WRITE, 1024, NULL, NULL);

    clEnqueueReadBuffer(ctxt->queue, m, CL_TRUE, 0, 1, buffer, 0, NULL, NULL);

    timeval_get(&t0);

    for (i=0; i<nloop; i++) {
        clEnqueueReadBuffer(ctxt->queue, m, CL_TRUE, 0, 1, buffer, 0, NULL, NULL);
        clFlush(ctxt->queue);
    }

    timeval_get(&t1);

    clReleaseMemObject(m);

    usec = timeval_diff_usec(&t0, &t1);
    usec /= nloop;

    r->code = BENCH_OK;
    r->fval = usec;
    get_score(r->score);
}

static void
memwrite_latency_run(struct bench_result *r,
                     struct clinst_bench_context *ctxt)
{
    int i;
    timeval_t t0, t1;
    double usec;
    int nloop = 100;
    cl_mem m;
    char buffer[1];

    m = clCreateBuffer(ctxt->ctxt, CL_MEM_READ_WRITE, 1024, NULL, NULL);

    clEnqueueReadBuffer(ctxt->queue, m, CL_TRUE, 0, 1, buffer, 0, NULL, NULL);

    timeval_get(&t0);

    for (i=0; i<nloop; i++) {
        clEnqueueWriteBuffer(ctxt->queue, m, CL_TRUE, 0, 1, buffer, 0, NULL, NULL);
        clFlush(ctxt->queue);
    }

    timeval_get(&t1);

    clReleaseMemObject(m);

    usec = timeval_diff_usec(&t0, &t1);
    usec /= nloop;

    r->code = BENCH_OK;
    r->fval = usec;
    get_score(r->score);
}


static void
memwrite_bandwidth_run(struct bench_result *r,
                       struct clinst_bench_context *ctxt)
{
    int i;
    timeval_t t0, t1;
    double usec;
    int nloop = 10;
    cl_mem m;
    int size = 1024*1024*128;

    char *host = aligned_malloc(size, 128);

    m = clCreateBuffer(ctxt->ctxt, CL_MEM_READ_WRITE, size, NULL, NULL);

    clEnqueueWriteBuffer(ctxt->queue, m, CL_TRUE, 0, size, host, 0, NULL, NULL);

    timeval_get(&t0);

    for (i=0; i<nloop; i++) {
        clEnqueueWriteBuffer(ctxt->queue, m, CL_TRUE, 0, size, host, 0, NULL, NULL);
        clFlush(ctxt->queue);
    }

    timeval_get(&t1);

    clReleaseMemObject(m);
    aligned_free(host);

    usec = timeval_diff_usec(&t0, &t1);

    r->code = BENCH_OK;
    r->fval = (size*(double)nloop) / usec;
    get_score(r->score);
}


static void
memread_bandwidth_run(struct bench_result *r,
                      struct clinst_bench_context *ctxt)
{
    int i;
    timeval_t t0, t1;
    double usec;
    int nloop = 10;
    cl_mem m;
    int size = 1024*1024*128;

    char *host = aligned_malloc(size, 128);

    m = clCreateBuffer(ctxt->ctxt, CL_MEM_READ_WRITE, size, NULL, NULL);

    clEnqueueReadBuffer(ctxt->queue, m, CL_TRUE, 0, size, host, 0, NULL, NULL);

    timeval_get(&t0);

    for (i=0; i<nloop; i++) {
        clEnqueueReadBuffer(ctxt->queue, m, CL_TRUE, 0, size, host, 0, NULL, NULL);
        clFinish(ctxt->queue);
    }

    timeval_get(&t1);

    clReleaseMemObject(m);
    aligned_free(host);

    usec = timeval_diff_usec(&t0, &t1);

    r->code = BENCH_OK;
    r->fval = (size*(double)nloop) / usec;
    get_score(r->score);
}

static void
global_read_run(struct bench_result *r,
                struct clinst_bench_context *ctxt,
                const char *kern,
                int elem_size)
{
    int i;
    timeval_t t0, t1;
    double usec;
    int nloop = 100;
    cl_mem data;
    int size = 1024*1024*128;
    char *host = aligned_malloc(size, 128);

    DECL_PARAM();
    KERNEL(kern);

    memset(host, 0, size);
    data = clCreateBuffer(ctxt->ctxt, CL_MEM_READ_WRITE, size, NULL, NULL);

    clEnqueueWriteBuffer(ctxt->queue, data, CL_TRUE, 0, size, host, 0, NULL, NULL);
    aligned_free(host);

    KERNEL_ARG_GMEM(0, data);
    KERNEL_ARG_GMEM(2, data);

    WORK_DIM_PREF();

    {
        int lsz = lw[0];
        int gsz = gw[0]/lsz;

        int nelem = size / elem_size;
        int per_group = nelem/gsz;
        int item_step = lsz;
        int nloop = per_group / item_step;

        KERNEL_ARG_INT(1, nloop);
        KERNEL_ARG_INT(3, per_group);
        KERNEL_ARG_INT(4, item_step);
    }

    RUN();

    timeval_get(&t0);

    for (i=0; i<nloop; i++) {
        timeval_get(&t1);
        double msec = timeval_diff_msec(&t0, &t1);
        if (msec >= TIMEOUT) {
            /* timeout */
            break;
        }

        RUN();
    }

    nloop = i;

    timeval_get(&t1);

    RELEASE_ALL();

    usec = timeval_diff_usec(&t0, &t1);

    r->code = BENCH_OK;
    r->fval = (size*(double)nloop) / (usec*1000.0);
    get_score(r->score);

    clReleaseMemObject(data);
}



static const char float1_read_kernel[] = 
    K(void __kernel f(__global float *data, int nloop, __global float *r, int per_group_size, int item_step) {
            float v = 0;
            int gid = get_group_id(0);
            int lid = get_local_id(0);
            __global float *in = r + per_group_size * gid + lid;
            for (int i=0; i<nloop; ) {
                ITER64(v += in[item_step * (i++)];);
            }
            *r = v;
        });

static void
float1_read_run(struct bench_result *r,
                struct clinst_bench_context *ctxt)
{
    global_read_run(r,ctxt,float1_read_kernel,4);
}

static const char float4_read_kernel[] = 
    K(void __kernel f(__global float4 *data, unsigned int nloop, __global float4 *r, int per_group_size, int item_step) {
            float4 v = 0;
            int gid = get_group_id(0);
            int lid = get_local_id(0);
            __global float4 *in = r + per_group_size * gid + lid;
            for (int i=0; i<nloop; ) {
                ITER64(v += in[item_step * (i++)];);
            }
            *r = v;
        });

static void
float4_read_run(struct bench_result *r,
                struct clinst_bench_context *ctxt)
{
    global_read_run(r,ctxt,float4_read_kernel,16);
}


