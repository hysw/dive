/*
Copyright 2025 Google Inc.

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
#pragma once

#include "dive_version.h"

namespace Dive
{
struct DiveVersion
{
    // Note: DiveVersion is a tag type.

    static constexpr char kRepoSha1[] = DIVE_VERSION_SHA1;

    static constexpr const char* GetSha1()
    {
        if constexpr (sizeof(kRepoSha1) > 0 && kRepoSha1[0] != 0)
        {
            return kRepoSha1;
        }
        else
        {
            return nullptr;
        }
    }
    template<typename StreamT> static void FormatOutput(StreamT& stream)
    {
        stream << DIVE_VERSION_MAJOR << "." << DIVE_VERSION_MINOR << "." << DIVE_VERSION_REVISION
               << " (";
        const auto sha1_len = sizeof(kRepoSha1);
        for (auto i = sha1_len - sha1_len; i < 8 && i + 1 < sha1_len; ++i)
        {
            stream << kRepoSha1[i];
        }
        if (DIVE_REPO_DIRTY)
        {
            stream << "-local";
        }
        stream << ")";
    }
};

template<typename StreamT> inline StreamT& operator<<(StreamT& stream, const DiveVersion&)
{
    DiveVersion::FormatOutput(stream);
    return stream;
}

}  // namespace Dive
