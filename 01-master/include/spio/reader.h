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

#ifndef SPIO_READER_H
#define SPIO_READER_H

#include "config.h"
#include "error.h"
#include "readable.h"
#include "reader_options.h"
#include "type.h"
#include "util.h"

namespace io {

template <typename Readable>
class reader {
public:
    using readable_type = Readable;
    using char_type = typename Readable::value_type;

    reader(Readable& r);

    template <typename T>
    bool read(T& elem, reader_options<T> opt = {})
    {
        if (m_eof) {
            SPIO_THROW(end_of_file, "EOF reached at " __FILE__ ":" SPIO_LINE);
        }
        return type<T>::read(*this, elem, std::move(opt));
    }
    template <typename T>
    bool read(span<T> elem, reader_options<span<T>> opt = {})
    {
        if (m_eof) {
            SPIO_THROW(end_of_file, "EOF reached at " __FILE__ ":" SPIO_LINE);
        }
        return type<span<T>>::read(*this, elem, std::move(opt));
    }

    template <typename T>
    bool read_raw(T& elem)
    {
        return read_raw(make_span(&elem, 1));
    }
    template <typename T>
    bool read_raw(span<T> elems)
    {
        if (m_eof) {
            SPIO_THROW(end_of_file, "EOF reached at " __FILE__ ":" SPIO_LINE);
        }
        auto error = _read(elems, elements{elems.length()});
        if (error.is_eof()) {
            m_eof = true;
        }
        if (error) {
            SPIO_THROW_EC(error);
        }
        return !m_eof;
    }

    bool get(char_type& ch)
    {
        return read_raw(ch);
    }
    template <typename T>
    bool getline(span<T> s)
    {
        return getline(s, static_cast<T>('\n'));
    }
    template <typename T>
    bool getline(span<T> s, char_type delim)
    {
        reader_options<span<T>> opt = {make_span(&delim, 1)};
        return read(s, opt);
    }

    template <typename ElementT = char_type>
    bool ignore(std::size_t count = 1)
    {
        SPIO_ASSERT(sizeof(ElementT) * count % sizeof(char_type) == 0,
                    "sizeof(ElementT) * count must be divisible by "
                    "sizeof(char_type) in reader::ignore");
        vector<char_type> arr(sizeof(ElementT) * count / sizeof(char_type));
        return read_raw(make_span(arr));
    }
    template <typename ElementT = char_type>
    bool ignore(std::size_t count, char_type delim)
    {
        char_type ch{};
        for (std::size_t i = 0; i < count; ++i) {
            if (!get(ch)) {
                return false;
            }
            if (ch == delim) {
                return !m_eof;
            }
        }
        return !m_eof;
    }

    Readable& get_readable()
    {
        return m_readable;
    }
    const Readable& get_readable() const
    {
        return m_readable;
    }

    template <typename T>
    void push(T elem);

    constexpr bool eof() const
    {
        return m_eof;
    }

private:
    template <typename T>
    error _read(span<T> s, elements length);

    Readable& m_readable;
    vector<char> m_buffer{};
    bool m_eof{false};
};
}  // namespace io

#include "reader.impl.h"

#endif  // SPIO_READER_H
