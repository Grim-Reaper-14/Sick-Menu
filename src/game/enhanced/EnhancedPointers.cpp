#include "EnhancedPointers.hpp"

#include "ScriptGlobal.hpp"
#include "core/memory/Pattern.hpp"

#include <cstdint>
#include <utility>

namespace Sick::Game::Enhanced
{
    namespace
    {
        using Memory::AddressId;
        using Memory::MemoryManager;
        using Memory::Pattern;
        using Memory::PointerCalculator;

        template <typename Callback>
        bool RegisterPattern(
            MemoryManager& memory,
            std::string_view name,
            std::string_view signature,
            Callback&& callback,
            bool required)
        {
            auto pattern = Pattern::Parse(name, signature);
            if (!pattern)
                return false;

            return memory.AddPattern(
                EnhancedPointers::ModuleName,
                std::move(*pattern),
                std::forward<Callback>(callback),
                required,
                true);
        }
    }

    bool EnhancedPointers::RegisterPatterns(MemoryManager& memory)
    {
        bool success = true;

        success = RegisterPattern(
            memory,
            "InitNativeTables",
            "EB 2A 0F 1F 40 00 48 8B 54 17 10",
            [](MemoryManager& manager, PointerCalculator match) {
                manager.Set(AddressId::InitNativeTables, match.Sub(0x2A));
            },
            true) && success;

        success = RegisterPattern(
            memory,
            "RunScriptThreads",
            "BE 40 5D C6 00",
            [](MemoryManager& manager, PointerCalculator match) {
                manager.Set(AddressId::RunScriptThreads, match.Sub(0xA));
            },
            true) && success;

        success = RegisterPattern(
            memory,
            "ScriptGlobals",
            "48 8B 8E B8 00 00 00 48 8D 15 ? ? ? ? 49 89 D8",
            [](MemoryManager& manager, PointerCalculator match) {
                manager.Set(AddressId::ScriptGlobals, match.Add(10).Rip());
            },
            true) && success;

        success = RegisterPattern(
            memory,
            "ScriptThreads",
            "48 8B 05 ? ? ? ? 48 89 34 F8 48 FF C7 48 39 FB 75 97",
            [](MemoryManager& manager, PointerCalculator match) {
                manager.Set(AddressId::ScriptThreads, match.Add(3).Rip());
            },
            false) && success;

        success = RegisterPattern(
            memory,
            "ScriptPrograms",
            "48 C7 84 C8 D8 00 00 00 00 00 00 00",
            [](MemoryManager& manager, PointerCalculator match) {
                manager.Set(AddressId::ScriptPrograms, match.Add(0x16).Rip().Add(0xD8));
            },
            false) && success;

        success = RegisterPattern(
            memory,
            "IDXGISwapChain",
            "72 C7 EB 02 31 C0 8B 0D",
            [](MemoryManager& manager, PointerCalculator match) {
                manager.Set(AddressId::CommandQueue, match.Add(0x1D).Rip());
                manager.Set(AddressId::SwapChain, match.Add(0x24).Rip());
            },
            false) && success;

        success = RegisterPattern(
            memory,
            "WndProc",
            "3D 85 00 00 00 0F 87 2D 02 00 00",
            [](MemoryManager& manager, PointerCalculator match) {
                manager.Set(AddressId::WindowProc, match.Sub(0x4F));
            },
            false) && success;

        success = RegisterPattern(
            memory,
            "HWND",
            "E8 ? ? ? ? 84 C0 74 25 48 8B 0D",
            [](MemoryManager& manager, PointerCalculator match) {
                manager.Set(AddressId::WindowHandle, match.Add(12).Rip());
            },
            false) && success;

        success = RegisterPattern(
            memory,
            "ScreenRes",
            "75 39 0F 57 C0 F3 0F 2A 05",
            [](MemoryManager& manager, PointerCalculator match) {
                manager.Set(AddressId::ScreenResX, match.Add(9).Rip());
                manager.Set(AddressId::ScreenResY, match.Add(0x22).Rip());
            },
            false) && success;

        success = RegisterPattern(
            memory,
            "Version",
            "4C 8D 0D ? ? ? ? 48 8D 5C 24 ? 48 89 D9 48 89 FA",
            [](MemoryManager& manager, PointerCalculator match) {
                manager.Set(AddressId::GameVersion, match.Add(3).Rip());
                manager.Set(AddressId::OnlineVersion, match.Add(0x4A).Rip());
            },
            false) && success;

        success = RegisterPattern(
            memory,
            "HandlesAndPtrs",
            "0F 1F 84 00 00 00 00 00 89 F8 0F 28 FE 41",
            [](MemoryManager& manager, PointerCalculator match) {
                manager.Set(AddressId::HandleToPtr, match.Add(0x22).Rip());
                manager.Set(AddressId::PtrToHandle, match.Sub(0xA).Rip());
            },
            false) && success;

        success = RegisterPattern(
            memory,
            "PedFactory",
            "C7 40 30 03 00 00 00 48 8B 0D",
            [](MemoryManager& manager, PointerCalculator match) {
                manager.Set(AddressId::PedFactory, match.Add(10).Rip());
            },
            false) && success;

        success = RegisterPattern(
            memory,
            "IsSessionStarted",
            "0F B6 05 ? ? ? ? 0A 05 ? ? ? ? 75 2A",
            [](MemoryManager& manager, PointerCalculator match) {
                manager.Set(AddressId::IsSessionStarted, match.Add(3).Rip());
            },
            false) && success;

        success = RegisterPattern(
            memory,
            "RegionCode",
            "4C 8D 05 ? ? ? ? 48 89 F1 48 89 FA E8 ? ? ? ? 84 C0 74 3D",
            [](MemoryManager& manager, PointerCalculator match) {
                manager.Set(AddressId::RegionCode, match.Add(3).Rip());
            },
            false) && success;

        return success;
    }

    bool EnhancedPointers::Initialize()
    {
        auto& memory = MemoryManager::Get();
        if (!memory.LoadModule(ModuleName))
            return false;

        if (!RegisterPatterns(memory))
            return false;

        if (!memory.Scan() || !CoreReady(memory))
            return false;

        return BindScriptGlobalResolver();
    }

    bool EnhancedPointers::CoreReady(const MemoryManager& memory) noexcept
    {
        return memory.Has(AddressId::InitNativeTables) &&
            memory.Has(AddressId::RunScriptThreads) &&
            memory.Has(AddressId::ScriptGlobals);
    }

    bool EnhancedPointers::RendererReady(const MemoryManager& memory) noexcept
    {
        return memory.Has(AddressId::SwapChain) &&
            memory.Has(AddressId::CommandQueue) &&
            memory.Has(AddressId::WindowHandle) &&
            memory.Has(AddressId::WindowProc);
    }

    bool EnhancedPointers::EntityBridgeReady(const MemoryManager& memory) noexcept
    {
        return memory.Has(AddressId::HandleToPtr) &&
            memory.Has(AddressId::PtrToHandle) &&
            memory.Has(AddressId::PedFactory);
    }

    void* EnhancedPointers::ResolveScriptGlobal(std::size_t index) noexcept
    {
        return ResolveScriptGlobal(MemoryManager::Get(), index);
    }

    void* EnhancedPointers::ResolveScriptGlobal(
        const MemoryManager& memory,
        std::size_t index) noexcept
    {
        constexpr std::size_t BlockShift = 0x12;
        constexpr std::size_t BlockMask = 0x3F;
        constexpr std::size_t OffsetMask = 0x3FFFF;

        auto globals = memory.Get<std::int64_t**>(AddressId::ScriptGlobals);
        if (!globals)
            return nullptr;

        const auto block = (index >> BlockShift) & BlockMask;
        auto page = globals[block];
        if (!page)
            return nullptr;

        return page + (index & OffsetMask);
    }

    bool EnhancedPointers::BindScriptGlobalResolver() noexcept
    {
        auto& memory = MemoryManager::Get();
        if (!memory.Has(AddressId::ScriptGlobals))
            return false;

        ScriptGlobal::BindResolver(&EnhancedPointers::ResolveScriptGlobal);
        return true;
    }
}
