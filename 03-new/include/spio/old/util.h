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

#include "fwd.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace spio {
#if SPIO_HAS_LOGICAL_TRAITS
template <typename... B>
using disjunction = std::disjunction<B...>;
#else
template <typename...>
struct disjunction : std::false_type {
};
template <typename B1>
struct disjunction<B1> : B1 {
};
template <typename B1, typename... Bn>
struct disjunction<B1, Bn...>
    : std::conditional_t<bool(B1::value), B1, disjunction<Bn...>> {
};
#endif

template <typename T, typename... Ts>
struct contains : disjunction<std::is_same<T, Ts>...> {
};

#if !SPIO_HAS_VOID_T
template <typename...>
using void_t = void;
#else
template <typename... T>
using void_t = std::void_t<T...>;
#endif

template <typename Dest, typename Source>
Dest bit_cast(const Source& s)
{
    static_assert(sizeof(Dest) == sizeof(Source),
                  "bit_cast<>: sizeof Dest and Source must be equal");
    static_assert(std::is_trivially_copyable<Dest>::value,
                  "bit_cast<>: Dest must be TriviallyCopyable");
    static_assert(std::is_trivially_copyable<Source>::value,
                  "bit_cast<>: Source must be TriviallyCopyable");

    Dest d;
    std::memcpy(&d, &s, sizeof(Dest));
    return d;
}

struct nonesuch {
    nonesuch() = delete;
    ~nonesuch() = delete;
    nonesuch(const nonesuch&) = delete;
    void operator=(const nonesuch&) = delete;
};

namespace detail {
    template <class Default,
              class AlwaysVoid,
              template <class...> class Op,
              class... Args>
    struct detector {
        using value_t = std::false_type;
        using type = Default;
    };

    template <class Default, template <class...> class Op, class... Args>
    struct detector<Default, void_t<Op<Args...>>, Op, Args...> {
        using value_t = std::true_type;
        using type = Op<Args...>;
    };

}  // namespace detail

template <template <class...> class Op, class... Args>
using is_detected =
    typename detail::detector<nonesuch, void, Op, Args...>::value_t;

template <template <class...> class Op, class... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

template <class Default, template <class...> class Op, class... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;

namespace detail {
    template <typename Device>
    using overread_t = decltype(std::declval<Device&>().can_overread());

    template <typename Device, typename = void>
    struct can_overread_impl {
        static bool value(Device&)
        {
            return true;
        }
    };

    template <typename Device>
    struct can_overread_impl<
        Device,
        std::enable_if_t<is_detected<overread_t, Device>::value>> {
        static bool value(Device& d)
        {
            return d.can_overread();
        }
    };
}  // namespace detail

template <typename Device>
bool can_overread(Device& d)
{
    return detail::can_overread_impl<Device>::value(d);
}

template <typename IntT>
constexpr int max_digits() noexcept
{
    auto i = std::numeric_limits<IntT>::max();

    int digits = 0;
    while (i) {
        i /= 10;
        digits++;
    }
#if SPIO_HAS_IF_CONSTEXPR && SPIO_HAS_TYPE_TRAITS_V
    if constexpr (std::is_signed_v<IntT>) {
#else
    if (std::is_signed<IntT>::value) {
#endif
        return digits + 1;
    }
    else {
        return digits;
    }
}

template <typename CharT>
constexpr bool is_digit(CharT c, int base = 10)
{
    assert(base >= 2 && base <= 36);
    if (base <= 10) {
        return c >= '0' && c <= '0' + (base - 1);
    }
    return is_digit(c, 10) || (c >= 'a' && c <= 'a' + (base - 1)) ||
           (c >= 'A' && c <= 'A' + (base - 1));
}

template <typename IntT, typename CharT>
constexpr IntT char_to_int(CharT c, int base)
{
    assert(base >= 2 && base <= 36);
    assert(is_digit(c, base));
    if (base <= 10) {
        assert(c <= '0' + (base - 1));
        return static_cast<IntT>(c - '0');
    }
    if (c <= '9') {
        return static_cast<IntT>(c - '0');
    }
    if (c >= 'a' && c <= 'z') {
        return 10 + static_cast<IntT>(c - 'a');
    }
    auto ret = 10 + static_cast<IntT>(c - 'A');
    return ret;
}

namespace detail {
    struct placement_deleter {
        template <typename T>
        void operator()(T* ptr) const
        {
            return ptr->~T();
        }
    };

    template <typename T, typename... Args>
    std::unique_ptr<T, placement_deleter> make_in_place(void* place,
                                                        Args&&... args)
    {
        return std::unique_ptr<T, placement_deleter>{
            ::new (place) T(std::forward<Args>(args)...)};
    }

    template <typename... Ts>
    struct variant_helper;

    template <typename Union, typename T, typename... Ts>
    struct variant_helper<Union, T, Ts...> {
        static void destroy(std::size_t i, Union* data)
        {
            if (i == 0) {
                reinterpret_cast<T*>(data)->~T();
            }
            else {
                variant_helper<Union, Ts...>::destroy(--i, data);
            }
        }
        static void move(std::size_t i, Union* src, Union* dest)
        {
            if (i == 0) {
                new (dest) T(std::move(*reinterpret_cast<T*>(src)));
            }
            else {
                variant_helper<Union, Ts...>::move(--i, src, dest);
            }
        }
        static void copy(std::size_t i, const Union* src, Union* dest)
        {
            if (i == 0) {
                new (dest) T(*reinterpret_cast<const T*>(src));
            }
            else {
                variant_helper<Union, Ts...>::copy(--i, src, dest);
            }
        }
    };

    template <typename Union>
    struct variant_helper<Union> {
        static void destroy(std::size_t, Union*) {}
        static void move(std::size_t, Union*, Union*) {}
        static void copy(std::size_t, const Union*, Union*) {}
    };

    template <typename T, typename... Ts>
    struct type_index;

    template <typename T, typename... Ts>
    struct type_index<T, T, Ts...> : std::integral_constant<std::size_t, 0> {
    };

    template <typename T, typename U, typename... Ts>
    struct type_index<T, U, Ts...>
        : std::integral_constant<std::size_t, 1 + type_index<T, Ts...>::value> {
    };

    struct dummy_visit {
    };
    template <typename F, typename Union, typename... Args>
    struct do_visit_impl;
    template <typename F, typename Union, typename T>
    struct do_visit_impl<F, Union, T> {
        constexpr static decltype(auto) visit(F&& f,
                                              Union& u,
                                              std::size_t i,
                                              std::size_t c = 0)
        {
            if (i != c) {
                throw failure(bad_variant_access);
            }
            return f(*reinterpret_cast<T*>(&u));
        }
    };
    template <typename F, typename Union, typename T, typename... Args>
    struct do_visit_impl<F, Union, T, Args...> {
        constexpr static decltype(auto) visit(F&& f,
                                              Union& u,
                                              std::size_t i,
                                              std::size_t c = 0)
        {
            if (i == c) {
                return f(*reinterpret_cast<T*>(&u));
            }
            return do_visit_impl<F, Union, Args...>::visit(std::forward<F>(f),
                                                           u, i, ++c);
        }
    };

    template <typename F, typename Union, typename... Args>
    constexpr decltype(auto) do_visit(F&& f,
                                      Union& u,
                                      std::size_t i,
                                      std::size_t c = 0)
    {
        return do_visit_impl<F, Union, Args...>::visit(std::forward<F>(f), u, i,
                                                       c);
    }
}  // namespace detail

template <typename... Types>
class variant {
    using storage_type = std::aligned_union_t<sizeof(char), Types...>;
    using helper_type = detail::variant_helper<storage_type, Types...>;

    static_assert(sizeof...(Types) > 1,
                  "Variant must have at least 2 different types");

    struct default_state {
        int m{0};
    };

public:
    template <typename T,
              typename = std::enable_if_t<contains<T, Types...>::value>>
    variant(T&& val) noexcept(noexcept(T(std::forward<T>(val))))
        : m_index(detail::type_index<T, Types...>::value)
    {
        _construct<T>(std::forward<T>(val));
    }
    variant(const variant& o) : m_index(o.m_index)
    {
        helper_type::copy(m_index, &o.m_storage, &m_storage);
    }
    variant& operator=(const variant& o)
    {
        _destruct();
        m_index = o.m_index;
        helper_type::copy(m_index, &o.m_storage, &m_storage);
        return *this;
    }
    variant(variant&& o) : m_index(std::move(o.m_index))
    {
        helper_type::move(m_index, &o.m_storage, &m_storage);
    }
    variant& operator=(variant&& o)
    {
        _destruct();
        m_index = o.m_index;
        helper_type::move(m_index, &o.m_storage, &m_storage);
        return *this;
    }
    ~variant()
    {
        _destruct();
    }

    template <typename T,
              typename = std::enable_if_t<contains<T, Types...>::value>>
    bool is() const
    {
        return m_index == detail::type_index<T, Types...>();
    }
    std::size_t index() const
    {
        return m_index;
    }

    template <typename T,
              typename = std::enable_if_t<contains<T, Types...>::value>,
              typename... Args>
    void set(Args&&... args)
    {
        _destruct();
        _construct<T>(std::forward<Args>(args)...);
    }

    template <typename T,
              typename = std::enable_if_t<contains<T, Types...>::value>,
              typename... Args>
    static variant create(Args&&... args)
    {
        variant<Args...> v(default_state{});
        v.template _construct<T>(std::forward<Args>(args)...);
        return v;
    }

    template <typename T,
              typename = std::enable_if_t<contains<T, Types...>::value>>
    T& get()
    {
        if (m_index == detail::type_index<T, Types...>::value) {
            return *reinterpret_cast<T*>(&m_storage);
        }
        throw failure(bad_variant_access);
    }
    template <typename T,
              typename = std::enable_if_t<contains<T, Types...>::value>>
    const T& get() const
    {
        if (m_index == detail::type_index<T, Types...>::value) {
            return *reinterpret_cast<const T*>(&m_storage);
        }
        throw failure(bad_variant_access);
    }

    template <typename F>
    constexpr decltype(auto) visit(F&& f)
    {
        return detail::do_visit<F, storage_type, Types...>(std::forward<F>(f),
                                                           m_storage, m_index);
    }
    template <typename F>
    constexpr decltype(auto) visit(F&& f) const
    {
        return detail::do_visit<F, const storage_type,
                                std::add_const_t<Types>...>(std::forward<F>(f),
                                                            m_storage, m_index);
    }

private:
    variant(default_state) {}

    storage_type m_storage{};
    std::size_t m_index{0};

    template <typename T, typename... Args>
    void _construct(Args&&... args) noexcept(
        noexcept(T(std::forward<Args>(args)...)))
    {
        ::new (&m_storage) T(std::forward<Args>(args)...);
        m_index = detail::type_index<T, Types...>::value;
    }
    void _destruct()
    {
        helper_type::destroy(m_index, &m_storage);
    }
};

namespace detail {
    template <typename F, typename Tuple, std::size_t... I>
    decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
    {
        return f(t.operator[](I)...);
    }
}  // namespace detail

template <std::size_t N, typename F, typename Tuple>
decltype(auto) apply_n(F&& f, Tuple&& t)
{
    return detail::apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
                              std::make_index_sequence<N>{});
}

namespace detail {
    template <typename T, std::size_t Size>
    class small_vector_storage {
        using array_type = std::array<T, Size>;

    public:
        static constexpr auto MaxSize = Size;

        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using iterator = pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        small_vector_storage() = default;
        small_vector_storage(size_type count, const T& value)
            : m_end(m_storage.data() + count)
        {
            std::fill(m_storage.begin(), m_storage.begin() + count, value);
        }
        explicit small_vector_storage(size_type count)
            : m_end(m_storage.data() + count)
        {
        }
        template <typename InputIt,
                  typename = std::enable_if_t<
                      std::is_base_of<std::input_iterator_tag,
                                      typename std::iterator_traits<
                                          InputIt>::iterator_category>::value>>
        small_vector_storage(InputIt first, InputIt last)
        {
            std::copy(first, last, m_storage.begin());
            m_end = m_storage.data() + std::distance(first, last);
        }
        small_vector_storage(std::initializer_list<T> init)
        {
            std::copy(init.begin(), init.end(), m_storage.begin());
            m_end = m_storage.data() + init.size();
        }

        small_vector_storage(const small_vector_storage& other)
        {
            std::copy(other.begin(), other.begin(), m_storage.begin());
            m_end = m_storage.data() + other.size();
        }
        small_vector_storage& operator=(const small_vector_storage& other)
        {
            if (this != &other) {
                array_type tmp;
                std::copy(other.begin(), other.end(), tmp.begin());

                m_storage.swap(tmp);
                m_end = m_storage.data() + other.size();
            }
            return *this;
        }
        small_vector_storage(small_vector_storage&& other) noexcept
            : m_storage(std::move(other.m_storage)),
              m_end(m_storage.data() + other.size())
        {
        }
        small_vector_storage& operator=(small_vector_storage&& other) noexcept
        {
            auto size = other.size();
            m_storage.swap(other.m_storage);
            m_end = m_storage.data() + size();
        }
        ~small_vector_storage() noexcept = default;

        constexpr iterator begin()
        {
            return m_storage.data();
        }
        constexpr const_iterator begin() const
        {
            return m_storage.data();
        }

        constexpr iterator end()
        {
            return m_end;
        }
        constexpr const_iterator end() const
        {
            return m_end;
        }

        constexpr size_type size() const noexcept
        {
            return static_cast<size_type>(std::distance(begin(), end()));
        }
        static constexpr size_type max_size() noexcept
        {
            return Size;
        }

        void push_back(const T& val)
        {
            _check_space();
            *m_end = val;
            ++m_end;
        }
        void push_back(T&& val)
        {
            _check_space();
            *m_end = std::move(val);
            ++m_end;
        }

        constexpr pointer data()
        {
            return m_end;
        }
        constexpr const_pointer data() const
        {
            return m_end;
        }

        constexpr reference operator[](size_type i)
        {
            return m_storage[i];
        }
        constexpr const_reference operator[](size_type i) const
        {
            return m_storage[i];
        }

    private:
        void _check_space(std::size_t n)
        {
            if (n > max_size() - size()) {
                throw failure{out_of_range};
            }
        }

        std::array<T, Size> m_storage{};
        T* m_end{m_storage.data()};
    };

    template <typename T>
    struct empty_small_vector_storage {
        std::size_t size() const
        {
            return 0;
        }

        [[noreturn]] T& operator[](std::size_t)
        {
            SPIO_UNREACHABLE;
        }
        [[noreturn]] const T& operator[](std::size_t) const
        {
            SPIO_UNREACHABLE;
        }

        [[noreturn]] T* data()
        {
            SPIO_UNREACHABLE;
        }
        [[noreturn]] const T* data() const
        {
            SPIO_UNREACHABLE;
        }
    };
}  // namespace detail
template <typename T, typename Allocator, std::size_t Size>
class small_vector {
    using empty_vector_type = detail::empty_small_vector_storage<T>;
    using stack_vector_type = detail::small_vector_storage<T, Size>;
    using heap_vector_type = std::vector<T, Allocator>;
    using variant_type =
        variant<empty_vector_type, stack_vector_type, heap_vector_type>;

public:
    static constexpr auto max_packed_size = Size;

    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    small_vector() noexcept = default;

    small_vector(size_type count,
                 const T& value,
                 const Allocator& alloc = Allocator())
        : m_vec(_construct(count, value, alloc))
    {
    }
    explicit small_vector(size_type count, const Allocator& alloc = Allocator())
        : m_vec(_construct(count, alloc))
    {
    }
    small_vector(std::initializer_list<T> init,
                 const Allocator& alloc = Allocator())
        : m_vec(_construct(init, alloc))
    {
    }

    constexpr size_type size() const
    {
        return m_vec.visit([](const auto& vec) { return vec.size(); });
    }

    constexpr iterator begin()
    {
        return m_vec.visit([](auto& vec) { return vec.data(); });
    }
    constexpr const_iterator begin() const
    {
        return m_vec.visit([](const auto& vec) { return vec.data(); });
    }

    constexpr iterator end()
    {
        return m_vec.visit([](auto& vec) { return vec.data() + vec.size(); });
    }
    constexpr const_iterator end() const
    {
        return m_vec.visit(
            [](const auto& vec) { return vec.data() + vec.size(); });
    }

    constexpr pointer data()
    {
        return m_vec.visit([](auto& vec) { return vec.data(); });
    }
    constexpr const_pointer data() const
    {
        return m_vec.visit([](const auto& vec) { return vec.data(); });
    }

    constexpr reference operator[](size_type i)
    {
        return m_vec.visit([i](auto& vec) -> reference { return vec[i]; });
    }
    constexpr const_reference operator[](size_type i) const
    {
        return m_vec.visit(
            [i](const auto& vec) -> const_reference { return vec[i]; });
    }

    constexpr variant_type& get()
    {
        return m_vec;
    }
    constexpr const variant_type& get() const
    {
        return m_vec;
    }

private:
    static variant_type _construct(size_type count,
                                   const T& value,
                                   const Allocator& alloc)
    {
        if (count == 0) {
            return variant_type(empty_vector_type{});
        }
        else if (count <= Size) {
            return variant_type(stack_vector_type(count, value));
        }
        else {
            return variant_type(heap_vector_type(count, value, alloc));
        }
    }
    static variant_type _construct(size_type count, const Allocator& alloc)
    {
        if (count == 0) {
            return variant_type(empty_vector_type{});
        }
        else if (count <= Size) {
            return variant_type(stack_vector_type(count));
        }
        else {
            return variant_type(heap_vector_type(count, alloc));
        }
    }
    static variant_type _construct(std::initializer_list<T> init,
                                   const Allocator& alloc)
    {
        const auto count = init.size();
        if (count == 0) {
            return variant_type(empty_vector_type{});
        }
        else if (count <= Size) {
            return variant_type(stack_vector_type(init));
        }
        else {
            return variant_type(heap_vector_type(init, alloc));
        }
    }

    variant_type m_vec{empty_vector_type{}};
};

namespace detail {
    template <typename A, typename B>
    static constexpr bool is_same()
    {
#if SPIO_HAS_TYPE_TRAITS_V
        return std::is_same_v<A, B>;
#else
        return std::is_same<A, B>::value;
#endif
    }

#if SPIO_HAS_IF_CONSTEXPR
    template <typename FloatingT>
    static constexpr auto powersOf10()
    {
        using T = std::decay_t<FloatingT>;
        if constexpr (is_same<T, float>()) {
            return std::array<float, 6>{
                {10.f, 100.f, 1.0e4f, 1.0e8f, 1.0e16f, 1.0e32f}};
        }
        if constexpr (is_same<T, double>()) {
            return std::array<double, 9>{{10., 100., 1.0e4, 1.0e8, 1.0e16,
                                          1.0e32, 1.0e64, 1.0e128, 1.0e256}};
        }
        else {
#ifdef _MSC_VER
            return std::array<long double, 9>{{10.l, 100.l, 1.0e4l, 1.0e8l,
                                               1.0e16l, 1.0e32l, 1.0e64l,
                                               1.0e128l, 1.0e256l}};
#else
            return std::array<long double, 11>{
                {10.l, 100.l, 1.0e4l, 1.0e8l, 1.0e16l, 1.0e32l, 1.0e64l,
                 1.0e128l, 1.0e256l, 1.0e512l, 1.0e1024l}};
#endif
        }
    }

    template <typename FloatingT>
    static constexpr auto maxExponent()
    {
        using T = std::decay_t<FloatingT>;
        if constexpr (is_same<T, float>()) {
            return 63;
        }
        if constexpr (is_same<T, double>()) {
            return 511;
        }
        else {
            return 2047;
        }
    }
#else
    template <typename FloatingT>
    constexpr auto powersOf10()
    {
#ifdef _MSC_VER
        return std::array<long double, 11>{{10.l, 100.l, 1.0e4l, 1.0e8l,
                                            1.0e16l, 1.0e32l, 1.0e64l, 1.0e128l,
                                            1.0e256l}};
#else
        return std::array<long double, 11>{{10.l, 100.l, 1.0e4l, 1.0e8l,
                                            1.0e16l, 1.0e32l, 1.0e64l, 1.0e128l,
                                            1.0e256l, 1.0e512l, 1.0e1024l}};
#endif
    }
    template <>
    constexpr auto powersOf10<float>()
    {
        return std::array<float, 6>{
            {10.f, 100.f, 1.0e4f, 1.0e8f, 1.0e16f, 1.0e32f}};
    }
    template <>
    constexpr auto powersOf10<double>()
    {
        return std::array<double, 9>{{10., 100., 1.0e4, 1.0e8, 1.0e16, 1.0e32,
                                      1.0e64, 1.0e128, 1.0e256}};
    }

    template <typename FloatingT>
    constexpr auto maxExponent()
    {
        return 2047;
    }
    template <>
    constexpr auto maxExponent<float>()
    {
        return 63;
    }
    template <>
    constexpr auto maxExponent<double>()
    {
        return 511;
    }
#endif
}  // namespace detail

/*
 * The original C implementation of this function:
 *
 * strtod.c --
 *
 *	Source code for the "strtod" library procedure.
 *
 * Copyright (c) 1988-1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */
template <typename FloatingT, typename CharT>
FloatingT str_to_floating(const CharT* str, CharT** end)
{
    static int maxExponent = detail::maxExponent<FloatingT>();
    static auto powersOf10 = detail::powersOf10<FloatingT>();

    bool sign;
    bool expSign = false;
    FloatingT fraction, dblExp, *d;
    const CharT* p;
    int c;
    int exp = 0;       /* Exponent read from "EX" field. */
    int fracExp = 0;   /* Exponent that derives from the fractional
                        * part.  Under normal circumstatnces, it is
                        * the negative of the number of digits in F.
                        * However, if I is very long, the last digits
                        * of I get dropped (otherwise a long I with a
                        * large negative exponent could cause an
                        * unnecessary overflow on I alone).  In this
                        * case, fracExp is incremented one for each
                        * dropped digit. */
    int mantSize;      /* Number of digits in mantissa. */
    int decPt;         /* Number of mantissa digits BEFORE decimal
                        * point. */
    const CharT* pExp; /* Temporarily holds location of exponent
                        * in string. */

    /*
     * Strip off leading blanks and check for a sign.
     */

    p = str;
    while (std::isspace(*p)) {
        p += 1;
    }
    if (*p == '-') {
        sign = true;
        p += 1;
    }
    else {
        if (*p == '+') {
            p += 1;
        }
        sign = false;
    }

    /*
     * Count the number of digits in the mantissa (including the decimal
     * point), and also locate the decimal point.
     */

    decPt = -1;
    for (mantSize = 0;; mantSize += 1) {
        c = *p;
        if (!std::isdigit(c)) {
            if ((c != '.') || (decPt >= 0)) {
                break;
            }
            decPt = mantSize;
        }
        p += 1;
    }

    /*
     * Now suck up the digits in the mantissa.  Use two integers to
     * collect 9 digits each (this is faster than using floating-point).
     * If the mantissa has more than 18 digits, ignore the extras, since
     * they can't affect the value anyway.
     */

    pExp = p;
    p -= mantSize;
    if (decPt < 0) {
        decPt = mantSize;
    }
    else {
        mantSize -= 1; /* One of the digits was the point. */
    }
    if (mantSize > 18) {
        fracExp = decPt - 18;
        mantSize = 18;
    }
    else {
        fracExp = decPt - mantSize;
    }
    if (mantSize == 0) {
        fraction = static_cast<FloatingT>(0.0);
        p = str;
        goto done;
    }
    else {
        int frac1, frac2;
        frac1 = 0;
        for (; mantSize > 9; mantSize -= 1) {
            c = *p;
            p += 1;
            if (c == '.') {
                c = *p;
                p += 1;
            }
            frac1 = 10 * frac1 + (c - '0');
        }
        frac2 = 0;
        for (; mantSize > 0; mantSize -= 1) {
            c = *p;
            p += 1;
            if (c == '.') {
                c = *p;
                p += 1;
            }
            frac2 = 10 * frac2 + (c - '0');
        }
        fraction = (static_cast<FloatingT>(1.0e9) * frac1) + frac2;
    }

    /*
     * Skim off the exponent.
     */

    p = pExp;
    if ((*p == 'E') || (*p == 'e')) {
        p += 1;
        if (*p == '-') {
            expSign = true;
            p += 1;
        }
        else {
            if (*p == '+') {
                p += 1;
            }
            expSign = false;
        }
        while (std::isdigit(*p)) {
            exp = exp * 10 + (*p - '0');
            p += 1;
        }
    }
    if (expSign) {
        exp = fracExp - exp;
    }
    else {
        exp = fracExp + exp;
    }

    /*
     * Generate a floating-point number that represents the exponent.
     * Do this by processing the exponent one bit at a time to combine
     * many powers of 2 of 10. Then combine the exponent with the
     * fraction.
     */

    if (exp < 0) {
        expSign = true;
        exp = -exp;
    }
    else {
        expSign = false;
    }
    if (exp > maxExponent) {
        exp = maxExponent;
        errno = ERANGE;
    }
    dblExp = static_cast<FloatingT>(1.0);
    for (d = &powersOf10[0]; exp != 0; exp >>= 1, d += 1) {
        if (exp & 01) {
            dblExp *= *d;
        }
    }
    if (expSign) {
        fraction /= dblExp;
    }
    else {
        fraction *= dblExp;
    }

done:
    if (end != nullptr) {
        *end = const_cast<CharT*>(p);
    }

    if (sign) {
        return -fraction;
    }
    return fraction;
}
}  // namespace spio

#endif
