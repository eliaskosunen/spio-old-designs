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

#ifndef SPIO_MEMORY_DEVICE_H
#define SPIO_MEMORY_DEVICE_H

#include "error.h"
#include "config.h"
#include "indirect_device.h"
#include "span.h"
#include "traits.h"

namespace spio {
namespace detail {
    template <typename Mode, typename CharT>
    class basic_memory_device_adaptor {
    public:
        using char_type = CharT;

        struct category : Mode, direct_tag {
        };

        constexpr basic_memory_device_adaptor() = default;
        constexpr basic_memory_device_adaptor(span<char_type> s) : m_buf(s) {}

        char_type* buffer()
        {
            return m_buf.data();
        }
        const char_type* buffer() const
        {
            return m_buf.data();
        }

        span<const char_type> input()
        {
            SPIO_ASSERT(
                buffer(),
                "basic_memory_device_adaptor::input: Sequence points to "
                "invalid memory!");
            return m_buf.as_const_span();
        }
        span<char_type> output()
        {
            SPIO_ASSERT(
                buffer(),
                "basic_memory_device_adaptor::output: Sequence points to "
                "invalid memory!");
            return m_buf;
        }

    private:
        span<char_type> m_buf{nullptr};
    };
}  // namespace detail

template <typename CharT>
class basic_memory_device
    : private detail::basic_memory_device_adaptor<seekable_device_tag, CharT> {
    using base =
        detail::basic_memory_device_adaptor<seekable_device_tag, CharT>;

public:
    using char_type = CharT;
    using category = typename base::category;

    using base::base;
    using base::buffer;
    using base::input;
    using base::output;
};

using memory_device = basic_memory_device<char>;
using wmemory_device = basic_memory_device<wchar_t>;

template <typename CharT>
class basic_memory_source
    : private detail::basic_memory_device_adaptor<seekable_source_tag, CharT> {
    using base =
        detail::basic_memory_device_adaptor<seekable_source_tag, CharT>;

public:
    using char_type = CharT;
    using category = typename base::category;

    using base::base;
    using base::buffer;
    using base::input;
};

using memory_source = basic_memory_source<char>;
using wmemory_source = basic_memory_source<wchar_t>;

template <typename CharT>
class basic_memory_sink
    : private detail::basic_memory_device_adaptor<seekable_sink_tag, CharT> {
    using base = detail::basic_memory_device_adaptor<seekable_sink_tag, CharT>;

public:
    using char_type = CharT;
    using category = typename base::category;

    using base::base;
    using base::buffer;
    using base::output;
};

using memory_sink = basic_memory_sink<char>;
using wmemory_sink = basic_memory_sink<wchar_t>;

template <typename CharT>
using basic_indirect_memory_device =
    basic_indirect_device<seekable_device_tag, CharT>;
template <typename CharT>
using basic_indirect_memory_source =
    basic_indirect_source<seekable_source_tag, CharT>;
template <typename CharT>
using basic_indirect_memory_sink =
    basic_indirect_sink<seekable_sink_tag, CharT>;

using indirect_memory_device = basic_indirect_memory_device<char>;
using windirect_memory_device = basic_indirect_memory_device<wchar_t>;

using indirect_memory_source = basic_indirect_memory_source<char>;
using windirect_memory_source = basic_indirect_memory_source<wchar_t>;

using indirect_memory_sink = basic_indirect_memory_sink<char>;
using windirect_memory_sink = basic_indirect_memory_sink<wchar_t>;
}  // namespace spio

#endif
