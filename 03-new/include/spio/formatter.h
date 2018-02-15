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

#include "codeconv.h"
#include "fmt.h"
#include "fwd.h"
#include "locale.h"

namespace spio {
template <typename CharT>
class basic_fmt_formatter;

template <>
class basic_fmt_formatter<char> {
public:
    using result = std::string;

    template <typename T>
    result operator()(const T& a) const
    {
        return fmt::format("{}", a);
    }

    template <typename... Args>
    result format(const char* f, const Args&... a) const
    {
        return fmt::format(f, a...);
    }
};

template <typename CharT>
class basic_fmt_formatter {
public:
    using result = std::basic_string<CharT>;

    template <typename T>
    result operator()(const T& a)
    {
        return m_conv(m_fmt(a));
    }
    result operator()(const CharT* s)
    {
        return m_conv(m_fmt(m_conv.reverse(s)));
    }

    template <typename... Args>
    result format(const CharT* f, const Args&... a)
    {
        auto fmt = m_conv.reverse(f);
        return m_conv(m_fmt.format(fmt.c_str(), a...));
    }

private:
    codeconv<char, CharT> m_conv;
    basic_fmt_formatter<char> m_fmt;
};

}  // namespace spio

#endif
