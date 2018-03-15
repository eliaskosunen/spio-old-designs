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

#ifndef SPIO_STREAM_ITERATOR_IMPL_H
#define SPIO_STREAM_ITERATOR_IMPL_H

#include "config.h"
#include "stream.h"
#include "stream_iterator.h"

namespace spio {
template <typename T, typename CharT>
auto outstream_iterator<T, CharT>::operator=(const T& value)
    -> outstream_iterator&
{
    m_out->get_formatter()(*this, "{}", value);
    if (m_delim.data()) {
        m_out->write(m_delim);
    }
    return *this;
}

template <typename T>
auto outstream_iterator<T, T>::operator=(span<const char_type> value)
    -> outstream_iterator&
{
    m_out->write(value);
    if (m_delim.data()) {
        m_out->write(m_delim);
    }
    return *this;
}

template <typename T, typename CharT>
void instream_iterator<T, CharT>::_read()
{
    SPIO_ASSERT(m_in,
                "instream_iterator::_read: Cannot read from nullptr stream");
    m_in->scan("{}", m_last);
}

template <typename T>
void instream_iterator<T, T>::_read()
{
    SPIO_ASSERT(m_in,
                "instream_iterator::_read: Cannot read from nullptr stream");
    m_in->read(make_span(&m_last, 1));
}

template <typename T>
void instream_iterator<T, T>::read_into(span<T> s)
{
    SPIO_ASSERT(m_in,
                "instream_iterator::_read: Cannot read from nullptr stream");
    m_in->read(s);
    m_last = s.back();
}

template <typename T>
void read_iterator_into_span(instream_iterator<T, T> it, span<const T> s)
{
    for (auto& c : s) {
        c = *it;
        ++it;
    }
}
}  // namespace spio

#endif
