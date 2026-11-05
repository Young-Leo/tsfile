#[[
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
]]

find_package(Threads REQUIRED)
add_library(tsfile_isol_vi SHARED tsfile_isol_vi.cc)
target_compile_definitions(tsfile_isol_vi PRIVATE TSFILE_ISOL_BUILDING)
target_include_directories(tsfile_isol_vi
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${PROJECT_SRC_DIR})
target_link_libraries(tsfile_isol_vi PRIVATE tsfile_labview tsfile Threads::Threads)
set_target_properties(tsfile_isol_vi PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH}
    RUNTIME_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH})

add_executable(tsfile_isol_benchmark benchmark_main.cc)
target_link_libraries(tsfile_isol_benchmark PRIVATE tsfile_isol_vi)
set_target_properties(tsfile_isol_benchmark PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH})

# Keep the matching shared libraries together when moving a Linux build.
# LD_LIBRARY_PATH in README also covers the core's versioned dependencies.
if (UNIX AND NOT APPLE)
    set_property(TARGET tsfile_isol_vi tsfile_isol_benchmark
                 APPEND PROPERTY BUILD_RPATH "$ORIGIN")
    set_property(TARGET tsfile_isol_vi tsfile_isol_benchmark
                 APPEND PROPERTY INSTALL_RPATH "$ORIGIN")
endif ()

install(TARGETS tsfile_isol_vi tsfile_isol_benchmark
        LIBRARY DESTINATION ${LIBRARY_OUTPUT_PATH}
        RUNTIME DESTINATION ${LIBRARY_OUTPUT_PATH})
install(FILES tsfile_isol_vi.h DESTINATION include)
