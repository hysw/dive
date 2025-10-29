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

#include <filesystem>

#include <QMetaType>

class TraceStates
{
public:
    enum class TraceType
    {
        kUnknown,  // Load failure
        kDive,     // Rd + Gfxr
        kRdOnly,
        kGfxrOnly,
    };

    using TokenType = uint64_t;
    template<typename T> struct WithToken
    {
        T         value = {};
        TokenType token = {};
    };
    using File = WithToken<std::filesystem::path>;

    struct Current
    {
        TraceType trace_type = TraceType::kUnknown;

        File gfxr;
        File gfxa;
        File perf_counter_csv;
        File gpu_timing_csv;
        File pm4_rd;
        File screenshot_png;
        bool trace_stats_loaded;
    };

    struct Expected
    {
        File gfxr;
        File gfxa;
        File perf_counter_csv;
        File gpu_timing_csv;
        File pm4_rd;
        File screenshot_png;
    };

    static TokenType NextToken();
};

Q_DECLARE_METATYPE(TraceStates::Expected)

