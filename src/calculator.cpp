// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "calculator.h"

#include <algorithm>

static bool isContinuationByte(char c)
{
    return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

static bool isAsciiAlphanumeric(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

std::optional<std::string> Calculator::calculate(const std::string& expression)
{
    // The shown result rounds its value; the full value is in ans. A digit,
    // a point or a letter after it would make it part of another number or name.
    std::string source = expression;
    if (!m_shownResult.empty() && expression.size() > m_shownResult.size() && expression.starts_with(m_shownResult)) {
        const char next = expression[m_shownResult.size()];
        if (!isAsciiAlphanumeric(next) && next != '.')
            source = "ans" + expression.substr(m_shownResult.size());
    }

    m_lastResult = m_evaluator.evaluate(source);
    if (!m_lastResult.value)
        return std::nullopt;

    const std::string result = format(*m_lastResult.value);
    if (expression != result)
        m_history.push_back(expression + " = " + result);
    m_shownResult = result;
    return result;
}

void Calculator::clearHistory()
{
    m_history.clear();
}

std::string Calculator::format(const Decimal& value)
{
    return value.toFixed(DisplayDecimals);
}

Calculator::Edit Calculator::insert(const std::string& text, size_t anchor, size_t cursor, std::string_view insertion)
{
    const size_t start = std::min({anchor, cursor, text.size()});
    const size_t end = std::min(std::max(anchor, cursor), text.size());
    Edit edit;
    edit.text = text.substr(0, start);
    edit.text += insertion;
    edit.text += text.substr(end);
    edit.cursor = start + insertion.size();
    return edit;
}

Calculator::Edit Calculator::backspace(const std::string& text, size_t anchor, size_t cursor)
{
    if (anchor != cursor)
        return insert(text, anchor, cursor, {});
    size_t start = std::min(cursor, text.size());
    if (start == 0)
        return {text, 0};
    --start;
    while (start > 0 && isContinuationByte(text[start]))
        --start;
    return insert(text, start, std::min(cursor, text.size()), {});
}

std::string Calculator::parenthesis(const std::string& text, size_t cursor)
{
    const std::string before = text.substr(0, std::min(cursor, text.size()));
    int open = 0;
    for (size_t i = 0; i < before.size(); ++i) {
        if (before[i] == '(')
            ++open;
        else if (before[i] == ')')
            --open;
        else if (before.compare(i, 3, "（") == 0)
            ++open;
        else if (before.compare(i, 3, "）") == 0)
            --open;
    }
    if (open <= 0)
        return "(";

    size_t end = before.size();
    while (end > 0 && before[end - 1] == ' ')
        --end;
    if (end == 0)
        return "(";

    // A name before the cursor is a function, which takes "(", or a constant
    // or variable, which ends an operand.
    size_t start = end;
    while (start > 0 && isAsciiAlphanumeric(before[start - 1]))
        --start;
    // Digits before a name are a number it multiplies, as in 2pi.
    while (start < end && before[start] >= '0' && before[start] <= '9')
        ++start;
    if (start < end) {
        const std::string name = before.substr(start, end - start);
        const bool constant = name == "pi" || name == "e" || name == "phi" || name == "ans";
        if (Evaluator::isReserved(name) && !constant)
            return "(";
        return ")";
    }

    const char last = before[end - 1];
    if ((last >= '0' && last <= '9') || last == '.' || last == ')' || last == '!' || last == '%')
        return ")";
    for (const char* operandEnd : {"π", "²", "³", "）", "％", "！"}) {
        if (std::string_view(before).substr(0, end).ends_with(operandEnd))
            return ")";
    }
    return "(";
}
