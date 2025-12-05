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

#include <QTimer>
#include <QPixmap>
#include <filesystem>

#include "ui/main_window.h"
#include "ui/scenarios/controller.h"
#include "ui/scenarios/scenarios.h"

constexpr std::string_view kScenarioScreenshot = "screenshot";
constexpr std::string_view kScenarioExitAfterLoad = "exit_after_load";
namespace
{
constexpr int kScreenshotDelay = 2000;  // 2s

void ScenarioScreenshot(ScenarioController& controller)
{
    auto func = [&controller]() {
        MainWindow* main_window = controller.GetMainWindow();
        auto        out_dir = std::filesystem::path(controller.GetOutput());
        auto        out_screenshot = out_dir / (controller.GetPrefix() + "screenshot.png");

        QPixmap pixmap(main_window->size());
        main_window->render(&pixmap);
        pixmap.save(QString::fromStdString(out_screenshot.string()));
        main_window->close();
    };

    QObject::connect(controller.GetMainWindow(),
                     &MainWindow::FileLoaded,
                     controller.GetMainWindow(),
                     [func, &controller]() {
                         QTimer::singleShot(kScreenshotDelay, controller.GetMainWindow(), func);
                     });
}

void ScenarioExitAfterLoad(ScenarioController& controller)
{
    QObject::connect(controller.GetMainWindow(),
                     &MainWindow::FileLoaded,
                     controller.GetMainWindow(),
                     &MainWindow::close);
}

}  // namespace

std::vector<std::string> DiveUIScenarios::GetScenarios()
{
    return { std::string(kScenarioScreenshot) };
}

void DiveUIScenarios::ExecuteScenario(ScenarioController& controller, std::string_view scenario)
{
    if (scenario == kScenarioScreenshot)
    {
        ScenarioScreenshot(controller);
        return;
    }

    if (scenario == kScenarioExitAfterLoad)
    {
        ScenarioExitAfterLoad(controller);
    }
}
