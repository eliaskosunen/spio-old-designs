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

namespace spio {
template <typename CharT, typename Arg>
basic_instream<CharT>& scan(basic_instream<CharT>& s, Arg& a)
{
    s.scanner().basic_scan<CharT>(s, a);
    return s;
}
template <typename CharT, typename... Args>
basic_instream<CharT>& scan(basic_instream<CharT>& s,
                            basic_string_view<CharT> f,
                            Args&... a)
{
    s.scanner().scan(s, f, a...);
    return s;
}

template <typename CharT>
basic_instream<CharT>& getln(basic_instream<CharT>& s,
                             span<CharT> a,
                             CharT delim = s.get_nl())
{
    return s;
}

template <typename CharT, typename<class> Container>
basic_instream<CharT>& getln(basic_instream<CharT>& s,
                             Container<CharT> a,
                             CharT delim = s.get_nl<CharT>())
{
    return s;
}

template <typename CharT>
basic_instream<CharT>& getchar(basic_instream<CharT>& s, CharT& c)
{
    return s.read(make_span(std::addressof(c), 1));
}
}  // namespace spio

#endif  // SPIO_OUTSTREAM_H
