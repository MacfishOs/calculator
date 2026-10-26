// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "ui/view.h"

#include <functional>
#include <string>
#include <vector>

namespace cutefish::ui {
class Icon;
class Label;
}

// A key of the keypad: a rounded plate inset by half its radius in its cell,
// with a label or a glyph.
class KeypadButton : public cutefish::ui::View
{
public:
    enum class Kind {
        Plain,
        // Its label in the highlight colour.
        Operator,
        // On the highlight colour.
        Accent,
    };

    KeypadButton(const std::string& text, Kind kind, const std::string& glyph = {});

    bool isPressed() const { return m_pressed; }

    bool acceptsPointer() const override { return true; }

    std::function<void()> onClicked;

protected:
    void layout() override;
    void paint(cutefish::ui::Painter& painter) override;
    void pointerEnter() override;
    void pointerLeave() override;
    void pointerPress(const cutefish::ui::PointerEvent& event) override;
    void pointerRelease(const cutefish::ui::PointerEvent& event) override;

private:
    Kind m_kind;
    cutefish::ui::Label* m_label = nullptr;
    cutefish::ui::Icon* m_icon = nullptr;
    bool m_hovered = false;
    bool m_pressed = false;
};

// The standard keypad: four columns of keys on the keypad's own background.
class Keypad : public cutefish::ui::View
{
public:
    // What the keys send: the digits and operators as they read, "BACKSPACE"
    // for the glyph key.
    static constexpr const char* Backspace = "BACKSPACE";

    Keypad();

    const std::vector<KeypadButton*>& buttons() const { return m_buttons; }
    KeypadButton* button(const std::string& key) const;

    std::function<void(const std::string& key)> onPressed;

protected:
    void layout() override;
    void paint(cutefish::ui::Painter& painter) override;

private:
    std::vector<KeypadButton*> m_buttons;
    std::vector<std::string> m_keys;
};
