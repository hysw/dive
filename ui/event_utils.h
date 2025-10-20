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

// Qt might run nested event inside a slot.
// Return immidiately from the inner calls and loop for the outer ones.
class NestingGuard
{
public:
    struct Flag
    {
        bool active = false;
    };
    explicit NestingGuard(Flag& flag) :
        m_flag(flag),
        m_already_active(flag.active)
    {
        if (!m_already_active)
        {
            m_flag.active = true;
        }
    }
    ~NestingGuard()
    {
        if (!m_already_active)
        {
            m_flag.active = false;
        }
    }
    bool AlreadyActive() const { return m_already_active; }

private:
    Flag& m_flag;
    bool  m_already_active;
};
