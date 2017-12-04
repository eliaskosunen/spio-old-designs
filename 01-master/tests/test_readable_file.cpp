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
#include "doctest.h"
#include "spio.h"

TEST_CASE("readable_file")
{
    io::owned_filehandle f("file.txt", io::open_mode::READ,
                           io::open_flags::BINARY);
    REQUIRE(f);
    io::readable_file r(f.get());
    SUBCASE("read_elem")
    {
        char c = '\0';
        auto error = r.read(c);
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
    SUBCASE("seek and tell")
    {
#define error_check(error)                        \
    do {                                          \
        CHECK_FALSE(error);                       \
        CHECK_FALSE(io::is_eof(error));           \
        if (error) {                              \
            std::cerr << error.message() << '\n'; \
        }                                         \
    } while (0);

        io::seek_type pos;
        auto error = r.tell(pos);
        error_check(error);
        CHECK(pos == 0);

        error = r.seek(io::seek_origin::CUR, 6);
        error_check(error);

        char c{};
        error = r.read(c);
        error_check(error);
        CHECK(c == 'i');

        error = r.tell(pos);
        error_check(error);
        CHECK(pos == 7);

        error = r.seek(io::seek_origin::SET, 0);
        error_check(error);

        error = r.read(c);
        error_check(error);
        CHECK(c == 'L');
#undef error_check
    }
}
#ifndef _WIN32
TEST_CASE("readable_wfile")
{
    io::owned_filehandle f("wchar.utf32.txt", io::open_mode::READ);
    REQUIRE(f);
    io::readable_wfile r(f.get());
    // Dismiss BOM
    r.skip();
    SUBCASE("read_elem")
    {
        wchar_t c = L'\0';
        auto error = r.read(c);
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
#endif
