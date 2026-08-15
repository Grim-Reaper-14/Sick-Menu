#include "BuildManager.hpp"

namespace Sick::Game::Enhanced
{
    void BuildManager::SetBuild(BuildId build) noexcept
    {
        s_Build.store(build, std::memory_order_relaxed);
    }

    BuildId BuildManager::Current() noexcept
    {
        return s_Build.load(std::memory_order_relaxed);
    }

    bool BuildManager::Supported() noexcept
    {
        return Current() != UnknownBuild;
    }

    std::string_view BuildManager::EditionName() noexcept
    {
        return "Grand Theft Auto V Enhanced";
    }
}
