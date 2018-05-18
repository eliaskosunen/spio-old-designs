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

#include <benchmark/benchmark.h>
#include <spio/spio.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>

static const std::string& get_data(std::size_t len)
{
    static const std::string str = [&]() {
        spio::file_instream f("string_input.txt");
        auto size = std::min(
            static_cast<std::size_t>(f.seek(0, spio::seekdir::end)), len);
        std::string buf(size, 0);
        f.seek(0, spio::seekdir::beg);
        f.read(buf);
        return buf;
    }();
    return str;
}

static void readstring_spio(benchmark::State& state)
{
    try {
        std::size_t bytes = 0;
        for (auto _ : state) {
            state.PauseTiming();
            auto data = get_data(static_cast<size_t>(state.range(0)));
            std::string word;
            state.ResumeTiming();

            spio::string_instream p{data};
            while (p && !p.eof()) {
                p.scan(word);
                bytes += word.length();
                benchmark::DoNotOptimize(word);
                benchmark::DoNotOptimize(p);
                benchmark::DoNotOptimize(data);
            }
        }
        state.SetBytesProcessed(static_cast<int64_t>(bytes));
    }
    catch (const spio::failure& f) {
        state.SkipWithError(f.what());
    }
}

static void readstring_ios(benchmark::State& state)
{
    std::size_t bytes = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto data = get_data(static_cast<size_t>(state.range(0)));
        std::string word;
        state.ResumeTiming();

        std::istringstream ss{data};
        while (ss) {
            ss >> word;
            bytes += word.length();
            benchmark::DoNotOptimize(word);
            benchmark::DoNotOptimize(ss);
            benchmark::DoNotOptimize(data);
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(bytes));
}

BENCHMARK(readstring_spio)->Range(8, 8 << 12);
BENCHMARK(readstring_ios)->Range(8, 8 << 12);
