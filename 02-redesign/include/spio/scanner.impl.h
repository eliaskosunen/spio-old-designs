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

#include "codeconv.h"
#include "scanner.h"

namespace spio {
template <typename CharT>
void basic_scanner<CharT>::vscan(stream_type& s,
                                 const CharT* format,
                                 bool readall,
                                 arg_list args)
{
    auto& arg_vec = args.get();
    auto arg = arg_vec.begin();
    auto opt = make_scan_options(readall);
    while (*format != 0) {
        skip_ws(s);
        if (*format == CharT('{')) {
            ++format;
            if (*format == 0) {
                throw failure(std::make_error_code(std::errc::invalid_argument),
                              "Invalid format string: no matching brace");
            }
            if (*format != CharT('{')) {
                if (!arg->scan(s, format, opt, arg->value)) {
                    throw failure(make_error_code(unknown_io_error),
                                  "Unknown error in scanning");
                }
                ++arg;
                if (*format != 0) {
                    ++format;
                }
                continue;
            }
        }
        if (*format == 0) {
            break;
        }
        skip_ws(s);
        if (*format == 0) {
            break;
        }
        auto ch = s.get();
        if (ch != *format) {
            codeconv<char, CharT> conv;
            throw failure(std::make_error_code(std::errc::invalid_argument),
                          "Invalid format string: no matching character '" +
                              conv.reverse(make_span(format, 1)) +
                              "' found in the input stream");
        }
    }
}
}  // namespace spio

#endif
