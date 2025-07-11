//  hsync_demo.cpp
//  client for sync patch
//      like zsync : http://zsync.moria.org.uk/
//  Created by housisong on 2019-09-18.
/*
 The MIT License (MIT)
 Copyright (c) 2019-2025 HouSisong
 
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
#include <vector>
#include "HDiffPatch/_clock_for_demo.h"
#include "HDiffPatch/_atosize.h"
#include "HDiffPatch/libParallel/parallel_import.h"
#include "HDiffPatch/file_for_patch.h"
#include "HDiffPatch/_dir_ignore.h"
#include "HDiffPatch/libHDiffPatch/HDiff/private_diff/mem_buf.h"

#include "HDiffPatch/libhsync/sync_client/sync_client.h"
#if (_IS_NEED_DIR_DIFF_PATCH)
#   include "HDiffPatch/libhsync/sync_client/dir_sync_client.h"
#   define _IS_NEED_tempDirPatchListener 0
#   include "HDiffPatch/hpatch_dir_listener.h"
#endif
#ifndef _IS_NEED_ZSYNC
#   define _IS_NEED_ZSYNC   1
#endif
#if (_IS_NEED_ZSYNC)
#   include "HDiffPatch/libhsync/zsync_client_wrapper/zsync_client_wrapper.h"
#endif
#include "hsync_import_patch.h"
#ifndef _IS_NEED_MAIN
#   define  _IS_NEED_MAIN 1
#endif
#ifndef _IS_NEED_CMDLINE
#   define  _IS_NEED_CMDLINE 1
#endif

#ifndef _IS_SYNC_PATCH_DEMO
#   define  _IS_SYNC_PATCH_DEMO 1
#endif
#ifndef _IS_NEED_PRINT_LOG
#   define _IS_NEED_PRINT_LOG   1
#endif
#if (_IS_NEED_PRINT_LOG)
#   define  _log_info_utf8  hpatch_printPath_utf8
#else
#   define  printf(...)
#   define  _log_info_utf8(...) do{}while(0)
#endif


typedef struct TSyncDownloadPlugin{
    //download range of file
    hpatch_BOOL (*download_range_open) (IReadSyncDataListener* out_listener,
                                        const char* file_url,size_t kStepRangeNumber);
    hpatch_BOOL (*download_range_close)(IReadSyncDataListener* listener);
    //download file
    hpatch_BOOL (*download_file)      (const char* file_url,const hpatch_TStreamOutput* out_stream,
                                       hpatch_StreamPos_t continueDownloadPos);
} TSyncDownloadPlugin;

#if (_IS_SYNC_PATCH_DEMO)
//simple file demo plugin
#   include "client_download_demo.h"
hpatch_BOOL getSyncDownloadPlugin(TSyncDownloadPlugin* out_downloadPlugin){
    out_downloadPlugin->download_range_open=downloadEmulation_open_by_file;
    out_downloadPlugin->download_range_close=downloadEmulation_close;
    out_downloadPlugin->download_file=downloadEmulation_download_file;
    return hpatch_TRUE;
}
#else
//NOTE: download by tcp/udp/http/https?  You can implement your download plugin
hpatch_BOOL getSyncDownloadPlugin(TSyncDownloadPlugin* out_downloadPlugin);
#endif

#ifndef _IS_NEED_DEFAULT_CompressPlugin
#   define _IS_NEED_DEFAULT_CompressPlugin 1
#endif
#if (_IS_NEED_DEFAULT_CompressPlugin)
//===== select needs decompress plugins or change to your plugin=====
//#   define _CompressPlugin_zlib
#   define _CompressPlugin_ldef  // compatible with zlib's deflate encoding
#   define _CompressPlugin_zstd
#endif

#ifdef _CompressPlugin_ldef
#   define _IS_USED_LIBDEFLATE_CRC32    1
#endif

#include "dict_decompress_plugin_demo.h"

#ifndef _IS_NEED_DEFAULT_ChecksumPlugin
#   define _IS_NEED_DEFAULT_ChecksumPlugin 1
#endif
#if (_IS_NEED_DEFAULT_ChecksumPlugin)
//===== select needs checksum plugins or change to your plugin=====
#   define _ChecksumPlugin_xxh128
#   define _ChecksumPlugin_mbedtls_md5
#   define _ChecksumPlugin_mbedtls_sha512
#   define _ChecksumPlugin_mbedtls_sha256
#   define _ChecksumPlugin_crc32
#  if (_IS_NEED_ZSYNC)
#   define _ChecksumPlugin_mbedtls_md4
#   define _ChecksumPlugin_mbedtls_sha1
#  endif
#endif
#include "HDiffPatch/checksum_plugin_demo.h"

#if (_IS_NEED_CMDLINE)

#if (_IS_SYNC_PATCH_DEMO)
#   define _NOTE_TEXT_URL  "test"
#   define _NOTE_TEXT_APP  "hsynz::hsync_demo"
#endif


static void printVersion(){
    printf(_NOTE_TEXT_APP " v" HSYNC_VERSION_STRING "\n\n");
}

static void printUsage(){
    printVersion();
    printf("sync  patch: [options] oldPath [-dl#hsyni_file_" _NOTE_TEXT_URL "] hsyni_file hsynz_file_" _NOTE_TEXT_URL " [-diffi#cacheTempFile] outNewPath\n"
           "download   : [options] -dl#hsyni_file_" _NOTE_TEXT_URL " hsyni_file\n"
           "local  diff: [options] oldPath hsyni_file hsynz_file_" _NOTE_TEXT_URL " -diff#outDiffFile\n"
           "local patch: [options] oldPath hsyni_file -patch#diffFile outNewPath\n"
           "sync  infos: [options] oldPath hsyni_file [-diffi#cacheTempFile]\n"
#if (_IS_NEED_DIR_DIFF_PATCH)
           "  oldPath can be file or directory(folder),\n"
#endif
           "  if oldPath is empty input parameter \"\"\n"
           "options:\n"
           "  -dl#hsyni_file_" _NOTE_TEXT_URL "\n"
           "    download hsyni_file from hsyni_file_" _NOTE_TEXT_URL " befor sync patch;\n"
           "  -diff#outDiffFile\n"
           "    create diffFile from ranges of hsynz_file_" _NOTE_TEXT_URL " befor local patch;\n"
           "  -diffi#cacheTempFile\n"
           "    saving diffInfo to cache file for optimize speed when continue sync patch;\n"
           "  -patch#diffFile\n"
           "    local patch(oldPath+diffFile) to outNewPath;\n"
           "  -cdl-{0|1}        or  -cdl-{off|on}\n"
           "    continue download data from breakpoint;\n"
           "    DEFAULT -cdl-1 opened, need set -cdl-0 or -cdl-off to close continue mode;\n"
           "  -rdl-retryDownloadNumber\n"
           "    number of auto retry connection, when network disconnected while downloading;\n"
           "    DEFAULT -rdl-0 retry closed; recommended 5,1k,1g,...\n"
           "  -r-stepRangeNumber\n"
           "    DEFAULT -r-32, recommended 16,20,...\n"
           "    limit the maximum number of .hsynz data ranges that can be downloaded \n"
           "    in a single request step; \n"
           "    if http(s) server not support multi-ranges request, must set -r-1\n"
#if (_IS_USED_MULTITHREAD)
           "  -p-parallelThreadNumber\n"
           "    DEFAULT -p-4; \n"
           "    if parallelThreadNumber>1 then open multi-thread Parallel mode;\n"
           "    NOTE: now download data always used single-thread.\n"
#endif
#if (_IS_NEED_DIR_DIFF_PATCH)
           "  -n-maxOpenFileNumber\n"
           "      limit Number of open files at same time when oldPath is directory;\n"
           "      maxOpenFileNumber>=8, DEFAULT -n-24, the best limit value by different\n"
           "        operating system.\n"
           "  -g#ignorePath[#ignorePath#...]\n"
           "      set iGnore path list in oldPath directory; ignore path list such as:\n"
           "        #.DS_Store#desktop.ini#*thumbs*.db#.git*#.svn/#cache_*/00*11/*.tmp\n"
           "      # means separator between names; (if char # in name, need write #: )\n"
           "      * means can match any chars in name; (if char * in name, need write *: );\n"
           "      / at the end of name means must match directory;\n"
#endif
           "  -f  Force overwrite, ignore write path already exists;\n"
           "      DEFAULT (no -f) not overwrite and then return error;\n"
           "      not support oldPath outNewPath same path!\n"
           "      if used -f and outNewPath is exist file:\n"
#if (_IS_NEED_DIR_DIFF_PATCH)
           "        if patch output file, will overwrite;\n"
#else
           "        will overwrite;\n"
#endif
#if (_IS_NEED_DIR_DIFF_PATCH)
           "        if patch output directory, will always return error;\n"
#endif
           "      if used -f and outNewPath is exist directory:\n"
#if (_IS_NEED_DIR_DIFF_PATCH)
           "        if patch output file, will always return error;\n"
#else
           "        will always return error;\n"
#endif
#if (_IS_NEED_DIR_DIFF_PATCH)
           "        if patch output directory, will overwrite, but not delete\n"
           "          needless existing files in directory.\n"
#endif
           "  -v  output Version info.\n"
           "  -h or -?\n"
           "      output Help info (this usage).\n\n"
           );
}


//return TSyncClient_resultType
int sync_client_cmd_line(int argc, const char * argv[]);
#endif //_IS_NEED_CMDLINE

TSyncClient_resultType
    hsync_patch_2file(const char* outNewFile,const char* oldPath,bool oldIsDir,
                      const std::vector<std::string>& ignoreOldPathList,
                      const char* hsyni_file,IReadSyncDataListener* syncDataListener,
                      const char* localDiffFile,TSyncDiffType diffType,hpatch_BOOL isUsedDownloadContinue,
                      size_t kMaxOpenFileNumber,hpatch_BOOL isZsyncType,int threadNum);
TSyncClient_resultType 
    hsync_patch_2file(const char* outNewFile,const char* oldPath,bool oldIsDir,
                      const std::vector<std::string>& ignoreOldPathList,
                      const char* hsyni_file,const TSyncDownloadPlugin* downloadPlugin,const char* hsynz_file_url,
                      const char* localDiffFile,TSyncDiffType diffType,hpatch_BOOL isUsedDownloadContinue,
                      size_t kStepRangeNumber,size_t kMaxOpenFileNumber,hpatch_BOOL isZsyncType,int threadNum);
#if (_IS_NEED_DIR_DIFF_PATCH)
TSyncClient_resultType
    hsync_patch_2dir(const char* outNewDir,const char* oldPath,bool oldIsDir,
                     const std::vector<std::string>& ignoreOldPathList,
                     const char* hsyni_file,const TSyncDownloadPlugin* downloadPlugin,const char* hsynz_file_url,
                     const char* localDiffFile,TSyncDiffType diffType,hpatch_BOOL isUsedDownloadContinue,
                     size_t kStepRangeNumber,size_t kMaxOpenFileNumber,int threadNum);
#endif

static TSyncClient_resultType
    downloadNewSyncInfoFile(const TSyncDownloadPlugin* downloadPlugin,const char* hsyni_file_url,
                            const char* out_hsyni_file,bool isUsedDownloadContinue);


#if (_IS_NEED_MAIN)
#   if (_IS_USED_WIN32_UTF8_WAPI)
int wmain(int argc,wchar_t* argv_w[]){
    hdiff_private::TAutoMem  _mem(hpatch_kPathMaxSize*4);
    char** argv_utf8=(char**)_mem.data();
    if (!_wFileNames_to_utf8((const wchar_t**)argv_w,argc,argv_utf8,_mem.size()))
        return kSyncClient_optionsError;
    SetDefaultStringLocale();
    return sync_client_cmd_line(argc,(const char**)argv_utf8);
}
#   else
int main(int argc,char* argv[]){
    return  sync_client_cmd_line(argc,(const char**)argv);
}
#   endif
#endif

//ISyncInfoListener::findDecompressPlugin
static hsync_TDictDecompress* _findDecompressPlugin(ISyncInfoListener* listener,const char* compressType,size_t dictSize){
    if (compressType==0) return 0; //ok
    hsync_TDictDecompress* decompressPlugin=0;
#ifdef  _CompressPlugin_ldef
    if ((!decompressPlugin)&&ldefDictDecompressPlugin.base.is_can_open(compressType)){
        static TDictDecompressPlugin_ldef _ldefDictDecompressPlugin=ldefDictDecompressPlugin;
        _ldefDictDecompressPlugin.dict_bits=(hpatch_byte)_dictSizeToDictBits(dictSize);
        decompressPlugin=&_ldefDictDecompressPlugin.base;
    }
#else
    #ifdef  _CompressPlugin_zlib
        if ((!decompressPlugin)&&zlibDictDecompressPlugin.base.is_can_open(compressType)){
            static TDictDecompressPlugin_zlib _zlibDictDecompressPlugin=zlibDictDecompressPlugin;
            _zlibDictDecompressPlugin.dict_bits=(hpatch_byte)_dictSizeToDictBits(dictSize);
            decompressPlugin=&_zlibDictDecompressPlugin.base;
        }
    #endif
#endif
#ifdef  _CompressPlugin_zstd
    if ((!decompressPlugin)&&zstdDictDecompressPlugin.base.is_can_open(compressType)){
        static TDictDecompressPlugin_zstd _zstdDictDecompressPlugin=zstdDictDecompressPlugin;
        _zstdDictDecompressPlugin.dictSize=dictSize;
        decompressPlugin=&_zstdDictDecompressPlugin.base;
    }
#endif
    if (decompressPlugin==0){
        printf("  hsync patch can't decompress type: \"%s\"\n",compressType);
        return 0; //unsupport error
    }else{
        printf("  hsync patch run with decompress type: \"%s\"\n",compressType);
        return decompressPlugin; //ok
    }
}
//ISyncInfoListener::findChecksumPlugin
static hpatch_TChecksum* _findChecksumPlugin(ISyncInfoListener* listener,const char* strongChecksumType){
    assert((strongChecksumType!=0)&&(strlen(strongChecksumType)>0));
    hpatch_TChecksum* strongChecksumPlugin=0;
#ifdef  _ChecksumPlugin_xxh128
    if ((!strongChecksumPlugin)&&(0==strcmp(strongChecksumType,xxh128ChecksumPlugin.checksumType())))
        strongChecksumPlugin=&xxh128ChecksumPlugin;
#endif
#ifdef  _ChecksumPlugin_mbedtls_md5
    if ((!strongChecksumPlugin)&&(0==strcmp(strongChecksumType,md5ChecksumPlugin.checksumType())))
        strongChecksumPlugin=&md5ChecksumPlugin;
#endif
#ifdef  _ChecksumPlugin_mbedtls_md4
    if ((!strongChecksumPlugin)&&(0==strcmp(strongChecksumType,md4ChecksumPlugin.checksumType())))
        strongChecksumPlugin=&md4ChecksumPlugin;
#endif
#ifdef  _ChecksumPlugin_mbedtls_sha512
    if ((!strongChecksumPlugin)&&(0==strcmp(strongChecksumType,sha512ChecksumPlugin.checksumType())))
        strongChecksumPlugin=&sha512ChecksumPlugin;
#endif
#ifdef  _ChecksumPlugin_mbedtls_sha256
    if ((!strongChecksumPlugin)&&(0==strcmp(strongChecksumType,sha256ChecksumPlugin.checksumType())))
        strongChecksumPlugin=&sha256ChecksumPlugin;
#endif
#ifdef  _ChecksumPlugin_mbedtls_sha1
    if ((!strongChecksumPlugin)&&(0==strcmp(strongChecksumType,sha1ChecksumPlugin.checksumType())))
        strongChecksumPlugin=&sha1ChecksumPlugin;
#endif
#ifdef  _ChecksumPlugin_crc32
    if ((!strongChecksumPlugin)&&(0==strcmp(strongChecksumType,crc32ChecksumPlugin.checksumType())))
        strongChecksumPlugin=&crc32ChecksumPlugin;
#endif
    if (strongChecksumPlugin==0){
        printf("  hsync patch can't found checksum type: \"%s\"\n",strongChecksumType);
        return 0; //unsupport error
    }else{
        printf("  hsync patch run with strongChecksum plugin: \"%s\"\n",strongChecksumType);
        return strongChecksumPlugin; //ok
    }
}

    static void printMatchResult(const TNeedSyncInfos* nsi) {
        const uint32_t kBlockCount=nsi->blockCount;
        const hpatch_StreamPos_t localDataSize=nsi->newDataSize-(nsi->kSyncBlockSize*(hpatch_StreamPos_t)nsi->needSyncBlockCount);
        printf("  syncBlockSize : %d\n  syncBlockCount: %d, /%d=%.1f%%\n"
               "  localDataSize : %" PRIu64 "\n  syncDataSize  : %" PRIu64 "\n",
               nsi->kSyncBlockSize,nsi->needSyncBlockCount,kBlockCount,
               100.0*nsi->needSyncBlockCount/kBlockCount,localDataSize,nsi->needSyncSumSize);
        hpatch_StreamPos_t downloadSize=nsi->newSyncInfoSize+nsi->needSyncSumSize;
        printf("  downloadSize  : %" PRIu64 "+%" PRIu64 "= %" PRIu64 ", /%" PRIu64 "=%.1f%%",
               nsi->newSyncInfoSize,nsi->needSyncSumSize,downloadSize,
               nsi->newDataSize,100.0*downloadSize/nsi->newDataSize);
        if ((nsi->newSyncDataSize>0)&&(nsi->newSyncDataSize<nsi->newDataSize)){
            hpatch_StreamPos_t maxDownloadSize=nsi->newSyncInfoSize+nsi->newSyncDataSize;
            printf(" (/%" PRIu64 "=%.1f%%)",maxDownloadSize,100.0*downloadSize/maxDownloadSize);
        }
        printf("\n");
        if (nsi->localDiffDataSize>0){
            printf("\n  localDiffDataSize    : %" PRIu64 "\n",nsi->localDiffDataSize);
            hpatch_StreamPos_t cdlSize=(nsi->needSyncSumSize>nsi->localDiffDataSize)?(nsi->needSyncSumSize-nsi->localDiffDataSize):0;
            printf(  "  continue downloadSize: %" PRIu64 ", /%" PRIu64 "=%.1f%%\n",
                   cdlSize,downloadSize,100.0*cdlSize/downloadSize);
        }
    }
//ISyncInfoListener::needSyncInfo
static void _onNeedSyncInfo(ISyncInfoListener* listener,const TNeedSyncInfos* needSyncInfo){
    printMatchResult(needSyncInfo);
}


#define _pferr(errorInfo) do{ hpatch_printStdErrPath_utf8(errorInfo); }while(0)
#define _check(value,exitCode,errInfo) do{ \
    if (!(value)) { _pferr(errInfo); LOG_ERR(" ERROR!\n"); return exitCode; } }while(0)
#define _check3(value,exitCode,errInfo0,errInfo1,errInfo2) do{ \
if (!(value)) { _pferr(errInfo0); _pferr(errInfo1); _pferr(errInfo2); LOG_ERR(" ERROR!\n"); return exitCode; } }while(0)

#if (_IS_NEED_CMDLINE)

#define _options_check(value,errorInfo) do{ \
    if (!(value)) { LOG_ERR("options " errorInfo " ERROR!\n\n"); \
                    printUsage(); return kSyncClient_optionsError; } }while(0)
#define _kNULL_VALUE    (-1)
#define _kNULL_SIZE     (~(size_t)0)
#define _kNULL_diffType ((TSyncDiffType)(_kSyncDiff_TYPE_MAX_+1))

#define _THREAD_NUMBER_NULL     0
#define _THREAD_NUMBER_MIN      1
#define _THREAD_NUMBER_DEFUALT  4
#define _THREAD_NUMBER_MAX      (1<<8)

#define kStepRangeNumber_default    32

enum TRunAsType{
    kRunAs_unknown =0,
    kRunAs_sync_patch,
    kRunAs_download,
    kRunAs_local_diff,
    kRunAs_local_patch,
    kRunAs_sync_infos,
};
           
#define _isSwapToPatchTag(tag) (0==strcmp("--patch",tag))

int isSwapToPatchMode(int argc,const char* argv[]){
    int i;
    for (i=1;i<argc;++i){
        if (_isSwapToPatchTag(argv[i]))
            return hpatch_TRUE;
    }
    return hpatch_FALSE;
}

#define _RETRY_DOWNLOAD_DO()        \
                size_t retryDownloadNumber=kRetryDownloadNumber; \
                do{
#define _RETRY_DOWNLOAD_WHILE(condition)    \
                    if ((retryDownloadNumber>0)&&(condition)){  \
                        LOG_ERR("\ndownload break ERROR! retry again(%" PRIu64 ") ...\n",(hpatch_uint64_t)retryDownloadNumber); \
                        --retryDownloadNumber;                  \
                        continue;   \
                    }else{ break; } \
                }while (1);


//return TSyncClient_resultType
int sync_client_cmd_line(int argc, const char * argv[]) {
    size_t      threadNum=_THREAD_NUMBER_NULL;
    hpatch_BOOL isForceOverwrite=_kNULL_VALUE;
    hpatch_BOOL isOutputHelp=_kNULL_VALUE;
    hpatch_BOOL isOutputVersion=_kNULL_VALUE;
    hpatch_BOOL isOldPathInputEmpty=_kNULL_VALUE;
    hpatch_BOOL isUsedDownloadContinue=_kNULL_VALUE;
    size_t      kStepRangeNumber=_kNULL_SIZE;
    size_t      kRetryDownloadNumber=_kNULL_SIZE;
    TSyncDiffType diffType=_kNULL_diffType;
    const char* hsyni_file_url=0;
    const char* diff_to_diff_file=0;
    const char* patch_by_diff_file=0;
    hpatch_BOOL isZsyncType=hpatch_FALSE;
//_IS_NEED_DIR_DIFF_PATCH
    size_t                      kMaxOpenFileNumber=_kNULL_SIZE; //only used in oldPath is dir
    std::vector<std::string>    ignoreOldPathList;
//
    std::vector<const char *> arg_values;
    for (int i=1; i<argc; ++i) {
        const char* op=argv[i];
        _options_check(op!=0,"?");
        if (op[0]!='-'){
            hpatch_BOOL isEmpty=(strlen(op)==0);
            if (isEmpty){
                if (isOldPathInputEmpty==_kNULL_VALUE)
                    isOldPathInputEmpty=hpatch_TRUE;
                else
                    _options_check(!isEmpty,"?"); //error return
            }else{
                if (isOldPathInputEmpty==_kNULL_VALUE)
                    isOldPathInputEmpty=hpatch_FALSE;
            }
            arg_values.push_back(op); //file path
            continue;
        }
        switch (op[1]) {
            case 'p':{
                if (strstr(op,"-patch#")==op){
                    const char* pfname=op+7;
                    _options_check((patch_by_diff_file==0)&&(strlen(pfname)>0),"-patch#?");
                    patch_by_diff_file=pfname;
#if (_IS_USED_MULTITHREAD)
                }else if (strstr(op,"-p-")==op){
                    _options_check(threadNum==_THREAD_NUMBER_NULL,"-p-?");
                    const char* pnum=op+3;
                    _options_check(a_to_size(pnum,strlen(pnum),&threadNum),"-p-?");
                    _options_check(threadNum>=_THREAD_NUMBER_MIN,"-p-?");
#endif
                }else{
#if (_IS_USED_MULTITHREAD)
                    _options_check(false,"-patch#? or -p-?");
#else
                    _options_check(false,"-patch#?");
#endif
                }
            } break;
            case 'c':{
                if ((strcmp(op,"-cdl")==0)||(strcmp(op,"-cdl-1")==0)||(strcmp(op,"-cdl-on")==0)
                  ||(strcmp(op,"-cdl-open")==0)||(strcmp(op,"-cdl-opened")==0)){
                    _options_check(isUsedDownloadContinue==_kNULL_VALUE,"-cdl?");
                    isUsedDownloadContinue=hpatch_TRUE;
                }else if ((strcmp(op,"-cdl-0")==0)||(strcmp(op,"-cdl-off")==0)
                  ||(strcmp(op,"-cdl-close")==0)||(strcmp(op,"-cdl-closed")==0)){
                    _options_check(isUsedDownloadContinue==_kNULL_VALUE,"-cdl?");
                    isUsedDownloadContinue=hpatch_FALSE;
                }else{
                    _options_check(false,"-cdl?");
                }
            } break;
            case 'r':{
                if (strstr(op,"-rdl-")==op){
                    const char* pnum=op+5;
                    _options_check((kRetryDownloadNumber==_kNULL_SIZE),"-rdl-?");
                    _options_check(kmg_to_size(pnum,strlen(pnum),&kRetryDownloadNumber),"-rdl-?");
                }else if (strstr(op,"-r-")==op){
                    const char* pnum=op+3;
                    _options_check((kStepRangeNumber==_kNULL_SIZE),"-r-?");
                    _options_check(kmg_to_size(pnum,strlen(pnum),&kStepRangeNumber),"-r-?");
                }else{
                    _options_check(false,"-r-? or -rdl-?");
                }
            } break;
            case 'd':{
                if (strstr(op,"-diff")==op){
                    if (strstr(op,"-diff#")==op){
                        const char* pfname=op+6;
                        _options_check((diff_to_diff_file==0)&&(strlen(pfname)>0),"-diff#?");
                        diff_to_diff_file=pfname;
                    }else if (strstr(op,"-diffi#")==op){ //now hidden option
                        const char* pfname=op+7;
                        _options_check((diff_to_diff_file==0)&&(strlen(pfname)>0),"-diffi#?");
                        diff_to_diff_file=pfname;
                        diffType=kSyncDiff_info;
                    }else if (strstr(op,"-diffd#")==op){ //now hidden option
                        const char* pfname=op+7;
                        _options_check((diff_to_diff_file==0)&&(strlen(pfname)>0),"-diffd#?");
                        diff_to_diff_file=pfname;
                        diffType=kSyncDiff_data; //NOTE: now, will run as single-thread
                    }else{
                        _options_check(false,"-diff?");
                    }
                }else if (strstr(op,"-dl#")==op){
                    const char* purl=op+4;
                    _options_check((hsyni_file_url==0)&&(strlen(purl)>0),"-dl#?");
                    hsyni_file_url=purl;
                }else{
                    _options_check(false,"-dl#? or -diff?");
                }
            } break;
#if (_IS_NEED_DIR_DIFF_PATCH)
            case 'n':{
                const char* pnum=op+3;
                _options_check((kMaxOpenFileNumber==_kNULL_SIZE)&&(op[2]=='-'),"-n-?");
                _options_check(kmg_to_size(pnum,strlen(pnum),&kMaxOpenFileNumber),"-n-?");
            } break;
            case 'g':{
                if (op[2]=='#'){ //-g#
                    const char* plist=op+3;
                    _options_check(_getIgnorePathSetList(ignoreOldPathList,plist),"-g#?");
                }else{
                    _options_check(hpatch_FALSE,"-g?");
                }
            } break;
#endif
            case 'f':{
                _options_check((isForceOverwrite==_kNULL_VALUE)&&(op[2]=='\0'),"-f");
                isForceOverwrite=hpatch_TRUE;
            } break;
            case '?':
            case 'h':{
                _options_check((isOutputHelp==_kNULL_VALUE)&&(op[2]=='\0'),"-h");
                isOutputHelp=hpatch_TRUE;
            } break;
            case 'v':{
                _options_check((isOutputVersion==_kNULL_VALUE)&&(op[2]=='\0'),"-v");
                isOutputVersion=hpatch_TRUE;
            } break;
            default: {
                _options_check(hpatch_FALSE,"?");
            } break;
        }//swich
    }
    
    if (isOutputHelp==_kNULL_VALUE)
        isOutputHelp=hpatch_FALSE;
    if (isOutputVersion==_kNULL_VALUE)
        isOutputVersion=hpatch_FALSE;
    if (isForceOverwrite==_kNULL_VALUE)
        isForceOverwrite=hpatch_FALSE;
    if (kStepRangeNumber==_kNULL_SIZE)
        kStepRangeNumber=kStepRangeNumber_default;
    else if (kStepRangeNumber<1)
        kStepRangeNumber=1;
#if (_IS_SYNC_PATCH_DEMO)
    kRetryDownloadNumber=0;
#else
    if (kRetryDownloadNumber==_kNULL_SIZE)
        kRetryDownloadNumber=0;
#endif
#if (_IS_USED_MULTITHREAD)
    if (threadNum==_THREAD_NUMBER_NULL)
        threadNum=_THREAD_NUMBER_DEFUALT;
    else if (threadNum>_THREAD_NUMBER_MAX)
        threadNum=_THREAD_NUMBER_MAX;
#else
    threadNum=1;
#endif
    
#if (_IS_NEED_DIR_DIFF_PATCH)
    if (kMaxOpenFileNumber==_kNULL_SIZE)
        kMaxOpenFileNumber=kMaxOpenFileNumber_default_patch;
    if (kMaxOpenFileNumber<kMaxOpenFileNumber_default_min)
        kMaxOpenFileNumber=kMaxOpenFileNumber_default_min;
#endif
    if (isOldPathInputEmpty==_kNULL_VALUE)
        isOldPathInputEmpty=hpatch_FALSE;
    if (isUsedDownloadContinue==_kNULL_VALUE)
        isUsedDownloadContinue=hpatch_TRUE;

    if (diffType==_kNULL_diffType)
        diffType=kSyncDiff_default;
    
    if (isOutputHelp||isOutputVersion){
        if (isOutputHelp)
            printUsage();//with version
        else
            printVersion();
        if (arg_values.empty())
            return kSyncClient_ok; //ok
    }
    if (threadNum>1)
        printf("multi-thread parallel: opened, threadNum: %d\n",(int)threadNum);
    printf("continue download: %s\n",isUsedDownloadContinue?"opened":"closed");

    _options_check((hsyni_file_url?1:0)+(patch_by_diff_file?1:0)<=1
                   +((diff_to_diff_file&&(diffType!=kSyncDiff_info))?1:0),
                   "-dl,-diff,-patch can't be used at the same time");

    const char* oldPath        =0; // input old
    const char* hsyni_file     =0; // .hsyni
    const char* hsynz_file_url =0; // .hsynz or newFile's url
    const char* outNewPath     =0; // output new
    TRunAsType runAs=kRunAs_unknown;
    if ((hsyni_file_url!=0)&&(arg_values.size()==1)){
        runAs=kRunAs_download;
        printf("run as: download\n");
        hsyni_file      =arg_values[0];
    }else if ((diff_to_diff_file!=0)&&(diffType!=kSyncDiff_info)){
        _options_check(arg_values.size()==3,"local diff input count");
        runAs=kRunAs_local_diff;
        printf("run as: local diff\n");
        oldPath         =arg_values[0];
        hsyni_file      =arg_values[1];
        hsynz_file_url  =arg_values[2];
    }else if (patch_by_diff_file!=0){
        _options_check(arg_values.size()==3,"local diff input count");
        runAs=kRunAs_local_patch;
        printf("run as: local patch\n");
        oldPath         =arg_values[0];
        hsyni_file      =arg_values[1];
        outNewPath      =arg_values[2];
    }else if (arg_values.size()==2){
        runAs=kRunAs_sync_infos;
        printf("run as: show sync infos\n");
        oldPath         =arg_values[0];
        hsyni_file      =arg_values[1];
    }else if (arg_values.size()==4){
        runAs=kRunAs_sync_patch;
        printf("run as: sync patch\n");
        oldPath         =arg_values[0];
        hsyni_file      =arg_values[1];
        hsynz_file_url  =arg_values[2];
        outNewPath      =arg_values[3];
    }else{
        _options_check(false,"input count");
    }
    
    TSyncDownloadPlugin downloadPlugin; memset(&downloadPlugin,0,sizeof(downloadPlugin));
    if (hsyni_file_url||hsynz_file_url)
        _check(getSyncDownloadPlugin(&downloadPlugin),
               kSyncClient_getSyncDownloadPluginError,"getSyncDownloadPlugin()");
    if (hsyni_file_url){
        if (!isForceOverwrite)
            _check3(hpatch_isPathNotExist(hsyni_file),kSyncClient_overwritePathError,
                    "file \"",hsyni_file,"\" already exists, overwrite");
        double dtime0=clock_s();
        printf(    "download .hsyni: \""); _log_info_utf8(hsyni_file);
        printf("\"\n       from URL: \""); _log_info_utf8(hsyni_file_url);
        printf("\"\n");
        int result;
        _RETRY_DOWNLOAD_DO();
        result=downloadNewSyncInfoFile(&downloadPlugin,hsyni_file_url,hsyni_file,
                                       isUsedDownloadContinue!=0);
        _RETRY_DOWNLOAD_WHILE(result==kSyncClient_newSyncInfoDownloadError);
        _check3(result==kSyncClient_ok,result,"download from url: \"",hsyni_file_url,"\"");
        double dtime1=clock_s();
        printf(    "  download time: %.3f s\n\n",(dtime1-dtime0));
        if (runAs==kRunAs_download)
            return result;
        //else continue sync patch
    }
    double time0=clock_s();
    
    {
        const bool isSamePath=(outNewPath!=0)&&(hpatch_getIsSamePath(oldPath,outNewPath)!=0);
        _check3(!isSamePath,kSyncClient_overwritePathError,
               "not support oldPath outNewPath same path \"",outNewPath,"\"");
    }

    hpatch_TPathType   outNewPathType=kPathType_notExist;
    if (outNewPath)
        _check3(hpatch_getPathStat(outNewPath,&outNewPathType,0),
                kSyncClient_pathTypeError,"get outNewPath \"",outNewPath,"\" type");
    if (!isForceOverwrite)
        _check3(outNewPathType==kPathType_notExist,kSyncClient_overwritePathError,
                "outNewPath \"",outNewPath,"\" already exists, overwrite");
    
    hpatch_TPathType oldPathType;
    if (0!=strcmp(oldPath,"")){ // isOldPathInputEmpty
        _check3(hpatch_getPathStat(oldPath,&oldPathType,0),kSyncClient_pathTypeError,
               "get oldPath \"",oldPath,"\" type");
        _check3((oldPathType!=kPathType_notExist),kSyncClient_pathTypeError,
                "oldPath \"",oldPath,"\" not exist");
    }else{ //same as empty file
        oldPathType=kPathType_file;
    }
    const bool oldIsDir=(oldPathType==kPathType_dir);
    
    const char* localDiffFile=diff_to_diff_file?diff_to_diff_file:patch_by_diff_file;
    if (localDiffFile){
        hpatch_TPathType diffFileType;
        _check3(hpatch_getPathStat(localDiffFile,&diffFileType,0),kSyncClient_pathTypeError,
                      "get diffFile \"",localDiffFile,"\" type");
        if ((localDiffFile==diff_to_diff_file)&&(!isForceOverwrite))
            _check3((diffFileType==kPathType_notExist),kSyncClient_overwritePathError,
                   "diffFile \"",localDiffFile,"\" already exists, overwrite");
        if (localDiffFile==patch_by_diff_file)
            _check3((diffFileType==kPathType_file),kSyncClient_pathTypeError,
                    "diffFile \"",localDiffFile,"\" must exists file, type");
    }
    
    TSyncClient_resultType result=kSyncClient_ok;
    hpatch_BOOL newIsDir=hpatch_FALSE;
    result=checkNewSyncInfoType_by_file(hsyni_file,&newIsDir);
#if (_IS_NEED_ZSYNC)
    if (result==kSyncClient_newSyncInfoTypeError){//try check hsyni_file as zsync file
        newIsDir=hpatch_FALSE;
        TSyncClient_resultType ret=checkNewZsyncInfoType_by_file(hsyni_file);
        if (ret==kSyncClient_ok){
            result=kSyncClient_ok;
            isZsyncType=hpatch_TRUE;
            _check3(!oldIsDir,kSyncClient_pathTypeError,
                    "now zsync patch unsupport oldPath \"",oldPath,"\" is dir, type");
        }
    }
#endif
    _check3(result==kSyncClient_ok,result,
            "check hsyni_file \"",hsyni_file,"\" type");
    if (outNewPath&&(!newIsDir))
        _check3(!hpatch_getIsDirName(outNewPath),kSyncClient_pathTypeError,
                "outNewPath \"",outNewPath,"\" must file, type");
    printf("\n");
    _RETRY_DOWNLOAD_DO();
#if (_IS_NEED_DIR_DIFF_PATCH)
    if (newIsDir)
        result=hsync_patch_2dir(outNewPath,oldPath,oldIsDir,ignoreOldPathList,
                                hsyni_file,&downloadPlugin,hsynz_file_url,
                                localDiffFile,diffType,isUsedDownloadContinue,
                                kStepRangeNumber,kMaxOpenFileNumber,(int)threadNum);
    else
#endif
        result=hsync_patch_2file(outNewPath,oldPath,oldIsDir,ignoreOldPathList,
                                 hsyni_file,&downloadPlugin,hsynz_file_url,
                                 localDiffFile,diffType,isUsedDownloadContinue,
                                 kStepRangeNumber,kMaxOpenFileNumber,isZsyncType,(int)threadNum);
    _RETRY_DOWNLOAD_WHILE((result==kSyncClient_syncDataDownloadError)
                        ||(result==kSyncClient_readSyncDataBeginError)
                        ||(result==kSyncClient_readSyncDataError));
    double time1=clock_s();
    printf("\nhsync patch %s2%s time: %.3f s\n",oldIsDir?"dir":"file",newIsDir?"dir":"file",(time1-time0));
    if (result==kSyncClient_ok) printf("run ok.\n"); else printf("\nERROR!\n\n");
    return result;
}

#if (_IS_NEED_DIR_DIFF_PATCH)
struct DirPathIgnoreListener:public CDirPathIgnore,IDirPathIgnore{
    DirPathIgnoreListener(const std::vector<std::string>& ignorePathList,bool isPrintIgnore=true)
    :CDirPathIgnore(ignorePathList,isPrintIgnore){}
    //IDirPathIgnore
    virtual bool isNeedIgnore(const std::string& path,size_t rootPathNameLen){
        return CDirPathIgnore::isNeedIgnore(path,rootPathNameLen);
    }
};
#endif

#endif //_IS_NEED_CMDLINE

#if (_IS_NEED_DIR_DIFF_PATCH)
bool getManifest(TManifest& out_manifest,const char* _rootPath,bool rootPathIsDir,
                 const std::vector<std::string>& ignorePathList){
    try {
        std::string rootPath(_rootPath);
        if (rootPathIsDir) assignDirTag(rootPath);
        DirPathIgnoreListener pathIgnore(ignorePathList);
        get_manifest(&pathIgnore,rootPath,out_manifest);
        return true;
    } catch (...) {
        return false;
    }
}
#endif


hpatch_inline static
bool getFileSize(const char *path_utf8,hpatch_StreamPos_t* out_fileSize){
    hpatch_TPathType out_type;
    if (!hpatch_getPathStat(path_utf8,&out_type,out_fileSize)) return false;
    return out_type==kPathType_file;
}

static bool printFileInfo(const char *path_utf8,const char *tag,bool isOutSize=true){
#if (_IS_NEED_PRINT_LOG)
    hpatch_StreamPos_t fileSize=0;
    if (!getFileSize(path_utf8,&fileSize)) return false;
    if (isOutSize)
        printf("%s: %" PRIu64 "   \"",tag,fileSize);
    else
        printf("%s: \"",tag);
    _log_info_utf8(path_utf8); printf("\"\n");
#endif
    return true;
}


static const char* _kEmptyStr="";
TSyncClient_resultType
   hsync_patch_2file(const char* outNewFile,const char* oldPath,bool oldIsDir,
                     const std::vector<std::string>& ignoreOldPathList,
                     const char* hsyni_file,IReadSyncDataListener* syncDataListener,
                     const char* localDiffFile,TSyncDiffType diffType,hpatch_BOOL isUsedDownloadContinue,
                     size_t kMaxOpenFileNumber,hpatch_BOOL isZsyncType,int threadNum){
    if (oldPath==0) oldPath=_kEmptyStr;
#if (_IS_NEED_DIR_DIFF_PATCH)
    std::string _oldPath(oldPath); if (oldIsDir) assignDirTag(_oldPath); oldPath=_oldPath.c_str();
#endif
    printFileInfo(oldPath,oldIsDir?"in old dir  ":"in old file ",!oldIsDir);
#if (_IS_NEED_DIR_DIFF_PATCH)
    TManifest oldManifest;
    if (oldIsDir){
        _check3(getManifest(oldManifest,oldPath,oldIsDir,ignoreOldPathList),
                kSyncClient_oldDirOpenError,"open oldPath \"",oldPath,"\"");
    }
#endif
    printFileInfo(hsyni_file,                                          "info .hsyni ");
  #if (_IS_NEED_ZSYNC)
    if (isZsyncType) printf("  hsync patch used .zsync file as .hsyni\n");
  #endif
    if (localDiffFile){
        if      (diffType==kSyncDiff_info) printFileInfo(localDiffFile,"diff  info  ",false);
        else if (diffType==kSyncDiff_data) printFileInfo(localDiffFile,"diff  data  ",false);
        else                               printFileInfo(localDiffFile,"diff  file  ");
    }
    if (outNewFile) printFileInfo(outNewFile,                          "out new file",false);

    ISyncInfoListener listener; memset(&listener,0,sizeof(listener));
    listener.findChecksumPlugin=_findChecksumPlugin;
    listener.findDecompressPlugin=_findDecompressPlugin;
    listener.onNeedSyncInfo=_onNeedSyncInfo;
    TSyncClient_resultType result=kSyncClient_ok;
#if (_IS_NEED_DIR_DIFF_PATCH)
    if (oldIsDir){
        assert(!isZsyncType);
        if ((localDiffFile==0)||(diffType==kSyncDiff_info))
            result=sync_patch_2file(&listener,syncDataListener,oldManifest,hsyni_file,outNewFile,
                                    isUsedDownloadContinue,localDiffFile,kMaxOpenFileNumber,threadNum);
        else if (outNewFile==0) //local diff
            result=sync_local_diff_2file(&listener,syncDataListener,oldManifest,hsyni_file,localDiffFile,
                                         diffType,isUsedDownloadContinue,kMaxOpenFileNumber,threadNum);
        else //local patch
            result=sync_local_patch_2file(&listener,localDiffFile,oldManifest,hsyni_file,outNewFile,
                                          kMaxOpenFileNumber,threadNum);
    }else
#endif
    {
        assert(!oldIsDir);
    #if (_IS_NEED_ZSYNC)
        if (isZsyncType){
            if ((localDiffFile==0)||(diffType==kSyncDiff_info))
                result=zsync_patch_file2file(&listener,syncDataListener,oldPath,hsyni_file,outNewFile,
                                             isUsedDownloadContinue,localDiffFile,threadNum);
            else if (outNewFile==0) //local diff
                result=zsync_local_diff_file2file(&listener,syncDataListener,oldPath,hsyni_file,
                                                 localDiffFile,diffType,isUsedDownloadContinue,threadNum);
            else //local patch
                result=zsync_local_patch_file2file(&listener,localDiffFile,oldPath,hsyni_file,
                                                   outNewFile,isUsedDownloadContinue,threadNum);
        }else
    #endif //_IS_NEED_ZSYNC
        {
            if ((localDiffFile==0)||(diffType==kSyncDiff_info))
                result=sync_patch_file2file(&listener,syncDataListener,oldPath,hsyni_file,outNewFile,
                                            isUsedDownloadContinue,localDiffFile,threadNum);
            else if (outNewFile==0) //local diff
                result=sync_local_diff_file2file(&listener,syncDataListener,oldPath,hsyni_file,
                                                 localDiffFile,diffType,isUsedDownloadContinue,threadNum);
            else //local patch
                result=sync_local_patch_file2file(&listener,localDiffFile,oldPath,hsyni_file,
                                                  outNewFile,isUsedDownloadContinue,threadNum);
        }
    }
    if ((result==kSyncClient_ok)&&localDiffFile&&(diffType!=kSyncDiff_info)&&(outNewFile==0))
        printFileInfo(localDiffFile,"\nout  diff   ");
    if ((result==kSyncClient_ok)&&outNewFile) printFileInfo(outNewFile,    "\nout new file");
    if (result!=kSyncClient_ok) {  LOG_ERR("\nrun throw errorCode: %d\n",result); }
    return result;
}

TSyncClient_resultType
   hsync_patch_2file(const char* outNewFile,const char* oldPath,bool oldIsDir,
                     const std::vector<std::string>& ignoreOldPathList,
                     const char* hsyni_file,const TSyncDownloadPlugin* downloadPlugin,const char* hsynz_file_url,
                     const char* localDiffFile,TSyncDiffType diffType,hpatch_BOOL isUsedDownloadContinue,
                     size_t kStepRangeNumber,size_t kMaxOpenFileNumber,hpatch_BOOL isZsyncType,int threadNum){
    if (hsynz_file_url) printFileInfo(hsynz_file_url,                  "sync  url   ",_IS_SYNC_PATCH_DEMO);

    IReadSyncDataListener syncDataListener; memset(&syncDataListener,0,sizeof(syncDataListener));
    if (hsynz_file_url)
        _check3(downloadPlugin->download_range_open(&syncDataListener,hsynz_file_url,kStepRangeNumber),
                kSyncClient_syncDataDownloadError,"download open sync file \"",hsynz_file_url,"\"");
    TSyncClient_resultType result=hsync_patch_2file(outNewFile,oldPath,oldIsDir,ignoreOldPathList,
                                                    hsyni_file,&syncDataListener,
                                                    localDiffFile,diffType,isUsedDownloadContinue,
                                                    kMaxOpenFileNumber,isZsyncType,threadNum);
    if (hsynz_file_url)
        _check3(downloadPlugin->download_range_close(&syncDataListener),
                (result!=kSyncClient_ok)?result:kSyncClient_syncDataCloseError,
                "close hsynz_file \"",hsynz_file_url,"\"");
    return result;
}

#if (_IS_NEED_DIR_DIFF_PATCH)
        
struct TDirSyncPatchListener:public IDirSyncPatchListener{
    const TManifest*    oldManifest;
    const std::string*  newDirRoot;
    std::string         oldPathTemp;
};
static const char* TDirSyncPatchListener_getOldPathByNewPath(TDirSyncPatchListener* self,const char* newPath){
    const size_t newPathLen=strlen(newPath);
    const size_t newDirRootLen=self->newDirRoot->size();
    assert(newPathLen>=newDirRootLen);
    assert(0==memcmp(newPath,self->newDirRoot->c_str(),newDirRootLen));
    self->oldPathTemp.assign(self->oldManifest->rootPath);
    self->oldPathTemp.insert(self->oldPathTemp.end(),newPath+newDirRootLen,newPath+newPathLen);
    return self->oldPathTemp.c_str();
}
static const char* TDirSyncPatchListener_getOldPathByIndex(TDirSyncPatchListener* self,size_t oldIndex){
    assert(oldIndex<self->oldManifest->pathList.size());
    return self->oldManifest->pathList[oldIndex].c_str();
}
    
    static hpatch_BOOL _dirSyncPatch_setIsExecuteFile(TNewDirOutput* newDirOutput){
        hpatch_BOOL result=hpatch_TRUE;
        for (size_t i=0;i<newDirOutput->newExecuteCount;++i){
            const char* executeFileName=TNewDirOutput_getNewExecuteFileByIndex(newDirOutput,i);
            if (!hpatch_setIsExecuteFile(executeFileName)){
                result=hpatch_FALSE;
                printf("WARNING: can't set Execute tag to new file \"");
                _log_info_utf8(executeFileName); printf("\"\n");
            }
        }
        return result;
    }
static hpatch_BOOL _dirSyncPatchFinish(IDirSyncPatchListener* listener,hpatch_BOOL isPatchSuccess,
                                       const TNewDataSyncInfo* newSyncInfo,TNewDirOutput* newDirOutput){
    TDirSyncPatchListener* self=(TDirSyncPatchListener*)listener->patchImport;
    //can do your works
    if (isPatchSuccess){
        if (self->newDirRoot){
            _dirSyncPatch_setIsExecuteFile(newDirOutput);
        }
    }
    return hpatch_TRUE;
}

TSyncClient_resultType
    hsync_patch_2dir(const char* outNewDir,const char* oldPath,bool oldIsDir,
                     const std::vector<std::string>& ignoreOldPathList,
                     const char* hsyni_file,const TSyncDownloadPlugin* downloadPlugin,const char* hsynz_file_url,
                     const char* localDiffFile,TSyncDiffType diffType,hpatch_BOOL isUsedDownloadContinue,
                     size_t kStepRangeNumber,size_t kMaxOpenFileNumber,int threadNum){
    if (oldPath==0) oldPath=_kEmptyStr;
    std::string _outNewDir(outNewDir?outNewDir:"");
    if (outNewDir) { assignDirTag(_outNewDir); outNewDir=outNewDir?_outNewDir.c_str():0; }
    std::string _oldPath(oldPath); if (oldIsDir) assignDirTag(_oldPath); oldPath=_oldPath.c_str();

    printFileInfo(oldPath,oldIsDir?"in old dir ":"in old file",!oldIsDir);
    TManifest oldManifest;
    {
        _check3(getManifest(oldManifest,oldPath,oldIsDir,ignoreOldPathList),
                kSyncClient_oldDirOpenError,"open oldPath \"",oldPath,"\"");
    }
    
    printFileInfo(hsyni_file,                                          "info .hsyni");
    if (hsynz_file_url) printFileInfo(hsynz_file_url,                  "sync  url  ",false);
    if (localDiffFile){
        if      (diffType==kSyncDiff_info) printFileInfo(localDiffFile,"diff  info ",false);
        else if (diffType==kSyncDiff_data) printFileInfo(localDiffFile,"diff  data ",false);
        else                               printFileInfo(localDiffFile,"diff  file ");
    }
    if (outNewDir) printFileInfo(outNewDir,                            "out new dir",false);
    
    IDirPatchListener     defaultPatchDirlistener={0,_makeNewDir,_copySameFile,_openNewFile,_closeNewFile};
    TDirSyncPatchListener listener; memset(&listener,0,sizeof(listener));
    IReadSyncDataListener syncDataListener; memset(&syncDataListener,0,sizeof(syncDataListener));
    listener.findChecksumPlugin=_findChecksumPlugin;
    listener.findDecompressPlugin=_findDecompressPlugin;
    listener.onNeedSyncInfo=_onNeedSyncInfo;
    
    listener.patchImport=&listener;
    listener.newDirRoot=outNewDir?&_outNewDir:0;
    listener.oldManifest=&oldManifest;
    listener.patchFinish=_dirSyncPatchFinish;
    
    if (hsynz_file_url)
        _check3(downloadPlugin->download_range_open(&syncDataListener,hsynz_file_url,kStepRangeNumber),
                kSyncClient_syncDataDownloadError,"download open sync file \"",hsynz_file_url,"\"");
    TSyncClient_resultType result=kSyncClient_ok;
    if ((localDiffFile==0)||(diffType==kSyncDiff_info)){
        if (isUsedDownloadContinue)
            printf("  NOTE: sync_patch_2dir() unsupport download continue;"
                   " you can add downloaded files to old data, same as download continue;"
                   " or use sync_local_diff_2dir() to support download continue.\n");
        result=sync_patch_2dir(&defaultPatchDirlistener,&listener,&syncDataListener,
                               oldManifest,hsyni_file,outNewDir,localDiffFile,kMaxOpenFileNumber,threadNum);
    }else if (outNewDir==0){ //local diff
        result=sync_local_diff_2dir(&defaultPatchDirlistener,&listener,&syncDataListener,
                                    oldManifest,hsyni_file,localDiffFile,diffType,
                                    isUsedDownloadContinue,kMaxOpenFileNumber,threadNum);
    }else{ //local patch
        result=sync_local_patch_2dir(&defaultPatchDirlistener,&listener,localDiffFile,
                                     oldManifest,hsyni_file,outNewDir,kMaxOpenFileNumber,threadNum);
    }
    if (hsynz_file_url)
        _check3(downloadPlugin->download_range_close(&syncDataListener),
                (result!=kSyncClient_ok)?result:kSyncClient_syncDataCloseError,
                "close hsynz_file \"",hsynz_file_url,"\"");
    if ((result==kSyncClient_ok)&&localDiffFile&&(diffType!=kSyncDiff_info)&&(outNewDir==0))
        printFileInfo(localDiffFile,"\nout  diff   ");
    if (result!=kSyncClient_ok) {  LOG_ERR("\nrun throw errorCode: %d\n",result); }
    return result;
}
#endif


static TSyncClient_resultType
           downloadNewSyncInfoFile(const TSyncDownloadPlugin* downloadPlugin,const char* hsyni_file_url,
                                   const char* out_hsyni_file,bool isUsedDownloadContinue){
    TSyncClient_resultType result=kSyncClient_ok;
    hpatch_TFileStreamOutput out_stream;
    hpatch_TFileStreamOutput_init(&out_stream);
    if (isUsedDownloadContinue&&hpatch_isPathExist(out_hsyni_file)){ // download continue
        printf("  download continue ");
        if (!hpatch_TFileStreamOutput_reopen(&out_stream,out_hsyni_file,(hpatch_StreamPos_t)(-1)))
            return kSyncClient_newSyncInfoCreateError;
        printf("at file pos: %" PRIu64 "\n",out_stream.out_length);
    }else{
        if (!hpatch_TFileStreamOutput_open(&out_stream,out_hsyni_file,(hpatch_StreamPos_t)(-1)))
            return kSyncClient_newSyncInfoCreateError;
    }
    //hpatch_TFileStreamOutput_setRandomOut(&out_stream,hpatch_TRUE);
    hpatch_StreamPos_t continueDownloadPos=out_stream.out_length;
    if (downloadPlugin->download_file(hsyni_file_url,&out_stream.base,continueDownloadPos))
        out_stream.base.streamSize=out_stream.out_length;
    else
        result=kSyncClient_newSyncInfoDownloadError;
    if (!hpatch_TFileStreamOutput_close(&out_stream)){
        if (result==kSyncClient_ok)
            result=kSyncClient_newSyncInfoCloseError;
    }
    return result;
}
