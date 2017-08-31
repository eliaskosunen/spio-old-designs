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

#ifndef SPIO_TYPE_H
#define SPIO_TYPE_H

#include <cmath>
#include "config.h"
#include "reader_options.h"
#include "stl.h"
#include "util.h"
#include "writer_options.h"

namespace io {
template <typename T, typename Enable = void>
struct type;

template <typename T>
struct type<T,
            std::enable_if_t<contains<std::decay_t<T>,
                                      char,
                                      wchar_t,
                                      unsigned char,
                                      signed char,
                                      char16_t,
                                      char32_t>::value>> {
    template <typename Reader>
    static bool read(Reader& p, T& val, reader_options<T> opt)
    {
        SPIO_UNUSED(opt);
        return p.read_raw(val);
    }

    template <typename Writer>
    static void write(Writer& w, const T& val, writer_options<T> opt)
    {
        SPIO_UNUSED(opt);
        w.write_raw(val);
    }
};

template <typename T>
struct type<T,
            std::enable_if_t<contains<std::decay_t<typename T::element_type>,
                                      char,
                                      wchar_t,
                                      unsigned char,
                                      signed char,
                                      char16_t,
                                      char32_t>::value>> {
    template <typename Reader>
    static bool read(Reader& p, T& val, reader_options<T> opt)
    {
        using char_type = typename Reader::readable_type::value_type;
        span<char_type> s = make_span(reinterpret_cast<char_type*>(&val[0]),
                                      val.size_bytes() / sizeof(char_type));
        {
            auto it = s.begin();
            while (it != s.end()) {
                char_type c{};
                if (p.eof()) {
                    break;
                }
                if (!p.read(c)) {
                    if (!is_space(c, opt.spaces)) {
                        *it = c;
                    }
                    ++it;
                    break;
                }
                if (is_space(c, opt.spaces)) {
                    if (it == s.begin()) {
                        continue;
                    }
                    break;
                }
                *it = c;
                ++it;
            }
            if (it != s.end()) {
                *it = '\0';
            }
        }
        return !p.eof();
    }

    template <typename Writer>
    static void write(Writer& w, const T& val, writer_options<T> opt)
    {
        SPIO_UNUSED(opt);
        for (const auto& c : val) {
            w.write(c);
        }
    }
};

template <typename T>
struct type<T,
            std::enable_if_t<contains<std::decay_t<T>,
                                      short,
                                      int,
                                      long,
                                      long long,
                                      unsigned short,
                                      unsigned int,
                                      unsigned long,
                                      unsigned long long>::value>> {
    template <typename Reader>
    static bool read(Reader& p, T& val, reader_options<T> opt)
    {
        using char_type = typename Reader::readable_type::value_type;
        char_type c{};
        p.read(c);
        if (is_space(c)) {
            if (p.eof()) {
                return false;
            }
            return p.read(val);
        }

        T tmp = 0;
        const bool sign = [&]() {
            if constexpr (std::is_unsigned_v<T>) {
                if (c == '-') {
                    SPIO_THROW(
                        invalid_input,
                        "Cannot read a signed integer into an unsigned value");
                }
            }
            else {
                if (c == '-')
                    return false;
            }
            if (c == '+')
                return true;
            if (is_digit(c, opt.base)) {
                tmp = tmp * static_cast<T>(opt.base) -
                      char_to_int<T>(c, opt.base);
                return true;
            }
            SPIO_THROW(invalid_input, "Invalid first character in integer");
        }();
        while (!p.eof()) {
            p.read(c);
            if (is_digit(c, opt.base)) {
                tmp = tmp * static_cast<T>(opt.base) -
                      char_to_int<T>(c, opt.base);
            }
            else {
                break;
            }
        }
        if (sign)
            tmp = -tmp;
        val = tmp;
        return !p.eof();
    }

    template <typename Writer>
    static void write(Writer& w, const T& val, writer_options<T> opt)
    {
        using char_type = typename Writer::writable_type::value_type;
        array<char_type, max_digits<T>() + 1> buf{};
        buf.fill(char_type{0});
        auto s = make_span(buf);
        int_to_char<char_type>(val, s, opt.base);
        w.write(s);
    }
};

template <typename T>
struct type<T,
            std::enable_if_t<
                contains<std::decay_t<T>, float, double, long double>::value>> {
    template <typename Reader>
    static bool read(Reader& p, T& val, reader_options<T> opt)
    {
        using char_type = typename Reader::readable_type::value_type;
        array<char_type, 64> buf{};
        buf.fill(char_type{0});

        bool point = false;
        for (auto& c : buf) {
            if (p.eof()) {
                break;
            }
            p.read(c);
            if (c == char_type{'.'}) {
                if (point) {
                    p.push(c);
                    c = char_type{0};
                    break;
                }
                point = true;
                continue;
            }
            if (!is_digit(c)) {
                p.push(c);
                c = char_type{0};
                break;
            }
        }

        if (buf[0] == char_type{0}) {
            SPIO_THROW(invalid_input, "Failed to parse floating-point value");
        }

        char_type* end = &buf[0];
        T tmp = str_to_floating<T, char_type>(&buf[0], &end);
        if (end != find(buf.begin(), buf.end(), 0)) {
            SPIO_THROW(invalid_input, "Failed to parse floating-point value");
        }
        val = tmp;
        SPIO_UNUSED(opt);
        return !p.eof();
    }

    template <typename Writer>
    static void write(Writer& w, const T& val, writer_options<T> opt)
    {
        using char_type = typename Writer::writable_type::value_type;
        T i{};
        auto frac = std::modf(val, &i);
        w.write(static_cast<int>(i));
        w.write(static_cast<char_type>('.'));
        w.write(frac);
        SPIO_UNUSED(opt);
    }
};

template <>
struct type<bool> {
    template <typename Reader>
    static bool read(Reader& p, bool& val, reader_options<bool> opt)
    {
        uint_fast16_t n = 0;
        auto ret = p.read(n);
        if (n == 0) {
            val = false;
        }
        else {
            val = true;
        }
        SPIO_UNUSED(opt);
        return ret;
    }

    template <typename Writer>
    static void write(Writer& w, const bool& val, writer_options<bool> opt)
    {
        if (opt.alpha) {
            w.write(val ? "true" : "false");
        }
        else {
            w.write(val ? 1 : 0);
        }
    }
};
}  // namespace io

#endif  // SPIO_TYPE_H
