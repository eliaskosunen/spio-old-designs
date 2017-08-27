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

#ifndef SPIO_UTIL_H
#define SPIO_UTIL_H

#include <cstring>
#include "config.h"
#include "error.h"
#include "stl.h"

namespace io {
namespace detail {
    template <typename T>
    struct default_delete {
        constexpr default_delete() noexcept = default;

        template <typename U,
                  typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        default_delete(const default_delete<U>&) noexcept
        {
        }

        void operator()(T* ptr) const
        {
            static_assert(!std::is_void_v<T>,
                          "can't delete pointer to incomplete type");
            static_assert(sizeof(T) > 0,
                          "can't delete pointer to incomplete type");
            delete ptr;
        }
    };

    template <typename T>
    struct default_delete<T[]> {
    public:
        constexpr default_delete() noexcept = default;

        template <typename U,
                  typename =
                      std::enable_if_t<std::is_convertible_v<U (*)[], T (*)[]>>>
        default_delete(const default_delete<U[]>&) noexcept
        {
        }

        template <typename U>
        std::enable_if_t<std::is_convertible_v<U (*)[], T (*)[]>> operator()(
            U* ptr) const
        {
            static_assert(sizeof(T) > 0,
                          "can't delete pointer to incomplete type");
            delete[] ptr;
        }
    };

    template <typename T, typename Deleter = default_delete<T>>
    class maybe_owned_ptr {
    public:
        constexpr maybe_owned_ptr() noexcept = default;
        constexpr maybe_owned_ptr(std::nullptr_t o) noexcept;
        constexpr maybe_owned_ptr(T* o, bool owned) noexcept;
        template <typename D>
        maybe_owned_ptr(T* o, bool owned, D&& deleter);

        constexpr maybe_owned_ptr(const maybe_owned_ptr&) = delete;
        constexpr maybe_owned_ptr(maybe_owned_ptr&& other) noexcept;
        constexpr maybe_owned_ptr& operator=(const maybe_owned_ptr&) = delete;
        constexpr maybe_owned_ptr& operator=(maybe_owned_ptr&& other) noexcept;

        ~maybe_owned_ptr() noexcept;

        constexpr bool has_value() const;
        constexpr operator bool() const;

        constexpr T* value() const;
        constexpr void value(T* val);

        constexpr bool owned() const;
        constexpr void owned(bool val);

    private:
        T* m_obj{nullptr};
        Deleter m_deleter{};
        bool m_owned{false};
    };
}  // namespace detail

using file_wrapper = detail::maybe_owned_ptr<std::FILE, decltype(&std::fclose)>;
file_wrapper make_file_wrapper(std::FILE* f, bool owned) noexcept;

bool is_eof(error c);

template <typename InputIt>
constexpr std::size_t distance_nonneg(InputIt first, InputIt last);

struct characters {
    const std::size_t n;

    constexpr operator std::size_t() const noexcept
    {
        return n;
    }
};
struct elements {
    const std::size_t n;

    constexpr operator std::size_t() const noexcept
    {
        return n;
    }
};
struct bytes {
    const std::size_t n;

    constexpr operator std::size_t() const noexcept
    {
        return n;
    }
};
struct bytes_contiguous {
    const std::size_t n;

    constexpr operator std::size_t() const noexcept
    {
        return n;
    }
};

template <typename T, typename... Ts>
struct contains : std::disjunction<std::is_same<T, Ts>...> {
};

template <typename FloatingT, typename CharT>
FloatingT str_to_floating(const CharT* str, CharT** end);
}  // namespace io

#include "span.h"

#include "util.impl.h"
#if SPIO_HEADER_ONLY
#include "util.cpp"
#endif  // SPIO_HEADER_ONLY

#endif  // SPIO_UTIL_H
