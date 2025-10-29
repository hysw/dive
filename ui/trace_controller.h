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
#include <QObject>

#include "trace_states.h"

class QThread;

namespace Dive
{
struct CaptureStats;
}

class TraceControllerData;

class TraceControllerWorker : public QObject
{
public:
    class Impl;

private:
    std::shared_ptr<TraceControllerData> m_data;
};

class TraceController : public QObject
{
    struct PrivateTag
    {
        explicit PrivateTag() = default;
    };

public:
    struct AsyncData;

    std::unique_ptr<TraceController> Create();

    explicit TraceController(PrivateTag);
    ~TraceController();

private:
    std::shared_ptr<TraceControllerData> m_data;
    std::mutex m_mutex;

    bool m_loading_in_progress = false;

    TraceStates::Current  m_current_state;
    TraceStates::Expected m_expected_state;

    std::unique_ptr<AsyncData> m_async_data;
};
