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

#ifndef SPIO_FILTER_H
#define SPIO_FILTER_H

#include "config.h"
#include "stream_base.h"
#include <memory>

namespace spio {
class filter {
public:
    filter(const filter&) = default;
    filter& operator=(const filter&) = default;
    filter(filter&&) SPIO_NOEXCEPT = default;
    filter& operator=(filter&&) SPIO_NOEXCEPT = default;

    virtual ~filter() SPIO_NOEXCEPT = default;

    virtual std::unique_ptr<filter> create() const = 0;

    void flush()
    {
        do_flush();
    }

private:
    virtual void do_flush() {}
};

class filter_chain {
public:
    using filter_pointer = std::unique_ptr<filter>;
    using storage_type = std::vector<filter_pointer>;
    using size_type = typename storage_type::size_type;
    using iterator = typename storage_type::iterator;
    using const_iterator = typename storage_type::const_iterator;

    filter_chain() = default;
    filter_chain(storage_type ch) : m_chain(std::move(ch)) {}

    filter_chain(const filter_chain& other) = delete;
    filter_chain& operator=(const filter_chain& other) = delete;
    filter_chain(filter_chain&&) SPIO_NOEXCEPT = default;
    filter_chain& operator=(filter_chain&&) SPIO_NOEXCEPT = default;
    ~filter_chain() SPIO_NOEXCEPT = default;

    filter_chain clone() const
    {
        storage_type ch(size());
        for (size_type i = 0; i < size(); ++i) {
            ch[i] = m_chain[i]->create();
        }
        return filter_chain(std::move(ch));
    }

    void push(filter_pointer f)
    {
        m_chain.push_back(std::move(f));
    }
    void pop()
    {
        m_chain.pop_back();
    }

    filter& top()
    {
        return *(m_chain.back().get());
    }
    const filter& top() const
    {
        return *(m_chain.back().get());
    }

    size_type size() const SPIO_NOEXCEPT
    {
        return m_chain.size();
    }
    bool empty() const SPIO_NOEXCEPT
    {
        return m_chain.empty();
    }

    void clear()
    {
        m_chain.clear();
    }

    iterator begin() SPIO_NOEXCEPT
    {
        return m_chain.begin();
    }
    const_iterator begin() const SPIO_NOEXCEPT
    {
        return m_chain.begin();
    }

    iterator end() SPIO_NOEXCEPT
    {
        return m_chain.end();
    }
    const_iterator end() const SPIO_NOEXCEPT
    {
        return m_chain.end();
    }

    filter& operator[](size_type i)
    {
        return *(m_chain[i].get());
    }
    const filter& operator[](size_type i) const
    {
        return *(m_chain[i].get());
    }

private:
    storage_type m_chain;
};

class filtered_stream : public virtual stream_base {
public:
    filter_chain& chain()
    {
        return m_filters;
    }
    const filter_chain& chain() const
    {
        return m_filters;
    }

private:
    filter_chain m_filters;
};
}  // namespace spio

#endif  // SPIO_FILTER_H
