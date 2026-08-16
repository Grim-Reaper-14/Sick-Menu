#include "game/enhanced/EnhancedPointers.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace
{
    using namespace Sick::Game::Enhanced;
    using namespace Sick::Memory;

    void WriteBytes(std::span<std::byte> image, std::size_t offset, std::initializer_list<std::uint8_t> bytes)
    {
        for (const auto byte : bytes)
            image[offset++] = static_cast<std::byte>(byte);
    }

    void WriteRipDisplacement(
        std::span<std::byte> image,
        std::size_t displacementOffset,
        std::size_t targetOffset)
    {
        const auto displacement = static_cast<std::int32_t>(
            targetOffset - (displacementOffset + sizeof(std::int32_t)));
        std::memcpy(image.data() + displacementOffset, &displacement, sizeof(displacement));
    }

    void TestCorePatternRegistrationAndResolution()
    {
        std::array<std::byte, 1024> image{};

        constexpr std::size_t initNativePattern = 128;
        WriteBytes(image, initNativePattern, {
            0xEB, 0x2A, 0x0F, 0x1F, 0x40, 0x00, 0x48, 0x8B, 0x54, 0x17, 0x10});

        constexpr std::size_t runScriptPattern = 256;
        WriteBytes(image, runScriptPattern, {0xBE, 0x40, 0x5D, 0xC6, 0x00});

        constexpr std::size_t scriptGlobalsPattern = 384;
        WriteBytes(image, scriptGlobalsPattern, {
            0x48, 0x8B, 0x8E, 0xB8, 0x00, 0x00, 0x00,
            0x48, 0x8D, 0x15, 0x00, 0x00, 0x00, 0x00,
            0x49, 0x89, 0xD8});
        constexpr std::size_t scriptGlobalsTarget = 800;
        WriteRipDisplacement(image, scriptGlobalsPattern + 10, scriptGlobalsTarget);

        MemoryManager memory;
        assert(memory.RegisterModule(Module{
            std::string{EnhancedPointers::ModuleName},
            reinterpret_cast<std::uintptr_t>(image.data()),
            image.size()}));

        assert(EnhancedPointers::RegisterPatterns(memory));
        assert(memory.PatternCount() == 14);
        assert(memory.Scan());
        assert(EnhancedPointers::CoreReady(memory));
        assert(!EnhancedPointers::RendererReady(memory));
        assert(!EnhancedPointers::EntityBridgeReady(memory));

        const auto base = reinterpret_cast<std::uintptr_t>(image.data());
        assert(memory.Address(AddressId::InitNativeTables) == base + initNativePattern - 0x2A);
        assert(memory.Address(AddressId::RunScriptThreads) == base + runScriptPattern - 0xA);
        assert(memory.Address(AddressId::ScriptGlobals) == base + scriptGlobalsTarget);

        const auto diagnostics = memory.Diagnostics();
        assert(diagnostics.size() == 14);
        for (const auto& diagnostic : diagnostics)
        {
            if (diagnostic.required)
            {
                assert(diagnostic.found);
                assert(!diagnostic.ambiguous);
                assert(diagnostic.matchCount == 1);
            }
        }
    }

    void TestScriptGlobalResolverLayout()
    {
        std::array<std::int64_t*, 64> globals{};
        std::array<std::int64_t, 128> page{};
        globals[5] = page.data();

        MemoryManager memory;
        memory.Set(
            AddressId::ScriptGlobals,
            PointerCalculator{reinterpret_cast<std::uintptr_t>(globals.data())});

        constexpr std::size_t index = (5ULL << 0x12) | 37ULL;
        assert(EnhancedPointers::ResolveScriptGlobal(memory, index) == &page[37]);

        constexpr std::size_t missingIndex = (6ULL << 0x12) | 4ULL;
        assert(EnhancedPointers::ResolveScriptGlobal(memory, missingIndex) == nullptr);
    }
}

int main()
{
    TestCorePatternRegistrationAndResolution();
    TestScriptGlobalResolverLayout();
    return 0;
}
