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

#include <functional>
#include <vector>
#include "config.h"
#include "error.h"

namespace spio {
using streamsize = std::ptrdiff_t;
using streamoff = std::ptrdiff_t;

class fpos {
public:
    SPIO_CONSTEXPR_STRICT fpos(int n) SPIO_NOEXCEPT
        : fpos(static_cast<streamoff>(n))
    {
    }
    SPIO_CONSTEXPR_STRICT fpos(streamoff n) SPIO_NOEXCEPT : m_pos(n) {}

    SPIO_CONSTEXPR_STRICT operator streamoff() const SPIO_NOEXCEPT
    {
        return m_pos;
    }

    bool operator==(const fpos& p) const
    {
        return m_pos == p.m_pos;
    }
    bool operator!=(const fpos& p) const
    {
        return !(*this == p);
    }

    fpos& operator+=(streamoff n)
    {
        m_pos += n;
        return *this;
    }
    fpos& operator-=(streamoff n)
    {
        return (*this += -n);
    }

    streamoff operator-(const fpos& p) const
    {
        return m_pos - p.m_pos;
    }

private:
    streamoff m_pos;
};

inline fpos operator+(fpos l, fpos r)
{
    l += r;
    return l;
}
inline fpos operator-(fpos l, fpos r)
{
    l -= r;
    return l;
}

class error_handler {
public:
    using callback = std::function<bool(const failure&)>;
    using storage = std::vector<callback>;
    using iterator = typename storage::iterator;
    using const_iterator = typename storage::const_iterator;
    using size_type = typename storage::size_type;

    error_handler(callback f = _default_callback()) : m_callbacks{f} {}

    void push(callback f)
    {
        m_callbacks.push_back(std::move(f));
    }
    void pop()
    {
        m_callbacks.pop_back();
    }
    size_type size() const SPIO_NOEXCEPT
    {
        return m_callbacks.size();
    }
    void restore(callback f = _default_callback())
    {
        m_callbacks.clear();
        push(std::move(f));
    }

    iterator begin()
    {
        return m_callbacks.begin();
    }
    iterator end()
    {
        return m_callbacks.end();
    }
    const_iterator begin() const
    {
        return m_callbacks.begin();
    }
    const_iterator end() const
    {
        return m_callbacks.end();
    }

private:
    static callback _default_callback()
    {
        return [](const failure& e) {
            if (e.code()) {
                throw e;
            }
            return true;
        };
    }

    std::vector<callback> m_callbacks;
};

class stream_base {
public:
    stream_base(const stream_base&) = delete;
    stream_base& operator=(const stream_base&) = delete;
    stream_base(stream_base&&) SPIO_NOEXCEPT = default;
    stream_base& operator=(stream_base&&) SPIO_NOEXCEPT = default;

    virtual ~stream_base() SPIO_NOEXCEPT = default;

    bool bad() const SPIO_NOEXCEPT
    {
        return m_bad;
    }
    virtual explicit operator bool() const SPIO_NOEXCEPT
    {
        return bad();
    }
    bool operator!() const SPIO_NOEXCEPT
    {
        return !(operator bool());
    }

protected:
    stream_base() = default;

    void _raise(const failure& e)
    {
        for (auto& h : m_error) {
            if (!h(e)) {
                _set_bad();
            }
        }
    }

    void _set_bad() SPIO_NOEXCEPT
    {
        m_bad = true;
    }
    void _clear_bad() SPIO_NOEXCEPT
    {
        m_bad = false;
    }

private:
    error_handler m_error{};
    bool m_bad{false};
};
}  // namespace spio

#endif  // SPIO_STREAM_BASE_H
