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

#include <type_traits>
#include <utility>

namespace Dive
{

template<typename Value, typename Tag = void> class Wrapper
{
public:
    using ValueType = Value;

    Wrapper() = default;
    Wrapper(const Wrapper&) = default;
    Wrapper(Wrapper&&) = default;
    Wrapper& operator=(const Wrapper&) & = default;
    Wrapper& operator=(Wrapper&&) & = default;
    Wrapper& operator=(const Wrapper&) && = delete;
    Wrapper& operator=(Wrapper&&) && = delete;
    explicit Wrapper(const ValueType& value) :
        m_value(value)
    {
    }
    explicit Wrapper(ValueType&& value) :
        m_value(std::move(value))
    {
    }
    template<typename... Args>
    Wrapper(std::in_place_t, Args&&... args) :
        m_value(std::forward<Args>(args)...)
    {
    }

    explicit operator ValueType() const { return m_value; }

    // std::optional symantics
    constexpr bool   has_value() const { return true; }
    ValueType&       value() { return m_value; }
    const ValueType& value() const { return m_value; }
    ValueType&       operator*() { return m_value; }
    const ValueType& operator*() const { return m_value; }
    ValueType*       operator->() { return &m_value; }
    const ValueType* operator->() const { return &m_value; }

private:
    ValueType m_value;
};

}  // namespace Dive
