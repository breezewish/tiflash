#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <grpcpp/client_context.h>
#include <grpcpp/server_context.h>
#pragma GCC diagnostic pop

#include <opentelemetry/context/propagation/text_map_propagator.h>

namespace DB
{

class GrpcClientCarrier : public opentelemetry::context::propagation::TextMapCarrier
{
public:
    explicit GrpcClientCarrier(grpc::ClientContext * context_)
        : context(context_)
    {}

    opentelemetry::nostd::string_view Get(
        opentelemetry::nostd::string_view /* key */) const noexcept override
    {
        return "";
    }

    void Set(opentelemetry::nostd::string_view key,
             opentelemetry::nostd::string_view value) noexcept override
    {
        // std::cout << " Client ::: Adding " << key << " " << value << "\n";
        context->AddMetadata(std::string(key), std::string(value));
    }

    grpc::ClientContext * context;
};

class GrpcClientInterceptorCarrier : public opentelemetry::context::propagation::TextMapCarrier
{
public:
    explicit GrpcClientInterceptorCarrier(::grpc::experimental::InterceptorBatchMethods * methods_)
        : methods(methods_)
    {}

    opentelemetry::nostd::string_view Get(
        opentelemetry::nostd::string_view /* key */) const noexcept override
    {
        return "";
    }

    void Set(opentelemetry::nostd::string_view key,
             opentelemetry::nostd::string_view value) noexcept override
    {
        // std::cout << " Client ::: Adding " << key << " " << value << "\n";
        methods->GetSendInitialMetadata()->emplace(std::string(key), std::string(value));
    }

    ::grpc::experimental::InterceptorBatchMethods * methods;
};

class GrpcServerCarrier : public opentelemetry::context::propagation::TextMapCarrier
{
public:
    explicit GrpcServerCarrier(grpc::ServerContextBase * context_)
        : context(context_)
    {}

    std::string_view Get(std::string_view key) const noexcept override
    {
        auto it = context->client_metadata().find(key.data());
        if (it != context->client_metadata().end())
        {
            return it->second.data();
        }

        return "";
    }

    void Set(std::string_view, std::string_view) noexcept override
    {
        // Not required for server
    }

    grpc::ServerContextBase * context;
};

} // namespace DB
