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
#include "reader_options.h"
#include "type.h"

namespace io {
template <typename Readable>
class basic_instream {
public:
    using readable_type = Readable;
    using char_type = typename readable_type::value_type;

    basic_instream(readable_type r) : m_readable(std::move(r)) {}

    basic_instream(const basic_instream&) = delete;
    basic_instream& operator=(const basic_instream&) = delete;
    basic_instream(basic_instream&&) = default;
    basic_instream& operator=(basic_instream&&) = default;

    virtual ~basic_instream() = default;

    template <typename T>
    basic_instream& read(T& elem, reader_options<T> opt = {})
    {
        if (eof()) {
            SPIO_THROW(end_of_file, "io::reader::read: EOF reached");
        }
        m_eof = !type<T>::read(*this, elem, std::move(opt));
        return *this;
    }
    template <typename T, span_extent_type N>
    basic_instream& read(span<T, N> elem, reader_options<span<T, N>> opt = {})
    {
        if (eof()) {
            SPIO_THROW(end_of_file, "io::reader::read: EOF reached");
        }
        m_eof = !type<span<T, N>>::read(*this, elem, std::move(opt));
        return *this;
    }

    template <typename T>
    basic_instream& read_raw(T& elem)
    {
        return read_raw(make_span<1>(&elem));
    }
    template <typename T, span_extent_type N>
    basic_instream& read_raw(span<T, N> elems)
    {
        if (eof()) {
            SPIO_THROW(end_of_file, "io::reader::read_raw: EOF reached");
        }
        auto error = _read(elems, elements{elems.length()});
        if (error.is_eof()) {
            m_eof = true;
        }
        if (error) {
            SPIO_THROW_EC(error);
        }
        return *this;
    }

    virtual basic_instream& get(char_type& ch)
    {
        return read_raw(ch);
    }

    template <typename T, span_extent_type N>
    basic_instream& getline(span<T, N> s, char_type delim = char_type{'\n'})
    {
        reader_options<span<T, N>> opt = {make_span<1>(&delim)};
        return read(s, opt);
    }

    template <typename ElementT = char_type>
    basic_instream& ignore(std::size_t count = 1)
    {
        SPIO_ASSERT(sizeof(ElementT) * count % sizeof(char_type) == 0,
                    "sizeof(ElementT) * count must be divisible by "
                    "sizeof(char_type) in reader::ignore");
        vector<char_type> arr(sizeof(ElementT) * count / sizeof(char_type));
        return read_raw(make_span(arr));
    }
    template <typename Element = char_type>
    basic_instream& ignore(std::size_t count, char_type delim)
    {
        char_type ch{};
        for (std::size_t i = 0; i < count; ++i) {
            if (!get(ch)) {
                return false;
            }
            if (ch == delim) {
                break;
            }
        }
        return *this;
    }

    template <typename T>
    void push(T elem);

    virtual bool eof() const
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

    template <typename... T>
    basic_instream& scan(T&&... args);

    template <typename = std::enable_if_t<!std::is_const<readable_type>::value>>
    readable_type& get_readable()
    {
        return m_readable;
    }
    const readable_type& get_readable() const
    {
        return m_readable;
    }

protected:
    template <typename T, span_extent_type N>
    error _read(span<T, N> s, elements length);

    readable_type m_readable{};
    vector<char> m_buffer{};
    bool m_eof{false};
};

#if SPIO_HAS_DEDUCTION_GUIDES
template <typename Readable>
basic_instream(Readable& r)->basic_instream<Readable>;
#endif

template <typename CharT>
class basic_file_instream : public basic_instream<basic_readable_file<CharT>> {
    using base_type = basic_instream<basic_readable_file<CharT>>;

public:
    using readable_type = typename base_type::readable_type;
    using char_type = typename base_type::char_type;

    using basic_instream<basic_readable_file<CharT>>::basic_instream;
    basic_file_instream() : base_type(readable_type{}) {}
    basic_file_instream(stdio_filehandle file)
        : base_type({}), m_file(std::move(file))
    {
        base_type::m_readable = readable_type{&m_file};
    }

private:
    stdio_filehandle m_file{};
};

template <typename CharT>
class basic_buffer_instream
    : public basic_instream<basic_readable_buffer<CharT>> {
    using base_type = basic_instream<basic_readable_buffer<CharT>>;

public:
    using readable_type = typename base_type::readable_type;
    using char_type = typename base_type::char_type;
    using buffer_type = typename readable_type::buffer_type;

    using basic_instream<basic_readable_buffer<CharT>>::basic_instream;
    basic_buffer_instream() : base_type(readable_type{}) {}
    basic_buffer_instream(buffer_type b) : base_type(b) {}
};

template <typename CharT>
class basic_stdin_instream : public basic_file_instream<CharT> {
public:
    basic_stdin_instream() : basic_file_instream<CharT>(stdin) {}
};

using file_instream = basic_file_instream<char>;
using file_winstream = basic_file_instream<wchar_t>;
using buffer_instream = basic_buffer_instream<char>;
using buffer_winstream = basic_buffer_instream<wchar_t>;

using sin = basic_stdin_instream<char>;
using wsin = basic_stdin_instream<wchar_t>;
}  // namespace io

#include "instream.impl.h"

#endif  // SPIO_INSTREAM_H
