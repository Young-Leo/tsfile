<!--
Licensed to the Apache Software Foundation (ASF) under one or more
contributor license agreements. See the NOTICE file distributed with
this work for additional information regarding copyright ownership.
The ASF licenses this file to You under the Apache License, Version 2.0.
-->

# 在 LabVIEW 中读写 TsFile —— 操作手册

> 已在 32 位 LabVIEW Community Edition 2026 Q1 验证。最简单的读取和
> DOUBLE 整块写入都只需要 **1 个 Call Library Function Node**，无需运行
> 「Import Shared Library」向导。

---

## 0. 选择与 LabVIEW 位数一致的包

- 32 位 LabVIEW：使用 `dist/TsFile-LabVIEW-x86`。
- 64 位 LabVIEW：使用 `dist/TsFile-LabVIEW`。

把对应目录整体复制到 VI 或最终 EXE 的同一目录。即使 Windows 是
64 位，只要 LabVIEW 是 32 位，也必须使用 x86 包。

包内运行文件如下：

| 文件 | 作用 |
|------|------|
| `libtsfile_labview.dll` | 要调用的库（含一键读和整块写函数） |
| `libtsfile.dll` | TsFile 核心引擎 |
| `libgcc_s_*.dll` / `libstdc++-6.dll` / `libwinpthread-1.dll` | 运行时依赖 |
| `tsfile_labview.h` | C/C++ 调用头文件 |
| `tsfile_labview_lv.h` | 32 位 LabVIEW 导入向导友好头文件 |

这些 DLL **必须始终在同一目录**，否则通常报 LabVIEW Error 1097。

---

## 1. 放一个 Call Library Function Node

框图（Ctrl+E）右键 → **Connectivity → Libraries & Executables → Call Library Function Node**，放到框图上。

## 2. 双击节点进行配置

**Function 选项卡：**

| 项 | 填写 |
|----|------|
| Library name or path | 当前 VI 旁的 `libtsfile_labview.dll` |
| Function name | `lv_tsfile_dump_to_csv` |
| Calling convention | **C**（不是 stdcall） |

**Parameters 选项卡**（只有 3 项，全是最简单类型）：

| 参数 | Type | 具体设置 |
|------|------|---------|
| return type | Numeric | Data type = **Signed 32-bit Integer (I32)** |
| tsfile_path | String | String format = **C String Pointer** |
| csv_out_path | String | String format = **C String Pointer** |

点 **OK**。节点左侧出现 `tsfile_path`、`csv_out_path` 两个字符串输入端，右侧出现一个 I32 返回端。

> 不会再出现之前的 "Empty Cluster / void\*" 问题——CLFN 里类型由你直接选定。

## 3. 接入两个字符串常量

- `tsfile_path` ← 字符串常量：`C:\TsFileData\example.tsfile`
- `csv_out_path` ← 字符串常量：`C:\TsFileData\dump_out.csv`

（先创建父目录，再换成你自己的 `.tsfile` 路径和 CSV 输出路径。）

## 4. 读取生成的 CSV 并显示成表格

在 CLFN 右边放一个 **Read Delimited Spreadsheet.vi**
（Programming → File I/O）：

- `file path` ← 与上面相同的 `dump_out.csv` 常量
- `delimiter` ← 逗号 `,`
- 输出 `all rows` → 接一个 **2D 字符串数组 Indicator**（前面板会显示成表格）

**保证执行顺序**：让 CLFN 先执行、Read Spreadsheet 后执行。
最简单做法——把 CLFN 的 I32 返回端经一个顺序结构（Flat Sequence）或用错误线串起来，确保 CSV 写完再读。

## 5. 运行（Ctrl+R）

前面板表格应显示：

```
time     device    temp      cnt
1        sensorA   20        0
2        sensorA   20.4207   10
3        sensorA   20.4546   20
4        sensorA   20.0706   30
5        sensorA   19.6216   40
```

---

## 函数说明

```c
int32_t lv_tsfile_dump_to_csv(const char* tsfile_path,
                              const char* csv_out_path);
```

- 打开 `tsfile_path`，**自动发现第一个表及其所有列**，读取全部行，写成逗号分隔 CSV。
- CSV 第一行是列名；第一列 `time` 是时间戳。
- 返回 **0 = 成功**；非 0 = 错误码（最常见是文件路径不对）。
- 你**不需要**填表名或列名，函数内部自动枚举。

---

## 常见问题

| 现象 | 原因 / 处理 |
|------|-------------|
| Error 1097 | 5 个 dll 没在同一目录，或 Calling convention 没选 C |
| 返回值非 0 | `tsfile_path` 路径错误，或文件无表/无数据 |
| 表格为空 | CSV 还没写完就被读了 —— 用顺序结构强制 CLFN 先执行 |
| 找不到函数名 | Library path 没指向正确的 `libtsfile_labview.dll` |

---

## 整块数组写入（推荐）

高频采集不要在 LabVIEW 循环里逐格调用 `lv_tsfile_tablet_set_f64`。
新版 DLL 提供一次写入整块数组的函数：

### 最简单：一个 CLFN 完成一个 DOUBLE 文件

原型：

```c
int lv_tsfile_write_file_f64(const char* tsfile_path,
                             const char* table_name,
                             const char* column_names_newline_separated,
                             const long long* ts,
                             const double* data,
                             int nrows,
                             int ncols);
```

这个函数内部完成建表、打开 writer、整块写入和关闭，适合先跑通测试
或一次性写一个数组。CLFN 选择 **C calling convention**：

|参数|LabVIEW 类型|传递方式|
|---|---|---|
|return type|I32|值|
|tsfile_path|String|C String Pointer|
|table_name|String|C String Pointer|
|column_names_newline_separated|String|C String Pointer|
|ts|一维 I64 Array|**Array Data Pointer**|
|data|一维 DBL Array|**Array Data Pointer**|
|nrows|I32|值|
|ncols|I32|值|

9 通道列名字符串示例：

```text
front_x\nfront_y\nfront_z\nmid_x\nmid_y\nmid_z\nrear_x\nrear_y\nrear_z
```

分隔符必须是实际的 LF 换行符。在 LabVIEW 字符串常量中可启用
“`\` Codes Display”后输入 `\n`，或直接在各列名之间插入换行。

`data` 必须有 `nrows*ncols` 个元素，排列方式是
`data[row*ncols + col]`。二维采集数组先按“行=采样时刻、列=通道”
展平成一维数组。

输出目录必须已经存在，而且目标 `.tsfile` 不能是已有文件；写入前请
删除/改名旧文件，或使用新的滚动文件名。否则函数会返回非 0。

### 连续采集：writer 打开后每批调用一次

```c
int lv_tsfile_write_block_f64(unsigned long long writer,
                              const long long* ts,
                              const double* data,
                              int nrows,
                              int ncols);
```

- `ts`：长度为 `nrows` 的 I64 一维数组。
- `data`：长度为 `nrows * ncols` 的 DBL 一维数组，按行主序排列，
  即 `data[row*ncols + col]`。
- `nrows` / `ncols`：I32，必须与数组长度以及 writer 的列数一致。
- writer 的全部列必须都是 DOUBLE；INT32/FLOAT 分别调用
  `lv_tsfile_write_block_i32` / `lv_tsfile_write_block_f32`。
- 每次调用会在 DLL 内创建 tablet、填充、写入并释放；返回 0 表示成功。

CLFN 选择 **C calling convention**，参数配置如下：

|参数|LabVIEW 类型|传递方式|
|---|---|---|
|return type|I32|值|
|writer|U64|值|
|ts|一维 I64 Array|**Array Data Pointer**|
|data|一维 DBL Array|**Array Data Pointer**|
|nrows|I32|值|
|ncols|I32|值|

如果采集结果是二维数组，先确保布局为“每行一个采样时刻、每列一个通道”，
再展平成一维数组。9 通道、10 kHz 时，每批 `nrows=10000`、`ncols=9`，
LabVIEW 每秒只跨 DLL 边界一次。连续采集的调用顺序是：建 schema →
`writer_open` → 每批 `write_block_f64` → 文件滚动时 `writer_close`。

## 其他写入接口

本 DLL 还导出了完整的写入函数（schema builder / writer / tablet 等，见 `tsfile_labview.h`）。
写入路径涉及 handle，建议同样用 CLFN 手动搭：
- handle 参数 → Numeric / **Unsigned 64-bit Integer (U64)** / Pass by Value
- `out_*` 输出 handle → U64 / **Pointer to Value**
- 时间/i64 → I64（Quad）
- 状态返回 → I32（Long）

先跑通时优先使用上面的 `lv_tsfile_write_file_f64`；确认数据布局后再切换
到 writer 常驻的连续采集方式。
