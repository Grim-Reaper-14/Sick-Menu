#include "NativeCrossmap.hpp"

#include <mutex>
#include <utility>

namespace Sick::Game::Enhanced
{
    NativeCrossmap& NativeCrossmap::Get() noexcept
    {
        static NativeCrossmap crossmap;
        return crossmap;
    }

    bool NativeCrossmap::Register(BuildId build, NativeHash original, NativeHash mapped)
    {
        if (build == UnknownBuild || original == 0 || mapped == 0)
            return false;

        std::unique_lock lock(m_Mutex);
        auto& map = m_Builds[build];

        if (const auto reverseIt = map.reverse.find(mapped);
            reverseIt != map.reverse.end() && reverseIt->second != original)
            return false;

        if (const auto forwardIt = map.forward.find(original); forwardIt != map.forward.end())
        {
            if (forwardIt->second == mapped)
                return true;

            map.reverse.erase(forwardIt->second);
        }

        map.forward[original] = mapped;
        map.reverse[mapped] = original;
        return true;
    }

    bool NativeCrossmap::Load(BuildId build, std::span<const NativeCrossmapEntry> entries)
    {
        if (build == UnknownBuild)
            return false;

        BuildCrossmap replacement;
        replacement.forward.reserve(entries.size());
        replacement.reverse.reserve(entries.size());

        for (const auto& entry : entries)
        {
            if (entry.original == 0 || entry.mapped == 0)
                return false;

            if (!replacement.forward.emplace(entry.original, entry.mapped).second)
                return false;

            if (!replacement.reverse.emplace(entry.mapped, entry.original).second)
                return false;
        }

        std::unique_lock lock(m_Mutex);
        m_Builds[build] = std::move(replacement);
        return true;
    }

    bool NativeCrossmap::Remove(BuildId build, NativeHash original) noexcept
    {
        std::unique_lock lock(m_Mutex);
        const auto buildIt = m_Builds.find(build);
        if (buildIt == m_Builds.end())
            return false;

        auto& map = buildIt->second;
        const auto forwardIt = map.forward.find(original);
        if (forwardIt == map.forward.end())
            return false;

        map.reverse.erase(forwardIt->second);
        map.forward.erase(forwardIt);

        if (map.forward.empty())
            m_Builds.erase(buildIt);

        return true;
    }

    void NativeCrossmap::Clear() noexcept
    {
        std::unique_lock lock(m_Mutex);
        m_Builds.clear();
    }

    void NativeCrossmap::Clear(BuildId build) noexcept
    {
        std::unique_lock lock(m_Mutex);
        m_Builds.erase(build);
    }

    NativeHash NativeCrossmap::Translate(NativeHash original, BuildId build) const noexcept
    {
        std::shared_lock lock(m_Mutex);
        const auto buildIt = m_Builds.find(build);
        if (buildIt == m_Builds.end())
            return original;

        const auto hashIt = buildIt->second.forward.find(original);
        return hashIt != buildIt->second.forward.end() ? hashIt->second : original;
    }

    std::optional<NativeHash> NativeCrossmap::Reverse(NativeHash mapped, BuildId build) const noexcept
    {
        std::shared_lock lock(m_Mutex);
        const auto buildIt = m_Builds.find(build);
        if (buildIt == m_Builds.end())
            return std::nullopt;

        const auto hashIt = buildIt->second.reverse.find(mapped);
        if (hashIt == buildIt->second.reverse.end())
            return std::nullopt;

        return hashIt->second;
    }

    bool NativeCrossmap::Contains(BuildId build, NativeHash original) const noexcept
    {
        std::shared_lock lock(m_Mutex);
        const auto buildIt = m_Builds.find(build);
        return buildIt != m_Builds.end() && buildIt->second.forward.contains(original);
    }

    std::size_t NativeCrossmap::Size(BuildId build) const noexcept
    {
        std::shared_lock lock(m_Mutex);
        const auto buildIt = m_Builds.find(build);
        return buildIt != m_Builds.end() ? buildIt->second.forward.size() : 0;
    }
}
