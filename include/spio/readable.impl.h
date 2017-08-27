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
template <typename CharT>
basic_readable_file<CharT>::basic_readable_file(file_wrapper file)
    : m_file(std::move(file))
{
    if (!file.value()) {
        SPIO_THROW(invalid_argument, "Nullptr file given");
    }
    valid = true;
}
template <typename CharT>
basic_readable_file<CharT>::basic_readable_file(const char* filename)
    : m_file(nullptr)
{
    auto f = std::fopen(filename, "rb");
    if (!f) {
        SPIO_THROW(invalid_argument, "Failed to open file");
    }
    m_file = make_file_wrapper(f, true);
    valid = true;
}

template <typename CharT>
template <typename T>
error basic_readable_file<CharT>::read(span<T> buf, characters length)
{
    SPIO_ASSERT(valid, "Cannot read from invalid stream");
    SPIO_ASSERT(length <= buf.size_bytes() / sizeof(CharT),
                 "buf is not big enough");
    static_assert(sizeof(T) >= sizeof(CharT),
                  "Truncation in basic_readable_file<CharT>::read: sizeof "
                  "buffer is less than CharT");
    static_assert(
        std::is_trivially_copyable_v<T>,
        "basic_readable_file<CharT>::read: T must be TriviallyCopyable");

    const auto ret = [&]() {
        if constexpr (sizeof(CharT) == 1) {
            return SPIO_FREAD(&buf[0], 1, length, m_file.value());
        }
        vector<char> char_buf(length * sizeof(CharT), 0);
        const auto r = SPIO_FREAD(&char_buf[0], 1, length * sizeof(CharT),
                                   m_file.value());
        for (std::size_t i = 0; i < length; ++i) {
            buf[i] = *reinterpret_cast<T*>(&char_buf[i * sizeof(CharT)]);
        }
        return r / sizeof(CharT);
    }();
    return get_error(ret, length);
}

template <typename CharT>
template <typename T>
error basic_readable_file<CharT>::read(span<T> buf, elements length)
{
    return read(buf, characters{length * sizeof(T) / sizeof(CharT)});
}

template <typename CharT>
template <typename T>
error basic_readable_file<CharT>::read(span<T> buf, bytes length)
{
    SPIO_ASSERT(length <= buf.size(), "buf is not big enough");
    vector<CharT> char_buf(length / sizeof(CharT), 0);
    const auto ret = read(char_buf, bytes_contiguous{length});
    if constexpr (sizeof(T) <= sizeof(CharT)) {
        copy(char_buf.begin(), char_buf.end(), buf.begin());
        return ret;
    }
    for (std::size_t i = 0; i < char_buf.length(); ++i) {
        buf[i * sizeof(CharT)] = *reinterpret_cast<T*>(&char_buf[i]);
    }
    return ret;
}

template <typename CharT>
template <typename T>
error basic_readable_file<CharT>::read(span<T> buf, bytes_contiguous length)
{
    SPIO_ASSERT(valid, "Cannot read from invalid stream");
    SPIO_ASSERT(length % sizeof(CharT) == 0,
                 "Length is not divisible by sizeof CharT");
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");
    vector<char> char_buf(length, 0);
    const auto ret = SPIO_FREAD(&char_buf[0], 1, length, m_file.value());
    if constexpr (sizeof(T) == 1) {
        copy(char_buf.begin(), char_buf.end(), buf.begin());
    }
    else {
        copy_contiguous(char_buf, buf);
    }
    return get_error(ret, length);
}

template <typename CharT>
error basic_readable_file<CharT>::read(CharT* c)
{
    span<CharT> s{c, 1};
    return read(s, characters{1});
}

template <typename CharT>
error basic_readable_file<CharT>::skip()
{
    CharT c = 0;
    return read(&c);
}

template <typename CharT>
error basic_readable_file<CharT>::get_error(std::size_t read_count,
                                            std::size_t expected) const
{
    if (read_count == expected) {
        return {};
    }
    if (std::ferror(m_file.value())) {
        return io_error;
    }
    if (std::feof(m_file.value())) {
        return end_of_file;
    }
    return default_error;
}

template <typename CharT>
basic_readable_buffer<CharT>::basic_readable_buffer(span<CharT> buf)
    : m_buffer(buf), m_it(m_buffer.begin()), valid(true)
{
}

template <typename CharT>
template <typename T>
error basic_readable_buffer<CharT>::read(span<T> buf, characters length)
{
    SPIO_ASSERT(valid, "Cannot read from invalid stream");
    SPIO_ASSERT(length <= buf.size(), "buf is not big enough");
    static_assert(sizeof(T) >= sizeof(CharT),
                  "Truncation in basic_readable_buffer<CharT>::read: sizeof "
                  "buffer is less than CharT");
    if (m_it == m_buffer.end()) {
        return end_of_file;
    }

    const auto dist = distance_nonneg(m_it, m_buffer.end());
    if (dist <= length) {
        const auto add =
            static_cast<typename decltype(m_it)::difference_type>(dist);
        copy(m_it, m_it + add, buf.begin());
        advance(m_it, add);
        return end_of_file;
    }
    const auto add =
        static_cast<typename decltype(m_it)::difference_type>(length);
    copy(m_it, m_it + add, buf.begin());
    advance(m_it, add);
    return {};
}

template <typename CharT>
template <typename T>
error basic_readable_buffer<CharT>::read(span<T> buf, elements length)
{
    return read(buf, characters{length * sizeof(T) / sizeof(CharT)});
}

template <typename CharT>
template <typename T>
error basic_readable_buffer<CharT>::read(span<T> buf, bytes length)
{
    SPIO_ASSERT(length <= buf.size(), "buf is not big enough");
    vector<CharT> char_buf(length / sizeof(CharT), 0);
    const auto ret = read(make_span(char_buf), bytes_contiguous{length});
    if constexpr (sizeof(T) <= sizeof(CharT)) {
        copy(char_buf.begin(), char_buf.end(), buf.begin());
        return ret;
    }
    for (std::size_t i = 0; i < char_buf.size(); ++i) {
        buf[i * sizeof(CharT)] = *reinterpret_cast<T*>(&char_buf[i]);
    }
    return ret;
}

template <typename CharT>
template <typename T>
error basic_readable_buffer<CharT>::read(span<T> buf, bytes_contiguous length)
{
    SPIO_ASSERT(length % sizeof(CharT) == 0,
                 "Length is not divisible by sizeof CharT");
    SPIO_ASSERT(length <= buf.size_bytes(), "buf is not big enough");
    if (m_it == m_buffer.end()) {
        return end_of_file;
    }

    const auto dist = distance_nonneg(m_it, m_buffer.end());
    if (dist <= length) {
        const auto add = static_cast<typename decltype(m_it)::difference_type>(
            dist / sizeof(CharT));
        auto s = span<CharT>{&*m_it, &*(m_it + add)};
        copy_contiguous(s, buf);
        advance(m_it, add);
        return end_of_file;
    }
    const auto add = static_cast<typename decltype(m_it)::difference_type>(
        length / sizeof(CharT));
    auto s = span<CharT>{&*m_it, &*(m_it + add)};
    copy_contiguous(s, buf);
    advance(m_it, add);
    return {};
}

template <typename CharT>
error basic_readable_buffer<CharT>::read(CharT* c)
{
    span<CharT> s{c, 1};
    return read(s, characters{1});
}

template <typename CharT>
error basic_readable_buffer<CharT>::skip()
{
    CharT c = 0;
    return read(&c);
}
}  // namespace io
