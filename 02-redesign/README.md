# spio

[![Build Status](https://travis-ci.org/eliaskosunen/spio.svg?branch=master)](https://travis-ci.org/eliaskosunen/spio)
[![Build status](https://ci.appveyor.com/api/projects/status/8inevtt3rbnx36ql/branch/master?svg=true)](https://ci.appveyor.com/project/varuna-lang/spio/branch/master)

This library aims to to combine the speed of C standard IO with the functionality and safety of C++ iostreams. WIP

## Benchmarks

The benchmark source code can be found from the `benchmark/` directory.

The benchmarks were run on an Intel i5-6600K quad-core processor at 3.9 GHz, with 16 GB of RAM and Arch Linux running kernel verson 4.12.8.
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
