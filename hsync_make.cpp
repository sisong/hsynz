//  hsync_make.cpp
//  hsync_make:  create sync files
//      like zsync : http://zsync.moria.org.uk/
//  Created by housisong on 2019-09-17.
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
#include <stdexcept>
#include <vector>
#include <string>
#include "HDiffPatch/file_for_patch.h"
#include "HDiffPatch/_clock_for_demo.h"
#include "HDiffPatch/_atosize.h"
#include "HDiffPatch/libHDiffPatch/HDiff/private_diff/mem_buf.h"
#include "HDiffPatch/libParallel/parallel_import.h"
#include "HDiffPatch/_dir_ignore.h"

#include "HDiffPatch/libhsync/sync_make/sync_make.h"
#include "HDiffPatch/libhsync/sync_make/sync_make_hash_clash.h"
#include "HDiffPatch/dirDiffPatch/dir_diff/dir_diff_tools.h"
#if (_IS_NEED_DIR_DIFF_PATCH)
#include "HDiffPatch/dirDiffPatch/dir_diff/dir_diff.h"
#include "HDiffPatch/libhsync/sync_make/dir_sync_make.h"
#endif
#include "hsync_import_patch.h"

#ifndef _IS_NEED_MAIN
#   define  _IS_NEED_MAIN 1
#endif

#ifndef _IS_NEED_DEFAULT_CompressPlugin
#   define _IS_NEED_DEFAULT_CompressPlugin 1
#endif
#if (_IS_NEED_DEFAULT_CompressPlugin)
//===== select needs decompress plugins or change to your plugin=====
#   define _CompressPlugin_zlib
#   define _CompressPlugin_zstd
#endif

#include "dict_compress_plugin_demo.h"


#ifndef _IS_NEED_DEFAULT_ChecksumPlugin
#   define _IS_NEED_DEFAULT_ChecksumPlugin 1
#endif
#if (_IS_NEED_DEFAULT_ChecksumPlugin)
//===== select needs checksum plugins or change to your plugin=====
//todo: #   define _ChecksumPlugin_md5
//#   define _ChecksumPlugin_blake3
#   define _ChecksumPlugin_crc32
#endif

#include "HDiffPatch/checksum_plugin_demo.h"

static void printVersion(){
    printf("hsync::hsync_make v" HSYNC_VERSION_STRING "\n\n");
}

static void printUsage(){
    printVersion();
    printf("hsync_make: [options] newDataPath out_hsyni_file [out_hsynz_file]\n"
#if (_IS_NEED_DIR_DIFF_PATCH)
           "  newDataPath can be file or directory(folder),\n"
#endif
           "  if newDataPath is a file & no -c-... option, out_hsynz_file can empty. \n"
           "options:\n"
           "  -s-matchBlockSize\n"
           "      matchBlockSize>=128, DEFAULT -s-2048, recommended 4096,8k,16k...\n"
           "  -b-safeBit\n"
           "      set allow patch fail hash clash probability: 1/2^safeBit;\n"
           "      safeBit>=20, DEFAULT -b-32, recommended 24,40,48...\n"
#if (_IS_USED_MULTITHREAD)
           "  -p-parallelThreadNumber\n"
           "    DEFAULT -p-1;\n"
           "    if parallelThreadNumber>1 then open multi-thread Parallel mode;\n"
           "    used when compress slow or strong checksum slow;\n"
#endif
           "  -c-compressType[-compressLevel]\n"
           "      set out_hsynz_file Compress type & level, DEFAULT uncompress;\n"
           "      support compress type & level:\n"
#ifdef _CompressPlugin_zlib
           "        -c-zlib[-{1..9}[-dictBits]]     DEFAULT level 9\n"
           "            dictBits can 9--15, DEFAULT 15.\n"
#endif
#ifdef _CompressPlugin_zstd
           "        -c-zstd[-{0..22}[-dictBits]]    DEFAULT level 20\n"
           "            dictBits can 10--27, DEFAULT 20.\n"
#endif
           "  -C-checksumType\n"
           "      set strong Checksum type for block data, DEFAULT "
#ifdef _ChecksumPlugin_md5
           "-C-md5;\n"
#else
#   ifdef _ChecksumPlugin_crc32
           "-C-crc32;\n"
#   else
#           error Need a strong Checksum!
#   endif
#endif
           "      support checksum type:\n"
#ifdef _ChecksumPlugin_md5
           "        -C-md5\n"
#endif
#ifdef _ChecksumPlugin_crc32
           "        -C-crc32\n"
#endif
#if (_IS_NEED_DIR_DIFF_PATCH)
           "  -n-maxOpenFileNumber\n"
           "      limit Number of open files at same time when newDataPath is directory;\n"
           "      maxOpenFileNumber>=8, DEFAULT -n-48, the best limit value by different\n"
           "        operating system.\n"
           "  -g#ignorePath[#ignorePath#...]\n"
           "      set iGnore path list in newDataPath directory; ignore path list such as:\n"
           "        #.DS_Store#desktop.ini#*thumbs*.db#.git*#.svn/#cache_*/00*11/*.tmp\n"
           "      # means separator between names; (if char # in name, need write #: )\n"
           "      * means can match any chars in name; (if char * in name, need write *: );\n"
           "      / at the end of name means must match directory;\n"
#endif
           "  -f  Force overwrite, ignore write path already exists;\n"
           "      DEFAULT (no -f) not overwrite and then return error;\n"
           "      if used -f and write path is exist directory, will always return error.\n"
           "  -h or -?\n"
           "      output Help info (this usage).\n"
           "  --patch\n"
           "      swap to hsync_demo mode.\n"
           "  -v  output Version info.\n\n"
           );
}

typedef enum TSyncMakeResult {
    SYNC_MAKE_SUCCESS=0,
    SYNC_MAKE_OPTIONS_ERROR,
    SYNC_MAKE_BLOCKSIZE_OR_SAFE_BITS_ERROR,
    SYNC_MAKE_NEWPATH_ERROR,
    SYNC_MAKE_OUTFILE_ERROR,
    SYNC_MAKE_CANNOT_OVERWRITE_ERROR, // 5
    SYNC_MAKE_CREATE_SYNC_DATA_ERROR,
    SYNC_MAKE_DIR_FILELIST_ERROR,
    SYNC_MAKE_CREATE_DIR_SYNC_DATA_ERROR,
} TSyncMakeResult;

int sync_make_cmd_line(int argc, const char * argv[]);

int create_sync_files_for_file(const char* newDataFile,const char* out_hsyni_file,
                               const char* out_hsynz_file,hpatch_TChecksum* strongChecksumPlugin,
                               hsync_TDictCompress* compressPlugin,uint32_t kSyncBlockSize,
                               size_t kSafeHashClashBit,size_t threadNum);
#if (_IS_NEED_DIR_DIFF_PATCH)
int create_sync_files_for_dir(const char* newDataDir,const char* out_hsyni_file,
                              const char* out_hsynz_file,hpatch_TChecksum* strongChecksumPlugin,
                              hsync_TDictCompress* compressPlugin,size_t kMaxOpenFileNumber,
                              const std::vector<std::string>& ignoreNewPathList,
                              uint32_t kSyncBlockSize,size_t kSafeHashClashBit,size_t threadNum);
#endif

#define _checkPatchMode(_argc,_argv)            \
    if (isSwapToPatchMode(_argc,_argv)){        \
        printf("hsync_make swap to hsync_demo mode.\n\n"); \
        return sync_client_cmd_line(_argc,_argv);    \
    }

#if (_IS_NEED_MAIN)
#   if (_IS_USED_WIN32_UTF8_WAPI)
int wmain(int argc,wchar_t* argv_w[]){
    hdiff_private::TAutoMem  _mem(hpatch_kPathMaxSize*4);
    char** argv_utf8=(char**)_mem.data();
    if (!_wFileNames_to_utf8((const wchar_t**)argv_w,argc,argv_utf8,_mem.size()))
        return SYNC_MAKE_OPTIONS_ERROR;
    SetDefaultStringLocale();
    _checkPatchMode(argc,(const char**)argv_utf8);
    return sync_make_cmd_line(argc,(const char**)argv_utf8);
}
#   else
int main(int argc,char* argv[]){
    _checkPatchMode(argc,(const char**)argv);
    return  sync_make_cmd_line(argc,(const char**)argv);
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
                    printUsage(); return SYNC_MAKE_OPTIONS_ERROR; } }while(0)

#define _return_check(value,exitCode,fmt,errorInfo) do{ \
    if (!(value)) { LOG_ERR(fmt " ERROR!\n",errorInfo); return exitCode; } }while(0)

#define _return_check2(value,exitCode,fmt,errorInfo0,errorInfo1) do{ \
    if (!(value)) { LOG_ERR(fmt " ERROR!\n",errorInfo0,errorInfo1); return exitCode; } }while(0)



#define __getCompressSet(_tryGet_code,_errTag)  \
    if (isMatchedType==0){                      \
        _options_check(_tryGet_code,_errTag);   \
        if (isMatchedType)

static int _checkSetCompress(hsync_TDictCompress** out_compressPlugin,
                             const char* ptype,const char* ptypeEnd){
    const char* isMatchedType=0;
    size_t      compressLevel=0;
#if (defined _CompressPlugin_zlib)||(defined _CompressPlugin_zstd)
    size_t       dictBits=0;
    const size_t defaultDictBits=20+0; //1m
    const size_t defaultDictBits_zlib=15; //32k
#endif
#ifdef _CompressPlugin_zlib
    __getCompressSet(_tryGetCompressSet(&isMatchedType,ptype,ptypeEnd,"zlib","zlibD",
                                        &compressLevel,1,9,9, &dictBits,9,15,defaultDictBits_zlib),"-c-zlib-?"){
        static TDictCompressPlugin_zlib _zlibCompressPlugin=zlibDictCompressPlugin;
        _zlibCompressPlugin.compress_level=(hpatch_byte)compressLevel;
        _zlibCompressPlugin.dict_bits=(hpatch_byte)dictBits;
        *out_compressPlugin=&_zlibCompressPlugin.base; }}
#endif
#ifdef _CompressPlugin_zstd
    __getCompressSet(_tryGetCompressSet(&isMatchedType,ptype,ptypeEnd,"zstd","zstdD",
                                        &compressLevel,0,22,20, &dictBits,10,
                                        30,defaultDictBits),"-c-zstd-?"){
        static TDictCompressPlugin_zstd _zstdCompressPlugin=zstdDictCompressPlugin;
        _zstdCompressPlugin.compress_level=(hpatch_byte)compressLevel;
        _zstdCompressPlugin.dict_bits=(hpatch_byte)dictBits;
        *out_compressPlugin=&_zstdCompressPlugin.base; }}
#endif

    _options_check((*out_compressPlugin!=0),"-c-?");
    return SYNC_MAKE_SUCCESS;
}

static void _trySetChecksum(hpatch_TChecksum** out_checksumPlugin,const char* checksumType,
                            hpatch_TChecksum* testChecksumPlugin){
    if ((*out_checksumPlugin)!=0) return;
    if (0==strcmp(checksumType,testChecksumPlugin->checksumType()))
        *out_checksumPlugin=testChecksumPlugin;
}
static hpatch_BOOL findChecksum(hpatch_TChecksum** out_checksumPlugin,const char* checksumType){
    *out_checksumPlugin=0;
    if (strlen(checksumType)==0) return hpatch_TRUE;
#ifdef _ChecksumPlugin_md5
    _trySetChecksum(out_checksumPlugin,checksumType,&md5ChecksumPlugin);
#endif
#ifdef _ChecksumPlugin_crc32
    _trySetChecksum(out_checksumPlugin,checksumType,&crc32ChecksumPlugin);
#endif
    return (0!=*out_checksumPlugin);
}

static hpatch_TChecksum* getDefaultStrongChecksum(){
#ifdef _ChecksumPlugin_md5
    return &md5ChecksumPlugin;
#endif
#ifdef _ChecksumPlugin_crc32
    return &crc32ChecksumPlugin;
#endif
    return 0; //error
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

static void printCreateSyncInfo(size_t kSafeHashClashBit,hpatch_StreamPos_t newDataSize,
                                uint32_t kSyncBlockSize,bool isUsedCompress){
    printf("  block size : %d\n",kSyncBlockSize);
    hpatch_StreamPos_t blockCount=getSyncBlockCount(newDataSize,kSyncBlockSize);
    printf("  block count: %" PRIu64 "\n",blockCount);
    printf("  safe  bit  : %d\n",(int)kSafeHashClashBit);
    double patchMemSize=(double)estimatePatchMemSize(kSafeHashClashBit,newDataSize,
                                                     kSyncBlockSize,isUsedCompress);
    if (patchMemSize>=(1<<20))
        printf("  sync_patch memory size: ~ %.1f MB\n",patchMemSize/(1<<20));
    else
        printf("  sync_patch memory size: ~ %.0f KB\n",patchMemSize/(1<<10)+1);
}

#define _kNULL_VALUE    ((hpatch_BOOL)(-1))
#define _kNULL_SIZE     (~(size_t)0)

#define _THREAD_NUMBER_NULL     0
#define _THREAD_NUMBER_MIN      1
#define _THREAD_NUMBER_DEFUALT  1
#define _THREAD_NUMBER_MAX      (1<<8)

int sync_make_cmd_line(int argc, const char * argv[]){
    hpatch_BOOL isForceOverwrite=_kNULL_VALUE;
    hpatch_BOOL isOutputHelp=_kNULL_VALUE;
    hpatch_BOOL isOutputVersion=_kNULL_VALUE;
    size_t      kSyncBlockSize=_kNULL_SIZE;
    size_t      kSafeHashClashBit=_kNULL_SIZE;
    hsync_TDictCompress*    compressPlugin=0;
    hpatch_TChecksum* strongChecksumPlugin=0;
    size_t      threadNum = _THREAD_NUMBER_NULL;
    std::vector<const char *> arg_values;
#if (_IS_NEED_DIR_DIFF_PATCH)
    size_t                      kMaxOpenFileNumber=_kNULL_SIZE; //only used in newDataPath is dir
    std::vector<std::string>    ignoreNewPathList;
#endif
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
            case 's':{
                _options_check((kSyncBlockSize==_kNULL_SIZE)&&(op[2]=='-'),"-s");
                const char* pnum=op+3;
                _options_check(kmg_to_size(pnum,strlen(pnum),&kSyncBlockSize),"-s-?");
                _options_check(kSyncBlockSize==(uint32_t)kSyncBlockSize,"-s-?");
                _options_check(kSyncBlockSize>=kSyncBlockSize_min,"-s-?");
            } break;
            case 'b':{
                _options_check((kSafeHashClashBit==_kNULL_SIZE)&&(op[2]=='-'),"-b");
                const char* pnum=op+3;
                _options_check(kmg_to_size(pnum,strlen(pnum),&kSafeHashClashBit),"-b-?");
                _options_check(kSafeHashClashBit>=kSafeHashClashBit_min,"-b-?");
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
                if (SYNC_MAKE_SUCCESS!=result)
                    return result;
            } break;
            case 'C':{
                _options_check((strongChecksumPlugin==0)&&(op[2]=='-'),"-C");
                const char* ptype=op+3;
                _options_check(findChecksum(&strongChecksumPlugin,ptype),"-C-?");
            } break;
#if (_IS_NEED_DIR_DIFF_PATCH)
            case 'n':{
                _options_check((kMaxOpenFileNumber==_kNULL_SIZE)&&(op[2]=='-'),"-n-?");
                const char* pnum=op+3;
                _options_check(kmg_to_size(pnum,strlen(pnum),&kMaxOpenFileNumber),"-n-?");
            } break;
            case 'g':{
                if (op[2]=='#'){ //-g#
                    const char* plist=op+3;
                    _options_check(_getIgnorePathSetList(ignoreNewPathList,plist),"-g#?");
                }else{
                    _options_check(hpatch_FALSE,"-g?");
                }
            } break;
#endif
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
    if (kSyncBlockSize==_kNULL_SIZE)
        kSyncBlockSize=kSyncBlockSize_default;
    if (kSafeHashClashBit==_kNULL_SIZE)
        kSafeHashClashBit=kSafeHashClashBit_default;
    if (strongChecksumPlugin==0)
        strongChecksumPlugin=getDefaultStrongChecksum();
    _options_check(strongChecksumPlugin!=0,"-C-?");
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
        kMaxOpenFileNumber=kMaxOpenFileNumber_default_diff;
    if (kMaxOpenFileNumber<kMaxOpenFileNumber_default_min)
        kMaxOpenFileNumber=kMaxOpenFileNumber_default_min;
#endif
    if (isOutputHelp||isOutputVersion){
        if (isOutputHelp)
            printUsage();//with version
        else
            printVersion();
        if (arg_values.empty())
            return SYNC_MAKE_SUCCESS; //ok
    }

    _options_check((arg_values.size()==2)||(arg_values.size()==3),"input count");
    const char* newDataPath       =arg_values[0];
    const char* out_hsyni_file=arg_values[1]; // .hsyni
    const char* out_hsynz_file=0;             // .hsynz
    if (arg_values.size()>=3){
        out_hsynz_file=arg_values[2];
        if (strlen(out_hsynz_file)==0) out_hsynz_file=0;
    }
    if (compressPlugin){
        _options_check(out_hsynz_file!=0,"used compress need out_hsynz_file");
    }

    if (!isForceOverwrite){
        hpatch_TPathType   outFileType;
        _return_check(hpatch_getPathStat(out_hsyni_file,&outFileType,0),
                      SYNC_MAKE_CANNOT_OVERWRITE_ERROR,"get %s type","out_hsyni_file");
        _return_check(outFileType==kPathType_notExist,
                      SYNC_MAKE_CANNOT_OVERWRITE_ERROR,"%s already exists, overwrite","out_hsyni_file");
        if (out_hsynz_file){
            _return_check(hpatch_getPathStat(out_hsynz_file,&outFileType,0),
                          SYNC_MAKE_CANNOT_OVERWRITE_ERROR,"get %s type","out_hsynz_file");
            _return_check(outFileType==kPathType_notExist,
                          SYNC_MAKE_CANNOT_OVERWRITE_ERROR,"%s already exists, overwrite","out_hsynz_file");
        }
    }
    hpatch_TPathType newType;
    _return_check(hpatch_getPathStat(newDataPath,&newType,0),
                  SYNC_MAKE_NEWPATH_ERROR,"get %s type","newDataPath");
    _return_check((newType!=kPathType_notExist),
                  SYNC_MAKE_NEWPATH_ERROR,"%s not exist","newDataPath");
#if (_IS_NEED_DIR_DIFF_PATCH)
    hpatch_BOOL isUseDirSyncUpdate=(kPathType_dir==newType);
    if (isUseDirSyncUpdate)
        _options_check(out_hsynz_file!=0,"used DirSyncUpdate need out_hsynz_file");
#else
    hpatch_BOOL isUseDirSyncUpdate=false;
    _return_check(kPathType_dir!=newType,
                  SYNC_MAKE_NEWPATH_ERROR,"%s must file","newDataPath");
#endif
    if ((compressPlugin==0)&&(kPathType_file==newType))
        printf("NOTE: out_hsynz_file's data is same as newDataPath file!\n\n");
    
    if (threadNum>1)
        printf("muti-thread parallel: opened, threadNum: %d\n",(uint32_t)threadNum);

    printf("create%s_sync_data run with strongChecksum plugin: \"%s\"\n",
           isUseDirSyncUpdate?"_dir":"",strongChecksumPlugin->checksumType());
    if (compressPlugin){
        printf("create%s_sync_data run with compress plugin: \"%s\"\n",
               isUseDirSyncUpdate?"_dir":"",compressPlugin->compressType());
    }
    double time0=clock_s();
    int result;
#if (_IS_NEED_DIR_DIFF_PATCH)
        if (isUseDirSyncUpdate)
            result=create_sync_files_for_dir(newDataPath,out_hsyni_file,out_hsynz_file,strongChecksumPlugin,
                                             compressPlugin,kMaxOpenFileNumber,ignoreNewPathList,
                                             (uint32_t)kSyncBlockSize,kSafeHashClashBit,threadNum);
        else
#endif
            result=create_sync_files_for_file(newDataPath,out_hsyni_file,out_hsynz_file,strongChecksumPlugin,
                                              compressPlugin,(uint32_t)kSyncBlockSize,kSafeHashClashBit,threadNum);
    double time1=clock_s();
    if (result==SYNC_MAKE_SUCCESS){
        _return_check(printFileInfo(out_hsyni_file,"out .hsyni"),
                      SYNC_MAKE_OUTFILE_ERROR,"run printFileInfo(%s,)",out_hsyni_file);
        if (out_hsynz_file){
            _return_check(printFileInfo(out_hsynz_file,"out .hsynz"),
                          SYNC_MAKE_OUTFILE_ERROR,"run printFileInfo(%s,)",out_hsynz_file);
        }
    }
    printf("\ncreate%s_sync_data time: %.3f s\n\n",isUseDirSyncUpdate?"_dir":"",(time1-time0));
    return result;
}


void create_sync_data_by_file(const char* newDataFile,
                              const char* out_hsyni_file,
                              const char* out_hsynz_file,
                              hpatch_TChecksum*      strongChecksumPlugin,
                              hsync_TDictCompress* compressPlugin,
                              uint32_t kSyncBlockSize,size_t kSafeHashClashBit,size_t threadNum){
    hdiff_private::CFileStreamInput  newData(newDataFile);
    hdiff_private::CFileStreamOutput out_newSyncInfo(out_hsyni_file,~(hpatch_StreamPos_t)0);
    hdiff_private::CFileStreamOutput out_newSyncData;
    const hpatch_TStreamOutput* newDataStream=0;
    if (out_hsynz_file){
        out_newSyncData.open(out_hsynz_file,~(hpatch_StreamPos_t)0);
        newDataStream=&out_newSyncData.base;
    }
    
    create_sync_data(&newData.base,&out_newSyncInfo.base,newDataStream,
                     strongChecksumPlugin,compressPlugin,
                     kSyncBlockSize,kSafeHashClashBit,threadNum);
}

void create_sync_data_by_file(const char* newDataFile,
                              const char* out_hsyni_file,
                              hpatch_TChecksum*      strongChecksumPlugin,
                              hsync_TDictCompress* compressPlugin,
                              uint32_t kSyncBlockSize,size_t kSafeHashClashBit,size_t threadNum){
    create_sync_data_by_file(newDataFile,out_hsyni_file,0,strongChecksumPlugin,compressPlugin,
                             kSyncBlockSize,kSafeHashClashBit,threadNum);
}


int create_sync_files_for_file(const char* newDataFile,const char* out_hsyni_file,
                               const char* out_hsynz_file,hpatch_TChecksum* strongChecksumPlugin,
                               hsync_TDictCompress* compressPlugin,uint32_t kSyncBlockSize,
                               size_t kSafeHashClashBit,size_t threadNum){
    hpatch_StreamPos_t newDataSize=0;
    _return_check(printFileInfo(newDataFile,"\nin new file",&newDataSize),
                  SYNC_MAKE_NEWPATH_ERROR,"run printFileInfo(%s,)",newDataFile);
    bool isSafeHashClash=getStrongForHashClash(kSafeHashClashBit,newDataSize,kSyncBlockSize,
                                               strongChecksumPlugin->checksumByteSize());
    _return_check2(isSafeHashClash,SYNC_MAKE_BLOCKSIZE_OR_SAFE_BITS_ERROR,
                   "hash clash error! matchBlockSize(%d) too small or safeHashClashBit(%d) too large",
                   kSyncBlockSize,(uint32_t)kSafeHashClashBit);
    printCreateSyncInfo(kSafeHashClashBit,newDataSize,kSyncBlockSize,(compressPlugin!=0));
    
    try {
        create_sync_data_by_file(newDataFile,out_hsyni_file,out_hsynz_file,
                                 strongChecksumPlugin,compressPlugin,
                                 kSyncBlockSize,kSafeHashClashBit,threadNum);
    } catch (const std::exception& e){
        _return_check(false,SYNC_MAKE_CREATE_SYNC_DATA_ERROR,
                      "run create_sync_data with \"%s\"",e.what());
    }
    return SYNC_MAKE_SUCCESS;
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

struct DirSyncListener:public IDirSyncListener{
    explicit DirSyncListener(bool isUsedCompress=true)
    :_isUsedCompress(isUsedCompress),_isSafeHashClash(true){ }
    const bool  _isUsedCompress;
    bool        _isSafeHashClash;
    
    //IDirSyncListener
    virtual bool isExecuteFile(const std::string& fileName) {
        bool result= 0!=hpatch_getIsExecuteFile(fileName.c_str());
        if (result){
            printf("  got file Execute tag: \"");
            hpatch_printPath_utf8(fileName.c_str()); printf("\"\n");
        }
        return result;
    }
    virtual void syncRefInfo(const char* rootDirPath,size_t pathCount,hpatch_StreamPos_t refFileSize,
                             uint32_t kSyncBlockSize,size_t kSafeHashClashBit,bool isSafeHashClash){
        _isSafeHashClash=isSafeHashClash;
        printf("  path count : %" PRIu64 "\n",(hpatch_StreamPos_t)pathCount);
        printf("  files size : %" PRIu64 "\n",(hpatch_StreamPos_t)refFileSize);
        printCreateSyncInfo(kSafeHashClashBit,refFileSize,kSyncBlockSize,_isUsedCompress);
    }
};

int create_sync_files_for_dir(const char* newDataDir,const char* out_hsyni_file,
                              const char* out_hsynz_file,hpatch_TChecksum* strongChecksumPlugin,
                              hsync_TDictCompress* compressPlugin,size_t kMaxOpenFileNumber,
                              const std::vector<std::string>& ignoreNewPathList,
                              uint32_t kSyncBlockSize,size_t kSafeHashClashBit,size_t threadNum){
    std::string newDir(newDataDir);
    assignDirTag(newDir);
    printf("\nin new dir: \""); hpatch_printPath_utf8(newDir.c_str()); printf("\"\n");
    DirSyncListener listener(compressPlugin!=0);
    TManifest newManifest;
    try {
        DirPathIgnoreListener pathIgnore(ignoreNewPathList);
        get_manifest(&pathIgnore,newDir.c_str(),newManifest);
    } catch (const std::exception& e){
        _return_check(false,SYNC_MAKE_DIR_FILELIST_ERROR,
                      "run get_manifest with \"%s\"",e.what());
    }
    try {
        create_dir_sync_data(&listener,newManifest,out_hsyni_file,out_hsynz_file,
                             strongChecksumPlugin,compressPlugin,kMaxOpenFileNumber,
                             kSyncBlockSize,kSafeHashClashBit,threadNum);
    } catch (const std::exception& e){
        if (!listener._isSafeHashClash){
            _return_check2(false,SYNC_MAKE_BLOCKSIZE_OR_SAFE_BITS_ERROR,
                           "hash clash error! matchBlockSize(%d) too small or safeHashClashBit(%d) too large",
                           kSyncBlockSize,(uint32_t)kSafeHashClashBit);
        }else{
            _return_check(false,SYNC_MAKE_CREATE_DIR_SYNC_DATA_ERROR,
                          "run create_dir_sync_data with \"%s\"",e.what());
        }
    }
    return SYNC_MAKE_SUCCESS;
}
#endif
