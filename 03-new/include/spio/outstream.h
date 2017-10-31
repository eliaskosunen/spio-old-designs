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
#include "type.h"
#include "writable.h"
#include "writer_options.h"

namespace io {
template <typename Writable>
class basic_outstream {
public:
    using writable_type = Writable;
    using char_type = typename writable_type::value_type;

    explicit basic_outstream(writable_type w) : m_writable(std::move(w)) {}

    basic_outstream(const basic_outstream&) = delete;
    basic_outstream& operator=(const basic_outstream&) = delete;
    basic_outstream(basic_outstream&&) = default;
    basic_outstream& operator=(basic_outstream&&) = default;

    virtual ~basic_outstream() = default;

    template <typename T,
              typename = std::enable_if_t<!detail::check_string_tag<T>::value>>
    basic_outstream& write(const T& elem, writer_options<T> opt = {})
    {
        if (m_eof) {
            SPIO_THROW(end_of_file, "io::writer::write: EOF reached");
        }
        m_eof = !type<T>::write(*this, elem, std::move(opt));
        return *this;
    }
#ifndef _MSC_VER  // Visual Studio really can't handle templates
    template <typename T,
              typename = std::enable_if_t<detail::check_string_tag<T>::value>>
    basic_outstream& write(T elem, writer_options<T> opt = {})
    {
        if (m_eof) {
            SPIO_THROW(end_of_file, "io::writer::write: EOF reached");
        }
        m_eof =
            !type<detail::string_tag<T>>::write(*this, elem, std::move(opt));
        return *this;
    }
    template <typename T,
              std::size_t N = 0,
              typename = std::enable_if_t<
                  detail::check_string_tag<const T (&)[N], N>::value>>
    basic_outstream& write(const T (&elem)[N],
                           writer_options<const T (&)[N]> opt = {})
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
    template <typename T, span_extent_type N>
    basic_outstream& write_raw(span<T, N> elems)
    {
        if (m_eof) {
            SPIO_THROW(end_of_file, "io::writer::write: EOF reached");
        }
        auto error = m_writable.write(elems);
        if (error.is_eof()) {
            m_eof = true;
        }
        if (error) {
            SPIO_THROW_EC(error);
        }
        return *this;
    }

    basic_outstream& put(char_type ch)
    {
        return write_raw(ch);
    }
    basic_outstream& flush()
    {
        auto error = get_writable().flush();
        if (error) {
            SPIO_THROW_EC(error);
        }
        return *this;
    }

    basic_outstream& ln()
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
        return print(format, args...).ln();
    }
#endif

    template <typename = std::enable_if_t<!std::is_const<writable_type>::value>>
    writable_type& get_writable()
    {
        return m_writable;
    }
    const writable_type& get_writable() const
    {
        return m_writable;
    }

protected:
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

    basic_file_outstream() : base_type(writable_type{}) {}
    explicit basic_file_outstream(writable_type w) : base_type(std::move(w)) {}
    basic_file_outstream(FileHandle file) : base_type({}), m_file(file)
    {
        base_type::m_writable = writable_type{m_file};
    }

private:
    FileHandle m_file{};
};

template <typename CharT, typename BufferT = dynamic_writable_buffer<CharT>>
class basic_buffer_outstream
    : public basic_outstream<basic_writable_buffer<CharT, BufferT>> {
    using base_type = basic_outstream<basic_writable_buffer<CharT, BufferT>>;

public:
    using writable_type = typename base_type::writable_type;
    using char_type = typename base_type::char_type;
    using buffer_type = typename writable_type::buffer_type;

    basic_buffer_outstream() : base_type(writable_type{}) {}
    explicit basic_buffer_outstream(writable_type w) : base_type(std::move(w))
    {
    }
    basic_buffer_outstream(buffer_type b) : base_type(std::move(b)) {}
};

template <typename CharT>
class basic_stdout_outstream
    : public basic_file_outstream<CharT, stdio_filehandle> {
public:
    basic_stdout_outstream()
        : basic_file_outstream<CharT, stdio_filehandle>(stdout)
    {
    }
};
template <typename CharT>
class basic_stderr_outstream
    : public basic_file_outstream<CharT, stdio_filehandle> {
public:
    basic_stderr_outstream()
        : basic_file_outstream<CharT, stdio_filehandle>(stderr)
    {
    }
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

#endif  // SPIO_OUTSTREAM_H
