#pragma once

#include "NativeTypes.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>

namespace Sick::Game::Natives
{
    struct ScriptVector
    {
        alignas(8) float x{};
        alignas(8) float y{};
        alignas(8) float z{};
    };
    static_assert(sizeof(ScriptVector) == 0x18);

    struct alignas(16) NativeVector3
    {
        float x{};
        float y{};
        float z{};
        float pad{};
    };
    static_assert(sizeof(NativeVector3) == 0x10);

    class NativeCallContext
    {
    public:
        static constexpr std::size_t MaxReturnBytes = 16 * sizeof(std::uint64_t);

        void Reset() noexcept
        {
            m_ArgumentCount = 0;
            m_VectorCount = 0;
        }

        template <typename T>
        void PushArgument(T&& value) noexcept
        {
            using Raw = std::decay_t<T>;
            static_assert(std::is_trivially_copyable_v<Raw>);
            static_assert(sizeof(Raw) <= sizeof(std::uint64_t));

            std::uint64_t slot{};
            Raw copy = std::forward<T>(value);
            std::memcpy(&slot, &copy, sizeof(Raw));

            static_cast<std::uint64_t*>(m_Arguments)[m_ArgumentCount++] = slot;
        }

        template <typename T>
        [[nodiscard]] T GetArgument(std::size_t index) const noexcept
        {
            static_assert(std::is_trivially_copyable_v<T>);
            static_assert(sizeof(T) <= sizeof(std::uint64_t));

            T value{};
            const auto slot = static_cast<const std::uint64_t*>(m_Arguments)[index];
            std::memcpy(&value, &slot, sizeof(T));
            return value;
        }

        template <typename T>
        void SetArgument(std::size_t index, const T& value) noexcept
        {
            static_assert(std::is_trivially_copyable_v<T>);
            static_assert(sizeof(T) <= sizeof(std::uint64_t));

            std::uint64_t slot{};
            std::memcpy(&slot, &value, sizeof(T));
            static_cast<std::uint64_t*>(m_Arguments)[index] = slot;
        }

        template <typename T>
        [[nodiscard]] T GetResult() const noexcept
        {
            static_assert(std::is_trivially_copyable_v<T>);
            static_assert(sizeof(T) <= MaxReturnBytes);

            T value{};
            std::memcpy(&value, m_ReturnValue, sizeof(T));
            return value;
        }

        template <typename T>
        void SetResult(const T& value) noexcept
        {
            static_assert(std::is_trivially_copyable_v<T>);
            static_assert(sizeof(T) <= MaxReturnBytes);
            std::memcpy(m_ReturnValue, &value, sizeof(T));
        }

        void FixVectors() noexcept
        {
            for (std::int32_t i = 0; i < m_VectorCount; ++i)
            {
                const auto& source = m_VectorSources[static_cast<std::size_t>(i)];
                auto* target = m_VectorTargets[static_cast<std::size_t>(i)];

                if (target)
                {
                    target->x = source.x;
                    target->y = source.y;
                    target->z = source.z;
                }
            }

            m_VectorCount = 0;
        }

        [[nodiscard]] std::uint32_t ArgumentCount() const noexcept
        {
            return m_ArgumentCount;
        }

    protected:
        void* m_ReturnValue{};                 // 0x00
        std::uint32_t m_ArgumentCount{};       // 0x08
        void* m_Arguments{};                   // 0x10
        std::int32_t m_VectorCount{};          // 0x18
        ScriptVector* m_VectorTargets[4]{};    // 0x20
        NativeVector3 m_VectorSources[4]{};    // 0x40
    };
    static_assert(sizeof(NativeCallContext) == 0x80);

    using NativeHandler = void (*)(NativeCallContext*);

    class NativeCallFrame final : public NativeCallContext
    {
    public:
        static constexpr std::size_t kReturnSlots = 16;
        static constexpr std::size_t kArgumentSlots = 64;

        NativeCallFrame() noexcept
        {
            m_ReturnValue = m_ReturnStorage.data();
            m_Arguments = m_ArgumentStorage.data();
            Reset();
        }

        void Reset() noexcept
        {
            m_ReturnStorage.fill(0);
            m_ArgumentStorage.fill(0);
            NativeCallContext::Reset();
        }

        template <typename T>
        bool Push(T&& value) noexcept
        {
            if (m_ArgumentCount >= kArgumentSlots)
                return false;

            PushArgument(std::forward<T>(value));
            return true;
        }

    private:
        std::array<std::uint64_t, kReturnSlots> m_ReturnStorage{};
        std::array<std::uint64_t, kArgumentSlots> m_ArgumentStorage{};
    };
}
