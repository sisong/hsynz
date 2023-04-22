// hsynz_export.h

//  Created by sisong on 2023-04-21.
//  Copyright © 2023 housisong. All rights reserved.
#ifndef hsynz_export_h
#define hsynz_export_h
#include <string.h> // strlen size_t
#include "../../HDiffPatch/libhsync/sync_client/sync_client_type.h"
#ifdef __cplusplus
extern "C" {
#endif
    #define HSYNZ_EXPORT __attribute__((visibility("default")))

//like as hsync_patch_2file in hsync_demo.c
//return TSyncClient_resultType
int hsynz_patch(const char* outNewFile,const char* oldPath,const char* hsyni_file,
                IReadSyncDataListener* syncDataListener,const char* localDiffFile,
                TSyncDiffType diffType,hpatch_BOOL isUsedDownloadContinue,int threadNum) HSYNZ_EXPORT;

#ifdef __cplusplus
}
#endif
#endif // hsynz_export_h
