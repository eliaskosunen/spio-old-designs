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

#ifndef SPIO_WRITER_H
#define SPIO_WRITER_H

#include "config.h"
#include "error.h"
#include "type.h"
#include "util.h"
#include "writable.h"

namespace io {
template <typename Writable>
class writer {
public:
    using writable_type = basic_writable_base<Writable>;
    using writable_impl = typename writable_type::implementation_type;
    using char_type = typename writable_impl::value_type;

    writer(writable_type& w);

    template <typename T>
    void write(T elem, writer_options<T> opt = {})
    {
        return type<T>::write(*this, elem, std::move(opt));
    }

    template <typename T>
    void write_raw(T elem)
    {
        return write_raw(make_span(&elem, 1));
    }
    template <typename T>
    void write_raw(span<T> elems)
    {
        auto error = _write(elems, elements{elems.length()});
        if (error) {
            SPIO_THROW_EC(error);
        }
    }

    void put(char_type ch)
    {
        write_raw(ch);
    }

    void flush()
    {
        auto error = get_writable().flush();
        if (error) {
            SPIO_THROW_EC(error);
        }
    }

    writable_type& get_writable()
    {
        return m_writable;
    }
    const writable_type& get_writable() const
    {
        return m_writable;
    }

private:
    template <typename T>
    error _write(span<T> s, elements length);

    writable_type& m_writable;
};
}  // namespace io

#include "writer.impl.h"

#endif  // SPIO_WRITER_H
