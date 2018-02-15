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
#include <numeric>

TEST_CASE("memory_device")
{
    SUBCASE("default-construct")
    {
        spio::memory_device dev{};
        CHECK(!dev.buffer());
    }

    SUBCASE("device")
    {
        std::array<char, 10> arr{{}};
        std::iota(arr.begin(), arr.end(), 'a');
        auto buf = spio::make_span(arr);
        spio::memory_device dev{buf};

        CHECK(dev.buffer() == buf.data());
        CHECK(dev.input() == buf);
        CHECK(dev.output() == buf);
    }

    SUBCASE("sink")
    {
        std::array<char, 10> arr{{}};
        std::iota(arr.begin(), arr.end(), 'a');
        auto buf = spio::make_span(arr);
        spio::memory_sink dev{buf};

        CHECK(dev.buffer() == buf.data());
        CHECK(dev.output() == buf);
    }

    SUBCASE("source")
    {
        std::array<char, 10> arr{{}};
        std::iota(arr.begin(), arr.end(), 'a');
        auto buf = spio::make_span(arr);
        spio::memory_source dev{buf};

        CHECK(dev.buffer() == buf.data());
        CHECK(dev.input() == buf);
    }

    SUBCASE("wide device")
    {
        std::array<wchar_t, 10> arr{{}};
        std::iota(arr.begin(), arr.end(), L'a');
        auto buf = spio::make_span(arr);
        spio::wmemory_device dev{buf};

        CHECK(dev.buffer() == buf.data());
        CHECK(dev.input() == buf);
        CHECK(dev.output() == buf);
    }
}

TEST_CASE("container_sink")
{
    SUBCASE("default-construct")
    {
        spio::string_sink dev{};
        CHECK(!dev.container());
    }

    SUBCASE("string write")
    {
        std::string str{};
        spio::string_sink dev{str};
        CHECK(dev.container() == &str);

        auto s = std::string{"Hello world!"};
        auto n = dev.write(s);
        CHECK(n == s.size());
        CHECK(str == s);
    }

    SUBCASE("string seek-n-write")
    {
        std::string str{};
        spio::string_sink dev{str};

        CHECK(dev.seek(0, spio::seekdir::cur) == 0);
        CHECK(dev.seek(0, spio::seekdir::beg) == 0);

        CHECK(dev.write(spio::make_span("String", 6)) == 6);
        CHECK(str == "String");

        CHECK(dev.seek(0, spio::seekdir::cur) == 6);
        CHECK(dev.seek(0, spio::seekdir::beg) == 0);

        CHECK(dev.write(spio::make_span("String", 6)) == 6);
        CHECK(str == "StringString");

        CHECK(dev.seek(0, spio::seekdir::cur) == 6);
        CHECK(dev.seek(0, spio::seekdir::end) == 12);

        CHECK(dev.write(spio::make_span("String", 6)) == 6);
        CHECK(str == "StringStringString");
    }

    SUBCASE("wstring")
    {
        std::wstring str{};
        spio::wstring_sink dev{str};
        CHECK(dev.container() == &str);

        auto s = std::wstring{L"Hello world!"};
        auto n = dev.write(s);
        CHECK(n == s.size());
        CHECK(str == s);
    }

    SUBCASE("vector")
    {
        std::vector<char> str{};
        spio::basic_vector_sink<char> dev{str};
        CHECK(dev.container() == &str);

        auto s = std::string{"Hello world!"};
        auto n = dev.write(s);
        CHECK(n == s.size());

        str.push_back('\0');
        CHECK(s == str.data());
    }
}
