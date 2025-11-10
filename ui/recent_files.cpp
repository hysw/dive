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

#include "recent_files.h"

#include <QAction>
#include <QMenu>
#include <QStringList>
#include <QFileInfo>
#include <QApplication>
#include <QStyle>

#include "settings.h"

struct RecentFiles::Impl
{
    std::array<QAction *, 3> m_recent_file_actions = {};
};

RecentFiles::~RecentFiles() = default;

RecentFiles::RecentFiles()
{
    // Recent file actions
    for (auto &action : m_impl->m_recent_file_actions)
    {
        action = new QAction(this);
        action->setVisible(false);
        QObject::connect(action, &QAction::triggered, this, &RecentFiles::OnRecentFilesAction);
    }
}

void RecentFiles::Init()
{
    Update(Settings::Get()->ReadRecentFiles(), false);
}

void RecentFiles::Update(const QStringList &recent_files, bool write_settings)
{
    if (write_settings)
    {
        Settings::Get()->WriteRecentFiles(recent_files);
    }
    int next_file_index = 0;
    for (auto action : m_impl->m_recent_file_actions)
    {
        int file_index = next_file_index++;
        if (file_index < recent_files.count())
        {
            QString text = tr("%1").arg(QFileInfo(recent_files[file_index]).fileName());
            action->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileIcon));
            action->setText(text);
            action->setData(recent_files[file_index]);
            action->setVisible(true);
        }
        else
        {
            action->setVisible(false);
        }
    }
}

void RecentFiles::Append(const Dive::FilePath &file)
{
    auto        file_name = file.ToQString();
    QStringList recent_files = Settings::Get()->ReadRecentFiles();
    recent_files.removeAll(file_name);
    recent_files.prepend(file_name);
    while (recent_files.size() > static_cast<int>(m_impl->m_recent_file_actions.size()))
    {
        recent_files.pop_back();
    }
    Update(recent_files);
}

void RecentFiles::InjectActions(QMenu *menu)
{
    for (auto action : m_impl->m_recent_file_actions)
    {
        menu->addAction(action);
    }
}

void RecentFiles::OnRecentFilesAction()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        FileSelected(
        Dive::FilePath{ std::filesystem::path(action->data().toString().toStdString()) });
    }
}
