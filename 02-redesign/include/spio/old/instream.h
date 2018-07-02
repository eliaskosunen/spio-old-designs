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

    static_assert(
        is_readable<Readable>::value,
        "basic_instream<T>: T does not satisfy the requirements of Readable");

    explicit basic_instream(readable_type r) : m_readable(std::move(r)) {}

    template <typename T>
    basic_instream& read(T& elem, reader_options<T> opt = {})
    {
        if (eof()) {
            SPIO_THROW(end_of_file, "io::reader::read: EOF reached");
        }
        m_eof = !type<T>::read(*this, elem, std::move(opt));
        return *this;
    }
    template <typename T, extent_t N>
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
    template <typename T, extent_t N>
    basic_instream& read_raw(span<T, N> elems)
    {
        if (eof()) {
            SPIO_THROW(end_of_file, "io::reader::read_raw: EOF reached");
        }
        m_eof = !_read(elems, elements{elems.length()});
        return *this;
    }

    basic_instream& get(char_type& ch)
    {
        return read_raw(ch);
    }

    template <extent_t N = dynamic_extent>
    basic_instream& getline(span<char_type, N> s,
                            char_type delim = char_type{'\n'})
    {
        reader_options<span<char_type, N>> opt = {make_span<1>(&delim)};
        return read(s, opt);
    }
    template <typename T>
    std::enable_if_t<
        is_growable_read_container<T>::value &&
            std::is_same<typename T::value_type,
                         typename basic_instream<Readable>::char_type>::value,
        basic_instream<Readable>>&
    getline(T& val, char_type delim = char_type{'\n'});

    template <typename ElementT = char_type>
    basic_instream& ignore(std::size_t count = 1)
    {
        SPIO_ASSERT(sizeof(ElementT) * count % sizeof(char_type) == 0,
                    "sizeof(ElementT) * count must be divisible by "
                    "sizeof(char_type) in reader::ignore");
        std::vector<char_type> arr(sizeof(ElementT) * count /
                                   sizeof(char_type));
        return read_raw(make_span(arr));
    }
    template <typename Element = char_type>
    basic_instream& ignore(std::size_t count, char_type delim)
    {
        char_type ch{};
        for (std::size_t i = 0; i < count; ++i) {
            if (!get(ch) || ch == delim) {
                break;
            }
        }
        return *this;
    }

    template <typename T>
    void push(T elem);
    template <typename T, extent_t N>
    void push(span<T, N> elems);

    bool is_overreadable() const
    {
        return m_readable.is_overreadable();
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

    template <typename... T>
    basic_instream& scan(const char_type* format, T&... args)
    {
        _scan(format, args...);
        return *this;
    }

    std::enable_if_t<!std::is_const<readable_type>::value, readable_type>&
    get_readable()
    {
        return m_readable;
    }
    const readable_type& get_readable() const
    {
        return m_readable;
    }

protected:
    basic_instream() = default;

    template <typename T, extent_t N>
    bool _read(span<T, N> s, elements length);

    void _scan(const char_type* format)
    {
        SPIO_UNUSED(format);
    }
    template <typename T, typename... Args>
    void _scan(const char_type* format, T& a, Args&... args);

    template <typename T>
    const char_type* _scan_arg(const char_type* format, T& a);

    readable_type m_readable{};
    std::vector<char> m_buffer{};
    bool m_eof{false};
};

#if SPIO_HAS_DEDUCTION_GUIDES
template <typename Readable,
          typename = std::enable_if_t<is_readable<Readable>::value>>
basic_instream(Readable& r)->basic_instream<Readable>;
#endif

template <typename CharT, class FileHandle = filehandle>
class basic_file_instream
    : public basic_instream<basic_readable_file<CharT, FileHandle>> {
    using base_type = basic_instream<basic_readable_file<CharT, FileHandle>>;

public:
    using readable_type = typename base_type::readable_type;
    using char_type = typename base_type::char_type;

    basic_file_instream() = default;
    explicit basic_file_instream(readable_type r) : base_type(std::move(r)) {}
    explicit basic_file_instream(FileHandle& file)
        : basic_file_instream(readable_type{file})
    {
    }
};

template <typename CharT>
class basic_buffer_instream
    : public basic_instream<basic_readable_buffer<CharT>> {
    using base_type = basic_instream<basic_readable_buffer<CharT>>;

public:
    using readable_type = typename base_type::readable_type;
    using char_type = typename base_type::char_type;
    using buffer_type = typename readable_type::buffer_type;

    basic_buffer_instream() = default;
    explicit basic_buffer_instream(readable_type r) : base_type(std::move(r)) {}
    /* explicit basic_buffer_instream(buffer_type b) */
    /*     : basic_buffer_instream(readable_type{b}) */
    /* { */
    /* } */
    template <extent_t N = dynamic_extent>
    explicit basic_buffer_instream(span<CharT, N> b)
        : basic_buffer_instream(readable_type{buffer_type{b.begin(), b.end()}})
    {
    }
};

#if 0
template <typename CharT, typename FileHandle = filehandle>
using basic_file_instream =
    basic_instream<basic_readable_file<CharT, FileHandle>>;
template <typename CharT>
using basic_buffer_instream = basic_instream<basic_readable_buffer<CharT>>;
#endif

using file_instream = basic_file_instream<char>;
using file_winstream = basic_file_instream<wchar_t>;
using buffer_instream = basic_buffer_instream<char>;
using buffer_winstream = basic_buffer_instream<wchar_t>;

#if SPIO_USE_THREADING
using mt_file_instream = basic_lockable_stream<file_instream>;
using mt_file_winstream = basic_lockable_stream<file_winstream>;
using mt_buffer_instream = basic_lockable_stream<buffer_instream>;
using mt_buffer_winstream = basic_lockable_stream<buffer_winstream>;
#endif

static_assert(is_reader<file_instream>::value,
              "file_instream does not satisfy the requirements of Reader");
static_assert(is_reader<file_winstream>::value,
              "file_winstream does not satisfy the requirements of Reader");
static_assert(is_reader<buffer_instream>::value,
              "buffer_instream does not satisfy the requirements of Reader");
static_assert(is_reader<buffer_winstream>::value,
              "buffer_winstream does not satisfy the requirements of Reader");

template <typename T>
auto& get_stdin()
{
#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif
    static auto f = stdio_filehandle{filebuffer::BUFFER_DEFAULT, stdin};
    static auto s = basic_file_instream<T, stdio_filehandle>{f};
    return s;
#if defined(__clang__)
#pragma GCC diagnostic pop
#endif
}

inline auto& sin()
{
    return get_stdin<char>();
}
inline auto& wsin()
{
    return get_stdin<wchar_t>();
}

#if SPIO_USE_THREADING
template <typename T>
auto& get_mt_stdin()
{
#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif
    static auto f = stdio_filehandle{filebuffer::BUFFER_DEFAULT, stdin};
    static auto s = basic_file_instream<T, stdio_filehandle>{f};
    static basic_lockable_stream<decltype(s)> mt{std::move(s)};
    return mt;
#if defined(__clang__)
#pragma GCC diagnostic pop
#endif
}

inline auto& mt_sin()
{
    return get_mt_stdin<char>();
}
inline auto& mt_wsin()
{
    return get_mt_stdin<wchar_t>();
}
#endif
}  // namespace io

#include "instream.impl.h"

#endif  // SPIO_INSTREAM_H
