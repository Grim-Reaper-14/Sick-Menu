#include "backend/system/FileSystem.hpp"
#include "backend/system/HandlingProfileManager.hpp"
#include "backend/system/IoService.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

namespace
{
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

    bool NearlyEqual(float left, float right) noexcept
    {
        return std::fabs(left - right) < 0.0001F;
    }
}

int main()
{
    using Sick::Backend::System::FileArea;
    using Sick::Backend::System::FileSystem;
    using Sick::Backend::System::HandlingProfileManager;
    using Sick::Backend::System::IoService;

    const auto base = std::filesystem::temp_directory_path() / "SickMenu-HandlingProfile-Tests";
    std::error_code error;
    std::filesystem::remove_all(base, error);

    FileSystem files;
    CHECK(files.Initialize(base));
    CHECK(HandlingProfileManager::ValidName("race setup"));
    CHECK(HandlingProfileManager::ValidName("drift-1_test"));
    CHECK(!HandlingProfileManager::ValidName(""));
    CHECK(!HandlingProfileManager::ValidName("../escape"));
    CHECK(!HandlingProfileManager::ValidName("folder/name"));

    IoService io;
    CHECK(io.Start(1, 64));
    HandlingProfileManager profiles;
    CHECK(profiles.Initialize(io, files));
    io.Stop();

    Sick::Handling::Values values{};
    for (std::size_t index = 0; index < Sick::Handling::FieldCount; ++index)
    {
        const auto& spec = Sick::Handling::FieldSpecs[index];
        auto value = std::min(spec.maximum, spec.minimum + std::max(spec.step * 3.0F, 0.2F));
        if (spec.integral)
            value = std::round(value);
        values[index] = value;
    }

    CHECK(io.Start(1, 64));
    CHECK(profiles.Save(values));
    io.Stop();

    const auto catalog = profiles.Snapshot();
    CHECK(catalog.generation > 0);
    CHECK(catalog.names.size() == 1);
    const auto savedName = catalog.names.front();
    CHECK(HandlingProfileManager::ValidName(savedName));
    CHECK(files.Exists(FileArea::Configs, std::filesystem::path{"handling"} / (savedName + ".json")));

    CHECK(io.Start(1, 64));
    CHECK(profiles.Load(savedName));
    io.Stop();
    const auto loaded = profiles.TakePendingValues();
    CHECK(loaded.has_value());
    for (std::size_t index = 0; index < Sick::Handling::FieldCount; ++index)
        CHECK(NearlyEqual((*loaded)[index], values[index]));

    CHECK(files.AtomicWriteText(
        FileArea::Configs,
        std::filesystem::path{"handling"} / "broken.json",
        "{ broken json"));
    CHECK(io.Start(1, 64));
    CHECK(profiles.Load("broken"));
    io.Stop();
    CHECK(!profiles.TakePendingValues().has_value());

    CHECK(io.Start(1, 64));
    CHECK(profiles.Refresh());
    io.Stop();
    const auto refreshed = profiles.Snapshot();
    CHECK(refreshed.names.size() == 2);
    CHECK(std::ranges::find(refreshed.names, savedName) != refreshed.names.end());
    CHECK(std::ranges::find(refreshed.names, std::string{"broken"}) != refreshed.names.end());

    profiles.Shutdown();
    std::filesystem::remove_all(base, error);
    return 0;
}
