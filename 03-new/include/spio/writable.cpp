// Copyright 2017 Elias Kosunen
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

#include "writable.h"

namespace io {
SPIO_INLINE vector<char> file_buffering::_initialize_buffer(bool use,
                                                             std::size_t len)
{
    if (use) {
        return std::vector<char>(len);
    }
    return {};
}

SPIO_INLINE file_buffering::file_buffering(bool use_buffer,
                                            mode_type m,
                                            std::size_t len)
    : m_buffer(_initialize_buffer(use_buffer, len)),
      m_length(len),
      m_mode(m),
      m_use(true)
{
}

SPIO_INLINE error file_buffering::set(file_wrapper& file)
{
    if (!file) {
        return invalid_argument;
    }
    auto buf = [&]() -> char* {
        if (use()) {
            return &get_buffer()[0];
        }
        return nullptr;
    }();
    if (std::setvbuf(file.value(), buf, static_cast<int>(m_mode), m_length) !=
        0) {
        return io_error;
    }
    return {};
}

SPIO_INLINE file_buffering file_buffering::disable()
{
    return file_buffering(false, BUFFER_NONE, 0);
}
SPIO_INLINE file_buffering file_buffering::full(std::size_t len, bool external)
{
    return file_buffering(external, BUFFER_FULL, len);
}
SPIO_INLINE file_buffering file_buffering::line(std::size_t len, bool external)
{
    return file_buffering(external, BUFFER_LINE, len);
}
}  // namespace io
