#
# Copyright (C) 2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit_only.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit from lunaa device
$(call inherit-product, device/realme/lunaa/device.mk)

# Inherit some common Lineage stuff.
$(call inherit-product, vendor/evolution/config/common_full_phone.mk)

#Evolution Flags
EXTRA_UDFPS_ANIMATIONS := true
TARGET_BOOT_ANIMATION_RES := 1080
TARGET_SUPPORTS_QUICK_TAP := true
TARGET_SUPPORTS_GOOGLE_RECORDER := false
TARGET_INCLUDE_LIVE_WALLPAPERS := true
TARGET_USES_OPLUS_CAMERA := true
TARGET_USES_PICO_GAPPS := true

PRODUCT_NAME := evolution_lunaa
PRODUCT_DEVICE := lunaa
PRODUCT_MANUFACTURER := realme
PRODUCT_BRAND := realme
PRODUCT_MODEL := RMX3360

PRODUCT_SYSTEM_NAME := RE54ABL1
PRODUCT_SYSTEM_DEVICE := RE54ABL1

PRODUCT_GMS_CLIENTID_BASE := android-oppo

PRODUCT_BUILD_PROP_OVERRIDES += \
    PRIVATE_BUILD_DESC="RMX3360-user 13 TP1A.220905.001 R.136e3d6-af7c-10d436 release-keys" \
    TARGET_DEVICE=$(PRODUCT_SYSTEM_NAME) \
    TARGET_PRODUCT=$(PRODUCT_SYSTEM_NAME)


BUILD_FINGERPRINT := realme/RMX3360/RE54ABL1:13/TP1A.220905.001/R.136e3d6-af7c-10d436:user/release-keys

PRODUCT_PRODUCT_PROPERTIES += \
    ro.build.fingerprint=$(BUILD_FINGERPRINT)
