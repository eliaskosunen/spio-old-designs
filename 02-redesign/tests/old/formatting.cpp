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

template <typename T>
static std::string format_spio(const T& val)
{
    auto w = io::buffer_outstream{};
    w.write(val);
    auto vec = w.consume_buffer();
    auto str = std::string{vec.begin(), vec.end()};
    return str;
}
template <typename T>
static std::string format_fmt(const T& val)
{
    auto w = io::buffer_outstream{};
    w.print("{}", val);
    auto vec = w.consume_buffer();
    auto str = std::string{vec.begin(), vec.end()};
    CHECK(str == fmt::format("{}", val));
    return str;
}

TEST_CASE("formatting")
{
    CHECK(format_spio(0) == format_fmt(0));
    CHECK(format_spio(-1) == format_fmt(-1));
    CHECK(format_spio(32768) == format_fmt(32768));
    CHECK(format_spio(0x7fffffff) == format_fmt(0x7fffffff));

    CHECK(format_spio("string") == format_fmt("string"));
    CHECK(format_spio("long string that doesn't fit in SSO") ==
          format_fmt("long string that doesn't fit in SSO"));
    CHECK(format_spio('c') == format_fmt('c'));
    //CHECK(format_spio(L'c') == format_fmt(L'c'));
    
    int value{};
    auto ptr = reinterpret_cast<void*>(&value);
    CHECK(format_spio(ptr) == format_fmt(ptr));

    CHECK(format_spio(true) == format_fmt(true));
    CHECK(format_spio(false) == format_fmt(false));

    CHECK(format_spio(0.) == format_fmt(0.));
    CHECK(format_spio(3.14) == format_fmt(3.14));
}
