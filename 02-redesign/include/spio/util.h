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
class stdio_filehandle {
    static std::FILE* s_open(const char* filename, const char* mode)
    {
        return std::fopen(filename, mode);
    }
    static std::FILE* s_open(const char* filename, uint32_t mode)
    {
        bool r = mode & READ;
        bool a = mode & APPEND;
        bool e = mode & EXTENDED;
        bool b = mode & BINARY;

        vector<char> str{};
        str.reserve(4);

        if (r) {
            str.push_back('r');
        }
        else if (a) {
            str.push_back('a');
        }
        else {
            str.push_back('w');
        }

        if (b) {
            str.push_back('b');
        }
        if (e) {
            str.push_back('+');
        }
        str.push_back('\0');

        return s_open(filename, &str[0]);
    }

public:
    enum open_mode {
        READ = 1,
        WRITE = 2,
        APPEND = 4,
        EXTENDED = 8,
        BINARY = 16
    };

    stdio_filehandle() = default;
    stdio_filehandle(std::FILE* ptr) : m_handle(ptr) {}
    stdio_filehandle(const char* filename, const char* mode)
        : m_handle(s_open(filename, mode))
    {
    }
    stdio_filehandle(const char* filename, uint32_t mode)
        : m_handle(s_open(filename, mode))
    {
    }


    stdio_filehandle(const stdio_filehandle&) = default;
    stdio_filehandle& operator=(const stdio_filehandle&) = default;
    stdio_filehandle(stdio_filehandle&&) noexcept = default;
    stdio_filehandle& operator=(stdio_filehandle&&) noexcept = default;
    ~stdio_filehandle() noexcept = default;

    bool open(const char* filename, const char* mode)
    {
        assert(!good());
        m_handle = s_open(filename, mode);
        return good();
    }
    bool open(const char* filename, uint32_t mode)
    {
        assert(!good());
        m_handle = s_open(filename, mode);
        return good();
    }

    void close()
    {
        assert(good());
        std::fclose(m_handle);
        m_handle = nullptr;
    }

    constexpr bool good() const
    {
        return m_handle != nullptr;
    }
#if defined(__GNUC__) && __GNUC__ < 7
    operator bool() const
#else
    constexpr operator bool() const
#endif
    {
        return good();
    }

    std::FILE* get() const
    {
        assert(good());
        return m_handle;
    }

    bool error() const
    {
        assert(m_handle);
        return std::ferror(m_handle) != 0;
    }
    void check_error() const
    {
        if (error()) {
            throw std::strerror(errno);
        }
    }
    bool eof() const
    {
        return std::feof(get()) != 0;
    }

    bool flush()
    {
        return std::fflush(get()) == 0;
    }

private:
    std::FILE* m_handle{nullptr};
};

struct owned_stdio_filehandle {
public:
    owned_stdio_filehandle() = default;
    owned_stdio_filehandle(const char* filename, const char* mode)
        : m_file(filename, mode)
    {
    }
    owned_stdio_filehandle(const char* filename, uint32_t mode)
        : m_file(filename, mode)
    {
    }

    owned_stdio_filehandle(const owned_stdio_filehandle&) = delete;
    owned_stdio_filehandle& operator=(const owned_stdio_filehandle&) = delete;
    owned_stdio_filehandle(owned_stdio_filehandle&&) noexcept = default;
    owned_stdio_filehandle& operator=(owned_stdio_filehandle&&) noexcept = default;
    ~owned_stdio_filehandle() noexcept {
        if(m_file) {
            m_file.close();
        }
    }

    bool open(const char* filename, const char* mode) {
        return m_file.open(filename, mode);
    }
    bool open(const char* filename, uint32_t mode) {
        return m_file.open(filename, mode);
    }

    void close() {
        return m_file.close();
    }

#if defined(__GNUC__) && __GNUC__ < 7
    operator bool() const
#else
    constexpr operator bool() const
#endif
    {
        return m_file.operator bool();
    }

    stdio_filehandle* get() {
        return &m_file;
    }

private:
    stdio_filehandle m_file{};
};
/* using stdio_filehandle = std::FILE*; */

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

namespace detail {
    template <typename T, std::size_t N = 0, typename Enable = void>
    struct string_tag : std::false_type {
        using type = T;
        static constexpr auto size = N;
    };

    template <typename T, std::size_t N>
    struct string_tag<
        const T (&)[N],
        N,
        std::enable_if_t<
            contains<std::decay_t<T>, char, wchar_t, char16_t, char32_t>::
                value>> : std::true_type {
        using type = const T (&)[N];
        using pointer = const T*;
        using char_type = std::decay_t<T>;
        static constexpr auto size = N;

        static constexpr pointer make_pointer(type v)
        {
            return &v[0];
        }
    };

    template <typename T>
    struct string_tag<
        const T*,
        0,
        std::enable_if_t<
            contains<std::decay_t<T>, char, wchar_t, char16_t, char32_t>::
                value>> : std::true_type {
        using type = const T*;
        using pointer = type;
        using char_type = std::decay_t<T>;
        static constexpr auto size = 0;

        static constexpr pointer make_pointer(type v)
        {
            return v;
        }
    };

    template <typename T, std::size_t N = 0>
    struct check_string_tag {
        static constexpr auto value = string_tag<T, N>::value;
    };

    template <typename T, std::size_t N>
    struct check_string_tag<const T (&)[N], N> : std::true_type {
    };

    template <typename T, std::size_t N>
    struct check_string_tag<const T*, N> : std::true_type {
    };
}  // namespace detail

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

#endif  // SPIO_UTIL_H
