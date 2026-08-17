#pragma once

#include "NativeDiagnostics.hpp"
#include "NativeHandlerTable.hpp"
#include "NativeResolver.hpp"
#include "generated/NativeHashes.hpp"

#include <optional>
#include <type_traits>
#include <utility>

namespace Sick::Game::Natives
{
    class NativeInvoker final
    {
    public:
        template <NativeIndex Index, typename Return = void, bool FixVectors = true, typename... Args>
        static Return Invoke(Args&&... args) noexcept
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

            auto handler = NativeHandlerTable::Get().Get(Index);
            if (!handler)
                handler = NativeResolver::Get().Resolve(Generated::HashFor(Index));

            if (!handler)
            {
                NativeDiagnostics::RecordCall(false);
                if constexpr (!std::is_void_v<Return>)
                    return Return{};
                else
                    return;
            }

            handler(&frame);
            if constexpr (FixVectors)
                frame.FixVectors();

            NativeDiagnostics::RecordCall(true);

            if constexpr (!std::is_void_v<Return>)
            {
                static_assert(std::is_trivially_copyable_v<Return>);
                static_assert(sizeof(Return) <= NativeCallContext::MaxReturnBytes);
                return frame.GetResult<Return>();
            }
        }

        template <NativeIndex Index, typename Return, bool FixVectors = true, typename... Args>
        static std::optional<Return> TryInvoke(Args&&... args) noexcept
        {
            static_assert(!std::is_void_v<Return>);
            static_assert(std::is_trivially_copyable_v<Return>);
            static_assert(sizeof(Return) <= NativeCallContext::MaxReturnBytes);

            NativeCallFrame frame;
            const bool pushed = (frame.Push(std::forward<Args>(args)) && ... && true);
            if (!pushed)
            {
                NativeDiagnostics::RecordCall(false);
                return std::nullopt;
            }

            auto handler = NativeHandlerTable::Get().Get(Index);
            if (!handler)
                handler = NativeResolver::Get().Resolve(Generated::HashFor(Index));

            if (!handler)
            {
                NativeDiagnostics::RecordCall(false);
                return std::nullopt;
            }

            handler(&frame);
            if constexpr (FixVectors)
                frame.FixVectors();

            NativeDiagnostics::RecordCall(true);
            return frame.GetResult<Return>();
        }

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
                static_assert(sizeof(Return) <= NativeCallContext::MaxReturnBytes);
                return frame.GetResult<Return>();
            }
        }

        template <typename Return, typename... Args>
        static std::optional<Return> TryCall(NativeHash hash, Args&&... args) noexcept
        {
            static_assert(!std::is_void_v<Return>);
            static_assert(std::is_trivially_copyable_v<Return>);
            static_assert(sizeof(Return) <= NativeCallContext::MaxReturnBytes);

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

        [[nodiscard]] static bool IsAvailable(NativeIndex index) noexcept
        {
            if (NativeHandlerTable::Get().Get(index))
                return true;

            return NativeResolver::Get().Resolve(Generated::HashFor(index)) != nullptr;
        }

        [[nodiscard]] static bool IsAvailable(NativeHash hash) noexcept
        {
            return NativeResolver::Get().Resolve(hash) != nullptr;
        }
    };
}
