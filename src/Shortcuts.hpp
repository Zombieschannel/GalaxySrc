#pragma once
#include <SFML/Window/Keyboard.hpp>
#include "inc/ZTB.hpp"

enum class ActionShortcut : int8_t
{
    NewImage,
    SaveImage,
    OpenSettings,
    ToggleFullscreen,
    OpenAbout,
    ZoomIn,
    ZoomOut,
    SaveImageAs,
    Resize,
    ResizeCanvas,
    Copy,
    Paste,
    SelectAll,
    Delete,
    OpenImage,
    Cut,
    DeselectAll,
    TransformImage,
    Count
};

class Shortcuts
{
    unordered_map<ActionShortcut, uint32_t> shortcuts;
    static Shortcuts& get()
    {
        static Shortcuts instance(true);
        return instance;
    }
public:
    explicit Shortcuts(const bool init = false)
    {
        if (init)
        {
            for (int8_t i = 0; i < static_cast<int8_t>(ActionShortcut::Count); i++)
                shortcuts.try_emplace(static_cast<ActionShortcut>(i));

            shortcuts.at(ActionShortcut::NewImage) = (1 << 21) | static_cast<uint32_t>(Keyboard::Key::N);
            shortcuts.at(ActionShortcut::SaveImage) = (1 << 21) | static_cast<uint32_t>(Keyboard::Key::S);
            shortcuts.at(ActionShortcut::OpenSettings) = static_cast<uint32_t>(Keyboard::Key::F10);
            shortcuts.at(ActionShortcut::ToggleFullscreen) = static_cast<uint32_t>(Keyboard::Key::F11);
            shortcuts.at(ActionShortcut::OpenAbout) = static_cast<uint32_t>(Keyboard::Key::F1);
            shortcuts.at(ActionShortcut::ZoomIn) = (1 << 21) | static_cast<uint32_t>(Keyboard::Key::Equal);
            shortcuts.at(ActionShortcut::ZoomOut) = (1 << 21) | static_cast<uint32_t>(Keyboard::Key::Hyphen);
            shortcuts.at(ActionShortcut::SaveImageAs) = (1 << 21) | (1 << 23) | static_cast<uint32_t>(Keyboard::Key::S);
            shortcuts.at(ActionShortcut::Resize) = (1 << 21) | static_cast<uint32_t>(Keyboard::Key::R);
            shortcuts.at(ActionShortcut::ResizeCanvas) = (1 << 21) | (1 << 23) | static_cast<uint32_t>(Keyboard::Key::R);
            shortcuts.at(ActionShortcut::Copy) = (1 << 21) | static_cast<uint32_t>(Keyboard::Key::C);
            shortcuts.at(ActionShortcut::Paste) = (1 << 21) | static_cast<uint32_t>(Keyboard::Key::V);
            shortcuts.at(ActionShortcut::SelectAll) = (1 << 21) | static_cast<uint32_t>(Keyboard::Key::A);
            shortcuts.at(ActionShortcut::Delete) = static_cast<uint32_t>(Keyboard::Key::Delete);
            shortcuts.at(ActionShortcut::OpenImage) =  (1 << 21) | static_cast<uint32_t>(Keyboard::Key::O);
            shortcuts.at(ActionShortcut::Cut) =  (1 << 21) | static_cast<uint32_t>(Keyboard::Key::X);
            shortcuts.at(ActionShortcut::DeselectAll) =  (1 << 21) | static_cast<uint32_t>(Keyboard::Key::D);
            shortcuts.at(ActionShortcut::TransformImage) =  (1 << 21) | (1 << 23) | static_cast<uint32_t>(Keyboard::Key::T);
        }
    }
    static string getKeyName(Keyboard::Key key)
    {
        const int32_t ID = static_cast<int32_t>(key);
        switch (key)
        {
        case Keyboard::Key::Escape: return "Escape";
        case Keyboard::Key::Menu: return "Menu";
        case Keyboard::Key::LBracket: return "LBracket";
        case Keyboard::Key::RBracket: return "RBracket";
        case Keyboard::Key::Semicolon: return "Semicolon";
        case Keyboard::Key::Comma: return "Comma";
        case Keyboard::Key::Period: return "Period";
        case Keyboard::Key::Apostrophe: return "Apostrophe";
        case Keyboard::Key::Slash: return "Slash";
        case Keyboard::Key::Backslash: return "Backslash";
        case Keyboard::Key::Grave: return "Grave";
        case Keyboard::Key::Equal: return "Equal";
        case Keyboard::Key::Hyphen: return "Hyphen";
        case Keyboard::Key::Space: return "Space";
        case Keyboard::Key::Enter: return "Enter";
        case Keyboard::Key::Backspace: return "Backspace";
        case Keyboard::Key::Tab: return "Tab";
        case Keyboard::Key::PageUp: return "PageUp";
        case Keyboard::Key::PageDown: return "PageDown";
        case Keyboard::Key::End: return "End";
        case Keyboard::Key::Home: return "Home";
        case Keyboard::Key::Insert: return "Insert";
        case Keyboard::Key::Delete: return "Delete";
        case Keyboard::Key::Add: return "Add";
        case Keyboard::Key::Subtract: return "Subtract";
        case Keyboard::Key::Multiply: return "Multiply";
        case Keyboard::Key::Divide: return "Divide";
        case Keyboard::Key::Left: return "Left";
        case Keyboard::Key::Right: return "Right";
        case Keyboard::Key::Up: return "Up";
        case Keyboard::Key::Down: return "Down";
        case Keyboard::Key::Unknown: return "?";
        case Keyboard::Key::Pause: return "Pause";
        default:
            if (ID >= static_cast<int32_t>(Keyboard::Key::A) && ID <= static_cast<int32_t>(Keyboard::Key::Z))
                return string(1, static_cast<char>('A' + ID - static_cast<int32_t>(Keyboard::Key::A)));
            if (ID >= static_cast<int32_t>(Keyboard::Key::Num0) && ID <= static_cast<int32_t>(Keyboard::Key::Num9))
                return to_string(ID - static_cast<int32_t>(Keyboard::Key::Num0));
            if (ID >= static_cast<int32_t>(Keyboard::Key::Numpad0) && ID <= static_cast<int32_t>(Keyboard::Key::Numpad9))
                return "Numpad" + to_string(ID - static_cast<int32_t>(Keyboard::Key::Numpad0));
            if (ID >= static_cast<int32_t>(Keyboard::Key::F1) && ID <= static_cast<int32_t>(Keyboard::Key::F15))
                return "F" + to_string(ID - static_cast<int32_t>(Keyboard::Key::F1) + 1);
            break;
        }
        return "";
    }
    bool operator[](const ActionShortcut shortcut) const
    {
        if ((get().shortcuts.at(shortcut) & 0xFFFF) == 0xFFFF)
            return false;
        if (((get().shortcuts.at(shortcut) >> 23) & 1) == InputEvent::isShiftPressed() &&
            ((get().shortcuts.at(shortcut) >> 22) & 1) == InputEvent::isAltPressed() &&
            ((get().shortcuts.at(shortcut) >> 21) & 1) == InputEvent::isCtrlPressed() &&
            ((get().shortcuts.at(shortcut) >> 20) & 1) == InputEvent::isSystemPressed() &&
            InputEvent::isKeyHeld(static_cast<Keyboard::Key>(get().shortcuts.at(shortcut) & (0xFFFF))))
        {
            return true;
        }
        return false;
    }
    static bool validKey(const Keyboard::Key key)
    {
        switch (key)
        {
        case Keyboard::Key::LAlt: case Keyboard::Key::RAlt:
        case Keyboard::Key::LControl: case Keyboard::Key::RControl:
        case Keyboard::Key::LSystem: case Keyboard::Key::RSystem:
        case Keyboard::Key::LShift: case Keyboard::Key::RShift:
            return false;
        default: break;
        }
        return true;
    }
    static uint32_t getShortcut(const ActionShortcut shortcut)
    {
        return get().shortcuts.at(shortcut);
    }
    static void setShortcut(const ActionShortcut shortcut, const uint32_t value)
    {
        get().shortcuts.at(shortcut) = value;
    }
    static string getName(const ActionShortcut shortcut)
    {
        string x;
        if ((get().shortcuts.at(shortcut) >> 20) & 1)
            x += "System + ";
        if ((get().shortcuts.at(shortcut) >> 21) & 1)
            x += "Ctrl + ";
        if ((get().shortcuts.at(shortcut) >> 22) & 1)
            x += "Alt + ";
        if ((get().shortcuts.at(shortcut) >> 23) & 1)
            x += "Shift + ";
        if ((get().shortcuts.at(shortcut) & 0xFFFF) == 0xFFFF)
            return x;
        x += getKeyName(static_cast<Keyboard::Key>(get().shortcuts.at(shortcut) & 0xFFFF));
        return x;
    }
    static void captureShortcut(const ActionShortcut shortcut)
    {
        get().shortcuts.at(shortcut) &= 1 << 23;
        get().shortcuts.at(shortcut) &= 1 << 22;
        get().shortcuts.at(shortcut) &= 1 << 21;
        get().shortcuts.at(shortcut) &= 1 << 20;
        get().shortcuts.at(shortcut) |= InputEvent::isShiftPressed() << 23;
        get().shortcuts.at(shortcut) |= InputEvent::isAltPressed() << 22;
        get().shortcuts.at(shortcut) |= InputEvent::isCtrlPressed() << 21;
        get().shortcuts.at(shortcut) |= InputEvent::isSystemPressed() << 20;
        get().shortcuts.at(shortcut) |= 0xFFFF;
        for (uint8_t i = 0; i < Keyboard::KeyCount; i++)
        {
            if (!validKey(static_cast<Keyboard::Key>(i)))
                continue;
            if (InputEvent::isKeyHeld(static_cast<Keyboard::Key>(i)))
            {
                get().shortcuts.at(shortcut) &= ~(0xFFFF);
                get().shortcuts.at(shortcut) |= i;
            }
        }
    }
    static void clear(const ActionShortcut shortcut)
    {
        get().shortcuts.at(shortcut) = 0xFFFF;
    }
};