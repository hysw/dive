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

#include "analyze_window.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QVBoxLayout>
#include <filesystem>
#include <future>
#include <optional>
#include <string>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "capture_service/constants.h"
#include "capture_service/device_mgr.h"
#include "common/macros.h"
#include "ui/application_controller.h"
#include "ui/overlay.h"
#include "ui/settings.h"

namespace
{
constexpr int kDefaultFrameCount = 3;
// Metric items:
constexpr int kMetricKeyRole = Qt::UserRole + 1;
constexpr int kMetricDescriptionRole = Qt::UserRole + 2;
// Device items:
constexpr int kDeviceSerialRole = Qt::UserRole + 1;
}  // namespace

//--------------------------------------------------------------------------------------------------
void AttemptDeletingTemporaryLocalFile(const std::filesystem::path& file_path)
{
    if (std::filesystem::remove(file_path))
    {
        qDebug() << "Successfully deleted: " << file_path.string().c_str();
    }
    else
    {
        qDebug() << "Was not present: " << file_path.string().c_str();
    }
}

struct AnalyzeDialog::ReplayConfig
{
    std::string device_serial = {};

    bool replay_dump_pm4 = false;
    bool replay_gpu_time = false;
    bool replay_renderdoc = false;
    bool replay_perf_counter = false;
    bool replay_custom = false;

    int replay_custom_frame_count;
    int replay_gpu_time_frame_count;

    std::vector<std::string> selected_metrics = {};
};

struct AnalyzeDialog::Impl
{

    struct ReplaySelection
    {
        QGroupBox* custom_replay = nullptr;
        QCheckBox* dump_pm4 = nullptr;
        QCheckBox* perf_counter = nullptr;
        QGroupBox* gpu_time_replay = nullptr;
        QCheckBox* renderdoc_capture = nullptr;
    };

    struct ReplayArgs
    {
        QSpinBox* m_gpu_time_replay_frame_count = nullptr;
        QSpinBox* m_custom_replay_frame_count = nullptr;
    };

    struct PerfCounterElements
    {
        const Dive::AvailableMetrics* m_available_metrics = nullptr;

        bool m_available = false;

        QLabel*      m_metrics_list_label = nullptr;
        QListWidget* m_metrics_list = nullptr;

        QLabel*    m_selected_metrics_description_label = nullptr;
        QTextEdit* m_selected_metrics_description = nullptr;

        QLabel*      m_enabled_metrics_list_label = nullptr;
        QListWidget* m_enabled_metrics_list = nullptr;

        void Initialize();
        void PopulateMetrics();
        void UpdateEnabledMetrics();
        void PopulateSelectedMetrics(std::vector<std::string>& metrics);
        void SetEnabled(bool enabled);
        void Hide();
    };

    struct AsyncExecuting
    {
        std::optional<std::string> pending_device_serial;
        std::optional<std::string> device_selection_error;
    };
    struct AsyncPending
    {
        std::optional<std::string> pending_device_serial;
    };

    AnalyzeDialog&         m_parent;
    ApplicationController& m_controller;

    OverlayHelper* m_overlay = nullptr;

    ReplayConfig        m_config;
    PerfCounterElements m_perf_counter_elems;

    std::future<void> m_async_active;
    AsyncExecuting    m_async_executing;
    AsyncPending      m_async_pending;

    // Other artifacts
    Dive::ComponentFilePaths m_local_capture_files = {};

    QLayout* CreateLayout();

    void InitializePerfCounterElements();

    QHBoxLayout* CreateReplayButtonLayout();
    QHBoxLayout* CreateDeviceSelectorLayout();
    QHBoxLayout* CreateSelectedFileLayout();
    QHBoxLayout* CreateReplayWarningLayout();
    QHBoxLayout* CreateDeleteReplayArtifactsLayout();

    QGroupBox* CreateCustomReplayBox();
    QCheckBox* CreatePm4ReplayBox();
    QCheckBox* CreatePerfCounterReplayBox();
    QGroupBox* CreateGpuTimeReplayBox();
    QCheckBox* CreateRenderDocReplayBox();

    void DispatchAsync();
    void ExecuteAsync();
    void ExecuteAsyncSelectDevice();
    void OnAsyncFinished();
};

using Impl = AnalyzeDialog::Impl;
using PerfCounterElements = AnalyzeDialog::Impl::PerfCounterElements;

void PerfCounterElements::UpdateEnabledMetrics()
{
    auto& metrics = *m_enabled_metrics_list;
    metrics.clear();
    for (auto item : m_metrics_list->selectedItems())
    {
        metrics.addItem(item->text());
    }
}

void PerfCounterElements::PopulateSelectedMetrics(std::vector<std::string>& metrics)
{
    metrics.clear();
    for (auto item : m_metrics_list->selectedItems())
    {
        metrics.push_back(item->data(kMetricKeyRole).toString().toStdString());
    }
}

void PerfCounterElements::Initialize()
{
    m_metrics_list_label = new QLabel(tr("Available Metrics:")), m_metrics_list = new QListWidget();

    m_selected_metrics_description_label = new QLabel(tr("Description:"));
    m_selected_metrics_description = new QTextEdit();
    m_selected_metrics_description->setReadOnly(true);
    m_selected_metrics_description->setPlaceholderText("Select a metric to see its description...");

    m_enabled_metrics_list_label = new QLabel(tr("Enabled Metrics:"));
    m_enabled_metrics_list = new QListWidget();
}

void Impl::InitializePerfCounterElements()
{
    m_perf_counter_elems.Initialize();
    m_perf_counter_elems.PopulateMetrics();

    QObject::connect(m_perf_counter_elems.m_metrics_list,
                     &QListWidget::itemChanged,
                     &m_parent,
                     [this]() {
                         m_perf_counter_elems.UpdateEnabledMetrics();
                         m_perf_counter_elems.PopulateSelectedMetrics(m_config.selected_metrics);
                     });

    QObject::connect(m_perf_counter_elems.m_metrics_list,
                     &QListWidget::currentItemChanged,
                     &m_parent,
                     [this](QListWidgetItem* current) {
                         if (!current)
                         {
                             return;
                         }
                         auto text = current->data(kMetricDescriptionRole).toString();
                         m_perf_counter_elems.m_selected_metrics_description->setText(text);
                     });
    if (!m_perf_counter_elems.m_available)
    {
        m_perf_counter_elems.Hide();
    }
}

QHBoxLayout* Impl::CreateReplayButtonLayout()
{
    auto* layout = new QHBoxLayout();
    auto* button = new QPushButton("&Replay", &m_parent);
    button->setEnabled(false);
    QObject::connect(&m_parent, &AnalyzeDialog::ReplayEnabled, button, &QPushButton::setEnabled);
    QObject::connect(button, &QPushButton::clicked, &m_parent, &AnalyzeDialog::OnReplay);
    layout->addWidget(button);
    return layout;
}

QHBoxLayout* Impl::CreateDeviceSelectorLayout()
{
    auto* layout = new QHBoxLayout();

    auto* model = new QStandardItemModel();
    auto* combo_box = new QComboBox(&m_parent);
    auto* device_refresh_button = new QPushButton("&Refresh", &m_parent);

    combo_box->setCurrentText("Please select a device");
    combo_box->setModel(model);
    combo_box->setCurrentIndex(0);
    layout->addWidget(new QLabel(tr("Devices:"), &m_parent));
    layout->addWidget(combo_box);
    layout->addWidget(device_refresh_button);

    QObject::connect(device_refresh_button,
                     &QPushButton::clicked,
                     &m_parent,
                     &AnalyzeDialog::OnDeviceListRefresh);
    QObject::connect(combo_box,
                     qOverload<int>(&QComboBox::currentIndexChanged),
                     &m_parent,
                     [this](int index) {
                         auto item = m_parent.m_device_model->item(index);
                         if (!item)
                         {
                             return;
                         }
                         auto serial = item->data(kDeviceSerialRole).toString();
                         m_async_pending.pending_device_serial = serial.toStdString();
                         DispatchAsync();
                     });

    m_parent.m_device_box = combo_box;
    m_parent.m_device_model = model;

    m_parent.UpdateDeviceList(false);
    return layout;
}

QHBoxLayout* Impl::CreateSelectedFileLayout()
{
    auto* layout = new QHBoxLayout();
    auto* selected_file_input_box = new QLineEdit();
    selected_file_input_box->setReadOnly(true);
    layout->addWidget(new QLabel("Selected Capture file:"));
    layout->addWidget(selected_file_input_box);
    QObject::connect(&m_parent,
                     &AnalyzeDialog::SelectedFileUpdated,
                     selected_file_input_box,
                     [selected_file_input_box](const QString& name) {
                         selected_file_input_box->setText(name);
                     });
    return layout;
}

QGroupBox* Impl::CreateCustomReplayBox()
{
    auto frame_count_layout = new QHBoxLayout();
    auto frame_count_label = new QLabel(tr("Loop Single Frame Count:"));
    auto frame_count_box = new QSpinBox(&m_parent);
    frame_count_box->setRange(1, std::numeric_limits<int>::max());
    frame_count_box->setValue(kDefaultFrameCount);
    frame_count_layout->addWidget(frame_count_label);
    frame_count_layout->addWidget(frame_count_box);

    auto group_box = new QGroupBox();
    group_box->setTitle("Custom Replay");
    group_box->setCheckable(true);
    group_box->setChecked(false);
    group_box->setLayout(frame_count_layout);

    group_box->setVisible(m_controller.AdvancedOptionEnabled());
    QObject::connect(&m_controller,
                     &ApplicationController::AdvancedOptionToggled,
                     group_box,
                     &QGroupBox::setVisible);
    QObject::connect(group_box, &QGroupBox::clicked, &m_parent, [this](bool checked) {
        m_config.replay_custom = checked;
    });
    QObject::connect(frame_count_box, &QSpinBox::valueChanged, &m_parent, [this](int value) {
        m_config.replay_custom_frame_count = value;
    });
    return group_box;
}

QCheckBox* Impl::CreatePm4ReplayBox()
{
    auto* box = new QCheckBox();
    box->setText(tr("Enable Dump Pm4"));
    box->setCheckState(Qt::Unchecked);
    QObject::connect(box, &QCheckBox::clicked, &m_parent, [this](bool checked) {
        m_config.replay_dump_pm4 = checked;
    });
    return box;
}

QCheckBox* Impl::CreatePerfCounterReplayBox()
{
    auto* box = new QCheckBox();
    box->setText(tr("Enable Perf Counters"));
    box->setCheckState(Qt::Unchecked);
    QObject::connect(box, &QCheckBox::toggled, &m_parent, [this](bool checked) {
        m_perf_counter_elems.SetEnabled(checked);
    });
    QObject::connect(box, &QCheckBox::clicked, &m_parent, [this](bool checked) {
        m_config.replay_perf_counter = checked;
    });
    if (m_perf_counter_elems.m_available)
    {
        box->setVisible(false);
    }
    return box;
}

QGroupBox* Impl::CreateGpuTimeReplayBox()
{
    auto layout = new QHBoxLayout();
    auto frame_count_label = new QLabel(tr("Loop Single Frame Count:"));
    auto frame_count_box = new QSpinBox(&m_parent);
    frame_count_box->setRange(1, std::numeric_limits<int>::max());
    frame_count_box->setValue(kDefaultFrameCount);
    layout->addWidget(frame_count_label);
    layout->addWidget(frame_count_box);

    auto group_box = new QGroupBox();
    group_box->setTitle("Enable GPU Time");
    group_box->setCheckable(true);
    group_box->setChecked(false);
    group_box->setLayout(layout);

    QObject::connect(group_box, &QGroupBox::clicked, &m_parent, [this](bool checked) {
        m_config.replay_gpu_time = checked;
    });
    QObject::connect(frame_count_box, &QSpinBox::valueChanged, &m_parent, [this](int value) {
        m_config.replay_gpu_time_frame_count = value;
    });
    return group_box;
}

QCheckBox* Impl::CreateRenderDocReplayBox()
{
    auto* box = new QCheckBox();
    box->setText(tr("Enable RenderDoc capture"));
    box->setCheckState(Qt::Unchecked);
    QObject::connect(box, &QCheckBox::clicked, &m_parent, [this](bool checked) {
        m_config.replay_renderdoc = checked;
    });
    return box;
}

QHBoxLayout* Impl::CreateReplayWarningLayout()
{
    auto* layout = new QHBoxLayout();
    auto* label = new QLabel(
    tr("⚠ Initiating replay will use and potentially overwrite temporary artifacts from previous "
       "replays. Save any desired artifacts manually in a separate folder before proceeding."));
    label->setWordWrap(true);
    layout->addWidget(label);
    return layout;
}

QHBoxLayout* Impl::CreateDeleteReplayArtifactsLayout()
{
    auto* layout = new QHBoxLayout();
    auto* button = new QPushButton("&Delete Previous Replay Artifacts", &m_parent);
    layout->addWidget(button);
    QObject::connect(button,
                     &QPushButton::clicked,
                     &m_parent,
                     &AnalyzeDialog::OnDeleteReplayArtifacts);
    return layout;
}

QLayout* Impl::CreateLayout()
{
    if (m_overlay)
    {
        return m_overlay->GetLayout();
    }

    m_overlay = new OverlayHelper(&m_parent);

    InitializePerfCounterElements();

    auto* replay_button_layout = CreateReplayButtonLayout();
    auto* device_layout = CreateDeviceSelectorLayout();
    auto* selected_file_layout = CreateSelectedFileLayout();
    auto* custom_replay_box = CreateCustomReplayBox();
    auto* dump_pm4_box = CreatePm4ReplayBox();
    auto* perf_counter_box = CreatePerfCounterReplayBox();
    auto* gpu_time_replay_box = CreateGpuTimeReplayBox();
    auto* renderdoc_capture_box = CreateRenderDocReplayBox();
    auto* replay_warning_layout = CreateReplayWarningLayout();
    auto* delete_replay_artifacts_layout = CreateDeleteReplayArtifactsLayout();

    // Left Panel Layout
    auto* left_panel_layout = new QVBoxLayout();
    left_panel_layout->addWidget(m_perf_counter_elems.m_metrics_list_label);
    left_panel_layout->addWidget(m_perf_counter_elems.m_metrics_list);

    // Right Panel Layout
    auto* right_panel_layout = new QVBoxLayout();
    {
        right_panel_layout->addWidget(m_perf_counter_elems.m_selected_metrics_description_label);
        right_panel_layout->addWidget(m_perf_counter_elems.m_selected_metrics_description);
        right_panel_layout->addWidget(m_perf_counter_elems.m_enabled_metrics_list_label);
        right_panel_layout->addWidget(m_perf_counter_elems.m_enabled_metrics_list);
        right_panel_layout->addLayout(device_layout);
        right_panel_layout->addLayout(selected_file_layout);
        right_panel_layout->addWidget(custom_replay_box);
        right_panel_layout->addWidget(dump_pm4_box);
        right_panel_layout->addWidget(perf_counter_box);
        right_panel_layout->addWidget(gpu_time_replay_box);
        right_panel_layout->addWidget(renderdoc_capture_box);
        right_panel_layout->addLayout(replay_warning_layout);
        right_panel_layout->addLayout(delete_replay_artifacts_layout);
        right_panel_layout->addLayout(replay_button_layout);
    }

    // Main Layout
    auto* main_layout = new QHBoxLayout();
    main_layout->addLayout(left_panel_layout);
    main_layout->addLayout(right_panel_layout);

    m_overlay->Initialize(main_layout, &m_parent);
    return m_overlay->GetLayout();
}

// =================================================================================================
// AnalyzeDialog
// =================================================================================================
AnalyzeDialog::AnalyzeDialog(ApplicationController&        controller,
                             const Dive::AvailableMetrics* available_metrics,
                             QWidget*                      parent) :
    QDialog(parent),
    m_impl(Impl{ *this, controller })
{
    qDebug() << "AnalyzeDialog created.";

    m_impl->m_perf_counter_elems.m_available_metrics = available_metrics;
    m_impl->CreateLayout();

    QObject::connect(this, &AnalyzeDialog::DeviceSelected, this, &AnalyzeDialog::OnDeviceSelected);

    QObject::connect(this,
                     &AnalyzeDialog::ReplayStatusUpdated,
                     this,
                     &AnalyzeDialog::OnReplayStatusUpdate);

    QObject::connect(this, &AnalyzeDialog::DisableOverlay, this, &AnalyzeDialog::OnDisableOverlay);
    QObject::connect(this, &AnalyzeDialog::OverlayMessage, this, &AnalyzeDialog::OnOverlayMessage);
    QObject::connect(this, &AnalyzeDialog::AsyncFinished, this, &AnalyzeDialog::OnAsyncFinished);
}

//--------------------------------------------------------------------------------------------------
AnalyzeDialog::~AnalyzeDialog()
{
    qDebug() << "AnalyzeDialog destroyed.";
    Dive::GetDeviceManager().RemoveDevice();
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::OnOverlayMessage(const QString& message)
{
    m_overlay->SetMessage(message);
    m_overlay->SetMessageIsTimed();
}

void AnalyzeDialog::OnDisableOverlay()
{
    m_overlay->Clear();
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::ShowMessage(const std::string& message)
{
    auto message_box = new QMessageBox(this);
    message_box->setAttribute(Qt::WA_DeleteOnClose, true);
    message_box->setText(message.c_str());
    message_box->open();
    return;
}

//--------------------------------------------------------------------------------------------------
void PerfCounterElements::PopulateMetrics()
{
    if (!m_available_metrics)
    {
        return;
    }

    for (const auto& key : m_available_metrics->GetAllMetricKeys())
    {
        const Dive::MetricInfo* info = m_available_metrics->GetMetricInfo(key);
        if (!info)
        {
            continue;
        }
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(info->m_name));
        item->setData(kMetricKeyRole, QString::fromStdString(key));
        item->setData(kMetricDescriptionRole, QString::fromStdString(info->m_description));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        m_metrics_list->addItem(item);
        m_available = true;
    }

    // Add spacer so that all metrics are visible at the end of the list.
    QListWidgetItem* spacer = new QListWidgetItem();
    spacer->setFlags(spacer->flags() & ~Qt::ItemIsSelectable);
    m_metrics_list->addItem(spacer);
}

//--------------------------------------------------------------------------------------------------
void PerfCounterElements::Hide()
{
    m_metrics_list_label->setVisible(false);
    m_metrics_list->setVisible(false);
    m_enabled_metrics_list_label->setVisible(false);
    m_enabled_metrics_list->setVisible(false);
    m_selected_metrics_description_label->setVisible(false);
    m_selected_metrics_description->setVisible(false);
}

void PerfCounterElements::SetEnabled(bool enabled)
{
    m_metrics_list_label->setEnabled(enabled);
    m_metrics_list->setEnabled(enabled);
    m_enabled_metrics_list_label->setEnabled(enabled);
    m_enabled_metrics_list->setEnabled(enabled);
    m_selected_metrics_description_label->setEnabled(enabled);
    m_selected_metrics_description->setEnabled(enabled);
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::UpdateDeviceList()
{
    auto devices = Dive::GetDeviceManager().ListDevice();

    QString current_serial;
    if (auto item = m_device_model->item(m_device_box->currentIndex()); item)
    {
        current_serial = item->data(kDeviceSerialRole).toString();
    }

    m_device_model->clear();
    // Replay button should only be enabled when a device is selected.
    ReplayEnabled(false);

    if (devices.empty())
    {
        QStandardItem* item = new QStandardItem("No devices found");
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_device_model->appendRow(item);
        m_device_box->setCurrentIndex(0);
    }
    else
    {
        for (size_t i = 0; i < devices.size(); i++)
        {
            if (i == 0)
            {
                QStandardItem* item = new QStandardItem("Please select a device");
                item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
                m_device_model->appendRow(item);
                m_device_box->setCurrentIndex(0);
            }

            QStandardItem* item = new QStandardItem(devices[i].GetDisplayName().c_str());
            auto           serial = QString::fromStdString(devices[i].m_serial);
            item->setData(serial, kDeviceSerialRole);
            m_device_model->appendRow(item);

            // Keep the original selected devices as selected.
            if (serial == current_serial)
            {
                m_device_box->setCurrentIndex(static_cast<int>(i));
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void Impl::DispatchAsync()
{
    if (m_async_active.valid())
    {
        return;
    }
    m_async_executing = AsyncExecuting{
        .pending_device_serial = m_async_pending.pending_device_serial,
        .device_selection_error = std::nullopt,
    };
    std::async([this]() {
        ExecuteAsync();
        emit m_parent.AsyncFinished();
    });
}

void Impl::ExecuteAsync()
{
    auto& state = m_async_executing;
    if (state.pending_device_serial)
    {
        ExecuteAsyncSelectDevice();
    }
}

void Impl::ExecuteAsyncSelectDevice()
{
    auto& state = m_async_executing;
    auto serial = *state.pending_device_serial;
    if (serial.empty())
    {
        // Succeed by default.
        return;
    }

    auto dev_ret = Dive::GetDeviceManager().SelectDevice(serial);
    if (!dev_ret.ok())
    {
        state.device_selection_error = absl::StrCat("Failed to select device ",
                                                    serial.c_str(),
                                                    ", error: ",
                                                    dev_ret.status().message());
        qDebug() << state.device_selection_error->c_str();
        return;
    }
}

void Impl::OnAsyncFinished()
{
    m_async_active.get();
    auto& state = m_async_executing;
    if (state.pending_device_serial)
    {
        if (state.device_selection_error)
        {
            m_parent.m_device_box->setCurrentIndex(0);
            m_parent.ShowMessage(*state.device_selection_error);
        }
        else
        {
            m_config.device_serial = *state.pending_device_serial;
        }
    }
}

void AnalyzeDialog::OnAsyncFinished()
{
    m_impl->OnAsyncFinished();
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::OnDeviceListRefresh()
{
    UpdateDeviceList(true);
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::OnAnalyzeCaptureStarted(const QString& file_path)
{
    // Clear members for previous session
    OnAnalyzeCaptureEnded();

    // Validate capture
    if (file_path.isEmpty())
    {
        qDebug() << "OnAnalyzeCaptureStarted(): empty filename";
        return;
    }
    std::filesystem::path    local_gfxr_parse = file_path.toStdString();
    Dive::ComponentFilePaths component_paths = {};
    {
        absl::StatusOr<Dive::ComponentFilePaths>
        ret = Dive::GetComponentFilesHostPaths(local_gfxr_parse.parent_path(),
                                               local_gfxr_parse.stem().string());
        if (!ret.ok())
        {
            std::string err_msg = absl::
            StrFormat("OnAnalyzeCaptureStarted(): could not get component files: %s",
                      ret.status().message());
            qDebug() << err_msg.c_str();
            return;
        }
        component_paths = *ret;
    }
    if (!std::filesystem::exists(component_paths.gfxr))
    {
        qDebug() << "OnAnalyzeCaptureStarted(): gfxr trace does not exist: "
                 << component_paths.gfxr.string().c_str();
        return;
    }
    if (!std::filesystem::exists(component_paths.gfxa))
    {
        qDebug() << "OnAnalyzeCaptureStarted(): gfxa trace does not exist: "
                 << component_paths.gfxa.string().c_str();
        QString title = QString("Unable to open file: %1").arg(file_path);
        QString description = QString("Required .gfxa file: %1 not found!")
                              .arg(QString::fromStdString(component_paths.gfxa.string()));
        QMessageBox::critical(this, title, description);
        return;
    }

    // Set members for current replay session
    assert(local_gfxr_parse == component_paths.gfxr);
    m_selected_capture_file_string = QString::fromStdString(component_paths.gfxr.generic_string());
    m_local_capture_file_directory = local_gfxr_parse.parent_path();
    m_impl->m_local_capture_files = component_paths;

    // Update display and settings
    SelectedFileUpdated(m_selected_capture_file_string);
    QString last_file_path = QString::fromStdString(
    local_gfxr_parse.parent_path().generic_string());
    Settings::Get()->WriteLastFilePath(last_file_path);

    // Open the dialog for users to initiate analysis
    open();
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::OnAnalyzeCaptureEnded()
{
    // Clear members for current analyze session
    m_selected_capture_file_string = "";
    m_local_capture_file_directory = "";
    m_impl->m_local_capture_files = {};
}

//--------------------------------------------------------------------------------------------------
absl::StatusOr<std::string> AnalyzeDialog::PushFilesToDevice(
Dive::AndroidDevice* device,
const std::string&   local_asset_file_path)
{
    const std::string remote_dir = "/sdcard/gfxr_captures_for_replay";

    // Create the remote directory on the device.
    RETURN_IF_ERROR(device->Adb().Run(absl::StrFormat("shell mkdir -p %s", remote_dir)));

    // Push the .gfxr file.
    std::string           local_gfxr_path = m_selected_capture_file_string.toStdString();
    std::filesystem::path gfxr_path(local_gfxr_path);
    std::string           gfxr_filename = gfxr_path.filename().string();
    std::string           remote_gfxr_path = absl::StrFormat("%s/%s", remote_dir, gfxr_filename);
    RETURN_IF_ERROR(
    device->Adb().Run(absl::StrFormat(R"(push "%s" "%s")", local_gfxr_path, remote_gfxr_path)));

    // Push the .gfxa file.
    std::filesystem::path asset_file_path(local_asset_file_path);
    std::string           asset_file_name = asset_file_path.filename().string();
    RETURN_IF_ERROR(device->Adb().Run(
    absl::StrFormat(R"(push "%s" "%s/%s")", local_asset_file_path, remote_dir, asset_file_name)));

    return remote_gfxr_path;
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::SetReplayButton(const std::string& message, bool is_enabled)
{
    ReplayEnabled(is_enabled);
}

//--------------------------------------------------------------------------------------------------
absl::Status AnalyzeDialog::NormalReplay(Dive::DeviceManager& device_manager,
                                         const std::string&   remote_gfxr_file,
                                         int                  frame_count)
{
    UpdateReplayStatus(ReplayStatusUpdateCode::kStartNormalReplay);
    Dive::GfxrReplaySettings replay_settings;
    replay_settings.remote_capture_path = remote_gfxr_file;
    replay_settings.local_download_dir = m_local_capture_file_directory.string();
    replay_settings.run_type = Dive::GfxrReplayOptions::kNormal;

    // Variant-specific config
    replay_settings.loop_single_frame_count = frame_count;

    return device_manager.RunReplayApk(replay_settings);
}

//--------------------------------------------------------------------------------------------------
absl::Status AnalyzeDialog::Pm4Replay(Dive::DeviceManager& device_manager,
                                      const std::string&   remote_gfxr_file)
{
    UpdateReplayStatus(ReplayStatusUpdateCode::kStartPm4Replay);
    Dive::GfxrReplaySettings replay_settings;
    replay_settings.remote_capture_path = remote_gfxr_file;
    replay_settings.local_download_dir = m_local_capture_file_directory.string();
    replay_settings.run_type = Dive::GfxrReplayOptions::kPm4Dump;

    return device_manager.RunReplayApk(replay_settings);
}

//--------------------------------------------------------------------------------------------------
absl::Status AnalyzeDialog::PerfCounterReplay(Dive::DeviceManager&            device_manager,
                                              const std::string&              remote_gfxr_file,
                                              const std::vector<std::string>& selected_metrics)
{
    UpdateReplayStatus(ReplayStatusUpdateCode::kStartPerfCounterReplay);
    Dive::GfxrReplaySettings replay_settings;
    replay_settings.remote_capture_path = remote_gfxr_file;
    replay_settings.local_download_dir = m_local_capture_file_directory.string();
    replay_settings.run_type = Dive::GfxrReplayOptions::kPerfCounters;

    // Variant-specific config
    replay_settings.metrics = selected_metrics;

    return device_manager.RunReplayApk(replay_settings);
}

//--------------------------------------------------------------------------------------------------
absl::Status AnalyzeDialog::GpuTimeReplay(Dive::DeviceManager& device_manager,
                                          const std::string&   remote_gfxr_file,
                                          int                  frame_count)
{
    UpdateReplayStatus(ReplayStatusUpdateCode::kStartGpuTimeReplay);
    Dive::GfxrReplaySettings replay_settings;
    replay_settings.remote_capture_path = remote_gfxr_file;
    replay_settings.local_download_dir = m_local_capture_file_directory.string();
    replay_settings.run_type = Dive::GfxrReplayOptions::kGpuTiming;

    // Variant-specific config
    replay_settings.loop_single_frame_count = frame_count;

    return device_manager.RunReplayApk(replay_settings);
}

//--------------------------------------------------------------------------------------------------
absl::Status AnalyzeDialog::RenderDocReplay(Dive::DeviceManager& device_manager,
                                            const std::string&   remote_gfxr_file)
{
    SetReplayButton("Replaying with RenderDoc...", false);
    Dive::GfxrReplaySettings replay_settings;
    replay_settings.remote_capture_path = remote_gfxr_file;
    replay_settings.local_download_dir = m_local_capture_file_directory.string();
    replay_settings.run_type = Dive::GfxrReplayOptions::kRenderDoc;

    // Variant-specific config
    // loop count will be set appropriately by ValidateGfxrReplaySettings

    return device_manager.RunReplayApk(replay_settings);
}

//--------------------------------------------------------------------------------------------------

void AnalyzeDialog::OnReplay()
{
    if (m_replay_active.valid())
    {
        return;
    }

    // Get info on which variants of replay to initiate runs for
    const ReplayConfig& config = m_impl->m_config;
    const bool          any_selected = config.replay_dump_pm4 || config.replay_gpu_time ||
                              config.replay_renderdoc || config.replay_perf_counter ||
                              config.replay_custom;
    if (!any_selected)
    {

        return ShowMessage("Select at least one option.");
    }
    if (config.replay_perf_counter && config.selected_metrics.empty())
    {
        return ShowMessage("Select at least one metrics.");
    }

    OverlayMessage("Replaying...");

    m_replay_active = std::async([this, config = config]() {
        ReplayImpl(config);
        UpdateReplayStatus(ReplayStatusUpdateCode::kDone);
    });
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::OnDeleteReplayArtifacts()
{
    if (m_replay_active.valid())
    {
        qDebug() << "Cannot call OnDeleteReplayArtifacts() during active replay";
        return;
    }

    m_replay_active = std::async([=, this]() {
        DeleteReplayArtifactsImpl();
        UpdateReplayStatus(ReplayStatusUpdateCode::kDone);
    });
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::OnReplayStatusUpdate(int status_code_int, const QString& message)
{
    // Cast from qt known type.
    auto status_code = static_cast<ReplayStatusUpdateCode>(status_code_int);
    bool execute_update = m_status_update_queue.empty();
    m_status_update_queue.push_back({ status_code, message });
    if (!execute_update)
    {
        // Only execute update if it's first call on the stack.
        // Qt does recursive signal processing sometime.
        return;
    }
    ExecuteStatusUpdate();
}
void AnalyzeDialog::ExecuteStatusUpdate()
{
    for (size_t index = 0; index < m_status_update_queue.size(); ++index)
    {
        // Note: copy the item to prevent iterator invalidation.
        StatusUpdateQueueItem item = m_status_update_queue[index];
        switch (item.status)
        {
        case ReplayStatusUpdateCode::kDone:
            if (m_replay_active.valid())
            {
                m_replay_active.get();
            }
            DisableOverlay();
            break;
        case ReplayStatusUpdateCode::kSuccess:
            ShowMessage(item.message.toStdString());
            SetReplayButton("", true);
            OverlayMessage("Replay done.");
            break;
        case ReplayStatusUpdateCode::kFailure:
            ShowMessage(item.message.toStdString());
            SetReplayButton("", true);
            OverlayMessage("Replay failed.");
            break;
        case ReplayStatusUpdateCode::kSetup:
            SetReplayButton("Setting up replay...", false);
            OverlayMessage("Setting up replay...");
            break;
        case ReplayStatusUpdateCode::kSetupDeviceFailure:
            ShowMessage(item.message.toStdString());
            SetReplayButton("", false);
            OnDeviceListRefresh();
            break;
        case ReplayStatusUpdateCode::kStartNormalReplay:
            SetReplayButton("Replaying...", false);
            OverlayMessage("Replaying...");
            break;
        case ReplayStatusUpdateCode::kStartPm4Replay:
            SetReplayButton("Replaying with PM4 dump enabled...", false);
            OverlayMessage("Replaying with PM4 dump enabled...");
            break;
        case ReplayStatusUpdateCode::kStartGpuTimeReplay:
            SetReplayButton("Replaying with GPU timing enabled...", false);
            OverlayMessage("Replaying with GPU timing enabled...");
            break;
        case ReplayStatusUpdateCode::kStartPerfCounterReplay:
            SetReplayButton("Replaying with perf counter settings...", false);
            OverlayMessage("Replaying with perf counter settings...");
            break;
        case ReplayStatusUpdateCode::kDeletingReplayArtifacts:
            OverlayMessage("Deleting temporary artifacts...");
            break;
        }
    }
    m_status_update_queue.clear();
}

void AnalyzeDialog::UpdateReplayStatus(ReplayStatusUpdateCode status, const std::string& message)
{
    qDebug() << message.c_str();
    ReplayStatusUpdated(static_cast<int>(status), QString::fromStdString(message));
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::ReplayImpl(const ReplayConfig& config)
{
    Dive::DeviceManager& device_manager = Dive::GetDeviceManager();

    auto device = device_manager.GetDevice();

    UpdateReplayStatus(ReplayStatusUpdateCode::kSetup);

    // Setup the device
    absl::Status ret = device->SetupDevice();
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Fail to setup device: ", ret.message());
        UpdateReplayStatus(ReplayStatusUpdateCode::kSetupDeviceFailure, err_msg);
        return;
    }

    // Get the asset file name
    absl::StatusOr<std::string>
    remote_file = PushFilesToDevice(device, m_impl->m_local_capture_files.gfxa.generic_string());
    if (!remote_file.ok())
    {
        std::string err_msg = absl::StrCat("Failed to deploy replay apk: ",
                                           remote_file.status().message());
        UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
        return;
    }

    // Deploying install/gfxr-replay.apk
    ret = device_manager.DeployReplayApk(config.device_serial);
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Failed to push files to device: ", ret.message());
        UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
        return;
    }

    // Run only replay with default settings
    if (config.replay_custom)
    {
        ret = NormalReplay(device_manager, remote_file.value(), config.replay_custom_frame_count);
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to run custom replay: ", ret.message());
            UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
            return;
        }

        UpdateReplayStatus(ReplayStatusUpdateCode::kSuccess,
                           "Custom Replay completed successfully.");
        // MainWindow needs to reload the capture so the correct PM4 data (or absence thereof) is
        // displayed
        emit CaptureUpdated(m_selected_capture_file_string);
        return;
    }

    // Run the pm4 replay
    if (config.replay_dump_pm4)
    {
        ret = Pm4Replay(device_manager, remote_file.value());
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to run pm4 replay: ", ret.message());
            UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
            return;
        }
        // MainWindow needs to reload the capture so the correct PM4 data (or absence thereof) is
        // displayed
        emit CaptureUpdated(m_selected_capture_file_string);
    }

    // Run the perf counter replay
    if (config.replay_perf_counter)
    {
        ret = PerfCounterReplay(device_manager, remote_file.value(), config.selected_metrics);
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to run perf counter replay: ",
                                               ret.message());
            UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
            return;
        }
    }

    // File could exist from previous runs
    if (std::filesystem::exists(m_impl->m_local_capture_files.perf_counter_csv))
    {
        qDebug() << "Loading perf counter data: "
                 << m_impl->m_local_capture_files.perf_counter_csv.string().c_str();
        emit DisplayPerfCounterResults(
        QString::fromStdString(m_impl->m_local_capture_files.perf_counter_csv.string()));
    }
    else
    {
        qDebug() << "Cleared perf counter data";
        emit DisplayPerfCounterResults("");
    }

    // Run the gpu_time replay
    if (config.replay_gpu_time)
    {
        ret = GpuTimeReplay(device_manager,
                            remote_file.value(),
                            config.replay_gpu_time_frame_count);
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to run gpu_time replay: ", ret.message());
            UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
            return;
        }
    }

    // File could exist from previous runs
    if (std::filesystem::exists(m_impl->m_local_capture_files.gpu_timing_csv))
    {
        qDebug() << "Loading gpu timing data: "
                 << m_impl->m_local_capture_files.gpu_timing_csv.string().c_str();
        emit DisplayGpuTimingResults(
        QString::fromStdString(m_impl->m_local_capture_files.gpu_timing_csv.string()));
    }
    else
    {
        qDebug() << "Cleared gpu timing data";
        emit DisplayGpuTimingResults("");
    }

    if (config.replay_renderdoc)
    {
        ret = RenderDocReplay(device_manager, remote_file.value());
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to run replay with RenderDoc Capture: ",
                                               ret.message());
            UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
            return;
        }
        qDebug() << "RenderDoc capture saved to: "
                 << m_impl->m_local_capture_files.renderdoc_rdc.string().c_str();
    }

    UpdateReplayStatus(ReplayStatusUpdateCode::kSuccess, "Replay completed successfully.");
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::DeleteReplayArtifactsImpl()
{
    qDebug() << "Attempting to delete replay artifacts from previous runs...";
    UpdateReplayStatus(ReplayStatusUpdateCode::kDeletingReplayArtifacts);

    AttemptDeletingTemporaryLocalFile(m_impl->m_local_capture_files.perf_counter_csv);
    AttemptDeletingTemporaryLocalFile(m_impl->m_local_capture_files.gpu_timing_csv);
    AttemptDeletingTemporaryLocalFile(m_impl->m_local_capture_files.pm4_rd);
}
