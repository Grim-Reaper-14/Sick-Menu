#include "backend/BackendTypes.hpp"
#include "backend/system/ConfigManager.hpp"
#include "backend/system/FileSystem.hpp"
#include "backend/system/IoService.hpp"

#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <string>

namespace
{
    using Sick::Backend::FeatureProfile;
    using Sick::Backend::System::ConfigManager;
    using Sick::Backend::System::FileArea;
    using Sick::Backend::System::FileSystem;
    using Sick::Backend::System::IoPriority;
    using Sick::Backend::System::IoService;

    bool Check(bool condition, const char* expression, int line)
    {
        if (condition) return true;
        std::cerr << "check failed at line " << line << ": " << expression << '\n';
        return false;
    }

#define CHECK(expression) do { if (!Check(static_cast<bool>(expression), #expression, __LINE__)) return 1; } while (false)

    bool HasTemporaryFile(FileSystem& files)
    {
        for (const auto& entry : files.List(FileArea::Configs))
        {
            if (entry.relativePath.filename().string().find(".tmp.") != std::string::npos)
                return true;
        }
        return false;
    }
}

int main()
{
    const auto base = std::filesystem::temp_directory_path() / "SickMenu-ConfigManager-Tests";
    std::error_code error;
    std::filesystem::remove_all(base, error);

    FileSystem files;
    CHECK(files.Initialize(base));
    CHECK(ConfigManager::ValidName("default"));
    CHECK(ConfigManager::ValidName("player_1-test"));
    CHECK(!ConfigManager::ValidName(""));
    CHECK(!ConfigManager::ValidName("../escape"));
    CHECK(!ConfigManager::ValidName("folder/name"));
    CHECK(!ConfigManager::ValidName("profile.json"));
    CHECK(!ConfigManager::ValidName(std::string(65, 'a')));

    IoService io;
    CHECK(io.Start(1, 32));
    ConfigManager configs;
    CHECK(configs.Initialize(io, files));
    CHECK(!configs.Save("../escape", {}));
    CHECK(!configs.Load("../escape"));

    FeatureProfile enabled{};
    enabled.player.godMode = true;
    enabled.player.infiniteOxygen = true;
    enabled.player.noRagdoll = true;
    enabled.player.superJump = true;
    enabled.player.seatBelt = true;
    enabled.player.noWantedLevel = true;
    enabled.player.wantedLevel = 4;
    enabled.player.fastRun = true;
    enabled.player.fastSwim = true;
    enabled.player.keepPlayerClean = true;
    enabled.player.aqualung = true;
    enabled.player.noGravity = true;
    enabled.player.waterproof = true;
    enabled.vehicle.godMode = true;
    enabled.vehicle.autoRepair = true;
    enabled.vehicle.keepClean = true;
    enabled.vehicle.engineAlwaysOn = true;
    enabled.vehicle.noGravity = true;
    enabled.vehicle.noCollision = true;
    CHECK(configs.Save("roundtrip", enabled));
    io.Stop();

    CHECK(files.Exists(FileArea::Configs, "roundtrip.json"));
    CHECK(!HasTemporaryFile(files));

    CHECK(io.Start(1, 32));
    CHECK(configs.Load("roundtrip"));
    io.Stop();
    auto loaded = configs.TakePendingProfile();
    CHECK(loaded.has_value());
    CHECK(loaded->version == FeatureProfile::CurrentVersion);
    CHECK(loaded->player.godMode);
    CHECK(loaded->player.infiniteOxygen);
    CHECK(loaded->player.noRagdoll);
    CHECK(loaded->player.superJump);
    CHECK(loaded->player.seatBelt);
    CHECK(loaded->player.noWantedLevel);
    CHECK(loaded->player.wantedLevel == 4);
    CHECK(loaded->player.fastRun);
    CHECK(loaded->player.fastSwim);
    CHECK(loaded->player.keepPlayerClean);
    CHECK(loaded->player.aqualung);
    CHECK(loaded->player.noGravity);
    CHECK(loaded->player.waterproof);
    CHECK(loaded->vehicle.godMode);
    CHECK(loaded->vehicle.autoRepair);
    CHECK(loaded->vehicle.keepClean);
    CHECK(loaded->vehicle.engineAlwaysOn);
    CHECK(loaded->vehicle.noGravity);
    CHECK(loaded->vehicle.noCollision);

    CHECK(files.AtomicWriteText(FileArea::Configs, "legacy.json", R"({"version":1,"player":{"god_mode":true}})"));
    CHECK(io.Start(1, 32));
    CHECK(configs.Load("legacy"));
    io.Stop();
    auto legacy = configs.TakePendingProfile();
    CHECK(legacy.has_value());
    CHECK(legacy->version == FeatureProfile::CurrentVersion);
    CHECK(legacy->player.godMode);
    CHECK(!legacy->player.superJump);
    CHECK(!legacy->vehicle.godMode);

    CHECK(files.AtomicWriteText(FileArea::Configs, "v2.json", R"({"version":2,"player":{"god_mode":true,"wanted_level":3,"super_jump":true}})"));
    CHECK(io.Start(1, 32));
    CHECK(configs.Load("v2"));
    io.Stop();
    auto v2 = configs.TakePendingProfile();
    CHECK(v2.has_value());
    CHECK(v2->version == FeatureProfile::CurrentVersion);
    CHECK(v2->player.godMode);
    CHECK(v2->player.superJump);
    CHECK(v2->player.wantedLevel == 3);
    CHECK(!v2->vehicle.autoRepair);

    CHECK(files.AtomicWriteText(FileArea::Configs, "broken.json", "{ not valid json"));
    CHECK(io.Start(1, 32));
    CHECK(configs.Load("broken"));
    io.Stop();
    CHECK(!configs.TakePendingProfile().has_value());

    CHECK(files.AtomicWriteText(FileArea::Configs, "future.json", R"({"version":999,"player":{"god_mode":true}})"));
    CHECK(io.Start(1, 32));
    CHECK(configs.Load("future"));
    io.Stop();
    CHECK(!configs.TakePendingProfile().has_value());

    FeatureProfile first{};
    first.player.godMode = true;
    FeatureProfile second{};
    second.player.godMode = false;
    CHECK(io.Start(1, 32));
    CHECK(configs.Save("first", first));
    CHECK(configs.Save("second", second));
    io.Stop();

    CHECK(io.Start(1, 32));
    std::promise<void> blockerReady;
    auto blockerReadyFuture = blockerReady.get_future();
    std::promise<void> releaseBlocker;
    auto releaseFuture = releaseBlocker.get_future().share();
    const auto blocker = io.Submit(IoPriority::Critical, [&blockerReady, releaseFuture]() mutable {
        blockerReady.set_value();
        releaseFuture.wait();
    });
    if (!Check(blocker.has_value(), "blocker.has_value()", __LINE__) || blockerReadyFuture.wait_for(std::chrono::seconds(2)) != std::future_status::ready)
    {
        releaseBlocker.set_value();
        io.Stop();
        return 1;
    }

    const bool firstQueued = configs.Load("first");
    const bool secondQueued = configs.Load("second");
    releaseBlocker.set_value();
    io.Stop();
    CHECK(firstQueued);
    CHECK(secondQueued);
    const auto latest = configs.TakePendingProfile();
    CHECK(latest.has_value());
    CHECK(!latest->player.godMode);

    configs.Shutdown();
    std::filesystem::remove_all(base, error);
    return 0;
}
