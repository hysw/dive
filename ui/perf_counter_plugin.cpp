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
#include "perf_counter_plugin.h"

#include "capture_service/constants.h"
#include "dive_core/available_metrics.h"
#include "dive_core/perf_metrics_data.h"
#include <memory>

namespace
{

static constexpr const char *kMetricsDescriptionFileName = "available_metrics.csv";

}  // namespace

PerfCounterPlugin::~PerfCounterPlugin()
{
    // For ~std::unique_ptr<T> where T is forward declared.
}

std::unique_ptr<PerfCounterPlugin> PerfCounterPlugin::Load()
{
    auto instance = std::unique_ptr<PerfCounterPlugin>(new PerfCounterPlugin);
    if (!instance->Initialize())
    {
        return {};
    }
    return instance;
}

bool PerfCounterPlugin::Initialize()
{
    std::optional<std::filesystem::path> metrics_description_file_path = std::nullopt;
    if (auto profile_plugin_folder = Dive::ResolveAssetDirectory(Dive::kProfilingPluginFolderName))
    {
        auto file_path = *profile_plugin_folder / kMetricsDescriptionFileName;
        if (std::filesystem::exists(file_path))
        {
            metrics_description_file_path = file_path;
        }
    }

    if (!metrics_description_file_path)
    {
        return false;
    }

    m_available_metrics = Dive::AvailableMetrics::LoadFromCsv(*metrics_description_file_path);
    return true;
}
