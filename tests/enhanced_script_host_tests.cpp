#include "Reaper.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace
{
    using Host = Reaper::Enhanced::ScriptHost;
    using Program = Sick::Game::Enhanced::LiveScriptProgram;
    using Thread = Sick::Game::Enhanced::LiveScriptThread;
    using ThreadArray = Sick::Game::Enhanced::LiveScriptThreadArray;
    using Tls = Sick::Game::Enhanced::LiveScriptTlsContext;

    constexpr auto Script = Reaper::Joaat("freemode");
    std::array<std::uint8_t, 0x4000> g_Code{};
    std::array<std::uint8_t*, 1> g_CodeBlocks{g_Code.data()};
    Program g_Program{};
    std::array<Program*, Host::DefaultProgramCapacity> g_Programs{};
    std::array<std::uint64_t, 128> g_Stack{};
    Thread g_Thread{};
    std::array<Thread*, 1> g_Threads{&g_Thread};
    ThreadArray g_ThreadArray{};
    std::array<std::int64_t*, 64> g_Globals{};
    Tls g_Tls{};
    bool g_SawExpectedTls{};
    Sick::Game::Enhanced::ScriptThreadState g_ObservedState{};
    std::uint32_t g_ObservedPc{};
    std::uint32_t g_ObservedStackPointer{};

    Tls* ResolveTls() noexcept
    {
        return &g_Tls;
    }

    std::int32_t RunVm(
        std::uint64_t* stack,
        std::int64_t** globals,
        Program* program,
        Sick::Game::Enhanced::LiveScriptThreadContext* context)
    {
        g_SawExpectedTls = g_Tls.currentScriptThread == &g_Thread &&
            g_Tls.scriptThreadActive;
        g_ObservedState = context->state;
        g_ObservedPc = context->programCounter;
        g_ObservedStackPointer = context->stackPointer;

        if (stack != g_Stack.data() || globals != g_Globals.data() || program != &g_Program)
            return -1;

        if (context->stackPointer == 11)
        {
            const Reaper::ScriptVector3 vector{1.0f, 2.0f, 3.0f};
            std::memcpy(stack + 10, &vector, sizeof(vector));
            return 0;
        }

        const auto left = static_cast<std::int32_t>(stack[10]);
        const auto right = static_cast<std::int32_t>(stack[11]);
        const auto result = left + right;
        std::memcpy(stack + 10, &result, sizeof(result));
        return 0;
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
        Host::Shutdown();
        CHECK(!Host::Ready());
        CHECK(!Host::Bind({}));
        CHECK(Host::LastError() ==
            Sick::Game::Enhanced::EnhancedScriptHostError::InvalidBindings);

        g_Code.fill(0xCC);
        // ApplyMPSVData catalog signature; its three-byte call target is 32.
        g_Code[4] = 0x5D;
        g_Code[5] = 0x20;
        g_Code[6] = 0x00;
        g_Code[7] = 0x00;
        g_Code[8] = 0x38;
        g_Code[9] = 0x2A;
        g_Code[10] = 0x71;

        g_Program.codeBlocks = g_CodeBlocks.data();
        g_Program.codeSize = static_cast<std::uint32_t>(g_Code.size());
        g_Program.nameHash = Script;
        g_Program.referenceCount = 2;
        g_Programs.fill(nullptr);
        g_Programs[0] = &g_Program;

        g_Stack.fill(0);
        g_Thread.context.threadId = 7;
        g_Thread.context.state = Sick::Game::Enhanced::ScriptThreadState::Paused;
        g_Thread.context.stackPointer = 10;
        g_Thread.context.stackSize = static_cast<std::uint32_t>(g_Stack.size());
        g_Thread.stack = g_Stack.data();
        g_Thread.scriptHash = Script;
        g_ThreadArray.data = g_Threads.data();
        g_ThreadArray.size = 1;
        g_ThreadArray.capacity = 1;

        Thread originalThread{};
        g_Tls.currentScriptThread = &originalThread;
        g_Tls.scriptThreadActive = false;

        CHECK(Host::Bind({
            &g_ThreadArray,
            g_Programs.data(),
            g_Globals.data(),
            &RunVm,
            &ResolveTls,
            g_Programs.size()}));
        CHECK(Host::Ready());
        CHECK(Host::LastError() == Sick::Game::Enhanced::EnhancedScriptHostError::None);

        const auto* spec = Reaper::ScriptFunctions::Find(
            Reaper::KnownScriptFunction::ApplyMpsvData);
        CHECK(spec != nullptr);
        CHECK(spec->script == Script);
        auto function = spec->Bind();

        const auto result = function.TryCall<std::int32_t>(20, 22);
        CHECK(result && *result == 42);
        CHECK(g_SawExpectedTls);
        CHECK(g_ObservedState == Sick::Game::Enhanced::ScriptThreadState::Idle);
        CHECK(g_ObservedPc == 32);
        CHECK(g_ObservedStackPointer == 13);
        CHECK(g_Tls.currentScriptThread == &originalThread);
        CHECK(!g_Tls.scriptThreadActive);
        CHECK(g_Thread.context.stackPointer == 10);
        CHECK(g_Thread.context.programCounter == 0);
        CHECK(g_Thread.context.state == Sick::Game::Enhanced::ScriptThreadState::Paused);

        const auto vector = function.TryCall<Reaper::ScriptVector3>();
        CHECK(vector.has_value());
        CHECK(vector->x == 1.0f && vector->y == 2.0f && vector->z == 3.0f);
        CHECK(g_ObservedStackPointer == 11);

        g_ThreadArray.size = 0;
        CHECK(!function.TryCall<std::int32_t>(1, 2));
        g_ThreadArray.size = 1;

        g_Thread.context.stackPointer = static_cast<std::uint32_t>(g_Stack.size() - 1);
        CHECK(!function.TryCall<std::int32_t>(1, 2));
        g_Thread.context.stackPointer = 10;

        CHECK(Reaper::ScriptFunctions::All().size() ==
            static_cast<std::size_t>(Reaper::KnownScriptFunction::Count));
        for (const auto& definition : Reaper::ScriptFunctions::All())
        {
            CHECK(!definition.name.empty());
            CHECK(!definition.scriptName.empty());
            CHECK(definition.Pointer().Valid());
            CHECK(Reaper::ScriptFunctions::Find(definition.id) == &definition);
            CHECK(Reaper::ScriptFunctions::Find(definition.name) == &definition);
        }
        CHECK(Reaper::ScriptFunctions::ReferenceEnhancedBuild == "1158.13");
        CHECK(Reaper::ScriptFunctions::Find("GetWeaponKills") != nullptr);
        CHECK(Reaper::ScriptFunctions::Find("DoesNotExist") == nullptr);

        const auto* dynamicSpec = Reaper::ScriptFunctions::Find(
            Reaper::KnownScriptFunction::SetFmContentServerState);
        CHECK(dynamicSpec && dynamicSpec->script == 0);
        auto dynamicFunction = dynamicSpec->Bind(Reaper::Joaat("fm_content_test"));
        CHECK(dynamicFunction.Script() == Reaper::Joaat("fm_content_test"));

        Host::Shutdown();
        CHECK(!Host::Ready());
        CHECK(!function.TryCall<std::int32_t>(1, 2));
        return true;
    }
}

int main()
{
    return RunTests() ? 0 : 1;
}
