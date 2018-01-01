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

#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include "benchmark/benchmark.h"
#include "spio/spio.h"

static void readstring_spio_stdio(benchmark::State& state)
{
    try {
        std::string str(64, '\0');
        auto s = io::make_span<63>(str);
        while (state.KeepRunning()) {
            state.PauseTiming();
            io::owned_stdio_filehandle h("strings.txt", io::open_mode::READ);
            io::basic_file_instream<char, io::stdio_filehandle> p{h.get()};
            state.ResumeTiming();

            while (p.read(s)) {
            }
        }
        state.SetBytesProcessed(state.iterations() *
                                static_cast<size_t>(state.range(0)));
    }
    catch (const io::failure& f) {
        state.SkipWithError(f.what());
    }
}

static void readstring_spio_native(benchmark::State& state)
{
    try {
        std::string str(64, '\0');
        auto s = io::make_span<63>(str);
        while (state.KeepRunning()) {
            state.PauseTiming();
            io::owned_native_filehandle h("strings.txt", io::open_mode::READ);
            io::basic_file_instream<char, io::native_filehandle> p{h.get()};
            state.ResumeTiming();

            while (p.read(s)) {
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
        std::ifstream fs("strings.txt");
        state.ResumeTiming();

        while (fs) {
            fs >> s;
        }
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<size_t>(state.range(0)));
}

static void readstring_fgets(benchmark::State& state)
{
    std::string s(64, '\0');
    while (state.KeepRunning()) {
        state.PauseTiming();
        FILE* f = std::fopen("strings.txt", "r");
        state.ResumeTiming();

        while (std::fscanf(f, "%63s ", &s[0]) != 1 && !std::feof(f)) {
        }
        std::fclose(f);
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<size_t>(state.range(0)));
}

BENCHMARK(readstring_spio_stdio)->Range(8, 8 << 8);
BENCHMARK(readstring_spio_native)->Range(8, 8 << 8);
BENCHMARK(readstring_ios)->Range(8, 8 << 8);
BENCHMARK(readstring_fgets)->Range(8, 8 << 8);
