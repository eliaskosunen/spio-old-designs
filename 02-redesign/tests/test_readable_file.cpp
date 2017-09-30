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
#include "spio.h"
#include "doctest.h"

TEST_CASE("readable_file")
{
    io::readable_file r("file.txt");
    REQUIRE(r.is_valid());
    SUBCASE("read_elem")
    {
        char c = '\0';
        auto error = r.read(&c);
        CHECK_FALSE(error);
        CHECK_FALSE(io::is_eof(error));
        if (error) {
            std::cerr << error.message() << '\n';
        }
        CHECK(c == 'L');
    }
    SUBCASE("read_range")
    {
        std::array<char, 6> a{{0}};
        auto error = r.read(io::make_span(a), io::elements{5});
        CHECK_FALSE(error);
        CHECK_FALSE(io::is_eof(error));
        if (error) {
            std::cerr << error.message() << '\n';
        }
        REQUIRE(a[5] == '\0');
        CHECK(std::equal(a.begin(), a.end(), "Lorem"));
    }
}
TEST_CASE("readable_wfile")
{
    io::readable_wfile r("wchar.utf32.txt");
    REQUIRE(r.is_valid());
    // Dismiss BOM
    r.skip();
    SUBCASE("read_elem")
    {
        wchar_t c = L'\0';
        auto error = r.read(&c);
        CHECK_FALSE(error);
        CHECK_FALSE(io::is_eof(error));
        if (error) {
            std::cerr << error.message() << '\n';
        }
        CHECK(c == L'L');
    }
    SUBCASE("read_range")
    {
        std::array<wchar_t, 6> a{{0}};
        auto error = r.read(io::make_span(a), io::elements{5});
        CHECK_FALSE(error);
        CHECK_FALSE(io::is_eof(error));
        if (error) {
            std::cerr << error.message() << '\n';
        }
        REQUIRE(a[5] == '\0');
        CHECK_EQ(std::wstring{a.data()}, L"Lorem");
    }
}
