#pragma once

#include "NativeDiagnostics.hpp"
#include "NativeHandlerTable.hpp"
#include "NativeResolver.hpp"
#include "generated/NativeHashes.hpp"
#include "core/diagnostics/Metrics.hpp"
#include "core/logging/Logger.hpp"
#include "core/logging/RateLimiter.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
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
            constexpr auto hash = Generated::HashFor(Index);
            NativeCallFrame frame;
            const bool pushed = (frame.Push(std::forward<Args>(args)) && ... && true);

            if (!pushed)
            {
                NativeDiagnostics::RecordCall(false);
                ReportFailure(hash, "argument_overflow", Core::Logging::EventId::NativeArgumentOverflow);
                if constexpr (!std::is_void_v<Return>)
                    return Return{};
                else
                    return;
            }

            auto handler = NativeHandlerTable::Get().Get(Index);
            if (!handler)
                handler = NativeResolver::Get().Resolve(hash);

            if (!handler)
            {
                NativeDiagnostics::RecordCall(false);
                ReportFailure(hash, "missing_handler", Core::Logging::EventId::NativeCallFailure);
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
                static_assert(sizeof(Return) <= sizeof(std::uint64_t));
                return frame.GetResult<Return>();
            }
        }

        template <NativeIndex Index, typename Return, bool FixVectors = true, typename... Args>
        static std::optional<Return> TryInvoke(Args&&... args) noexcept
        {
            static_assert(!std::is_void_v<Return>);
            static_assert(std::is_trivially_copyable_v<Return>);
            static_assert(sizeof(Return) <= sizeof(std::uint64_t));

            constexpr auto hash = Generated::HashFor(Index);
            NativeCallFrame frame;
            const bool pushed = (frame.Push(std::forward<Args>(args)) && ... && true);
            if (!pushed)
            {
                NativeDiagnostics::RecordCall(false);
                ReportFailure(hash, "argument_overflow", Core::Logging::EventId::NativeArgumentOverflow);
                return std::nullopt;
            }

            auto handler = NativeHandlerTable::Get().Get(Index);
            if (!handler)
                handler = NativeResolver::Get().Resolve(hash);

            if (!handler)
            {
                NativeDiagnostics::RecordCall(false);
                ReportFailure(hash, "missing_handler", Core::Logging::EventId::NativeCallFailure);
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
                ReportFailure(hash, "argument_overflow", Core::Logging::EventId::NativeArgumentOverflow);
                if constexpr (!std::is_void_v<Return>)
                    return Return{};
                else
                    return;
            }

            const auto handler = NativeResolver::Get().Resolve(hash);
            if (!handler)
            {
                NativeDiagnostics::RecordCall(false);
                ReportFailure(hash, "missing_handler", Core::Logging::EventId::NativeCallFailure);
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
                ReportFailure(hash, "argument_overflow", Core::Logging::EventId::NativeArgumentOverflow);
                return std::nullopt;
            }

            const auto handler = NativeResolver::Get().Resolve(hash);
            if (!handler)
            {
                NativeDiagnostics::RecordCall(false);
                ReportFailure(hash, "missing_handler", Core::Logging::EventId::NativeCallFailure);
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
                ReportFailure(hash, "argument_overflow", Core::Logging::EventId::NativeArgumentOverflow);
                return false;
            }

            const auto handler = NativeResolver::Get().Resolve(hash);
            if (!handler)
            {
                NativeDiagnostics::RecordCall(false);
                ReportFailure(hash, "missing_handler", Core::Logging::EventId::NativeCallFailure);
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

    private:
        static void ReportFailure(
            NativeHash hash,
            std::string_view reason,
            Core::Logging::EventId event) noexcept
        {
            Core::Metrics::Increment("native.failures");

            try
            {
                const auto rateKey = Core::Logging::Detail::Format("native.{}.{}", reason, hash);
                if (!Core::Logging::RateLimiter::Global().Allow(rateKey, std::chrono::seconds{1}))
                    return;

                Core::Logging::Logger::Get().Submit(
                    Core::Logging::Level::Error,
                    "Native",
                    Core::Logging::Detail::Format("Native call failed: {}", reason),
                    event,
                    {
                        Core::Logging::MakeField("hash", Core::Logging::Detail::Hex(hash)),
                        Core::Logging::MakeField("reason", reason)
                    });
            }
            catch (...)
            {
            }
        }
    };
}
