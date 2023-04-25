package com.github.sisong;

import android.text.TextUtils;  //isEmpty

public class hsynz{

        public final static class TNeedDownloadRanges{
            private long sumRangeCount;
            private long sumDataLen;
            private long cNeedRangesHandle;
            public final long getSumRangeCount(){return sumRangeCount;}
            public final long getSumDataLen(){return sumDataLen;}

            //return got range count             
            //  note: 2 long values are a range; if range is (25,29) means need from file pos 25, download (29+1-25) length data;
            public final int getNextRanges(long[] dstRanges){
                final int dstRangePos=0;
                final int maxGetRangeLen=(dstRanges.length>>1)-dstRangePos;
                return nativeGetNextRanges(cNeedRangesHandle,dstRanges,dstRangePos,maxGetRangeLen);
            }
        }
        
        public final static class TByteBuf{
            private long cBufHandle;
            public final void setData(int bufPos,byte[] src,int srcPos,int dataLen){
                setByteBufData(cBufHandle,bufPos,src,srcPos,dataLen);
            }
        }

    public static interface IRangeDownloader{
        //only call once
        public boolean downloadRanges(TNeedDownloadRanges needRanges);
        //if no data wait until got; if error return false;
        public boolean readDownloadedData(TByteBuf dstBuf,int readDataLen);
    }

    //sync diff: download diffFile
    // isContinue: if (isContinue&(outDiffFile's fileSize>0)) then continue download;
    // return TSyncClient_resultType, 0 is ok;
    public final static int diff(String oldFile,String hsyniFile,IRangeDownloader hsynzDownloader,
                                 String outDiffFile,boolean isContinue,int threadNum){
        if (TextUtils.isEmpty(hsyniFile)) return kSyncClient_optionsError;
        if (TextUtils.isEmpty(outDiffFile)) return kSyncClient_optionsError;
        if (hsynzDownloader==null) return kSyncClient_optionsError;
        return doSyncPatch(oldFile,hsyniFile,hsynzDownloader,outDiffFile,kSyncDiff_default,
                           null,isContinue,threadNum,new TByteBuf(),new TNeedDownloadRanges());
    }

    //local patch: patch with diffFile
    // return TSyncClient_resultType, 0 is ok;
    public final static int patch(String oldFile,String hsyniFile,String diffFile,
                                  String outNewFile,boolean isContinue){
        if (TextUtils.isEmpty(hsyniFile)) return kSyncClient_optionsError;
        if (TextUtils.isEmpty(diffFile)) return kSyncClient_optionsError;
        if (TextUtils.isEmpty(outNewFile)) return kSyncClient_optionsError;
        return doSyncPatch(oldFile,hsyniFile,null,diffFile,kSyncDiff_default,
                           outNewFile,isContinue,1,null,null);
    }


    //sync patch: sync diff + patch
    // diffInfoCacheTempFile: can null; cache store temp info of diff result; 
    //    Different oldFile--hsyniFile pair, can't used same cache file;
    //    sync_patch() func will not auto delete this temp file.
    // return TSyncClient_resultType, 0 is ok;
    public final static int sync_patch(String oldFile,String hsyniFile,IRangeDownloader hsynzDownloader,
                                       String diffInfoCacheTempFile,String outNewFile,boolean isContinue,int threadNum){
        if (TextUtils.isEmpty(hsyniFile)) return kSyncClient_optionsError;
        if (hsynzDownloader==null) return kSyncClient_optionsError;
        if (TextUtils.isEmpty(outNewFile)) return kSyncClient_optionsError;
        return doSyncPatch(oldFile,hsyniFile,hsynzDownloader,diffInfoCacheTempFile,kSyncDiff_info,
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
    private static native int  nativeGetNextRanges(long cNeedRangesHandle,long[] dstNextRanges,int dstRangePos,int maxRangeLen);
}