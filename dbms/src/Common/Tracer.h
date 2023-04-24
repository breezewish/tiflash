// Copyright 2022 PingCAP, Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <opentelemetry/trace/noop.h>
#include <opentelemetry/trace/tracer.h>

#include <chrono>

namespace DB
{

using Tracer = opentelemetry::trace::Tracer;
using TracerPtr = std::shared_ptr<Tracer>;

class DeferredSpan
{
    friend class GlobalTracer;

public:
    int elapsedMillis() const
    {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_tick).count();
    }

    void commit(
        std::string_view name,
        std::initializer_list<std::pair<std::string_view, opentelemetry::common::AttributeValue>> attributes = {}) const;

private:
    DeferredSpan() = default;

    std::chrono::system_clock::time_point start_ts = std::chrono::system_clock::now();
    std::chrono::steady_clock::time_point start_tick = std::chrono::steady_clock::now();
    std::shared_ptr<opentelemetry::trace::Span> span;
};

class GlobalTracer
{
    static inline TracerPtr instance = std::make_shared<opentelemetry::trace::NoopTracer>();

public:
    static void init();

    static TracerPtr get()
    {
        return instance;
    }

    /// Deferred span means it is not really started here, only its time is recorded.
    /// We can decide whether the span should be kept later.
    /// A deferred span does not have scope.
    static DeferredSpan startDeferredSpan()
    {
        return DeferredSpan{};
    }
};

} // namespace DB
