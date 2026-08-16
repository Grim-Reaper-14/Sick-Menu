#pragma once

#include "Module.hpp"

#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Sick::Memory
{
    class ModuleManager final
    {
    public:
        bool Register(Module module);
        bool Load(std::string_view name = {});

        [[nodiscard]] std::optional<Module> Find(std::string_view name) const;
        [[nodiscard]] std::size_t Count() const noexcept;

        void Clear() noexcept;

    private:
        [[nodiscard]] static std::string Normalize(std::string_view name);

        mutable std::shared_mutex m_Mutex;
        std::unordered_map<std::string, Module> m_Modules;
    };
}
