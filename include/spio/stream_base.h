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

#include "fwd.h"

#include "error.h"
#include "locale.h"
#include "span.h"
#include "traits.h"

namespace spio {
struct iostate {
    enum : char { good = 0, bad = 1, fail = 2, eof = 4 };
};

namespace detail {
    class stream_error_handler {
    public:
        using error_function_type = std::function<bool(const failure&)>;

        stream_error_handler() = default;

        void push(error_function_type f)
        {
            m_error_handlers.push_back(std::move(f));
        }
        void pop()
        {
            m_error_handlers.pop_back();
        }
        auto size()
        {
            return m_error_handlers.size();
        }
        void restore()
        {
            m_error_handlers.clear();
            push(_default_error_handler());
        }

        auto begin()
        {
            return m_error_handlers.begin();
        }
        auto end()
        {
            return m_error_handlers.end();
        }

        auto begin() const
        {
            return m_error_handlers.begin();
        }
        auto end() const
        {
            return m_error_handlers.end();
        }

    private:
        static error_function_type _default_error_handler()
        {
            return [](const failure& e) {
                if (e.code()) {
                    throw e;
                }
                return true;
            };
        }
        static std::vector<error_function_type> _init_error_handlers()
        {
            return {_default_error_handler()};
        }

        std::vector<error_function_type> m_error_handlers{
            _init_error_handlers()};
    };
}  // namespace detail

class stream_base {
public:
    stream_base(const stream_base&) = delete;
    stream_base& operator=(const stream_base&) = delete;
    stream_base(stream_base&&) noexcept = default;
    stream_base& operator=(stream_base&&) noexcept = default;

    virtual ~stream_base() noexcept = default;

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
    void clear_eof()
    {
        clear((bad() ? iostate::bad : iostate::good) |
              (fail() ? iostate::fail : iostate::good));
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

    auto& error()
    {
        return m_error;
    }
    auto& error() const
    {
        return m_error;
    }

protected:
    stream_base() = default;

    void _handle_error(const failure& e)
    {
        for (auto& h : m_error) {
            if (!h(e)) {
                break;
            }
        }
    }
    void _set_error(int state, const failure& f)
    {
        setstate(state);
        _handle_error(f);
    }

private:
    int m_state{iostate::good};
    detail::stream_error_handler m_error{};
};
}  // namespace spio

#endif
