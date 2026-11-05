#!/usr/bin/env bash
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements. See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to you under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy at
# https://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -euo pipefail
source_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)
build_dir=${1:-build/labview-benchmark}
if (( $# > 0 )); then shift; fi
generator=()
case "$(uname -s)" in
    MINGW*|MSYS*)
        generator=(-G "MinGW Makefiles"
                   "-DCMAKE_C_COMPILER=${CC:-gcc}"
                   "-DCMAKE_CXX_COMPILER=${CXX:-g++}")
        ;;
esac

cmake -S "$source_dir" -B "$build_dir" "${generator[@]}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TEST=OFF -DBUILD_TOOLS=OFF \
    -DBUILD_LABVIEW_WRAPPER=ON -DBUILD_LABVIEW_WRAPPER_TESTS=OFF \
    -DBUILD_LABVIEW_ISOLATED_BENCHMARK=ON \
    -DENABLE_ANTLR4=OFF -DENABLE_SIMD=OFF \
    -DENABLE_SNAPPY=OFF -DENABLE_LZ4=OFF -DENABLE_LZOKAY=OFF \
    -DENABLE_ZLIB=OFF -DENABLE_ZSTD=OFF -DENABLE_LZMA2=OFF "$@"
cmake --build "$build_dir" --target tsfile_isol_benchmark --parallel "${JOBS:-4}"
