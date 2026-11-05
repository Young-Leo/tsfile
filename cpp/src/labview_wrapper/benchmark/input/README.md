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

# 原九通道输入数据

`wave_60000x9_f64.bin` 与此前 Windows 九通道测试及 2026-11-05 测试文件包中的
输入逐字节相同。它是一块模拟数据，不包含设备采集数据。

- 大小：4,320,000 字节。
- 类型：小端序 IEEE 754 `DOUBLE`，无文件头。
- 形状：60,000 行 × 9 通道，行主序。
- SHA-256：`364e235e36faf3c3af153397a5d2a80dd3abbf705d8054d477312e01eb613265`。

测速时重复使用这一块数值，每块的时间戳继续递增；写入 25 块共 1,350 万点。
使用方法见上级目录的 README。
