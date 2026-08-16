#pragma once

#include "BackendTypes.hpp"

#include <string_view>

namespace Sick::Backend
{
    class BackendApi final
    {
    public:
        static BackendApi& Get() noexcept;

        void SetGodMode(bool enabled) noexcept;
        [[nodiscard]] bool SaveProfile(std::string_view name);
        [[nodiscard]] bool LoadProfile(std::string_view name);
        [[nodiscard]] bool RunScriptVmTest();
        [[nodiscard]] BackendSnapshot Snapshot() const noexcept;

    private:
        BackendApi() = default;
    };
}
