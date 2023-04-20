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

namespace DB
{

using Tracer = opentelemetry::trace::Tracer;
using TracerPtr = std::shared_ptr<Tracer>;

class GlobalTracer
{
    static inline TracerPtr instance = std::make_shared<opentelemetry::trace::NoopTracer>();

public:
    static void init();

    static TracerPtr get()
    {
        return instance;
    }
};

} // namespace DB
