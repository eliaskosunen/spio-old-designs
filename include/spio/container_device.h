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

#ifndef SPIO_CONTAINER_DEVICE_H
#define SPIO_CONTAINER_DEVICE_H

#include "fwd.h"

#include <string>
#include <vector>
#include "error.h"
#include "span.h"
#include "traits.h"
#include "util.h"

namespace spio {
template <typename Container, typename Traits>
class basic_container_device {
public:
    using container_type = Container;
    using char_type = typename Container::value_type;
    using iterator = typename Container::iterator;
    using traits = Traits;

    struct category : seekable_device_tag {
    };

    basic_container_device() = default;
    basic_container_device(container_type& c)
        : m_buf(std::addressof(c)), m_it(m_buf->begin())
    {
    }

    container_type* container()
    {
        return m_buf;
    }
    const container_type* container() const
    {
        return m_buf;
    }

    streamsize read(span<char_type> s)
    {
        SPIO_ASSERT(m_buf,
                    "basic_container_device::read: Cannot write to a nullptr "
                    "container!");

        if (m_it == m_buf->end()) {
            return -1;
        }
        auto dist = std::distance(m_it, m_buf->end());
        auto n = std::min(dist, s.size());
        std::copy_n(m_it, n, s.begin());
        m_it += n;
        return n;
    }
    streamsize write(span<const char_type> s)
    {
        SPIO_ASSERT(m_buf,
                    "basic_container_device::write: Cannot write to a nullptr "
                    "container!");

        if (m_it == m_buf->end()) {
            if (s.size_us() > m_buf->size()) {
                m_buf->reserve(m_buf->size() + s.size_us());
            }
            /* m_buf->insert(m_buf->end(), s.begin(), s.end()); */
            append(s.begin(), s.end());
            m_it = m_buf->end();
        }
        else {
            auto dist = std::distance(m_buf->begin(), m_it);
            if (s.size_us() > m_buf->size()) {
                m_buf->reserve(m_buf->size() + s.size_us());
                m_buf->insert(m_buf->begin() + dist, s.begin(), s.end());
            }
            else {
                m_buf->insert(m_it, s.begin(), s.end());
            }
            m_it = m_buf->begin() + dist + s.size();
        }
        return s.size();
    }

    typename Traits::pos_type seek(typename Traits::off_type off,
                                   seekdir way,
                                   int which = openmode::out)
    {
        SPIO_UNUSED(which);
        SPIO_ASSERT(
            m_buf,
            "basic_container_device::seek: Cannot seek a nullptr container!");

        if (way == seekdir::beg) {
            auto size = static_cast<typename Traits::off_type>(m_buf->size());
            if (size < off || off < 0) {
                throw failure{
                    make_error_code(std::errc::invalid_argument),
                    "basic_container_sink::seek: offset is out of range"};
            }
            m_it = m_buf->begin() + off;
            return off;
        }
        if (way == seekdir::cur) {
            if (off == 0) {
                return std::distance(m_buf->begin(), m_it);
            }
            if (off < 0) {
                auto dist = std::distance(m_buf->begin(), m_it);
                if (dist < -off) {
                    throw failure{
                        make_error_code(std::errc::invalid_argument),
                        "basic_container_sink::seek: offset is out of range"};
                }
                m_it -= off;
                return std::distance(m_buf->begin(), m_it);
            }
            auto dist = std::distance(m_it, m_buf->end());
            if (dist < off) {
                throw failure{
                    make_error_code(std::errc::invalid_argument),
                    "basic_container_sink::seek: offset is out of range"};
            }
            m_it += off;
            return std::distance(m_buf->begin(), m_it);
        }

        auto size = static_cast<typename Traits::off_type>(m_buf->size());
        if (size < -off || off > 0) {
            throw failure{make_error_code(std::errc::invalid_argument),
                          "basic_container_sink::seek: offset is out of range"};
        }
        m_it = m_buf->end() + off;
        return std::distance(m_buf->begin(), m_it);
    }

protected:
    template <typename Iterator, typename C = container_type, typename = void>
    struct has_append : std::false_type {
    };
    template <typename Iterator, typename C>
    struct has_append<
        Iterator,
        C,
        void_t<decltype(std::declval<C>().append(std::declval<Iterator>(),
                                                 std::declval<Iterator>()),
                        void())>> : std::true_type {
    };

    template <typename Iterator, typename C = container_type>
    auto append(Iterator b, Iterator e)
        -> std::enable_if_t<has_append<Iterator, C>::value>
    {
        m_buf->append(b, e);
    }
    template <typename Iterator, typename C = container_type>
    auto append(Iterator b, Iterator e)
        -> std::enable_if_t<!has_append<Iterator, C>::value>
    {
        m_buf->insert(m_buf->end(), b, e);
    }

private:
    container_type* m_buf{nullptr};
    iterator m_it{};
};

template <typename Container, typename Traits>
class basic_container_source
    : private basic_container_device<Container, Traits> {
    using base = basic_container_device<Container, Traits>;

public:
    using container_type = Container;
    using char_type = typename base::char_type;
    using iterator = typename base::iterator;
    using traits = Traits;

    struct category : seekable_source_tag {
    };

#if defined(__GNUC__) && __GNUC__ < 7
    template <typename... Args>
    basic_container_source(Args&&... a) : base(std::forward<Args>(a)...)
    {
    }
#else
    using base::base;
#endif
    using base::container;
    using base::read;
    using base::seek;
};

template <typename Container, typename Traits>
class basic_container_sink : private basic_container_device<Container, Traits> {
    using base = basic_container_device<Container, Traits>;

public:
    using container_type = Container;
    using char_type = typename base::char_type;
    using iterator = typename base::iterator;
    using traits = Traits;

    struct category : seekable_sink_tag, nobuffer_tag {
    };

#if defined(__GNUC__) && __GNUC__ < 7
    template <typename... Args>
    basic_container_sink(Args&&... a) : base(std::forward<Args>(a)...)
    {
    }
#else
    using base::base;
#endif
    using base::container;
    using base::seek;
    using base::write;
};

template <typename CharT, typename Allocator = std::allocator<CharT>>
using basic_vector_source =
    basic_container_source<std::vector<CharT, Allocator>>;
template <typename CharT,
          typename Traits = std::char_traits<CharT>,
          typename Allocator = std::allocator<CharT>>
using basic_string_source =
    basic_container_source<std::basic_string<CharT, Traits, Allocator>>;

using string_source = basic_string_source<char>;
using wstring_source = basic_string_source<wchar_t>;

template <typename CharT, typename Allocator = std::allocator<CharT>>
using basic_vector_sink = basic_container_sink<std::vector<CharT, Allocator>>;
template <typename CharT,
          typename Traits = std::char_traits<CharT>,
          typename Allocator = std::allocator<CharT>>
using basic_string_sink =
    basic_container_sink<std::basic_string<CharT, Traits, Allocator>>;

using string_sink = basic_string_sink<char>;
using wstring_sink = basic_string_sink<wchar_t>;
}  // namespace spio

#endif
