// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "evaluator.h"

#include "mathfunctions.h"

#include <algorithm>
#include <cmath>
#include <cstring>

using Error = Evaluator::Error;

static constexpr int kDigits = Decimal::Precision;
// Results stay below 10^1000; smaller than 10^-1000 they become zero.
static constexpr int64_t kLargestExponent = 999;
// Enough digits for any whole number below the limit, which stays exact.
static constexpr int kExactDigits = kLargestExponent + 2;
static constexpr int64_t kSmallestExponent = -1000;
// The largest n whose n! is below 10^1000.
static constexpr int64_t kLargestFactorial = 449;

namespace {

enum class TokenKind {
    End,
    Number,
    Name,
    Plus,
    Minus,
    Times,
    Divide,
    Power,
    Open,
    Close,
    Separator,
    Factorial,
    Percent,
    Assign,
    SquareRoot,
    CubeRoot,
};

struct Token
{
    TokenKind kind = TokenKind::End;
    size_t position = 0;
    std::string name;
    Decimal number;
};

struct Symbol
{
    const char* text;
    TokenKind kind;
};

struct FunctionInfo
{
    const char* name;
    int minimum;
    // -1 for any number.
    int maximum;
};

// An operand and whether it ended in %, for a ± b%.
struct Operand
{
    Decimal value;
    bool percent = false;
    // The value before the last %.
    Decimal percentBase;
};

} // namespace

// Characters other than ASCII that are typed, pasted or shown for operators.
static const Symbol kSymbols[] = {
    {"**", TokenKind::Power},     {"×", TokenKind::Times},      {"✕", TokenKind::Times},     {"∗", TokenKind::Times},
    {"·", TokenKind::Times},      {"⋅", TokenKind::Times},      {"＊", TokenKind::Times},    {"÷", TokenKind::Divide},
    {"∕", TokenKind::Divide},     {"／", TokenKind::Divide},    {"−", TokenKind::Minus},     {"－", TokenKind::Minus},
    {"–", TokenKind::Minus},      {"＋", TokenKind::Plus},      {"（", TokenKind::Open},     {"）", TokenKind::Close},
    {"，", TokenKind::Separator}, {"；", TokenKind::Separator}, {"＝", TokenKind::Assign},   {"！", TokenKind::Factorial},
    {"％", TokenKind::Percent},   {"√", TokenKind::SquareRoot}, {"∛", TokenKind::CubeRoot},
};

// Spaces, the no-break ones and the thin ones included.
static const char* const kSpaces[] = {" ", "\t", "\n", "\r", "\xc2\xa0", "\xe2\x80\x89", "\xe2\x80\xaf"};

static const FunctionInfo kFunctions[] = {
    {"abs", 1, 1},     {"sqrt", 1, 1},     {"cbrt", 1, 1},    {"exp", 1, 1},      {"ln", 1, 1},
    {"log", 1, 2},     {"lg", 1, 1},       {"sin", 1, 1},     {"cos", 1, 1},      {"tan", 1, 1},
    {"cot", 1, 1},     {"sec", 1, 1},      {"csc", 1, 1},     {"asin", 1, 1},     {"acos", 1, 1},
    {"atan", 1, 1},    {"sinh", 1, 1},     {"cosh", 1, 1},    {"tanh", 1, 1},     {"asinh", 1, 1},
    {"acosh", 1, 1},   {"atanh", 1, 1},    {"degrees", 1, 1}, {"radians", 1, 1},  {"floor", 1, 1},
    {"ceil", 1, 1},    {"round", 1, 2},    {"trunc", 1, 2},   {"int", 1, 1},      {"frac", 1, 1},
    {"sign", 1, 1},    {"min", 1, -1},     {"max", 1, -1},    {"sum", 1, -1},     {"product", 1, -1},
    {"average", 1, -1}, {"geomean", 1, -1}, {"gcd", 1, -1},   {"lcm", 1, -1},     {"ncr", 2, 2},
    {"npr", 2, 2},
};

static const char* const kConstants[] = {"pi", "e", "phi"};

static const FunctionInfo* findFunction(const std::string& name)
{
    for (const FunctionInfo& function : kFunctions) {
        if (name == function.name)
            return &function;
    }
    return nullptr;
}

static bool isConstant(const std::string& name)
{
    return std::find_if(std::begin(kConstants), std::end(kConstants),
                        [&name](const char* constant) { return name == constant; })
        != std::end(kConstants);
}

static bool isOperatorWord(const std::string& name)
{
    return name == "mod" || name == "div";
}

static bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

static bool isNameStart(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isDigitOf(char c, unsigned radix)
{
    if (isDigit(c))
        return unsigned(c - '0') < radix;
    if (radix == 16)
        return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    return false;
}

// Null on an unknown character, with its position.
static std::optional<std::vector<Token>> tokenize(std::string_view text, size_t* errorPosition)
{
    std::vector<Token> tokens;
    size_t i = 0;
    const auto startsWith = [&](const char* prefix) { return text.substr(i).starts_with(prefix); };
    const auto add = [&](TokenKind kind, size_t position) {
        Token token;
        token.kind = kind;
        token.position = position;
        tokens.push_back(std::move(token));
    };

    while (i < text.size()) {
        const size_t start = i;
        const char c = text[i];

        bool space = false;
        for (const char* blank : kSpaces) {
            if (startsWith(blank)) {
                i += std::strlen(blank);
                space = true;
                break;
            }
        }
        if (space)
            continue;

        if (isDigit(c) || (c == '.' && i + 1 < text.size() && isDigit(text[i + 1]))) {
            unsigned radix = 10;
            if (c == '0' && i + 2 < text.size()) {
                const char prefix = char(text[i + 1] | 0x20);
                const unsigned prefixRadix = prefix == 'x' ? 16 : prefix == 'b' ? 2 : prefix == 'o' ? 8 : 0;
                if (prefixRadix && isDigitOf(text[i + 2], prefixRadix))
                    radix = prefixRadix;
            }
            Token token;
            token.kind = TokenKind::Number;
            token.position = start;
            if (radix != 10) {
                i += 2;
                const size_t digits = i;
                while (i < text.size() && isDigitOf(text[i], radix))
                    ++i;
                token.number = Decimal(*Natural::fromDigits(text.substr(digits, i - digits), radix), 0);
            } else {
                while (i < text.size() && isDigit(text[i]))
                    ++i;
                if (i < text.size() && text[i] == '.') {
                    ++i;
                    while (i < text.size() && isDigit(text[i]))
                        ++i;
                }
                // An exponent only when digits follow; "2e" is 2 × e.
                if (i < text.size() && (text[i] == 'e' || text[i] == 'E')) {
                    size_t j = i + 1;
                    if (j < text.size() && (text[j] == '+' || text[j] == '-'))
                        ++j;
                    if (j < text.size() && isDigit(text[j])) {
                        i = j;
                        while (i < text.size() && isDigit(text[i]))
                            ++i;
                    }
                }
                const std::optional<Decimal> number = Decimal::parse(text.substr(start, i - start));
                if (!number) {
                    *errorPosition = start;
                    return std::nullopt;
                }
                token.number = *number;
            }
            tokens.push_back(std::move(token));
            continue;
        }

        if (isNameStart(c)) {
            while (i < text.size() && (isNameStart(text[i]) || isDigit(text[i])))
                ++i;
            Token token;
            token.kind = TokenKind::Name;
            token.position = start;
            token.name = std::string(text.substr(start, i - start));
            tokens.push_back(std::move(token));
            continue;
        }

        if (startsWith("π")) {
            Token token;
            token.kind = TokenKind::Name;
            token.position = start;
            token.name = "pi";
            tokens.push_back(std::move(token));
            i += std::strlen("π");
            continue;
        }

        if (startsWith("²") || startsWith("³")) {
            const bool square = startsWith("²");
            add(TokenKind::Power, start);
            Token exponent;
            exponent.kind = TokenKind::Number;
            exponent.position = start;
            exponent.number = Decimal(square ? 2 : 3);
            tokens.push_back(std::move(exponent));
            i += std::strlen("²");
            continue;
        }

        bool matched = false;
        for (const Symbol& symbol : kSymbols) {
            if (startsWith(symbol.text)) {
                add(symbol.kind, start);
                i += std::strlen(symbol.text);
                matched = true;
                break;
            }
        }
        if (matched)
            continue;

        TokenKind kind;
        switch (c) {
        case '+':
            kind = TokenKind::Plus;
            break;
        case '-':
            kind = TokenKind::Minus;
            break;
        case '*':
            kind = TokenKind::Times;
            break;
        case '/':
            kind = TokenKind::Divide;
            break;
        case '^':
            kind = TokenKind::Power;
            break;
        case '(':
            kind = TokenKind::Open;
            break;
        case ')':
            kind = TokenKind::Close;
            break;
        case ';':
        case ',':
            kind = TokenKind::Separator;
            break;
        case '!':
            kind = TokenKind::Factorial;
            break;
        case '%':
            kind = TokenKind::Percent;
            break;
        case '=':
            kind = TokenKind::Assign;
            break;
        default:
            *errorPosition = start;
            return std::nullopt;
        }
        add(kind, start);
        ++i;
    }

    add(TokenKind::End, text.size());
    return tokens;
}

// log10(x) for x > 0, to a double's precision, at any magnitude.
static double log10Of(const Decimal& x)
{
    const Natural& coefficient = x.coefficient();
    const double leading = Decimal(coefficient, 1 - int64_t(coefficient.digitCount())).toDouble();
    return std::log10(leading) + double(x.adjustedExponent());
}

static const Decimal& one()
{
    static const Decimal value(1);
    return value;
}

struct Evaluator::Parser
{
    Evaluator& evaluator;
    std::vector<Token> tokens;
    size_t index = 0;
    Result failure;

    const Token& peek() const { return tokens[index]; }
    const Token& next() { return tokens[index++]; }

    bool failed() const { return failure.error != Error::None; }

    std::nullopt_t fail(Error error, size_t position = 0, std::string name = {})
    {
        if (!failed()) {
            failure.error = error;
            failure.position = position;
            failure.name = std::move(name);
        }
        return std::nullopt;
    }

    // Whole numbers keep every digit through +, − and ×; anything else keeps the precision.
    static int digitsFor(const Decimal& a, const Decimal& b)
    {
        return a.isInteger() && b.isInteger() ? kExactDigits : kDigits;
    }

    std::optional<Decimal> checked(const Decimal& value)
    {
        if (value.isZero())
            return value;
        const int64_t exponent = value.adjustedExponent();
        if (exponent > kLargestExponent)
            return fail(Error::Overflow);
        if (exponent < kSmallestExponent)
            return Decimal();
        return value;
    }

    bool startsOperand(const Token& token) const
    {
        switch (token.kind) {
        case TokenKind::Number:
        case TokenKind::Open:
        case TokenKind::SquareRoot:
        case TokenKind::CubeRoot:
        case TokenKind::Plus:
        case TokenKind::Minus:
            return true;
        case TokenKind::Name:
            return !isOperatorWord(token.name);
        default:
            return false;
        }
    }

    // A factor written right after another, without an operator.
    bool startsImplicitFactor(const Token& token) const
    {
        switch (token.kind) {
        case TokenKind::Open:
        case TokenKind::SquareRoot:
        case TokenKind::CubeRoot:
            return true;
        case TokenKind::Name:
            return !isOperatorWord(token.name);
        default:
            return false;
        }
    }

    std::optional<Operand> expression()
    {
        std::optional<Operand> left = term();
        if (!left)
            return std::nullopt;
        while (peek().kind == TokenKind::Plus || peek().kind == TokenKind::Minus) {
            const bool plus = next().kind == TokenKind::Plus;
            const std::optional<Operand> right = term();
            if (!right)
                return std::nullopt;
            Decimal amount = right->value;
            if (right->percent)
                amount = Decimal::divide(left->value * right->percentBase, Decimal(100));
            const int digits = digitsFor(left->value, amount);
            const std::optional<Decimal> sum = checked(plus ? Decimal::add(left->value, amount, digits)
                                                            : Decimal::subtract(left->value, amount, digits));
            if (!sum)
                return std::nullopt;
            left = Operand{*sum};
        }
        return left;
    }

    std::optional<Operand> term()
    {
        std::optional<Operand> left = implicitProduct();
        if (!left)
            return std::nullopt;
        for (;;) {
            const Token& token = peek();
            const bool word = token.kind == TokenKind::Name && isOperatorWord(token.name);
            if (token.kind != TokenKind::Times && token.kind != TokenKind::Divide && !word)
                break;
            const Token& op = next();
            const std::optional<Operand> right = implicitProduct();
            if (!right)
                return std::nullopt;
            std::optional<Decimal> result;
            if (op.kind == TokenKind::Times) {
                result = checked(Decimal::multiply(left->value, right->value, digitsFor(left->value, right->value)));
            } else {
                if (right->value.isZero())
                    return fail(Error::DivisionByZero, op.position);
                if (op.kind == TokenKind::Divide) {
                    result = checked(left->value / right->value);
                } else {
                    Decimal quotient;
                    Decimal remainder;
                    math::divideTruncated(left->value, right->value, &quotient, &remainder);
                    result = checked(op.name == "div" ? quotient : remainder);
                }
            }
            if (!result)
                return std::nullopt;
            left = Operand{*result};
        }
        return left;
    }

    std::optional<Operand> implicitProduct()
    {
        std::optional<Operand> left = unary();
        if (!left)
            return std::nullopt;
        while (startsImplicitFactor(peek())) {
            const std::optional<Operand> right = unary();
            if (!right)
                return std::nullopt;
            const std::optional<Decimal> product =
                checked(Decimal::multiply(left->value, right->value, digitsFor(left->value, right->value)));
            if (!product)
                return std::nullopt;
            left = Operand{*product};
        }
        return left;
    }

    std::optional<Operand> unary()
    {
        if (peek().kind == TokenKind::Plus) {
            next();
            return unary();
        }
        if (peek().kind == TokenKind::Minus) {
            next();
            std::optional<Operand> operand = unary();
            if (operand) {
                operand->value = -operand->value;
                operand->percentBase = -operand->percentBase;
            }
            return operand;
        }
        return power();
    }

    std::optional<Operand> power()
    {
        std::optional<Operand> base = postfix();
        if (!base || peek().kind != TokenKind::Power)
            return base;
        const size_t position = next().position;
        const std::optional<Operand> exponent = unary();
        if (!exponent)
            return std::nullopt;
        const std::optional<Decimal> value = raise(base->value, exponent->value, position);
        if (!value)
            return std::nullopt;
        return Operand{*value};
    }

    std::optional<Operand> postfix()
    {
        std::optional<Operand> operand = primary();
        if (!operand)
            return std::nullopt;
        for (;;) {
            if (peek().kind == TokenKind::Factorial) {
                const std::optional<Decimal> value = factorial(operand->value, next().position);
                if (!value)
                    return std::nullopt;
                operand = Operand{*value};
            } else if (peek().kind == TokenKind::Percent) {
                next();
                operand->percentBase = operand->value;
                operand->value = Decimal::divide(operand->value, Decimal(100));
                operand->percent = true;
            } else {
                return operand;
            }
        }
    }

    // A closing parenthesis, or the end, which closes it.
    bool close()
    {
        if (peek().kind == TokenKind::Close) {
            next();
            return true;
        }
        if (peek().kind == TokenKind::End)
            return true;
        fail(Error::Syntax, peek().position);
        return false;
    }

    std::optional<Operand> primary()
    {
        const Token& token = next();
        switch (token.kind) {
        case TokenKind::Number: {
            const std::optional<Decimal> value = checked(token.number.rounded(kExactDigits));
            if (!value)
                return std::nullopt;
            return Operand{*value};
        }
        case TokenKind::Open: {
            const std::optional<Operand> inner = expression();
            if (!inner || !close())
                return std::nullopt;
            return Operand{inner->value};
        }
        case TokenKind::SquareRoot:
        case TokenKind::CubeRoot: {
            if (!startsOperand(peek()))
                return fail(Error::Syntax, peek().position);
            const std::optional<Operand> operand = unary();
            if (!operand)
                return std::nullopt;
            const std::optional<Decimal> value =
                call(token.kind == TokenKind::SquareRoot ? "sqrt" : "cbrt", {operand->value}, token.position);
            if (!value)
                return std::nullopt;
            return Operand{*value};
        }
        case TokenKind::Name:
            return name(token);
        default:
            return fail(Error::Syntax, token.position);
        }
    }

    std::optional<Operand> name(const Token& token)
    {
        if (isOperatorWord(token.name))
            return fail(Error::Syntax, token.position);

        if (const FunctionInfo* function = findFunction(token.name)) {
            std::vector<Decimal> arguments;
            if (peek().kind == TokenKind::Open) {
                next();
                if (peek().kind != TokenKind::Close) {
                    for (;;) {
                        const std::optional<Operand> argument = expression();
                        if (!argument)
                            return std::nullopt;
                        arguments.push_back(argument->value);
                        if (peek().kind != TokenKind::Separator)
                            break;
                        next();
                    }
                }
                if (!close())
                    return std::nullopt;
            } else if (startsOperand(peek())) {
                const std::optional<Operand> argument = unary();
                if (!argument)
                    return std::nullopt;
                arguments.push_back(argument->value);
            } else {
                return fail(Error::Syntax, peek().position);
            }
            if (int(arguments.size()) < function->minimum
                || (function->maximum >= 0 && int(arguments.size()) > function->maximum))
                return fail(Error::ArgumentCount, token.position, token.name);
            const std::optional<Decimal> value = call(token.name, arguments, token.position);
            if (!value)
                return std::nullopt;
            return Operand{*value};
        }

        if (token.name == "pi")
            return Operand{math::pi(kDigits)};
        if (token.name == "e") {
            static const Decimal e = math::exp(one(), kDigits);
            return Operand{e};
        }
        if (token.name == "phi")
            return Operand{(one() + math::sqrt(Decimal(5), kDigits + 5)).rounded(kDigits) / Decimal(2)};
        const auto variable = evaluator.m_variables.find(token.name);
        if (variable != evaluator.m_variables.end())
            return Operand{variable->second};
        return fail(Error::UnknownName, token.position, token.name);
    }

    std::optional<Decimal> factorial(const Decimal& x, size_t position)
    {
        if (!x.isInteger() || x.isNegative())
            return fail(Error::Domain, position);
        if (x > Decimal(kLargestFactorial))
            return fail(Error::Overflow, position);
        const int64_t n = *x.toInt64();
        Natural product(1);
        for (int64_t i = 2; i <= n; ++i)
            product = product * uint32_t(i);
        return Decimal(std::move(product), 0);
    }

    std::optional<Decimal> raise(const Decimal& base, const Decimal& exponent, size_t position)
    {
        if (exponent.isZero())
            return one();
        if (base.isZero()) {
            if (exponent.isNegative())
                return fail(Error::DivisionByZero, position);
            return Decimal();
        }
        if (!exponent.isInteger() && base.isNegative())
            return fail(Error::Domain, position);

        // The result's magnitude decides overflow before any work; an exponent
        // past a double's digits needs the logarithm to more.
        double magnitude = log10Of(base.abs()) * exponent.toDouble();
        if (exponent.adjustedExponent() > 15 && base.abs() != one()) {
            const int digits = 40 + int(exponent.adjustedExponent());
            magnitude = Decimal::divide(Decimal::multiply(math::ln(base.abs(), digits), exponent, digits),
                                        math::ln(Decimal(10), digits), 20)
                            .toDouble();
        }
        if (magnitude > double(kLargestExponent) + 1)
            return fail(Error::Overflow, position);
        if (magnitude < double(kSmallestExponent) - 1)
            return Decimal();

        if (exponent.isInteger()) {
            const Natural count = exponent.coefficient().shifted(unsigned(exponent.exponent()));
            Decimal result;
            if (base.abs() == one())
                result = base.isNegative() && count.isOdd() ? -one() : one();
            else
                result = math::power(base, count, base.isInteger() && !exponent.isNegative() ? kExactDigits : kDigits);
            if (exponent.isNegative())
                result = one() / result;
            return checked(result);
        }
        if (exponent == *Decimal::parse("0.5"))
            return checked(math::sqrt(base, kDigits));
        const int working = kDigits + 10;
        return checked(math::exp(Decimal::multiply(exponent, math::ln(base, working), working), kDigits));
    }

    std::pair<Decimal, Decimal> sinCos(const Decimal& x) const
    {
        if (evaluator.m_angleUnit == AngleUnit::Degrees)
            return math::sinCosDegrees(x, kDigits);
        return math::sinCosRadians(x, kDigits);
    }

    // An angle in radians in the evaluator's unit.
    Decimal fromRadians(const Decimal& radians) const
    {
        if (evaluator.m_angleUnit == AngleUnit::Radians)
            return radians.rounded(kDigits);
        const int working = kDigits + 10;
        return Decimal::divide(Decimal::multiply(radians, Decimal(180), working), math::pi(working), kDigits);
    }

    // asin, with the angles a whole or half sine gives exact in degrees.
    std::optional<Decimal> arcSine(const Decimal& x, size_t position)
    {
        if (x.abs() > one())
            return fail(Error::Domain, position);
        const bool degrees = evaluator.m_angleUnit == AngleUnit::Degrees;
        const Decimal half = *Decimal::parse("0.5");
        if (x.isZero())
            return Decimal();
        if (degrees && x.abs() == half)
            return x.isNegative() ? Decimal(-30) : Decimal(30);
        const int working = kDigits + 10;
        if (x.abs() == one()) {
            if (degrees)
                return x.isNegative() ? Decimal(-90) : Decimal(90);
            const Decimal right = Decimal::divide(math::pi(working), Decimal(2), kDigits);
            return x.isNegative() ? -right : right;
        }
        const Decimal cosine = math::sqrt(Decimal::subtract(one(), Decimal::multiply(x, x, working), working), working);
        return fromRadians(math::atan(Decimal::divide(x, cosine, working), working));
    }

    std::optional<Decimal> call(const std::string& name, const std::vector<Decimal>& arguments, size_t position)
    {
        const Decimal& x = arguments[0];
        const int working = kDigits + 10;
        const auto domain = [this, position] { return fail(Error::Domain, position); };
        const auto integerArgument = [](const Decimal& value) { return value.isInteger(); };

        if (name == "abs")
            return x.abs();
        if (name == "sqrt") {
            if (x.isNegative())
                return domain();
            return math::sqrt(x, kDigits);
        }
        if (name == "cbrt")
            return math::cbrt(x, kDigits);
        if (name == "exp") {
            if (x > Decimal(2303))
                return fail(Error::Overflow, position);
            if (x < Decimal(-2400))
                return Decimal();
            return checked(math::exp(x, kDigits));
        }
        if (name == "ln" || name == "log" || name == "lg") {
            if (x.isNegative() || x.isZero())
                return domain();
            if (name == "ln")
                return math::ln(x, kDigits);
            if (name == "log" && arguments.size() == 1 && x.coefficient() == Natural(1))
                return Decimal(x.exponent());
            Decimal base = Decimal(name == "lg" ? 2 : 10);
            if (arguments.size() == 2) {
                base = arguments[1];
                if (base.isNegative() || base.isZero() || base == one())
                    return domain();
            }
            return Decimal::divide(math::ln(x, working), math::ln(base, working), kDigits);
        }

        if (name == "sin" || name == "cos" || name == "tan" || name == "cot" || name == "sec" || name == "csc") {
            const auto [sine, cosine] = sinCos(x);
            if (name == "sin")
                return sine;
            if (name == "cos")
                return cosine;
            const Decimal& divisor = name == "tan" || name == "sec" ? cosine : sine;
            if (divisor.isZero())
                return domain();
            if (name == "tan")
                return checked(sine / cosine);
            if (name == "cot")
                return checked(cosine / sine);
            return checked(one() / divisor);
        }
        if (name == "asin")
            return arcSine(x, position);
        if (name == "acos") {
            const std::optional<Decimal> sine = arcSine(x, position);
            if (!sine)
                return std::nullopt;
            if (evaluator.m_angleUnit == AngleUnit::Degrees)
                return Decimal(90) - *sine;
            return Decimal::divide(math::pi(working), Decimal(2), working) - *sine;
        }
        if (name == "atan") {
            if (evaluator.m_angleUnit == AngleUnit::Degrees && x.abs() == one())
                return x.isNegative() ? Decimal(-45) : Decimal(45);
            return fromRadians(math::atan(x, working));
        }

        if (name == "sinh" || name == "cosh") {
            if (x.abs() > Decimal(2303))
                return fail(Error::Overflow, position);
            const Decimal grown = math::exp(x, working);
            const Decimal shrunk = Decimal::divide(one(), grown, working);
            const Decimal sum = name == "sinh" ? Decimal::subtract(grown, shrunk, working) : Decimal::add(grown, shrunk, working);
            return checked(Decimal::divide(sum, Decimal(2), kDigits));
        }
        if (name == "tanh") {
            if (x.abs() > Decimal(200))
                return x.isNegative() ? -one() : one();
            const Decimal grown = math::exp(Decimal::multiply(x, Decimal(2), working), working);
            return Decimal::divide(Decimal::subtract(grown, one(), working), Decimal::add(grown, one(), working), kDigits);
        }
        if (name == "asinh") {
            const Decimal magnitude = x.abs();
            const Decimal root = math::sqrt(Decimal::add(Decimal::multiply(x, x, working), one(), working), working);
            const Decimal value = math::ln(Decimal::add(magnitude, root, working), kDigits);
            return x.isNegative() ? -value : value;
        }
        if (name == "acosh") {
            if (x < one())
                return domain();
            const Decimal root = math::sqrt(Decimal::subtract(Decimal::multiply(x, x, working), one(), working), working);
            return math::ln(Decimal::add(x, root, working), kDigits);
        }
        if (name == "atanh") {
            if (x.abs() >= one())
                return domain();
            const Decimal ratio = Decimal::divide(Decimal::add(one(), x, working), Decimal::subtract(one(), x, working), working);
            return Decimal::divide(math::ln(ratio, working), Decimal(2), kDigits);
        }

        if (name == "degrees")
            return checked(Decimal::divide(Decimal::multiply(x, Decimal(180), working), math::pi(working), kDigits));
        if (name == "radians")
            return checked(Decimal::divide(Decimal::multiply(x, math::pi(working), working), Decimal(180), kDigits));

        if (name == "floor")
            return x.roundedToDecimals(0, Decimal::Rounding::Floor);
        if (name == "ceil")
            return checked(x.roundedToDecimals(0, Decimal::Rounding::Ceiling));
        if (name == "round" || name == "trunc") {
            int64_t decimals = 0;
            if (arguments.size() == 2) {
                const std::optional<int64_t> places = arguments[1].toInt64();
                if (!places || *places > 1000 || *places < -1000)
                    return domain();
                decimals = *places;
            }
            return checked(x.roundedToDecimals(
                decimals, name == "round" ? Decimal::Rounding::HalfAwayFromZero : Decimal::Rounding::Truncate));
        }
        if (name == "int")
            return x.roundedToDecimals(0, Decimal::Rounding::Truncate);
        if (name == "frac")
            return x - x.roundedToDecimals(0, Decimal::Rounding::Truncate);
        if (name == "sign")
            return Decimal(x.isZero() ? 0 : x.isNegative() ? -1 : 1);

        if (name == "min")
            return *std::min_element(arguments.begin(), arguments.end());
        if (name == "max")
            return *std::max_element(arguments.begin(), arguments.end());
        if (name == "sum" || name == "average") {
            Decimal sum;
            for (const Decimal& argument : arguments)
                sum = Decimal::add(sum, argument, digitsFor(sum, argument));
            if (name == "average")
                sum = sum / Decimal(int64_t(arguments.size()));
            return checked(sum);
        }
        if (name == "product") {
            Decimal product = one();
            for (const Decimal& argument : arguments) {
                const std::optional<Decimal> next = checked(Decimal::multiply(product, argument, digitsFor(product, argument)));
                if (!next)
                    return std::nullopt;
                product = *next;
            }
            return product;
        }
        if (name == "geomean") {
            // The mean of the logarithms, which never leaves the range.
            Decimal sum;
            for (const Decimal& argument : arguments) {
                if (argument.isNegative() || argument.isZero())
                    return domain();
                sum = Decimal::add(sum, math::ln(argument, working), working);
            }
            return math::exp(Decimal::divide(sum, Decimal(int64_t(arguments.size())), working), kDigits);
        }
        if (name == "gcd" || name == "lcm") {
            if (!std::all_of(arguments.begin(), arguments.end(), integerArgument))
                return domain();
            Natural result;
            bool first = true;
            for (const Decimal& argument : arguments) {
                const Natural value = argument.coefficient().shifted(unsigned(argument.exponent()));
                if (first) {
                    result = value;
                    first = false;
                } else if (name == "gcd") {
                    result = greatestCommonDivisor(result, value);
                } else if (result.isZero() || value.isZero()) {
                    result = Natural();
                } else {
                    Natural quotient;
                    Natural::divide(result, greatestCommonDivisor(result, value), &quotient, nullptr);
                    result = quotient * value;
                    if (result.digitCount() > kLargestExponent + 1)
                        return fail(Error::Overflow, position);
                }
            }
            return Decimal(std::move(result), 0);
        }
        if (name == "ncr" || name == "npr")
            return arrangements(arguments[0], arguments[1], name == "ncr", position);
        return fail(Error::UnknownName, position, name);
    }

    static Natural greatestCommonDivisor(Natural a, Natural b)
    {
        while (!b.isZero()) {
            Natural remainder;
            Natural::divide(a, b, nullptr, &remainder);
            a = std::move(b);
            b = std::move(remainder);
        }
        return a;
    }

    // Combinations or permutations of k out of n.
    std::optional<Decimal> arrangements(const Decimal& n, const Decimal& k, bool combinations, size_t position)
    {
        if (!n.isInteger() || !k.isInteger() || n.isNegative() || k.isNegative())
            return fail(Error::Domain, position);
        if (k > n)
            return Decimal();
        const Natural total = n.coefficient().shifted(unsigned(n.exponent()));
        Natural chosen = k.coefficient().shifted(unsigned(k.exponent()));
        if (combinations) {
            const Natural rest = total - chosen;
            if (rest < chosen)
                chosen = rest;
        }

        // Past 3400 factors of at least 2 the result is above 10^1000; below
        // that, the sum of the factors' logarithms says.
        const std::optional<uint64_t> count = chosen.toUint64();
        if (!count || *count > 3400)
            return fail(Error::Overflow, position);
        double magnitude = 0;
        for (uint64_t i = 1; i <= *count; ++i) {
            magnitude += log10Of(Decimal(total - chosen + Natural(i), 0));
            if (combinations)
                magnitude -= std::log10(double(i));
        }
        if (magnitude > double(kLargestExponent) + 1)
            return fail(Error::Overflow, position);

        Natural result(1);
        for (uint64_t i = 1; i <= *count; ++i) {
            result = result * (total - chosen + Natural(i));
            if (combinations)
                result.divide(uint32_t(i));
        }
        return checked(Decimal(std::move(result), 0));
    }
};

Evaluator::Evaluator()
{
    clearVariables();
}

Evaluator::~Evaluator() = default;

Evaluator::Result Evaluator::evaluate(std::string_view expression)
{
    Result result;
    size_t errorPosition = 0;
    std::optional<std::vector<Token>> tokens = tokenize(expression, &errorPosition);
    if (!tokens) {
        result.error = Error::Syntax;
        result.position = errorPosition;
        return result;
    }
    if (tokens->size() == 1) {
        result.error = Error::Empty;
        return result;
    }

    Parser parser{*this, std::move(*tokens)};
    std::string assigned;
    if (parser.tokens.size() > 2 && parser.tokens[0].kind == TokenKind::Name && parser.tokens[1].kind == TokenKind::Assign) {
        assigned = parser.tokens[0].name;
        if (isReserved(assigned) || assigned == "ans") {
            result.error = Error::ReadOnlyName;
            result.name = assigned;
            return result;
        }
        parser.index = 2;
    }

    const std::optional<Operand> value = parser.expression();
    if (value && parser.peek().kind != TokenKind::End)
        parser.fail(Error::Syntax, parser.peek().position);
    if (parser.failed())
        return parser.failure;

    result.value = value->value;
    m_variables["ans"] = *result.value;
    if (!assigned.empty())
        m_variables[assigned] = *result.value;
    return result;
}

std::optional<Decimal> Evaluator::variable(const std::string& name) const
{
    const auto found = m_variables.find(name);
    if (found == m_variables.end())
        return std::nullopt;
    return found->second;
}

bool Evaluator::setVariable(const std::string& name, const Decimal& value)
{
    if (isReserved(name) || name.empty() || !isNameStart(name[0]))
        return false;
    m_variables[name] = value;
    return true;
}

void Evaluator::clearVariables()
{
    m_variables.clear();
    m_variables["ans"] = Decimal();
}

bool Evaluator::isReserved(const std::string& name)
{
    return findFunction(name) || isConstant(name) || isOperatorWord(name);
}
