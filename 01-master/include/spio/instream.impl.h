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
#if SPIO_HAS_FOLD_EXPRESSIONS
template <typename Readable>
template <typename... T>
bool basic_instream<Readable>::scan(T&&... args)
{
    return (read(std::forward<T>(args)) && ...);
}
#else
namespace detail {
    template <typename ReadF, typename T, typename... Args>
    bool instream_scan(ReadF&& read, T&& arg, Args&&... args)
    {
        if (!read(std::forward<T>(arg))) {
            return false;
        }
        return instream_scan(read, std::forward<Args>(args)...);
    }
    template <typename ReadF, typename T>
    bool instream_scan(ReadF&& read, T&& arg)
    {
        return read(std::forward<T>(arg));
    }
}  // namespace detail

template <typename Readable>
template <typename... T>
bool basic_instream<Readable>::scan(T&&... args)
{
    return detail::instream_scan(read, std::forward<T>(args)...);
}
#endif
}
