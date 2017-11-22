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

#ifndef SPIO_BUFFERING_H
#define SPIO_BUFFERING_H

#include <cstdio>
#include "span.h"
#include "stl.h"

namespace io {
template <typename Alloc = stl::allocator<char>>
class basic_filebuffer {
public:
    using allocator_type = Alloc;

    enum buffer_mode { BUFFER_DEFAULT, BUFFER_FULL, BUFFER_LINE, BUFFER_NONE };

#ifdef BUFSIZ
    static constexpr std::size_t default_size = BUFSIZ;
#else
    static constexpr std::size_t default_size = 8192;
#endif

    basic_filebuffer(const Alloc& alloc)
        : m_alloc(alloc),
          m_buffer{_initialize_buffer(BUFFER_FULL, default_size, m_alloc)},
          m_it{m_buffer.begin()},
          m_mode{BUFFER_FULL}
    {
    }

    basic_filebuffer(buffer_mode mode = BUFFER_FULL,
                     std::size_t len = default_size,
                     const Alloc& alloc = Alloc{})
        : m_alloc(alloc),
          m_buffer{_initialize_buffer(mode, len, m_alloc)},
          m_it{m_buffer.begin()},
          m_mode(mode)
    {
    }

    basic_filebuffer(const basic_filebuffer&) = delete;
    basic_filebuffer& operator=(const basic_filebuffer&) = delete;
    basic_filebuffer(basic_filebuffer&&) = default;
    basic_filebuffer& operator=(basic_filebuffer&&) = default;
    ~basic_filebuffer() noexcept = default;

    char* get_buffer()
    {
        assert(!m_buffer.empty());
        return &m_buffer[0];
    }
    const char* get_buffer() const
    {
        assert(!m_buffer.empty());
        return &m_buffer[0];
    }

    auto size() const
    {
        return m_buffer.size();
    }

    auto mode() const
    {
        return m_mode;
    }

    template <typename FlushFn>
    std::size_t write(const_byte_span data, FlushFn&& flush)
    {
        assert(m_mode != BUFFER_NONE && m_mode != BUFFER_DEFAULT);
#if SPIO_HAS_INVOCABLE
        static_assert(
            std::is_invocable_r_v<bool, decltype(flush), const_byte_span>,
            "filebuffer::write: flush must be Invocable with type "
            "bool(io::const_byte_span)");
#endif
        auto dist_till_end = stl::distance(m_it, m_buffer.end());
        if (dist_till_end >= data.size()) {
            m_it = stl::copy(data.begin(), data.end(), m_it);
            return data.size_us();
        }
        stl::copy(data.begin(), data.begin() + dist_till_end, m_it);
        flush(as_bytes(make_span(m_buffer)));
        m_it = m_buffer.begin();
        write(as_bytes(data.subspan(dist_till_end)), flush);
        auto f = flush_if_needed(flush);
        if (f.first) {
            return f.second;
        }
        return data.size_us();
    }

    template <typename FlushFn>
    std::pair<bool, std::size_t> flush_if_needed(FlushFn&& flush)
    {
        assert(m_mode != BUFFER_NONE && m_mode != BUFFER_DEFAULT);
#if SPIO_HAS_INVOCABLE
        static_assert(
            std::is_invocable_r_v<bool, decltype(flush), const_byte_span>,
            "filebuffer::flush_if_needed: flush must be Invocable with type "
            "bool(io::const_byte_span)");
#endif
        if (m_mode == BUFFER_LINE) {
            for (auto it = m_it; it != m_buffer.begin(); --it) {
                if (*it == '\n') {
                    return {true, flush(as_bytes(make_span(
                                      get_buffer(),
                                      stl::distance(m_buffer.begin(), it))))};
                }
            }
        }
        return {false, 0};
    }

    const_byte_span get_flushable_data()
    {
        auto size = stl::distance(m_buffer.begin(), m_it);
        if (size == 0) {
            return {};
        }
        auto s = make_span(&m_buffer[0], size);
        return as_bytes(s);
    }

private:
    using vector_type = stl::vector<char, Alloc>;

    static vector_type _initialize_buffer(buffer_mode mode,
                                          std::size_t len,
                                          const Alloc& a)
    {
        if (mode == BUFFER_NONE || mode == BUFFER_DEFAULT) {
            return {};
        }
        return vector_type(len, '\0', a);
    }

    const allocator_type& m_alloc;
    vector_type m_buffer;
    typename vector_type::iterator m_it;
    buffer_mode m_mode;
};

using filebuffer = basic_filebuffer<>;
}  // namespace io

#endif  // SPIO_FILEHANDLE_H
