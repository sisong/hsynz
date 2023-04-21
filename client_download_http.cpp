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

static const size_t kBestBufSize = hpatch_kFileIOBestMaxSize>>4;
static const size_t kBestRangesCacheSize=hpatch_kFileIOBestMaxSize;
static const size_t kStepLimitRangCount=32;
static const int    kTimeout_s=10;
static const hpatch_StreamPos_t kEmptyEndPos=kNullRangePos;
static const char*  kHttpUserAgent="hsync/" HSYNC_VERSION_STRING;

static inline 
bool _isCanCombine(const std::vector<TRange>& out_ranges,hpatch_StreamPos_t rangeBegin){
    return (!out_ranges.empty())&&(out_ranges.back().second+1==rangeBegin);
}
static inline
void _addRange(std::vector<TRange>& out_ranges,
                hpatch_StreamPos_t rangeBegin,hpatch_StreamPos_t rangeEnd){
    assert(rangeBegin<rangeEnd);
    hpatch_StreamPos_t rangLast=(rangeEnd!=kEmptyEndPos)?(rangeEnd-1):kEmptyEndPos;
    out_ranges.push_back(TRange(rangeBegin,rangLast));
}


struct THttpDownload:public HttpSocket{
    explicit THttpDownload(const hpatch_TStreamOutput* out_stream=0,hpatch_StreamPos_t curOutPos=0)
    :requestSumSize(0),requestCount(0),is_write_error(false),
     _out_stream(out_stream),_cur_pos(curOutPos){
        if (!doInitNetwork()){ throw std::runtime_error("InitNetwork error!"); }
        this->SetBufsizeIn(kBestBufSize);
        this->SetNonBlocking(false);
        this->SetFollowRedirect(true);
        this->SetAlwaysHandle(false);
        this->SetKeepAlive(kTimeout_s);
        this->SetUserAgent(kHttpUserAgent);
    }

    inline bool isDownloadSuccess(){
        return IsSuccess() && (requestCount==0) && (!is_write_error)
               &&((requestSumSize==0)||(requestSumSize==kEmptyEndPos)); }
    inline bool isNeedUpdate(){
        return (isOpen() || HasPendingTask())&&(!is_write_error)&&(requestCount>0);
    }
    bool doDownload(const std::string& file_url){
        const hpatch_StreamPos_t requestSumSize_back=requestSumSize;
        const std::vector<TRange>& ranges=this->Ranges();
        if (ranges.empty())
            requestSumSize=kEmptyEndPos;
        for (size_t i=0;(i<ranges.size())&&(requestSumSize!=kEmptyEndPos);++i){
            if (ranges[i].second!=kEmptyEndPos)
                requestSumSize+=(hpatch_StreamPos_t)(ranges[i].second+1-ranges[i].first);
            else
                requestSumSize=kEmptyEndPos;
        }
        ++requestCount;
        if (Download(file_url)){
            return true;
        }else{
            --requestCount;
            requestSumSize=requestSumSize_back;
            return false;
        }
    }
    bool waitDownloaded(const std::string& file_url){
        if (!doDownload(file_url))
            return false;
        while(isNeedUpdate()){
            if (!update()){
#if (_IS_USED_MULTITHREAD)
                this_thread_yield();
#else
                //sleep?
#endif
            }
        }
        return isDownloadSuccess();
    }
protected:
    volatile hpatch_StreamPos_t requestSumSize;
    volatile size_t requestCount;
    volatile bool   is_write_error;
    
    const hpatch_TStreamOutput* _out_stream;
    hpatch_StreamPos_t          _cur_pos;
    virtual void _OnRequestDone(){
        HttpSocket::_OnRequestDone();
        if (requestCount<1)
            is_write_error=true;
        --requestCount;
    }
    inline void _OnRecvSize(unsigned size){
        if (requestSumSize<size)
            is_write_error=true;
        requestSumSize-=size;
    }
    virtual void _OnRecv(void *incoming, unsigned size){
        if(!size || !IsSuccess() || is_write_error)
            return;
        _OnRecvSize(size);
        const unsigned char* data=(const unsigned char*)incoming;
        if (_out_stream->write(_out_stream,_cur_pos,data,data+size)){
            _cur_pos+=size;
        }else{
            is_write_error=true;
            this->close();
        }
    }
};


struct THttpRangeDownload:public THttpDownload{
    explicit THttpRangeDownload(const char* file_url)
    :_hd(*this),_file_url(file_url),_readPos(0),_writePos(0),nsi(0),
    curBlockIndex(~0),curPosInNewSyncData(hpatch_kNullStreamPos){}
    virtual ~THttpRangeDownload(){ _closeAll();  }
    static hpatch_BOOL readSyncDataBegin(IReadSyncDataListener* listener,const TNeedSyncInfos* needSyncInfo,
                                         uint32_t blockIndex,hpatch_StreamPos_t posInNewSyncData,hpatch_StreamPos_t posInNeedSyncData){
        THttpRangeDownload* self=(THttpRangeDownload*)listener->readSyncDataImport;
        self->nsi=needSyncInfo;
        try{
            size_t cacheSize=kBestRangesCacheSize;
            self->_cache.realloc(cacheSize);
            self->_writePos=0;
            self->_readPos=0;
            //if (!self->_sendDownloads_all(blockIndex,posInNewSyncData))
            //    return hpatch_FALSE;
            if (!self->_sendDownloads_init(blockIndex,posInNewSyncData)) //step by step send
                return hpatch_FALSE;
        }catch(...){
            return hpatch_FALSE;
        }

        return hpatch_TRUE;
    }
    static void readSyncDataEnd(IReadSyncDataListener* listener){ 
        THttpRangeDownload* self=(THttpRangeDownload*)listener->readSyncDataImport;
        self->_closeAll(); 
    }
    
    static hpatch_BOOL readSyncData(IReadSyncDataListener* listener,uint32_t blockIndex,
                                    hpatch_StreamPos_t posInNewSyncData,hpatch_StreamPos_t posInNeedSyncData,
                                    unsigned char* out_syncDataBuf,uint32_t syncDataSize){
        THttpRangeDownload* self=(THttpRangeDownload*)listener->readSyncDataImport;
        try{
            return self->readSyncData(blockIndex,posInNewSyncData,posInNeedSyncData,out_syncDataBuf,syncDataSize);
        }catch(...){
            return hpatch_FALSE;
        }
    }
protected:
    THttpDownload&    _hd;
    const std::string _file_url;
    TAutoMem          _cache;
    size_t            _readPos;
    size_t            _writePos;
    const TNeedSyncInfos* nsi;
    void makeRanges(std::vector<TRange>& out_ranges,uint32_t& blockIndex,
                    hpatch_StreamPos_t& posInNewSyncData){
        out_ranges.clear();
        while (blockIndex<nsi->blockCount){
            hpatch_BOOL isNeedSync;
            uint32_t    syncSize;
            nsi->getBlockInfoByIndex(nsi,blockIndex,&isNeedSync,&syncSize);
            if (isNeedSync){
                if (_isCanCombine(out_ranges,posInNewSyncData))
                    out_ranges.back().second+=syncSize;
                else if (out_ranges.size()>=kStepLimitRangCount)
                    break; //finish
                else
                    _addRange(out_ranges,posInNewSyncData,posInNewSyncData+syncSize);
            }
            posInNewSyncData+=syncSize;
            ++blockIndex;
        }
    }
    inline void _closeAll(){
        _hd.close();
    }

    inline hpatch_BOOL readSyncData(uint32_t blockIndex,hpatch_StreamPos_t posInNewSyncData,
                                    hpatch_StreamPos_t posInNeedSyncData,
                                    unsigned char* out_syncDataBuf,uint32_t syncDataSize){
        while (syncDataSize>0){
            size_t savedSize=_savedSize();
            if (savedSize>0){
                size_t readSize=(savedSize<=syncDataSize)?savedSize:syncDataSize;
                _readFromCache(out_syncDataBuf,readSize);
                out_syncDataBuf+=readSize;
                syncDataSize-=(uint32_t)readSize;
                continue;
            }

            while(isNeedUpdate()&&(_emptySize()>GetBufSize())){
                if (!update()){
    #if (_IS_USED_MULTITHREAD)
                    this_thread_yield();
    #else
                    //sleep?
    #endif
                }
            }
            savedSize=_savedSize();
            if (savedSize==0){
                if (!IsSuccess()||is_write_error)
                    return hpatch_FALSE;
                if ((!isNeedUpdate())&&(requestCount==0))
                    return hpatch_FALSE;
            }
        }
        return hpatch_TRUE;
    }

    inline size_t _savedSize()const{
        return ((_writePos>=_readPos)?_writePos:_writePos+_cache.size())-_readPos;
    }
    inline size_t _emptySize()const{
        return _cache.size()-_savedSize();
    }

    uint32_t            curBlockIndex;
    hpatch_StreamPos_t  curPosInNewSyncData;
    bool _sendDownloads_init(uint32_t blockIndex,hpatch_StreamPos_t posInNewSyncData){
        assert(nsi!=0);
        printf("\nhttp download from block index %d/%d\n",blockIndex,nsi->blockCount);
        curBlockIndex=blockIndex;
        curPosInNewSyncData=posInNewSyncData;
        return _sendDownloads_step();
    }
    bool _sendDownloads_step(){
        try{
            while (curBlockIndex<nsi->blockCount){
                makeRanges(_hd.Ranges(),curBlockIndex,curPosInNewSyncData);
                if (!_hd.Ranges().empty())
                    return _hd.doDownload(_file_url);
            }
        }catch(...){
            return false;
        }
        return true;
    }
    virtual void _OnRequestDone(){
        THttpDownload::_OnRequestDone();
        if(!IsSuccess() || is_write_error)
            return;
        if (curBlockIndex<nsi->blockCount) {
            if (!_sendDownloads_step())
                is_write_error=true;
        }
    }
    bool _sendDownloads_all(uint32_t blockIndex,hpatch_StreamPos_t posInNewSyncData){
        if (!_sendDownloads_init(blockIndex,posInNewSyncData))
            return false;
        while (curBlockIndex<nsi->blockCount){
            if (!_sendDownloads_step())
                return false;
        }
        return true;
    }
    
    inline void _readFromCache(unsigned char* out_syncDataBuf,size_t readSize){
        const size_t cacheSize=_cache.size();
        while (readSize>0){
            size_t rlen=cacheSize-_readPos;
            rlen=(rlen<=readSize)?rlen:readSize;
            memcpy(out_syncDataBuf,_cache.data()+_readPos,rlen);
            out_syncDataBuf+=rlen;
            readSize-=rlen;
            _readPos+=rlen;
            _readPos=(_readPos<cacheSize)?_readPos:(_readPos-cacheSize);
        }
    }

    virtual void _OnRecv(void *incoming, unsigned size){
        if(!size || !IsSuccess() || is_write_error)
            return;
        _OnRecvSize(size);
        size_t emptySize=_emptySize();
        if (emptySize<=size)
            is_write_error=true;
        if (is_write_error)
            this->close();

        const unsigned char* data=(const unsigned char*)incoming;
        const size_t cacheSize=_cache.size();
        while (size>0){
            size_t rlen=cacheSize-_writePos;
            rlen=(rlen<=size)?rlen:size;
            memcpy(_cache.data()+_writePos,data,rlen);
            data+=rlen;
            size-=(unsigned)rlen;
            _writePos+=rlen;
            _writePos=(_writePos<cacheSize)?_writePos:(_writePos-cacheSize);
        }
    }
};

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
    if (self==0) return hpatch_TRUE;
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
    hpatch_StreamPos_t endPos=kEmptyEndPos;
    THttpDownload hd(out_stream,continueDownloadPos);
    if (continueDownloadPos>0)
        _addRange(hd.Ranges(),continueDownloadPos,endPos);

    if (!hd.waitDownloaded(file_url))
        return hpatch_FALSE;
                         
    if (continueDownloadPos>0)
        endPos=hd.GetRangsBytesLen();
    else
        endPos=continueDownloadPos+hd.GetContentLen();
    return hpatch_TRUE;
}
