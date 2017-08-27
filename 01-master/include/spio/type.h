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

#include "config.h"
#include "util.h"

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
    template <typename InputParser>
    static void read(InputParser& p, T& val)
    {
        p.read_raw(io::make_span(&val, 1));
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
    template <typename InputParser>
    static void read(InputParser& p, T& val)
    {
        using char_type = typename InputParser::readable_type::value_type;
        char_type c{};
        p.read(c);
        auto is_digit = [](char_type ch) {
            return ch >= static_cast<char_type>('0') &&
                   ch <= static_cast<char_type>('9');
        };
        auto to_number = [](char_type ch) {
            return static_cast<T>(ch - char_type{'0'});
        };
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
            if (is_digit(c)) {
                tmp = tmp * 10 - to_number(c);
                return true;
            }
            SPIO_THROW(invalid_input, "Invalid first character in integer");
        }();
        while (!p.eof()) {
            p.read(c);
            if (is_digit(c)) {
                tmp = tmp * 10 - to_number(c);
            }
            else {
                break;
            }
        }
        if (sign)
            tmp = -tmp;
        val = tmp;
    }
};

template <typename T>
struct type<T,
            std::enable_if_t<
                contains<std::decay_t<T>, float, double, long double>::value>> {
    template <typename InputParser>
    static void read(InputParser& p, T& val)
    {
        using char_type = typename InputParser::readable_type::value_type;
        array<char_type, 64> buf{};
        buf.fill(char_type{0});

        auto is_digit = [](char_type ch) {
            return ch >= static_cast<char_type>('0') &&
                   ch <= static_cast<char_type>('9');
        };
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
        return;
    }
};

template <>
struct type<bool> {
    template <typename InputParser>
    static void read(InputParser& p, bool& val)
    {
        uint_fast16_t n = 0;
        p.read(n);
        if (n == 0) {
            val = false;
        }
        else {
            val = true;
        }
    }
};
}  // namespace io

#endif  // SPIO_TYPE_H
