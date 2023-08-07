// hsynz_jni.cpp
//  Created by sisong on 2020-12-25.
//  Copyright © 2020-2022 housisong. All rights reserved.
#include "hsynz_jni.h"
#ifdef __cplusplus
extern "C" {
#endif

    typedef struct{
        jfieldID    sumRangeCount;
        jfieldID    sumDataLen;
        jfieldID    cNeedRangesHandle;
        jmethodID   nullRanges_MTSafe;
    } TNeedDownloadRangesIDs;
    static TNeedDownloadRangesIDs _g_needDownloadRangesIDs={0};
    static inline const TNeedDownloadRangesIDs* getNeedDownloadRangesIDs(){
        return &_g_needDownloadRangesIDs;
    }

    typedef struct{
        jmethodID   onSyncInfo;
        jmethodID   downloadRanges;
        jmethodID   readDownloadedData;
        jfieldID    cBufHandle;
    } IRangeDownloaderIDs;
    static IRangeDownloaderIDs _g_rangeDownloaderIDs={0};
    static inline const IRangeDownloaderIDs* getRangeDownloaderIDs(){
        return &_g_rangeDownloaderIDs;
    }

    JNIEXPORT void
    Java_com_github_sisong_hsynz_nativeInit(JNIEnv* jenv,jobject jobj){
        jclass objClass = jenv->FindClass("com/github/sisong/hsynz$TNeedDownloadRanges");
        _g_needDownloadRangesIDs.sumRangeCount    =jenv->GetFieldID(objClass,"sumRangeCount",    "J");
        _g_needDownloadRangesIDs.sumDataLen       =jenv->GetFieldID(objClass,"sumDataLen",       "J");
        _g_needDownloadRangesIDs.cNeedRangesHandle=jenv->GetFieldID(objClass,"cNeedRangesHandle","J");
        _g_needDownloadRangesIDs.nullRanges_MTSafe=jenv->GetMethodID(objClass,"nullRanges_MTSafe","()V");

        objClass=jenv->FindClass("com/github/sisong/hsynz$IRangeDownloader");
        _g_rangeDownloaderIDs.onSyncInfo        =jenv->GetMethodID(objClass,"onSyncInfo",        "(JJJ)V");
        _g_rangeDownloaderIDs.downloadRanges    =jenv->GetMethodID(objClass,"downloadRanges",    "(Lcom/github/sisong/hsynz$TNeedDownloadRanges;)Z");
        _g_rangeDownloaderIDs.readDownloadedData=jenv->GetMethodID(objClass,"readDownloadedData","(Lcom/github/sisong/hsynz$TByteBuf;I)Z");

        objClass=jenv->FindClass("com/github/sisong/hsynz$TByteBuf");
        _g_rangeDownloaderIDs.cBufHandle = jenv->GetFieldID(objClass,"cBufHandle","J");
    }

    JNIEXPORT void 
    Java_com_github_sisong_hsynz_setByteBufData(JNIEnv* jenv,jobject jobj,
                                                jlong cBufHandle,jint bufPos,jbyteArray src,jint srcPos,jint dataLen){
         signed char* buf=(signed char*)(size_t)cBufHandle;
         jenv->GetByteArrayRegion(src,srcPos,dataLen,buf+bufPos);
    }

    struct TNeedDownloadRanges_c{
        const TNeedSyncInfos*   needSyncInfo;
        uint32_t                curBlockIndex;
        hpatch_StreamPos_t      curPosInNewSyncData;
        inline int getNextRanges(jlong* dstRanges,int maxGetRangeLen){
            assert(sizeof(jlong)==sizeof(hpatch_StreamPos_t));
            return (int)TNeedSyncInfos_getNextRanges(needSyncInfo,(hpatch_StreamPos_t*)dstRanges,maxGetRangeLen,
                                                     &curBlockIndex,&curPosInNewSyncData);
        }
    };

    struct TReadSyncDataListener{
        IReadSyncDataListener   base;
        JNIEnv*                 jenv;
        jobject                 hsynzDownloader;
        jobject                 dstBuf;
        jobject                 needRanges;
        TNeedDownloadRanges_c*  cNeedRanges;
    };

    static void TReadSyncDataListener_onNeedSyncInfo(struct  IReadSyncDataListener* listener,const TNeedSyncInfos* needSyncInfo){
        TReadSyncDataListener* self=(TReadSyncDataListener*)listener->readSyncDataImport;
        self->cNeedRanges->needSyncInfo=needSyncInfo;
        const IRangeDownloaderIDs* rangeDownloaderIDs=getRangeDownloaderIDs();
        self->jenv->CallVoidMethod(self->hsynzDownloader,rangeDownloaderIDs->onSyncInfo,
                                   needSyncInfo->newDataSize,needSyncInfo->newSyncDataSize,needSyncInfo->needSyncSumSize);
    }

    static hpatch_BOOL TReadSyncDataListener_readSyncDataBegin(struct  IReadSyncDataListener* listener,const TNeedSyncInfos* needSyncInfo,
                                                               uint32_t blockIndex,hpatch_StreamPos_t posInNewSyncData,hpatch_StreamPos_t posInNeedSyncData){
        TReadSyncDataListener* self=(TReadSyncDataListener*)listener->readSyncDataImport;
        self->cNeedRanges->needSyncInfo=needSyncInfo;
        self->cNeedRanges->curBlockIndex=blockIndex;
        self->cNeedRanges->curPosInNewSyncData=posInNewSyncData;

        const TNeedDownloadRangesIDs* needRangesIDs=getNeedDownloadRangesIDs();
        self->jenv->SetLongField(self->needRanges,needRangesIDs->sumRangeCount,(jlong)TNeedSyncInfos_getRangeCount(needSyncInfo,blockIndex,posInNewSyncData));
        self->jenv->SetLongField(self->needRanges,needRangesIDs->sumDataLen,   (jlong)(size_t)(needSyncInfo->needSyncSumSize-posInNeedSyncData));
        self->jenv->SetLongField(self->needRanges,needRangesIDs->cNeedRangesHandle,(jlong)(size_t)self->cNeedRanges);
        
        const IRangeDownloaderIDs* rangeDownloaderIDs=getRangeDownloaderIDs();
        jboolean ret=self->jenv->CallBooleanMethod(self->hsynzDownloader,rangeDownloaderIDs->downloadRanges,self->needRanges);
        return ret;
    }
    static hpatch_BOOL TReadSyncDataListener_readSyncData(struct IReadSyncDataListener* listener,uint32_t blockIndex,
                                                          hpatch_StreamPos_t posInNewSyncData,hpatch_StreamPos_t posInNeedSyncData,
                                                          unsigned char* out_syncDataBuf,uint32_t syncDataSize){
        TReadSyncDataListener* self=(TReadSyncDataListener*)listener->readSyncDataImport;
        const IRangeDownloaderIDs* rangeDownloaderIDs=getRangeDownloaderIDs();
        self->jenv->SetLongField(self->dstBuf,rangeDownloaderIDs->cBufHandle,(jlong)(size_t)out_syncDataBuf);
        jboolean ret=self->jenv->CallBooleanMethod(self->hsynzDownloader,rangeDownloaderIDs->readDownloadedData,
                                                   self->dstBuf,(int)syncDataSize);
        return ret;
    }
    static void TReadSyncDataListener_readSyncDataEnd(struct IReadSyncDataListener* listener){
        //TReadSyncDataListener* self=(TReadSyncDataListener*)listener->readSyncDataImport;
    }

    static hpatch_inline void _listener_finish(struct TReadSyncDataListener* self){
        const TNeedDownloadRangesIDs* needRangesIDs=getNeedDownloadRangesIDs();
        if (self->needRanges&&needRangesIDs->nullRanges_MTSafe)
            self->jenv->CallVoidMethod(self->needRanges,needRangesIDs->nullRanges_MTSafe);
    }

    
    JNIEXPORT jint 
    Java_com_github_sisong_hsynz_nativeGetNextRanges(JNIEnv* jenv,jobject jobj,
                                                     jlong cNeedRangesHandle,jlongArray dstNextRanges,
                                                     jint dstRangeBeginIndex,jint maxGetRangeLen){
        jlong* dstRanges=jenv->GetLongArrayElements(dstNextRanges,NULL);
        if (dstRanges==0) return 0; //error
        TNeedDownloadRanges_c* needRanges=(TNeedDownloadRanges_c*)cNeedRangesHandle;
        int result=needRanges->getNextRanges(dstRanges+dstRangeBeginIndex*2,maxGetRangeLen);
        jenv->ReleaseLongArrayElements(dstNextRanges,dstRanges,0);
        return result;
    }

    JNIEXPORT jint
    Java_com_github_sisong_hsynz_doSyncPatch(JNIEnv* jenv,jobject jobj,
                                             jstring oldFile,jstring hsyniFile,jobject hsynzDownloader,
                                             jstring localDiffFile,jint diffType,jstring outNewFile,jboolean isContinue,
                                             jint threadNum,jobject dstBuf,jobject needRanges){
        const char* cOldFile=0;
        const char* cHsyniFile=0;
        const char* cLocalDiffFile=0;
        const char* cOutNewFile=0;
        TNeedDownloadRanges_c cNeedRanges={0};
        TReadSyncDataListener syncDataListener={{0},0};
        int result=kSyncClient_ok;
        assert(_kSyncDiff_TYPE_MAX_>=(size_t)diffType);

        _check_jn2cstr(oldFile,cOldFile);
        _check_j2cstr(hsyniFile,cHsyniFile);
        _check_jn2cstr(localDiffFile,cLocalDiffFile);
        _check_jn2cstr(outNewFile,cOutNewFile);

        syncDataListener.jenv=jenv;
        syncDataListener.hsynzDownloader=hsynzDownloader;
        syncDataListener.dstBuf=dstBuf;
        syncDataListener.needRanges=needRanges;
        syncDataListener.cNeedRanges=&cNeedRanges;
        syncDataListener.base.readSyncDataImport=&syncDataListener;
        syncDataListener.base.onNeedSyncInfo=TReadSyncDataListener_onNeedSyncInfo;
        syncDataListener.base.readSyncDataBegin=TReadSyncDataListener_readSyncDataBegin;
        syncDataListener.base.readSyncData=TReadSyncDataListener_readSyncData;
        syncDataListener.base.readSyncDataEnd=TReadSyncDataListener_readSyncDataEnd;

        result=hsynz_patch(cOutNewFile,cOldFile,cHsyniFile,&syncDataListener.base,cLocalDiffFile,
                           (TSyncDiffType)diffType,isContinue?1:0,threadNum);
        _listener_finish(&syncDataListener);

    _clear:
        _jrelease_cstr(outNewFile,cOutNewFile);
        _jrelease_cstr(localDiffFile,cLocalDiffFile);
        _jrelease_cstr(hsyniFile,cHsyniFile);
        _jrelease_cstr(oldFile,cOldFile);
        return result;
    }

#ifdef __cplusplus
}
#endif
