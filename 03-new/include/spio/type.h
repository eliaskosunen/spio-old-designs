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
struct type<
    T,
    std::enable_if_t<contains<std::remove_reference_t<std::remove_cv_t<T>>,
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
    static void write(Writer& w, T val, writer_options<T> opt)
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
        using char_type = typename Reader::char_type;
        span<char_type> s =
            make_span(reinterpret_cast<char_type*>(&val[0]),
                      val.size_bytes() / quantity_type{sizeof(char_type)});
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
    static void write(Writer& w, T val, writer_options<T> opt)
    {
        SPIO_UNUSED(opt);
        w.write_raw(val);
    }
};

template <typename T, std::size_t N>
struct type<detail::string_tag<T, N>> {
    using string_type = detail::string_tag<T, N>;

    template <typename Reader>
    static bool read(Reader& p,
                     typename string_type::type val,
                     reader_options<T> opt) = delete;

    template <typename Writer>
    static void write(Writer& w,
                      typename string_type::type val,
                      writer_options<T> opt)
    {
        SPIO_UNUSED(opt);
#if SPIO_HAS_IF_CONSTEXPR
        if constexpr (N != 0) {
            return w.write(make_span(&val, N));
        }
        else {
#else
        {
#endif
            auto ptr = string_type::make_pointer(val);
            const auto len = [&]() -> span_extent_type {
                if (N != 0) {
                    return N;
                }
                else if (sizeof(T) == 1) {
                    return static_cast<span_extent_type>(strlen(ptr));
                }
                else {
                    for (auto i = 0;; ++i) {
                        if (*(ptr + i) ==
                            typename string_type::char_type{'\0'}) {
                            return i;
                        }
                    }
                    return 0;
                }
            }();
            return w.write(make_span(ptr, len));
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
        using char_type = typename Reader::char_type;
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
#if SPIO_HAS_IF_CONSTEXPR
            if constexpr (std::is_unsigned_v<T>) {
#else
            if (std::is_unsigned<T>::value) {
#endif
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
    static void write(Writer& w, T val, writer_options<T> opt)
    {
        using char_type = typename Writer::char_type;
        array<char_type, max_digits<std::remove_reference_t<T>>() + 1> buf{};
        buf.fill(char_type{0});
        auto s = make_span(buf);
        int_to_char<char_type>(val, s, opt.base);
        /* buf[0] = '1'; */
        /* buf[1] = '2'; */
        /* buf[2] = '3'; */
        /* buf[3] = '\0'; */
        const auto len = [&]() {
            return distance(s.begin(), find(s.begin(), s.end(), '\0'));
            /* for (auto i = 0; i < s.length(); ++i) { */
            /*     if (s[i] == '\0') { */
            /*         return i; */
            /*     } */
            /* } */
            /* SPIO_THROW(assertion_failure, */
            /*            "Unreachable: (probably) a bug in int_to_char"); */
        }();
        w.write(s.first(len));
    }
};

namespace detail {
#if SPIO_HAS_IF_CONSTEXPR
    template <typename T>
    auto floating_write_arr(T val)
    {
        vector<char> arr([&]() {
            if constexpr (std::is_same_v<std::decay_t<T>, long double>) {
                return static_cast<std::size_t>(
                    std::snprintf(nullptr, 0, "%Lf", val));
            }
            else {
                return static_cast<std::size_t>(
                    std::snprintf(nullptr, 0, "%f", val));
            }
        }() + 1);
        if constexpr (std::is_same_v<std::decay_t<T>, long double>) {
            std::snprintf(&arr[0], arr.size(), "%Lf", val);
        }
        else {
            std::snprintf(&arr[0], arr.size(), "%f", val);
        }
        return arr;
    }
#else
    template <typename T>
    auto floating_write_arr(T val)
    {
        vector<char> arr(
            static_cast<std::size_t>(std::snprintf(nullptr, 0, "%f", val)) + 1);
        std::snprintf(&arr[0], arr.size(), "%f", val);
        return arr;
    }

    template <>
    inline auto floating_write_arr(long double val)
    {
        vector<char> arr(
            static_cast<std::size_t>(std::snprintf(nullptr, 0, "%Lf", val)) +
            1);
        std::snprintf(&arr[0], arr.size(), "%Lf", val);
        return arr;
    }
#endif
}

template <typename T>
struct type<T,
            std::enable_if_t<
                contains<std::decay_t<T>, float, double, long double>::value>> {
    template <typename Reader>
    static bool read(Reader& p, T& val, reader_options<T> opt)
    {
        using char_type = typename Reader::char_type;
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
    static void write(Writer& w, T val, writer_options<T> opt)
    {
        using char_type = typename Writer::char_type;

        auto arr = detail::floating_write_arr(val);
        auto char_span =
            make_span(&arr[0], static_cast<span_extent_type>(strlen(&arr[0])));

        if (sizeof(char_type) == 1) {
            w.write(char_span);
        }
        else {
            vector<char_type> buf{};
            buf.reserve(char_span.size_us());
            for (auto& c : char_span) {
                buf.push_back(static_cast<char_type>(c));
            }
            w.write(make_span(buf));
        }

        SPIO_UNUSED(opt);
    }
};

template <>
struct type<bool> {
    template <typename Reader>
    static bool read(Reader& p, bool& val, reader_options<bool> opt)
    {
        if (opt.alpha) {
            using char_type = typename Reader::char_type;
            array<char_type, 5> buf{};
            auto ret = p.read(make_span(buf));
            if (buf[0] == 't' && buf[1] == 'r' && buf[2] == 'u' &&
                buf[3] == 'e') {
                val = true;
                p.push(buf[4]);
                return ret;
            }
            if (buf[0] == 'f' && buf[1] == 'a' && buf[2] == 'l' &&
                buf[3] == 's' && buf[4] == 'e') {
                val = false;
                return ret;
            }
            SPIO_THROW(invalid_input, "Failed to parse boolean value");
        }
        else {
            uint_fast16_t n = 0;
            auto ret = p.read(n);
            if (n == 0) {
                val = false;
            }
            else {
                val = true;
            }
            return ret;
        }
    }

    template <typename Writer>
    static void write(Writer& w, bool val, writer_options<bool> opt)
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
