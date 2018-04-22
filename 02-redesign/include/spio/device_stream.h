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

#ifndef SPIO_DEVICE_STREAM_H
#define SPIO_DEVICE_STREAM_H

#include "fwd.h"

#include "file_device.h"
#include "stream.h"
#if SPIO_HAS_NATIVE_FILEIO
#include "native_file_device.h"
#endif

namespace spio {
namespace detail {
    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits>
    class basic_container_stream : public basic_stream<Device,
                                                       Formatter,
                                                       Scanner,
                                                       SinkBuffer,
                                                       SourceBuffer,
                                                       Traits> {
        using base = basic_stream<Device,
                                  Formatter,
                                  Scanner,
                                  SinkBuffer,
                                  SourceBuffer,
                                  Traits>;

    public:
        using device_type = typename base::device_type;
        using char_type = typename base::char_type;
        using container_type = typename device_type::container_type;
        using category = typename base::category;
        using formatter_type = typename base::formatter_type;
        using scanner_type = typename base::scanner_type;
        using sink_buffer_type = typename base::sink_buffer_type;
        using source_buffer_type = typename base::source_buffer_type;
        using traits = typename base::traits;
        using int_type = typename base::int_type;
        using tied_type = typename base::tied_type;

        using base::base;

        span<char_type> get_buffer()
        {
            return make_span(*base::get_device().container());
        }
        span<const char_type> get_buffer() const
        {
            return make_span(*base::get_device().container());
        }
    };
}  // namespace detail

// container
template <typename Container>
using basic_container_instream =
    detail::basic_container_stream<basic_container_source<Container>>;
template <typename Container>
using basic_container_outstream =
    detail::basic_container_stream<basic_container_sink<Container>>;
template <typename Container>
using basic_container_iostream =
    detail::basic_container_stream<basic_container_device<Container>>;

// vector
template <typename CharT, typename Allocator = std::allocator<CharT>>
using basic_vector_instream =
    basic_container_instream<std::vector<CharT, Allocator>>;
template <typename CharT, typename Allocator = std::allocator<CharT>>
using basic_vector_outstream =
    basic_container_outstream<std::vector<CharT, Allocator>>;
template <typename CharT, typename Allocator = std::allocator<CharT>>
using basic_vector_iostream =
    basic_container_iostream<std::vector<CharT, Allocator>>;

using vector_instream = basic_vector_instream<char>;
using vector_outstream = basic_vector_outstream<char>;
using vector_iostream = basic_vector_iostream<char>;

using wvector_instream = basic_vector_instream<wchar_t>;
using wvector_outstream = basic_vector_outstream<wchar_t>;
using wvector_iostream = basic_vector_iostream<wchar_t>;

// string
template <typename CharT,
          typename Traits = std::char_traits<CharT>,
          typename Allocator = std::allocator<CharT>>
using basic_string_instream =
    basic_container_instream<std::basic_string<CharT, Traits, Allocator>>;
template <typename CharT,
          typename Traits = std::char_traits<CharT>,
          typename Allocator = std::allocator<CharT>>
using basic_string_outstream =
    basic_container_outstream<std::basic_string<CharT, Traits, Allocator>>;
template <typename CharT,
          typename Traits = std::char_traits<CharT>,
          typename Allocator = std::allocator<CharT>>
using basic_string_iostream =
    basic_container_iostream<std::basic_string<CharT, Traits, Allocator>>;

using string_instream = basic_string_instream<char>;
using string_outstream = basic_string_outstream<char>;
using string_iostream = basic_string_iostream<char>;

using wstring_instream = basic_string_instream<wchar_t>;
using wstring_outstream = basic_string_outstream<wchar_t>;
using wstring_iostream = basic_string_iostream<wchar_t>;

// stdio file
template <typename CharT, typename Category>
using basic_stdio_filehandle_stream =
    basic_stream<basic_filehandle_device<CharT, Category>>;
template <typename CharT>
using basic_stdio_file_instream = basic_stream<basic_file_source<CharT>>;
template <typename CharT>
using basic_stdio_file_outstream = basic_stream<basic_file_sink<CharT>>;
template <typename CharT>
using basic_stdio_file_iostream = basic_stream<basic_file_device<CharT>>;

// native file
#if SPIO_HAS_NATIVE_FILEIO
template <typename CharT, typename Category>
using basic_native_filehandle_stream =
    basic_stream<basic_native_filehandle_device<CharT, Category>>;
template <typename CharT>
using basic_native_file_instream =
    basic_stream<basic_native_file_source<CharT>>;
template <typename CharT>
using basic_native_file_outstream = basic_stream<basic_native_file_sink<CharT>>;
template <typename CharT>
using basic_native_file_iostream =
    basic_stream<basic_native_file_device<CharT>>;
#endif

// file
template <typename CharT, typename Category>
using basic_filehandle_stream =
    basic_stream<basic_default_filehandle_device<CharT, Category>>;
template <typename CharT>
using basic_file_instream = basic_stream<basic_default_file_source<CharT>>;
template <typename CharT>
using basic_file_outstream = basic_stream<basic_default_file_sink<CharT>>;
template <typename CharT>
using basic_file_iostream = basic_stream<basic_default_file_device<CharT>>;

// memory
template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_memory_instream = basic_stream<basic_memory_source<CharT, Traits>>;
template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_memory_outstream = basic_stream<basic_memory_sink<CharT, Traits>>;
template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_memory_iostream = basic_stream<basic_memory_device<CharT, Traits>>;

using memory_instream = basic_memory_instream<char>;
using memory_outstream = basic_memory_outstream<char>;
using memory_iostream = basic_memory_iostream<char>;

using wmemory_instream = basic_memory_instream<wchar_t>;
using wmemory_outstream = basic_memory_outstream<wchar_t>;
using wmemory_iostream = basic_memory_iostream<wchar_t>;

namespace detail {
    struct stdin_category : source_tag {
    };
    struct stdout_category : sink_tag {
    };

    template <typename CharT>
    class std_stream_init {
    public:
        using char_type = CharT;
        using stdin_type = basic_filehandle_stream<char_type, stdin_category>;
        using stdout_type = basic_filehandle_stream<char_type, stdout_category>;
        using stdin_storage =
            std::aligned_storage_t<sizeof(stdin_type), alignof(stdin_type)>;
        using stdout_storage =
            std::aligned_storage_t<sizeof(stdin_type), alignof(stdin_type)>;

        std_stream_init()
        {
            if (count++ == 0) {
                init();
            }
        }
        ~std_stream_init()
        {
            if (--count == 0) {
                cleanup();
            }
        }

        std_stream_init(const std_stream_init&) = delete;
        std_stream_init& operator=(const std_stream_init&) = delete;
        std_stream_init(std_stream_init&&) = delete;
        std_stream_init& operator=(std_stream_init&&) = delete;

        static stdin_type* in;
        static stdout_type* out;
        static stdout_type* err;
        static stdout_type* log;

    private:
        using tied_type = typename stdout_type::tied_type;

        void init()
        {
            in = new (&stdin_buf)
                stdin_type(stdin_type::device_type::get_stdin_handle());
            out = new (&stdout_buf)
                stdout_type(stdout_type::device_type::get_stdout_handle());
            log = new (&stdlog_buf)
                stdout_type(stdout_type::device_type::get_stderr_handle());

            using stream_buffer = typename stdout_type::sink_buffer_type;
            err = new (&stderr_buf) stdout_type(
                {stdout_type::device_type::get_stderr_handle()},
                std::make_unique<stream_buffer>(stream_buffer::mode::none),
                nullptr);

            stdout_ref.reset(*out);
            in->tie(&stdout_ref);
            err->tie(&stdout_ref);
        }
        void cleanup()
        {
            err->~stdout_type();
            log->~stdout_type();
            out->~stdout_type();
            in->~stdin_type();
        }

        static int count;
        static stdin_storage stdin_buf;
        static stdout_storage stdout_buf;
        static stdout_storage stderr_buf;
        static stdout_storage stdlog_buf;
        static tied_type stdout_ref;
    };

    namespace {
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif
        std_stream_init<char> init_char;
        std_stream_init<wchar_t> init_wchar;
        std_stream_init<char16_t> init_char16;
        std_stream_init<char32_t> init_char32;
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
    }  // namespace

    template <typename CharT>
    typename std_stream_init<CharT>::stdin_type* std_stream_init<CharT>::in =
        nullptr;
    template <typename CharT>
    typename std_stream_init<CharT>::stdout_type* std_stream_init<CharT>::out =
        nullptr;
    template <typename CharT>
    typename std_stream_init<CharT>::stdout_type* std_stream_init<CharT>::err =
        nullptr;
    template <typename CharT>
    typename std_stream_init<CharT>::stdout_type* std_stream_init<CharT>::log =
        nullptr;

    template <typename CharT>
    int std_stream_init<CharT>::count = 0;

    template <typename CharT>
    typename std_stream_init<CharT>::stdin_storage
        std_stream_init<CharT>::stdin_buf{};
    template <typename CharT>
    typename std_stream_init<CharT>::stdout_storage
        std_stream_init<CharT>::stdout_buf{};
    template <typename CharT>
    typename std_stream_init<CharT>::stdout_storage
        std_stream_init<CharT>::stderr_buf{};
    template <typename CharT>
    typename std_stream_init<CharT>::stdout_storage
        std_stream_init<CharT>::stdlog_buf{};

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif
    template <typename CharT>
    typename std_stream_init<CharT>::tied_type
        std_stream_init<CharT>::stdout_ref{};
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
}  // namespace detail

template <typename CharT>
auto& get_stdin()
{
    return *detail::std_stream_init<CharT>::in;
}
template <typename CharT>
auto& get_stdout()
{
    return *detail::std_stream_init<CharT>::out;
}
template <typename CharT>
auto& get_stderr()
{
    return *detail::std_stream_init<CharT>::err;
}
template <typename CharT>
auto& get_stdlog()
{
    return *detail::std_stream_init<CharT>::log;
}

inline auto& in()
{
    return get_stdin<char>();
}
inline auto& win()
{
    return get_stdin<wchar_t>();
}

inline auto& out()
{
    return get_stdout<char>();
}
inline auto& wout()
{
    return get_stdout<char>();
}

inline auto& err()
{
    return get_stderr<char>();
}
inline auto& werr()
{
    return get_stderr<char>();
}

inline auto& log()
{
    return get_stdlog<char>();
}
inline auto& wlog()
{
    return get_stdlog<char>();
}

}  // namespace spio

#endif
