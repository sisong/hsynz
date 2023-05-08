//  client_download_demo.cpp
//  sync_client
//  Created by housisong on 2019-09-23.
/*
 The MIT License (MIT)
 Copyright (c) 2019-2020 HouSisong
 
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
#include "client_download_demo.h"
#include "HDiffPatch/file_for_patch.h"

struct TDownloadEmulation {
    const hpatch_TStreamInput* emulation_newSyncData;
    hpatch_TFileStreamInput    newSyncFile;
};

static hpatch_BOOL _readSyncData(IReadSyncDataListener* listener,uint32_t blockIndex,
                                 hpatch_StreamPos_t posInNewSyncData,hpatch_StreamPos_t posInNeedSyncData,
                                 unsigned char* out_syncDataBuf,uint32_t syncDataSize){
//warning: Read newSyncData from emulation data;
//         In the actual project, these data need downloaded from server.
    TDownloadEmulation* self=(TDownloadEmulation*)listener->readSyncDataImport;
    return self->emulation_newSyncData->read(self->emulation_newSyncData,posInNewSyncData,out_syncDataBuf,
                                             out_syncDataBuf+syncDataSize);
}

static void downloadEmulation_open_by(TDownloadEmulation* self,IReadSyncDataListener* out_emulation,
                                      const hpatch_TStreamInput* newSyncData){
    assert(out_emulation->readSyncDataImport==0);
    self->emulation_newSyncData=newSyncData;
    out_emulation->readSyncDataImport=self;
    out_emulation->readSyncData=_readSyncData;
}

hpatch_BOOL downloadEmulation_open(IReadSyncDataListener* out_emulation,
                                   const hpatch_TStreamInput* newSyncData){
    assert(out_emulation->readSyncDataImport==0);
    TDownloadEmulation* self=(TDownloadEmulation*)malloc(sizeof(TDownloadEmulation));
    if (self==0) return hpatch_FALSE;
    memset(self,0,sizeof(*self));
    downloadEmulation_open_by(self,out_emulation,newSyncData);
    return hpatch_TRUE;
}

hpatch_BOOL downloadEmulation_open_by_file(IReadSyncDataListener* out_emulation,
                                           const char* newSyncDataPath,size_t kStepRangeNumber){
    assert(out_emulation->readSyncDataImport==0);
    TDownloadEmulation* self=(TDownloadEmulation*)malloc(sizeof(TDownloadEmulation));
    if (self==0) return hpatch_FALSE;
    memset(self,0,sizeof(*self));
    if (!hpatch_TFileStreamInput_open(&self->newSyncFile,newSyncDataPath)){
        free(self);
        return hpatch_FALSE;
    }
    downloadEmulation_open_by(self,out_emulation,&self->newSyncFile.base);
    return hpatch_TRUE;
}

hpatch_BOOL downloadEmulation_close(IReadSyncDataListener* emulation){
    if (emulation==0) return hpatch_TRUE;
    TDownloadEmulation* self=(TDownloadEmulation*)emulation->readSyncDataImport;
    memset(emulation,0,sizeof(*emulation));
    if (self==0) return hpatch_TRUE;
    hpatch_BOOL result=hpatch_TFileStreamInput_close(&self->newSyncFile);
    free(self);
    return result;
}

#define  check(value) { if (!(value)){ fprintf(stderr,"check "#value" error!\n");  \
                            result=hpatch_FALSE; goto clear; } }

hpatch_BOOL downloadEmulation_download_file(const char* file_url,const hpatch_TStreamOutput* out_stream,
                                            hpatch_StreamPos_t continueDownloadPos){
    //copy file_url file data to out_stream
    const size_t _tempCacheSize=hpatch_kFileIOBufBetterSize;
    hpatch_BOOL result=hpatch_TRUE;
    TByte        temp_cache[_tempCacheSize];
    hpatch_StreamPos_t pos=continueDownloadPos;
    hpatch_TFileStreamInput   oldFile;
    hpatch_TFileStreamInput_init(&oldFile);
    
    check(hpatch_TFileStreamInput_open(&oldFile,file_url)); //local file!
    check(oldFile.base.streamSize>=continueDownloadPos);
    while (pos<oldFile.base.streamSize){
        size_t copyLen=_tempCacheSize;
        if (pos+copyLen>oldFile.base.streamSize)
            copyLen=(size_t)(oldFile.base.streamSize-pos);
        check(oldFile.base.read(&oldFile.base,pos,temp_cache,temp_cache+copyLen));
        check(out_stream->write(out_stream,pos,temp_cache,temp_cache+copyLen));
        pos+=copyLen;
    }
    check(!oldFile.fileError);
clear:
    hpatch_TFileStreamInput_close(&oldFile);
    return result;
}
