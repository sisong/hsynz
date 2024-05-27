LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := hsynz

MT    := 1
ZSTD  := 1
LDEF  := 1
MD5   := 0
SHA256:= 0
SHA512:= 0

jni_PATH:= $(LOCAL_PATH)

ZSTD_PATH  := $(LOCAL_PATH)/../../zstd/lib
ifeq ($(ZSTD),0)
Zstd_Files :=
else
Zstd_Files := $(ZSTD_PATH)/decompress/zstd_decompress.c \
              $(ZSTD_PATH)/decompress/zstd_decompress_block.c \
              $(ZSTD_PATH)/decompress/zstd_ddict.c \
              $(ZSTD_PATH)/decompress/huf_decompress.c \
              $(ZSTD_PATH)/common/zstd_common.c \
              $(ZSTD_PATH)/common/entropy_common.c \
              $(ZSTD_PATH)/common/fse_decompress.c \
              $(ZSTD_PATH)/common/xxhash.c \
              $(ZSTD_PATH)/common/error_private.c
endif

LDEF_PATH := $(LOCAL_PATH)/../../libdeflate
ifeq ($(LDEF),0)
Ldef_Files :=
else
Ldef_Files := $(LDEF_PATH)/lib/crc32.c \
              $(LDEF_PATH)/lib/deflate_decompress.c \
              $(LDEF_PATH)/lib/utils.c \
              $(LDEF_PATH)/lib/x86/cpu_features.c \
              $(LDEF_PATH)/lib/arm/cpu_features.c
endif

HDP_PATH  := $(LOCAL_PATH)/../../HDiffPatch
Hdp_Files := $(HDP_PATH)/file_for_patch.c \
             $(HDP_PATH)/libHDiffPatch/HPatch/patch.c \
             $(HDP_PATH)/libHDiffPatch/HDiff/private_diff/limit_mem_diff/adler_roll.c \
             $(HDP_PATH)/dirDiffPatch/dir_diff/dir_diff_tools.cpp
ifeq ($(MT),0)
else
Hdp_Files += $(HDP_PATH)/libParallel/parallel_import.cpp \
             $(HDP_PATH)/libParallel/parallel_channel.cpp
endif

SYNC_PATH  := $(HDP_PATH)
Sync_Files := $(SYNC_PATH)/libhsync/sync_client/sync_client.cpp \
			  $(SYNC_PATH)/libhsync/sync_client/sync_info_client.cpp \
			  $(SYNC_PATH)/libhsync/sync_client/sync_diff_data.cpp \
			  $(SYNC_PATH)/libhsync/sync_client/match_in_old.cpp

HTTPS_C := $(LOCAL_PATH)/../../minihttp/mbedtls/library
HTTPS_H := $(LOCAL_PATH)/../../minihttp/mbedtls/include
ifeq ($(MD5),0)
else
	Sync_Files += $(HTTPS_C)/md5.c
endif
ifeq ($(SHA256),0)
else
	Sync_Files += $(HTTPS_C)/sha256.c
endif
ifeq ($(SHA512),0)
else
	Sync_Files += $(HTTPS_C)/sha512.c
endif

Src_Files := $(jni_PATH)/hsynz_export.cpp \
             $(jni_PATH)/hsynz_jni.cpp

LOCAL_SRC_FILES  := $(Src_Files) $(Sync_Files) $(Hdp_Files) $(Zstd_Files) $(Ldef_Files)

LOCAL_LDLIBS     := -llog
ifeq ($(LDEF),0)
	LOCAL_LDLIBS += -lz
else
endif
LOCAL_CFLAGS     := -Os -DANDROID_NDK -DNDEBUG -D_LARGEFILE_SOURCE \
					-D_IS_NEED_DIR_DIFF_PATCH=0 \
					-D_IS_NEED_DEFAULT_CompressPlugin=0 \
					-D_IS_NEED_DEFAULT_ChecksumPlugin=0 \
					-D_ChecksumPlugin_xxh128 -I'../../xxHash'
ifeq ($(MT),0)
  LOCAL_CFLAGS   += -D_IS_USED_MULTITHREAD=0
else
  LOCAL_CFLAGS   += -D_IS_USED_MULTITHREAD=1 -D_IS_USED_PTHREAD=1
endif
ifeq ($(LDEF),0)
	LOCAL_CFLAGS     += -D_CompressPlugin_zlib
else
	LOCAL_CFLAGS     += -D_CompressPlugin_ldef -I$(LDEF_PATH) -DHAVE_PMULL_INTRIN=0 -DLIBDEFLATEAPI=
endif

ifeq ($(ZSTD),0)
else
LOCAL_CFLAGS     += -D_CompressPlugin_zstd -I$(ZSTD_PATH) -I$(ZSTD_PATH)/common -I$(ZSTD_PATH)/decompress
LOCAL_CFLAGS     += -DZSTD_HAVE_WEAK_SYMBOLS=0 -DZSTD_TRACE=0 -DZSTD_DISABLE_ASM=1 -DZSTDLIB_VISIBLE= -DZSTDLIB_HIDDEN= \
					-DDYNAMIC_BMI2=0 -DZSTD_LEGACY_SUPPORT=0 -DZSTD_LIB_DEPRECATED=0 -DHUF_FORCE_DECOMPRESS_X1=1 \
					-DZSTD_FORCE_DECOMPRESS_SEQUENCES_SHORT=1 -DZSTD_NO_INLINE=1 -DZSTD_STRIP_ERROR_STRINGS=1 
endif
ifeq ($(MD5),0)
else
	LOCAL_CFLAGS += -D_ChecksumPlugin_mbedtls_md5 -I$(HTTPS_H)
endif
ifeq ($(SHA256),0)
else
	LOCAL_CFLAGS += -D_ChecksumPlugin_mbedtls_sha256 -I$(HTTPS_H)
endif
ifeq ($(SHA512),0)
else
	LOCAL_CFLAGS += -D_ChecksumPlugin_mbedtls_sha512 -I$(HTTPS_H)
endif

include $(BUILD_SHARED_LIBRARY)

