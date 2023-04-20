#include <opentelemetry/sdk/trace/exporter.h>
#include <opentelemetry/sdk/trace/span_data.h>

namespace DB
{

class LogSpanExporter final : public opentelemetry::sdk::trace::SpanExporter
{
public:
    LogSpanExporter();

    std::unique_ptr<opentelemetry::sdk::trace::Recordable> MakeRecordable() noexcept override;

    opentelemetry::sdk::common::ExportResult Export(
        const opentelemetry::nostd::span<std::unique_ptr<opentelemetry::sdk::trace::Recordable>> & spans) noexcept override;

    bool Shutdown(std::chrono::microseconds timeout) noexcept override;

private:
    // Mapping status number to the string from api/include/opentelemetry/trace/canonical_code.h
    std::map<int, std::string> status_map{{0, "Unset"}, {1, "Ok"}, {2, "Error"}};
};

} // namespace DB
