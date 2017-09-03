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

template <typename T = int>
static std::vector<T> generate_data(size_t len)
{
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(),
                                          std::numeric_limits<T>::max());

    std::vector<T> data(len);
    std::generate_n(data.begin(), len, [&dist, &rng]() { return dist(rng); });
    return data;
}

template <typename T>
static void writeint_spio(benchmark::State& state)
{
    try {
        while (state.KeepRunning()) {
            state.PauseTiming();
            auto data = generate_data<T>(static_cast<size_t>(state.range(0)));
            state.ResumeTiming();

            io::writable_buffer w{};
            io::writer<decltype(w)> p{w};
            for (auto& n : data) {
                p.write(n);
            }
        }
        state.SetBytesProcessed(state.iterations() *
                                static_cast<size_t>(state.range(0)) *
                                sizeof(T));
    }
    catch (const io::failure& f) {
        state.SkipWithError(f.what());
    }
}

template <typename T>
static void write_raw(benchmark::State& state)
{
    try {
        while (state.KeepRunning()) {
            state.PauseTiming();
            auto data = generate_data<T>(static_cast<size_t>(state.range(0)));
            state.ResumeTiming();

            io::writable_buffer w{};
            io::writer<decltype(w)> p{w};
            for (auto& n : data) {
                p.write_raw(n);
            }
        }
        state.SetBytesProcessed(state.iterations() *
                                static_cast<size_t>(state.range(0)) *
                                sizeof(T));
    }
    catch (const io::failure& f) {
        state.SkipWithError(f.what());
    }
}

template <typename T>
static void writeint_ios(benchmark::State& state)
{
    while (state.KeepRunning()) {
        state.PauseTiming();
        auto data = generate_data<T>(static_cast<size_t>(state.range(0)));
        state.ResumeTiming();

        std::stringstream ss{};
        for (auto& n : data) {
            ss << n;
        }
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<size_t>(state.range(0)) * sizeof(T));
}

BENCHMARK_TEMPLATE(writeint_spio, int)->Range(8, 8 << 6);
BENCHMARK_TEMPLATE(writeint_ios, int)->Range(8, 8 << 6);
BENCHMARK_TEMPLATE(write_raw, int)->Range(8, 8 << 6);
