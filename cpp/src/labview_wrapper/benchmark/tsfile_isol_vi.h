/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LABVIEW_BENCHMARK_TSFILE_ISOL_VI_H_
#define LABVIEW_BENCHMARK_TSFILE_ISOL_VI_H_

#include <stdint.h>

#if defined(_WIN32) && defined(TSFILE_ISOL_BUILDING)
#define TSFILE_ISOL_API __declspec(dllexport)
#elif defined(_WIN32)
#define TSFILE_ISOL_API __declspec(dllimport)
#else
#define TSFILE_ISOL_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Same C/cdecl signature as TsFile-9ch-VI-aligned-isolated.vi.
 * TSFILE_BENCH_INPUT selects an exact rows_per_block * 9 little-endian
 * DOUBLE fixture. If unset/empty, data is generated before timing starts.
 * Writes output and output.metrics; neither file may already exist.
 * 0 = success; positive = TsFile status; negative = benchmark error:
 * -1 invalid argument, -2 existing output, -3 input error, -4 metrics I/O,
 * -5 allocation failure, -6 another benchmark is running, -7 exception.
 * Run in a separate process from other TsFile writers: codec configuration
 * is process-wide. Calls within this benchmark library are serialized. */
TSFILE_ISOL_API int32_t lv_tsfile_write_demo_9ch(const char* output,
                                                 int32_t rows_per_block,
                                                 int32_t blocks);

/* Explicit fixture version for command-line clients. NULL/empty = generated.
 * This unique name avoids ambiguity with older demonstration libraries. */
TSFILE_ISOL_API int32_t lv_tsfile_isol_run_9ch(const char* output,
                                               const char* input,
                                               int32_t rows_per_block,
                                               int32_t blocks);

/* Full readback of all timestamps, nine values per row, null flags and EOF.
 * Uses the same input selection as the explicit run function; not timed.
 * Returns 0 on success, -8 on a mismatch, or another status as above. */
TSFILE_ISOL_API int32_t lv_tsfile_isol_verify_9ch(const char* output,
                                                  const char* input,
                                                  int32_t rows_per_block,
                                                  int32_t blocks);

#ifdef __cplusplus
}
#endif

#endif  // LABVIEW_BENCHMARK_TSFILE_ISOL_VI_H_
