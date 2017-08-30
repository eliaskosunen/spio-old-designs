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

#include <doctest.h>
#include <iostream>
#include "spio.h"

TEST_CASE("util")
{
    SUBCASE("contains")
    {
        CHECK(io::contains<int, short, int, long>::value);
        CHECK_FALSE(io::contains<float, short, int, long>::value);
        CHECK_FALSE(io::contains<unsigned int, short, int, long>::value);
        CHECK_FALSE(io::contains<const void*, short, int, long>::value);
    }

    SUBCASE("char_to_int")
    {
        CHECK(0 == io::char_to_int<int>('0'));
        CHECK(3 == io::char_to_int<int>('3'));
        CHECK(6 == io::char_to_int<int>('6'));
        CHECK(9 == io::char_to_int<int>('9'));

        CHECK(0 == io::char_to_int<int>('0', 2));
        CHECK(1 == io::char_to_int<int>('1', 2));

        CHECK(10 == io::char_to_int<int>('a', 16));
        CHECK(15 == io::char_to_int<int>('F', 16));
    }

    SUBCASE("is_space")
    {
        CHECK(io::is_space(' '));
        CHECK(io::is_space('\n'));
        CHECK(io::is_space('\r'));
        CHECK(io::is_space('\t'));
        CHECK(io::is_space('\v'));

        CHECK_FALSE(io::is_space('a'));

        std::array<char, 3> alternative_spaces{{'a', '5', '!'}};
        auto s = io::make_span(alternative_spaces);
        CHECK_FALSE(io::is_space(' ', s));
        CHECK(io::is_space('a', s));
        CHECK(io::is_space('5', s));
        CHECK(io::is_space('!', s));
    }

    SUBCASE("str_to_floating")
    {
#define STRTOD_CHECK(s, d, r)                                       \
    {                                                               \
        char* end;                                                  \
        double result = io::str_to_floating<double, char>(s, &end); \
        CHECK(result == doctest::Approx(d));                        \
        CHECK(std::strcmp(end, r) == 0);                            \
        if (std::strcmp(end, r) != 0) {                             \
            std::cout << end << std::endl;                          \
        };                                                          \
    }

        {
            char* end;
            double pz = 0.0;
            double nz = -0.0;

            double parsed = io::str_to_floating<double, char>("0.0", &end);
            CHECK(std::memcmp(&parsed, &pz, sizeof pz) == 0);

            parsed = io::str_to_floating<double, char>("-0.0", &end);
            CHECK(std::memcmp(&parsed, &nz, sizeof nz) == 0);
        }

        STRTOD_CHECK("0", 0, "");
        STRTOD_CHECK("-0", 0, "");
        STRTOD_CHECK("12", 12, "");
        STRTOD_CHECK("23.5", 23.5, "");
        STRTOD_CHECK("-14", -14, "");
        STRTOD_CHECK("-", 0, "-");
        STRTOD_CHECK("-2-a", -2, "-a");
        STRTOD_CHECK("-2a", -2, "a");
        STRTOD_CHECK("0.036", 0.036, "");
        STRTOD_CHECK("12.5E2", 12.5E2, "");
        STRTOD_CHECK("12.5e-3", 12.5E-3, "");
        STRTOD_CHECK("12.5E0", 12.5E0, "");
        STRTOD_CHECK("12.5e", 12.5, "");
        STRTOD_CHECK("12.5E-", 12.5, "");
        STRTOD_CHECK("", 0, "");
        STRTOD_CHECK("a", 0, "a");
        STRTOD_CHECK("E10", 0, "E10");
        STRTOD_CHECK("-e10", 0, "-e10");
        STRTOD_CHECK("-0E10", 0, "");
        STRTOD_CHECK(".3", 0.3, "");
        STRTOD_CHECK("-.3", -0.3, "");
        io::str_to_floating<double, char>("42C",
                                          nullptr);  // tests endptr == null
        STRTOD_CHECK("+12", 12, "");
        STRTOD_CHECK("+-12", 0, "+-12");
        STRTOD_CHECK("12.5E+3", 12.5E+3, "");
        STRTOD_CHECK("12.5e+-3", 12.5, "-3");
    }
}
