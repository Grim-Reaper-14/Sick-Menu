#pragma once

#include "backend/BackendTypes.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>

namespace Sick::Backend::System
{
    class FileSystem;
    class IoService;

    class AssetCatalog final
    {
    public:
        bool Initialize(IoService& io, FileSystem& files) noexcept;
        void Shutdown() noexcept;
        [[nodiscard]] bool Refresh();
        [[nodiscard]] AssetCatalogSnapshot Snapshot() const;
        [[nodiscard]] std::uint64_t Generation() const noexcept;

    private:
        mutable std::mutex m_Mutex;
        IoService* m_Io{};
        FileSystem* m_Files{};
        AssetCatalogSnapshot m_Snapshot;
        std::uint64_t m_RefreshGeneration{};
        std::atomic_uint64_t m_PublishedGeneration{};
        bool m_Enabled{};
    };
}
