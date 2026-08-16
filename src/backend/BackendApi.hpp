#pragma once

#include "BackendTypes.hpp"

namespace Sick::Backend
{
    class BackendApi final
    {
    public:
        static BackendApi& Get() noexcept;

        void SetGodMode(bool enabled) noexcept;
        [[nodiscard]] bool RunScriptVmTest();
        [[nodiscard]] BackendSnapshot Snapshot() const noexcept;

    private:
        BackendApi() = default;
    };
}
