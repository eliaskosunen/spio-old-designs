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

#ifndef SPIO_READER_OPTIONS_H
#define SPIO_READER_OPTIONS_H

#include "util.h"

namespace io {
#ifdef _MSC_VER
template <typename T>
struct is_reader : std::true_type {
};
#else
template <typename T, typename = void>
struct is_reader : std::false_type {
};
template <typename T>
struct is_reader<
    T,
    void_t<
        typename T::readable_type,
        typename T::char_type,
        decltype(std::declval<T>().read(
            std::declval<const typename T::char_type&>())),
        decltype(std::declval<T>().read(std::declval<int&>())),
        decltype(std::declval<T>().read(std::declval<double&>())),
        decltype(
            std::declval<T>().read(std::declval<typename T::char_type&>())),
        decltype(std::declval<T>().read(
            std::declval<span<typename T::char_type>>())),
        decltype(std::declval<T>().read_raw(
            std::declval<const typename T::char_type&>())),
        decltype(std::declval<T>().read_raw(
            std::declval<span<typename T::char_type>>())),
        decltype(std::declval<T>().get(std::declval<typename T::char_type&>())),
        decltype(std::declval<T>().getline(
            std::declval<span<typename T::char_type>>(),
            std::declval<typename T::char_type>())),
        decltype(std::declval<T>().ignore(std::declval<std::size_t>())),
        decltype(
            std::declval<T>().ignore(std::declval<std::size_t>(),
                                     std::declval<typename T::char_type>())),
        decltype(std::declval<T>().push(std::declval<typename T::char_type>())),
        decltype(std::declval<T>().push(
            std::declval<span<typename T::char_type>>())),
#if SPIO_USE_FMT
        decltype(std::declval<T>().scan(std::declval<const char*>(),
                                        std::declval<int>(),
                                        std::declval<bool>())),
#endif
        decltype(!std::declval<T>()),
        decltype(std::declval<T>().get_readable())>> : std::true_type {
};
#endif

template <typename T, typename Enable = void>
struct reader_options {
};

template <typename T>
struct reader_options<T,
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

template <typename T>
struct reader_options<
    T,
    std::enable_if_t<contains<std::decay_t<typename T::element_type>,
                              char,
                              wchar_t,
                              unsigned char,
                              signed char,
                              char16_t,
                              char32_t>::value>> {
    span<typename T::element_type> spaces{nullptr};
};

template <>
struct reader_options<bool> {
    bool alpha{false};
};
}  // namespace io

#endif  // SPIO_READER_OPTIONS_H
