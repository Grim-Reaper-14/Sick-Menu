#include "BuildManager.hpp"

namespace Sick::Game::Enhanced
{
    void BuildManager::SetBuild(BuildId build) noexcept
    {
        s_Build = build;
    }

    BuildId BuildManager::Current() noexcept
    {
        return s_Build;
    }

    bool BuildManager::Supported() noexcept
    {
        return s_Build != UnknownBuild;
    }

    std::string_view BuildManager::EditionName() noexcept
    {
        return "Grand Theft Auto V Enhanced";
    }
}
