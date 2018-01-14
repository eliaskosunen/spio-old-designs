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
#include "stl.h"

#if SPIO_USE_EXCEPTIONS
#include <exception>
#endif
#include <cassert>
#include <cerrno>

namespace io {
enum error_code {
    no_error = 0,
    invalid_argument,
    invalid_input,
    invalid_operation,
    io_error,
    assertion_failure,
    end_of_file,
    logic_error,
    default_error,
    unknown_error
};

struct error {
    error() = default;
    error(error_code c) : code(c) {}

    constexpr bool is_error() const
    {
        return code != no_error && !is_eof();
    }
    constexpr bool is_eof() const
    {
        return code == end_of_file;
    }

    constexpr operator bool() const
    {
        return is_error();
    }

    constexpr const char* to_string() const
    {
        switch (code) {
            case no_error:
                return "Success";
            case invalid_argument:
                return "Invalid argument";
            case invalid_input:
                return "Invalid input";
            case invalid_operation:
                return "Invalid operation";
            case io_error:
                return "IO error";
            case assertion_failure:
                return "Assertion failure";
            case end_of_file:
                return "End of file";
            case logic_error:
                return "Logic error";
            case default_error:
                return "Default error";
            case unknown_error:
                return "Unknown error";
        }
        return "";
    }
    constexpr const char* message() const
    {
        return to_string();
    }

    error_code code{no_error};
};
#if SPIO_USE_EXCEPTIONS
class failure : public std::exception {
public:
    explicit failure(error e) : failure(e, e.to_string()) {}
    failure(error e, const char* message) : m_error(e)
    {
        ::memcpy(&m_message[0], message, m_message.size());
    }
    explicit failure(error e, const char* message, std::size_t s) : m_error(e)
    {
        assert(s < 64);
        ::memcpy(&m_message[0], &message[0], s);
        m_message[s] = '\0';
    }

    const char* what() const noexcept override
    {
        return m_message.data();
    }

    error get_error() const noexcept
    {
        return m_error;
    }

private:
    error m_error;
    stl::array<char, 64> m_message{};
};

#define SPIO_THROW_MSG(msg) throw ::io::failure(default_error, msg)
#define SPIO_THROW_EC(ec) throw ::io::failure(ec, ::io::error(ec).to_string())
#define SPIO_THROW(ec, msg) throw ::io::failure(ec, msg)
#define SPIO_THROW_FAILURE(f) throw f
#define SPIO_RETHROW throw
#else
class failure {
};
#define SPIO_THROW_MSG(msg) \
    assert(false && (msg)); \
    std::terminate()
#define SPIO_THROW_EC(ec)            \
    assert(false && ec.to_string()); \
    std::terminate()
#define SPIO_THROW(ec, msg) \
    assert(false && (msg)); \
    std::terminate()
#define SPIO_THROW_FAILURE(f)    \
    assert(false && (f.what())); \
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

#define SPIO_UNUSED(x) (static_cast<void>(sizeof(x)))
}  // namespace io

#endif  // SPIO_ERROR_H
