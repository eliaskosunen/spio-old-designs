// Copyright 2017-2018 Elias Kosunen
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include "util.h"

namespace io {
inline bool is_eof(error c)
{
    return c.is_eof();
}

template <typename Dest, typename Source>
Dest bit_cast(const Source& s)
{
    static_assert(sizeof(Dest) == sizeof(Source),
                  "bit_cast<>: sizeof Dest and Source must be equal");
    static_assert(std::is_trivially_copyable<Dest>::value,
                  "bit_cast<>: Dest must be TriviallyCopyable");
    static_assert(std::is_trivially_copyable<Source>::value,
                  "bit_cast<>: Source must be TriviallyCopyable");

    Dest d;
    std::memcpy(&d, &s, sizeof(Dest));
    return d;
}

template <typename InputIt>
constexpr std::size_t distance_nonneg(InputIt first, InputIt last)
{
    SPIO_ASSERT(first < last, "distance_nonneg requires first < last");
    const auto dist = distance(first, last);
    SPIO_ASSERT(
        dist >= 0,
        "distance_nonneg requires distance between first and last to be 0 "
        "or more");
    return static_cast<std::size_t>(dist);
}

template <typename CharT>
constexpr bool is_space(CharT c, span<CharT> spaces)
{
    if (spaces.empty()) {
        //       space         \n        \t         \r         \v
        return c == 32 || c == 10 || c == 9 || c == 13 || c == 11;
    }
    return stl::find(spaces.begin(), spaces.end(), c) != spaces.end();
}

template <typename CharT>
constexpr bool is_digit(CharT c, int base)
{
    assert(base >= 2 && base <= 36);
    if (base <= 10) {
        return c >= '0' && c <= '0' + (base - 1);
    }
    return is_digit(c, 10) || (c >= 'a' && c <= 'a' + (base - 1)) ||
           (c >= 'A' && c <= 'A' + (base - 1));
}

template <typename IntT, typename CharT>
constexpr IntT char_to_int(CharT c, int base)
{
    assert(base >= 2 && base <= 36);
    assert(is_digit(c, base));
    if (base <= 10) {
        assert(c <= '0' + (base - 1));
        return static_cast<IntT>(c - '0');
    }
    if (c <= '9') {
        return static_cast<IntT>(c - '0');
    }
    if (c >= 'a' && c <= 'z') {
        return 10 + static_cast<IntT>(c - 'a');
    }
    auto ret = 10 + static_cast<IntT>(c - 'A');
    return ret;
}

namespace detail {
    template <typename CharT, typename IntT>
    constexpr void itoa(IntT n, CharT* s, int base)
    {
        {
            IntT sign{0};

#if SPIO_HAS_IF_CONSTEXPR
            if constexpr (std::is_signed<IntT>::value)
#endif
            {
                if ((sign = n) < 0) /* record sign */
                {
                    n = -n; /* make n positive */
                }
            }

            IntT i = 0;
            auto casted_base = static_cast<IntT>(base);

            auto nth_digit = [casted_base](IntT num) -> CharT {
                auto tmp = num % casted_base;
                if (casted_base <= 10 || tmp < 10) {
                    return static_cast<CharT>(tmp) + CharT{'0'};
                }
                return static_cast<CharT>(tmp - 10) + CharT{'a'};
            };
            do {                       /* generate digits in reverse order */
                s[i++] = nth_digit(n); /* get next digit */
            } while ((n /= casted_base) > 0); /* delete it */
            if (sign < 0) {
                s[i++] = CharT{'-'};
            }
            s[i] = '\0';
        }

        auto reverse = [](CharT* str) {
            for (std::ptrdiff_t i = 0, j = stl::strlen(str) - 1; i < j;
                 i++, j--) {
                CharT tmp = str[i];
                str[i] = str[j];
                str[j] = tmp;
            }
        };
        reverse(s);
    }
}  // namespace detail

template <typename CharT, typename IntT>
constexpr void int_to_char(IntT value, span<CharT> result, int base)
{
    assert(base >= 2 && base <= 36);

    detail::itoa(value, &result[0], base);
}

template <typename IntT>
constexpr int max_digits() noexcept
{
    auto i = std::numeric_limits<IntT>::max();

    int digits = 0;
    while (i) {
        i /= 10;
        digits++;
    }
#if SPIO_HAS_IF_CONSTEXPR && SPIO_HAS_TYPE_TRAITS_V
    if constexpr (std::is_signed_v<IntT>) {
#else
    if (std::is_signed<IntT>::value) {
#endif
        return digits + 1;
    }
    else {
        return digits;
    }
}

namespace detail {
    template <typename A, typename B>
    static constexpr bool is_same()
    {
#if SPIO_HAS_TYPE_TRAITS_V
        return std::is_same_v<A, B>;
#else
        return std::is_same<A, B>::value;
#endif
    }

#if SPIO_HAS_IF_CONSTEXPR
    template <typename FloatingT>
    static constexpr auto powersOf10()
    {
        using T = std::decay_t<FloatingT>;
        if constexpr (is_same<T, float>()) {
            return stl::array<float, 6>{
                {10.f, 100.f, 1.0e4f, 1.0e8f, 1.0e16f, 1.0e32f}};
        }
        if constexpr (is_same<T, double>()) {
            return stl::array<double, 9>{{10., 100., 1.0e4, 1.0e8, 1.0e16,
                                          1.0e32, 1.0e64, 1.0e128, 1.0e256}};
        }
        else {
#ifdef _MSC_VER
            return stl::array<long double, 9>{{10.l, 100.l, 1.0e4l, 1.0e8l,
                                               1.0e16l, 1.0e32l, 1.0e64l,
                                               1.0e128l, 1.0e256l}};
#else
            return stl::array<long double, 11>{
                {10.l, 100.l, 1.0e4l, 1.0e8l, 1.0e16l, 1.0e32l, 1.0e64l,
                 1.0e128l, 1.0e256l, 1.0e512l, 1.0e1024l}};
#endif
        }
    }

    template <typename FloatingT>
    static constexpr auto maxExponent()
    {
        using T = std::decay_t<FloatingT>;
        if constexpr (is_same<T, float>()) {
            return 63;
        }
        if constexpr (is_same<T, double>()) {
            return 511;
        }
        else {
            return 2047;
        }
    }
#else
    template <typename FloatingT>
    constexpr auto powersOf10()
    {
#ifdef _MSC_VER
        return stl::array<long double, 11>{{10.l, 100.l, 1.0e4l, 1.0e8l,
                                            1.0e16l, 1.0e32l, 1.0e64l, 1.0e128l,
                                            1.0e256l}};
#else
        return stl::array<long double, 11>{{10.l, 100.l, 1.0e4l, 1.0e8l,
                                            1.0e16l, 1.0e32l, 1.0e64l, 1.0e128l,
                                            1.0e256l, 1.0e512l, 1.0e1024l}};
#endif
    }
    template <>
    constexpr auto powersOf10<float>()
    {
        return stl::array<float, 6>{
            {10.f, 100.f, 1.0e4f, 1.0e8f, 1.0e16f, 1.0e32f}};
    }
    template <>
    constexpr auto powersOf10<double>()
    {
        return stl::array<double, 9>{{10., 100., 1.0e4, 1.0e8, 1.0e16, 1.0e32,
                                      1.0e64, 1.0e128, 1.0e256}};
    }

    template <typename FloatingT>
    constexpr auto maxExponent()
    {
        return 2047;
    }
    template <>
    constexpr auto maxExponent<float>()
    {
        return 63;
    }
    template <>
    constexpr auto maxExponent<double>()
    {
        return 511;
    }
#endif
}  // namespace detail

/*
 * The original C implementation of this function:
 *
 * strtod.c --
 *
 *	Source code for the "strtod" library procedure.
 *
 * Copyright (c) 1988-1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */
template <typename FloatingT, typename CharT>
FloatingT str_to_floating(const CharT* str, CharT** end)
{
    static int maxExponent = detail::maxExponent<FloatingT>();
    static auto powersOf10 = detail::powersOf10<FloatingT>();

    bool sign;
    bool expSign = false;
    FloatingT fraction, dblExp, *d;
    const CharT* p;
    int c;
    int exp = 0;       /* Exponent read from "EX" field. */
    int fracExp = 0;   /* Exponent that derives from the fractional
                        * part.  Under normal circumstatnces, it is
                        * the negative of the number of digits in F.
                        * However, if I is very long, the last digits
                        * of I get dropped (otherwise a long I with a
                        * large negative exponent could cause an
                        * unnecessary overflow on I alone).  In this
                        * case, fracExp is incremented one for each
                        * dropped digit. */
    int mantSize;      /* Number of digits in mantissa. */
    int decPt;         /* Number of mantissa digits BEFORE decimal
                        * point. */
    const CharT* pExp; /* Temporarily holds location of exponent
                        * in string. */

    /*
     * Strip off leading blanks and check for a sign.
     */

    p = str;
    while (std::isspace(*p)) {
        p += 1;
    }
    if (*p == '-') {
        sign = true;
        p += 1;
    }
    else {
        if (*p == '+') {
            p += 1;
        }
        sign = false;
    }

    /*
     * Count the number of digits in the mantissa (including the decimal
     * point), and also locate the decimal point.
     */

    decPt = -1;
    for (mantSize = 0;; mantSize += 1) {
        c = *p;
        if (!std::isdigit(c)) {
            if ((c != '.') || (decPt >= 0)) {
                break;
            }
            decPt = mantSize;
        }
        p += 1;
    }

    /*
     * Now suck up the digits in the mantissa.  Use two integers to
     * collect 9 digits each (this is faster than using floating-point).
     * If the mantissa has more than 18 digits, ignore the extras, since
     * they can't affect the value anyway.
     */

    pExp = p;
    p -= mantSize;
    if (decPt < 0) {
        decPt = mantSize;
    }
    else {
        mantSize -= 1; /* One of the digits was the point. */
    }
    if (mantSize > 18) {
        fracExp = decPt - 18;
        mantSize = 18;
    }
    else {
        fracExp = decPt - mantSize;
    }
    if (mantSize == 0) {
        fraction = static_cast<FloatingT>(0.0);
        p = str;
        goto done;
    }
    else {
        int frac1, frac2;
        frac1 = 0;
        for (; mantSize > 9; mantSize -= 1) {
            c = *p;
            p += 1;
            if (c == '.') {
                c = *p;
                p += 1;
            }
            frac1 = 10 * frac1 + (c - '0');
        }
        frac2 = 0;
        for (; mantSize > 0; mantSize -= 1) {
            c = *p;
            p += 1;
            if (c == '.') {
                c = *p;
                p += 1;
            }
            frac2 = 10 * frac2 + (c - '0');
        }
        fraction = (static_cast<FloatingT>(1.0e9) * frac1) + frac2;
    }

    /*
     * Skim off the exponent.
     */

    p = pExp;
    if ((*p == 'E') || (*p == 'e')) {
        p += 1;
        if (*p == '-') {
            expSign = true;
            p += 1;
        }
        else {
            if (*p == '+') {
                p += 1;
            }
            expSign = false;
        }
        while (std::isdigit(*p)) {
            exp = exp * 10 + (*p - '0');
            p += 1;
        }
    }
    if (expSign) {
        exp = fracExp - exp;
    }
    else {
        exp = fracExp + exp;
    }

    /*
     * Generate a floating-point number that represents the exponent.
     * Do this by processing the exponent one bit at a time to combine
     * many powers of 2 of 10. Then combine the exponent with the
     * fraction.
     */

    if (exp < 0) {
        expSign = true;
        exp = -exp;
    }
    else {
        expSign = false;
    }
    if (exp > maxExponent) {
        exp = maxExponent;
        errno = ERANGE;
    }
    dblExp = static_cast<FloatingT>(1.0);
    for (d = &powersOf10[0]; exp != 0; exp >>= 1, d += 1) {
        if (exp & 01) {
            dblExp *= *d;
        }
    }
    if (expSign) {
        fraction /= dblExp;
    }
    else {
        fraction *= dblExp;
    }

done:
    if (end != nullptr) {
        *end = const_cast<CharT*>(p);
    }

    if (sign) {
        return -fraction;
    }
    return fraction;
}
}  // namespace io
