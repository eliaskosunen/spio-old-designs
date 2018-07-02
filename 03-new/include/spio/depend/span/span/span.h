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
//

#ifndef SPAN_SPAN_H
#define SPAN_SPAN_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#ifdef _MSC_VER
#define SPAN_MSC_VER _MSC_VER
#else
#define SPAN_MSC_VER 0
#endif

// C++17 feature detect
// if constexpr
#if defined(__cpp_if_constexpr)
#define SPAN_HAS_IF_CONSTEXPR 1
#else
#define SPAN_HAS_IF_CONSTEXPR 0
#endif

// Deduction guides
#if defined(__cpp_deduction_guides)
#if defined(__clang__) && __clang_major__ < 6
#define SPAN_HAS_DEDUCTION_GUIDES 0
#else
#define SPAN_HAS_DEDUCTION_GUIDES 1
#endif  // __clang__
#else
#define SPAN_HAS_DEDUCTION_GUIDES 0
#endif

// void_t
#if defined(__cpp_lib_void_t)
#define SPAN_HAS_VOID_T 1
#else
#define SPAN_HAS_VOID_T 0
#endif

// std::byte
#if defined(__cpp_lib_byte)
#define SPAN_HAS_BYTE 1
#else
#define SPAN_HAS_BYTE 0
#endif

// C++14 feature detect
// (type_trait)_t instead of (type_trait)::type
#if defined(__cpp_lib_transformation_trait_aliases)
#define SPAN_HAS_TYPE_TRAIT_T 1
#else
#define SPAN_HAS_TYPE_TRAIT_T 0
#endif

// make_reverse_iterator
#if defined(__cpp_lib_make_reverse_iterator)
#define SPAN_HAS_MAKE_REVERSE_ITERATOR 1
#else
#define SPAN_HAS_MAKE_REVERSE_ITERATOR 0
#endif

// Relaxed constexpr
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304
#define SPAN_HAS_RELAXED_CONSTEXPR 1
#else
#define SPAN_HAS_RELAXED_CONSTEXPR 0
#endif

// Inherting constructors
#if defined(__cpp_inheriting_constuctors)
#define SPAN_HAS_INHERITING_CONSTRUCTORS 1
#else
#define SPAN_HAS_INHERITING_CONSTRUCTORS 0
#endif

// std::equal with two-range overloads
#if defined(__cpp_lib_robust_nonmodifying_seq_ops)
#define SPAN_HAS_EQUAL 1
#else
#define SPAN_HAS_EQUAL 0
#endif

// [[noreturn]]
#if defined(__cpp_attributes)
#define SPAN_HAS_NORETURN 1
#else
#define SPAN_HAS_NORETURN 0
#endif

// Portability
#if SPAN_HAS_RELAXED_CONSTEXPR
#define SPAN_CONSTEXPR constexpr
#else
#define SPAN_CONSTEXPR /*constexpr*/
#endif

#if SPAN_HAS_NORETURN
#define SPAN_NORETURN [[noreturn]]
#else
#define SPAN_NORETURN /*noreturn*/
#endif

#ifdef SPAN_NOTHROW
#define SPAN_THROW(t) assert(false && t)
#else
#define SPAN_THROW(t) throw ::span::exception(t)
#endif

// In C++11-mode (no relaxed constexpr) we assert in constexpr functions by
// throwing On g++ version < 5, this emits a warning for "always terminating
// throw" This is exactly the behavior we want, so we silence the warning
#if defined(__GNUC__) && __GNUC__ >= 5 && !defined(__clang__) && \
    !SPAN_HAS_RELAXED_CONSTEXPR
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wterminate"
#endif

// Same deal with MSVC
// "function assumed not to throw an exception but does"
#if SPAN_MSC_VER
#pragma warning(push)
#pragma warning(disable : 4297)
#endif

/// Span namespace
namespace span {
/// Implementation details namespace
namespace detail {
// Prefer type_trait_t aliases
#if SPAN_HAS_TYPE_TRAIT_T
    using ::std::add_const_t;
    using ::std::add_lvalue_reference_t;
    using ::std::add_pointer_t;
    using ::std::enable_if_t;
    using ::std::remove_const_t;
    using ::std::remove_cv_t;
    using ::std::remove_reference_t;
#else
    template <bool B, class T = void>
    using enable_if_t = typename ::std::enable_if<B, T>::type;
    template <typename T>
    using add_pointer_t = typename ::std::add_pointer<T>::type;
    template <typename T>
    using add_lvalue_reference_t =
        typename ::std::add_lvalue_reference<T>::type;
    template <typename T>
    using add_const_t = typename ::std::add_const<T>::type;
    template <typename T>
    using remove_cv_t = typename ::std::remove_cv<T>::type;
    template <typename T>
    using remove_const_t = typename ::std::remove_const<T>::type;
    template <typename T>
    using remove_reference_t = typename ::std::remove_reference<T>::type;
#endif

#if SPAN_HAS_VOID_T
    using ::std::void_t;
#else
    template <typename...>
    using void_t = void;
#endif

#if SPAN_HAS_MAKE_REVERSE_ITERATOR
    using ::std::make_reverse_iterator;
#else
    template <typename Iterator>
    std::reverse_iterator<Iterator> make_reverse_iterator(Iterator i)
    {
        return std::reverse_iterator<Iterator>(i);
    }
#endif

#if SPAN_HAS_EQUAL
    using ::std::equal;
#else
    template <typename InputIt1, typename InputIt2>
    SPAN_CONSTEXPR bool equal(InputIt1 first1,
                              InputIt1 last1,
                              InputIt2 first2,
                              InputIt2 last2)
    {
        for (; !(first1 == last1) && !(first2 == last2);
             ++first1, (void)++first2) {
            if (!(*first1 == *first2)) {
                return false;
            }
        }
        return first1 == last1 && first2 == last2;
    }
#endif
}  // namespace detail

using extent_t = std::ptrdiff_t;
constexpr const extent_t dynamic_extent = -1;

// Forward declare for use in support classes
template <typename ValueType, extent_t Extent = dynamic_extent>
class span;

// Dumb warning if you ask me
#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
class exception : public std::runtime_error {
public:
#if SPAN_HAS_INHERITING_CONSTRUCTORS
    using runtime_error::runtime_error;
#else
    // Identical to runtime_error constructors
    explicit exception(const char* s) : runtime_error(s) {}
    explicit exception(const std::string& s) : runtime_error(s) {}
#endif
};

class assert_failure : public exception {
public:
    SPAN_NORETURN explicit assert_failure(const char* s) : exception(s)
    {
        std::fprintf(stderr, "Assertion failure: %s\n", s);
        std::exit(EXIT_FAILURE);
    }
};
#if defined(__clang__)
#pragma GCC diagnostic pop
#endif

namespace detail {
    template <extent_t From, extent_t To>
    struct is_allowed_extent_conversion
        : public std::integral_constant<bool,
                                        From == To || From == dynamic_extent ||
                                            To == dynamic_extent> {
    };

    template <class From, class To>
    struct is_allowed_element_type_conversion
        : public std::integral_constant<
              bool,
              std::is_convertible<From (*)[], To (*)[]>::value> {
    };

    /**
     * Class which stores the pointer to the span data.
     * We specialize this class for `dynamic_extent`, so we can save the space
     * of storing the size when it's known at compile time
     * \see storage_type<ValueType, dynamic_extent>
     */
    template <typename ValueType, extent_t Extent>
    class storage_type {
    public:
        constexpr const static extent_t extent = Extent;

        // Second parameter defined in the prototype but unused to provide
        // compatibility with the `dynamic_extent` specialization
        constexpr storage_type(ValueType* d, extent_t) : m_data(d) {}
        /// Get pointer to data
        constexpr ValueType* data() const noexcept
        {
            return m_data;
        }
        /// Get element at index.
        /// Requires `Idx < Extent`
        template <extent_t Idx>
        constexpr ValueType* at() const noexcept
        {
            static_assert(Idx < Extent,
                          "Out of bounds access in span storage_type");
            return data() + Idx;
        }
        /// Get element at index.
        /// Requires `idx < Extent`
        constexpr ValueType* at(extent_t idx) const noexcept
        {
#if SPAN_HAS_RELAXED_CONSTEXPR
            assert(idx >= 0 && idx < Extent &&
                   "Out of bounds access in span storage_type");
            return data() + idx;
#else
            return idx >= 0 && idx < Extent
                       ? data() + idx
                       : throw assert_failure(
                             "span::storage_type::at: Out of bounds access");
#endif
        }
        /// Get element at index, or `nullptr` if out of range
        constexpr ValueType* at_checked(extent_t idx) const noexcept
        {
            return idx >= 0 && idx < Extent ? data() + idx : nullptr;
        }
        /// Get size of span
        constexpr extent_t size() const noexcept
        {
            return Extent;
        }

    private:
        ValueType* m_data;
        // No size stored!
    };

    /**
     * Specialization of storage_type which stores the size when not known at
     * compile-time.
     * \see storage_type
     */
    template <typename ValueType>
    class storage_type<ValueType, dynamic_extent> {
    public:
        constexpr const static extent_t extent = dynamic_extent;

        constexpr storage_type(ValueType* d, extent_t s) : m_data(d), m_size(s)
        {
        }

        /// Get pointer to data
        constexpr ValueType* data() const noexcept
        {
            return m_data;
        }
        /// Get element at index.
        /// Requires `Idx < Extent`
        template <extent_t Idx>
        constexpr ValueType* at() const noexcept
        {
#if SPAN_HAS_RELAXED_CONSTEXPR
            assert(Idx < size() && "Out of bounds access in span storage_type");
            return data() + Idx;
#else
            return Idx < size()
                       ? data() + Idx
                       : throw assert_failure(
                             "span::storage_type::at: Out of bounds access");
#endif
        }
        /// Get element at index.
        /// Requires `Idx < Extent`
        constexpr ValueType* at(extent_t idx) const noexcept
        {
#if SPAN_HAS_RELAXED_CONSTEXPR
            assert(idx >= 0 && idx < size() &&
                   "Out of bounds access in span storage_type");
            return data() + idx;
#else
            return idx >= 0 && idx < size()
                       ? data() + idx
                       : throw assert_failure(
                             "span::storage_type::at: Out of bounds access");
#endif
        }
        /// Get element at index, or `nullptr` if out of range
        constexpr ValueType* at_checked(extent_t idx) const noexcept
        {
            return idx >= 0 && idx < size() ? data() + idx : nullptr;
        }
        /// Get size of span
        constexpr extent_t size() const noexcept
        {
            return m_size;
        }

    private:
        ValueType* m_data;
        extent_t m_size;
    };

    template <typename T>
    struct is_span_oracle : std::false_type {
    };

    template <typename ElementType, std::ptrdiff_t Extent>
    struct is_span_oracle<span<ElementType, Extent>> : std::true_type {
    };

    template <typename T>
    struct is_span : public is_span_oracle<detail::remove_cv_t<T>> {
    };

    template <typename T>
    struct is_std_array_oracle : std::false_type {
    };

    template <typename ElementType, std::size_t Extent>
    struct is_std_array_oracle<std::array<ElementType, Extent>>
        : std::true_type {
    };

    template <typename T>
    struct is_std_array : public is_std_array_oracle<detail::remove_cv_t<T>> {
    };

    template <typename T, typename = void, typename = void>
    struct is_input_iterator : std::false_type {
    };

    template <typename T>
    struct is_input_iterator<
        T,
        enable_if_t<std::is_copy_constructible<T>::value &&
                    std::is_copy_assignable<T>::value &&
                    std::is_destructible<T>::value>,
        void_t<decltype(*std::declval<T>()),
               decltype(++std::declval<T>()),
               decltype(std::declval<T>() == std::declval<T>()),
               decltype(std::declval<T>() != std::declval<T>()),
               decltype(std::declval<T>().operator->()),
               decltype((void)std::declval<T>()++),
               decltype(*std::declval<T>()++)>> : std::true_type {
    };

// g++ bails out here for whatever reason
#if defined(__clang__) || !defined(__GNUC__)
    template <typename T, typename = void, typename = void>
    struct is_container : std::false_type {
    };

    template <typename T>
    struct is_container<T,
                        enable_if_t<std::is_copy_constructible<T>::value>,
                        void_t<decltype(std::declval<T>().size()),
                               decltype(std::declval<T>().begin()),
                               typename T::value_type>> : std::true_type {
    };
#else
    template <typename T, typename = void>
    struct is_container : std::true_type {
    };
#endif
}  // namespace detail

#ifndef NDEBUG
// Common base for `iterator` and `const_iterator`
template <typename Iterator>
struct span_iterator_base {
};

template <
    typename L,
    typename R,
    typename = detail::enable_if_t<
        std::is_same<typename L::value_type, typename R::value_type>::value>>
SPAN_CONSTEXPR bool operator==(const span_iterator_base<L>& lhs,
                               const span_iterator_base<R>& rhs)
{
    auto& l = static_cast<
        const detail::remove_const_t<detail::remove_reference_t<L>>&>(lhs);
    auto& r = static_cast<
        const detail::remove_const_t<detail::remove_reference_t<R>>&>(rhs);
    return l.get_span().data() == r.get_span().data() &&
           l.get_index() == r.get_index();
}
template <
    typename L,
    typename R,
    typename = detail::enable_if_t<
        std::is_same<typename L::value_type, typename R::value_type>::value>>
SPAN_CONSTEXPR bool operator!=(const span_iterator_base<L>& lhs,
                               const span_iterator_base<R>& rhs)
{
    return !(lhs == rhs);
}

template <
    typename L,
    typename R,
    typename = detail::enable_if_t<
        std::is_same<typename L::value_type, typename R::value_type>::value>>
SPAN_CONSTEXPR bool operator<(const span_iterator_base<L>& lhs,
                              const span_iterator_base<R>& rhs)
{
    auto& l = static_cast<
        const detail::remove_const_t<detail::remove_reference_t<L>>&>(lhs);
    auto& r = static_cast<
        const detail::remove_const_t<detail::remove_reference_t<R>>&>(rhs);
    assert(l.get_span().data() == r.get_span().data());
    return l.get_index() < r.get_index();
}
template <
    typename L,
    typename R,
    typename = detail::enable_if_t<
        std::is_same<typename L::value_type, typename R::value_type>::value>>
SPAN_CONSTEXPR bool operator<=(const span_iterator_base<L>& lhs,
                               const span_iterator_base<R>& rhs)
{
    return !(rhs < lhs);
}
template <
    typename L,
    typename R,
    typename = detail::enable_if_t<
        std::is_same<typename L::value_type, typename R::value_type>::value>>
SPAN_CONSTEXPR bool operator>(const span_iterator_base<L>& lhs,
                              const span_iterator_base<R>& rhs)
{
    return rhs < lhs;
}
template <typename L,
          typename R,
          typename = detail::enable_if_t<
              std::is_same<typename L::element_type,
                           typename R::element_type>::value>>
SPAN_CONSTEXPR bool operator>=(const span_iterator_base<L>& lhs,
                               const span_iterator_base<R>& rhs)
{
    return !(rhs > lhs);
}
#endif

/**
 * Span class.
 * A span is a view over a contiguous sequence of objects,
 * which storage is owned by some other object.
 */
template <typename ElementType, extent_t Extent>
class span {
public:
    using element_type = ElementType;
    using value_type = detail::remove_cv_t<element_type>;
    using index_type = extent_t;
    using difference_type = std::ptrdiff_t;
    using index_type_us = std::size_t;
    using pointer = detail::add_pointer_t<element_type>;
    using reference = detail::add_lvalue_reference_t<element_type>;
    using const_pointer =
        detail::add_const_t<detail::add_pointer_t<element_type>>;
    using const_reference =
        detail::add_const_t<detail::add_lvalue_reference_t<element_type>>;

    static constexpr const index_type extent = Extent;
    static constexpr const bool is_const = false;

#ifdef NDEBUG
    using iterator = pointer;
    using const_iterator = const_pointer;

private:
    iterator create_iterator(index_type i = 0)
    {
        return data() + i;
    }
    const_iterator create_iterator(index_type i = 0) const
    {
        return data() + i;
    }

public:
#else
    class iterator;
    class const_iterator;

    /**
     * Random-access mutable iterator over the elements of a span
     * \see const_iterator
     */
    class iterator : public span_iterator_base<iterator> {
    public:
        using span_type = span<ElementType, Extent>;
        using iterator_category = std::random_access_iterator_tag;
        using element_type = typename span_type::element_type;
        using value_type = typename span_type::value_type;
        using index_type = typename span_type::index_type;
        using difference_type = typename span_type::difference_type;
        using pointer = detail::add_pointer_t<element_type>;
        using reference = detail::add_lvalue_reference_t<element_type>;
        using const_pointer =
            detail::add_const_t<detail::add_pointer_t<element_type>>;
        using const_reference =
            detail::add_const_t<detail::add_lvalue_reference_t<element_type>>;

        friend span_type;

        constexpr iterator() = default;

        /**
         * Construct an iterator.
         * Requires `i >= 0 && i <= s.length()`
         * \param s Span *this points to
         * \param i Index of the element *this points to
         */
        constexpr explicit iterator(span_type* s, index_type i) noexcept
            : m_span(s), m_index(i)
        {
#if SPAN_HAS_RELAXED_CONSTEXPR
            assert(s == nullptr || (i >= 0 && i <= s->length()));
#endif
        }

        SPAN_CONSTEXPR reference operator*() noexcept
        {
            return (*m_span)[m_index];
        }
        SPAN_CONSTEXPR pointer operator->() noexcept
        {
            return &(operator*());
        }
        constexpr const_reference operator*() const noexcept
        {
            return (*m_span)[m_index];
        }
        constexpr const_pointer operator->() const noexcept
        {
            return &(operator*());
        }

        SPAN_CONSTEXPR span_type& get_span() noexcept
        {
            return *m_span;
        }
        constexpr const span_type& get_span() const noexcept
        {
            return *m_span;
        }
        constexpr index_type get_index() const noexcept
        {
            return m_index;
        }

        SPAN_CONSTEXPR iterator& operator++() noexcept
        {
            assert(m_index >= 0 && m_index < m_span->size());
            ++m_index;
            return *this;
        }
        SPAN_CONSTEXPR iterator& operator--() noexcept
        {
            assert(m_index > 0 && m_index <= m_span->size());
            --m_index;
            return *this;
        }

        SPAN_CONSTEXPR iterator operator++(int)noexcept
        {
            auto ret = *this;
            ++(*this);
            return ret;
        }
        SPAN_CONSTEXPR iterator operator--(int)noexcept
        {
            auto ret = *this;
            --(*this);
            return ret;
        }

        SPAN_CONSTEXPR iterator operator+(difference_type n) const noexcept
        {
            auto ret = *this;
            ret += n;
            return ret;
        }
        SPAN_CONSTEXPR iterator operator-(difference_type n) const noexcept
        {
            auto ret = *this;
            ret -= n;
            return ret;
        }
        SPAN_CONSTEXPR iterator& operator+=(difference_type n) noexcept
        {
            assert((m_index + n) >= 0 && (m_index + n) <= m_span->size());
            m_index += n;
            return *this;
        }
        SPAN_CONSTEXPR iterator& operator-=(difference_type n) noexcept
        {
            return *this += -n;
        }

        /**
         * Get the difference between *this an another `iterator` or
         * `const_iterator`. Requires that `r` points to the same span, or
         * `get_span().data() == r.get_span().data()`
         */
        template <typename T>
        constexpr difference_type operator-(
            const span_iterator_base<T>& r) const noexcept
        {
#if SPAN_HAS_RELAXED_CONSTEXPR
            auto rhs = static_cast<const T&>(r);
            assert(m_span->data() == rhs.m_span->data());
            return static_cast<difference_type>(m_index - rhs.m_index);
#else
            return m_span->data() == static_cast<const T&>(r).m_span->data()
                       ? static_cast<difference_type>(
                             m_index - static_cast<const T&>(r).m_index)
                       : throw assert_failure(
                             "span::iterator::operator-: rhs points to "
                             "different data");
#endif
        }

        SPAN_CONSTEXPR operator const_iterator() const noexcept
        {
            return _get_const_iterator();
        }

    private:
        SPAN_CONSTEXPR const_iterator _get_const_iterator() const noexcept;

        span_type* m_span{nullptr};
        index_type m_index{0};
    };

    /**
     * Random-access constant iterator over the elements of a span
     * \see iterator
     */
    class const_iterator : public span_iterator_base<const_iterator> {
    public:
        using span_type = const span<ElementType, Extent>;
        using iterator_category = std::random_access_iterator_tag;
        using element_type =
            detail::add_const_t<typename span_type::element_type>;
        using value_type = typename span_type::value_type;
        using index_type = typename span_type::index_type;
        using difference_type = typename span_type::difference_type;
        using pointer = detail::add_pointer_t<element_type>;
        using reference = detail::add_lvalue_reference_t<element_type>;
        using const_pointer = pointer;
        using const_reference = reference;

        static constexpr const index_type extent = Extent;
        static constexpr const bool is_const = false;

        friend span_type;

        constexpr const_iterator() = default;

        /**
         * Construct an iterator.
         * Requires `i >= 0 && i <= s.length()`
         * \param s Span *this points to
         * \param i Index of the element *this points to
         */
        constexpr explicit const_iterator(span_type* s, index_type i) noexcept
            : m_span(s), m_index(i)
        {
#if SPAN_HAS_RELAXED_CONSTEXPR
            assert(s == nullptr || (i >= 0 && i <= s->length()));
#endif
        }

        constexpr const_iterator(const iterator& it) noexcept
            : const_iterator(it.m_span, it.m_index)
        {
        }

        constexpr reference operator*() const noexcept
        {
            return m_span->operator[](m_index);
        }
        constexpr pointer operator->() const noexcept
        {
            return &(operator*());
        }

        constexpr const span_type& get_span() const noexcept
        {
            return *m_span;
        }
        constexpr index_type get_index() const noexcept
        {
            return m_index;
        }

        SPAN_CONSTEXPR const_iterator& operator++() noexcept
        {
            assert(m_index >= 0 && m_index < m_span->size());
            ++m_index;
            return *this;
        }
        SPAN_CONSTEXPR const_iterator& operator--() noexcept
        {
            assert(m_index > 0 && m_index <= m_span->size());
            --m_index;
            return *this;
        }

        SPAN_CONSTEXPR const const_iterator operator++(int)noexcept
        {
            auto ret = *this;
            ++(*this);
            return ret;
        }
        SPAN_CONSTEXPR const const_iterator operator--(int)noexcept
        {
            auto ret = *this;
            --(*this);
            return ret;
        }

        SPAN_CONSTEXPR const_iterator operator+(difference_type n) const
            noexcept
        {
            auto ret = *this;
            ret += n;
            return ret;
        }
        SPAN_CONSTEXPR const_iterator operator-(difference_type n) const
            noexcept
        {
            auto ret = *this;
            ret -= n;
            return ret;
        }
        SPAN_CONSTEXPR const_iterator& operator+=(difference_type n) noexcept
        {
            assert((m_index + n) >= 0 && (m_index + n) <= m_span->size());
            m_index += n;
            return *this;
        }
        SPAN_CONSTEXPR const_iterator& operator-=(difference_type n) noexcept
        {
            return *this += -n;
        }

        /**
         * Get the difference between *this an another `iterator` or
         * `const_iterator`. Requires that `r` points to the same span, or
         * `get_span().data() == r.get_span().data()`
         */
        template <typename T>
        constexpr difference_type operator-(
            const span_iterator_base<T>& r) const noexcept
        {
#if SPAN_HAS_RELAXED_CONSTEXPR
            auto rhs = static_cast<const T&>(r);
            assert(m_span->data() == rhs.m_span->data());
            return static_cast<difference_type>(m_index - rhs.m_index);
#else
            return m_span->data() == static_cast<const T&>(r).m_span->data()
                       ? static_cast<difference_type>(
                             m_index - static_cast<const T&>(r).m_index)
                       : throw assert_failure(
                             "span::const_iterator::operator-: rhs points to "
                             "different data");
#endif
        }

    private:
        span_type* m_span{nullptr};
        index_type m_index{0};
    };

private:
    iterator create_iterator(index_type i = 0)
    {
        return iterator{this, i};
    }
    const_iterator create_iterator(index_type i = 0) const
    {
        return const_iterator{this, i};
    }

public:
#endif  // NDEBUG

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static_assert(Extent >= 0 || Extent == dynamic_extent,
                  "Extent must be >= 0 or dynamic_extent");

    /// Construct an empty span
    constexpr span() noexcept : span(nullptr) {}
    /// Construct an empty span
    constexpr span(std::nullptr_t) noexcept : m_storage(nullptr, 0)
    {
        static_assert(Extent == dynamic_extent || Extent == 0,
                      "Invalid extent for an empty span");
    }
    /**
     * Construct a span over the sequence pointed to by `ptr` and with the size
     * of `count`. If `ptr` is a null ponter or `count == 0` an empty span is
     * constructed.
     */
    SPAN_CONSTEXPR span(pointer ptr, index_type count) : m_storage(ptr, count)
    {
        assert(ptr || count == 0);
        assert(count >= 0);
        assert(Extent == dynamic_extent || count == Extent);
    }
    /**
     * Construct a span over the range `[firstElem, lastElem)`.
     * If `ptr` is a null pointer or `std::distance(firstElem, lastElem) == 0`
     * an empty span is constructed.
     */
    SPAN_CONSTEXPR span(pointer firstElem, pointer lastElem)
        : span(firstElem, std::distance(firstElem, lastElem))
    {
        assert(std::distance(firstElem, lastElem) >= 0);
    }

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif
    /**
     * Construct a span over the sequence pointed to by the iterator `first` and
     * with the size of `count`. If `count == 0` an empty span is constructed.
     * `first` must point to a `ContiguousContainer`, or the program is
     * ill-formed. This constructor is not present at the standards proposal.
     */
    template <typename InputIt,
              typename = detail::enable_if_t<
                  detail::is_input_iterator<InputIt>::value &&
                  std::is_convertible<decltype(*std::declval<InputIt>()),
                                      element_type>::value>>
    constexpr span(InputIt first, index_type count) : span(&*first, count)
    {
    }
    /**
     * Construct a span over the range `[first, last)`.
     * `first` and `last` must both be `InputIterator`s.
     * If `std::distance(first, last) == 0` an empty span is constructed.
     * `first` must point to a `ContiguousContainer`, or the program is
     * ill-formed. This constructor is not present at the standards proposal.
     */
    template <typename InputIt,
              typename = detail::enable_if_t<
                  detail::is_input_iterator<InputIt>::value &&
                  std::is_convertible<decltype(*std::declval<InputIt>()),
                                      element_type>::value>>
    constexpr span(InputIt first, InputIt last)
        : span(&*first, std::distance(first, last))
    {
    }
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

    /**
     * Construct a span over the supplied array.
     * Requires `Extent == dynamic_extent || N == Extent`.
     */
    template <
        size_t N,
        typename = detail::enable_if_t<Extent == dynamic_extent || N == Extent>>
    constexpr span(element_type (&arr)[N]) noexcept
        : m_storage(std::addressof(arr[0]), N)
    {
    }

    /**
     * Construct a span over the supplied array.
     * Requires `Extent == dynamic_extent || N == Extent`.
     */
    template <
        size_t N,
        typename Element = element_type,
        typename = typename detail::enable_if_t<
            (Extent == dynamic_extent || N == Extent) &&
            std::is_same<typename detail::remove_const_t<Element>,
                         typename detail::remove_const_t<element_type>>::value>>
    constexpr span(std::array<Element, N>& arr) noexcept
        : m_storage(std::addressof(arr[0]), N)
    {
    }

    /**
     * Construct a span over the sequence of objects managed by `ptr` and with
     * the size of `count`. If `ptr.get()` is a null ponter or `count == 0` an
     * empty span is constructed.
     */
    template <typename T = pointer>
    SPAN_CONSTEXPR span(const std::unique_ptr<T>& ptr,
                        index_type count) noexcept
        : m_storage(ptr.get(), count)
    {
        assert(ptr ? (Extent == dynamic_extent ? count >= 0 : count == Extent)
                   : count == 0);
    }

    /**
     * Construct a span over the object managed by `ptr`.
     * If `ptr.get() == nullptr` an empty span is constructed, otherwise `size()
     * == 1`.
     */
    constexpr span(const std::unique_ptr<element_type>& ptr) noexcept
        : m_storage(ptr.get(), ptr ? 1 : 0)
    {
    }
    /**
     * Construct a span over the object managed by `ptr`.
     * If `ptr.get() == nullptr` an empty span is constructed, otherwise `size()
     * == 1`.
     */
    constexpr span(const std::shared_ptr<element_type>& ptr) noexcept
        : m_storage(ptr.get(), ptr ? 1 : 0)
    {
    }

    /**
     * Construct a span over the sequence of elements owned by `c`.
     * `Container` shall be a `ContiguousContainer`, `SequenceContainer`, have
     * `operator[]`, and `Container::value_type ==
     * std::remove_const<element_type>::type`.
     * Does not participate in overload resolution if `Container` is a `span` or
     * `std::array`.
     */
    template <
        typename Container,
        typename = detail::enable_if_t<
            !detail::is_span<Container>::value &&
            !detail::is_std_array<Container>::value &&
            detail::is_container<Container>::value &&
            std::is_same<typename Container::value_type,
                         typename detail::remove_const_t<element_type>>::value>,
        typename = detail::void_t<
            decltype(std::declval<Container>()[std::declval<std::size_t>()])>>
    constexpr span(Container& c) noexcept
        : m_storage(&*c.begin(), static_cast<index_type>(c.size()))
    {
#if SPAN_HAS_RELAXED_CONSTEXPR
        assert(Extent == dynamic_extent ||
               static_cast<index_type>(c.size()) == Extent);
#endif
    }

    /**
     * Construct a span over the sequence of elements owned by `c`.
     * `Container` shall be a `ContiguousContainer`, `SequenceContainer`, have
     * `operator[]`, and `Container::value_type ==
     * std::remove_const<element_type>::type`.
     * Does not participate in overload resolution if `Container` is a `span` or
     * `std::array`, or `std::is_const<element_type>::value == false`.
     */
    template <
        typename Container,
        typename = detail::enable_if_t<
            !detail::is_span<Container>::value &&
            !detail::is_std_array<Container>::value &&
            detail::is_container<Container>::value &&
            std::is_const<element_type>::value &&
            std::is_same<typename Container::value_type,
                         typename detail::remove_const_t<element_type>>::value>,
        typename = detail::void_t<
            decltype(std::declval<Container>()[std::declval<std::size_t>()])>>
    constexpr span(const Container& c)
        : m_storage(&*c.begin(), static_cast<index_type>(c.size()))
    {
#if SPAN_HAS_RELAXED_CONSTEXPR
        assert(Extent == dynamic_extent || c.size() == Extent);
#endif
    }

    constexpr span(const span& other) noexcept = default;
    constexpr span(span&& other) noexcept = default;

    template <
        typename OtherElementType,
        index_type OtherExtent,
        typename = detail::enable_if_t<
            detail::is_allowed_extent_conversion<OtherExtent, Extent>::value &&
            detail::is_allowed_element_type_conversion<OtherElementType,
                                                       element_type>::value>>
    constexpr span(const span<OtherElementType, OtherExtent>& other)
        : m_storage(other.data(), other.size())
    {
    }
    template <
        typename OtherElementType,
        index_type OtherExtent,
        typename = detail::enable_if_t<
            detail::is_allowed_extent_conversion<OtherExtent, Extent>::value &&
            detail::is_allowed_element_type_conversion<OtherElementType,
                                                       element_type>::value>>
    constexpr span(span<OtherElementType, OtherExtent>&& other)
        : m_storage(other.data(), other.size())
    {
    }
    SPAN_CONSTEXPR span& operator=(const span& other) noexcept = default;
    SPAN_CONSTEXPR span& operator=(span&& other) noexcept = default;
    ~span() noexcept = default;

    /**
     * Return a span that is a view over the initial `Count` elements of the
     * current span. Requires `Count >= 0 && Count <= size()`
     */
    template <index_type Count>
    constexpr span<element_type, Count> first() const noexcept
    {
#if SPAN_HAS_RELAXED_CONSTEXPR
        assert(Count >= 0 && Count <= size());
        return {data(), Count};
#else
        return (Count >= 0 && Count <= size())
                   ? span<element_type, Count>{data(), Count}
                   : throw assert_failure("span::first: Count out of range");
#endif
    }
    /**
     * Return a span that is a view over the final `Count` elements of the
     * current span. Requires `Count > 0 && Count <= size()`.
     * The requirements for this function are different in the standards
     * proposal, which requires `Count >= 0 && ...`
     */
    template <index_type Count>
    constexpr span<element_type, Count> last() const noexcept
    {
#if SPAN_HAS_RELAXED_CONSTEXPR
        assert(Count > 0 && Count <= size());
        return {_at_ptr(size() - Count), Count};
#else
        return (Count > 0 && Count <= size())
                   ? span<element_type, Count>{_at_ptr(size() - Count), Count}
                   : throw assert_failure("span::last: Count out of range");
#endif
    }
    /**
     * Return a span that is a view over `Count` elements of the current span
     * starting at element `Offset`. If `Count == dynamic_extent`, calling has
     * similar behavior to calling `first<Offset>()`,
     * meaning that a span over all elements from `Offset onwards is returned.
     *
     * Requires `(Offset == 0 || (Offset > 0 && Offset < size())) &&`
     *          `(Count == dynamic_extent ||`
     *          `(Count >= 0 && Offset + Count <= size()))`
     */
    template <index_type Offset, index_type Count = dynamic_extent>
    SPAN_CONSTEXPR span<element_type, Count> subspan() const noexcept
    {
        assert((Offset == 0 || (Offset > 0 && Offset < size())) &&
               (Count == dynamic_extent ||
                (Count >= 0 && Offset + Count <= size())));
#if SPAN_HAS_IF_CONSTEXPR
        if constexpr (Count == dynamic_extent) {
            return {_at_ptr(Offset), size() - Offset};
        }
        else {
            return {_at_ptr(Offset), Count};
        }
#else
        return {_at_ptr(Offset),
                Count == dynamic_extent ? size() - Offset : Count};
#endif
    }

    /**
     * Return a span that is a view over the initial `count` elements of the
     * current span. Requires `count >= 0 && count <= size()`
     */
    constexpr span<element_type, dynamic_extent> first(index_type count) const
        noexcept
    {
#if SPAN_HAS_RELAXED_CONSTEXPR
        assert(count >= 0 && count <= size());
        return {data(), count};
#else
        return (count >= 0 && count <= size())
                   ? span<element_type>{data(), count}
                   : throw assert_failure("span::first: Count out of range");
#endif
    }
    /**
     * Return a span that is a view over the final `count` elements of the
     * current span. Requires `count >= 0 && count <= size()`
     */
    constexpr span<element_type, dynamic_extent> last(index_type count) const
        noexcept
    {
#if SPAN_HAS_RELAXED_CONSTEXPR
        assert(count >= 0 && count <= size());
        return {_at_ptr(size() - count), count};
#else
        return (count >= 0 && count <= size())
                   ? span<element_type>{_at_ptr(size() - count), count}
                   : throw assert_failure("span::first: Count out of range");
#endif
    }
    /**
     * Return a span that is a view over `count` elements of the current span
     * starting at element `offset`. If `count == dynamic_extent`, calling has
     * similar behavior to calling `first(offset)`,
     * meaning that a span over all elements from `Offset onwards is returned.
     *
     * Requires `(offset == 0 || (offset > 0 && offset < size())) &&`
     *          `(count == dynamic_extent ||`
     *          `(count >= 0 && offset + count <= size()))`
     */
    SPAN_CONSTEXPR span<element_type, dynamic_extent> subspan(
        index_type offset,
        index_type count = dynamic_extent) const noexcept
    {
        assert((offset == 0 || (offset > 0 && offset < size())) &&
               (count == dynamic_extent ||
                (count >= 0 && offset + count <= size())));
        return {_at_ptr(offset),
                count == dynamic_extent ? size() - offset : count};
    }

    /**
     * Equivalent to `size()`
     * \see `size()`
     */
    constexpr index_type length() const noexcept
    {
        return size();
    }
    /**
     * Return the number of elements accesible through this `span`
     */
    constexpr index_type size() const noexcept
    {
#if SPAN_HAS_RELAXED_CONSTEXPR
        const auto s = m_storage.size();
        assert(s >= 0);
        return s;
#else
        return m_storage.size() >= 0
                   ? m_storage.size()
                   : throw assert_failure("span::size: size is negative");
#endif
    }
    /**
     * Equivalent to `size_us()`.
     * \see size_us()
     */
    constexpr index_type_us length_us() const noexcept
    {
        return size_us();
    }
    /**
     * Return the number of elements accessible through this `span` as an
     * unsigned integer
     */
    constexpr index_type_us size_us() const noexcept
    {
        return static_cast<index_type_us>(size());
    }

    /**
     * Equivalent to `size_bytes()`.
     * \see size_bytes()
     */
    constexpr index_type length_bytes() const noexcept
    {
        return size_bytes();
    }
    /**
     * Return the number of bytes used for the object representation of all
     * elements accessible through this `span`.
     * \return `size() * sizeof(element_type)`
     */
    constexpr index_type size_bytes() const noexcept
    {
        return size() * static_cast<index_type>(sizeof(element_type));
    }
    /**
     * Equivalent to `size_bytes_us()`.
     * \see size_bytes_us()
     */
    constexpr index_type_us length_bytes_us() const noexcept
    {
        return size_bytes_us();
    }
    /**
     * Return the number of bytes used for the object representation of all
     * elements accessible through this `span` as an unsigned integer.
     * \return `size() * sizeof(element_type)`
     */
    constexpr index_type_us size_bytes_us() const noexcept
    {
        return static_cast<index_type_us>(size_bytes());
    }
    /**
     * \return `size() == 0`
     * \see size()
     */
    constexpr bool empty() const noexcept
    {
        return size() == 0;
    }

    /**
     * Return a reference to the element at position `idx`.
     * Requires `idx >= 0 && idx < size()`.
     * \return `*(data() + idx)`
     */
    constexpr reference operator[](index_type idx) const noexcept
    {
        return _at(idx);
    }
    /**
     * Equivalent to `operator[](idx)`.
     * \see operator[]()
     */
    constexpr reference operator()(index_type idx) const noexcept
    {
        return _at(idx);
    }
    /**
     * Return a reference to the element at position `idx` or thrown an
     * exception if `idx` is out of range. If `SPAN_NOTHROW` is defined and
     * `idx` is not valid, `assert` is called.
     * This function is not present at the standards proposal.
     *
     * \exception exception if `!(idx >= 0 && idx < size())`
     * \return `*(data() + idx)`
     */
    reference at(index_type idx) const
    {
        auto ptr = m_storage.at_checked(idx);
        if (!ptr) {
            SPAN_THROW("span::at: access out of range");
        }
        return *ptr;
    }

    /**
     * Return either a pointer to the first element in the sequence accessible
     * via this `span` or the null pointer if that was the value used to
     * construct this `span`
     */
    constexpr pointer data() const noexcept
    {
        return m_storage.data();
    }

    /// Return an iterator referring to the first element in the span
    SPAN_CONSTEXPR iterator begin() noexcept
    {
        return create_iterator();
    }
    /// Return a past-the-end iterator of this span
    SPAN_CONSTEXPR iterator end() noexcept
    {
        return create_iterator(size());
    }

    /// Return an constant iterator referring to the first element in the span
    constexpr const_iterator begin() const noexcept
    {
        return create_iterator();
    }
    /// Return a constant past-the-end iterator of this span
    constexpr const_iterator end() const noexcept
    {
        return create_iterator(size());
    }

    /**
     * Return a constant iterator to the first element in the span.
     * This function is not present at the standards proposal.
     */
    constexpr const_iterator cbegin() const noexcept
    {
        return create_iterator();
    }
    /**
     * Return a constant past-the-end iterator of this span
     * This function is not present at the standards proposal.
     */
    constexpr const_iterator cend() const noexcept
    {
        return create_iterator(size());
    }

    /// \return `std::reverse_iterator(end())`
    SPAN_CONSTEXPR reverse_iterator rbegin() noexcept
    {
        return detail::make_reverse_iterator(end());
    }
    /// \return `std::reverse_iterator(begin())`
    SPAN_CONSTEXPR reverse_iterator rend() noexcept
    {
        return detail::make_reverse_iterator(begin());
    }

    /// \return `std::reverse_iterator(end())`
    constexpr const_reverse_iterator rbegin() const noexcept
    {
        return detail::make_reverse_iterator(end());
    }
    /// \return `std::reverse_iterator(begin())`
    constexpr const_reverse_iterator rend() const noexcept
    {
        return detail::make_reverse_iterator(begin());
    }

    /// \return `std::reverse_iterator(cend())`
    /// This function is not present at the standards proposal.
    constexpr const_reverse_iterator crbegin() const noexcept
    {
        return detail::make_reverse_iterator(cend());
    }
    /// \return `std::reverse_iterator(cbegin())`
    /// This function is not present at the standards proposal.
    constexpr const_reverse_iterator crend() const noexcept
    {
        return detail::make_reverse_iterator(cbegin());
    }

    /**
     * Return a new constant span that points to the same sequence of elements.
     * This function is not present at the standards proposal.
     * \return `span<...>{const_cast<add_const_t<ElementType>*>(data()),
     * size()}`
     */
    constexpr span<detail::add_const_t<ElementType>, Extent> as_const_span()
        const noexcept
    {
        return {const_cast<detail::add_const_t<ElementType>*>(data()), size()};
    }

private:
    detail::storage_type<ElementType, Extent> m_storage;

    constexpr reference _at(index_type idx) const noexcept
    {
        return *_at_ptr(idx);
    }
    constexpr pointer _at_ptr(index_type idx) const noexcept
    {
        return m_storage.at(idx);
    }
};  // namespace span

#ifndef NDEBUG
template <typename T, extent_t N>
SPAN_CONSTEXPR auto span<T, N>::iterator::_get_const_iterator() const noexcept
    -> const_iterator
{
    return span<T, N>::const_iterator(m_span, m_index);
}
#endif

template <typename ElementL,
          typename ElementR,
          extent_t ExtentL,
          extent_t ExtentR,
          typename = detail::enable_if_t<
              (detail::is_allowed_element_type_conversion<ElementL,
                                                          ElementR>::value &&
               detail::is_allowed_extent_conversion<ExtentL, ExtentR>::value) ||
              (detail::is_allowed_element_type_conversion<ElementR,
                                                          ElementL>::value &&
               detail::is_allowed_extent_conversion<ExtentR, ExtentL>::value)>>
inline bool operator==(const span<ElementL, ExtentL>& l,
                       const span<ElementR, ExtentR>& r)
{
    return detail::equal(l.begin(), l.end(), r.begin(), r.end());
}
template <typename ElementL,
          typename ElementR,
          extent_t ExtentL,
          extent_t ExtentR>
inline bool operator!=(const span<ElementL, ExtentL>& l,
                       const span<ElementR, ExtentR>& r)
{
    return !(l == r);
}
template <typename ElementL,
          typename ElementR,
          extent_t ExtentL,
          extent_t ExtentR,
          typename = detail::enable_if_t<
              (detail::is_allowed_element_type_conversion<ElementL,
                                                          ElementR>::value &&
               detail::is_allowed_extent_conversion<ExtentL, ExtentR>::value) ||
              (detail::is_allowed_element_type_conversion<ElementR,
                                                          ElementL>::value &&
               detail::is_allowed_extent_conversion<ExtentR, ExtentL>::value)>>
inline bool operator<(const span<ElementL, ExtentL>& l,
                      const span<ElementR, ExtentR>& r)
{
    return std::lexicographical_compare(l.begin(), l.end(), r.begin(), r.end());
}
template <typename ElementL,
          typename ElementR,
          extent_t ExtentL,
          extent_t ExtentR>
inline bool operator<=(const span<ElementL, ExtentL>& l,
                       const span<ElementR, ExtentR>& r)
{
    return !(l > r);
}
template <typename ElementL,
          typename ElementR,
          extent_t ExtentL,
          extent_t ExtentR>
inline bool operator>(const span<ElementL, ExtentL>& l,
                      const span<ElementR, ExtentR>& r)
{
    return (r < l);
}
template <typename ElementL,
          typename ElementR,
          extent_t ExtentL,
          extent_t ExtentR>
inline bool operator>=(const span<ElementL, ExtentL>& l,
                       const span<ElementR, ExtentR>& r)
{
    return !(l < r);
}

#if SPAN_HAS_DEDUCTION_GUIDES
template <typename Element>
span(Element* ptr, typename span<Element>::index_type count)->span<Element>;
template <typename Element>
span(Element* first, Element* last)->span<Element>;

template <typename T, std::size_t N>
span(std::array<T, N> arr)->span<T, static_cast<extent_t>(N)>;

template <
    typename Container,
    typename = detail::enable_if_t<detail::is_container<Container>::value &&
                                   !std::is_const<Container>::value>>
span(Container& c)->span<typename Container::value_type>;
template <
    typename Container,
    typename = detail::enable_if_t<detail::is_container<Container>::value>>
span(const Container& c)->span<const typename Container::value_type>;

template <typename ElementType, std::size_t N>
span(ElementType (&arr)[N])->span<ElementType, static_cast<extent_t>(N)>;
#endif

// Helper functions for pre-C++17
template <typename Element,
          typename = detail::enable_if_t<!std::is_const<Element>::value>>
constexpr span<Element> make_span(Element* ptr,
                                  typename span<Element>::index_type count)
{
    return span<Element>(ptr, count);
}

template <typename Element>
constexpr span<const Element> make_span(
    const Element* ptr,
    typename span<const Element>::index_type count)
{
    return span<const Element>(ptr, count);
}

template <typename Element,
          typename = detail::enable_if_t<!std::is_const<Element>::value>>
constexpr span<Element> make_span(Element* first, Element* last)
{
    return span<Element>(first, last);
}

template <typename Element>
constexpr span<const Element> make_span(const Element* first,
                                        const Element* last)
{
    return span<const Element>(first, last);
}

template <
    typename InputIt,
    typename =
        decltype(*std::declval<InputIt&>(), ++std::declval<InputIt&>(), void()),
    typename ValueType = typename std::iterator_traits<InputIt>::value_type>
constexpr span<ValueType> make_span(InputIt first, InputIt last)
{
    return span<ValueType>(first, last);
}

template <
    typename Element,
    std::size_t N,
    typename = detail::enable_if_t<N != 0 && !std::is_const<Element>::value>>
constexpr auto make_span(Element (&arr)[N])
    -> span<Element, static_cast<extent_t>(N)>
{
    return span<Element, static_cast<extent_t>(N)>(arr);
}

template <typename Element,
          std::size_t N,
          typename = detail::enable_if_t<N != 0>>
constexpr auto make_span(const Element (&arr)[N])
    -> span<const Element, static_cast<extent_t>(N)>
{
    return span<const Element, static_cast<extent_t>(N)>(arr);
}

template <typename Container,
          typename = detail::enable_if_t<
              !std::is_const<typename Container::value_type>::value &&
              !std::is_const<Container>::value>>
constexpr span<typename Container::value_type> make_span(Container& c)
{
    return span<typename Container::value_type>(c);
}

template <typename Container>
constexpr span<const typename Container::value_type> make_span(
    const Container& c)
{
    return span<const typename Container::value_type>(c);
}

template <extent_t N, typename Element>
constexpr span<Element, N> make_span(Element* ptr)
{
    return span<Element, N>(ptr, N);
}

template <extent_t N,
          typename Container,
          typename = detail::enable_if_t<
              !std::is_const<typename Container::value_type>::value &&
              !std::is_const<Container>::value>>
SPAN_CONSTEXPR span<typename Container::value_type, N> make_span(Container& c)
{
    assert(std::distance(std::begin(c), std::end(c)) >= N);
    return span<typename Container::value_type, N>(&*std::begin(c), N);
}

template <extent_t N,
          typename Container,
          typename = detail::void_t<typename Container::value_type>>
SPAN_CONSTEXPR span<const typename Container::value_type, N> make_span(
    const Container& c)
{
    assert(std::distance(std::begin(c), std::end(c)) >= N);
    return span<const typename Container::value_type, N>(&*std::begin(c), N);
}

#if !SPAN_HAS_BYTE || (defined(SPAN_BYTE_USE_UCHAR) && SPAN_BYTE_USE_UCHAR)
/// `byte_type` defaults to `std::byte`. If it is not available, or
/// `SPAN_BYTE_USE_CHAR` is defined to `true`, `unsigned char` is used
using byte_type = unsigned char;
#else
using byte_type = std::byte;
#endif

#if !SPAN_MSC_VER
/**
 * Construct a span over the object representation of the elements in `s`.
 * \return `{ reinterpret_cast<const byte_type*>(s.data()), s.size_bytes() }`
 * \see as_writable_bytes
 * \see byte_type
 */
template <typename ElementType, extent_t Extent>
inline SPAN_CONSTEXPR
    span<const byte_type,
         (Extent == dynamic_extent) ? dynamic_extent
                                    : (extent_t{sizeof(ElementType)} * Extent)>
    as_bytes(span<ElementType, Extent> s) noexcept
{
    return {reinterpret_cast<const byte_type*>(s.data()), s.size_bytes()};
}
/**
 * Construct a span over the object representation of the elements in `s`.
 * Does not participate in overload resolution when
 * `std::is_const<ElementType>::value == true`
 *
 * \return `{ reinterpret_cast<byte_type*>(s.data()), s.size_bytes() }`
 * \see as_bytes
 * \see byte_type
 */
template <typename ElementType,
          extent_t Extent,
          typename = detail::enable_if_t<!std::is_const<ElementType>::value>>
inline SPAN_CONSTEXPR
    span<byte_type,
         (Extent == dynamic_extent) ? dynamic_extent
                                    : (extent_t{sizeof(ElementType)} * Extent)>
    as_writable_bytes(span<ElementType, Extent> s) noexcept
{
    return {reinterpret_cast<byte_type*>(s.data()), s.size_bytes()};
}
#else
// All spans returned are dynamic_extent because MSVC is confused with anything
// related to templates
template <typename ElementType, extent_t N>
inline constexpr span<const byte_type> as_bytes(span<ElementType, N> s) noexcept
{
    return {reinterpret_cast<const byte_type*>(s.data()),
            static_cast<extent_t>(sizeof(ElementType)) * s.size()};
}
template <typename ElementType,
          extent_t N,
          typename = detail::enable_if_t<!std::is_const<ElementType>::value>>
inline constexpr span<byte_type> as_writable_bytes(
    span<ElementType, N> s) noexcept
{
    return {reinterpret_cast<byte_type*>(s.data()),
            static_cast<extent_t>(sizeof(ElementType)) * s.size()};
}
#endif  // !SPAN_MSC_VER

using const_byte_span = span<const byte_type>;
using writable_byte_span = span<byte_type>;
}  // namespace span

#if defined(__GNUC__) && __GNUC__ >= 5 && !defined(__clang__) && \
    !SPAN_HAS_RELAXED_CONSTEXPR
#pragma GCC diagnostic pop
#endif

#if SPAN_MSC_VER
#pragma warning(pop)
#endif

#endif  // SPAN_SPAN_H
