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

#pragma once

#include <memory>
#include <filesystem>

#include "dive_host_platform.h"

namespace Dive
{
class AvailableMetrics;
}  // namespace Dive

class PerfCounterPlugin
{
public:
    static std::unique_ptr<PerfCounterPlugin> Load();

    ~PerfCounterPlugin();
    PerfCounterPlugin(const PerfCounterPlugin&) = delete;
    PerfCounterPlugin(PerfCounterPlugin&&) = delete;
    PerfCounterPlugin& operator=(const PerfCounterPlugin&) = delete;
    PerfCounterPlugin& operator=(PerfCounterPlugin&&) = delete;

    const Dive::AvailableMetrics& GetAvailableMetrics() const { return *m_available_metrics; }

private:
    PerfCounterPlugin() = default;
    bool Initialize();

    std::unique_ptr<Dive::AvailableMetrics> m_available_metrics;
};
