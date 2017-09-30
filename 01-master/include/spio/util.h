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
#include <limits>
#include "config.h"
#include "error.h"
#include "span.h"
#include "stl.h"

namespace io {
namespace detail {
    template <typename T>
    struct default_delete {
        constexpr default_delete() noexcept = default;

        template <
            typename U,
            typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
        default_delete(const default_delete<U>&) noexcept
        {
        }

        void operator()(T* ptr) const
        {
            static_assert(!std::is_void<T>::value,
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
                  typename = std::enable_if_t<
                      std::is_convertible<U (*)[], T (*)[]>::value>>
        default_delete(const default_delete<U[]>&) noexcept
        {
        }

        template <typename U>
        std::enable_if_t<std::is_convertible<U (*)[], T (*)[]>::value>
        operator()(U* ptr) const
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

using quantity_type = span_extent_type;
namespace detail {
#if !SPIO_HAS_IF_CONSTEXPR
    template <bool Signed>
    struct quantity_base_signed {
        constexpr quantity_base_signed(quantity_type n) : m_n(n) {}

        constexpr auto get_signed() const noexcept
        {
            return m_n;
        }

        constexpr auto get_unsigned() const noexcept
        {
            assert(m_n >= 0);
            return static_cast<std::make_unsigned_t<quantity_type>>(m_n);
        }

    protected:
        quantity_type m_n;
    };

    template <>
    struct quantity_base_signed<false> {
        constexpr quantity_base_signed(quantity_type n) : m_n(n) {}

        constexpr auto get_signed() const noexcept
        {
            return static_cast<std::make_signed_t<quantity_type>>(m_n);
        }
        constexpr auto get_unsigned() const noexcept
        {
            return m_n;
        }

    protected:
        quantity_type m_n;
    };
#else
    template <bool>
    struct quantity_base_signed {
        constexpr quantity_base_signed(quantity_type n) : m_n(n) {}

    protected:
        quantity_type m_n;
    };
#endif

    struct quantity_base
        : quantity_base_signed<std::is_signed<quantity_type>::value> {
        using quantity_base_signed<
            std::is_signed<quantity_type>::value>::quantity_base_signed;

        constexpr operator quantity_type() const noexcept
        {
            assert(m_n >= 0);
            return m_n;
        }

        constexpr auto get() const noexcept
        {
            return m_n;
        }

#if SPIO_HAS_IF_CONSTEXPR
        constexpr auto get_signed() const noexcept
        {
            if constexpr (std::is_signed_v<quantity_type>) {
                return m_n;
            }
            else {
                return static_cast<std::make_signed_t<quantity_type>>(m_n);
            }
        }
        constexpr auto get_unsigned() const noexcept
        {
            assert(m_n >= 0);
            if constexpr (std::is_unsigned_v<quantity_type>) {
                return m_n;
            }
            else {
                return static_cast<std::make_unsigned_t<quantity_type>>(m_n);
            }
        }

#endif
    };
}  // namespace detail
struct characters : detail::quantity_base {
    using quantity_base::quantity_base;
};
struct elements : detail::quantity_base {
    using quantity_base::quantity_base;
};
struct bytes : detail::quantity_base {
    using quantity_base::quantity_base;
};
struct bytes_contiguous : detail::quantity_base {
    using quantity_base::quantity_base;
};

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

template <typename FloatingT, typename CharT>
FloatingT str_to_floating(const CharT* str, CharT** end);

template <typename CharT>
constexpr bool is_space(CharT c, span<CharT> spaces = span<CharT>{nullptr});

template <typename CharT>
constexpr bool is_digit(CharT c, int base = 10);

template <typename IntT, typename CharT>
constexpr IntT char_to_int(CharT c, int base = 10);

template <typename CharT, typename IntT>
constexpr void int_to_char(IntT i, span<CharT> result, int base = 10);

template <typename IntT>
constexpr int max_digits() noexcept;
}  // namespace io

#include "util.impl.h"
#if SPIO_HEADER_ONLY
#include "util.cpp"
#endif  // SPIO_HEADER_ONLY

#endif  // SPIO_UTIL_H
