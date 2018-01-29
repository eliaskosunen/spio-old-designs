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

#include <iostream>
#include <sstream>
#include "doctest.h"
#include "spio.h"

struct my_type {
    int a, b;

    constexpr bool operator==(const my_type& o) const
    {
        return a == o.a && b == o.b;
    }
};

struct ostream_type {
    int a, b;

    friend std::ostream& operator<<(std::ostream& os, const ostream_type& t)
    {
        os << t.a << ' ' << t.b;
        return os;
    }

    constexpr bool operator==(const ostream_type& o) const
    {
        return a == o.a && b == o.b;
    }
};

namespace io {
template <>
struct custom_write<my_type> {
    template <typename Writer>
    static bool write(Writer& w,
                      const my_type& val,
                      writer_options<my_type> opt)
    {
        SPIO_UNUSED(opt);
        return w.print("[my_type: a={}, b={}]", val.a, val.b);
    }
};

template <>
struct custom_read<my_type> {
    template <typename Reader>
    static bool read(Reader& w, my_type& val, reader_options<my_type> opt)
    {
        SPIO_UNUSED(opt);
        return w.scan("{} {}", val.a, val.b);
    }
};
}  // namespace io

TEST_CASE("usertype")
{
    SUBCASE("write")
    {
        my_type a{1, 2};
        io::buffer_outstream out{};
        out.write(a);
        out.flush();
        auto buf = out.get_buffer();
        CHECK(std::string{buf.begin(), buf.end()} == "[my_type: a=1, b=2]");
    }

    SUBCASE("read")
    {
        std::string buf{"1 2"};
        io::buffer_instream in{io::make_span(buf)};
        my_type a{};
        in.read(a);
        CHECK(a == my_type{1, 2});
    }

    SUBCASE("ostream")
    {
        ostream_type a{1, 2};
        std::ostringstream os{};
        os << a;
        auto str = os.str();
        CHECK(str == "1 2");

        io::buffer_outstream out{};
        out.write(a);
        out.flush();
        auto buf = out.get_buffer();
        CHECK(std::string{buf.begin(), buf.end()} == str);
    }
}
