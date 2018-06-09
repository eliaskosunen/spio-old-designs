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

// readme.cpp
// Example from README.md

#include "examples.h"

void readme()
{
    // Write to standard streams
    spio::out().print("Hello world!\n");
    spio::err().print("Hello stderr!\n");

    // Read from standard streams
    spio::out().print("Give me an integer: ");
    int num{};
    spio::in().scan("{}", num);

    // Formatted output
    spio::out().print("Your integer was {}", num).nl();

    spio::out().print("Do you happen to have something to say?\n");
    std::string say{};
    spio::getline(spio::in(), say);

    // Buffer IO
    spio::memory_instream in{spio::make_span(say)};
    std::vector<std::string> words;
    while (in && !in.eof()) {
        std::string tmp;
        in.scan("{}", tmp);
        words.push_back(std::move(tmp));
    }

    // Write words to buffer_outstream in reverse order
    std::vector<char> out_buf;
    spio::vector_outstream out{out_buf};
    for (auto it = words.rbegin(); it != words.rend(); ++it) {
        out.write(*it);
        if (it + 1 != words.rend()) {
            out.put(' ');
        }
    }
    spio::out().write(out_buf).nl();
}
