#include <Common/LogSpanExporter.h>
#include <Common/Logger.h>
#include <common/logger_useful.h>

#include <magic_enum.hpp>

namespace trace_sdk = opentelemetry::sdk::trace;
namespace sdkcommon = opentelemetry::sdk::common;

namespace fmt
{

template <>
struct formatter<opentelemetry::trace::SpanKind>
{
    static constexpr auto parse(format_parse_context & ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const opentelemetry::trace::SpanKind & v, FormatContext & ctx)
    {
        return format_to(ctx.out(), "{}", magic_enum::enum_name(v));
    }
};

template <>
struct formatter<sdkcommon::OwnedAttributeValue>
{
    static constexpr auto parse(format_parse_context & ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const sdkcommon::OwnedAttributeValue & v, FormatContext & ctx)
    {
        auto ret = ctx.out();
        std::visit(
            [&](auto && arg) {
                ret = format_to(ctx.out(), "{}", arg);
            },
            v);
        return ret;
    }
};

} // namespace fmt

namespace DB
{

void printAttributes(
    const std::unordered_map<std::string, sdkcommon::OwnedAttributeValue> & map,
    const std::string prefix = "    ")
{
    for (const auto & kv : map)
    {
        LOG_INFO(Logger::get(), "{}{}: {}", prefix, kv.first, kv.second);
    }
}

void printEvents(const std::vector<trace_sdk::SpanDataEvent> & events)
{
    for (const auto & event : events)
    {
        LOG_INFO(Logger::get(), "    {");
        LOG_INFO(Logger::get(), "      name          : {}", event.GetName());
        LOG_INFO(Logger::get(), "      timestamp     : {}", event.GetTimestamp().time_since_epoch().count());
        LOG_INFO(Logger::get(), "      attributes    : ");
        printAttributes(event.GetAttributes(), "        ");
        LOG_INFO(Logger::get(), "    }");
    }
}

void printLinks(const std::vector<trace_sdk::SpanDataLink> & links)
{
    for (const auto & link : links)
    {
        char trace_id[32] = {0};
        char span_id[16] = {0};
        link.GetSpanContext().trace_id().ToLowerBase16(trace_id);
        link.GetSpanContext().span_id().ToLowerBase16(span_id);
        LOG_INFO(Logger::get(), "    {");
        LOG_INFO(Logger::get(), "      trace_id      : {}", std::string(trace_id, 32));
        LOG_INFO(Logger::get(), "      span_id       : {}", std::string(span_id, 16));
        LOG_INFO(Logger::get(), "      tracestate    : {}", link.GetSpanContext().trace_state()->ToHeader());
        LOG_INFO(Logger::get(), "      attributes    : ");
        printAttributes(link.GetAttributes(), "        ");
        LOG_INFO(Logger::get(), "    }");
    }
}

void printResources(const opentelemetry::sdk::resource::Resource & resources)
{
    const auto & attributes = resources.GetAttributes();
    if (!attributes.empty())
    {
        printAttributes(attributes, "    ");
    }
}

LogSpanExporter::LogSpanExporter() = default;

std::unique_ptr<trace_sdk::Recordable> LogSpanExporter::MakeRecordable() noexcept
{
    return std::unique_ptr<trace_sdk::Recordable>(new trace_sdk::SpanData);
}

opentelemetry::sdk::common::ExportResult LogSpanExporter::Export(
    const std::span<std::unique_ptr<trace_sdk::Recordable>> & spans) noexcept
{
    for (auto & recordable : spans)
    {
        auto span = std::unique_ptr<trace_sdk::SpanData>(
            static_cast<trace_sdk::SpanData *>(recordable.release()));

        if (span != nullptr)
        {
            char trace_id[32] = {0};
            char span_id[16] = {0};
            char parent_span_id[16] = {0};

            span->GetTraceId().ToLowerBase16(trace_id);
            span->GetSpanId().ToLowerBase16(span_id);
            span->GetParentSpanId().ToLowerBase16(parent_span_id);

            LOG_INFO(Logger::get(), "{");
            LOG_INFO(Logger::get(), "  name          : {}", span->GetName());
            LOG_INFO(Logger::get(), "  trace_id      : {}", std::string(trace_id, 32));
            LOG_INFO(Logger::get(), "  span_id       : {}", std::string(span_id, 16));
            LOG_INFO(Logger::get(), "  tracestate    : {}", span->GetSpanContext().trace_state()->ToHeader());
            LOG_INFO(Logger::get(), "  parent_span_id: {}", std::string(parent_span_id, 16));
            LOG_INFO(Logger::get(), "  start         : {}", span->GetStartTime().time_since_epoch().count());
            LOG_INFO(Logger::get(), "  duration      : {}", span->GetDuration().count());
            LOG_INFO(Logger::get(), "  description   : {}", span->GetDescription());
            LOG_INFO(Logger::get(), "  span kind     : {}", span->GetSpanKind());
            LOG_INFO(Logger::get(), "  status        : {}", status_map[int(span->GetStatus())]);
            LOG_INFO(Logger::get(), "  attributes    : ");
            printAttributes(span->GetAttributes());
            LOG_INFO(Logger::get(), "  events        : ");
            printEvents(span->GetEvents());
            LOG_INFO(Logger::get(), "  links         : ");
            printLinks(span->GetLinks());
            LOG_INFO(Logger::get(), "  resources     : ");
            printResources(span->GetResource());
            LOG_INFO(Logger::get(), "}");
        }
    }

    return opentelemetry::sdk::common::ExportResult::kSuccess;
}

bool LogSpanExporter::Shutdown(std::chrono::microseconds /* timeout */) noexcept
{
    return true;
}

} // namespace DB
