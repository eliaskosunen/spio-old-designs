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
    const std::vector<char> chars = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',  'A',  'B',
        'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',  'M',  'N',
        'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',  'Y',  'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',  'k',  'l',
        'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',  'w',  'x',
        'y', 'z', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\n', '\n', '\t'};
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, chars.size() - 1);

    std::string str(len, '\0');
    std::generate_n(str.begin(), len, [&chars, &dist, &rng]() {
        return chars[static_cast<size_t>(dist(rng))];
    });
    return str;
}

static void readstring_spio(benchmark::State& state)
{
    try {
        std::string str(64, '\0');
        auto s = io::make_span(&*str.begin(), &*(str.end() - 1));
        while (state.KeepRunning()) {
            state.PauseTiming();
            std::string data =
                generate_string(static_cast<size_t>(state.range(0)));
            state.ResumeTiming();

            io::readable_buffer r(io::make_span(data));
            io::input_parser<decltype(r)> p{std::move(r)};
            while (!p.eof() && p.read(s)) {
            }
        }
        state.SetBytesProcessed(state.iterations() *
                                static_cast<size_t>(state.range(0)));
    }
    catch (const io::failure& f) {
        state.SkipWithError(f.what());
    }
}

static void readstring_ios(benchmark::State& state)
{
    std::string s;
    while (state.KeepRunning()) {
        state.PauseTiming();
        std::string data = generate_string(static_cast<size_t>(state.range(0)));
        state.ResumeTiming();

        std::stringstream ss(data);
        while (ss) {
            ss >> s;
        }
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<size_t>(state.range(0)));
}

static void readstring_ptr(benchmark::State& state)
{
    std::array<char, 64> str;
    str.fill('\0');
    while (state.KeepRunning()) {
        state.PauseTiming();
        std::string data = generate_string(static_cast<size_t>(state.range(0)));
        state.ResumeTiming();
        size_t i = 0, datai = 0;
        while(datai < data.size()) {
            i = 0;
            while(i + 1 < str.size() && datai < data.size()) {
                datai++; 
                if(data[datai] == ' ') {
                    break;
                }
                str[i++] = data[datai];
            }
            datai += i;
            str[i] = '\0';
        }
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<size_t>(state.range(0)));
}

BENCHMARK(readstring_spio)->Range(8, 8 << 8);
BENCHMARK(readstring_ios)->Range(8, 8 << 8);
BENCHMARK(readstring_ptr)->Range(8, 8 << 8);
