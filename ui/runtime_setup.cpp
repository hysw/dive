/*
 Copyright 2025 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#include "runtime_setup.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <iostream>

#include "absl/debugging/failure_signal_handler.h"
#include "absl/debugging/symbolize.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/flags/usage_config.h"
#include "dive/utils/version_info.h"

#ifdef __linux__
#    include <dlfcn.h>
#endif

#if defined(_WIN32)
#    include <Windows.h>
#    include <io.h>
#else
#    include <unistd.h>
#endif

namespace
{

class CrashHandler
{
public:
    static void Initialize(const char *argv0)
    {
        QString filename = "dive-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") +
                           ".log.txt";

        // Try to open in the executable directory
        // This might fail if the executable folder is not writable
        QString exe_dir = QFileInfo(argv0).absolutePath();
        QString exe_full_path = QDir(exe_dir).filePath(filename);

        // If we couldn't write next to the exe (permission denied), use temp folder.
        // Windows: %TEMP%
        // Linux: /tmp
        QString temp_dir = QDir::tempPath();
        QString temp_full_path = QDir(temp_dir).filePath(filename);

        SafeStrCopy(m_primary_path, exe_full_path.toLocal8Bit().constData());
        SafeStrCopy(m_fallback_path, temp_full_path.toLocal8Bit().constData());

        std::cout << "Crash handler initialized" << std::endl;
        std::cout << "  1. Primary Log Path:  " << exe_full_path.toStdString() << std::endl;
        std::cout << "  2. Fallback Log Path: " << temp_full_path.toStdString() << std::endl;
    }

    static void Writer(const char *data)
    {
        if (data == nullptr)
        {
            return;
        }

        // Avoid strlen for crash handler to be safe
        uint32_t len = 0;
        while (data[len] != '\0')
        {
            len++;
        }

        if (m_fd == kInvalidFd)
        {
            m_fd = SysOpen(m_primary_path);

            if (m_fd == kInvalidFd)
            {
                m_fd = SysOpen(m_fallback_path);
            }
        }

        if (m_fd != kInvalidFd)
        {
            SysWrite(m_fd, data, len);
        }
    }

private:
    static constexpr int kInvalidFd = -1;
    static constexpr int kMaxPath = 2048;

    inline static int m_fd = kInvalidFd;

    // Use char array to avoid potential allocation within the crash handler
    inline static char m_primary_path[kMaxPath] = { 0 };
    inline static char m_fallback_path[kMaxPath] = { 0 };

    template<size_t N> static void SafeStrCopy(char (&dest)[N], const char *src)
    {
        if (!src)
        {
            return;
        }

        size_t i = 0;
        for (; i < N - 1 && src[i] != '\0'; ++i)
        {
            dest[i] = src[i];
        }
        dest[i] = '\0';
    }

    static int SysOpen(const char *path)
    {
#if defined(_WIN32)
        constexpr int flags = _O_CREAT | _O_TRUNC | _O_WRONLY | _O_TEXT;
        constexpr int mode = _S_IREAD | _S_IWRITE;
        return _open(path, flags, mode);
#else
        constexpr int flags = O_CREAT | O_TRUNC | O_WRONLY;
        // 0: Indicates this is an octal number
        // 6: (Owner):  Read (4) + Write (2) = Read/Write
        // 6: (Group):  Read (4) + Write (2) = Read/Write
        // 4: (Others): Read (4) = Read Only
        constexpr int mode = 0664;
        return open(path, flags, mode);
#endif
    }

    static void SysWrite(int fd, const char *data, uint32_t len)
    {
#if defined(_WIN32)
        _write(fd, data, len);
#else
        [[maybe_unused]] ssize_t res = write(fd, data, len);
#endif
    }
};

}  // namespace

struct DiveApplicationRuntimeGuard::Impl
{
    int    argc = 0;
    char **argv = nullptr;
};
DiveApplicationRuntimeGuard::DiveApplicationRuntimeGuard(int argc, char *argv[])
{
    m_impl->argc = argc;
    m_impl->argv = argv;
}

void DiveApplicationRuntimeGuard::InstallCrashHandler()
{
    absl::InitializeSymbolizer(m_impl->argv[0]);
    CrashHandler::Initialize(m_impl->argv[0]);

    absl::FailureSignalHandlerOptions options;
    options.writerfn = CrashHandler::Writer;
    absl::InstallFailureSignalHandler(options);
}

void DiveApplicationRuntimeGuard::ParseFlags()
{
    absl::FlagsUsageConfig flags_usage_config;
    flags_usage_config.version_string = Dive::GetCompleteVersionString;
    absl::SetFlagsUsageConfig(flags_usage_config);
    absl::SetProgramUsageMessage("Dive GUI");
    absl::ParseCommandLine(m_impl->argc, m_impl->argv);
}

DiveApplicationRuntimeGuard::~DiveApplicationRuntimeGuard() {}
