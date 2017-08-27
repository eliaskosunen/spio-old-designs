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

#ifndef SPIO_INPUT_PARSER_H
#define SPIO_INPUT_PARSER_H

#include "config.h"
#include "error.h"
#include "readable.h"
#include "type.h"
#include "util.h"

namespace io {
template <typename Readable>
class input_parser {
public:
    using readable_type = Readable;

    input_parser(Readable r);

    template <typename T>
    void read(T& elem)
    {
        if (m_eof) {
            SPIO_THROW(end_of_file, "EOF reached");
        }
        type<T>::read(*this, elem);
    }
    template <typename T>
    void read(span<T> elems)
    {
        for (auto& e : elems) {
            read(e);
        }
    }

    template <typename T>
    void read_raw(span<T> elems)
    {
        if (m_eof) {
            SPIO_THROW(end_of_file, "EOF reached");
        }
        auto error = _read(elems, elements{elems.length()});
        if (is_eof(error)) {
            m_eof = true;
        }
        if (error) {
            SPIO_THROW_EC(error);
        }
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

    Readable m_readable;
    vector<char> m_buffer{};
    bool m_eof{false};
};
}  // namespace io

#include "input_parser.impl.h"

#endif  // SPIO_INPUT_PARSER_H
