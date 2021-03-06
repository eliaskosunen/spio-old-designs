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

#include "config.h"
#include "fmt.h"
#include "type.h"
#include "writable.h"
#include "writer_options.h"

namespace io {
template <typename Writable>
class basic_outstream {
public:
    using writable_type = Writable;
    using char_type = typename writable_type::value_type;

    static_assert(
        is_writable<Writable>::value,
        "basic_outstream<T>: T does not satisfy the requirements of Writable");

    explicit basic_outstream(writable_type w) : m_writable(std::move(w)) {}

    template <typename T>
    std::enable_if_t<!detail::check_string_tag<T>::value, basic_outstream>&
    write(const T& elem, writer_options<T> opt = {})
    {
        if (m_eof) {
            SPIO_THROW(end_of_file, "io::writer::write: EOF reached");
        }
        m_eof = !type<T>::write(*this, elem, std::move(opt));
        return *this;
    }
#ifndef _MSC_VER  // Visual Studio really can't handle templates
    template <typename T>
    std::enable_if_t<detail::check_string_tag<T>::value, basic_outstream>&
    write(T elem, writer_options<T> opt = {})
    {
        if (m_eof) {
            SPIO_THROW(end_of_file, "io::writer::write: EOF reached");
        }
        m_eof =
            !type<detail::string_tag<T>>::write(*this, elem, std::move(opt));
        return *this;
    }
    template <typename T, std::size_t N = 0>
    std::enable_if_t<detail::check_string_tag<const T (&)[N], N>::value,
                     basic_outstream>&
    write(const T (&elem)[N], writer_options<const T (&)[N]> opt = {})
    {
        if (m_eof) {
            SPIO_THROW(end_of_file, "io::writer::write: EOF reached");
        }
        m_eof = !type<detail::string_tag<const T(&)[N], N>>::write(
            *this, elem, std::move(opt));
        return *this;
    }
#endif

    template <typename T>
    basic_outstream& write_raw(const T& elem)
    {
        return write_raw(make_span<1>(&elem));
    }
    template <typename T, extent_t N>
    basic_outstream& write_raw(span<T, N> elems)
    {
        if (m_eof) {
            SPIO_THROW(end_of_file, "io::writer::write: EOF reached");
        }
        if (auto e = m_writable.write(elems)) {
            if (is_eof(e)) {
                m_eof = true;
                return *this;
            }
            SPIO_THROW_EC(e);
        }
        return *this;
    }

    basic_outstream& put(char_type ch)
    {
        return write_raw(ch);
    }
    basic_outstream& flush()
    {
        if (auto e = get_writable().flush()) {
            SPIO_THROW_EC(e);
        }
        return *this;
    }

    basic_outstream& nl()
    {
        put(static_cast<char_type>('\n'));
        return *this;
    }

    bool eof() const
    {
        return m_eof;
    }
    template <typename Then>
    auto eof_then(Then&& f)
    {
        return f(eof(), *this);
    }

    operator bool() const
    {
        return !eof();
    }

#if SPIO_USE_FMT
    template <typename... Args>
    basic_outstream& print(const char* format, const Args&... args);

    template <typename... Args>
    basic_outstream& println(const char* format, const Args&... args)
    {
        return print(format, args...).nl();
    }
#endif

    std::enable_if_t<!std::is_const<writable_type>::value, writable_type>&
    get_writable()
    {
        return m_writable;
    }
    const writable_type& get_writable() const
    {
        return m_writable;
    }

protected:
    basic_outstream() = default;

    writable_type m_writable{};
    bool m_eof{false};
};

#if SPIO_HAS_DEDUCTION_GUIDES
template <typename Writable>
basic_outstream(Writable& r)->basic_outstream<Writable>;
#endif

template <typename CharT, typename FileHandle = filehandle>
class basic_file_outstream
    : public basic_outstream<basic_writable_file<CharT, FileHandle>> {
    using base_type = basic_outstream<basic_writable_file<CharT, FileHandle>>;

public:
    using writable_type = typename base_type::writable_type;
    using char_type = typename base_type::char_type;

    basic_file_outstream() = default;
    explicit basic_file_outstream(writable_type w) : base_type(std::move(w)) {}
    explicit basic_file_outstream(FileHandle& file)
        : basic_file_outstream(writable_type{file})
    {
    }
};

template <typename CharT, typename BufferT = dynamic_writable_buffer<CharT>>
class basic_buffer_outstream
    : public basic_outstream<basic_writable_buffer<CharT, BufferT>> {
    using base_type = basic_outstream<basic_writable_buffer<CharT, BufferT>>;

public:
    using writable_type = typename base_type::writable_type;
    using char_type = typename base_type::char_type;
    using buffer_type = typename writable_type::buffer_type;

    basic_buffer_outstream() = default;
    explicit basic_buffer_outstream(writable_type w) : base_type(std::move(w))
    {
    }
    explicit basic_buffer_outstream(buffer_type b)
        : basic_buffer_outstream(writable_type{std::move(b)})
    {
    }

    auto get_buffer()
    {
        return base_type::m_writable.get_buffer().to_span();
    }
    auto get_buffer() const
    {
        return base_type::m_writable.get_buffer().to_span();
    }
    auto&& consume_buffer()
    {
        return base_type::m_writable.consume_buffer();
    }
};

using file_outstream = basic_file_outstream<char>;
using file_woutstream = basic_file_outstream<wchar_t>;
using buffer_outstream = basic_buffer_outstream<char>;
using buffer_woutstream = basic_buffer_outstream<wchar_t>;

#if SPIO_USE_THREADING
using mt_file_outstream = basic_lockable_stream<file_outstream>;
using mt_file_woutstream = basic_lockable_stream<file_woutstream>;
using mt_buffer_outstream = basic_lockable_stream<buffer_outstream>;
using mt_buffer_woutstream = basic_lockable_stream<buffer_woutstream>;
#endif

static_assert(is_writer<file_outstream>::value,
              "file_outstream does not satisfy the requirements of Writer");
static_assert(is_writer<file_woutstream>::value,
              "file_woutstream does not satisfy the requirements of Writer");
static_assert(is_writer<buffer_outstream>::value,
              "buffer_outstream does not satisfy the requirements of Writer");
static_assert(is_writer<buffer_woutstream>::value,
              "buffer_woutstream does not satisfy the requirements of Writer");

#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

template <typename T>
auto& get_stdout()
{
    static auto f = stdio_filehandle{stdout};
    static auto s = basic_file_outstream<T, stdio_filehandle>{f};
    return s;
}
template <typename T>
auto& get_stderr()
{
    static auto f = stdio_filehandle{filebuffer::BUFFER_NONE, stderr};
    static auto s = basic_file_outstream<T, stdio_filehandle>{f};
    return s;
}
template <typename T>
auto& get_stdlog()
{
    static auto f = stdio_filehandle{stderr};
    static auto s = basic_file_outstream<T, stdio_filehandle>{f};
    return s;
}

#if defined(__clang__)
#pragma GCC diagnostic pop
#endif

inline auto& sout()
{
    return get_stdout<char>();
}
inline auto& serr()
{
    return get_stderr<char>();
}
inline auto& slog()
{
    return get_stdlog<char>();
}
inline auto& wsout()
{
    return get_stdout<wchar_t>();
}
inline auto& wserr()
{
    return get_stderr<wchar_t>();
}
inline auto& wslog()
{
    return get_stdlog<wchar_t>();
}

#if SPIO_USE_THREADING
#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

template <typename T>
auto& get_mt_stdout()
{
    static auto f = stdio_filehandle{stdout};
    static auto s = basic_file_outstream<T, stdio_filehandle>{f};
    static basic_lockable_stream<decltype(s)> mt{std::move(s)};
    return mt;
}
template <typename T>
auto& get_mt_stderr()
{
    static auto f = stdio_filehandle{filebuffer::BUFFER_NONE, stderr};
    static auto s = basic_file_outstream<T, stdio_filehandle>{f};
    static basic_lockable_stream<decltype(s)> mt{std::move(s)};
    return mt;
}
template <typename T>
auto& get_mt_stdlog()
{
    static auto f = stdio_filehandle{stderr};
    static auto s = basic_file_outstream<T, stdio_filehandle>{f};
    static basic_lockable_stream<decltype(s)> mt{std::move(s)};
    return mt;
}

#if defined(__clang__)
#pragma GCC diagnostic pop
#endif

inline auto& mt_sout()
{
    return get_mt_stdout<char>();
}
inline auto& mt_wsout()
{
    return get_mt_stdout<wchar_t>();
}
inline auto& mt_serr()
{
    return get_mt_stderr<char>();
}
inline auto& mt_wserr()
{
    return get_mt_stderr<wchar_t>();
}
inline auto& mt_slog()
{
    return get_mt_stdlog<char>();
}
inline auto& mt_wslog()
{
    return get_mt_stdlog<wchar_t>();
}
#endif
}  // namespace io

#include "outstream.impl.h"

#endif  // SPIO_OUTSTREAM_H
