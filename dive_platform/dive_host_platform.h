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

#include <filesystem>
#include <optional>
#include <string>
#include "absl/status/statusor.h"

namespace Dive
{
// Logs the command and the result of a command line application.
// Returns the output of the command if it finished successfully, or error status otherwise
absl::StatusOr<std::string> LogCommand(const std::string &command,
                                       const std::string &output,
                                       int                ret);

// Runs a command line application.
// Returns the output of the command if it finished successfully, or error status otherwise
absl::StatusOr<std::string> RunCommand(const std::string &command);

// Returns the directory of the currently running executable.
absl::StatusOr<std::filesystem::path> GetExecutableDirectory();

std::optional<std::filesystem::path> ResolveAssetDirectory(const std::string& name);

}  // namespace Dive
