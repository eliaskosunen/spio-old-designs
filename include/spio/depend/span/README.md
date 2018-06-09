# span

[![Build Status](https://travis-ci.org/eliaskosunen/span.svg?branch=master)](https://travis-ci.org/eliaskosunen/span)
[![Build status](https://ci.appveyor.com/api/projects/status/426r5pqh3n5e8r9t/branch/master?svg=true)](https://ci.appveyor.com/project/varuna-lang/span/branch/master)

> The span type is an abstraction that provides a view over a contiguous sequence of objects, the storage of which is owned by some other object. The design for span – – provides bounds-safety guarantees through a combination of compile-time and (configurable) run-time constraints.
>
> – P0122

This library is a rewritten fork of the [GSL](https://github.com/Microsoft/GSL) (Guidelines Support Library) implementation of `span`.
It is compatible with the current (P0122R5) [standards proposal](http://wg21.link/P0122) for the class, with some minor differences and additions.

The library is just a single header file, `span.h`, found in the directory `span`. No linking required.

## Documentation

API documentation can be built with `doxygen`.

## Example

```cpp
#include <span.h>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

// Get the last element on a span
template <typename T, span::extent_t N>
T& last_element(span<T, N> s)
{
    if (s.empty()) {
        throw std::runtime_error(
            "Cannot take the last element of an empty span!");
    }
    return *s.rbegin();
}

int main()
{
    // Create C array with elements [0, 10)
    int carr[10] = {};
    std::iota(std::begin(carr), std::end(carr), 0);

    // Create std::array with same contents
    std::array<int, 10> arr{{}};
    std::copy(std::begin(carr), std::end(carr), arr.begin());

    // Create std::vector with the same contents
    std::vector<int> vec(arr.begin(), arr.end());

    assert(last_element(span::make_span(carr)) ==
           last_element(span::make_span(arr)) ==
           last_element(span::make_span(vec)));
    /*
    If you have C++17:
    assert(last_element(span::span{carr}) ==
           last_element(span::span{arr} ==
           last_element(span::span{vec});
    */
}
```

## Supported compilers

This library depends on some modern (>= C++11) features, which ultimately limit the available compilers.

In order to use the library in C++11 mode, you'll need:

 * GCC 4.7 or greater (template aliases and delegating constructors block the support for 4.6)
 * Clang 3.0
 * Visual Studio 2015 (expression SFINAE and no `noexcept` block the support for 2013)

If you use a newer compiler with more features and appropriate flags (`-std=c++1y/14/1z/17` or `/std:c++latest`),
the library will detect the availability of those features automatically and use them.
It is highly recommended to use C++14 if you can, as the relaxed `constexpr` rules will give better performance and more safety.

## Comparison with the GSL implementation

### Why would you want to use this implementation instead of the one found in the GSL?

1. You don't want to pull the entire GSL in order to just use `span`

   This implementation is entirely self-contained and has just a single header file, which can just be copied into your repository.

2. You need to support C++11

   GSL assumes C++14 support, so you can't use it on a project where it is not available (for example RHEL7 only has gcc 4.8).
   If you need C++98 support, move to this millenium.

3. You need some of the features not found in the GSL

   * `span::at()` for checked throwing access (similar to `std::vector::at()`)
   * more `constexpr` and `noexcept`
   * C++17 deduction guides
   * iterator constructors (this may or may not be an advantage)
   * more constructors and `make_span` overloads
   * ~~concepts TS support~~ (coming soon (tm))
   * other miscellaneous functions: non-`const` `begin()` and `end()`,
   `as_const_span()`, `size_us()`, `size_bytes_us()`, `length_us()`, `length_bytes_us()`

4. You want/need strict aliasing support

   The GSL is compiled with strict aliasing disabled

### Why would you want to use the GSL instead of this library?

1. More widely used

   If you are working on a modern C++ project, chances are, that you are already using the GSL or something similar.
   In that case, it may not be worth it to add essentially duplicate code in the form of another library dependency.
   Furthermore, with greater usage comes better availability: the GSL is even available in the [Compiler Explorer](https://godbolt.org).

2. Better support and community
 
   The GSL a *lot* of contributors and users

3. (Maybe) more portable

   The GSL is tested on FreeBSD, various OS X compilers and clang on Windows.
   This library isn't, since I don't have access to these platforms.

As far as I'm aware, the GSL has no additional features compared to this implementation aside from some minor optimizations.

## Differences with the proposal

* C++11 and C++14 compatibility
* Added checked access with throwing `.at()`
* Added `size_us` and `size_bytes_us` for easier interoperability with the STL
* More `constexpr` and `noexcept`
* More constructors
* Added `make_span` and C++17 deduction guides

## License

MIT License

Copyright (c) 2017-2018 Elias Kosunen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
