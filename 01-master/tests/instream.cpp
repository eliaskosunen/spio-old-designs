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
#include <memory>
#include "doctest.h"
#include "spio.h"

TEST_CASE("file_instream")
{
    SUBCASE("read")
    {
        io::owned_filehandle h("file.txt", io::open_mode::READ);
        REQUIRE(h);
        io::file_instream f{h.get()};
        std::vector<char> str(20, '\0');
        f.read(io::make_span(str));
        CHECK_EQ(std::strcmp("Lorem", str.data()), 0);
    }
    SUBCASE("getline")
    {
        io::owned_filehandle h("file.txt", io::open_mode::READ);
        REQUIRE(h);
        io::file_instream f{h.get()};
        std::vector<char> str(20, '\0');
        f.getline(io::make_span(str));
        CHECK_EQ(std::strcmp("Lorem ipsum", str.data()), 0);
    }
    SUBCASE("scan")
    {
        io::owned_filehandle h("file.txt", io::open_mode::READ);
        REQUIRE(h);
        io::file_instream f{h.get()};
        std::string str, str2;
        f.scan("{} {}", str, str2);
        CHECK("Lorem" == str);
        CHECK("ipsum" == str2);
    }
}
TEST_CASE("instream int")
{
    SUBCASE("basic")
    {
        std::string buf = "123";
        io::readable_buffer r(io::make_span(buf));
        io::buffer_instream p{std::move(r)};

        int val;
        p.read(val);
        CHECK(val == 123);
    }
    SUBCASE("signed")
    {
        std::string buf = "-273";
        io::readable_buffer r(io::make_span(buf));
        io::buffer_instream p{std::move(r)};

        int val;
        p.read(val);
        CHECK(val == -273);
    }
#if SPIO_USE_EXCEPTIONS
    SUBCASE("error")
    {
        std::string buf = "foo";
        io::readable_buffer r(io::make_span(buf));
        io::buffer_instream p{std::move(r)};

        int val;
        CHECK_THROWS_AS(p.read(val), io::failure);
    }
#endif
}
TEST_CASE("instream uint")
{
    SUBCASE("basic")
    {
        std::string buf = "123";
        io::readable_buffer r(io::make_span(buf));
        io::buffer_instream p{std::move(r)};

        uint32_t val;
        p.read(val);
        CHECK(val == 123);
    }
#if SPIO_USE_EXCEPTIONS
    SUBCASE("signed")
    {
        std::string buf = "-273";
        io::readable_buffer r(io::make_span(buf));
        io::buffer_instream p{std::move(r)};

        uint32_t val;
        CHECK_THROWS_AS(p.read(val), io::failure);
    }
    SUBCASE("error")
    {
        std::string buf = "foo";
        io::readable_buffer r(io::make_span(buf));
        io::buffer_instream p{std::move(r)};

        uint32_t val;
        CHECK_THROWS_AS(p.read(val), io::failure);
    }
#endif
}
TEST_CASE("instream float")
{
    SUBCASE("basic")
    {
        std::string buf = "3.14";
        io::readable_buffer r(io::make_span(buf));
        io::buffer_instream p{std::move(r)};

        double val;
        p.read(val);
        CHECK(val == doctest::Approx(3.14));
    }
#if SPIO_USE_EXCEPTIONS
    SUBCASE("error")
    {
        std::string buf = "foo";
        io::readable_buffer r(io::make_span(buf));
        io::buffer_instream p{std::move(r)};

        double val;
        CHECK_THROWS_AS(p.read(val), io::failure);
    }
#endif
}
TEST_CASE("instream string")
{
    SUBCASE("basic")
    {
        std::string buf = "foo";
        io::readable_buffer r(io::make_span(buf));
        io::buffer_instream p{std::move(r)};

        std::vector<char> val(4, '\0');
        auto s = io::make_span(val);
        p.read(s);
        CHECK_EQ(std::strcmp(val.data(), "foo"), 0);
    }
    SUBCASE("multiple words")
    {
        std::string buf = "Hello world!";
        io::readable_buffer r(io::make_span(buf));
        io::buffer_instream p{std::move(r)};

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
TEST_CASE("instream getline")
{
    SUBCASE("basic")
    {
        std::string buf = "String spanning\nmultiple\nlines";
        io::readable_buffer r(io::make_span(buf));
        io::buffer_instream p{std::move(r)};

        std::vector<char> val(32, '\0');
        CHECK(p.getline(io::make_span(val)));
        CHECK_EQ(std::strcmp(val.data(), "String spanning"), 0);

        std::fill(val.begin(), val.end(), '\0');
        CHECK(p.getline(io::make_span(val)));
        CHECK_EQ(std::strcmp(val.data(), "multiple"), 0);

        std::fill(val.begin(), val.end(), '\0');
        CHECK_FALSE(p.getline(io::make_span(val)));
        CHECK_EQ(std::strcmp(val.data(), "lines"), 0);
    }
    SUBCASE("different delim")
    {
        std::string buf = "String spanning\ttwo\nlines";
        io::readable_buffer r(io::make_span(buf));
        io::buffer_instream p{std::move(r)};

        std::vector<char> val(32, '\0');
        CHECK(p.getline(io::make_span(val), '\t'));
        CHECK_EQ(std::strcmp(val.data(), "String spanning"), 0);

        std::fill(val.begin(), val.end(), '\0');
        CHECK(p.getline(io::make_span(val)));
        CHECK_EQ(std::strcmp(val.data(), "two"), 0);

        std::fill(val.begin(), val.end(), '\0');
        CHECK_FALSE(p.getline(io::make_span(val)));
        CHECK_EQ(std::strcmp(val.data(), "lines"), 0);
    }
}
TEST_CASE("instream ignore")
{
    std::string buf = "String spanning\nmultiple\nlines";
    io::readable_buffer r(io::make_span(buf));
    io::buffer_instream p{std::move(r)};

    CHECK(p.ignore(32, '\n'));

    std::vector<char> val(32, '\0');
    CHECK(p.getline(io::make_span(val)));
    CHECK_EQ(std::strcmp(val.data(), "multiple"), 0);

    CHECK(p.ignore());

    char c;
    CHECK(p.get(c));
    CHECK(c == 'i');
}
