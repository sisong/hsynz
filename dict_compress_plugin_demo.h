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
    } TDictCompressPlugin_zlib;
    typedef struct{
        z_stream            stream;
    } _TDictCompressPlugin_zlib_data;

    static size_t _zlib_getDictSizeByData(struct hsync_TDictCompress* dictCompressPlugin,hpatch_StreamPos_t dataSize){
        TDictCompressPlugin_zlib*  plugin=(TDictCompressPlugin_zlib*)dictCompressPlugin;
        size_t dictBits=_getDictBitsByData(plugin->dict_bits,9,dataSize);
        plugin->dict_bits=(hpatch_byte)dictBits;
        return ((size_t)1)<<dictBits;
    }

    static hsync_dictCompressHandle _zlib_dictCompressOpen(const struct hsync_TDictCompress* dictCompressPlugin){
        const TDictCompressPlugin_zlib*  plugin=(const TDictCompressPlugin_zlib*)dictCompressPlugin;
        _TDictCompressPlugin_zlib_data* self=(_TDictCompressPlugin_zlib_data*)malloc(sizeof(_TDictCompressPlugin_zlib_data));
        if (self==0) return 0;
        memset(self,0,sizeof(*self));
        if (deflateInit2(&self->stream,plugin->compress_level,Z_DEFLATED,
                         -(int)plugin->dict_bits,plugin->mem_level,Z_DEFAULT_STRATEGY)!=Z_OK){
            free(self);
            return 0;// error
        }
        return self;
    }
    static void _zlib_dictCompressClose(const struct hsync_TDictCompress* dictCompressPlugin,
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
    static size_t _zlib_dictCompress(hsync_dictCompressHandle dictHandle,
                                     unsigned char* out_code,unsigned char* out_codeEnd,
                                     const hpatch_byte* in_dict,const hpatch_byte* in_dictEnd_and_dataBegin,
                                     const unsigned char* in_dataEnd,hpatch_BOOL dict_isReset,hpatch_BOOL in_isEnd){
        _TDictCompressPlugin_zlib_data* self=(_TDictCompressPlugin_zlib_data*)dictHandle;
        int z_ret;
        size_t result;
        size_t dictSize=in_dictEnd_and_dataBegin-in_dict;
        if ((dictSize>0)&&dict_isReset){ //reset dict
            if (deflateReset(&self->stream)!=Z_OK)
                return 0;
            assert(dictSize==(uInt)dictSize);
            z_ret=deflateSetDictionary(&self->stream,in_dict,(uInt)dictSize);
            if (z_ret!=Z_OK)
                return 0; //error
        }
        self->stream.next_in =(Bytef*)in_dictEnd_and_dataBegin;
        self->stream.avail_in=(uInt)(in_dataEnd-in_dictEnd_and_dataBegin);
        assert(self->stream.avail_in==(size_t)(in_dataEnd-in_dictEnd_and_dataBegin));
        self->stream.next_out =out_code;
        self->stream.avail_out=(uInt)(out_codeEnd-out_code);
        assert(self->stream.avail_out==(size_t)(out_codeEnd-out_code));
        if (!in_isEnd){
            int bits; 
            z_ret=deflate(&self->stream,Z_BLOCK);
            if (z_ret!=Z_OK)
                return 0; //error
            // add enough empty blocks to get to a byte boundary
            z_ret=deflatePending(&self->stream,Z_NULL,&bits);
            if (z_ret!=Z_OK)
                return 0; //error
            if (bits & 1){
                z_ret=deflate(&self->stream,Z_SYNC_FLUSH);
                if (z_ret!=Z_OK)
                    return 0; //error
            } else if (bits & 7) {
                do { // add static empty blocks
                    z_ret=deflatePrime(&self->stream, 10, 2);
                    if (z_ret!=Z_OK)
                        return 0; //error
                    z_ret=deflatePending(&self->stream,Z_NULL,&bits);
                    if (z_ret!=Z_OK)
                        return 0; //error
                } while (bits & 7);
                z_ret=deflate(&self->stream,Z_BLOCK);
                if (z_ret!=Z_OK)
                    return 0; //error
            }
        }else{
            z_ret=deflate(&self->stream,Z_FINISH);
            if (z_ret!=Z_STREAM_END)
                return 0; //error
        }
        result=self->stream.total_out;
        self->stream.total_out=0;
        assert(self->stream.avail_in==0);
        return result;
    }
    
    _def_fun_compressType(_zlib_dictCompressType,"zlibD");
    static const TDictCompressPlugin_zlib zlibDictCompressPlugin={
        {_zlib_dictCompressType,_default_maxCompressedSize,_zlib_getDictSizeByData,
            _zlib_dictCompressOpen,_zlib_dictCompressClose,_zlib_dictCompress},
        9,8,MAX_WBITS};
    
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
    
    static size_t _zstd_getDictSizeByData(struct hsync_TDictCompress* dictCompressPlugin,hpatch_StreamPos_t dataSize){
        TDictCompressPlugin_zstd*  plugin=(TDictCompressPlugin_zstd*)dictCompressPlugin;
        size_t dictBits=_getDictBitsByData(plugin->dict_bits,10,dataSize);
        plugin->dict_bits=(hpatch_byte)dictBits;
        return ((size_t)1)<<dictBits;
    }

    static void _zstd_dictCompressClose(const struct hsync_TDictCompress* dictCompressPlugin,
                                        hsync_dictCompressHandle dictHandle){
        ZSTD_CCtx* s=(ZSTD_CCtx*)dictHandle;
        if (s!=0){
            size_t ret=ZSTD_freeCCtx(s);
            assert(ret==0);
        }
    }
    static hsync_dictCompressHandle _zstd_dictCompressOpen(const struct hsync_TDictCompress* dictCompressPlugin){
        const TDictCompressPlugin_zstd*  plugin=(const TDictCompressPlugin_zstd*)dictCompressPlugin;
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
        _zstd_dictCompressClose(dictCompressPlugin,s);
        return 0; //error
    }
    static size_t _zstd_dictCompress(hsync_dictCompressHandle dictHandle,
                                     unsigned char* out_code,unsigned char* out_codeEnd,
                                     const hpatch_byte* in_dict,const hpatch_byte* in_dictEnd_and_dataBegin,
                                     const unsigned char* in_dataEnd,hpatch_BOOL dict_isReset,hpatch_BOOL in_isEnd){
        ZSTD_CCtx* s=(ZSTD_CCtx*)dictHandle;
        ZSTD_inBuffer       s_input;
        ZSTD_outBuffer      s_output;
        size_t ret;
        size_t dictSize=in_dictEnd_and_dataBegin-in_dict;
        if ((dictSize>0)){//&&dict_isReset){ //reset dict
            ret=ZSTD_CCtx_reset(s,ZSTD_reset_session_only);
            if (ZSTD_isError(ret))
                return 0; //error
            ret=ZSTD_CCtx_refPrefix(s,in_dict,dictSize);
            if (ZSTD_isError(ret))
                return 0; //error
        }

        s_input.src=in_dictEnd_and_dataBegin;
        s_input.size=in_dataEnd-in_dictEnd_and_dataBegin;
        s_input.pos=0;
        s_output.dst=out_code;
        s_output.size=out_codeEnd-out_code;
        s_output.pos=0;
        ret=ZSTD_compressStream2(s,&s_output,&s_input,in_isEnd?ZSTD_e_end:ZSTD_e_flush);
        if (ZSTD_isError(ret))
            return 0; //error
        assert(s_input.pos==s_input.size);
        return s_output.pos;
    }
    
    _def_fun_compressType(_zstd_dictCompressType,"zstdD");
    static const TDictCompressPlugin_zstd zstdDictCompressPlugin={
        {_zstd_dictCompressType,_default_maxCompressedSize,_zstd_getDictSizeByData,
            _zstd_dictCompressOpen,_zstd_dictCompressClose,_zstd_dictCompress},
        20,20};
#endif//_CompressPlugin_zstd


#ifdef __cplusplus
}
#endif
#endif
