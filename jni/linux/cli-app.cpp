#include <CL/cl.h>
#include <CL/cl.hpp>
#include <stdio.h>
#include <ostream>
#include <sstream>
#include <iostream>

#include "bench/bench.h"

int main(int argc, char **argv)
{
    std::vector<cl::Platform> platforms;

    cl::Platform::get(&platforms);
    struct clinst_bench *b = clinst_bench_init();


    for (auto && p : platforms) {
        std::vector<cl::Device> devs;

        p.getDevices(CL_DEVICE_TYPE_ALL, &devs);

        for (auto && d : devs) {
            struct clinst_bench_context bench_ctxt;

            auto n = d.getInfo<CL_DEVICE_NAME>();
            std::cout << "dev = " << n << '\n';

            clinst_bench_init_context(&bench_ctxt, d());

            for (int i=0; i<BENCH_NUM; i++) {
                const char *invalid_reason;
                if (b[i].is_valid(d(), &invalid_reason)) {
                    struct bench_result result;

                    b[i].run(&result, &bench_ctxt);

                    if (result.code == BENCH_OK) {
                        switch (b[i].result_type) {
                        case RESULT_TYPE_FLOAT:
                            printf("%s:%f %s\n",
                                   b[i].name, result.fval, b[i].unit_str);
                            break;

                        case RESULT_TYPE_INT:
                            printf("%s:%d %s\n",
                                   b[i].name, result.ival, b[i].unit_str);
                            break;
                        }
                    }
                }
            }

            clinst_bench_fini_context(&bench_ctxt);
        }
    }

    return 0;
}