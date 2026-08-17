#include "Reaper.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <type_traits>

namespace
{
    struct TestProgram
    {
        std::array<std::uint8_t, 64> code{};
    };

    struct Vector3
    {
        float x{};
        float y{};
        float z{};
    };

    static_assert(std::is_trivially_copyable_v<Vector3>);

    TestProgram g_Program;
    constexpr auto g_Script = Reaper::Joaat("freemode");
    std::size_t g_ProgramResolves{};
    std::size_t g_CodeReads{};
    std::size_t g_Invocations{};
    Reaper::ScriptHash g_LastScript{};
    std::uint32_t g_LastProgramCounter{};
    std::array<std::uint64_t, 8> g_LastArguments{};
    std::size_t g_LastArgumentCount{};
    std::array<std::byte, 32> g_ReturnBytes{};
    std::size_t g_ReturnSize{};
    bool g_InvokeSucceeds{true};

    const std::uint8_t* CodeAddress(const void* program, std::uint32_t index) noexcept
    {
        ++g_CodeReads;
        const auto* testProgram = static_cast<const TestProgram*>(program);
        return index < testProgram->code.size() ? &testProgram->code[index] : nullptr;
    }

    Reaper::ScriptProgramView ResolveProgram(Reaper::ScriptHash script) noexcept
    {
        ++g_ProgramResolves;
        if (script != g_Script)
            return {};

        return Reaper::ScriptProgramView{
            &g_Program,
            static_cast<std::uint32_t>(g_Program.code.size()),
            &CodeAddress};
    }

    bool InvokeScript(
        Reaper::ScriptHash script,
        std::uint32_t programCounter,
        const std::uint64_t* arguments,
        std::size_t argumentCount,
        void* returnValue,
        std::size_t returnSize) noexcept
    {
        ++g_Invocations;
        g_LastScript = script;
        g_LastProgramCounter = programCounter;
        g_LastArgumentCount = argumentCount;
        g_LastArguments.fill(0);

        if (!g_InvokeSucceeds || argumentCount > g_LastArguments.size())
            return false;

        if (argumentCount != 0)
            std::memcpy(g_LastArguments.data(), arguments, argumentCount * sizeof(std::uint64_t));

        if (returnSize != 0)
        {
            if (!returnValue || returnSize != g_ReturnSize)
                return false;

            std::memcpy(returnValue, g_ReturnBytes.data(), returnSize);
        }

        return true;
    }

    template <typename T>
    void SetReturn(const T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(sizeof(T) <= g_ReturnBytes.size());
        g_ReturnBytes.fill(std::byte{});
        std::memcpy(g_ReturnBytes.data(), &value, sizeof(value));
        g_ReturnSize = sizeof(value);
    }

    template <typename T>
    T Unpack(std::uint64_t slot)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(sizeof(T) <= sizeof(slot));
        T value{};
        std::memcpy(&value, &slot, sizeof(value));
        return value;
    }

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
            return false; \
    } while (false)

    bool RunTests()
    {
        CHECK(g_Script != 0);
        CHECK(Reaper::Joaat("FREEMODE") == g_Script);

        g_Program.code.fill(0xCC);

        // Direct function entry at bytecode offset 4.
        constexpr std::array directPattern{0x2Du, 0x01u, 0x09u, 0x00u, 0x00u};
        for (std::size_t index = 0; index < directPattern.size(); ++index)
            g_Program.code[4 + index] = static_cast<std::uint8_t>(directPattern[index]);

        // Yim-style call opcode whose three-byte operand points to offset 32.
        g_Program.code[12] = 0x5D;
        g_Program.code[13] = 0x20;
        g_Program.code[14] = 0x00;
        g_Program.code[15] = 0x00;
        g_Program.code[16] = 0x38;

        CHECK(!Reaper::ScriptPointer("invalid", "GG").Valid());
        CHECK(Reaper::Enhanced::Game::BindScriptRuntime(&ResolveProgram, &InvokeScript));
        CHECK(Reaper::Enhanced::Game::ScriptFunctionsReady());

        Reaper::ScriptFunction direct{
            g_Script,
            Reaper::ScriptPointer{"Direct", "2D 01 ? 00 00"}};

        SetReturn<std::int32_t>(77);
        const auto first = direct.TryCall<std::int32_t>(-2, 1.5f, true);
        CHECK(first && *first == 77);
        CHECK(direct.Resolved());
        CHECK(direct.ProgramCounter() && *direct.ProgramCounter() == 4);
        CHECK(g_LastScript == g_Script);
        CHECK(g_LastProgramCounter == 4);
        CHECK(g_LastArgumentCount == 3);
        CHECK(Unpack<std::int32_t>(g_LastArguments[0]) == -2);
        CHECK(Unpack<float>(g_LastArguments[1]) == 1.5f);
        CHECK(Unpack<bool>(g_LastArguments[2]));

        const auto resolvesAfterFirstCall = g_ProgramResolves;
        const auto readsAfterFirstCall = g_CodeReads;
        CHECK(direct.Call<std::int32_t>() == 77);
        CHECK(g_ProgramResolves == resolvesAfterFirstCall);
        CHECK(g_CodeReads == readsAfterFirstCall);

        Reaper::ScriptFunction ripped{
            g_Script,
            Reaper::ScriptPointer{"Ripped", "5D ? ? ? 38"}.Add(1).Rip()};

        const Vector3 expected{1.0f, 2.0f, 3.0f};
        SetReturn(expected);
        const auto vector = ripped.TryCall<Vector3>(42);
        CHECK(vector.has_value());
        CHECK(vector->x == expected.x && vector->y == expected.y && vector->z == expected.z);
        CHECK(g_LastProgramCounter == 32);

        SetReturn<std::int32_t>(123);
        CHECK(ripped.TryCallVoid());
        CHECK(g_LastArgumentCount == 0);

        Reaper::ScriptFunction missing{
            g_Script,
            Reaper::ScriptPointer{"Missing", "DE AD BE EF"}};
        const auto invocationsBeforeMissing = g_Invocations;
        CHECK(!missing.TryCall<std::int32_t>());
        CHECK(g_Invocations == invocationsBeforeMissing);

        // Rebinding changes the runtime generation and invalidates cached PCs.
        for (std::size_t index = 0; index < directPattern.size(); ++index)
            g_Program.code[4 + index] = 0xCC;
        for (std::size_t index = 0; index < directPattern.size(); ++index)
            g_Program.code[40 + index] = static_cast<std::uint8_t>(directPattern[index]);

        CHECK(Reaper::Enhanced::Game::BindScriptRuntime(&ResolveProgram, &InvokeScript));
        SetReturn<std::int32_t>(88);
        CHECK(direct.Call<std::int32_t>() == 88);
        CHECK(g_LastProgramCounter == 40);

        g_InvokeSucceeds = false;
        CHECK(!direct.TryCall<std::int32_t>());
        g_InvokeSucceeds = true;

        Reaper::Enhanced::Game::Shutdown();
        CHECK(!Reaper::Enhanced::Game::ScriptFunctionsReady());
        CHECK(!direct.TryCall<std::int32_t>());
        return true;
    }
}

int main()
{
    return RunTests() ? 0 : 1;
}
