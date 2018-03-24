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

#include <doctest.h>
#include <spio/spio.h>

TEST_CASE("print")
{
    auto str = std::string{};
    spio::basic_stream<spio::string_sink> s(str);
    SUBCASE("string")
    {
        s.print("Hello world!");
        CHECK(str == "Hello world!");

        str.clear();
        s.seek(0, spio::seekdir::beg);
        s.print("{}{}", "Hello world!", "\n");
        CHECK(str == "Hello world!\n");
    }
    SUBCASE("various")
    {
        s.print("{} {} {} {}", 100, 3.14, static_cast<void*>(nullptr), true);
        CHECK(str == "100 3.14 0x0 true");
    }
}

TEST_CASE("scan")
{
    SUBCASE("span")
    {
        auto str = std::string{"Hello world"};
        spio::basic_stream<spio::basic_container_source<std::string>> s(str);

        std::string r(5, '\0');
        auto span = spio::make_span(r);
        s.scan("{}", span);
        CHECK_EQ(std::strcmp(r.c_str(), "Hello"), 0);

        s.scan("{}", span);
        CHECK_EQ(std::strcmp(r.c_str(), "world"), 0);
    }
    SUBCASE("int")
    {
        auto str = std::string{"42 240 7f 1010"};
        spio::basic_stream<spio::basic_container_source<std::string>> s(str);

        int n;
        s.scan("{}", n);
        CHECK(n == 42);

        s.scan("{}", n);
        CHECK(n == 240);

        s.scan("{x}", n);
        CHECK(n == 0x7f);

        s.scan("{b}", n);
        CHECK(n == 10);
    }
}
