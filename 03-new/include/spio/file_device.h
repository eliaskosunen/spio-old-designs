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

#ifndef SPIO_FILE_DEVICE_H
#define SPIO_FILE_DEVICE_H

#include "fwd.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include "span.h"
#include "traits.h"

namespace spio {
template <typename CharT, typename Category>
class basic_filehandle_device {
public:
    using char_type = CharT;
    using category = Category;

    constexpr basic_filehandle_device() = default;
    constexpr basic_filehandle_device(std::FILE* h) : m_handle(h) {}

    void open(std::FILE* h)
    {
        SPIO_ASSERT(!is_open(),
                    "basic_filehandle_device::open: Cannot reopen an already "
                    "open file");
        m_handle = h;
    }
    bool is_open() const
    {
        return m_handle != nullptr;
    }

    constexpr std::FILE* handle()
    {
        return m_handle;
    }
    constexpr const std::FILE* handle() const
    {
        return m_handle;
    }

    void flush();

    streamsize read(span<char_type> s);
    streamsize write(span<const char_type> s);

    bool putback(char_type c);

    streampos seek(streamoff off,
                   seekdir way,
                   int which = openmode::in | openmode::out);

    bool can_overread() const
    {
        return m_handle != stdin;
    }

protected:
    std::FILE* m_handle{nullptr};
};

using filehandle_device = basic_filehandle_device<char>;
using wfilehandle_device = basic_filehandle_device<wchar_t>;

template <typename CharT>
class basic_file_device : public basic_filehandle_device<CharT> {
    using base = basic_filehandle_device<CharT>;

public:
    using char_type = CharT;

    struct category : base::category, closable_tag {
    };

    basic_file_device(const std::string& path,
                      int mode = openmode::in | openmode::out,
                      int base_mode = openmode::in | openmode::out);

    constexpr basic_file_device(const basic_file_device&) = delete;
    constexpr basic_file_device& operator=(const basic_file_device&) = delete;
    constexpr basic_file_device(basic_file_device&&) = default;
    constexpr basic_file_device& operator=(basic_file_device&&) = default;

    ~basic_file_device()
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

using file_device = basic_file_device<char>;
using wfile_device = basic_file_device<wchar_t>;

template <typename CharT>
class basic_file_source : private basic_file_device<CharT> {
    using base = basic_file_device<CharT>;

public:
    using char_type = CharT;

    struct category : seekable_source_tag, closable_tag {
    };

    using base::can_overread;
    using base::close;
    using base::is_open;
    using base::putback;
    using base::read;
    using base::seek;

    basic_file_source(const std::string& path, int mode = openmode::in)
        : base(path, mode & ~openmode::out, openmode::in)
    {
    }
    void open(const std::string& path, int mode = openmode::in)
    {
        base::open(path, mode & ~openmode::out, openmode::in);
    }
};

using file_source = basic_file_source<char>;
using wfile_source = basic_file_source<wchar_t>;

template <typename CharT>
class basic_file_sink : private basic_file_device<CharT> {
    using base = basic_file_device<CharT>;

public:
    using char_type = CharT;

    struct category : seekable_sink_tag, closable_tag, flushable_tag {
    };

    using base::close;
    using base::flush;
    using base::is_open;
    using base::seek;
    using base::write;

    basic_file_sink(const std::string& path, int mode = openmode::out)
        : base(path, mode & ~openmode::in, openmode::out)
    {
    }
    void open(const std::string& path, int mode = openmode::out)
    {
        base::open(path, mode & ~openmode::in, openmode::out);
    }
};

using file_sink = basic_file_sink<char>;
using wfile_sink = basic_file_sink<wchar_t>;
}  // namespace spio

#include "file_device.impl.h"

#endif
