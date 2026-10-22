// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstdint>

#include "ui/color_provider.h"

// The calculator's own colours, after the toolkit's; colors.cpp gives them
// for each theme.
enum class CalculatorColor : uint16_t {
    // Translucent over the window's blur.
    WindowBackground = uint16_t(cutefish::ui::ColorId::ApplicationFirst),
    KeypadBackground,
    Key,
    KeyHovered,
    KeyPressed,
    // The = key.
    AccentKey,
    AccentKeyHovered,
    AccentKeyPressed,
};

void addCalculatorColors(cutefish::ui::ColorMixer& mixer, const cutefish::ui::Theme& theme);

// Added before main by every program that names one of the colours.
[[maybe_unused]] inline const bool calculatorColorsAdded =
    (cutefish::ui::ColorProvider::addMixer(addCalculatorColors), true);
