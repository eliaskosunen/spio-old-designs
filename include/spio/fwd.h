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

#ifndef SPIO_FWD_H
#define SPIO_FWD_H

#include "config.h"
#include <type_traits>

namespace io {
#if SPIO_HAS_LOGICAL_TRAITS
template <typename... B>
using disjunction = std::disjunction<B...>;
#else
template <typename...>
struct disjunction : std::false_type {
};
template <typename B1>
struct disjunction<B1> : B1 {
};
template <typename B1, typename... Bn>
struct disjunction<B1, Bn...>
    : std::conditional_t<bool(B1::value), B1, disjunction<Bn...>> {
};
#endif

template <typename T, typename... Ts>
struct contains : disjunction<std::is_same<T, Ts>...> {
};

#if !SPIO_HAS_VOID_T
template <typename...>
using void_t = void;
#else
template <typename... T>
using void_t = std::void_t<T...>;
#endif
}  // namespace io

#endif
