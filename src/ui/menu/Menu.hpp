#pragma once

#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Sick::Ui
{
    class MenuPage;

    enum class MenuInput
    {
        Toggle,
        Up,
        Down,
        Left,
        Right,
        Select,
        Back
    };

    enum class MenuOptionKind
    {
        Action,
        Toggle,
        Integer,
        Float,
        Choice,
        Submenu,
        Label,
        Info
    };

    class MenuOption final
    {
    public:
        using ActionCallback = std::function<void()>;
        using ToggleCallback = std::function<void(bool)>;
        using IntegerCallback = std::function<void(int)>;
        using FloatCallback = std::function<void(float)>;
        using ChoiceCallback = std::function<void(std::size_t)>;
        using ValueCallback = std::function<std::string()>;
        using EnabledPredicate = std::function<bool()>;

        static MenuOption Action(std::string label, ActionCallback callback = {});
        static MenuOption Toggle(std::string label, bool& value, ToggleCallback callback = {});
        static MenuOption Integer(
            std::string label,
            int& value,
            int minimum,
            int maximum,
            int step = 1,
            IntegerCallback callback = {});
        static MenuOption Float(
            std::string label,
            float& value,
            float minimum,
            float maximum,
            float step,
            int precision = 2,
            FloatCallback callback = {});
        static MenuOption Choice(
            std::string label,
            std::size_t& index,
            std::vector<std::string> values,
            ChoiceCallback callback = {});
        static MenuOption Submenu(std::string label, MenuPage& page);
        static MenuOption Label(std::string label);
        static MenuOption Info(std::string label, ValueCallback value);

        MenuOption& EnabledWhen(EnabledPredicate predicate);
        MenuOption& Describe(std::string description);

        [[nodiscard]] MenuOptionKind Kind() const noexcept;
        [[nodiscard]] std::string_view LabelText() const noexcept;
        [[nodiscard]] std::string_view Description() const noexcept;
        [[nodiscard]] bool Enabled() const;
        [[nodiscard]] bool Selectable() const noexcept;
        [[nodiscard]] bool ToggleValue() const;
        [[nodiscard]] std::string ValueText() const;
        [[nodiscard]] MenuPage* ChildPage() const noexcept;

        void Activate();
        void Adjust(int direction);

    private:
        explicit MenuOption(MenuOptionKind kind, std::string label, bool selectable = true);

        MenuOptionKind m_Kind{MenuOptionKind::Action};
        std::string m_Label;
        std::string m_Description;
        bool m_Selectable{true};
        ActionCallback m_Activate;
        std::function<void(int)> m_Adjust;
        std::function<bool()> m_ToggleValue;
        ValueCallback m_ValueText;
        EnabledPredicate m_Enabled;
        MenuPage* m_ChildPage{};
    };

    class MenuPage final
    {
    public:
        explicit MenuPage(std::string title);

        MenuOption& Add(MenuOption option);
        MenuOption& AddAction(std::string label, MenuOption::ActionCallback callback = {});
        MenuOption& AddToggle(std::string label, bool& value, MenuOption::ToggleCallback callback = {});
        MenuOption& AddInteger(
            std::string label,
            int& value,
            int minimum,
            int maximum,
            int step = 1,
            MenuOption::IntegerCallback callback = {});
        MenuOption& AddFloat(
            std::string label,
            float& value,
            float minimum,
            float maximum,
            float step,
            int precision = 2,
            MenuOption::FloatCallback callback = {});
        MenuOption& AddChoice(
            std::string label,
            std::size_t& index,
            std::vector<std::string> values,
            MenuOption::ChoiceCallback callback = {});
        MenuOption& AddSubmenu(std::string label, MenuPage& page);
        MenuOption& AddLabel(std::string label);
        MenuOption& AddInfo(std::string label, MenuOption::ValueCallback value);

        [[nodiscard]] std::string_view Title() const noexcept;
        [[nodiscard]] const std::vector<MenuOption>& Options() const noexcept;
        [[nodiscard]] std::vector<MenuOption>& Options() noexcept;

    private:
        std::string m_Title;
        std::vector<MenuOption> m_Options;
    };

    struct MenuSelectionCounter
    {
        std::size_t current{};
        std::size_t total{};
    };

    class MenuController final
    {
    public:
        static constexpr std::size_t NoSelection = std::numeric_limits<std::size_t>::max();

        MenuController() = default;
        explicit MenuController(MenuPage& root);

        void SetRoot(MenuPage& root);
        void Open();
        void Close() noexcept;
        void Toggle() noexcept;
        [[nodiscard]] bool IsOpen() const noexcept;

        bool Handle(MenuInput input);
        bool SelectOption(std::size_t optionIndex);

        void SetVisibleRows(std::size_t rows);
        [[nodiscard]] std::size_t VisibleRows() const noexcept;
        [[nodiscard]] std::size_t VisibleStart() const noexcept;
        [[nodiscard]] std::size_t SelectedOptionIndex() const noexcept;
        [[nodiscard]] std::size_t Depth() const noexcept;
        [[nodiscard]] MenuSelectionCounter SelectionCounter() const noexcept;
        [[nodiscard]] MenuPage* CurrentPage() noexcept;
        [[nodiscard]] const MenuPage* CurrentPage() const noexcept;
        [[nodiscard]] MenuOption* SelectedOption() noexcept;
        [[nodiscard]] const MenuOption* SelectedOption() const noexcept;

    private:
        struct PageFrame
        {
            MenuPage* page{};
            std::size_t selected{NoSelection};
            std::size_t firstVisible{};
        };

        [[nodiscard]] static bool CanSelect(const MenuOption& option);
        void NormalizeSelection(PageFrame& frame);
        bool Move(int direction);
        void EnsureSelectionVisible(PageFrame& frame);

        std::vector<PageFrame> m_Stack;
        std::size_t m_VisibleRows{8};
        bool m_Open{};
    };
}
