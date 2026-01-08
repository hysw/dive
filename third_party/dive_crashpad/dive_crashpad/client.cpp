/*
 Copyright 2026 Google LLC

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

#include "dive_crashpad/client.h"

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "absl/base/no_destructor.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "base/files/file_path.h"
#include "client/crash_report_database.h"
#include "client/crashpad_client.h"
#include "client/settings.h"

namespace Dive
{

namespace
{

constexpr char kDbDirectory[] = "crash_database";
constexpr char kMetricsDirectory[] = "crash_metrics";
constexpr char kCrashReportUrl[] = "https://clients2.google.com/cr/report";
constexpr char kFormat[] = "minidump";
constexpr char kNoRateLimitFlag[] = "--no-rate-limit";
constexpr int kMaxCrashpadVersionLength = 30;

}  // namespace

absl::Status InitializeDiveCrashpad(const DiveCrashpadOptions& opts)
{
    // Crashpad requires explicit user consent or a programmatic override to upload
    // reports. We enable uploads here to ensure the client can transmit crash
    // data to the remote collection server.
    std::unique_ptr<crashpad::CrashReportDatabase> database =
        crashpad::CrashReportDatabase::Initialize(base::FilePath(opts.database_path.native()));
    if (database && database->GetSettings())
    {
        database->GetSettings()->SetUploadsEnabled(true);
    }
    else
    {
        return absl::InternalError("Failed to initialize Crashpad database.");
    }

    const std::string& version = opts.dive_version;
    if (version.size() > kMaxCrashpadVersionLength)
    {
        return absl::InternalError(absl::StrCat("Crashpad version string '", version, "' (",
                                                version.size(), ") is too long, max is ",
                                                kMaxCrashpadVersionLength));
    }

    std::map<std::string, std::string> annotations = {
        {"product", opts.product_name},
        {"format", opts.crash_format},
        {"version", version},
    };

    std::vector<std::string> arguments = {kNoRateLimitFlag};

    static absl::NoDestructor<crashpad::CrashpadClient> client;

    bool success = client->StartHandler(
        base::FilePath(opts.handler_path.native()), base::FilePath(opts.database_path.native()),
        base::FilePath(opts.metrics_path.native()), kCrashReportUrl, annotations, arguments,
        /*restartable=*/true,
        /*asynchronous_start=*/false);

    if (!success)
    {
        return absl::InternalError("Failed to start Crashpad handler.");
    }
    return absl::OkStatus();
}

}  // namespace Dive
