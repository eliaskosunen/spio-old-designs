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

#ifndef SPIO_LOCALE_H
#define SPIO_LOCALE_H

#include "fwd.h"

#include <functional>
#include <locale>

namespace spio {
// Holy crap this is inefficient
inline const std::locale& global_locale()
{
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif
    static auto loc = std::locale{};
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
    auto tmp = std::locale{};
    if (loc != tmp) {
        loc = tmp;
    }
    return loc;
}
}  // namespace spio

#endif
