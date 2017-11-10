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

#ifndef SPIO_BUFFERING_H
#define SPIO_BUFFERING_H

#include <cstdio>
#include "span.h"

namespace io {
class filebuffer {
public:
    enum buffer_mode { BUFFER_FULL, BUFFER_LINE, BUFFER_NONE };

#ifdef BUFSIZ
    static constexpr std::size_t default_size = BUFSIZ;
#else
    static constexpr std::size_t default_size = 8192;
#endif

    filebuffer(buffer_mode mode = BUFFER_FULL, std::size_t len = default_size)
        : m_buffer{_initialize_buffer(mode, len)},
          m_it{m_buffer.begin()},
          m_mode(mode)
    {
    }

    filebuffer(const filebuffer&) = delete;
    filebuffer& operator=(const filebuffer&) = delete;
    filebuffer(filebuffer&&) = default;
    filebuffer& operator=(filebuffer&&) = default;
    ~filebuffer() noexcept = default;

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
        assert(m_mode != BUFFER_NONE);
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
        assert(m_mode != BUFFER_NONE);
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
    static stl::vector<char> _initialize_buffer(buffer_mode mode,
                                                std::size_t len)
    {
        if (mode == BUFFER_NONE) {
            return {};
        }
        return stl::vector<char>(len, '\0');
    }

    stl::vector<char> m_buffer;
    decltype(m_buffer)::iterator m_it;
    buffer_mode m_mode;
};
}  // namespace io

#endif  // SPIO_FILEHANDLE_H
