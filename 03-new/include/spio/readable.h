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

#ifndef SPIO_READABLE_H
#define SPIO_READABLE_H

#include "config.h"
#include "error.h"
#include "filehandle.h"
#include "util.h"

namespace io {
#ifdef _MSC_VER
template <typename T>
struct is_readable : std::true_type {
};
#else
template <typename T, typename = void>
struct is_readable : std::false_type {
};
template <typename T>
struct is_readable<T,
                   void_t<typename T::value_type,
                          decltype(std::declval<T>().read(
                              std::declval<span<char, dynamic_extent>>())),
                          decltype(std::declval<T>().read(
                              std::declval<span<char, dynamic_extent>>(),
                              std::declval<characters>())),
                          decltype(std::declval<T>().read(
                              std::declval<span<char, dynamic_extent>>(),
                              std::declval<elements>())),
                          decltype(std::declval<T>().read(
                              std::declval<span<char, dynamic_extent>>(),
                              std::declval<bytes>())),
                          decltype(std::declval<T>().read(
                              std::declval<span<char, dynamic_extent>>(),
                              std::declval<bytes_contiguous>())),
                          decltype(std::declval<T>().read(
                              std::declval<typename T::value_type&>())),
                          decltype(std::declval<T>().skip()),
                          decltype(std::declval<T>().is_overreadable())>>
    : std::true_type {
};
#endif

template <typename CharT,
          typename FileHandle = filehandle,
          typename Alloc = stl::allocator<CharT>>
class basic_readable_file {
public:
    using value_type = CharT;
    using allocator_type = Alloc;

    static constexpr bool is_trivially_rewindable = false;

    static_assert(
        std::is_trivially_copyable<CharT>::value,
        "basic_readable_file<CharT>: CharT must be TriviallyCopyable");
    static_assert(is_filehandle<FileHandle>::value,
                  "basic_readable_file<CharT, T>: T does not satisfy the "
                  "requirements of FileHandle");

    constexpr basic_readable_file() = default;
    explicit basic_readable_file(FileHandle& file);

    constexpr basic_readable_file(const basic_readable_file&) = delete;
    constexpr basic_readable_file& operator=(const basic_readable_file&) =
        delete;
    constexpr basic_readable_file(basic_readable_file&&) noexcept = default;
    constexpr basic_readable_file& operator=(basic_readable_file&&) noexcept =
        default;
    ~basic_readable_file() noexcept = default;

    template <typename T, extent_t N>
    error read(span<T, N> buf);
    template <typename T, extent_t N>
    error read(span<T, N> buf, characters length);
    template <typename T, extent_t N>
    error read(span<T, N> buf, elements length);
    template <typename T, extent_t N>
    error read(span<T, N> buf, bytes length);
    template <typename T, extent_t N>
    error read(span<T, N> buf, bytes_contiguous length);
    error read(CharT& c);

    error skip();

    error seek(seek_origin origin, seek_type offset);
    error tell(seek_type& pos);

    bool is_overreadable() const
    {
        assert(get_file());
        return !get_file()->is_stdin();
    }

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

    FileHandle* m_file{};
};

static_assert(
    is_readable<basic_readable_file<char>>::value,
    "basic_readable_file<char> does not satisfy the requirements of Readable");
static_assert(is_readable<basic_readable_file<wchar_t>>::value,
              "basic_readable_file<wchar_t> does not satisfy the requirements "
              "of Readable");

template <typename CharT>
class basic_readable_buffer {
public:
    using value_type = CharT;
    static constexpr const extent_t BufferExtent = dynamic_extent;
    using buffer_type = span<CharT, BufferExtent>;
    static constexpr bool is_trivially_rewindable = true;

    constexpr basic_readable_buffer() = default;
    explicit constexpr basic_readable_buffer(buffer_type buf);

    constexpr basic_readable_buffer(const basic_readable_buffer&) = delete;
    constexpr basic_readable_buffer& operator=(const basic_readable_buffer&) =
        delete;
    constexpr basic_readable_buffer(basic_readable_buffer&& other) noexcept
    {
        *this = std::move(other);
    }
    constexpr basic_readable_buffer& operator=(
        basic_readable_buffer&& other) noexcept
    {
        auto n = stl::distance(other.m_buffer.begin(), other.m_it);
        m_buffer = std::move(other.m_buffer);
        m_it = m_buffer.begin();
        stl::advance(m_it, n);
        return *this;
    }
    ~basic_readable_buffer() noexcept = default;

    template <typename T, extent_t N>
    error read(span<T, N> buf);
    template <typename T, extent_t N>
    error read(span<T, N> buf, characters length);
    template <typename T, extent_t N>
    error read(span<T, N> buf, elements length);
    template <typename T, extent_t N>
    error read(span<T, N> buf, bytes length);
    template <typename T, extent_t N>
    error read(span<T, N> buf, bytes_contiguous length);
    error read(CharT& c);

    error skip();

    error rewind(typename buffer_type::difference_type steps = 1);

    error seek(seek_origin origin, seek_type offset);
    error tell(seek_type& pos);

    bool is_overreadable() const
    {
        return true;
    }

    buffer_type& get_buffer()
    {
        return m_buffer;
    }
    const buffer_type& get_buffer() const
    {
        return m_buffer;
    }

    auto& get_iterator()
    {
        return m_it;
    }
    const auto& get_iterator() const
    {
        return m_it;
    }

private:
    buffer_type m_buffer{};
    typename buffer_type::iterator m_it{m_buffer.begin()};
};

static_assert(is_readable<basic_readable_buffer<char>>::value,
              "basic_readable_buffer<char> does not satisfy the requirements "
              "of Readable");
static_assert(
    is_readable<basic_readable_buffer<wchar_t>>::value,
    "basic_readable_buffer<wchar_t> does not satisfy the requirements "
    "of Readable");

template <typename CharT>
using basic_readable_stdio_file = basic_readable_file<CharT, stdio_filehandle>;
template <typename CharT>
using basic_readable_native_file =
    basic_readable_file<CharT, native_filehandle>;

using readable_file = basic_readable_file<char>;
using readable_wfile = basic_readable_file<wchar_t>;
using readable_file16 = basic_readable_file<char16_t>;
using readable_file32 = basic_readable_file<char32_t>;
using readable_ufile = basic_readable_file<unsigned char>;

using readable_buffer = basic_readable_buffer<char>;
using readable_wbuffer = basic_readable_buffer<wchar_t>;
using readable_buffer16 = basic_readable_buffer<char16_t>;
using readable_buffer32 = basic_readable_buffer<char32_t>;
using readable_ubuffer = basic_readable_buffer<unsigned char>;
}  // namespace io

#include "readable.impl.h"

#endif  // SPIO_READABLE_H
