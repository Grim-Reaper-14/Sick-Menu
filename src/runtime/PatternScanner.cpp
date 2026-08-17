#include "PatternScanner.hpp"

#include <cstring>
#include <limits>
#include <string>
#include <vector>
#include <windows.h>

namespace
{
    struct PatternByte
    {
        std::uint8_t value{};
        bool wildcard{};
    };

    [[nodiscard]] std::optional<std::vector<PatternByte>> Parse(std::string_view signature)
    {
        std::vector<PatternByte> bytes;
        for (std::size_t cursor = 0; cursor < signature.size();)
        {
            while (cursor < signature.size() && signature[cursor] == ' ')
                ++cursor;
            if (cursor == signature.size())
                break;
            if (signature[cursor] == '?')
            {
                bytes.push_back({0, true});
                while (cursor < signature.size() && signature[cursor] == '?')
                    ++cursor;
                continue;
            }
            if (cursor + 1 >= signature.size())
                return std::nullopt;

            const auto nibble = [](char character) -> int {
                if (character >= '0' && character <= '9') return character - '0';
                if (character >= 'A' && character <= 'F') return character - 'A' + 10;
                if (character >= 'a' && character <= 'f') return character - 'a' + 10;
                return -1;
            };
            const int high = nibble(signature[cursor]);
            const int low = nibble(signature[cursor + 1]);
            if (high < 0 || low < 0)
                return std::nullopt;
            bytes.push_back({static_cast<std::uint8_t>((high << 4) | low), false});
            cursor += 2;
        }
        return bytes.empty() ? std::nullopt : std::optional{std::move(bytes)};
    }
}

namespace Sick::Runtime
{
    bool ModuleImage::Contains(std::uintptr_t address, std::size_t length) const noexcept
    {
        if (!base || !size || address < base || length > size)
            return false;
        return address - base <= size - length;
    }

    std::optional<ModuleImage> LoadModuleImage(std::wstring_view name) noexcept
    {
        const std::wstring moduleName{name};
        const auto module = GetModuleHandleW(moduleName.c_str());
        if (!module)
            return std::nullopt;
        const auto base = reinterpret_cast<std::uintptr_t>(module);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
            return std::nullopt;
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || !nt->OptionalHeader.SizeOfImage)
            return std::nullopt;
        const auto imageSize = static_cast<std::size_t>(nt->OptionalHeader.SizeOfImage);
        const auto* sections = IMAGE_FIRST_SECTION(nt);
        for (std::uint16_t index = 0; index < nt->FileHeader.NumberOfSections; ++index)
        {
            if (std::memcmp(sections[index].Name, ".text", 5) != 0)
                continue;
            const auto offset = static_cast<std::size_t>(sections[index].VirtualAddress);
            const auto size = static_cast<std::size_t>(sections[index].Misc.VirtualSize);
            if (!size || offset >= imageSize || size > imageSize - offset)
                return std::nullopt;
            return ModuleImage{base, imageSize, reinterpret_cast<const std::uint8_t*>(base + offset), size};
        }
        return std::nullopt;
    }

    std::optional<std::uintptr_t> FindUnique(const ModuleImage& image, std::string_view signature) noexcept
    {
        const auto parsed = Parse(signature);
        if (!parsed || parsed->size() > image.textSize)
            return std::nullopt;
        std::optional<std::uintptr_t> result;
        for (std::size_t offset = 0; offset <= image.textSize - parsed->size(); ++offset)
        {
            bool matches = true;
            for (std::size_t index = 0; index < parsed->size(); ++index)
            {
                if (!(*parsed)[index].wildcard && image.text[offset + index] != (*parsed)[index].value)
                {
                    matches = false;
                    break;
                }
            }
            if (!matches)
                continue;
            if (result)
                return std::nullopt;
            result = reinterpret_cast<std::uintptr_t>(image.text + offset);
        }
        return result;
    }

    std::optional<std::uintptr_t> Rip(const ModuleImage& image, std::uintptr_t address) noexcept
    {
        if (!image.Contains(address, sizeof(std::int32_t)))
            return std::nullopt;
        std::int32_t displacement{};
        std::memcpy(&displacement, reinterpret_cast<const void*>(address), sizeof(displacement));
        const auto end = address + sizeof(displacement);
        const auto target = static_cast<std::uintptr_t>(static_cast<std::intptr_t>(end) + displacement);
        return image.Contains(target) ? std::optional{target} : std::nullopt;
    }
}
