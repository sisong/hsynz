//  client_download_http.cpp
//  hsync_http: http(s) download demo by http(s)
//  Created by housisong on 2020-01-29.
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
#include "client_download_http.h"
#include <assert.h>
#include "hsync_import_patch.h" // HSYNC_VERSION_STRING
#include "../HDiffPatch/file_for_patch.h"
#include "../HDiffPatch/libHDiffPatch/HDiff/private_diff/mem_buf.h"
#if (_IS_USED_MULTITHREAD)
#   include "../HDiffPatch/libParallel/parallel_import.h" //this_thread_yield
#   include "../HDiffPatch/libParallel/parallel_channel.h"
#endif
using namespace hdiff_private;

#include "minihttp/minihttp.h" // https://github.com/sisong/minihttp
using namespace minihttp;
#if defined(_MSC_VER)
#	ifdef MINIHTTP_USE_MBEDTLS
#		pragma comment( lib, "Advapi32.lib" )
#	endif
#	if defined(_WIN32_WCE)
#		pragma comment( lib, "ws2.lib" )
#	else
#		pragma comment( lib, "ws2_32.lib" )
#	endif
#endif /* _MSC_VER */

namespace{

static bool doInitNetwork(){
    struct TInitNetwork {
        TInitNetwork():initOk(false){ initOk=InitNetwork(); }
        ~TInitNetwork(){ if (initOk) StopNetwork(); }
        bool initOk;
    };
    static TInitNetwork nt;
    return nt.initOk;
}

static const size_t kBestBufSize=hpatch_kFileIOBufBetterSize;
static const size_t kStepLimitRangCount=64;
static const int    kTimeout_s=6;
const hpatch_StreamPos_t kEmptyEndPos=~(hpatch_StreamPos_t)0;
static const char*  kHttpUserAgent="hsync" HSYNC_VERSION_STRING;


struct THttpDownload:public HttpSocket{
    explicit THttpDownload(const hpatch_TStreamOutput* out_stream=0,
                           hpatch_StreamPos_t curOutPos=0,hpatch_StreamPos_t endOutPos=kEmptyEndPos)
    :is_finished(false),is_write_error(false),_out_stream(out_stream),
    _cur_pos(curOutPos),_end_pos(endOutPos){
        this->SetBufsizeIn(kBestBufSize);
        this->SetNonBlocking(false);
        this->SetFollowRedirect(true);
        this->SetAlwaysHandle(false);
        this->SetUserAgent(kHttpUserAgent);
    }

    inline bool isDownloadSuccess(){
        return IsSuccess() && is_finished && (!is_write_error); }
    inline bool isNeedUpdate(){
        return (isOpen() || HasPendingTask())&&(!is_write_error)&&(!is_finished);
    }
    inline void setWork(const hpatch_TStreamOutput* out_stream){
        _out_stream=out_stream;
        _cur_pos=0;
        is_finished=false;
    }
    bool doDownload(const std::string& file_url){
        if (!Download(file_url))
            return false;
        while(isNeedUpdate()){
            if (!update())
#if (_IS_USED_MULTITHREAD)
                this_thread_yield();
#else
            /*sleep?*/;
#endif
        }
        return isDownloadSuccess();
    }
private:
    volatile bool   is_finished;
    volatile bool   is_write_error;
    
    const hpatch_TStreamOutput* _out_stream;
    hpatch_StreamPos_t          _cur_pos;
    const hpatch_StreamPos_t    _end_pos;
    virtual void _OnRequestDone(){
        is_finished = true;
    }
    virtual void _OnRecv(void *incoming, unsigned size){
        if(!size || !IsSuccess() || is_write_error)
            return;
        assert(_cur_pos+size<=_end_pos);
        const unsigned char* data=(const unsigned char*)incoming;
        if (_out_stream->write(_out_stream,_cur_pos,data,data+size)){
            _cur_pos+=size;
        }else{
            is_write_error=true;
            this->close();
        }
    }
};


struct THttpRangeDownload{
    explicit THttpRangeDownload(const char* file_url)
    :_file_url(file_url),cache(0),readedSize(0),nsi(0),isInited(false),
    kStepMaxCacheSize(kBestBufSize){
        if (!doInitNetwork()){ throw std::runtime_error("InitNetwork error!"); }
    }
    virtual ~THttpRangeDownload(){ _clearAll();  }
    static hpatch_BOOL readSyncDataBegin(IReadSyncDataListener* listener,
                                         const TNeedSyncInfos* needSyncInfo){
        THttpRangeDownload* self=(THttpRangeDownload*)listener->readSyncDataImport;
        self->nsi=needSyncInfo;
        return hpatch_TRUE;
    }
    static void readSyncDataEnd(IReadSyncDataListener* listener){ }
    
    static hpatch_BOOL readSyncData(IReadSyncDataListener* listener,uint32_t blockIndex,
                                    hpatch_StreamPos_t posInNewSyncData,hpatch_StreamPos_t posInNeedSyncData,
                                    unsigned char* out_syncDataBuf,uint32_t syncDataSize){
        THttpRangeDownload* self=(THttpRangeDownload*)listener->readSyncDataImport;
        return self->readSyncData(blockIndex,posInNewSyncData,posInNeedSyncData,out_syncDataBuf,syncDataSize);
    }
private:
    THttpDownload     _hd;
    const std::string _file_url;
    TAutoMem        _mem;
    struct TDataBuf{
        TByte*      buf;
        size_t      savedSize;
        size_t      bufSize;
        inline TDataBuf():buf(0),savedSize(0),bufSize(0){}
    };
    TDataBuf*       cache;
    size_t          readedSize;
    std::vector<TDataBuf> _dataBufs;
    const TNeedSyncInfos* nsi;
    bool           isInited;
    const size_t kStepMaxCacheSize;
    hpatch_BOOL readFromCache(uint32_t syncDataSize,unsigned char* out_syncDataBuf){
        if (!cache) return hpatch_FALSE;
        size_t cachedSize=cache->savedSize-readedSize;
        if (cachedSize>0){
            assert(cachedSize>=syncDataSize);
            memcpy(out_syncDataBuf,cache->buf+readedSize,syncDataSize);
            readedSize+=syncDataSize;
            return hpatch_TRUE;
        }else{
            return hpatch_FALSE;
        }
    }
    size_t setRanges(std::vector<TRange>& out_ranges,uint32_t& blockIndex,
                     hpatch_StreamPos_t& posInNewSyncData,size_t bufSize){
        out_ranges.clear();
        size_t sumSize=0;
        for (uint32_t i=blockIndex;i<nsi->blockCount;++i){
            hpatch_BOOL isNeedSync;
            uint32_t    syncSize;
            nsi->getBlockInfoByIndex(nsi,i,&isNeedSync,&syncSize);
            if (!isNeedSync){ posInNewSyncData+=syncSize; ++blockIndex; continue; }
            if (sumSize+syncSize>bufSize)
                break; //finish
            if ((!out_ranges.empty())&&(out_ranges.back().second+1==posInNewSyncData))
                out_ranges.back().second+=syncSize;
            else if (out_ranges.size()>=kStepLimitRangCount)
                break; //finish
            else
                out_ranges.push_back(TRange(posInNewSyncData,posInNewSyncData+syncSize-1));
            sumSize+=syncSize;
            ++blockIndex;
            posInNewSyncData+=syncSize;
        }
        return sumSize;
    }
    inline void _clearAll(){
        _closeAll();
    }
    inline void _closeAll(){
        _hd.close();
    }
    bool getData(THttpDownload& hd,uint32_t blockIndex,hpatch_StreamPos_t posInNewSyncData,
                 uint32_t syncDataSize,unsigned char* out_syncDataBuf){
        hpatch_TStreamOutput out_stream;
        if (!cache){
            hd.Ranges().clear();
            hd.Ranges().push_back(TRange(posInNewSyncData,posInNewSyncData+syncDataSize-1));
            mem_as_hStreamOutput(&out_stream,out_syncDataBuf,out_syncDataBuf+syncDataSize);
        }else{
            assert(readedSize==cache->savedSize);
            cache->savedSize=setRanges(hd.Ranges(),blockIndex,
                                       posInNewSyncData,cache->bufSize);
            readedSize=0;
            mem_as_hStreamOutput(&out_stream,cache->buf,cache->buf+cache->savedSize);
        }
        hd.setWork(&out_stream);
        return hd.doDownload(_file_url);
    }

    inline hpatch_BOOL readSyncData(uint32_t blockIndex,hpatch_StreamPos_t posInNewSyncData,
                                    hpatch_StreamPos_t posInNeedSyncData,
                                    unsigned char* out_syncDataBuf,uint32_t syncDataSize){
        if (!isInited){
            try{
                _initDownload(blockIndex,posInNewSyncData,posInNeedSyncData);
            }catch(...){
                return hpatch_FALSE;
            }
        }
        if (readFromCache(syncDataSize,out_syncDataBuf))
            return hpatch_TRUE;
        if (!getData(_hd,blockIndex,posInNewSyncData,syncDataSize,out_syncDataBuf))
            return hpatch_FALSE;
        if (!cache)
            return hpatch_TRUE;
        else
            return readFromCache(syncDataSize,out_syncDataBuf);
    }

    void _initDownload(uint32_t blockIndex,hpatch_StreamPos_t posInNewSyncData,
                       hpatch_StreamPos_t posInNeedSyncData){
        assert(cache==0);
        assert(!isInited);
        isInited=true;
        readedSize=0;
        size_t bufSize=0;
        if ((kStepMaxCacheSize>=nsi->kSyncBlockSize*2))
            bufSize=kStepMaxCacheSize;
        //else not need cache
        if (bufSize>0){
            _dataBufs.resize(1);
            _mem.realloc(bufSize);
            _dataBufs[0].buf=_mem.data();
            _dataBufs[0].bufSize=bufSize;
            cache=_dataBufs.data();
            cache->savedSize=0;
        }
    }
};

static hpatch_BOOL _download(const char* file_url,const hpatch_TStreamOutput* out_stream,
                             hpatch_StreamPos_t continueDownloadPos,hpatch_StreamPos_t& endPos){
    if (!doInitNetwork()){ throw std::runtime_error("InitNetwork error!"); }
    assert(continueDownloadPos<=endPos);
    THttpDownload hd(out_stream,continueDownloadPos);
    if ((continueDownloadPos>0)||(endPos!=kEmptyEndPos)){
        hpatch_StreamPos_t rangEnd=(endPos==kEmptyEndPos)?kEmptyEndPos:(endPos-1);
        hd.Ranges().push_back(TRange(continueDownloadPos,rangEnd));
    }
    if (!hd.doDownload(file_url))
        return hpatch_FALSE;
    if (hd.Ranges().empty())
        endPos=continueDownloadPos+hd.GetContentLen();
    else
        endPos=hd.GetRangsBytesLen();
    return hpatch_TRUE;
}

} //end namespace

hpatch_BOOL download_range_by_http_open(IReadSyncDataListener* out_httpListener,const char* file_url){
    THttpRangeDownload* self=0;
    try {
        self=new THttpRangeDownload(file_url);
    } catch (...) {
        if (self) delete self;
        return hpatch_FALSE;
    }
    out_httpListener->readSyncDataImport=self;
    out_httpListener->readSyncDataBegin=THttpRangeDownload::readSyncDataBegin;
    out_httpListener->readSyncData=THttpRangeDownload::readSyncData;
    out_httpListener->readSyncDataEnd=THttpRangeDownload::readSyncDataEnd;
    return hpatch_TRUE;
}

hpatch_BOOL download_range_by_http_close(IReadSyncDataListener* httpListener){
    if (httpListener==0) return hpatch_TRUE;
    THttpRangeDownload* self=(THttpRangeDownload*)httpListener->readSyncDataImport;
    if (self==0) return true;
    httpListener->readSyncDataImport=0;
    try {
        delete self;
        return hpatch_TRUE;
    } catch (...) {
        return hpatch_FALSE;
    }
}

hpatch_BOOL download_file_by_http(const char* file_url,const hpatch_TStreamOutput* out_stream,
                                  hpatch_StreamPos_t continueDownloadPos){
    if (continueDownloadPos>0){
        hpatch_TStreamOutput temp_stream; unsigned char tempBuf[1];
        mem_as_hStreamOutput(&temp_stream,tempBuf,tempBuf+1);
        hpatch_StreamPos_t endPos=1;
        if (!_download(file_url,&temp_stream,0,endPos)) return hpatch_FALSE;//not request "HEAD" to get contentLen
        if (continueDownloadPos>endPos){
            fprintf(stderr,"  download continue pos(%" PRIu64 ") > \"content-length\"(%" PRIu64 ") ERROR!\n",
                    continueDownloadPos,endPos);
            return hpatch_FALSE;
        }
        if (continueDownloadPos==endPos) return hpatch_TRUE;
    }
    
    hpatch_StreamPos_t endPos=kEmptyEndPos;
    return _download(file_url,out_stream,continueDownloadPos,endPos);
}
