#include "ModuleManager.hpp"

#include <algorithm>
#include <cctype>
#include <mutex>
#include <utility>

namespace Sick::Memory
{
    bool ModuleManager::Register(Module module)
    {
        if (!module.Valid())
            return false;

        auto key = Normalize(module.Name());
        if (key.empty())
            return false;

        std::unique_lock lock(m_Mutex);
        m_Modules.insert_or_assign(std::move(key), std::move(module));
        return true;
    }

    bool ModuleManager::Load(std::string_view name)
    {
        auto module = Module::FromLoaded(name);
        if (!module)
            return false;

        return Register(std::move(*module));
    }

    std::optional<Module> ModuleManager::Find(std::string_view name) const
    {
        const auto key = Normalize(name.empty() ? MainModuleName : name);

        std::shared_lock lock(m_Mutex);
        const auto it = m_Modules.find(key);
        if (it == m_Modules.end())
            return std::nullopt;

        return it->second;
    }

    std::size_t ModuleManager::Count() const noexcept
    {
        std::shared_lock lock(m_Mutex);
        return m_Modules.size();
    }

    void ModuleManager::Clear() noexcept
    {
        std::unique_lock lock(m_Mutex);
        m_Modules.clear();
    }

    std::string ModuleManager::Normalize(std::string_view name)
    {
        std::string result{name};
        std::ranges::transform(result, result.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return result;
    }
}
