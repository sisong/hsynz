# [hsync](https://github.com/sisong/hsync)
[![release](https://img.shields.io/badge/release-v0.8.0-blue.svg)](https://github.com/sisong/hsync/releases) 
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/sisong/hsync/blob/main/LICENSE) 
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-blue.svg)](https://github.com/sisong/hsync/pulls)
[![+issue Welcome](https://img.shields.io/github/issues-raw/sisong/hsync?color=green&label=%2Bissue%20welcome)](https://github.com/sisong/hsync/issues)   

[![Build Status](https://github.com/sisong/hsync/workflows/ci/badge.svg?branch=main)](https://github.com/sisong/hsync/actions?query=workflow%3Aci+branch%3Amain)   

 english | [中文版](README_cn.md)   

hsync is a    


    

---
## Releases/Binaries
[Download from latest release](https://github.com/sisong/hsync/releases) : Command line app for Windows, Linux, MacOS.     
( release files build by projects in path `hsync/builds` )   

## Build it yourself
### Linux or MacOS X ###
```
$ cd <dir>
$ git clone --recursive https://github.com/sisong/hsync.git
$ cd hsync
$ make
```

### Windows ###
```
$ cd <dir>
$ git clone --recursive https://github.com/sisong/hsync.git
```
build `hsync/builds/vc/hsync.sln` with [`Visual Studio`](https://visualstudio.microsoft.com)   

---
## **hsync_make** command line usage:  
```
hsync_make: [options] newDataPath out_hsyni_file [out_hsynz_file]
  newDataPath can be file or directory(folder),
  if newDataPath is a file & no -c-... option, out_hsynz_file can empty.
options:
  -s-matchBlockSize
      matchBlockSize>=128, DEFAULT -s-2k, recommended 1024,4k,...
  -b-safeBit
      set allow patch fail hash clash probability: 1/2^safeBit;
      safeBit>=14, DEFAULT -b-24, recommended 20,32...
  -p-parallelThreadNumber
    DEFAULT -p-4;
    if parallelThreadNumber>1 then open multi-thread Parallel mode;
  -c-compressType[-compressLevel]
      set out_hsynz_file Compress type & level, DEFAULT uncompress;
      support compress type & level:
        -c-zlib[-{1..9}[-dictBits]]     DEFAULT level 9
            dictBits can 9--15, DEFAULT 15.
        -c-gzip[-{1..9}[-dictBits]]     DEFAULT level 9
            dictBits can 9--15, DEFAULT 15.
            compress by zlib, but out_hsynz_file is .gz file format.
        -c-zstd[-{10..22}[-dictBits]]    DEFAULT level 21
            dictBits can 15--30, DEFAULT 24.
  -C-checksumType
      set strong Checksum type for block data, DEFAULT -C-blake3;
      support checksum type:
        -C-blake3
        -C-md5
        -C-sha256
        -C-crc32
            WARNING: crc32 is not strong & secure enough!
  -n-maxOpenFileNumber
      limit Number of open files at same time when newDataPath is directory;
      maxOpenFileNumber>=8, DEFAULT -n-48, the best limit value by different
        operating system.
  -g#ignorePath[#ignorePath#...]
      set iGnore path list in newDataPath directory; ignore path list such as:
        #.DS_Store#desktop.ini#*thumbs*.db#.git*#.svn/#cache_*/00*11/*.tmp
      # means separator between names; (if char # in name, need write #: )
      * means can match any chars in name; (if char * in name, need write *: );
      / at the end of name means must match directory;
  -f  Force overwrite, ignore write path already exists;
      DEFAULT (no -f) not overwrite and then return error;
      if used -f and write path is exist directory, will always return error.
  --patch
      swap to hsync_demo mode.
  -v  output Version info.
  -h or -?
      output Help info (this usage).
```

## **hsync_demo** command line usage:  
```
sync  patch: [options] oldPath [-dl#hsyni_file_test] hsyni_file hsynz_file_test outNewPath
download   : [options] -dl#hsyni_file_test hsyni_file
local  diff: [options] oldPath hsyni_file hsynz_file_test -diff#diffFile
local patch: [options] oldPath hsyni_file -patch#diffFile outNewPath
sync  infos: [options] oldPath hsyni_file
  oldPath can be file or directory(folder),
  if oldPath is empty input parameter ""
options:
  -dl#hsyni_file_test
    download hsyni_file from hsyni_file_test befor sync patch;
  -diff#outDiffFile
    create diffFile from part of hsynz_file_test befor local patch;
  -patch#diffFile
    local patch(oldPath+diffFile) to outNewPath;
  -cdl
    continue download data from breakpoint;
    DEFAULT continue download mode is closed;
  -p-parallelThreadNumber
    DEFAULT -p-4;
    if parallelThreadNumber>1 then open multi-thread Parallel mode;
    NOTE: now download data always used single-thread.
  -n-maxOpenFileNumber
      limit Number of open files at same time when oldPath is directory;
      maxOpenFileNumber>=8, DEFAULT -n-24, the best limit value by different
        operating system.
  -g#ignorePath[#ignorePath#...]
      set iGnore path list in oldPath directory; ignore path list such as:
        #.DS_Store#desktop.ini#*thumbs*.db#.git*#.svn/#cache_*/00*11/*.tmp
      # means separator between names; (if char # in name, need write #: )
      * means can match any chars in name; (if char * in name, need write *: );
      / at the end of name means must match directory;
  -f  Force overwrite, ignore write path already exists;
      DEFAULT (no -f) not overwrite and then return error;
      not support oldPath outNewPath same path!
      if used -f and outNewPath is exist file:
        if patch output file, will overwrite;
        if patch output directory, will always return error;
      if used -f and outNewPath is exist directory:
        if patch output file, will always return error;
        if patch output directory, will overwrite, but not delete
          needless existing files in directory.
  -v  output Version info.
  -h or -?
      output Help info (this usage).
```

## **hsync_http** command line usage:  
//todo:

---
## Contact
housisong@hotmail.com  