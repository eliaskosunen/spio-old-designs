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

#ifndef SPIO_LOCALE_CTYPE_H
#define SPIO_LOCALE_CTYPE_H

#include <array>
#include <cctype>
#include "depend/span.h"
#include "locale/facet.h"

namespace spio {
struct ctype_base {
    using mask = int;
    static const int space = 1 << 0;
    static const int print = 1 << 1;
    static const int cntrl = 1 << 2;
    static const int upper = 1 << 3;
    static const int lower = 1 << 4;
    static const int alpha = 1 << 5;
    static const int digit = 1 << 6;
    static const int punct = 1 << 7;
    static const int xdigit = 1 << 8;
    static const int blank = 1 << 9;
    static const int alnum = alpha | digit;
    static const int graph = alnum | punct;
};

template <typename Char>
class ctype : public detail::facet, public ctype_base {
public:
    using char_type = Char;

    bool is(mask m, char_type c) const
    {
        return do_is(m, c);
    }
    std::size_t is(span<const char_type> in, span<mask> out) const
    {
        return do_is(in, out);
    }
    std::size_t scan_is(mask m, span<const char_type> s) const
    {
        return do_scan_in(m, s);
    }
    std::size_t scan_not(mask m, span<const char_type> s) const
    {
        return do_scan_not(m, s);
    }

protected:
    virtual bool do_is(mask m, char_type c) const = 0;
    virtual std::size_t do_is(span<const char_type> in,
                              span<mask> out) const = 0;
    virtual std::size_t do_scan_in(mask k, span<const char_type> s) const = 0;
    virtual std::size_t do_scan_not(mask k, span<const char_type> s) const = 0;
};

template <>
class ctype<char> : public detail::facet, public ctype_base {
public:
    using char_type = char;
    using table_type = std::array<mask, 256>;

    const table_type& table() const
    {
        return m_table;
    }

    static table_type classic_table()
    {
        static table_type t{{
            cntrl,  // 0
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,                  // 8
            space | blank | cntrl,  // 9 '\t'
            space | cntrl,
            space | cntrl,
            space | cntrl,
            space | cntrl,
            cntrl,  // 14
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            cntrl,
            space | blank | print,  // 32 ' '
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            digit | xdigit | print,  // 48 '0'
            digit | xdigit | print,
            digit | xdigit | print,
            digit | xdigit | print,
            digit | xdigit | print,
            digit | xdigit | print,
            digit | xdigit | print,
            digit | xdigit | print,
            digit | xdigit | print,
            digit | xdigit | print,
            punct | print,  // 58 ':'
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            upper | alpha | xdigit | print,  // 65 'A'
            upper | alpha | xdigit | print,
            upper | alpha | xdigit | print,
            upper | alpha | xdigit | print,
            upper | alpha | xdigit | print,
            upper | alpha | xdigit | print,
            upper | alpha | print,  // 71 'G'
            upper | alpha | print,
            upper | alpha | print,
            upper | alpha | print,
            upper | alpha | print,
            upper | alpha | print,
            upper | alpha | print,
            upper | alpha | print,
            upper | alpha | print,
            upper | alpha | print,  // 80 'P'
            upper | alpha | print,
            upper | alpha | print,
            upper | alpha | print,
            upper | alpha | print,
            upper | alpha | print,
            upper | alpha | print,
            upper | alpha | print,
            upper | alpha | print,
            upper | alpha | print,
            upper | alpha | print,
            punct | print,  // 91 '['
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            punct | print,
            lower | alpha | xdigit | print,  // 97 'a'
            lower | alpha | xdigit | print,
            lower | alpha | xdigit | print,
            lower | alpha | xdigit | print,
            lower | alpha | xdigit | print,
            lower | alpha | xdigit | print,
            lower | alpha | print,  // 103 'g'
            lower | alpha | print,
            lower | alpha | print,
            lower | alpha | print,
            lower | alpha | print,
            lower | alpha | print,
            lower | alpha | print,
            lower | alpha | print,  // 110 'n'
            lower | alpha | print,
            lower | alpha | print,
            lower | alpha | print,
            lower | alpha | print,
            lower | alpha | print,
            lower | alpha | print,
            lower | alpha | print,
            lower | alpha | print,
            lower | alpha | print,
            lower | alpha | print,
            lower | alpha | print,
            lower | alpha | print,
            punct | print,  // 123 '{'
            punct | print,
            punct | print,
            punct | print,
            cntrl  // 127
        }};
        return t;
    }

    bool is(mask m, char_type c) const
    {
        return (table()[static_cast<std::size_t>(c)] & m) == 0;
    }
    std::size_t is(span<const char_type> in, span<mask> out) const
    {
        auto out_it = out.begin();
        const auto& t = table();
        std::size_t i = 0;
        for (; i != in.size_us(); ++i) {
            *out_it =
                t[static_cast<std::size_t>(in[static_cast<std::ptrdiff_t>(i)])];
            ++out_it;
        }
        return i;
    }
    std::size_t scan_is(mask m, span<const char_type> s) const
    {
        std::size_t i = 0;
        for (; i < s.size_us() && is(m, s[static_cast<std::ptrdiff_t>(i)]);
             ++i) {
        }
        return i;
    }
    std::size_t scan_not(mask m, span<const char_type> s) const
    {
        std::size_t i = 0;
        for (; i < s.size_us() && !is(m, s[static_cast<std::ptrdiff_t>(i)]);
             ++i) {
        }
        return i;
    }

private:
    table_type m_table{classic_table()};
};

template <>
class ctype<wchar_t> : public detail::facet, public ctype_base {
public:
    using char_type = wchar_t;

    bool is(mask m, char_type c) const
    {
        return (get_mask(c) & m) == 0;
    }
    std::size_t is(span<const char_type> in, span<mask> out) const
    {
        auto out_it = out.begin();
        std::size_t i = 0;
        for (; i != in.size_us(); ++i) {
            *out_it = get_mask(in[static_cast<std::ptrdiff_t>(i)]);
            ++out_it;
        }
        return i;
    }
    std::size_t scan_is(mask m, span<const char_type> s) const
    {
        std::size_t i = 0;
        for (; i < s.size_us() && is(m, s[static_cast<std::ptrdiff_t>(i)]);
             ++i) {
        }
        return i;
    }
    std::size_t scan_not(mask m, span<const char_type> s) const
    {
        std::size_t i = 0;
        for (; i < s.size_us() && !is(m, s[static_cast<std::ptrdiff_t>(i)]);
             ++i) {
        }
        return i;
    }

private:
    int get_mask(char_type ch) const
    {
        auto c = static_cast<std::wint_t>(ch);
        return (std::iswspace(c) != 0 && space) |
               (std::iswprint(c) != 0 && print) |
               (std::iswcntrl(c) != 0 && cntrl) |
               (std::iswupper(c) != 0 && upper) |
               (std::iswlower(c) != 0 && lower) |
               (std::iswalpha(c) != 0 && alpha) |
               (std::iswdigit(c) != 0 && digit) |
               (std::iswpunct(c) != 0 && punct) |
               (std::iswxdigit(c) != 0 && xdigit) |
               (std::iswblank(c) != 0 && blank);
    }
};
}  // namespace spio

#endif  // SPIO_LOCALE_CTYPE_H
