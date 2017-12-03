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

#ifndef SPIO_WRITER_OPTIONS_H
#define SPIO_WRITER_OPTIONS_H

#include "util.h"

namespace io {
#ifdef _MSC_VER
template <typename T>
struct is_writer : std::true_type {
};
#else
template <typename T, typename = void>
struct is_writer : std::false_type {
};
template <typename T>
struct is_writer<
    T,
    void_t<typename T::writable_type,
           typename T::char_type,
           decltype(std::declval<T>().write(
               std::declval<const typename T::char_type&>())),
           decltype(std::declval<T>().write(std::declval<const int&>())),
           decltype(std::declval<T>().write(std::declval<const double&>())),
           decltype(std::declval<T>().write(
               std::declval<const typename T::char_type*&>())),
           decltype(std::declval<T>().write_raw(
               std::declval<const typename T::char_type&>())),
           decltype(std::declval<T>().write_raw(
               std::declval<span<typename T::char_type>>())),
           decltype(
               std::declval<T>().put(std::declval<typename T::char_type>())),
           decltype(std::declval<T>().flush()),
           decltype(std::declval<T>().nl()),
           decltype(std::declval<T>().eof()),
#if SPIO_USE_FMT
           decltype(std::declval<T>().print(std::declval<const char*>(),
                                            std::declval<int>(),
                                            std::declval<bool>())),
           decltype(std::declval<T>().println(std::declval<const char*>(),
                                              std::declval<int>(),
                                              std::declval<bool>())),
#endif
           decltype(!std::declval<T>()),
           decltype(std::declval<T>().get_writable())>> : std::true_type {
};
#endif

template <typename T, typename Enable = void>
struct writer_options {
};

template <typename T>
struct writer_options<T,
                      std::enable_if_t<contains<std::decay_t<T>,
                                                short,
                                                int,
                                                long,
                                                long long,
                                                unsigned short,
                                                unsigned int,
                                                unsigned long,
                                                unsigned long long>::value>> {
    int base{10};
};

template <>
struct writer_options<bool> {
    bool alpha{true};
};
}  // namespace io

#endif  // SPIO_WRITER_OPTIONS_H
