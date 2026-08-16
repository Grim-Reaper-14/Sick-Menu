#include "Menu.hpp"

#include <algorithm>
#include <utility>

namespace Sick::Ui
{
    MenuOption::MenuOption(MenuOptionKind kind, std::string label, bool selectable)
        : m_Kind(kind),
          m_Label(std::move(label)),
          m_Selectable(selectable)
    {
    }

    MenuOption MenuOption::Action(std::string label, ActionCallback callback)
    {
        MenuOption option{MenuOptionKind::Action, std::move(label)};
        option.m_Activate = std::move(callback);
        return option;
    }

    MenuOption MenuOption::Toggle(
        std::string label,
        bool& value,
        ToggleCallback callback)
    {
        MenuOption option{MenuOptionKind::Toggle, std::move(label)};
        const auto activateCallback = callback;
        option.m_Activate = [&value, activateCallback]() {
            value = !value;
            if (activateCallback)
                activateCallback(value);
        };
        option.m_Adjust = [&value, callback = std::move(callback)](int direction) {
            const bool next = direction > 0;
            if (value == next)
                return;

            value = next;
            if (callback)
                callback(value);
        };
        option.m_ToggleValue = [&value]() { return value; };
        return option;
    }

    MenuOption MenuOption::Integer(
        std::string label,
        int& value,
        int minimum,
        int maximum,
        int step,
        IntegerCallback callback)
    {
        if (minimum > maximum)
            std::swap(minimum, maximum);
        step = std::max(step, 1);
        value = std::clamp(value, minimum, maximum);

        MenuOption option{MenuOptionKind::Integer, std::move(label)};
        option.m_Adjust = [&value, minimum, maximum, step, callback = std::move(callback)](int direction) {
            if (direction == 0)
                return;

            const auto delta = static_cast<long long>(step) * (direction > 0 ? 1LL : -1LL);
            const auto candidate = std::clamp(
                static_cast<long long>(value) + delta,
                static_cast<long long>(minimum),
                static_cast<long long>(maximum));
            const auto next = static_cast<int>(candidate);
            if (next == value)
                return;

            value = next;
            if (callback)
                callback(value);
        };
        option.m_ValueText = [&value]() { return std::to_string(value); };
        return option;
    }

    MenuOption MenuOption::Choice(
        std::string label,
        std::size_t& index,
        std::vector<std::string> values,
        ChoiceCallback callback)
    {
        auto sharedValues = std::make_shared<std::vector<std::string>>(std::move(values));
        if (sharedValues->empty())
            index = 0;
        else if (index >= sharedValues->size())
            index = sharedValues->size() - 1;

        MenuOption option{MenuOptionKind::Choice, std::move(label)};
        const auto activateCallback = callback;
        option.m_Adjust = [&index, sharedValues, callback = std::move(callback)](int direction) {
            if (direction == 0 || sharedValues->empty())
                return;

            if (direction > 0)
                index = (index + 1) % sharedValues->size();
            else
                index = (index + sharedValues->size() - 1) % sharedValues->size();

            if (callback)
                callback(index);
        };
        option.m_Activate = [&optionIndex = index, sharedValues, activateCallback]() {
            if (sharedValues->empty())
                return;

            optionIndex = (optionIndex + 1) % sharedValues->size();
            if (activateCallback)
                activateCallback(optionIndex);
        };
        option.m_ValueText = [&index, sharedValues]() {
            if (sharedValues->empty())
                return std::string{};

            return (*sharedValues)[index] + " [ " + std::to_string(index + 1) +
                " / " + std::to_string(sharedValues->size()) + " ]";
        };
        option.m_Enabled = [sharedValues]() { return !sharedValues->empty(); };
        return option;
    }

    MenuOption MenuOption::Submenu(std::string label, MenuPage& page)
    {
        MenuOption option{MenuOptionKind::Submenu, std::move(label)};
        option.m_ChildPage = &page;
        option.m_ValueText = []() { return std::string{">"}; };
        return option;
    }

    MenuOption MenuOption::Label(std::string label)
    {
        return MenuOption{MenuOptionKind::Label, std::move(label), false};
    }

    MenuOption& MenuOption::EnabledWhen(EnabledPredicate predicate)
    {
        m_Enabled = std::move(predicate);
        return *this;
    }

    MenuOption& MenuOption::Describe(std::string description)
    {
        m_Description = std::move(description);
        return *this;
    }

    MenuOptionKind MenuOption::Kind() const noexcept
    {
        return m_Kind;
    }

    std::string_view MenuOption::LabelText() const noexcept
    {
        return m_Label;
    }

    std::string_view MenuOption::Description() const noexcept
    {
        return m_Description;
    }

    bool MenuOption::Enabled() const
    {
        return !m_Enabled || m_Enabled();
    }

    bool MenuOption::Selectable() const noexcept
    {
        return m_Selectable;
    }

    bool MenuOption::ToggleValue() const
    {
        return m_ToggleValue && m_ToggleValue();
    }

    std::string MenuOption::ValueText() const
    {
        return m_ValueText ? m_ValueText() : std::string{};
    }

    MenuPage* MenuOption::ChildPage() const noexcept
    {
        return m_ChildPage;
    }

    void MenuOption::Activate()
    {
        if (Enabled() && m_Activate)
            m_Activate();
    }

    void MenuOption::Adjust(int direction)
    {
        if (Enabled() && m_Adjust)
            m_Adjust(direction);
    }

    MenuPage::MenuPage(std::string title)
        : m_Title(std::move(title))
    {
    }

    MenuOption& MenuPage::Add(MenuOption option)
    {
        m_Options.push_back(std::move(option));
        return m_Options.back();
    }

    MenuOption& MenuPage::AddAction(std::string label, MenuOption::ActionCallback callback)
    {
        return Add(MenuOption::Action(std::move(label), std::move(callback)));
    }

    MenuOption& MenuPage::AddToggle(
        std::string label,
        bool& value,
        MenuOption::ToggleCallback callback)
    {
        return Add(MenuOption::Toggle(std::move(label), value, std::move(callback)));
    }

    MenuOption& MenuPage::AddInteger(
        std::string label,
        int& value,
        int minimum,
        int maximum,
        int step,
        MenuOption::IntegerCallback callback)
    {
        return Add(MenuOption::Integer(
            std::move(label), value, minimum, maximum, step, std::move(callback)));
    }

    MenuOption& MenuPage::AddChoice(
        std::string label,
        std::size_t& index,
        std::vector<std::string> values,
        MenuOption::ChoiceCallback callback)
    {
        return Add(MenuOption::Choice(
            std::move(label), index, std::move(values), std::move(callback)));
    }

    MenuOption& MenuPage::AddSubmenu(std::string label, MenuPage& page)
    {
        return Add(MenuOption::Submenu(std::move(label), page));
    }

    MenuOption& MenuPage::AddLabel(std::string label)
    {
        return Add(MenuOption::Label(std::move(label)));
    }

    std::string_view MenuPage::Title() const noexcept
    {
        return m_Title;
    }

    const std::vector<MenuOption>& MenuPage::Options() const noexcept
    {
        return m_Options;
    }

    std::vector<MenuOption>& MenuPage::Options() noexcept
    {
        return m_Options;
    }

    MenuController::MenuController(MenuPage& root)
    {
        SetRoot(root);
    }

    void MenuController::SetRoot(MenuPage& root)
    {
        m_Stack.clear();
        m_Stack.push_back(PageFrame{&root});
        NormalizeSelection(m_Stack.back());
    }

    void MenuController::Open()
    {
        if (!m_Stack.empty())
        {
            NormalizeSelection(m_Stack.back());
            m_Open = true;
        }
    }

    void MenuController::Close() noexcept
    {
        m_Open = false;
    }

    void MenuController::Toggle() noexcept
    {
        m_Open = !m_Open && !m_Stack.empty();
    }

    bool MenuController::IsOpen() const noexcept
    {
        return m_Open;
    }

    bool MenuController::Handle(MenuInput input)
    {
        if (input == MenuInput::Toggle)
        {
            Toggle();
            return true;
        }

        if (!m_Open || m_Stack.empty())
            return false;

        switch (input)
        {
        case MenuInput::Up:
            return Move(-1);
        case MenuInput::Down:
            return Move(1);
        case MenuInput::Left:
        case MenuInput::Right:
            if (auto* option = SelectedOption())
            {
                option->Adjust(input == MenuInput::Right ? 1 : -1);
                return true;
            }
            return false;
        case MenuInput::Select:
            if (auto* option = SelectedOption())
            {
                if (auto* child = option->ChildPage())
                {
                    m_Stack.push_back(PageFrame{child});
                    NormalizeSelection(m_Stack.back());
                }
                else
                {
                    option->Activate();
                }
                return true;
            }
            return false;
        case MenuInput::Back:
            if (m_Stack.size() > 1)
            {
                m_Stack.pop_back();
                EnsureSelectionVisible(m_Stack.back());
            }
            else
            {
                Close();
            }
            return true;
        case MenuInput::Toggle:
            break;
        }

        return false;
    }

    bool MenuController::SelectOption(std::size_t optionIndex)
    {
        if (m_Stack.empty())
            return false;

        auto& frame = m_Stack.back();
        auto& options = frame.page->Options();
        if (optionIndex >= options.size() || !CanSelect(options[optionIndex]))
            return false;

        frame.selected = optionIndex;
        EnsureSelectionVisible(frame);
        return true;
    }

    void MenuController::SetVisibleRows(std::size_t rows)
    {
        m_VisibleRows = std::max<std::size_t>(rows, 1);
        if (!m_Stack.empty())
            EnsureSelectionVisible(m_Stack.back());
    }

    std::size_t MenuController::VisibleRows() const noexcept
    {
        return m_VisibleRows;
    }

    std::size_t MenuController::VisibleStart() const noexcept
    {
        return m_Stack.empty() ? 0 : m_Stack.back().firstVisible;
    }

    std::size_t MenuController::SelectedOptionIndex() const noexcept
    {
        return m_Stack.empty() ? NoSelection : m_Stack.back().selected;
    }

    std::size_t MenuController::Depth() const noexcept
    {
        return m_Stack.size();
    }

    MenuSelectionCounter MenuController::SelectionCounter() const noexcept
    {
        MenuSelectionCounter counter{};
        if (m_Stack.empty())
            return counter;

        const auto& frame = m_Stack.back();
        const auto& options = frame.page->Options();
        for (std::size_t index = 0; index < options.size(); ++index)
        {
            if (!options[index].Selectable())
                continue;

            ++counter.total;
            if (index == frame.selected)
                counter.current = counter.total;
        }
        return counter;
    }

    MenuPage* MenuController::CurrentPage() noexcept
    {
        return m_Stack.empty() ? nullptr : m_Stack.back().page;
    }

    const MenuPage* MenuController::CurrentPage() const noexcept
    {
        return m_Stack.empty() ? nullptr : m_Stack.back().page;
    }

    MenuOption* MenuController::SelectedOption() noexcept
    {
        if (m_Stack.empty())
            return nullptr;

        auto& frame = m_Stack.back();
        auto& options = frame.page->Options();
        return frame.selected < options.size() ? &options[frame.selected] : nullptr;
    }

    const MenuOption* MenuController::SelectedOption() const noexcept
    {
        if (m_Stack.empty())
            return nullptr;

        const auto& frame = m_Stack.back();
        const auto& options = frame.page->Options();
        return frame.selected < options.size() ? &options[frame.selected] : nullptr;
    }

    bool MenuController::CanSelect(const MenuOption& option)
    {
        return option.Selectable() && option.Enabled();
    }

    void MenuController::NormalizeSelection(PageFrame& frame)
    {
        if (!frame.page)
        {
            frame.selected = NoSelection;
            return;
        }

        const auto& options = frame.page->Options();
        if (frame.selected < options.size() && CanSelect(options[frame.selected]))
        {
            EnsureSelectionVisible(frame);
            return;
        }

        frame.selected = NoSelection;
        for (std::size_t index = 0; index < options.size(); ++index)
        {
            if (CanSelect(options[index]))
            {
                frame.selected = index;
                break;
            }
        }
        EnsureSelectionVisible(frame);
    }

    bool MenuController::Move(int direction)
    {
        if (direction == 0 || m_Stack.empty())
            return false;

        auto& frame = m_Stack.back();
        const auto& options = frame.page->Options();
        if (options.empty())
            return false;

        if (frame.selected == NoSelection)
        {
            NormalizeSelection(frame);
            return frame.selected != NoSelection;
        }

        const auto original = frame.selected;
        auto candidate = original;
        for (std::size_t attempt = 0; attempt < options.size(); ++attempt)
        {
            if (direction > 0)
                candidate = (candidate + 1) % options.size();
            else
                candidate = (candidate + options.size() - 1) % options.size();

            if (CanSelect(options[candidate]))
            {
                frame.selected = candidate;
                EnsureSelectionVisible(frame);
                return candidate != original;
            }
        }

        return false;
    }

    void MenuController::EnsureSelectionVisible(PageFrame& frame)
    {
        if (!frame.page)
            return;

        const auto optionCount = frame.page->Options().size();
        if (optionCount <= m_VisibleRows)
        {
            frame.firstVisible = 0;
            return;
        }

        if (frame.selected != NoSelection)
        {
            if (frame.selected < frame.firstVisible)
                frame.firstVisible = frame.selected;
            else if (frame.selected >= frame.firstVisible + m_VisibleRows)
                frame.firstVisible = frame.selected - m_VisibleRows + 1;
        }

        frame.firstVisible = std::min(frame.firstVisible, optionCount - m_VisibleRows);
    }
}
