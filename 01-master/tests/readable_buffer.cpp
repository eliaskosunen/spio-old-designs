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

#include "doctest.h"
#include "spio.h"

TEST_CASE("readable_buffer")
{
    std::string buf = "Lorem";
    io::readable_buffer r(io::make_span(buf));

    SUBCASE("read_elem")
    {
        char c = '\0';
        auto error = r.read(c);
        CHECK_FALSE(check_error(error));
        CHECK(c == 'L');
    }
    SUBCASE("read_bytes")
    {
        std::vector<char> buf2 = {0x7f, 0x20, 0};
        io::readable_buffer r2(io::make_span(buf2));

        std::array<char, 2> b{{0}};
        auto error = r2.read(io::make_span(b), io::bytes{2});
        CHECK_FALSE(check_error(error));
        CHECK(b[0] == 0x7f);
        CHECK(b[1] == 0x20);
    }
    SUBCASE("read_range")
    {
        std::array<char, 6> a{{0}};
        auto error = r.read(io::make_span(a), io::elements{5});
        CHECK_FALSE(check_error(error));
        REQUIRE(a[5] == '\0');
        CHECK(buf == a.data());
    }
    SUBCASE("read_range double")
    {
        std::array<char, 3> a{};
        auto error = r.read(io::make_span(a), io::elements{2});
        CHECK_FALSE(check_error(error));
        CHECK(a[0] == 'L');
        CHECK(a[1] == 'o');

        error = r.read(io::make_span(a), io::elements{3});
        CHECK_FALSE(check_error(error));
        CHECK(a[0] == 'r');
        CHECK(a[1] == 'e');
        CHECK(a[2] == 'm');
    }

    SUBCASE("seek and tell")
    {
        buf = "Lorem ipsum";
        r = io::readable_buffer{io::make_span(buf)};

        io::seek_type pos;
        auto error = r.tell(pos);
        CHECK_FALSE(check_error(error));
        CHECK(pos == 0);

        error = r.seek(io::seek_origin::CUR, 6);
        CHECK_FALSE(check_error(error));

        char c{};
        error = r.read(c);
        CHECK_FALSE(check_error(error));
        CHECK(c == 'i');

        error = r.tell(pos);
        CHECK_FALSE(check_error(error));
        CHECK(pos == 7);

        error = r.seek(io::seek_origin::SET, 0);
        CHECK_FALSE(check_error(error));

        error = r.read(c);
        CHECK_FALSE(check_error(error));
        CHECK(c == 'L');
    }
}
TEST_CASE("readable_wbuffer")
{
    std::wstring buf = L"Lorem";
    io::readable_wbuffer r(io::make_span(buf));
    SUBCASE("read_elem")
    {
        wchar_t c = 0;
        auto error = r.read(c);
        CHECK_FALSE(check_error(error));
        CHECK(c == L'L');
    }
    SUBCASE("read_bytes")
    {
        std::vector<char32_t> buf2 = {0x7f, 0xff, 0x42, 0x20, 0};
        io::readable_buffer32 r2(io::make_span(buf2));

        std::array<char, 4> b{{0}};
        auto error =
            r2.read(io::make_span(reinterpret_cast<char32_t*>(&b[0]),
                                  reinterpret_cast<char32_t*>(&b[0]) + 2),
                    io::bytes{4});
        CHECK_FALSE(check_error(error));
        CHECK(b[0] == 0x7f);
        CHECK(b[1] == 0x00);
        CHECK(b[2] == 0x00);
        CHECK(b[3] == 0x00);
    }
    SUBCASE("read_range")
    {
        std::array<wchar_t, 6> a{{0}};
        auto error = r.read(io::make_span(a), io::elements{5});
        CHECK_FALSE(check_error(error));
        REQUIRE(a[5] == 0);
        CHECK(buf == a.data());
    }
}
