LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := clminibench
LOCAL_SRC_FILES := CLlib.c clminibench.cpp bench/bench.c npr/strbuf.c npr/mempool-c.c port.c
LOCAL_LDLIBS    := -llog # -L$(LOCAL_PATH) -lOpenCL

include $(BUILD_SHARED_LIBRARY)
