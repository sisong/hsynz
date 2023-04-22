package com.github.sisong;

public class hsynz{

        public final static class TNeedDownloadRanges{
            private long rangeCount;
            private long sumDataLen;
            private long cNeedRangesHandle;
            public final long sumNeedRangeCount(){ return rangeCount;}
            public final long sumNeedDownloadDataLen(){return sumDataLen;}

            //return got range count             
            // note: 2 long values are a range
            public final int getNextRanges(long[] dstRanges){
                final int dstRangePos=0;
                final int maxRangeLen=(dstRanges.length>>1)-dstRangePos;
                return getNextRanges(cNeedRangesHandle,dstRanges,dstRangePos,maxRangeLen); 
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
        return hsyncz_patch(oldFile,hsyniFile,hsynzDownloader,outDiffFile,kSyncDiff_default,
                            null,isContinue,threadNum,new TByteBuf(),new TNeedDownloadRanges());
    }

    //local patch: patch with diffFile
    // return TSyncClient_resultType, 0 is ok;
    public final static int patch(String oldFile,String hsyniFile,String diffFile,
                                  String outNewFile,boolean isContinue){
        return hsyncz_patch(oldFile,hsyniFile,null,diffFile,kSyncDiff_default,
                            outNewFile,isContinue,1,null,null);
    }


    //sync patch: sync diff + patch
    // diffInfoCacheTempFile: can null; cache store temp info of diff result; 
    //    Different oldFile--hsyniFile pair, can't used same cache file;
    //    sync_patch() func will not auto delete this temp file.
    // return TSyncClient_resultType, 0 is ok;
    public final static int sync_patch(String oldFile,String hsyniFile,IRangeDownloader hsynzDownloader,
                                       String diffInfoCacheTempFile,String outNewFile,boolean isContinue,int threadNum){
        return hsyncz_patch(oldFile,hsyniFile,hsynzDownloader,diffInfoCacheTempFile,kSyncDiff_info,
                            outNewFile,isContinue,threadNum,new TByteBuf(),new TNeedDownloadRanges());
    }

    final static private String libName="hsynz"; //libhsynz.so
    static public void initLib(){
        System.loadLibrary(libName);
        nativeInit();
    }

    private static native void nativeInit();
    private static final  int  kSyncDiff_default=0; // diffFile saved info+data
    private static final  int  kSyncDiff_info=1;    // saved diff result info
    private static final  int  kSyncDiff_data=2;    // saved download diff data, only for test 
    private static native int  hsyncz_patch(String oldFile,String hsyniFile,IRangeDownloader hsynzDownloader,
                                            String localDiffFile,int diffType,String outNewFile,boolean isContinue,
                                            int threadNum,TByteBuf dstBuf,TNeedDownloadRanges needRanges);
    private static native void setByteBufData(long cBufHandle,int bufPos,byte[] src,int srcPos,int dataLen);
    private static native int  getNextRanges(long cNeedRangesHandle,long[] dstNextRanges,int dstRangePos,int maxRangeLen);
}