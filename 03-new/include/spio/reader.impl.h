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

namespace io {
template <typename Readable>
reader<Readable>::reader(readable_type& r) : m_readable(r)
{
}

template <typename Readable>
template <typename T>
error reader<Readable>::_read(span<T> s, elements length)
{
    if (m_buffer.empty()) {
        return m_readable.read(s, length);
    }
    assert(s.size() >= length);
    auto len_bytes = length.get_unsigned() * sizeof(T);
    if (m_buffer.size() >= len_bytes) {
        auto len = static_cast<typename decltype(m_buffer)::difference_type>(
            len_bytes);
        copy(m_buffer.begin(), m_buffer.begin() + len,
             reinterpret_cast<char*>(&s[0]));
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + len);
        return {};
    }
    auto i = m_buffer.size();
    copy(m_buffer.begin(), m_buffer.end(), reinterpret_cast<char*>(&s[0]));
    m_buffer.clear();
    if (i % sizeof(T) == 0) {
        auto range = make_span(&s[0] + i / sizeof(T), &s[0] + s.size());
        return m_readable.read(range, elements{range.size()});
    }
    auto range = make_span(reinterpret_cast<char*>(&s[0] + i),
                           reinterpret_cast<char*>(&s[0] + s.size()));
    return m_readable.read(range, bytes{range.size()});
}

template <typename Readable>
template <typename T>
void reader<Readable>::push(T elem)
{
    auto chars = reinterpret_cast<char*>(&elem);
    m_buffer.insert(m_buffer.end(), chars, chars + sizeof(T));
    m_eof = false;
}
}  // namespace io
