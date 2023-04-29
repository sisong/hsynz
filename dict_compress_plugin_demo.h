//dict_compress_plugin_demo.h
//  dict compress plugin demo for hsync_make
/*
 The MIT License (MIT)
 Copyright (c) 2020-2023 HouSisong
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef dict_compress_plugin_demo_h
#define dict_compress_plugin_demo_h
//dict compress plugin demo:
//  zlibDictCompressPlugin
//  zstdDictCompressPlugin

#include "HDiffPatch/libhsync/sync_make/dict_compress_plugin.h"
#ifdef __cplusplus
extern "C" {
#endif
#ifndef HDiff_compress_plugin_demo_h
#ifndef _IsNeedIncludeDefaultCompressHead
#   define _IsNeedIncludeDefaultCompressHead 1
#endif

#define _def_fun_compressType(_fun_name,cstr)   \
static const char*  _fun_name(void){            \
    static const char* kCompressType=cstr;      \
    return kCompressType;                       \
}

hpatch_inline static
hpatch_StreamPos_t _default_maxCompressedSize(hpatch_StreamPos_t dataSize){
    hpatch_StreamPos_t result=dataSize+(dataSize>>3)+64;
    assert(result>dataSize);
    return result;
}
#endif

#ifndef IS_REUSE_compress_handle
#   define IS_REUSE_compress_handle 0
#endif

static size_t _default_getBestWorkBlockCount(hsync_TDictCompress* compressPlugin,size_t blockCount,
                                             size_t blockSize,size_t defaultWorkBlockCount){
    return defaultWorkBlockCount;
}

static size_t _getDictBitsByData(size_t bits,size_t kMinBits,hpatch_StreamPos_t dataSize){
    while ((bits>kMinBits)&&(((hpatch_StreamPos_t)1)<<(bits-1)>=dataSize))
        --bits;
    return bits;
}

#define _checkCompress(v)  do { if (!(v)) return kDictCompressError; } while(0)


#ifdef  _CompressPlugin_zlib
#if (_IsNeedIncludeDefaultCompressHead)
#   include "zlib.h" // http://zlib.net/  https://github.com/madler/zlib
#   define _IS_ZLIB_CompressContinueAfterEnd 1
#   if (_IS_ZLIB_CompressContinueAfterEnd)
#       include "deflate.h" // if no "deflate.h" file, define _IS_ZLIB_CompressContinueAfterEnd to 0
#   endif
#endif
    //zlibDictCompressPlugin
    typedef struct{
        hsync_TDictCompress base;
        hpatch_byte         compress_level; // 0..9
        hpatch_byte         mem_level;   // 8..9
        hpatch_byte         dict_bits;   // 9..15
        hpatch_BOOL         is_gzip;
    } TDictCompressPlugin_zlib;
    typedef struct{
        z_stream            stream;
        _CacheBlockDict_t   cache;
        hpatch_BOOL         dict_isInReset;
        hpatch_BOOL         is_gzip;
    } _TDictCompressPlugin_zlib_data;

    static size_t _zlib_getDictSize(struct hsync_TDictCompress* compressPlugin){
        TDictCompressPlugin_zlib*  plugin=(TDictCompressPlugin_zlib*)compressPlugin;
        const size_t dictSize=((size_t)1<<plugin->dict_bits);
        return dictSize;
    }

    static size_t _zlib_limitDictSizeByData(struct hsync_TDictCompress* compressPlugin,size_t blockCount,size_t blockSize){
        TDictCompressPlugin_zlib*  plugin=(TDictCompressPlugin_zlib*)compressPlugin;
        const hpatch_StreamPos_t dataSize=((hpatch_StreamPos_t)blockCount)*blockSize;
        size_t dictBits=_getDictBitsByData(plugin->dict_bits,9,dataSize);
        plugin->dict_bits=(hpatch_byte)dictBits;
        return ((size_t)1)<<dictBits;
    }

    static hsync_dictCompressHandle _zlib_dictCompressOpen(struct hsync_TDictCompress* compressPlugin,size_t blockCount,size_t blockSize){
        const TDictCompressPlugin_zlib*  plugin=(const TDictCompressPlugin_zlib*)compressPlugin;
        const size_t dictSize=((size_t)1<<plugin->dict_bits);
        const size_t cacheDictSize=_getCacheBlockDictSize(dictSize,blockCount,blockSize);
        _TDictCompressPlugin_zlib_data* self=(_TDictCompressPlugin_zlib_data*)malloc(sizeof(_TDictCompressPlugin_zlib_data)+cacheDictSize);
        if (self==0) return 0;
        memset(self,0,sizeof(*self));
        _CacheBlockDict_init(&self->cache,((hpatch_byte*)self)+sizeof(*self),
                             cacheDictSize,dictSize,blockCount,blockSize);
        if (deflateInit2(&self->stream,plugin->compress_level,Z_DEFLATED,
                         -(int)plugin->dict_bits,plugin->mem_level,Z_DEFAULT_STRATEGY)!=Z_OK){
            free(self);
            return 0;// error
        }
        self->is_gzip=plugin->is_gzip;
        return self;
    }
    static void _zlib_dictCompressClose(struct hsync_TDictCompress* compressPlugin,
                                        hsync_dictCompressHandle dictHandle){
        _TDictCompressPlugin_zlib_data* self=(_TDictCompressPlugin_zlib_data*)dictHandle;
        if (self!=0){
            if (self->stream.state!=0){
                int ret;
                #if (_IS_ZLIB_CompressContinueAfterEnd)
                    self->stream.state->status=FINISH_STATE;
                #endif
                ret=deflateEnd(&self->stream);
                self->stream.state=0;
                assert(Z_OK==ret);
            }
            free(self);
        }
    }
 
    static hpatch_byte* _zlib_getResetDictBuffer(hsync_dictCompressHandle dictHandle,size_t blockIndex,
                                                 size_t* out_dictSize){
        _TDictCompressPlugin_zlib_data* self=(_TDictCompressPlugin_zlib_data*)dictHandle;
        assert(!self->dict_isInReset);
        self->dict_isInReset=hpatch_TRUE;
        return _CacheBlockDict_getResetDictBuffer(&self->cache,blockIndex,out_dictSize);
    }

    static size_t _zlib_dictCompress(hsync_dictCompressHandle dictHandle,size_t blockIndex,
                                     hpatch_byte* out_code,hpatch_byte* out_codeEnd,
                                     const hpatch_byte* in_dataBegin,const hpatch_byte* in_dataEnd){
        _TDictCompressPlugin_zlib_data* self=(_TDictCompressPlugin_zlib_data*)dictHandle;
        const size_t dataSize=in_dataEnd-in_dataBegin;
        const hpatch_BOOL in_isEnd=(blockIndex+1==self->cache.blockCount);
        size_t result;
        if (self->dict_isInReset){ //reset dict
            hpatch_byte* dict;
            size_t       dictSize;
            _CacheBlockDict_usedDict(&self->cache,blockIndex,&dict,&dictSize);
            assert(dictSize==(uInt)dictSize);
            _checkCompress(deflateReset(&self->stream)==Z_OK);
            _checkCompress(deflateSetDictionary(&self->stream,dict,(uInt)dictSize)==Z_OK);
            self->dict_isInReset=hpatch_FALSE;
        }
        self->stream.next_in =(Bytef*)in_dataBegin;
        self->stream.avail_in=(uInt)dataSize;
        assert(self->stream.avail_in==dataSize);
        self->stream.next_out =out_code;
        self->stream.avail_out=(uInt)(out_codeEnd-out_code);
        assert(self->stream.avail_out==(size_t)(out_codeEnd-out_code));
        if (self->is_gzip&&(!in_isEnd)){
            int bits; 
            _checkCompress(deflate(&self->stream,Z_BLOCK)==Z_OK);
            // add enough empty blocks to get to a byte boundary
            _checkCompress(deflatePending(&self->stream,Z_NULL,&bits)==Z_OK);
            if (bits & 1){
                _checkCompress(deflate(&self->stream,Z_SYNC_FLUSH)==Z_OK);
            } else if (bits & 7) {
                do { // add static empty blocks
                    _checkCompress(deflatePrime(&self->stream,10,2)==Z_OK);
                    _checkCompress(deflatePending(&self->stream,Z_NULL,&bits)==Z_OK);
                } while (bits & 7);
                _checkCompress(deflate(&self->stream,Z_BLOCK)==Z_OK);
            }
        }else{
            _checkCompress(deflate(&self->stream,Z_FINISH)==Z_STREAM_END);
            if ((!self->is_gzip)&&(!in_isEnd)){
                #if (_IS_ZLIB_CompressContinueAfterEnd)
                    //hack zlib state, compress continue
                    self->stream.state->status=BUSY_STATE;
                #else
                    //update dict & deflateSetDictionary every block (NOTE: deflateSetDictionary slow)
                    _CacheBlockDict_dictUncompress(&self->cache,blockIndex,blockIndex+1,
                                                   in_dataBegin,in_dataEnd);
                    self->dict_isInReset=hpatch_TRUE;
                #endif
            }
        }
        result=self->stream.total_out;
        self->stream.total_out=0;
        assert(self->stream.avail_in==0);
        if ((result>=dataSize)&&(!self->is_gzip))
            return kDictCompressCancel;// cancel compress
        return result;
    }
    
    _def_fun_compressType(_zlib_dictCompressType,"zlibD");
    static const TDictCompressPlugin_zlib zlibDictCompressPlugin={
        {_zlib_dictCompressType,_default_maxCompressedSize,
            _zlib_limitDictSizeByData,_default_getBestWorkBlockCount,
            _zlib_getDictSize,_zlib_dictCompressOpen,_zlib_dictCompressClose,0,
            _zlib_getResetDictBuffer,_zlib_dictCompress},
        9,8,MAX_WBITS,hpatch_FALSE};
    static const char* k_gzip_dictCompressType="gzipD";
    _def_fun_compressType(_gzip_dictCompressType,k_gzip_dictCompressType);
    typedef TDictCompressPlugin_zlib TDictCompressPlugin_gzip;
    static const TDictCompressPlugin_gzip gzipDictCompressPlugin={
        {_gzip_dictCompressType,_default_maxCompressedSize,
            _zlib_limitDictSizeByData,_default_getBestWorkBlockCount,
            _zlib_getDictSize,_zlib_dictCompressOpen,_zlib_dictCompressClose,0,
            _zlib_getResetDictBuffer,_zlib_dictCompress},
        9,8,MAX_WBITS,hpatch_TRUE};

#endif//_CompressPlugin_zlib


#ifdef  _CompressPlugin_zstd
#if (_IsNeedIncludeDefaultCompressHead)
#define ZSTD_STATIC_LINKING_ONLY
#   include "zstd.h" // "zstd/lib/zstd.h" https://github.com/sisong/zstd
#endif
    struct TDictCompressPlugin_zstd{
        hsync_TDictCompress base;
        hpatch_byte         compress_level; //10..22
        hpatch_byte         dict_bits;  // 15..30
    };

    typedef struct{
        ZSTD_CCtx*          s;
        ZSTD_CDict*         d;
        _CacheBlockDict_t   cache;
        hpatch_BOOL         isDeltaDict;
        hpatch_BOOL         dict_isInReset;
        hpatch_byte         compress_level;
    } _TDictCompressPlugin_zstd_data;
    
    static hpatch_BOOL __zstd_isCanDeltaDict(size_t dictSize,size_t kBlockCount,size_t kBlockSize,size_t* out_deltaSize){
        const size_t __zstd_kMinCacheBlock=2;
        const size_t backDictSize=dictSize;
        if (kBlockCount<=1)
            return hpatch_FALSE; //not need cache
        size_t _deltaSize=dictSize/_kMinCacheBlockDictSize_r;
        size_t deltaCount=_deltaSize/kBlockSize;
        if (deltaCount>kBlockCount)
            deltaCount=kBlockCount;
        *out_deltaSize=deltaCount*kBlockSize;
        return deltaCount>=__zstd_kMinCacheBlock;
    }

    static size_t _zstd_getDictSize(struct hsync_TDictCompress* compressPlugin){
        TDictCompressPlugin_zstd*  plugin=(TDictCompressPlugin_zstd*)compressPlugin;
        const size_t dictSize=((size_t)1<<plugin->dict_bits);
        return dictSize;
    }

    static size_t _zstd_limitDictSizeByData(struct hsync_TDictCompress* compressPlugin,size_t blockCount,size_t blockSize){
        TDictCompressPlugin_zstd*  plugin=(TDictCompressPlugin_zstd*)compressPlugin;
        const hpatch_StreamPos_t dataSize=((hpatch_StreamPos_t)blockCount)*blockSize;
        size_t dictBits=_getDictBitsByData(plugin->dict_bits,10,dataSize);
        plugin->dict_bits=(hpatch_byte)dictBits;
        return ((size_t)1)<<dictBits;
    }

    static size_t _zstd_getBestWorkBlockCount(hsync_TDictCompress* compressPlugin,size_t blockCount,
                                              size_t blockSize,size_t workBlockCount){
        #define _kMinWorkBlockDictSize_r _kMinCacheBlockDictSize_r  // dictSize/_kMinWorkBlockDictSize_r
        TDictCompressPlugin_zstd*  plugin=(TDictCompressPlugin_zstd*)compressPlugin;
        const size_t dictSize=((size_t)1)<<plugin->dict_bits;
        const size_t minWorkBlockCount=(dictSize/_kMinWorkBlockDictSize_r+blockSize-1)/blockSize;
        workBlockCount=(workBlockCount>=minWorkBlockCount)?workBlockCount:minWorkBlockCount;
        size_t deltaSize;
        const hpatch_BOOL isDeltaDict=__zstd_isCanDeltaDict(dictSize,blockCount,blockSize,&deltaSize);
        if (isDeltaDict){
            const size_t deltaCount=deltaSize/blockSize;
            workBlockCount=(workBlockCount+deltaCount-1)/deltaCount*deltaCount;
        }
        return workBlockCount;
    }

    
    static hpatch_inline void __zstd_clear_cd(_TDictCompressPlugin_zstd_data* self){
        ZSTD_CDict* d=self->d;
        self->d=0;
        if (d!=0){
            size_t ret=ZSTD_freeCDict(d);
            assert(ret==0);
        }
    }

    static void _zstd_dictCompressClose(struct hsync_TDictCompress* compressPlugin,
                                        hsync_dictCompressHandle dictHandle){
        _TDictCompressPlugin_zstd_data* self=(_TDictCompressPlugin_zstd_data*)dictHandle;
        if (self!=0){
            ZSTD_CCtx*  s=self->s;
            __zstd_clear_cd(self);
            self->s=0;
            if (s!=0){
#if (IS_REUSE_compress_handle)
                size_t ret=ZSTD_CCtx_reset(s,ZSTD_reset_parameters);
                assert(ret==0);
#else
                size_t ret=ZSTD_freeCCtx(s);
                assert(ret==0);
#endif
            }
            free(self);
        }
    }

    #define _zstd_checkOpen(v) do { if (ZSTD_isError(v)) goto _on_error; } while(0)

    static hsync_dictCompressHandle _zstd_dictCompressOpen(struct hsync_TDictCompress* compressPlugin,size_t blockCount,size_t blockSize){
        const TDictCompressPlugin_zstd*  plugin=(const TDictCompressPlugin_zstd*)compressPlugin;
        const size_t dictSize=((size_t)1<<plugin->dict_bits);
        const size_t cacheDictSize=_getCacheBlockDictSize(dictSize,blockCount,blockSize);
        size_t deltaSize;
        const hpatch_BOOL isDeltaDict=__zstd_isCanDeltaDict(dictSize,blockCount,blockSize,&deltaSize);
        _TDictCompressPlugin_zstd_data* self=(_TDictCompressPlugin_zstd_data*)
                            malloc(sizeof(_TDictCompressPlugin_zstd_data)+(isDeltaDict?dictSize:cacheDictSize));
        if (self==0) return 0;
        memset(self,0,sizeof(*self));
        self->isDeltaDict=isDeltaDict;
        self->compress_level=plugin->compress_level;
        _CacheBlockDict_init(&self->cache,((hpatch_byte*)self)+sizeof(*self),
                             (isDeltaDict?dictSize:cacheDictSize),(isDeltaDict?dictSize-deltaSize:dictSize),
                             blockCount,blockSize);
#if (IS_REUSE_compress_handle)
        static ZSTD_CCtx*   _s=0;
        if (_s==0) _s=ZSTD_createCCtx();
        self->s=_s;
#else
        self->s=ZSTD_createCCtx();
#endif
        if (self->s==0) goto _on_error;
        _zstd_checkOpen(ZSTD_CCtx_setParameter(self->s,ZSTD_c_compressionLevel,plugin->compress_level));
        _zstd_checkOpen(ZSTD_CCtx_setParameter(self->s,ZSTD_c_windowLog,(int)_getDictBitsByData(plugin->dict_bits,10,blockSize)));
        _zstd_checkOpen(ZSTD_CCtx_setParameter(self->s,ZSTD_c_format,ZSTD_f_zstd1_magicless));
        return self;
    _on_error:
        _zstd_dictCompressClose(compressPlugin,self);
        return 0; //error
    }

    static hpatch_byte* _zstd_getResetDictBuffer(hsync_dictCompressHandle dictHandle,size_t blockIndex,
                                                 size_t* out_dictSize){
        _TDictCompressPlugin_zstd_data* self=(_TDictCompressPlugin_zstd_data*)dictHandle;
        assert(!self->dict_isInReset);
        self->dict_isInReset=hpatch_TRUE;
        return _CacheBlockDict_getResetDictBuffer(&self->cache,blockIndex,out_dictSize);
    }

    #define _zstd_checkComp(v) _checkCompress(!ZSTD_isError(v))

    static void __zstd_reCreateCDict(_TDictCompressPlugin_zstd_data* self){
        const size_t kMaxDictSize=self->cache.uncompressEnd-self->cache.uncompress;
        const size_t dataSize=self->cache.uncompressCur-self->cache.uncompress;
        assert(self->isDeltaDict);
        __zstd_clear_cd(self);
        self->d=ZSTD_createCDict_delta(self->cache.uncompress,kMaxDictSize,
                                       kMaxDictSize-dataSize,self->compress_level,0);
    }

    static size_t _zstd_dictCompress(hsync_dictCompressHandle dictHandle,size_t blockIndex,
                                     hpatch_byte* out_code,hpatch_byte* out_codeEnd,
                                     const hpatch_byte* in_dataBegin,const hpatch_byte* in_dataEnd){
        _TDictCompressPlugin_zstd_data* self=(_TDictCompressPlugin_zstd_data*)dictHandle;
        ZSTD_CCtx* s=self->s;
        ZSTD_inBuffer       s_input;
        ZSTD_outBuffer      s_output;
        const size_t dataSize=in_dataEnd-in_dataBegin;
        const hpatch_BOOL in_isEnd=blockIndex+1==self->cache.blockCount;
        if (_CacheBlockDict_isHaveDict(&self->cache)){ //reset dict
            _zstd_checkComp(ZSTD_CCtx_reset(s,ZSTD_reset_session_only));
            if (self->isDeltaDict){
                if (self->dict_isInReset)
                    __zstd_reCreateCDict(self);
                _zstd_checkComp(ZSTD_CCtx_refCDict(s,self->d));
            }else{
                hpatch_byte* dict;
                size_t       dictSize;
                _CacheBlockDict_usedDict(&self->cache,blockIndex,&dict,&dictSize);
                _zstd_checkComp(ZSTD_CCtx_refPrefix(s,dict,dictSize));
            }
            self->dict_isInReset=hpatch_FALSE;
        }

        s_input.src=in_dataBegin;
        s_input.size=dataSize;
        s_input.pos=0;
        s_output.dst=out_code;
        s_output.size=out_codeEnd-out_code;
        s_output.pos=0;
        _zstd_checkComp(ZSTD_compressStream2(s,&s_output,&s_input,in_isEnd?ZSTD_e_end:ZSTD_e_flush));
        assert(s_input.pos==s_input.size);
        if (!in_isEnd){//update cache & dict
            const hpatch_byte* lastUpdated=self->cache.uncompressCur;
            _CacheBlockDict_dictUncompress(&self->cache,blockIndex,blockIndex+1,in_dataBegin,in_dataEnd);
            if (self->isDeltaDict){
                if ((self->d!=0)&&(lastUpdated+dataSize==self->cache.uncompressCur)){
                    _zstd_checkComp(ZSTD_updateCDict_delta(self->d,dataSize));
                }else{
                    __zstd_reCreateCDict(self);
                    _checkCompress(self->d!=0);
                }
            }
        }
        if (s_output.pos>=s_input.size)
            return kDictCompressCancel; // cancel compress
        return s_output.pos;
    }
    
    _def_fun_compressType(_zstd_dictCompressType,"zstdD");
    static const TDictCompressPlugin_zstd zstdDictCompressPlugin={
        {_zstd_dictCompressType,_default_maxCompressedSize,
            _zstd_limitDictSizeByData,_zstd_getBestWorkBlockCount,
            _zstd_getDictSize,_zstd_dictCompressOpen,_zstd_dictCompressClose,0,
            _zstd_getResetDictBuffer,_zstd_dictCompress},
        21,24};
#endif//_CompressPlugin_zstd


#ifdef __cplusplus
}
#endif
#endif
