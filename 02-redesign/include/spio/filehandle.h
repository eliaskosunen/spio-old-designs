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

#ifndef SPIO_FILEHANDLE_H
#define SPIO_FILEHANDLE_H

#include "config.h"
#include "error.h"
#include "stl.h"

#if SPIO_POSIX
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#endif

#if SPIO_WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace io {
namespace detail {
    using std_file = std::FILE;
}  // namespace detail

struct open_mode {
    enum { READ = 1, WRITE = 2 };
};

struct open_flags {
    enum { NONE = 0, APPEND = 1, EXTENDED = 2, BINARY = 4 };
};

class stdio_filehandle {
    static detail::std_file* s_open(const char* filename, const char* mode)
    {
        return std::fopen(filename, mode);
    }
    static detail::std_file* s_open(const char* filename,
                                    uint32_t mode,
                                    uint32_t flags)
    {
        bool r = (mode & open_mode::READ) != 0;
        bool w = (mode & open_mode::WRITE) != 0;
        bool a = (flags & open_flags::APPEND) != 0;
        bool e = (flags & open_flags::EXTENDED) != 0;
        bool b = (flags & open_flags::BINARY) != 0;

        stl::array<char, 5> str{};
        auto it = str.begin();

        if (r && w) {
            e = true;
        }
        if (r) {
            *it++ = 'r';
        }
        else if (w) {
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
        if (e) {
            *it++ = '+';
        }
        *it = '\0';

        return s_open(filename, &str[0]);
    }

public:
    stdio_filehandle() = default;
    stdio_filehandle(detail::std_file* ptr) : m_handle(ptr) {}
    stdio_filehandle(const char* filename, const char* mode)
        : m_handle(s_open(filename, mode))
    {
    }
    stdio_filehandle(const char* filename,
                     uint32_t mode,
                     uint32_t flags = open_flags::NONE)
        : m_handle(s_open(filename, mode, flags))
    {
    }

    bool open(const char* filename, const char* mode)
    {
        assert(!good());
        m_handle = s_open(filename, mode);
        return good();
    }
    bool open(const char* filename,
              uint32_t mode,
              uint32_t flags = open_flags::NONE)
    {
        assert(!good());
        m_handle = s_open(filename, mode, flags);
        return good();
    }

    void close()
    {
        assert(good());
        std::fclose(m_handle);
        m_handle = nullptr;
    }

    constexpr bool good() const
    {
        return m_handle != nullptr;
    }
#if defined(__GNUC__) && __GNUC__ < 7
    operator bool() const
#else
    constexpr operator bool() const
#endif
    {
        return good();
    }

    std::FILE* get() const
    {
        return m_handle;
    }

    bool error() const
    {
        assert(m_handle);
        return std::ferror(m_handle) != 0;
    }
    void check_error() const
    {
        if (error()) {
            SPIO_THROW_MSG(std::strerror(errno));
        }
    }
    bool eof() const
    {
        return std::feof(get()) != 0;
    }

    bool flush()
    {
        return std::fflush(get()) == 0;
    }

    std::size_t read(void* ptr, std::size_t bytes)
    {
        assert(good());
        return std::fread(ptr, 1, bytes, m_handle);
    }
    std::size_t write(const void* ptr, std::size_t bytes)
    {
        assert(good());
        return std::fwrite(ptr, 1, bytes, m_handle);
    }

private:
    detail::std_file* m_handle{nullptr};
};

#if SPIO_HAS_NATIVE_FILEIO
struct os_filehandle {
#if SPIO_POSIX
    static constexpr auto invalid = -1;
    using handle_type = int;
#elif SPIO_WIN32
    static constexpr auto invalid = INVALID_HANDLE_VALUE;
    using handle_type = HANDLE;
#endif

    handle_type h{invalid};

#if SPIO_POSIX
    bool eof{false};
#endif

    constexpr handle_type& get() noexcept
    {
        return h;
    }
    constexpr const handle_type& get() const noexcept
    {
        return h;
    }
};

class native_filehandle {
    static os_filehandle::handle_type s_open(const char* filename,
                                             uint32_t mode,
                                             uint32_t flags);

public:
    native_filehandle() = default;
    native_filehandle(os_filehandle h) : m_handle{h} {}
    native_filehandle(const char* file,
                      uint32_t mode,
                      uint32_t flags = open_flags::NONE)
        : m_handle{s_open(file, mode, flags)}
    {
    }

    bool open(const char* file,
              uint32_t mode,
              uint32_t flags = open_flags::NONE);
    void close();

    constexpr bool good() const noexcept
    {
        return get() != os_filehandle::invalid;
    }
#if defined(__GNUC__) && __GNUC__ < 7
    operator bool() const noexcept
#else
    constexpr operator bool() const noexcept
#endif
    {
        return good();
    }

    constexpr os_filehandle::handle_type& get() noexcept
    {
        return m_handle.get();
    }
    constexpr const os_filehandle::handle_type& get() const noexcept
    {
        return m_handle.get();
    }

    bool error() const;
    void check_error() const;

    bool eof() const;
    bool flush();

    std::size_t read(void* ptr, std::size_t bytes);
    std::size_t write(const void* ptr, std::size_t bytes);

private:
    os_filehandle m_handle{};
};

#if SPIO_POSIX
inline os_filehandle::handle_type
native_filehandle::s_open(const char* filename, uint32_t mode, uint32_t flags)
{
    bool r = (mode & open_mode::READ) != 0;
    bool w = (mode & open_mode::WRITE) != 0;
    bool a = (flags & open_flags::APPEND) != 0;
    bool e = (flags & open_flags::EXTENDED) != 0;

    int f = 0;
    if (r && w) {
        e = true;
    }
    else if (r) {
        f |= O_RDONLY;
    }
    else {
        f |= O_WRONLY;
    }

    if (e) {
        f = O_RDWR;
        if (w && !a) {
            f |= O_CREAT | O_TRUNC;
        }
    }

    if (a) {
        f |= O_APPEND;
    }

    int fd = os_filehandle::invalid;
    if (f & O_CREAT) {
        fd = ::open(filename, f,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);  // 0644
    }
    else {
        fd = ::open(filename, f);
    }
    return fd;
}

inline bool native_filehandle::open(const char* file,
                                    uint32_t mode,
                                    uint32_t flags)
{
    assert(!good());
    get() = s_open(file, mode, flags);
    return good();
}
inline void native_filehandle::close()
{
    assert(good());
    auto ret = ::close(get());
    if (ret == -1) {
        auto code = [&]() {
            assert(errno != EBADF);
            if (errno == EIO) {
                return io_error;
            }
            return unknown_error;
        }();
        SPIO_THROW(code, std::strerror(errno));
    }
}

inline bool native_filehandle::error() const
{
    return errno != 0;
}
inline void native_filehandle::check_error() const
{
    if (error()) {
        SPIO_THROW_MSG(std::strerror(errno));
    }
}

inline bool native_filehandle::eof() const
{
    assert(good());
    return m_handle.eof;
}
inline bool native_filehandle::flush()
{
    assert(good());
    return ::fsync(get()) == 0;
}

inline std::size_t native_filehandle::read(void* ptr, std::size_t bytes)
{
    assert(good());
    auto ret = ::read(get(), ptr, bytes);
    if (ret == -1) {
        ret = 0;
    }
    return static_cast<std::size_t>(ret);
}
inline std::size_t native_filehandle::write(const void* ptr, std::size_t bytes)
{
    assert(good());
    auto ret = ::write(get(), ptr, bytes);
    if (ret == -1) {
        ret = 0;
    }
    return static_cast<std::size_t>(ret);
}
#elif SPIO_WIN32

#endif

using filehandle = native_filehandle;
/* using filehandle = stdio_filehandle; */
#else
using filehandle = stdio_filehandle;
#endif  // SPIO_HAS_NATIVE_FILEIO

template <typename FileHandle>
class basic_owned_filehandle {
public:
    basic_owned_filehandle() = default;
    basic_owned_filehandle(const char* filename,
                           uint32_t mode,
                           uint32_t flags = open_flags::NONE)
        : m_file(filename, mode, flags)
    {
    }

    basic_owned_filehandle(const basic_owned_filehandle&) = delete;
    basic_owned_filehandle& operator=(const basic_owned_filehandle&) = delete;
    basic_owned_filehandle(basic_owned_filehandle&&) noexcept = default;
    basic_owned_filehandle& operator=(basic_owned_filehandle&&) noexcept =
        default;
    ~basic_owned_filehandle() noexcept
    {
        if (m_file) {
            m_file.close();
        }
    }

    bool open(const char* filename,
              uint32_t mode,
              uint32_t flags = open_flags::NONE)
    {
        return m_file.open(filename, mode, flags);
    }

    void close()
    {
        return m_file.close();
    }

#if defined(__GNUC__) && __GNUC__ < 7
    operator bool() const
#else
    constexpr operator bool() const
#endif
    {
        return m_file.operator bool();
    }

    FileHandle& get()
    {
        return m_file;
    }
    const FileHandle& get() const
    {
        return m_file;
    }

private:
    FileHandle m_file{};
};

using owned_stdio_filehandle = basic_owned_filehandle<stdio_filehandle>;
using owned_filehandle = basic_owned_filehandle<filehandle>;
#if SPIO_HAS_NATIVE_FILEIO
using owned_native_filehandle = basic_owned_filehandle<native_filehandle>;
#endif
}  // namespace io

#endif
