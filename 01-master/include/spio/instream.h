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

namespace io {
template <typename CharT, typename Readable>
class basic_instream {
public:
    using char_type = CharT;
    using readable_type = Readable;

    basic_instream(basic_readable_base<CharT, Readable> r)
        : m_readable(std::move(r))
    {
    }

    template <typename T>
    basic_instream& read(span<T>& elems);
    template <typename T>
    basic_instream& read(T& elem);
    template <typename InputIt>
    basic_instream& read(InputIt begin, InputIt end);

    constexpr bool eof() const
    {
        return m_eof;
    }

private:
    Readable m_readable;
    bool m_eof{false};
};

template <typename CharT, typename Readable>
auto make_basic_instream(basic_readable_base<CharT, Readable> r)
{
    return basic_instream<CharT, Readable>{std::move(r)};
}

}  // namespace io

#include "instream.impl.h"

#endif  // SPIO_INSTREAM_H
