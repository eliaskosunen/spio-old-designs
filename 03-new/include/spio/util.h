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

#include <algorithm>
#include <cstring>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "config.h"

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
    template <template <typename...> class T,
              typename CharT,
              typename... Options>
    struct apply_fmt_supported_types {
        using type = T<Options...,
                       const int*,
                       const unsigned*,
                       const long long*,
                       const unsigned long long*,
                       const double*,
                       const long double*,
                       const void**,
                       fmt::internal::string_value<CharT>,
                       fmt::internal::string_value<signed char>,
                       fmt::internal::string_value<unsigned char>>;
    };

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

    template <typename F, typename Union, typename T, typename... Args>
    auto do_visit(F&& f, Union& u, std::size_t i, std::size_t c = 0)
    {
        if (i == c) {
            return f(*reinterpret_cast<T*>(&u));
        }
        return do_visit<F, Union, Args...>(std::forward<F>(f), u, i, ++c);
    }
    template <typename F, typename Pointer, typename T, typename... Args>
    auto do_pointer_visit(F&& f, Pointer p, std::size_t i, std::size_t c = 0)
    {
        if (i == c) {
            return f(reinterpret_cast<T>(p));
        }
        return do_pointer_visit<F, Pointer, Args...>(std::forward<F>(f), p, i,
                                                     ++c);
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
    variant(T&& val) : m_index(detail::type_index<T, Types...>::value)
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
    auto visit(F&& f)
    {
        return detail::do_visit<F, storage_type, Types...>(std::forward<F>(f),
                                                           m_storage, m_index);
    }

private:
    variant(default_state) {}

    storage_type m_storage{};
    std::size_t m_index{0};

    template <typename T, typename... Args>
    void _construct(Args&&... args)
    {
        ::new (&m_storage) T(std::forward<Args>(args)...);
        m_index = detail::type_index<T, Types...>::value;
    }
    void _destruct()
    {
        helper_type::destroy(m_index, &m_storage);
    }
};

template <typename Pointer, typename... Types>
class variant_pointer {
    static_assert(sizeof...(Types) > 1,
                  "Variant must have at least 2 different types");

public:
    using pointer_type = Pointer;

    template <typename T,
              typename = std::enable_if_t<contains<T, Types...>::value>>
    variant_pointer(T val)
        : m_ptr(reinterpret_cast<Pointer>(val)),
          m_index(detail::type_index<T, Types...>::value)
    {
    }

    template <typename T,
              typename = std::enable_if_t<contains<T, Types...>::value>>
    bool is() const
    {
        return m_index == detail::type_index<T, Types...>::value;
    }
    std::size_t index() const
    {
        return m_index;
    }

    template <typename T,
              typename = std::enable_if_t<contains<T, Types...>::value>>
    void set(T val)
    {
        m_ptr = reinterpret_cast<Pointer>(val);
        m_index = detail::type_index<T, Types...>::value;
    }

    template <typename T,
              typename = std::enable_if_t<contains<T, Types...>::value>>
    T get()
    {
        if (m_index == detail::type_index<T, Types...>::value) {
            return reinterpret_cast<T>(m_ptr);
        }
        throw failure(bad_variant_access);
    }
    template <typename T,
              typename = std::enable_if_t<contains<T, Types...>::value>>
    const T get() const
    {
        if (m_index == detail::type_index<T, Types...>::value) {
            return reinterpret_cast<const T>(m_ptr);
        }
        throw failure(bad_variant_access);
    }

    template <typename F>
    auto visit(F&& f)
    {
        return detail::do_pointer_visit<F, Pointer, Types...>(
            std::forward<F>(f), m_ptr, m_index);
    }

private:
    Pointer m_ptr{};
    std::size_t m_index{0};
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

template <typename... Types>
class basic_arg {
public:
    using variant_type = variant_pointer<const void*, Types...>;

    basic_arg(variant_type v) : m_arg(std::move(v)) {}

    variant_type& operator*()
    {
        return m_arg;
    }
    const variant_type& operator*() const
    {
        return m_arg;
    }

    variant_type& get()
    {
        return m_arg;
    }
    const variant_type& get() const
    {
        return m_arg;
    }

    variant_type* operator->()
    {
        return &m_arg;
    }
    const variant_type* operator->() const
    {
        return &m_arg;
    }

private:
    variant_type m_arg;
};

template <typename Size, typename... Types>
class basic_arg_list {
public:
    using arg_type = basic_arg<Types...>;
    using storage_type = std::vector<arg_type>;
    static constexpr auto arg_count = Size::value;

    basic_arg_list(storage_type v) : m_vec(std::move(v)) {}
    template <typename... Args>
    basic_arg_list(Args&... a)
        : m_vec{std::initializer_list<basic_arg<Args...>>{a...}}
    {
    }

    basic_arg_list& operator[](std::size_t i)
    {
        return m_vec[i];
    }
    const basic_arg_list& operator[](std::size_t i) const
    {
        return m_vec[i];
    }

    storage_type& get()
    {
        return m_vec;
    }
    const storage_type& get() const
    {
        return m_vec;
    }

    template <typename Transformer>
    auto transform()
    {
        return Transformer::template transform<Size>(std::move(m_vec));
    }

private:
    storage_type m_vec;
};

template <typename T, typename... Args>
auto make_args(Args&... a)
{
    typename T::storage_type vec{std::initializer_list<typename T::arg_type>{
        typename T::arg_type(std::addressof(a))...}};
    return T(std::move(vec));
}
}  // namespace spio

#endif
