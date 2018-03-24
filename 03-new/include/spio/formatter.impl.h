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

#ifndef SPIO_FORMATTER_IMPL_H
#define SPIO_FORMATTER_IMPL_H

#include "config.h"
#include "formatter.h"
#include "stream_iterator.h"

namespace spio {
template <typename CharT>
template <typename T>
auto basic_fmt_formatter<CharT>::to_string(const T& a) const -> string_type
{
    return detail::fmt_to_string<CharT>::str(a);
}

template <typename CharT>
template <typename Args>
auto basic_fmt_formatter<CharT>::operator()(const char_type* f, Args a) const
    -> std::basic_string<char_type>
{
    using buffer = fmt::basic_memory_buffer<CharT>;
    buffer b;
    fmt::vformat_to(b, f, a);
    return {b.data(), b.size()};
}
}  // namespace spio

#endif
