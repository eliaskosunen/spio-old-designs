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

TEST_CASE("stream")
{
    SUBCASE("container out")
    {
        auto str = std::string{};
        spio::basic_stream<spio::string_sink> s(str);
        s.write("Hello world!");
        s.print("{}", str);

        spio::basic_stream<spio::filehandle_device> sout{stdout};
        sout.print("{}{}", "Hello world!", "\n");
    }

    SUBCASE("container in")
    {
        auto str = std::string{"Hello world!"};
        spio::basic_stream<spio::basic_container_source<std::string>> s(str);

        std::string r(64, '\0');
        auto span = spio::make_span(r);
        s.scan("{}", span);
        CHECK_EQ(std::strcmp(r.c_str(), "Hello"), 0);
    }
}
