message(CHECK_START "Generate build files for third_party/dive_crashpad")
list(APPEND CMAKE_MESSAGE_INDENT "  ")

add_custom_command(
    OUTPUT crashpad_handler libbase.a libcommon.a libclient.a libutil.a
    DEPENDS crashpad_commit.txt build_crashpad_linux.sh
    COMMAND "${CMAKE_CURRENT_LIST_DIR}/build_crashpad_linux.sh" "${CMAKE_CURRENT_LIST_DIR}/crashpad_commit.txt"
)

add_custom_target(
    libdive_crashpad_static
    DEPENDS crashpad_handler libbase.a libcommon.a libclient.a libutil.a
)

add_library(
    dive_crashpad_static
    STATIC
    dive_crashpad/client.cpp
    dive_crashpad/client.h
)

add_dependencies(dive_crashpad_static libdive_crashpad_static)
target_link_libraries(
    dive_crashpad_static
    PUBLIC
        absl::base
        absl::status
        absl::strings
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/libclient.a"
        "${CMAKE_CURRENT_BINARY_DIR}/libcommon.a"
        "${CMAKE_CURRENT_BINARY_DIR}/libutil.a"
        "${CMAKE_CURRENT_BINARY_DIR}/libbase.a"
)

target_include_directories(
    dive_crashpad_static
    PUBLIC "${CMAKE_CURRENT_LIST_DIR}"
    PRIVATE

  "${CMAKE_CURRENT_BINARY_DIR}/crashpad"
  "${CMAKE_CURRENT_BINARY_DIR}/crashpad/third_party/mini_chromium/mini_chromium"
)

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "done")
