#pragma once

#include "game/scripts/ScriptRuntime.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Sick::Game::Enhanced
{
    enum class ScriptThreadState : std::uint32_t
    {
        Idle = 0,
        Running,
        Killed,
        Paused,
        Unknown4
    };

    struct alignas(8) LiveScriptThreadContext
    {
        std::uint32_t threadId{};
        std::uint32_t padding0{};
        std::uint64_t scriptHash{};
        ScriptThreadState state{};
        std::uint32_t programCounter{};
        std::uint32_t framePointer{};
        std::uint32_t stackPointer{};
        float timerA{};
        float timerB{};
        float waitTimer{};
        std::array<std::byte, 0x2C> padding1{};
        std::uint32_t stackSize{};
        std::array<std::byte, 0x54> padding2{};
    };

    struct alignas(8) LiveScriptThread
    {
        void* vtable{};
        LiveScriptThreadContext context{};
        std::uint64_t* stack{};
        std::array<std::byte, 4> padding0{};
        std::uint32_t parameterSize{};
        std::uint32_t parameterLocation{};
        std::array<std::byte, 4> padding1{};
        std::array<char, 128> errorMessage{};
        std::uint32_t scriptHash{};
        std::array<char, 64> scriptName{};
        std::array<std::byte, 4> padding2{};
    };

    struct alignas(8) LiveScriptProgram
    {
        std::array<std::byte, 0x10> prefix{};
        std::uint8_t** codeBlocks{};
        std::uint32_t hash{};
        std::uint32_t codeSize{};
        std::uint32_t argumentCount{};
        std::uint32_t localCount{};
        std::uint32_t globalCount{};
        std::uint32_t nativeCount{};
        void* localData{};
        void** globalData{};
        void* nativeEntrypoints{};
        std::uint32_t procedureCount{};
        std::uint32_t padding0{};
        const char** procedureNames{};
        std::uint32_t nameHash{};
        std::uint32_t referenceCount{};
        const char* name{};
        const char** stringsData{};
        std::uint32_t stringsCount{};
        std::array<std::byte, 0x0C> breakpoints{};
    };

    struct alignas(8) LiveScriptThreadArray
    {
        LiveScriptThread** data{};
        std::uint16_t size{};
        std::uint16_t capacity{};
        std::uint32_t padding{};
    };

    struct alignas(8) LiveScriptTlsContext
    {
        std::array<std::byte, 0x7A0> padding{};
        LiveScriptThread* currentScriptThread{};
        bool scriptThreadActive{};
        std::array<std::byte, 7> tailPadding{};
    };

    static_assert(sizeof(void*) == 8, "The Enhanced script host requires a 64-bit process");
    static_assert(sizeof(LiveScriptThreadContext) == 0xB0);
    static_assert(offsetof(LiveScriptThreadContext, programCounter) == 0x14);
    static_assert(offsetof(LiveScriptThreadContext, stackPointer) == 0x1C);
    static_assert(offsetof(LiveScriptThreadContext, stackSize) == 0x58);
    static_assert(sizeof(LiveScriptThread) == 0x198);
    static_assert(offsetof(LiveScriptThread, stack) == 0xB8);
    static_assert(offsetof(LiveScriptThread, scriptHash) == 0x150);
    static_assert(sizeof(LiveScriptProgram) == 0x80);
    static_assert(offsetof(LiveScriptProgram, codeBlocks) == 0x10);
    static_assert(offsetof(LiveScriptProgram, nameHash) == 0x58);
    static_assert(sizeof(LiveScriptThreadArray) == 0x10);
    static_assert(sizeof(LiveScriptTlsContext) == 0x7B0);
    static_assert(offsetof(LiveScriptTlsContext, currentScriptThread) == 0x7A0);

    enum class EnhancedScriptHostError : std::uint8_t
    {
        None = 0,
        UnsupportedPlatform,
        ModuleUnavailable,
        PatternMissing,
        InvalidResolvedAddress,
        InvalidBindings
    };

    class EnhancedScriptHost final
    {
    public:
        static constexpr std::size_t DefaultProgramCapacity = 176;

        using ScriptVmFn = std::int32_t (*)(
            std::uint64_t* stack,
            std::int64_t** scriptGlobals,
            LiveScriptProgram* program,
            LiveScriptThreadContext* context);
        using TlsResolverFn = LiveScriptTlsContext* (*)() noexcept;

        struct Bindings
        {
            LiveScriptThreadArray* scriptThreads{};
            LiveScriptProgram** scriptPrograms{};
            std::int64_t** scriptGlobals{};
            ScriptVmFn scriptVm{};
            TlsResolverFn tlsResolver{};
            std::size_t programCapacity{DefaultProgramCapacity};
        };

        // Locates the Enhanced script tables and VM inside GTA5_Enhanced.exe.
        static bool Initialize() noexcept;

        // Allows tests or an existing pointer manager to supply already-resolved addresses.
        static bool Bind(Bindings bindings) noexcept;
        static void Shutdown() noexcept;

        [[nodiscard]] static bool Ready() noexcept;
        [[nodiscard]] static EnhancedScriptHostError LastError() noexcept;
        [[nodiscard]] static std::string_view ErrorMessage(EnhancedScriptHostError error) noexcept;

    private:
        [[nodiscard]] static Scripts::ScriptProgramView ResolveProgram(
            Scripts::ScriptHash script) noexcept;
        [[nodiscard]] static bool Invoke(
            Scripts::ScriptHash script,
            std::uint32_t programCounter,
            const std::uint64_t* arguments,
            std::size_t argumentCount,
            void* returnValue,
            std::size_t returnSize) noexcept;
    };
}
