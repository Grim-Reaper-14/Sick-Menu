#pragma once

#include "Menu.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Sick::Ui
{
    struct MenuColor
    {
        std::uint8_t red{};
        std::uint8_t green{};
        std::uint8_t blue{};
        std::uint8_t alpha{255};
        [[nodiscard]] constexpr bool operator==(const MenuColor&) const noexcept = default;
    };

    struct MenuPoint { float x{}; float y{}; };
    struct MenuRect { float left{}; float top{}; float right{}; float bottom{}; };

    enum class MenuTextAlign { Left, Center, Right };
    enum class MenuDrawCommandKind { FilledRect, Text, FilledCircle, Line, Image };
    using MenuTexture = std::uintptr_t;

    struct MenuDrawCommand
    {
        MenuDrawCommandKind kind{MenuDrawCommandKind::FilledRect};
        MenuRect bounds{};
        MenuPoint start{};
        MenuPoint end{};
        MenuColor color{};
        std::string text;
        MenuTextAlign textAlign{MenuTextAlign::Left};
        float fontSize{};
        float radius{};
        float thickness{1.0F};
        MenuTexture texture{};
    };

    class MenuDrawList final
    {
    public:
        void AddFilledRect(MenuRect bounds, MenuColor color);
        void AddText(MenuRect bounds, std::string text, MenuColor color, float fontSize, MenuTextAlign alignment = MenuTextAlign::Left);
        void AddFilledCircle(MenuPoint center, float radius, MenuColor color);
        void AddLine(MenuPoint start, MenuPoint end, MenuColor color, float thickness);
        void AddImage(MenuRect bounds, MenuTexture texture);
        [[nodiscard]] const std::vector<MenuDrawCommand>& Commands() const noexcept;
        [[nodiscard]] bool Empty() const noexcept;
    private:
        std::vector<MenuDrawCommand> m_Commands;
    };

    struct MenuViewport { float width{1920.0F}; float height{1080.0F}; };

    struct MenuStyle
    {
        float referenceWidth{1920.0F};
        float referenceHeight{1080.0F};
        float left{48.0F};
        float top{12.0F};
        float uiScale{1.25F};
        float width{780.0F};
        float border{10.0F};
        float headerHeight{150.0F};
        float titleHeight{60.0F};
        float rowHeight{52.0F};
        float descriptionHeight{48.0F};
        float footerHeight{56.0F};
        float horizontalPadding{20.0F};
        float titleFontSize{31.0F};
        float optionFontSize{28.0F};
        float descriptionFontSize{19.0F};
        float brandFontSize{18.0F};
        std::size_t maxVisibleRows{10};

        MenuColor borderColor{35, 50, 77, 255};
        MenuColor headerColor{0, 7, 50, 255};
        MenuColor headerBandColor{0, 11, 70, 255};
        MenuColor titleColor{2, 4, 9, 255};
        MenuColor bodyColor{7, 13, 22, 255};
        MenuColor footerColor{2, 4, 9, 255};
        MenuColor selectedColor{246, 246, 246, 255};
        MenuColor textColor{244, 244, 246, 255};
        MenuColor selectedTextColor{16, 18, 22, 255};
        MenuColor disabledTextColor{116, 122, 132, 255};
        MenuColor accentColor{226, 0, 82, 255};
        MenuColor inactiveToggleColor{66, 72, 84, 255};
        MenuColor logoCyan{41, 214, 255, 255};
        MenuColor logoMagenta{226, 0, 198, 255};
        MenuColor logoShadow{28, 8, 80, 255};
    };

    class MenuRenderer final
    {
    public:
        MenuRenderer() = default;
        explicit MenuRenderer(MenuStyle style);
        [[nodiscard]] MenuDrawList Render(MenuController& controller, MenuViewport viewport, MenuTexture headerTexture = 0) const;
        [[nodiscard]] MenuStyle& Style() noexcept;
        [[nodiscard]] const MenuStyle& Style() const noexcept;
    private:
        MenuStyle m_Style;
    };
}
