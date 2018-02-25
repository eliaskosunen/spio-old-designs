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

#ifndef SPIO_OUTSTREAM_H
#define SPIO_OUTSTREAM_H

#include <memory>
#include "buffered_device.h"
#include "config.h"
#include "formatter.h"
#include "stream_base.h"

namespace spio {
template <typename CharT,
          typename Formatter = basic_default_formatter<CharT>,
          typename Buffer = basic_default_device_buffer<CharT>,
          typename Traits = std::char_traits<CharT>>
class basic_outstream {
public:
    using char_type = CharT;
    using formatter_type = Formatter;
    using traits_type = Traits;
    using buffer_type = Buffer;

    virtual basic_outstream& print(
        const char_type* f,
        fmt::basic_format_args<fmt::buffer_context<char_type>> args) = 0;
    virtual void write(span<const CharT> s) = 0;
    virtual void flush() = 0;

protected:
    basic_outstream() = default;
    basic_outstream(std::unique_ptr<buffer_type> b) : m_buffer(std::move(b)) {}

    void vprint(const char_type* f,
                fmt::basic_format_args<fmt::buffer_context<char_type>> args)
    {
        auto str = m_fmt.vformat(f, args);
        write(make_span(str));
    }

    std::unique_ptr<buffer_type> m_buffer{nullptr};
    formatter_type m_fmt{};
};

template <typename Sink,
          typename Derived,
          typename Formatter =
              basic_default_formatter<typename Sink::char_type>,
          typename Buffer = basic_default_formatter<typename Sink::char_type>,
          typename Traits = std::char_traits<typename Sink::char_type>,
          typename = std::enable_if_t<has_category<Sink, output>::value>>
class basic_sink_outstream : public basic_outstream<typename Sink::char_type,
                                                    Formatter,
                                                    Buffer,
                                                    Traits> {
    using base =
        basic_outstream<typename Sink::char_type, Formatter, Buffer, Traits>;
    using child = Derived;

public:
    using typename base::buffer_type;
    using typename base::char_type;
    using typename base::formatter_type;
    using typename base::traits_type;
    using sink_type = Sink;

    template <typename... Args>
    child& print(const char_type* f, const Args&... a)
    {
        return print(f, fmt::make_args(a...));
    }
    child& print(const char_type* f,
                 fmt::basic_format_args<fmt::buffer_context<char_type>> args)
    {
        base::vprint(f, std::move(args));
    }

    void write(span<const char_type> s)
    {
        m_sink.write(s);
    }
    void flush() {}

protected:
    basic_sink_outstream(base b, sink_type s)
        : base(std::move(b)), m_sink(std::move(s))
    {
    }

    sink_type m_sink;
};
}  // namespace spio

#endif
