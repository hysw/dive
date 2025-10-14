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

#include <QThread>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qthread.h>

#include "async_worker.h"

AsyncWorkerImpl::AsyncWorkerImpl(QObject* owner) :
    m_owner(owner)
{
}

AsyncWorkerImpl::~AsyncWorkerImpl()
{
    if (m_thread)
    {
        m_thread->quit();
    }
}

void AsyncWorkerImpl::Initialize(QObject* worker)
{
    m_thread = new QThread(m_owner);
    QObject::connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);
    worker->moveToThread(m_thread);
    m_thread->start();
}

void AsyncWorkerImpl::MaybeDelete(QObject* worker)
{
    if (!worker || m_owner)
    {
        return;
    }
    worker->deleteLater();
}
