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

#ifndef SPIO_SCANNER_H
#define SPIO_SCANNER_H

#include "fwd.h"

#include <cstdio>
#include <cwchar>
#include "codeconv.h"
#include "span.h"
#include "stream_iterator.h"

namespace spio {
template <typename CharT>
struct scan_options {
    using char_type = CharT;
    bool readall;
    const std::locale* locale{std::addressof(global_locale())};

    bool is_space(char_type ch) const
    {
        return std::isspace(ch, *locale);
    }
};

template <typename CharT>
struct basic_arg {
    using char_type = CharT;
    void* value;

    using stream_type = basic_stream_ref<char_type, input>;
    using scanner_fn_type = stream_type& (*)(stream_type&,
                                             const CharT*&,
                                             const scan_options<char_type>&,
                                             void*);
    scanner_fn_type scan;
};

template <typename CharT,
          typename Arg = basic_arg<CharT>,
          typename Allocator = std::allocator<Arg>>
class basic_arg_list {
public:
    using char_type = CharT;
    using arg_type = Arg;
    using allocator_type = Allocator;

    // using storage_type = small_vector<arg_type, allocator_type, 16>;
    using storage_type = std::vector<arg_type, allocator_type>;

    basic_arg_list(storage_type v) : m_vec(std::move(v)) {}
    basic_arg_list(std::initializer_list<arg_type> i,
                   const Allocator& a = Allocator())
        : m_vec(i, a)
    {
    }

    basic_arg_list& operator[](std::size_t i)
    {
        return m_vec[i];
    }
    const basic_arg_list& operator[](std::size_t i) const
    {
        return m_vec[i];
    }

    storage_type& get()
    {
        return m_vec;
    }
    const storage_type& get() const
    {
        return m_vec;
    }

private:
    storage_type m_vec;
};

template <typename CharT, typename T, typename Enable = void>
void custom_scan(basic_stream_ref<CharT, input>&, const CharT*&, T&);

template <typename CharT>
inline void skip_format(const CharT*& f)
{
    if (*f == 0) {
        return;
    }
    if (*f != CharT('}')) {
        throw failure(std::make_error_code(std::errc::invalid_argument),
                      "Invalid format string: expected '}'");
    }
    ++f;
}

namespace detail {
    template <typename CharT>
    using scan_stream = basic_stream_ref<CharT, input>;
    template <typename CharT>
    using scan_stream_traits = typename scan_stream<CharT>::traits;

    template <typename CharT, typename T, typename = void>
    struct builtin_scan {
        static scan_stream<CharT>& scan(scan_stream<CharT>& s,
                                        const CharT*& format,
                                        const scan_options<CharT>& opt,
                                        void* data)
        {
            T& val = *reinterpret_cast<T*>(data);
            SPIO_UNUSED(opt);

            custom_scan(s, format, val);
            return s;
        }
    };

    template <typename CharT, typename T>
    struct builtin_scan<
        CharT,
        T,
        std::enable_if_t<std::is_same<std::decay_t<T>, CharT>::value>> {
        static scan_stream<CharT>& scan(scan_stream<CharT>& s,
                                        const CharT*& format,
                                        const scan_options<CharT>& opt,
                                        void* data)
        {
            T& ch = *reinterpret_cast<CharT*>(data);
            SPIO_UNUSED(opt);
            ch = s.get();

            skip_format(format);
            return s;
        }
    };

    template <typename CharT, typename T>
    struct builtin_scan<
        CharT,
        span<T>,
        std::enable_if_t<std::is_same<std::decay_t<T>, CharT>::value>> {
        static scan_stream<CharT>& scan(scan_stream<CharT>& s,
                                        const CharT*& format,
                                        const scan_options<CharT>& opt,
                                        void* data)
        {
            span<T> val = *reinterpret_cast<span<T>*>(data);
            if (val.size() == 0) {
                return s;
            }

            const bool read_till_ws = *format == CharT('w');
            if (read_till_ws) {
                ++format;
            }

            if (opt.readall && !read_till_ws) {
                small_vector<CharT> str(val.size_us());
                auto strspan = make_span(str);
                if (!s.read(strspan)) {
                    return s;
                }
                /* std::copy(strspan.begin(), strspan.end(), val.begin()); */
                scan_stream_traits<CharT>::copy(val.data(), strspan.data(),
                                                strspan.size_us());
            }
            else {
                small_vector<CharT> str(val.size_us());
                s.readword(make_span(str));
                if (s) {
                    /* std::copy(str.begin(), str.end(), val.begin()); */
                    scan_stream_traits<CharT>::copy(val.data(), str.data(),
                                                    str.size());
                }
            }

            skip_format(format);
            return s;
        }
    };

    template <typename CharT, typename T>
    struct builtin_scan<
        CharT,
        T,
        std::enable_if_t<std::is_integral<T>::value &&
                         !std::is_same<CharT, std::decay_t<T>>::value &&
                         !std::is_same<std::decay_t<T>, bool>::value>> {
        static scan_stream<CharT>& scan(scan_stream<CharT>& s,
                                        const CharT*& format,
                                        const scan_options<CharT>& opt,
                                        void* data)
        {
            T& val = *reinterpret_cast<T*>(data);

            const auto base = [&format]() {
                if (*format == CharT('}') || *format == CharT('d')) {
                    return 10;
                }
                else if (*format == CharT('x')) {
                    return 16;
                }
                else if (*format == CharT('o')) {
                    return 8;
                }
                else if (*format == CharT('b')) {
                    return 2;
                }
                throw failure(std::make_error_code(std::errc::invalid_argument),
                              "Invalid format string: `int`-like types only "
                              "support bases 'd,x,o,b'");
            }();
            if (*format != CharT('}')) {
                ++format;
            }

            constexpr auto n = max_digits<std::remove_reference_t<T>>() + 1;
            std::array<CharT, n> buf{};
            buf.fill(0);
            auto bufspan = make_span(buf);
            if (!s.readword(bufspan)) {
                return s;
            }

            T tmp = 0;
            auto it = buf.begin();

            {
                const bool sign = [&]() {
#if SPIO_HAS_IF_CONSTEXPR && SPIO_HAS_TYPE_TRAITS_V
                    if constexpr (std::is_unsigned_v<T>) {
#else
                    if (std::is_unsigned<T>::value) {
#endif
                        if (scan_stream_traits<CharT>::eq(*it, '-')) {
                            throw failure(
                                invalid_input,
                                "Cannot read a signed integer into an "
                                "unsigned value");
                        }
                    }
                    else {
                        if (scan_stream_traits<CharT>::eq(*it, '-')) {
                            return false;
                        }
                    }
                    if (scan_stream_traits<CharT>::eq(*it, '+')) {
                        return true;
                    }
                    if (is_digit(*it, base)) {
                        tmp = tmp * static_cast<T>(base) -
                              char_to_int<T>(*it, base);
                        return true;
                    }
                    std::array<char, 128> errbuf{};
                    errbuf.fill('\0');
                    std::snprintf(&errbuf[0], 128,
                                  "Invalid first character in integer: 0x%x",
                                  static_cast<int>(*it));
                    throw failure(invalid_input, &errbuf[0]);
                }();
                ++it;

                for (; it != buf.end(); ++it) {
                    if (is_digit(*it, base)) {
                        tmp = tmp * static_cast<T>(base) -
                              char_to_int<T>(*it, base);
                    }
                    else {
                        break;
                    }
                }
                if (sign) {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4146)
#endif
                    tmp = -tmp;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
                }
            }

            val = tmp;
            if (opt.readall) {
                if (it != buf.end()) {
                    const auto end = [&]() {
                        for (auto i = it; i != buf.end(); ++i) {
                            if (scan_stream_traits<CharT>::eq(*i, 0)) {
                                return i;
                            }
                        }
                        return buf.end();
                    }();
                    s.putback(make_span(it, end));
                }
            }

            ++format;
            return s;
        }
    };

    template <typename CharT, typename T>
    struct builtin_scan<CharT,
                        T,
                        std::enable_if_t<std::is_floating_point<T>::value>> {
        static scan_stream<CharT>& scan(scan_stream<CharT>& s,
                                        const CharT*& format,
                                        const scan_options<CharT>& opt,
                                        void* data)
        {
            T& val = *reinterpret_cast<T*>(data);
            SPIO_UNUSED(opt);

            std::array<CharT, 64> buf{};
            buf.fill(CharT(0));

            bool point = false;
            for (auto& c : buf) {
                if (s.eof()) {
                    break;
                }
                s.read(make_span(&c, 1));
                if (c == CharT('.')) {
                    if (point) {
                        s.putback(c);
                        c = CharT(0);
                        break;
                    }
                    point = true;
                    continue;
                }
                if (!is_digit(c)) {
                    s.putback(c);
                    c = CharT(0);
                    break;
                }
            }

            if (buf[0] == CharT(0)) {
                throw failure(invalid_input,
                              "Failed to parse floating-point value");
            }

            CharT* end = buf.data();
            T tmp = str_to_floating<T, CharT>(buf.data(), &end);
            if (&*std::find(buf.begin(), buf.end(), 0) != end) {
                throw failure(invalid_input,
                              "Failed to parse floating-point value");
            }
            val = tmp;

            skip_format(format);
            return s;
        }
    };

    template <typename CharT>
    struct builtin_scan<CharT, bool> {
        static scan_stream<CharT>& scan(scan_stream<CharT>& s,
                                        const CharT*& format,
                                        const scan_options<CharT>& opt,
                                        void* data)
        {
            bool& val = *reinterpret_cast<bool*>(data);

            auto ch = s.get();
            s.putback(ch);
            if (is_digit(ch)) {
                int_fast16_t n;
                auto fmt = "}";
                builtin_scan<CharT, int_fast16_t>::scan(s, fmt, opt,
                                                        std::addressof(n));
                if (!s) {
                    return s;
                }
                val = static_cast<bool>(n);
            }
            else {
                std::array<CharT, 6> buf;
                buf.fill(0);

                if (!s.readword(make_span(buf))) {
                    return s;
                }
                if (buf[0] == CharT('t') && buf[1] == CharT('r') &&
                    buf[2] == CharT('u') && buf[3] == CharT('e')) {
                    val = true;
                }
                else if (buf[0] == CharT('f') && buf[1] == CharT('a') &&
                         buf[2] == CharT('l') && buf[3] == CharT('s') &&
                         buf[4] == CharT('e')) {
                    val = false;
                }
                else {
                    throw failure(invalid_input, "Invalid bool value");
                }
            }

            skip_format(format);
            return s;
        }
    };
}  // namespace detail

template <typename CharT, typename Allocator, typename Traits>
void custom_scan(basic_stream_ref<CharT, input>& s,
                 const CharT*& format,
                 std::basic_string<CharT, Allocator, Traits>& str)
{
    if (str.empty()) {
        str.resize(15);
    }

    auto it = str.begin();
    auto opt = scan_options<CharT>{
        true, std::addressof(s.get_stream().get_scanner().get_locale())};
    while (true) {
        s.readword(make_span(it, str.end()));
        if (!s || s.eof()) {
            break;
        }
        auto ch = s.get();
        s.putback(ch);
        if (opt.is_space(ch)) {
            break;
        }
        auto n = static_cast<
            typename std::basic_string<CharT, Allocator, Traits>::difference_type>(
            str.size());
        str.resize(str.size() * 2);
        it = str.begin() + n;
    }
    str.erase(std::find(str.begin(), str.end(), 0), str.end());
    str.shrink_to_fit();
    ++format;
}

template <typename T, typename... Args>
auto make_args(Args&... a)
{
    typename T::storage_type vec{
        typename T::arg_type{basic_arg<typename T::char_type>{
            reinterpret_cast<void*>(std::addressof(a)),
            &detail::builtin_scan<typename T::char_type,
                                  std::decay_t<Args>>::scan}}...};
    return T(std::move(vec));
}

template <typename CharT>
class basic_builtin_scanner {
public:
    using char_type = CharT;
    using arg_list = basic_arg_list<char_type>;
    using stream_type = basic_stream_ref<char_type, input>;

    basic_builtin_scanner() = default;

    void imbue(const std::locale& l)
    {
        m_locale = std::addressof(l);
    }
    const std::locale& get_locale() const
    {
        return *m_locale;
    }

    template <typename... Args>
    void operator()(stream_type& s,
                    const char_type* format,
                    bool readall,
                    Args&... args)
    {
        vscan(s, format, readall, make_args<arg_list>(args...));
    }

    void vscan(stream_type& s,
               const char_type* format,
               bool readall,
               arg_list args);

    template <typename Stream>
    void skip_ws(Stream& s)
    {
        auto opt = make_scan_options(true);
        auto ch = s.get();
        while (s && opt.is_space(ch)) {
            ch = s.get();
        }
        s.putback(ch);
    }
    void skip_ws(stream_type& s)
    {
        auto opt = make_scan_options(true);
        auto ch = s.get();
        while (s && opt.is_space(ch)) {
            ch = s.get();
        }
        s.putback(ch);
    }

    const std::locale* m_locale{std::addressof(global_locale())};
    scan_options<char_type> make_scan_options(bool readall) const
    {
        return {readall, m_locale};
    }
};
}  // namespace spio

#include "scanner.impl.h"

#endif
