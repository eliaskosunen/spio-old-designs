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
template <typename CharT>
basic_writable_file<CharT>::basic_writable_file(file_wrapper file,
                                                file_buffering&& buffering)
    : m_file(std::move(file)), m_buffering(std::move(buffering))
{
    if (!file) {
        SPIO_THROW(invalid_argument, "Nullptr file given");
    }
    m_buffering.set(file);
    valid = true;
}
template <typename CharT>
basic_writable_file<CharT>::basic_writable_file(const char* filename,
                                                bool append,
                                                file_buffering&& buffering)
    : m_file(nullptr), m_buffering(std::move(buffering))
{
    auto f = std::fopen(filename, append ? "ab" : "wb");
    if (!f) {
        SPIO_THROW(invalid_argument, "Failed to open file");
    }
    m_file = make_file_wrapper(f, true);
    valid = true;
}

template <typename CharT>
template <typename T>
error basic_writable_file<CharT>::write(span<T> buf, characters length)
{
    SPIO_ASSERT(valid, "Cannot write on invalid stream");
    SPIO_ASSERT(length <= buf.size(), "buf is not big enough");
    static_assert(sizeof(T) >= sizeof(CharT),
                  "Truncation in basic_writable_file<CharT>::write: sizeof "
                  "buffer is less than CharT");
    static_assert(
        std::is_trivially_copyable_v<T>,
        "basic_writable_file<CharT>::write: T must be TriviallyCopyable");

    const auto ret = [&]() {
        if constexpr (sizeof(CharT) == 1) {
            return SPIO_FWRITE(&buf[0], 1, length, m_file.value());
        }
        else {
            vector<uint8_t> char_buf(length * sizeof(T), 0);
            for (std::size_t i = 0; i < length; ++i) {
                for (std::size_t j = 0; j < sizeof(T); ++j) {
                    char_buf[i * sizeof(T) + j] =
                        static_cast<uint8_t>((buf[i] >> (j * 8)) & 0xFF);
                }
            }
            return SPIO_FWRITE(&char_buf[0], 1, length * sizeof(T),
                                m_file.value()) /
                   sizeof(T);
        }
    }();
    return get_error(ret, length);
}

template <typename CharT>
template <typename T>
error basic_writable_file<CharT>::write(span<T> buf, elements length)
{
    return write(buf, characters{length * sizeof(T) / sizeof(CharT)});
}

template <typename CharT>
template <typename T>
error basic_writable_file<CharT>::write(span<T> buf, bytes length)
{
    return write(buf, characters{length * sizeof(CharT)});
}

template <typename CharT>
template <typename T>
error basic_writable_file<CharT>::write(span<T> buf, bytes_contiguous length)
{
    SPIO_ASSERT(valid, "Cannot write on invalid stream");
    SPIO_ASSERT(length % sizeof(CharT) == 0,
                 "Length is not divisible by sizeof CharT");
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");
    vector<char> char_buf(length, 0);
    const auto ret = SPIO_FWRITE(&char_buf[0], 1, length, m_file.value());
    if constexpr (sizeof(T) == 1) {
        copy(char_buf.begin(), char_buf.end(), buf.begin());
    }
    else {
        copy_contiguous(char_buf, buf);
    }
    return get_error(ret, length);
}

template <typename CharT>
error basic_writable_file<CharT>::write(CharT* c)
{
    span<CharT> s{c, 1};
    return write(s, characters{1});
}

template <typename CharT>
error basic_writable_file<CharT>::get_error(std::size_t read_count,
                                            std::size_t expected) const
{
    if (read_count == expected) {
        return {};
    }
    if (std::ferror(m_file.value())) {
        return io_error;
    }
    return default_error;
}

template <typename CharT>
error basic_writable_file<CharT>::flush() noexcept
{
    if (std::fflush(m_file.value()) != 0) {
        return io_error;
    }
    return {};
}

template <typename CharT>
template <typename T>
error basic_writable_buffer<CharT>::write(span<T> buf, characters length)
{
    SPIO_ASSERT(valid, "Cannot write on invalid stream");
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");
    static_assert(sizeof(T) >= sizeof(CharT),
                  "Truncation in basic_writable_buffer<CharT>::read: sizeof "
                  "buffer is less than CharT");

    copy(buf.begin(), buf.begin() + length, std::back_inserter(m_buffer));
    return {};
}

template <typename CharT>
template <typename T>
error basic_writable_buffer<CharT>::write(span<T> buf, elements length)
{
    return write(buf, characters{length * sizeof(T) / sizeof(CharT)});
}

template <typename CharT>
template <typename T>
error basic_writable_buffer<CharT>::write(span<T> buf, bytes length)
{
    return write(buf, characters{length * sizeof(CharT)});
}

template <typename CharT>
template <typename T>
error basic_writable_buffer<CharT>::write(span<T> buf, bytes_contiguous length)
{
    SPIO_ASSERT(valid, "Cannot write on invalid stream");
    SPIO_ASSERT(length % sizeof(CharT) == 0,
                 "Length is not divisible by sizeof CharT");
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");

    auto i = m_buffer.size();
    m_buffer.reserve(m_buffer.size() + length / sizeof(CharT));
    copy_contiguous({buf.begin(), buf.end()},
                    {m_buffer.begin() + i, m_buffer.end()});
    return {};
}

template <typename CharT>
error basic_writable_buffer<CharT>::write(CharT* c)
{
    span<CharT> s{c, 1};
    return write(s, characters{1});
}
}  // namespace io
