// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "decimal.h"

#include <utility>

// Elementary functions on decimals, each correct to the significant digits
// asked for but for the last one or two. Arguments are inside the function's
// domain and small enough for the result to be represented; the evaluator
// checks both.
namespace math {

Decimal pi(int digits);

// The integer part of a / b toward zero, and what is left with the sign of
// a, both exact; b is not zero.
void divideTruncated(const Decimal& a, const Decimal& b, Decimal* quotient, Decimal* remainder);

Decimal exp(const Decimal& x, int digits);
// x > 0.
Decimal ln(const Decimal& x, int digits);
// x >= 0; exact when the root is.
Decimal sqrt(const Decimal& x, int digits);
// Exact when the root is.
Decimal cbrt(const Decimal& x, int digits);
// An integer power, by squaring.
Decimal power(const Decimal& x, const Natural& exponent, int digits);

// The sine and the cosine; exact zeros and halves where the angle gives them.
std::pair<Decimal, Decimal> sinCosDegrees(const Decimal& degrees, int digits);
std::pair<Decimal, Decimal> sinCosRadians(const Decimal& radians, int digits);
// In radians.
Decimal atan(const Decimal& x, int digits);

} // namespace math
