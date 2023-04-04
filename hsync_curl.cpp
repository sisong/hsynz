//  hsync_curl.cpp
//  hsync_curl: client sync patch support http(s) by curl
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
#define  _IS_NEED_MAIN 1
#define  _IS_SYNC_PATCH_DEMO 0
#define _NOTE_TEXT_URL  "url"
#define _NOTE_TEXT_APP  "hsync::hsync_curl"

#include "hsync_demo.cpp" //main()
#include "client_download_curl.h"

hpatch_BOOL getSyncDownloadPlugin(TSyncDownloadPlugin* out_downloadPlugin){
    out_downloadPlugin->download_range_open=download_range_by_http_open;
    out_downloadPlugin->download_range_close=download_range_by_http_close;
    out_downloadPlugin->download_file=download_file_by_http;
    return hpatch_TRUE;
}
