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

// bufferio.cpp
// Buffer input/output example

#include "examples.h"
#include "spio/spio.h"

void bufferio()
{
    io::sout().println(" *** Buffer IO *** ");

    io::buffer_outstream out{};
    out.write("Word 123");

    auto& w = out.get_writable();
    auto& buf = w.get_buffer();
    auto s = io::make_span(buf.begin(), buf.end());
    io::buffer_instream in{s};
    /* io::buffer_instream in{out.get_buffer()}; */
    std::string word;
    int num;
    in.scan("{} {}", word, num);

    io::sout().println("'{}', '{}'", word, num);
}
