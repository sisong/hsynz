//  client_download_demo.h
//  sync_client
//  Created by housisong on 2019-09-23.
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
#ifndef client_download_demo_h
#define client_download_demo_h
#include "HDiffPatch/libhsync/sync_client/sync_client_type.h"
#ifdef __cplusplus
extern "C" {
#endif

//downloadEmulation for patch test:
//  when need to download part of newSyncData, emulation read it from local data;
hpatch_BOOL downloadEmulation_open_by_file(IReadSyncDataListener* out_emulation,
                                           const char* newSyncDataPath,size_t kStepRangeNumber);
hpatch_BOOL downloadEmulation_open(IReadSyncDataListener* out_emulation,const hpatch_TStreamInput* newSyncData);
hpatch_BOOL downloadEmulation_close(IReadSyncDataListener* emulation);
hpatch_BOOL downloadEmulation_download_file(const char* file_url,const hpatch_TStreamOutput* out_stream,
                                            hpatch_StreamPos_t continueDownloadPos);

#ifdef __cplusplus
}
#endif
        
#endif // client_download_demo_h
