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

#ifndef SPIO_FWD_H
#define SPIO_FWD_H

#include <deque>
#include <string>
#include <vector>
#include "config.h"
#include "traits.h"

namespace spio {
template <typename CharT>
class basic_scanner;

template <typename Container,
          typename Traits = std::char_traits<typename Container::value_type>>
class basic_container_device;
template <typename Container,
          typename Traits = std::char_traits<typename Container::value_type>>
class basic_container_sink;
template <typename Container,
          typename Traits = std::char_traits<typename Container::value_type>>
class basic_container_source;

template <typename Buffer>
class basic_sink_buffer;
template <typename CharT, typename Allocator = std::allocator<CharT>>
using sink_buffer = basic_sink_buffer<std::vector<CharT, Allocator>>;

template <typename Buffer>
class basic_source_buffer;
template <typename CharT, typename Allocator = std::allocator<CharT>>
using source_buffer = basic_source_buffer<std::deque<CharT, Allocator>>;

namespace detail {
    struct filehandle_device_default_category : seekable_device_tag,
                                                syncable_tag,
                                                no_output_buffer_tag {
    };
}  // namespace detail

template <typename CharT,
          typename Category = detail::filehandle_device_default_category,
          typename Traits = std::char_traits<CharT>>
class basic_filehandle_device;
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_file_device;
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_file_sink;
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_file_source;

template <typename CharT>
class basic_formatter;

template <typename Category,
          typename Device,
          typename Traits = std::char_traits<typename Device::char_type>,
          typename = void>
class basic_indirect_device;
template <typename Category,
          typename Device,
          typename Traits = std::char_traits<typename Device::char_type>>
class basic_indirect_sink;
template <typename Category,
          typename Device,
          typename Traits = std::char_traits<typename Device::char_type>>
class basic_indirect_source;

template <typename CharT>
class basic_ios_base;

template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_memory_device;
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_memory_sink;
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_memory_source;

template <typename CharT,
          typename Category = detail::filehandle_device_default_category,
          typename Traits = std::char_traits<CharT>>
class basic_native_filehandle_device;
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_native_file_device;
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_native_file_sink;
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_native_file_source;

#if SPIO_HAS_NATIVE_FILEIO
template <typename CharT,
          typename Category = detail::filehandle_device_default_category>
using basic_default_filehandle_device =
    basic_native_filehandle_device<CharT, Category>;
template <typename CharT>
using basic_default_file_device = basic_native_file_device<CharT>;
template <typename CharT>
using basic_default_file_sink = basic_native_file_sink<CharT>;
template <typename CharT>
using basic_default_file_source = basic_native_file_source<CharT>;
#else
template <typename CharT,
          typename Category = detail::filehandle_device_default_category,
          typename Traits = std::char_traits<CharT>>
using basic_default_filehandle_device =
    basic_filehandle_device<CharT, Category>;
template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_default_file_device = basic_file_device<CharT>;
template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_default_file_sink = basic_file_sink<CharT>;
template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_default_file_source = basic_file_source<CharT>;
#endif

template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_null_device;
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_null_sink;
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_null_source;

template <typename Source, typename Dest>
class codeconv;

class failure;

template <typename T, typename CharT>
class instream_iterator;
template <typename T, typename CharT>
class outstream_iterator;

class stream_base;

template <typename Device,
          typename Traits = std::char_traits<typename Device::char_type>>
class basic_stream;

namespace detail {
    template <typename Device,
              typename Traits = std::char_traits<typename Device::char_type>>
    class basic_container_stream;
}

template <typename CharT,
          typename Category,
          typename Traits = std::char_traits<CharT>>
class basic_stream_ref;

template <typename... Types>
class variant;

template <typename T,
          typename Allocator = std::allocator<T>,
          std::size_t Size = 4096 / sizeof(T)>
class small_vector;
}  // namespace spio

#endif
