// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "calculator.h"

#include <gtest/gtest.h>

TEST(CalculatorTest, ResultsAndHistory)
{
    Calculator calculator;
    EXPECT_EQ(calculator.calculate("1+2"), "3");
    EXPECT_EQ(calculator.calculate("7×8"), "56");
    // The same text as its result adds nothing.
    EXPECT_EQ(calculator.calculate("56"), "56");
    EXPECT_EQ(calculator.calculate("1÷0"), std::nullopt);
    EXPECT_EQ(calculator.lastResult().error, Evaluator::Error::DivisionByZero);
    EXPECT_EQ(calculator.calculate(""), std::nullopt);
    EXPECT_EQ(calculator.history(), (std::vector<std::string>{"1+2 = 3", "7×8 = 56"}));
    calculator.clearHistory();
    EXPECT_TRUE(calculator.history().empty());
}

TEST(CalculatorTest, GoesOnFromTheFullResult)
{
    Calculator calculator;
    EXPECT_EQ(calculator.calculate("1÷3"), "0.3333333333");
    EXPECT_EQ(calculator.calculate("0.3333333333×3"), "1");
    EXPECT_EQ(calculator.history().back(), "0.3333333333×3 = 1");
    // More digits after it make another number.
    EXPECT_EQ(calculator.calculate("2÷3"), "0.6666666667");
    EXPECT_EQ(calculator.calculate("0.66666666671×3"), "2.0000000001");
    // The result typed again by hand elsewhere in the expression is taken as typed.
    EXPECT_EQ(calculator.calculate("1÷3"), "0.3333333333");
    EXPECT_EQ(calculator.calculate("3×0.3333333333"), "0.9999999999");
    EXPECT_EQ(calculator.calculate("2^0.5"), "1.4142135624");
    EXPECT_EQ(calculator.calculate("1.4142135624^2"), "2");
}

TEST(CalculatorTest, Insert)
{
    Calculator::Edit edit = Calculator::insert("12", 2, 2, "+");
    EXPECT_EQ(edit.text, "12+");
    EXPECT_EQ(edit.cursor, 3u);
    edit = Calculator::insert("12", 1, 1, "×");
    EXPECT_EQ(edit.text, "1×2");
    EXPECT_EQ(edit.cursor, 3u);
    // A selection is replaced, whichever end the cursor is at.
    edit = Calculator::insert("1234", 1, 3, "9");
    EXPECT_EQ(edit.text, "194");
    EXPECT_EQ(edit.cursor, 2u);
    edit = Calculator::insert("1234", 3, 1, "9");
    EXPECT_EQ(edit.text, "194");
    edit = Calculator::insert("12", 9, 9, "3");
    EXPECT_EQ(edit.text, "123");
}

TEST(CalculatorTest, Backspace)
{
    Calculator::Edit edit = Calculator::backspace("123", 3, 3);
    EXPECT_EQ(edit.text, "12");
    EXPECT_EQ(edit.cursor, 2u);
    edit = Calculator::backspace("123", 1, 1);
    EXPECT_EQ(edit.text, "23");
    EXPECT_EQ(edit.cursor, 0u);
    edit = Calculator::backspace("123", 0, 0);
    EXPECT_EQ(edit.text, "123");
    EXPECT_EQ(edit.cursor, 0u);
    // A character of several bytes goes whole.
    edit = Calculator::backspace("6×", 3, 3);
    EXPECT_EQ(edit.text, "6");
    EXPECT_EQ(edit.cursor, 1u);
    edit = Calculator::backspace("1234", 3, 1);
    EXPECT_EQ(edit.text, "14");
    EXPECT_EQ(edit.cursor, 1u);
}

TEST(CalculatorTest, Parenthesis)
{
    const auto at = [](const std::string& text) { return Calculator::parenthesis(text, text.size()); };
    EXPECT_EQ(at(""), "(");
    EXPECT_EQ(at("2"), "(");
    EXPECT_EQ(at("(2"), ")");
    EXPECT_EQ(at("(2+"), "(");
    EXPECT_EQ(at("(2)"), "(");
    EXPECT_EQ(at("((2)"), ")");
    EXPECT_EQ(at("(sin"), "(");
    EXPECT_EQ(at("(pi"), ")");
    EXPECT_EQ(at("(2e"), ")");
    EXPECT_EQ(at("(x"), ")");
    EXPECT_EQ(at("(2."), ")");
    EXPECT_EQ(at("(5%"), ")");
    EXPECT_EQ(at("(3!"), ")");
    EXPECT_EQ(at("(2π"), ")");
    EXPECT_EQ(at("(2×"), "(");
    EXPECT_EQ(at("(2 "), ")");
    EXPECT_EQ(Calculator::parenthesis("(2)", 2), ")");
    EXPECT_EQ(Calculator::parenthesis("(2)", 0), "(");
}
