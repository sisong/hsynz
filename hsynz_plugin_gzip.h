//  hsynz_plugin_gzip.h
//  sync_make
//  Created by housisong on 2022-12-15.
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
#ifndef hsynz_plugin_gzip_h
#define hsynz_plugin_gzip_h
#include "HDiffPatch/libhsync/sync_make/hsynz_plugin.h" //hsync_THsynz
#include <assert.h>
#if ((defined _CompressPlugin_zlib)||(defined _CompressPlugin_ldef))
#if (_IsNeedIncludeDefaultCompressHead)
# ifdef _CompressPlugin_ldef
#   include "libdeflate.h" // https://github.com/ebiggers/libdeflate
# else
#   include "zlib.h" // http://zlib.net/  https://github.com/madler/zlib
# endif
#endif
#ifdef _CompressPlugin_ldef
#   define _crc32 libdeflate_crc32
#else
#   define _crc32 crc32
#endif
# ifdef _CompressPlugin_ldef
#endif

    class hsync_THsynz_gzip:public hsync_THsynz{
    public:
        hsync_THsynz_gzip(){
            hsynz_write_head=_write_head;
            hsynz_readed_data=_readed_data;
            hsynz_write_foot=_write_foot;
        }
    private:
        hpatch_StreamPos_t newDataSize;
    #if (_DEBUG)
        hpatch_StreamPos_t _newDataSize;
    #endif
        uint32_t crc;
        inline static void pushUInt32(hpatch_byte* buf,uint32_t v){
            buf[0]=v&0xFF;       buf[1]=(v>>8)&0xFF;
            buf[2]=(v>>16)&0xFF; buf[3]=(v>>24)&0xFF;
        }
        static hpatch_StreamPos_t _write_head(struct hsync_THsynz* zPlugin,
                                              const hpatch_TStreamOutput* out_stream,hpatch_StreamPos_t curOutPos,
                                              bool isDirSync,hpatch_StreamPos_t newDataSize,uint32_t kSyncBlockSize,
                                              hpatch_TChecksum* strongChecksumPlugin,hsync_TDictCompress* compressPlugin){
            hsync_THsynz_gzip* self=(hsync_THsynz_gzip*)zPlugin;
            const size_t _kGzipHeadSize=4+4+2;
            hpatch_byte buf[_kGzipHeadSize]={0};
            self->newDataSize=newDataSize;
    #if (_DEBUG)
            self->_newDataSize=newDataSize;
    #endif
            self->crc=(uint32_t)_crc32(0,0,0);
            buf[0]='\x1F'; buf[1]='\x8B'; buf[2]='\x08';
            buf[9]='\x03';
            checkv(out_stream->write(out_stream,curOutPos,&buf[0],&buf[0]+_kGzipHeadSize));
            return curOutPos+_kGzipHeadSize;
        }
        static void _readed_data(struct hsync_THsynz* zPlugin,
                                 const hpatch_byte* newData,size_t dataSize){
            hsync_THsynz_gzip* self=(hsync_THsynz_gzip*)zPlugin;
            assert(dataSize==(uInt)dataSize);
    #if (_DEBUG)
            assert(self->_newDataSize>=dataSize);
            self->_newDataSize-=dataSize;
    #endif
            self->crc=(uint32_t)_crc32(self->crc,newData,dataSize);       
        }
        static hpatch_StreamPos_t _write_foot(struct hsync_THsynz* zPlugin,
                                              const hpatch_TStreamOutput* out_stream,hpatch_StreamPos_t curOutPos,
                                              const hpatch_byte* newDataCheckChecksum,size_t checksumSize){
            hsync_THsynz_gzip* self=(hsync_THsynz_gzip*)zPlugin;
            const size_t _kGzipFootSize=4+4;
            hpatch_byte buf[_kGzipFootSize]={0};
    #if (_DEBUG)
            assert(self->_newDataSize==0);
    #endif
            pushUInt32(&buf[0],(uint32_t)self->crc);
            pushUInt32(&buf[4],(uint32_t)self->newDataSize);
            checkv(out_stream->write(out_stream,curOutPos,&buf[0],&buf[0]+_kGzipFootSize));
            return curOutPos+_kGzipFootSize;
        }

    };

#endif //_CompressPlugin_zlib||_CompressPlugin_ldef
#endif // hsynz_plugin_gzip_h
