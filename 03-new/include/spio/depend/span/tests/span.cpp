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

#include "span.h"
#include <numeric>
#include <vector>
#include "doctest.h"

static std::array<int, 10>& get_arr()
{
    static std::array<int, 10> arr = [&]() {
        std::array<int, 10> tmp{{}};
        std::iota(tmp.begin(), tmp.end(), 0);
        return tmp;
    }();
    return arr;
}

TEST_CASE("constructors")
{
    SUBCASE("default")
    {
        span::span<int, 0> s1{};
        CHECK(s1.size() == 0);
        CHECK(s1.data() == nullptr);

        span::span<int> s2{};
        CHECK(s2.size() == 0);
        CHECK(s2.data() == nullptr);

        // static_assert fail
        // span::span<int, 1> s3{};

        span::span<int> s4{nullptr};
        CHECK(s4.size() == 0);
        CHECK(s4.data() == nullptr);
        CHECK(s2 == s4);
    }

    auto& arr = get_arr();
    std::vector<int> vec{};
    std::copy(arr.begin(), arr.end(), std::back_inserter(vec));

    SUBCASE("pointer")
    {
        span::span<int, 10> s1{&arr[0], 10};
        CHECK(s1.size() == arr.size());
        CHECK(s1.data() == arr.data());

        span::span<int, 10> s2{&arr[0], &arr[9] + 1};
        CHECK(s2.size() == arr.size());
        CHECK(s2.data() == arr.data());
        CHECK(s1 == s2);
    }

    SUBCASE("iterator")
    {
        span::span<int, 10> s1{arr.begin(), 10};
        CHECK(s1.size() == arr.size());
        CHECK(s1.data() == arr.data());

        span::span<int, 10> s2{arr.begin(), arr.end()};
        CHECK(s2.size() == arr.size());
        CHECK(s2.data() == arr.data());
        CHECK(s1 == s2);
    }

    SUBCASE("array")
    {
        span::span<int, 10> s1{arr};
        CHECK(s1.size() == arr.size());
        CHECK(s1.data() == arr.data());

        auto carr = reinterpret_cast<int(*)[10]>(arr.data());
        span::span<int, 10> s2{*carr};
        CHECK(s2.size() == arr.size());
        CHECK(s2.data() == arr.data());
        CHECK(s1 == s2);
    }

    SUBCASE("container")
    {
        span::span<int> s1{vec};
        CHECK(s1.size() == vec.size());
        CHECK(s1.data() == vec.data());
        CHECK(s1 == span::span<int, 10>{arr});
    }

    SUBCASE("special copy")
    {
        span::span<int, 10> s1{arr};
        span::span<int> s2{s1};
        CHECK(s1.size() == s2.size());
        CHECK(s1.data() == s2.data());
        CHECK(s1 == s2);
    }
}

TEST_CASE("subviews")
{
    span::span<int, 10> s{get_arr()};

    SUBCASE("first")
    {
        auto a = s.first<3>();
        auto b = s.first(3);

        CHECK(a == b);
        CHECK(a.size() == 3);
        CHECK(b.size() == 3);
        CHECK(a.data() == s.data());
        CHECK(b.data() == s.data());

        CHECK(a[0] == 0);
        CHECK(a[1] == 1);
        CHECK(a[2] == 2);
        CHECK(a[0] == b[0]);
        CHECK(a[1] == b[1]);
        CHECK(a[2] == b[2]);

        CHECK(s.first(0).size() == 0);
        CHECK(s.first(0).data() == s.data());
    }

    SUBCASE("last")
    {
        auto a = s.last<3>();
        auto b = s.last(3);

        CHECK(a == b);
        CHECK(a.size() == 3);
        CHECK(b.size() == 3);
        CHECK(a.data() == s.data() + (s.size() - 3));
        CHECK(b.data() == s.data() + (s.size() - 3));

        CHECK(a[0] == 7);
        CHECK(a[1] == 8);
        CHECK(a[2] == 9);
        CHECK(a[0] == b[0]);
        CHECK(a[1] == b[1]);
        CHECK(a[2] == b[2]);
    }

    SUBCASE("subspan")
    {
        auto a = s.subspan<3, 3>();
        auto b = s.subspan(3, 3);

        CHECK(a == b);
        CHECK(a.size() == 3);
        CHECK(b.size() == 3);
        CHECK(a.data() == s.data() + 3);
        CHECK(b.data() == s.data() + 3);

        CHECK(a[0] == 3);
        CHECK(a[1] == 4);
        CHECK(a[2] == 5);
        CHECK(a[0] == b[0]);
        CHECK(a[1] == b[1]);
        CHECK(a[2] == b[2]);
    }

    SUBCASE("subspan dynamic_extent")
    {
        auto a = s.subspan<7>();
        auto b = s.subspan(7);

        CHECK(a == b);
        CHECK(a == s.last<3>());
        CHECK(a.size() == 3);
        CHECK(b.size() == 3);
        CHECK(a.data() == s.data() + 7);
        CHECK(b.data() == s.data() + 7);

        CHECK(a[0] == 7);
        CHECK(a[1] == 8);
        CHECK(a[2] == 9);
        CHECK(a[0] == b[0]);
        CHECK(a[1] == b[1]);
        CHECK(a[2] == b[2]);
    }
}

TEST_CASE("observers")
{
    SUBCASE("full")
    {
        span::span<int, 10> s{get_arr()};

        CHECK(s.size() == 10);
        CHECK(s.size() == s.length());
        CHECK(s.size() ==
              static_cast<typename decltype(s)::index_type>(s.size_us()));
        CHECK(s.size_us() == s.length_us());
        CHECK(!s.empty());

        CHECK(s.size_bytes() == 10 * sizeof(int));
        CHECK(s.size_bytes() == s.length_bytes());
        CHECK(s.size_bytes() ==
              static_cast<typename decltype(s)::index_type>(s.size_bytes_us()));
        CHECK(s.size_bytes_us() == s.length_bytes_us());
    }

    SUBCASE("empty")
    {
        span::span<int> s{};

        CHECK(s.size() == 0);
        CHECK(s.size_us() == 0);
        CHECK(s.length() == 0);
        CHECK(s.length_us() == 0);
        CHECK(s.size_bytes() == 0);
        CHECK(s.size_bytes_us() == 0);
        CHECK(s.length_bytes() == 0);
        CHECK(s.length_bytes_us() == 0);
        CHECK(s.empty());
    }
}

TEST_CASE("accessors")
{
    span::span<int, 10> s{get_arr()};

    SUBCASE("unchecked")
    {
        for (auto i = 0; i < s.size(); ++i) {
            CHECK(s[i] == i);
            CHECK(s[i] == s(i));
        }
    }

    SUBCASE("checked")
    {
        for (auto i = 0; i < s.size(); ++i) {
            CHECK(s.at(i) == i);
        }
        CHECK_THROWS_AS(s.at(-1), span::exception);
        CHECK_THROWS_AS(s.at(10), span::exception);
    }

    SUBCASE("direct")
    {
        CHECK(s.data() == get_arr().data());
        for (auto i = 0; i < s.size(); ++i) {
            CHECK(*(s.data() + i) == i);
        }
    }
}

TEST_CASE("iterators")
{
    span::span<int, 10> s{get_arr()};

    SUBCASE("regular")
    {
        CHECK(*s.begin() == *get_arr().begin());
        CHECK(std::distance(s.begin(), s.end()) == s.size());

        {
            auto span_it = s.begin();
            auto arr_it = get_arr().begin();
            auto i = 0;
            for (; span_it != s.end(); ++span_it, (void)++arr_it, (void)++i) {
                CHECK(*span_it == *arr_it);
                CHECK(*span_it == s[i]);
            }
            CHECK(i == s.size());
            CHECK(span_it == s.end());
            CHECK(arr_it == get_arr().end());
        }
    }

    SUBCASE("reverse")
    {
        CHECK(*s.rbegin() == *get_arr().rbegin());
        CHECK(s.rbegin() == span::detail::make_reverse_iterator(s.end()));
        CHECK(std::distance(s.rbegin(), s.rend()) == s.size());

        {
            auto span_it = s.rbegin();
            auto arr_it = get_arr().rbegin();
            auto i = 9;
            for (; span_it != s.rend(); ++span_it, (void)++arr_it, (void)--i) {
                CHECK(*span_it == *arr_it);
                CHECK(*span_it == s[i]);
            }
            CHECK(i == -1);
            CHECK(span_it == s.rend());
            CHECK(arr_it == get_arr().rend());
        }
    }

    SUBCASE("const")
    {
        CHECK(s.begin() == s.cbegin());
        CHECK(s.end() == s.cend());
        CHECK(s.rbegin() == s.crbegin());
        CHECK(s.rend() == s.crend());

        {
            auto it = s.begin();
            auto cit = s.cbegin();
            for (; cit != s.cend(); ++it, (void)++cit) {
                CHECK(it == cit);
                CHECK(*it == *cit);
            }
            CHECK(it == cit);
        }
    }
}

TEST_CASE("comparison operators")
{
    span::span<int, 10> s{get_arr()};

    SUBCASE("equal")
    {
        CHECK(s == s);
        CHECK(s == span::span<int, 10>{get_arr()});
        CHECK(span::span<int, 10>{get_arr()} == s);
        CHECK(s == span::span<int>{get_arr()});
        CHECK(span::span<int>{get_arr()} == s);

        CHECK(!(s == s.first(9)));
        CHECK(!(s == span::span<int>{}));

        CHECK_EQ(s == s, std::equal(s.begin(), s.end(), s.begin()));
    }

    SUBCASE("not equal")
    {
        CHECK_EQ(s != s, !(s == s));
    }

    std::array<int, 10> arr{{}};
    std::iota(arr.begin(), arr.end(), 1);
    span::span<int, 10> s2{arr};

    SUBCASE("less than")
    {
        CHECK(s < s2);
        CHECK_EQ(s < s2, std::lexicographical_compare(s.begin(), s.end(),
                                                      s2.begin(), s2.end()));
    }

    SUBCASE("ordering")
    {
        CHECK_EQ(s <= s2, !(s > s2));
        CHECK_EQ(s > s2, s2 < s);
        CHECK_EQ(s >= s2, !(s < s2));
    }
}

#if SPAN_HAS_DEDUCTION_GUIDES
TEST_CASE("deduction guides")
{
    auto& arr = get_arr();
    auto vec = std::vector<int>{arr.begin(), arr.end()};

    CHECK(span::span{&arr[0], 10} == span::span<int>{&arr[0], 10});
    CHECK(span::span{&arr[0], &arr[9] + 1} ==
          span::span<int>{&arr[0], &arr[9] + 1});
    CHECK(span::span{arr} == span::span<int>{arr});
    CHECK(span::span{vec} == span::span<int>{vec});

    auto carr = reinterpret_cast<int(*)[10]>(arr.data());
    CHECK(span::span{*carr} == span::span<int, 10>{*carr});
}
#endif

TEST_CASE("make_span")
{
    auto& arr = get_arr();
    auto carr = reinterpret_cast<int(*)[10]>(arr.data());
    auto vec = std::vector<int>{arr.begin(), arr.end()};

    CHECK(span::make_span(&arr[0], 10) == span::span<int>{&arr[0], 10});
    CHECK(span::make_span(&arr[0], &arr[9] + 1) ==
          span::span<int>{&arr[0], &arr[9] + 1});
    CHECK(span::make_span(arr.begin(), arr.end()) ==
          span::span<int>{arr.begin(), arr.end()});
    CHECK(span::make_span(*carr) == span::span<int>{*carr});
    CHECK(span::make_span(arr) == span::span<int>{arr});
    CHECK(span::make_span(vec) == span::span<int>{vec});
}

TEST_CASE("as_bytes")
{
    auto& arr = get_arr();
    auto b = span::as_bytes(span::make_span(arr));
    auto wb = span::as_writable_bytes(span::make_span(arr));

    CHECK(b.size() == wb.size());
    CHECK(b.size_bytes() == b.size());
    CHECK(b.size_bytes() == span::span<int>{arr}.size_bytes());
}

template <typename T, span::extent_t N>
static T& last_element(span::span<T, N> s)
{
    if (s.empty()) {
        throw std::runtime_error(
            "Cannot take the last element of an empty span!");
    }
    return *s.rbegin();
}

TEST_CASE("example")
{
    int carr[10] = {};
    std::iota(std::begin(carr), std::end(carr), 0);
    std::array<int, 10> arr{{}};
    std::copy(std::begin(carr), std::end(carr), arr.begin());
    std::vector<int> vec(arr.begin(), arr.end());

    CHECK(last_element(span::make_span(carr)) ==
          last_element(span::make_span(vec)));
    CHECK(last_element(span::make_span(arr)) ==
          last_element(span::make_span(vec)));
}

TEST_CASE("array")
{
    std::array<int, 10> a{{0}};
    std::iota(a.begin(), a.end(), 0);
    span::span<int, 10> s(&*a.begin(), &*a.begin() + 10);
    span::span<decltype(a)::value_type, static_cast<span::extent_t>(a.size())>
        s2(&a[0], static_cast<span::extent_t>(a.size()));
    CHECK(s[0] == 0);
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
