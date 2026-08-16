#include "MemoryManager.hpp"

#include <algorithm>
#include <mutex>
#include <utility>

namespace Sick::Memory
{
    MemoryManager& MemoryManager::Get() noexcept
    {
        static MemoryManager manager;
        return manager;
    }

    bool MemoryManager::RegisterModule(Module module)
    {
        return m_Modules.Register(std::move(module));
    }

    bool MemoryManager::LoadModule(std::string_view name)
    {
        return m_Modules.Load(name);
    }

    bool MemoryManager::AddPattern(
        std::string_view moduleName,
        Pattern pattern,
        ResolveCallback callback,
        bool required)
    {
        if (pattern.Empty() || !callback)
            return false;

        const auto resolvedModule = moduleName.empty() ? std::string{MainModuleName} : std::string{moduleName};

        std::unique_lock lock(m_Mutex);
        if (m_Ready)
            return false;

        m_Bindings.push_back({
            resolvedModule,
            std::move(pattern),
            std::move(callback),
            required});
        return true;
    }

    bool MemoryManager::Scan()
    {
        std::vector<Binding> bindings;
        {
            std::unique_lock lock(m_Mutex);
            std::ranges::fill(m_Addresses, 0);
            m_Diagnostics.clear();
            m_Ready = false;
            bindings = m_Bindings;
        }

        bool success = true;
        std::vector<ScanDiagnostic> diagnostics;
        diagnostics.reserve(bindings.size());

        for (const auto& binding : bindings)
        {
            const auto module = m_Modules.Find(binding.module);
            if (!module)
            {
                diagnostics.push_back({
                    binding.pattern.Name(),
                    binding.module,
                    binding.required,
                    false,
                    0});

                if (binding.required)
                    success = false;
                continue;
            }

            PatternScanner scanner{*module};
            scanner.Add(
                binding.pattern,
                [this, &binding](PointerCalculator pointer) {
                    binding.callback(*this, pointer);
                },
                binding.required);

            auto summary = scanner.Scan();
            success = success && summary.success;
            diagnostics.insert(
                diagnostics.end(),
                std::make_move_iterator(summary.diagnostics.begin()),
                std::make_move_iterator(summary.diagnostics.end()));
        }

        {
            std::unique_lock lock(m_Mutex);
            m_Diagnostics = std::move(diagnostics);
            m_Ready = success;
        }

        return success;
    }

    void MemoryManager::ClearResolved() noexcept
    {
        std::unique_lock lock(m_Mutex);
        std::ranges::fill(m_Addresses, 0);
        m_Diagnostics.clear();
        m_Ready = false;
    }

    void MemoryManager::Reset() noexcept
    {
        {
            std::unique_lock lock(m_Mutex);
            m_Bindings.clear();
            std::ranges::fill(m_Addresses, 0);
            m_Diagnostics.clear();
            m_Ready = false;
        }

        m_Modules.Clear();
    }

    void MemoryManager::Set(AddressId id, PointerCalculator pointer) noexcept
    {
        const auto offset = Offset(id);
        if (offset >= AddressCount)
            return;

        std::unique_lock lock(m_Mutex);
        m_Addresses[offset] = pointer.Address();
    }

    std::uintptr_t MemoryManager::Address(AddressId id) const noexcept
    {
        const auto offset = Offset(id);
        if (offset >= AddressCount)
            return 0;

        std::shared_lock lock(m_Mutex);
        return m_Addresses[offset];
    }

    bool MemoryManager::Has(AddressId id) const noexcept
    {
        return Address(id) != 0;
    }

    bool MemoryManager::Ready() const noexcept
    {
        std::shared_lock lock(m_Mutex);
        return m_Ready;
    }

    std::size_t MemoryManager::PatternCount() const noexcept
    {
        std::shared_lock lock(m_Mutex);
        return m_Bindings.size();
    }

    std::size_t MemoryManager::ModuleCount() const noexcept
    {
        return m_Modules.Count();
    }

    std::vector<ScanDiagnostic> MemoryManager::Diagnostics() const
    {
        std::shared_lock lock(m_Mutex);
        return m_Diagnostics;
    }

    std::size_t MemoryManager::Offset(AddressId id) noexcept
    {
        return static_cast<std::size_t>(id);
    }
}
