#include <jni.h>
#include <OpenCL.h>
#include <android/log.h>
#include <stdint.h>
#include <stdlib.h>
#include "CLlib.h"
#include "npr/strbuf.h"
#include "bench/bench.h"

#define  LOG_TAG    "libclminibench"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static jfieldID ptr_id;
static jfieldID cur_dev_config_id;
static jfieldID cur_dev_name_id;
static jfieldID cur_platform_name_id;
static jfieldID dev_names_id;
static jfieldID bench_name_list_id;
static jfieldID bench_desc_list_id;
static jfieldID bench_unit_list_id;
static jfieldID bench_valid_list_id;
static jfieldID bench_cl_code_list_id;
static jfieldID num_bench_id;
static jfieldID result_type_list_id;

static jfieldID result_code_id;
static jfieldID result_fval_id;
static jfieldID result_ival_id;
static jfieldID result_score_id;
static jfieldID result_error_message_id;

enum {
    SELECT_PLATFORM,
    SELECT_DEVICE,
};

struct cl_minibench {
    int cur_platform;
    int cur_device;

    int num_platform;
    cl_platform_id *platforms;
    int num_device;
    cl_device_id *devices;

    int state;

    struct clinst_bench_context bench_ctxt;
    struct clinst_bench *bench;

    cl_minibench() {
        platforms = NULL;
        devices = NULL;
        bench = NULL;
    }
};

extern "C" {

JNIEXPORT void JNICALL Java_jp_main_Int_clminibench_CLminibench_seldev(JNIEnv *env, jobject obj, int dev);
JNIEXPORT void JNICALL Java_jp_main_Int_clminibench_CLminibench_init0(JNIEnv *env, jclass obj);
JNIEXPORT void JNICALL Java_jp_main_Int_clminibench_CLminibench_init(JNIEnv *env, jobject obj);
JNIEXPORT jobject JNICALL Java_jp_main_Int_clminibench_CLminibench_run(JNIEnv *env, jobject obj, jint id);

}


JNIEXPORT void JNICALL
Java_jp_main_Int_clminibench_CLminibench_init0(JNIEnv *env, jclass cls)
{
    if (ptr_id == 0) {
        ptr_id = env->GetFieldID(cls, "ptr_value", "J");
        cur_dev_config_id = env->GetFieldID(cls, "cur_dev_config", "Ljava/lang/String;");
        cur_dev_name_id = env->GetFieldID(cls, "cur_dev_name", "Ljava/lang/String;");
        cur_platform_name_id = env->GetFieldID(cls, "cur_platform_name", "Ljava/lang/String;");
        cur_platform_name_id = env->GetFieldID(cls, "cur_platform_name", "Ljava/lang/String;");
        bench_name_list_id = env->GetFieldID(cls, "bench_name_list", "[Ljava/lang/String;");
        bench_desc_list_id = env->GetFieldID(cls, "bench_desc_list", "[Ljava/lang/String;");
        bench_unit_list_id = env->GetFieldID(cls, "bench_unit_list", "[Ljava/lang/String;");
        bench_cl_code_list_id = env->GetFieldID(cls, "bench_cl_code_list", "[Ljava/lang/String;");
        result_type_list_id = env->GetFieldID(cls, "result_type_list", "[I");
        bench_valid_list_id = env->GetFieldID(cls, "bench_valid_list", "[Z");
        num_bench_id = env->GetFieldID(cls, "num_bench", "I");

        dev_names_id = env->GetFieldID(cls, "dev_names", "[Ljava/lang/String;");


        {
            jclass result_cls = env->FindClass("jp/main/Int/clminibench/BenchResult");
            result_code_id = env->GetFieldID(result_cls, "code", "I");
            result_fval_id = env->GetFieldID(result_cls, "fval", "D");
            result_ival_id = env->GetFieldID(result_cls, "ival", "I");
            result_score_id = env->GetFieldID(result_cls, "score", "Ljava/lang/String;");
            result_error_message_id = env->GetFieldID(result_cls, "error_message", "Ljava/lang/String;");
        }
    }
}

static void
platform_select(JNIEnv *env, jobject obj, cl_minibench *b, int sel)
{
    cl_platform_id p = b->platforms[sel];
    cl_uint num, num2;
    b->cur_platform = sel;
    clGetDeviceIDs(p, CL_DEVICE_TYPE_ALL, 0, NULL, &num);

    cl_device_id *devs = new cl_device_id[num];

    clGetDeviceIDs(p, CL_DEVICE_TYPE_ALL, num, devs, &num2);

    delete [] b->devices;
    b->devices = devs;
    b->num_device = num;

    jclass string_class = env->FindClass("java/lang/String");
    jobjectArray dev_names = env->NewObjectArray(num, string_class, NULL);

    for (int i=0; i<num; i++) {
        char *buffer;
        size_t sz;
        clGetDeviceInfo(devs[i], CL_DEVICE_NAME, 0, NULL, &sz);
        buffer = (char*)malloc(sz+1);
        clGetDeviceInfo(devs[i], CL_DEVICE_NAME, sz, buffer, &sz);
        buffer[sz] = '\0';
        jstring name = env->NewStringUTF(buffer);
        free(buffer);
        env->SetObjectArrayElement(dev_names, i, name);
    }

    {
        char *buffer;
        size_t len;
        clGetPlatformInfo(p, CL_PLATFORM_NAME, 0, NULL, &len);
        buffer = (char*)malloc(len+1);
        clGetPlatformInfo(p, CL_PLATFORM_NAME, len, buffer, &len);
        buffer[len] = '\0';
        jstring platform_name = env->NewStringUTF(buffer);
        free(buffer);
        env->SetObjectField(obj, cur_platform_name_id, platform_name);
    }
    env->SetObjectField(obj, dev_names_id, dev_names);
}


JNIEXPORT void JNICALL
Java_jp_main_Int_clminibench_CLminibench_init(JNIEnv *env, jobject obj)
{
    int r = cllib_init();
    if (r < 0) {
        return;
    }

    jclass cls = env->GetObjectClass(obj);

    cl_int ret;
    cl_uint num, num2;
    cl_platform_id *platforms;
    cl_minibench *b;

    clGetPlatformIDs(0, NULL, &num);
    platforms = new cl_platform_id[num];

    clGetPlatformIDs(num, platforms, &num2);

    b = new cl_minibench();

    b->num_platform = num;
    b->platforms = platforms;
    b->state = SELECT_PLATFORM;
    b->bench = clinst_bench_init();

    platform_select(env, obj, b, 0);

    jclass string_class = env->FindClass("java/lang/String");
    jobjectArray bench_name_list = env->NewObjectArray(BENCH_NUM, string_class, NULL);
    jobjectArray bench_desc_list = env->NewObjectArray(BENCH_NUM, string_class, NULL);
    jobjectArray bench_unit_list = env->NewObjectArray(BENCH_NUM, string_class, NULL);
    jobjectArray bench_cl_code_list = env->NewObjectArray(BENCH_NUM, string_class, NULL);
    jbooleanArray bench_valid_list = env->NewBooleanArray(BENCH_NUM);
    jintArray bench_result_type_list = env->NewIntArray(BENCH_NUM);


    for (int i=0; i<BENCH_NUM; i++) {
        jstring str;
        str = env->NewStringUTF(b->bench[i].name);
        env->SetObjectArrayElement(bench_name_list, i, str);

        str = env->NewStringUTF(b->bench[i].cl_code);
        env->SetObjectArrayElement(bench_cl_code_list, i, str);

        str = env->NewStringUTF(b->bench[i].desc);
        env->SetObjectArrayElement(bench_desc_list, i, str);

        str = env->NewStringUTF(b->bench[i].unit_str);
        env->SetObjectArrayElement(bench_unit_list, i, str);

        {
            jint v = b->bench[i].result_type;
            env->SetIntArrayRegion(bench_result_type_list, i, 1, &v);
        }
    }

    env->SetObjectField(obj, bench_name_list_id, bench_name_list);
    env->SetObjectField(obj, bench_desc_list_id, bench_desc_list);
    env->SetObjectField(obj, bench_unit_list_id, bench_unit_list);
    env->SetObjectField(obj, bench_cl_code_list_id, bench_cl_code_list);
    env->SetIntField(obj, num_bench_id, BENCH_NUM);
    env->SetObjectField(obj, result_type_list_id, bench_result_type_list);

    env->SetLongField(obj, ptr_id, (uintptr_t)b);
}

static void
int_info(struct npr_strbuf *sb,
         cl_device_id dev,
         const char *tag,
         int param)
{
    int val;
    size_t sz;
    cl_int r;
    r = clGetDeviceInfo(dev, param, sizeof(val), &val, &sz);

    if (r == CL_SUCCESS) {
        npr_strbuf_printf(sb, "%s:%d\n", tag, val);
    }
}

static void
ulong_info(struct npr_strbuf *sb,
           cl_device_id dev,
           const char *tag,
           int param)
{
    uint64_t val;
    size_t sz;
    cl_int r;
    r = clGetDeviceInfo(dev, param, sizeof(val), &val, &sz);

    if (r == CL_SUCCESS) {
        npr_strbuf_printf(sb, "%s:%lld\n", tag, val);
    }
}

static void
fp_config_info(struct npr_strbuf *sb,
               cl_device_id dev,
               const char *tag,
               int param)
{
    cl_device_fp_config val;
    size_t sz;
    cl_int r;
    r = clGetDeviceInfo(dev, param, sizeof(val), &val, &sz);

    if (r == CL_SUCCESS) {
        npr_strbuf_printf(sb, "%s:", tag);

        if (val & CL_FP_DENORM) {
            npr_strbuf_puts(sb,"denorm,");
        }
        if (val & CL_FP_INF_NAN) {
            npr_strbuf_puts(sb,"inf/nan,");
        }
        if (val & CL_FP_ROUND_TO_NEAREST) {
            npr_strbuf_puts(sb,"rte,");
        }
        if (val & CL_FP_ROUND_TO_ZERO) {
            npr_strbuf_puts(sb,"rtz,");
        }
        if (val & CL_FP_ROUND_TO_INF) {
            npr_strbuf_puts(sb,"rtpn,");
        }
        if (val & CL_FP_FMA) {
            npr_strbuf_puts(sb,"fma,");
        }
        if (val & CL_FP_SOFT_FLOAT) {
            npr_strbuf_puts(sb,"soft,");
        }
#ifdef CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT
        if (val & CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT) {
            npr_strbuf_puts(sb,"divsqrt,");
        }
#endif

        npr_strbuf_putc(sb,'\n');
    }
}

static void
string_info(struct npr_strbuf *sb,
            cl_device_id dev,
            const char *tag,
            int param)
{
    char buf[1024];
    size_t sz;
    clGetDeviceInfo(dev, param, sizeof(buf), buf, &sz);

    npr_strbuf_printf(sb, "%s:%s\n", tag, buf);
}



JNIEXPORT void JNICALL
Java_jp_main_Int_clminibench_CLminibench_seldev(JNIEnv *env, jobject obj, int dev)
{
    cl_minibench *app = (cl_minibench*)env->GetLongField(obj, ptr_id);

    app->cur_device = dev;

    cl_device_id *devs = app->devices;
    struct npr_strbuf sb;
    npr_strbuf_init(&sb);
    int_info(&sb, devs[dev], "max compute units", CL_DEVICE_MAX_COMPUTE_UNITS);
    int_info(&sb, devs[dev], "max work item dimensions", CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS);
    int_info(&sb, devs[dev], "max work group size", CL_DEVICE_MAX_WORK_GROUP_SIZE);
    //int_array_info(&sb, devs[dev], "max work item sizes", CL_DEVICE_MAX_WORK_ITEM_SIZES);
    int_info(&sb, devs[dev], "pref vector width char", CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR);
    int_info(&sb, devs[dev], "pref vector width short", CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT);
    int_info(&sb, devs[dev], "pref vector width int", CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT);
    int_info(&sb, devs[dev], "pref vector width long", CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG);
    int_info(&sb, devs[dev], "pref vector width float", CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT);
    int_info(&sb, devs[dev], "pref vector width double", CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE);
    int_info(&sb, devs[dev], "clock", CL_DEVICE_MAX_CLOCK_FREQUENCY);
    int_info(&sb, devs[dev], "address bits", CL_DEVICE_ADDRESS_BITS);
    int_info(&sb, devs[dev], "cache type", CL_DEVICE_GLOBAL_MEM_CACHE_TYPE);
    int_info(&sb, devs[dev], "cacheline size", CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE);
    ulong_info(&sb, devs[dev], "cache size", CL_DEVICE_GLOBAL_MEM_CACHE_SIZE);
    ulong_info(&sb, devs[dev], "mem size", CL_DEVICE_GLOBAL_MEM_SIZE);
    ulong_info(&sb, devs[dev], "max mem alloc size", CL_DEVICE_MAX_MEM_ALLOC_SIZE);
    int_info(&sb, devs[dev], "image", CL_DEVICE_IMAGE_SUPPORT);
#ifdef CL_DEVICE_WAVEFRONT_WIDTH_AMD
    int_info(&sb, devs[dev], "wavefront width", CL_DEVICE_WAVEFRONT_WIDTH_AMD);
#endif
    int_info(&sb, devs[dev], "warp size", CL_DEVICE_WARP_SIZE_NV);
#ifdef CL_DEVICE_SIMD_PER_COMPUTE_UNIT_AMD
    int_info(&sb, devs[dev], "simd per CU", CL_DEVICE_SIMD_PER_COMPUTE_UNIT_AMD);
#endif
#ifdef CL_DEVICE_SIMD_WIDTH_AMD
    int_info(&sb, devs[dev], "simd width", CL_DEVICE_SIMD_WIDTH_AMD);
#endif
#ifdef CL_DEVICE_SIMD_INSTRUCTION_WIDTH_AMD
    int_info(&sb, devs[dev], "simd instruction width", CL_DEVICE_SIMD_INSTRUCTION_WIDTH_AMD);
#endif
    int_info(&sb, devs[dev], "host unified memory", CL_DEVICE_HOST_UNIFIED_MEMORY);
    fp_config_info(&sb, devs[dev], "float", CL_DEVICE_SINGLE_FP_CONFIG);
    fp_config_info(&sb, devs[dev], "double", CL_DEVICE_DOUBLE_FP_CONFIG);
    fp_config_info(&sb, devs[dev], "half", CL_DEVICE_HALF_FP_CONFIG);
    string_info(&sb, devs[dev], "profile", CL_DEVICE_PROFILE);
    int_info(&sb, devs[dev], "local mem type", CL_DEVICE_LOCAL_MEM_TYPE);
    ulong_info(&sb, devs[dev], "local mem size", CL_DEVICE_LOCAL_MEM_SIZE);
    string_info(&sb, devs[dev], "version", CL_DEVICE_VERSION);
    string_info(&sb, devs[dev], "extensions", CL_DEVICE_EXTENSIONS);

    if (app->state == SELECT_DEVICE) {
        clinst_bench_fini_context(&app->bench_ctxt);
    }

    app->state = SELECT_DEVICE;

    clinst_bench_init_context(&app->bench_ctxt, devs[dev]);

    npr_strbuf_putc(&sb, '\n');

    jstring config = env->NewStringUTF(sb.buf);
    npr_strbuf_fini(&sb);

    env->SetObjectField(obj, cur_dev_config_id, config);

    {
        char buffer[1024];
        size_t sz;
        clGetDeviceInfo(devs[dev], CL_DEVICE_NAME, 1024, buffer, &sz);
        jstring name = env->NewStringUTF(buffer);
        env->SetObjectField(obj, cur_dev_name_id, name);
    }

    /*
    for (int i=0; i<BENCH_NUM; i++) {
        int valid = app->bench[i].is_valid(devs[dev], &app->invalid_reason[i]);
        app->valid[i] = valid;
        EnableWindow(app->bench_run_button[i], valid);
        Edit_SetText(app->bench_label[i], app->bench[i].name);
    }
    */
}


JNIEXPORT jobject JNICALL
Java_jp_main_Int_clminibench_CLminibench_run(JNIEnv *env, jobject obj, jint id)
{
    cl_minibench *app = (cl_minibench*)env->GetLongField(obj, ptr_id);
    struct clinst_bench *b = app->bench;
    struct bench_result r;
    jclass result_cls = env->FindClass("jp/main/Int/clminibench/BenchResult");
    jmethodID mid = env->GetMethodID(result_cls, "<init>", "()V");
    jobject robj = env->NewObject(result_cls, mid);

    r.fval = 0;

    //LOGI("run %d\n", id);

    b[id].run(&r, &app->bench_ctxt);

    env->SetIntField(robj, result_code_id, r.code);

    if (r.code != BENCH_OK) {
        jstring error_message = env->NewStringUTF(r.error_message);
        env->SetObjectField(robj, result_error_message_id, error_message);

        //LOGI("run %d: code=%d, error=%s, ker=%s\n", id, r.code, r.error_message, b[id].cl_code);
    } else {
        env->SetIntField(robj, result_ival_id, r.ival);
        env->SetDoubleField(robj, result_fval_id, r.fval);

        jstring score_message = env->NewStringUTF(r.score);
        env->SetObjectField(robj, result_score_id, score_message);

        //LOGI("run %d: code=%d, ival=%d, fval=%f\n", id, r.code, r.ival, r.fval);
    }

    return robj;
}
