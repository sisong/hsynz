# [hsynz](https://github.com/sisong/hsynz)
[![release](https://img.shields.io/badge/release-v0.9.1-blue.svg)](https://github.com/sisong/hsynz/releases) 
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/sisong/hsynz/blob/main/LICENSE) 
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-blue.svg)](https://github.com/sisong/hsynz/pulls)
[![+issue Welcome](https://img.shields.io/github/issues-raw/sisong/hsynz?color=green&label=%2Bissue%20welcome)](https://github.com/sisong/hsynz/issues)   

[![Build Status](https://github.com/sisong/hsynz/workflows/ci/badge.svg?branch=main)](https://github.com/sisong/hsynz/actions?query=workflow%3Aci+branch%3Amain)   

 中文版 | [english](README.md)   

hsynz 是一个用使用同步算法来进行增量更新的库，类似于[zsync](http://zsync.moria.org.uk)。   

适用的场景：旧版本数量非常多 或者 无法得到旧版本(没有保存或被修改等) 从而无法提前计算出全部的增量补丁，这时推荐使用hsync同步分发技术。    

服务端使用hsync_make对最新版本的数据进行一次处理，将新版本数据按块生成摘要信息文件(hsyni)，同时也可以选择对新版本数据分块进行压缩得到发布文件(hsynz)，如果不压缩新版本原文件就是hsynz等价文件。   

客户端先从服务端下载hsyni文件，根据自己的旧版本计算出需要下载的更新块，并根据hsyni中的信息得知这些块在hsynz中的位置，选择一种通讯方式从服务端的hsynz文件中按需下载，下载好的块和本地已有数据合并得到最新版本的数据。   

hsync_demo提供了一个测试客户端demo，用于本地文件测试。   
hsync_http提供了一个支持http(s)的下载客户端demo，支持从提供http(s)文件下载服务的服务端(比如CDN服务器)进行同步更新。   
提示：你也可以自定义其他通讯方式用于同步。   
   
---
## 和 [zsync](http://zsync.moria.org.uk) 对比
* 除了支持源和目标为文件，还为文件夹(目录)提供了支持。
* 除了支持zlib压缩发布包，还支持zstd压缩，提供更好的压缩率；即下载的补丁包更小。
* 对服务端的make速度进行了优化，并且提供了多线程并行加速的支持。
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
## 性能测试
  见 [HDiffPatch](https://github.com/sisong/HDiffPatch)

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
            也同样使用zlib算法来压缩, 但out_hsynz_file输出文件将是一个标准的.gz文件格式。
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
## 联系
housisong@hotmail.com  