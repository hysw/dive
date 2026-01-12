
/*
 Copyright 2019 Google LLC

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

#include "command_hierarchy.h"
#include "dive_capture_data.h"
#include "gfxr_capture_data.h"
#include "pm4_capture_data.h"
#include "progress_tracker.h"

namespace Dive
{

struct CaptureMetadata;
class EventStateInfo;

class CaptureMetadataRef
{
 public:
    explicit CaptureMetadataRef(const CaptureMetadata& ref) : m_ref(&ref) {}

    size_t GetShaderCount() const;
    size_t GetBufferCount() const;
    size_t GetEventCount() const;

    const Disassembly& GetShader(size_t index) const;
    const BufferInfo& GetBuffer(size_t index) const;
    const EventInfo& GetEvent(size_t index) const;

    const EventStateInfo& GetEventState() const;

 private:
    const CaptureMetadata* m_ref;
};

//--------------------------------------------------------------------------------------------------
// Main container for the capture data as well as associated metadata
class DataCore
{
 public:
    DataCore() = default;
    virtual ~DataCore() = default;

    static std::unique_ptr<DataCore> Create(ProgressTracker* tracker = nullptr);

    // Load the capture file
    virtual CaptureData::LoadResult LoadDiveCaptureData(const std::string& file_name) = 0;
    virtual CaptureData::LoadResult LoadPm4CaptureData(const std::string& file_name) = 0;
    virtual CaptureData::LoadResult LoadGfxrCaptureData(const std::string& file_name) = 0;

    // Parse the capture to generate info that describes the capture
    virtual bool ParseDiveCaptureData() = 0;
    virtual bool ParsePm4CaptureData() = 0;
    virtual bool ParseGfxrCaptureData() = 0;

    // Create meta data from the captured data
    virtual bool CreateDiveMetaData() = 0;
    virtual bool CreatePm4MetaData() = 0;

    // Get the pm4 capture data (includes access to raw command buffers and memory blocks)
    virtual const Pm4CaptureData& GetPm4CaptureData() const = 0;
    virtual Pm4CaptureData& GetMutablePm4CaptureData() = 0;

    // Get the gfxr capture data
    virtual const GfxrCaptureData& GetGfxrCaptureData() const = 0;
    virtual GfxrCaptureData& GetMutableGfxrCaptureData() = 0;

    // Get the command-hierarchy, which is a tree view interpretation of the command buffer
    virtual const CommandHierarchy& GetCommandHierarchy() const = 0;

    // Get metadata describing the capture (info obtained by parsing the capture)
    virtual CaptureMetadataRef GetCaptureMetadata() const = 0;
};

}  // namespace Dive
