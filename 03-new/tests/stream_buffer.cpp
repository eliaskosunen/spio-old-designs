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

TEST_CASE("source buffer")
{
    SUBCASE("construct")
    {
        spio::basic_default_source_buffer<char> buf{};
        CHECK(buf.size() == 0);
    }

    {
        spio::basic_default_source_buffer<char> buf{};
        std::vector<char> data{'1', '2', '3', '4', '5'};
        std::vector<char> in(5, 0);

        SUBCASE("test")
        {
            buf.push(data);

            buf.read(in);
            CHECK(data == in);
            CHECK(buf.size() == 0);

            buf.push(data);

            buf.read(spio::make_span(in.data(), 4));
            CHECK(spio::make_span(in.data(), 4) ==
                  spio::make_span(data.data(), 4));

            buf.push(data);
            buf.read(in);
            CHECK(in[0] == data[4]);
            CHECK(spio::make_span(in).last(4) ==
                  spio::make_span(data).first(4));
            CHECK(buf.size() == 1);

            buf.read(spio::make_span(in.data(), 1));
            CHECK(in[0] == data[4]);
            CHECK(buf.size() == 0);
        }
    }
}
