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

#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include "benchmark/benchmark.h"
#include "spio/spio.h"

static std::vector<std::string> generate_data(size_t len)
{
    const std::vector<char> chars = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',  'A',  'B',
        'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',  'M',  'N',
        'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',  'Y',  'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',  'k',  'l',
        'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',  'w',  'x',
        'y', 'z', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\n', '\n', '\t'};
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, static_cast<int>(chars.size() - 1));

    std::vector<std::string> data;
    data.emplace_back();
    for (std::size_t i = 0; i < len; ++i) {
        auto c = chars[static_cast<size_t>(dist(rng))];
        if (io::is_space(c)) {
            data.emplace_back();
        }
        else {
            data.back().push_back(c);
        }
    }
    return data;
}

static void writestring_spio(benchmark::State& state)
{
    try {
        size_t bytes = 0;
        while (state.KeepRunning()) {
            state.PauseTiming();
            auto data = generate_data(static_cast<size_t>(state.range(0)));
            state.ResumeTiming();

            io::writable_buffer p{};
            /* io::writable_buffer w{}; */
            /* io::writer<decltype(w)> p{w}; */
            for (auto& n : data) {
                p.write(io::make_span(n));
                bytes += n.length();
            }
        }
        state.SetBytesProcessed(bytes);
    }
    catch (const io::failure& f) {
        state.SkipWithError(f.what());
    }
}

static void writestring_spio_static(benchmark::State& state)
{
    try {
        size_t bytes = 0;
        while (state.KeepRunning()) {
            state.PauseTiming();
            auto data = generate_data(static_cast<size_t>(state.range(0)));
            io::dynamic_writable_buffer<char> buffer;
            buffer.reserve(static_cast<size_t>(state.range(0)));
            state.ResumeTiming();

            io::writable_buffer p{std::move(buffer)};
            /* io::writable_buffer w{std::move(buffer)}; */
            /* io::writer<decltype(w)> p{w}; */
            for (auto& n : data) {
                p.write(io::make_span(n));
                bytes += n.length();
            }
        }
        state.SetBytesProcessed(bytes);
    }
    catch (const io::failure& f) {
        state.SkipWithError(f.what());
    }
}

static void writestring_ios(benchmark::State& state)
{
    size_t bytes = 0;
    while (state.KeepRunning()) {
        state.PauseTiming();
        auto data = generate_data(static_cast<size_t>(state.range(0)));
        state.ResumeTiming();

        std::stringstream ss{};
        for (auto& n : data) {
            ss << n;
            bytes += n.length();
        }
    }
    state.SetBytesProcessed(bytes);
}

BENCHMARK(writestring_spio)->Range(8, 8 << 8);
BENCHMARK(writestring_spio_static)->Range(8, 8 << 8);
BENCHMARK(writestring_ios)->Range(8, 8 << 8);
