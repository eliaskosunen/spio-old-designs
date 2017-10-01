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

#ifndef SPIO_INSTREAM_H
#define SPIO_INSTREAM_H

#include "config.h"
#include "readable.h"
#include "reader.h"

namespace io {
template <typename Readable>
class basic_instream {
public:
    using readable_type = Readable;
    using reader_type = reader<readable_type>;
    using char_type = typename reader_type::char_type;

    basic_instream(readable_type r)
        : m_readable(std::move(r)), m_reader(m_readable)
    {
    }

    basic_instream(const basic_instream&) = delete;
    basic_instream& operator=(const basic_instream&) = delete;
    basic_instream(basic_instream&&) = default;
    basic_instream& operator=(basic_instream&&) = default;

    virtual ~basic_instream() = default;

    template <typename T>
    bool read(T& elem, reader_options<T> opt = {})
    {
        return m_reader.read(elem, std::move(opt));
    }
    template <typename T>
    bool read(span<T> elem, reader_options<span<T>> opt = {})
    {
        return m_reader.read(std::move(elem), std::move(opt));
    }

    template <typename T>
    bool read_raw(T& elem)
    {
        return m_reader.read_raw(elem);
    }
    template <typename T>
    bool read_raw(span<T> elem)
    {
        return m_reader.read_raw(elem);
    }

    virtual bool get(char_type& ch)
    {
        return m_reader.read_raw(ch);
    }

    template <typename T>
    bool getline(span<T> s, char_type delim = char_type{'\n'})
    {
        return m_reader.getline(s, delim);
    }

    template <typename Element = char_type>
    bool ignore(std::size_t count = 1)
    {
        return m_reader.ignore(count);
    }
    template <typename Element = char_type>
    bool ignore(std::size_t count, char_type delim)
    {
        return m_reader.ignore(count, delim);
    }

    virtual bool eof() const
    {
        return m_reader.eof();
    }

    template <typename... T>
    bool scan(T&&... args);

    template <typename = std::enable_if_t<!std::is_const<reader_type>::value>>
    reader_type& get_reader()
    {
        return m_reader;
    }
    const reader_type& get_reader() const
    {
        return m_reader;
    }

    template <typename = std::enable_if_t<!std::is_const<readable_type>::value>>
    readable_type& get_readable()
    {
        return m_reader.get_readable();
    }
    const readable_type& get_readable() const
    {
        return m_reader.get_readable();
    }

protected:
    readable_type m_readable;
    reader_type m_reader;
};

template <typename CharT>
class basic_file_instream : public basic_instream<basic_readable_file<CharT>> {
    using base_type = basic_instream<basic_readable_file<CharT>>;

public:
    using readable_type = typename base_type::readable_type;
    using reader_type = typename base_type::reader_type;
    using char_type = typename base_type::char_type;

    using basic_instream<basic_readable_file<CharT>>::basic_instream;
    basic_file_instream() : base_type(readable_type{}) {}
    basic_file_instream(file_wrapper file) : base_type(std::move(file)) {}
    basic_file_instream(const char* filename) : base_type(filename) {}
};

template <typename CharT>
class basic_buffer_instream : public basic_instream<basic_readable_buffer<CharT>> {
    using base_type = basic_instream<basic_readable_buffer<CharT>>;

public:
    using readable_type = typename base_type::readable_type;
    using reader_type = typename base_type::reader_type;
    using char_type = typename base_type::char_type;
    using buffer_type = typename readable_type::buffer_type;

    using basic_instream<basic_readable_buffer<CharT>>::basic_instream;
    basic_buffer_instream() : base_type(readable_type{}) {}
    basic_buffer_instream(buffer_type b) : base_type(b) {}
};

using file_instream = basic_file_instream<char>;
using file_winstream = basic_file_instream<wchar_t>;
using buffer_instream = basic_buffer_instream<char>;
using buffer_winstream = basic_buffer_instream<wchar_t>;
}  // namespace io

#include "instream.impl.h"

#endif  // SPIO_INSTREAM_H
