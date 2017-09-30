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

#include <iostream>
#include "doctest.h"
#include "spio.h"

TEST_CASE("writable_buffer")
{
    io::writable_buffer w;
    REQUIRE(w.is_valid());
    SUBCASE("write_elem")
    {
        char c = 'A';
        {
            auto error = w.write(&c);
            CHECK_FALSE(error);
            if (error) {
                std::cerr << error.message() << '\n';
            }
        }
        {
            auto buf = w.consume_buffer();
            REQUIRE(!buf.empty());
            CHECK(buf.size() == 1);
            CHECK(buf[0] == c);
        }
    }
    SUBCASE("write_range")
    {
        std::array<char, 5> a{{'W', 'o', 'r', 'd', '\0'}};
        {
            auto error = w.write(
                io::make_span(a),
                io::elements{static_cast<io::span_extent_type>(a.size())});
            CHECK_FALSE(error);
            if (error) {
                std::cerr << error.message() << '\n';
            }
        }
        {
            auto buf = w.consume_buffer();
            REQUIRE(!buf.empty());
            CHECK(buf.size() == 5);
            CHECK_EQ(std::strcmp(a.data(), "Word"), 0);
        }
    }
}
