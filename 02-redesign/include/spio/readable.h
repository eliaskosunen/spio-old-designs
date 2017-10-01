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

#ifndef SPIO_READABLE_H
#define SPIO_READABLE_H

#include "config.h"
#include "error.h"
#include "util.h"

namespace io {
template <typename ImplT>
class basic_readable_base {
public:
    using implementation_type = ImplT;

#define THIS static_cast<ImplT*>(this)
#define CONST_THIS static_cast<const ImplT*>(this)

    template <typename T>
    error read(span<T> buf, characters length)
    {
        return THIS->read(std::move(buf), length);
    }
    template <typename T>
    error read(span<T> buf, elements length)
    {
        return THIS->read(std::move(buf), length);
    }
    template <typename T>
    error read(span<T> buf, bytes length)
    {
        return THIS->read(std::move(buf), length);
    }
    template <typename T>
    error read(span<T> buf, bytes_contiguous length)
    {
        return THIS->read(std::move(buf), length);
    }

    error skip()
    {
        return THIS->skip();
    }

    constexpr bool is_valid() const
    {
        return CONST_THIS->is_valid();
    }

#undef CONST_THIS
#undef THIS
};

#ifndef SPIO_FREAD
#define SPIO_FREAD ::std::fread
#endif

template <typename CharT>
class basic_readable_file
    : public basic_readable_base<basic_readable_file<CharT>> {
public:
    using value_type = CharT;

    static_assert(
        std::is_trivially_copyable<CharT>::value,
        "basic_readable_file<CharT>: CharT must be TriviallyCopyable");

    basic_readable_file() = default;
    /*implicit*/ basic_readable_file(file_wrapper file);
    /*implicit*/ basic_readable_file(const char* filename);

    template <typename T>
    error read(span<T> buf, characters length);
    template <typename T>
    error read(span<T> buf, elements length);
    template <typename T>
    error read(span<T> buf, bytes length);
    template <typename T>
    error read(span<T> buf, bytes_contiguous length);
    error read(CharT* c);

    error skip();

    constexpr bool is_valid() const
    {
        return valid;
    }

private:
    error get_error(quantity_type read_count, quantity_type expected) const;

    file_wrapper m_file{};
    bool valid{false};
};

template <typename CharT>
class basic_readable_buffer
    : public basic_readable_base<basic_readable_buffer<CharT>> {
public:
    using value_type = CharT;
    using buffer_type = span<CharT>;

    constexpr basic_readable_buffer() = default;
    basic_readable_buffer(buffer_type buf);

    template <typename T>
    error read(span<T> buf, characters length);
    template <typename T>
    error read(span<T> buf, elements length);
    template <typename T>
    error read(span<T> buf, bytes length);
    template <typename T>
    error read(span<T> buf, bytes_contiguous length);
    error read(CharT* c);

    error skip();

    constexpr bool is_valid() const
    {
        return valid && m_it != m_buffer.end();
    }

private:
    buffer_type m_buffer;
    typename buffer_type::iterator m_it;
    bool valid{false};
};

using readable_file = basic_readable_file<char>;
using readable_wfile = basic_readable_file<wchar_t>;
using readable_file16 = basic_readable_file<char16_t>;
using readable_file32 = basic_readable_file<char32_t>;
using readable_ufile = basic_readable_file<unsigned char>;

using readable_buffer = basic_readable_buffer<char>;
using readable_wbuffer = basic_readable_buffer<wchar_t>;
using readable_buffer16 = basic_readable_buffer<char16_t>;
using readable_buffer32 = basic_readable_buffer<char32_t>;
using readable_ubuffer = basic_readable_buffer<unsigned char>;
}  // namespace io

#include "readable.impl.h"
#if SPIO_HEADER_ONLY
#include "readable.cpp"
#endif  // SPIO_HEADER_ONLY

#endif  // SPIO_READABLE_H
