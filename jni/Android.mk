LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := clminibench
LOCAL_SRC_FILES := clminibench.cpp bench/bench.c npr/strbuf.c npr/mempool-c.c port.c
LOCAL_LDLIBS    := -llog -lGLESv2 -lGLES_mali

include $(BUILD_SHARED_LIBRARY)
