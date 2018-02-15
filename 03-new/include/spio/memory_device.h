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

#ifndef SPIO_MEMORY_DEVICE_H
#define SPIO_MEMORY_DEVICE_H

#include "error.h"
#include "fwd.h"
#include "span.h"
#include "traits.h"

namespace spio {
namespace detail {
    template <typename Mode, typename CharT>
    class basic_memory_device_adaptor {
    public:
        using char_type = CharT;

        struct category : Mode, direct_tag {
        };

        constexpr basic_memory_device_adaptor() = default;
        constexpr basic_memory_device_adaptor(span<char_type> s) : m_buf(s) {}

        char_type* buffer()
        {
            return m_buf.data();
        }
        const char_type* buffer() const
        {
            return m_buf.data();
        }

        span<const char_type> input()
        {
            SPIO_ASSERT(
                buffer(),
                "basic_memory_device_adaptor::input: Sequence points to "
                "invalid memory!");
            return m_buf.as_const_span();
        }
        span<char_type> output()
        {
            SPIO_ASSERT(
                buffer(),
                "basic_memory_device_adaptor::output: Sequence points to "
                "invalid memory!");
            return m_buf;
        }

    private:
        span<char_type> m_buf{nullptr};
    };
}  // namespace detail

template <typename CharT>
class basic_memory_device
    : private detail::basic_memory_device_adaptor<seekable_device_tag, CharT> {
    using base =
        detail::basic_memory_device_adaptor<seekable_device_tag, CharT>;

public:
    using char_type = CharT;
    using category = typename base::category;

    using base::base;
    using base::buffer;
    using base::input;
    using base::output;
};

using memory_device = basic_memory_device<char>;
using wmemory_device = basic_memory_device<wchar_t>;

template <typename CharT>
class basic_memory_source
    : private detail::basic_memory_device_adaptor<seekable_source_tag, CharT> {
    using base =
        detail::basic_memory_device_adaptor<seekable_source_tag, CharT>;

public:
    using char_type = CharT;
    using category = typename base::category;

    using base::base;
    using base::buffer;
    using base::input;
};

using memory_source = basic_memory_source<char>;
using wmemory_source = basic_memory_source<wchar_t>;

template <typename CharT>
class basic_memory_sink
    : private detail::basic_memory_device_adaptor<seekable_sink_tag, CharT> {
    using base = detail::basic_memory_device_adaptor<seekable_sink_tag, CharT>;

public:
    using char_type = CharT;
    using category = typename base::category;

    using base::base;
    using base::buffer;
    using base::output;
};

using memory_sink = basic_memory_sink<char>;
using wmemory_sink = basic_memory_sink<wchar_t>;

template <typename Container>
class basic_container_sink {
public:
    using container_type = Container;
    using char_type = typename Container::value_type;
    using iterator = typename Container::iterator;

    struct category : seekable_sink_tag {
    };

    basic_container_sink() = default;
    basic_container_sink(container_type& c)
        : m_buf{std::addressof(c)}, m_it{m_buf->begin()}
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

    streamsize write(span<const char_type> s)
    {
        SPIO_ASSERT(m_buf,
                    "basic_container_sink::write: Cannot write to a nullptr "
                    "container!");

        m_it = m_buf->insert(m_it, s.begin(), s.end());
        m_it += s.size();
        return s.size();
    }

    streampos seek(streamoff off, seekdir way, uint64_t which = openmode::out)
    {
        SPIO_UNUSED(which);
        SPIO_ASSERT(
            m_buf,
            "basic_container_sink::seek: Cannot seek a nullptr container!");

        if (way == seekdir::beg) {
            auto size = static_cast<streamoff>(m_buf->size());
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

        auto size = static_cast<streamoff>(m_buf->size());
        if (size < -off || off > 0) {
            throw failure{make_error_code(std::errc::invalid_argument),
                          "basic_container_sink::seek: offset is out of range"};
        }
        m_it = m_buf->end() + off;
        return std::distance(m_buf->begin(), m_it);
    }

private:
    container_type* m_buf{nullptr};
    iterator m_it{};
};

template <typename CharT>
using basic_vector_sink = basic_container_sink<std::vector<CharT>>;
template <typename CharT>
using basic_string_sink = basic_container_sink<std::basic_string<CharT>>;

using string_sink = basic_string_sink<char>;
using wstring_sink = basic_string_sink<wchar_t>;
}  // namespace spio

#endif
