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

#ifndef SPIO_NATIVE_FILE_DEVICE_WIN32_IMPL_H
#define SPIO_NATIVE_FILE_DEVICE_WIN32_IMPL_H

#include "error.h"
#include "native_file_device.h"
#include "util.h"

namespace spio {
template <typename CharT>
basic_native_file_device<CharT>::basic_native_file_device(
    const std::string& path,
    uint64_t mode,
    uint64_t base_mode)
{
    open(path, mode, base_mode);
}

template <typename CharT>
void basic_native_file_device<CharT>::open(const std::string& path,
                                           uint64_t mode,
                                           uint64_t base_mode)
{
    SPIO_ASSERT(
        !is_open(),
        "basic_native_file_device::open: Cannot open an already open Device!");

    auto m = mode | base_mode;
    auto b = (m & openmode::binary) != 0;
    auto i = (m & openmode::in) != 0;
    auto o = (m & openmode::out) != 0;
    auto a = (m & openmode::append) != 0;
    // auto e = (m & openmode::ate) != 0;
    auto t = (m & openmode::truncate) != 0;

    DWORD open_type = 0;
    if(i) {
        open_type |= GENERIC_READ;
    }
    if(o) {
        open_type |= GENERIC_WRITE;
    }

    DWORD open_flags = 0;
    if(i && o) {
        if(a) {
            open_flags = OPEN_EXISTING;
        } else {
            open_flags = CREATE_ALWAYS;
        }
    }

    if (i && o) {
        f = O_RDWR;
        if (!a) {
            f |= O_CREAT;
        }
    }
    else if (i) {
        f = O_RDONLY;
    }
    else if (o) {
        f = O_WRONLY | O_TRUNC;
    }

    if (t) {
        f |= O_TRUNC;
    }
    if (a) {
        f |= O_APPEND;
    }

    auto h = [&]() {
        if (f & O_CREAT) {
            return ::open(path.c_str(), f,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);  // 0644
        }
        return ::open(path.c_str(), f);
    }();
    if (h == detail::os_file_descriptor::invalid()) {
        throw failure{SPIO_MAKE_ERRNO};
    }
    m_handle = h;
}

template <typename CharT>
void basic_native_file_device<CharT>::close()
{
    SPIO_ASSERT(is_open(),
                "basic_native_file_device::close: Cannot close a Device which "
                "is not open!");

    ::close(m_handle);
    m_handle = nullptr;
}

template <typename CharT>
void basic_native_file_device<CharT>::flush()
{
    SPIO_ASSERT(is_open(),
                "basic_native_file_device::flush: Cannot flush a Device which "
                "is not open!");

    if (::fsync(m_handle) != 0) {
        throw failure{SPIO_MAKE_ERRNO};
    }
}

template <typename CharT>
streamsize basic_native_file_device<CharT>::read(span<char_type> s)
{
    SPIO_ASSERT(
        is_open(),
        "basic_native_file_device::read: Cannot read from a Device which is "
        "not open!");
    if (m_handle.eof) {
        throw failure{make_error_condition(end_of_file)};
    }
    if (s.size() == 0) {
        return 0;
    }

    auto b = ::read(m_handle, s.data(), s.size_us());
    if (b == 0) {
        return -1;
    }
    if (b == -1) {
        throw failure{SPIO_MAKE_ERRNO};
    }
    return b;
}

template <typename CharT>
streamsize basic_native_file_device<CharT>::write(span<const char_type> s)
{
    SPIO_ASSERT(is_open(),
                "basic_file_device::write: Cannot write to a Device which is "
                "not open!");

    if (s.size() == 0) {
        return 0;
    }

    auto b = ::write(m_handle, s.data(), s.size_us());
    if (b == -1) {
        throw failure{SPIO_MAKE_ERRNO};
    }
    return b;
}

template <typename CharT>
streampos basic_native_file_device<CharT>::seek(streamoff off,
                                                seekdir way,
                                                uint64_t which)
{
    SPIO_UNUSED(which);
    SPIO_ASSERT(
        is_open(),
        "basic_file_device::seek: Cannot seek a Device which is not open!");

    const auto origin = [&]() {
        if (way == seekdir::beg) {
            return SEEK_SET;
        }
        if (way == seekdir::cur) {
            return SEEK_CUR;
        }
        return SEEK_END;
    }();
    auto ret = ::lseek(m_handle, off, origin);
    if (ret == -1) {
        throw failure{SPIO_MAKE_ERRNO};
    }
    return static_cast<streampos>(ret);
}
}  // namespace spio

#endif
