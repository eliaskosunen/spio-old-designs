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

#ifndef SPIO_OUTSTREAM_H
#define SPIO_OUTSTREAM_H

#include "config.h"
#include "fmt.h"
#include "writable.h"
#include "writer.h"

namespace io {
template <typename Writable>
class basic_outstream {
public:
    using writable_type = Writable;
    using writer_type = writer<writable_type>;
    using char_type = typename writer_type::char_type;

    basic_outstream(writable_type w)
        : m_writable(std::move(w)), m_writer(m_writable)
    {
    }

    basic_outstream(const basic_outstream&) = delete;
    basic_outstream& operator=(const basic_outstream&) = delete;
    basic_outstream(basic_outstream&&) = default;
    basic_outstream& operator=(basic_outstream&&) = default;

    virtual ~basic_outstream() = default;

    template <typename T,
              typename = std::enable_if_t<!detail::check_string_tag<T>::value>>
    basic_outstream& write(const T& elem, writer_options<T> opt = {})
    {
        m_writer.write(elem, std::move(opt));
        return *this;
    }
    template <typename T,
              typename = std::enable_if_t<detail::check_string_tag<T>::value>>
    basic_outstream& write(T elem, writer_options<T> opt = {})
    {
        m_writer.write(elem, std::move(opt));
        return *this;
    }
    template <typename T,
              std::size_t N = 0,
              typename = std::enable_if_t<
                  detail::check_string_tag<const T (&)[N], N>::value>>
    basic_outstream& write(const T (&elem)[N],
                           writer_options<const T (&)[N]> opt = {})
    {
        m_writer.write(elem, std::move(opt));
        return *this;
    }

    template <typename T>
    basic_outstream& write_raw(const T& elem)
    {
        m_writer.write_raw(elem);
        return *this;
    }

    virtual basic_outstream& put(char_type ch)
    {
        m_writer.write_raw(ch);
        return *this;
    }
    virtual basic_outstream& flush()
    {
        m_writer.flush();
        return *this;
    }

    virtual basic_outstream& ln()
    {
        put(static_cast<char_type>('\n'));
        return *this;
    }

#if SPIO_USE_FMT
    template <typename... Args>
    basic_outstream& print(const char* format, const Args&... args);

    template <typename... Args>
    basic_outstream& println(const char* format, const Args&... args)
    {
        return print(format, args...).ln();
    }
#endif

    template <typename = std::enable_if_t<!std::is_const<writer_type>::value>>
    writable_type& get_writable()
    {
        return m_writable;
    }
    const writable_type& get_writable() const
    {
        return m_writable;
    }

    template <typename = std::enable_if_t<!std::is_const<writer_type>::value>>
    writer_type& get_writer()
    {
        return m_writer;
    }
    const writer_type& get_writer() const
    {
        return m_writer;
    }

protected:
    writable_type m_writable;
    writer_type m_writer;
};

template <typename CharT>
class basic_file_outstream
    : public basic_outstream<basic_writable_file<CharT>> {
    using base_type = basic_outstream<basic_writable_file<CharT>>;

public:
    using writable_type = typename base_type::writable_type;
    using writer_type = typename base_type::writer_type;
    using char_type = typename base_type::char_type;

    using basic_outstream<basic_writable_file<CharT>>::basic_outstream;
    basic_file_outstream() : base_type(writable_type{}) {}
    basic_file_outstream(stdio_filehandle file)
        : base_type({}), m_file(std::move(file))
    {
        base_type::m_writable = writable_type{&m_file};
    }

private:
    stdio_filehandle m_file{};
};

template <typename CharT, typename BufferT = dynamic_writable_buffer<CharT>>
class basic_buffer_outstream
    : public basic_outstream<basic_writable_buffer<CharT, BufferT>> {
    using base_type = basic_outstream<basic_writable_buffer<CharT, BufferT>>;

public:
    using writable_type = typename base_type::writable_type;
    using writer_type = typename base_type::writer_type;
    using char_type = typename base_type::char_type;
    using buffer_type = typename writable_type::buffer_type;

    using basic_outstream<
        basic_writable_buffer<CharT, BufferT>>::basic_outstream;
    basic_buffer_outstream() : base_type(writable_type{}) {}
    basic_buffer_outstream(buffer_type b) : base_type(b) {}
};

template <typename CharT>
class basic_stdout_outstream : public basic_file_outstream<CharT> {
public:
    basic_stdout_outstream() : basic_file_outstream<CharT>(stdout) {}
};
template <typename CharT>
class basic_stderr_outstream : public basic_file_outstream<CharT> {
public:
    basic_stderr_outstream() : basic_file_outstream<CharT>(stderr) {}
};

using file_outstream = basic_file_outstream<char>;
using file_woutstream = basic_file_outstream<wchar_t>;
using buffer_outstream = basic_buffer_outstream<char>;
using buffer_woutstream = basic_buffer_outstream<wchar_t>;

using sout = basic_stdout_outstream<char>;
using wsout = basic_stdout_outstream<wchar_t>;
using serr = basic_stderr_outstream<char>;
using wserr = basic_stderr_outstream<wchar_t>;
}  // namespace io

#include "outstream.impl.h"

#endif  // SPIO_INSTREAM_H