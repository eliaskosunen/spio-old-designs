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
#include "writable.h"

namespace io {
SPIO_INLINE stl::vector<char> file_buffering::_initialize_buffer(
    bool use,
    std::size_t len)
{
    if (use) {
        return stl::vector<char>(len);
    }
    return {};
}

SPIO_INLINE file_buffering::file_buffering(bool use_buffer,
                                           mode_type m,
                                           std::size_t len)
    : m_buffer(_initialize_buffer(use_buffer, len)),
      m_length(len),
      m_mode(m),
      m_use(true)
{
}

SPIO_INLINE error file_buffering::set(stdio_filehandle& file)
{
    if (!file) {
        return invalid_argument;
    }
    auto buf = [&]() -> char* {
        if (use()) {
            return &get_buffer()[0];
        }
        return nullptr;
    }();
    if (std::setvbuf(file.get(), buf, static_cast<int>(m_mode), m_length) !=
        0) {
        return io_error;
    }
    return {};
}

SPIO_INLINE file_buffering file_buffering::disable()
{
    return file_buffering(false, BUFFER_NONE, 0);
}
SPIO_INLINE file_buffering file_buffering::full(std::size_t len, bool external)
{
    return file_buffering(external, BUFFER_FULL, len);
}
SPIO_INLINE file_buffering file_buffering::line(std::size_t len, bool external)
{
    return file_buffering(external, BUFFER_LINE, len);
}

template <typename CharT>
basic_writable_file<CharT>::basic_writable_file(stdio_filehandle* file,
                                                file_buffering&& buffering)
    : m_file(file), m_buffering(std::move(buffering))
{
    if (!m_file) {
        SPIO_THROW(invalid_argument, "Nullptr file given");
    }
    m_buffering.set(*file);
}

template <typename CharT>
template <typename T, span_extent_type N>
error basic_writable_file<CharT>::write(span<T, N> buf)
{
    return write(buf, elements{buf.length()});
}

template <typename CharT>
template <typename T, span_extent_type N>
error basic_writable_file<CharT>::write(span<T, N> buf, characters length)
{
    assert(m_file);
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
            return SPIO_FWRITE(&buf[0], 1, length.get_unsigned(),
                               m_file->get());
        }
        else {
            auto char_buf =
                as_bytes(buf).first(length * quantity_type{sizeof(T)});
            return SPIO_FWRITE(&char_buf[0], 1,
                               length.get_unsigned() * sizeof(T),
                               m_file->get()) /
                   sizeof(T);
        }
    }();
    return get_error(static_cast<quantity_type>(ret), length);
}

template <typename CharT>
template <typename T, span_extent_type N>
error basic_writable_file<CharT>::write(span<T, N> buf, elements length)
{
    return write(buf, characters{length * quantity_type{sizeof(T)} /
                                 quantity_type{sizeof(CharT)}});
}

template <typename CharT>
template <typename T, span_extent_type N>
error basic_writable_file<CharT>::write(span<T, N> buf, bytes length)
{
    return write(buf, characters{length * quantity_type{sizeof(CharT)}});
}

template <typename CharT>
template <typename T, span_extent_type N>
error basic_writable_file<CharT>::write(span<T, N> buf, bytes_contiguous length)
{
    assert(m_file);
    SPIO_ASSERT(length % sizeof(CharT) == 0,
                "Length is not divisible by sizeof CharT");
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");
    auto char_buf = as_bytes(buf).first(length);
    const auto ret = SPIO_FWRITE(&char_buf[0], 1, length, m_file->get());
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

template <typename CharT>
error basic_writable_file<CharT>::write(CharT c)
{
    span<CharT> s{&c, 1};
    return write(s, characters{1});
}

template <typename CharT>
error basic_writable_file<CharT>::get_error(quantity_type read_count,
                                            quantity_type expected) const
{
    assert(m_file);
    if (read_count == expected) {
        return {};
    }
    if (m_file->error()) {
        return io_error;
    }
    return default_error;
}

template <typename CharT>
error basic_writable_file<CharT>::flush() noexcept
{
    assert(m_file);
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
    copy(buf.begin(), buf.begin() + length,
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