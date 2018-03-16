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

#ifndef SPIO_TRAITS_H
#define SPIO_TRAITS_H

#include <cstddef>
#include <type_traits>
#include "config.h"

namespace spio {
using streamsize = std::ptrdiff_t;
using streampos = std::ptrdiff_t;
using streamoff = std::ptrdiff_t;

enum class seekdir { beg, cur, end };

struct openmode {
    enum { append = 1, binary = 2, in = 4, out = 8, truncate = 16, ate = 32 };
};

struct any_tag {
};

namespace detail {
    struct two_sequence : virtual any_tag {
    };
    struct random_access : virtual any_tag {
    };
    struct one_head : virtual any_tag {
    };
    struct two_head : virtual any_tag {
    };
}  // namespace detail

struct input : virtual any_tag {
};
struct output : virtual any_tag {
};
struct bidirectional : virtual any_tag {
};
struct input_seekable : virtual input, virtual detail::random_access {
};
struct output_seekable : virtual output, virtual detail::random_access {
};
struct seekable : virtual input_seekable,
                  virtual output_seekable,
                  detail::one_head {
};
struct dual_seekable : virtual input_seekable,
                       virtual output_seekable,
                       detail::two_head {
};
struct bidirectional_seekable : input_seekable,
                                output_seekable,
                                bidirectional,
                                detail::two_head {
};

struct device_tag : virtual any_tag {
};

struct asynchronized_tag : virtual any_tag {
};
struct closable_tag : virtual any_tag {
};
struct direct_tag : virtual any_tag {
};
struct flushable_tag : virtual any_tag {
};
struct localizable_tag : virtual any_tag {
};
struct revertible_tag : virtual any_tag {
};
struct nobuffer_tag : virtual any_tag {
};

struct source_tag : device_tag, input {
};
struct sink_tag : device_tag, output {
};
struct seekable_source_tag : device_tag, input_seekable {
};
struct seekable_sink_tag : device_tag, output_seekable {
};
struct bidirectional_device_tag : device_tag, bidirectional {
};
struct seekable_device_tag : device_tag, seekable {
};
struct dual_seekable_device_tag : device_tag, dual_seekable {
};
struct bidirectional_seekable_device_tag : device_tag, bidirectional_seekable {
};

template <typename T, typename Category>
struct has_category : std::is_base_of<Category, typename T::category> {
};
template <typename T, typename Category>
struct is_category : std::is_base_of<Category, T> {
};
}  // namespace spio

#endif
