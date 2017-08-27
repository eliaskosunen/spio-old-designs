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

static std::string generate_string(size_t len)
{
    const std::vector<char> chars = {'0', '1', '2', '3',  '4', '5',
                                     '6', '7', '8', '9',  ' ', ' ',
                                     ' ', ' ', ' ', '\n', '\t'};
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, chars.size() - 1);

    std::string str(len, '\0');
    std::generate_n(str.begin(), len, [&chars, &dist, &rng]() {
        return chars[static_cast<size_t>(dist(rng))];
    });
    return str;
}

template <typename T>
static void readint_spio(benchmark::State& state)
{
    try {
        while (state.KeepRunning()) {
            state.PauseTiming();
            std::string data = generate_string(state.range(0));
            state.ResumeTiming();

            io::readable_buffer r(io::make_span(data));
            io::input_parser<decltype(r)> p{std::move(r)};
            T num;
            while (!p.eof() && p.read(num)) {
            }
        }
        state.SetBytesProcessed(state.iterations() * state.range(0));
    }
    catch (const io::failure& f) {
        state.SkipWithError(f.what());
    }
}
template <typename T>
static void readint_ios(benchmark::State& state)
{
    while (state.KeepRunning()) {
        state.PauseTiming();
        std::string data = generate_string(state.range(0));
        state.ResumeTiming();

        std::stringstream ss(data);
        T num;
        while (ss) {
            ss >> num;
        }
    }
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
template <typename T>
static void readint_scanf(benchmark::State& state)
{
    while (state.KeepRunning()) {
        state.PauseTiming();
        std::string data = generate_string(state.range(0));
        state.ResumeTiming();
        auto p = &data[0];
        int n, total = 0;
        long long i;
        while (std::sscanf(p + total, "%*[^0123456789]%lld%n", &i, &n) != 0) {
            total += n;
        }
    }
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
template <typename T>
static void readint_strtol(benchmark::State& state)
{
    while (state.KeepRunning()) {
        state.PauseTiming();
        std::string data = generate_string(state.range(0));
        state.ResumeTiming();
        auto p = &data[0];
        long long num;
        while (*p) {
            if (std::isdigit(*p)) {
                num = std::strtoll(p, &p, 10);
            }
            else {
                p++;
            }
        }
        SPIO_UNUSED(num);
    }
    state.SetBytesProcessed(state.iterations() * state.range(0));
}

BENCHMARK_TEMPLATE(readint_spio, int)->Range(8, 8 << 6);
BENCHMARK_TEMPLATE(readint_ios, int)->Range(8, 8 << 6);
BENCHMARK_TEMPLATE(readint_scanf, int)->Range(8, 8 << 6);
BENCHMARK_TEMPLATE(readint_strtol, int)->Range(8, 8 << 6);

BENCHMARK_TEMPLATE(readint_spio, unsigned)->Range(8, 8 << 6);
BENCHMARK_TEMPLATE(readint_ios, unsigned)->Range(8, 8 << 6);
BENCHMARK_TEMPLATE(readint_scanf, unsigned)->Range(8, 8 << 6);
BENCHMARK_TEMPLATE(readint_strtol, unsigned)->Range(8, 8 << 6);

BENCHMARK_MAIN()
