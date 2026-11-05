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

#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "tsfile_isol_vi.h"

static bool parse_positive(const char* text, int32_t* result) {
    char* end = nullptr;
    errno = 0;
    const long value = std::strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value < 1 ||
        value > INT32_MAX)
        return false;
    *result = static_cast<int32_t>(value);
    return true;
}

int main(int argc, char** argv) {
    const bool verify = argc > 1 && std::strcmp(argv[1], "--verify") == 0;
    const bool compat = argc > 1 && std::strcmp(argv[1], "--compat") == 0;
    const int first = verify || compat ? 2 : 1;
    int32_t rows = 0;
    int32_t blocks = 0;
    if ((argc != first + 3 && (compat || argc != first + 4)) ||
        !parse_positive(argv[first + 1], &rows) ||
        !parse_positive(argv[first + 2], &blocks)) {
        std::fprintf(
            stderr,
            "usage: %s [--verify|--compat] output.tsfile rows-per-block blocks "
            "[input.bin]\n",
            argv[0]);
        return 2;
    }
    const char* input =
        argc == first + 4 ? argv[first + 3] : std::getenv("TSFILE_BENCH_INPUT");
    const int32_t status =
        compat
            ? lv_tsfile_write_demo_9ch(argv[first], rows, blocks)
            : (verify
                   ? lv_tsfile_isol_verify_9ch(argv[first], input, rows, blocks)
                   : lv_tsfile_isol_run_9ch(argv[first], input, rows, blocks));
    std::printf("%s_STATUS=%d\n", verify ? "VERIFY" : "WRITE", status);
    if (status != 0) return 1;
    return 0;
}
