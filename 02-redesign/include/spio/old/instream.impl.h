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

#include "instream.h"

namespace io {
template <typename Readable>
template <typename T, extent_t N>
bool basic_instream<Readable>::_read(span<T, N> s, elements length)
{
    if (m_buffer.empty()) {
        if (auto e = m_readable.read(s, length)) {
            if (is_eof(e)) {
                return false;
            }
            SPIO_THROW_EC(e);
        }
        return true;
    }
    SPIO_ASSERT(s.size() >= length,
                "basic_instream<>::_read: span.size() must be greater or equal "
                "to the element count to read");
    auto len_bytes = length.get_unsigned() * sizeof(T);
    if (m_buffer.size() >= len_bytes) {
        auto len = static_cast<typename decltype(m_buffer)::difference_type>(
            len_bytes);
        copy(m_buffer.begin(), m_buffer.begin() + len,
             as_writable_bytes(s).data());
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + len);
        return true;
    }
    auto i = m_buffer.size();
    copy(m_buffer.begin(), m_buffer.end(), as_writable_bytes(s).data());
    m_buffer.clear();
    if (i % sizeof(T) == 0) {
        auto range = make_span(s.data() + i / sizeof(T), s.data() + s.size());
        if (auto e = m_readable.read(range, elements{range.size()})) {
            if (is_eof(e)) {
                return false;
            }
            SPIO_THROW_EC(e);
        }
        return true;
    }
    auto range = make_span(s.data() + i, s.data() + s.size());
    if (auto e = m_readable.read(range, bytes{range.size()})) {
        if (is_eof(e)) {
            return false;
        }
        SPIO_THROW_EC(e);
    }
    return true;
}

template <typename Readable>
template <typename T>
void basic_instream<Readable>::push(T elem)
{
    auto chars = reinterpret_cast<char*>(&elem);
    m_buffer.insert(m_buffer.begin(), chars, chars + sizeof(T));
    m_eof = false;
}

template <typename Readable>
template <typename T, extent_t N>
void basic_instream<Readable>::push(span<T, N> elems)
{
    if (!elems.empty()) {
        auto chars = reinterpret_cast<char*>(elems.data());
        m_buffer.insert(m_buffer.begin(), chars, chars + elems.size_bytes());
        m_eof = false;
    }
}

template <typename Readable>
template <typename T>
std::enable_if_t<
    is_growable_read_container<T>::value &&
        std::is_same<typename T::value_type,
                     typename basic_instream<Readable>::char_type>::value,
    basic_instream<Readable>>&
basic_instream<Readable>::getline(T& val, char_type delim)
{
    struct getline_growable_read : public growable_read<T> {
    };
    span<char_type, 1> spaces{&delim, 1};
#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif
    reader_options<std::remove_reference_t<T>> opt = {spaces, false};
#if defined(__clang__)
#pragma GCC diagnostic pop
#endif
    m_eof = !getline_growable_read::read(*this, val, opt);
    return *this;
}

template <typename Readable>
template <typename T, typename... Args>
void basic_instream<Readable>::_scan(const char_type* format,
                                     T& a,
                                     Args&... args)
{
    {
        char_type ch;
        while (get(ch)) {
            if (!is_space(ch)) {
                push(ch);
                break;
            }
        }
    }
    while (*format) {
        for (; *format && is_space(*format); ++format) {
            char_type ch;
            while (get(ch)) {
                if (!is_space(ch)) {
                    push(ch);
                    break;
                }
            }
        }
        if (!(*format)) {
            break;
        }

        if (*format == char_type{'{'}) {
            if (*(format + 1) == char_type{'{'}) {
                format++;
            }
            else {
                return _scan(_scan_arg(format, a), args...);
            }
        }
        if (!(*format)) {
            break;
        }

        char_type ch;
        get(ch);
        if (ch != *format) {
            // TODO: Handle error
        }
        ++format;
    }
    // TODO: Handle error
}

template <typename Readable>
template <typename T>
auto basic_instream<Readable>::_scan_arg(const char_type* format, T& a)
    -> const char_type*
{
    // TODO: Save formatting specifier
    ++format;
    for (; *format && *(format - 1) != '}'; ++format) {
    }

    read(a);
    return format;
}

#if 0
#if SPIO_HAS_FOLD_EXPRESSIONS
template <typename Readable>
template <typename... T>
basic_instream<Readable>& basic_instream<Readable>::scan(T&&... args)
{
    (read(std::forward<T>(args)) && ...);
    return *this;
}
#else
namespace detail {
    template <typename In>
    bool instream_scan(In&)
    {
        return true;
    }
    template <typename In, typename T, typename... Args>
    bool instream_scan(In& r, T&& arg, Args&&... args)
    {
        if (!r.read(std::forward<T>(arg))) {
            return false;
        }
        return instream_scan(r, std::forward<Args>(args)...);
    }
}  // namespace detail

template <typename Readable>
template <typename... T>
basic_instream<Readable>& basic_instream<Readable>::scan(T&&... args)
{
    detail::instream_scan(*this, std::forward<T>(args)...);
    return *this;
}
#endif
#endif
}  // namespace io
