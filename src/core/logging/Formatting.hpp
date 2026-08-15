#pragma once

#include <array>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace Sick::Core::Logging::Detail
{
    template <typename T>
    [[nodiscard]] std::string ToText(T&& value)
    {
        using Raw = std::remove_cvref_t<T>;

        if constexpr (std::is_same_v<Raw, std::string>)
        {
            return std::forward<T>(value);
        }
        else if constexpr (std::is_same_v<Raw, std::string_view>)
        {
            return std::string{value};
        }
        else if constexpr (std::is_same_v<Raw, const char*> || std::is_same_v<Raw, char*>)
        {
            return value ? std::string{value} : std::string{"<null>"};
        }
        else if constexpr (std::is_same_v<Raw, bool>)
        {
            return value ? "true" : "false";
        }
        else if constexpr (std::is_enum_v<Raw>)
        {
            using Underlying = std::underlying_type_t<Raw>;
            return ToText(static_cast<Underlying>(value));
        }
        else
        {
            std::ostringstream stream;
            stream << std::forward<T>(value);
            return stream.str();
        }
    }

    template <typename... Args>
    [[nodiscard]] std::string Format(std::string_view format, Args&&... args)
    {
        std::array<std::string, sizeof...(Args)> values{
            ToText(std::forward<Args>(args))...
        };

        std::string output;
        output.reserve(format.size() + values.size() * 12);

        std::size_t valueIndex = 0;
        for (std::size_t i = 0; i < format.size(); ++i)
        {
            const char ch = format[i];
            if (ch == '{' && i + 1 < format.size())
            {
                if (format[i + 1] == '{')
                {
                    output.push_back('{');
                    ++i;
                    continue;
                }

                if (format[i + 1] == '}')
                {
                    if (valueIndex < values.size())
                        output += values[valueIndex++];
                    else
                        output += "{}";

                    ++i;
                    continue;
                }
            }
            else if (ch == '}' && i + 1 < format.size() && format[i + 1] == '}')
            {
                output.push_back('}');
                ++i;
                continue;
            }

            output.push_back(ch);
        }

        while (valueIndex < values.size())
        {
            output += " [arg";
            output += std::to_string(valueIndex);
            output += '=';
            output += values[valueIndex++];
            output += ']';
        }

        return output;
    }

    template <typename T>
    struct HexValue
    {
        T value;
    };

    template <typename T>
    [[nodiscard]] constexpr HexValue<T> Hex(T value) noexcept
    {
        return {value};
    }

    template <typename T>
    std::ostream& operator<<(std::ostream& stream, const HexValue<T>& value)
    {
        const auto flags = stream.flags();
        stream << "0x" << std::uppercase << std::hex << value.value;
        stream.flags(flags);
        return stream;
    }

    [[nodiscard]] inline std::string EscapeJson(std::string_view value)
    {
        std::string output;
        output.reserve(value.size() + 8);

        for (const char ch : value)
        {
            switch (ch)
            {
            case '\\': output += "\\\\"; break;
            case '"': output += "\\\""; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20)
                {
                    std::ostringstream stream;
                    stream << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                           << static_cast<int>(static_cast<unsigned char>(ch));
                    output += stream.str();
                }
                else
                {
                    output.push_back(ch);
                }
                break;
            }
        }

        return output;
    }
}
