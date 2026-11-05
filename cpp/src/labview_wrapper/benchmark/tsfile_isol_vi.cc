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

#include "tsfile_isol_vi.h"

#include <fcntl.h>
#include <sys/stat.h>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "cwrapper/tsfile_cwrapper.h"
#include "labview_wrapper/tsfile_labview.h"

namespace {

const int32_t kColumns = 9;
const char* const kColumnList =
    "ch_0\nch_1\nch_2\nch_3\nch_4\nch_5\nch_6\nch_7\nch_8";
std::mutex benchmark_mutex;
typedef std::chrono::steady_clock Clock;
typedef std::unique_ptr<FILE, int (*)(FILE*)> File;

double seconds_since(Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}

bool valid_path(const char* path) {
    return path != nullptr && path[0] != '\0' &&
           std::strpbrk(path, "\r\n") == nullptr;
}

bool valid_size(int32_t rows, int32_t blocks) {
    return rows > 0 && blocks > 0 &&
           static_cast<uint64_t>(rows) <= std::numeric_limits<size_t>::max() /
                                              (kColumns * sizeof(double)) &&
           static_cast<int64_t>(rows) * blocks <=
               std::numeric_limits<int64_t>::max() / kColumns;
}

// Integer arithmetic and binary fractions give identical data on every CPU.
double generated_value(uint32_t row, uint32_t column) {
    uint32_t noise = row * 1664525u + column * 1013904223u + 12345u;
    noise ^= noise >> 13;
    const int32_t wave =
        static_cast<int32_t>((row * (column + 1u) * 37u) & 65535u) - 32768;
    return column + wave / 32768.0 + (noise & 1023u) / 1048576.0;
}

int load_values(const char* input, int32_t rows, std::vector<double>* values) {
    values->resize(static_cast<size_t>(rows) * kColumns);
    if (input == nullptr || input[0] == '\0') {
        for (int32_t row = 0; row < rows; ++row) {
            for (int32_t col = 0; col < kColumns; ++col) {
                (*values)[static_cast<size_t>(row) * kColumns + col] =
                    generated_value(row, col);
            }
        }
        return 0;
    }
    if (!valid_path(input)) return -1;
    File file(std::fopen(input, "rb"), &std::fclose);
    if (!file ||
        std::fread(values->data(), sizeof(double), values->size(),
                   file.get()) != values->size() ||
        std::fgetc(file.get()) != EOF || std::ferror(file.get())) {
        return -3;
    }
    const uint16_t endian = 1;
    if (*reinterpret_cast<const unsigned char*>(&endian) != 1) {
        for (size_t i = 0; i < values->size(); ++i) {
            unsigned char* bytes =
                reinterpret_cast<unsigned char*>(&(*values)[i]);
            for (size_t j = 0; j < sizeof(double) / 2; ++j) {
                const unsigned char tmp = bytes[j];
                bytes[j] = bytes[sizeof(double) - j - 1];
                bytes[sizeof(double) - j - 1] = tmp;
            }
        }
    }
    return 0;
}

FILE* create_metrics(const std::string& path) {
#ifdef _WIN32
    const int fd =
        _open(path.c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY,
              _S_IREAD | _S_IWRITE);
    if (fd < 0) return nullptr;
    FILE* file = _fdopen(fd, "wb");
    if (file == nullptr) _close(fd);
#else
    const int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd < 0) return nullptr;
    FILE* file = fdopen(fd, "wb");
    if (file == nullptr) close(fd);
#endif
    return file;
}

// Restore caller settings even when a write fails. Other TsFile writers must
// not run concurrently because the core's codec settings are process-wide.
struct Codecs {
    uint8_t value_encoding = get_datatype_encoding(TS_DATATYPE_DOUBLE);
    uint8_t value_compression = get_global_compression();
    uint8_t time_encoding = get_global_time_encoding();
    uint8_t time_compression = get_global_time_compression();

    int configure() {
        int status =
            set_datatype_encoding(TS_DATATYPE_DOUBLE, TS_ENCODING_PLAIN);
        if (status == 0)
            status = set_global_compression(TS_COMPRESSION_UNCOMPRESSED);
        if (status == 0)
            status = set_global_time_encoding(TS_ENCODING_TS_2DIFF);
        if (status == 0)
            status = set_global_time_compression(TS_COMPRESSION_UNCOMPRESSED);
        return status;
    }

    ~Codecs() {
        set_datatype_encoding(TS_DATATYPE_DOUBLE, value_encoding);
        set_global_compression(value_compression);
        set_global_time_encoding(time_encoding);
        set_global_time_compression(time_compression);
    }
};

struct Writer {
    LV_Handle builder = 0;
    LV_Handle handle = 0;

    int open_file(const char* output) {
        builder = lv_tsfile_schema_builder_new("vib");
        if (builder == 0) return -5;
        for (int32_t col = 0; col < kColumns; ++col) {
            const std::string name = "ch_" + std::to_string(col);
            const int status = lv_tsfile_schema_builder_add_column(
                builder, name.c_str(), LV_TYPE_DOUBLE, LV_CAT_FIELD);
            if (status != 0) return status;
        }
        const int status = lv_tsfile_writer_open(output, builder, 0, &handle);
        lv_tsfile_schema_builder_free(builder);
        builder = 0;
        return status;
    }

    int close_file() {
        const LV_Handle current = handle;
        handle = 0;
        return lv_tsfile_writer_close(current);
    }

    ~Writer() {
        if (handle != 0) lv_tsfile_writer_close(handle);
        if (builder != 0) lv_tsfile_schema_builder_free(builder);
    }
};

int run(const char* output, const char* input, int32_t rows, int32_t blocks) {
    if (!valid_path(output) || !valid_size(rows, blocks)) return -1;
    struct stat info;
    if (stat(output, &info) == 0) return -2;
    if (errno != ENOENT) return -4;
    std::vector<double> source;
    int status = load_values(input, rows, &source);
    if (status != 0) return status;

    File metrics(create_metrics(std::string(output) + ".metrics"),
                 &std::fclose);
    if (!metrics) return errno == EEXIST ? -2 : -4;
    Codecs codecs;
    status = codecs.configure();
    double open_seconds = 0, prepare_seconds = 0, block_seconds = 0;
    double write_seconds = 0, flush_seconds = 0, close_seconds = 0;
    const Clock::time_point full_start = Clock::now();
    {
        Writer writer;
        if (status == 0) status = writer.open_file(output);
        open_seconds = seconds_since(full_start);
        const Clock::time_point write_start = Clock::now();
        for (int32_t block = 0; status == 0 && block < blocks; ++block) {
            Clock::time_point phase = Clock::now();
            // Keep allocation, the source copy and timestamp preparation inside
            // the full path, as in the original nine-channel benchmark.
            std::vector<double> values(source);
            std::vector<int64_t> timestamps(static_cast<size_t>(rows));
            for (int32_t row = 0; row < rows; ++row)
                timestamps[row] = static_cast<int64_t>(block) * rows + row;
            prepare_seconds += seconds_since(phase);
            phase = Clock::now();
            status = lv_tsfile_write_block_f64(writer.handle, timestamps.data(),
                                               values.data(), rows, kColumns);
            block_seconds += seconds_since(phase);
        }
        write_seconds = seconds_since(write_start);
        if (status == 0) {
            const Clock::time_point phase = Clock::now();
            status = lv_tsfile_writer_flush(writer.handle);
            flush_seconds = seconds_since(phase);
        }
        if (writer.handle != 0) {
            const Clock::time_point phase = Clock::now();
            const int close_status = writer.close_file();
            close_seconds = seconds_since(phase);
            if (status == 0) status = close_status;
        }
    }
    const double full_seconds = seconds_since(full_start);
    const int64_t total_rows = static_cast<int64_t>(rows) * blocks;
    const int64_t points = total_rows * kColumns;
#ifdef _WIN32
    struct _stat64 output_info;
    const int stat_status = _stat64(output, &output_info);
#else
    struct stat output_info;
    const int stat_status = stat(output, &output_info);
#endif
    if (status == 0 && stat_status != 0) status = -4;
    std::fprintf(
        metrics.get(),
        "STATUS=%d\nINPUT_MODE=%s\nROWS_PER_BLOCK=%d\nCOLS=9\n"
        "BLOCKS=%d\nTOTAL_ROWS=%lld\nTOTAL_POINTS=%lld\nOUTPUT_BYTES=%lld\n"
        "VALUE_ENCODING=plain\nVALUE_COMPRESSION=uncompressed\n"
        "TIME_ENCODING=ts2diff\nTIME_COMPRESSION=uncompressed\n"
        "OPEN_SECONDS=%.9f\nPREPARE_SECONDS=%.9f\n"
        "WRAPPER_BLOCK_SECONDS=%.9f\nWRITE_SECONDS=%.9f\n"
        "FLUSH_SECONDS=%.9f\nCLOSE_SECONDS=%.9f\nFULL_SECONDS=%.9f\n"
        "FULL_POINTS_PER_SECOND=%.3f\nCORE_POINTS_PER_SECOND=%.3f\n",
        status, input && input[0] ? "fixture" : "generated", rows, blocks,
        static_cast<long long>(total_rows), static_cast<long long>(points),
        stat_status == 0 ? static_cast<long long>(output_info.st_size) : -1LL,
        open_seconds, prepare_seconds, block_seconds, write_seconds,
        flush_seconds, close_seconds, full_seconds,
        status == 0 && full_seconds > 0 ? points / full_seconds : 0.0,
        status == 0 && block_seconds > 0 ? points / block_seconds : 0.0);
    const bool metrics_error = std::ferror(metrics.get()) != 0;
    if (std::fclose(metrics.release()) != 0 || metrics_error) return -4;
    return status;
}

struct Reader {
    LV_Handle handle = 0;
    LV_Handle result = 0;
    ~Reader() {
        if (result != 0) lv_tsfile_rs_free(result);
        if (handle != 0) lv_tsfile_reader_close(handle);
    }
};

int verify(const char* output, const char* input, int32_t rows,
           int32_t blocks) {
    if (!valid_path(output) || !valid_size(rows, blocks)) return -1;
    std::vector<double> source;
    int status = load_values(input, rows, &source);
    if (status != 0) return status;
    Reader reader;
    status = lv_tsfile_reader_open(output, &reader.handle);
    if (status != 0) return status;
    const int32_t batch = 4096;
    const int64_t total_rows = static_cast<int64_t>(rows) * blocks;
    status = lv_tsfile_query_table_batch(
        reader.handle, "vib", kColumnList, std::numeric_limits<int64_t>::min(),
        std::numeric_limits<int64_t>::max(), batch, &reader.result);
    if (status != 0) return status;
    std::vector<int64_t> timestamps(batch);
    std::vector<double> values(batch * kColumns);
    std::vector<uint8_t> nulls(batch * kColumns);
    int64_t offset = 0;
    for (;;) {
        int32_t count = 0;
        status = lv_tsfile_rs_read_block_f64(reader.result, timestamps.data(),
                                             values.data(), nulls.data(), batch,
                                             kColumns, &count);
        if (status != 0) return status;
        if (count == 0) return offset == total_rows ? 0 : -8;
        if (count < 0 || count > batch || offset + count > total_rows)
            return -8;
        for (int32_t row = 0; row < count; ++row) {
            if (timestamps[row] != offset + row) return -8;
            const size_t expected =
                static_cast<size_t>((offset + row) % rows) * kColumns;
            const size_t actual = static_cast<size_t>(row) * kColumns;
            for (int32_t col = 0; col < kColumns; ++col) {
                if (nulls[actual + col] ||
                    std::memcmp(&values[actual + col], &source[expected + col],
                                sizeof(double)) != 0)
                    return -8;
            }
        }
        offset += count;
    }
}

template <typename Operation>
int32_t guarded(Operation operation) noexcept {
    try {
        std::unique_lock<std::mutex> lock(benchmark_mutex, std::try_to_lock);
        if (!lock.owns_lock()) return -6;
        return operation();
    } catch (const std::bad_alloc&) {
        return -5;
    } catch (const std::length_error&) {
        return -5;
    } catch (...) {
        return -7;
    }
}

}  // namespace

extern "C" {

int32_t lv_tsfile_write_demo_9ch(const char* output, int32_t rows_per_block,
                                 int32_t blocks) {
    return lv_tsfile_isol_run_9ch(output, std::getenv("TSFILE_BENCH_INPUT"),
                                  rows_per_block, blocks);
}

int32_t lv_tsfile_isol_run_9ch(const char* output, const char* input,
                               int32_t rows_per_block, int32_t blocks) {
    return guarded(
        [&]() { return run(output, input, rows_per_block, blocks); });
}

int32_t lv_tsfile_isol_verify_9ch(const char* output, const char* input,
                                  int32_t rows_per_block, int32_t blocks) {
    return guarded(
        [&]() { return verify(output, input, rows_per_block, blocks); });
}

}  // extern "C"
