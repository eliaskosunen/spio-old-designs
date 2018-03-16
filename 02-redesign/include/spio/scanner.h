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
class basic_builtin_scanner {
public:
    using char_type = CharT;
    using iterator = instream_iterator<char_type, char_type>;

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
        return do_scan(it, format, readall, args...);
    }

private:
#if SPIO_USE_LOCALE
    const std::locale* m_locale{std::addressof(global_locale())};

    bool is_space(char_type ch) const
    {
        return std::isspace(ch, get_locale());
    }
#else
    bool is_space(char_type ch)
    {
        return ch == 32 || ch == 10 || ch == 9 || ch == 13 || ch == 11;
    }
#endif

    template <typename T, typename... Args>
    iterator do_scan(iterator it,
                     const char_type* format,
                     bool readall,
                     T& arg,
                     Args&... args);
    iterator do_scan(iterator it, const char_type* format, bool readall);

    template <typename T>
    auto scan(iterator it, const char_type*& format, T& val, bool readall)
        -> std::enable_if_t<
            std::is_same<std::remove_reference_t<std::remove_cv_t<T>>,
                         char_type>::value,
            iterator>
    {
        SPIO_UNUSED(readall);
        it.read_into(make_span(&val, 1));
        if (*format != char_type('}')) {
            throw failure(std::make_error_code(std::errc::invalid_argument),
                          "Invalid format string: `char_type` doesn't support "
                          "format specifiers, expected '}'");
        }
        ++format;
        return it;
    }

    template <typename T>
    auto scan(iterator it, const char_type*& format, span<T> val, bool readall)
        -> std::enable_if_t<
            std::is_same<std::remove_reference_t<std::remove_cv_t<T>>,
                         char_type>::value,
            iterator>
    {
        if (readall) {
            std::vector<char_type> str(val.size_us());
            auto strspan = make_span(str);
            it.read_into(strspan);
            const auto len = std::strlen(str.data());
            const auto end = [&]() {
                for (std::ptrdiff_t i = 0; i < len; ++i) {
                    if (is_space(strspan[i])) {
                        return i;
                    }
                }
                return strspan.size();
            }();
            std::copy(strspan.begin(), strspan.begin() + end, val.begin());
            if (end + 1 < len) {
                it.get_stream().push(make_span(strspan.begin() + end + 1,
                                               strspan.begin() + len));
            }
        }
        else {
            for (auto val_it = val.begin();
                 val_it != val.end() && it != iterator{}; ++val_it, ++it) {
                if (is_space(*it)) {
                    it.get_stream().push_back();
                    break;
                }
                *val_it = *it;
            }
        }

        if (*format != char_type('}')) {
            throw failure(
                std::make_error_code(std::errc::invalid_argument),
                "Invalid format string: `span<char_type>` doesn't support "
                "format specifiers, expected '}'");
        }
        ++format;
        return it;
    }
};
}  // namespace spio

#endif
