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

#include "dive_application.h"

#include <fcntl.h>

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSplashScreen>
#include <QStyleFactory>
#include <QTimer>
#include <cstdio>
#include <iostream>

#include "absl/debugging/failure_signal_handler.h"
#include "absl/debugging/symbolize.h"
#include "application_controller.h"
#include "custom_metatypes.h"
#include "dive_core/common.h"
#include "dive_core/pm4_info.h"
#include "main_window.h"
#include "utils/version_info.h"
#ifdef __linux__
#    include <dlfcn.h>
#endif

#if defined(_WIN32)
#    include <io.h>
#else
#    include <unistd.h>
#endif

constexpr int kSplashScreenDuration = 2000;  // 2s
constexpr int kStartDelay = 500;             // 0.5s

namespace
{

bool SetApplicationStyle(QString style_key)
{
    QStringList style_list = QStyleFactory::keys();
    for (uint32_t i = 0; i < (uint32_t)style_list.size(); ++i)
    {
        if (style_list[i] == style_key)
        {
            QApplication::setStyle(QStyleFactory::create(style_key));
            return true;
        }
    }
    return false;
}

int SetupQtForDiveImpl()
{
    Dive::RegisterCustomMetaType();

    // Optional command arg loading method for fast iteration
    // Note: Set the style *before* QApplication constructor. This allows commandline
    // "-style <style>" style override to still work properly.

    // Try setting "Fusion" style. If not found, set "Windows".
    // And if that's not found, default to whatever style the factory provides.
    if (!SetApplicationStyle("Fusion"))
    {
        if (!SetApplicationStyle("Windows"))
        {
            if (!QStyleFactory::keys().empty())
            {
                SetApplicationStyle(QStyleFactory::keys()[0]);
            }
        }
    }

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    return 0;
}

void SetupQtForDive()
{
    [[maybe_unused]] static int ignore = SetupQtForDiveImpl();
}

void SetDarkMode(QApplication &app)
{
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(40, 40, 40));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    darkPalette.setColor(QPalette::Disabled, QPalette::Window, QColor(90, 90, 90));
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::Disabled, QPalette::Base, QColor(80, 80, 80));
    darkPalette.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(90, 90, 90));
    darkPalette.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(90, 90, 90));
    darkPalette.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(160, 160, 160));
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::Disabled, QPalette::Button, QColor(80, 80, 80));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(160, 160, 160));
    darkPalette.setColor(QPalette::Disabled, QPalette::Link, QColor(160, 160, 160));
    darkPalette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(90, 90, 90));
    darkPalette.setColor(QPalette::Disabled, QPalette::Light, QColor(53, 53, 53));

    QApplication::setPalette(darkPalette);
}

}  // namespace


DiveApplication::DiveApplication(int argc, char *argv[]) :
    QApplication(argc, argv)
{
}

DiveApplication *DiveApplication::Create(int argc, char *argv[])
{
    SetupQtForDive();

    DiveApplication *app = new DiveApplication(argc, argv);
    app->Initialize();
    return app;
}

void DiveApplication::Initialize()
{
    this->setWindowIcon(QIcon(":/images/dive.ico"));
    SetDarkMode(*this);

    // Load and apply the style sheet
    QFile style_sheet(":/stylesheet.qss");
    style_sheet.open(QFile::ReadOnly);
    QString style(style_sheet.readAll());
    this->setStyleSheet(style);

    // Display splash screen
    QSplashScreen *splash_screen = new QSplashScreen();
    splash_screen->setPixmap(QPixmap(":/images/dive.png"));
    splash_screen->show();
    QTimer::singleShot(kSplashScreenDuration, splash_screen, SLOT(close()));

    // Initialize packet info query data structures needed for parsing
    Pm4InfoInit();

    ApplicationController controller;
    MainWindow           *main_window = new MainWindow(controller);
    if (exit_after_load)
    {
        QObject::connect(main_window, &MainWindow::FileLoaded, main_window, &MainWindow::close);
    }

    if (!controller.InitializePlugins())
    {
        qDebug()
        << "Application: Plugin initialization failed. Application may proceed without plugins.";
    }

    if (argc == 2)
    {
        // This is executed async.
        main_window->LoadFile(argv[1], false, true);
    }

    QTimer::singleShot(kStartDelay, main_window, SLOT(show()));

    return app.exec();
}
