# spio

[![Build Status (Linux)](https://travis-ci.org/eliaskosunen/spio.svg?branch=master)](https://travis-ci.org/eliaskosunen/spio)
[![Build Status (Windows)](https://ci.appveyor.com/api/projects/status/8inevtt3rbnx36ql/branch/master?svg=true)](https://ci.appveyor.com/project/varuna-lang/spio/branch/master)
[![Coverage Status](https://coveralls.io/repos/github/eliaskosunen/spio/badge.svg)](https://coveralls.io/github/eliaskosunen/spio)

This library aims to to combine the speed of C standard IO with the functionality and safety of C++ iostreams. WIP

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
The benchmarks were compiled with GCC version 7.1.1 and with `-O3`.

All times are nanoseconds.

### Integer reading

`bench_readint.cpp`: read base-10 integers from a randomly generated string buffer with length _n_.

n   | `spio::input_parser` | `std::stringstream` | `std::scanf` | `std::strtol`
:-- | :------------------: | :-----------------: | :----------: | :-----------:
8   | 228                  | 702                 | 5120         | 201
64  | 726                  | 1624                | 9107         | 572
512 | 4506                 | 5953                | 14314        | 3240

### Word (string) reading

`bench_readstring.cpp`: read whitespace-separated words from a randomly generated alphanumeric string buffer with length _n_.

n    | `spio::input_parser` | `std::stringstream` | Pointer arithmetic
:--- | :------------------: | :-----------------: | :----------------:
8    | 202                  | 574                 | 153
64   | 517                  | 1033                | 215
512  | 2970                 | 3884                | 535
2048 | 10959                | 13654               | 1708
