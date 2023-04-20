# [hsync](https://github.com/sisong/hsync)
[![release](https://img.shields.io/badge/release-v0.9.0-blue.svg)](https://github.com/sisong/hsync/releases) 
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/sisong/hsync/blob/main/LICENSE) 
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-blue.svg)](https://github.com/sisong/hsync/pulls)
[![+issue Welcome](https://img.shields.io/github/issues-raw/sisong/hsync?color=green&label=%2Bissue%20welcome)](https://github.com/sisong/hsync/issues)   

[![Build Status](https://github.com/sisong/hsync/workflows/ci/badge.svg?branch=main)](https://github.com/sisong/hsync/actions?query=workflow%3Aci+branch%3Amain)   

 english | [中文版](README_cn.md)   

hsync is a library for delta update using sync algorithm, like [zsync](http://zsync.moria.org.uk).   

Recommended scenarios: Very large number of older versions or where older versions are not available (not saved or modified, etc.) so that all deltas cannot be calculated in advance.   

The server uses hsync_make to process the latest version of the data once, generating a summary info file(hsyni) of the new version of the data in chunks, and optionally compressing the new version of the data in chunks to get the release file(hsynz), which would be the hsynz equivalent if the new version of the original file were not compressed.   

The client first downloads the hsyni file from the server, calculates the updated blocks it needs to download based on its old version, and learns the location of these blocks in hsynz based on the information in hsyni, selects a communication method to download them on demand from the server's hsynz file, and merges the downloaded blocks with the existing data locally to get the latest version of the data.   

hsync_demo provides a test client demo for local file testing.   
hsync_http provides a download client demo with http(s) support for sync update from a server that provides an http(s) file download service(e.g CDN).   
Tip: You can also customise other communication methods for sync.   
    

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
## performance testing
  see [HDiffPatch](https://github.com/sisong/HDiffPatch)

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
        -c-zstd[-{10..22}[-dictBits]]   DEFAULT level 21
            dictBits can 15--30, DEFAULT 24.
  -C-checksumType
      set strong Checksum type for block data, DEFAULT -C-xxh128;
      support checksum type:
        -C-xxh128
        -C-md5
        -C-sha512
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

## **hsync_http** command line usage:  
```outNewPath
download   : [options] -dl#hsyni_file_url hsyni_file
local  diff: [options] oldPath hsyni_file hsynz_file_url -diff#outDiffFile
local patch: [options] oldPath hsyni_file -patch#diffFile outNewPath
sync  infos: [options] oldPath hsyni_file
sync  patch: [options] oldPath [-dl#hsyni_file_url] hsyni_file hsynz_file_url [-diffi#cacheTempFile] 
  oldPath can be file or directory(folder),
  if oldPath is empty input parameter ""
options:
  -dl#hsyni_file_url
    download hsyni_file from hsyni_file_url befor sync patch;
  -diff#outDiffFile
    create diffFile from ranges of hsynz_file_url befor local patch;
  -diffi#cacheTempFile
    saving diffInfo to cache file for optimize speed when continue sync patch;
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

## **hsync_demo** command line usage:  
This cmdline is used for local sync tests, replacing the actual URL remote file with local file, see the hsync_http usage.

---
## Contact
housisong@hotmail.com  