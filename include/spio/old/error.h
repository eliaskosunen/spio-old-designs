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

#ifndef SPIO_ERROR_H
#define SPIO_ERROR_H

#include "fwd.h"

#include <cassert>
#include <cerrno>
#include <exception>
#include <stdexcept>
#include <string>
#include <system_error>
#include "fmt.h"

namespace spio {
enum error {
    invalid_input,
    invalid_operation,
    assertion_failure,
    end_of_file,
    unknown_io_error,
    bad_variant_access,
    out_of_range,
    sentry_error,
    unimplemented,
    unreachable,
    undefined_error
};
}

namespace std {
template <>
struct is_error_code_enum<spio::error> : true_type {
};
}  // namespace std

namespace spio {
struct error_category : public std::error_category {
    const char* name() const noexcept override
    {
        return "spio";
    }

    std::string message(int ev) const override
    {
        switch (static_cast<error>(ev)) {
            case invalid_input:
                return "Invalid input";
            case invalid_operation:
                return "Invalid operation";
            case assertion_failure:
                return "Assertion failure";
            case end_of_file:
                return "EOF";
            case unknown_io_error:
                return "Unknown IO error";
            case bad_variant_access:
                return "Bad variant access";
            case out_of_range:
                return "Out of range";
            case sentry_error:
                return "Sentry error";
            case unimplemented:
                return "Unimplemented";
            case unreachable:
                return "Unreachable code";
            case undefined_error:
                return "[undefined error]";
        }
        assert(false);
        std::terminate();
    }
};

namespace detail {
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif
    inline const error_category& get_error_category()
    {
        static error_category inst;
        return inst;
    }
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
}  // namespace detail

inline std::error_code make_error_code(error e)
{
    return {static_cast<int>(e), detail::get_error_category()};
}

inline bool is_eof(const std::error_code& e)
{
    return e == make_error_code(end_of_file);
}

#if SPIO_WIN32
#define SPIO_MAKE_ERRNO ::std::error_code(errno, ::std::generic_category())
#define SPIO_MAKE_WIN32_ERROR \
    ::std::error_code(::GetLastError(), ::std::system_category())
#else
#define SPIO_MAKE_ERRNO ::std::error_code(errno, ::std::system_category())
#endif

class failure : public std::system_error {
public:
    failure(std::error_code c) : system_error(c) {}
    failure(std::error_code c, const std::string& desc) : system_error(c, desc)
    {
    }

    failure(error c)
        : system_error(static_cast<int>(c), detail::get_error_category())
    {
    }
    failure(error c, const std::string& desc)
        : system_error(static_cast<int>(c), detail::get_error_category(), desc)
    {
    }
};

#if SPIO_THROW_ON_ASSERT
#define SPIO_ASSERT(cond, msg)                             \
    do {                                                   \
        if (!(cond)) {                                     \
            throw failure(::spio::assertion_failure, msg); \
        }                                                  \
    } while (false)
#else
#define SPIO_ASSERT(cond, msg) assert((cond) && msg)
#endif

#ifdef _MSC_VER
#define SPIO_DEBUG_UNREACHABLE                                            \
    __pragma(warning(suppress : 4702)) throw failure(::spio::unreachable, \
                                                     "Unreachable");      \
    __pragma(warning(suppress : 4702)) std::terminate()
#else
#define SPIO_DEBUG_UNREACHABLE                         \
    throw failure(::spio::unreachable, "Unreachable"); \
    std::terminate()
#endif

#ifdef NDEBUG

#ifdef __GNUC__
#define SPIO_UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
#define SPIO_UNREACHABLE __pragma(warning(suppress : 4702)) __assume(false)
#else
#define SPIO_UNREACHABLE SPIO_DEBUG_UNREACHABLE
#endif  // __GNUC__

#else
#define SPIO_UNREACHABLE SPIO_DEBUG_UNREACHABLE
#endif  // NDEBUG

#ifdef _MSC_VER
#define SPIO_UNIMPLEMENTED_DEBUG                                            \
    __pragma(warning(suppress : 4702)) throw failure(::spio::unimplemented, \
                                                     "Unimplemented");      \
    __pragma(warning(suppress : 4702)) std::terminate()
#else
#define SPIO_UNIMPLEMENTED_DEBUG                           \
    throw failure(::spio::unimplemented, "Unimplemented"); \
    std::terminate()
#endif

#ifdef NDEBUG
#define SPIO_UNIMPLEMENTED SPIO_UNREACHABLE
#else
#define SPIO_UNIMPLEMENTED SPIO_UNIMPLEMENTED_DEBUG
#endif  // NDEBUG
}  // namespace spio

#endif  // SPIO_ERROR_H
