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

#include "fwd.h"

#include <codecvt>
#include <cstring>
#include "locale.h"
#include "span.h"

namespace spio {
template <typename Source, typename Dest>
class codeconv {
    using converter_type = std::wstring_convert<std::codecvt_utf8<Dest>, Dest>;
    converter_type converter;

public:
    template <typename T>
    auto operator()(T s)
        -> std::enable_if_t<std::is_same<Source, typename T::value_type>::value,
                            typename converter_type::wide_string>
    {
        return converter.from_bytes({s.begin(), s.end()});
    }
    template <typename T>
    auto operator()(T* s)
        -> std::enable_if_t<std::is_same<Source, std::remove_cv_t<T>>::value,
                            typename converter_type::wide_string>
    {
        return operator()(typename converter_type::byte_string(s));
    }
    typename converter_type::wide_string operator()(
        typename converter_type::byte_string s)
    {
        return converter.from_bytes(s);
    }

    template <typename T>
    auto reverse(T s)
        -> std::enable_if_t<std::is_same<Dest, typename T::value_type>::value,
                            typename converter_type::byte_string>
    {
        return converter.to_bytes({s.begin(), s.end()});
    }
    template <typename T>
    auto reverse(T* s)
        -> std::enable_if_t<std::is_same<Dest, std::remove_cv_t<T>>::value,
                            typename converter_type::byte_string>
    {
        return reverse(typename converter_type::wide_string(s));
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
    template <typename T>
    auto operator()(T s) const
        -> std::enable_if_t<std::is_same<Char, typename T::value_type>::value,
                            std::basic_string<Char>>
    {
        return {s.begin(), s.end()};
    }
    template <typename T>
    auto operator()(T* s) const
        -> std::enable_if_t<std::is_same<Char, std::remove_cv_t<T>>::value,
                            std::basic_string<Char>>
    {
        return operator()(std::basic_string<Char>(s));
    }
    std::basic_string<Char> operator()(std::basic_string<Char> s) const
    {
        return s;
    }

    template <typename T>
    auto reverse(T s) const
        -> std::enable_if_t<std::is_same<Char, typename T::value_type>::value,
                            std::basic_string<Char>>
    {
        return operator()(s);
    }
    template <typename T>
    auto reverse(T* s) const
        -> std::enable_if_t<std::is_same<Char, std::remove_cv_t<T>>::value,
                            std::basic_string<Char>>
    {
        return reverse(std::basic_string<Char>(s));
    }
    auto reverse(std::basic_string<Char> s) const
    {
        return operator()(s);
    }
};
}  // namespace spio

#endif
