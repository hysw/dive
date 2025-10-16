/*
Copyright 2023 Google Inc.

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

#include "dive_host_platform.h"

#include <filesystem>
#include <optional>
#include <string>

namespace Dive
{

std::optional<std::filesystem::path> ResolveAssetDirectory(const std::string& name)
{
    std::filesystem::path base_search_path = ".";
    if (auto ret = GetExecutableDirectory(); ret.ok())
    {
        base_search_path = *ret;
    }

    auto search_paths = std::array{
        std::filesystem::path{ base_search_path / "install" },
        std::filesystem::path{ base_search_path / "../../build_android/Release/bin" },
        std::filesystem::path{ base_search_path / "../../install" },
        std::filesystem::path{ base_search_path },
        std::filesystem::path{ "." },
    };

    for (const auto& p : search_paths)
    {
        auto result_path = p / name;
        if (std::filesystem::exists(result_path))
        {
            return std::filesystem::canonical(result_path);
        }
    }
    return std::nullopt;
}

}  // namespace Dive
