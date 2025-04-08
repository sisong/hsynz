# args
DIR_DIFF := 1
MT       := 1
ZLIB     := 1
LDEF     := 1
ZSTD     := 1
HTTPS    := 1

STATIC_CPP := 0
# used clang?
CL  	 := 0
# build with -m32?
M32      := 0
# build for out min size
MINS     := 0

HDP_OBJ := \
    HDiffPatch/file_for_patch.o \
    HDiffPatch/libHDiffPatch/HPatch/patch.o \
    HDiffPatch/libHDiffPatch/HDiff/private_diff/limit_mem_diff/adler_roll.o \
    HDiffPatch/dirDiffPatch/dir_diff/dir_diff_tools.o
ifeq ($(DIR_DIFF),0)
else
  HDP_OBJ += \
    HDiffPatch/dirDiffPatch/dir_patch/dir_patch_tools.o \
    HDiffPatch/dirDiffPatch/dir_patch/res_handle_limit.o \
    HDiffPatch/dirDiffPatch/dir_patch/ref_stream.o \
    HDiffPatch/dirDiffPatch/dir_patch/new_stream.o \
    HDiffPatch/dirDiffPatch/dir_patch/new_dir_output.o \
    HDiffPatch/dirDiffPatch/dir_diff/dir_manifest.o
endif
ifeq ($(MT),0)
else
  HDP_OBJ += \
    HDiffPatch/libParallel/parallel_import.o \
    HDiffPatch/libParallel/parallel_channel.o
endif

CLIENT_OBJ := \
    HDiffPatch/libhsync/sync_client/sync_client.o \
    HDiffPatch/libhsync/sync_client/sync_info_client.o \
    HDiffPatch/libhsync/sync_client/sync_diff_data.o \
    HDiffPatch/libhsync/sync_client/match_in_old.o \
    $(HDP_OBJ)
ifeq ($(DIR_DIFF),0)
else
  CLIENT_OBJ += \
    HDiffPatch/libhsync/sync_client/dir_sync_client.o
endif
MAKE_OBJ := \
    HDiffPatch/libhsync/sync_make/sync_make.o \
    HDiffPatch/libhsync/sync_make/sync_info_make.o \
    HDiffPatch/libhsync/sync_make/match_in_new.o
ifeq ($(DIR_DIFF),0)
else
  MAKE_OBJ += \
    HDiffPatch/libhsync/sync_make/dir_sync_make.o
endif

LDEF_PATH := libdeflate
ifeq ($(LDEF),0)
else
  # https://github.com/sisong/libdeflate
  LDEF_OBJ := $(LDEF_PATH)/lib/crc32.o \
  				$(LDEF_PATH)/lib/deflate_decompress.o \
  				$(LDEF_PATH)/lib/utils.o \
  				$(LDEF_PATH)/lib/x86/cpu_features.o \
  				$(LDEF_PATH)/lib/arm/cpu_features.o

  CLIENT_OBJ += $(LDEF_OBJ)
  MAKE_OBJ   += $(LDEF_PATH)/lib/deflate_compress.o
endif

ZLIB_PATH := zlib
ifeq ($(ZLIB),0)
else
  # http://zlib.net  https://github.com/sisong/zlib  
  _ZLIB_OBJ := $(ZLIB_PATH)/adler32.o \
  				$(ZLIB_PATH)/crc32.o \
  				$(ZLIB_PATH)/inffast.o \
  				$(ZLIB_PATH)/inflate.o \
  				$(ZLIB_PATH)/inftrees.o \
  				$(ZLIB_PATH)/trees.o \
  				$(ZLIB_PATH)/zutil.o
  ifeq ($(LDEF),0)
    CLIENT_OBJ += $(_ZLIB_OBJ)
  else
    MAKE_OBJ   += $(_ZLIB_OBJ)
  endif
  MAKE_OBJ   += $(ZLIB_PATH)/deflate.o 
endif

ZSTD_PATH := zstd/lib
ifeq ($(ZSTD),0)
else # https://github.com/sisong/zstd
  CLIENT_OBJ += $(ZSTD_PATH)/common/debug.o \
  				$(ZSTD_PATH)/common/entropy_common.o \
  				$(ZSTD_PATH)/common/error_private.o \
  				$(ZSTD_PATH)/common/fse_decompress.o \
  				$(ZSTD_PATH)/common/xxhash.o \
  				$(ZSTD_PATH)/common/zstd_common.o \
  				$(ZSTD_PATH)/decompress/huf_decompress.o \
  				$(ZSTD_PATH)/decompress/zstd_ddict.o \
  				$(ZSTD_PATH)/decompress/zstd_decompress.o \
  				$(ZSTD_PATH)/decompress/zstd_decompress_block.o
  MAKE_OBJ   += $(ZSTD_PATH)/compress/fse_compress.o \
  				$(ZSTD_PATH)/compress/hist.o \
  				$(ZSTD_PATH)/compress/huf_compress.o \
  				$(ZSTD_PATH)/compress/zstd_compress.o \
  				$(ZSTD_PATH)/compress/zstd_compress_literals.o \
  				$(ZSTD_PATH)/compress/zstd_compress_sequences.o \
  				$(ZSTD_PATH)/compress/zstd_compress_superblock.o \
  				$(ZSTD_PATH)/compress/zstd_preSplit.o \
  				$(ZSTD_PATH)/compress/zstd_double_fast.o \
  				$(ZSTD_PATH)/compress/zstd_fast.o \
  				$(ZSTD_PATH)/compress/zstd_lazy.o \
  				$(ZSTD_PATH)/compress/zstd_ldm.o \
  				$(ZSTD_PATH)/compress/zstd_opt.o
endif

# https://github.com/sisong/minihttp
HTTP_PATH := minihttp
HTTP_OBJ := $(HTTP_PATH)/minihttp.o
ifeq ($(HTTPS),0)
else
    HTTPS_C:= $(HTTP_PATH)/mbedtls/library
	CLIENT_OBJ+=$(HTTPS_C)/md5.o $(HTTPS_C)/sha256.o $(HTTPS_C)/sha512.o 
    HTTP_OBJ += $(HTTPS_C)/aes.o $(HTTPS_C)/aesni.o $(HTTPS_C)/arc4.o \
				$(HTTPS_C)/asn1parse.o $(HTTPS_C)/asn1write.o $(HTTPS_C)/base64.o \
				$(HTTPS_C)/bignum.o $(HTTPS_C)/blowfish.o $(HTTPS_C)/camellia.o \
				$(HTTPS_C)/ccm.o $(HTTPS_C)/certs.o $(HTTPS_C)/cipher.o \
				$(HTTPS_C)/cipher_wrap.o $(HTTPS_C)/cmac.o $(HTTPS_C)/ctr_drbg.o \
				$(HTTPS_C)/debug.o $(HTTPS_C)/des.o $(HTTPS_C)/dhm.o \
				$(HTTPS_C)/ecdh.o $(HTTPS_C)/ecdsa.o $(HTTPS_C)/ecjpake.o \
				$(HTTPS_C)/ecp.o $(HTTPS_C)/ecp_curves.o $(HTTPS_C)/entropy.o \
				$(HTTPS_C)/entropy_poll.o $(HTTPS_C)/error.o $(HTTPS_C)/gcm.o \
				$(HTTPS_C)/havege.o $(HTTPS_C)/hmac_drbg.o $(HTTPS_C)/md.o \
				$(HTTPS_C)/md_wrap.o $(HTTPS_C)/md2.o $(HTTPS_C)/md4.o \
				$(HTTPS_C)/memory_buffer_alloc.o $(HTTPS_C)/net_sockets.o \
				$(HTTPS_C)/oid.o $(HTTPS_C)/padlock.o $(HTTPS_C)/pem.o \
				$(HTTPS_C)/pk.o $(HTTPS_C)/pk_wrap.o $(HTTPS_C)/pkcs5.o \
				$(HTTPS_C)/pkcs11.o $(HTTPS_C)/pkcs12.o $(HTTPS_C)/pkparse.o \
				$(HTTPS_C)/pkwrite.o $(HTTPS_C)/platform.o $(HTTPS_C)/ripemd160.o \
				$(HTTPS_C)/rsa.o $(HTTPS_C)/rsa_internal.o $(HTTPS_C)/sha1.o \
				$(HTTPS_C)/ssl_cache.o \
				$(HTTPS_C)/ssl_ciphersuites.o $(HTTPS_C)/ssl_cli.o $(HTTPS_C)/ssl_cookie.o \
				$(HTTPS_C)/ssl_srv.o $(HTTPS_C)/ssl_ticket.o $(HTTPS_C)/ssl_tls.o \
				$(HTTPS_C)/threading.o $(HTTPS_C)/timing.o $(HTTPS_C)/version.o \
				$(HTTPS_C)/version_features.o $(HTTPS_C)/x509.o $(HTTPS_C)/x509_create.o \
				$(HTTPS_C)/x509_crl.o $(HTTPS_C)/x509_crt.o $(HTTPS_C)/x509_csr.o \
				$(HTTPS_C)/x509write_crt.o $(HTTPS_C)/x509write_csr.o $(HTTPS_C)/xtea.o
endif

MAKE_OBJ += $(CLIENT_OBJ)

DEF_FLAGS := \
    -O3 -DNDEBUG -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 \
    -D_IS_NEED_DEFAULT_CompressPlugin=0 \
    -D_IS_NEED_DEFAULT_ChecksumPlugin=0 \
    -D_ChecksumPlugin_xxh128 -I'xxHash'
ifeq ($(DIR_DIFF),0)
  DEF_FLAGS += -D_IS_NEED_DIR_DIFF_PATCH=0
else
  DEF_FLAGS += -D_IS_NEED_DIR_DIFF_PATCH=1
endif
ifeq ($(MT),0)
  DEF_FLAGS += \
    -D_IS_USED_MULTITHREAD=0 
else
  DEF_FLAGS += \
    -D_IS_USED_MULTITHREAD=1 \
    -D_IS_USED_PTHREAD=1
endif

ifeq ($(ZLIB),0)
else
  DEF_FLAGS += -D_CompressPlugin_zlib -I$(ZLIB_PATH)
  DEF_FLAGS += -D_ChecksumPlugin_crc32
endif
ifeq ($(LDEF),0)
else
  DEF_FLAGS += -D_CompressPlugin_ldef -I$(LDEF_PATH)
  ifeq ($(ZLIB),0)
    DEF_FLAGS += -D_ChecksumPlugin_crc32
  endif
endif
ifeq ($(ZSTD),0)
else
    DEF_FLAGS += -D_CompressPlugin_zstd
    DEF_FLAGS += -DZSTD_HAVE_WEAK_SYMBOLS=0 -DZSTD_TRACE=0 -DZSTD_DISABLE_ASM=1 -DZSTDLIB_VISIBLE= -DZSTDLIB_HIDDEN= \
	    -I$(ZSTD_PATH) -I$(ZSTD_PATH)/common -I$(ZSTD_PATH)/compress -I$(ZSTD_PATH)/decompress
endif
DEF_FLAGS += -I$(HTTP_PATH)
ifeq ($(HTTPS),0)
else
  DEF_FLAGS += -I$(HTTP_PATH)/mbedtls/include -DMINIHTTP_USE_MBEDTLS \
      -D_ChecksumPlugin_mbedtls_md5 -D_ChecksumPlugin_mbedtls_sha256 -D_ChecksumPlugin_mbedtls_sha512
endif

ifeq ($(M32),0)
else
  DEF_FLAGS += -m32
endif
ifeq ($(MINS),0)
else
  DEF_FLAGS += \
    -s \
    -Wno-error=format-security \
    -fvisibility=hidden  \
    -ffunction-sections -fdata-sections \
    -ffat-lto-objects -flto
  CXXFLAGS += -fvisibility-inlines-hidden
endif
CFLAGS   += $(DEF_FLAGS) 
CXXFLAGS += $(DEF_FLAGS) -std=c++11

DIFF_LINK :=
ifeq ($(MT),0)
else
  DIFF_LINK += -lpthread
endif

ifeq ($(M32),0)
else
  DIFF_LINK += -m32
endif
ifeq ($(MINS),0)
else
  DIFF_LINK += -Wl,--gc-sections,--as-needed
endif
ifeq ($(CL),1)
  CXX := clang++
  CC  := clang
endif
ifeq ($(STATIC_CPP),0)
  DIFF_LINK += -lstdc++
else
  DIFF_LINK += -static-libstdc++
endif

.PHONY: all clean

all: libhsync.a hsync_demo hsync_make hsync_http


libhsync.a: $(MAKE_OBJ) $(HTTP_OBJ) 
	$(AR) rcs $@ $^

hsync_demo: libhsync.a
	$(CXX)	hsync_demo.cpp client_download_demo.cpp \
		$(CLIENT_OBJ) $(CXXFLAGS) $(DIFF_LINK) -o hsync_demo
hsync_make: libhsync.a
	$(CXX)	hsync_make.cpp client_download_demo.cpp hsync_import_patch.cpp \
		$(MAKE_OBJ) $(CXXFLAGS) $(DIFF_LINK) -o hsync_make
hsync_http: libhsync.a
	$(CXX)	hsync_http.cpp client_download_http.cpp \
		$(CLIENT_OBJ) $(HTTP_OBJ) $(CXXFLAGS) $(DIFF_LINK) -o hsync_http


RM := rm -f
clean:
	$(RM)	libhsync.a hsync_make hsync_demo hsync_http \
		client_download_demo.o hsync_import_patch.o client_download_http.o \
		$(MAKE_OBJ) $(HTTP_OBJ)
