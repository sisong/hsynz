//dict_compress_plugin_demo.h
//  dict compress plugin demo for hsync_make
/*
 The MIT License (MIT)
 Copyright (c) 2020-2022 HouSisong
 
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
    hpatch_StreamPos_t result=dataSize+(dataSize>>3)+1024*2;
    assert(result>dataSize);
    return result;
}
#endif

static size_t _getDictBitsByData(size_t bits,size_t kMinBits,hpatch_StreamPos_t dataSize){
    while ((bits>kMinBits)&&(((hpatch_StreamPos_t)1)<<(bits-1)>=dataSize))
        --bits;
    return bits;
}

#define _checkCompress(v)  do { if (!(v)) return kDictCompressError; } while(0)

#ifdef  _CompressPlugin_zlib
#if (_IsNeedIncludeDefaultCompressHead)
#   include "zlib.h" // http://zlib.net/  https://github.com/madler/zlib
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
        hpatch_BOOL         dict_isReset;
        hpatch_BOOL         is_gzip;
    } _TDictCompressPlugin_zlib_data;

    static size_t _zlib_limitDictSizeByData(struct hsync_TDictCompress* compressPlugin,hpatch_StreamPos_t dataSize){
        TDictCompressPlugin_zlib*  plugin=(TDictCompressPlugin_zlib*)compressPlugin;
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
        assert(blockCount>0);
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
                int ret=deflateEnd(&self->stream);
                self->stream.state=0;
                assert(Z_OK==ret);
            }
            free(self);
        }
    }

    static size_t _zlib_writeUncompressedBlock(unsigned char* out_code,unsigned char* out_codeEnd,const hpatch_byte* in_data,
                                               const unsigned char* in_dataEnd,hpatch_BOOL in_isEnd){
        size_t result=0;
        do{
            const size_t kMaxULen=(((size_t)1)<<16)-1;
            size_t ulen=in_dataEnd-in_data;
            _checkCompress(ulen+5<=(size_t)(out_codeEnd-out_code));
            ulen=(ulen<=kMaxULen)?ulen:kMaxULen;
            *out_code++=((in_isEnd?1:0)|((0)<<1))<<5; // blockType
            *out_code++=(hpatch_byte)ulen;
            *out_code++=(hpatch_byte)(ulen>>8);
            *out_code++=(hpatch_byte)(ulen^0xFF);
            *out_code++=(hpatch_byte)((ulen>>8)^0xFF);
            memcpy(out_code,in_data,ulen);
            result+=ulen+5;
            out_code+=ulen;
            in_data+=ulen;
        }while (in_data<in_dataEnd);
        return result;
    }
 
    static hpatch_byte* _zlib_getResetDictBuffer(hsync_dictCompressHandle dictHandle,size_t blockIndex,
                                                 size_t* out_dictSize){
        _TDictCompressPlugin_zlib_data* self=(_TDictCompressPlugin_zlib_data*)dictHandle;
        assert(!self->dict_isReset);
        self->dict_isReset=hpatch_TRUE;
        return _CacheBlockDict_getResetDictBuffer(&self->cache,blockIndex,out_dictSize);
    }

    static size_t _zlib_dictCompress(hsync_dictCompressHandle dictHandle,size_t blockIndex,
                                     hpatch_byte* out_code,hpatch_byte* out_codeEnd,
                                     const hpatch_byte* in_dataBegin,const hpatch_byte* in_dataEnd){
        _TDictCompressPlugin_zlib_data* self=(_TDictCompressPlugin_zlib_data*)dictHandle;
        const size_t dataSize=in_dataEnd-in_dataBegin;
        const hpatch_BOOL in_isEnd=blockIndex+1==self->cache.blockCount;
        size_t result;
        if (self->dict_isReset){ //reset dict
            hpatch_byte* dict;
            size_t       dictSize;
            _CacheBlockDict_usedDict(&self->cache,blockIndex,&dict,&dictSize);
            self->dict_isReset=hpatch_FALSE;
            assert(dictSize==(uInt)dictSize);
            _checkCompress(deflateReset(&self->stream)==Z_OK);
            _checkCompress(deflateSetDictionary(&self->stream,dict,(uInt)dictSize)==Z_OK);
        }
        self->stream.next_in =(Bytef*)in_dataBegin;
        self->stream.avail_in=(uInt)dataSize;
        assert(self->stream.avail_in==dataSize);
        self->stream.next_out =out_code;
        self->stream.avail_out=(uInt)(out_codeEnd-out_code);
        assert(self->stream.avail_out==(size_t)(out_codeEnd-out_code));
        if (!in_isEnd){
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
        }
        result=self->stream.total_out;
        self->stream.total_out=0;
        assert(self->stream.avail_in==0);
        if (result<dataSize)
            return result;
        else if (!self->is_gzip)
            return kDictCompressCancel;// cancel compress
        else
            return _zlib_writeUncompressedBlock(out_code,out_codeEnd,
                                                in_dataBegin,in_dataEnd,in_isEnd);
    }
    
    _def_fun_compressType(_zlib_dictCompressType,"zlibD");
    static const TDictCompressPlugin_zlib zlibDictCompressPlugin={
        {_zlib_dictCompressType,_default_maxCompressedSize,_zlib_limitDictSizeByData,
            _zlib_dictCompressOpen,_zlib_dictCompressClose,0,
            _zlib_getResetDictBuffer,_zlib_dictCompress},
        9,8,MAX_WBITS,hpatch_FALSE};
    static const char* k_gzip_dictCompressType="gzipD";
    _def_fun_compressType(_gzip_dictCompressType,k_gzip_dictCompressType);
    typedef TDictCompressPlugin_zlib TDictCompressPlugin_gzip;
    static const TDictCompressPlugin_gzip gzipDictCompressPlugin={
        {_gzip_dictCompressType,_default_maxCompressedSize,_zlib_limitDictSizeByData,
            _zlib_dictCompressOpen,_zlib_dictCompressClose,0,
            _zlib_getResetDictBuffer,_zlib_dictCompress},
        9,8,MAX_WBITS,hpatch_TRUE};

#endif//_CompressPlugin_zlib


#ifdef  _CompressPlugin_zstd
#if (_IsNeedIncludeDefaultCompressHead)
#define ZSTD_STATIC_LINKING_ONLY
#   include "zstd.h" // "zstd/lib/zstd.h" https://github.com/facebook/zstd
#endif
    struct TDictCompressPlugin_zstd{
        hsync_TDictCompress base;
        hpatch_byte         compress_level; //0..22
        hpatch_byte         dict_bits;  // 10..27
    };
    
    static size_t _zstd_limitDictSizeByData(struct hsync_TDictCompress* compressPlugin,hpatch_StreamPos_t dataSize){
        TDictCompressPlugin_zstd*  plugin=(TDictCompressPlugin_zstd*)compressPlugin;
        size_t dictBits=_getDictBitsByData(plugin->dict_bits,10,dataSize);
        plugin->dict_bits=(hpatch_byte)dictBits;
        return ((size_t)1)<<dictBits;
    }

    static void _zstd_dictCompressClose(const struct hsync_TDictCompress* compressPlugin,
                                        hsync_dictCompressHandle dictHandle){
        ZSTD_CCtx* s=(ZSTD_CCtx*)dictHandle;
        if (s!=0){
            size_t ret=ZSTD_freeCCtx(s);
            assert(ret==0);
        }
    }
    static hsync_dictCompressHandle _zstd_dictCompressOpen(const struct hsync_TDictCompress* compressPlugin){
        const TDictCompressPlugin_zstd*  plugin=(const TDictCompressPlugin_zstd*)compressPlugin;
        size_t     ret;
        ZSTD_CCtx* s=ZSTD_createCCtx();
        if (s==0) return 0; //error
        ret=ZSTD_CCtx_setParameter(s,ZSTD_c_compressionLevel,plugin->compress_level);
        if (ZSTD_isError(ret)) goto _on_error;
        ret=ZSTD_CCtx_setParameter(s,ZSTD_c_windowLog,plugin->dict_bits);
        if (ZSTD_isError(ret)) goto _on_error;
        ret=ZSTD_CCtx_setParameter(s,ZSTD_c_format,ZSTD_f_zstd1_magicless);
        if (ZSTD_isError(ret)) goto _on_error;
        return s;
    _on_error:
        _zstd_dictCompressClose(compressPlugin,s);
        return 0; //error
    }

    #define _zstd_checkComp(v) _checkCompress(!ZSTD_isError(v))

    static size_t _zstd_dictCompress(hsync_dictCompressHandle dictHandle,
                                     unsigned char* out_code,unsigned char* out_codeEnd,
                                     const hpatch_byte* in_dict,const hpatch_byte* in_dictEnd_and_dataBegin,
                                     const unsigned char* in_dataEnd,hpatch_BOOL dict_isReset,hpatch_BOOL in_isEnd){
        ZSTD_CCtx* s=(ZSTD_CCtx*)dictHandle;
        ZSTD_inBuffer       s_input;
        ZSTD_outBuffer      s_output;
        size_t dictSize=in_dictEnd_and_dataBegin-in_dict;
        if ((dictSize>0)){//&&dict_isReset){ //reset dict
            _zstd_checkComp(ZSTD_CCtx_reset(s,ZSTD_reset_session_only));
            _zstd_checkComp(ZSTD_CCtx_refPrefix(s,in_dict,dictSize));
        }

        s_input.src=in_dictEnd_and_dataBegin;
        s_input.size=in_dataEnd-in_dictEnd_and_dataBegin;
        s_input.pos=0;
        s_output.dst=out_code;
        s_output.size=out_codeEnd-out_code;
        s_output.pos=0;
        _zstd_checkComp(ZSTD_compressStream2(s,&s_output,&s_input,in_isEnd?ZSTD_e_end:ZSTD_e_flush));
        assert(s_input.pos==s_input.size);
        if (s_output.pos>=s_input.size)
            return kDictCompressCancel; // cancel compress
        return s_output.pos;
    }
    
    _def_fun_compressType(_zstd_dictCompressType,"zstdD");
    static const TDictCompressPlugin_zstd zstdDictCompressPlugin={
        {_zstd_dictCompressType,_default_maxCompressedSize,_zstd_limitDictSizeByData,
            _zstd_dictCompressOpen,_zstd_dictCompressClose,0,_zstd_dictCompress},
        22,17};
#endif//_CompressPlugin_zstd


#ifdef __cplusplus
}
#endif
#endif
