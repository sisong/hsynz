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


static const size_t kStepBufSize=(1<<20);
static const size_t kStepLimitRangCount=64;
static const int    kTimeout_s=6;
const hpatch_StreamPos_t kEmptyEndPos=~(hpatch_StreamPos_t)0;
static const char*  kHttpUserAgent="hsync" HSYNC_VERSION_STRING;


#define _check(_v) do{ if (!(_v)) { result=hpatch_FALSE; goto _clear; } }while(0)


hpatch_BOOL download_range_by_http_open(IReadSyncDataListener* out_httpListener,const char* url){
    return hpatch_FALSE;
}

hpatch_BOOL download_range_by_http_close(IReadSyncDataListener* httpListener){
    return hpatch_FALSE;
}

hpatch_BOOL download_file_by_http(const char* url,const hpatch_TStreamOutput* out_stream,
                                  hpatch_StreamPos_t continueDownloadPos){
    hpatch_BOOL result=hpatch_TRUE;
    hpatch_StreamPos_t endPos=kEmptyEndPos;
 

_clear:

    return result;
}
