// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "colors.h"

#include "ui/theme.h"

using namespace cutefish::ui;

void addCalculatorColors(ColorMixer& mixer, const Theme& theme)
{
    const bool dark = theme.darkMode;
    mixer[CalculatorColor::WindowBackground] = (dark ? Color::rgb(0x2E2E2E) : Color::rgb(0xF0EEF1)).withAlpha(0.95);
    mixer[CalculatorColor::KeypadBackground] = withAlpha(ColorId::SecondBackground, 0.9);
    mixer[CalculatorColor::Key] = withAlpha(ColorId::Background, 0.7);
    mixer[CalculatorColor::KeyHovered] =
        withAlpha(dark ? lighter(ColorId::Background, 1.7) : darker(ColorId::Background, 1.1), 0.6);
    mixer[CalculatorColor::KeyPressed] =
        withAlpha(dark ? lighter(ColorId::Background, 1.4) : darker(ColorId::Background, 1.3), 0.6);
    mixer[CalculatorColor::AccentKey] = ColorId::Highlight;
    mixer[CalculatorColor::AccentKeyHovered] = dark ? lighter(ColorId::Highlight, 1.1) : darker(ColorId::Highlight, 1.1);
    mixer[CalculatorColor::AccentKeyPressed] = dark ? lighter(ColorId::Highlight, 1.05) : darker(ColorId::Highlight, 1.3);
}
