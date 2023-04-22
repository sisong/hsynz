// hsynz_jni.cpp
//  Created by sisong on 2020-12-25.
//  Copyright © 2020-2022 housisong. All rights reserved.
#include "hsynz_jni.h"
#ifdef __cplusplus
extern "C" {
#endif

    JNIEXPORT int
    Java_com_github_sisong_hsynz_patch(JNIEnv* jenv,jobject jobj,int threadNum){
        return hsynz_patch(0,0,0,0,0,kSyncDiff_default,0,threadNum);
    }

#ifdef __cplusplus
}
#endif
