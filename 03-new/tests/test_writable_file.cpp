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

TEST_CASE("writable_file")
{
    io::writable_file w("write.txt", false);
    REQUIRE(w.is_valid());
    char ln = '\n';
    SUBCASE("write_elem")
    {
        char c = 'A';
        {
            auto error = w.write(&c);
            CHECK_FALSE(error);
            if (error) {
                std::cerr << error.message() << '\n';
            }
            CHECK_FALSE(w.write(&ln));
            CHECK_FALSE(w.flush());
        }
        {
            io::readable_file r("write.txt");
            REQUIRE(r.is_valid());
            auto error = r.read(&c);
            CHECK_FALSE(error);
            if (error && !io::is_eof(error)) {
                std::cerr << error.message() << '\n';
            }
            CHECK(c == 'A');
        }
    }
    SUBCASE("write_range")
    {
        std::array<char, 5> a{{'W', 'o', 'r', 'd', '\0'}};
        {
            auto error = w.write(io::make_span(a), io::elements{4});
            CHECK_FALSE(error);
            if (error) {
                std::cerr << error.message() << '\n';
            }
            CHECK_FALSE(w.write(&ln));
            CHECK_FALSE(w.flush());
        }
        {
            io::readable_file r("write.txt");
            auto error = r.read(io::make_span(a), io::elements{4});
            REQUIRE(a[4] == '\0');
            CHECK_FALSE(error);
            if (error && !io::is_eof(error)) {
                std::cerr << error.message() << '\n';
            }
            CHECK_EQ(std::strcmp(a.data(), "Word"), 0);
        }
    }
}
TEST_CASE("writable_wfile")
{
    SUBCASE("write_elem")
    {
        io::writable_wfile w("write.utf32.txt", false);
        REQUIRE(w.is_valid());
        wchar_t ln = L'\n';
        wchar_t c = L'A';
        {
            auto error = w.write(&c);
            CHECK_FALSE(error);
            if (error) {
                std::cerr << error.message() << '\n';
            }
            CHECK_FALSE(w.write(&ln));
            CHECK_FALSE(w.flush());
            w.flush();
        }
        {
            io::readable_wfile r("write.utf32.txt");
            REQUIRE(r.is_valid());
            auto error = r.read(&c);
            CHECK_FALSE(error);
            if (error && !io::is_eof(error)) {
                std::cerr << error.message() << '\n';
            }
            CHECK(c == L'A');
        }
    }
    SUBCASE("write_range")
    {
        io::writable_wfile w("write.utf32.txt", false);
        REQUIRE(w.is_valid());
        wchar_t ln = L'\n';
        std::array<wchar_t, 5> a{{L'W', L'o', L'r', L'd', L'\0'}};
        {
            auto error = w.write(io::make_span(a), io::elements{4});
            CHECK_FALSE(error);
            if (error) {
                std::cerr << error.message() << '\n';
            }
            CHECK_FALSE(w.write(&ln));
            CHECK_FALSE(w.flush());
        }
        {
            io::readable_file r("write.utf32.txt");
            auto error = r.read(io::make_span(a), io::elements{4});
            REQUIRE(a[4] == 0);
            CHECK_FALSE(error);
            if (error && !io::is_eof(error)) {
                std::cerr << error.message() << '\n';
            }
            CHECK_EQ(std::wcscmp(a.data(), L"Word"), 0);
        }
    }
}
