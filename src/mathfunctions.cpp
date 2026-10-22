// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "mathfunctions.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace math {

static const Decimal kOne(1);
static const Decimal kTwo(2);

// Whether a series term no longer changes a sum near one at these digits.
static bool negligible(const Decimal& term, int digits)
{
    return term.isZero() || term.adjustedExponent() < -int64_t(digits) - 2;
}

// atan(1 / n) for a whole n > 1, by its alternating series.
static Decimal arctanInverse(uint32_t n, int digits)
{
    const Decimal square(int64_t(n) * n);
    Decimal power = Decimal::divide(kOne, Decimal(n), digits);
    Decimal sum = power;
    for (int64_t k = 1;; ++k) {
        power = Decimal::divide(power, square, digits);
        const Decimal term = Decimal::divide(power, Decimal(2 * k + 1), digits);
        if (negligible(term, digits))
            break;
        sum = k % 2 ? Decimal::subtract(sum, term, digits) : Decimal::add(sum, term, digits);
    }
    return sum;
}

Decimal pi(int digits)
{
    static std::map<int, Decimal> cache;
    const auto cached = cache.find(digits);
    if (cached != cache.end())
        return cached->second;
    // Machin's formula: pi = 16 atan(1/5) - 4 atan(1/239).
    const int working = digits + 10;
    const Decimal value = Decimal::subtract(Decimal::multiply(Decimal(16), arctanInverse(5, working), working),
                                            Decimal::multiply(Decimal(4), arctanInverse(239, working), working), working)
                              .rounded(digits);
    cache.emplace(digits, value);
    return value;
}

void divideTruncated(const Decimal& a, const Decimal& b, Decimal* quotient, Decimal* remainder)
{
    const int64_t exponent = std::min(a.exponent(), b.exponent());
    const Natural x = a.coefficient().shifted(unsigned(a.exponent() - exponent));
    const Natural y = b.coefficient().shifted(unsigned(b.exponent() - exponent));
    Natural q;
    Natural r;
    Natural::divide(x, y, &q, &r);
    if (quotient)
        *quotient = Decimal(std::move(q), 0, a.isNegative() != b.isNegative());
    if (remainder)
        *remainder = Decimal(std::move(r), exponent, a.isNegative());
}

Decimal exp(const Decimal& x, int digits)
{
    if (x.isZero())
        return kOne;
    // exp(x) = exp(x / 2^k)^(2^k), with x / 2^k small enough for the series
    // to converge fast; each squaring doubles the error, so more digits.
    const double magnitude = std::fabs(x.toDouble());
    const int halvings = magnitude > 1e-3 ? int(std::ceil(std::log2(magnitude / 1e-3))) : 0;
    const int working = digits + 12 + halvings / 3 + int(std::max(0.0, std::log10(magnitude)));
    Decimal reduced = x;
    if (halvings > 0)
        reduced = Decimal::divide(x, power(kTwo, Natural(uint64_t(halvings)), working), working);

    Decimal sum = kOne;
    Decimal term = kOne;
    for (int64_t n = 1;; ++n) {
        term = Decimal::divide(Decimal::multiply(term, reduced, working), Decimal(n), working);
        if (negligible(term, working))
            break;
        sum = Decimal::add(sum, term, working);
    }
    for (int i = 0; i < halvings; ++i)
        sum = Decimal::multiply(sum, sum, working);
    return sum.rounded(digits);
}

Decimal ln(const Decimal& x, int digits)
{
    if (x == kOne)
        return {};
    // Halley's iteration on exp(y) = x from the double's logarithm; each step
    // triples the correct digits.
    const int working = digits + 10;
    const double leading = Decimal(x.coefficient(), -int64_t(x.coefficient().digitCount()) + 1).toDouble();
    Decimal y = Decimal::fromDouble(std::log(leading) + double(x.adjustedExponent()) * std::log(10.0));
    for (int i = 0; i < 12; ++i) {
        const Decimal e = exp(y, working);
        const Decimal step = Decimal::divide(Decimal::multiply(kTwo, Decimal::subtract(x, e, working), working),
                                             Decimal::add(x, e, working), working);
        y = Decimal::add(y, step, working);
        if (step.isZero() || step.adjustedExponent() < std::max<int64_t>(y.adjustedExponent(), 0) - working + 2)
            break;
    }
    return y.rounded(digits);
}

// x × 10^(n·k): the decimal point moved, exactly.
static Decimal scaledByPowerOfTen(const Decimal& x, int64_t exponent)
{
    return Decimal(x.coefficient(), x.exponent() + exponent, x.isNegative());
}

// The n-th root of x > 0 by Newton's method, on a mantissa between 1 and 10^n
// so that the double estimate is in range; exact when the root is.
static Decimal root(const Decimal& x, unsigned n, int digits)
{
    const int working = digits + 10;
    const int64_t adjusted = x.adjustedExponent();
    const int64_t shift = adjusted >= 0 ? adjusted / n : -((-adjusted + n - 1) / n);
    const Decimal mantissa = scaledByPowerOfTen(x, -shift * n);
    Decimal y = Decimal::fromDouble(std::pow(mantissa.toDouble(), 1.0 / n));
    const Decimal order(n);
    const Decimal lower(n - 1);
    for (int i = 0; i < 20; ++i) {
        Decimal power = kOne;
        for (unsigned j = 0; j + 1 < n; ++j)
            power = Decimal::multiply(power, y, working);
        const Decimal next = Decimal::divide(
            Decimal::add(Decimal::multiply(lower, y, working), Decimal::divide(mantissa, power, working), working), order,
            working);
        const Decimal change = Decimal::subtract(next, y, working);
        y = next;
        if (change.isZero() || change.adjustedExponent() < -working + 3)
            break;
    }

    // An exact root shows as one after rounding off the last few digits.
    const Decimal candidate = y.rounded(digits - 5);
    Decimal check = kOne;
    for (unsigned j = 0; j < n; ++j)
        check = Decimal::multiply(check, candidate, 100000);
    if (check == mantissa)
        y = candidate;
    return scaledByPowerOfTen(y.rounded(digits), shift);
}

Decimal sqrt(const Decimal& x, int digits)
{
    if (x.isZero())
        return {};
    return root(x, 2, digits);
}

Decimal cbrt(const Decimal& x, int digits)
{
    if (x.isZero())
        return {};
    const Decimal magnitude = root(x.abs(), 3, digits);
    return x.isNegative() ? -magnitude : magnitude;
}

Decimal power(const Decimal& x, const Natural& exponent, int digits)
{
    const int working = digits + 10 + int(exponent.digitCount());
    Decimal result = kOne;
    Decimal base = x;
    Natural rest = exponent;
    while (!rest.isZero()) {
        if (rest.isOdd())
            result = Decimal::multiply(result, base, working);
        rest.divide(2);
        if (!rest.isZero())
            base = Decimal::multiply(base, base, working);
    }
    return result.rounded(digits);
}

// The series for |r| <= pi/4.
static Decimal sinSeries(const Decimal& r, int digits)
{
    const Decimal square = Decimal::multiply(r, r, digits);
    Decimal term = r;
    Decimal sum = r;
    for (int64_t n = 1;; ++n) {
        term = Decimal::divide(Decimal::multiply(term, square, digits), Decimal(-(2 * n) * (2 * n + 1)), digits);
        if (negligible(term, digits))
            break;
        sum = Decimal::add(sum, term, digits);
    }
    return sum;
}

static Decimal cosSeries(const Decimal& r, int digits)
{
    const Decimal square = Decimal::multiply(r, r, digits);
    Decimal term = kOne;
    Decimal sum = kOne;
    for (int64_t n = 1;; ++n) {
        term = Decimal::divide(Decimal::multiply(term, square, digits), Decimal(-(2 * n - 1) * (2 * n)), digits);
        if (negligible(term, digits))
            break;
        sum = Decimal::add(sum, term, digits);
    }
    return sum;
}

// The sine and cosine of quadrant × 90° + an angle within the first quadrant,
// given the angle's sine and cosine.
static std::pair<Decimal, Decimal> inQuadrant(int quadrant, const Decimal& sine, const Decimal& cosine)
{
    switch (quadrant) {
    case 1:
        return {cosine, -sine};
    case 2:
        return {-sine, -cosine};
    case 3:
        return {-cosine, sine};
    default:
        return {sine, cosine};
    }
}

// The quadrant of an integer count of right angles.
static int quadrantOf(const Decimal& count)
{
    Decimal remainder;
    divideTruncated(count, Decimal(4), nullptr, &remainder);
    const int quadrant = int(*remainder.toInt64());
    return quadrant < 0 ? quadrant + 4 : quadrant;
}

std::pair<Decimal, Decimal> sinCosDegrees(const Decimal& degrees, int digits)
{
    const int working = digits + 10;
    const Decimal right(90);
    Decimal count;
    Decimal angle;
    divideTruncated(degrees, right, &count, &angle);
    if (angle.isNegative()) {
        angle = angle + right;
        count = count - kOne;
    }
    const int quadrant = quadrantOf(count);

    // Angles whose sine and cosine are whole or half come out exact.
    if (angle.isZero())
        return inQuadrant(quadrant, Decimal(), kOne);
    const Decimal half = *Decimal::parse("0.5");
    if (angle == Decimal(30))
        return inQuadrant(quadrant, half, sqrt(Decimal(3), working).rounded(digits) * half);
    if (angle == Decimal(60))
        return inQuadrant(quadrant, sqrt(Decimal(3), working).rounded(digits) * half, half);
    if (angle == Decimal(45)) {
        const Decimal value = Decimal::multiply(sqrt(kTwo, working), half, digits);
        return inQuadrant(quadrant, value, value);
    }

    const Decimal toRadians = Decimal::divide(pi(working), Decimal(180), working);
    if (angle > Decimal(45)) {
        const Decimal r = Decimal::multiply(right - angle, toRadians, working);
        return inQuadrant(quadrant, cosSeries(r, working).rounded(digits), sinSeries(r, working).rounded(digits));
    }
    const Decimal r = Decimal::multiply(angle, toRadians, working);
    return inQuadrant(quadrant, sinSeries(r, working).rounded(digits), cosSeries(r, working).rounded(digits));
}

std::pair<Decimal, Decimal> sinCosRadians(const Decimal& radians, int digits)
{
    if (radians.isZero())
        return {Decimal(), kOne};
    // Reducing a large angle needs pi to the angle's digits before the point too.
    const int working = digits + 10 + int(std::max<int64_t>(0, radians.adjustedExponent()));
    const Decimal right = Decimal::divide(pi(working), kTwo, working);
    const Decimal count = Decimal::divide(radians, right, working).roundedToDecimals(0, Decimal::Rounding::Floor);
    const Decimal angle = Decimal::subtract(radians, Decimal::multiply(count, right, working), working);
    const int quadrant = quadrantOf(count);
    if (angle > Decimal::divide(right, kTwo, working)) {
        const Decimal r = Decimal::subtract(right, angle, working);
        return inQuadrant(quadrant, cosSeries(r, working).rounded(digits), sinSeries(r, working).rounded(digits));
    }
    return inQuadrant(quadrant, sinSeries(angle, working).rounded(digits), cosSeries(angle, working).rounded(digits));
}

Decimal atan(const Decimal& x, int digits)
{
    if (x.isZero())
        return {};
    const int working = digits + 10;
    if (x.isNegative())
        return -atan(-x, digits);
    if (x > kOne)
        return Decimal::subtract(Decimal::divide(pi(working), kTwo, working),
                                 atan(Decimal::divide(kOne, x, working), working), working)
            .rounded(digits);

    // atan(x) = 2 atan(x / (1 + sqrt(1 + x²))) until the series converges fast.
    Decimal y = x;
    int doublings = 0;
    const Decimal small = *Decimal::parse("0.05");
    while (y > small) {
        const Decimal hypotenuse = sqrt(Decimal::add(kOne, Decimal::multiply(y, y, working), working), working);
        y = Decimal::divide(y, Decimal::add(kOne, hypotenuse, working), working);
        ++doublings;
    }
    const Decimal square = Decimal::multiply(y, y, working);
    Decimal power = y;
    Decimal sum = y;
    for (int64_t k = 1;; ++k) {
        power = Decimal::multiply(power, square, working);
        const Decimal term = Decimal::divide(power, Decimal(2 * k + 1), working);
        if (negligible(term, working))
            break;
        sum = k % 2 ? Decimal::subtract(sum, term, working) : Decimal::add(sum, term, working);
    }
    for (int i = 0; i < doublings; ++i)
        sum = Decimal::multiply(sum, kTwo, working);
    return sum.rounded(digits);
}

} // namespace math
