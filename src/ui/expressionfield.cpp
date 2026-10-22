// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "expressionfield.h"

#include "ui/label.h"
#include "ui/text_field.h"
#include "ui/theme.h"
#include "ui/window.h"

#include <algorithm>
#include <cmath>

using namespace cutefish::ui;

ExpressionField::ExpressionField()
    : m_field(addChild<TextField>())
    , m_measure(addChild<Label>())
    , m_figures(addChild<Label>("7"))
{
    setName("expressionRow");
    m_field->setName("expression");
    m_field->setFrameVisible(false);
    m_field->setFontSize(FontSize);
    m_field->onTextChanged = [this] {
        m_measure->setText(m_field->text());
        requestLayout();
    };
    m_measure->setFontSize(FontSize);
    m_measure->setVisible(false);
    m_figures->setFontSize(FontSize);
    m_figures->setVisible(false);
}

double ExpressionField::baseline(double height) const
{
    // The field centres its line box and puts the baseline on the nearest device row.
    const double scale = window() ? window()->scale() : 1;
    return std::round(((height - m_measure->sizeHint().height) / 2 + m_measure->baselineOffset()) * scale) / scale;
}

RectF ExpressionField::figureInk() const
{
    return m_figures->inkBoundingRect();
}

void ExpressionField::layout()
{
    const double margin = Theme::current().largeSpacing;
    m_field->setTextMargins(margin, margin);
    const double width = geometry().width;
    // The text, the margins and room for the caret after the text.
    const double text = m_field->text().empty() ? 0 : m_measure->sizeHint().width;
    const double fieldWidth = std::min(width, std::ceil(text + 2 * margin + 1));
    m_field->setGeometry({width - fieldWidth, 0, fieldWidth, geometry().height});
}

void ExpressionField::pointerPress(const PointerEvent& event)
{
    if (event.button != MouseButton::Left)
        return;
    m_field->setFocus(FocusReason::Mouse);
    m_field->setCursorPosition(0);
}
