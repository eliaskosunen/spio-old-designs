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

#include "config.h"
#include "fmt.h"

#include <cassert>
#include <cerrno>
#include <exception>
#include <stdexcept>
#include <string>
#include <system_error>

namespace io {
enum error {
    invalid_input,
    invalid_operation,
    assertion_failure,
    end_of_file,
    unimplemented,
    unreachable,
    undefined_error
};
}

namespace std {
template <>
struct is_error_condition_enum<io::error> : true_type {
};
}  // namespace std

namespace io {
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
            case unimplemented:
                return "Unimplemented";
            case unreachable:
                return "Unreachable code";
            case undefined_error:
                return "[undefined error]";
        }
        assert(false);
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

inline std::error_code make_error_condition(error e)
{
    return {static_cast<int>(e), detail::get_error_category()};
}

inline bool is_eof(const std::error_code& e)
{
    return e == make_error_condition(end_of_file);
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

#if SPIO_USE_EXCEPTIONS
#define SPIO_THROW_MSG(msg) throw ::io::failure(::io::default_error, msg)
#define SPIO_THROW_EC(ec) throw ::io::failure(ec)
#define SPIO_THROW_ERRNO throw ::io::failure(SPIO_MAKE_ERRNO)
#define SPIO_THROW(ec, msg) throw ::io::failure(ec, msg)
#define SPIO_THROW_FAILURE(f) throw f
#define SPIO_RETHROW throw
#else
class failure {
};
#define SPIO_THROW_MSG(msg) \
    assert(false && (msg)); \
    std::terminate()
#define SPIO_THROW_EC(ec) \
    assert(false && ec);  \
    std::terminate()
#define SPIO_THROW_ERRNO    \
    assert(false && errno); \
    std::terminate()
#define SPIO_THROW(ec, msg) \
    SPIO_UNUSED(ec);        \
    assert(false && (msg)); \
    std::terminate()
#define SPIO_THROW_FAILURE(f) \
    SPIO_UNUSED(f);           \
    assert(false);            \
    std::terminate()
#define SPIO_RETHROW                                   \
    assert(false && "Rethrowing caught exception..."); \
    std::terminate()
#endif

#if SPIO_THROW_ON_ASSERT
#define SPIO_ASSERT(cond, msg)                        \
    do {                                              \
        if (!(cond)) {                                \
            SPIO_THROW(::io::assertion_failure, msg); \
        }                                             \
    } while (false)
#else
#define SPIO_ASSERT(cond, msg) assert((cond) && msg)
#endif

#define SPIO_UNIMPLEMENTED SPIO_THROW(::io::unimplemented, "Unimplemented")
#define SPIO_UNREACHABLE SPIO_THROW(::io::unreachable, "Unreachable")
}  // namespace io

#endif  // SPIO_ERROR_H
