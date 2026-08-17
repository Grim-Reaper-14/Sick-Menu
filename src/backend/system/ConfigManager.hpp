#pragma once

#include "backend/BackendTypes.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string_view>

namespace Sick::Backend::System
{
    class FileSystem;
    class IoService;

    // Parses and writes profiles only on IoService workers. Loaded profiles are
    // handed back to BackendCore and applied from the game thread.
    class ConfigManager final
    {
    public:
        bool Initialize(IoService& io, FileSystem& files) noexcept;
        void Shutdown() noexcept;

        [[nodiscard]] bool Save(std::string_view name, FeatureProfile profile);
        [[nodiscard]] bool Load(std::string_view name);
        [[nodiscard]] std::optional<FeatureProfile> TakePendingProfile() noexcept;

        [[nodiscard]] static bool ValidName(std::string_view name) noexcept;

    private:
        mutable std::mutex m_Mutex;
        IoService* m_Io{};
        FileSystem* m_Files{};
        std::optional<FeatureProfile> m_PendingProfile;
        std::uint64_t m_LoadGeneration{};
        bool m_Enabled{};
    };
}
