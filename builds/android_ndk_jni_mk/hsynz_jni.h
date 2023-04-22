// sf_patch_jni.h

//  Created by sisong on 2020-12-25.
//  Copyright © 2020--2022 housisong. All rights reserved.
#include <jni.h>
#include <assert.h>
#include "hsynz_export.h"
#ifdef __cplusplus
extern "C" {
#endif

    #define _check_rt(v)              do { if (!(v)) { result=kSyncClient_optionsError; goto _clear; }; } while(0)
    #define __j2cstr_(jstr,cstr)      do { (cstr)=jenv->GetStringUTFChars(jstr,NULL); _check_rt(cstr); } while(0)
    #define _check_j2cstr(jstr,cstr)  do { _check_rt(jstr); __j2cstr_(jstr,cstr); } while(0)
    #define _check_jn2cstr(jstr,cstr) do { if (jstr) __j2cstr_(jstr,cstr); else (cstr)=0; } while(0)
    #define _jrelease_cstr(jstr,cstr) do { if (cstr) jenv->ReleaseStringUTFChars(jstr,cstr); } while(0)

#ifdef __cplusplus
}
#endif
