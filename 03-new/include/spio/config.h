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

#ifndef SPIO_HEADER_ONLY
#define SPIO_HEADER_ONLY 1
#endif

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

#if defined(__cpp_if_constexpr) || __cplusplus >= 201703L
#define SPIO_HAS_IF_CONSTEXPR 1
#else
#define SPIO_HAS_IF_CONSTEXPR 0
#endif

#if defined(__cpp_lib_type_trait_variable_templates) || __cplusplus >= 201703L
#define SPIO_HAS_TYPE_TRAITS_V 1
#else
#define SPIO_HAS_TYPE_TRAITS_V 0
#endif

#if defined(__cpp_lib_experimental_logical_traits) || __cplusplus >= 201703L
#define SPIO_HAS_LOGICAL_TRAITS 1
#else
#define SPIO_HAS_LOGICAL_TRAITS 0
#endif

#if defined(__cpp_lib_byte) || __cplusplus >= 201703L
#define SPIO_HAS_BYTE 1
#else
#define SPIO_HAS_BYTE 0
#endif

#if defined(__cpp_fold_expression) || __cplusplus >= 201703L
#define SPIO_HAS_FOLD_EXPRESSIONS 1
#else
#define SPIO_HAS_FOLD_EXPRESSIONS 0
#endif

#if defined(__cpp_deduction_guides) || __cplusplus >= 201703L
#define SPIO_HAS_DEDUCTION_GUIDES 1
#else
#define SPIO_HAS_DEDUCTION_GUIDES 0
#endif

//
// Definitions
//

#if SPIO_HEADER_ONLY
#define SPIO_INLINE inline
#else
#define SPIO_INLINE
#endif

#if !SPIO_USE_STL
#if !defined(SPIO_VECTOR) || !defined(SPIO_ARRAY)
#error !SPIO_USE_STL requires SPIO_VECTOR and SPIO_ARRAY to be defined
#endif
#endif  // !SPIO_USE_STL

#define SPIO_STRINGIZE_DETAIL(x) #x
#define SPIO_STRINGIZE(x) SPIO_STRINGIZE_DETAIL(x)
#define SPIO_LINE SPIO_STRINGIZE(__LINE__)

#endif  // SPIO_CONFIG_H
