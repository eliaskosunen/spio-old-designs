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

#ifndef SPIO_FILE_DEVICE_IMPL_H
#define SPIO_FILE_DEVICE_IMPL_H

#include "error.h"
#include "file_device.h"
#include "util.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

namespace spio {
template <typename CharT, typename Category, typename Traits>
void basic_filehandle_device<CharT, Category, Traits>::sync()
{
    SPIO_ASSERT(is_open(),
                "basic_filehandle_device::sync: Cannot sync a Device which is "
                "not open!");

    if (std::fflush(m_handle) != 0) {
        throw failure{SPIO_MAKE_ERRNO};
    }
}

template <typename CharT, typename Category, typename Traits>
streamsize basic_filehandle_device<CharT, Category, Traits>::read(
    span<char_type> s)
{
    SPIO_ASSERT(
        is_open(),
        "basic_filehandle_device::read: Cannot read from a Device which is "
        "not open!");
    if (std::feof(m_handle) != 0) {
        throw failure{make_error_code(end_of_file)};
    }

    auto b = std::fread(s.data(), 1, s.size_bytes_us(), m_handle);
    if (b < s.size_bytes_us()) {
        if (std::ferror(m_handle) != 0) {
            throw failure{SPIO_MAKE_ERRNO};
        }
        if (std::feof(m_handle) != 0) {
            return -1;
        }
        SPIO_UNREACHABLE;
    }
    return static_cast<streamsize>(b / sizeof(CharT));
}

template <typename CharT, typename Category, typename Traits>
streamsize basic_filehandle_device<CharT, Category, Traits>::write(
    span<const char_type> s)
{
    SPIO_ASSERT(
        is_open(),
        "basic_filehandle_device::write: Cannot write to a Device which is "
        "not open!");

    auto b = std::fwrite(s.data(), 1, s.size_bytes_us(), m_handle);
    if (b < s.size_bytes_us()) {
        if (std::ferror(m_handle) != 0) {
            throw failure{SPIO_MAKE_ERRNO};
        }
        SPIO_UNREACHABLE;
    }
    return static_cast<streamsize>(b / sizeof(CharT));
}

template <typename CharT, typename Category, typename Traits>
bool basic_filehandle_device<CharT, Category, Traits>::putback(char_type c)
{
    SPIO_ASSERT(
        is_open(),
        "basic_filehandle_device::putback: Cannot put back into a Device "
        "which is not open!");

    if (sizeof(char_type) == 1) {
        return std::ungetc(m_handle, c) != EOF;
    }
    SPIO_UNREACHABLE;
}

template <typename CharT, typename Category, typename Traits>
typename Traits::pos_type
basic_filehandle_device<CharT, Category, Traits>::seek(
    typename Traits::off_type off,
    seekdir way,
    int which)
{
    SPIO_UNUSED(which);
    SPIO_ASSERT(is_open(),
                "basic_filehandle_device::seek: Cannot seek a Device which is "
                "not open!");

    const auto origin = [&]() {
        if (way == seekdir::beg) {
            return SEEK_SET;
        }
        if (way == seekdir::cur) {
            return SEEK_CUR;
        }
        return SEEK_END;
    }();
    if (std::fseek(m_handle, off, origin) != 0) {
        throw failure{SPIO_MAKE_ERRNO};
    }

    auto p = std::ftell(m_handle);
    if (p != 0) {
        throw failure{SPIO_MAKE_ERRNO};
    }
    return p;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

template <typename CharT, typename Traits>
basic_file_device<CharT, Traits>::basic_file_device(const std::string& path,
                                                    int mode,
                                                    int base_mode)
{
    open(path, mode, base_mode);
}

template <typename CharT, typename Traits>
void basic_file_device<CharT, Traits>::open(const std::string& path,
                                            int mode,
                                            int base_mode)
{
    SPIO_ASSERT(!base::is_open(),
                "basic_file_device::open: Cannot open an already open Device!");

    auto m = mode | base_mode;
    auto b = (m & openmode::binary) != 0;
    auto i = (m & openmode::in) != 0;
    auto o = (m & openmode::out) != 0;
    auto a = (m & openmode::append) != 0;
    // auto e = (m & openmode::ate) != 0;
    // auto t = (m & openmode::truncate) != 0;

    std::array<char, 5> str{};
    auto it = str.begin();

    if (i) {
        *it++ = 'r';
    }
    else if (o) {
        if (a) {
            *it++ = 'a';
        }
        else {
            *it++ = 'w';
        }
    }

    if (b) {
        *it++ = 'b';
    }
    *it = '\0';

    auto h = std::fopen(path.c_str(), str.data());
    if (!h) {
        throw failure{SPIO_MAKE_ERRNO};
    }
    base::m_handle = h;
}

template <typename CharT, typename Traits>
void basic_file_device<CharT, Traits>::close()
{
    SPIO_ASSERT(
        base::is_open(),
        "basic_file_device::close: Cannot close a Device which is not open!");

    std::fclose(base::m_handle);
    base::m_handle = nullptr;
}
}  // namespace spio

#endif
