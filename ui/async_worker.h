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

class QObject;
class QThread;
class QMetaObject;

class AsyncWorkerImpl
{
protected:
    explicit AsyncWorkerImpl(QObject* owner);
    ~AsyncWorkerImpl();

    AsyncWorkerImpl(const AsyncWorkerImpl&) = delete;
    AsyncWorkerImpl(AsyncWorkerImpl&&) = delete;
    AsyncWorkerImpl& operator=(const AsyncWorkerImpl&) = delete;
    AsyncWorkerImpl& operator=(AsyncWorkerImpl&&) = delete;

    void Initialize(QObject* worker);
    void MaybeDelete(QObject* worker);

private:
    QObject* m_owner;
    QThread* m_thread = nullptr;
};

// An async worker wrapper.
template<typename WorkerObjectT = QObject, typename InvokerT = QMetaObject>
class AsyncWorker : public AsyncWorkerImpl
{
public:
    explicit AsyncWorker(QObject* owner = nullptr) :
        AsyncWorkerImpl(owner),
        m_worker(new WorkerObjectT(owner))
    {
        Initialize(m_worker);
    }
    ~AsyncWorker() { MaybeDelete(m_worker); }

    WorkerObjectT* get() const { return m_worker; }
    WorkerObjectT* operator->() const { return m_worker; }
    WorkerObjectT& operator*() const { return *m_worker; }

    template<typename Func> void Run(Func&& func)
    {
        InvokerT::invokeMethod(m_worker, [=]() { func(); });
    }

private:
    WorkerObjectT* m_worker;
};
