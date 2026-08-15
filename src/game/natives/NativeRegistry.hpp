#pragma once

#include "NativeMetadata.hpp"

#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Sick::Game::Natives
{
    class NativeRegistry final
    {
    public:
        static NativeRegistry& Get() noexcept;

        void Register(NativeMetadata metadata);
        void RegisterDefaults();
        void Clear();

        [[nodiscard]] std::optional<NativeMetadata> Find(NativeHash hash) const;
        [[nodiscard]] std::optional<NativeMetadata> Find(std::string_view name) const;
        [[nodiscard]] std::vector<NativeMetadata> All() const;
        [[nodiscard]] std::size_t Size() const noexcept;

    private:
        NativeRegistry() = default;

        mutable std::shared_mutex m_Mutex;
        std::unordered_map<NativeHash, NativeMetadata> m_ByHash;
        std::unordered_map<std::string, NativeHash> m_ByName;
    };
}
