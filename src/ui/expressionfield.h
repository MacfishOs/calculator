// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "ui/view.h"

namespace cutefish::ui {
class Label;
class TextField;
}

// The expression being typed, in large text against the right edge: a text
// field without a frame, as wide as its text until it fills the row.
class ExpressionField : public cutefish::ui::View
{
public:
    static constexpr double FontSize = 24;

    ExpressionField();

    cutefish::ui::TextField* textField() const { return m_field; }
    // The text's baseline below the top, in a row of the height.
    double baseline(double height) const;
    // The ink of the figures around the baseline.
    cutefish::ui::RectF figureInk() const;

    bool acceptsPointer() const override { return true; }

protected:
    void layout() override;
    // A press left of the text puts the cursor at its start.
    void pointerPress(const cutefish::ui::PointerEvent& event) override;

private:
    cutefish::ui::TextField* m_field;
    // Measure the text and the figures; never shown.
    cutefish::ui::Label* m_measure;
    cutefish::ui::Label* m_figures;
};
