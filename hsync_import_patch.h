//hsync_import_patch.h
// import hsync_make cmd line to hsync_demo
//  Created by housisong on 2022-12-08.
/*
 The MIT License (MIT)
 Copyright (c) 2022 HouSisong All Rights Reserved.
 */
#ifndef hsync_import_patch_h
#define hsync_import_patch_h
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif
#define HSYNC_VERSION_MAJOR    0
#define HSYNC_VERSION_MINOR    9
#define HSYNC_VERSION_RELEASE  2

#define _HSYNC_VERSION          HSYNC_VERSION_MAJOR.HSYNC_VERSION_MINOR.HSYNC_VERSION_RELEASE
#define _HSYNC_QUOTE(str) #str
#define _HSYNC_EXPAND_AND_QUOTE(str) _HSYNC_QUOTE(str)
#define HSYNC_VERSION_STRING   _HSYNC_EXPAND_AND_QUOTE(_HSYNC_VERSION)

int isSwapToPatchMode(int argc,const char* argv[]);
int sync_client_cmd_line(int argc,const char* argv[]);

#ifdef __cplusplus
}
#endif
#endif
