#include "CrashReporter.hpp"
#include "Metrics.hpp"
#include "StackTrace.hpp"
#include "core/logging/Formatter.hpp"
#include "core/logging/Logger.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#endif

namespace
{
    std::mutex g_CrashMutex;
    Sick::Core::Crash::Config g_Config;
    std::terminate_handler g_PreviousTerminate{};
    std::atomic_bool g_Installed{};
    std::atomic_flag g_Capturing = ATOMIC_FLAG_INIT;

#ifdef _WIN32
    LPTOP_LEVEL_EXCEPTION_FILTER g_PreviousExceptionFilter{};
#endif

    std::string TimestampForPath()
    {
        const auto now = std::chrono::system_clock::now();
        const auto raw = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &raw);
#else
        localtime_r(&raw, &tm);
#endif
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::ostringstream stream;
        stream << std::put_time(&tm, "%Y%m%d-%H%M%S")
               << '-' << std::setw(3) << std::setfill('0') << ms.count();
        return stream.str();
    }

#ifdef _WIN32
    void WriteMiniDump(
        const std::filesystem::path& path,
        EXCEPTION_POINTERS* exception) noexcept
    {
        const auto file = CreateFileW(
            path.wstring().c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (file == INVALID_HANDLE_VALUE)
            return;

        MINIDUMP_EXCEPTION_INFORMATION info{};
        info.ThreadId = GetCurrentThreadId();
        info.ExceptionPointers = exception;
        info.ClientPointers = FALSE;

        MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            file,
            MiniDumpWithThreadInfo,
            exception ? &info : nullptr,
            nullptr,
            nullptr);

        CloseHandle(file);
    }
#endif

    void CaptureImpl(
        std::string_view reason
#ifdef _WIN32
        , EXCEPTION_POINTERS* exception = nullptr
#endif
    ) noexcept
    {
        if (g_Capturing.test_and_set(std::memory_order_acq_rel))
            return;

        try
        {
            Sick::Core::Crash::Config config;
            {
                std::scoped_lock lock(g_CrashMutex);
                config = g_Config;
            }

            const auto folder = config.directory / ("crash-" + TimestampForPath());
            std::error_code ec;
            std::filesystem::create_directories(folder, ec);

            std::ofstream report(folder / "crash.txt", std::ios::out | std::ios::trunc);
            if (report.is_open())
            {
                const auto loggerStats = Sick::Core::Logging::Logger::Get().Stats();
                const auto recent = Sick::Core::Logging::Logger::Get().Recent(config.recentEvents);
                const auto stack = Sick::Core::Diagnostics::CaptureStack(1, 64);
                const auto metrics = Sick::Core::Metrics::GetSnapshot();

                report << "REAPER CRASH REPORT\n"
                       << "===================\n"
                       << "Reason: " << reason << '\n'
                       << "Thread: " << std::hash<std::thread::id>{}(std::this_thread::get_id()) << '\n'
                       << "Logger queued: " << loggerStats.enqueued << '\n'
                       << "Logger dispatched: " << loggerStats.dispatched << '\n'
                       << "Logger dropped: " << loggerStats.dropped << '\n'
                       << "Emergency dispatches: " << loggerStats.emergencyDispatches << '\n'
                       << "Anomaly detections: " << loggerStats.anomalyDetections << "\n\n"
                       << "Stack\n-----\n"
                       << Sick::Core::Diagnostics::FormatStack(stack) << "\n\n"
                       << "Metrics\n-------\n";

                for (const auto& [name, value] : metrics.counters)
                    report << "counter " << name << '=' << value << '\n';
                for (const auto& [name, value] : metrics.gauges)
                    report << "gauge " << name << '=' << value << '\n';
                for (const auto& [name, value] : metrics.distributions)
                {
                    report << "distribution " << name
                           << " count=" << value.count
                           << " total=" << value.total
                           << " min=" << value.minimum
                           << " max=" << value.maximum << '\n';
                }

                report << "\nRecent events\n-------------\n";
                for (const auto& record : recent)
                    report << Sick::Core::Logging::FormatText(record) << '\n';

                report.flush();
            }

#ifdef _WIN32
            if (config.writeMiniDump)
                WriteMiniDump(folder / "crash.dmp", exception);
#endif
        }
        catch (...)
        {
        }

        g_Capturing.clear(std::memory_order_release);
    }

    [[noreturn]] void TerminateHandler() noexcept
    {
#ifdef _WIN32
        CaptureImpl("std::terminate", nullptr);
#else
        CaptureImpl("std::terminate");
#endif
        Sick::Core::Logging::Logger::Get().Flush();

        if (g_PreviousTerminate && g_PreviousTerminate != &TerminateHandler)
            g_PreviousTerminate();

        std::abort();
    }

#ifdef _WIN32
    LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* exception)
    {
        CaptureImpl("Unhandled Windows exception", exception);
        Sick::Core::Logging::Logger::Get().Flush();
        return EXCEPTION_EXECUTE_HANDLER;
    }
#endif
}

namespace Sick::Core::Crash
{
    bool Install(Config config)
    {
        bool expected = false;
        if (!g_Installed.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
            return true;

        try
        {
            std::scoped_lock lock(g_CrashMutex);
            g_Config = std::move(config);
            std::error_code ec;
            std::filesystem::create_directories(g_Config.directory, ec);
            g_PreviousTerminate = std::set_terminate(&TerminateHandler);
#ifdef _WIN32
            g_PreviousExceptionFilter = SetUnhandledExceptionFilter(&ExceptionFilter);
#endif
            return true;
        }
        catch (...)
        {
            g_Installed.store(false, std::memory_order_release);
            return false;
        }
    }

    void Uninstall() noexcept
    {
        if (!g_Installed.exchange(false, std::memory_order_acq_rel))
            return;

        try
        {
            std::scoped_lock lock(g_CrashMutex);
            std::set_terminate(g_PreviousTerminate);
            g_PreviousTerminate = nullptr;
#ifdef _WIN32
            SetUnhandledExceptionFilter(g_PreviousExceptionFilter);
            g_PreviousExceptionFilter = nullptr;
#endif
        }
        catch (...)
        {
        }
    }

    void Capture(std::string_view reason) noexcept
    {
#ifdef _WIN32
        CaptureImpl(reason, nullptr);
#else
        CaptureImpl(reason);
#endif
    }

    bool Installed() noexcept
    {
        return g_Installed.load(std::memory_order_acquire);
    }
}
