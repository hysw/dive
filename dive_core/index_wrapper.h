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

#include <limits>
#include <optional>

namespace Dive
{

// A wrapper type for uint64_t / size_t to reduce the chance of using the wrong index.
template<typename ValueT, typename TagT = void> class IndexWrapper
{
public:
    using ValueType = ValueT;
    using TagType = TagT;

    struct Hash
    {
        std::hash<ValueType> m_impl;

        auto operator()(const IndexWrapper& w) const { return m_impl(w.m_value); }
    };

    static constexpr ValueType kInvalid = std::numeric_limits<ValueType>::max();

    IndexWrapper() = default;
    explicit IndexWrapper(ValueType value) :
        m_value(value)
    {
    }

    std::optional<ValueType> AsOptional() const
    {
        if (has_value())
        {
            return value();
        }
        return std::nullopt;
    }

    // std::optional
    bool      has_value() const { return m_value != kInvalid; }
    ValueType value() const
    {
        assert(has_value());
        return m_value;
    }

    ValueType operator*() const { return value(); }
    explicit  operator bool() const { return has_value(); }

    bool operator==(const IndexWrapper& other) const { return m_value == other.m_value; }

    [[deprecated]] explicit operator ValueType() { return m_value; }

private:
    ValueType m_value = kInvalid;
};

}  // namespace Dive
