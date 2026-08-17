#include "game/enhanced/BuildManager.hpp"
#include "game/handling/HandlingBackend.hpp"

#include <cmath>
#include <cstddef>
#include <iostream>

namespace
{
    Sick::Handling::Values g_Values{};
    std::size_t g_Reads{};
    std::size_t g_Writes{};

    bool Check(bool condition, const char* expression, int line)
    {
        if (condition)
            return true;
        std::cerr << "check failed at line " << line << ": " << expression << '\n';
        return false;
    }

#define CHECK(expression) \
    do \
    { \
        if (!Check(static_cast<bool>(expression), #expression, __LINE__)) \
            return 1; \
    } while (false)

    bool NearlyEqual(float left, float right) noexcept
    {
        return std::fabs(left - right) < 0.0001F;
    }

    bool ReadHandling(Sick::Game::Vehicle vehicle, Sick::Handling::Values& values) noexcept
    {
        ++g_Reads;
        if (vehicle != 77)
            return false;
        values = g_Values;
        return true;
    }

    bool WriteHandling(Sick::Game::Vehicle vehicle, Sick::Handling::Field field, float value) noexcept
    {
        ++g_Writes;
        if (vehicle != 77 || Sick::Handling::ToIndex(field) >= Sick::Handling::FieldCount)
            return false;
        g_Values[Sick::Handling::ToIndex(field)] = value;
        return true;
    }
}

int main()
{
    using Sick::Game::Enhanced::BuildManager;
    using Sick::Game::Enhanced::UnknownBuild;
    using Sick::Game::Handling::HandlingBackend;

    for (std::size_t index = 0; index < Sick::Handling::FieldCount; ++index)
        g_Values[index] = static_cast<float>(index) + 0.25F;

    BuildManager::SetBuild(1234);
    auto& backend = HandlingBackend::Get();
    backend.Clear();
    backend.Configure({1234, &ReadHandling, &WriteHandling});
    CHECK(backend.Available());

    Sick::Handling::Values read{};
    CHECK(backend.Read(77, read));
    CHECK(g_Reads == 1);
    CHECK(NearlyEqual(read[3], g_Values[3]));

    CHECK(backend.Write(77, Sick::Handling::Field::Mass, 321.5F));
    CHECK(g_Writes == 1);
    CHECK(NearlyEqual(g_Values[Sick::Handling::ToIndex(Sick::Handling::Field::Mass)], 321.5F));

    BuildManager::SetBuild(4321);
    CHECK(!backend.Available());
    CHECK(!backend.Read(77, read));
    CHECK(!backend.Write(77, Sick::Handling::Field::Mass, 500.0F));
    CHECK(g_Reads == 1);
    CHECK(g_Writes == 1);

    backend.Clear();
    CHECK(!backend.Available());
    BuildManager::SetBuild(UnknownBuild);
    return 0;
}
