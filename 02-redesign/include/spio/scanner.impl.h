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

#ifndef SPIO_SCANNER_IMPL_H
#define SPIO_SCANNER_IMPL_H

#include "fwd.h"

#include "scanner.h"

namespace spio {
template <typename CharT>
template <typename... Args>
auto basic_builtin_scanner<CharT>::do_scan(iterator it,
                                           const char_type* format,
                                           bool readall,
                                           T& arg,
                                           Args&... args) -> iterator
{
    for (; *format != char_type(0); ++format) {
        if (*format == char_type('{')) {
            ++format;
            if(*format != char_type('{') {
                it = scan(it, format, readall, arg);
                break;
            }
        }
        ++it;
        auto ch = *it;
        if (ch != *format) {
            throw failure(std::make_error_code(std::errc::invalid_argument),

                          "Invalid format string: matching character " +
                              std::basic_string<char_type>(*format) +
                              " found in the input stream");
        }
    }
    return do_scan(it, format, readall, args...);
}

template <typename CharT>
auto basic_builtin_scanner<CharT>::do_scan(iterator it,
                                           const char_type* format,
                                           bool readall) -> iterator
{
    for (; *format != char_type(0); ++format) {
        if (*format == char_type('{')) {
            ++format;
            if (*format != char_type('{')) {
                throw failure(
                    std::make_error_code(std::errc::invalid_argument),
                    "Invalid format string: argument pack size mismatch");
            }
        }
        ++it;
        auto ch = *it;
        if (ch != *format) {
            throw failure(std::make_error_code(std::errc::invalid_argument),

                          "Invalid format string: matching character " +
                              std::basic_string<char_type>(*format) +
                              " found in the input stream");
        }
    }
    return it;
}
}  // namespace spio

#endif
