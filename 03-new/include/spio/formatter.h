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

#ifndef SPIO_FORMATTER_H
#define SPIO_FORMATTER_H

#include "fwd.h"

#include "codeconv.h"
#include "fmt.h"
#include "locale.h"
#include "util.h"

namespace spio {
namespace detail {
    template <typename CharT>
    struct fmt_to_string;

    template <>
    struct fmt_to_string<char> {
        template <typename T>
        static std::string str(const T& a)
        {
            return fmt::to_string(a);
        }
    };
    template <>
    struct fmt_to_string<wchar_t> {
        template <typename T>
        static std::wstring str(const T& a)
        {
            return fmt::to_wstring(a);
        }
    };
}  // namespace detail

template <typename CharT>
class basic_fmt_formatter {
public:
    using char_type = CharT;
    using string_type = std::basic_string<char_type>;

    template <typename Args>
    string_type operator()(const char_type* f, Args a) const;

    template <typename... Args>
    string_type format(const char_type* f, const Args&... a) const
    {
        using context = typename fmt::buffer_context<char_type>::type;
        return operator()(
            f, fmt::basic_format_args<context>(fmt::make_args<context>(a...)));
    }

    template <typename T>
    string_type to_string(const T& a) const;
};

#if 0
class basic_fmt_formatter {
public:
    using char_type = CharT;
    using result = std::basic_string<char_type>;

    template <typename T>
    result operator()(const T& a)
    {
        return m_conv(m_fmt(a));
    }
    result operator()(const char_type* s)
    {
        return m_conv(m_fmt(m_conv.reverse(s)));
    }

    template <typename... Args>
    result format(const char_type* f, const Args&... a)
    {
        auto fmt = m_conv.reverse(f);
        return m_conv(m_fmt.format(fmt.c_str(), a...));
    }

private:
    codeconv<char, char_type> m_conv;
    basic_fmt_formatter<char> m_fmt;
};
#endif
}  // namespace spio

#include "formatter.impl.h"

#endif
