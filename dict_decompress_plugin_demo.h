//dict_decompress_plugin_demo.h
//  dict decompress plugin demo for hsync_demo
/*
 The MIT License (MIT)
 Copyright (c) 2022 HouSisong
 
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
#ifndef dict_decompress_plugin_demo_h
#define dict_decompress_plugin_demo_h
//dict decompress plugin demo:
//  zlibDictDecompressPlugin

#include "HDiffPatch/libhsync/sync_client/dict_decompress_plugin.h"
#ifndef HPatch_decompress_plugin_demo_h
#ifndef _IsNeedIncludeDefaultCompressHead
#   define _IsNeedIncludeDefaultCompressHead 1
#endif
#endif
#ifdef __cplusplus
extern "C" {
#endif

#ifdef  _CompressPlugin_zlib
#if (_IsNeedIncludeDefaultCompressHead)
#   include "zlib.h" // http://zlib.net/  https://github.com/madler/zlib
#endif
    //zlibDictDecompressPlugin
    typedef struct{
        hsync_TDictDecompress base;
        int                   windowBits;
    } TDictDecompressPlugin_zlib;
    typedef struct{
        z_stream              stream;
    } _TDictDecompressPlugin_zlib_data;
    
    static hpatch_BOOL _zlib_dict_is_can_open(const char* compressType){
        return (0==strcmp(compressType,"zlibD"));
    }
    static hsync_dictDecompressHandle _zlib_dictDecompressOpen(struct hsync_TDictDecompress* dictDecompressPlugin){
        const TDictDecompressPlugin_zlib*  plugin=(const TDictDecompressPlugin_zlib*)dictDecompressPlugin;
        _TDictDecompressPlugin_zlib_data* self=(_TDictDecompressPlugin_zlib_data*)malloc(sizeof(_TDictDecompressPlugin_zlib_data));
        if (self==0) return 0;
        memset(self,0,sizeof(*self));
        if (inflateInit2(&self->stream,plugin->windowBits)!=Z_OK){
            free(self);
            return 0;// error
        }
        return self;
    }
    static void _zlib_dictDecompressClose(struct hsync_TDictDecompress* dictDecompressPlugin,
                                          hsync_dictDecompressHandle dictHandle){
        _TDictDecompressPlugin_zlib_data* self=(_TDictDecompressPlugin_zlib_data*)dictHandle;
        if (self!=0){
            if (self->stream.state!=0){
                int ret=inflateEnd(&self->stream);
                self->stream.state=0;
                assert(Z_OK==ret);
            }
            free(self);
        }
    }
    static hpatch_BOOL _zlib_dictDecompress(hsync_dictDecompressHandle dictHandle,
                                            const unsigned char* in_code,const unsigned char* in_codeEnd,
                                            const unsigned char* in_dict,unsigned char* in_dictEnd_and_out_dataBegin,
                                            unsigned char* out_dataEnd,hpatch_BOOL dict_isReset,hpatch_BOOL out_isEnd){
        _TDictDecompressPlugin_zlib_data* self=(_TDictDecompressPlugin_zlib_data*)dictHandle;
        int z_ret;
        size_t result;
        size_t dictSize=in_dictEnd_and_out_dataBegin-in_dict;
        const size_t dataSize=(size_t)(out_dataEnd-in_dictEnd_and_out_dataBegin);
        if ((dictSize>0)&&dict_isReset){ //reset dict
            if (inflateReset(&self->stream)!=Z_OK)
                return hpatch_FALSE;
            assert(dictSize==(uInt)dictSize);
            z_ret=inflateSetDictionary(&self->stream,in_dict,(uInt)dictSize);
            if (z_ret!=Z_OK)
                return hpatch_FALSE; //error
        }
        self->stream.next_in =(Bytef*)in_code;
        self->stream.avail_in=(uInt)(in_codeEnd-in_code);
        assert(self->stream.avail_in==(size_t)(in_codeEnd-in_code));
        self->stream.next_out =in_dictEnd_and_out_dataBegin;
        self->stream.avail_out=(uInt)dataSize;
        assert(self->stream.avail_out==dataSize);
        z_ret=inflate(&self->stream,out_isEnd?Z_FINISH:Z_PARTIAL_FLUSH);
        if (out_isEnd){
            if (z_ret!=Z_STREAM_END) 
                return hpatch_FALSE;//error
        }else{
            if (z_ret!=Z_OK)
                return hpatch_FALSE;//error
        }
        result=self->stream.total_out;
        self->stream.total_out=0;
        assert(self->stream.avail_in==0);
        assert(result==dataSize);
        return (result==dataSize);
    }
    
    
    static const TDictDecompressPlugin_zlib zlibDictDecompressPlugin={
        { _zlib_dict_is_can_open, _zlib_dictDecompressOpen,
          _zlib_dictDecompressClose, _zlib_dictDecompress },
        -MAX_WBITS };
    
#endif//_CompressPlugin_zlib


#ifdef __cplusplus
}
#endif
#endif
