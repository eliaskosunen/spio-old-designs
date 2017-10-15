// Copyright 2017 Elias Kosunen
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

#ifndef SPIO_CUSTOM_TYPE_H
#define SPIO_CUSTOM_TYPE_H

#include "config.h"
#include "fmt.h"
#include "reader_options.h"
#include "writer_options.h"

namespace io {
template <typename T>
struct custom_read;

template <typename T>
struct custom_write;

#if SPIO_USE_FMT
template <typename T>
struct custom_write {
    template <typename Writer>
    static void write(Writer& w, const T& val, writer_options<T> opt)
    {
        SPIO_UNUSED(opt);
        const auto str = fmt::format("{}", val);
        w.write(str.c_str());
    }
};
#endif

#if SPIO_USE_STL
template <typename CharT, typename Allocator>
struct custom_read<std::basic_string<CharT, Allocator>> {
    using type = std::basic_string<CharT, Allocator>;

    template <typename Reader>
    static bool read(Reader& p, type& val, reader_options<type> opt) {
        SPIO_UNUSED(opt);
        return p.read(make_span(val));
    }
};

template <typename CharT, typename Allocator>
struct custom_write<std::basic_string<CharT, Allocator>> {
    using type = std::basic_string<CharT, Allocator>;

    template <typename Writer>
    static void write(Writer& p, const type& val, writer_options<type> opt) {
        SPIO_UNUSED(opt);
        p.write(val.c_str());
    }
};
#endif
}  // namespace io

/* #if SPIO_USE_FMT */
/* namespace fmt { */
/* template <typename T, typename CharT> */
/* void format_arg(fmt::BasicFormatter<CharT>& f, */
/*                 const CharT*& format_str, */
/*                 const T& val) */
/* { */
/*     io::custom_write<T>::write_fmt(f, format_str, val); */
/* } */
/* }  // namespace fmt */
/* #endif */

#endif  // SPIO_CUSTOM_TYPE_H
