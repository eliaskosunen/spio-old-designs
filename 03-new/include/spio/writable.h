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

#ifndef SPIO_WRITABLE_H
#define SPIO_WRITABLE_H

#include <cstdio>
#include "config.h"
#include "error.h"
#include "stl.h"
#include "util.h"

namespace io {
class file_buffering {
public:
    enum mode_type {
        BUFFER_FULL = _IOFBF,
        BUFFER_LINE = _IOLBF,
        BUFFER_NONE = _IONBF
    };

    file_buffering() = default;
    file_buffering(bool use_buffer, mode_type m, std::size_t len = BUFSIZ);

    constexpr bool use() const
    {
        return m_use;
    }
    vector<char>& get_buffer()
    {
        return m_buffer;
    }

    error set(file_wrapper& file);

    static file_buffering disable();
    static file_buffering full(std::size_t len = BUFSIZ, bool external = false);
    static file_buffering line(std::size_t len = BUFSIZ, bool external = false);

private:
    static vector<char> _initialize_buffer(bool use, std::size_t len);

    vector<char> m_buffer{};
    std::size_t m_length{BUFSIZ};
    mode_type m_mode{BUFFER_FULL};
    bool m_use{false};
};

template <typename CharT>
class basic_writable_base {
public:
    using value_type = CharT;
};

#ifndef SPIO_FWRITE
#define SPIO_FWRITE ::std::fwrite
#endif

template <typename CharT>
class basic_writable_file : public basic_writable_base<CharT> {
public:
    static_assert(
        std::is_trivially_copyable<CharT>::value,
        "basic_writable_file<CharT>: CharT must be TriviallyCopyable");

    basic_writable_file() = default;
    /*implicit*/ basic_writable_file(
        file_wrapper file,
        file_buffering&& buffering = file_buffering{});
    basic_writable_file(const char* filename,
                        bool append,
                        file_buffering&& buffering = file_buffering{});

    template <typename T>
    error write(span<T> buf, characters length);
    template <typename T>
    error write(span<T> buf, elements length);
    template <typename T>
    error write(span<T> buf, bytes length);
    template <typename T>
    error write(span<T> buf, bytes_contiguous length);
    error write(CharT* c);

    error flush() noexcept;

    constexpr bool is_valid() const
    {
        return valid;
    }

private:
    error get_error(quantity_type read_count, quantity_type expected) const;

    file_wrapper m_file{};
    file_buffering m_buffering{file_buffering{}};
    bool valid{false};
};

template <typename T>
struct dynamic_writable_buffer : public vector<T> {
    using vector<T>::vector;

    constexpr bool is_end()
    {
        return false;
    }
};
template <typename T, std::size_t N>
struct static_writable_buffer {
public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = value_type&;
    using const_reference = std::add_const_t<reference>;

    constexpr static_writable_buffer() : m_it(m_buf.begin()) {}

    constexpr reference operator[](std::size_t i) noexcept
    {
        return m_buf[i];
    }
    constexpr const_reference operator[](std::size_t i) const noexcept
    {
        return m_buf[i];
    }

    constexpr auto data()
    {
        return m_buf.data();
    }

    constexpr size_type size() const
    {
        return distance_nonneg(m_buf.begin(), m_it);
    }
    constexpr size_type max_size() const
    {
        return m_buf.size();
    }

    constexpr auto begin()
    {
        return m_buf.begin();
    }
    constexpr auto begin() const
    {
        return m_buf.begin();
    }

    constexpr auto end()
    {
        return m_buf.end();
    }
    constexpr auto end() const
    {
        return m_buf.end();
    }

    constexpr bool is_end() const
    {
        return m_it == m_buf.end();
    }

    constexpr void push_back(const T& value)
    {
        if (is_end()) {
            return;
        }
        *m_it = value;
        ++m_it;
    }
    constexpr void push_back(T&& value)
    {
        if (is_end()) {
            return;
        }
        *m_it = std::move(value);
        ++m_it;
    }

private:
    array<T, N> m_buf{};
    typename decltype(m_buf)::iterator m_it{};
};

template <typename CharT, typename BufferT = dynamic_writable_buffer<CharT>>
class basic_writable_buffer : public basic_writable_base<CharT> {
public:
    using buffer_type = BufferT;

    constexpr basic_writable_buffer(buffer_type b = buffer_type{});

    template <typename T>
    error write(span<T> buf, characters length);
    template <typename T>
    error write(span<T> buf, elements length);
    template <typename T>
    error write(span<T> buf, bytes length);
    template <typename T>
    error write(span<T> buf, bytes_contiguous length);
    error write(CharT* c);

    error flush() noexcept
    {
        return {};
    }

    constexpr buffer_type& get_buffer()
    {
        return m_buffer;
    }
    constexpr const buffer_type& get_buffer() const
    {
        return m_buffer;
    }
    template <typename T = buffer_type>
    constexpr std::enable_if_t<std::is_copy_constructible<T>::value, T>
    consume_buffer() const
    {
        return T{m_buffer};
    }
    template <typename T = buffer_type>
    constexpr std::enable_if_t<!std::is_copy_constructible<T>::value &&
                                   std::is_move_constructible<T>::value,
                               T&&>
    consume_buffer()
    {
        return std::move(m_buffer);
    }

    constexpr bool is_valid() const
    {
        return valid;
    }

private:
    buffer_type m_buffer{};
    bool valid{true};
};

using writable_file = basic_writable_file<char>;
using writable_wfile = basic_writable_file<wchar_t>;
using writable_file16 = basic_writable_file<char16_t>;
using writable_file32 = basic_writable_file<char32_t>;
using writable_ufile = basic_writable_file<unsigned char>;

using writable_buffer = basic_writable_buffer<char>;
using writable_wbuffer = basic_writable_buffer<wchar_t>;
using writable_buffer16 = basic_writable_buffer<char16_t>;
using writable_buffer32 = basic_writable_buffer<char32_t>;

using writable_ubuffer = basic_writable_buffer<unsigned char>;

template <typename CharT, std::size_t N>
using basic_writable_static_buffer =
    basic_writable_buffer<CharT, static_writable_buffer<CharT, N>>;
}  // namespace io

#include "writable.impl.h"
#if SPIO_HEADER_ONLY
#include "writable.cpp"
#endif

#endif  // SPIO_WRITABLE_H
