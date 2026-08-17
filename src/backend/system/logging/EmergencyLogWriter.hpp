#pragma once

#include <atomic>
#include <filesystem>
#include <string_view>

namespace Sick::Backend::System::Logging
{
    class EmergencyLogWriter final
    {
    public:
        bool Initialize(const std::filesystem::path& path) noexcept;
        void Shutdown() noexcept;
        void Write(std::string_view message) noexcept;
        [[nodiscard]] bool Ready() const noexcept;

    private:
        void* m_Handle{};
        std::atomic_bool m_Ready{};
    };
}
