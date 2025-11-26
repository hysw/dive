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

#include <QDialog>
#include <future>
#include <optional>
#include <string_view>

#include "capture_service/device_mgr.h"
#include "dive/ui/forward.h"
#include "dive/utils/component_files.h"
#include "dive_core/available_metrics.h"
#include "ui/impl_pointer.h"

class AnalyzeDialog : public QDialog
{
    Q_OBJECT

    // Data structure to hold a single item from the CSV
    struct CsvItem
    {
        QString          id;
        Dive::MetricType type;
        QString          key;
        QString          name;
        QString          description;
    };

    // Describes all files associated with a GFXR file, does not guarantee existence
    struct ReplayArtifactsPaths
    {
        std::filesystem::path gfxr;
        // TODO: std::filesystem::path gfxa;
        std::filesystem::path perf_counter_csv;
        std::filesystem::path gpu_timing_csv;
        std::filesystem::path pm4_rd;
        std::filesystem::path renderdoc_rdc;
    };

    enum class ReplayStatusUpdateCode : int
    {
        // This signals the async process is finished:
        kDone,

        // End of task status:
        kSuccess,
        kFailure,
        kSetupDeviceFailure,  // Special case where we disable replay button.

        // Individual step of replay:
        kSetup,
        kStartNormalReplay,
        kStartPm4Replay,
        kStartGpuTimeReplay,
        kStartPerfCounterReplay,

        // Deleting replay artifacts associated with currently opened GFXR file
        kDeletingReplayArtifacts,
    };

public:
    struct Impl;
    AnalyzeDialog(ApplicationController&        controller,
                  const Dive::AvailableMetrics* available_metrics,
                  QWidget*                      parent = nullptr);
    ~AnalyzeDialog();
    void UpdateDeviceList();
private slots:
    void OnReplayStatusUpdate(int status_code, const QString& error_message);
    void OnDeviceInitialized();
    void OnDeviceListRefresh();
    void OnReplay();
    void OnOverlayMessage(const QString& message);
    void OnDisableOverlay();
    void OnDeleteReplayArtifacts();
    void OnAsyncFinished();

public slots:
    void OnAnalyzeCaptureStarted(const QString& file_path);

signals:
    void ReplayStatusUpdated(int status_code, const QString& error_message);
    void DisplayPerfCounterResults(const QString& file_path);
    void DisplayGpuTimingResults(const QString& file_path);
    void CaptureUpdated(const QString& file_path);
    void OverlayMessage(const QString& message);
    void DisableOverlay();

    // Private signals:
    void SelectedFileUpdated(const QString&);
    void ReplayEnabled(bool enabled);
    void DeviceInitialized();
    void AsyncFinished();

private:
    struct ReplayConfig;

    void ShowMessage(const std::string& message);
    void SetReplayButton(const std::string& message, bool is_enabled);
    void PopulateMetrics();
    void UpdateSelectedMetricsList();
    void UpdatePerfCounterElements(bool show);

    absl::StatusOr<std::string> PushFilesToDevice(Dive::AndroidDevice* device,
                                                  const std::string&   local_asset_file_path);

    absl::Status NormalReplay(Dive::DeviceManager& device_manager,
                              const std::string&   remote_gfxr_file,
                              int                  frame_count);
    absl::Status Pm4Replay(Dive::DeviceManager& device_manager,
                           const std::string&   remote_gfxr_file);
    absl::Status PerfCounterReplay(Dive::DeviceManager&            device_manager,
                                   const std::string&              remote_gfxr_file,
                                   const std::vector<std::string>& selected_metrics);
    absl::Status GpuTimeReplay(Dive::DeviceManager& device_manager,
                               const std::string&   remote_gfxr_file,
                               int                  frame_count);
    absl::Status RenderDocReplay(Dive::DeviceManager& device_manager,
                                 const std::string&   remote_gfxr_file);

    void UpdateReplayStatus(ReplayStatusUpdateCode status, const std::string& messge = "");
    void ExecuteStatusUpdate();

    void ReplayImpl(const ReplayConfig&);
    void DeleteReplayArtifactsImpl();
    void OnAnalyzeCaptureEnded();

    ImplPointer<Impl> m_impl;

    QStandardItemModel* m_device_model = nullptr;
    QComboBox*          m_device_box = nullptr;

    // Representing a session with a specific GFXR capture file opened
    //
    // Set only in OnOpenFile(), this is the filename of the GFXR capture that will be replayed
    QString m_selected_capture_file_string = "";
    // The dir that contains m_selected_capture_file_string
    std::filesystem::path m_local_capture_file_directory = "";

    OverlayHelper*    m_overlay;

    struct StatusUpdateQueueItem
    {
        ReplayStatusUpdateCode status;
        QString                message;
    };
    std::vector<StatusUpdateQueueItem> m_status_update_queue;
};
