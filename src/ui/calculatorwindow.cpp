// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "calculatorwindow.h"

#include "colors.h"
#include "expressionfield.h"
#include "historyview.h"
#include "keypad.h"

#include "base/i18n.h"
#include "ui/application.h"
#include "ui/text_field.h"
#include "ui/theme.h"
#include "ui/window_header.h"

#include <algorithm>
#include <cmath>

using namespace cutefish::ui;
using cutefish::i18n::tr;

// The history and the expression take this share of the height under the header.
static constexpr double kDisplayShare = 0.35;
static constexpr double kExpressionHeight = 50;

class CalculatorWindow::Root : public View
{
public:
    Root()
        : header(addChild<WindowHeader>(HeaderHeight))
        , history(addChild<HistoryView>())
        , expression(addChild<ExpressionField>())
        , keypad(addChild<Keypad>())
    {
    }

    bool acceptsPointer() const override { return false; }

    WindowHeader* header;
    HistoryView* history;
    ExpressionField* expression;
    Keypad* keypad;

protected:
    void layout() override
    {
        const double width = geometry().width;
        const double height = std::max(0.0, geometry().height - HeaderHeight);
        const double display = std::round(height * kDisplayShare);
        const double spacing = Theme::current().smallSpacing;
        const double expressionTop = HeaderHeight + display - kExpressionHeight;
        header->setGeometry({0, 0, width, HeaderHeight});
        expression->setGeometry({0, expressionTop, width, kExpressionHeight});

        // The last calculation's ink ends as far above the expression's
        // figures as the expression's row reaches below them.
        const double baseline = expression->baseline(kExpressionHeight);
        const RectF figures = expression->figureInk();
        const double below = kExpressionHeight - baseline - figures.bottom();
        const double historyBaseline = expressionTop + baseline + figures.y - below - history->figureInkBelowBaseline();
        history->setGeometry({0, HeaderHeight, width,
                              std::max(0.0, historyBaseline + history->descentBelowBaseline() - HeaderHeight)});
        keypad->setGeometry({0, HeaderHeight + display + spacing, width, std::max(0.0, height - display - spacing)});

        if (Window* window = this->window())
            window->setBackgroundBlur(localRect(), Theme::current().windowRadius);
    }
};

CalculatorWindow::CalculatorWindow()
{
    m_window.resize({350, 550});
    m_window.setMinimumSize({350, 550});
    m_window.setRestoresSize("Main");
    m_window.setBackgroundColor(CalculatorColor::WindowBackground);
    auto root = std::make_unique<Root>();
    m_root = root.get();
    m_window.setContentView(std::move(root));

    m_root->keypad->onPressed = [this](const std::string& key) { press(key); };
    TextField* field = expression();
    field->onAccepted = [this] { press("="); };
    field->setFocus();

    retranslate();
    m_languageChanged = Application::instance()->languageChanged.connect([this] { retranslate(); });
}

CalculatorWindow::~CalculatorWindow() = default;

TextField* CalculatorWindow::expression() const
{
    return m_root->expression->textField();
}

ExpressionField* CalculatorWindow::expressionField() const
{
    return m_root->expression;
}

HistoryView* CalculatorWindow::history() const
{
    return m_root->history;
}

Keypad* CalculatorWindow::keypad() const
{
    return m_root->keypad;
}

void CalculatorWindow::press(const std::string& key)
{
    TextField* field = expression();
    const std::string& text = field->text();
    const size_t cursor = field->cursorPosition();
    // The other end of the selection: the selected text lies on one side of the cursor.
    size_t anchor = cursor;
    if (field->hasSelection()) {
        const std::string selected = field->selectedText();
        anchor = text.compare(cursor, selected.size(), selected) == 0 ? cursor + selected.size() : cursor - selected.size();
    }

    if (key == "=") {
        if (const std::optional<std::string> result = m_calculator.calculate(text)) {
            field->setText(*result);
            m_root->history->setEntries(m_calculator.history());
        }
    } else if (key == "C") {
        if (!text.empty()) {
            field->clear();
        } else {
            m_calculator.clearHistory();
            m_root->history->setEntries(m_calculator.history());
        }
    } else if (key == Keypad::Backspace) {
        edit(Calculator::backspace(text, anchor, cursor));
    } else if (key == "()") {
        edit(Calculator::insert(text, anchor, cursor, Calculator::parenthesis(text, std::min(anchor, cursor))));
    } else {
        edit(Calculator::insert(text, anchor, cursor, key));
    }
    field->setFocus();
}

void CalculatorWindow::edit(const Calculator::Edit& edit)
{
    TextField* field = expression();
    field->setText(edit.text);
    field->setCursorPosition(edit.cursor);
}

void CalculatorWindow::retranslate()
{
    m_window.setTitle(tr("Calculator"));
}
