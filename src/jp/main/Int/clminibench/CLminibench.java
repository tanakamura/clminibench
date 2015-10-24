package jp.main.Int.clminibench;

public class CLminibench {
    static final int RESULT_TYPE_FLOAT = 0;
    static final int RESULT_TYPE_INT = 1;

    static {
	//System.load("/system/vendor/lib/egl/libGLES_mali.so");
        //System.load("/system/vendor/lib/libOpenCL.so");
	System.loadLibrary("clminibench");
    }

    long ptr_value;

    String cur_platform_name;
    String cur_dev_name;
    String cur_dev_config;

    int num_bench;

    String dev_names[];
    String bench_name_list[];
    String bench_desc_list[];
    String bench_unit_list[];
    String bench_cl_code_list[];
    boolean bench_valid_list[];
    int result_type_list[];

    public static native int init0();

    public native void init();

    public native void seldev(int dev);

    public native BenchResult run(int bench_id);
}
