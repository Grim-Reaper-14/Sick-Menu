#include "Reaper.hpp"
#include "core/logging/sinks/MemorySink.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <memory>
#include <thread>

int main()
{
    using namespace std::chrono_literals;

    Reaper::Log::LoggerConfig config;
    config.minimumLevel = Reaper::Log::Level::Trace;
    config.console = false;
    config.debugger = false;
    config.filePath.clear();
    config.queueCapacity = 128;
    config.recentCapacity = 128;

    assert(Reaper::Log::Initialize(config));

    auto memory = std::make_shared<Sick::Core::Logging::MemorySink>(128);
    Reaper::Log::Logger::Get().AddSink(memory);
    Reaper::Log::SetThreadName("ObservabilityTest");

    {
        Reaper::CorrelationScope correlation{42};
        Reaper::LogContext context{
            Reaper::Log::MakeField("feature", "test")
        };

        REAPER_INFO("Test", "Structured message {}", 7);
        REAPER_WARN_ONCE("Test", "This warning should be rate limited");

        Reaper::Trace::Span span{"Test", "MeasuredWork", 50ms};
        span.Field("phase", "unit-test");
        std::this_thread::sleep_for(1ms);
    }

    Reaper::Log::AnomalyDetector::Get().Configure(1s, 3);
    for (int i = 0; i < 3; ++i)
        REAPER_WARN("Burst", "Repeated failure");

    assert(!REAPER_VERIFY(false, "Test", "Verification failure captured"));

    Reaper::Metrics::Increment("test.counter", 3);
    Reaper::Metrics::SetGauge("test.gauge", 12.5);
    Reaper::Metrics::Observe("test.latency", 2.0);
    Reaper::Metrics::Observe("test.latency", 4.0);

    const auto metricSnapshot = Reaper::Metrics::GetSnapshot();
    assert(metricSnapshot.counters.at("test.counter") == 3);
    assert(metricSnapshot.gauges.at("test.gauge") == 12.5);
    assert(metricSnapshot.distributions.at("test.latency").count == 2);
    assert(metricSnapshot.distributions.at("test.latency").minimum == 2.0);
    assert(metricSnapshot.distributions.at("test.latency").maximum == 4.0);

    Reaper::Health::Register("TestHeartbeat", 1ms);
    std::this_thread::sleep_for(5ms);
    const auto issues = Reaper::Health::CheckNow();
    assert(!issues.empty());
    Reaper::Health::Heartbeat("TestHeartbeat");

    auto& scheduler = Reaper::Scheduler::Get();
    scheduler.SetSlowJobThreshold(1ms);
    scheduler.Queue([]
    {
        std::this_thread::sleep_for(3ms);
    });
    assert(scheduler.Tick() == 1);

    Reaper::Log::Flush();

    const auto records = memory->Snapshot();
    assert(!records.empty());

    const auto structured = std::find_if(records.begin(), records.end(), [](const auto& record)
    {
        return record.category == "Test" && record.message == "Structured message 7";
    });
    assert(structured != records.end());
    assert(structured->correlationId == 42);
    assert(structured->threadName == "ObservabilityTest");
    assert(!structured->fields.empty());

    const auto anomaly = std::find_if(records.begin(), records.end(), [](const auto& record)
    {
        return std::any_of(record.fields.begin(), record.fields.end(), [](const auto& field)
        {
            return field.key == "anomaly" && field.value == "burst";
        });
    });
    assert(anomaly != records.end());

    const auto assertion = std::find_if(records.begin(), records.end(), [](const auto& record)
    {
        return record.event == Reaper::Log::EventId::AssertionFailed;
    });
    assert(assertion != records.end());

    const auto health = std::find_if(records.begin(), records.end(), [](const auto& record)
    {
        return record.event == Reaper::Log::EventId::HealthStall;
    });
    assert(health != records.end());

    const auto slowJob = std::find_if(records.begin(), records.end(), [](const auto& record)
    {
        return record.event == Reaper::Log::EventId::SchedulerSlowJob;
    });
    assert(slowJob != records.end());

    const auto stats = Reaper::Log::Logger::Get().Stats();
    assert(stats.dispatched >= records.size());
    assert(stats.dropped == 0);
    assert(stats.anomalyDetections >= 1);

    Reaper::Health::Stop();
    Reaper::Log::Shutdown();
    return 0;
}
