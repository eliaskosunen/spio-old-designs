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

TEST_CASE("reader int")
{
    SUBCASE("basic")
    {
        std::string buf = "123";
        io::readable_buffer r(io::make_span(buf));
        io::reader<decltype(r)> p{r};

        int val;
        p.read(val);
        CHECK(val == 123);
    }
    SUBCASE("signed")
    {
        std::string buf = "-273";
        io::readable_buffer r(io::make_span(buf));
        io::reader<decltype(r)> p{r};

        int val;
        p.read(val);
        CHECK(val == -273);
    }
    SUBCASE("error")
    {
        std::string buf = "foo";
        io::readable_buffer r(io::make_span(buf));
        io::reader<decltype(r)> p{r};

        int val;
        CHECK_THROWS_AS(p.read(val), const io::failure&);
    }
}
TEST_CASE("reader uint")
{
    SUBCASE("basic")
    {
        std::string buf = "123";
        io::readable_buffer r(io::make_span(buf));
        io::reader<decltype(r)> p{r};

        uint32_t val;
        p.read(val);
        CHECK(val == 123);
    }
    SUBCASE("signed")
    {
        std::string buf = "-273";
        io::readable_buffer r(io::make_span(buf));
        io::reader<decltype(r)> p{r};

        uint32_t val;
        CHECK_THROWS_AS(p.read(val), const io::failure&);
    }
    SUBCASE("error")
    {
        std::string buf = "foo";
        io::readable_buffer r(io::make_span(buf));
        io::reader<decltype(r)> p{r};

        uint32_t val;
        CHECK_THROWS_AS(p.read(val), const io::failure&);
    }
}
TEST_CASE("reader float")
{
    SUBCASE("basic")
    {
        std::string buf = "3.14";
        io::readable_buffer r(io::make_span(buf));
        io::reader<decltype(r)> p{r};

        double val;
        p.read(val);
        CHECK(val == doctest::Approx(3.14));
    }
    SUBCASE("error")
    {
        std::string buf = "foo";
        io::readable_buffer r(io::make_span(buf));
        io::reader<decltype(r)> p{r};

        double val;
        CHECK_THROWS_AS(p.read(val), const io::failure&);
    }
}
TEST_CASE("reader string")
{
    SUBCASE("basic")
    {
        std::string buf = "foo";
        io::readable_buffer r(io::make_span(buf));
        io::reader<decltype(r)> p{r};

        std::vector<char> val(4, '\0');
        auto s = io::make_span(val);
        p.read(s);
        CHECK_EQ(std::strcmp(val.data(), "foo"), 0);
    }
    SUBCASE("multiple words")
    {
        std::string buf = "Hello world!";
        io::readable_buffer r(io::make_span(buf));
        io::reader<decltype(r)> p{r};

        std::vector<char> val(6, '\0');
        auto s = io::make_span(val);
        p.read(s.first(5));
        CHECK_EQ(std::strcmp(val.data(), "Hello"), 0);

        val = std::vector<char>(7, '\0');
        s = io::make_span(val);
        p.read(s.first(6));
        CHECK_EQ(std::strcmp(val.data(), "world!"), 0);
    }
}
TEST_CASE("reader getline")
{
    SUBCASE("basic")
    {
        std::string buf = "String spanning\nmultiple\nlines";
        io::readable_buffer r(io::make_span(buf));
        io::reader<decltype(r)> p{r};

        std::vector<char> val(32, '\0');
        CHECK(p.getline(io::make_span(val)));
        CHECK_EQ(std::strcmp(val.data(), "String spanning"), 0);

        CHECK(p.getline(io::make_span(val)));
        CHECK_EQ(std::strcmp(val.data(), "multiple"), 0);

        CHECK_FALSE(p.getline(io::make_span(val)));
        CHECK_EQ(std::strcmp(val.data(), "lines"), 0);
    }
    SUBCASE("different delim")
    {
        std::string buf = "String spanning\ttwo\nlines";
        io::readable_buffer r(io::make_span(buf));
        io::reader<decltype(r)> p{r};

        std::vector<char> val(32, '\0');
        CHECK(p.getline(io::make_span(val), '\t'));
        CHECK_EQ(std::strcmp(val.data(), "String spanning"), 0);

        CHECK(p.getline(io::make_span(val)));
        CHECK_EQ(std::strcmp(val.data(), "two"), 0);

        CHECK_FALSE(p.getline(io::make_span(val)));
        CHECK_EQ(std::strcmp(val.data(), "lines"), 0);
    }
}
TEST_CASE("reader ignore")
{
        std::string buf = "String spanning\nmultiple\nlines";
        io::readable_buffer r(io::make_span(buf));
        io::reader<decltype(r)> p{r};

        CHECK(p.ignore(32, '\n'));

        std::vector<char> val(32, '\0');
        CHECK(p.getline(io::make_span(val)));
        CHECK_EQ(std::strcmp(val.data(), "multiple"), 0);

        CHECK(p.ignore());
        
        char c;
        CHECK(p.get(c));
        CHECK(c == 'i');
}
