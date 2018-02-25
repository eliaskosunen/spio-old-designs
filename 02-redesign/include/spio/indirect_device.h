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

#ifndef SPIO_INDIRECT_DEVICE
#define SPIO_INDIRECT_DEVICE

#include "config.h"
#include "span.h"
#include "traits.h"

namespace spio {
template <typename Device>
class basic_indirect_device_base {
public:
    using device_type = Device;
    using char_type = typename Device::char_type;
    using buffer_type = span<char_type>;
    using iterator = typename buffer_type::iterator;
    using const_iterator = typename buffer_type::const_iterator;

    constexpr basic_indirect_device_base() = default;
    constexpr basic_indirect_device_base(Device& dev)
        : m_dev(std::addressof(dev)),
          m_input(_init_input_iterator(dev)),
          m_output(_init_output_iterator(dev))
    {
    }

protected:
    Device* m_dev{};
    iterator m_input{};
    iterator m_output{};

private:
    template <bool Dependent = true>
    static std::enable_if_t<has_category<Device, input>::value, iterator>
    _init_input_iterator(Device& d)
    {
        return d.input().begin();
    }
    template <bool Dependent = true>
    static std::enable_if_t<!has_category<Device, input>::value, iterator>
    _init_input_iterator(Device& d)
    {
        SPIO_UNUSED(d);
        return {};
    }

    template <bool Dependent = true>
    static std::enable_if_t<has_category<Device, output>::value, iterator>
    _init_output_iterator(Device& d)
    {
        return d.output().begin();
    }
    template <bool Dependent = true>
    static std::enable_if_t<!has_category<Device, output>::value, iterator>
    _init_output_iterator(Device& d)
    {
        SPIO_UNUSED(d);
        return {};
    }
};

template <typename Category, typename Device, typename = void>
class basic_indirect_device : public basic_indirect_device_base<Device> {
    using base = basic_indirect_device_base<Device>;

    using base::m_dev;
    using base::m_input;
    using base::m_output;

public:
    using char_type = typename Device::char_type;
    using category = Category;

    using base::base;

    streamsize read(span<char_type> s)
    {
        if (m_input == m_dev->input().end()) {
            return -1;
        }
        auto dist = std::distance(m_input, m_dev->input().end());
        auto n = std::max(dist, s.size());
        m_input = std::copy_n(m_input, n, s.begin());
        m_output += n;
        return n;
    }
    streamsize write(span<const char_type> s)
    {
        auto dist = std::distance(m_output, m_dev->output().end());
        auto n = std::max(dist, s.size());
        std::copy_n(s.begin(), n, m_output);
        m_output += n;
        m_input += n;
        return n;
    }

    bool putback(char_type c)
    {
        if (m_input == m_dev->output().begin()) {
            throw failure{make_error_condition(invalid_operation),
                          "basic_indirect_device::putback: Cannot putback into "
                          "Device that has not been read from!"};
        }
        if (*(m_input - 1) != c) {
            return false;
        }
        --m_input;
        --m_output;
        return true;
    }

    template <bool Dependent = true>
    std::enable_if_t<has_category<Device, detail::random_access>::value,
                     streampos>
    seek(streamoff off,
         seekdir way,
         uint64_t which = openmode::in | openmode::out)
    {
        SPIO_UNUSED(which);

        auto buf = m_dev->input();
        if (way == seekdir::beg) {
            auto size = static_cast<streamoff>(buf.size());
            if (size < off || off < 0) {
                throw failure{
                    make_error_code(std::errc::invalid_argument),
                    "basic_indirect_device::seek: offset is out of range"};
            }
            m_input = m_dev->input().begin() + off;
            m_output = m_dev->output().begin() + off;
            return off;
        }
        if (way == seekdir::cur) {
            if (off == 0) {
                return std::distance(buf.begin(), m_input);
            }
            if (off < 0) {
                auto dist = std::distance(buf.begin(), m_input);
                if (dist < -off) {
                    throw failure{
                        make_error_code(std::errc::invalid_argument),
                        "basic_indirect_device::seek: offset is out of range"};
                }
                m_input -= off;
                m_output -= off;
                return std::distance(buf.begin(), m_input);
            }
            auto dist = std::distance(m_input, buf.end());
            if (dist < off) {
                throw failure{
                    make_error_code(std::errc::invalid_argument),
                    "basic_indirect_device::seek: offset is out of range"};
            }
            m_input += off;
            m_output += off;
            return std::distance(buf.begin(), m_input);
        }

        auto size = static_cast<streamoff>(buf.size());
        if (size < -off || off > 0) {
            throw failure{
                make_error_code(std::errc::invalid_argument),
                "basic_indirect_device::seek: offset is out of range"};
        }
        m_input = buf.end() + off;
        m_output = m_dev->output().end() + off;
        return std::distance(buf.begin(), m_input);
    }
};

template <typename Category, typename Device>
class basic_indirect_device<
    Category,
    Device,
    std::enable_if_t<has_category<Device, detail::two_head>::value>>
    : public basic_indirect_device_base<Device> {
    using base = basic_indirect_device_base<Device>;

    using base::m_dev;
    using base::m_input;
    using base::m_output;

public:
    using char_type = typename Device::char_type;
    using category = Category;

    using base::base;

    streamsize read(span<char_type> s)
    {
        if (m_input == m_dev->input().end()) {
            return -1;
        }
        auto dist = std::distance(m_input, m_dev->input().end());
        auto n = std::max(dist, s.size());
        m_input = std::copy_n(m_input, n, s.begin());
        return n;
    }
    streamsize write(span<const char_type> s)
    {
        auto dist = std::distance(m_output, m_dev->output().end());
        auto n = std::max(dist, s.size());
        std::copy_n(s.begin(), n, m_output);
        m_output += n;
        return n;
    }

    bool putback(char_type c)
    {
        if (m_input == m_dev->output().begin()) {
            throw failure{make_error_condition(invalid_operation),
                          "basic_indirect_device::putback: Cannot putback into "
                          "Device that has not been read from!"};
        }
        if (*(m_input - 1) != c) {
            return false;
        }
        --m_input;
        return true;
    }
};

template <typename Category, typename Device>
class basic_indirect_source : basic_indirect_device<Category, Device> {
    using base = basic_indirect_device_base<Device>;

public:
    using char_type = typename Device::char_type;
    using category = typename base::category;

    using base::base;
    using base::putback;
    using base::read;

    template <bool Dependent = true>
    std::enable_if_t<has_category<Category, input_seekable>::value, streampos>
    seek(streamoff off, seekdir way, uint64_t which = openmode::in)
    {
        return base::seek(off, way, which);
    }
};

template <typename Category, typename Device>
class basic_indirect_sink : basic_indirect_device<Category, Device> {
    using base = basic_indirect_device_base<Device>;

public:
    using char_type = typename Device::char_type;
    using category = typename base::category;

    using base::base;
    using base::write;

    template <bool Dependent = true>
    std::enable_if_t<has_category<Category, output_seekable>::value, streampos>
    seek(streamoff off, seekdir way, uint64_t which = openmode::out)
    {
        return base::seek(off, way, which);
    }
};
}  // namespace spio

#endif
