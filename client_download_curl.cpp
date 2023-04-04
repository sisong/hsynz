//  client_download_curl.cpp
//  hsync_curl: http(s) download demo by curl
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
#include "client_download_curl.h"
#include <assert.h>
#include "hsync_import_patch.h" // HSYNC_VERSION_STRING
#include "curl.h" // "curl/include/curl/curl.h" https://github.com/curl/curl

static const size_t kBestBufSize=hpatch_kFileIOBufBetterSize;
static const size_t kStepMaxRangCount=64;
static const int    kKeepAliveTime_s=5;
static const char*  kHttpUserAgent="hsync" HSYNC_VERSION_STRING;

const hpatch_StreamPos_t kEmptyEndPos=~(hpatch_StreamPos_t)0;

#define _check_r(_curlCode) do{ if (_curlCode!=CURLE_OK) goto _clear; }while(0)

hpatch_BOOL download_range_by_http_open(IReadSyncDataListener* out_httpListener,const char* file_url){
    return hpatch_FALSE;
}

hpatch_BOOL download_range_by_http_close(IReadSyncDataListener* httpListener){
    return hpatch_FALSE;
}


hpatch_BOOL download_file_by_http(const char* file_url,const hpatch_TStreamOutput* out_stream,
                                  hpatch_StreamPos_t continueDownloadPos){
    hpatch_StreamPos_t endPos=kEmptyEndPos;
    CURL* _curl=0;

	_curl = curl_easy_init();
    _check_r(curl_easy_setopt(_curl,CURLOPT_NOBODY,1L));
    
_clear:
    if (_curl) curl_easy_cleanup(_curl);
}
