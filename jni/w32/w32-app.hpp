#ifndef APP_H
#define APP_H

#include "CL/cl.h"
#include "bench/bench.h"

enum control_id {
    ID_PLATFORM_SELECT = 2000,
    ID_DEVICE_SELECT,
    ID_INFO_EDIT,
    ID_DESC_EDIT,

    ID_BENCH_LABEL = 3000,
    ID_BENCH_BUTTON = 4000
};

struct App {
    enum state {
        SELECT_PLATFORM,
        SELECT_DEVICE
    } state ;

    HINSTANCE app;
    HWND main_win;
    HWND platform_comb;
    HWND device_comb;
    HWND info_edit;
    HWND desc_edit;
    HWND error_edit;

    WNDPROC def_bench_label_proc[BENCH_NUM];
    WNDPROC def_bench_button_proc[BENCH_NUM];
    HWND bench_label[BENCH_NUM];
    HWND bench_run_button[BENCH_NUM];
    const char *invalid_reason[BENCH_NUM];
    int valid[BENCH_NUM];

    HFONT win_font;

    int num_platform;
    int cur_platform;
    cl_platform_id *platforms;

    int num_device;
    int cur_device;
    cl_device_id *devices;

    struct clinst_bench_context bench_ctxt;
    struct clinst_bench *bench;

    App() {
        platforms = NULL;
        devices = NULL;
    }

    ~App() {
        delete [] platforms;
        delete [] devices;
    }
};


#endif
