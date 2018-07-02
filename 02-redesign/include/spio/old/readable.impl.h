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

#include "error.h"
#include "readable.h"

namespace io {
template <typename CharT, typename FileHandle, typename Alloc>
basic_readable_file<CharT, FileHandle, Alloc>::basic_readable_file(
    FileHandle& file)
    : m_file(&file)
{
    if (!file) {
        SPIO_THROW(make_error_code(std::errc::invalid_argument),
                   "basic_readable_file: Invalid file given");
    }
}

template <typename CharT, typename FileHandle, typename Alloc>
template <typename T, extent_t N>
std::error_code basic_readable_file<CharT, FileHandle, Alloc>::read(
    span<T, N> buf)
{
    return write(buf, elements{buf.length()});
}

template <typename CharT, typename FileHandle, typename Alloc>
template <typename T, extent_t N>
std::error_code basic_readable_file<CharT, FileHandle, Alloc>::read(
    span<T, N> buf,
    characters length)
{
    assert(m_file && *m_file);
    SPIO_ASSERT(
        length <= buf.size_bytes() / static_cast<quantity_type>(sizeof(CharT)),
        "buf is not big enough");
    static_assert(sizeof(T) >= sizeof(CharT),
                  "Truncation in basic_readable_file<CharT>::read: sizeof "
                  "buffer is less than CharT");
    static_assert(
        std::is_trivially_copyable<T>::value,
        "basic_readable_file<CharT>::read: T must be TriviallyCopyable");

    std::size_t bytes = 0;
    if (auto e = m_file->read(
            as_writable_bytes(buf).first(
                length * static_cast<quantity_type>(sizeof(CharT))),
            bytes)) {
        if (!is_eof(e)) {
            return e;
        }
    }
    bytes /= sizeof(CharT);
    return get_error(static_cast<quantity_type>(bytes), length);
}

template <typename CharT, typename FileHandle, typename Alloc>
template <typename T, extent_t N>
std::error_code basic_readable_file<CharT, FileHandle, Alloc>::read(
    span<T, N> buf,
    elements length)
{
    return read(buf, characters{length * quantity_type{sizeof(T)} /
                                quantity_type{sizeof(CharT)}});
}

template <typename CharT, typename FileHandle, typename Alloc>
template <typename T, extent_t N>
std::error_code basic_readable_file<CharT, FileHandle, Alloc>::read(
    span<T, N> buf,
    bytes length)
{
    SPIO_ASSERT(length <= buf.size(), "buf is not big enough");
    return read(buf,
                characters{length / static_cast<quantity_type>(sizeof(CharT))});
}

template <typename CharT, typename FileHandle, typename Alloc>
template <typename T, extent_t N>
std::error_code basic_readable_file<CharT, FileHandle, Alloc>::read(
    span<T, N> buf,
    bytes_contiguous length)
{
    assert(m_file && *m_file);
    SPIO_ASSERT(length % sizeof(CharT) == 0,
                "Length is not divisible by sizeof CharT");
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");
    auto char_buf = as_writable_bytes(buf).first(length);
    const auto ret = m_file->read(&char_buf[0], length);
    return get_error(ret, length);
}

template <typename CharT, typename FileHandle, typename Alloc>
std::error_code basic_readable_file<CharT, FileHandle, Alloc>::read(CharT& c)
{
    span<CharT> s{&c, 1};
    return read(s, characters{1});
}

template <typename CharT, typename FileHandle, typename Alloc>
std::error_code basic_readable_file<CharT, FileHandle, Alloc>::skip()
{
    CharT c = 0;
    return read(c);
}

template <typename CharT, typename FileHandle, typename Alloc>
std::error_code basic_readable_file<CharT, FileHandle, Alloc>::seek(
    seek_origin origin,
    seek_type offset)
{
    assert(get_file());
    if (auto e = get_file()->seek(origin, offset)) {
        return e;
    }
    return {};
}

template <typename CharT, typename FileHandle, typename Alloc>
std::error_code basic_readable_file<CharT, FileHandle, Alloc>::tell(
    seek_type& pos)
{
    assert(get_file());
    if (auto e = get_file()->tell(pos)) {
        return e;
    }
    return {};
}

template <typename CharT, typename FileHandle, typename Alloc>
std::error_code basic_readable_file<CharT, FileHandle, Alloc>::get_error(
    quantity_type read_count,
    quantity_type expected) const
{
    assert(m_file && *m_file);
    if (read_count == expected) {
        return {};
    }
    if (auto e = m_file->error()) {
        return e;
    }
    return {};
}

template <typename CharT>
constexpr basic_readable_buffer<CharT>::basic_readable_buffer(
    typename basic_readable_buffer<CharT>::buffer_type buf)
    : m_buffer(buf), m_it(m_buffer.begin())
{
}

template <typename CharT>
template <typename T, extent_t N>
std::error_code basic_readable_buffer<CharT>::read(span<T, N> buf)
{
    return write(buf, elements{buf.length()});
}

template <typename CharT>
template <typename T, extent_t N>
std::error_code basic_readable_buffer<CharT>::read(span<T, N> buf,
                                                   characters length)
{
    SPIO_ASSERT(length <= buf.size(), "buf is not big enough");
    static_assert(sizeof(T) >= sizeof(CharT),
                  "Truncation in basic_readable_buffer<CharT>::read: sizeof "
                  "buffer is less than CharT");
    if (m_it == m_buffer.end()) {
        return make_error_condition(end_of_file);
    }

    const auto dist = std::distance(m_it, m_buffer.end());
    if (dist <= length) {
        const auto add = dist;
        std::copy(m_it, m_it + add, buf.begin());
        std::advance(m_it, add);
        return make_error_condition(end_of_file);
    }
    const auto add = length;
    std::copy(m_it, m_it + add, buf.begin());
    std::advance(m_it, add);
    return {};
}

template <typename CharT>
template <typename T, extent_t N>
std::error_code basic_readable_buffer<CharT>::read(span<T, N> buf,
                                                   elements length)
{
    return read(buf, characters{length * quantity_type{sizeof(T)} /
                                quantity_type{sizeof(CharT)}});
}

template <typename CharT>
template <typename T, extent_t N>
std::error_code basic_readable_buffer<CharT>::read(span<T, N> buf, bytes length)
{
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");
    return read(buf,
                characters{length / static_cast<quantity_type>(sizeof(CharT))});
}

template <typename CharT>
template <typename T, extent_t N>
std::error_code basic_readable_buffer<CharT>::read(span<T, N> buf,
                                                   bytes_contiguous length)
{
    SPIO_ASSERT(length % sizeof(CharT) == 0,
                "Length is not divisible by sizeof CharT");
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");
    if (m_it == m_buffer.end()) {
        return make_error_condition(end_of_file);
    }

    const auto dist = distance(m_it, m_buffer.end());
    if (dist <= length) {
        const auto add = dist / static_cast<quantity_type>(sizeof(CharT));
        auto s = span<CharT>(m_it, m_it + add);
        copy_contiguous(s, buf);
        advance(m_it, add);
        return make_error_condition(end_of_file);
    }
    const auto add = length / static_cast<quantity_type>(sizeof(CharT));
    auto s = span<CharT>(m_it, m_it + add);
    copy_contiguous(s, buf);
    advance(m_it, add);
    return {};
}

template <typename CharT>
std::error_code basic_readable_buffer<CharT>::read(CharT& c)
{
    span<CharT> s{&c, 1};
    return read(s, characters{1});
}

template <typename CharT>
std::error_code basic_readable_buffer<CharT>::skip()
{
    CharT c = 0;
    return read(c);
}

template <typename CharT>
std::error_code basic_readable_buffer<CharT>::rewind(
    typename buffer_type::difference_type steps)
{
    const auto dist = std::distance(m_buffer.begin(), m_it);
    if (dist > steps) {
        return make_error_code(std::errc::invalid_argument);
    }
    m_it -= steps;
    return {};
}

template <typename CharT>
std::error_code basic_readable_buffer<CharT>::seek(seek_origin origin,
                                                   seek_type offset)
{
    if (origin == seek_origin::SET) {
        if (m_buffer.size() < offset) {
            return make_error_code(std::errc::invalid_argument);
        }
        m_it = m_buffer.begin() + offset;
        return {};
    }
    if (origin == seek_origin::CUR) {
        if (offset == 0) {
            return {};
        }
        if (offset > 0) {
            auto diff = std::distance(m_it, m_buffer.end());
            if (offset > diff) {
                return make_error_code(std::errc::invalid_argument);
            }
            m_it += offset;
            return {};
        }
        auto diff = std::distance(m_it, m_buffer.begin());
        if (offset < diff) {
            return make_error_code(std::errc::invalid_argument);
        }
        m_it += offset;
        return {};
    }
    if (offset > 0) {
        return make_error_code(std::errc::invalid_argument);
    }
    m_it = m_buffer.end() + offset;
    return {};
}

template <typename CharT>
std::error_code basic_readable_buffer<CharT>::tell(seek_type& pos)
{
    pos = static_cast<seek_type>(std::distance(m_buffer.begin(), m_it));
    return {};
}
}  // namespace io
