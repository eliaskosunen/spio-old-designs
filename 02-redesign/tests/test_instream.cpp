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
#include "doctest.h"
#include "spio.h"

TEST_CASE("file_instream")
{
    SUBCASE("read")
    {
        io::owned_stdio_filehandle h("file.txt", io::stdio_filehandle::READ |
                                               io::stdio_filehandle::BINARY);
        REQUIRE(h);
        io::file_instream f{h.get()};
        std::vector<char> str(20, '\0');
        f.read(io::make_span(str));
        CHECK_EQ(std::strcmp("Lorem", str.data()), 0);
    }
    SUBCASE("getline")
    {
        io::owned_stdio_filehandle h("file.txt", io::stdio_filehandle::READ |
                                               io::stdio_filehandle::BINARY);
        REQUIRE(h);
        io::file_instream f{h.get()};
        std::vector<char> str(20, '\0');
        f.getline(io::make_span(str));
        CHECK_EQ(std::strcmp("Lorem ipsum", str.data()), 0);
    }
    SUBCASE("scan")
    {
        io::owned_stdio_filehandle h("file.txt", io::stdio_filehandle::READ |
                                               io::stdio_filehandle::BINARY);
        REQUIRE(h);
        io::file_instream f{h.get()};
        std::vector<char> str(20, '\0');
        std::vector<char> str2(20, '\0');
        f.scan(io::make_span(str), io::make_span(str2));
        CHECK_EQ(std::strcmp("Lorem", str.data()), 0);
        CHECK_EQ(std::strcmp("ipsum", str2.data()), 0);
    }
}
