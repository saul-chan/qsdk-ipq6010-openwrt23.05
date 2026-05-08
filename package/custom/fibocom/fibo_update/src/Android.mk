LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS:= optional

LOCAL_C_INCLUDES = $(wildcard $(LOCAL_PATH)/zte_code/ $(LOCAL_PATH)/qcom_code/ $(LOCAL_PATH)/unisoc_code/ $(LOCAL_PATH)/misc_code/)

SRC_LIST := $(wildcard $(LOCAL_PATH)/zte_code/*.c $(LOCAL_PATH)/qcom_code/*.c $(LOCAL_PATH)/unisoc_code/*.c $(LOCAL_PATH)/misc_code/*.c $(LOCAL_PATH)/*.c)
LOCAL_SRC_FILES:= $(SRC_LIST:$(LOCAL_PATH)/%=%)

LOCAL_CFLAGS += -Wall
# LOCAL_LDFLAGS +=

LOCAL_MODULE:= upgrade_tool


include $(BUILD_EXECUTABLE)
