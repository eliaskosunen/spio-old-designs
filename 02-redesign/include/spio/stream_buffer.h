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

#ifndef SPIO_BUFFERED_DEVICE_H
#define SPIO_BUFFERED_DEVICE_H

#include "fwd.h"

#include <cstdio>
#include <vector>
#include "error.h"
#include "span.h"
#include "traits.h"

namespace spio {
template <typename Buffer>
class basic_sink_buffer {
public:
    using buffer_type = Buffer;
    using value_type = typename buffer_type::value_type;
    using iterator = typename buffer_type::iterator;
    using span_type = span<const value_type>;

    enum class mode { external, full, line, none };

#ifdef BUFSIZ
    static constexpr std::size_t default_size = BUFSIZ;
#else
    static constexpr std::size_t default_size =
        4096;  // 4k is the memory page size on many architectures
#endif

    basic_sink_buffer(buffer_type b)
        : m_buffer(std::move(b)),
          m_it(m_buffer.begin()),
          m_begin(m_it),
          m_mode(mode::full)
    {
    }
    basic_sink_buffer(mode m = mode::line, std::size_t len = default_size)
        : m_buffer(_init_buffer(m, len)),
          m_it(m_buffer.begin()),
          m_begin(m_it),
          m_mode(m)
    {
    }

    value_type* get_buffer()
    {
        SPIO_ASSERT(size() != 0 && is_writable_mode(),
                    "basic_device_buffer<>::get_buffer: requires `size() != 0 "
                    "&& is_writable_mode()`");
        return m_buffer.data();
    }
    const value_type* get_buffer() const
    {
        SPIO_ASSERT(size() != 0 && is_writable_mode(),
                    "basic_device_buffer<>::get_buffer: requires `size() != 0 "
                    "&& is_writable_mode()`");
        return m_buffer.data();
    }

    auto size() const
    {
        return m_buffer.size();
    }
    constexpr auto mode() const
    {
        return m_mode;
    }

    constexpr bool is_writable_mode() const
    {
        return mode() != mode::external && mode() != mode::none;
    }

    template <typename FlushFn>
    streamsize write(span_type data, FlushFn&& flush)
    {
        SPIO_ASSERT(
            is_writable_mode(),
            "basic_device_buffer<>::write: requires `is_writable_mode()`");
#if SPIO_HAS_INVOCABLE
        static_assert(
            std::is_invocable_r_v<streamsize, FlushFn, span<const value_type>>,
            "basic_device_buffer<>::write: flush must be Invocable with "
            "parameter type span_type");
#endif

        // Check if data fits in the buffer
        auto dist_till_end = std::distance(m_it, m_buffer.end());
        if (dist_till_end >= data.size()) {
            // If it does, copy the data to the buffer
            m_it = std::copy(data.begin(), data.end(), m_it);
            flush_if_needed(flush);
            return data.size();
        }

        // Data wouldn't fit in one go, copy what fits
        m_it = std::copy(data.begin(), data.begin() + dist_till_end, m_it);
        assert(m_it == m_buffer.end());
        m_it = m_buffer.begin();

        // Flush the buffer
        auto s = span_type(m_begin, m_buffer.end());
        auto flushed = flush(s);
        if (flushed != s.size()) {
            // Not everything was flushed, move the begin pointer accordingly
            m_begin += flushed;
            // Copy everything that was not flushed to the beginning of the
            // buffer
            m_it = std::copy(m_it + flushed, m_buffer.end(), m_it);

            // Copy the rest of the data to the buffer
            auto dist = std::distance(m_it, m_buffer.end());
            if (dist >= data.size() - dist_till_end) {
                m_it =
                    std::copy(data.begin() + dist_till_end, data.end(), m_it);
                flushed += data.size_us();
            }
            else {
                // If all of it does not fit, copy everything that does
                m_it = std::copy(data.begin() + dist_till_end,
                                 data.begin() + dist_till_end + dist, m_it);
                flushed += static_cast<std::size_t>(dist);
            }
            return flushed;
        }

        // For our intents, the buffer is now empty.
        // Move the pointers to the beginning
        m_begin = m_it = m_buffer.begin();

        // Take what was not yet written and write it recursively
        s = data.subspan(dist_till_end);
        auto written = write(s, flush);
        if (written != s.size()) {
            // Not everything was written successfully, copy unwritten data to
            // the beginning
            m_it = std::copy(m_it + static_cast<std::ptrdiff_t>(written),
                             m_buffer.end(), m_it);
            flush_if_needed(flush);
            return dist_till_end + written;
        }

        flush_if_needed(flush);
        assert(written + dist_till_end == data.size());
        return data.size();
    }

    template <typename FlushFn>
    streamsize flush_if_needed(FlushFn&& flush)
    {
        SPIO_ASSERT(is_writable_mode(),
                    "basic_device_buffer<>::flush_if_needed: requires "
                    "`is_writable_mode()`");
#if SPIO_HAS_INVOCABLE
        static_assert(
            std::is_invocable_r_v<streamsize, FlushFn, span<const value_type>>,
            "basic_device_buffer<>::flush_if_needed: flush must be Invocable "
            "with parameter type span_type");
#endif

        if (m_it == m_buffer.end()) {
            // If buffer is full, flush all of it
            m_begin = m_it = m_buffer.begin();
            return flush(make_span(m_begin, m_it));
        }
        if (m_mode == mode::full && m_it != m_begin) {
            // If we're doing line buffering, check for newlines
            auto doit = [&](auto it) {
                auto begin = m_begin;
                m_begin = ++it;
                return flush(make_span(begin, it));
            };
            auto ret = [&](auto r) {
                if (m_begin == m_buffer.end()) {
                    m_begin = m_it = m_buffer.begin();
                }
                return r;
            };
            if (*m_it == '\n') {
                return ret(doit(m_it));
            }
            for (auto it = m_it; it != m_begin; --it) {
                if (*it == '\n') {
                    return ret(doit(it));
                }
            }
        }
        return -1;
    }

    span_type get_flushable_data()
    {
        return {m_begin, m_it};
    }

    void flag_flushed(streamsize elements)
    {
        if (elements == -1) {
            m_begin = m_it;
        }
        else {
            m_begin += elements;
            SPIO_ASSERT(
                m_begin <= m_it,
                "basic_device_buffer<>::flag_flushed: Begin pointer must "
                "be less or equal to end pointer; was the value "
                "`elements` too big?");
        }
    }

private:
    static buffer_type _init_buffer(enum mode m, std::size_t len)
    {
        if (m == mode::external || m == mode::none) {
            return {};
        }
        return buffer_type(len, '\0');
    }

    buffer_type m_buffer;
    iterator m_it;
    iterator m_begin;
    enum mode m_mode;
};

template <typename Buffer>
class basic_source_buffer {
public:
    using buffer_type = Buffer;
    using value_type = typename buffer_type::value_type;
    using push_span_type = span<const value_type>;
    using read_span_type = span<value_type>;

    basic_source_buffer() = default;
    basic_source_buffer(buffer_type b) : m_buffer(std::move(b)) {}

    std::size_t size() const
    {
        return m_buffer.size();
    }

    void read(read_span_type s)
    {
        SPIO_ASSERT(s.size_us() <= size(),
                    "source_buffer::read: Cannot read more than size()");

        std::copy_n(m_buffer.begin(), s.size(), s.begin());
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + s.size());
    }

    void push(push_span_type s)
    {
        m_buffer.insert(m_buffer.end(), s.begin(), s.end());
    }

private:
    buffer_type m_buffer{};
};
}  // namespace spio

#endif
