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

#include "depend/span.h"
#include "formatter.h"
#include "stream_base.h"
#include "string_view.h"

namespace spio {
class outstream_base : public virtual stream_base {
public:
    streamsize write(const_byte_span s)
    {
        return do_write(s);
    }
    void flush()
    {
        do_flush();
    }

private:
    virtual streamsize do_write(const_byte_span s) = 0;
    virtual void do_flush() {}
};

class sink_filter {
public:
    streamsize write(outstream_base* stream, const_byte_span s)
    {
        return do_write(stream, s);
    }

private:
    virtual streamsize do_write(outstream_base* stream, const_byte_span s) = 0;

    sink_filter* next{nullptr};
};

template <typename CharT>
class basic_outstream : public outstream_base, public filtered_stream {
public:
    using char_type = CharT;
    using formatter_type = basic_formatter<char_type>;

    template <typename... Args>
    basic_outstream& print(basic_string_view<char_type> f, const Args&... a)
    {
    }

private:
    streamsize do_write(const_byte_span s) override
    {
        auto& ch = filtered_stream::chain();
        return static_cast<sink_filter*>(ch.top())->write(*this, s);
    }
};
#if 0
template <typename CharT>
class basic_outstream : public virtual stream_base {
public:
    using char_type = CharT;
    using formatter_type = basic_formatter<char_type>;

    streamsize write(span<const char_type> s)
    {
        return do_write(s);
    }
    void flush()
    {
        return do_flush();
    }

    const formatter_type& formatter() const
    {
        return m_formatter;
    }

    basic_outstream& write_nl()
    {
        const auto ch = CharT('\n');
        write(make_span(std::addressof(ch), 1));
        return *this;
    }

private:
    virtual streamsize do_write(span<const char_type> s) = 0;
    virtual void do_flush() {}

    formatter_type m_formatter{};
};

using outstream = basic_outstream<char>;
using woutstream = basic_outstream<wchar_t>;

template <typename CharT>
class basic_span_outstream : public basic_outstream<CharT> {
public:
    using char_type = CharT;
    using span_type = span<char_type>;
    using const_span_type = span<const char_type>;
    using iterator = typename span_type::iterator;

    basic_span_outstream(span_type c) : m_cont(c), m_it(m_cont.begin()) {}

    span_type buf() SPIO_NOEXCEPT
    {
        return m_cont;
    }
    const_span_type buf() const SPIO_NOEXCEPT
    {
        return m_cont.as_const_span();
    }

private:
    streamsize do_write(const_span_type s)
    {
        auto space =
            static_cast<std::size_t>(std::distance(m_it, m_cont.end()));
        auto n = std::min(space, s.size());
        m_it = std::copy_n(s.begin(), n, m_it);
        if (SPIO_UNLIKELY(n != s.size())) {
            stream_base::_raise(
                failure{out_of_range,
                        "basic_static_container_outstream::do_write: No "
                        "space left in container"});
        }
        return n;
    }

    span_type m_cont;
    iterator m_it;
};

using span_outstream = basic_span_outstream<char>;
using wspan_outstream = basic_span_outstream<wchar_t>;

namespace detail {
    template <typename Container>
    class basic_container_outstream
        : public basic_outstream<typename Container::value_type> {
    public:
        using container_type = Container;
        using char_type = typename container_type::value_type;
        using iterator = typename container_type::iterator;

        basic_container_outstream() = default;
        basic_container_outstream(container_type& c)
            : m_cont(std::addressof(c)), m_it(m_cont->begin())
        {
        }

        container_type* container() SPIO_NOEXCEPT
        {
            return m_cont;
        }
        const container_type* container() const SPIO_NOEXCEPT
        {
            return m_cont;
        }

    private:
        streamsize do_write(span<const char_type> s) override
        {
            if (m_it == m_cont->end()) {
                if (s.size_us() > m_cont->size()) {
                    try {
                        m_cont->reserve(m_cont->size() + s.size_us());
                    }
                    catch (const std::length_error& ex) {
                        stream_base::_raise(
                            failure{out_of_range,
                                    "basic_container_outstream::do_write: "
                                    "Container grew larger than max_size()"});
                        return -1;
                    }
                }
                append(s.begin(), s.end());
                m_it = m_cont->end();
            }
            else {
                auto dist = std::distance(m_cont->begin(), m_it);
                if (s.size_us() > m_cont->size()) {
                    try {
                        m_cont->reserve(m_cont->size() + s.size_us());
                    }
                    catch (const std::length_error& ex) {
                        stream_base::_raise(
                            failure{out_of_range,
                                    "basic_container_outstream::do_write: "
                                    "Container grew larger than max_size()"});
                        return -1;
                    }
                    m_cont->insert(m_cont->begin() + dist, s.begin(), s.end());
                }
                else {
                    m_cont->insert(m_it, s.begin(), s.end());
                }
                m_it = m_cont->begin() + dist + s.size();
            }
            return s.size();
        }

        template <typename Iterator,
                  typename C = container_type,
                  typename = void>
        struct has_append : std::false_type {
        };
        template <typename Iterator, typename C>
        struct has_append<Iterator,
                          C,
                          decltype(std::declval<C>().append(
                                       std::declval<Iterator>(),
                                       std::declval<Iterator>()),
                                   void())> : std::true_type {
        };

        template <typename Iterator, typename C = container_type>
        auto append(Iterator b, Iterator e) ->
            typename std::enable_if<has_append<Iterator, C>::value>::type
        {
            m_cont->append(b, e);
        }
        template <typename Iterator, typename C = container_type>
        auto append(Iterator b, Iterator e) ->
            typename std::enable_if<!has_append<Iterator, C>::value>::type
        {
            m_cont->insert(m_cont->end(), b, e);
        }

        container_type* m_cont{nullptr};
        iterator m_it;
    };
}  // namespace detail

template <typename CharT, typename Allocator = std::allocator<CharT>>
using basic_vector_outstream =
    detail::basic_container_outstream<std::vector<CharT, Allocator>>;
using vector_outstream = basic_vector_outstream<char>;
using wvector_outstream = basic_vector_outstream<wchar_t>;

namespace detail {
    template <typename CharT, typename Impl>
    class basic_stdio_outstream_base : public basic_outstream<CharT> {
    public:
        using char_type = CharT;

        std::FILE* handle() SPIO_NOEXCEPT
        {
            return static_cast<Impl*>(this)->handle();
        }

    private:
        streamsize do_write(span<const char_type> s) override
        {
            auto b = std::fwrite(s.data(), 1, s.size() * sizeof(char_type),
                                 handle());
            if (SPIO_UNLIKELY(b < s.size() * sizeof(char_type))) {
                if (std::ferror(handle()) != 0) {
                    stream_base::_raise(failure{SPIO_MAKE_ERRNO});
                    return -1;
                }
                SPIO_UNREACHABLE;
            }
            return static_cast<streamsize>(b / sizeof(char_type));
        }

        void do_flush() override
        {
            if (SPIO_UNLIKELY(std::fflush(handle()) != 0)) {
                stream_base::_raise(failure{SPIO_MAKE_ERRNO});
            }
        }
    };
}  // namespace detail

template <typename CharT>
class basic_stdio_outstream
    : public detail::basic_stdio_outstream_base<CharT,
                                                basic_stdio_outstream<CharT>> {
public:
    using char_type = CharT;

    basic_stdio_outstream() = default;
    explicit basic_stdio_outstream(std::FILE* f) : m_file(f) {}

    std::FILE* handle()
    {
        return m_file;
    }

private:
    std::FILE* m_file;
};

using stdio_outstream = basic_stdio_outstream<char>;
using wstdio_outstream = basic_stdio_outstream<wchar_t>;

template <typename CharT, typename... Args>
basic_outstream<CharT>& print(basic_outstream<CharT>& s,
                              basic_string_view<CharT> format,
                              const Args&... a)
{
    auto str = s.formatter().format(format, a...);
    s.write(str);
    return s;
}
template <typename... Args>
outstream& print(outstream& s, string_view format, const Args&... a)
{
    auto str = s.formatter().format(format, a...);
    s.write(str);
    return s;
}
template <typename... Args>
woutstream& print(woutstream& s, wstring_view format, const Args&... a)
{
    auto str = s.formatter().format(format, a...);
    s.write(str);
    return s;
}

template <typename CharT, typename... Args>
basic_outstream<CharT>& println(basic_outstream<CharT>& s,
                                basic_string_view<CharT> format,
                                const Args&... a)
{
    return print(s, format, a...).write_nl();
}
template <typename... Args>
outstream& println(outstream& s, string_view format, const Args&... a)
{
    return print(s, format, a...).write_nl();
}
template <typename... Args>
woutstream& println(woutstream& s, wstring_view format, const Args&... a)
{
    return print(s, format, a...).write_nl();
}

template <typename CharT>
basic_outstream<CharT>& putstr(basic_outstream<CharT>& s,
                               basic_string_view<CharT> str)
{
    s.write(str);
    return s;
}
outstream& putstr(outstream& s, string_view str)
{
    s.write(str);
    return s;
}
woutstream& putstr(woutstream& s, wstring_view str)
{
    s.write(str);
    return s;
}

template <typename CharT>
basic_outstream<CharT>& putln(basic_outstream<CharT>& s,
                              basic_string_view<CharT> str)
{
    return putstr(s, str).write_nl();
}
outstream& putln(outstream& s, string_view str)
{
    return putstr(s, str).write_nl();
}
woutstream& putln(woutstream& s, wstring_view str)
{
    return putstr(s, str).write_nl();
}

template <typename CharT>
basic_outstream<CharT>& putchar(basic_outstream<CharT>& s, CharT c)
{
    s.write(basic_string_view<CharT>(std::addressof(c), 1));
    return s;
}
outstream& putchar(outstream& s, char c)
{
    s.write(string_view(&c, 1));
    return s;
}
woutstream& putchar(woutstream& s, wchar_t c)
{
    s.write(wstring_view(&c, 1));
    return s;
}
#endif
}  // namespace spio

#endif  // SPIO_OUTSTREAM_H
