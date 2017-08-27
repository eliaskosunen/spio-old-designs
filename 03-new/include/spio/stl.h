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

#ifndef SPIO_STL_H
#define SPIO_STL_H

#include "config.h"

#if SPIO_USE_STL
#include <algorithm>
#include <array>
#include <vector>
#endif  // SPIO_USE_STL
#include <cstring>
#include <iterator>

namespace io {
using std::strcpy;
using std::strlen;

using std::advance;
using std::distance;

#if SPIO_USE_STL
template <typename Element, std::size_t N>
using array = std::array<Element, N>;

template <typename Element>
using vector = std::vector<Element>;

using std::copy;
using std::equal;
using std::find;
using std::lexicographical_compare;
#else
template <typename Element, std::size_t N>
using array = SPIO_ARRAY<Element, N>;

template <typename Element>
using vector = SPIO_VECTOR<Element>;

template <typename InputIt, typename T>
constexpr InputIt find(InputIt first, InputIt last, const T& value)
{
    for (; first != last; ++first) {
        if (*first == value) {
            return first;
        }
    }
    return last;
}

template <typename InputIt, typename OutputIt>
constexpr OutputIt copy(InputIt first, InputIt last, OutputIt d_first)
{
    while (first != last) {
        *d_first++ = *first++;
        return d_first;
    }
}

template <typename InputIt1, typename InputIt2>
constexpr bool equal(InputIt1 first1,
                     InputIt1 last2,
                     InputIt2 first2,
                     InputIt2 last2)
{
    for (; (first1 != last1) && (first2 != last2); ++first1, (void)++first2) {
        if (!(*first1 == *first2)) {
            return false;
        }
    }
    return (first1 == last1) && (first2 != last2);
}

template <class InputIt1, class InputIt2>
constexpr bool lexicographical_compare(InputIt1 first1,
                                       InputIt1 last1,
                                       InputIt2 first2,
                                       InputIt2 last2)
{
    for (; (first1 != last1) && (first2 != last2); ++first1, (void)++first2) {
        if (*first1 < *first2)
            return true;
        if (*first2 < *first1)
            return false;
    }
    return (first1 == last1) && (first2 != last2);
}
#endif  // SPIO_USE_STL
}  // namespace io

#endif  // SPIO_STL_H
