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

#ifndef SPIO_NULL_DEVICE_H
#define SPIO_NULL_DEVICE_H

#include "fwd.h"

namespace spio {
template <typename CharT, typename Traits>
class basic_null_device {
public:
    using char_type = CharT;
    using traits = Traits;

    struct category : bidirectional_seekable_device_tag {
    };

    basic_null_device() = default;

    streamsize read(span<char_type> s)
    {
        SPIO_UNUSED(s);
        return -1;
    }
    streamsize write(span<const char_type> s)
    {
        return s.size();
    }
};

using null_device = basic_null_device<char>;
using wnull_device = basic_null_device<wchar_t>;

template <typename CharT, typename Traits>
class basic_null_source : private basic_null_device<CharT> {
    using base = basic_null_device<CharT>;

public:
    using char_type = typename base::char_type;
    using traits = Traits;

    using base::base;
    using base::read;
};

using null_source = basic_null_source<char>;
using wnull_source = basic_null_source<wchar_t>;

template <typename CharT, typename Traits>
class basic_null_sink : private basic_null_device<CharT> {
    using base = basic_null_device<CharT>;

public:
    using char_type = typename base::char_type;
    using traits = Traits;

    using base::base;
    using base::write;
};

using null_sink = basic_null_sink<char>;
using wnull_sink = basic_null_sink<wchar_t>;
}  // namespace spio

#endif
