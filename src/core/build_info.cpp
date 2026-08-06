#include "sick/core/build_info.hpp"

#include <format>
#include <vector>

namespace sick::core
{
    bool BuildVersion::valid() const noexcept
    {
        return major != 0 || minor != 0 || patch != 0 || revision != 0;
    }

    std::string BuildVersion::to_string() const
    {
        return std::format("{}.{}.{}.{}", major, minor, patch, revision);
    }

    BuildInfo::BuildInfo(const HMODULE module) noexcept
    {
        wchar_t path[MAX_PATH]{};
        const DWORD length = GetModuleFileNameW(module, path, MAX_PATH);
        if (length == 0 || length == MAX_PATH)
            return;

        m_path.assign(path, length);

        DWORD handle{};
        const DWORD version_size = GetFileVersionInfoSizeW(m_path.c_str(), &handle);
        if (version_size == 0)
            return;

        std::vector<std::byte> buffer(version_size);
        if (GetFileVersionInfoW(m_path.c_str(), 0, version_size, buffer.data()) == FALSE)
            return;

        VS_FIXEDFILEINFO* info{};
        UINT info_size{};
        if (VerQueryValueW(buffer.data(), L"\\", reinterpret_cast<void**>(&info), &info_size) == FALSE ||
            info == nullptr || info_size < sizeof(VS_FIXEDFILEINFO) || info->dwSignature != 0xFEEF04BD)
        {
            return;
        }

        m_version.major = HIWORD(info->dwFileVersionMS);
        m_version.minor = LOWORD(info->dwFileVersionMS);
        m_version.patch = HIWORD(info->dwFileVersionLS);
        m_version.revision = LOWORD(info->dwFileVersionLS);
    }

    bool BuildInfo::valid() const noexcept
    {
        return !m_path.empty() && m_version.valid();
    }

    const BuildVersion& BuildInfo::version() const noexcept
    {
        return m_version;
    }

    const std::wstring& BuildInfo::path() const noexcept
    {
        return m_path;
    }
}
