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

#include "fwd.h"
#include "span.h"
#include "traits.h"

namespace spio {
    namespace detail {
        template <typename T>
        struct get_direct_buffer {

        };
    }

template <typename Mode, typename Device>
class basic_indirect_device {
public:
    using device_type = Device;
    using char_type = typename Device::char_type;
    using buffer_type = span<char_type>;
    using iterator = typename buffer_type::iterator;
    using const_iterator = typename buffer_type::const_iterator;

    struct category : Mode {
    };

    constexpr basic_indirect_device() = default;
    template <typename = std::enable_if_t<has_category<Device, input>::value>>
    constexpr basic_indirect_device(Device& dev) : m_dev(std::addressof(dev)), {}

private:
    Device* m_dev{};
    iterator m_input{};
    iterator m_output{};
};
}  // namespace spio

#endif
