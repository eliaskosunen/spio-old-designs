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
//
// This file is based on the `span`-implementation of the GSL:
// https://github.com/Microsoft/GSL/blob/master/include/gsl/span
//
// Copyright (c) 2015 Microsoft Corporation. All rights reserved.
//
// This code is licensed under the MIT License (MIT).
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef SPIO_SPAN_H
#define SPIO_SPAN_H

#include <cstddef>
#include "config.h"
#include "error.h"
#include "stl.h"

namespace io {
using span_extent_type = std::ptrdiff_t;
constexpr auto dynamic_extent = -1;

template <typename ValueType, span_extent_type Extent = dynamic_extent>
class span;

namespace detail {
    template <span_extent_type From, span_extent_type To>
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

    template <typename ValueType, span_extent_type Extent>
    class storage_type {
    public:
        constexpr storage_type(ValueType* d, span_extent_type) : m_data(d) {}
        constexpr ValueType* data() const noexcept
        {
            return m_data;
        }
        template <span_extent_type Idx>
        constexpr ValueType* at() const noexcept
        {
            static_assert(Idx < Extent,
                          "Out of bounds access in span storage_type");
            return data() + Idx;
        }
        constexpr ValueType* at(span_extent_type idx) const noexcept
        {
            assert(idx < Extent && "Out of bounds access in span storage_type");
            return data() + idx;
        }
        constexpr ValueType* at_checked(span_extent_type idx) const noexcept
        {
            if (idx < Extent) {
                return data() + idx;
            }
            return nullptr;
        }
        constexpr span_extent_type size() const noexcept
        {
            return Extent;
        }

    private:
        ValueType* m_data;
    };
    template <typename ValueType>
    class storage_type<ValueType, dynamic_extent> {
    public:
        constexpr storage_type(ValueType* d, span_extent_type s)
            : m_data(d), m_size(s)
        {
        }

        constexpr ValueType* data() const noexcept
        {
            return m_data;
        }
        template <span_extent_type Idx>
        constexpr ValueType* at() const noexcept
        {
            assert(Idx < size() && "Out of bounds access in span storage_type");
            return data() + Idx;
        }
        constexpr ValueType* at(span_extent_type idx) const noexcept
        {
            assert(idx < size() && "Out of bounds access in span storage_type");
            return data() + idx;
        }
        constexpr ValueType* at_checked(span_extent_type idx) const noexcept
        {
            if (idx < size()) {
                return data() + idx;
            }
            return nullptr;
        }
        constexpr span_extent_type size() const noexcept
        {
            return m_size;
        }

    private:
        ValueType* m_data;
        span_extent_type m_size;
    };

    template <typename Iterator>
    struct span_iterator_base {
    };

    template <typename L,
              typename R,
              typename = std::enable_if_t<
                  std::is_same<typename L::element_type,
                               typename R::element_type>::value>>
    constexpr bool operator==(const span_iterator_base<L>& lhs,
                              const span_iterator_base<R>& rhs)
    {
        auto l = static_cast<const L&>(lhs);
        auto r = static_cast<const R&>(rhs);
        return l.get_span().data() == r.get_span().data() &&
               l.get_index() == r.get_index();
    }
    template <typename L,
              typename R,
              typename = std::enable_if_t<
                  std::is_same<typename L::element_type,
                               typename R::element_type>::value>>
    constexpr bool operator!=(const span_iterator_base<L>& lhs,
                              const span_iterator_base<R>& rhs)
    {
        return !(lhs == rhs);
    }

    template <typename L,
              typename R,
              typename = std::enable_if_t<
                  std::is_same<typename L::element_type,
                               typename R::element_type>::value>>
    constexpr bool operator<(const span_iterator_base<L>& lhs,
                             const span_iterator_base<R>& rhs)
    {
        auto l = static_cast<const L&>(lhs);
        auto r = static_cast<const R&>(rhs);
        assert(l.get_span().data() == r.get_span().data());
        return l.get_index() < r.get_index();
    }
    template <typename L,
              typename R,
              typename = std::enable_if_t<
                  std::is_same<typename L::element_type,
                               typename R::element_type>::value>>
    constexpr bool operator<=(const span_iterator_base<L>& lhs,
                              const span_iterator_base<R>& rhs)
    {
        return !(rhs < lhs);
    }
    template <typename L,
              typename R,
              typename = std::enable_if_t<
                  std::is_same<typename L::element_type,
                               typename R::element_type>::value>>
    constexpr bool operator>(const span_iterator_base<L>& lhs,
                             const span_iterator_base<R>& rhs)
    {
        return rhs < lhs;
    }
    template <typename L,
              typename R,
              typename = std::enable_if_t<
                  std::is_same<typename L::element_type,
                               typename R::element_type>::value>>
    constexpr bool operator>=(const span_iterator_base<L>& lhs,
                              const span_iterator_base<R>& rhs)
    {
        return !(rhs > lhs);
    }

    template <class T>
    struct is_span_oracle : std::false_type {
    };

    template <class ElementType, std::ptrdiff_t Extent>
    struct is_span_oracle<span<ElementType, Extent>> : std::true_type {
    };

    template <class T>
    struct is_span : public is_span_oracle<std::remove_cv_t<T>> {
    };

    template <class T>
    struct is_stl_array_oracle : std::false_type {
    };

    template <class ElementType, std::size_t Extent>
    struct is_stl_array_oracle<stl::array<ElementType, Extent>>
        : std::true_type {
    };

    template <class T>
    struct is_stl_array : public is_stl_array_oracle<std::remove_cv_t<T>> {
    };
}  // namespace detail

template <typename ElementType, span_extent_type Extent>
class span {
public:
    using element_type = ElementType;
    using value_type = std::remove_cv_t<element_type>;
    using index_type = span_extent_type;
    using difference_type = std::ptrdiff_t;
    using index_type_us = std::size_t;
    using pointer = std::add_pointer_t<element_type>;
    using reference = std::add_lvalue_reference_t<element_type>;

    constexpr static index_type extent = Extent;

    class iterator : public detail::span_iterator_base<iterator> {
    public:
        using span_type = const span<ElementType, Extent>;
        using iterator_category = std::random_access_iterator_tag;
        using element_type = typename span_type::element_type;
        using value_type = typename span_type::value_type;
        using index_type = typename span_type::index_type;
        using difference_type = std::ptrdiff_t;
        using reference = element_type&;
        using pointer = element_type*;

        friend span_type;

        constexpr iterator(span_type& s) : m_span(&s) {}
        constexpr iterator(span_type& s, index_type i) noexcept
            : m_span(&s), m_index(i)
        {
            assert(i >= 0 && i <= s.length());
        }

        constexpr reference operator*() noexcept
        {
            return m_span->operator[](m_index);
        }
        constexpr pointer operator->() noexcept
        {
            return m_span->_at_ptr(m_index);
        }

        constexpr pointer operator[](index_type idx) noexcept
        {
            return m_span->operator[](idx);
        }

        constexpr span_type& get_span() noexcept
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

        constexpr iterator& operator++() noexcept
        {
            assert(m_index >= 0 && m_index < m_span->size());
            ++m_index;
            return *this;
        }
        constexpr iterator& operator--() noexcept
        {
            assert(m_index > 0 && m_index <= m_span->size());
            --m_index;
            return *this;
        }

        constexpr const iterator operator++(int)noexcept
        {
            auto ret = *this;
            ++(*this);
            return ret;
        }
        constexpr const iterator operator--(int)noexcept
        {
            auto ret = *this;
            --(*this);
            return ret;
        }

        constexpr iterator operator+(difference_type n) const noexcept
        {
            auto ret = *this;
            ret += n;
            return ret;
        }
        constexpr iterator operator-(difference_type n) const noexcept
        {
            auto ret = *this;
            ret -= n;
            return ret;
        }
        constexpr iterator& operator+=(difference_type n) noexcept
        {
            assert((m_index + n) >= 0 && (m_index + n) <= m_span->size());
            m_index += n;
            return *this;
        }
        constexpr iterator& operator-=(difference_type n) noexcept
        {
            return *this += -n;
        }

        template <typename T>
        constexpr difference_type operator-(
            const detail::span_iterator_base<T>& r) const
        {
            auto rhs = static_cast<const T&>(r);
            assert(m_span->data() == rhs.m_span->data());
            return static_cast<difference_type>(m_index - rhs.m_index);
        }

    private:
        span_type* m_span;
        index_type m_index{0};
    };

    class const_iterator : public detail::span_iterator_base<const_iterator> {
    public:
        using span_type = const span<std::add_const_t<ElementType>, Extent>;
        using iterator_category = std::random_access_iterator_tag;
        using element_type = std::add_const_t<typename span_type::element_type>;
        using value_type = typename span_type::value_type;
        using index_type = std::add_const_t<typename span_type::index_type>;
        using difference_type = std::ptrdiff_t;
        using reference = element_type&;
        using pointer = element_type*;

        friend span_type;

        constexpr const_iterator(const span_type& s) : m_span(&s) {}
        constexpr const_iterator(const span_type& s, index_type i) noexcept
            : m_span(&s), m_index(i)
        {
            assert(i >= 0 && i <= s.length());
        }

        constexpr reference operator*() const noexcept
        {
            return m_span->operator[](m_index);
        }
        constexpr pointer operator->() const noexcept
        {
            return m_span->_at_ptr(m_index);
        }

        constexpr pointer operator[](index_type idx) const noexcept
        {
            return m_span->operator[](idx);
        }

        constexpr const span_type& get_span() const noexcept
        {
            return *m_span;
        }
        constexpr index_type get_index() const noexcept
        {
            return m_index;
        }

        constexpr const_iterator& operator++() noexcept
        {
            assert(m_index >= 0 && m_index < m_span->size());
            ++m_index;
            return *this;
        }
        constexpr const_iterator& operator--() noexcept
        {
            assert(m_index > 0 && m_index <= m_span->size());
            --m_index;
            return *this;
        }

        constexpr const const_iterator operator++(int)noexcept
        {
            auto ret = *this;
            ++(*this);
            return ret;
        }
        constexpr const const_iterator operator--(int)noexcept
        {
            auto ret = *this;
            --(*this);
            return ret;
        }

        constexpr const_iterator operator+(difference_type n) const noexcept
        {
            auto ret = *this;
            ret += n;
            return ret;
        }
        constexpr const_iterator operator-(difference_type n) const noexcept
        {
            auto ret = *this;
            ret -= n;
            return ret;
        }
        constexpr const_iterator& operator+=(difference_type n) noexcept
        {
            assert((m_index + n) >= 0 && (m_index + n) <= m_span->size());
            m_index += n;
            return *this;
        }
        constexpr const_iterator& operator-=(difference_type n) noexcept
        {
            return *this -= -n;
        }

        template <typename T>
        constexpr difference_type operator-(
            const detail::span_iterator_base<T>& r) const
        {
            auto rhs = static_cast<const T&>(r);
            assert(m_span->data() == rhs.m_span->data());
            return static_cast<difference_type>(m_index - rhs.m_index);
        }

    private:
        const span_type* m_span;
        index_type m_index{0};
    };

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static_assert(Extent >= 0 || Extent == dynamic_extent,
                  "Extent must be >= 0 or dynamic_extent");

    constexpr span() noexcept : span(nullptr) {}
    template <bool Dependent = false,
              typename = std::enable_if_t<
                  Dependent || (Extent == dynamic_extent || Extent == 0)>>
    constexpr span(std::nullptr_t) noexcept : m_storage(nullptr, 0)
    {
    }
    constexpr span(pointer ptr, index_type count) : m_storage(ptr, count)
    {
        assert(Extent == dynamic_extent || count == Extent);
    }
    constexpr span(pointer firstElem, pointer lastElem)
        : m_storage(firstElem, stl::distance(firstElem, lastElem))
    {
        assert(Extent == dynamic_extent || size() == Extent);
    }
    template <typename InputIt,
              typename = decltype(*std::declval<InputIt&>(),
                                  ++std::declval<InputIt&>(),
                                  void())>
    constexpr span(InputIt first, InputIt last)
        : span(&*first, stl::distance(first, last))
    {
    }
    template <
        size_t N,
        typename = std::enable_if_t<(Extent == dynamic_extent || N == Extent) &&
                                    !std::is_const<element_type>::value>>
    constexpr span(value_type (&arr)[N]) noexcept
        : m_storage(std::addressof(arr[0]), N)
    {
    }
    template <
        size_t N,
        typename = std::enable_if_t<(Extent == dynamic_extent || N == Extent)>>
    constexpr span(const value_type (&arr)[N]) noexcept
        : m_storage(std::addressof(arr[0]), N)
    {
    }

    template <std::size_t N,
              typename ArrayElement = std::remove_const_t<element_type>>
    constexpr span(stl::array<ArrayElement, N>& arr) noexcept
        : m_storage(std::addressof(arr[0]), N)
    {
    }

    template <std::size_t N>
    constexpr span(
        const stl::array<std::remove_const_t<element_type>, N>& arr) noexcept
        : m_storage(std::addressof(arr[0]), N)
    {
    }

    template <
        typename Container,
        typename = std::enable_if_t<
            !detail::is_span<Container>::value &&
            !detail::is_stl_array<Container>::value &&
            std::is_convertible<typename Container::pointer, pointer>::value &&
            std::is_convertible<
                typename Container::pointer,
                decltype(std::declval<Container>().data())>::value>>
    constexpr span(Container& c)
        : m_storage(c.data(), static_cast<index_type>(c.size()))
    {
    }

    template <
        typename Container,
        typename = std::enable_if_t<
            std::is_const<element_type>::value &&
            !detail::is_span<Container>::value &&
            std::is_convertible<typename Container::pointer, pointer>::value &&
            std::is_convertible<
                typename Container::pointer,
                decltype(std::declval<Container>().data())>::value>>
    constexpr span(const Container& c)
        : m_storage(c.data(), static_cast<index_type>(c.size()))
    {
    }

    constexpr span(const span& other) noexcept = default;
    constexpr span(span&& other) noexcept = default;
    template <
        typename OtherElementType,
        span_extent_type OtherExtent,
        typename = std::enable_if_t<
            detail::is_allowed_extent_conversion<OtherExtent, Extent>::value &&
            detail::is_allowed_element_type_conversion<OtherElementType,
                                                       element_type>::value>>
    constexpr span(const span<OtherElementType, OtherExtent>& other)
        : m_storage(other.data(), other.size())
    {
    }
    template <
        typename OtherElementType,
        span_extent_type OtherExtent,
        typename = std::enable_if_t<
            detail::is_allowed_extent_conversion<OtherExtent, Extent>::value &&
            detail::is_allowed_element_type_conversion<OtherElementType,
                                                       element_type>::value>>
    constexpr span(span<OtherElementType, OtherExtent>&& other)
        : m_storage(other.data(), other.size())
    {
    }
    constexpr span& operator=(const span& other) noexcept = default;
    constexpr span& operator=(span&& other) noexcept = default;
    ~span() noexcept = default;

    template <span_extent_type Count>
    constexpr span<element_type, Count> first() const
    {
        assert(Count >= 0 && Count <= size());
        return {data(), Count};
    }
    template <span_extent_type Count>
    constexpr span<element_type, Count> last() const
    {
        assert(Count >= 0 && Count <= size());
        return {_at_ptr(size() - Count), Count};
    }
    template <span_extent_type Offset, span_extent_type Count = dynamic_extent>
    constexpr span<element_type, Count> subspan() const
    {
        assert((Offset == 0 || (Offset > 0 && Offset < size())) &&
               (Count == dynamic_extent ||
                (Count >= 0 && Offset + Count <= size())));
#if SPIO_HAS_IF_CONSTEXPR
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

    constexpr span<element_type, dynamic_extent> first(index_type count) const
    {
        assert(count >= 0 && count <= size());
        return {data(), count};
    }
    constexpr span<element_type, dynamic_extent> last(index_type count) const
    {
        assert(count >= 0 && count <= size());
        return {_at_ptr(size() - count), count};
    }
    constexpr span<element_type, dynamic_extent> subspan(
        index_type offset,
        index_type count = dynamic_extent) const
    {
        assert((offset == 0 || (offset > 0 && offset < size())) &&
               (count == dynamic_extent ||
                (count >= 0 && offset + count <= size())));
        return {_at_ptr(offset),
                count == dynamic_extent ? size() - offset : count};
    }

    constexpr index_type length() const noexcept
    {
        return size();
    }
    constexpr index_type size() const noexcept
    {
        return m_storage.size();
    }
    constexpr index_type_us length_us() const noexcept
    {
        return size_us();
    }
    constexpr index_type_us size_us() const noexcept
    {
        assert(size() >= 0);
        return static_cast<index_type_us>(size());
    }
    constexpr index_type length_bytes() const noexcept
    {
        return size_bytes();
    }
    constexpr index_type size_bytes() const noexcept
    {
        return size() * static_cast<index_type>(sizeof(element_type));
    }
    constexpr index_type_us length_bytes_us() const noexcept
    {
        return size_bytes_us();
    }
    constexpr index_type_us size_bytes_us() const noexcept
    {
        assert(size_bytes() >= 0);
        return static_cast<index_type_us>(size_bytes());
    }
    constexpr bool empty() const noexcept
    {
        return size() == 0;
    }

    constexpr reference operator[](index_type idx) const noexcept
    {
        return _at(idx);
    }
    constexpr reference operator()(index_type idx) const noexcept
    {
        return _at(idx);
    }
    reference at(index_type idx) const
    {
        auto ptr = m_storage.at_checked(idx);
        if (!ptr) {
            SPIO_THROW(invalid_argument, "span::at: access out of range");
        }
        return *ptr;
    }
    constexpr pointer data() const noexcept
    {
        return m_storage.data();
    }

    iterator begin() const noexcept
    {
        return {*this, 0};
    }
    iterator end() const noexcept
    {
        return {*this, size()};
    }

    const_iterator cbegin() const noexcept
    {
        return {*this, 0};
    }
    const_iterator cend() const noexcept
    {
        return {*this, size()};
    }

    reverse_iterator rbegin() const noexcept
    {
        return reverse_iterator{end()};
    }
    reverse_iterator rend() const noexcept
    {
        return reverse_iterator{begin()};
    }

    const_reverse_iterator crbegin() const noexcept
    {
        return reverse_iterator{cend()};
    }
    const_reverse_iterator crend() const noexcept
    {
        return reverse_iterator{cbegin()};
    }

    constexpr span<std::add_const_t<ElementType>, Extent> as_const_span() const
        noexcept
    {
        return {const_cast<std::add_const_t<ElementType>*>(data()), size()};
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
};

#if SPIO_HAS_DEDUCTION_GUIDES
template <typename Container>
span(Container& c)->span<typename Container::value_type>;

template <typename Container>
span(const Container& c)->span<typename Container::value_type>;

template <typename ElementType, std::size_t N>
span(ElementType (&arr)[N])
    ->span<ElementType, static_cast<span_extent_type>(N)>;
#endif

template <typename Element,
          typename = std::enable_if_t<!std::is_const<Element>::value>>
constexpr auto make_span(Element* ptr, typename span<Element>::index_type count)
{
    return span<Element>(ptr, count);
}

template <typename Element>
constexpr auto make_span(const Element* ptr,
                         typename span<const Element>::index_type count)
{
    return span<const Element>(ptr, count);
}

template <typename Element,
          typename = std::enable_if_t<!std::is_const<Element>::value>>
constexpr auto make_span(Element* first, Element* last)
{
    return span<Element>(first, last);
}

template <typename Element>
constexpr auto make_span(const Element* first, const Element* last)
{
    return span<const Element>(first, last);
}

template <typename InputIt,
          typename = decltype(*std::declval<InputIt&>(),
                              ++std::declval<InputIt&>(),
                              void())>
constexpr auto make_span(InputIt first, InputIt last)
{
    return span<typename std::iterator_traits<InputIt>::value_type>(first,
                                                                    last);
}

template <typename Element,
          std::size_t N,
          typename = std::enable_if_t<N != 0 && !std::is_const<Element>::value>>
constexpr auto make_span(Element (&arr)[N])
{
    return span<Element, static_cast<span_extent_type>(N)>(arr);
}

template <typename Element, std::size_t N, typename = std::enable_if_t<N != 0>>
constexpr auto make_span(const Element (&arr)[N])
{
    return span<const Element, static_cast<span_extent_type>(N)>(arr);
}

template <typename Container,
          typename = std::enable_if_t<
              !std::is_const<typename Container::value_type>::value &&
              !std::is_const<Container>::value>>
constexpr auto make_span(Container& c)
{
    return span<typename Container::value_type>(&*c.begin(),
                                                &*(c.end() - 1) + 1);
}

template <typename Container>
constexpr auto make_span(const Container& c)
{
    return span<const typename Container::value_type>(&*c.begin(), &*c.end());
}

template <span_extent_type N,
          typename Element,
          typename = std::enable_if_t<!std::is_const<Element>::value>>
constexpr auto make_span(Element* ptr)
{
    return span<Element, N>(ptr, N);
}

template <span_extent_type N, typename Element>
constexpr auto make_span(const Element* ptr)
{
    return span<const Element, N>(ptr, N);
}

template <span_extent_type N,
          typename Container,
          typename = std::enable_if_t<
              !std::is_const<typename Container::value_type>::value &&
              !std::is_const<Container>::value>>
constexpr auto make_span(Container& c)
{
    assert(stl::distance(c.begin(), c.end()) >= N);
    return span<typename Container::value_type, N>(&*c.begin(), N);
}

template <span_extent_type N, typename Container>
constexpr auto make_span(const Container& c)
{
    assert(stl::distance(c.begin(), c.end()) >= N);
    return span<const typename Container::value_type, N>(&*c.begin(), N);
}

namespace detail {
#if SPIO_HAS_BYTE
    using span_as_bytes_type = std::byte;
#else
    using span_as_bytes_type = unsigned char;
#endif
}  // namespace detail

#ifdef _MSC_VER
template <typename ElementType, span_extent_type N>
inline constexpr span<const detail::span_as_bytes_type> as_bytes(
    span<ElementType, N> s) noexcept
{
    return {reinterpret_cast<const detail::span_as_bytes_type*>(s.data()),
            static_cast<span_extent_type>(sizeof(ElementType)) * s.size()};
}
template <typename ElementType,
          span_extent_type N,
          typename = std::enable_if_t<!std::is_const<ElementType>::value>>
inline constexpr span<detail::span_as_bytes_type> as_writable_bytes(
    span<ElementType, N> s) noexcept
{
    return {reinterpret_cast<detail::span_as_bytes_type*>(s.data()),
            static_cast<span_extent_type>(sizeof(ElementType)) * s.size()};
}

template <typename ElementFrom,
          typename ElementTo,
          span_extent_type ExtentFrom,
          span_extent_type ExtentTo>
void copy_contiguous(span<ElementFrom, ExtentFrom> from,
                     span<ElementTo, ExtentTo> to)
{
    assert(from.size_bytes() <= to.size_bytes());

#if SPIO_HAS_IF_CONSTEXPR
    if constexpr (sizeof(ElementFrom) == sizeof(ElementTo))
#else
    if (sizeof(ElementFrom) == sizeof(ElementTo))
#endif
    {
        stl::copy(from.begin(), from.end(), to.begin());
        return;
    }
    else {
        auto from_bytes = as_bytes(from);
        auto to_bytes = as_writable_bytes(to);
        stl::copy(from_bytes.begin(), from_bytes.end(), to_bytes.begin());
    }
}

#else
template <typename ElementType, span_extent_type Extent>
inline constexpr span<const detail::span_as_bytes_type,
                      (Extent == dynamic_extent)
                          ? dynamic_extent
                          : (span_extent_type{sizeof(ElementType)} * Extent)>
as_bytes(span<ElementType, Extent> s) noexcept
{
    return {reinterpret_cast<const detail::span_as_bytes_type*>(s.data()),
            span_extent_type{sizeof(ElementType)} * s.size()};
}
template <typename ElementType,
          span_extent_type Extent,
          typename = std::enable_if_t<!std::is_const<ElementType>::value>>
inline constexpr span<detail::span_as_bytes_type,
                      (Extent == dynamic_extent)
                          ? dynamic_extent
                          : (span_extent_type{sizeof(ElementType)} * Extent)>
as_writable_bytes(span<ElementType, Extent> s) noexcept
{
    return {reinterpret_cast<detail::span_as_bytes_type*>(s.data()),
            span_extent_type{sizeof(ElementType)} * s.size()};
}

template <typename ElementFrom,
          typename ElementTo,
          span_extent_type ExtentFrom,
          span_extent_type ExtentTo,
          typename = std::enable_if_t<
              (ExtentFrom == dynamic_extent || ExtentTo == dynamic_extent) ||
              sizeof(ElementFrom) * ExtentFrom == sizeof(ElementTo) * ExtentTo>>
void copy_contiguous(span<ElementFrom, ExtentFrom> from,
                     span<ElementTo, ExtentTo> to)
{
    assert(from.size_bytes() <= to.size_bytes());

#if SPIO_HAS_IF_CONSTEXPR
    if constexpr (sizeof(ElementFrom) == sizeof(ElementTo))
#else
    if (sizeof(ElementFrom) == sizeof(ElementTo))
#endif
    {
        stl::copy(from.begin(), from.end(), to.begin());
        return;
    }
    else {
        auto from_bytes = as_bytes(from);
        auto to_bytes = as_writable_bytes(to);
        stl::copy(from_bytes.begin(), from_bytes.end(), to_bytes.begin());
    }
}
#endif

namespace stl {
    template <typename T, span_extent_type N>
    constexpr std::ptrdiff_t strlen(span<T, N> str) noexcept
    {
        auto it = stl::find(str.begin(), str.end(), static_cast<T>('\0'));
        if (it == str.end()) {
            return str.size();
        }
        return stl::distance(str.begin(), it);
    }
}  // namespace stl
}  // namespace io

namespace std {
template <typename Element, io::span_extent_type Extent>
inline constexpr Element& at(const io::span<Element, Extent>& s,
                             io::span_extent_type index)
{
    return s[index];
}
}  // namespace std

#endif  // SPIO_SPAN_H
