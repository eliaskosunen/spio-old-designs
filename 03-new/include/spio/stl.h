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

#ifndef SPIO_STL_H
#define SPIO_STL_H

#include "config.h"

#if SPIO_USE_STL
#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <vector>
#endif  // SPIO_USE_STL
#include <cstring>
#include <iterator>

namespace io {
#if SPIO_USE_STL
#define SPIO_STL_NS ::std

#ifndef SPIO_STL_DEFAULT_ALLOCATOR
#define SPIO_STL_DEFAULT_ALLOCATOR ::std::allocator
#endif
#endif

namespace stl {
    template <typename T>
    using allocator = SPIO_STL_DEFAULT_ALLOCATOR<T>;

    template <typename Element,
              typename Allocator = allocator<Element>>
    using vector = SPIO_STL_NS::vector<Element, Allocator>;

    using SPIO_STL_NS::array;
    using SPIO_STL_NS::char_traits;
    using SPIO_STL_NS::reference_wrapper;
    using SPIO_STL_NS::reverse_iterator;
    using SPIO_STL_NS::unique_ptr;

    using SPIO_STL_NS::advance;
    using SPIO_STL_NS::begin;
    using SPIO_STL_NS::copy;
    using SPIO_STL_NS::distance;
    using SPIO_STL_NS::end;
    using SPIO_STL_NS::equal;
    using SPIO_STL_NS::find;
    using SPIO_STL_NS::find_if;
    using SPIO_STL_NS::lexicographical_compare;
    using SPIO_STL_NS::make_reverse_iterator;
    using SPIO_STL_NS::make_unique;

    template <typename CharT>
    constexpr std::ptrdiff_t strlen(const CharT* str) noexcept
    {
        assert(str);
        const CharT* s = str;
        for (; *s; ++s) {
        }
        return (s - str);
    }
    template <>
    inline std::ptrdiff_t strlen(const char* str) noexcept
    {
        return static_cast<std::ptrdiff_t>(SPIO_STL_NS::strlen(str));
    }
    template <>
    inline std::ptrdiff_t strlen(const wchar_t* str) noexcept
    {
        return static_cast<std::ptrdiff_t>(SPIO_STL_NS::wcslen(str));
    }
}  // namespace stl
}  // namespace io

#endif  // SPIO_STL_H
