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

TEST_CASE("filebuffer")
{
    SUBCASE("construct")
    {
        io::filebuffer buf{};

        // Avoid ODR-using to not get a linker error on C++14
        REQUIRE(buf.size() == std::size_t{io::filebuffer::default_size});
        CHECK(buf.mode() == io::filebuffer::BUFFER_FULL);
        buf.get_buffer();
        CHECK(buf.get_flushable_data().empty());

        auto f = buf.flush_if_needed([](io::const_byte_span, std::size_t&) {
            return std::error_code{};
        });
        CHECK(!f.first);
        CHECK(f.second == 0);
    }

    {
        std::string flush_target{};
        auto flush = [&](io::const_byte_span data, std::size_t& bytes) {
            flush_target.append(data.begin(), data.end());
            bytes = data.size_us();
            return std::error_code{};
        };

        SUBCASE("write: buffer_line")
        {
            io::filebuffer buf{io::filebuffer::BUFFER_LINE};

            auto w = buf.write(io::as_bytes(io::make_span("Data\n", 5)), flush);
            CHECK(w == 5);
            CHECK(flush_target == "Data\n");
            CHECK(std::string{buf.get_buffer()} == "Data\n");

            flush_target.clear();
            w = buf.write(io::as_bytes(io::make_span("Data", 4)), flush);
            CHECK(w == 4);
            CHECK(flush_target.empty());
            CHECK(std::string{buf.get_buffer()} == "Data\nData");
            CHECK(std::string{reinterpret_cast<const char*>(
                      buf.get_flushable_data().data())} == "Data");

            flush_target.clear();
            w = buf.write(io::as_bytes(io::make_span("Data\n", 5)), flush);
            CHECK(w == 5);
            CHECK(flush_target == "DataData\n");
            CHECK(std::string{buf.get_buffer()} == "Data\nDataData\n");
        }

        SUBCASE("write: buffer_full")
        {
            io::filebuffer buf{io::filebuffer::BUFFER_FULL};

            auto w = buf.write(io::as_bytes(io::make_span("Data\n", 5)), flush);
            CHECK(w == 5);
            CHECK(flush_target.empty());
            CHECK(std::string{buf.get_buffer()} == "Data\n");

            flush_target.clear();
            w = buf.write(io::as_bytes(io::make_span("Data", 4)), flush);
            CHECK(w == 4);
            CHECK(flush_target.empty());
            CHECK(std::string{buf.get_buffer()} == "Data\nData");
        }

        SUBCASE("write: buffer_line fill-up")
        {
            const auto buffer_size = 20ul;
            io::filebuffer buf{io::filebuffer::BUFFER_LINE, buffer_size};

            auto str = std::string{"Long line with line break\n"};
            auto w = buf.write(io::as_bytes(io::make_span(str)), flush);
            CHECK(w == str.size());
            CHECK(flush_target == str);

            flush_target.clear();
            str = std::string{"Long line without line break"};
            w = buf.write(io::as_bytes(io::make_span(str)), flush);
            CHECK(w == str.size());

            std::size_t n = 0;
            flush(buf.get_flushable_data(), n);
            buf.flag_flushed();
            CHECK(flush_target == str);
            CHECK(buf.get_flushable_data().empty());
        }
    }
}
