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
void basic_builtin_scanner<CharT>::vscan(stream_type& s,
                                         span<const char> format,
                                         bool readall,
                                         arg_list args)
{
    auto& arg_vec = args.get();
    auto arg = arg_vec.begin();
    auto opt = make_scan_options(readall);
    auto f_it = format.begin();
    while (f_it != format.end()) {
        skip_ws(s);
        if (*f_it == char_type('{')) {
            ++f_it;
            if (f_it == format.end()) {
                throw failure(std::make_error_code(std::errc::invalid_argument),
                              "Invalid format string: no matching brace");
            }
            if (*f_it != char_type('{')) {
                if (!arg->scan(s, f_it, opt, arg->value)) {
                    throw failure(make_error_code(unknown_io_error),
                                  "Unknown error in scanning");
                }
                ++arg;
                if (f_it != format.end()) {
                    ++f_it;
                }
                continue;
            }
        }
        if (f_it == format.end()) {
            break;
        }
        skip_ws(s);
        if (f_it == format.end()) {
            break;
        }
        auto ch = s.get();
        if (ch != *f_it) {
            throw failure(std::make_error_code(std::errc::invalid_argument),
                          "Invalid format string: no matching character " +
                              std::basic_string<char_type>(f_it, format.end()) +
                              " found in the input stream");
        }
    }
}
}  // namespace spio

#endif
