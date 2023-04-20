#include <Flash/GrpcTracer.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wdeprecated-builtins"
#include "../../contrib/grpc/src/core/lib/surface/call.h"
#pragma GCC diagnostic pop

namespace DB
{

opentelemetry::trace::Scope GrpcTracer::getScopeFromGrpcContext(grpc::ServerContextBase * grpc_ctx)
{
    auto * trace_ctx = getGrpcTraceContext(grpc_ctx);
    std::shared_ptr<opentelemetry::trace::Span> active_span;
    if (trace_ctx != nullptr)
        active_span = trace_ctx->span;
    else
        active_span = std::make_shared<opentelemetry::trace::DefaultSpan>(opentelemetry::trace::SpanContext::GetInvalid());
    return GlobalTracer::get()->WithActiveSpan(active_span);
}

GrpcTraceContext * GrpcTracer::getGrpcTraceContext(grpc::ServerContextBase * grpc_ctx)
{
    auto * trace_ctx = grpc_call_context_get(grpc_ctx->c_call(), GRPC_CONTEXT_TRACING);
    if (trace_ctx == nullptr)
        return nullptr;
    return static_cast<GrpcTraceContext *>(trace_ctx);
}

void GrpcTracer::setGrpcTraceContext(grpc::ServerContextBase * grpc_ctx, GrpcTraceContext * trace_ctx)
{
    RUNTIME_CHECK(trace_ctx != nullptr);
    grpc_call_context_set(grpc_ctx->c_call(), GRPC_CONTEXT_TRACING, trace_ctx, nullptr);
}

} // namespace DB
