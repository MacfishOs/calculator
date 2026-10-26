// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "evaluator.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The calculator apart from its window: what = and C do, the history of
// calculations, and the edits the keypad makes to the expression.
class Calculator
{
public:
    // Results show at most this many decimals.
    static constexpr int DisplayDecimals = 10;

    // The result as shown, or nothing when the expression has none. Adds
    // "expression = result" to the history unless the two are the same. An
    // expression that starts with the last result shown goes on from its
    // full value, so 1 ÷ 3 = then × 3 gives 1.
    std::optional<std::string> calculate(const std::string& expression);
    const Evaluator::Result& lastResult() const { return m_lastResult; }

    const std::vector<std::string>& history() const { return m_history; }
    void clearHistory();

    Evaluator& evaluator() { return m_evaluator; }

    static std::string format(const Decimal& value);

    // The expression and the cursor after an edit, as UTF-8 byte offsets.
    struct Edit
    {
        std::string text;
        size_t cursor = 0;
    };
    // The insertion replaces the selection between the two positions, in either order.
    static Edit insert(const std::string& text, size_t anchor, size_t cursor, std::string_view insertion);
    // Removes the selection, or the character before the cursor.
    static Edit backspace(const std::string& text, size_t anchor, size_t cursor);
    // What the () key types at the cursor: ")" where it closes an open
    // parenthesis after an operand, "(" otherwise.
    static std::string parenthesis(const std::string& text, size_t cursor);

private:
    Evaluator m_evaluator;
    Evaluator::Result m_lastResult;
    std::vector<std::string> m_history;
    // The last result as shown, and so as it starts an expression that goes on from it.
    std::string m_shownResult;
};
