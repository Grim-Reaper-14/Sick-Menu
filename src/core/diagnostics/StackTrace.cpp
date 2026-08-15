#include "StackTrace.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <execinfo.h>
#endif

namespace Sick::Core::Diagnostics
{
    std::vector<std::uintptr_t> CaptureStack(std::size_t skip, std::size_t maxFrames) noexcept
    {
        try
        {
            maxFrames = std::min<std::size_t>(maxFrames, 128);
            std::vector<std::uintptr_t> result;
            result.reserve(maxFrames);

#ifdef _WIN32
            void* frames[128]{};
            const auto captured = CaptureStackBackTrace(
                static_cast<DWORD>(std::min<std::size_t>(skip + 1, 127)),
                static_cast<DWORD>(maxFrames),
                frames,
                nullptr);

            for (USHORT i = 0; i < captured; ++i)
                result.push_back(reinterpret_cast<std::uintptr_t>(frames[i]));
#elif defined(__unix__) || defined(__APPLE__)
            void* frames[128]{};
            const auto requested = std::min<std::size_t>(128, maxFrames + skip + 1);
            const auto captured = ::backtrace(frames, static_cast<int>(requested));
            const auto first = std::min<int>(captured, static_cast<int>(std::min<std::size_t>(skip + 1, 128)));
            for (int i = first; i < captured && result.size() < maxFrames; ++i)
                result.push_back(reinterpret_cast<std::uintptr_t>(frames[i]));
#else
            (void)skip;
#endif

            return result;
        }
        catch (...)
        {
            return {};
        }
    }

    std::string FormatStack(const std::vector<std::uintptr_t>& frames)
    {
        std::ostringstream stream;
        for (std::size_t i = 0; i < frames.size(); ++i)
        {
            stream << '#' << i << " 0x"
                   << std::uppercase << std::hex << frames[i] << std::dec;
            if (i + 1 < frames.size())
                stream << '\n';
        }
        return stream.str();
    }
}
