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

#ifndef SPIO_FILEHANDLE_H
#define SPIO_FILEHANDLE_H

#include <cstdlib>
#include "buffering.h"
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
#ifdef _MSC_VER
template <typename T>
struct is_filehandle : std::true_type {
};
#else
template <typename T, typename = void>
struct is_filehandle : std::false_type {
};

template <typename T>
struct is_filehandle<
    T,
    void_t<decltype(T::builtin_buffering),
           decltype(std::declval<T>().open(std::declval<const char*>(),
                                           std::declval<uint32_t>(),
                                           std::declval<uint32_t>())),
           decltype(std::declval<T>().close()),
           decltype(std::declval<T>().good()),
           decltype(!std::declval<T>()),
           decltype(std::declval<T>().error()),
           decltype(std::declval<T>().check_error()),
           decltype(std::declval<T>().eof()),
           decltype(std::declval<T>().flush()),
           decltype(std::declval<T>().read(std::declval<writable_byte_span>())),
           decltype(std::declval<T>().write(std::declval<const_byte_span>()))>>
    : std::true_type {
};
#endif

enum class seek_origin { SET = SEEK_SET, CUR = SEEK_CUR, END = SEEK_END };
using seek_type = long;

template <typename FileHandle, typename Alloc = stl::allocator<char>>
class basic_buffered_filehandle_base : public FileHandle {
public:
    using allocator_type = Alloc;
    using filebuffer_type = basic_filebuffer<Alloc>;

    static_assert(is_filehandle<FileHandle>::value,
                  "basic_buffered_filehandle_base<T>: T doesn't satisfy the "
                  "requirements of FileHandle");
    template <typename... Args>
    basic_buffered_filehandle_base(Args&&... args)
        : FileHandle(std::forward<Args>(args)...), m_buf{}
    {
    }
    template <typename... Args>
    basic_buffered_filehandle_base(filebuffer_type buf, Args&&... args)
        : FileHandle(std::forward<Args>(args)...), m_buf{std::move(buf)}
    {
        FileHandle::_set_buffering(m_buf);
    }
    template <typename... Args>
    basic_buffered_filehandle_base(typename filebuffer_type::buffer_mode buf,
                                   Args&&... args)
        : FileHandle(std::forward<Args>(args)...), m_buf{buf}
    {
        FileHandle::_set_buffering(m_buf);
    }

    constexpr basic_buffered_filehandle_base(
        const basic_buffered_filehandle_base&) = delete;
    constexpr basic_buffered_filehandle_base& operator=(
        const basic_buffered_filehandle_base&) = delete;
    constexpr basic_buffered_filehandle_base(basic_buffered_filehandle_base&&) =
        default;
    constexpr basic_buffered_filehandle_base& operator=(
        basic_buffered_filehandle_base&&) = default;
    ~basic_buffered_filehandle_base() = default;

    template <typename... Args>
    bool open(Args&&... args)
    {
        if (FileHandle::open(std::forward<Args>(args)...)) {
            FileHandle::_set_buffering(m_buf);
            return true;
        }
        return false;
    }

protected:
    basic_filebuffer<Alloc> m_buf{};
};

template <typename FileHandle, typename Enable = void>
class basic_buffered_filehandle;

template <typename FileHandle>
class basic_buffered_filehandle<
    FileHandle,
    std::enable_if_t<!FileHandle::builtin_buffering>>
    : public basic_buffered_filehandle_base<FileHandle> {
    using base_type = basic_buffered_filehandle_base<FileHandle>;

public:
#if defined(__GNUC__) && !defined(__clang__)
    template <typename... Args>
    basic_buffered_filehandle(Args&&... args)
        : base_type(std::forward<Args>(args)...)
    {
    }
#else
    using base_type::base_type;
#endif

    std::size_t write(const_byte_span data)
    {
        if (base_type::m_buf.mode() == filebuffer::BUFFER_NONE ||
            base_type::m_buf.mode() == filebuffer::BUFFER_DEFAULT) {
            return base_type::write(data);
        }
        return base_type::m_buf.write(
            data, [this](const_byte_span d) { return base_type::write(d); });
    }
    bool flush()
    {
        if (base_type::m_buf.mode() == filebuffer::BUFFER_LINE ||
            base_type::m_buf.mode() == filebuffer::BUFFER_FULL) {
            auto s = base_type::m_buf.get_flushable_data();
            auto w = base_type::write(s);
            if (w != s.size_us()) {
                base_type::m_buf.flag_flushed(w);
                return base_type::flush();
            }
            base_type::m_buf.flag_flushed();
        }
        return base_type::flush();
    }
};

template <typename FileHandle>
class basic_buffered_filehandle<FileHandle,
                                std::enable_if_t<FileHandle::builtin_buffering>>
    : public basic_buffered_filehandle_base<FileHandle> {
    using base_type = basic_buffered_filehandle_base<FileHandle>;

public:
#if defined(__GNUC__) && !defined(__clang__)
    template <typename... Args>
    basic_buffered_filehandle(Args&&... args)
        : base_type(std::forward<Args>(args)...)
    {
    }
#else
    using base_type::base_type;
#endif
};

namespace detail {
    using std_file = std::FILE;
}  // namespace detail

struct open_mode {
    enum { READ = 1, WRITE = 2 };
};

struct open_flags {
    enum { NONE = 0, APPEND = 1, EXTENDED = 2, BINARY = 4 };
};

class unbuf_stdio_filehandle {
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
    static constexpr bool builtin_buffering = true;

    unbuf_stdio_filehandle() = default;
    unbuf_stdio_filehandle(detail::std_file* ptr) : m_handle(ptr) {}
    unbuf_stdio_filehandle(const char* filename, const char* mode)
        : m_handle(s_open(filename, mode))
    {
    }
    unbuf_stdio_filehandle(const char* filename,
                           uint32_t mode,
                           uint32_t flags = open_flags::NONE)
        : m_handle(s_open(filename, mode, flags))
    {
    }

    bool open(const char* filename, const char* mode)
    {
        SPIO_ASSERT(!good(),
                    "unbuf_stdio_filehandle::open: Cannot reopen a file in an "
                    "already opened filehandle; `good()` is true");
        assert(!good());
        m_handle = s_open(filename, mode);
        return good();
    }
    bool open(const char* filename,
              uint32_t mode,
              uint32_t flags = open_flags::NONE)
    {
        SPIO_ASSERT(!good(),
                    "unbuf_stdio_filehandle::open: Cannot reopen a file in an "
                    "already opened filehandle; `good()` is true");
        m_handle = s_open(filename, mode, flags);
        return good();
    }

    void close()
    {
        SPIO_ASSERT(good(),
                    "unbuf_stdio_filehandle::close: Cannot close a bad "
                    "filehandle; `good()` is false");
        std::fclose(m_handle);
        m_handle = nullptr;
    }

#if (defined(__GNUC__) && __GNUC__ < 7) || defined(_MSC_VER)
    bool good() const
    {
        return m_handle != nullptr;
    }
    operator bool() const
    {
        return good();
    }
#else
    constexpr bool good() const
    {
        return m_handle != nullptr;
    }
    constexpr operator bool() const
    {
        return good();
    }
#endif

    detail::std_file* get() const
    {
        return m_handle;
    }

    bool error() const
    {
        SPIO_ASSERT(good(), "unbuf_stdio_filehandle::error: Bad filehandle");
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

    bool is_stdin() const
    {
        return m_handle == stdin;
    }

    std::size_t read(writable_byte_span data)
    {
        SPIO_ASSERT(good(), "unbuf_stdio_filehandle::read: Bad filehandle");
        return std::fread(data.data(), 1, data.size_us(), m_handle);
    }
    std::size_t write(const_byte_span data)
    {
        SPIO_ASSERT(good(), "unbuf_stdio_filehandle::write: Bad filehandle");
        return std::fwrite(data.data(), 1, data.size_us(), m_handle);
    }

    bool seek(seek_origin origin, seek_type offset)
    {
        SPIO_ASSERT(good(), "unbuf_stdio_filehandle::seek: Bad filehandle");
        return std::fseek(m_handle, offset, static_cast<int>(origin)) == 0;
    }
    bool tell(seek_type& pos)
    {
        SPIO_ASSERT(good(), "unbuf_stdio_filehandle::tell: Bad filehandle");
        auto p = std::ftell(m_handle);
        if (p == -1) {
            return false;
        }
        pos = p;
        return true;
    }

protected:
    bool _set_buffering(filebuffer& buf)
    {
        assert(good());
        if (buf.mode() == filebuffer::BUFFER_NONE) {
            std::setbuf(m_handle, nullptr);
            return true;
        }
        if (buf.mode() == filebuffer::BUFFER_DEFAULT) {
            return true;
        }
        return std::setvbuf(m_handle, buf.get_buffer(),
                            [&]() {
                                if (buf.mode() == filebuffer::BUFFER_FULL) {
                                    return _IOFBF;
                                }
                                return _IOLBF;
                            }(),
                            buf.size()) == 0;
    }

private:
    detail::std_file* m_handle{nullptr};
};

#if SPIO_HAS_NATIVE_FILEIO
struct os_filehandle {
#if SPIO_POSIX
    using handle_type = int;
    static constexpr handle_type invalid()
    {
        return -1;
    }
#elif SPIO_WIN32
    using handle_type = HANDLE;
    static handle_type invalid()
    {
        return INVALID_HANDLE_VALUE;
    }
#endif

    handle_type h{invalid()};
    bool eof{false};

    constexpr handle_type& get() noexcept
    {
        return h;
    }
    constexpr const handle_type& get() const noexcept
    {
        return h;
    }
};

class unbuf_native_filehandle {
    static os_filehandle::handle_type s_open(const char* filename,
                                             uint32_t mode,
                                             uint32_t flags);

public:
    static constexpr bool builtin_buffering = false;

    unbuf_native_filehandle() = default;
    unbuf_native_filehandle(os_filehandle h) : m_handle{h} {}
    unbuf_native_filehandle(const char* file,
                            uint32_t mode,
                            uint32_t flags = open_flags::NONE)
        : m_handle{s_open(file, mode, flags)}
    {
    }

    bool open(const char* file,
              uint32_t mode,
              uint32_t flags = open_flags::NONE);
    void close();

#if (defined(__GNUC__) && __GNUC__ < 7) || defined(_MSC_VER)
#define SPIO_CONSTEXPR /*constexpr*/
#else
#define SPIO_CONSTEXPR constexpr
#endif

    SPIO_CONSTEXPR bool good() const noexcept
    {
        return get() != os_filehandle::invalid();
    }
    SPIO_CONSTEXPR operator bool() const noexcept
    {
        return good();
    }

    SPIO_CONSTEXPR os_filehandle::handle_type& get() noexcept
    {
        return m_handle.get();
    }
    SPIO_CONSTEXPR const os_filehandle::handle_type& get() const noexcept
    {
        return m_handle.get();
    }

#undef SPIO_CONSTEXPR

    bool error() const;
    void check_error() const;

    bool eof() const;
    bool flush();

    bool is_stdin() const;

    std::size_t read(writable_byte_span data);
    std::size_t write(const_byte_span data);

    bool seek(seek_origin origin, seek_type offset);
    bool tell(seek_type& pos);

protected:
    bool _set_buffering(filebuffer& buf)
    {
        SPIO_UNUSED(buf);
        return true;
    }

private:
    os_filehandle m_handle{};
};

#if SPIO_POSIX
inline os_filehandle::handle_type unbuf_native_filehandle::s_open(
    const char* filename,
    uint32_t mode,
    uint32_t flags)
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
        f |= O_WRONLY | O_TRUNC;
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

    if (f & O_CREAT) {
        return ::open(filename, f,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);  // 0644
    }
    else {
        return ::open(filename, f);
    }
}

inline bool unbuf_native_filehandle::open(const char* file,
                                          uint32_t mode,
                                          uint32_t flags)
{
    assert(!good());
    get() = s_open(file, mode, flags);
    return good();
}
inline void unbuf_native_filehandle::close()
{
    assert(good());
    auto ret = ::close(get());
    if (ret == -1) {
        const auto code = [&]() {
            assert(errno != EBADF);
            if (errno == EIO) {
                return io_error;
            }
            return unknown_error;
        }();
        SPIO_THROW(code, std::strerror(errno));
    }
}

inline bool unbuf_native_filehandle::error() const
{
    return errno != 0;
}
inline void unbuf_native_filehandle::check_error() const
{
    if (error()) {
        SPIO_THROW_MSG(std::strerror(errno));
    }
}

inline bool unbuf_native_filehandle::eof() const
{
    assert(good());
    return m_handle.eof;
}
inline bool unbuf_native_filehandle::flush()
{
    assert(good());
    return ::fsync(get()) == 0;
}

inline bool unbuf_native_filehandle::is_stdin() const
{
    return get() == 0;
}

inline std::size_t unbuf_native_filehandle::read(writable_byte_span data)
{
    assert(good());
    if (eof()) {
        return 0;
    }
    auto ret = ::read(get(), data.data(), data.size_us());
    if (ret == 0) {
        m_handle.eof = data.size() != 0;
    }
    if (ret == -1) {
        ret = 0;
    }
    return static_cast<std::size_t>(ret);
}
inline std::size_t unbuf_native_filehandle::write(const_byte_span data)
{
    assert(good());
    auto ret = ::write(get(), data.data(), data.size_us());
    if (ret == -1) {
        ret = 0;
    }
    return static_cast<std::size_t>(ret);
}

inline bool unbuf_native_filehandle::seek(seek_origin origin, seek_type offset)
{
    assert(good());

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
    auto off = static_cast<off_t>(offset);
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

    return ::lseek(get(), off, static_cast<int>(origin)) != -1;
}
inline bool unbuf_native_filehandle::tell(seek_type& pos)
{
    assert(good());
    auto ret = ::lseek(get(), 0, SEEK_CUR);
    if (ret == -1) {
        return false;
    }

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
    pos = static_cast<seek_type>(ret);
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

    return true;
}
#elif SPIO_WIN32
inline os_filehandle::handle_type unbuf_native_filehandle::s_open(
    const char* filename,
    uint32_t mode,
    uint32_t flags)
{
    bool r = (mode & open_mode::READ) != 0;
    bool w = (mode & open_mode::WRITE) != 0;
    bool a = (flags & open_flags::APPEND) != 0;
    // bool e = (flags & open_flags::EXTENDED) != 0;
    // bool b = (flags & open_flags::BINARY) != 0;

    DWORD open_type = 0;
    if (r) {
        open_type |= GENERIC_READ;
    }
    if (w) {
        open_type |= GENERIC_WRITE;
    }
    DWORD open_flags = 0;
    if (a) {
        open_type |= OPEN_ALWAYS;
    }
    else {
        open_type |= CREATE_ALWAYS;
    }
    return ::CreateFile(filename, open_type, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        nullptr, open_flags, FILE_ATTRIBUTE_NORMAL, nullptr);
}
inline bool unbuf_native_filehandle::open(const char* file,
                                          uint32_t mode,
                                          uint32_t flags)
{
    assert(!good());
    get() = s_open(file, mode, flags);
    return good();
}
inline void unbuf_native_filehandle::close()
{
    assert(good());
    ::CloseHandle(get());
}

inline bool unbuf_native_filehandle::error() const
{
    return ::GetLastError() != ERROR_SUCCESS;
}
inline void unbuf_native_filehandle::check_error() const
{
    DWORD errcode = ::GetLastError();
    if (errcode == ERROR_SUCCESS) {
        return;
    }

    LPSTR msgBuf = nullptr;
    auto size = ::FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&msgBuf), 0, nullptr);
    failure f{default_error, msgBuf, static_cast<int>(size)};
    ::LocalFree(msgBuf);
    SPIO_THROW_FAILURE(f);
}

inline bool unbuf_native_filehandle::eof() const
{
    return m_handle.eof;
}
inline bool unbuf_native_filehandle::flush()
{
    assert(good());
    return ::FlushFileBuffers(get());
}

inline bool unbuf_native_filehandle::is_stdin() const
{
    return get() == ::GetStdHandle(STD_INPUT_HANDLE);
}

inline std::size_t unbuf_native_filehandle::read(writable_byte_span data)
{
    assert(good());
    DWORD bytes_read = 0;
    if (!::ReadFile(get(), data.data(), static_cast<DWORD>(data.size()),
                    &bytes_read, nullptr)) {
        DWORD err = ::GetLastError();
        if (err == ERROR_HANDLE_EOF) {
            m_handle.eof = true;
        }
        else {
            SetLastError(err);
        }
        if (bytes_read == 0) {
            m_handle.eof = data.size() != 0;
        }
        return 0;
    }
    else {
        if (bytes_read == 0) {
            m_handle.eof = data.size() != 0;
        }
    }
    return bytes_read;
}
inline std::size_t unbuf_native_filehandle::write(const_byte_span data)
{
    assert(good());
    DWORD bytes_written = 0;
    if (!::WriteFile(get(), data.data(), static_cast<DWORD>(data.size()),
                     &bytes_written, nullptr)) {
        return 0;
    }
    return bytes_written;
}

namespace detail {
    inline constexpr DWORD win32_move_method(seek_origin o)
    {
        switch (o) {
            case seek_origin::CUR:
                return FILE_CURRENT;
            case seek_origin::END:
                return FILE_END;
            case seek_origin::SET:
            default:
                return FILE_BEGIN;
        }
    }
}  // namespace detail
inline bool unbuf_native_filehandle::seek(seek_origin origin, seek_type offset)
{
    assert(good());
    return ::SetFilePointer(get(), static_cast<LONG>(offset), nullptr,
                            detail::win32_move_method(origin)) !=
           INVALID_SET_FILE_POINTER;
}
inline bool unbuf_native_filehandle::tell(seek_type& pos)
{
    assert(good());
    auto ret = ::SetFilePointer(get(), 0, nullptr, FILE_CURRENT);
    if (ret == INVALID_SET_FILE_POINTER) {
        return false;
    }
    pos = static_cast<seek_type>(ret);
    return true;
}
#endif
#endif  // SPIO_HAS_NATIVE_FILEIO

using stdio_filehandle = basic_buffered_filehandle<unbuf_stdio_filehandle>;

#if SPIO_HAS_NATIVE_FILEIO
using native_filehandle = basic_buffered_filehandle<unbuf_native_filehandle>;
using filehandle = native_filehandle;
/* using filehandle = stdio_filehandle; */
#else
using filehandle = stdio_filehandle;
#endif

template <typename FileHandle>
class basic_owned_filehandle {
public:
    static_assert(is_filehandle<FileHandle>::value,
                  "basic_owned_filehandle<T>: T does not satisfy the "
                  "requirements of FileHandle");

    basic_owned_filehandle() = default;
    basic_owned_filehandle(const char* filename,
                           uint32_t mode,
                           uint32_t flags = open_flags::NONE)
        : m_file(filename, mode, flags)
    {
    }

    basic_owned_filehandle(const basic_owned_filehandle&) = delete;
    basic_owned_filehandle& operator=(const basic_owned_filehandle&) = delete;
    basic_owned_filehandle(basic_owned_filehandle&&) = default;
    basic_owned_filehandle& operator=(basic_owned_filehandle&&) = default;
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
        m_file.close();
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
