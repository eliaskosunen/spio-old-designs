// Copyright 2017 Elias Kosunen
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
namespace detail {
    template <typename T, typename Deleter>
    constexpr maybe_owned_ptr<T, Deleter>::maybe_owned_ptr(T* o,
                                                           bool owned) noexcept
        : m_obj(o), m_owned(owned)
    {
    }
    template <typename T, typename Deleter>
    constexpr maybe_owned_ptr<T, Deleter>::maybe_owned_ptr(
        std::nullptr_t o) noexcept
        : m_obj(o), m_owned(false)
    {
    }
    template <typename T, typename Deleter>
    template <typename D>
    maybe_owned_ptr<T, Deleter>::maybe_owned_ptr(T* o, bool owned, D&& deleter)
        : m_obj(o),
          m_deleter(std::forward<decltype(deleter)>(deleter)),
          m_owned(owned)
    {
    }

    template <typename T, typename Deleter>
    constexpr maybe_owned_ptr<T, Deleter>::maybe_owned_ptr(
        maybe_owned_ptr&& other) noexcept
        : m_obj(std::move(other.m_obj)),
          m_deleter(std::move(other.m_deleter)),
          m_owned(std::move(other.m_owned))
    {
        other.m_obj = nullptr;
        other.m_owned = false;
    }
    template <typename T, typename Deleter>
    constexpr maybe_owned_ptr<T, Deleter>& maybe_owned_ptr<T, Deleter>::
    operator=(maybe_owned_ptr&& other) noexcept
    {
        m_obj = std::move(other.m_obj);
        m_deleter = std::move(other.m_deleter);
        m_owned = std::move(other.m_owned);

        other.m_obj = nullptr;
        other.m_owned = false;

        return *this;
    }

    template <typename T, typename Deleter>
    maybe_owned_ptr<T, Deleter>::~maybe_owned_ptr() noexcept
    {
        if (m_owned) {
            if (!m_obj) {
                std::fprintf(stderr, "Owned ptr is null");
                std::terminate();
            }
            m_deleter(m_obj);
        }
        m_obj = nullptr;
    }

    template <typename T, typename Deleter>
    constexpr bool maybe_owned_ptr<T, Deleter>::has_value() const
    {
        return m_obj;
    }
    template <typename T, typename Deleter>
    constexpr maybe_owned_ptr<T, Deleter>::operator bool() const
    {
        return has_value();
    }

    template <typename T, typename Deleter>
    constexpr T* maybe_owned_ptr<T, Deleter>::value() const
    {
        return m_obj;
    }
    template <typename T, typename Deleter>
    constexpr void maybe_owned_ptr<T, Deleter>::value(T* val)
    {
        m_obj = val;
    }
    template <typename T, typename Deleter>
    constexpr bool maybe_owned_ptr<T, Deleter>::owned() const
    {
        return m_owned;
    }
    template <typename T, typename Deleter>
    constexpr void maybe_owned_ptr<T, Deleter>::owned(bool val)
    {
        m_owned = val;
    }
}  // namespace detail

template <typename InputIt>
constexpr std::size_t distance_nonneg(InputIt first, InputIt last)
{
    SPIO_ASSERT(first < last, "distance_nonneg requires first < last");
    const auto dist = std::distance(first, last);
    SPIO_ASSERT(
        dist >= 0,
        "distance_nonneg requires distance between first and last to be 0 "
        "or more");
    return static_cast<std::size_t>(dist);
}

namespace detail {
    template <typename FloatingT>
    static constexpr auto powersOf10()
    {
        using T = std::decay_t<FloatingT>;
        if constexpr (std::is_same_v<T, float>) {
            return array<float, 6>{
                {10.f, 100.f, 1.0e4f, 1.0e8f, 1.0e16f, 1.0e32f}};
        }
        else if constexpr (std::is_same_v<T, double>) {
            return array<double, 9>{{10., 100., 1.0e4, 1.0e8, 1.0e16, 1.0e32,
                                     1.0e64, 1.0e128, 1.0e256}};
        }
        else {
            return array<long double, 11>{{10.l, 100.l, 1.0e4l, 1.0e8l, 1.0e16l,
                                           1.0e32l, 1.0e64l, 1.0e128l, 1.0e256l,
                                           1.0e512l, 1.0e1024l}};
        }
    }

    template <typename FloatingT>
    static constexpr auto maxExponent()
    {
        using T = std::decay_t<FloatingT>;
        if constexpr (std::is_same_v<T, float>) {
            return 63;
        }
        else if constexpr (std::is_same_v<T, double>) {
            return 511;
        }
        else {
            return 2047;
        }
    }
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