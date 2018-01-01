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
#include "writable.h"

namespace io {
template <typename CharT, typename FileHandle>
basic_writable_file<CharT, FileHandle>::basic_writable_file(FileHandle& file)
    : m_file(&file)
{
    if (!file) {
        SPIO_THROW(invalid_argument, "basic_writable_file: Invalid file given");
    }
}

template <typename CharT, typename FileHandle>
template <typename T, span_extent_type N>
error basic_writable_file<CharT, FileHandle>::write(span<T, N> buf)
{
    return write(buf, elements{buf.length()});
}

template <typename CharT, typename FileHandle>
template <typename T, span_extent_type N>
error basic_writable_file<CharT, FileHandle>::write(span<T, N> buf,
                                                    characters length)
{
    assert(m_file && *m_file);
    SPIO_ASSERT(length <= buf.size(), "buf is not big enough");
    static_assert(sizeof(T) <= sizeof(CharT),
                  "Truncation in basic_writable_file<CharT>::write: sizeof "
                  "buffer is more than CharT");
    static_assert(
        std::is_trivially_copyable<T>::value,
        "basic_writable_file<CharT>::write: T must be TriviallyCopyable");

    const auto ret = [&]() {
#if SPIO_HAS_IF_CONSTEXPR
        if constexpr (sizeof(CharT) == 1) {
#else
        if (sizeof(CharT) == 1) {
#endif
            return m_file->write(as_bytes(buf.first(length)));
        }
        else {
            auto char_buf =
                as_bytes(buf).first(length * quantity_type{sizeof(T)});
            return m_file->write(char_buf) / sizeof(T);
        }
    }();
    return get_error(static_cast<quantity_type>(ret), length);
}

template <typename CharT, typename FileHandle>
template <typename T, span_extent_type N>
error basic_writable_file<CharT, FileHandle>::write(span<T, N> buf,
                                                    elements length)
{
    return write(buf, characters{length * quantity_type{sizeof(T)} /
                                 quantity_type{sizeof(CharT)}});
}

template <typename CharT, typename FileHandle>
template <typename T, span_extent_type N>
error basic_writable_file<CharT, FileHandle>::write(span<T, N> buf,
                                                    bytes length)
{
    return write(buf, characters{length * quantity_type{sizeof(CharT)}});
}

template <typename CharT, typename FileHandle>
template <typename T, span_extent_type N>
error basic_writable_file<CharT, FileHandle>::write(span<T, N> buf,
                                                    bytes_contiguous length)
{
    assert(m_file && *m_file);
    SPIO_ASSERT(length % sizeof(CharT) == 0,
                "Length is not divisible by sizeof CharT");
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");
    auto char_buf = as_bytes(buf).first(length);
    const auto ret = m_file->write(char_buf);
#if SPIO_HAS_IF_CONSTEXPR
    if constexpr (sizeof(T) == 1) {
#else
    if (sizeof(T) == 1) {
#endif
        copy(char_buf.begin(), char_buf.end(), buf.begin());
    }
    else {
        copy_contiguous(char_buf, buf);
    }
    return get_error(ret, length);
}

template <typename CharT, typename FileHandle>
error basic_writable_file<CharT, FileHandle>::write(CharT c)
{
    span<CharT> s{&c, 1};
    return write(s, characters{1});
}

template <typename CharT, typename FileHandle>
error basic_writable_file<CharT, FileHandle>::get_error(
    quantity_type read_count,
    quantity_type expected) const
{
    assert(m_file && *m_file);
    if (read_count == expected) {
        return {};
    }
    if (m_file->eof()) {
        return end_of_file;
    }
    if (m_file->error()) {
        return io_error;
    }
    return default_error;
}

template <typename CharT, typename FileHandle>
error basic_writable_file<CharT, FileHandle>::flush() noexcept
{
    assert(m_file && *m_file);
    if (!m_file->flush()) {
        return io_error;
    }
    return {};
}

template <typename CharT, typename BufferT>
constexpr basic_writable_buffer<CharT, BufferT>::basic_writable_buffer(
    buffer_type b)
    : m_buffer(std::move(b))
{
}

template <typename CharT, typename BufferT>
template <typename T, span_extent_type N>
error basic_writable_buffer<CharT, BufferT>::write(span<T, N> buf)
{
    return write(buf, elements{buf.length()});
}

template <typename CharT, typename BufferT>
template <typename T, span_extent_type N>
error basic_writable_buffer<CharT, BufferT>::write(span<T, N> buf,
                                                   characters length)
{
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");
    static_assert(
        sizeof(T) <= sizeof(CharT),
        "Truncation in basic_writable_buffer<CharT, BufferT>::read: sizeof "
        "buffer is more than CharT");

    const auto i = m_buffer.size();
    m_buffer.resize(i + length.get_unsigned());
    stl::copy(buf.begin(), buf.begin() + length,
              m_buffer.begin() +
                  static_cast<typename decltype(m_buffer)::difference_type>(i));
    if (m_buffer.is_end()) {
        return end_of_file;
    }
    return {};
}

template <typename CharT, typename BufferT>
template <typename T, span_extent_type N>
error basic_writable_buffer<CharT, BufferT>::write(span<T, N> buf,
                                                   elements length)
{
    return write(buf, characters{length * quantity_type{sizeof(T)} /
                                 quantity_type{sizeof(CharT)}});
}

template <typename CharT, typename BufferT>
template <typename T, span_extent_type N>
error basic_writable_buffer<CharT, BufferT>::write(span<T, N> buf, bytes length)
{
    return write(buf, characters{length * quantity_type{sizeof(CharT)}});
}

template <typename CharT, typename BufferT>
template <typename T, span_extent_type N>
error basic_writable_buffer<CharT, BufferT>::write(span<T, N> buf,
                                                   bytes_contiguous length)
{
    SPIO_ASSERT(length % sizeof(CharT) == 0,
                "Length is not divisible by sizeof CharT");
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");

    const auto i = m_buffer.size();
    m_buffer.resize(i + length / sizeof(CharT));
    copy_contiguous({buf.begin(), buf.end()},
                    {m_buffer.begin() + i, m_buffer.end()});
    if (m_buffer.is_end()) {
        return end_of_file;
    }
    return {};
}

template <typename CharT, typename BufferT>
error basic_writable_buffer<CharT, BufferT>::write(CharT c)
{
    span<CharT> s{&c, 1};
    return write(s, characters{1});
}
}  // namespace io
