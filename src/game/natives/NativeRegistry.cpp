#include "NativeRegistry.hpp"
#include "generated/NativeCatalog.hpp"

#include <mutex>
#include <utility>

namespace Sick::Game::Natives
{
    NativeRegistry& NativeRegistry::Get() noexcept
    {
        static NativeRegistry registry;
        return registry;
    }

    void NativeRegistry::Register(NativeMetadata metadata)
    {
        std::unique_lock lock(m_Mutex);
        m_ByName[metadata.name] = metadata.hash;
        m_ByHash[metadata.hash] = std::move(metadata);
    }

    void NativeRegistry::RegisterDefaults()
    {
        Clear();

        for (const auto& definition : Generated::NativeCatalog)
        {
            Register({
                definition.hash,
                std::string{definition.name},
                std::string{definition.nameSpace},
                definition.argumentCount,
                definition.vectorFixup,
            });
        }
    }

    void NativeRegistry::Clear()
    {
        std::unique_lock lock(m_Mutex);
        m_ByHash.clear();
        m_ByName.clear();
    }

    std::optional<NativeMetadata> NativeRegistry::Find(NativeHash hash) const
    {
        std::shared_lock lock(m_Mutex);
        const auto it = m_ByHash.find(hash);
        if (it == m_ByHash.end())
            return std::nullopt;

        return it->second;
    }

    std::optional<NativeMetadata> NativeRegistry::Find(std::string_view name) const
    {
        std::shared_lock lock(m_Mutex);
        const auto nameIt = m_ByName.find(std::string{name});
        if (nameIt == m_ByName.end())
            return std::nullopt;

        const auto hashIt = m_ByHash.find(nameIt->second);
        if (hashIt == m_ByHash.end())
            return std::nullopt;

        return hashIt->second;
    }

    std::vector<NativeMetadata> NativeRegistry::All() const
    {
        std::shared_lock lock(m_Mutex);
        std::vector<NativeMetadata> result;
        result.reserve(m_ByHash.size());

        for (const auto& [_, metadata] : m_ByHash)
            result.push_back(metadata);

        return result;
    }

    std::size_t NativeRegistry::Size() const noexcept
    {
        std::shared_lock lock(m_Mutex);
        return m_ByHash.size();
    }
}
