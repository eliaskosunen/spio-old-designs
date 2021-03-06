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

#ifndef SPIO_CONFIG_H
#define SPIO_CONFIG_H

//
// Options
//

#ifndef SPIO_USE_THREADING
#define SPIO_USE_THREADING 1
#endif

#ifndef SPIO_USE_FMT_OSTREAM
#define SPIO_USE_FMT_OSTREAM 1
#endif

#ifndef SPIO_FAILURE_USE_STRING
#define SPIO_FAILURE_USE_STRING 1
#endif

#ifndef SPIO_THROW_ON_ASSERT
#define SPIO_THROW_ON_ASSERT 0
#endif

//
// Compatibility
//

#if !defined(SPIO_POSIX) && !defined(SPIO_WIN32)
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#define SPIO_POSIX 1
#define SPIO_WIN32 0
#endif

#if defined(_WIN32) || defined(WIN32)
#define SPIO_POSIX 0
#define SPIO_WIN32 1
#endif
#endif

#if SPIO_POSIX && SPIO_WIN32
#error Both SPIO_POSIX and SPIO_WIN32 detected
#endif

#ifdef _MSC_VER
#define SPIO_MSC_VER _MSC_VER
#else
#define SPIO_MSC_VER 0
#endif

#if !SPIO_POSIX
#define SPIO_HAS_NATIVE_FILEIO 0
#else
#define SPIO_HAS_NATIVE_FILEIO 1
#endif

#if defined(__cpp_if_constexpr) || SPIO_MSC_VER >= 1910
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

#if defined(__cpp_lib_byte) || \
    (SPIO_MSC_VER >= 1910 && defined(_MSVC_LANG) && _MSVC_LANG >= 201703)
#define SPIO_HAS_BYTE 1
#else
#define SPIO_HAS_BYTE 0
#endif

#if defined(__cpp_fold_expression) || SPIO_MSC_VER >= 1910
#define SPIO_HAS_FOLD_EXPRESSIONS 1
#else
#define SPIO_HAS_FOLD_EXPRESSIONS 0
#endif

#if defined(__cpp_deduction_guides)
#define SPIO_HAS_DEDUCTION_GUIDES 1
#else
#define SPIO_HAS_DEDUCTION_GUIDES 0
#endif

#if defined(__cpp_lib_void_t) || SPIO_MSC_VER >= 1900
#define SPIO_HAS_VOID_T 1
#else
#define SPIO_HAS_VOID_T 0
#endif

#if defined(__cpp_lib_is_invocable) || \
    (SPIO_MSC_VER >= 1910 && defined(_MSVC_LANG) && _MSVC_LANG >= 201703)
#define SPIO_HAS_INVOCABLE 1
#else
#define SPIO_HAS_INVOCABLE 0
#endif

//
// Definitions
//

#define SPIO_STRINGIZE_DETAIL(x) #x
#define SPIO_STRINGIZE(x) SPIO_STRINGIZE_DETAIL(x)
#define SPIO_LINE SPIO_STRINGIZE(__LINE__)

#define SPIO_UNUSED(x) (static_cast<void>(sizeof(x)))

#endif  // SPIO_CONFIG_H
