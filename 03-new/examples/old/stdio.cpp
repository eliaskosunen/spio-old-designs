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

// stdio.cpp
// Standard input/output stream example

#include <cmath>
#include "examples.h"
#include "spio/spio.h"

#define PI 3.14159265358979323846L

void stdio()
{
    spio::out().print(" *** Standard streams IO *** ").nl();

    spio::out().print("Hello world!\n");

    spio::out().print("What's your name and age? ");
    std::string str;
    int age;
    spio::in().scan("{} {}", str, age);

    spio::out().print("Hi, {}, {}", str, age).nl();

    spio::out().print("How well do you remember pi? ");
    long double pi;
    spio::in().scan("{}", pi);

    spio::out()
        .print("You were {}% off from {}!", std::fabs((PI - pi) / PI), PI)
        .nl();
}
