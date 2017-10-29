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

#include "error.h"
#include "readable.h"

namespace io {
template <typename CharT, typename FileHandle>
basic_readable_file<CharT, FileHandle>::basic_readable_file(FileHandle* file)
    : m_file(file)
{
    if (!m_file) {
        SPIO_THROW(invalid_argument, "Nullptr file given");
    }
}

template <typename CharT, typename FileHandle>
template <typename T, span_extent_type N>
error basic_readable_file<CharT, FileHandle>::read(span<T, N> buf)
{
    return write(buf, elements{buf.length()});
}

template <typename CharT, typename FileHandle>
template <typename T, span_extent_type N>
error basic_readable_file<CharT, FileHandle>::read(span<T, N> buf,
                                                   characters length)
{
    assert(m_file);
    SPIO_ASSERT(
        length <= buf.size_bytes() / static_cast<quantity_type>(sizeof(CharT)),
        "buf is not big enough");
    static_assert(sizeof(T) >= sizeof(CharT),
                  "Truncation in basic_readable_file<CharT>::read: sizeof "
                  "buffer is less than CharT");
    static_assert(
        std::is_trivially_copyable<T>::value,
        "basic_readable_file<CharT>::read: T must be TriviallyCopyable");

    const auto ret = [&]() {
#if SPIO_HAS_IF_CONSTEXPR
        if constexpr (sizeof(CharT) == 1) {
#else
        if (sizeof(CharT) == 1) {
#endif
            return m_file->read(&buf[0], length.get_unsigned());
        }
        else {
            stl::vector<char> char_buf(length.get_unsigned() * sizeof(CharT),
                                       0);
            const auto r = m_file->read(&char_buf[0],
                                        length.get_unsigned() * sizeof(CharT));
            for (auto i = 0; i < length; ++i) {
                buf[i] = *reinterpret_cast<T*>(
                    &char_buf[static_cast<std::size_t>(i) * sizeof(CharT)]);
            }
            return r / sizeof(CharT);
        }
    }();
    return get_error(static_cast<quantity_type>(ret), length);
}

template <typename CharT, typename FileHandle>
template <typename T, span_extent_type N>
error basic_readable_file<CharT, FileHandle>::read(span<T, N> buf,
                                                   elements length)
{
    return read(buf, characters{length * quantity_type{sizeof(T)} /
                                quantity_type{sizeof(CharT)}});
}

template <typename CharT, typename FileHandle>
template <typename T, span_extent_type N>
error basic_readable_file<CharT, FileHandle>::read(span<T, N> buf, bytes length)
{
    SPIO_ASSERT(length <= buf.size(), "buf is not big enough");
    return read(buf,
                characters{length / static_cast<quantity_type>(sizeof(CharT))});
}

template <typename CharT, typename FileHandle>
template <typename T, span_extent_type N>
error basic_readable_file<CharT, FileHandle>::read(span<T, N> buf,
                                                   bytes_contiguous length)
{
    assert(m_file);
    SPIO_ASSERT(length % sizeof(CharT) == 0,
                "Length is not divisible by sizeof CharT");
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");
    auto char_buf = as_writable_bytes(buf).first(length);
    const auto ret = m_file->read(&char_buf[0], length);
    return get_error(ret, length);
}

template <typename CharT, typename FileHandle>
error basic_readable_file<CharT, FileHandle>::read(CharT& c)
{
    span<CharT> s{&c, 1};
    return read(s, characters{1});
}

template <typename CharT, typename FileHandle>
error basic_readable_file<CharT, FileHandle>::skip()
{
    CharT c = 0;
    return read(c);
}

template <typename CharT, typename FileHandle>
error basic_readable_file<CharT, FileHandle>::get_error(
    quantity_type read_count,
    quantity_type expected) const
{
    assert(m_file);
    if (read_count == expected) {
        return {};
    }
    if (m_file->error()) {
        return io_error;
    }
    if (m_file->eof()) {
        return end_of_file;
    }
    return default_error;
}

template <typename CharT, span_extent_type BufferExtent>
basic_readable_buffer<CharT, BufferExtent>::basic_readable_buffer(
    span<CharT, BufferExtent> buf)
    : m_buffer(buf), m_it(m_buffer.begin())
{
}

template <typename CharT, span_extent_type BufferExtent>
template <typename T, span_extent_type N>
error basic_readable_buffer<CharT, BufferExtent>::read(span<T, N> buf)
{
    return write(buf, elements{buf.length()});
}

template <typename CharT, span_extent_type BufferExtent>
template <typename T, span_extent_type N>
error basic_readable_buffer<CharT, BufferExtent>::read(span<T, N> buf,
                                                       characters length)
{
    SPIO_ASSERT(length <= buf.size(), "buf is not big enough");
    static_assert(sizeof(T) >= sizeof(CharT),
                  "Truncation in basic_readable_buffer<CharT>::read: sizeof "
                  "buffer is less than CharT");
    if (m_it == m_buffer.end()) {
        return end_of_file;
    }

    const auto dist = stl::distance(m_it, m_buffer.end());
    if (dist <= length) {
        const auto add = dist;
        stl::copy(m_it, m_it + add, buf.begin());
        stl::advance(m_it, add);
        return end_of_file;
    }
    const auto add = length;
    stl::copy(m_it, m_it + add, buf.begin());
    stl::advance(m_it, add);
    return {};
}

template <typename CharT, span_extent_type BufferExtent>
template <typename T, span_extent_type N>
error basic_readable_buffer<CharT, BufferExtent>::read(span<T, N> buf,
                                                       elements length)
{
    return read(buf, characters{length * quantity_type{sizeof(T)} /
                                quantity_type{sizeof(CharT)}});
}

template <typename CharT, span_extent_type BufferExtent>
template <typename T, span_extent_type N>
error basic_readable_buffer<CharT, BufferExtent>::read(span<T, N> buf,
                                                       bytes length)
{
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");
    return read(buf,
                characters{length / static_cast<quantity_type>(sizeof(CharT))});
}

template <typename CharT, span_extent_type BufferExtent>
template <typename T, span_extent_type N>
error basic_readable_buffer<CharT, BufferExtent>::read(span<T, N> buf,
                                                       bytes_contiguous length)
{
    SPIO_ASSERT(length % sizeof(CharT) == 0,
                "Length is not divisible by sizeof CharT");
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");
    if (m_it == m_buffer.end()) {
        return end_of_file;
    }

    const auto dist = distance(m_it, m_buffer.end());
    if (dist <= length) {
        const auto add = dist / static_cast<quantity_type>(sizeof(CharT));
        auto s = span<CharT>(m_it, m_it + add);
        copy_contiguous(s, buf);
        advance(m_it, add);
        return end_of_file;
    }
    const auto add = length / static_cast<quantity_type>(sizeof(CharT));
    auto s = span<CharT>(m_it, m_it + add);
    copy_contiguous(s, buf);
    advance(m_it, add);
    return {};
}

template <typename CharT, span_extent_type BufferExtent>
error basic_readable_buffer<CharT, BufferExtent>::read(CharT& c)
{
    span<CharT> s{&c, 1};
    return read(s, characters{1});
}

template <typename CharT, span_extent_type BufferExtent>
error basic_readable_buffer<CharT, BufferExtent>::skip()
{
    CharT c = 0;
    return read(c);
}

template <typename CharT, span_extent_type BufferExtent>
error basic_readable_buffer<CharT, BufferExtent>::rewind(
    typename buffer_type::difference_type steps)
{
    const auto dist = stl::distance(m_buffer.begin(), m_it);
    if (dist > steps) {
        return invalid_argument;
    }
    m_it -= steps;
    return {};
}
}  // namespace io
