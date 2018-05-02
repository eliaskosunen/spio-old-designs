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

#ifndef SPIO_NATIVE_FILE_DEVICE_H
#define SPIO_NATIVE_FILE_DEVICE_H

#include "fwd.h"

#if !SPIO_HAS_NATIVE_FILEIO
#error Native file IO not supported on this platform
#endif

#include <cstdio>
#include "span.h"
#include "traits.h"

#if SPIO_POSIX
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#if SPIO_WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace spio {
namespace detail {
    struct os_file_descriptor {
#if SPIO_POSIX
        using handle_type = int;
        static constexpr handle_type invalid()
        {
            return -1;
        }
        static constexpr handle_type stdin_handle()
        {
            return 0;
        }
        static constexpr handle_type stdout_handle()
        {
            return 1;
        }
        static constexpr handle_type stderr_handle()
        {
            return 2;
        }
#elif SPIO_WIN32
        using handle_type = void*;
        static handle_type invalid()
        {
            return INVALID_HANDLE_VALUE;
        }
        static constexpr handle_type stdin_handle()
        {
            return GetStdHandle(STD_INPUT_HANDLE);
        }
        static constexpr handle_type stdout_handle()
        {
            return GetStdHandle(STD_OUTPUT_HANDLE);
        }
        static constexpr handle_type stderr_handle()
        {
            return GetStdHandle(STD_ERROR_HANDLE);
        }
#endif

        constexpr os_file_descriptor() = default;
        constexpr os_file_descriptor(handle_type fd) : h(fd) {}

        handle_type h{invalid()};
        bool eof{false};

        constexpr handle_type& get() noexcept
        {
            return h;
        }
        constexpr const handle_type& get() const noexcept
        {
            return h;
        }
    };
}  // namespace detail

template <typename CharT, typename Category, typename Traits>
class basic_native_filehandle_device {
public:
    using char_type = CharT;
    using category = Category;
    using traits = Traits;

    constexpr basic_native_filehandle_device() = default;
    constexpr basic_native_filehandle_device(detail::os_file_descriptor h)
        : m_handle(h)
    {
    }

    void open(detail::os_file_descriptor h)
    {
        SPIO_ASSERT(
            !is_open(),
            "basic_native_filehandle_device::open: Cannot reopen an already "
            "open file");
        m_handle = h;
    }
    bool is_open() const
    {
        return m_handle.get() != detail::os_file_descriptor::invalid();
    }

    constexpr auto handle() const
    {
        return m_handle.get();
    }

    void sync();

    streamsize read(span<char_type> s);
    streamsize write(span<const char_type> s);

    typename Traits::pos_type seek(typename Traits::off_type off,
                                   seekdir way,
                                   int which = openmode::in | openmode::out);

    bool can_overread() const
    {
        return m_handle.get() == detail::os_file_descriptor::stdin_handle();
    }

    static detail::os_file_descriptor get_stdin_handle()
    {
        return detail::os_file_descriptor::stdin_handle();
    }
    static detail::os_file_descriptor get_stdout_handle()
    {
        return detail::os_file_descriptor::stdout_handle();
    }
    static detail::os_file_descriptor get_stderr_handle()
    {
        return detail::os_file_descriptor::stderr_handle();
    }

protected:
    detail::os_file_descriptor m_handle{};

private:
#if SPIO_WIN32
    bool m_append{false};
    bool m_binary{false};
#endif
};

using native_filehandle_device = basic_native_filehandle_device<char>;
using wnative_filehandle_device = basic_native_filehandle_device<wchar_t>;

template <typename CharT, typename Traits>
class basic_native_file_device
    : public basic_native_filehandle_device<CharT, Traits> {
    using base = basic_native_filehandle_device<CharT, Traits>;

public:
    using char_type = CharT;
    using traits = Traits;

    struct category : base::category, closable_tag {
    };

    basic_native_file_device(const std::string& path,
                             int mode = openmode::in | openmode::out,
                             int base_mode = openmode::in | openmode::out);

    constexpr basic_native_file_device(const basic_native_file_device&) =
        delete;
    constexpr basic_native_file_device& operator=(
        const basic_native_file_device&) = delete;
    constexpr basic_native_file_device(basic_native_file_device&&) noexcept =
        default;
    constexpr basic_native_file_device& operator=(
        basic_native_file_device&&) noexcept = default;

    ~basic_native_file_device() noexcept
    {
        if (base::is_open()) {
            close();
        }
    }

    void open(const std::string& path,
              int mode = openmode::in | openmode::out,
              int base_mode = openmode::in | openmode::out);

    void close();
};

using native_file_device = basic_native_file_device<char>;
using wnative_file_device = basic_native_file_device<wchar_t>;

template <typename CharT, typename Traits>
class basic_native_file_source
    : private basic_native_file_device<CharT, Traits> {
    using base = basic_native_file_device<CharT, Traits>;

public:
    using char_type = CharT;
    using traits = Traits;

    struct category : seekable_source_tag, closable_tag {
    };

    using base::can_overread;
    using base::close;
    using base::is_open;
    using base::putback;
    using base::read;
    using base::seek;

    basic_native_file_source(const std::string& path, int mode = openmode::in)
        : base(path, mode & ~openmode::out, openmode::in)
    {
    }
    void open(const std::string& path, int mode = openmode::in)
    {
        base::open(path, mode & ~openmode::out, openmode::in);
    }
};

using native_file_source = basic_native_file_source<char>;
using wnative_file_source = basic_native_file_source<wchar_t>;

template <typename CharT, typename Traits>
class basic_native_file_sink : private basic_native_file_device<CharT, Traits> {
    using base = basic_native_file_device<CharT, Traits>;

public:
    using char_type = CharT;
    using traits = Traits;

    struct category : seekable_sink_tag, closable_tag, syncable_tag {
    };

    using base::close;
    using base::is_open;
    using base::seek;
    using base::sync;
    using base::write;

    basic_native_file_sink(const std::string& path, int mode = openmode::out)
        : base(path, mode & ~openmode::in, openmode::out)
    {
    }
    void open(const std::string& path, int mode = openmode::out)
    {
        base::open(path, mode & ~openmode::in, openmode::out);
    }
};

using native_file_sink = basic_native_file_sink<char>;
using wnative_file_sink = basic_native_file_sink<wchar_t>;
}  // namespace spio

#include "native_file_device_posix.impl.h"

#endif
