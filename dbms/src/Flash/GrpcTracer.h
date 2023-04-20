#pragma once

#include <Common/Exception.h>
#include <Common/Tracer.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wdeprecated-builtins"
#include <grpcpp/client_context.h>
#include <grpcpp/server_context.h>
#pragma GCC diagnostic pop

namespace DB
{

struct GrpcTraceContext
{
    std::shared_ptr<opentelemetry::trace::Span> span = nullptr;
    std::string foo;
};

class GrpcTracer
{
public:
    static opentelemetry::trace::Scope getScopeFromGrpcContext(grpc::ServerContextBase * grpc_ctx);

    static GrpcTraceContext * getGrpcTraceContext(grpc::ServerContextBase * grpc_ctx);

    static void setGrpcTraceContext(grpc::ServerContextBase * grpc_ctx, GrpcTraceContext * trace_ctx);
};

} // namespace DB
