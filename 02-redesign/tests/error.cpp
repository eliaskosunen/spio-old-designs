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

TEST_CASE("error")
{
    SUBCASE("default-ctor")
    {
        io::error e;
        CHECK(!e);
        CHECK(e.operator bool() == e.is_error());
        CHECK(!e.is_eof());
        CHECK_EQ(std::strcmp(e.message(), e.to_string()), 0);
        CHECK_EQ(std::strcmp(e.message(), "Success"), 0);
    }
    SUBCASE("io_error")
    {
        io::error e{io::io_error};
        CHECK(e);
        CHECK(e.operator bool() == e.is_error());
        CHECK(!e.is_eof());
        CHECK_EQ(std::strcmp(e.message(), e.to_string()), 0);
        CHECK_EQ(std::strcmp(e.message(), "IO error"), 0);
    }
    SUBCASE("end_of_file")
    {
        io::error e{io::end_of_file};
        CHECK(!e);
        CHECK(e.operator bool() == e.is_error());
        CHECK(e.is_eof());
        CHECK_EQ(std::strcmp(e.message(), e.to_string()), 0);
        CHECK_EQ(std::strcmp(e.message(), "EOF"), 0);
    }
}

#if SPIO_USE_EXCEPTIONS
TEST_CASE("failure")
{
    auto file = __FILE__;
    auto line = __LINE__;
    SUBCASE("default")
    {
        io::failure f{{}, file, line};

        CHECK(f.get_error().code == io::error{}.code);
        CHECK_EQ(std::strcmp(f.what(), io::error{}.message()), 0);
        CHECK(f.file() == file);
        CHECK(f.line() == line);
    }
    SUBCASE("message")
    {
        io::failure f{{}, "Message", file, line};

        CHECK(f.get_error().code == io::error{}.code);
        CHECK_EQ(std::strcmp(f.what(), "Message"), 0);
        CHECK(f.file() == file);
        CHECK(f.line() == line);
    }
    SUBCASE("sized message")
    {
        io::failure f{{}, "Message with size", 7, file, line};

        CHECK(f.get_error().code == io::error{}.code);
        CHECK_EQ(std::strcmp(f.what(), "Message"), 0);
        CHECK(f.file() == file);
        CHECK(f.line() == line);
    }
}

TEST_CASE("throw macros")
{
    SUBCASE("check_throws")
    {
        CHECK_THROWS_AS(SPIO_THROW_MSG("Message"), io::failure);
        CHECK_THROWS_AS(SPIO_THROW_EC(io::default_error), io::failure);
        CHECK_THROWS_AS(SPIO_THROW(io::default_error, "Message"), io::failure);
    }
    SUBCASE("check threw exceptions")
    {
        try {
            SPIO_THROW_MSG("Message");
        }
        catch (const io::failure& f) {
            CHECK(f.get_error().code == io::default_error);
            CHECK_EQ(std::strcmp(f.what(), "Message"), 0);
        }
    }
}
#endif
