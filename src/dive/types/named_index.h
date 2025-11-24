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

#include <cassert>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

namespace Dive
{

template<typename Value, typename Tag = void> class TypedIndex
{
public:
    using ValueType = Value;
    static constexpr ValueType kInvalidValue = std::is_unsigned_v<ValueType> ?
                                               std::numeric_limits<ValueType>::max() :
                                               -1;

    TypedIndex() = default;
    TypedIndex(const TypedIndex&) = default;
    TypedIndex(TypedIndex&&) = default;
    TypedIndex& operator=(const TypedIndex&) & = default;
    TypedIndex& operator=(TypedIndex&&) & = default;
    TypedIndex& operator=(const TypedIndex&) && = delete;
    TypedIndex& operator=(TypedIndex&&) && = delete;
    explicit TypedIndex(const ValueType& value) :
        m_value(value)
    {
    }
    static TypedIndex Invalid() { return TypedIndex(); }

    explicit operator ValueType() const { return m_value; }

    std::optional<ValueType> AsOptional() const
    {
        if (has_value())
        {
            return value();
        }
        return std::nullopt;
    }

    // std::optional symantics
    bool     has_value() const { return m_value != kInvalidValue; }
    explicit operator bool() const { return has_value(); }

    ValueType& value()
    {
        assert(has_value());
        return m_value;
    }
    const ValueType& value() const
    {
        assert(has_value());
        return m_value;
    }

    ValueType&       operator*() { return value(); }
    const ValueType& operator*() const { return value(); }
    ValueType*       operator->() { return &value(); }
    const ValueType* operator->() const { return &value(); }

    auto operator<=>(const TypedIndex&) const = default;

    struct Hash
    {
        std::hash<ValueType> m_impl;

        auto operator()(const TypedIndex& index) const { return m_impl(index.m_value); }
    };

private:
    ValueType m_value = kInvalidValue;
};

#define DIVE_DEFINE_TYPED_INDEX(Name, Type) \
    struct DiveTypeTagFor##Name;            \
    using Name = TypedIndex<Type, DiveTypeTagFor##Name>;

}  // namespace Dive
