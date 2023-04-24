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

#include <Common/Tracer.h>
#include <opentelemetry/trace/provider.h>

namespace DB
{

void GlobalTracer::init()
{
    auto provider = opentelemetry::trace::Provider::GetTracerProvider();
    auto tracer = provider->GetTracer("tiflash_dbms", "0.0.0");
    instance = tracer;
}

void DeferredSpan::commit(
    std::string_view name,
    std::initializer_list<std::pair<std::string_view, opentelemetry::common::AttributeValue>> attributes) const
{
    auto now = std::chrono::steady_clock::now();
    auto span = GlobalTracer::get()->StartSpan(
        name,
        attributes,
        opentelemetry::trace::StartSpanOptions{
            .start_system_time = start_ts,
            .start_steady_time = start_tick,
        });
    span->End({
        .end_steady_time = now,
    });
}

} // namespace DB
