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

#ifndef SPIO_STREAM_ITERATOR_H
#define SPIO_STREAM_ITERATOR_H

#include "fwd.h"

#include "span.h"

namespace spio {
template <typename T, typename CharT>
class outstream_iterator {
public:
    using char_type = CharT;
    using outstream_type = basic_stream_ref<CharT, output>;

    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;
    using iterator_category = std::output_iterator_tag;

    outstream_iterator(outstream_type& o) : m_out(std::addressof(o)) {}
    outstream_iterator(outstream_type& o, const char_type* d)
        : m_out(std::addressof(o)), m_delim(d, std::strlen(d))
    {
    }

    outstream_iterator& operator=(const T& value);

    outstream_iterator& operator*()
    {
        return *this;
    }
    outstream_iterator& operator++()
    {
        return *this;
    }
    outstream_iterator& operator++(int)
    {
        return *this;
    }

private:
    outstream_type* m_out;
    span<const char_type> m_delim{nullptr};
};

template <typename CharT>
class outstream_iterator<CharT, CharT> {
public:
    using char_type = CharT;
    using outstream_type = basic_stream_ref<CharT, output>;

    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;
    using iterator_category = std::output_iterator_tag;

    outstream_iterator(outstream_type& o) : m_out(std::addressof(o)) {}
    outstream_iterator(outstream_type& o, const char_type* d)
        : m_out(std::addressof(o)), m_delim(d, std::strlen(d))
    {
    }

    outstream_iterator& operator=(char_type value)
    {
        return operator=(make_span(&value, 1));
    }
    outstream_iterator& operator=(span<const char_type> value);

    outstream_iterator& operator*()
    {
        return *this;
    }
    outstream_iterator& operator++()
    {
        return *this;
    }
    outstream_iterator& operator++(int)
    {
        return *this;
    }

private:
    outstream_type* m_out;
    span<const char_type> m_delim{nullptr};
};

template <typename T, typename CharT>
class instream_iterator {
public:
    using char_type = CharT;
    using instream_type = basic_stream_ref<CharT, input>;

    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;
    using iterator_category = std::input_iterator_tag;

    constexpr instream_iterator() = default;
    instream_iterator(instream_type& i) : m_in(std::addressof(i)) {}

    reference operator*() const
    {
        return m_last;
    }
    pointer operator->() const
    {
        return std::addressof(m_last);
    }

    instream_iterator& operator++()
    {
        _read();
        return *this;
    }
    instream_iterator operator++(int)
    {
        instream_iterator tmp(*this);
        operator++();
        return tmp;
    }

    instream_type& get_stream()
    {
        return *m_in;
    }
    const instream_type& get_stream() const
    {
        return *m_in;
    }

    template <typename T_, typename CharT_>
    friend constexpr bool operator==(const instream_iterator<T_, CharT_>&,
                                     const instream_iterator<T_, CharT_>&);
    template <typename T_, typename CharT_>
    friend constexpr bool operator!=(const instream_iterator<T_, CharT_>&,
                                     const instream_iterator<T_, CharT_>&);

private:
    void _read();

    instream_type* m_in{nullptr};
    T m_last{};
};

template <typename T>
class instream_iterator<T, T> {
public:
    using char_type = T;
    using instream_type = basic_stream_ref<T, input>;

    friend class basic_builtin_scanner<T>;

    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;
    using iterator_category = std::input_iterator_tag;

    constexpr instream_iterator() = default;
    instream_iterator(instream_type& i) : m_in(std::addressof(i)) {}

    reference operator*() const
    {
        return m_last;
    }
    pointer operator->() const
    {
        return std::addressof(m_last);
    }

    instream_iterator& operator++()
    {
        _read();
        return *this;
    }
    instream_iterator operator++(int)
    {
        instream_iterator tmp(*this);
        operator++();
        return tmp;
    }

    instream_type& get_stream()
    {
        return *m_in;
    }
    const instream_type& get_stream() const
    {
        return *m_in;
    }

    void read_into(span<T> s);

    template <typename T_>
    friend constexpr bool operator==(const instream_iterator<T_, T_>&,
                                     const instream_iterator<T_, T_>&);
    template <typename T_>
    friend constexpr bool operator!=(const instream_iterator<T_, T_>&,
                                     const instream_iterator<T_, T_>&);

private:
    void _read();

    instream_type* m_in{nullptr};
    T m_last{};
};

template <typename T>
void read_iterator_into_span(instream_iterator<T, T> it, span<const T> s);

template <typename T, typename CharT>
constexpr bool operator==(const instream_iterator<T, CharT>& lhs,
                          const instream_iterator<T, CharT>& rhs)
{
    return lhs.m_in == rhs.m_in;
}
template <typename T, typename CharT>
constexpr bool operator!=(const instream_iterator<T, CharT>& lhs,
                          const instream_iterator<T, CharT>& rhs)
{
    return !(lhs == rhs);
}

template <typename T>
constexpr bool operator==(const instream_iterator<T, T>& lhs,
                          const instream_iterator<T, T>& rhs)
{
    return lhs.m_in == rhs.m_in;
}
template <typename T>
constexpr bool operator!=(const instream_iterator<T, T>& lhs,
                          const instream_iterator<T, T>& rhs)
{
    return !(lhs == rhs);
}
}  // namespace spio

#include "stream_iterator.impl.h"

#endif
