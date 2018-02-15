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

#ifndef SPIO_CODECONV_H
#define SPIO_CODECONV_H

#include <codecvt>
#include "fwd.h"
#include "locale.h"
#include "span.h"

namespace spio {
template <typename Source, typename Dest>
class codeconv {
    using converter_type = std::wstring_convert<std::codecvt_utf8<Dest>, Dest>;
    converter_type converter;

public:
    typename converter_type::wide_string operator()(span<Source> s)
    {
        return converter.from_bytes({s.begin(), s.end()});
    }
    typename converter_type::wide_string operator()(
        typename converter_type::byte_string s)
    {
        return converter.from_bytes(s);
    }

    typename converter_type::byte_string reverse(span<Dest> s)
    {
        return converter.to_bytes({s.begin(), s.end()});
    }
    typename converter_type::byte_string reverse(
        typename converter_type::wide_string s)
    {
        return converter.to_bytes(s);
    }
};

template <typename Char>
class codeconv<Char, Char> {
public:
    std::basic_string<Char> operator()(span<Char> s) const
    {
        return {s.begin(), s.end()};
    }
    std::basic_string<Char> operator()(std::basic_string<Char> s) const
    {
        return s;
    }

    auto reverse(span<Char> s) const
    {
        return operator()(s);
    }
    auto reverse(std::basic_string<Char> s) const
    {
        return operator()(s);
    }
};
}  // namespace spio

#endif
