#pragma once

#include "backend/BackendTypes.hpp"
#include "backend/tasking/ThreadPool.hpp"

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string_view>

namespace Sick::Backend::System
{
    // Performs config file I/O on the background pool. Loaded profiles are
    // handed back to BackendCore and applied from the game thread.
    class ConfigManager final
    {
    public:
        bool Initialize(
            Tasking::ThreadPool& pool,
            const std::filesystem::path& directory) noexcept;
        void Shutdown() noexcept;

        [[nodiscard]] bool Save(std::string_view name, FeatureProfile profile);
        [[nodiscard]] bool Load(std::string_view name);
        [[nodiscard]] std::optional<FeatureProfile> TakePendingProfile() noexcept;

        [[nodiscard]] static bool ValidName(std::string_view name) noexcept;

    private:
        mutable std::mutex m_Mutex;
        Tasking::ThreadPool* m_Pool{};
        std::filesystem::path m_Directory;
        std::optional<FeatureProfile> m_PendingProfile;
        std::uint64_t m_LoadGeneration{};
        bool m_Enabled{};
    };
}
