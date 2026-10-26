// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "decimal.h"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Evaluates an expression such as "2 + 3 × (4 − 1)", "sin 30", "15%" or
// "x = sqrt(2)". Keeps the variables, "ans" (the last result) among them.
//
// Precedence, lowest first: + and −; ×, ÷, mod and div; a factor written
// right after another, as in 2π or 3(4 + 5); a sign; ^ and the superscripts
// ² and ³, from the right; ! and %, after the operand. A function takes its
// arguments in parentheses, separated by ";" or ",", or a single operand
// without them. Parentheses left open at the end close themselves.
//
// x% is x / 100, except as the right side of + and −, where a ± b% adds or
// takes away b percent of a.
class Evaluator
{
public:
    enum class Error {
        None,
        // Nothing but white space.
        Empty,
        Syntax,
        DivisionByZero,
        // A name that is neither a variable, a constant nor a function.
        UnknownName,
        // A function called outside its domain, as sqrt(-1) or tan 90.
        Domain,
        ArgumentCount,
        // A magnitude of 10^1000 or more.
        Overflow,
        // An assignment to a constant, a function or "ans".
        ReadOnlyName,
    };

    struct Result
    {
        std::optional<Decimal> value;
        Error error = Error::None;
        // The name the error is about, for UnknownName, ArgumentCount and ReadOnlyName.
        std::string name;
        // Where in the expression, in bytes, for Syntax.
        size_t position = 0;
    };

    enum class AngleUnit {
        Degrees,
        Radians,
    };

    Evaluator();
    ~Evaluator();

    // Assigns "ans" and, for "name = …", the variable.
    Result evaluate(std::string_view expression);

    // Degrees unless set.
    void setAngleUnit(AngleUnit unit) { m_angleUnit = unit; }
    AngleUnit angleUnit() const { return m_angleUnit; }

    std::optional<Decimal> variable(const std::string& name) const;
    // False for a reserved name.
    bool setVariable(const std::string& name, const Decimal& value);
    void clearVariables();

    // A constant, a function or an operator word.
    static bool isReserved(const std::string& name);

private:
    struct Parser;

    std::map<std::string, Decimal> m_variables;
    AngleUnit m_angleUnit = AngleUnit::Degrees;
};
