# [hsynz](https://github.com/sisong/hsynz)
[![release](https://img.shields.io/badge/release-v1.2.0-blue.svg)](https://github.com/sisong/hsynz/releases) 
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/sisong/hsynz/blob/main/LICENSE) 
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-blue.svg)](https://github.com/sisong/hsynz/pulls)
[![+issue Welcome](https://img.shields.io/github/issues-raw/sisong/hsynz?color=green&label=%2Bissue%20welcome)](https://github.com/sisong/hsynz/issues)   

[![Build Status](https://github.com/sisong/hsynz/workflows/ci/badge.svg?branch=main)](https://github.com/sisong/hsynz/actions?query=workflow%3Aci+branch%3Amain)   

 english | [中文版](README_cn.md)   

hsynz is a library for delta update using sync algorithm, like [zsync](http://zsync.moria.org.uk).   
rsync over http(s); implement the sync algorithm on client side, and server side only need http(s) cdn. support compressor zstd & libdeflate & zlib, support large file & directory(folder), support multi-thread.   
   

Recommended scenarios: Very large number of older versions or where older versions are not available (not saved or modified, etc.) so that all deltas cannot be calculated in advance.   
   

The server uses hsync_make to process the latest version of the data once, generating a summary info file(hsyni) of the new version of the data in chunks, and optionally compressing the new version of the data in chunks to get the release file(hsynz), which would be the hsynz equivalent if the new version of the original file were not compressed.   
   

The client first downloads the hsyni file from the server or another user's share, calculates the updated blocks it needs to download based on its old version, and learns the location of these blocks in hsynz based on the information in hsyni, selects a communication method to download them on demand from the server's hsynz file, and merges the downloaded blocks with the existing data locally to get the latest version of the data.   
   

hsync_demo provides a test client demo for local file testing.   
hsync_http provides a download client demo with http(s) support for sync update from a server that provides an http(s) file download service(e.g CDN, support HTTP/1.1 multi range Requests).   
Tip: You can also customise other communication methods for sync.   
    
---
## Compare with [zsync](http://zsync.moria.org.uk)
* In addition to supporting source and target as files, support is also provided for directories(folders).
* In addition to supporting compressed release package by zlib; also supported libdeflate & zstd compressor, providing better compression ratio, i.e. smaller downloaded patch package.
* The server-side make support multi-threaded parallel acceleration.
* The client-side diff speed has been optimized, and also support multi-threaded parallel acceleration.

---
## Releases/Binaries
[Download from latest release](https://github.com/sisong/hsynz/releases) : Command line app for Windows, Linux, MacOS; and .so lib for Android.   
( release files build by projects in path `hsynz/builds` )   

## Build it yourself
### Linux or MacOS X ###
```
$ cd <dir>
$ git clone --recursive https://github.com/sisong/hsynz.git
$ cd hsynz
$ make
```

### Windows ###
```
$ cd <dir>
$ git clone --recursive https://github.com/sisong/hsynz.git
```
build `hsynz/builds/vc/hsynz.sln` with [`Visual Studio`](https://visualstudio.microsoft.com)   

### libhsynz.so for Android ###   
* install [Android NDK](https://developer.android.google.cn/ndk/downloads)
* `$ cd <dir>/hsynz/builds/android_ndk_jni_mk`
* `$ build_libs_static.sh`  (or `$ build_libs_static.bat` on windows, then got \*.so files)
* import file `com/github/sisong/hsynz.java` (from `hsynz/builds/android_ndk_jni_mk/java/`) & .so files, java code can call the sync patch function in libhsynz.so
   

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
            compress by zlib, out_hsynz_file is .gz file format.
        -c-ldef[-{1..12}]           DEFAULT level 12 (dictBits always 15).
            compress by libdeflate, compatible with zlib's deflate encoding.
        -c-lgzip[-{1..12}]          DEFAULT level 12 (dictBits always 15)
            compress by libdeflate, out_hsynz_file is .gz file format.
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
```
download   : [options] -dl#hsyni_file_url hsyni_file
local  diff: [options] oldPath hsyni_file hsynz_file_url -diff#outDiffFile
local patch: [options] oldPath hsyni_file -patch#diffFile outNewPath
sync  infos: [options] oldPath hsyni_file [-diffi#cacheTempFile]
sync  patch: [options] oldPath [-dl#hsyni_file_url] hsyni_file hsynz_file_url [-diffi#cacheTempFile] outNewPath
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
  -cdl-{0|1}        or  -cdl-{off|on}
    continue download data from breakpoint;
    DEFAULT -cdl-1 opened, need set -cdl-0 or -cdl-off to close continue mode;
  -rdl-retryDownloadNumber
    number of auto retry connection, when network disconnected while downloading;
    DEFAULT -rdl-0 retry closed; recommended 5,1k,1g,...
  -r-stepRangeNumber
    DEFAULT -r-32, recommended 16,20,...
    limit the maximum number of .hsynz data ranges that can be downloaded
    in a single request step;
    if http(s) server not support multi-ranges request, must set -r-1
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
## hsynz vs [zsync](http://zsync.moria.org.uk):
case list([download from OneDrive](https://1drv.ms/u/s!Aj8ygMPeifoQgUIZxYac5_uflNoN)):   
| |newFile <-- oldFile|newSize|oldSize|
|----:|:----|----:|----:|
|1|7-Zip_22.01.win.tar <-- 7-Zip_21.07.win.tar|5908992|5748224|
|2|Chrome_107.0.5304.122-x64-Stable.win.tar <-- 106.0.5249.119|278658560|273026560|
|3|cpu-z_2.03-en.win.tar <-- cpu-z_2.02-en.win.tar|8718336|8643072|
|4|curl_7.86.0.src.tar <-- curl_7.85.0.src.tar|26275840|26030080|
|5|douyin_1.5.1.mac.tar <-- douyin_1.4.2.mac.tar|407940608|407642624|
|6|Emacs_28.2-universal.mac.tar <-- Emacs_27.2-3-universal.mac.tar|196380160|257496064|
|7|FFmpeg-n_5.1.2.src.tar <-- FFmpeg-n_4.4.3.src.tar|80527360|76154880|
|8|gcc_12.2.0.src.tar <-- gcc_11.3.0.src.tar|865884160|824309760|
|9|git_2.33.0-intel-universal-mavericks.mac.tar <-- 2.31.0|73302528|70990848|
|10|go_1.19.3.linux-amd64.tar <-- go_1.19.2.linux-amd64.tar|468835840|468796416|
|11|jdk_x64_mac_openj9_16.0.1_9_openj9-0.26.0.tar <-- 9_15.0.2_7-0.24.0|363765760|327188480|
|12|jre_1.8.0_351-linux-x64.tar <-- jre_1.8.0_311-linux-x64.tar|267796480|257996800|
|13|linux_5.19.9.src.tar <-- linux_5.15.80.src.tar|1269637120|1138933760|
|14|Minecraft_175.win.tar <-- Minecraft_172.win.tar|166643200|180084736|
|15|OpenOffice_4.1.13.mac.tar <-- OpenOffice_4.1.10.mac.tar|408364032|408336896|
|16|postgresql_15.1.src.tar <-- postgresql_14.6.src.tar|151787520|147660800|
|17|QQ_9.6.9.win.tar <-- QQ_9.6.8.win.tar|465045504|464837120|
|18|tensorflow_2.10.1.src.tar <-- tensorflow_2.8.4.src.tar|275548160|259246080|
|19|VSCode-win32-x64_1.73.1.tar <-- VSCode-win32-x64_1.69.2.tar|364025856|340256768|
|20|WeChat_3.8.0.41.win.tar <-- WeChat_3.8.0.33.win.tar|505876992|505018368|
   

**test PC**: Windows11, CPU R9-7945HX, SSD PCIe4.0x4 4T, DDR5 5200MHz 32Gx2   
**Program version**: hsynz 1.1.1, zsync 0.6.2  (more programs's testing see [HDiffPatch](https://github.com/sisong/HDiffPatch))   
**test Program**:   
**zsync** run make with `zsyncmake -b 2048 -o {out_newi} {new}`,   
client sync diff&patch by `zsync -i {old} -o {out_new} {newi}` (all files are local)   
**zsync -z** run make with `zsyncmake -b 2048 -z -u {new.gz} -o {out_newi} {new}`   
**hsynz** run make with `hsync_make -s-2k {new} {out_newi} [{-c-?} {out_newz}]`,    
client sync diff&patch by `hsync_demo {old} {newi} {newz} {out_new}` (all files are local)   
**hsynz p1** run make without compressor & out_newz , add `-p-1`   
**hsynz p8** run make without compressor & out_newz , add `-p-8`   
**hsynz p1 -zlib** run make with `-p-1 -c-zlib-9` (run `hsync_demo` with `-p-1`)   
**hsynz p8 -zlib** run make with `-p-8 -c-zlib-9` (run `hsync_demo` with `-p-8`)   
**hsynz p1 -gzip** run make with `-p-1 -c-gzip-9` (run `hsync_demo` with `-p-1`)   
**hsynz p8 -gzip** run make with `-p-8 -c-gzip-9` (run `hsync_demo` with `-p-8`)   
**hsynz p1 -ldef** run make with `-p-1 -c-ldef-12` (run `hsync_demo` with `-p-1`)   
**hsynz p8 -ldef** run make with `-p-8 -c-ldef-12` (run `hsync_demo` with `-p-8`)   
**hsynz p1 -lgzip** run make with `-p-1 -c-lgzip-12` (run `hsync_demo` with `-p-1`)   
**hsynz p8 -lgzip** run make with `-p-8 -c-lgzip-12` (run `hsync_demo` with `-p-8`)   
**hsynz p1 -zstd** run make with `-p-1 -c-zstd-21-24` (run `hsync_demo` with `-p-1`)   
**hsynz p8 -zstd** run make with `-p-8 -c-zstd-21-24` (run `hsync_demo` with `-p-8`)   
   
**test result average**:
|Program|compress|make mem|speed|sync mem|max mem|speed|
|:----|----:|----:|----:|----:|----:|----:|
|zsync|52.94%|1M|353.9MB/s|7M|23M|34MB/s|
|zsync -z|20.67%|1M|14.8MB/s|12M|37M|28MB/s|
|hsynz p1|51.05%|5M|2039.5MB/s|5M|19M|307MB/s|
|hsynz p8|51.05%|21M|4311.9MB/s|12M|27M|533MB/s|
|hsynz p1 zlib|20.05%|6M|17.3MB/s|6M|22M|273MB/s|
|hsynz p8 zlib|20.05%|30M|115.1MB/s|13M|29M|435MB/s|
|hsynz p1 gzip|20.12%|6M|17.3MB/s|6M|22M|268MB/s|
|hsynz p8 gzip|20.12%|30M|115.0MB/s|13M|29M|427MB/s|
|hsynz p1 ldef|19.57%|15M|7.8MB/s|6M|22M|272MB/s|
|hsynz p8 ldef|19.57%|96M|57.0MB/s|13M|29M|431MB/s|
|hsynz p1 lgzip|19.64%|15M|7.9MB/s|6M|22M|267MB/s|
|hsynz p8 lgzip|19.64%|96M|56.9MB/s|13M|29M|419MB/s|
|hsynz p1 zstd|14.96%|532M|1.9MB/s|24M|34M|264MB/s|
|hsynz p8 zstd|14.95%|3349M|10.1MB/s|24M|34M|410MB/s|
    

## input Apk Files for test: 
case list:
| |app|newFile <-- oldFile|newSize|oldSize|
|----:|:---:|:----|----:|----:|
|1|<img src="https://github.com/sisong/sfpatcher/raw/master/img/cn.wps.moffice_eng.png" width="36">|cn.wps.moffice_eng_13.30.0.apk <-- 13.29.0|95904918|94914262|
|2|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.achievo.vipshop.png" width="36">|com.achievo.vipshop_7.80.2.apk <-- 7.79.9|127395632|120237937|
|3|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.adobe.reader.png" width="36">|com.adobe.reader_22.9.0.24118.apk <-- 22.8.1.23587|27351437|27087718|
|4|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.alibaba.android.rimet.png" width="36">|com.alibaba.android.rimet_6.5.50.apk <-- 6.5.45|195314449|193489159|
|5|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.amazon.mShop.android.shopping.png" width="36">|com.amazon.mShop.android.shopping_24.18.2.apk <-- 24.18.0|76328858|76287423|
|6|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.baidu.BaiduMap.png" width="36">|com.baidu.BaiduMap_16.5.0.apk <-- 16.4.5|131382821|132308374|
|7|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.dragon.read.png" width="36">|com.dragon.read_5.5.3.33.apk <-- 5.5.1.32|45112658|43518658|
|8|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.ebay.mobile.png" width="36">|com.ebay.mobile_6.80.0.1.apk <-- 6.79.0.1|61202587|61123285|
|9|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.eg.android.AlipayGphone.png" width="36">|com.eg.android.AlipayGphone_10.3.0.apk <-- 10.2.96|122073135|119046208|
|10|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.google.android.apps.translate.png" width="36">|com.google.android.apps.translate_6.46.0.apk <-- 6.45.0|48892967|48843378|
|11|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.google.android.googlequicksearchbox.png" width="36">|com.google.android.googlequicksearchbox_13.38.11.apk <-- 13.37.10|190539272|189493966|
|12|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.jingdong.app.mall.png" width="36">|com.jingdong.app.mall_11.3.2.apk <-- 11.3.0|101098430|100750191|
|13|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.netease.cloudmusic.png" width="36">|com.netease.cloudmusic_8.8.45.apk <-- 8.8.40|181914846|181909451|
|14|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.reddit.frontpage.png" width="36">|com.reddit.frontpage_2022.36.0.apk <-- 2022.34.0|50205119|47854461|
|15|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.sankuai.meituan.takeoutnew.png" width="36">|com.sankuai.meituan.takeoutnew_7.94.3.apk <-- 7.92.2|74965893|74833926|
|16|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.sankuai.meituan.png" width="36">|com.sankuai.meituan_12.4.207.apk <-- 12.4.205|93613732|93605911|
|17|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.sina.weibo.png" width="36">|com.sina.weibo_12.10.0.apk <-- 12.9.5|156881776|156617913|
|18|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.smile.gifmaker.png" width="36">|com.smile.gifmaker_10.8.40.27845.apk <-- 10.8.30.27728|102403847|101520138|
|19|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.ss.android.article.news.png" width="36">|com.ss.android.article.news_9.0.7.apk <-- 9.0.6|54444003|53947221|
|20|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.ss.android.ugc.aweme.png" width="36">|com.ss.android.ugc.aweme_22.6.0.apk <-- 22.5.0|171683897|171353597|
|21|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.taobao.taobao.png" width="36">|com.taobao.taobao_10.18.10.apk <-- 10.17.0|117218670|117111874|
|22|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.tencent.mm.png" width="36">|com.tencent.mm_8.0.28.apk <-- 8.0.27|266691829|276603782|
|23|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.tencent.mobileqq.png" width="36">|com.tencent.mobileqq_8.9.15.apk <-- 8.9.13|311322716|310529631|
|24|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.tencent.mtt.png" width="36">|com.tencent.mtt_13.2.0.0103.apk <-- 13.2.0.0045|97342747|97296757|
|25|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.tripadvisor.tripadvisor.png" width="36">|com.tripadvisor.tripadvisor_49.5.apk <-- 49.3|28744498|28695346|
|26|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.twitter.android.png" width="36">|com.twitter.android_9.61.0.apk <-- 9.58.2|36141840|35575484|
|27|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.ubercab.png" width="36">|com.ubercab_4.442.10002.apk <-- 4.439.10002|69923232|64284150|
|28|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.ximalaya.ting.android.png" width="36">|com.ximalaya.ting.android_9.0.66.3.apk <-- 9.0.62.3|115804845|113564876|
|29|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.xunmeng.pinduoduo.png" width="36">|com.xunmeng.pinduoduo_6.30.0.apk <-- 6.29.1|30896833|30951567|
|30|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.youdao.dict.png" width="36">|com.youdao.dict_9.2.29.apk <-- 9.2.28|110624682|110628778|
|31|<img src="https://github.com/sisong/sfpatcher/raw/master/img/org.mozilla.firefox.png" width="36">|org.mozilla.firefox_105.2.0.apk <-- 105.1.0|83078464|83086656|
|32|<img src="https://github.com/sisong/sfpatcher/raw/master/img/tv.danmaku.bili.png" width="36">|tv.danmaku.bili_7.1.0.apk <-- 7.0.0|104774723|104727005|
   

**changed test Program**:   
**zsync ...** make `-b 2048` changed to `-b 1024`   
**hsynz ...** make `-s-2k` changed to `-s-1k`   

**test result average**:
|Program|compress|make mem|speed|sync mem|max mem|speed|
|:----|----:|----:|----:|----:|----:|----:|
|zsync|62.80%|1M|329.8MB/s|6M|12M|76MB/s|
|zsync -z|59.56%|1M|19.8MB/s|8M|19M|56MB/s|
|hsynz p1|62.43%|4M|1533.5MB/s|4M|10M|236MB/s|
|hsynz p8|62.43%|18M|2336.4MB/s|12M|18M|394MB/s|
|hsynz p1 zlib|58.67%|5M|22.7MB/s|4M|11M|243MB/s|
|hsynz p8 zlib|58.67%|29M|138.6MB/s|12M|19M|410MB/s|
|hsynz p1 gzip|58.95%|5M|22.6MB/s|4M|11M|242MB/s|
|hsynz p8 gzip|58.95%|29M|138.9MB/s|12M|19M|407MB/s|
|hsynz p1 ldef|58.61%|14M|23.7MB/s|4M|11M|242MB/s|
|hsynz p8 ldef|58.61%|96M|149.1MB/s|12M|19M|413MB/s|
|hsynz p1 lgzip|58.90%|14M|23.6MB/s|4M|11M|240MB/s|
|hsynz p8 lgzip|58.90%|96M|149.1MB/s|12M|19M|405MB/s|
|hsynz p1 zstd|57.74%|534M|2.7MB/s|24M|28M|234MB/s|
|hsynz p8 zstd|57.74%|3434M|13.4MB/s|24M|28M|390MB/s|
   

---
## Contact
housisong@hotmail.com  