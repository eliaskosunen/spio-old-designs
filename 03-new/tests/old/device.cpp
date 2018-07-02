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

TEST_CASE("container_device")
{
    SUBCASE("source")
    {
        std::string str{"hello"};
        spio::basic_container_source<std::string> s{str};
        CHECK(s.container() == std::addressof(str));

        std::array<char, 5> a;
        CHECK(s.read(a) == 5);
        CHECK(std::equal(a.begin(), a.end(), str.begin()));
    }

    SUBCASE("sink")
    {
        std::string str;
        spio::basic_container_sink<std::string> s{str};
        CHECK(s.container() == std::addressof(str));

        std::string a{"hello"};
        CHECK(s.write(a) == 5);
        CHECK(std::equal(a.begin(), a.end(), str.begin()));
    }

    SUBCASE("device")
    {
        std::string str{"hello"};
        spio::basic_container_device<std::string> s{str};

        CHECK(s.seek(1, spio::seekdir::beg) == 1);
        CHECK(s.seek(0, spio::seekdir::cur) == 1);
        CHECK(s.seek(0, spio::seekdir::end) == 5);
        CHECK(s.seek(0, spio::seekdir::beg) == 0);
    }
}

TEST_CASE("file_device")
{
    SUBCASE("handle")
    {
        spio::basic_filehandle_device<char> f{stdout};
        CHECK(f.is_open());
        CHECK_EQ(f.handle(), stdout);
        CHECK(f.get_stdout_handle() == f.handle());
    }

    SUBCASE("sink & source")
    {
        std::string data{
            "Hello world!\nHello another line!\n\t123 one two three"};

        {
            spio::basic_file_sink<char> sink{"file.txt"};
            CHECK(sink.is_open());

            auto n = sink.write(data);
            CHECK(n == data.size());

            spio::basic_file_source<char> source{"file.txt"};
            std::string read_data(data.size(), 0);
            CHECK(source.is_open());

            n = source.read(read_data);
            CHECK(n == read_data.size());
            CHECK(read_data == data);

            CHECK(sink.seek(0, spio::seekdir::beg) == 0);
            CHECK(sink.seek(0, spio::seekdir::cur) == 0);
            CHECK(sink.seek(0, spio::seekdir::end) == data.size());
        }
    }
}

#if SPIO_HAS_NATIVE_FILEIO
TEST_CASE("native_file_device")
{
    SUBCASE("handle")
    {
        auto h =
            spio::basic_native_filehandle_device<char>::get_stdout_handle();
        spio::basic_native_filehandle_device<char> f{h};
        CHECK(f.is_open());
        CHECK(f.handle() == h);
        CHECK(f.get_stdout_handle() == f.handle());
    }

    SUBCASE("sink & source")
    {
        std::string data{
            "Hello world!\nHello another line!\n\t123 one two three"};

        {
            spio::basic_native_file_sink<char> sink{"file.txt"};
            CHECK(sink.is_open());

            auto n = sink.write(data);
            CHECK(n == data.size());

            spio::basic_native_file_source<char> source{"file.txt"};
            std::string read_data(data.size(), 0);
            CHECK(source.is_open());

            n = source.read(read_data);
            CHECK(n == read_data.size());
            CHECK(read_data == data);

            CHECK(sink.seek(0, spio::seekdir::beg) == 0);
            CHECK(sink.seek(0, spio::seekdir::cur) == 0);
            CHECK(sink.seek(0, spio::seekdir::end) == data.size());
        }
    }
}
#endif

TEST_CASE("memory_device")
{
    SUBCASE("source")
    {
        std::string str{"hello"};
        spio::basic_memory_source<char> s{str};
        CHECK(s.input() == spio::make_span(str));
    }

    SUBCASE("sink")
    {
        std::string str{"hello"};
        spio::basic_memory_sink<char> s{str};
        CHECK(s.output() == spio::make_span(str));
    }
}

TEST_CASE("null_device")
{
    SUBCASE("source")
    {
        spio::basic_null_source<char> s;
        CHECK(s.read({}) == -1);
    }
    SUBCASE("sink")
    {
        spio::basic_null_sink<char> s;
        CHECK(s.write({}) == 0);
    }
}
