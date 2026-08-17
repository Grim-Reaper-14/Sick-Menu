#include "backend/system/FileSystem.hpp"
#include "backend/system/IoService.hpp"
#include "backend/system/LoggerApi.hpp"
#include "backend/tasking/TaskAffinity.hpp"

#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

namespace
{
    using namespace Sick::Backend;
    using namespace Sick::Backend::System;
    using namespace Sick::Backend::Tasking;

    bool Check(bool condition, const char* expression, int line)
    {
        if (condition)
            return true;
        std::cerr << "check failed at line " << line << ": " << expression << '\n';
        return false;
    }

#define CHECK(expression) \
    do \
    { \
        if (!Check(static_cast<bool>(expression), #expression, __LINE__)) \
            return 1; \
    } while (false)
}

int main()
{
    const auto base = std::filesystem::temp_directory_path() / "SickMenu-BackgroundCore-Tests";
    std::error_code error;
    std::filesystem::remove_all(base, error);

    FileSystem files;
    CHECK(files.Initialize(base));
    CHECK(!files.Resolve(FileArea::Configs, "../escape.json").has_value());
#ifdef _WIN32
    CHECK(!files.Resolve(FileArea::Configs, "C:\\outside.json").has_value());
#endif

    {
        ScopedTaskAffinity affinity{TaskAffinity::Game};
        CHECK(!files.AtomicWriteText(FileArea::Cache, "game-thread.txt", "blocked"));
    }
    {
        ScopedTaskAffinity affinity{TaskAffinity::Background};
        CHECK(files.AtomicWriteText(FileArea::Cache, "worker.txt", "background-data"));
        const auto contents = files.ReadText(FileArea::Cache, "worker.txt");
        CHECK(contents.has_value());
        CHECK(*contents == "background-data");
    }
    const auto fileStats = files.Snapshot();
    CHECK(fileStats.writes >= 1);
    CHECK(fileStats.reads >= 1);
    CHECK(fileStats.rejectedPaths >= 1);
    CHECK(fileStats.rejectedAffinity >= 1);

    IoService io;
    CHECK(io.Start(1, 32));
    std::promise<void> blockerReady;
    auto blockerReadyFuture = blockerReady.get_future();
    std::promise<void> releaseBlocker;
    auto releaseFuture = releaseBlocker.get_future().share();
    const auto blocker = io.Submit(IoPriority::Critical, [&blockerReady, releaseFuture]() mutable {
        blockerReady.set_value();
        releaseFuture.wait();
    });
    if (!Check(blocker.has_value(), "blocker.has_value()", __LINE__) ||
        blockerReadyFuture.wait_for(std::chrono::seconds(2)) != std::future_status::ready)
    {
        releaseBlocker.set_value();
        io.Stop();
        return 1;
    }

    std::mutex orderMutex;
    std::vector<int> order;
    const auto maintenance = io.Submit(IoPriority::Maintenance, [&]() {
        std::scoped_lock lock(orderMutex);
        order.push_back(3);
    });
    const auto normal = io.Submit(IoPriority::Normal, [&]() {
        std::scoped_lock lock(orderMutex);
        order.push_back(2);
    });
    const auto critical = io.Submit(IoPriority::Critical, [&]() {
        std::scoped_lock lock(orderMutex);
        order.push_back(1);
    });
    releaseBlocker.set_value();
    io.Stop();
    CHECK(maintenance.has_value());
    CHECK(normal.has_value());
    CHECK(critical.has_value());
    CHECK(order.size() == 3);
    CHECK(order[0] == 1 && order[1] == 2 && order[2] == 3);
    const auto ioStats = io.Snapshot();
    CHECK(ioStats.accepted >= 4);
    CHECK(ioStats.completed >= 4);
    CHECK(ioStats.peakPending >= 3);

    auto& logger = LoggerApi::Get();
    CHECK(logger.Initialize(files));
    {
        ScopedTaskAffinity affinity{TaskAffinity::Game};
        logger.Info("test", "game-thread enqueue is allowed", {{"phase", "4"}});
    }
    logger.Warn("test", "structured warning", {{"kind", "test"}});
    logger.Flush();
    const auto logStats = logger.Snapshot();
    CHECK(logStats.accepted >= 2);
    CHECK(logStats.written >= 2);
    CHECK(logStats.pending == 0);
    const auto recent = logger.Recent(10);
    CHECK(recent.size() >= 2);
    logger.Emergency("background system emergency test");
    logger.Shutdown();

    CHECK(files.Exists(FileArea::Logs, "SickMenu.log"));
    CHECK(files.Exists(FileArea::Logs, "SickMenu.jsonl"));
    CHECK(files.Exists(FileArea::Crashes, "emergency.log"));

    std::filesystem::remove_all(base, error);
    return 0;
}
