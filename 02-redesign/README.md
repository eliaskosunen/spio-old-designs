# spio

[![Build Status (Linux)](https://travis-ci.org/eliaskosunen/spio.svg?branch=master)](https://travis-ci.org/eliaskosunen/spio)
[![Build Status (Windows)](https://ci.appveyor.com/api/projects/status/8inevtt3rbnx36ql/branch/master?svg=true)](https://ci.appveyor.com/project/varuna-lang/spio/branch/master)
[![Coverage Status](https://coveralls.io/repos/github/eliaskosunen/spio/badge.svg)](https://coveralls.io/github/eliaskosunen/spio)

## Introduction

It is frequently heard from C++ novices and experts alike, that `iostreams` are poorly designed, bloated, error-prone and inefficient.
Many C++ developers tend to avoid using `iostreams` because they accuse the library of abusing virtual functions, making internationalization difficult and/or being inadequate in performance.
This leaves the other option, C `stdio`, being used, which is better, but not by much.
C `stdio` suffers from poor type and memory safety, unfamiliar (for beginners) formatting syntax and promoting old C-with-classes-style programming.

Then there is the option of using a library to solve these problems.

`spio` is a header-only IO-library written in modern C++14/17 aimed to solve the problem of poor IO support of the standard library
by combining the performance of C `stdio` and native OS IO APIs and the type safety and extensibility of C++ `iostreams`.

## Installation

Installing the library is as simple as cloning the repository and giving the compiler the correct include directory to search header files.

```sh
$ git clone https://github.com/eliaskosunen/spio
# Use for example the example code from below
$ vim main.cpp
$ g++ main.cpp -std=c++1z -Wall -Wextra -Wpedantic -Ispio/include
$ ./a.out
```

Note that the above example uses the compiler flag `-std=c++1z`. Using the latest C++17 standard is recommended if possible, since the new features give a small performance boost.
Using the older C++14 standard (`-std=c++14`) is supported, too.

## Supported compilers

Every commit is tested in CI using the following compilers:

* g++ 7.2.0, 6.3.0 and 5.4.1 (Ubuntu 14.04 Trusty)
* clang 5.0.1, 4.0.1 and 3.9.1 (Ubuntu 14.04 Trusty)
* Visual Studio 2017, running MSVC 19.12 (Windows Server 2016)

Each commit in `master` branch is guaranteed to build on these compilers.
As long as your compiler and standard library have the same level of support for C++14/17 features that a compiler listed above, it should work.
If you think your compiler version should be supported, please file an issue.

## Example

```cpp
#include "spio/spio.h"

int main()
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
    io::buffer_instream in{{say}};
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
```

## Benchmarks

The benchmark source code can be found from the `benchmark/` directory.

The benchmarks were run on an Intel i5-6600K quad-core processor clocked at 3.9 GHz, with 16 GB of RAM and Arch Linux running kernel verson 4.12.8.
The benchmarks were compiled with clang version 5.0.1 and with `-O3 -std=c++17`.

All times are nanoseconds of CPU time.

### Writing to buffer

`bench_writestring.cpp`: Write randomly generated alphanumeric words separated by whitespace totaling in length of _n_.

n                                        | 8    | 64    | 512   | 2048
 :-------------------------------------- | :--: | :---: | :---: | :-:
`spio::basic_vector_sink<char>::write`   | 412  | 704   | 1986  | 6413
`spio::basic_stream<vector_sink>::write` | 636  | 1001  | 2536  | 7769
`spio::basic_stream<vector_sink>::print` | 739  | 1758  | 8607  | 32765
`std::stringbuf::sputn`                  | 726  | 912   | 1695  | 4694
`std::stringstream::operator<<`          | 762  | 1049  | 2307  | 7487
`std::string::append`                    | 382  | 547   | 1399  | 3903
`std::vector::insert`                    | 407  | 636   | 1629  | 4921

## License

`spio` is distributed under the MIT license.

Copyright (c) 2017-2018 Elias Kosunen

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
