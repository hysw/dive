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

set(DIVE_LIB_OS_SOURCES terminal.cpp terminal.h)

add_library(dive_lib_os ${DIVE_LIB_OS_SOURCES})
target_link_libraries(dive_lib_os PUBLIC dive_src_includes)

# === command_utils ==========================================================

if(WIN32)
    set(DIVE_COMMAND_UTILS_SRC command_utils.h command_utils_win32.cc)
    add_definitions(-DUNICODE -D_UNICODE)
else()
    set(DIVE_COMMAND_UTILS_SRC command_utils.h command_utils.cc)
endif()

add_library(command_utils ${DIVE_COMMAND_UTILS_SRC})
target_link_libraries(command_utils PRIVATE dive_src_includes absl::status)
