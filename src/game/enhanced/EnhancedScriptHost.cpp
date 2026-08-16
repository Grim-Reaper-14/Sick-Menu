#include "EnhancedScriptHost.hpp"

#include "game/scripts/ScriptPointer.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <mutex>
#include <optional>
#include <span>

#if defined(_WIN32)
#include <windows.h>
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#endif

namespace
{
    using Sick::Game::Enhanced::EnhancedScriptHost;
    using Sick::Game::Enhanced::EnhancedScriptHostError;
    using Sick::Game::Enhanced::LiveScriptProgram;
    using Sick::Game::Enhanced::LiveScriptThread;
    using Sick::Game::Enhanced::LiveScriptTlsContext;
    using Sick::Game::Enhanced::ScriptThreadState;
    using Sick::Game::Scripts::ScriptHash;
    using Sick::Game::Scripts::ScriptPattern;
    using Sick::Game::Scripts::ScriptProgramView;

    std::mutex g_BindingsMutex;
    EnhancedScriptHost::Bindings g_Bindings;
    std::atomic g_LastError{EnhancedScriptHostError::None};

    [[nodiscard]] bool Valid(const EnhancedScriptHost::Bindings& bindings) noexcept
    {
        return bindings.scriptThreads != nullptr &&
            bindings.scriptPrograms != nullptr &&
            bindings.scriptGlobals != nullptr &&
            bindings.scriptVm != nullptr &&
            bindings.tlsResolver != nullptr &&
            bindings.programCapacity != 0 &&
            bindings.programCapacity <= 4096;
    }

    [[nodiscard]] EnhancedScriptHost::Bindings SnapshotBindings() noexcept
    {
        std::scoped_lock lock(g_BindingsMutex);
        return g_Bindings;
    }

    void ClearBindings() noexcept
    {
        std::scoped_lock lock(g_BindingsMutex);
        g_Bindings = {};
    }

    [[nodiscard]] const std::uint8_t* CodeAddress(
        const void* handle,
        std::uint32_t index) noexcept
    {
        const auto* program = static_cast<const LiveScriptProgram*>(handle);
        if (!program || !program->codeBlocks || index >= program->codeSize)
            return nullptr;

        const auto page = index >> 14;
        const auto offset = index & 0x3FFF;
        const auto* block = program->codeBlocks[page];
        return block ? block + offset : nullptr;
    }

    [[nodiscard]] LiveScriptProgram* FindProgram(
        const EnhancedScriptHost::Bindings& bindings,
        ScriptHash script) noexcept
    {
        if (!Valid(bindings))
            return nullptr;

        for (std::size_t index = 0; index < bindings.programCapacity; ++index)
        {
            auto* program = bindings.scriptPrograms[index];
            if (program && program->nameHash == script &&
                program->codeBlocks && program->codeSize != 0)
            {
                return program;
            }
        }

        return nullptr;
    }

    [[nodiscard]] LiveScriptThread* FindThread(
        const EnhancedScriptHost::Bindings& bindings,
        ScriptHash script) noexcept
    {
        if (!Valid(bindings) || !bindings.scriptThreads->data)
            return nullptr;

        const auto size = bindings.scriptThreads->size;
        const auto capacity = bindings.scriptThreads->capacity;
        if (size == 0 || size > capacity || capacity > 4096)
            return nullptr;

        for (std::uint16_t index = 0; index < size; ++index)
        {
            auto* thread = bindings.scriptThreads->data[index];
            if (thread && thread->context.threadId != 0 && thread->scriptHash == script)
                return thread;
        }

        return nullptr;
    }

    class TlsScriptScope final
    {
    public:
        TlsScriptScope(LiveScriptTlsContext& tls, LiveScriptThread& thread) noexcept :
            m_Tls(tls),
            m_OriginalThread(tls.currentScriptThread),
            m_OriginalActive(tls.scriptThreadActive)
        {
            m_Tls.currentScriptThread = &thread;
            m_Tls.scriptThreadActive = true;
        }

        ~TlsScriptScope()
        {
            m_Tls.scriptThreadActive = m_OriginalActive;
            m_Tls.currentScriptThread = m_OriginalThread;
        }

        TlsScriptScope(const TlsScriptScope&) = delete;
        TlsScriptScope& operator=(const TlsScriptScope&) = delete;

    private:
        LiveScriptTlsContext& m_Tls;
        LiveScriptThread* m_OriginalThread{};
        bool m_OriginalActive{};
    };

#if defined(_WIN32)
    struct ModuleImage
    {
        std::uintptr_t base{};
        std::size_t size{};
        const std::uint8_t* text{};
        std::size_t textSize{};

        [[nodiscard]] bool Contains(std::uintptr_t address, std::size_t length = 1) const noexcept
        {
            if (base == 0 || size == 0 || address < base || length > size)
                return false;

            const auto offset = address - base;
            return offset <= size - length;
        }
    };

    struct ContiguousCode
    {
        const std::uint8_t* bytes{};
        std::uint32_t size{};
    };

    [[nodiscard]] const std::uint8_t* ContiguousCodeAddress(
        const void* handle,
        std::uint32_t index) noexcept
    {
        const auto* code = static_cast<const ContiguousCode*>(handle);
        return code && code->bytes && index < code->size ? code->bytes + index : nullptr;
    }

    [[nodiscard]] std::optional<ModuleImage> LoadedEnhancedImage() noexcept
    {
        const auto module = ::GetModuleHandleW(L"GTA5_Enhanced.exe");
        if (!module)
            return std::nullopt;

        const auto base = reinterpret_cast<std::uintptr_t>(module);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
            return std::nullopt;

        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.SizeOfImage == 0)
            return std::nullopt;

        const auto imageSize = static_cast<std::size_t>(nt->OptionalHeader.SizeOfImage);
        const auto* section = IMAGE_FIRST_SECTION(nt);
        for (std::uint16_t index = 0; index < nt->FileHeader.NumberOfSections; ++index)
        {
            if (std::memcmp(section[index].Name, ".text", 5) != 0)
                continue;

            const auto sectionOffset = static_cast<std::size_t>(section[index].VirtualAddress);
            const auto virtualSize = static_cast<std::size_t>(section[index].Misc.VirtualSize);
            if (sectionOffset >= imageSize || virtualSize == 0 || virtualSize > imageSize - sectionOffset)
                return std::nullopt;

            return ModuleImage{
                base,
                imageSize,
                reinterpret_cast<const std::uint8_t*>(base + sectionOffset),
                virtualSize};
        }

        return std::nullopt;
    }

    [[nodiscard]] std::optional<std::uintptr_t> FindUniqueSignature(
        const ModuleImage& image,
        std::string_view signature) noexcept
    {
        if (!image.text || image.textSize == 0 ||
            image.textSize > std::numeric_limits<std::uint32_t>::max())
        {
            return std::nullopt;
        }

        const ScriptPattern pattern{signature};
        const auto textSize = static_cast<std::uint32_t>(image.textSize);
        if (!pattern.Valid() || pattern.Size() > textSize)
            return std::nullopt;

        std::optional<std::uintptr_t> result;
        std::size_t searchOffset{};

        while (searchOffset <= image.textSize - pattern.Size())
        {
            const auto remaining = image.textSize - searchOffset;
            const ContiguousCode code{
                image.text + searchOffset,
                static_cast<std::uint32_t>(remaining)};
            const ScriptProgramView view{&code, code.size, &ContiguousCodeAddress};
            const auto match = pattern.Find(view);
            if (!match)
                break;

            const auto address = reinterpret_cast<std::uintptr_t>(image.text) +
                searchOffset + *match;
            if (result)
                return std::nullopt;

            result = address;
            searchOffset += static_cast<std::size_t>(*match) + 1;
        }

        return result;
    }

    [[nodiscard]] std::optional<std::uintptr_t> RipAddress(
        const ModuleImage& image,
        std::uintptr_t displacementAddress) noexcept
    {
        if (!image.Contains(displacementAddress, sizeof(std::int32_t)))
            return std::nullopt;

        std::int32_t displacement{};
        std::memcpy(
            &displacement,
            reinterpret_cast<const void*>(displacementAddress),
            sizeof(displacement));

        const auto instructionEnd = displacementAddress + sizeof(displacement);
        const auto target = static_cast<std::uintptr_t>(
            static_cast<std::intptr_t>(instructionEnd) + displacement);
        return image.Contains(target) ? std::optional<std::uintptr_t>{target} : std::nullopt;
    }

    [[nodiscard]] LiveScriptTlsContext* ResolveTlsContext() noexcept
    {
#if defined(_MSC_VER) && defined(_M_X64)
        const auto tlsStorage = static_cast<std::uintptr_t>(__readgsqword(0x58));
        if (tlsStorage == 0)
            return nullptr;

        return *reinterpret_cast<LiveScriptTlsContext**>(tlsStorage);
#else
        return nullptr;
#endif
    }
#endif
}

namespace Sick::Game::Enhanced
{
    bool EnhancedScriptHost::Initialize() noexcept
    {
        ClearBindings();
        Scripts::ScriptRuntime::Get().Reset();

#if defined(_WIN32)
        const auto image = LoadedEnhancedImage();
        if (!image)
        {
            g_LastError.store(EnhancedScriptHostError::ModuleUnavailable, std::memory_order_release);
            return false;
        }

        const auto threadsMatch = FindUniqueSignature(
            *image,
            "48 8B 05 ? ? ? ? 48 89 34 F8 48 FF C7 48 39 FB 75 97");
        const auto programsMatch = FindUniqueSignature(
            *image,
            "48 C7 84 C8 D8 00 00 00 00 00 00 00");
        const auto globalsMatch = FindUniqueSignature(
            *image,
            "48 8B 8E B8 00 00 00 48 8D 15 ? ? ? ? 49 89 D8");
        const auto vmMatch = FindUniqueSignature(*image, "49 63 41 1C");

        if (!threadsMatch || !programsMatch || !globalsMatch || !vmMatch)
        {
            g_LastError.store(EnhancedScriptHostError::PatternMissing, std::memory_order_release);
            return false;
        }

        const auto threads = RipAddress(*image, *threadsMatch + 3);
        const auto programsBase = RipAddress(*image, *programsMatch + 0x16);
        const auto globals = RipAddress(*image, *globalsMatch + 10);
        if (!threads || !programsBase || !globals || *vmMatch < image->base + 0x24)
        {
            g_LastError.store(EnhancedScriptHostError::InvalidResolvedAddress, std::memory_order_release);
            return false;
        }

        const auto programs = *programsBase + 0xD8;
        const auto scriptVm = *vmMatch - 0x24;
        if (!image->Contains(*threads, sizeof(LiveScriptThreadArray)) ||
            !image->Contains(
                programs,
                DefaultProgramCapacity * sizeof(LiveScriptProgram*)) ||
            !image->Contains(*globals, 64 * sizeof(std::int64_t*)) ||
            !image->Contains(scriptVm))
        {
            g_LastError.store(EnhancedScriptHostError::InvalidResolvedAddress, std::memory_order_release);
            return false;
        }

        return Bind({
            reinterpret_cast<LiveScriptThreadArray*>(*threads),
            reinterpret_cast<LiveScriptProgram**>(programs),
            reinterpret_cast<std::int64_t**>(*globals),
            reinterpret_cast<ScriptVmFn>(scriptVm),
            &ResolveTlsContext,
            DefaultProgramCapacity});
#else
        g_LastError.store(EnhancedScriptHostError::UnsupportedPlatform, std::memory_order_release);
        return false;
#endif
    }

    bool EnhancedScriptHost::Bind(Bindings bindings) noexcept
    {
        ClearBindings();
        Scripts::ScriptRuntime::Get().Reset();

        if (!Valid(bindings))
        {
            g_LastError.store(EnhancedScriptHostError::InvalidBindings, std::memory_order_release);
            return false;
        }

        {
            std::scoped_lock lock(g_BindingsMutex);
            g_Bindings = bindings;
        }

        if (!Scripts::ScriptRuntime::Get().Configure(&ResolveProgram, &Invoke))
        {
            ClearBindings();
            g_LastError.store(EnhancedScriptHostError::InvalidBindings, std::memory_order_release);
            return false;
        }

        g_LastError.store(EnhancedScriptHostError::None, std::memory_order_release);
        return true;
    }

    void EnhancedScriptHost::Shutdown() noexcept
    {
        ClearBindings();
        Scripts::ScriptRuntime::Get().Reset();
        g_LastError.store(EnhancedScriptHostError::None, std::memory_order_release);
    }

    bool EnhancedScriptHost::Ready() noexcept
    {
        return Valid(SnapshotBindings()) && Scripts::ScriptRuntime::Get().Ready();
    }

    EnhancedScriptHostError EnhancedScriptHost::LastError() noexcept
    {
        return g_LastError.load(std::memory_order_acquire);
    }

    std::string_view EnhancedScriptHost::ErrorMessage(EnhancedScriptHostError error) noexcept
    {
        switch (error)
        {
        case EnhancedScriptHostError::None:
            return "ready";
        case EnhancedScriptHostError::UnsupportedPlatform:
            return "automatic script-host discovery is only available on 64-bit Windows";
        case EnhancedScriptHostError::ModuleUnavailable:
            return "GTA5_Enhanced.exe is not loaded";
        case EnhancedScriptHostError::PatternMissing:
            return "one or more Enhanced script-host patterns were not found";
        case EnhancedScriptHostError::InvalidResolvedAddress:
            return "an Enhanced script-host pattern resolved outside the game image";
        case EnhancedScriptHostError::InvalidBindings:
            return "the supplied Enhanced script-host bindings are incomplete";
        }

        return "unknown Enhanced script-host error";
    }

    Scripts::ScriptProgramView EnhancedScriptHost::ResolveProgram(Scripts::ScriptHash script) noexcept
    {
        auto* program = FindProgram(SnapshotBindings(), script);
        return program
            ? Scripts::ScriptProgramView{program, program->codeSize, &CodeAddress}
            : Scripts::ScriptProgramView{};
    }

    bool EnhancedScriptHost::Invoke(
        Scripts::ScriptHash script,
        std::uint32_t programCounter,
        const std::uint64_t* arguments,
        std::size_t argumentCount,
        void* returnValue,
        std::size_t returnSize) noexcept
    {
        const auto bindings = SnapshotBindings();
        auto* program = FindProgram(bindings, script);
        auto* thread = FindThread(bindings, script);
        auto* tls = bindings.tlsResolver ? bindings.tlsResolver() : nullptr;
        if (!program || !thread || !thread->stack || !tls || programCounter >= program->codeSize)
            return false;

        if ((argumentCount != 0 && !arguments) || (returnSize != 0 && !returnValue) ||
            argumentCount > std::numeric_limits<std::uint32_t>::max() ||
            returnSize > std::numeric_limits<std::size_t>::max() -
                (sizeof(std::uint64_t) - 1))
        {
            return false;
        }

        auto context = thread->context;
        const auto top = static_cast<std::size_t>(context.stackPointer);
        const auto stackSize = static_cast<std::size_t>(context.stackSize);
        const auto returnSlots = (returnSize + sizeof(std::uint64_t) - 1) / sizeof(std::uint64_t);

        if (top > stackSize || argumentCount > stackSize - top)
            return false;

        const auto afterArguments = top + argumentCount;
        if (afterArguments >= stackSize || returnSlots > stackSize - top)
            return false;

        for (std::size_t index = 0; index < argumentCount; ++index)
            thread->stack[top + index] = arguments[index];

        // The zero slot is the return address used by GTA's script calling convention.
        thread->stack[afterArguments] = 0;
        context.stackPointer = static_cast<std::uint32_t>(afterArguments + 1);
        context.programCounter = programCounter;
        context.state = ScriptThreadState::Idle;

        {
            TlsScriptScope scope{*tls, *thread};
            static_cast<void>(bindings.scriptVm(
                thread->stack,
                bindings.scriptGlobals,
                program,
                &context));
        }

        if (returnSize != 0)
            std::memcpy(returnValue, thread->stack + top, returnSize);

        return true;
    }
}
