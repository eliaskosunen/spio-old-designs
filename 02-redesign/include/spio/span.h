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
//

#ifndef SPIO_SPAN_H
#define SPIO_SPAN_H

#if !SPIO_USE_EXCEPTIONS
#define SPAN_NOTHROW
#endif

#define SPAN_BYTE_USE_UCHAR 1
#include "span/span/span.h"

namespace spio {
using ::span::as_bytes;
using ::span::as_writable_bytes;
using ::span::const_byte_span;
using ::span::dynamic_extent;
using ::span::extent_t;
using ::span::make_span;
using ::span::span;
using ::span::byte_type;
using ::span::writable_byte_span;
}  // namespace spio

#endif  // SPIO_SPAN_H
