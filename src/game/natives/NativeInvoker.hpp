#pragma once

#include "NativeResolver.hpp"

#include <optional>
#include <type_traits>
#include <utility>

namespace Sick::Game::Natives
{
    class NativeInvoker final
    {
    public:
        template <typename Return = void, typename... Args>
        static Return Call(NativeHash hash, Args&&... args) noexcept
        {
            NativeCallFrame frame;
            const bool pushed = (frame.Push(std::forward<Args>(args)) && ... && true);

            if (!pushed)
            {
                if constexpr (!std::is_void_v<Return>)
                    return Return{};
                else
                    return;
            }

            const auto handler = NativeResolver::Get().Resolve(hash);
            if (!handler)
            {
                if constexpr (!std::is_void_v<Return>)
                    return Return{};
                else
                    return;
            }

            handler(&frame);
            frame.FixVectors();

            if constexpr (!std::is_void_v<Return>)
            {
                static_assert(std::is_trivially_copyable_v<Return>);
                static_assert(sizeof(Return) <= sizeof(std::uint64_t));
                return frame.GetResult<Return>();
            }
        }

        template <typename Return, typename... Args>
        static std::optional<Return> TryCall(NativeHash hash, Args&&... args) noexcept
        {
            static_assert(!std::is_void_v<Return>);
            static_assert(std::is_trivially_copyable_v<Return>);
            static_assert(sizeof(Return) <= sizeof(std::uint64_t));

            NativeCallFrame frame;
            const bool pushed = (frame.Push(std::forward<Args>(args)) && ... && true);
            if (!pushed)
                return std::nullopt;

            const auto handler = NativeResolver::Get().Resolve(hash);
            if (!handler)
                return std::nullopt;

            handler(&frame);
            frame.FixVectors();
            return frame.GetResult<Return>();
        }

        template <typename... Args>
        static bool TryCallVoid(NativeHash hash, Args&&... args) noexcept
        {
            NativeCallFrame frame;
            const bool pushed = (frame.Push(std::forward<Args>(args)) && ... && true);
            if (!pushed)
                return false;

            const auto handler = NativeResolver::Get().Resolve(hash);
            if (!handler)
                return false;

            handler(&frame);
            frame.FixVectors();
            return true;
        }

        [[nodiscard]] static bool IsAvailable(NativeHash hash) noexcept
        {
            return NativeResolver::Get().Resolve(hash) != nullptr;
        }
    };
}
