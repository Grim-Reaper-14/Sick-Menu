#include "core/memory/MemoryManager.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace
{
    using namespace Sick::Memory;

    void TestPatternParser()
    {
        const auto pattern = Pattern::Parse("wildcard", "48 8B ?? 05 ? 90");
        assert(pattern.has_value());
        assert(pattern->Size() == 6);
        assert(pattern->Hash() != 0);

        assert(!Pattern::Parse("bad", "48 GG").has_value());
        assert(!Pattern::Parse("bad", "").has_value());
    }

    void TestPointerCalculatorRip()
    {
        std::array<std::byte, 64> image{};
        const auto base = reinterpret_cast<std::uintptr_t>(image.data());

        constexpr std::size_t displacementOffset = 12;
        constexpr std::size_t targetOffset = 44;
        const auto displacement = static_cast<std::int32_t>(
            targetOffset - (displacementOffset + sizeof(std::int32_t)));

        std::memcpy(image.data() + displacementOffset, &displacement, sizeof(displacement));

        const PointerCalculator pointer{base + displacementOffset};
        assert(pointer.Rip().Address() == base + targetOffset);
        assert(pointer.Add(5).Sub(5) == pointer);
    }

    void TestScanAndResolvedStore()
    {
        std::array<std::byte, 64> image{};
        image[8] = std::byte{0x48};
        image[9] = std::byte{0x8B};
        image[10] = std::byte{0x7A};
        image[11] = std::byte{0x05};
        image[12] = std::byte{0x90};

        MemoryManager manager;
        assert(manager.RegisterModule(Module{
            "GTA5_Enhanced.exe",
            reinterpret_cast<std::uintptr_t>(image.data()),
            image.size()}));

        auto required = Pattern::Parse("NativeRegistrationTable", "48 8B ? 05 90");
        assert(required.has_value());
        assert(manager.AddPattern(
            "gta5_enhanced.EXE",
            *required,
            [](MemoryManager& memory, PointerCalculator match) {
                memory.Set(AddressId::NativeRegistrationTable, match.Add(2));
            }));

        auto optional = Pattern::Parse("OptionalMissing", "DE AD BE EF");
        assert(optional.has_value());
        assert(manager.AddPattern(
            "GTA5_Enhanced.exe",
            *optional,
            [](MemoryManager&, PointerCalculator) {},
            false));

        assert(manager.Scan());
        assert(manager.Ready());
        assert(manager.Has(AddressId::NativeRegistrationTable));
        assert(manager.Address(AddressId::NativeRegistrationTable) ==
            reinterpret_cast<std::uintptr_t>(image.data()) + 10);

        const auto diagnostics = manager.Diagnostics();
        assert(diagnostics.size() == 2);
        assert(diagnostics[0].found);
        assert(!diagnostics[1].found);
        assert(!diagnostics[1].required);

        manager.ClearResolved();
        assert(!manager.Ready());
        assert(!manager.Has(AddressId::NativeRegistrationTable));
    }

    void TestRequiredFailure()
    {
        std::array<std::byte, 16> image{};
        MemoryManager manager;
        assert(manager.RegisterModule(Module{
            "test.exe",
            reinterpret_cast<std::uintptr_t>(image.data()),
            image.size()}));

        auto missing = Pattern::Parse("RequiredMissing", "AA BB CC DD");
        assert(missing.has_value());
        assert(manager.AddPattern(
            "test.exe",
            *missing,
            [](MemoryManager&, PointerCalculator) {}));

        assert(!manager.Scan());
        assert(!manager.Ready());

        const auto diagnostics = manager.Diagnostics();
        assert(diagnostics.size() == 1);
        assert(diagnostics[0].required);
        assert(!diagnostics[0].found);
    }
}

int main()
{
    TestPatternParser();
    TestPointerCalculatorRip();
    TestScanAndResolvedStore();
    TestRequiredFailure();
    return 0;
}
