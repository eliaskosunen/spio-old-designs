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

#include <numeric>
#include "spio.h"
#include "doctest.h"

TEST_CASE("span")
{
    SUBCASE("array")
    {
        std::array<int, 10> a{{0}};
        std::iota(a.begin(), a.end(), 0);
        io::span<decltype(a)::value_type, a.size()> s(a.begin(), a.end());
        io::span<decltype(a)::value_type, a.size()> s2(&a[0], a.size());
        CHECK(s[0] == 0);
        CHECK(s(1) == 1);
        CHECK(s.at(2) == 2);
        CHECK(s.size() == 10);
        CHECK_FALSE(s.empty());
        auto first = s.first<4>();
        CHECK(first[3] == 3);
        auto last = s.last(2);
        CHECK(last[1] == 9);
        auto middle = s.subspan<5, 2>();
        CHECK(middle[0] == 5);
    }
    SUBCASE("copy_contiguous")
    {
        std::array<uint16_t, 10> a{};
        std::fill(a.begin(), a.end() - 1, 0xffff);
        a[9] = 0;
        std::array<uint32_t, 5> b{{0}};
        io::span<uint16_t, 10> sa{a.begin(), a.end()};
        io::span<uint32_t, 5> sb{b.begin(), b.end()};
        io::copy_contiguous(sa, sb);
        CHECK(b[0] == 0xffffffff);
        CHECK(b[3] == 0xffffffff);
        CHECK(b[4] == 0xffff);
    }
}
