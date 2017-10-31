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
#include "filehandle.h"
#include "stl.h"
#include "util.h"

namespace io {
template <typename ImplT>
class basic_writable_base {
public:
    using implementation_type = ImplT;

#define THIS (static_cast<ImplT*>(this))

    template <typename T, span_extent_type N>
    error write(span<T, N> buf)
    {
        return THIS->write(std::move(buf));
    }
    template <typename T, span_extent_type N>
    error write(span<T, N> buf, characters length)
    {
        return THIS->write(std::move(buf), length);
    }
    template <typename T, span_extent_type N>
    error write(span<T, N> buf, elements length)
    {
        return THIS->write(std::move(buf), length);
    }
    template <typename T, span_extent_type N>
    error write(span<T, N> buf, bytes length)
    {
        return THIS->write(std::move(buf), length);
    }
    template <typename T, span_extent_type N>
    error write(span<T, N> buf, bytes_contiguous length)
    {
        return THIS->write(std::move(buf), length);
    }

    error flush() noexcept
    {
        return THIS->flush();
    }

#undef THIS
};

template <typename CharT, typename FileHandle = filehandle>
class basic_writable_file
    : public basic_writable_base<basic_writable_file<CharT>> {
public:
    using value_type = CharT;

    static_assert(
        std::is_trivially_copyable<CharT>::value,
        "basic_writable_file<CharT>: CharT must be TriviallyCopyable");

    basic_writable_file() = default;
    /*implicit*/ basic_writable_file(FileHandle file);

    template <typename T, span_extent_type N>
    error write(span<T, N> buf);
    template <typename T, span_extent_type N>
    error write(span<T, N> buf, characters length);
    template <typename T, span_extent_type N>
    error write(span<T, N> buf, elements length);
    template <typename T, span_extent_type N>
    error write(span<T, N> buf, bytes length);
    template <typename T, span_extent_type N>
    error write(span<T, N> buf, bytes_contiguous length);
    error write(CharT c);

    error flush() noexcept;

    FileHandle& get_file()
    {
        return m_file;
    }
    const FileHandle& get_file() const
    {
        return m_file;
    }

private:
    error get_error(quantity_type read_count, quantity_type expected) const;

    FileHandle m_file{};
};

template <typename T>
struct dynamic_writable_buffer : public stl::vector<T> {
    using stl::vector<T>::vector;

    constexpr bool is_end()
    {
        return false;
    }
};
template <typename T, span_extent_type Extent>
class span_writable_buffer {
public:
    using span_type = span<T, Extent>;
    using value_type = typename span_type::value_type;
    using size_type = typename span_type::index_type;
    using difference_type = typename span_type::difference_type;
    using reference = value_type&;
    using const_reference = std::add_const_t<reference>;

    constexpr span_writable_buffer(span<T, Extent> s)
        : m_buf(std::move(s)), m_it(m_buf.begin())
    {
    }

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
        return distance(m_buf.begin(), m_it);
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

    constexpr void resize(std::size_t size)
    {
        if (size > max_size()) {
            return;
        }
    }

private:
    span<T, Extent> m_buf{};
    typename span<T, Extent>::iterator m_it{};
};
template <typename T,
          std::size_t N,
          span_extent_type Extent = static_cast<span_extent_type>(N)>
class static_writable_buffer : public span_writable_buffer<T, Extent> {
public:
    static_writable_buffer(stl::array<T, N> a = {})
        : m_buf(std::move(a)), span_writable_buffer<T, Extent>(m_buf)
    {
    }

private:
    stl::array<T, N> m_buf{};
};

template <typename CharT, typename BufferT = dynamic_writable_buffer<CharT>>
class basic_writable_buffer
    : public basic_writable_base<basic_writable_buffer<CharT>> {
public:
    using value_type = CharT;
    using buffer_type = BufferT;

    constexpr basic_writable_buffer(buffer_type b = buffer_type{});

    template <typename T, span_extent_type N>
    error write(span<T, N> buf);
    template <typename T, span_extent_type N>
    error write(span<T, N> buf, characters length);
    template <typename T, span_extent_type N>
    error write(span<T, N> buf, elements length);
    template <typename T, span_extent_type N>
    error write(span<T, N> buf, bytes length);
    template <typename T, span_extent_type N>
    error write(span<T, N> buf, bytes_contiguous length);
    error write(CharT c);

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

private:
    buffer_type m_buffer{};
};

template <typename CharT>
using basic_writable_native_file = basic_writable_file<char, native_filehandle>;
template <typename CharT>
using basic_writable_stdio_file = basic_writable_file<char, stdio_filehandle>;

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

#endif  // SPIO_WRITABLE_H
