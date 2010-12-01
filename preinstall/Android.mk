LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_DEVICE),droid2)

# update.zip!
file := $(TARGET_PREINSTALL_OUT)/tanzanite/update-boot.zip
ALL_PREBUILT += $(file)
$(file) : $(LOCAL_PATH)/update-boot.zip | $(ACP)
	$(transform-prebuilt-to-target)
file := $(TARGET_PREINSTALL_OUT)/tanzanite/update-recovery.zip
ALL_PREBUILT += $(file)
$(file) : $(LOCAL_PATH)/update-recovery.zip | $(ACP)
	$(transform-prebuilt-to-target)

# update-binary!
file := $(TARGET_PREINSTALL_OUT)/tanzanite/update-binary
ALL_PREBUILT += $(file)
$(file) : $(TARGET_OUT)/bin/updater | $(ACP)
	$(transform-prebuilt-to-target)

endif
