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

#ifndef SPIO_CONFIG_H
#define SPIO_CONFIG_H

//
// Options
//

#ifndef SPIO_USE_STREAMS
#define SPIO_USE_STREAMS 1
#endif

#ifndef SPIO_USE_STL
#define SPIO_USE_STL 1
#endif

#ifndef SPIO_USE_FMT
#define SPIO_USE_FMT 1
#endif

#ifndef SPIO_USE_FMT_OSTREAM
#define SPIO_USE_FMT_OSTREAM 1
#endif

#ifndef SPIO_USE_EXCEPTIONS
#define SPIO_USE_EXCEPTIONS 1
#endif

#ifndef SPIO_THROW_ON_ASSERT
#define SPIO_THROW_ON_ASSERT 0
#endif

#if !SPIO_USE_STL && SPIO_USE_FMT
#error fmtlib (SPIO_USE_FMT) requires STL (SPIO_USE_STL)
#endif

#if !SPIO_USE_FMT && SPIO_USE_FMT_OSTREAM
#error SPIO_USE_FMT required for SPIO_USE_FMT_OSTREAM
#endif

//
// Compatibility
//

#if !defined(SPIO_POSIX) && !defined(SPIO_WIN32)
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#define SPIO_POSIX 1
#define SPIO_WIN32 0
#endif

#ifdef _WIN32
#define SPIO_POSIX 0
#define SPIO_WIN32 1
#endif
#endif

#if SPIO_POSIX && SPIO_WIN32
#error Both SPIO_POSIX and SPIO_WIN32 detected
#endif

#if !SPIO_POSIX && !SPIO_WIN32
#define SPIO_HAS_NATIVE_FILEIO 0
#else
#define SPIO_HAS_NATIVE_FILEIO 1
#endif

#if defined(__cpp_if_constexpr)
#define SPIO_HAS_IF_CONSTEXPR 1
#else
#define SPIO_HAS_IF_CONSTEXPR 0
#endif

#if defined(__cpp_lib_type_trait_variable_templates)
#define SPIO_HAS_TYPE_TRAITS_V 1
#else
#define SPIO_HAS_TYPE_TRAITS_V 0
#endif

#if defined(__cpp_lib_experimental_logical_traits)
#define SPIO_HAS_LOGICAL_TRAITS 1
#else
#define SPIO_HAS_LOGICAL_TRAITS 0
#endif

#if defined(__cpp_lib_byte)
#define SPIO_HAS_BYTE 1
#else
#define SPIO_HAS_BYTE 0
#endif

#if defined(__cpp_fold_expression)
#define SPIO_HAS_FOLD_EXPRESSIONS 1
#else
#define SPIO_HAS_FOLD_EXPRESSIONS 0
#endif

#if defined(__cpp_deduction_guides)
#define SPIO_HAS_DEDUCTION_GUIDES 1
#else
#define SPIO_HAS_DEDUCTION_GUIDES 0
#endif

//
// Definitions
//

#define SPIO_INLINE inline

#if !SPIO_USE_STL
#if !defined(SPIO_VECTOR) || !defined(SPIO_ARRAY)
#error !SPIO_USE_STL requires SPIO_VECTOR and SPIO_ARRAY to be defined
#endif
#endif  // !SPIO_USE_STL

#define SPIO_STRINGIZE_DETAIL(x) #x
#define SPIO_STRINGIZE(x) SPIO_STRINGIZE_DETAIL(x)
#define SPIO_LINE SPIO_STRINGIZE(__LINE__)

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
}  // namespace io

#endif  // SPIO_CONFIG_H
