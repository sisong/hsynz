// hsynz_export.c

//  Created by sisong on 2023-04-21.
//  Copyright © 2023 housisong. All rights reserved.
#include "hsynz_export.h"

#define  _IS_NEED_MAIN              0
#define  _IS_NEED_CMDLINE           0
#define  _IS_SYNC_PATCH_DEMO        0
#define  _IS_NEED_DIR_DIFF_PATCH    0
#define  _IS_NEED_PRINT_LOG         0
#define  _IS_NEED_ZSYNC             0

#include "../../hsync_demo.cpp"

static const std::vector<std::string> _emptyIgnore;

int hsynz_patch(const char* outNewFile,const char* oldFile,const char* hsyni_file,
                IReadSyncDataListener* syncDataListener,const char* localDiffFile,
                TSyncDiffType diffType,hpatch_BOOL isContinue,int threadNum){
    return hsync_patch_2file(outNewFile,oldFile,false,_emptyIgnore,hsyni_file,syncDataListener,
                             localDiffFile,diffType,isContinue,-1,threadNum);
}
