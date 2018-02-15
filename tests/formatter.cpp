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

TEST_CASE("formatter")
{
    SUBCASE("char")
    {
        spio::basic_fmt_formatter<char> fmt;
        CHECK(fmt("str") == fmt::format("str"));
        CHECK(fmt(0) == fmt::format("{}", 0));
        CHECK(fmt(0x8000) == fmt::format("{}", 0x8000));
        CHECK(fmt(-1) == fmt::format("{}", -1));
        CHECK(fmt(3.14) == fmt::format("{}", 3.14));

        CHECK(fmt.format("{} {}", 1, 2) == fmt::format("{} {}", 1, 2));
    }
    SUBCASE("wchar_t")
    {
        spio::basic_fmt_formatter<wchar_t> fmt;
        spio::codeconv<char, wchar_t> conv;

        CHECK(fmt(L"str") == conv(fmt::format("str")));
        CHECK(fmt(0) == conv(fmt::format("{}", 0)));
        CHECK(fmt(0x8000) == conv(fmt::format("{}", 0x8000)));
        CHECK(fmt(-1) == conv(fmt::format("{}", -1)));
        CHECK(fmt(3.14) == conv(fmt::format("{}", 3.14)));

        CHECK(fmt.format(L"{} {}", 1, 2) == conv(fmt::format("{} {}", 1, 2)));
    }
}
