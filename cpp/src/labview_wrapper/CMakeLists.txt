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

# LabVIEW-friendly shim DLL. Links against the existing `tsfile` shared lib.
message(STATUS "Running in src/labview_wrapper directory")

add_library(tsfile_labview SHARED tsfile_labview.cc)

# cwrapper headers are included as "cwrapper/tsfile_cwrapper.h".
target_include_directories(tsfile_labview PRIVATE ${PROJECT_SRC_DIR})

target_link_libraries(tsfile_labview PRIVATE tsfile)

set_target_properties(tsfile_labview PROPERTIES
    OUTPUT_NAME "tsfile_labview"
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON)

if (WIN32)
    set_target_properties(tsfile_labview PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif ()

install(TARGETS tsfile_labview
        LIBRARY DESTINATION ${LIBRARY_OUTPUT_PATH}
        RUNTIME DESTINATION ${LIBRARY_OUTPUT_PATH})
install(FILES tsfile_labview.h DESTINATION include)
