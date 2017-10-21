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

TEST_CASE("outstream int")
{
    SUBCASE("basic")
    {
        io::writable_buffer w{};
        io::buffer_outstream p{std::move(w)};

        p.write(123);
        CHECK(p.get_writable().get_buffer()[0] == '1');
        CHECK(p.get_writable().get_buffer()[1] == '2');
        CHECK(p.get_writable().get_buffer()[2] == '3');
        CHECK(p.get_writable().get_buffer().size() == 3);
    }
    SUBCASE("signed")
    {
        io::writable_buffer w{};
        io::buffer_outstream p{std::move(w)};

        p.write(-273);
        CHECK(p.get_writable().get_buffer()[0] == '-');
        CHECK(p.get_writable().get_buffer()[1] == '2');
        CHECK(p.get_writable().get_buffer()[2] == '7');
        CHECK(p.get_writable().get_buffer()[3] == '3');
        CHECK(p.get_writable().get_buffer().size() == 4);
    }
}
TEST_CASE("outstream float")
{
    SUBCASE("basic")
    {
        io::writable_buffer w{};
        io::buffer_outstream p{std::move(w)};

        p.write(3.14);
        CHECK(p.get_writable().get_buffer()[0] == '3');
        CHECK(p.get_writable().get_buffer()[1] == '.');
        CHECK(p.get_writable().get_buffer()[2] == '1');
        CHECK(p.get_writable().get_buffer()[3] == '4');
    }
}
