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

#include "config.h"
#include "error.h"
#include "stl.h"

namespace io {
constexpr std::size_t dynamic_extent = std::numeric_limits<std::size_t>::max();

namespace detail {
    template <std::size_t From, std::size_t To>
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

    template <typename Span, bool IsConst>
    class span_iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = std::remove_cv_t<typename Span::element_type>;
        using difference_type = typename Span::index_type;

        using reference =
            std::conditional_t<IsConst, const value_type, value_type>&;
        using pointer = std::add_pointer_t<reference>;

        span_iterator() = default;

        constexpr span_iterator(const Span* s, difference_type i) noexcept
            : m_span(s), m_index(i)
        {
            assert((s == nullptr) || (i >= 0 && i <= s->length()));
        }

        friend span_iterator<Span, true>;
        template <bool B, std::enable_if_t<!B && IsConst>* = nullptr>
        constexpr span_iterator(const span_iterator<Span, B>& o) noexcept
            : span_iterator(o.m_span, o.m_index)
        {
        }

        constexpr reference operator*() const noexcept
        {
            assert(m_span);
            return (*m_span)[m_index];
        }
        constexpr pointer operator->() const noexcept
        {
            assert(m_span && m_index >= 0 && m_index <= m_span->length());
            return m_span->data() + m_index;
        }

        constexpr span_iterator& operator++() noexcept
        {
            assert(m_span && m_index >= 0 && m_index < m_span->length());
            ++m_index;
            return *this;
        }
        constexpr span_iterator& operator--() noexcept
        {
            assert(m_span && m_index > 0 && m_index <= m_span->length());
            --m_index;
            return *this;
        }

        constexpr span_iterator operator++(int)noexcept
        {
            auto ret = *this;
            ++(*this);
            return ret;
        }
        constexpr span_iterator operator--(int)noexcept
        {
            auto ret = *this;
            --(*this);
            return ret;
        }

        constexpr span_iterator operator+(difference_type n) const noexcept
        {
            auto ret = *this;
            return ret += n;
        }
        constexpr span_iterator operator-(difference_type n) const noexcept
        {
            auto ret = *this;
            return ret -= n;
        }
        constexpr span_iterator& operator+=(difference_type n) noexcept
        {
            assert(m_span && (m_index + n) >= 0 &&
                   (m_index + n) <= m_span->length());
            m_index += n;
            return *this;
        }
        constexpr span_iterator& operator-=(difference_type n) noexcept
        {
            return *this += -n;
        }

        constexpr difference_type operator-(const span_iterator& r) const
            noexcept
        {
            assert(m_span->data() == r.m_span->data());
            return m_index - r.m_index;
        }

        constexpr reference operator[](difference_type n) const noexcept
        {
            return *(*this + n);
        }

        constexpr const Span* get_span() const noexcept
        {
            return m_span;
        }
        constexpr difference_type get_index() const noexcept
        {
            return m_index;
        }

    private:
        const Span* m_span{nullptr};
        difference_type m_index = 0;
    };

    template <typename Span, bool IsConstL, bool IsConstR>
    constexpr bool operator==(const span_iterator<Span, IsConstL>& l,
                              const span_iterator<Span, IsConstR>& r) noexcept
    {
        return l.get_span()->data() == r.get_span()->data() &&
               l.get_index() == r.get_index();
    }
    template <typename Span, bool IsConstL, bool IsConstR>
    constexpr bool operator!=(const span_iterator<Span, IsConstL>& l,
                              const span_iterator<Span, IsConstR>& r) noexcept
    {
        return !(l == r);
    }
    template <typename Span, bool IsConstL, bool IsConstR>
    constexpr bool operator<(const span_iterator<Span, IsConstL>& l,
                             const span_iterator<Span, IsConstR>& r) noexcept
    {
        assert(l.get_span()->data() == r.get_span()->data());
        return l.get_index() < r.get_index();
    }
    template <typename Span, bool IsConstL, bool IsConstR>
    constexpr bool operator<=(const span_iterator<Span, IsConstL>& l,
                              const span_iterator<Span, IsConstR>& r) noexcept
    {
        return !(r < l);
    }
    template <typename Span, bool IsConstL, bool IsConstR>
    constexpr bool operator>(const span_iterator<Span, IsConstL>& l,
                             const span_iterator<Span, IsConstR>& r) noexcept
    {
        return r < l;
    }
    template <typename Span, bool IsConstL, bool IsConstR>
    constexpr bool operator>=(const span_iterator<Span, IsConstL>& l,
                              const span_iterator<Span, IsConstR>& r) noexcept
    {
        return !(r > l);
    }

    template <std::size_t Extent>
    class extent_type {
    public:
        using index_type = std::size_t;

        static_assert(Extent >= 0, "Fixed-size spans must be >= 0 in size");

        constexpr extent_type() noexcept {}

        template <index_type Other>
        constexpr extent_type(extent_type<Other> ext)
        {
            static_assert(Other == Extent || Other == dynamic_extent,
                          "Mismatch between fixed-size extent and size of "
                          "initializing data");
            assert(ext.size() == Extent);
            SPIO_UNUSED(ext);
        }

        constexpr extent_type(index_type size)
        {
            assert(size == Extent);
            SPIO_UNUSED(size);
        }

        constexpr index_type size() const noexcept
        {
            return Extent;
        }
    };

    template <>
    class extent_type<dynamic_extent> {
    public:
        using index_type = std::size_t;

        template <index_type Other>
        explicit constexpr extent_type(extent_type<Other> ext)
            : m_size(ext.size())
        {
        }

        explicit constexpr extent_type(index_type size) : m_size(size) {}

        constexpr index_type size() const noexcept
        {
            return m_size;
        }

    private:
        index_type m_size;
    };
}  // namespace detail

template <typename ElementType, std::size_t Extent = dynamic_extent>
class span {
public:
    using element_type = ElementType;
    using index_type = std::size_t;
    using pointer = element_type*;
    using reference = element_type&;

    using iterator = detail::span_iterator<span<ElementType, Extent>, false>;
    using const_iterator =
        detail::span_iterator<span<ElementType, Extent>, true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static_assert(Extent == dynamic_extent || Extent >= 0,
                  "Extent must be >= 0 or dynamic_extent");

    constexpr static index_type extent = Extent;

    template <bool Dependent = false,
              typename = std::enable_if_t<(Dependent || Extent <= 0)>>
    constexpr span() noexcept : m_storage(nullptr, detail::extent_type<0>())
    {
    }
    constexpr span(std::nullptr_t) noexcept : span() {}
    constexpr span(pointer p, index_type count) : m_storage(p, count) {}
    constexpr span(pointer first, pointer last)
        : m_storage(first, distance_nonneg(first, last))
    {
    }
    template <std::size_t N, typename = std::enable_if_t<N != 0>>
    constexpr span(element_type (&arr)[N])
        : m_storage(&arr[0], detail::extent_type<static_cast<index_type>(N)>())
    {
    }

    template <
        typename OtherElementType,
        std::size_t OtherExtent,
        typename = std::enable_if_t<
            detail::is_allowed_extent_conversion<OtherExtent, Extent>::value &&
            detail::is_allowed_element_type_conversion<OtherElementType,
                                                       element_type>::value>>
    constexpr span(const span<OtherElementType, OtherExtent>& o)
        : m_storage(o.data(), detail::extent_type<OtherExtent>(o.size()))
    {
    }
    template <
        typename OtherElementType,
        std::size_t OtherExtent,
        typename = std::enable_if_t<
            detail::is_allowed_extent_conversion<OtherExtent, Extent>::value &&
            detail::is_allowed_element_type_conversion<OtherElementType,
                                                       element_type>::value>>
    constexpr span(span<OtherElementType, OtherExtent>&& o)
        : m_storage(o.data(), detail::extent_type<OtherExtent>(o.size()))
    {
    }

    constexpr span(const span& o) noexcept = default;
    constexpr span(span&& o) noexcept = default;
    constexpr span& operator=(const span& o) noexcept = default;
    constexpr span& operator=(span&& o) noexcept = default;
    ~span() noexcept = default;

    template <index_type Count>
    constexpr span<element_type, Count> first() const
    {
        assert(Count >= 0 && Count <= size());
        return {data(), Count};
    }
    template <index_type Count>
    constexpr span<element_type, Count> last() const
    {
        assert(Count >= 0 && Count <= size());
        return {data() + (size() - Count), Count};
    }
    template <index_type Offset, index_type Count = dynamic_extent>
    constexpr span<element_type, Count> subspan() const
    {
        assert((Offset == 0 || (Offset > 0 && Offset <= size())) &&
               (Count == dynamic_extent ||
                (Count >= 0 && Offset + Count <= size())));
        return {data() + Offset,
                Count == dynamic_extent ? size() - Offset : Count};
    }

    constexpr span<element_type, dynamic_extent> first(index_type count) const
    {
        assert(count >= 0 && count <= size());
        return {data(), count};
    }
    constexpr span<element_type, dynamic_extent> last(index_type count) const
    {
        assert(count >= 0 && count <= size());
        return {data() + (size() - count), count};
    }
    constexpr span<element_type, dynamic_extent> subspan(
        index_type offset,
        index_type count = dynamic_extent) const
    {
        assert((offset == 0 || (offset > 0 && offset <= size())) &&
               (count == dynamic_extent ||
                (count >= 0 && offset + count <= size())));
        return {data() + offset,
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
    constexpr index_type length_bytes() const noexcept
    {
        return size_bytes();
    }
    constexpr index_type size_bytes() const noexcept
    {
        return size() * static_cast<index_type>(sizeof(element_type));
    }
    constexpr bool empty() const noexcept
    {
        return size() == 0;
    }

    constexpr reference at(index_type idx) const
    {
        assert(idx >= 0 && idx < m_storage.size());
        return data()[idx];
    }
    constexpr reference operator[](index_type idx) const
    {
        return at(idx);
    }
    constexpr reference operator()(index_type idx) const
    {
        return at(idx);
    }
    constexpr pointer data() const noexcept
    {
        return m_storage.data();
    }

    iterator begin() const noexcept
    {
        return {this, 0};
    }
    iterator end() const noexcept
    {
        return {this, size()};
    }

    const_iterator cbegin() const noexcept
    {
        return {this, 0};
    }
    const_iterator cend() const noexcept
    {
        return {this, size()};
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

private:
    template <typename ExtentType>
    class storage_type : public ExtentType {
    public:
        template <typename OtherExtentType>
        constexpr storage_type(pointer data, OtherExtentType ext)
            : ExtentType(ext), m_data(data)
        {
            assert((!data && ExtentType::size() == 0) || data);
        }

        constexpr pointer data() const noexcept
        {
            return m_data;
        }

    private:
        pointer m_data;
    };

    storage_type<detail::extent_type<Extent>> m_storage;
};

template <typename ElementType, std::size_t Extent>
constexpr bool operator==(const span<ElementType, Extent>& l,
                          const span<ElementType, Extent>& r) noexcept
{
    return equal(l.begin(), l.end(), r.begin(), r.end());
}
template <typename ElementType, std::size_t Extent>
constexpr bool operator!=(const span<ElementType, Extent>& l,
                          const span<ElementType, Extent>& r) noexcept
{
    return !(l == r);
}
template <typename ElementType, std::size_t Extent>
constexpr bool operator<(const span<ElementType, Extent>& l,
                         const span<ElementType, Extent>& r) noexcept
{
    return lexicographical_compare(l.begin(), l.end(), r.begin(), r.end());
}
template <typename ElementType, std::size_t Extent>
constexpr bool operator<=(const span<ElementType, Extent>& l,
                          const span<ElementType, Extent>& r) noexcept
{
    return !(l > r);
}
template <typename ElementType, std::size_t Extent>
constexpr bool operator>(const span<ElementType, Extent>& l,
                         const span<ElementType, Extent>& r) noexcept
{
    return r < l;
}
template <typename ElementType, std::size_t Extent>
constexpr bool operator>=(const span<ElementType, Extent>& l,
                          const span<ElementType, Extent>& r) noexcept
{
    return !(l < r);
}

template <typename Element>
constexpr auto make_span(Element* ptr, typename span<Element>::index_type count)
{
    return span<Element>(ptr, count);
}

template <typename Element>
constexpr auto make_span(Element* first, Element* last)
{
    return span<Element>(first, last);
}

template <typename Element, std::size_t N, typename = std::enable_if_t<N != 0>>
constexpr auto make_span(Element (&arr)[N])
{
    return span<Element, static_cast<std::size_t>(N)>(arr);
}

template <typename Container>
constexpr auto make_span(Container& c)
{
    return span<typename Container::value_type>(&*c.begin(), &*c.end());
}

template <typename Container>
constexpr auto make_span(const Container& c)
{
    return span<const typename Container::value_type>(&*c.begin(), &*c.end());
}

namespace detail {
    template <typename Element, std::size_t Extent>
    auto copy_contiguous_storage(const span<Element, Extent>& s)
    {
        if constexpr (Extent == dynamic_extent) {
            return vector<uint8_t>(sizeof(Element) * s.size(), 0);
        }
        else {
            return array<uint8_t, sizeof(Element) * Extent>{{0}};
        }
    }
}  // namespace detail

template <typename ElementFrom,
          typename ElementTo,
          std::size_t ExtentFrom,
          std::size_t ExtentTo,
          typename = std::enable_if_t<
              (ExtentFrom == dynamic_extent || ExtentTo == dynamic_extent) ||
              sizeof(ElementFrom) * ExtentFrom == sizeof(ElementTo) * ExtentTo>>
void copy_contiguous(const span<ElementFrom, ExtentFrom>& from,
                     span<ElementTo, ExtentTo>& to)
{
    assert(sizeof(ElementFrom) * from.size() <= sizeof(ElementTo) * to.size());

    if constexpr (sizeof(ElementFrom) == sizeof(ElementTo)) {
        copy(from.begin(), from.end(), to.begin());
        return;
    }

    auto bytes = detail::copy_contiguous_storage(from);
    copy(reinterpret_cast<uint8_t*>(&from[0]),
         (reinterpret_cast<uint8_t*>(&from[0])) + bytes.size(), bytes.begin());

    copy(reinterpret_cast<ElementTo*>(&*bytes.begin()),
         reinterpret_cast<ElementTo*>(&*bytes.end()), to.begin());
    /* copy(to.begin(), to.end(), reinterpret_cast<uint8_t*>(&bytes[0]));
     */
}
}  // namespace io

namespace std {
template <typename Element, std::size_t Extent>
inline constexpr Element& at(const io::span<Element, Extent>& s,
                             std::size_t index)
{
    return s[index];
}
}  // namespace std

#endif  // SPIO_SPAN_H
