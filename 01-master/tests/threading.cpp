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
#include <thread>
#include "doctest.h"
#include "spio.h"

TEST_CASE("multithreaded buffer_outstream")
{
    auto fn = [](io::mt_buffer_outstream& stream, auto id) {
        for (auto i = 0; i < 10; ++i) {
            auto s = stream.lock();
            s->println("Thread {}, Row {}", id, i);
        }
    };
    auto s = io::buffer_outstream{};
    io::mt_buffer_outstream mt_s{std::move(s)};
    std::vector<std::thread> threads;
    for (auto i = 0; i < 5; ++i) {
        threads.emplace_back(fn, std::ref(mt_s), i);
    }
    for (auto& t : threads) {
        t.join();
    }
    CHECK(mt_s.lock()->get_buffer().size() ==
          50 * std::strlen("Thread 0, Row 1\n"));
}

TEST_CASE("multithreaded stdout")
{
    auto fn = [](decltype(io::mt_sout()) stream, auto id) {
        for (auto i = 0; i < 3; ++i) {
            auto s = stream.lock();
            s->println("Thread {}, Row {}", id, i);
        }
    };
    std::vector<std::thread> threads;
    for (auto i = 0; i < 5; ++i) {
        threads.emplace_back(fn, std::ref(io::mt_sout()), i);
    }
    for (auto& t : threads) {
        t.join();
    }
}
