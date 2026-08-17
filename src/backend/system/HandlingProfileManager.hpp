#pragma once

#include "backend/BackendTypes.hpp"
#include "shared/HandlingTypes.hpp"

#include <atomic>
#include <mutex>
#include <optional>
#include <string_view>
#include <vector>

namespace Sick::Backend::System
{
    class FileSystem;
    class IoService;

    class HandlingProfileManager final
    {
    public:
        bool Initialize(IoService& io, FileSystem& files) noexcept;
        void Shutdown() noexcept;

        [[nodiscard]] bool Save(const Handling::Values& values);
        [[nodiscard]] bool Load(std::string_view name);
        [[nodiscard]] bool Refresh();
        [[nodiscard]] std::optional<Handling::Values> TakePendingValues() noexcept;
        [[nodiscard]] HandlingProfileCatalogSnapshot Snapshot() const;
        [[nodiscard]] std::uint64_t Generation() const noexcept;
        [[nodiscard]] static bool ValidName(std::string_view name) noexcept;

    private:
        mutable std::mutex m_Mutex;
        IoService* m_Io{};
        FileSystem* m_Files{};
        std::optional<Handling::Values> m_PendingValues;
        std::vector<std::string> m_Profiles;
        std::uint64_t m_LoadGeneration{};
        std::uint64_t m_RefreshGeneration{};
        std::atomic_uint64_t m_PublishedGeneration{};
        std::atomic_uint64_t m_SaveSequence{};
        bool m_Enabled{};
    };
}
