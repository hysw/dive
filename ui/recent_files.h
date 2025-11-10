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

#include <QObject>

#include "file_path.h"
#include "impl_pointer.h"

class QMenu;
class QStringList;

class RecentFiles : public QObject
{
    Q_OBJECT
public:
    RecentFiles();
    ~RecentFiles();
    void setParent(QObject* parent) = delete;

    struct Impl;

    void Init();
    void Append(const Dive::FilePath& file);

    void InjectActions(QMenu* menu);

signals:
    void FileSelected(const Dive::FilePath& file);

private slots:
    void OnRecentFilesAction();

private:
    ImplPointer<Impl> m_impl;

    void Update(const QStringList& recent_files, bool write_settings = true);
};
