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
    using vector_type = stl::vector<char, Alloc>;

    /// Buffering mode
    enum buffer_mode {
        BUFFER_DEFAULT,  ///< Default buffering mode, buffering handled by the
                         ///< filehandle
        BUFFER_FULL,  ///< Full buffering, automatic flush when buffer reaches
                      ///< its maximum size
        BUFFER_LINE,  ///< Line buffering, automatic flush when buffer fills up
                      ///< or a newline '\\n' is reached
        BUFFER_NONE   ///< No buffering used here or in the filehandle
    };

#ifdef BUFSIZ
    static constexpr std::size_t default_size = BUFSIZ;
#else
    static constexpr std::size_t default_size =
        4096;  // 4k is the memory page size on many architectures
#endif

    basic_filebuffer(const Alloc& alloc)
        : m_alloc(alloc),
          m_buffer{_initialize_buffer(BUFFER_FULL, default_size, m_alloc)},
          m_it{m_buffer.begin()},
          m_begin{m_it},
          m_mode{BUFFER_FULL}
    {
    }

    basic_filebuffer(buffer_mode mode = BUFFER_FULL,
                     // Avoid ODR-using to not get a linker error on C++14
                     std::size_t len = std::size_t{default_size},
                     const Alloc& alloc = Alloc{})
        : m_alloc(alloc),
          m_buffer{_initialize_buffer(mode, len, m_alloc)},
          m_it{m_buffer.begin()},
          m_begin{m_it},
          m_mode(mode)
    {
    }

    basic_filebuffer(const basic_filebuffer&) = delete;
    basic_filebuffer& operator=(const basic_filebuffer&) = delete;
    basic_filebuffer(basic_filebuffer&&) = default;
    basic_filebuffer& operator=(basic_filebuffer&&) = default;
    ~basic_filebuffer() noexcept = default;

    /**
     * Get a pointer to the beginning of the buffer sized `size()`.
     * Due to implementation details, to get properly sized buffer use
     * `get_flushable_data()`. Requires: `size() != 0 && is_writable_mode()`
     */
    char* get_buffer()
    {
        assert(size() != 0 && is_writable_mode());
        return &m_buffer[0];
    }
    const char* get_buffer() const
    {
        assert(size() != 0 && is_writable_mode());
        return &m_buffer[0];
    }

    /// Get the size of the buffer
    auto size() const
    {
        return m_buffer.size();
    }

    /// Get buffering mode
    constexpr auto mode() const
    {
        return m_mode;
    }

    /// Can buffer be written to.
    /// Equivalent to `mode() != BUFFER_DEFAULT && mode() != BUFFER_NONE`
    constexpr bool is_writable_mode() const
    {
        return mode() != BUFFER_DEFAULT && mode() != BUFFER_NONE;
    }

    /**
     * Write to the buffer, and flush if needed in the process.
     * \param data  Data to write
     * \param flush Flush function, must be able to be invoked with argument
     *              type `const_byte_span` and yield a result convertible
     *              to `std::size_t`; `is_invocable_r_v<std::size_t,
     *              decltype(flush), const_byte_span>` must evaluate to `true`
     * \return Bytes processed, either written to the buffer or flushed
     *
     * Requires `is_writable_mode()`
     */
    template <typename FlushFn>
    std::size_t write(const_byte_span data, FlushFn&& flush)
    {
        assert(is_writable_mode());
#if SPIO_HAS_INVOCABLE
        static_assert(std::is_invocable_r_v<std::size_t, decltype(flush),
                                            const_byte_span>,
                      "filebuffer::write: flush must be Invocable with type "
                      "std::size_t(io::const_byte_span)");
#endif
        // Check if data fits in the buffer
        auto dist_till_end = stl::distance(m_it, m_buffer.end());
        if (dist_till_end >= data.size()) {
            // If it does, copy the data to the buffer
            m_it = stl::copy(data.begin(), data.end(), m_it);
            flush_if_needed(flush);
            return data.size_us();
        }

        // Data wouldn't fit in one go, copy what fits
        m_it = stl::copy(data.begin(), data.begin() + dist_till_end, m_it);
        assert(m_it == m_buffer.end());
        m_it = m_buffer.begin();

        // Flush the buffer
        auto s = as_bytes(make_span(m_begin, m_buffer.end()));
        auto flushed = flush(s);
        if (flushed != s.size_us()) {
            // Not everything was flushed, move the begin pointer accordingly
            m_begin += static_cast<std::ptrdiff_t>(flushed);
            // Copy everything that was not flushed to the beginning of the
            // buffer
            m_it = stl::copy(m_it + static_cast<std::ptrdiff_t>(flushed),
                             m_buffer.end(), m_it);

            // Copy the rest of the data to the buffer
            auto dist = stl::distance(m_it, m_buffer.end());
            if (dist >= data.size() - dist_till_end) {
                m_it =
                    stl::copy(data.begin() + dist_till_end, data.end(), m_it);
                flushed += data.size_us();
            }
            else {
                // If all of it does not fit, copy everything that does
                m_it = stl::copy(data.begin() + dist_till_end,
                                 data.begin() + dist_till_end + dist, m_it);
                flushed += static_cast<std::size_t>(dist);
            }
            return flushed;
        }

        // For our intents, the buffer is now empty.
        // Move the pointers to the beginning
        m_begin = m_it = m_buffer.begin();

        // Take what was not yet written and write it recursively
        s = as_bytes(data.subspan(dist_till_end));
        auto written = write(s, flush);
        if (written != s.size_us()) {
            // Not everything was written successfully, copy unwritten data to
            // the beginning
            m_it = stl::copy(m_it + static_cast<std::ptrdiff_t>(written),
                             m_buffer.end(), m_it);
            flush_if_needed(flush);
            return static_cast<std::size_t>(dist_till_end) + written;
        }

        flush_if_needed(flush);
        assert(written + static_cast<std::size_t>(dist_till_end) ==
               data.size_us());
        return data.size_us();
    }

    /**
     * Flush the buffer if necessary.
     * Flushes the buffer if:
     *  - it is full
     *  - `mode() == BUFFER_LINE` and a newline was found`
     *
     * \param  flush Flush function, for requirements see `write()`
     * \return Pair of `bool` and `size_t`:
     *         {did a flush happen, how many bytes were flushed}
     */
    template <typename FlushFn>
    std::pair<bool, std::size_t> flush_if_needed(FlushFn&& flush)
    {
        assert(is_writable_mode());
#if SPIO_HAS_INVOCABLE
        static_assert(
            std::is_invocable_r_v<std::size_t, decltype(flush),
                                  const_byte_span>,
            "filebuffer::flush_if_needed: flush must be Invocable with type "
            "std::size_t(io::const_byte_span)");
#endif
        if (m_it == m_buffer.end()) {
            // If buffer is full, flush all of it
            m_begin = m_it = m_buffer.begin();
            return {true, flush(as_bytes(make_span(m_begin, m_it)))};
        }
        if (m_mode == BUFFER_LINE && m_it != m_begin) {
            // If we're doing line buffering, check for newlines
            auto doit = [&](auto it) -> std::pair<bool, std::size_t> {
                auto begin = m_begin;
                m_begin = it + 1;
                return {true, flush(as_bytes(make_span(begin, it + 1)))};
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
            for (auto it = m_it; it-- != m_begin;) {
                if (*it == '\n') {
                    return ret(doit(it));
                }
            }
        }
        return {false, 0};
    }

    /**
     * Get data written to the buffer that can be (manually) flushed.
     * Remember to call `flag_flushed()` accordingly
     */
    const_byte_span get_flushable_data()
    {
        return as_bytes(make_span(m_begin, m_it));
    }

    /**
     * Flag part of the buffer as flushed.
     * Must be used if data with `get_flushable_data()` is flushed
     * \param bytes_flushed Count of bytes that was flushed, 0 marks that the
     *                      whole buffer was flushed
     */
    void flag_flushed(std::size_t bytes_flushed = 0)
    {
        if (bytes_flushed == 0) {
            m_begin = m_it;
        }
        else {
            m_begin += static_cast<std::ptrdiff_t>(bytes_flushed);
            assert(m_begin <= m_it);
        }
    }

private:
    static vector_type _initialize_buffer(buffer_mode mode,
                                          std::size_t len,
                                          const Alloc& a)
    {
        if (mode == BUFFER_DEFAULT || mode == BUFFER_NONE) {
            return vector_type(a);
        }
        return vector_type(len, '\0', a);
    }

    const allocator_type& m_alloc;
    vector_type m_buffer;
    /// Iterator pointing to the next writable byte of the buffer.
    /// If `m_it == m_buffer.end()`, buffer is full and a flush is needed.
    typename vector_type::iterator m_it;
    /// Iterator pointing to the first non-flushed element.
    /// Always satisfies `m_begin <= m_it`.
    /// This means, that the data returned by `get_flushable_data()` is always
    /// `(m_begin, m_it]`
    typename vector_type::iterator m_begin;
    buffer_mode m_mode;
};

using filebuffer = basic_filebuffer<>;
}  // namespace io

#endif  // SPIO_FILEHANDLE_H
