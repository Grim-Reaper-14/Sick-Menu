#pragma once

#include "game/enhanced/BuildManager.hpp"
#include "game/natives/NativeTypes.hpp"
#include "shared/HandlingTypes.hpp"

#include <mutex>

namespace Sick::Game::Handling
{
    class HandlingBackend final
    {
    public:
        using ReadCallback = bool (*)(Vehicle vehicle, Sick::Handling::Values& values) noexcept;
        using WriteCallback = bool (*)(Vehicle vehicle, Sick::Handling::Field field, float value) noexcept;

        struct Adapter
        {
            Enhanced::BuildId build{Enhanced::UnknownBuild};
            ReadCallback read{};
            WriteCallback write{};
        };

        static HandlingBackend& Get() noexcept;

        // Runtime code may register an adapter only after its Enhanced build and
        // CHandlingData layout have been validated. No guessed offsets live here.
        void Configure(Adapter adapter) noexcept;
        void Clear() noexcept;

        [[nodiscard]] bool Available() const noexcept;
        [[nodiscard]] bool Read(Vehicle vehicle, Sick::Handling::Values& values) const noexcept;
        [[nodiscard]] bool Write(Vehicle vehicle, Sick::Handling::Field field, float value) const noexcept;

    private:
        HandlingBackend() = default;
        [[nodiscard]] Adapter CurrentAdapter() const noexcept;

        mutable std::mutex m_Mutex;
        Adapter m_Adapter{};
    };
}
