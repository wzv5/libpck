libpck
======

[![Build status](https://ci.appveyor.com/api/projects/status/p4afro8y8h11mmqj?svg=true)](https://ci.appveyor.com/project/wzv5/libpck)

这是一个跨平台的、纯C++实现的、基于最新C++11、14标准（包括部分C++17标准）的诛仙pck文件包处理库。

特点： 
- 完整的功能支持，包括读取、解压、压缩、重建、增删改等
- 解压时自动使用多线程，效率最大化
- 增删改操作基于简单事务机制，缓存操作直到提交事务，提高效率
- PckItem文件对象分离，支持跨pck包传递PckItem文件对象

我写这个库的目的是，复习C++，同时练习掌握最新的C++标准，所以尽可能使用最新的C++标准来完成，避免使用操作系统相关的API。

代码中大量使用C++异常机制，由于我之前写过C#、Python，还是很喜欢用异常机制的，不过可能大部分只写C++的人不习惯这种用法，对此我也只能说，慢慢习惯吧，这是个好东西！

虽然这是一个练习性质的作品，但我依然努力让它更稳定、更好用，同时也会一直维护下去。

下载
====
已编译的dll及易语言模块：<https://github.com/wzv5/libpck/releases/latest>

编译
====

由于使用了最新的C++标准，所以编译器也必须用最新的。

本代码在 VS2015、VS2017、GCC 5.3（Arch Linux）测试通过，低于此版本的编译器可能会出错。

使用 VS2017 编译
----------------

使用 [vcpkg](https://github.com/Microsoft/vcpkg) 管理第三方依赖，请自行安装 vcpkg，并安装它的 [VS integrate](https://github.com/Microsoft/vcpkg/blob/master/docs/users/integration.md)。

```
vcpkg install zlib:x86-windows-static
vcpkg install zlib:x64-windows-static
```

打开 ```libpck.sln```，其中包含以下项目： 
- libpck：编译静态库
- libpck.dll：编译C接口的动态库
- pcktool：一个简单的控制台程序

使用 boost 代替 C++17 标准库中的 filesystem
------------------------------------------

默认使用 C++17 标准库中的 filesystem（还处于试验阶段），如果想要使用 boost，可以 `#define USE_BOOST` ，同时在项目属性中添加 boost 的相关目录。

使用
====

推荐使用C++接口，只需要 ```#include "pckfile.h"``` 即可，可选 ```#include "pckitem.h"```。

同时提供了C接口，```#include "pckfile_c.h"```，但不推荐使用，因为这降低了安全性，且不支持异常机制。

还提供有易语言接口，参考 ```ELanguageInterface``` 目录中的 ```libpck.e``` 和 ```pcktest.e```。

C++接口使用示例： 
```cpp
// 打开pck文件
auto pck = PckFile::Open("xxx.pck");
// 获取文件对象
auto& item = pck->GetSingleFileItem("文件名");
// 获取文件数据
auto data = item.GetData();
// 解压整个pck
pck->Extract("X:\\...\\解压目录");

// 从目录创建pck
PckFile::CreateFromDirectory("xxx.pck", "X:\\...");
// 重建pck（清除冗余数据）
PckFile::ReBuild("old.pck", "new.pck");
```

已知问题
========

<https://github.com/wzv5/libpck/issues>


以后的打算
=========

- 对压缩部分进行多线程改造
- 添加更多的辅助函数，如模糊搜索、从目录或其他压缩包更新pck
- 添加额外的哈希表存储文件名，实现更快的基于文件名的搜索
- 添加GUI编辑工具
- ~~所有内部字符串全部使用 Unicode 编码（貌似在Linux中这样干吃力不讨好？）~~

感谢
=====
stsm/liqf/李秋枫 开源的 [WinPCK](http://bbs.duowan.com/thread-27298877-1-1.html)，我参考了其中pck文件结构的定义，但由于他的代码是针对UI设计的，层层耦合较强，接口复杂，不易分离使用，所以我才决定完全重写一个只实现pck核心逻辑的库。

License
=======
[MIT](http://opensource.org/licenses/MIT)
