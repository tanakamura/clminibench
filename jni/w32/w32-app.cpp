#include <windows.h>
#include <windowsx.h>
#include <CL/cl.h>
#include <stdio.h>
#include <commctrl.h>
#include <ostream>
#include <sstream>
#include "resource.h"
#include "w32-app.hpp"

#pragma comment(linker, \
    "\"/manifestdependency:type='Win32' "\
    "name='Microsoft.Windows.Common-Controls' "\
    "version='6.0.0.0' "\
    "processorArchitecture='*' "\
    "publicKeyToken='6595b64144ccf1df' "\
    "language='*'\"")

#ifndef CL_DEVICE_HALF_FP_CONFIG
#define CL_DEVICE_HALF_FP_CONFIG                    0x1033
#endif
#ifndef CL_DEVICE_SIMD_WIDTH_AMD
#define CL_DEVICE_SIMD_WIDTH_AMD                    0x4041
#endif
#ifndef CL_DEVICE_SIMD_INSTRUCTION_WIDTH_AMD
#define CL_DEVICE_SIMD_INSTRUCTION_WIDTH_AMD        0x4042
#endif
#ifndef CL_DEVICE_WAVEFRONT_WIDTH_AMD
#define CL_DEVICE_WAVEFRONT_WIDTH_AMD               0x4043
#endif
#ifndef CL_DEVICE_SIMD_PER_COMPUTE_UNIT_AMD
#define CL_DEVICE_SIMD_PER_COMPUTE_UNIT_AMD         0x4040
#endif

#ifndef CL_DEVICE_WARP_SIZE_NV
#define CL_DEVICE_WARP_SIZE_NV                      0x4003
#endif

#ifndef CL_DEVICE_REGISTERS_PER_BLOCK_NV
#define CL_DEVICE_REGISTERS_PER_BLOCK_NV            0x4002
#endif

#ifndef CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV
#define CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV       0x4000
#endif

#ifndef CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV
#define CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV       0x4001
#endif



#define CLASS_NAME TEXT("clibench")

static LRESULT CALLBACK
bench_label_wndproc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    App *app = (App*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    int id = (int)GetWindowLongPtr(hWnd,GWLP_ID) - ID_BENCH_LABEL;

    switch (message) {
    case WM_MOUSEMOVE: {
        if (app->state == App::SELECT_DEVICE) {
            if (! app->valid[id]) {
                char desc[4096];
                _snprintf(desc, sizeof(desc), "%s (%s)",
                          app->bench[id].desc,
                          app->invalid_reason[id]
                    );
                Edit_SetText(app->desc_edit, desc);
            } else {
                Edit_SetText(app->desc_edit, app->bench[id].desc);
                Edit_SetText(app->error_edit, app->bench[id].cl_code);
            }
        } else {
            Edit_SetText(app->desc_edit, app->bench[id].desc);
        }
    }
        return 0;
    default:
        break;
    }

    return CallWindowProc(app->def_bench_label_proc[id], hWnd, message, wParam, lParam);
}


static int
on_create(HWND w, LPCREATESTRUCT cs)
{
    App *app;
    cl_uint num, num2;
    cl_platform_id *platforms;

    app = new App();

    app->main_win = w;
    app->app = cs->hInstance;

    SetWindowLongPtr(w, GWLP_USERDATA, (LONG_PTR)app);

    clGetPlatformIDs(0, NULL, &num);

    platforms = new cl_platform_id[num];

    clGetPlatformIDs(num, platforms, &num2);

    app->platforms = platforms;
    app->num_platform = num;
    app->bench = clinst_bench_init();

    HFONT fnt = CreateFont(18, 0, 0, 0, FW_DONTCARE,
                           FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS,
                           CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY,
                           DEFAULT_PITCH,
                           "メイリオ");

    {
        app->platform_comb = CreateWindowEx(0, WC_COMBOBOX, TEXT(""),
                                            WS_OVERLAPPED|WS_CHILD|WS_VISIBLE | CBS_DROPDOWN| CBS_HASSTRINGS,
                                            0, 0, 300, 100, w, (HMENU)ID_PLATFORM_SELECT,
                                            cs->hInstance, NULL);
        SetWindowFont(app->platform_comb, fnt, TRUE);

        for (unsigned i=0; i<num; i++) {
            char buffer[1024];
            size_t len;
            clGetPlatformInfo(platforms[i],
                              CL_PLATFORM_NAME,
                              1024,
                              buffer, &len);

            ComboBox_AddString(app->platform_comb, buffer);
        }
    }

    {
        app->device_comb = CreateWindowEx(0, WC_COMBOBOX, TEXT(""),
                                          WS_OVERLAPPED|WS_CHILD|WS_VISIBLE | CBS_DROPDOWN| CBS_HASSTRINGS,
                                          0, 80, 300, 100, w, (HMENU)ID_DEVICE_SELECT,
                                          cs->hInstance, NULL);
        SetWindowFont(app->device_comb, fnt, TRUE);

        ComboBox_Enable(app->device_comb, FALSE);
    }
    {
        int st = WS_OVERLAPPED|WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_WANTRETURN;
        st |= ES_AUTOHSCROLL | ES_AUTOVSCROLL | WS_HSCROLL | WS_VSCROLL;
        app->info_edit = CreateWindowEx(0, WC_EDIT, TEXT(""),
                                        st,
                                        0, 120, 300, 500, w, (HMENU)ID_INFO_EDIT,
                                        cs->hInstance, NULL);
        SetWindowFont(app->info_edit, fnt, TRUE);
    }
    {
        int st = WS_OVERLAPPED|WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_WANTRETURN;
        st |= ES_AUTOVSCROLL | WS_VSCROLL;
        app->desc_edit = CreateWindowEx(0, WC_EDIT, TEXT("せつめー"),
                                        st,
                                        0, 620, 300, 200, w, (HMENU)ID_DESC_EDIT,
                                        cs->hInstance, NULL);
        SetWindowFont(app->desc_edit, fnt, TRUE);
    }
    {
        int st = WS_OVERLAPPED|WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_WANTRETURN;
        st |= ES_AUTOVSCROLL | WS_VSCROLL;

        app->error_edit = CreateWindowEx(0, WC_EDIT, TEXT("エラーログ"),
                                         st,
                                         300, 620, 600, 200, w, (HMENU)ID_DESC_EDIT,
                                         cs->hInstance, NULL);
        SetWindowFont(app->error_edit, fnt, TRUE);

    }

    {
        int x = 320, y = 0;
        HWND *l = app->bench_label;
        HWND *b = app->bench_run_button;
        struct clinst_bench *bench = app->bench;
        for (int i=0; i<BENCH_NUM; i++) {
            int st = WS_CHILD | WS_VISIBLE | WS_OVERLAPPED;

            int lwid = 200;
            int bwid = 80;
            int ht = 60;

            l[i] = CreateWindowEx(0, WC_STATIC, bench[i].name,
                                  st|SS_NOTIFY,
                                  x, y, lwid, ht, w, (HMENU)(ID_BENCH_LABEL + i),
                                  cs->hInstance, NULL);

            b[i] = CreateWindowEx(0, WC_BUTTON, TEXT("run"),
                                  st | WS_DISABLED,
                                  x+lwid, y+ht/2, bwid, ht/2, w, (HMENU)(ID_BENCH_BUTTON + i),
                                  cs->hInstance, NULL);

            SetWindowFont(l[i], fnt, TRUE);
            SetWindowFont(b[i], fnt, TRUE);

            app->def_bench_label_proc[i] = (WNDPROC)GetWindowLongPtr(l[i], GWLP_WNDPROC);
            SetWindowLongPtr(l[i], GWLP_WNDPROC, (LONG_PTR)bench_label_wndproc);
            SetWindowLongPtr(l[i], GWLP_USERDATA, (LONG_PTR)app);

            y += ht;

            if (y > 500) {
                y = 0;
                x += lwid + bwid + 20;
            }
        }
    }

    return 1;
}

static void
int_info(std::ostringstream &oss,
         cl_device_id dev,
         const char *tag,
         int param)
{
    cl_int val;
    size_t sz;
    cl_int r;
    r = clGetDeviceInfo(dev, param, sizeof(val), &val, &sz);

    if (r == CL_SUCCESS) {
        oss << tag << ":" << val << "\r\n";
    }
}

static void
ulong_info(std::ostringstream &oss,
           cl_device_id dev,
           const char *tag,
           int param)
{
    __int64 val;
    size_t sz;
    cl_int r;
    r = clGetDeviceInfo(dev, param, sizeof(val), &val, &sz);

    if (r == CL_SUCCESS) {
        oss << tag << ":" << val << "\r\n";
    }
}

static void
fp_config_info(std::ostringstream &oss,
               cl_device_id dev,
               const char *tag,
               int param)
{
    cl_device_fp_config val;
    size_t sz;
    cl_int r;
    r = clGetDeviceInfo(dev, param, sizeof(val), &val, &sz);

    if (r == CL_SUCCESS) {
        oss << tag << ":" ;

        if (val & CL_FP_DENORM) {
            oss << "denorm,";
        }
        if (val & CL_FP_INF_NAN) {
            oss << "inf/nan,";
        }
        if (val & CL_FP_ROUND_TO_NEAREST) {
            oss << "rte,";
        }
        if (val & CL_FP_ROUND_TO_ZERO) {
            oss << "rtz,";
        }
        if (val & CL_FP_ROUND_TO_INF) {
            oss << "rtpn,";
        }
        if (val & CL_FP_FMA) {
            oss << "fma,";
        }
        if (val & CL_FP_SOFT_FLOAT) {
            oss << "soft,";
        }
#ifdef CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT
        if (val & CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT) {
            oss << "divsqrt,";
        }
#endif

        oss << "\r\n";
    }
}

static void
string_info(std::ostringstream &oss,
            cl_device_id dev,
            const char *tag,
            int param)
{
    char buf[1024];
    size_t sz;
    clGetDeviceInfo(dev, param, sizeof(buf), buf, &sz);

    oss << tag << ":" << buf << "\r\n";
}


static void
select_device(App *app, int dev)
{
    app->cur_device = dev;
    ComboBox_SetCurSel(app->device_comb, dev);

    cl_device_id *devs = app->devices;
    std::ostringstream oss;
    int_info(oss, devs[dev], "max compute units", CL_DEVICE_MAX_COMPUTE_UNITS);
    int_info(oss, devs[dev], "max work item dimensions", CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS);
    int_info(oss, devs[dev], "max work group size", CL_DEVICE_MAX_WORK_GROUP_SIZE);
    //int_array_info(oss, devs[dev], "max work item sizes", CL_DEVICE_MAX_WORK_ITEM_SIZES);
    {
        size_t wsz[3];
        size_t sz;
        clGetDeviceInfo(devs[dev], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(wsz), &wsz, &sz);
        oss << "max work item sizes : " << wsz[0] << ", " << wsz[1] << ", " << wsz[2] << "\r\n";
    }
    int_info(oss, devs[dev], "pref vector width char", CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR);
    int_info(oss, devs[dev], "pref vector width short", CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT);
    int_info(oss, devs[dev], "pref vector width int", CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT);
    int_info(oss, devs[dev], "pref vector width long", CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG);
    int_info(oss, devs[dev], "pref vector width float", CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT);
    int_info(oss, devs[dev], "pref vector width double", CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE);
    int_info(oss, devs[dev], "clock", CL_DEVICE_MAX_CLOCK_FREQUENCY);
    int_info(oss, devs[dev], "address bits", CL_DEVICE_ADDRESS_BITS);
    int_info(oss, devs[dev], "cache type", CL_DEVICE_GLOBAL_MEM_CACHE_TYPE);
    int_info(oss, devs[dev], "cacheline size", CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE);
    ulong_info(oss, devs[dev], "cache size", CL_DEVICE_GLOBAL_MEM_CACHE_SIZE);
    ulong_info(oss, devs[dev], "mem size", CL_DEVICE_GLOBAL_MEM_SIZE);
    ulong_info(oss, devs[dev], "max mem alloc size", CL_DEVICE_MAX_MEM_ALLOC_SIZE);
    int_info(oss, devs[dev], "image", CL_DEVICE_IMAGE_SUPPORT);
    int_info(oss, devs[dev], "wavefront width", CL_DEVICE_WAVEFRONT_WIDTH_AMD);
    int_info(oss, devs[dev], "warp size", CL_DEVICE_WARP_SIZE_NV);
    int_info(oss, devs[dev], "simd per CU", CL_DEVICE_SIMD_PER_COMPUTE_UNIT_AMD);
    int_info(oss, devs[dev], "simd width", CL_DEVICE_SIMD_WIDTH_AMD);
    int_info(oss, devs[dev], "simd instruction width", CL_DEVICE_SIMD_INSTRUCTION_WIDTH_AMD);
    int_info(oss, devs[dev], "host unified memory", CL_DEVICE_HOST_UNIFIED_MEMORY);
    fp_config_info(oss, devs[dev], "float", CL_DEVICE_SINGLE_FP_CONFIG);
    fp_config_info(oss, devs[dev], "double", CL_DEVICE_DOUBLE_FP_CONFIG);
    fp_config_info(oss, devs[dev], "half", CL_DEVICE_HALF_FP_CONFIG);
    string_info(oss, devs[dev], "profile", CL_DEVICE_PROFILE);
    int_info(oss, devs[dev], "local mem type", CL_DEVICE_LOCAL_MEM_TYPE);
    ulong_info(oss, devs[dev], "local mem size", CL_DEVICE_LOCAL_MEM_SIZE);
    string_info(oss, devs[dev], "version", CL_DEVICE_VERSION);
    string_info(oss, devs[dev], "extensions", CL_DEVICE_EXTENSIONS);

    Edit_SetText(app->info_edit, oss.str().c_str());

    if (app->state == App::SELECT_DEVICE) {
        clinst_bench_fini_context(&app->bench_ctxt);
    }

    app->state = App::SELECT_DEVICE;

    clinst_bench_init_context(&app->bench_ctxt, devs[dev]);

    for (int i=0; i<BENCH_NUM; i++) {
        int valid = app->bench[i].is_valid(devs[dev], &app->invalid_reason[i]);
        app->valid[i] = valid;
        EnableWindow(app->bench_run_button[i], valid);
        Edit_SetText(app->bench_label[i], app->bench[i].name);
    }
}

static void
on_command(HWND wnd, int id, HWND com_wnd, UINT code)
{
    App *app = (App*)GetWindowLongPtr(wnd, GWLP_USERDATA);

    switch (id) {
    case IDM_EXIT:
        PostQuitMessage(0);
        break;

    case ID_PLATFORM_SELECT:
        switch (code) {
        case CBN_SELCHANGE: {
            int sel = ComboBox_GetCurSel(app->platform_comb);
            if (sel >= 0) {
                cl_uint num, num2;
                delete [] app->devices;

                app->cur_platform = sel;

                cl_platform_id p = app->platforms[sel];

                clGetDeviceIDs(p, CL_DEVICE_TYPE_ALL, 0, NULL, &num);

                cl_device_id *devs = new cl_device_id[num];

                clGetDeviceIDs(p, CL_DEVICE_TYPE_ALL, num, devs, &num2);

                app->devices = devs;
                app->num_device = num;

                ComboBox_ResetContent(app->device_comb);
                ComboBox_Enable(app->device_comb, TRUE);

                for (unsigned int i=0; i<num; i++) {
                    char buffer[1024];
                    size_t sz;
                    clGetDeviceInfo(devs[i], CL_DEVICE_NAME, 1024, buffer, &sz);
                    ComboBox_AddString(app->device_comb, buffer);
                }

                select_device(app, 0);
            }
        }
            break;
        }
        break;

    case ID_DEVICE_SELECT: 
        switch (code) {
        case CBN_SELCHANGE: {
            int sel = ComboBox_GetCurSel(app->device_comb);
            if (sel >= 0) {
                select_device(app, sel);
            }
        }
            break;

        default:
            break;
        }
        break;

    default:
        if (id >= ID_BENCH_BUTTON && id < (ID_BENCH_BUTTON+BENCH_NUM)) {
            int bench_id = id - ID_BENCH_BUTTON;
            struct bench_result result;
            struct clinst_bench *b = &app->bench[bench_id];
            b->run(&result, &app->bench_ctxt);

            if (result.code != BENCH_OK) {
                Edit_SetText(app->error_edit, result.error_message);
            } else {
                char buffer[4096];
                switch (b->result_type) {
                case RESULT_TYPE_FLOAT:
                    _snprintf(buffer, sizeof(buffer),
                               "%s\r\n%f %s\r\n%s",
                              b->name, result.fval, b->unit_str, result.score);
                    break;
                case RESULT_TYPE_INT:
                    _snprintf(buffer, sizeof(buffer),
                              "%s\r\n%d %s\r\n%s",
                              b->name, result.ival, b->unit_str, result.score);
                    break;

                default:
                    _snprintf(buffer,sizeof(buffer),
                               "%s", b->name);
                    break;
                }

                Static_SetText(app->bench_label[bench_id], buffer);
            }
        }
        break;
    }

}


static LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    case WM_DESTROY: {
        App *app = (App*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        delete app;
    }
        return 0;

        HANDLE_MSG(hWnd, WM_CREATE, on_create);
        HANDLE_MSG(hWnd, WM_COMMAND, on_command);

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

int APIENTRY
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
    HWND main_win;
    {
        WNDCLASSEX wcex;

        wcex.cbSize = sizeof(WNDCLASSEX);

        wcex.style          = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc    = WndProc;
        wcex.cbClsExtra     = 0;
        wcex.cbWndExtra     = 0;
        wcex.hInstance      = hInstance;
        wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
        wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW);
        wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_MAINMENU);
        wcex.lpszClassName  = CLASS_NAME;
        wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

        RegisterClassEx(&wcex);
    }

    InitCommonControls();

    main_win = CreateWindowEx(0,
                              CLASS_NAME,
                              TEXT("hello"),
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, 0,
                              CW_USEDEFAULT, 0,
                              NULL,
                              NULL,
                              hInstance,
                              NULL);

    ShowWindow(main_win, nCmdShow);
    UpdateWindow(main_win);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAINMENU));

    MSG msg;

    while (1) {
        if (!GetMessage(&msg, NULL, 0, 0)) {
            break;
        }

        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}
