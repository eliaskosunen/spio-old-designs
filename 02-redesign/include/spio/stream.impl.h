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

#ifndef SPIO_STREAM_IMPL_H
#define SPIO_STREAM_IMPL_H

#include "fwd.h"
#include "stream.h"
#include "stream_ref.h"
#include "util.h"

namespace spio {
namespace detail {
    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits>
    auto basic_stream_base<Device,
                           Formatter,
                           Scanner,
                           SinkBuffer,
                           SourceBuffer,
                           Traits>::tie() const -> tied_type*
    {
        return m_tied;
    }
    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits>
    auto basic_stream_base<Device,
                           Formatter,
                           Scanner,
                           SinkBuffer,
                           SourceBuffer,
                           Traits>::tie(tied_type* s) -> tied_type*
    {
        auto prev = m_tied;
        m_tied = s;
        return prev;
    }

    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits>
    void basic_stream_base<Device,
                           Formatter,
                           Scanner,
                           SinkBuffer,
                           SourceBuffer,
                           Traits>::_handle_tied()
    {
        if (m_tied) {
            m_tied->flush();
        }
    }
}  // namespace detail

template <typename Device,
          typename Formatter,
          typename Scanner,
          typename SinkBuffer,
          typename SourceBuffer,
          typename Traits,
          typename Enable>
template <typename C, typename... Args>
auto basic_stream<Device,
                  Formatter,
                  Scanner,
                  SinkBuffer,
                  SourceBuffer,
                  Traits,
                  Enable>::print(const char_type* f, const Args&... a)
    -> std::enable_if_t<is_category<C, output>::value, basic_stream&>
{
    using context = typename fmt::buffer_context<char_type>::type;
    auto str = base::get_formatter()(
        f,
        fmt::basic_format_args<context>(fmt::make_format_args<context>(a...)));
    write(make_span(str));
    return *this;
}

template <typename Device,
          typename Formatter,
          typename Scanner,
          typename SinkBuffer,
          typename SourceBuffer,
          typename Traits,
          typename Enable>
template <typename C, typename... Args>
auto basic_stream<Device,
                  Formatter,
                  Scanner,
                  SinkBuffer,
                  SourceBuffer,
                  Traits,
                  Enable>::scan(const char_type* f, Args&... a)
    -> std::enable_if_t<is_category<C, input>::value, basic_stream&>
{
    auto ref = basic_stream_ref<char_type, input>(*this);
    base::get_scanner()(ref, f, can_overread(*this), a...);
    return *this;
}
}  // namespace spio

#endif
