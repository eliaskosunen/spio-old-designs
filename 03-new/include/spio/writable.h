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

#ifndef SPIO_WRITABLE_H
#define SPIO_WRITABLE_H

#include <cstdio>
#include "config.h"
#include "error.h"
#include "filehandle.h"
#include "stl.h"
#include "util.h"

namespace io {
#ifdef _MSC_VER
template <typename T>
struct is_writable : std::true_type {
};
#else
template <typename T, typename = void>
struct is_writable : std::false_type {
};
template <typename T>
struct is_writable<T,
                   void_t<typename T::value_type,
                          decltype(std::declval<T>().write(
                              std::declval<span<char, dynamic_extent>>())),
                          decltype(std::declval<T>().write(
                              std::declval<span<char, dynamic_extent>>(),
                              std::declval<characters>())),
                          decltype(std::declval<T>().write(
                              std::declval<span<char, dynamic_extent>>(),
                              std::declval<elements>())),
                          decltype(std::declval<T>().write(
                              std::declval<span<char, dynamic_extent>>(),
                              std::declval<bytes>())),
                          decltype(std::declval<T>().write(
                              std::declval<span<char, dynamic_extent>>(),
                              std::declval<bytes_contiguous>())),
                          decltype(std::declval<T>().write(
                              std::declval<typename T::value_type>())),
                          decltype(std::declval<T>().flush())>>
    : std::true_type {
};
#endif

template <typename CharT, typename FileHandle = filehandle>
class basic_writable_file {
public:
    using value_type = CharT;

    static_assert(
        std::is_trivially_copyable<CharT>::value,
        "basic_writable_file<CharT>: CharT must be TriviallyCopyable");
    static_assert(is_filehandle<FileHandle>::value,
                  "basic_writable_file<CharT, T>: T does not satisfy the "
                  "requirements of FileHandle");

    constexpr basic_writable_file() = default;
    explicit basic_writable_file(FileHandle& file);

    constexpr basic_writable_file(const basic_writable_file&) = delete;
    constexpr basic_writable_file& operator=(const basic_writable_file&) = delete;
    constexpr basic_writable_file(basic_writable_file&&) = default;
    constexpr basic_writable_file& operator=(basic_writable_file&&) = default;
    ~basic_writable_file() noexcept
    {
        _flush_destruct();
    }

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

    FileHandle* get_file()
    {
        return m_file;
    }
    const FileHandle* get_file() const
    {
        return m_file;
    }

private:
    error get_error(quantity_type read_count, quantity_type expected) const;

    void _flush_destruct() noexcept
    {
        if (m_file && *m_file) {
            m_file->flush();
        }
    }

    FileHandle* m_file{};
};

static_assert(
    is_writable<basic_writable_file<char>>::value,
    "basic_writable_file<char> does not satisfy the requirements of Writable");
static_assert(is_writable<basic_writable_file<wchar_t>>::value,
              "basic_writable_file<wchar_t> does not satisfy the requirements "
              "of Writable");

#ifdef _MSC_VER
template <typename T>
struct is_writable_buffer_type : std::true_type {
};
#else
template <typename T, typename = void>
struct is_writable_buffer_type : std::false_type {
};
template <typename T>
struct is_writable_buffer_type<
    T,
    void_t<typename T::value_type,
           typename T::size_type,
           typename T::difference_type,
           typename T::reference,
           typename T::const_reference,
           decltype(std::declval<T>()[std::declval<typename T::size_type>()]),
           decltype(std::declval<T>().data()),
           decltype(std::declval<T>().size()),
           decltype(std::declval<T>().max_size()),
           decltype(std::declval<T>().begin()),
           decltype(std::declval<T>().end()),
           decltype(std::declval<T>().is_end()),
           decltype(std::declval<T>().push_back(
               std::declval<const typename T::value_type&>())),
           decltype(std::declval<T>().push_back(
               std::declval<typename T::value_type&&>())),
           decltype(std::declval<T>().resize(
               std::declval<typename T::size_type>()))>> : std::true_type {
};
#endif

template <typename T, typename Alloc = stl::allocator<T>>
struct dynamic_writable_buffer : public stl::vector<T, Alloc> {
    using stl::vector<T, Alloc>::vector;

    constexpr bool is_end()
    {
        return false;
    }
};
template <typename T, span_extent_type Extent = dynamic_extent>
class span_writable_buffer {
public:
    using span_type = span<T, Extent>;
    using value_type = typename span_type::value_type;
    using size_type = typename span_type::index_type_us;
    using index_type = typename span_type::index_type;
    using difference_type = typename span_type::difference_type;
    using reference = value_type&;
    using const_reference = std::add_const_t<reference>;

    constexpr span_writable_buffer(span<T, Extent> s)
        : m_buf(std::move(s)), m_it(m_buf.begin())
    {
    }

    constexpr reference operator[](size_type i) noexcept
    {
        return m_buf[static_cast<index_type>(i)];
    }
    constexpr const_reference operator[](size_type i) const noexcept
    {
        return m_buf[static_cast<index_type>(i)];
    }

    constexpr auto data()
    {
        return m_buf.data();
    }

    constexpr size_type size() const
    {
        return static_cast<size_type>(stl::distance(m_buf.begin(), m_it));
    }
    constexpr size_type max_size() const
    {
        return m_buf.size_us();
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

    constexpr void resize(size_type size)
    {
        SPIO_UNUSED(size);
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

static_assert(is_writable_buffer_type<dynamic_writable_buffer<char>>::value,
              "dynamic_writable_buffer<char> does not satisfy the requirements "
              "of WritableBufferType");
static_assert(is_writable_buffer_type<span_writable_buffer<char>>::value,
              "span_writable_buffer<char> does not satisfy the requirements "
              "of WritableBufferType");
static_assert(
    is_writable_buffer_type<static_writable_buffer<char, 64>>::value,
    "static_writable_buffer<char, 64> does not satisfy the requirements "
    "of WritableBufferType");

template <typename CharT, typename BufferT = dynamic_writable_buffer<CharT>>
class basic_writable_buffer {
public:
    using value_type = CharT;
    using buffer_type = BufferT;

    constexpr basic_writable_buffer() = default;
    constexpr explicit basic_writable_buffer(buffer_type b);

    constexpr basic_writable_buffer(const basic_writable_buffer&) = delete;
    constexpr basic_writable_buffer& operator=(const basic_writable_buffer&) = delete;
    constexpr basic_writable_buffer(basic_writable_buffer&&) = default;
    constexpr basic_writable_buffer& operator=(basic_writable_buffer&&) = default;
    ~basic_writable_buffer() noexcept = default;

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

static_assert(is_writable<basic_writable_buffer<char>>::value,
              "basic_writable_buffer<char> does not satisfy the requirements "
              "of Writable");
static_assert(is_writable<basic_writable_buffer<wchar_t>>::value,
              "basic_writable_buffer<wchar_t> does not satisfy the "
              "requirements of Writable");

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
