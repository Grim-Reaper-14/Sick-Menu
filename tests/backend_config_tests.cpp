#include "backend/BackendTypes.hpp"
#include "backend/system/ConfigManager.hpp"
#include "backend/tasking/ThreadPool.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <future>
#include <string>

namespace
{
    using Sick::Backend::FeatureProfile;
    using Sick::Backend::System::ConfigManager;
    using Sick::Backend::Tasking::ThreadPool;

    void WriteRaw(
        const std::filesystem::path& directory,
        std::string_view name,
        std::string_view contents)
    {
        std::ofstream output(directory / (std::string(name) + ".json"), std::ios::trunc);
        assert(output);
        output << contents;
    }
}

int main()
{
    const auto directory = std::filesystem::temp_directory_path() / "SickMenu-ConfigManager-Tests";
    std::error_code error;
    std::filesystem::remove_all(directory, error);
    std::filesystem::create_directories(directory, error);
    assert(!error);

    assert(ConfigManager::ValidName("default"));
    assert(ConfigManager::ValidName("player_1-test"));
    assert(!ConfigManager::ValidName(""));
    assert(!ConfigManager::ValidName("../escape"));
    assert(!ConfigManager::ValidName("folder/name"));
    assert(!ConfigManager::ValidName("profile.json"));
    assert(!ConfigManager::ValidName(std::string(65, 'a')));

    ThreadPool pool;
    assert(pool.Start(1, 32));

    ConfigManager configs;
    assert(configs.Initialize(pool, directory));
    assert(!configs.Save("../escape", {}));
    assert(!configs.Load("../escape"));

    FeatureProfile enabled{};
    enabled.player.godMode = true;
    assert(configs.Save("roundtrip", enabled));
    pool.Stop();

    assert(std::filesystem::exists(directory / "roundtrip.json"));
    assert(!std::filesystem::exists(directory / "roundtrip.json.tmp"));

    assert(pool.Start(1, 32));
    assert(configs.Load("roundtrip"));
    pool.Stop();
    auto loaded = configs.TakePendingProfile();
    assert(loaded.has_value());
    assert(loaded->version == FeatureProfile::CurrentVersion);
    assert(loaded->player.godMode);

    WriteRaw(directory, "broken", "{ not valid json");
    assert(pool.Start(1, 32));
    assert(configs.Load("broken"));
    pool.Stop();
    assert(!configs.TakePendingProfile().has_value());

    WriteRaw(directory, "future", R"({"version":999,"player":{"god_mode":true}})");
    assert(pool.Start(1, 32));
    assert(configs.Load("future"));
    pool.Stop();
    assert(!configs.TakePendingProfile().has_value());

    FeatureProfile first{};
    first.player.godMode = true;
    FeatureProfile second{};
    second.player.godMode = false;
    assert(pool.Start(1, 32));
    assert(configs.Save("first", first));
    assert(configs.Save("second", second));
    pool.Stop();

    assert(pool.Start(1, 32));
    std::promise<void> blockerReady;
    auto blockerReadyFuture = blockerReady.get_future();
    std::promise<void> releaseBlocker;
    auto releaseFuture = releaseBlocker.get_future().share();
    assert(pool.Submit([&blockerReady, releaseFuture]() mutable {
        blockerReady.set_value();
        releaseFuture.wait();
    }));
    blockerReadyFuture.wait();

    assert(configs.Load("first"));
    assert(configs.Load("second"));
    releaseBlocker.set_value();
    pool.Stop();

    const auto latest = configs.TakePendingProfile();
    assert(latest.has_value());
    assert(!latest->player.godMode);

    configs.Shutdown();
    std::filesystem::remove_all(directory, error);
    return 0;
}
