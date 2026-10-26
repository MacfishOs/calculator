// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "keypad.h"

#include "colors.h"

#include "ui/icon.h"
#include "ui/label.h"
#include "ui/painter.h"
#include "ui/theme.h"

#include <cmath>

using namespace cutefish::ui;

static constexpr int kColumns = 4;
static constexpr double kKeySpacing = 1;

namespace {

struct KeyInfo
{
    const char* key;
    KeypadButton::Kind kind;
};

} // namespace

static const KeyInfo kKeys[] = {
    {"C", KeypadButton::Kind::Plain},      {Keypad::Backspace, KeypadButton::Kind::Plain},
    {"%", KeypadButton::Kind::Plain},      {"÷", KeypadButton::Kind::Operator},
    {"7", KeypadButton::Kind::Plain},      {"8", KeypadButton::Kind::Plain},
    {"9", KeypadButton::Kind::Plain},      {"×", KeypadButton::Kind::Operator},
    {"4", KeypadButton::Kind::Plain},      {"5", KeypadButton::Kind::Plain},
    {"6", KeypadButton::Kind::Plain},      {"-", KeypadButton::Kind::Operator},
    {"1", KeypadButton::Kind::Plain},      {"2", KeypadButton::Kind::Plain},
    {"3", KeypadButton::Kind::Plain},      {"+", KeypadButton::Kind::Operator},
    {"0", KeypadButton::Kind::Plain},      {".", KeypadButton::Kind::Plain},
    {"()", KeypadButton::Kind::Plain},     {"=", KeypadButton::Kind::Accent},
};

KeypadButton::KeypadButton(const std::string& text, Kind kind, const std::string& glyph)
    : m_kind(kind)
{
    if (!glyph.empty()) {
        m_icon = addChild<Icon>(glyph);
        m_icon->setColor(ColorId::Text);
        return;
    }
    m_label = addChild<Label>(text);
    m_label->setTextAlignment(Label::TextAlignment::Center);
    m_label->setVerticallyCentered(true);
    m_label->setColor(kind == Kind::Accent ? ColorId::HighlightedText : kind == Kind::Operator ? ColorId::Highlight : ColorId::Text);
}

void KeypadButton::layout()
{
    const double height = geometry().height;
    if (m_label) {
        // A point size of a fifth of the height, in pixels at 96 dpi.
        m_label->setFontSize(std::round(height / 5) * 4 / 3);
        m_label->setGeometry(localRect());
    }
    if (m_icon) {
        const double size = height * 0.45;
        m_icon->setSize(size);
        m_icon->setGeometry({(geometry().width - size) / 2, (height - size) / 2, size, size});
    }
}

void KeypadButton::paint(Painter& painter)
{
    const double radius = Theme::current().smallRadius;
    const RectF plate = localRect().adjusted(radius / 2, radius / 2, -radius / 2, -radius / 2);
    CalculatorColor id;
    if (m_kind == Kind::Accent)
        id = m_pressed ? CalculatorColor::AccentKeyPressed : m_hovered ? CalculatorColor::AccentKeyHovered : CalculatorColor::AccentKey;
    else
        id = m_pressed ? CalculatorColor::KeyPressed : m_hovered ? CalculatorColor::KeyHovered : CalculatorColor::Key;
    painter.drawRoundedRect(plate, {radius}, color(id));
}

void KeypadButton::pointerEnter()
{
    m_hovered = true;
    requestPaint();
}

void KeypadButton::pointerLeave()
{
    m_hovered = false;
    requestPaint();
}

void KeypadButton::pointerPress(const PointerEvent& event)
{
    if (event.button != MouseButton::Left)
        return;
    m_pressed = true;
    requestPaint();
}

void KeypadButton::pointerRelease(const PointerEvent& event)
{
    if (!m_pressed)
        return;
    m_pressed = false;
    requestPaint();
    if (localRect().contains(event.position) && onClicked)
        onClicked();
}

Keypad::Keypad()
{
    for (const KeyInfo& info : kKeys) {
        const std::string key = info.key;
        const bool backspace = key == Backspace;
        KeypadButton* button = addChild<KeypadButton>(backspace ? std::string() : key, info.kind,
                                                      backspace ? "backspace" : std::string());
        button->setName("key " + key);
        button->onClicked = [this, key] {
            if (onPressed)
                onPressed(key);
        };
        m_buttons.push_back(button);
        m_keys.push_back(key);
    }
}

KeypadButton* Keypad::button(const std::string& key) const
{
    for (size_t i = 0; i < m_keys.size(); ++i) {
        if (m_keys[i] == key)
            return m_buttons[i];
    }
    return nullptr;
}

void Keypad::layout()
{
    // Cells of equal size inside the margins; the plates snap to the device
    // grid as they paint, so no key comes out a whole pixel narrower.
    const double margin = Theme::current().smallSpacing;
    const int rows = int(m_buttons.size() + kColumns - 1) / kColumns;
    const double width = std::max(0.0, geometry().width - 2 * margin);
    const double height = std::max(0.0, geometry().height - 2 * margin);
    const double cellWidth = std::max(0.0, (width - (kColumns - 1) * kKeySpacing) / kColumns);
    const double cellHeight = std::max(0.0, (height - (rows - 1) * kKeySpacing) / rows);
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        const int column = int(i) % kColumns;
        const int row = int(i) / kColumns;
        m_buttons[i]->setGeometry({margin + column * (cellWidth + kKeySpacing), margin + row * (cellHeight + kKeySpacing),
                                   cellWidth, cellHeight});
    }
}

void Keypad::paint(Painter& painter)
{
    painter.fillRect(localRect(), color(CalculatorColor::KeypadBackground));
}
