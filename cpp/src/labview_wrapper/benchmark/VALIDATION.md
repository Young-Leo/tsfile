<!--
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements. See the NOTICE file
distributed with this work for additional information
regarding copyright ownership. The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License. You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied. See the License for the
specific language governing permissions and limitations
under the License.
-->

# Windows 验证记录（2026-11-05）

本次从工作区源码重新构建测速库、LabVIEW 封装库和 TsFile 核心库。
没有使用旧测速包里改名的 DLL。

| 项目 | Windows x86 | Windows x64 |
| --- | --- | --- |
| 编译器 | MinGW GCC 16.1.0 | MinGW GCC 8.1.0 |
| Release 编译三个 DLL 和命令行程序 | 通过 | 通过 |
| 库内生成数据、原三参数 CLFN 入口 | 通过 | 通过 |
| 二进制输入文件、环境变量指定输入 | 通过 | 通过 |
| 小数据完整读回及错误输入检查 | 通过 | 通过 |
| 60,000 行 × 九通道 × 25 块写入 | 通过 | 通过 |
| 150 万个时间戳、1,350 万个数值完整读回 | 全部一致 | 全部一致 |
| 另用已安装的 TsFile Python reader 核对上述文件 | 全部一致 | 全部一致 |
| 生成文件大小 | 110,064,618 B | 110,064,618 B |

原负载检查使用 `input/wave_60000x9_f64.bin`，数值 `PLAIN + UNCOMPRESSED`，
时间戳 `TS_2DIFF + UNCOMPRESSED`。这是编译和正确性验证，每个位数各跑一轮原负载，
没有重新执行此前 15 轮 TsFile/TDMS 性能对比。当前目录也不包含 TDMS 测试实现。

构建中修正了两处已有兼容问题：

- 内存页为对齐预留空间，修复 i686 整页和零长度分配返回空指针的问题。
  同时保护 32 位内存分配大小加上头部时的整数溢出，补充内存页回归测试。
- Windows 文件信息查询使用旧版 MinGW SDK 也支持的 API。

随附 VI 是此前的可编辑 Windows LabVIEW 2026 示例，调用函数名和三个参数均保留。
本次测试通过匹配位数的命令行程序检查该 C ABI；没有重新启动 LabVIEW 跑这版 DLL。
在 LabVIEW 打开后按 README 重新选择库和输出路径即可进行目标机测试。

Linux 提供同一份 C++11 源码、CMake 目标和构建脚本，平台差异限定在文件打开和文件大小查询。
本次未在 Linux 或 NI Linux Real-Time 上实测，不附 Linux 性能数值。
对方应按目标系统和 CPU 架构构建后，先执行 `test_benchmark.py`，再在 LabVIEW 运行 VI。
