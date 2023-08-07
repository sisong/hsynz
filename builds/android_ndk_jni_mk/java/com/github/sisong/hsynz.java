package com.github.sisong;

import android.text.TextUtils;  //isEmpty

/*

Usage A:  diff(oldFile,hsyniFile,hsynzDownloader,outDiffFile,...)
        + patch(oldFile,hsyniFile,diffFile,outNewFile,...)

Usage B:  sync_info(oldFile,hsyniFile,hsynzDownloader,diffInfoCacheFile,...)
        + sync_patch(oldFile,hsyniFile,hsynzDownloader,diffInfoCacheFile,outNewFile,...)

Usage C: sync_patch(oldFile,hsyniFile,hsynzDownloader,diffInfoCacheFile,outNewFile,...)

*/

public class hsynz{

    public final static class TNeedDownloadRanges{
        private long sumRangeCount;
        private long sumDataLen;
        private long cNeedRangesHandle;
        protected final void nullRanges_MTSafe(){ //call this func when doSyncPatch() exit
            synchronized(this){
                cNeedRangesHandle=0;
            }
        }

        //get NeedDownloadRanges's count
        public final long getSumRangeCount(){return sumRangeCount;}
        //get NeedDownloadRanges's sum len
        public final long getSumDataLen(){return sumDataLen;}

        //return got range count
        //  note: 2 long values are a range; if range is (25,29) means need from file pos 25, download (29+1-25) length data;
        public final int getNextRanges(long[] dstRanges){
            if (cNeedRangesHandle==0) return 0;
            final int dstRangeBeginIndex=0;
            final int dstRangeEndIndex=(dstRanges.length>>1);
            //if (dstRangeBeginIndex>=dstRangeEndIndex) return 0;
            return nativeGetNextRanges(cNeedRangesHandle,dstRanges,dstRangeBeginIndex,dstRangeEndIndex-dstRangeBeginIndex);
        }
        //same as getNextRanges(), but muti-thread safe
        public final int getNextRanges_MTSafe(long[] dstRanges){
            synchronized(this){
                return getNextRanges(dstRanges);
            }
        }
    }

    public final static class TByteBuf{
        private long cBufHandle;
        //copy src data to this buf;
        //  this buf needed data sum size is readDataLen, see IRangeDownloader::readDownloadedData();
        //  dstBufPos default 0, if copyDataLen<readDataLen then dstBufPos+=copyDataLen for next;
        public final void setData(int dstBufPos,byte[] src,int srcReadPos,int copyDataLen){
            setByteBufData(cBufHandle,dstBufPos,src,srcReadPos,copyDataLen);
        }
    }

    public static interface IRangeDownloader{
        //only call once when got sync info
        //  hsynzDiffSize is diff data total size
        public void    onSyncInfo(long newFileSize,long hsynzFileSize,long hsynzDiffSize);

        //only call once befor download begin
        // if used continue download, needRanges's sum size == hsynzDiffSize - (downloaded size)
        public boolean downloadRanges(TNeedDownloadRanges needRanges);
        //loop call for get the downloaded data in order; if no data wait until got; if error return false;
        public boolean readDownloadedData(TByteBuf dstBuf,int readDataLen);
    }

    //sync diff: download diffFile
    //  isContinue: if (isContinue&(outDiffFile's fileSize>0)) then continue download;
    //  return TSyncClient_resultType, 0 is ok;
    public final static int diff(String oldFile,String hsyniFile,IRangeDownloader hsynzDownloader,
                                 String outDiffFile,boolean isContinue,int threadNum){
        if (TextUtils.isEmpty(hsyniFile)) return kSyncClient_optionsError;
        if (TextUtils.isEmpty(outDiffFile)) return kSyncClient_optionsError;
        if (hsynzDownloader==null) return kSyncClient_optionsError;
        return doSyncPatch(oldFile,hsyniFile,hsynzDownloader,outDiffFile,kSyncDiff_default,
                null,isContinue,threadNum,new TByteBuf(),new TNeedDownloadRanges());
    }

    //local patch: patch with diffFile
    //  return TSyncClient_resultType, 0 is ok;
    public final static int patch(String oldFile,String hsyniFile,String diffFile,
                                  String outNewFile,boolean isContinue){
        if (TextUtils.isEmpty(hsyniFile)) return kSyncClient_optionsError;
        if (TextUtils.isEmpty(diffFile)) return kSyncClient_optionsError;
        if (TextUtils.isEmpty(outNewFile)) return kSyncClient_optionsError;
        return doSyncPatch(oldFile,hsyniFile,null,diffFile,kSyncDiff_default,
                outNewFile,isContinue,1,null,null);
    }


    //get sync info: time-consuming diff calculation, & call back hsynzDownloader.onSyncInfo(); not download data.
    //  diffInfoCacheFile: can null; cache store diff result info, sync_patch() can skip the diff calculation when used this cache file.
    //  return TSyncClient_resultType, 0 is ok;
    public final static int sync_info(String oldFile,String hsyniFile,IRangeDownloader hsynzDownloader,
                                      String diffInfoCacheFile,int threadNum){
        if (TextUtils.isEmpty(hsyniFile)) return kSyncClient_optionsError;
        if (hsynzDownloader==null) return kSyncClient_optionsError;
        return doSyncPatch(oldFile,hsyniFile,hsynzDownloader,diffInfoCacheFile,kSyncDiff_info,
                null,true,threadNum,null,null);
    }

    //sync patch: sync diff + patch
    //  diffInfoCacheFile: can null; cache store diff result info;
    //    if different oldFile--hsyniFile pair, can't used same cache file;
    //    sync_patch() func will not auto delete this temp cache file.
    //  return TSyncClient_resultType, 0 is ok;
    public final static int sync_patch(String oldFile,String hsyniFile,IRangeDownloader hsynzDownloader,
                                       String diffInfoCacheFile,String outNewFile,boolean isContinue,int threadNum){
        if (TextUtils.isEmpty(hsyniFile)) return kSyncClient_optionsError;
        if (hsynzDownloader==null) return kSyncClient_optionsError;
        if (TextUtils.isEmpty(outNewFile)) return kSyncClient_optionsError;
        return doSyncPatch(oldFile,hsyniFile,hsynzDownloader,diffInfoCacheFile,kSyncDiff_info,
                outNewFile,isContinue,threadNum,new TByteBuf(),new TNeedDownloadRanges());
    }


    final static private String libName="hsynz"; //libhsynz.so
    static {
        System.loadLibrary(libName);
        nativeInit();
    }

    private static native void nativeInit();
    private static final  int  kSyncClient_ok=0;
    private static final  int  kSyncClient_optionsError=1;
    private static final  int  kSyncDiff_default=0; // diffFile saved info+data
    private static final  int  kSyncDiff_info=1;    // saved diff result info
    private static final  int  kSyncDiff_data=2;    // saved download diff data, only for test 
    private static native int  doSyncPatch(String oldFile,String hsyniFile,IRangeDownloader hsynzDownloader,
                                           String localDiffFile,int diffType,String outNewFile,boolean isContinue,
                                           int threadNum,TByteBuf dstBuf,TNeedDownloadRanges needRanges);
    private static native void setByteBufData(long cBufHandle,int bufPos,byte[] src,int srcPos,int dataLen);
    private static native int  nativeGetNextRanges(long cNeedRangesHandle,long[] dstNextRanges,int dstRangePos,int maxGetRangeLen);
}
