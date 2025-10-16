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

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(DiveHostPlatform)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(WIN32)
  set(DIVE_HOST_PLATFORM_SOURCE "dive_host_platform_win32.cpp")
  add_definitions(-DUNICODE -D_UNICODE)
else()
  set(DIVE_HOST_PLATFORM_SOURCE "dive_host_platform_linux.cpp")
endif()

add_library(dive_host_platform dive_host_platform.cpp ${DIVE_HOST_PLATFORM_SOURCE})

target_link_libraries(dive_host_platform PUBLIC absl::core_headers)
target_include_directories(dive_host_platform
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
