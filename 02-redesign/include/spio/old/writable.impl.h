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
#include "writable.h"

namespace io {
template <typename CharT, typename FileHandle>
basic_writable_file<CharT, FileHandle>::basic_writable_file(FileHandle& file)
    : m_file(&file)
{
    if (!file) {
        SPIO_THROW(make_error_code(std::errc::invalid_argument),
                   "basic_writable_file: Invalid file given");
    }
}

template <typename CharT, typename FileHandle>
template <typename T, extent_t N>
std::error_code basic_writable_file<CharT, FileHandle>::write(span<T, N> buf)
{
    return write(buf, elements{buf.length()});
}

template <typename CharT, typename FileHandle>
template <typename T, extent_t N>
std::error_code basic_writable_file<CharT, FileHandle>::write(span<T, N> buf,
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

    std::size_t bytes = 0;
#if SPIO_HAS_IF_CONSTEXPR
    if constexpr (sizeof(CharT) == 1) {
#else
    if (sizeof(CharT) == 1) {
#endif
        if (auto e = m_file->write(as_bytes(buf.first(length)), bytes)) {
            return e;
        }
    }
    else {
        auto char_buf = as_bytes(buf).first(length * quantity_type{sizeof(T)});
        auto e = m_file->write(char_buf, bytes);
        bytes /= sizeof(T);
        if (e) {
            return e;
        }
    }
    return get_error(static_cast<quantity_type>(bytes), length);
}

template <typename CharT, typename FileHandle>
template <typename T, extent_t N>
std::error_code basic_writable_file<CharT, FileHandle>::write(span<T, N> buf,
                                                              elements length)
{
    return write(buf, characters{length * quantity_type{sizeof(T)} /
                                 quantity_type{sizeof(CharT)}});
}

template <typename CharT, typename FileHandle>
template <typename T, extent_t N>
std::error_code basic_writable_file<CharT, FileHandle>::write(span<T, N> buf,
                                                              bytes length)
{
    return write(buf, characters{length * quantity_type{sizeof(CharT)}});
}

template <typename CharT, typename FileHandle>
template <typename T, extent_t N>
std::error_code basic_writable_file<CharT, FileHandle>::write(
    span<T, N> buf,
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
std::error_code basic_writable_file<CharT, FileHandle>::write(CharT c)
{
    span<CharT> s{&c, 1};
    return write(s, characters{1});
}

template <typename CharT, typename FileHandle>
std::error_code basic_writable_file<CharT, FileHandle>::seek(seek_origin origin,
                                                             seek_type offset)
{
    assert(m_file && *m_file);
    if (auto e = m_file->flush()) {
        return e;
    }
    if (auto e = m_file->seek(origin, offset)) {
        return e;
    }
    return {};
}

template <typename CharT, typename FileHandle>
std::error_code basic_writable_file<CharT, FileHandle>::tell(seek_type& pos)
{
    assert(m_file && *m_file);
    if (auto e = m_file->tell(pos)) {
        return e;
    }
    return {};
}

template <typename CharT, typename FileHandle>
std::error_code basic_writable_file<CharT, FileHandle>::get_error(
    quantity_type read_count,
    quantity_type expected) const
{
    assert(m_file && *m_file);
    if (read_count == expected) {
        return {};
    }
    if (m_file->eof()) {
        return make_error_condition(end_of_file);
    }
    if (auto e = m_file->error()) {
        return e;
    }
    return make_error_condition(undefined_error);
}

template <typename CharT, typename FileHandle>
std::error_code basic_writable_file<CharT, FileHandle>::flush() noexcept
{
    assert(m_file && *m_file);
    if (auto e = m_file->flush()) {
        return e;
    }
    return {};
}

template <typename CharT, typename BufferT>
constexpr basic_writable_buffer<CharT, BufferT>::basic_writable_buffer(
    buffer_type b)
    : m_buffer(std::move(b)), m_it{m_buffer.begin()}
{
}

template <typename CharT, typename BufferT>
template <typename T, extent_t N>
std::error_code basic_writable_buffer<CharT, BufferT>::write(span<T, N> buf)
{
    return write(buf, elements{buf.length()});
}

template <typename CharT, typename BufferT>
template <typename T, extent_t N>
std::error_code basic_writable_buffer<CharT, BufferT>::write(span<T, N> buf,
                                                             characters length)
{
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");
    static_assert(
        sizeof(T) <= sizeof(CharT),
        "Truncation in basic_writable_buffer<CharT, BufferT>::write: sizeof "
        "buffer is more than CharT");

    m_it = m_buffer.insert(typename BufferT::const_iterator{m_it}, buf.begin(),
                           buf.end());
    m_it++;
    if (m_buffer.is_end()) {
        return make_error_condition(end_of_file);
    }
    return {};
}

template <typename CharT, typename BufferT>
template <typename T, extent_t N>
std::error_code basic_writable_buffer<CharT, BufferT>::write(span<T, N> buf,
                                                             elements length)
{
    return write(buf, characters{length * quantity_type{sizeof(T)} /
                                 quantity_type{sizeof(CharT)}});
}

template <typename CharT, typename BufferT>
template <typename T, extent_t N>
std::error_code basic_writable_buffer<CharT, BufferT>::write(span<T, N> buf,
                                                             bytes length)
{
    return write(buf, characters{length * quantity_type{sizeof(CharT)}});
}

template <typename CharT, typename BufferT>
template <typename T, extent_t N>
std::error_code basic_writable_buffer<CharT, BufferT>::write(
    span<T, N> buf,
    bytes_contiguous length)
{
    SPIO_ASSERT(length % sizeof(CharT) == 0,
                "Length is not divisible by sizeof CharT");
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");

    auto b = as_bytes(buf);
    m_it = m_buffer.insert(m_it, b.begin(), b.end());
    m_it++;
    if (m_buffer.is_end()) {
        return make_error_condition(end_of_file);
    }
    return {};
}

template <typename CharT, typename BufferT>
std::error_code basic_writable_buffer<CharT, BufferT>::write(CharT c)
{
    span<CharT> s{&c, 1};
    return write(s, characters{1});
}

template <typename CharT, typename BufferT>
std::error_code basic_writable_buffer<CharT, BufferT>::seek(seek_origin origin,
                                                            seek_type offset)
{
    if (origin == seek_origin::SET) {
        if (static_cast<seek_type>(m_buffer.size()) < offset) {
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

template <typename CharT, typename BufferT>
std::error_code basic_writable_buffer<CharT, BufferT>::tell(seek_type& pos)
{
    pos = static_cast<seek_type>(std::distance(m_buffer.begin(), m_it));
    return {};
}

template <typename CharT, typename BufferT>
constexpr auto basic_writable_buffer<CharT, BufferT>::to_readable()
{
    auto buffer = get_buffer().to_span();
    return basic_readable_buffer<CharT>{buffer};
}
}  // namespace io
