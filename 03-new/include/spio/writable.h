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

    static constexpr bool is_readable_convertible = false;

    static_assert(
        std::is_trivially_copyable<CharT>::value,
        "basic_writable_file<CharT>: CharT must be TriviallyCopyable");
    static_assert(is_filehandle<FileHandle>::value,
                  "basic_writable_file<CharT, T>: T does not satisfy the "
                  "requirements of FileHandle");

    constexpr basic_writable_file() = default;
    explicit basic_writable_file(FileHandle& file);

    constexpr basic_writable_file(const basic_writable_file&) = delete;
    constexpr basic_writable_file& operator=(const basic_writable_file&) =
        delete;
    constexpr basic_writable_file(basic_writable_file&&) = default;
    constexpr basic_writable_file& operator=(basic_writable_file&&) = default;
    ~basic_writable_file() noexcept
    {
        _flush_destruct();
    }

    template <typename T, extent_t N>
    std::error_code write(span<T, N> buf);
    template <typename T, extent_t N>
    std::error_code write(span<T, N> buf, characters length);
    template <typename T, extent_t N>
    std::error_code write(span<T, N> buf, elements length);
    template <typename T, extent_t N>
    std::error_code write(span<T, N> buf, bytes length);
    template <typename T, extent_t N>
    std::error_code write(span<T, N> buf, bytes_contiguous length);
    std::error_code write(CharT c);

    std::error_code flush() noexcept;

    std::error_code seek(seek_origin origin, seek_type offset);
    std::error_code tell(seek_type& pos);

    FileHandle* get_file()
    {
        return m_file;
    }
    const FileHandle* get_file() const
    {
        return m_file;
    }

private:
    std::error_code get_error(quantity_type read_count,
                              quantity_type expected) const;

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

template <typename T, typename Alloc = std::allocator<T>>
struct dynamic_writable_buffer : public std::vector<T, Alloc> {
    static constexpr auto extent = dynamic_extent;

    using std::vector<T, Alloc>::vector;

    constexpr bool is_end()
    {
        return false;
    }

    constexpr auto to_span()
    {
        return span<T, extent>{this->begin(), this->end()};
    }
};
template <typename T, extent_t Extent = dynamic_extent>
class span_writable_buffer {
public:
    using span_type = span<T, Extent>;
    using value_type = typename span_type::value_type;
    using size_type = typename span_type::index_type_us;
    using index_type = typename span_type::index_type;
    using difference_type = typename span_type::difference_type;
    using reference = value_type&;
    using const_reference = std::add_const_t<reference>;
    using iterator = typename span_type::iterator;
    using const_iterator = typename span_type::const_iterator;

    static constexpr auto extent = Extent;

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
        return static_cast<size_type>(std::distance(
            m_buf.begin(),
            static_cast<typename span_type::const_iterator>(m_it)));
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

	template <typename InputIt>
	constexpr iterator insert(const_iterator pos, InputIt first, InputIt last)
	{
		if (pos == m_it)
		{
			for (auto it = first; it != last; ++it)
			{
				if (is_end()) {
					break;
				}
				*m_it = std::move(*it);
				++m_it;
			}
			return m_it;
		}

		std::array<T, 256> tail;
		auto dist = std::distance(m_it, m_buf.end());
		std::copy(m_it, m_buf.end(), tail.begin());
		m_it = std::copy(first, last, m_it);
        return insert(const_iterator{m_it}, tail.begin(), tail.begin() + dist);
    }

    constexpr auto to_span()
    {
        return span<T, extent>{begin(), end()};
    }

private:
    span_type m_buf{};
    iterator m_it{};
};
template <typename T, std::size_t N, extent_t Extent = static_cast<extent_t>(N)>
class static_writable_buffer : public span_writable_buffer<T, Extent> {
public:
    static constexpr auto extent = Extent;

    static_writable_buffer(std::array<T, N> a = {})
        : m_buf(std::move(a)), span_writable_buffer<T, Extent>(m_buf)
    {
    }

    constexpr auto to_span()
    {
        return span<T, extent>{this->begin(), this->end()};
    }

private:
    std::array<T, N> m_buf{};
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

    static constexpr bool is_readable_convertible = true;

    constexpr basic_writable_buffer() = default;
    constexpr explicit basic_writable_buffer(buffer_type b);

    constexpr basic_writable_buffer(const basic_writable_buffer&) = delete;
    constexpr basic_writable_buffer& operator=(const basic_writable_buffer&) =
        delete;
    constexpr basic_writable_buffer(basic_writable_buffer&&) = default;
    constexpr basic_writable_buffer& operator=(basic_writable_buffer&&) =
        default;
    ~basic_writable_buffer() noexcept = default;

    template <typename T, extent_t N>
    std::error_code write(span<T, N> buf);
    template <typename T, extent_t N>
    std::error_code write(span<T, N> buf, characters length);
    template <typename T, extent_t N>
    std::error_code write(span<T, N> buf, elements length);
    template <typename T, extent_t N>
    std::error_code write(span<T, N> buf, bytes length);
    template <typename T, extent_t N>
    std::error_code write(span<T, N> buf, bytes_contiguous length);
    std::error_code write(CharT c);

    std::error_code flush() noexcept
    {
        return {};
    }

    std::error_code seek(seek_origin origin, seek_type offset);
    std::error_code tell(seek_type& pos);

    constexpr buffer_type& get_buffer()
    {
        return m_buffer;
    }
    constexpr const buffer_type& get_buffer() const
    {
        return m_buffer;
    }
    constexpr buffer_type&& consume_buffer()
    {
        return std::move(m_buffer);
    }

    constexpr auto to_readable();

private:
    buffer_type m_buffer{};
    typename buffer_type::iterator m_it{m_buffer.begin()};
};

static_assert(is_writable<basic_writable_buffer<char>>::value,
              "basic_writable_buffer<char> does not satisfy the requirements "
              "of Writable");
static_assert(is_writable<basic_writable_buffer<wchar_t>>::value,
              "basic_writable_buffer<wchar_t> does not satisfy the "
              "requirements of Writable");

template <typename CharT>
using basic_writable_stdio_file = basic_writable_file<char, stdio_filehandle>;
#if SPIO_HAS_NATIVE_FILEIO
template <typename CharT>
using basic_writable_native_file = basic_writable_file<char, native_filehandle>;
#endif

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
