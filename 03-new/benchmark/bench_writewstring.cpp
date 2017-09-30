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

static std::vector<std::wstring> generate_data(size_t len)
{
    const std::vector<wchar_t> chars = {
        L'0', L'1', L'2', L'3',  L'4',  L'5', L'6', L'7', L'8', L'9', L'A',
        L'B', L'C', L'D', L'E',  L'F',  L'G', L'H', L'I', L'J', L'K', L'L',
        L'M', L'N', L'O', L'P',  L'Q',  L'R', L'S', L'T', L'U', L'V', L'W',
        L'X', L'Y', L'Z', L'a',  L'b',  L'c', L'd', L'e', L'f', L'g', L'h',
        L'i', L'j', L'k', L'l',  L'm',  L'n', L'o', L'p', L'q', L'r', L's',
        L't', L'u', L'v', L'w',  L'x',  L'y', L'z', L' ', L' ', L' ', L' ',
        L' ', L' ', L' ', L'\n', L'\n', L'\t'};
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, static_cast<int>(chars.size() - 1));

    std::vector<std::wstring> data;
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

static void writewstring_spio(benchmark::State& state)
{
    try {
        size_t bytes = 0;
        while (state.KeepRunning()) {
            state.PauseTiming();
            auto data = generate_data(static_cast<size_t>(state.range(0)));
            state.ResumeTiming();

            io::writable_wbuffer w{};
            io::writer<decltype(w)> p{w};
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

static void writewstring_spio_static(benchmark::State& state)
{
    try {
        size_t bytes = 0;
        while (state.KeepRunning()) {
            state.PauseTiming();
            auto data = generate_data(static_cast<size_t>(state.range(0)));
            io::dynamic_writable_buffer<wchar_t> buffer;
            buffer.reserve(static_cast<size_t>(state.range(0)));
            state.ResumeTiming();

            io::writable_wbuffer w{std::move(buffer)};
            io::writer<decltype(w)> p{w};
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

static void writewstring_ios(benchmark::State& state)
{
    size_t bytes = 0;
    while (state.KeepRunning()) {
        state.PauseTiming();
        auto data = generate_data(static_cast<size_t>(state.range(0)));
        state.ResumeTiming();

        std::wstringstream ss{};
        for (auto& n : data) {
            ss << n;
            bytes += n.length();
        }
    }
    state.SetBytesProcessed(bytes);
}

BENCHMARK(writewstring_spio)->Range(8, 8 << 8);
BENCHMARK(writewstring_spio_static)->Range(8, 8 << 8);
BENCHMARK(writewstring_ios)->Range(8, 8 << 8);
