#include "HandlingBackend.hpp"

namespace Sick::Game::Handling
{
    HandlingBackend& HandlingBackend::Get() noexcept
    {
        static HandlingBackend backend;
        return backend;
    }

    void HandlingBackend::Configure(Adapter adapter) noexcept
    {
        if (adapter.build == Enhanced::UnknownBuild || !adapter.read || !adapter.write)
            adapter = {};
        std::scoped_lock lock(m_Mutex);
        m_Adapter = adapter;
    }

    void HandlingBackend::Clear() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Adapter = {};
    }

    HandlingBackend::Adapter HandlingBackend::CurrentAdapter() const noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return m_Adapter;
    }

    bool HandlingBackend::Available() const noexcept
    {
        const auto adapter = CurrentAdapter();
        return adapter.build != Enhanced::UnknownBuild &&
            adapter.build == Enhanced::BuildManager::Current() &&
            adapter.read && adapter.write;
    }

    bool HandlingBackend::Read(Vehicle vehicle, Sick::Handling::Values& values) const noexcept
    {
        const auto adapter = CurrentAdapter();
        if (vehicle == 0 || adapter.build == Enhanced::UnknownBuild ||
            adapter.build != Enhanced::BuildManager::Current() || !adapter.read)
            return false;
        return adapter.read(vehicle, values);
    }

    bool HandlingBackend::Write(Vehicle vehicle, Sick::Handling::Field field, float value) const noexcept
    {
        const auto adapter = CurrentAdapter();
        if (vehicle == 0 || Sick::Handling::ToIndex(field) >= Sick::Handling::FieldCount ||
            adapter.build == Enhanced::UnknownBuild ||
            adapter.build != Enhanced::BuildManager::Current() || !adapter.write)
            return false;
        return adapter.write(vehicle, field, value);
    }
}
