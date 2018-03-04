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

#ifndef SPIO_STREAM_BASE_H
#define SPIO_STREAM_BASE_H

#include "config.h"
#include "error.h"
#include "locale.h"
#include "span.h"
#include "traits.h"

namespace spio {
struct iostate {
    enum : char { good = 0, bad = 1, fail = 2, eof = 4 };
};

class stream_base {
public:
    stream_base(const stream_base&) = delete;
    stream_base& operator=(const stream_base&) = delete;
    stream_base(stream_base&&) = default;
    stream_base& operator=(stream_base&&) = default;

    virtual ~stream_base() = default;

    int rdstate() const
    {
        return m_state;
    }
    void clear(int s = iostate::good)
    {
        m_state = s;
    }
    void setstate(int s)
    {
        clear(rdstate() | s);
    }

    bool good() const
    {
        return rdstate() == iostate::good;
    }
    bool bad() const
    {
        return (rdstate() & iostate::bad) != 0;
    }
    bool fail() const
    {
        return (rdstate() & iostate::fail) != 0 || bad();
    }
    bool eof() const
    {
        return (rdstate() & iostate::eof) != 0;
    }

    explicit operator bool() const
    {
        return !fail();
    }
    bool operator!() const
    {
        return !(operator bool());
    }

    const std::error_condition& error() const
    {
        return m_error;
    }

    int exceptions() const
    {
        return m_exceptions;
    }
    void exceptions(int e)
    {
        m_exceptions = e;
    }

protected:
    stream_base() = default;

private:
    std::error_condition m_error{};
    int m_state{iostate::good};
    int m_exceptions{iostate::fail | iostate::bad};
};

namespace detail {
    template <typename CharT>
    struct basic_input_stream {
        virtual streamsize read(span<CharT>) = 0;
        virtual ~basic_input_stream() = default;
    };
    template <typename CharT>
    struct basic_output_stream {
        virtual streamsize write(span<const CharT>) = 0;
        virtual ~basic_output_stream() = default;
    };
    struct seekable_stream {
        virtual streampos seek(streamoff, seekdir, int) = 0;
        virtual ~seekable_stream() = default;
    };
    struct closable_stream {
        virtual void close() = 0;
        virtual ~closable_stream() = default;
    };
    struct flushable_stream {
        virtual void flush() = 0;
        virtual ~flushable_stream() = default;
    };
    struct localizable_stream {
#if SPIO_USE_LOCALE
        virtual void imbue(const std::locale&) = 0;
        virtual const std::locale& get_locale() const = 0;
#endif
        virtual ~localizable_stream() = default;
    };
    struct isopen_stream {
        virtual bool is_open() const = 0;
        virtual ~isopen_stream() = default;
    };
}  // namespace detail

template <typename CharT>
class basic_ios_base : public stream_base {
public:
    virtual streampos seek(streamoff, seekdir, int) = 0;
    virtual void close() = 0;
    virtual void flush() = 0;
#if SPIO_USE_LOCALE
    virtual void imbue(const std::locale&) = 0;
    virtual const std::locale& get_locale() const = 0;
#endif
    virtual bool is_open() const = 0;
};
}  // namespace spio

#endif
