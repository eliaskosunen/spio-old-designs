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

#ifndef SPIO_TYPE_H
#define SPIO_TYPE_H

#include <cmath>
#include "config.h"
#include "custom_type.h"
#include "reader_options.h"
#include "stl.h"
#include "util.h"
#include "writer_options.h"

namespace io {
#define CHECK_READER(fn)                    \
    static_assert(is_reader<Reader>::value, \
                  fn ": T must satisfy the requirements of Reader")
#define CHECK_WRITER(fn)                    \
    static_assert(is_writer<Writer>::value, \
                  fn ": T must satisfy the requirements of Writer")

template <typename T, typename Enable = void>
struct type {
    template <typename Reader>
    static bool read(Reader& p, T& val, reader_options<T> opt)
    {
        CHECK_READER("type<>::read<T>");
        return custom_read<T>::read(p, val, opt);
    }

    template <typename Writer>
    static bool write(Writer& w, const T& val, writer_options<T> opt)
    {
        CHECK_WRITER("type<>::write<T>");
        return custom_write<T>::write(w, val, opt);
    }
};

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
        CHECK_READER("type<Char>::read<T>");
        SPIO_UNUSED(opt);
        return p.read_raw(val);
    }

    template <typename Writer>
    static bool write(Writer& w, const T& val, writer_options<T> opt)
    {
        CHECK_WRITER("type<Char>::write<T>");
        SPIO_UNUSED(opt);
        return w.write_raw(val);
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
        CHECK_READER("type<span<Char>>::read<T>");
        using char_type = typename Reader::char_type;
        span<char_type> s =
            make_span(reinterpret_cast<char_type*>(&val[0]),
                      val.size_bytes() / quantity_type{sizeof(char_type)});

        char_type ch{};
        while (p.get(ch)) {
            if (!is_space(ch, opt.spaces)) {
                p.push(ch);
                break;
            }
        }
        if (p.eof()) {
            return false;
        }

        if (opt.readall) {
            stl::vector<char_type> str(s.size_us(), '\0');
            auto strspan = make_span(str);
            p.read_raw(strspan);
            const auto str_len = stl::strlen(strspan);
            const auto end = [&]() {
                for (std::ptrdiff_t i = 0; i < str_len; ++i) {
                    if (is_space(strspan[i], opt.spaces)) {
                        return i;
                    }
                }
                return strspan.size();
            }();
            auto bytespan =
                as_bytes(make_span(strspan.begin(), strspan.begin() + end));
            copy_contiguous(bytespan, as_writable_bytes(s));
            if (end + 1 < str_len) {
#if SPIO_HAS_IF_CONSTEXPR
            /* if constexpr (Reader::readable_type::is_trivially_rewindable) {
             */
            /*     p.get_readable().rewind( */
            /*         stl::distance(strspan.begin() + end + 1, strspan.end()));
             */
            /* } */
            /* else */
#endif
                {
                    p.push(make_span(strspan.begin() + end + 1,
                                     strspan.begin() + str_len));
                }
            }
        }
        else {
            for (auto it = val.begin(); it != val.end() && p.read(ch); ++it) {
                if (is_space(ch, opt.spaces)) {
                    p.push(ch);
                    break;
                }
                *it = ch;
            }
        }
        return !p.eof();
    }

    template <typename Writer>
    static bool write(Writer& w, const T& val, writer_options<T> opt)
    {
        CHECK_WRITER("type<span<Char>>::write<T>");
        SPIO_UNUSED(opt);
        return w.write_raw(val);
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
    static bool write(Writer& w,
                      typename string_type::type val,
                      writer_options<T> opt)
    {
        CHECK_WRITER("type<String>::write<T>");
        SPIO_UNUSED(opt);
        auto ptr = string_type::make_pointer(val);
#if SPIO_HAS_IF_CONSTEXPR
        if constexpr (N != 0) {
            return w.write(make_span<N - 1>(ptr));
        }
        else
#endif
        {
            const auto len = [&]() -> span_extent_type {
                if (N != 0) {
                    return static_cast<span_extent_type>(
                        N - 1);  // Don't write the null terminator
                }
                return stl::strlen(ptr);
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
        CHECK_READER("type<Int>::read<T>");
        using char_type = typename Reader::char_type;

        char_type ch{};
        while (p.get(ch)) {
            if (!is_space(ch)) {
                p.push(ch);
                break;
            }
        }
        if (p.eof()) {
            return false;
        }

        constexpr auto n = max_digits<std::remove_reference_t<T>>() + 1;
        stl::array<char_type, n> buf{};
        buf.fill(0);
        if (p.is_overreadable()) {
            p.read(make_span<n>(buf));
        }
        else {
            for (auto& c : buf) {
                p.get(c);
                if (is_space(c)) {
                    p.push(c);
                    c = '\0';
                    break;
                }
                if (p.eof()) {
                    break;
                }
            }
        }

        T tmp = 0;
        auto it = buf.begin();
        const bool sign = [&]() {
#if SPIO_HAS_IF_CONSTEXPR && SPIO_HAS_TYPE_TRAITS_V
            if constexpr (std::is_unsigned_v<T>) {
#else
            if (std::is_unsigned<T>::value) {
#endif
                if (*it == '-') {
                    SPIO_THROW(
                        invalid_input,
                        "Cannot read a signed integer into an unsigned value");
                }
            }
            else {
                if (*it == '-') {
                    return false;
                }
            }
            if (*it == '+') {
                return true;
            }
            if (is_digit(*it, opt.base)) {
                tmp = tmp * static_cast<T>(opt.base) -
                      char_to_int<T>(*it, opt.base);
                return true;
            }
            stl::vector<char> errbuf(128, '\0');
            std::snprintf(&errbuf[0], 128,
                          "Invalid first character in integer: 0x%x",
                          static_cast<int>(*it));
            SPIO_THROW(invalid_input, &errbuf[0]);
        }();
        ++it;

        for (; it != buf.end(); ++it) {
            if (is_digit(*it, opt.base)) {
                tmp = tmp * static_cast<T>(opt.base) -
                      char_to_int<T>(*it, opt.base);
            }
            else {
                break;
            }
        }
        if (sign) {
            tmp = -tmp;
        }
        val = tmp;
        /* #if SPIO_HAS_IF_CONSTEXPR */
        /*         if constexpr (Reader::readable_type::is_trivially_rewindable)
         * { */
        /*             p.get_readable().rewind(stl::distance(it, buf.end())); */
        /*         } */
        /*         else */
        /* #endif */
        if (p.is_overreadable()) {
            if (it != buf.end()) {
                const auto end = [&]() {
                    for (auto i = it; i != buf.end(); ++i) {
                        if (*i == '\0') {
                            return i;
                        }
                    }
                    return buf.end();
                }();
                p.push(make_span(it, end));
            }
        }
        return !p.eof();
    }

    template <typename Writer>
    static bool write(Writer& w, const T& val, writer_options<T> opt)
    {
        CHECK_WRITER("type<Int>::write<T>");
        using char_type = typename Writer::char_type;

        if (opt.base == 10) {
            constexpr auto n = max_digits<std::remove_reference_t<T>>() + 1;
            stl::array<char_type, n> buf{};
            buf.fill(char_type{0});
            auto s = make_span<n>(buf);
            int_to_char<char_type>(val, s, 10);
            return w.write(s.first(stl::strlen(s)).as_const_span());
        }

        auto n = sizeof(std::remove_reference_t<T>) * 8 + 1;
        stl::vector<char_type> buf(n, char_type{0});
        auto s = make_span(buf);
        int_to_char<char_type>(val, s, opt.base);
        return w.write(s.first(stl::strlen(s)).as_const_span());
    }
};  // namespace io

namespace detail {
#if SPIO_HAS_IF_CONSTEXPR
    template <typename T>
    auto floating_write_arr(T val)
    {
        stl::vector<char> arr([&]() {
            if constexpr (std::is_same<std::decay_t<T>, long double>::value) {
                return static_cast<std::size_t>(
                    std::snprintf(nullptr, 0, "%Lg", val));
            }
            else {
                return static_cast<std::size_t>(
                    std::snprintf(nullptr, 0, "%g", static_cast<double>(val)));
            }

        }() + 1);
#if SPIO_HAS_TYPE_TRAITS_V
        if constexpr (std::is_same_v<std::decay_t<T>, long double>) {
#else
        if constexpr (std::is_same<std::decay_t<T>, long double>::value) {
#endif
            std::snprintf(&arr[0], arr.size(), "%Lg", val);
        }
        else {
            std::snprintf(&arr[0], arr.size(), "%g", static_cast<double>(val));
        }
        auto size = arr.size();
        bool period_found = false;
        for (auto i = arr.size() - 1; i > 0; --i) {
            if (arr[i] == '.') {
                period_found = true;
                break;
            }
            if (arr[i] == '0') {
                --size;
            }
        }
        if (!period_found) {
            return arr;
        }
        return stl::vector<char>(
            arr.begin(),
            arr.begin() +
                static_cast<typename decltype(arr)::difference_type>(size));
    }
#else
    template <typename T>
    auto floating_write_arr(T val)
    {
        stl::vector<char> arr(
            static_cast<std::size_t>(std::snprintf(nullptr, 0, "%f", val)) + 1);
        std::snprintf(&arr[0], arr.size(), "%f", val);
        return arr;
    }

    template <>
    inline auto floating_write_arr(long double val)
    {
        stl::vector<char> arr(
            static_cast<std::size_t>(std::snprintf(nullptr, 0, "%Lf", val)) +
            1);
        std::snprintf(&arr[0], arr.size(), "%Lf", val);
        return arr;
    }
#endif
}  // namespace detail

template <typename T>
struct type<T,
            std::enable_if_t<
                contains<std::decay_t<T>, float, double, long double>::value>> {
    template <typename Reader>
    static bool read(Reader& p, T& val, reader_options<T> opt)
    {
        CHECK_READER("type<Float>::read<T>");
        using char_type = typename Reader::char_type;

        char_type ch{};
        while (p.get(ch)) {
            if (!is_space(ch)) {
                p.push(ch);
                break;
            }
        }
        if (p.eof()) {
            return false;
        }

        stl::array<char_type, 64> buf{};
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
        if (&*stl::find(buf.begin(), buf.end(), 0) != end) {
            SPIO_THROW(invalid_input, "Failed to parse floating-point value");
        }
        val = tmp;
        SPIO_UNUSED(opt);
        return !p.eof();
    }

    template <typename Writer>
    static bool write(Writer& w, const T& val, writer_options<T> opt)
    {
        CHECK_WRITER("type<Float>::write<T>");
        using char_type = typename Writer::char_type;

        auto arr = detail::floating_write_arr(val);
        auto char_span = make_span(&arr[0], stl::strlen(&arr[0]));

        SPIO_UNUSED(opt);

#if SPIO_HAS_IF_CONSTEXPR
        if constexpr (sizeof(char_type) == 1)
#else
        if (sizeof(char_type) == 1)
#endif
        {
            return w.write(char_span);
        }
        else {
            stl::vector<char_type> buf{};
            buf.reserve(char_span.size_us());
            for (auto& c : char_span) {
                buf.push_back(static_cast<char_type>(c));
            }
            return w.write(make_span(buf));
        }
    }
};

template <>
struct type<void*> {
    template <typename Reader>
    static bool read(Reader& p, void*& val, reader_options<void*> opt) = delete;

    template <typename Writer>
    static bool write(Writer& w, const void* val, writer_options<void*> opt)
    {
        CHECK_WRITER("type<void*>::write<T>");
        SPIO_UNUSED(opt);
        using char_type = typename Writer::char_type;

        if (!w.put(char_type{'0'}) || !w.put(char_type{'x'})) {
            return false;
        }

        writer_options<std::uintptr_t> o;
        o.base = 16;
        return w.write(reinterpret_cast<std::uintptr_t>(val), o);
    }
};

template <>
struct type<bool> {
    template <typename Reader>
    static bool read(Reader& p, bool& val, reader_options<bool> opt)
    {
        CHECK_READER("type<bool>::read<T>");
        if (opt.alpha) {
            using char_type = typename Reader::char_type;
            stl::array<char_type, 5> buf{};
            auto ret = p.read(make_span<5>(buf));
            if (buf[0] == 't' && buf[1] == 'r' && buf[2] == 'u' &&
                buf[3] == 'e') {
                val = true;
#if SPIO_HAS_IF_CONSTEXPR
                /* if constexpr (Reader::readable_type::is_trivially_rewindable)
                 * { */
                /*     p.get_readable().rewind(1); */
                /* } */
                /* else */
#endif
                {
                    p.push(buf[4]);
                }
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
    static bool write(Writer& w, const bool& val, writer_options<bool> opt)
    {
        CHECK_WRITER("type<bool>::write<T>");
        if (opt.alpha) {
            if (val) {
                return w.write("true");
            }
            return w.write("false");
        }
        return w.write(val ? 1 : 0);
    }
};
}  // namespace io

#undef CHECK_WRITER
#undef CHECK_READER

#endif  // SPIO_TYPE_H
