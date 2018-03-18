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
#if SPIO_USE_LOCALE
    const std::locale* locale{std::addressof(global_locale())};

    bool is_space(char_type ch) const
    {
        return std::isspace(ch, *locale);
    }
#else
    bool is_space(char_type ch)
    {
        return ch == 32 || ch == 10 || ch == 9 || ch == 13 || ch == 11;
    }
#endif
};

template <typename CharT>
struct basic_arg {
    using char_type = CharT;
    void* value;

    using iterator_type = instream_iterator<char_type, char_type>;
    using scanner_fn_type =
        iterator_type (*)(iterator_type,
                          typename span<const char_type>::iterator&,
                          const scan_options<char_type>&,
                          void*);
    scanner_fn_type scan;
};

template <typename CharT, typename... Types>
class basic_arg_list {
public:
    using arg_type = basic_arg<CharT>;
    using char_type = CharT;
    using storage_type = std::vector<arg_type>;

    basic_arg_list(storage_type v) : m_vec(std::move(v)) {}
    template <typename... Args>
    basic_arg_list(Args&... a)
        : m_vec{std::initializer_list<basic_arg<Args...>>{a...}}
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

namespace detail {
    template <typename CharT>
    using scan_iterator = instream_iterator<CharT, CharT>;
    template <typename CharT, typename T, typename = void>
    struct builtin_scan;

    template <typename CharT, typename T>
    struct builtin_scan<
        CharT,
        T,
        std::enable_if_t<std::is_same<std::decay_t<T>, CharT>::value>> {
        static scan_iterator<CharT> scan(
            scan_iterator<CharT> it,
            typename span<const CharT>::iterator& format,
            const scan_options<CharT>& opt,
            void* data)
        {
            T& ch = *reinterpret_cast<CharT*>(data);
            SPIO_UNUSED(opt);
            it.read_into(make_span(&ch, 1));

            if (*format != CharT('}')) {
                throw failure(
                    std::make_error_code(std::errc::invalid_argument),
                    "Invalid format string: `char_type` doesn't support "
                    "format specifiers, expected '}'");
            }
            ++format;
            return it;
        }
    };

    template <typename CharT, typename T>
    struct builtin_scan<
        CharT,
        span<T>,
        std::enable_if_t<std::is_same<std::decay_t<T>, CharT>::value>> {
        static scan_iterator<CharT> scan(
            scan_iterator<CharT> it,
            typename span<const CharT>::iterator& format,
            const scan_options<CharT>& opt,
            void* data)
        {
            span<T> val = *reinterpret_cast<span<T>*>(data);
            if (opt.readall) {
                std::vector<CharT> str(val.size_us());
                auto strspan = make_span(str);
                it.read_into(strspan);
                const auto len =
                    static_cast<std::ptrdiff_t>(std::strlen(str.data()));
                const auto end = [&]() {
                    for (std::ptrdiff_t i = 0; i < len; ++i) {
                        if (opt.is_space(strspan[i])) {
                            return i;
                        }
                    }
                    return strspan.size();
                }();
                std::copy(strspan.begin(), strspan.begin() + end, val.begin());
                if (end + 1 < len) {
                    it.get_stream().get_source_buffer().push(make_span(
                        strspan.begin() + end + 1, strspan.begin() + len));
                }
            }
            else {
                for (auto val_it = val.begin();
                     val_it != val.end() && it != scan_iterator<CharT>{};
                     ++val_it, ++it) {
                    if (opt.is_space(*it)) {
                        it.get_stream().get_source_buffer().push(
                            make_span(&*it, 1));
                        break;
                    }
                    *val_it = *it;
                }
            }

            if (*format != CharT('}')) {
                throw failure(
                    std::make_error_code(std::errc::invalid_argument),
                    "Invalid format string: `span<char_type>` doesn't support "
                    "format specifiers, expected '}'");
            }
            ++format;
            return it;
        }
    };
}  // namespace detail

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
    using iterator = instream_iterator<char_type, char_type>;
    using arg_list = basic_arg_list<char_type, char_type, span<char_type>>;

    basic_builtin_scanner() = default;
#if SPIO_USE_LOCALE
    basic_builtin_scanner(const std::locale& l) : m_locale(std::addressof(l)) {}

    void imbue(const std::locale& l)
    {
        m_locale = std::addressof(l);
    }
    const std::locale& get_locale() const
    {
        return *m_locale;
    }
#endif

    template <typename... Args>
    iterator operator()(iterator it,
                        const char_type* format,
                        bool readall,
                        Args&... args)
    {
        return vscan(it, make_span(format, std::strlen(format)), readall,
                     make_args<arg_list>(args...));
    }

    iterator vscan(iterator it,
                   span<const char> format,
                   bool readall,
                   arg_list args);

private:
#if SPIO_USE_LOCALE
    const std::locale* m_locale{std::addressof(global_locale())};
    scan_options<char_type> make_scan_options(bool readall) const
    {
        return {readall, m_locale};
    }
#else
    scan_options<char_type> make_scan_options(bool readall) const
    {
        return {readall};
    }
#endif

    void skip_ws(iterator& it)
    {
        auto opt = make_scan_options(true);
        if (*it == char_type(0)) {
            ++it;
            if (!opt.is_space(*it)) {
                it.get_stream().get_source_buffer().push(make_span(&*it, 1));
                return;
            }
        }
        for (; *it == char_type(0) || opt.is_space(*it); ++it) {
        }
    }
};
}  // namespace spio

#include "scanner.impl.h"

#endif
