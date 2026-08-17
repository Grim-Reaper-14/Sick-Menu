#include "AssetCatalog.hpp"

#include "FileSystem.hpp"
#include "IoService.hpp"
#include "LoggerApi.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace Sick::Backend::System
{
    namespace
    {
        std::string Lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            });
            return value;
        }

        bool HasExtension(const std::filesystem::path& path, std::initializer_list<std::string_view> extensions)
        {
            const auto extension = Lower(path.extension().string());
            return std::any_of(extensions.begin(), extensions.end(), [&](std::string_view candidate) {
                return extension == candidate;
            });
        }

        std::vector<AssetEntry> ScanArea(
            FileSystem& files,
            FileArea area,
            std::initializer_list<std::string_view> extensions)
        {
            std::vector<AssetEntry> entries;
            for (const auto& file : files.List(area))
            {
                if (file.directory || !HasExtension(file.relativePath, extensions))
                    continue;
                const auto absolute = files.Resolve(area, file.relativePath);
                if (!absolute)
                    continue;
                entries.push_back({file.relativePath.stem().string(), absolute->string()});
            }
            return entries;
        }

        std::optional<std::uint32_t> ParseColor(const nlohmann::json& value)
        {
            if (!value.is_string())
                return std::nullopt;
            auto text = value.get<std::string>();
            if (!text.empty() && text.front() == '#')
                text.erase(text.begin());
            if (text.size() != 6 && text.size() != 8)
                return std::nullopt;
            try
            {
                const auto parsed = static_cast<std::uint32_t>(std::stoul(text, nullptr, 16));
                return text.size() == 6 ? ((parsed << 8U) | 0xFFU) : parsed;
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        void ReadColor(const nlohmann::json& colors, const char* key, std::uint32_t& destination)
        {
            const auto it = colors.find(key);
            if (it == colors.end())
                return;
            if (const auto parsed = ParseColor(*it))
                destination = *parsed;
        }

        ThemeEntry LoadTheme(FileSystem& files, const FileEntry& file)
        {
            ThemeEntry theme{};
            const auto absolute = files.Resolve(FileArea::Themes, file.relativePath);
            theme.asset.name = file.relativePath.stem().string();
            theme.asset.path = absolute ? absolute->string() : file.relativePath.string();

            const auto contents = files.ReadText(FileArea::Themes, file.relativePath);
            if (!contents)
                return theme;
            try
            {
                const auto root = nlohmann::json::parse(*contents);
                if (root.contains("name") && root["name"].is_string())
                    theme.asset.name = root["name"].get<std::string>();
                if (!root.contains("colors") || !root["colors"].is_object())
                    return theme;
                const auto& colors = root["colors"];
                ReadColor(colors, "border", theme.palette.border);
                ReadColor(colors, "header", theme.palette.header);
                ReadColor(colors, "header_band", theme.palette.headerBand);
                ReadColor(colors, "title", theme.palette.title);
                ReadColor(colors, "body", theme.palette.body);
                ReadColor(colors, "footer", theme.palette.footer);
                ReadColor(colors, "selected", theme.palette.selected);
                ReadColor(colors, "text", theme.palette.text);
                ReadColor(colors, "selected_text", theme.palette.selectedText);
                ReadColor(colors, "disabled_text", theme.palette.disabledText);
                ReadColor(colors, "accent", theme.palette.accent);
                ReadColor(colors, "inactive_toggle", theme.palette.inactiveToggle);
                ReadColor(colors, "logo_cyan", theme.palette.logoCyan);
                ReadColor(colors, "logo_magenta", theme.palette.logoMagenta);
                ReadColor(colors, "logo_shadow", theme.palette.logoShadow);
            }
            catch (...)
            {
                LoggerApi::Get().Warn("assets", "Theme JSON could not be parsed", {{"file", file.relativePath.string()}});
            }
            return theme;
        }

        std::vector<ThemeEntry> ScanThemes(FileSystem& files)
        {
            std::vector<ThemeEntry> entries;
            for (const auto& file : files.List(FileArea::Themes))
            {
                if (file.directory || !HasExtension(file.relativePath, {".json"}))
                    continue;
                entries.push_back(LoadTheme(files, file));
            }
            std::sort(entries.begin(), entries.end(), [](const ThemeEntry& left, const ThemeEntry& right) {
                return Lower(left.asset.name) < Lower(right.asset.name);
            });
            return entries;
        }

        void SortUnique(std::vector<AssetEntry>& entries)
        {
            std::sort(entries.begin(), entries.end(), [](const AssetEntry& left, const AssetEntry& right) {
                return Lower(left.name) < Lower(right.name);
            });
            entries.erase(std::unique(entries.begin(), entries.end(), [](const AssetEntry& left, const AssetEntry& right) {
                return Lower(left.path) == Lower(right.path);
            }), entries.end());
        }

        void AppendWindowsFonts(std::vector<AssetEntry>& fonts)
        {
#if defined(_WIN32)
            wchar_t windowsDirectory[MAX_PATH]{};
            const auto length = GetWindowsDirectoryW(windowsDirectory, MAX_PATH);
            if (length == 0 || length >= MAX_PATH)
                return;
            const std::filesystem::path directory = std::filesystem::path(windowsDirectory) / L"Fonts";
            std::error_code error;
            for (std::filesystem::directory_iterator it(directory, error), end; !error && it != end; it.increment(error))
            {
                if (!it->is_regular_file(error) || error)
                    continue;
                const auto path = it->path();
                if (!HasExtension(path, {".ttf", ".otf", ".ttc"}))
                    continue;
                fonts.push_back({"Windows / " + path.stem().string(), path.string()});
            }
#endif
        }
    }

    bool AssetCatalog::Initialize(IoService& io, FileSystem& files) noexcept
    {
        {
            std::scoped_lock lock(m_Mutex);
            m_Io = &io;
            m_Files = &files;
            m_Snapshot = {};
            m_RefreshGeneration = 0;
            m_Enabled = true;
        }
        m_PublishedGeneration.store(0, std::memory_order_release);
        return Refresh();
    }

    void AssetCatalog::Shutdown() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        m_Enabled = false;
        ++m_RefreshGeneration;
        m_Io = nullptr;
        m_Files = nullptr;
    }

    bool AssetCatalog::Refresh()
    {
        IoService* io{};
        FileSystem* files{};
        std::uint64_t generation{};
        {
            std::scoped_lock lock(m_Mutex);
            if (!m_Enabled || !m_Io || !m_Files)
                return false;
            io = m_Io;
            files = m_Files;
            generation = ++m_RefreshGeneration;
        }

        return io->Submit(IoPriority::Normal, [this, files, generation]() {
            AssetCatalogSnapshot snapshot{};
            snapshot.generation = generation;
            snapshot.themes = ScanThemes(*files);
            snapshot.images = ScanArea(*files, FileArea::Images, {".png", ".jpg", ".jpeg", ".bmp"});
            snapshot.fonts = ScanArea(*files, FileArea::Fonts, {".ttf", ".otf", ".ttc"});
            for (auto& font : snapshot.fonts)
                font.name = "Custom / " + font.name;
            snapshot.scripts = ScanArea(*files, FileArea::Scripts, {".lua"});
            AppendWindowsFonts(snapshot.fonts);
            SortUnique(snapshot.images);
            SortUnique(snapshot.fonts);
            SortUnique(snapshot.scripts);

            {
                std::scoped_lock lock(m_Mutex);
                if (!m_Enabled || generation != m_RefreshGeneration)
                    return;
                m_Snapshot = std::move(snapshot);
            }
            m_PublishedGeneration.store(generation, std::memory_order_release);
            LoggerApi::Get().Debug("assets", "Asset catalog refreshed");
        }).has_value();
    }

    AssetCatalogSnapshot AssetCatalog::Snapshot() const
    {
        std::scoped_lock lock(m_Mutex);
        return m_Snapshot;
    }

    std::uint64_t AssetCatalog::Generation() const noexcept
    {
        return m_PublishedGeneration.load(std::memory_order_acquire);
    }
}
