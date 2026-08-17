#include "MenuRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace Sick::Ui
{
    void MenuDrawList::AddFilledRect(MenuRect bounds, MenuColor color) { MenuDrawCommand command{}; command.kind = MenuDrawCommandKind::FilledRect; command.bounds = bounds; command.color = color; m_Commands.push_back(std::move(command)); }
    void MenuDrawList::AddText(MenuRect bounds, std::string text, MenuColor color, float fontSize, MenuTextAlign alignment) { MenuDrawCommand command{}; command.kind = MenuDrawCommandKind::Text; command.bounds = bounds; command.color = color; command.text = std::move(text); command.textAlign = alignment; command.fontSize = fontSize; m_Commands.push_back(std::move(command)); }
    void MenuDrawList::AddFilledCircle(MenuPoint center, float radius, MenuColor color) { MenuDrawCommand command{}; command.kind = MenuDrawCommandKind::FilledCircle; command.start = center; command.radius = radius; command.color = color; m_Commands.push_back(std::move(command)); }
    void MenuDrawList::AddLine(MenuPoint start, MenuPoint end, MenuColor color, float thickness) { MenuDrawCommand command{}; command.kind = MenuDrawCommandKind::Line; command.start = start; command.end = end; command.color = color; command.thickness = thickness; m_Commands.push_back(std::move(command)); }
    void MenuDrawList::AddImage(MenuRect bounds, MenuTexture texture) { if (texture == 0) return; MenuDrawCommand command{}; command.kind = MenuDrawCommandKind::Image; command.bounds = bounds; command.texture = texture; command.color = MenuColor{255, 255, 255, 255}; m_Commands.push_back(std::move(command)); }
    const std::vector<MenuDrawCommand>& MenuDrawList::Commands() const noexcept { return m_Commands; }
    bool MenuDrawList::Empty() const noexcept { return m_Commands.empty(); }

    MenuRenderer::MenuRenderer(MenuStyle style) : m_Style(std::move(style)) {}

    MenuDrawList MenuRenderer::Render(MenuController& controller, MenuViewport viewport, MenuTexture headerTexture) const
    {
        MenuDrawList drawList;
        if (!controller.IsOpen()) return drawList;
        auto* page = controller.CurrentPage();
        if (!page || viewport.width <= 0.0F || viewport.height <= 0.0F) return drawList;

        controller.SetVisibleRows(std::max<std::size_t>(m_Style.maxVisibleRows, 1));
        const float widthScale = viewport.width / std::max(m_Style.referenceWidth, 1.0F);
        const float heightScale = viewport.height / std::max(m_Style.referenceHeight, 1.0F);
        const float viewportScale = std::max(std::min(widthScale, heightScale), 0.35F);
        const float scale = viewportScale * std::clamp(m_Style.uiScale, 0.5F, 2.5F);
        const auto scaled = [scale](float value) { return value * scale; };

        const auto& options = page->Options();
        const std::size_t first = controller.VisibleStart();
        const std::size_t remaining = first < options.size() ? options.size() - first : 0;
        const std::size_t rowCount = std::max<std::size_t>(1, std::min(remaining, controller.VisibleRows()));
        const auto* selectedOption = controller.SelectedOption();
        const bool showDescription = selectedOption && !selectedOption->Description().empty();

        const float border = scaled(m_Style.border);
        const float innerLeft = m_Style.left * viewportScale + border;
        const float innerTop = m_Style.top * viewportScale + border;
        const float innerWidth = scaled(m_Style.width);
        const float headerHeight = scaled(m_Style.headerHeight);
        const float titleHeight = scaled(m_Style.titleHeight);
        const float rowHeight = scaled(m_Style.rowHeight);
        const float descriptionHeight = showDescription ? scaled(m_Style.descriptionHeight) : 0.0F;
        const float footerHeight = scaled(m_Style.footerHeight);
        const float bodyHeight = rowHeight * static_cast<float>(rowCount);
        const float innerHeight = headerHeight + titleHeight + bodyHeight + descriptionHeight + footerHeight;

        const MenuRect outer{innerLeft - border, innerTop - border, innerLeft + innerWidth + border, innerTop + innerHeight + border};
        drawList.AddFilledRect(outer, m_Style.borderColor);

        const MenuRect header{innerLeft, innerTop, innerLeft + innerWidth, innerTop + headerHeight};
        drawList.AddFilledRect(header, m_Style.headerColor);
        if (headerTexture != 0) drawList.AddImage(header, headerTexture);
        else
        {
            const MenuRect band{header.left, header.top + headerHeight * 0.67F, header.right, header.bottom};
            drawList.AddFilledRect(band, m_Style.headerBandColor);
            const float centerX = (header.left + header.right) * 0.5F;
            const float centerY = header.top + headerHeight * 0.47F;
            const float armX = scaled(34.0F), armY = scaled(29.0F), shadow = scaled(10.0F), stroke = scaled(6.0F);
            drawList.AddLine({centerX - armX, centerY - armY}, {centerX + armX, centerY + armY}, m_Style.logoShadow, shadow);
            drawList.AddLine({centerX + armX, centerY - armY}, {centerX - armX, centerY + armY}, m_Style.logoShadow, shadow);
            drawList.AddLine({centerX - armX, centerY - armY}, {centerX + armX, centerY + armY}, m_Style.logoCyan, stroke);
            drawList.AddLine({centerX + armX, centerY - armY}, {centerX - armX, centerY + armY}, m_Style.logoMagenta, stroke);
            drawList.AddText({header.left, centerY + scaled(29.0F), header.right, header.bottom}, "SICK MENU", m_Style.textColor, scaled(m_Style.brandFontSize), MenuTextAlign::Center);
        }

        const MenuRect titleBar{innerLeft, header.bottom, innerLeft + innerWidth, header.bottom + titleHeight};
        drawList.AddFilledRect(titleBar, m_Style.titleColor);
        const float padding = scaled(m_Style.horizontalPadding);
        const MenuRect titleTextBounds{titleBar.left + padding, titleBar.top, titleBar.right - padding, titleBar.bottom};
        drawList.AddText(titleTextBounds, std::string{page->Title()}, m_Style.textColor, scaled(m_Style.titleFontSize), MenuTextAlign::Left);
        const auto counter = controller.SelectionCounter();
        drawList.AddText(titleTextBounds, std::to_string(counter.current) + " / " + std::to_string(counter.total), m_Style.textColor, scaled(m_Style.titleFontSize), MenuTextAlign::Right);

        const MenuRect body{innerLeft, titleBar.bottom, innerLeft + innerWidth, titleBar.bottom + bodyHeight};
        drawList.AddFilledRect(body, m_Style.bodyColor);
        const auto selectedIndex = controller.SelectedOptionIndex();
        for (std::size_t row = 0; row < rowCount; ++row)
        {
            const std::size_t optionIndex = first + row;
            if (optionIndex >= options.size()) break;
            const auto& option = options[optionIndex];
            const float rowTop = body.top + rowHeight * static_cast<float>(row);
            const MenuRect rowBounds{body.left, rowTop, body.right, rowTop + rowHeight};
            const bool selected = optionIndex == selectedIndex;
            if (selected) drawList.AddFilledRect(rowBounds, m_Style.selectedColor);

            const auto textColor = !option.Enabled() ? m_Style.disabledTextColor : (selected ? m_Style.selectedTextColor : m_Style.textColor);
            const MenuRect optionTextBounds{rowBounds.left + padding, rowBounds.top, rowBounds.right - padding, rowBounds.bottom};
            if (option.Kind() == MenuOptionKind::Label)
            {
                drawList.AddText(optionTextBounds, std::string{option.LabelText()}, textColor, scaled(m_Style.optionFontSize), MenuTextAlign::Center);
                continue;
            }
            if (option.Kind() == MenuOptionKind::Info)
            {
                drawList.AddText(optionTextBounds, std::string{option.LabelText()}, textColor, scaled(m_Style.optionFontSize), MenuTextAlign::Left);
                drawList.AddText(optionTextBounds, option.ValueText(), textColor, scaled(m_Style.optionFontSize), MenuTextAlign::Right);
                continue;
            }

            drawList.AddText(optionTextBounds, std::string{option.LabelText()}, textColor, scaled(m_Style.optionFontSize), MenuTextAlign::Left);
            if (option.Kind() == MenuOptionKind::Toggle)
            {
                const bool enabled = option.Enabled();
                const bool active = option.ToggleValue();
                const float trackRadius = scaled(8.0F), trackWidth = scaled(38.0F);
                const float trackRight = rowBounds.right - padding, trackLeft = trackRight - trackWidth;
                const float centerY = (rowBounds.top + rowBounds.bottom) * 0.5F;
                const MenuColor trackColor = enabled && active ? m_Style.accentColor : m_Style.inactiveToggleColor;
                const MenuColor stateTextColor = !enabled ? m_Style.disabledTextColor : (active ? m_Style.accentColor : m_Style.disabledTextColor);
                drawList.AddText({optionTextBounds.left, optionTextBounds.top, trackLeft - scaled(10.0F), optionTextBounds.bottom}, active ? "ON" : "OFF", stateTextColor, scaled(m_Style.optionFontSize * 0.78F), MenuTextAlign::Right);
                drawList.AddFilledRect({trackLeft + trackRadius, centerY - trackRadius, trackRight - trackRadius, centerY + trackRadius}, trackColor);
                drawList.AddFilledCircle({trackLeft + trackRadius, centerY}, trackRadius, trackColor);
                drawList.AddFilledCircle({trackRight - trackRadius, centerY}, trackRadius, trackColor);
                const float knobRadius = scaled(6.0F);
                drawList.AddFilledCircle({active ? trackRight - trackRadius : trackLeft + trackRadius, centerY}, knobRadius, m_Style.selectedColor);
            }
            else
            {
                const auto value = option.ValueText();
                if (!value.empty())
                {
                    const bool adjustable = option.Kind() == MenuOptionKind::Integer ||
                        option.Kind() == MenuOptionKind::Float || option.Kind() == MenuOptionKind::Choice;
                    const auto displayValue = adjustable ? std::string{"< "} + value + " >" : value;
                    drawList.AddText(optionTextBounds, displayValue, textColor, scaled(m_Style.optionFontSize), MenuTextAlign::Right);
                }
            }
        }

        float footerTop = body.bottom;
        if (showDescription)
        {
            const MenuRect description{innerLeft, body.bottom, innerLeft + innerWidth, body.bottom + descriptionHeight};
            drawList.AddFilledRect(description, m_Style.titleColor);
            drawList.AddLine({description.left, description.top}, {description.right, description.top}, m_Style.accentColor, scaled(1.5F));
            drawList.AddText({description.left + padding, description.top, description.right - padding, description.bottom}, std::string{selectedOption->Description()}, m_Style.textColor, scaled(m_Style.descriptionFontSize), MenuTextAlign::Left);
            footerTop = description.bottom;
        }

        const MenuRect footer{innerLeft, footerTop, innerLeft + innerWidth, footerTop + footerHeight};
        drawList.AddFilledRect(footer, m_Style.footerColor);
        const float arrowX = (footer.left + footer.right) * 0.5F, arrowWidth = scaled(8.0F), arrowHeight = scaled(6.0F);
        const float upperY = footer.top + footerHeight * 0.40F, lowerY = footer.top + footerHeight * 0.67F, arrowStroke = scaled(3.5F);
        drawList.AddLine({arrowX - arrowWidth, upperY + arrowHeight}, {arrowX, upperY}, m_Style.textColor, arrowStroke);
        drawList.AddLine({arrowX, upperY}, {arrowX + arrowWidth, upperY + arrowHeight}, m_Style.textColor, arrowStroke);
        drawList.AddLine({arrowX - arrowWidth, lowerY}, {arrowX, lowerY + arrowHeight}, m_Style.textColor, arrowStroke);
        drawList.AddLine({arrowX, lowerY + arrowHeight}, {arrowX + arrowWidth, lowerY}, m_Style.textColor, arrowStroke);
        return drawList;
    }

    MenuStyle& MenuRenderer::Style() noexcept { return m_Style; }
    const MenuStyle& MenuRenderer::Style() const noexcept { return m_Style; }
}
