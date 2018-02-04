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

#ifndef SPIO_UTIL_H
#define SPIO_UTIL_H

#include <cstdio>
#include <cstring>
#include <limits>
#include <mutex>
#include "config.h"
#include "error.h"
#include "span.h"
#include "stl.h"

namespace io {
template <typename Base>
class erased_type {
public:
    using base_type = Base;
    using Pointer = stl::unique_ptr<Base>;

    template <typename T, typename Enable = void>
    class inner;

    erased_type() = default;
    erased_type(const erased_type& o) : m_inner(o.inner_clone()) {}
    erased_type& operator=(const erased_type& o)
    {
        erased_type tmp(o);
        std::swap(tmp, *this);
        return *this;
    }
    erased_type(erased_type&&) = default;
    erased_type& operator=(erased_type&&) = default;
    ~erased_type() = default;

    template <typename T>
    erased_type(T&& src)
        : m_inner{stl::make_unique<inner<std::remove_reference_t<T>>>(
              std::forward<T>(src))}
    {
    }

    template <typename T>
    erased_type& operator=(T&& o)
    {
        m_inner = stl::make_unique<inner<std::remove_reference_t<T>>>(
            std::forward<T>(o));
        return *this;
    }

    Pointer inner_clone() const
    {
        if (m_inner) {
            return m_inner->clone();
        }
        return nullptr;
    }

    template <typename T>
    T* cast()
    {
        assert(valid());
        return dynamic_cast<inner<T>*>(m_inner.get());
    }
    template <typename T>
    const T* cast() const
    {
        return *dynamic_cast<inner<T>&>(*m_inner);
    }

    auto& get()
    {
        assert(valid());
        return *(m_inner.get());
    }
    const auto& get() const
    {
        assert(valid());
        return *(m_inner.get());
    }

    auto& operator*()
    {
        return get();
    }
    const auto& operator*() const
    {
        return get();
    }

    auto* operator-> ()
    {
        return m_inner.operator->();
    }
    const auto* operator-> () const
    {
        return m_inner.operator->();
    }

    bool valid() const
    {
        return m_inner.operator bool();
    }
    operator bool() const
    {
        return valid();
    }

protected:
    template <typename T>
    T& _static_cast()
    {
        assert(valid());
        auto ptr = static_cast<inner<T>*>(m_inner.get());
        assert(ptr);
        return *ptr;
    }

    template <typename T>
    const T& _static_cast() const
    {
        assert(valid());
        auto ptr = static_cast<inner<T>*>(m_inner.get());
        assert(ptr);
        return *ptr;
    }

private:
    Pointer m_inner{};
};

#if SPIO_USE_THREADING
template <typename T>
class basic_lockable_stream {
public:
    using lock_type = std::unique_lock<std::mutex>;

    class locked_stream {
    public:
        friend class basic_lockable_stream<T>;

        T& get()
        {
            assert(owns_lock());
            return m_stream;
        }
        const T& get() const
        {
            assert(owns_lock());
            return m_stream;
        }

        T& operator*()
        {
            assert(owns_lock());
            return get();
        }
        const T& operator*() const
        {
            assert(owns_lock());
            return get();
        }

        T* operator->()
        {
            if (!owns_lock()) {
                return nullptr;
            }
            return &get();
        }
        const T* operator->() const
        {
            if (!owns_lock()) {
                return nullptr;
            }
            return &get();
        }

        const lock_type& get_lock() const
        {
            return m_lock;
        }

        void unlock()
        {
            m_lock.unlock();
        }

        bool owns_lock() const
        {
            return m_lock.owns_lock();
        }
        operator bool() const
        {
            return owns_lock();
        }

    private:
        locked_stream(T& s, lock_type l = {})
            : m_stream(s), m_lock(std::move(l))
        {
        }

        void set_lock(lock_type lock)
        {
            m_lock = std::move(lock);
        }
        lock_type& get_lock()
        {
            return m_lock;
        }

        T& m_stream;
        lock_type m_lock{};
    };

    basic_lockable_stream(T&& stream) : m_stream{std::move(stream)} {}

    locked_stream lock()
    {
        return _do_lock([](locked_stream& s) { s.get_lock().lock(); });
    }
    locked_stream try_lock(bool& success)
    {
        return _do_lock(
            [&](locked_stream& s) { success = s.get_lock().try_lock(); });
    }

    template <typename Rep, typename Period>
    locked_stream try_lock_for(bool& success,
                               const std::chrono::duration<Rep, Period>& dur)
    {
        return _do_lock([&](locked_stream& s) {
            success = s.get_lock().try_lock_for(dur);
        });
    }
    template <typename Clock, typename Duration>
    locked_stream try_lock_until(
        bool& success,
        const std::chrono::time_point<Clock, Duration>& timeout)
    {
        return _do_lock([&](locked_stream& s) {
            success = s.get_lock().try_lock_until(timeout);
        });
    }

    const std::mutex& mutex() const
    {
        return m_mutex;
    }
    const T& stream() const
    {
        return m_stream;
    }

private:
    locked_stream _get_locked()
    {
        lock_type l{m_mutex, std::defer_lock};
        locked_stream s{m_stream, std::move(l)};
        return s;
    }

    template <typename F>
    locked_stream _do_lock(F&& fn)
    {
        auto _do = [&]() {
            auto s = _get_locked();
            fn(s);
            return s;
        };
#if SPIO_USE_EXCEPTIONS
        try {
            return _do();
        }
        catch (std::system_error& e) {
            assert(e.code() != std::errc::operation_not_permitted);
            assert(e.code() != std::errc::resource_deadlock_would_occur);
            SPIO_THROW_FAILURE(e);
        }
        catch (...) {
            SPIO_RETHROW;
        }
        SPIO_THROW(logic_error, "Unreachable in lockable_stream::_do_lock");
#else
        return _do();
#endif
    }

    T m_stream;
    std::mutex m_mutex{};
};
#endif

bool is_eof(error c);

template <typename InputIt>
constexpr std::size_t distance_nonneg(InputIt first, InputIt last);

using quantity_type = extent_t;
namespace detail {
#if !(SPIO_HAS_IF_CONSTEXPR && SPIO_HAS_TYPE_TRAITS_V)
    template <bool Signed>
    struct quantity_base_signed {
        constexpr quantity_base_signed(quantity_type n) : m_n(n) {}

        constexpr auto get_signed() const noexcept
        {
            return m_n;
        }

        constexpr auto get_unsigned() const noexcept
        {
            assert(m_n >= 0);
            return static_cast<std::make_unsigned_t<quantity_type>>(m_n);
        }

    protected:
        quantity_type m_n;
    };

    template <>
    struct quantity_base_signed<false> {
        constexpr quantity_base_signed(quantity_type n) : m_n(n) {}

        constexpr auto get_signed() const noexcept
        {
            return static_cast<std::make_signed_t<quantity_type>>(m_n);
        }
        constexpr auto get_unsigned() const noexcept
        {
            return m_n;
        }

    protected:
        quantity_type m_n;
    };
#else
    template <bool>
    struct quantity_base_signed {
        constexpr quantity_base_signed(quantity_type n) : m_n(n) {}

    protected:
        quantity_type m_n;
    };
#endif

    struct quantity_base
        : quantity_base_signed<std::is_signed<quantity_type>::value> {
        using quantity_base_signed<
            std::is_signed<quantity_type>::value>::quantity_base_signed;

        constexpr operator quantity_type() const noexcept
        {
            assert(m_n >= 0);
            return m_n;
        }

        constexpr auto get() const noexcept
        {
            return m_n;
        }

#if SPIO_HAS_IF_CONSTEXPR && SPIO_HAS_TYPE_TRAITS_V
        constexpr auto get_signed() const noexcept
        {
            if constexpr (std::is_signed_v<quantity_type>) {
                return m_n;
            }
            else {
                return static_cast<std::make_signed_t<quantity_type>>(m_n);
            }
        }
        constexpr auto get_unsigned() const noexcept
        {
            assert(m_n >= 0);
            if constexpr (std::is_unsigned_v<quantity_type>) {
                return m_n;
            }
            else {
                return static_cast<std::make_unsigned_t<quantity_type>>(m_n);
            }
        }

#endif
    };
}  // namespace detail
struct characters : detail::quantity_base {
    using quantity_base::quantity_base;
};
struct elements : detail::quantity_base {
    using quantity_base::quantity_base;
};
struct bytes : detail::quantity_base {
    using quantity_base::quantity_base;
};
struct bytes_contiguous : detail::quantity_base {
    using quantity_base::quantity_base;
};

namespace detail {
    template <typename T, std::size_t N = 0, typename Enable = void>
    struct string_tag : std::false_type {
        using type = T;
        static constexpr auto size = N;
    };

    template <typename T, std::size_t N>
    struct string_tag<
        const T (&)[N],
        N,
        std::enable_if_t<
            contains<std::decay_t<T>, char, wchar_t, char16_t, char32_t>::
                value>> : std::true_type {
        using type = const T (&)[N];
        using pointer = const T*;
        using char_type = std::decay_t<T>;
        static constexpr auto size = N;

        static constexpr pointer make_pointer(type v)
        {
            return &v[0];
        }
    };
    template <typename T>
    struct string_tag<
        const T (&)[],
        0,
        std::enable_if_t<
            contains<std::decay_t<T>, char, wchar_t, char16_t, char32_t>::
                value>> : std::true_type {
        using type = const T*;
        using pointer = const T*;
        using char_type = std::decay_t<T>;
        static constexpr auto size = 0;

        static constexpr pointer make_pointer(type v)
        {
            return &v[0];
        }
    };

    template <typename T>
    struct string_tag<
        const T*,
        0,
        std::enable_if_t<
            contains<std::decay_t<T>, char, wchar_t, char16_t, char32_t>::
                value>> : std::true_type {
        using type = const T*;
        using pointer = type;
        using char_type = std::decay_t<T>;
        static constexpr auto size = 0;

        static constexpr pointer make_pointer(type v)
        {
            return v;
        }
    };

    template <typename T, std::size_t N = 0>
    struct check_string_tag {
        static constexpr auto value = string_tag<T, N>::value;
    };

    template <typename T, std::size_t N>
    struct check_string_tag<const T (&)[N], N> : std::true_type {
    };

    template <typename T, std::size_t N>
    struct check_string_tag<const T*, N> : std::true_type {
    };
}  // namespace detail

template <typename FloatingT, typename CharT>
FloatingT str_to_floating(const CharT* str, CharT** end);

template <typename CharT>
constexpr bool is_space(CharT c, span<CharT> spaces = span<CharT>{nullptr});

template <typename CharT>
constexpr bool is_digit(CharT c, int base = 10);

template <typename IntT, typename CharT>
constexpr IntT char_to_int(CharT c, int base = 10);

template <typename CharT, typename IntT>
constexpr void int_to_char(IntT value, span<CharT> result, int base = 10);

template <typename IntT>
constexpr int max_digits() noexcept;

template <typename Dest, typename Source>
Dest bit_cast(const Source& s);
}  // namespace io

#include "util.impl.h"

#endif  // SPIO_UTIL_H
