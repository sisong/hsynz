# [hsynz](https://github.com/sisong/hsynz)
[![release](https://img.shields.io/badge/release-v0.9.2-blue.svg)](https://github.com/sisong/hsynz/releases) 
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/sisong/hsynz/blob/main/LICENSE) 
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-blue.svg)](https://github.com/sisong/hsynz/pulls)
[![+issue Welcome](https://img.shields.io/github/issues-raw/sisong/hsynz?color=green&label=%2Bissue%20welcome)](https://github.com/sisong/hsynz/issues)   

[![Build Status](https://github.com/sisong/hsynz/workflows/ci/badge.svg?branch=main)](https://github.com/sisong/hsynz/actions?query=workflow%3Aci+branch%3Amain)   

 中文版 | [english](README.md)   

hsynz 是一个用使用同步算法来进行增量更新的库，类似于 [zsync](http://zsync.moria.org.uk)。   

适用的场景：旧版本数量非常多 或者 无法得到旧版本(没有保存或被修改等) 从而无法提前计算出全部的增量补丁，这时推荐使用hsynz同步分发技术。    

服务端使用hsync_make对最新版本的数据进行一次处理，将新版本数据按块生成摘要信息文件(hsyni)，同时也可以选择对新版本数据分块进行压缩得到发布文件(hsynz)，如果不压缩新版本原文件就是hsynz等价文件。   

客户端先从服务端或其他用户分享处下载hsyni文件，根据自己的旧版本计算出需要下载的更新块，并根据hsyni中的信息得知这些块在hsynz中的位置，选择一种通讯方式从服务端的hsynz文件中按需下载，下载好的块和本地已有数据合并得到最新版本的数据。   

hsync_demo提供了一个测试客户端demo，用于本地文件测试。   
hsync_http提供了一个支持http(s)的下载客户端demo，支持从提供http(s)文件下载服务的服务端(比如支持HTTP/1.1的多range请求的CDN服务器)进行同步更新。   
提示：你也可以自定义其他通讯方式用于同步。   
   
---
## 特性和 [zsync](http://zsync.moria.org.uk) 对比
* 除了支持源和目标为文件，还为文件夹(目录)提供了支持。
* 除了支持zlib压缩发布包，还支持zstd压缩，提供更好的压缩率；即下载的补丁包更小。
* 服务端make时提供了多线程并行加速的支持。
* 对客户端的diff速度进行了优化，并且提供了多线程并行加速的支持。

---
## 二进制发布包
[从 release 下载](https://github.com/sisong/hsynz/releases) : 分别运行在 Windows、Linux、MacOS操作系统的命令行程序。     
( 编译出这些发布文件的项目路径在 `hsynz/builds` )   

## 自己编译
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
用 [`Visual Studio`](https://visualstudio.microsoft.com) 打开 `hsynz/builds/vc/hsynz.sln` 编译   
   

---
## **hsync_make** 命令行使用说明:  
```
hsync_make: [options] newDataPath out_hsyni_file [out_hsynz_file]
  newDataPath可以是文件或目录, 如果newDataPath是文件并且不使用压缩参数 -c-... , 那out_hsynz_file
    参数可以为空.（因为这时out_hsynz_file和newDataPath文件完全一致，不需要重新再复制一个）
选项:
  -s-matchBlockSize
      匹配块大小matchBlockSize>=128, 默认为-s-2k, 推荐设置值 1024,4k,... (一般文件越大设置值建议越大)
  -b-safeBit
      设置允许patch时因hash冲突而失败的概率为: 1/2^safeBit;
      安全比特数safeBit>=14, 默认为 -b-24, 推荐 20,32... 
      (safeBit=24，也就是对任意一个旧版本大概有1/2^24的概率patch一定会失败；而值越大out_hsyni_file信息文件越大)
  -p-parallelThreadNumber
      设置线程数parallelThreadNumber>1时,开启多线程并行模式;
      默认为4;需要占用较多的内存。
  -c-compressType[-compressLevel]
      设置out_hsynz_file输出文件使用的压缩算法和压缩级别等, 默认不压缩;
      所有的压缩算法都支持单线程并行压缩;
      支持的压缩算法、压缩级别和字典大小等:
        -c-zlib[-{1..9}[-dictBits]]     默认级别 9
            压缩字典比特数dictBits可以为9到15, 默认为15。
        -c-gzip[-{1..9}[-dictBits]]     默认级别 9
            压缩字典比特数dictBits可以为9到15, 默认为15。
            也同样使用zlib算法来压缩, 但out_hsynz_file输出文件将是一个标准的.gz格式文件。
            （会比 -c-zlib 生成的文件稍大一点）
        -c-zstd[-{10..22}[-dictBits]]   默认级别 21
            压缩字典比特数dictBits 可以为15到30, 默认为24。
  -C-checksumType
      设置块数据的强校验算法, 默认为-C-xxh128;
      支持的校验选项:
        -C-xxh128
        -C-md5
        -C-sha512
        -C-sha256
        -C-crc32
            警告: crc32不够强和安全!
  -n-maxOpenFileNumber
      当newDataPath为文件夹时，设置最大允许同时打开的文件数;
      maxOpenFileNumber>=8, 默认为48; 合适的限制值可能不同系统下不同。
  -g#ignorePath[#ignorePath#...]
      当newDataPath为文件夹时，设置忽略路径(路径可能是文件或文件夹); 忽略路径列表的格式如下:
        #.DS_Store#desktop.ini#*thumbs*.db#.git*#.svn/#cache_*/00*11/*.tmp
      # 意味着路径名称之间的间隔; (如果名称中有“#”号, 需要改写为“#:” )
      * 意味着匹配名称中的任意一段字符; (如果名称中有“*”号, 需要改写为“*:” )
      / 如果该符号放在名称末尾,意味着必须匹配文件夹;
  -f  强制文件写覆盖, 忽略输出的路径是否已经存在;
      默认不执行覆盖, 如果输出路径已经存在, 直接返回错误;
      如果设置了-f,但路径已经存在并且是一个文件夹, 那么会始终返回错误。
  --patch
      切换命令行到hsync_demo模式; 可以支持hsync_demo命令行的相关参数和功能。
  -v  输出程序版本信息。
  -h 或 -?
      输出命令行帮助信息 (该说明)。
```

## **hsync_http** command line usage:
```
下载文件    : [options] -dl#hsyni_file_url hsyni_file
创建本地补丁: [options] oldPath hsyni_file hsynz_file_url -diff#diffFile
本地打补丁  : [options] oldPath hsyni_file -patch#diffFile outNewPath
显示同步信息: [options] oldPath hsyni_file
同步打补丁  : [options] oldPath [-dl#hsyni_file_url] hsyni_file hsynz_file_url outNewPath [-diffi#cacheTempFile] 
  oldPath可以是文件或文件夹；oldPath可以为空, 输入参数为 ""
选项:
  -dl#hsyni_file_url
    在同步打补丁开始前，将hsyni_file_url对应的文件下载为本地文件hsyni_file; 
  -diff#outDiffFile
    开始打补丁前，创建oldPath到hsyni_file描述的新版间的diffFile补丁文件，本地没有的块按需从hsynz_file_url下载;
  -patch#diffFile
    对oldPath应用diffFile补丁文件后得到outNewPath;
  -cdl
    开启断点续传；默认关闭;
  -p-parallelThreadNumber
    设置线程数parallelThreadNumber>1时,开启多线程并行模式;
    默认为4;
    注意: 当前下载数据时使用的是单线程。
  -n-maxOpenFileNumber
      当路径为文件夹时，设置最大允许同时打开的文件数;
      maxOpenFileNumber>=8, 默认为24; 合适的限制值可能不同系统下不同。
  -g#ignorePath[#ignorePath#...]
      当oldPath为文件夹时，设置忽略路径(路径可能是文件或文件夹); 忽略路径列表的格式如下:
        #.DS_Store#desktop.ini#*thumbs*.db#.git*#.svn/#cache_*/00*11/*.tmp
      # 意味着路径名称之间的间隔; (如果名称中有“#”号, 需要改写为“#:” )
      * 意味着匹配名称中的任意一段字符; (如果名称中有“*”号, 需要改写为“*:” )
      / 如果该符号放在名称末尾,意味着必须匹配文件夹;
  -f  强制文件写覆盖, 忽略输出的路径是否已经存在;
      默认不执行覆盖, 如果输出路径已经存在, 直接返回错误;
      如果设置了-f,但outNewPath已经存在并且是一个文件:
        如果patch输出一个文件, 那么会执行写覆盖;
        如果patch输出一个文件夹, 那么会始终返回错误。
      如果设置了-f,但outNewPath已经存在并且是一个文件夹:
        如果patch输出一个文件, 那么会始终返回错误;
        如果patch输出一个文件夹, 那么会执行写覆盖, 但不会删除文件夹中已经存在的无关文件。
  -v  输出程序版本信息。
  -h 或 -?
      输出命令行帮助信息 (该说明)。
```

## **hsync_demo** command line usage:
   该程序功能用于本地同步测试，用本地文件代替实际中的URL远程文件，详见hsync_http命令行。
   

---
## hsynz 和 [zsync](http://zsync.moria.org.uk) 性能对比:
测试用例([从 OneDrive 下载](https://1drv.ms/u/s!Aj8ygMPeifoQgUIZxYac5_uflNoN)):   
| |新版本文件 <-- 旧版本|新版本大小|旧版本大小|
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
   

**测试PC**: Windows11, CPU Ryzen 5800H, SSD Disk, Memroy 8G*2 DDR4 3200MHz   
**参与测试的程序版本**: hsynz 0.9.1, zsync 0.6.2  (更多程序的测试结果见 [HDiffPatch](https://github.com/sisong/HDiffPatch))   
**程序测试参数**:   
**zsync** 运行 make 参数 `zsyncmake -b 2048 -o {out_newi} {new}`,   
客户端同步 diff&patch 时参数 `zsync -i {old} -o {out_new} {newi}` (所有文件都在本地)   
**zsync -z** 运行 make 参数 `zsyncmake -b 2048 -z -u {new.gz} -o {out_newi} {new}`   
**hsynz** 运行 make 参数 `hsync_make -s-2k {new} {out_newi} [{-c-?} {out_newz}]`,   
客户端同步 diff&patch 时参数 `hsync_demo {old} {newi} {newz} {out_new}` (所有文件都在本地)   
**hsynz p1** 运行 make 时没有压缩器，也没有压缩文件out_newz输出, 添加了参数 `-p-1`   
**hsynz p8** 运行 make 时没有压缩器，也没有压缩文件out_newz输出, 添加了参数 `-p-8`   
**hsynz p1 -zlib** 运行 make 时添加 `-p-1 -c-zlib-9` (运行 `hsync_demo` 时添加 `-p-1`) 
**hsynz p8 -zlib** 运行 make 时添加 `-p-8 -c-zlib-9` (运行 `hsync_demo` 时添加 `-p-8`)   
**hsynz p1 -gzip** 运行 make 时添加 `-p-1 -c-gzip-9` (运行 `hsync_demo` 时添加 `-p-1`) 
**hsynz p8 -gzip** 运行 make 时添加 `-p-8 -c-gzip-9` (运行 `hsync_demo` 时添加 `-p-8`)   
**hsynz p1 -zstd** 运行 make 时添加 `-p-1 -c-zstd-21-24` (运行 `hsync_demo` 时添加 `-p-1`) 
**hsynz p8 -zstd** 运行 make 时添加 `-p-8 -c-zstd-21-24` (运行 `hsync_demo` 时添加 `-p-8`)   
   
**测试平均结果**:
|程序|压缩率|make内存|速度|sync内存|最大内存|速度|
|:----|----:|----:|----:|----:|----:|----:|
|zsync|52.94%|1M|285.1MB/s|7M|23M|25MB/s|
|zsync -z|20.67%|1M|11.9MB/s|12M|37M|21MB/s|
|hsynz p1|51.05%|5M|1687.8MB/s|5M|19M|222MB/s|
|hsynz p8|51.05%|22M|3016.1MB/s|13M|27M|391MB/s|
|hsynz p1 -zlib|20.05%|6M|14.3MB/s|6M|21M|172MB/s|
|hsynz p8 -zlib|20.05%|30M|89.8MB/s|13M|29M|254MB/s|
|hsync p1 -gzip|20.12%|6M|14.2MB/s|6M|21M|171MB/s|
|hsync p8 -gzip|20.12%|30M|89.1MB/s|13M|29M|251MB/s|
|hsynz p1 -zstd|14.90%|532M|1.3MB/s|24M|35M|192MB/s|
|hsynz p8 -zstd|14.90%|3349M|5.1MB/s|24M|35M|301MB/s|
    

## 输入Apk文件进行测试: 
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
   

**调整测试程序**:   
**zsync ...** make 参数 `-b 2048` 修改为 `-b 1024`   
**hsynz ...** make 参数 `-s-2k` 修改为 `-s-1k`   

**测试平均结果**:
|程序|压缩率|make内存|速度|sync内存|最大内存|速度|
|:----|----:|----:|----:|----:|----:|----:|
|zsync|62.80%|1M|270.9MB/s|6M|12M|62MB/s|
|zsync -z|59.56%|1M|15.8MB/s|8M|19M|50MB/s|
|hsynz p1|62.43%|4M|1243.4MB/s|4M|10M|172MB/s|
|hsynz p8|62.43%|25M|1902.6MB/s|12M|18M|293MB/s|
|hsynz p1 -zlib|58.67%|5M|18.5MB/s|4M|11M|170MB/s|
|hsynz p8 -zlib|58.67%|29M|107.6MB/s|12M|19M|285MB/s|
|hsync p1 -gzip|58.95%|5M|18.4MB/s|4M|11M|165MB/s|
|hsync p8 -gzip|58.95%|29M|106.6MB/s|12M|19M|266MB/s|
|hsynz p1 -zstd|57.92%|534M|2.2MB/s|24M|28M|173MB/s|
|hsynz p8 -zstd|57.92%|3434M|7.6MB/s|24M|28M|294MB/s|
   

---
## 联系
housisong@hotmail.com  