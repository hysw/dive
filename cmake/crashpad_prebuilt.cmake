#
# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set(DIVE_CRASHPAD_PREBUILT_INCLUDE_PATH
    "/repo/crashpad"
    CACHE STRING
    "Prebuilt crashpad"
)

set(DIVE_CRASHPAD_PREBUILT_ARTIFACT_PATH
    "/repo/crashpad/out/Default"
    CACHE STRING
    "Prebuilt crashpad"
)

if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  # Not implemented
  return()
endif()

if(NOT EXISTS "${DIVE_CRASHPAD_PREBUILT_INCLUDE_PATH}")
  return()
endif()

if(NOT EXISTS "${DIVE_CRASHPAD_PREBUILT_ARTIFACT_PATH}")
  return()
endif()

message(CHECK_START "Generate build files for crashpad_prebuilt.cmake")
list(APPEND CMAKE_MESSAGE_INDENT "  ")

set(DIVE_CRASHPAD_USE_PREBUILT TRUE)
cmake_path(SET CRASHPAD_HANDLER_PATH "${DIVE_CRASHPAD_PREBUILT_ARTIFACT_PATH}/crashpad_handler")
cmake_path(SET CRASHPAD_LIBBASE_PATH "${DIVE_CRASHPAD_PREBUILT_ARTIFACT_PATH}/obj/third_party/mini_chromium/mini_chromium/base/libbase.a")
cmake_path(SET CRASHPAD_LIBCOMMON_PATH "${DIVE_CRASHPAD_PREBUILT_ARTIFACT_PATH}/obj/client/libcommon.a")
cmake_path(SET CRASHPAD_LIBCLIENT_PATH "${DIVE_CRASHPAD_PREBUILT_ARTIFACT_PATH}/obj/client/libclient.a")
cmake_path(SET CRASHPAD_LIBUTIL_PATH "${DIVE_CRASHPAD_PREBUILT_ARTIFACT_PATH}/obj/util/libutil.a")
cmake_path(SET CRASHPAD_INCLUDE_PATH "${DIVE_CRASHPAD_PREBUILT_INCLUDE_PATH}")
cmake_path(SET CRASHPAD_MINICHROMIUM_INCLUDE_PATH "${DIVE_CRASHPAD_PREBUILT_INCLUDE_PATH}/third_party/mini_chromium/mini_chromium")

add_library(crashpad_client INTERFACE)
target_link_libraries(
    crashpad_client
    INTERFACE
        "${CRASHPAD_LIBBASE_PATH}"
        "${CRASHPAD_LIBCOMMON_PATH}"
        "${CRASHPAD_LIBCLIENT_PATH}"
        "${CRASHPAD_LIBUTIL_PATH}"
)
target_include_directories(
    crashpad_client
    INTERFACE
        "${CRASHPAD_INCLUDE_PATH}"
        "${CRASHPAD_MINICHROMIUM_INCLUDE_PATH}"
)

add_executable(crashpad_handler IMPORTED)
set_target_properties(crashpad_handler PROPERTIES
    IMPORTED_LOCATION "${CRASHPAD_HANDLER_PATH}"
)
list(POP_BACK CMAKE_MESSAGE_INDENT)
