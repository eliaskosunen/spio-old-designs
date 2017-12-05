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

// readme.cpp
// Example from README.md

#include "examples.h"
#include "spio/spio.h"

void readme()
{
    // Write to standard streams
    // sout stands for Standard OUT
    io::sout().write("Hello world!\n");
    io::serr().write("Hello stderr!\n");

    // Read from standard streams
    // sin = Standard IN
    io::sout().write("Give me an integer: ");
    int num{};
    io::sin().read(num);

    // Formatted output
    io::sout().println("Your integer was {}", num);

    io::sout().write("Do you happen to have something to say?\n");
    std::string say{};
    io::sin().getline(say);

    // Buffer IO
    io::buffer_instream in{io::make_span(say)};
    std::vector<std::string> words;
    while (in) {
        std::string tmp;
        in.read(tmp);
        words.push_back(std::move(tmp));
    }

    // Write words to buffer_outstream in reverse order
    io::buffer_outstream out{};
    for (auto it = words.rbegin(); it != words.rend(); ++it) {
        out.write(*it);
        if (it + 1 != words.rend()) {
            out.put(' ');
        }
    }
    io::sout().write(out.get_buffer()).nl();
}
