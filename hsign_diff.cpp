//  hsign_diff.cpp
//  hsign_diff: create diffFile between oldData & newData, only used oldData's hsyni_file & newData, not need oldData;
//              hsyni_file is hash signature file created by hsync_make cmdline for oldData;
//              NOTE: apply diffFile used hpatchz cmdline, newData=hpatchz(oldData,diffFile);
//  Created by housisong on 2025-04-10.
/*
 The MIT License (MIT)
 Copyright (c) 2025 HouSisong
 
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
#include <stdexcept>
#include <vector>
#include <string>
#include "HDiffPatch/_clock_for_demo.h"
#include "HDiffPatch/_atosize.h"
#include "HDiffPatch/file_for_patch.h"
#include "HDiffPatch/libHDiffPatch/HDiff/private_diff/mem_buf.h"
#include "HDiffPatch/libParallel/parallel_import.h"

#include "HDiffPatch/libhsync/sync_client/sync_info_client.h"
#include "HDiffPatch/libhsync/sync_client/sync_client_type_private.h"
#include "HDiffPatch/libhsync/sign_diff/sign_diff.h"
#include "hsync_import_patch.h" //for HSYNC_VERSION_STRING

#ifndef _IS_NEED_MAIN
#   define  _IS_NEED_MAIN 1
#endif

#ifndef _IS_NEED_DEFAULT_CompressPlugin
#   define _IS_NEED_DEFAULT_CompressPlugin 1
#endif
#if (_IS_NEED_DEFAULT_CompressPlugin)
//===== select needs decompress plugins or change to your plugin=====
#   define _CompressPlugin_zlib
#   define _CompressPlugin_ldef // compatible with zlib's deflate encoding
#   define _CompressPlugin_zstd
#endif

#ifdef _CompressPlugin_ldef
#   define _IS_USED_LIBDEFLATE_CRC32    1
#endif


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
#endif

#include "HDiffPatch/compress_plugin_demo.h"
#include "dict_decompress_plugin_demo.h"
#include "HDiffPatch/checksum_plugin_demo.h"


static void printVersion(){
    printf("hsynz::hsign_diff v" HSYNC_VERSION_STRING "\n\n");
}

static void printUsage(){
    printVersion();
    printf("hsign_diff: [options] old_hsyni_file newFile outDiffFile\n"
           "  create outDiffFile compatible with hpatchz;\n"
           "  NOTE: must use $hpatchz when patching, old must a file & not compressed .hsynz file.\n"
           "options:\n"
           "  -c-compressType[-compressLevel]\n"
           "      set outDiffFile Compress type & level, DEFAULT uncompress;\n"
           "      support compress type & level:\n"
#ifdef _CompressPlugin_zlib
           "        -c-zlib[-{1..9}]                DEFAULT level 6\n"
#endif
#ifdef _CompressPlugin_ldef
           "        -c-ldef[-{1..12}]               DEFAULT level 9\n"
           "            compress by libdeflate, compatible with -c-zlib, \n"
           "            faster or better compress than zlib;\n"
#else
#endif
#ifdef _CompressPlugin_zstd
           "        -c-zstd[-{0..22}[-dictBits]]    DEFAULT level 15\n"
           "            dictBits can 15--30, DEFAULT 25.\n"
#endif
           "  -SD[-stepSize]\n"
           "      stepSize>=" _HDIFFPATCH_EXPAND_AND_QUOTE(hpatch_kStreamCacheSize) ", DEFAULT -SD-256k, recommended 64k,2m etc...\n"
#if (_IS_USED_MULTITHREAD)
           "  -p-parallelThreadNumber\n"
           "      DEFAULT -p-4;\n"
           "      if parallelThreadNumber>1 then open multi-thread Parallel mode;\n"
#endif
           "  -f  Force overwrite, ignore write path already exists;\n"
           "      DEFAULT (no -f) not overwrite and then return error;\n"
           "      if used -f and write path is exist directory, will always return error.\n"
           "  -v  output Version info.\n"
           "  -h or -?\n"
           "      output Help info (this usage).\n\n"
           );
}

typedef enum THSignDiffResult { //+ TSyncClient_resultType
    kSignDiff_ok=0,
    kSignDiff_optionsError,
    kSignDiff_newFileError=kSyncClient_ERROR_CODE_MAX+1, //201
    kSignDiff_outFileError,
    kSignDiff_overwritePathError,
    kSignDiff_newFileOpenReadError,
    kSignDiff_newFileReadError, //205
    kSignDiff_newFileCloseError,
    kSignDiff_diffFileOpenWriteError,
    kSignDiff_diffFileWriteError,
    kSignDiff_diffFileCloseError,
    kSignDiff_diffError, //210
} THSignDiffResult;

int sign_diff_cmd_line(int argc, const char * argv[]);
int hsign_diff_by_file(const char* old_hsyni_file,const char* newFileName,const char* outDiffFileName,
                       ISyncInfoListener* listener,const hdiff_TCompress* compressPlugin,
                       size_t patchStepMemSize,size_t threadNum);


#if (_IS_NEED_MAIN)
#   if (_IS_USED_WIN32_UTF8_WAPI)
int wmain(int argc,wchar_t* argv_w[]){
    hdiff_private::TAutoMem  _mem(hpatch_kPathMaxSize*4);
    char** argv_utf8=(char**)_mem.data();
    if (!_wFileNames_to_utf8((const wchar_t**)argv_w,argc,argv_utf8,_mem.size()))
        return kSignDiff_optionsError;
    SetDefaultStringLocale();
    return sign_diff_cmd_line(argc,(const char**)argv_utf8);
}
#   else
int main(int argc,char* argv[]){
    return  sign_diff_cmd_line(argc,(const char**)argv);
}
#   endif
#endif


static bool _tryGetCompressSet(const char** isMatchedType,const char* ptype,const char* ptypeEnd,
                               const char* cmpType,const char* cmpType2=0,
                               size_t* compressLevel=0,size_t levelMin=0,size_t levelMax=0,size_t levelDefault=0,
                               size_t* dictSize=0,size_t dictSizeMin=0,size_t dictSizeMax=0,size_t dictSizeDefault=0){
    assert (0==(*isMatchedType));
    const size_t ctypeLen=strlen(cmpType);
    const size_t ctype2Len=(cmpType2!=0)?strlen(cmpType2):0;
    if ( ((ctypeLen==(size_t)(ptypeEnd-ptype))&&(0==strncmp(ptype,cmpType,ctypeLen)))
        || ((cmpType2!=0)&&(ctype2Len==(size_t)(ptypeEnd-ptype))&&(0==strncmp(ptype,cmpType2,ctype2Len))) )
        *isMatchedType=cmpType; //ok
    else
        return true;//type mismatch
    
    if ((compressLevel)&&(ptypeEnd[0]=='-')){
        const char* plevel=ptypeEnd+1;
        const char* plevelEnd=findUntilEnd(plevel,'-');
        if (!kmg_to_size(plevel,plevelEnd-plevel,compressLevel)) return false; //error
        if (*compressLevel<levelMin) *compressLevel=levelMin;
        else if (*compressLevel>levelMax) *compressLevel=levelMax;
        if ((dictSize)&&(plevelEnd[0]=='-')){
            const char* pdictSize=plevelEnd+1;
            const char* pdictSizeEnd=findUntilEnd(pdictSize,'-');
            if (!kmg_to_size(pdictSize,pdictSizeEnd-pdictSize,dictSize)) return false; //error
            if (*dictSize<dictSizeMin) *dictSize=dictSizeMin;
            else if (*dictSize>dictSizeMax) *dictSize=dictSizeMax;
        }else{
            if (plevelEnd[0]!='\0') return false; //error
            if (dictSize) *dictSize=(dictSizeDefault<dictSizeMax)?dictSizeDefault:dictSizeMax;
        }
    }else{
        if (ptypeEnd[0]!='\0') return false; //error
        if (compressLevel) *compressLevel=levelDefault;
        if (dictSize) *dictSize=dictSizeDefault;
    }
    return true;
}


#define _options_check(value,errorInfo) do{ \
    if (!(value)) { LOG_ERR("options " errorInfo " ERROR!\n\n"); \
                    printUsage(); return kSignDiff_optionsError; } }while(0)

#define _return_check(value,exitCode,fmt,errorInfo) do{ \
    if (!(value)) { LOG_ERR(fmt " ERROR!\n",errorInfo); return exitCode; } }while(0)

#define _return_check2(value,exitCode,fmt,errorInfo0,errorInfo1) do{ \
    if (!(value)) { LOG_ERR(fmt " ERROR!\n",errorInfo0,errorInfo1); return exitCode; } }while(0)



#define __getCompressSet(_tryGet_code,_errTag)  \
    if (isMatchedType==0){                      \
        _options_check(_tryGet_code,_errTag);   \
        if (isMatchedType)

static int _checkSetCompress(hdiff_TCompress** out_compressPlugin,
                             const char* ptype,const char* ptypeEnd){
    const char* isMatchedType=0;
    size_t      compressLevel=0;
#if (defined _CompressPlugin_zlib)||(defined _CompressPlugin_ldef)||(defined _CompressPlugin_zstd)
    size_t       dictBits=0;
    const size_t defaultDictBits=25; //32m
    const size_t defaultDictBits_zlib=15; //32k
#endif
#ifdef _CompressPlugin_zlib
    __getCompressSet(_tryGetCompressSet(&isMatchedType,ptype,ptypeEnd,"zlib","pzlib",
                                        &compressLevel,1,9,6, &dictBits,9,15,defaultDictBits_zlib),"-c-zlib-?"){
#   if (!_IS_USED_MULTITHREAD)
        static TCompressPlugin_zlib _zlibCompressPlugin=zlibCompressPlugin;
        _zlibCompressPlugin.compress_level=(int)compressLevel;
        _zlibCompressPlugin.windowBits=(signed char)(-dictBits);
        *out_compressPlugin=&_zlibCompressPlugin.base; }}
#   else
        static TCompressPlugin_pzlib _pzlibCompressPlugin=pzlibCompressPlugin;
        _pzlibCompressPlugin.base.compress_level=(int)compressLevel;
        _pzlibCompressPlugin.base.windowBits=(signed char)(-(int)dictBits);
        *out_compressPlugin=&_pzlibCompressPlugin.base.base; }}
#   endif // _IS_USED_MULTITHREAD
#endif
#ifdef _CompressPlugin_ldef
    __getCompressSet(_tryGetCompressSet(&isMatchedType,ptype,ptypeEnd,"ldef","pldef",
                                        &compressLevel,1,12,9, &dictBits,15,15,defaultDictBits_zlib),"-c-ldef-?"){
#   if (!_IS_USED_MULTITHREAD)
        static TCompressPlugin_ldef _ldefCompressPlugin=ldefCompressPlugin;
        _ldefCompressPlugin.compress_level=(int)compressLevel;
        *out_compressPlugin=&_ldefCompressPlugin.base; }}
#   else
        static TCompressPlugin_pldef _pldefCompressPlugin=pldefCompressPlugin;
        _pldefCompressPlugin.base.compress_level=(int)compressLevel;
        *out_compressPlugin=&_pldefCompressPlugin.base.base; }}
#   endif // _IS_USED_MULTITHREAD
#endif
#ifdef _CompressPlugin_zstd
    __getCompressSet(_tryGetCompressSet(&isMatchedType,ptype,ptypeEnd,"zstd",0,
                                        &compressLevel,0,22,15, &dictBits,10,
                                        _ZSTD_WINDOWLOG_MAX,defaultDictBits),"-c-zstd-?"){
        static TCompressPlugin_zstd _zstdCompressPlugin=zstdCompressPlugin;
        _zstdCompressPlugin.compress_level=(int)compressLevel;
        _zstdCompressPlugin.dict_bits = (int)dictBits;
        *out_compressPlugin=&_zstdCompressPlugin.base; }}
#endif

    _options_check((*out_compressPlugin!=0),"-c-?");
    return kSignDiff_ok;
}

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
        printf("  can't found decompress type: \"%s\"\n",compressType);
        return 0; //unsupport error
    }else{
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
#ifdef  _ChecksumPlugin_mbedtls_sha512
    if ((!strongChecksumPlugin)&&(0==strcmp(strongChecksumType,sha512ChecksumPlugin.checksumType())))
        strongChecksumPlugin=&sha512ChecksumPlugin;
#endif
#ifdef  _ChecksumPlugin_mbedtls_sha256
    if ((!strongChecksumPlugin)&&(0==strcmp(strongChecksumType,sha256ChecksumPlugin.checksumType())))
        strongChecksumPlugin=&sha256ChecksumPlugin;
#endif
#ifdef  _ChecksumPlugin_crc32
    if ((!strongChecksumPlugin)&&(0==strcmp(strongChecksumType,crc32ChecksumPlugin.checksumType())))
        strongChecksumPlugin=&crc32ChecksumPlugin;
#endif
    if (strongChecksumPlugin==0){
        printf("can't found old .hysni checksum type: \"%s\"\n",strongChecksumType);
        return 0; //unsupport error
    }else{
        return strongChecksumPlugin; //ok
    }
}

hpatch_inline static
bool getFileSize(const char *path_utf8,hpatch_StreamPos_t* out_fileSize){
    hpatch_TPathType out_type;
    if (!hpatch_getPathStat(path_utf8,&out_type,out_fileSize)) return false;
    return out_type==kPathType_file;
}

static bool printFileInfo(const char *path_utf8,const char *tag,hpatch_StreamPos_t* out_fileSize=0){
    hpatch_StreamPos_t fileSize=0;
    if (!getFileSize(path_utf8,&fileSize)) return false;
    printf("%s: %" PRIu64 "   \"",tag,fileSize); hpatch_printPath_utf8(path_utf8); printf("\"\n");
    if (out_fileSize) * out_fileSize=fileSize;
    return true;
}


#define _kNULL_VALUE    ((hpatch_BOOL)(-1))
#define _kNULL_SIZE     (~(size_t)0)

#define _THREAD_NUMBER_NULL     0
#define _THREAD_NUMBER_MIN      1
#define _THREAD_NUMBER_DEFUALT  4
#define _THREAD_NUMBER_MAX      (1<<8)

int sign_diff_cmd_line(int argc, const char * argv[]){
    hpatch_BOOL isForceOverwrite=_kNULL_VALUE;
    hpatch_BOOL isOutputHelp=_kNULL_VALUE;
    hpatch_BOOL isOutputVersion=_kNULL_VALUE;
    size_t      patchStepMemSize=_kNULL_SIZE;
    hdiff_TCompress*  compressPlugin=0;
    hpatch_size_t     threadNum = _THREAD_NUMBER_NULL;
    std::vector<const char *> arg_values;
    for (int i=1; i<argc; ++i) {
        const char* op=argv[i];
        _options_check(op!=0,"?");
        if (op[0]!='-'){
            arg_values.push_back(op); //file path
            continue;
        }
        switch (op[1]) {
            case '?':
            case 'h':{
                _options_check((isOutputHelp==_kNULL_VALUE)&&(op[2]=='\0'),"-h");
                isOutputHelp=hpatch_TRUE;
            } break;
            case 'v':{
                _options_check((isOutputVersion==_kNULL_VALUE)&&(op[2]=='\0'),"-v");
                isOutputVersion=hpatch_TRUE;
            } break;
            case 'f':{
                _options_check((isForceOverwrite==_kNULL_VALUE)&&(op[2]=='\0'),"-f");
                isForceOverwrite=hpatch_TRUE;
            } break;
#if (_IS_USED_MULTITHREAD)
            case 'p':{
                _options_check((threadNum==_THREAD_NUMBER_NULL)&&(op[2]=='-'),"-p-?");
                const char* pnum=op+3;
                _options_check(a_to_size(pnum,strlen(pnum),&threadNum),"-p-?");
                _options_check(threadNum>=_THREAD_NUMBER_MIN,"-p-?");
            } break;
#endif
            case 'c':{
                _options_check((compressPlugin==0)&&(op[2]=='-'),"-c");
                const char* ptype=op+3;
                const char* ptypeEnd=findUntilEnd(ptype,'-');
                int result=_checkSetCompress(&compressPlugin,ptype,ptypeEnd);
                if (kSignDiff_ok!=result)
                    return result;
            } break;
            case 'S':{
                _options_check((patchStepMemSize==_kNULL_SIZE)
                               &&(op[2]=='D')&&((op[3]=='\0')||(op[3]=='-')),"-SD");
                if (op[3]=='-'){
                    const char* pnum=op+4;
                    _options_check(kmg_to_size(pnum,strlen(pnum),&patchStepMemSize),"-SD-?");
                    _options_check((patchStepMemSize>=hpatch_kStreamCacheSize),"-SD-?");
                }else{
                    patchStepMemSize=kDefaultPatchStepMemSize;
                }
            } break;
            default: {
                _options_check(hpatch_FALSE,"-?");
            } break;
        }//swich
    }
    
    if (isOutputHelp==_kNULL_VALUE)
        isOutputHelp=hpatch_FALSE;
    if (isOutputVersion==_kNULL_VALUE)
        isOutputVersion=hpatch_FALSE;
    if (isForceOverwrite==_kNULL_VALUE)
        isForceOverwrite=hpatch_FALSE;
    if (patchStepMemSize==_kNULL_SIZE)
        patchStepMemSize=kDefaultPatchStepMemSize;
#if (_IS_USED_MULTITHREAD)
    if (threadNum==_THREAD_NUMBER_NULL)
        threadNum=_THREAD_NUMBER_DEFUALT;
    else if (threadNum>_THREAD_NUMBER_MAX)
        threadNum=_THREAD_NUMBER_MAX;
#else
    threadNum=1;
#endif
    if (isOutputHelp||isOutputVersion){
        if (isOutputHelp)
            printUsage();//with version
        else
            printVersion();
        if (arg_values.empty())
            return kSignDiff_ok; //ok
    }

    _options_check((arg_values.size()==3),"input count");
    const char* old_hsyni_file =arg_values[0]; // .hsyni
    const char* newFile        =arg_values[1]; 
    const char* outDiffFile    =arg_values[2]; 
    if (!isForceOverwrite){
        hpatch_TPathType   outFileType;
        _return_check(hpatch_getPathStat(outDiffFile,&outFileType,0),
                      kSignDiff_overwritePathError,"get %s type","outDiffFile");
        _return_check(outFileType==kPathType_notExist,
                      kSignDiff_overwritePathError,"%s already exists, overwrite","outDiffFile");
    }
    hpatch_TPathType newType;
    _return_check(hpatch_getPathStat(newFile,&newType,0),
                  kSignDiff_newFileError,"get %s type","newFile");
    _return_check((newType!=kPathType_notExist),
                  kSignDiff_newFileError,"%s not exist","newFile");
    hpatch_BOOL isUseDirSyncUpdate=false;
    _return_check(kPathType_dir!=newType,
                  kSignDiff_newFileError,"%s must file","newFile");

    if (threadNum>1)
        printf("multi-thread parallel: opened, threadNum: %d\n",(uint32_t)threadNum);

    if (compressPlugin)
        compressPlugin->setParallelThreadNumber(compressPlugin,(int)threadNum);

    ISyncInfoListener listener; memset(&listener,0,sizeof(listener));
    listener.findChecksumPlugin=_findChecksumPlugin;
    listener.findDecompressPlugin=_findDecompressPlugin;
    double time0=clock_s();
    int result=hsign_diff_by_file(old_hsyni_file,newFile,outDiffFile,&listener,
                                  compressPlugin,patchStepMemSize,threadNum);
    double time1=clock_s();
    if (result==kSignDiff_ok){
        _return_check(printFileInfo(outDiffFile,"outDiffFile size"),
                      kSignDiff_outFileError,"run printFileInfo(\"%s\")",outDiffFile);
    }
    printf("\nhsign_diff time: %.3f s\n",(time1-time0));
    if (result==kSignDiff_ok) printf("run ok.\n"); else printf("\nERROR!\n\n");
    return result;
}


    static void printSyncInfo(const TOldDataSyncInfo* oldSyncInfo) {
        printf("  old .hsynz file checksum type: \"%s\"\n",oldSyncInfo->strongChecksumType);
        printf("  old uncompress size: %" PRIu64 "\n",oldSyncInfo->newDataSize);
        if (oldSyncInfo->_decompressPlugin!=0){
            printf("  old .hsynz file size: %" PRIu64 "\n",oldSyncInfo->newSyncDataSize);
            printf("  old .hsynz file compressed type: \"%s\"\n",oldSyncInfo->compressType);
        }
        printf("  kSyncBlockSize: %" PRIu64 "\n",(hpatch_uint64_t)oldSyncInfo->kSyncBlockSize);
    }

    #define check(v,errorCode,errorInfo) \
                do{ if (!(v)) { if (result==kSignDiff_ok) result=errorCode; \
                                LOG_ERR(errorInfo " ERROR!\n"); \
                                if (!_isInClear) goto clear; } }while(0)

int hsign_diff_by_file(const char* old_hsyni_file,const char* newFileName,const char* outDiffFileName,
                       ISyncInfoListener* listener,const hdiff_TCompress* compressPlugin,
                       size_t patchStepMemSize,size_t threadNum){
    int result=kSignDiff_ok;
    int _isInClear=hpatch_FALSE;
    TOldDataSyncInfo         oldSyncInfo;
    hpatch_TFileStreamInput  newData;
    hpatch_TFileStreamOutput diffData_out;
    sync_private::TNewDataSyncInfo_init(&oldSyncInfo);
    hpatch_TFileStreamInput_init(&newData);
    hpatch_TFileStreamOutput_init(&diffData_out);
    
    printf("hsign_diff create diffFile compatible with hpatchz!\n");
    if (compressPlugin){
        printf("hsign_diff run with compress plugin: \"%s\"\n",
               compressPlugin->compressTypeForDisplay?compressPlugin->compressTypeForDisplay():compressPlugin->compressType());
    }
    result=TNewDataSyncInfo_open_by_file(&oldSyncInfo,old_hsyni_file,listener);
    check(result==kSignDiff_ok,result,"open old_hsyni_file");
    printSyncInfo(&oldSyncInfo);

    check(hpatch_TFileStreamInput_open(&newData,newFileName),kSignDiff_newFileOpenReadError,"open newFile");
    printf("  new file size : %" PRIu64 "\n",newData.base.streamSize);
    check(hpatch_TFileStreamOutput_open(&diffData_out,outDiffFileName,hpatch_kNullStreamPos),
          kSignDiff_diffFileOpenWriteError,"open out diffFile");
    hpatch_TFileStreamOutput_setRandomOut(&diffData_out,hpatch_TRUE);
    try{
        create_hdiff_by_sign(&newData.base,&oldSyncInfo,&diffData_out.base,
                                compressPlugin,patchStepMemSize,threadNum);
        diffData_out.base.streamSize=diffData_out.out_length;
    }catch(const std::exception& e){
        check(!newData.fileError,kSignDiff_newFileReadError,"read newFile");
        check(!diffData_out.fileError,kSignDiff_diffFileWriteError,"write diffFile");
        LOG_ERR("  create diff throw an error: %s\n",e.what());
        check(false,kSignDiff_diffError,"hsign_diff diff");
    }

clear:
    _isInClear=hpatch_TRUE;
    check(hpatch_TFileStreamOutput_close(&diffData_out),kSignDiff_diffFileCloseError,"close diffFile");
    check(hpatch_TFileStreamInput_close(&newData),kSignDiff_newFileCloseError,"close newFile");
    TNewDataSyncInfo_close(&oldSyncInfo);
    return result;
}
