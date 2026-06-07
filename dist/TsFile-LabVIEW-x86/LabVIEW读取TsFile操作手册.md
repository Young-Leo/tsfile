# 在 LabVIEW 中读取 TsFile —— 操作手册

> 32 位 LabVIEW Community Edition 2026 Q1。整个读取只用 **1 个 Call Library Function Node**，
> 无 handle、无 64 位整数、无指针、无 While 循环，无需运行「Import Shared Library」向导。

---

## 0. 前置：文件已就位（已由后台准备好）

工作目录 `D:\ly\labview\tsfile_test\` 已包含运行所需的全部文件：

| 文件 | 作用 |
|------|------|
| `libtsfile_labview.dll` | 要调用的库（含一键函数 `lv_tsfile_dump_to_csv`） |
| `libtsfile.dll` | TsFile 核心引擎 |
| `libgcc_s_dw2-1.dll` / `libstdc++-6.dll` / `libwinpthread-1.dll` | 运行时依赖 |

这 5 个 dll **必须始终在同一目录**，否则报 LabVIEW Error 1097。

> 那些之前向导生成的 9 个 VI（在 `user.lib\libtsfile_labview\`）已不再使用，可忽略。

---

## 1. 放一个 Call Library Function Node

框图（Ctrl+E）右键 → **Connectivity → Libraries & Executables → Call Library Function Node**，放到框图上。

## 2. 双击节点进行配置

**Function 选项卡：**

| 项 | 填写 |
|----|------|
| Library name or path | `D:\ly\labview\tsfile_test\libtsfile_labview.dll` |
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

- `tsfile_path` ← 字符串常量：`d:\ly\tsfile\sandbox\file_from_32.tsfile`
- `csv_out_path` ← 字符串常量：`d:\ly\tsfile\sandbox\dump_out.csv`

（换成你自己的 .tsfile 路径和想要的 CSV 输出路径即可。）

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
time     device    temp   cnt
0        sensorA   20     0
1        sensorA   20.5   10
2        sensorA   21     20
3        sensorA   21.5   30
4        sensorA   22     40
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

## 进阶：以后要写入 TsFile

本 DLL 还导出了完整的写入函数（schema builder / writer / tablet 等，见 `tsfile_labview.h`）。
写入路径涉及 handle，建议同样用 CLFN 手动搭：
- handle 参数 → Numeric / **Unsigned 64-bit Integer (U64)** / Pass by Value
- `out_*` 输出 handle → U64 / **Pointer to Value**
- 时间/i64 → I64（Quad）
- 状态返回 → I32（Long）

需要时再说，可以同样封装成一个「一键写入」函数。
