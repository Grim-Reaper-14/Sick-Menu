#pragma once

// This adapter is intentionally header-only so sick_native does not require
// Dear ImGui. Include it from the host project after ImGui is available and
// call ImGuiMenuBackend::Render() from the host's existing ImGui frame.

#include "SickMenu.hpp"

#include <imgui.h>

#include <cfloat>
#include <cstdint>
#include <type_traits>

namespace Sick::Ui
{
    struct ImGuiMenuKeys
    {
        ImGuiKey toggle{ImGuiKey_F4};
        ImGuiKey up{ImGuiKey_Keypad8};
        ImGuiKey down{ImGuiKey_Keypad2};
        ImGuiKey left{ImGuiKey_Keypad4};
        ImGuiKey right{ImGuiKey_Keypad6};
        ImGuiKey select{ImGuiKey_Keypad5};
        ImGuiKey back{ImGuiKey_Keypad0};
        ImGuiKey backAlternate{ImGuiKey_Insert};
    };

    class ImGuiMenuBackend final
    {
    public:
        static void PollKeyboard(
            MenuController& controller,
            const ImGuiMenuKeys& keys = {})
        {
            if (ImGui::IsKeyPressed(keys.toggle, false))
                controller.Handle(MenuInput::Toggle);
            if (!controller.IsOpen())
                return;
            if (ImGui::IsKeyPressed(keys.up))
                controller.Handle(MenuInput::Up);
            if (ImGui::IsKeyPressed(keys.down))
                controller.Handle(MenuInput::Down);
            if (ImGui::IsKeyPressed(keys.left))
                controller.Handle(MenuInput::Left);
            if (ImGui::IsKeyPressed(keys.right))
                controller.Handle(MenuInput::Right);
            if (ImGui::IsKeyPressed(keys.select, false))
                controller.Handle(MenuInput::Select);
            const bool backPressed = ImGui::IsKeyPressed(keys.back, false) ||
                (keys.back == ImGuiKey_Keypad0 && ImGui::IsKeyPressed(keys.backAlternate, false));
            if (backPressed)
                controller.Handle(MenuInput::Back);
        }

        static void PollKeyboard(
            SickMenu& menu,
            const ImGuiMenuKeys& keys = {})
        {
            if (ImGui::IsKeyPressed(keys.toggle, false))
                menu.Handle(MenuInput::Toggle);
            if (!menu.Controller().IsOpen())
                return;
            if (ImGui::IsKeyPressed(keys.up))
                menu.Handle(MenuInput::Up);
            if (ImGui::IsKeyPressed(keys.down))
                menu.Handle(MenuInput::Down);
            if (ImGui::IsKeyPressed(keys.left))
                menu.Handle(MenuInput::Left);
            if (ImGui::IsKeyPressed(keys.right))
                menu.Handle(MenuInput::Right);
            if (ImGui::IsKeyPressed(keys.select, false))
                menu.Handle(MenuInput::Select);
            const bool backPressed = ImGui::IsKeyPressed(keys.back, false) ||
                (keys.back == ImGuiKey_Keypad0 && ImGui::IsKeyPressed(keys.backAlternate, false));
            if (backPressed)
                menu.Handle(MenuInput::Back);
        }

        static void Submit(
            const MenuDrawList& commands,
            ImDrawList* target = nullptr,
            ImFont* font = nullptr)
        {
            target = target ? target : ImGui::GetForegroundDrawList();
            font = font ? font : ImGui::GetFont();

            for (const auto& command : commands.Commands())
            {
                const auto color = ToImColor(command.color);
                switch (command.kind)
                {
                case MenuDrawCommandKind::FilledRect:
                    target->AddRectFilled(
                        {command.bounds.left, command.bounds.top},
                        {command.bounds.right, command.bounds.bottom},
                        color);
                    break;
                case MenuDrawCommandKind::Text:
                {
                    const auto textSize = font->CalcTextSizeA(
                        command.fontSize,
                        FLT_MAX,
                        0.0F,
                        command.text.c_str());
                    float x = command.bounds.left;
                    if (command.textAlign == MenuTextAlign::Center)
                        x = (command.bounds.left + command.bounds.right - textSize.x) * 0.5F;
                    else if (command.textAlign == MenuTextAlign::Right)
                        x = command.bounds.right - textSize.x;
                    const float y = (command.bounds.top + command.bounds.bottom - textSize.y) * 0.5F;
                    target->AddText(font, command.fontSize, {x, y}, color, command.text.c_str());
                    break;
                }
                case MenuDrawCommandKind::FilledCircle:
                    target->AddCircleFilled(
                        {command.start.x, command.start.y},
                        command.radius,
                        color);
                    break;
                case MenuDrawCommandKind::Line:
                    target->AddLine(
                        {command.start.x, command.start.y},
                        {command.end.x, command.end.y},
                        color,
                        command.thickness);
                    break;
                case MenuDrawCommandKind::Image:
                    target->AddImage(
                        ToImTexture(command.texture),
                        {command.bounds.left, command.bounds.top},
                        {command.bounds.right, command.bounds.bottom});
                    break;
                }
            }
        }

        static void Render(
            SickMenu& menu,
            const ImGuiMenuKeys& keys = {},
            ImDrawList* target = nullptr,
            ImFont* font = nullptr)
        {
            PollKeyboard(menu, keys);
            const auto displaySize = ImGui::GetIO().DisplaySize;
            Submit(menu.Draw({displaySize.x, displaySize.y}), target, font);
        }

    private:
        [[nodiscard]] static ImU32 ToImColor(MenuColor color) noexcept
        {
            return IM_COL32(color.red, color.green, color.blue, color.alpha);
        }

        template <typename TextureId = ImTextureID>
        [[nodiscard]] static TextureId ToImTexture(MenuTexture texture) noexcept
        {
            if constexpr (std::is_pointer_v<TextureId>)
                return reinterpret_cast<TextureId>(texture);
            else
                return static_cast<TextureId>(texture);
        }
    };
}
