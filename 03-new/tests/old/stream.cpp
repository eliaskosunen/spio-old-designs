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

TEST_CASE("state")
{
    spio::basic_stream<spio::null_device> s;

    CHECK(s.rdstate() == spio::iostate::good);
    CHECK(s.rdstate() == 0);
    CHECK(s.good());
    CHECK(!s.bad());
    CHECK(!s.fail());
    CHECK(!s.eof());
    CHECK(s);

    s.clear(spio::iostate::fail);
    CHECK(s.rdstate() == spio::iostate::fail);
    CHECK(!s.good());
    CHECK(!s.bad());
    CHECK(s.fail());
    CHECK(!s.eof());
    CHECK(!s);

    s.setstate(spio::iostate::eof);
    CHECK_EQ(s.rdstate(), spio::iostate::fail | spio::iostate::eof);
    CHECK(!s.good());
    CHECK(!s.bad());
    CHECK(s.fail());
    CHECK(s.eof());
    CHECK(!s);

    s.clear_eof();
    CHECK(s.rdstate() == spio::iostate::fail);
    CHECK(s.fail());
    CHECK(!s.eof());

    s.clear();
    CHECK(s.rdstate() == spio::iostate::good);
    CHECK(s.good());
    CHECK(s);
}

struct user_type {
    int num;
};

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

TEST_CASE("wide print")
{
    auto str = std::wstring{};
    spio::basic_stream<spio::wstring_sink> s(str);
    SUBCASE("string")
    {
        s.print(L"Hello world!");
        CHECK(str == L"Hello world!");

        str.clear();
        s.seek(0, spio::seekdir::beg);
        s.print(L"{}{}", L"Hello world!", L'\n');
        CHECK(str == L"Hello world!\n");
    }
    SUBCASE("various")
    {
        s.print(L"{} {} {} {}", 100, 3.14, static_cast<void*>(nullptr), true);
        CHECK(str == L"100 3.14 0x0 true");
    }
}

template <typename CharT>
void custom_scan(spio::basic_stream_ref<CharT, spio::input>& s,
                 const CharT*& format,
                 user_type& val)
{
    spio::codeconv<char, CharT> conv;
    s.scan(conv("{}").c_str(), val.num);
    spio::skip_format(format);
}

TEST_CASE("scan")
{
    SUBCASE("span")
    {
        auto str = std::string{"Hello world"};
        spio::basic_stream<spio::string_source> s(str);

        std::string r(5, '\0');
        auto span = spio::make_span(r);
        s.scan("{}", span);
        CHECK_EQ(std::strcmp(r.c_str(), "Hello"), 0);

        s.scan("{}", span);
        CHECK_EQ(std::strcmp(r.c_str(), "world"), 0);
    }
    SUBCASE("int")
    {
        auto str = std::string{"42 240 7f 1010 256"};
        spio::basic_stream<spio::string_source> s(str);

        int n;
        s.scan("{}", n);
        CHECK(n == 42);

        s.scan("{}", n);
        CHECK(n == 240);

        s.scan("{x}", n);
        CHECK(n == 0x7f);

        s.scan("{b}", n);
        CHECK(n == 10);

        unsigned u;
        s.scan("{}", u);
        CHECK(u == 256);
    }
    SUBCASE("float")
    {
        auto str = std::string{"3.14159"};
        spio::basic_stream<spio::string_source> s(str);

        double d;
        s.scan("{}", d);
        CHECK(d == doctest::Approx(3.14159));
    }
    SUBCASE("bool")
    {
        auto str = std::string{"1 false"};
        spio::basic_stream<spio::string_source> s(str);

        bool b1, b2;
        s.scan("{} {}", b1, b2);
        CHECK(b1);
        CHECK(!b2);
    }
    SUBCASE("string")
    {
        auto str = std::string{"This-is-a-very-long-word"};
        spio::basic_stream<spio::string_source> s(str);

        std::string res;
        s.scan("{}", res);
        CHECK(str == res);
    }
    SUBCASE("user type")
    {
        auto str = std::string{"255"};
        spio::basic_stream<spio::string_source> s(str);

        user_type ut;
        s.scan("{}", ut);
        CHECK(ut.num == 255);
    }
}

TEST_CASE("wide scan")
{
    SUBCASE("span")
    {
        auto str = std::wstring{L"Hello world"};
        spio::basic_stream<spio::wstring_source> s(str);

        std::wstring r(5, L'\0');
        auto span = spio::make_span(r);
        s.scan(L"{}", span);
        CHECK_EQ(std::wcscmp(r.c_str(), L"Hello"), 0);

        s.scan(L"{}", span);
        CHECK_EQ(std::wcscmp(r.c_str(), L"world"), 0);
    }
    SUBCASE("int")
    {
        auto str = std::wstring{L"42 240 7f 1010 256"};
        spio::basic_stream<spio::wstring_source> s(str);

        int n;
        s.scan(L"{}", n);
        CHECK(n == 42);
    }
    SUBCASE("float")
    {
        auto str = std::wstring{L"3.14159"};
        spio::basic_stream<spio::wstring_source> s(str);

        double d;
        s.scan(L"{}", d);
        CHECK(d == doctest::Approx(3.14159));
    }
    SUBCASE("string")
    {
        auto str = std::wstring{L"This-is-a-very-long-word"};
        spio::basic_stream<spio::wstring_source> s(str);

        std::wstring res;
        s.scan(L"{}", res);
        CHECK(str == res);
    }
    SUBCASE("user type")
    {
        auto str = std::wstring{L"255"};
        spio::basic_stream<spio::wstring_source> s(str);

        user_type ut;
        s.scan(L"{}", ut);
        CHECK(ut.num == 255);
    }
}

TEST_CASE("getline")
{
    auto str = std::string{"Hello world!\nHello another line!"};
    spio::string_instream s(str);

    SUBCASE("fixed")
    {
        std::string c(12, 0);
        auto n = s.getline(c);
        CHECK(n == 12);
        CHECK(c == "Hello world!");

        c = std::string(19, 0);
        n = s.getline(c);
        CHECK(n == 19);
        CHECK(c == "Hello another line!");
    }

    SUBCASE("fixed oversized")
    {
        std::string c(25, 0);
        s.getline(c);
        auto hellow = "Hello world!";
        CHECK(std::equal(hellow, hellow + std::strlen(hellow), std::begin(c)));

        auto hellow_another = "Hello another line!";
        s.getline(c);
        CHECK(std::equal(hellow_another,
                         hellow_another + std::strlen(hellow_another),
                         std::begin(c)));
    }

    SUBCASE("fixed oversized delim")
    {
        std::string c(25, 0);
        s.getline(c, ' ');
        auto hello = "Hello";
        CHECK(std::equal(hello, hello + std::strlen(hello), std::begin(c)));

        auto world = "world!";
        c = std::string(25, 0);
        s.getline(c, ' ');
        CHECK(std::equal(world, world + std::strlen(world), std::begin(c)));
    }

    SUBCASE("dynamic")
    {
        std::string c;
        spio::getline(s, c);
        CHECK(c == "Hello world!");

        c.clear();
        spio::getline(s, c);
        CHECK(c == "Hello another line!");
    }

    SUBCASE("dynamic delim")
    {
        std::string c;
        spio::getline(s, c, ' ');
        CHECK(c == "Hello");

        spio::getline(s, c, ' ');
        CHECK(c == "world!\nHello");
    }
}
