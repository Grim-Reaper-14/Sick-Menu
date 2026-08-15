#pragma once

#include "NativeDiagnostics.hpp"
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
                NativeDiagnostics::RecordCall(false);
                if constexpr (!std::is_void_v<Return>)
                    return Return{};
                else
                    return;
            }

            const auto handler = NativeResolver::Get().Resolve(hash);
            if (!handler)
            {
                NativeDiagnostics::RecordCall(false);
                if constexpr (!std::is_void_v<Return>)
                    return Return{};
                else
                    return;
            }

            handler(&frame);
            frame.FixVectors();
            NativeDiagnostics::RecordCall(true);

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
            {
                NativeDiagnostics::RecordCall(false);
                return std::nullopt;
            }

            const auto handler = NativeResolver::Get().Resolve(hash);
            if (!handler)
            {
                NativeDiagnostics::RecordCall(false);
                return std::nullopt;
            }

            handler(&frame);
            frame.FixVectors();
            NativeDiagnostics::RecordCall(true);
            return frame.GetResult<Return>();
        }

        template <typename... Args>
        static bool TryCallVoid(NativeHash hash, Args&&... args) noexcept
        {
            NativeCallFrame frame;
            const bool pushed = (frame.Push(std::forward<Args>(args)) && ... && true);
            if (!pushed)
            {
                NativeDiagnostics::RecordCall(false);
                return false;
            }

            const auto handler = NativeResolver::Get().Resolve(hash);
            if (!handler)
            {
                NativeDiagnostics::RecordCall(false);
                return false;
            }

            handler(&frame);
            frame.FixVectors();
            NativeDiagnostics::RecordCall(true);
            return true;
        }

        [[nodiscard]] static bool IsAvailable(NativeHash hash) noexcept
        {
            return NativeResolver::Get().Resolve(hash) != nullptr;
        }
    };
}
