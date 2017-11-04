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

#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include "benchmark/benchmark.h"
#include "spio/spio.h"

template <typename T>
static void readint_spio_stdio(benchmark::State& state)
{
    try {
        while (state.KeepRunning()) {
            state.PauseTiming();
            io::owned_stdio_filehandle h("integers.txt", io::open_mode::READ);
            io::basic_file_instream<char, io::stdio_filehandle> p{h.get()};
            state.ResumeTiming();

            T num;
            while (p.read(num)) {
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
static void readint_spio_native(benchmark::State& state)
{
    try {
        while (state.KeepRunning()) {
            state.PauseTiming();
            io::owned_native_filehandle h("integers.txt", io::open_mode::READ);
            io::basic_file_instream<char, io::native_filehandle> p{h.get()};
            state.ResumeTiming();

            T num;
            while (p.read(num)) {
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
static void readint_ios(benchmark::State& state)
{
    while (state.KeepRunning()) {
        state.PauseTiming();
        std::ifstream fs("integers.txt");
        state.ResumeTiming();

        T num;
        while (fs) {
            fs >> num;
        }
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<size_t>(state.range(0)) * sizeof(T));
}
template <typename T>
static void readint_scanf(benchmark::State& state)
{
    while (state.KeepRunning()) {
        state.PauseTiming();
        FILE* file = std::fopen("integers.txt", "r");
        state.ResumeTiming();

        T num;
        while (std::fscanf(file, "%d", &num) != 1 && !std::feof(file)) {
        }
        std::fclose(file);
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<size_t>(state.range(0)) * sizeof(T));
}

BENCHMARK_TEMPLATE(readint_spio_stdio, int)->Range(8, 8 << 6);
BENCHMARK_TEMPLATE(readint_spio_native, int)->Range(8, 8 << 6);
BENCHMARK_TEMPLATE(readint_ios, int)->Range(8, 8 << 6);
BENCHMARK_TEMPLATE(readint_scanf, int)->Range(8, 8 << 6);
