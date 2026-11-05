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

# LabVIEW 九通道模拟写入测试

这份示例用于在目标电脑或控制器上测试写 TsFile 的速度，不需要连接采集设备。
默认在测速库内生成一块九通道 `DOUBLE` 数据，再分块写入同一个 TsFile。
也可以读取 `input/wave_60000x9_f64.bin`，复用此前九通道测试的输入数据。

| 文件 | 用途 |
| --- | --- |
| `tsfile_isol_vi.cc/.h` | 测速共享库源码，Windows 生成 DLL，Linux 生成 SO |
| `build.sh` | Windows Git Bash / Linux 通用构建入口 |
| `vi/TsFile-9ch-VI-aligned-isolated.vi` | 已有 CLFN 节点和连线的可编辑示例 |
| `input/wave_60000x9_f64.bin` | 原九通道测试输入，一块 60,000 行 |
| `test_benchmark.py` | 写入、完整读回、计时输出和异常参数检查 |

## 1. 编译共享库

在仓库根目录执行。构建脚本需要 CMake 3.13 及以上、支持 C++11 的编译器和 Bash。
此构建关闭可选压缩库和 SQL 解析器，测试本身也使用无压缩配置。

**Linux：** 安装 GCC/G++、CMake、make，然后执行：

```bash
bash cpp/src/labview_wrapper/benchmark/build.sh build/labview-benchmark-linux
```

`build/labview-benchmark-linux/lib/` 下会得到：

```text
libtsfile_isol_vi.so       测速入口，供本示例 VI 调用
libtsfile_labview.so       批量写入接口
libtsfile.so*              TsFile 核心库及版本链接
tsfile_isol_benchmark     命令行写入和读回程序
```

三个共享库一起部署，保留 `libtsfile.so*` 的版本文件和符号链接。
运行环境还需要编译器对应的 C/C++ 运行库。
可用 `ldd build/labview-benchmark-linux/lib/libtsfile_isol_vi.so` 检查依赖是否齐全。

**Windows：** 在 Git Bash 中将 MinGW 的 `bin` 加入 PATH，选择与 LabVIEW
位数一致的工具链。下面的路径替换为实际安装位置：

```bash
export PATH="/c/path/to/mingw32/bin:$PATH"
bash cpp/src/labview_wrapper/benchmark/build.sh build/labview-benchmark-x86
```

64 位 LabVIEW 改用 x64 MinGW，并使用单独的 `build/labview-benchmark-x64` 目录。
`lib/` 下会得到 `libtsfile_isol_vi.dll`、`libtsfile_labview.dll`、`libtsfile.dll`
和 `tsfile_isol_benchmark.exe`。部署时还要带上该工具链的 MinGW 运行库；不要混用
x86 和 x64 文件。无需注册 DLL，也无需安装 ActiveX 组件来加载这个库。

**NI Linux Real-Time：** 使用目标系统对应的 NI 工具链和 sysroot；不能直接使用
为普通 Linux 电脑编译的库。构建入口支持附加 CMake 参数，例如：

```bash
bash cpp/src/labview_wrapper/benchmark/build.sh build/labview-benchmark-ni \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/target-toolchain.cmake
```

工具链文件需按目标控制器准备；系统、CPU 架构和 LabVIEW 位数均须匹配。
Windows DLL 不能直接改后缀当作 Linux SO 使用。

## 2. 先用少量数据检查

Linux 在仓库根目录运行：

```bash
export LD_LIBRARY_PATH="$PWD/build/labview-benchmark-linux/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
python3 cpp/src/labview_wrapper/benchmark/test_benchmark.py \
  --exe build/labview-benchmark-linux/lib/tsfile_isol_benchmark
```

Windows 在 Git Bash 中运行：

```bash
python cpp/src/labview_wrapper/benchmark/test_benchmark.py \
  --exe build/labview-benchmark-x86/lib/tsfile_isol_benchmark.exe
```

通过后显示 `PASS`。测试会逐点核对时间戳、九通道数值、空值标志和总行数，
并检查重复输出、错误输入文件、错误路径等情况。测试文件使用临时目录，结束后清理。

## 3. 在 LabVIEW 中运行 VI

打开 `vi/TsFile-9ch-VI-aligned-isolated.vi`，按 `Ctrl+E` 查看框图。
这份 VI 由 Windows 32 位 LabVIEW 2026 保存，目标 LabVIEW 需要能打开该版本。
首次打开时，原测试机的路径不可用，需修改下面两处：

1. 双击 CLFN，库路径选择本次编译的 `libtsfile_isol_vi.dll` 或
   `libtsfile_isol_vi.so`，函数选择 `lv_tsfile_write_demo_9ch`。
2. 将粉色字符串改为目标机上一个**尚不存在**的输出文件完整路径。
   上级目录要已存在，字符串末尾不要加回车或换行。

CLFN 配置如下；原 VI 已有这些参数和连线：

| 参数顺序 | CLFN 配置 | 含义 |
| --- | --- | --- |
| 返回值 | Signed 32-bit Integer | `0` 表示成功 |
| `output` | String → C String Pointer | 输出 TsFile 的完整路径 |
| `rows_per_block` | Signed 32-bit Integer，Value | 每块采样行数，每行固定九通道 |
| `blocks` | Signed 32-bit Integer，Value | 写入块数 |

调用约定选 **C**，线程设置选 **Run in UI thread**，一次只运行一个测速 VI。
先把两个蓝色整数改为 `1000` 和 `2`。运行成功后，再改为 `60000` 和 `25`，
即 150 万行、1,350 万个通道数值。它们表示数据量，没有一块必须耗时一秒的限制。

Linux 启动 LabVIEW 前，先在同一个终端设置上节的 `LD_LIBRARY_PATH`，
再从这个终端启动 LabVIEW，使其能找到三个共享库。
NI Linux Real-Time 则在开发电脑上配置并部署 VI，将库放到控制器可加载的目录。
Windows 的 Python/ActiveX 自动运行脚本不适用于 Linux；这份 VI 的 CLFN 调用不依赖 ActiveX。

调用链为：

```text
LabVIEW VI → CLFN → libtsfile_isol_vi
                       → libtsfile_labview → libtsfile → TsFile
```

测速库准备模拟输入并循环调用 `lv_tsfile_write_block_f64`，每块分别传入时间戳和九通道数组。
接入真实采集程序时，采集 VI 直接调用 `libtsfile_labview`，不需要经过测速库。
本目录的测速库正常链接上述库，不再需要旧包的 `libtsfile_lvbmark`、`libtsref1` 别名。

## 4. 使用原九通道输入复测

默认模拟数据由整数运算生成，便于 Windows/Linux 使用相同数值。
若要复用此前测试输入，在**启动 LabVIEW 前**设置：

```bash
export TSFILE_BENCH_INPUT="$PWD/cpp/src/labview_wrapper/benchmark/input/wave_60000x9_f64.bin"
```

此时 VI 的每块行数必须是 `60000`；块数可自行设置。输入文件是小端序、无文件头的
`DOUBLE` 数组，按“第一个时刻九个通道、第二个时刻九个通道……”排列。
库会检查文件大小恰好等于 `每块行数 × 9 × 8` 字节。取消环境变量即可恢复内生成数据：

```bash
unset TSFILE_BENCH_INPUT
```

独立命令行也可用同一个库写入和完整读回。Linux 示例：

```bash
mkdir -p run
build/labview-benchmark-linux/lib/tsfile_isol_benchmark \
  "$PWD/run/check.tsfile" 60000 25 \
  cpp/src/labview_wrapper/benchmark/input/wave_60000x9_f64.bin
build/labview-benchmark-linux/lib/tsfile_isol_benchmark --verify \
  "$PWD/run/check.tsfile" 60000 25 \
  cpp/src/labview_wrapper/benchmark/input/wave_60000x9_f64.bin
```

Windows 将程序路径换为对应 `lib/tsfile_isol_benchmark.exe`。
命令行程序用于检查库和文件；“LabVIEW 通过 CLFN 写 TsFile”的结果需由 LabVIEW 实际运行 VI 测得。

## 5. 查看速度与返回码

成功后生成 `check.tsfile` 和 `check.tsfile.metrics`。用文本编辑器查看 metrics：

| 字段 | 含义 |
| --- | --- |
| `STATUS` | `0` 表示成功，非零时不使用该轮性能数据 |
| `INPUT_MODE` | `generated` 为库内生成，`fixture` 为读取输入文件 |
| `TOTAL_ROWS` / `TOTAL_POINTS` | 总行数 / 九通道数值总数 |
| `WRAPPER_BLOCK_SECONDS` | 各次批量写入函数耗时之和 |
| `FULL_SECONDS` | 从建表、创建文件开始，到同步 flush、关闭返回的耗时 |
| `FULL_POINTS_PER_SECOND` | 总数值点数除以 `FULL_SECONDS`，作为完整路径吞吐 |
| `OUTPUT_BYTES` | 关闭后的 TsFile 文件字节数 |

输入文件读取或模拟数据生成在 `FULL_SECONDS` 计时前完成；每块缓冲区分配、复制、
时间戳准备在完整计时内。时间戳为 `0..总行数-1`，用于模拟有序采样。
数值采用 `PLAIN + UNCOMPRESSED`，时间戳采用 `TS_2DIFF + UNCOMPRESSED`。
关闭返回表示文件 API 已完成，不代表断电后必然保存。
如另行测量“VI 调用到返回”，会包含输入准备和 VI 调度等时间，需另列计时范围。

每次测试更换文件名。正式对比时使用同一输入、相同数据量和计时边界，先预热一轮，
再重复多轮取中位数。记录目标机配置、库版本、LabVIEW 版本及位数。

| 返回码 | 处理方法 |
| --- | --- |
| `-1` | 检查路径换行、行数和块数 |
| `-2` | TsFile 或 metrics 已存在，换一个输出文件名 |
| `-3` | 输入文件找不到或大小与每块行数不符 |
| `-4` | 检查输出目录、写权限和剩余空间 |
| `-5` | 内存不足，减小每块行数 |
| `-6` | 已有测速调用，等待结束 |
| `-7` | 库内异常，保留日志排查 |
| `-8` | 读回核对不一致，检查是否使用了相同输入和参数 |
| 正数 | TsFile 返回码，参见 `cpp/src/cwrapper/errno_define_c.h` |

测速会暂时调整 TsFile 进程级编码配置，结束后恢复。请单独运行测速，
不要与其他正在写入的 TsFile VI 共用同一个 LabVIEW 进程。
返回失败时可能留下不完整输出和 metrics，保留用于排查并更换新文件名重试。
