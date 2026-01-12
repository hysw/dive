/*
 Copyright 2021 Google LLC

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

#include <QFrame>

#include "dive/ui/types/impl_pointer.h"

// Forward declaration
class QTreeWidget;
class QTreeWidgetItem;
namespace Dive
{
class DataCore;
class EventStateInfo;
}  // namespace Dive

//--------------------------------------------------------------------------------------------------
class EventStateView : public QFrame
{
    Q_OBJECT

 public:
    struct Impl;
    EventStateView(const Dive::DataCore& data_core);
    ~EventStateView();

 private slots:
    void OnEventSelected(uint64_t node_index);
    void OnHover(QTreeWidgetItem* item_ptr, int column);

 protected:
    virtual void leaveEvent(QEvent* event) override;

 private:
    ImplPointer<Impl> m_impl;
};
