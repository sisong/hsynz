//  hsync_demo.cpp
//  client for sync patch
//      like zsync : http://zsync.moria.org.uk/
//  Created by housisong on 2019-09-18.
/*
 The MIT License (MIT)
 Copyright (c) 2019-2023 HouSisong
 
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
#ifndef _IS_NEED_MAIN
#   define  _IS_NEED_MAIN 1
#endif

#ifndef _IS_SYNC_PATCH_DEMO
#   define  _IS_SYNC_PATCH_DEMO 1
#endif

#if (_IS_SYNC_PATCH_DEMO)
//simple file demo plugin
#   include "client_download_demo.h"
#   include "hsync_import_patch.h"
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
#   define _CompressPlugin_zlib
#   define _CompressPlugin_zstd
#endif

#include "dict_decompress_plugin_demo.h"

#ifndef _IS_NEED_DEFAULT_ChecksumPlugin
#   define _IS_NEED_DEFAULT_ChecksumPlugin 1
#endif
#if (_IS_NEED_DEFAULT_ChecksumPlugin)
//===== select needs checksum plugins or change to your plugin=====
#   define _ChecksumPlugin_blake3
#   define _ChecksumPlugin_curl_md5
#   define _ChecksumPlugin_curl_sha256
#   define _ChecksumPlugin_crc32
#endif
#include "HDiffPatch/checksum_plugin_demo.h"

#if (_IS_SYNC_PATCH_DEMO)
#   define _NOTE_TEXT_URL  "test"
#   define _NOTE_TEXT_APP  "hsync::hsync_demo"
#endif


static void printVersion(){
    printf(_NOTE_TEXT_APP " v" HSYNC_VERSION_STRING "\n\n");
}

static void printUsage(){
    printVersion();
    printf("sync  patch: [options] oldPath [-dl#hsyni_file_" _NOTE_TEXT_URL "] hsyni_file hsynz_file_" _NOTE_TEXT_URL " outNewPath\n"
           "download   : [options] -dl#hsyni_file_" _NOTE_TEXT_URL " hsyni_file\n"
           "local  diff: [options] oldPath hsyni_file hsynz_file_" _NOTE_TEXT_URL " -diff#diffFile\n"
           "local patch: [options] oldPath hsyni_file -patch#diffFile outNewPath\n"
           "sync  infos: [options] oldPath hsyni_file\n"
#if (_IS_NEED_DIR_DIFF_PATCH)
           "  oldPath can be file or directory(folder),\n"
#endif
           "  if oldPath is empty input parameter \"\"\n"
           "options:\n"
           "  -dl#hsyni_file_" _NOTE_TEXT_URL "\n"
           "    download hsyni_file from hsyni_file_" _NOTE_TEXT_URL " befor sync patch;\n"
           "  -diff#outDiffFile\n"
           "    create diffFile from ranges of hsynz_file_" _NOTE_TEXT_URL " befor local patch;\n"
           "  -patch#diffFile\n"
           "    local patch(oldPath+diffFile) to outNewPath;\n"
           "  -cdl\n"
           "    continue download data from breakpoint;\n"
           "    DEFAULT continue download mode is closed;\n"
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

TSyncClient_resultType 
    hsync_patch_2file(const char* outNewFile,const char* oldPath,bool oldIsDir,
                      const std::vector<std::string>& ignoreOldPathList,
                      const char* hsyni_file,const char* hsynz_file_url,
                      const char* localDiffFile,hpatch_BOOL isMinDiff,hpatch_BOOL isUsedDownloadContinue,
                      const TSyncDownloadPlugin* downloadPlugin,size_t kMaxOpenFileNumber,int threadNum);
#if (_IS_NEED_DIR_DIFF_PATCH)
TSyncClient_resultType
    hsync_patch_2dir(const char* outNewDir,const char* oldPath,bool oldIsDir,
                     const std::vector<std::string>& ignoreOldPathList,
                     const char* hsyni_file,const char* hsynz_file_url,
                     const char* localDiffFile,hpatch_BOOL isMinDiff,hpatch_BOOL isUsedDownloadContinue,
                     const TSyncDownloadPlugin* downloadPlugin,size_t kMaxOpenFileNumber,int threadNum);
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

static int _dictSizeToDictBits(size_t dictSize){
    int bits=1;
    while ((((size_t)1)<<bits)<dictSize){
        ++bits;
        if (bits==sizeof(size_t)*8) break;
    }
    return bits;
}

//ISyncInfoListener::findDecompressPlugin
static hsync_TDictDecompress* _findDecompressPlugin(ISyncInfoListener* listener,const char* compressType,size_t dictSize){
    if (compressType==0) return 0; //ok
    hsync_TDictDecompress* decompressPlugin=0;
#ifdef  _CompressPlugin_zlib
    if ((!decompressPlugin)&&zlibDictDecompressPlugin.base.is_can_open(compressType)){
        static TDictDecompressPlugin_zlib _zlibDictDecompressPlugin=zlibDictDecompressPlugin;
        _zlibDictDecompressPlugin.dict_bits=(hpatch_byte)_dictSizeToDictBits(dictSize);
        decompressPlugin=&_zlibDictDecompressPlugin.base;
    }
#endif
#ifdef  _CompressPlugin_zstd
    if ((!decompressPlugin)&&zstdDictDecompressPlugin.base.is_can_open(compressType)){
        static TDictDecompressPlugin_zstd _zstdDictDecompressPlugin=zstdDictDecompressPlugin;
        _zstdDictDecompressPlugin.dictSize=dictSize;
        decompressPlugin=&_zstdDictDecompressPlugin.base;
    }
#endif
    if (decompressPlugin==0){
        printf("  sync_patch can't decompress type: \"%s\"\n",compressType);
        return 0; //unsupport error
    }else{
        printf("  sync_patch run with decompress plugin: \"%s\"\n",compressType);
        return decompressPlugin; //ok
    }
}
//ISyncInfoListener::findChecksumPlugin
static hpatch_TChecksum* _findChecksumPlugin(ISyncInfoListener* listener,const char* strongChecksumType){
    assert((strongChecksumType!=0)&&(strlen(strongChecksumType)>0));
    hpatch_TChecksum* strongChecksumPlugin=0;
#ifdef  _ChecksumPlugin_blake3
    if ((!strongChecksumPlugin)&&(0==strcmp(strongChecksumType,blake3ChecksumPlugin.checksumType())))
        strongChecksumPlugin=&blake3ChecksumPlugin;
#endif
#ifdef  _ChecksumPlugin_curl_md5
    if ((!strongChecksumPlugin)&&(0==strcmp(strongChecksumType,md5ChecksumPlugin.checksumType())))
        strongChecksumPlugin=&md5ChecksumPlugin;
#endif
#ifdef  _ChecksumPlugin_curl_sha256
    if ((!strongChecksumPlugin)&&(0==strcmp(strongChecksumType,sha256ChecksumPlugin.checksumType())))
        strongChecksumPlugin=&sha256ChecksumPlugin;
#endif
#ifdef  _ChecksumPlugin_crc32
    if ((!strongChecksumPlugin)&&(0==strcmp(strongChecksumType,crc32ChecksumPlugin.checksumType())))
        strongChecksumPlugin=&crc32ChecksumPlugin;
#endif
    if (strongChecksumPlugin==0){
        printf("  sync_patch can't found checksum type: \"%s\"\n",strongChecksumType);
        return 0; //unsupport error
    }else{
        printf("  sync_patch run with strongChecksum plugin: \"%s\"\n",strongChecksumType);
        return strongChecksumPlugin; //ok
    }
}

    static void printMatchResult(const TNeedSyncInfos* nsi) {
        const uint32_t kBlockCount=nsi->blockCount;
        printf("  syncBlockCount: %d, /%d=%.1f%%\n  localDataSize : %" PRIu64 "\n  syncDataSize  : %" PRIu64 "\n",
               nsi->needSyncBlockCount,kBlockCount,100.0*nsi->needSyncBlockCount/kBlockCount,
               nsi->newSyncDataSize-nsi->needSyncSumSize,nsi->needSyncSumSize);
        hpatch_StreamPos_t downloadSize=nsi->newSyncInfoSize+nsi->needSyncSumSize;
        printf("  downloadSize  : %" PRIu64 "+%" PRIu64 "= %" PRIu64 ", /%" PRIu64 "=%.1f%%",
               nsi->newSyncInfoSize,nsi->needSyncSumSize,downloadSize,
               nsi->newDataSize,100.0*downloadSize/nsi->newDataSize);
        if (nsi->newSyncDataSize<nsi->newDataSize){
            hpatch_StreamPos_t maxDownloadSize=nsi->newSyncInfoSize+nsi->newSyncDataSize;
            printf(" (/%" PRIu64 "=%.1f%%)",maxDownloadSize,100.0*downloadSize/maxDownloadSize);
        }
        printf("\n");
        if (nsi->localDiffDataSize>0){
            printf("\n  localDiffDataSize    : %" PRIu64 "\n",nsi->localDiffDataSize);
            printf(  "  continue downloadSize: %" PRIu64 ", /%" PRIu64 "=%.1f%%\n",
                   nsi->needSyncSumSize-nsi->localDiffDataSize,downloadSize,
                   100.0*(nsi->needSyncSumSize-nsi->localDiffDataSize)/downloadSize);
        }
    }
//ISyncInfoListener::needSyncInfo
static void _needSyncInfo(ISyncInfoListener* listener,const TNeedSyncInfos* needSyncInfo){
    printMatchResult(needSyncInfo);
}

#define _options_check(value,errorInfo) do{ \
    if (!(value)) { LOG_ERR("options " errorInfo " ERROR!\n\n"); \
                    printUsage(); return kSyncClient_optionsError; } }while(0)
#define _pferr(errorInfo) do{ hpatch_printStdErrPath_utf8(errorInfo); }while(0)
#define _check(value,exitCode,errInfo) do{ \
    if (!(value)) { _pferr(errInfo); LOG_ERR(" ERROR!\n"); return exitCode; } }while(0)
#define _check3(value,exitCode,errInfo0,errInfo1,errInfo2) do{ \
if (!(value)) { _pferr(errInfo0); _pferr(errInfo1); _pferr(errInfo2); LOG_ERR(" ERROR!\n"); return exitCode; } }while(0)

#define _kNULL_VALUE    (-1)
#define _kNULL_SIZE     (~(size_t)0)

#define _THREAD_NUMBER_NULL     0
#define _THREAD_NUMBER_MIN      1
#define _THREAD_NUMBER_DEFUALT  4
#define _THREAD_NUMBER_MAX      (1<<8)

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

//return TSyncClient_resultType
int sync_client_cmd_line(int argc, const char * argv[]) {
    size_t      threadNum=_THREAD_NUMBER_NULL;
    hpatch_BOOL isForceOverwrite=_kNULL_VALUE;
    hpatch_BOOL isOutputHelp=_kNULL_VALUE;
    hpatch_BOOL isOutputVersion=_kNULL_VALUE;
    hpatch_BOOL isOldPathInputEmpty=_kNULL_VALUE;
    hpatch_BOOL isUsedDownloadContinue=_kNULL_VALUE;
    hpatch_BOOL isMinDiff=hpatch_FALSE;
    const char* hsyni_file_url=0;
    const char* diff_to_diff_file=0;
    const char* patch_by_diff_file=0;
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
                if ((strcmp(op,"-cdl")==0)||(strcmp(op,"-cdl-on")==0)||(strcmp(op,"-cdl-opened")==0)){
                    _options_check(isUsedDownloadContinue==_kNULL_VALUE,"-cdl?");
                    isUsedDownloadContinue=hpatch_TRUE;
                }else if ((strcmp(op,"-cdl-off")==0)||(strcmp(op,"-cdl-closed")==0)){
                    _options_check(isUsedDownloadContinue==_kNULL_VALUE,"-cdl?");
                    isUsedDownloadContinue=hpatch_FALSE;
                }else{
                    _options_check(false,"-cdl?");
                }
            } break;
            case 'd':{
                if (strstr(op,"-diff")==op){
                    if (strstr(op,"-diff#")==op){
                        const char* pfname=op+6;
                        _options_check((diff_to_diff_file==0)&&(strlen(pfname)>0),"-diff#?");
                        diff_to_diff_file=pfname;
                    }else if (strstr(op,"-diff_min#")==op){ //now hidden option
                        const char* pfname=op+10;
                        _options_check((diff_to_diff_file==0)&&(strlen(pfname)>0),"-diff_min#?");
                        diff_to_diff_file=pfname;
                        isMinDiff=hpatch_TRUE; //NOTE: now, will run as single-thread
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
        isUsedDownloadContinue=hpatch_FALSE;
    
    if (isOutputHelp||isOutputVersion){
        if (isOutputHelp)
            printUsage();//with version
        else
            printVersion();
        if (arg_values.empty())
            return kSyncClient_ok; //ok
    }
    if (threadNum>1)
        printf("muti-thread parallel: opened, threadNum: %d\n",(int)threadNum);
    printf("continue download: %s\n",isUsedDownloadContinue?"opened":"closed");

    _options_check((hsyni_file_url?1:0)+(diff_to_diff_file?1:0)+(patch_by_diff_file?1:0)<=1,
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
    }else if (diff_to_diff_file!=0){
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
        printf(    "download .hsyni: \""); hpatch_printPath_utf8(hsyni_file);
        printf("\"\n       from URL: \""); hpatch_printPath_utf8(hsyni_file_url);
        printf("\"\n");
        int result=downloadNewSyncInfoFile(&downloadPlugin,hsyni_file_url,hsyni_file,
                                           isUsedDownloadContinue!=0);
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
        if (diff_to_diff_file&&(!isForceOverwrite))
            _check3((diffFileType==kPathType_notExist),kSyncClient_overwritePathError,
                   "diffFile \"",localDiffFile,"\" already exists, overwrite");
        if (patch_by_diff_file)
            _check3((diffFileType==kPathType_file),kSyncClient_pathTypeError,
                    "diffFile \"",localDiffFile,"\" must exists file, type");
    }
    
    TSyncClient_resultType result=kSyncClient_ok;
    hpatch_BOOL newIsDir=hpatch_FALSE;
    result=checkNewSyncInfoType_by_file(hsyni_file,&newIsDir);
    _check3(result==kSyncClient_ok,result,
            "check hsyni_file \"",hsyni_file,"\" type");
    if (outNewPath&&(!newIsDir))
        _check3(!hpatch_getIsDirName(outNewPath),kSyncClient_pathTypeError,
                "outNewPath \"",outNewPath,"\" must file, type");

#if (_IS_NEED_DIR_DIFF_PATCH)
    if (newIsDir)
        result=hsync_patch_2dir(outNewPath,oldPath,oldIsDir,ignoreOldPathList,
                                hsyni_file,hsynz_file_url,
                                localDiffFile,isMinDiff,isUsedDownloadContinue,
                                &downloadPlugin,kMaxOpenFileNumber,(int)threadNum);
    else
#endif
        result=hsync_patch_2file(outNewPath,oldPath,oldIsDir,ignoreOldPathList,
                                 hsyni_file,hsynz_file_url,
                                 localDiffFile,isMinDiff,isUsedDownloadContinue,
                                 &downloadPlugin,kMaxOpenFileNumber,(int)threadNum);
    double time1=clock_s();
    printf("\nsync_patch_%s2%s time: %.3f s\n",oldIsDir?"dir":"file",newIsDir?"dir":"file",(time1-time0));
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

static bool printFileInfo(const char *path_utf8,const char *tag,bool isOutSize=true,hpatch_StreamPos_t* out_fileSize=0){
    hpatch_StreamPos_t fileSize=0;
    if (!getFileSize(path_utf8,&fileSize)) return false;
    if (isOutSize)
        printf("%s: %" PRIu64 "   \"",tag,fileSize);
    else
        printf("%s: \"",tag);
    hpatch_printPath_utf8(path_utf8); printf("\"\n");
    if (out_fileSize) *out_fileSize=fileSize;
    return true;
}

TSyncClient_resultType
   hsync_patch_2file(const char* outNewFile,const char* oldPath,bool oldIsDir,
                     const std::vector<std::string>& ignoreOldPathList,
                     const char* hsyni_file,const char* hsynz_file_url,
                     const char* localDiffFile,hpatch_BOOL isMinDiff,hpatch_BOOL isUsedDownloadContinue,
                     const TSyncDownloadPlugin* downloadPlugin,size_t kMaxOpenFileNumber,int threadNum){
#if (_IS_NEED_DIR_DIFF_PATCH)
    std::string _oldPath(oldPath); if (oldIsDir) assignDirTag(_oldPath); oldPath=_oldPath.c_str();
#endif
    printf("\n");
    printFileInfo(oldPath,oldIsDir?"in old dir  ":"in old file ",!oldIsDir);
#if (_IS_NEED_DIR_DIFF_PATCH)
    TManifest oldManifest;
    if (oldIsDir){
        _check3(getManifest(oldManifest,oldPath,oldIsDir,ignoreOldPathList),
                kSyncClient_oldDirOpenError,"open oldPath \"",oldPath,"\"");
        _oldPath=oldManifest.rootPath.c_str();
    }
#endif
    printFileInfo(hsyni_file,                                       "info .hsyni ");
    if (hsynz_file_url) printFileInfo(hsynz_file_url,               "sync  url   ",false);
    if (localDiffFile&&(outNewFile==0)) printFileInfo(localDiffFile,"out  diff   ",false);
    if (localDiffFile&&(outNewFile!=0)) printFileInfo(localDiffFile,"in   diff   ");
    if (outNewFile) printFileInfo(outNewFile,                       "out new file",false);

    ISyncInfoListener listener; memset(&listener,0,sizeof(listener));
    IReadSyncDataListener syncDataListener; memset(&syncDataListener,0,sizeof(syncDataListener));
    listener.findChecksumPlugin=_findChecksumPlugin;
    listener.findDecompressPlugin=_findDecompressPlugin;
    listener.needSyncInfo=_needSyncInfo;
    if (hsynz_file_url)
        _check3(downloadPlugin->download_range_open(&syncDataListener,hsynz_file_url),
                kSyncClient_syncDataDownloadError,"download open sync file \"",hsynz_file_url,"\"");
    TSyncClient_resultType result=kSyncClient_ok;
#if (_IS_NEED_DIR_DIFF_PATCH)
    if (oldIsDir){
        if (localDiffFile==0)
            result=sync_patch_2file(&listener,&syncDataListener,oldManifest,hsyni_file,outNewFile,
                                    isUsedDownloadContinue,kMaxOpenFileNumber,threadNum);
        else if (outNewFile==0) //local diff
            result=sync_local_diff_2file(&listener,&syncDataListener,oldManifest,hsyni_file,localDiffFile,
                                         isMinDiff,isUsedDownloadContinue,kMaxOpenFileNumber,threadNum);
        else //local patch
            result=sync_local_patch_2file(&listener,localDiffFile,oldManifest,hsyni_file,outNewFile,
                                          kMaxOpenFileNumber,threadNum);
    }else
#endif
    {
        assert(!oldIsDir);
        if (localDiffFile==0)
            result=sync_patch_file2file(&listener,&syncDataListener,oldPath,hsyni_file,outNewFile,
                                        isUsedDownloadContinue,threadNum);
        else if (outNewFile==0) //local diff
            result=sync_local_diff_file2file(&listener,&syncDataListener,oldPath,hsyni_file,
                                             localDiffFile,isMinDiff,isUsedDownloadContinue,threadNum);
        else //local patch
            result=sync_local_patch_file2file(&listener,localDiffFile,oldPath,hsyni_file,
                                              outNewFile,isUsedDownloadContinue,threadNum);
    }
    if (hsynz_file_url)
        _check3(downloadPlugin->download_range_close(&syncDataListener),
                (result!=kSyncClient_ok)?result:kSyncClient_syncDataCloseError,
                "close hsynz_file \"",hsynz_file_url,"\"");
    if ((result==kSyncClient_ok)&&localDiffFile&&(outNewFile==0)) printFileInfo(localDiffFile,"\nout  diff   ");
    if ((result==kSyncClient_ok)&&outNewFile) printFileInfo(outNewFile,    "\nout new file");
    if (result!=kSyncClient_ok) {  LOG_ERR("\nrun throw errorCode: %d\n",result); }
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
                hpatch_printPath_utf8(executeFileName); printf("\"\n");
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
                     const char* hsyni_file,const char* hsynz_file_url,
                     const char* localDiffFile,hpatch_BOOL isMinDiff,hpatch_BOOL isUsedDownloadContinue,
                     const TSyncDownloadPlugin* downloadPlugin,size_t kMaxOpenFileNumber,int threadNum){
    std::string _outNewDir(outNewDir?outNewDir:"");
    if (outNewDir) { assignDirTag(_outNewDir); outNewDir=outNewDir?_outNewDir.c_str():0; }
    std::string _oldPath(oldPath); if (oldIsDir) assignDirTag(_oldPath); oldPath=_oldPath.c_str();
    printf("\n");
    printFileInfo(oldPath,oldIsDir?"in old dir ":"in old file",!oldIsDir);
    TManifest oldManifest;
    {
        _check3(getManifest(oldManifest,oldPath,oldIsDir,ignoreOldPathList),
                kSyncClient_oldDirOpenError,"open oldPath \"",oldPath,"\"");
        _oldPath=oldManifest.rootPath.c_str();
    }
    
    printFileInfo(hsyni_file,                                      "info .hsyni");
    if (hsynz_file_url) printFileInfo(hsynz_file_url,              "sync  url  ",false);
    if (localDiffFile&&(outNewDir==0)) printFileInfo(localDiffFile,"out  diff  ",false);
    if (localDiffFile&&(outNewDir!=0)) printFileInfo(localDiffFile,"in   diff  ");
    if (outNewDir) printFileInfo(outNewDir,                        "out new dir",false);
    
    IDirPatchListener     defaultPatchDirlistener={0,_makeNewDir,_copySameFile,_openNewFile,_closeNewFile};
    TDirSyncPatchListener listener; memset(&listener,0,sizeof(listener));
    IReadSyncDataListener syncDataListener; memset(&syncDataListener,0,sizeof(syncDataListener));
    listener.findChecksumPlugin=_findChecksumPlugin;
    listener.findDecompressPlugin=_findDecompressPlugin;
    listener.needSyncInfo=_needSyncInfo;
    
    listener.patchImport=&listener;
    listener.newDirRoot=outNewDir?&_outNewDir:0;
    listener.oldManifest=&oldManifest;
    listener.patchFinish=_dirSyncPatchFinish;
    
    if (hsynz_file_url)
        _check3(downloadPlugin->download_range_open(&syncDataListener,hsynz_file_url),
                kSyncClient_syncDataDownloadError,"download open sync file \"",hsynz_file_url,"\"");
    TSyncClient_resultType result=kSyncClient_ok;
    if (localDiffFile==0){
        if (isUsedDownloadContinue)
            printf("  NOTE: sync_patch_2dir() unsupport download continue;"
                   " you can add downloaded files to old data, same as download continue;"
                   " or use sync_local_diff_2dir() to support download continue.\n");
        result=sync_patch_2dir(&defaultPatchDirlistener,&listener,&syncDataListener,
                               oldManifest,hsyni_file,outNewDir,kMaxOpenFileNumber,threadNum);
    }else if (outNewDir==0){ //local diff
        result=sync_local_diff_2dir(&defaultPatchDirlistener,&listener,&syncDataListener,
                                    oldManifest,hsyni_file,localDiffFile,isMinDiff,
                                    isUsedDownloadContinue,kMaxOpenFileNumber,threadNum);
    }else{ //local patch
        result=sync_local_patch_2dir(&defaultPatchDirlistener,&listener,localDiffFile,
                                     oldManifest,hsyni_file,outNewDir,kMaxOpenFileNumber,threadNum);
    }
    if (hsynz_file_url)
        _check3(downloadPlugin->download_range_close(&syncDataListener),
                (result!=kSyncClient_ok)?result:kSyncClient_syncDataCloseError,
                "close hsynz_file \"",hsynz_file_url,"\"");
    if ((result==kSyncClient_ok)&&localDiffFile&&(outNewDir==0)) printFileInfo(localDiffFile,"\nout  diff  ");
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
