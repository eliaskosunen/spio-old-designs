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
    struct stdin_category : source_tag {
    };
    struct stdout_category : sink_tag {
    };

    template <typename CharT>
    class std_stream_init {
    public:
        using char_type = CharT;
        using stdin_type = basic_stream<
            basic_default_filehandle_device<char_type, stdin_category>>;
        using stdout_type = basic_stream<
            basic_default_filehandle_device<char_type, stdout_category>>;
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
