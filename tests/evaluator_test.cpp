// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "calculator.h"
#include "evaluator.h"

#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <utility>
#include <vector>

using Error = Evaluator::Error;

// The result as the calculator shows it.
static std::string shown(Evaluator& evaluator, const std::string& expression)
{
    const Evaluator::Result result = evaluator.evaluate(expression);
    if (!result.value)
        return "error";
    return Calculator::format(*result.value);
}

static void expectResults(const std::vector<std::pair<const char*, const char*>>& cases,
                          Evaluator::AngleUnit unit = Evaluator::AngleUnit::Degrees)
{
    Evaluator evaluator;
    evaluator.setAngleUnit(unit);
    for (const auto& [expression, expected] : cases)
        EXPECT_EQ(shown(evaluator, expression), expected) << expression;
}

static void expectError(const char* expression, Error error)
{
    Evaluator evaluator;
    EXPECT_EQ(evaluator.evaluate(expression).error, error) << expression;
}

TEST(EvaluatorTest, Arithmetic)
{
    expectResults({
        {"1+2", "3"},
        {"2+3*4", "14"},
        {"(2+3)*4", "20"},
        {"10/4", "2.5"},
        {"1/3", "0.3333333333"},
        {"2/3", "0.6666666667"},
        {"0.1+0.2", "0.3"},
        {"1-0.9", "0.1"},
        {"0.1*3", "0.3"},
        {"1/3*3", "1"},
        {"10-2-3", "5"},
        {"100/10/5", "2"},
        {"8/2*4", "16"},
        {"-5+2", "-3"},
        {"--5", "5"},
        {"+5", "5"},
        {"3*-2", "-6"},
        {"1e20+1", "100000000000000000001"},
        {"123456789*987654321", "121932631112635269"},
        {"1e3", "1000"},
        {"1.5E-3", "0.0015"},
        {".5+.5", "1"},
        {"7 mod 3", "1"},
        {"-7 mod 3", "-1"},
        {"7.5 mod 2", "1.5"},
        {"7 div 2", "3"},
        {"-7 div 2", "-3"},
        {"0xFF", "255"},
        {"0b1010", "10"},
        {"0o17", "15"},
        {"0x10+1", "17"},
    });
}

TEST(EvaluatorTest, Powers)
{
    expectResults({
        {"2^10", "1024"},
        {"-2^2", "-4"},
        {"(-2)^2", "4"},
        {"(-2)^3", "-8"},
        {"2^-2", "0.25"},
        {"2^3^2", "512"},
        {"2**3", "8"},
        {"3²", "9"},
        {"2³", "8"},
        {"-3²", "-9"},
        {"2^0.5", "1.4142135624"},
        {"2^0.5*2^0.5", "2"},
        {"4^0.5", "2"},
        {"8^(1/3)", "2"},
        {"0^0", "1"},
        {"0^2", "0"},
        {"1^1e999", "1"},
        {"(-1)^1e999", "1"},
        {"0.5^1e20", "0"},
        {"2^100", "1267650600228229401496703205376"},
        {"2^200", "1606938044258990275541962092341162602522202993782792835301376"},
        {"10^-1001", "0"},
    });
}

TEST(EvaluatorTest, Factorials)
{
    expectResults({
        {"0!", "1"},
        {"5!", "120"},
        {"10!", "3628800"},
        {"20!", "2432902008176640000"},
        {"25!", "15511210043330985984000000"},
        {"100!",
         "93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000"},
        {"3!!", "720"},
        {"-3!", "-6"},
        {"2^3!", "64"},
    });
}

TEST(EvaluatorTest, Percent)
{
    expectResults({
        {"50%", "0.5"},
        {"200+10%", "220"},
        {"200-10%", "180"},
        {"200*10%", "20"},
        {"10%*200", "20"},
        {"200/50%", "400"},
        {"50%%", "0.005"},
        {"200+(10%)", "200.1"},
        {"100+-10%", "90"},
        {"(100+50)+10%", "165"},
        {"10%+5", "5.1"},
    });
}

TEST(EvaluatorTest, ImplicitProducts)
{
    expectResults({
        {"2(3+4)", "14"},
        {"(1+2)(3+4)", "21"},
        {"2pi", "6.2831853072"},
        {"2π", "6.2831853072"},
        {"2e", "5.4365636569"},
        {"2√9", "6"},
        {"3sin 30", "1.5"},
        {"sin 30 cos 60", "0.25"},
        {"1/2pi", "0.1591549431"},
    });
}

TEST(EvaluatorTest, Notation)
{
    expectResults({
        {"6÷3×2", "4"},
        {"5−3", "2"},
        {"（1＋2）×3", "9"},
        {"2 · 3", "6"},
        {"1 000 + 1", "error"},
        {"  1 +\t2  ", "3"},
        {"2*(3+4", "14"},
        {"sqrt(16", "4"},
        {"((2", "2"},
        {"max(1, 2)", "2"},
        {"max(1; 2)", "2"},
    });
}

TEST(EvaluatorTest, Constants)
{
    expectResults({
        {"pi", "3.1415926536"},
        {"π", "3.1415926536"},
        {"e", "2.7182818285"},
        {"phi", "1.6180339887"},
        {"phi^2-phi", "1"},
    });
}

TEST(EvaluatorTest, Roots)
{
    expectResults({
        {"sqrt(16)", "4"},
        {"sqrt 2", "1.4142135624"},
        {"sqrt(0.0004)", "0.02"},
        {"sqrt(1e-10)", "0.00001"},
        {"sqrt(2)^2", "2"},
        {"√9", "3"},
        {"√(9)", "3"},
        {"cbrt(27)", "3"},
        {"cbrt(-8)", "-2"},
        {"∛64", "4"},
        {"cbrt(2)", "1.2599210499"},
    });
}

TEST(EvaluatorTest, TrigonometryInDegrees)
{
    expectResults({
        {"sin(30)", "0.5"},
        {"sin 30", "0.5"},
        {"cos(60)", "0.5"},
        {"tan(45)", "1"},
        {"sin(90)", "1"},
        {"cos(90)", "0"},
        {"sin(180)", "0"},
        {"sin(-30)", "-0.5"},
        {"sin(390)", "0.5"},
        {"cos(120)", "-0.5"},
        {"cos(-120)", "-0.5"},
        {"sin(45)", "0.7071067812"},
        {"sin(1)", "0.0174524064"},
        {"sin(1e20)", "-0.984807753"},
        {"cot(45)", "1"},
        {"sec(60)", "2"},
        {"csc(30)", "2"},
        {"sin(30)^2+cos(30)^2", "1"},
        {"asin(0.5)", "30"},
        {"asin(-1)", "-90"},
        {"acos(0)", "90"},
        {"acos(-1)", "180"},
        {"acos(0.5)", "60"},
        {"atan(1)", "45"},
        {"atan(1e300)", "90"},
        {"atan(-1e300)", "-90"},
        {"asin(sin(20))", "20"},
        {"degrees(pi)", "180"},
        {"radians(180)", "3.1415926536"},
    });
}

TEST(EvaluatorTest, TrigonometryInRadians)
{
    expectResults(
        {
            {"sin(pi)", "0"},
            {"cos(pi)", "-1"},
            {"sin(pi/6)", "0.5"},
            {"cos(pi/3)", "0.5"},
            {"tan(pi/4)", "1"},
            {"sin(1)", "0.8414709848"},
            {"cos(1)", "0.5403023059"},
            {"sin(-1)", "-0.8414709848"},
            {"sin(100)", "-0.5063656411"},
            {"atan(1)*4", "3.1415926536"},
            {"asin(1)", "1.5707963268"},
            {"acos(-1)", "3.1415926536"},
        },
        Evaluator::AngleUnit::Radians);
}

TEST(EvaluatorTest, Logarithms)
{
    expectResults({
        {"ln(e)", "1"},
        {"ln(1)", "0"},
        {"ln(10)", "2.302585093"},
        {"ln(0.5)", "-0.6931471806"},
        {"exp(1)", "2.7182818285"},
        {"exp(10)", "22026.4657948067"},
        {"exp(-1)", "0.3678794412"},
        {"exp(-3000)", "0"},
        {"ln(exp(5))", "5"},
        {"log(1000)", "3"},
        {"log(0.001)", "-3"},
        {"log(2)", "0.3010299957"},
        {"lg(8)", "3"},
        {"log(8;2)", "3"},
        {"log(81, 3)", "4"},
        {"ln(1e999)", "2300.2825079011"},
        {"sinh(0)", "0"},
        {"cosh(0)", "1"},
        {"sinh(1)", "1.1752011936"},
        {"tanh(1)", "0.761594156"},
        {"tanh(1000)", "1"},
        {"asinh(1)", "0.881373587"},
        {"asinh(-1)", "-0.881373587"},
        {"acosh(1)", "0"},
        {"acosh(2)", "1.3169578969"},
        {"atanh(0.5)", "0.5493061443"},
    });
}

TEST(EvaluatorTest, OtherFunctions)
{
    expectResults({
        {"abs(-5)", "5"},
        {"abs -5", "5"},
        {"floor(-2.5)", "-3"},
        {"ceil(2.1)", "3"},
        {"round(2.5)", "3"},
        {"round(-2.5)", "-3"},
        {"round(3.14159;2)", "3.14"},
        {"round(1250;-2)", "1300"},
        {"trunc(-2.7)", "-2"},
        {"trunc(3.14159;3)", "3.141"},
        {"int(9.99)", "9"},
        {"frac(-2.75)", "-0.75"},
        {"sign(-3)", "-1"},
        {"sign(0)", "0"},
        {"min(3;1;2)", "1"},
        {"max(3,1,2)", "3"},
        {"sum(1;2;3)", "6"},
        {"product(2;3;4)", "24"},
        {"average(1;2;3;4)", "2.5"},
        {"geomean(2;8)", "4"},
        {"gcd(12;18)", "6"},
        {"gcd(12;18;8)", "2"},
        {"lcm(4;6)", "12"},
        {"lcm(4;0)", "0"},
        {"ncr(5;2)", "10"},
        {"ncr(52;5)", "2598960"},
        {"ncr(2;5)", "0"},
        {"ncr(1000;500)",
         "270288240945436569515614693625975275496152008446548287007392875106625428705522193898612483924502370165362606085021546104802209750050679917549894219699518475423665484263751733356162464079737887344364574161119497604571044985756287880514600994219426752366915856603136862602484428109296905863799821216320"},
        {"npr(5;2)", "20"},
        {"npr(10;10)", "3628800"},
    });
}

TEST(EvaluatorTest, AgreesWithTheCLibrary)
{
    struct Case
    {
        const char* name;
        double (*function)(double);
        double low;
        double high;
    };
    const Case cases[] = {
        {"sin", std::sin, -10, 10},    {"cos", std::cos, -10, 10},   {"tan", std::tan, -1.5, 1.5},
        {"atan", std::atan, -50, 50},  {"exp", std::exp, -50, 50},   {"ln", std::log, 0.001, 1000},
        {"sqrt", std::sqrt, 0, 1e6},   {"cbrt", std::cbrt, -1e6, 1e6}, {"sinh", std::sinh, -20, 20},
        {"tanh", std::tanh, -5, 5},    {"asin", std::asin, -0.99, 0.99}, {"acos", std::acos, -0.99, 0.99},
    };
    Evaluator evaluator;
    evaluator.setAngleUnit(Evaluator::AngleUnit::Radians);
    for (const Case& test : cases) {
        for (int i = 0; i <= 40; ++i) {
            // Values a double holds exactly, so both sides compute the same input.
            const double x = std::ldexp(std::round(std::ldexp(test.low + (test.high - test.low) * i / 40, 20)), -20);
            char argument[64];
            std::snprintf(argument, sizeof(argument), "%.17g", x);
            const std::string expression = std::string(test.name) + "(" + argument + ")";
            const Evaluator::Result result = evaluator.evaluate(expression);
            ASSERT_TRUE(result.value) << expression;
            const double expected = test.function(x);
            EXPECT_NEAR(result.value->toDouble(), expected, 1e-13 * std::max(1.0, std::fabs(expected))) << expression;
        }
    }
}

TEST(EvaluatorTest, Errors)
{
    expectError("", Error::Empty);
    expectError("   ", Error::Empty);
    expectError("1+", Error::Syntax);
    expectError("*2", Error::Syntax);
    expectError("2 3", Error::Syntax);
    expectError("(1+2))", Error::Syntax);
    expectError("()", Error::Syntax);
    expectError("1,2", Error::Syntax);
    expectError("sin", Error::Syntax);
    expectError("mod 3", Error::Syntax);
    expectError("1..2", Error::Syntax);
    expectError("1/0", Error::DivisionByZero);
    expectError("5 mod 0", Error::DivisionByZero);
    expectError("0^-1", Error::DivisionByZero);
    expectError("foo", Error::UnknownName);
    expectError("sqrt(-1)", Error::Domain);
    expectError("ln(0)", Error::Domain);
    expectError("log(8;1)", Error::Domain);
    expectError("asin(2)", Error::Domain);
    expectError("tan(90)", Error::Domain);
    expectError("cot(180)", Error::Domain);
    expectError("(-8)^(1/3)", Error::Domain);
    expectError("2.5!", Error::Domain);
    expectError("(-1)!", Error::Domain);
    expectError("gcd(1.5;2)", Error::Domain);
    expectError("max()", Error::ArgumentCount);
    expectError("ncr(5)", Error::ArgumentCount);
    expectError("sqrt(1;2)", Error::ArgumentCount);
    expectError("10^1000", Error::Overflow);
    expectError("1e1000", Error::Overflow);
    expectError("450!", Error::Overflow);
    expectError("exp(3000)", Error::Overflow);
    expectError("2^(10^20)", Error::Overflow);
    expectError("1.0000000000000000001^(10^30)", Error::Overflow);
    expectError("ncr(100000;50000)", Error::Overflow);
    expectError("pi = 3", Error::ReadOnlyName);
    expectError("sin = 1", Error::ReadOnlyName);
    expectError("ans = 2", Error::ReadOnlyName);

    Evaluator evaluator;
    Evaluator::Result result = evaluator.evaluate("1 + $");
    EXPECT_EQ(result.error, Error::Syntax);
    EXPECT_EQ(result.position, 4u);
    result = evaluator.evaluate("2 * foo");
    EXPECT_EQ(result.error, Error::UnknownName);
    EXPECT_EQ(result.name, "foo");
    result = evaluator.evaluate("ncr(1)");
    EXPECT_EQ(result.name, "ncr");
}

TEST(EvaluatorTest, Limits)
{
    Evaluator evaluator;
    const Evaluator::Result largest = evaluator.evaluate("10^999");
    ASSERT_TRUE(largest.value);
    EXPECT_EQ(Calculator::format(*largest.value), "1" + std::string(999, '0'));
    EXPECT_TRUE(evaluator.evaluate("449!").value);
    EXPECT_EQ(shown(evaluator, "1e-1001"), "0");
    EXPECT_EQ(shown(evaluator, "1e-999*1e999"), "1");
    // Whole numbers keep every digit up to the limit.
    EXPECT_EQ(shown(evaluator, "2^3322"), "error");
    EXPECT_EQ(shown(evaluator, "2^3000/2^2999"), "2");
    EXPECT_EQ(shown(evaluator, "2^3321-2^3321+1"), "1");
    EXPECT_EQ(shown(evaluator, "(10^999-1)+1-10^999"), "0");
    EXPECT_EQ(shown(evaluator, "10^998+1-10^998"), "1");
}

TEST(EvaluatorTest, Variables)
{
    Evaluator evaluator;
    EXPECT_EQ(shown(evaluator, "ans"), "0");
    EXPECT_EQ(shown(evaluator, "x = 5"), "5");
    EXPECT_EQ(shown(evaluator, "x*2"), "10");
    EXPECT_EQ(shown(evaluator, "ans+1"), "11");
    EXPECT_EQ(shown(evaluator, "y = x + ans"), "16");
    EXPECT_EQ(evaluator.variable("y")->toString(), "16");
    EXPECT_EQ(shown(evaluator, "2x"), "10");
    EXPECT_EQ(shown(evaluator, "1/0"), "error");
    // A failed evaluation keeps ans.
    EXPECT_EQ(shown(evaluator, "ans"), "10");
    EXPECT_TRUE(evaluator.setVariable("rate", Decimal(3)));
    EXPECT_FALSE(evaluator.setVariable("pi", Decimal(3)));
    EXPECT_FALSE(evaluator.setVariable("2x", Decimal(3)));
    EXPECT_EQ(shown(evaluator, "rate"), "3");
    evaluator.clearVariables();
    EXPECT_EQ(evaluator.evaluate("x").error, Error::UnknownName);
    EXPECT_EQ(shown(evaluator, "ans"), "0");
}
