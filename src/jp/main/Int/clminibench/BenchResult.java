package jp.main.Int.clminibench;

public class BenchResult {
    static final int BENCH_OK = 0;
    static final int BUILD_ERROR = 1;

    int code;                   // 0 = OK, 1=BUILD_ERROR

    double fval;
    int ival;
    String score;
    String error_message;
}
