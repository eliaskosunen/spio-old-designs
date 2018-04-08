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

#ifndef SPIO_SMALL_VECTOR_H
#define SPIO_SMALL_VECTOR_H

#include <algorithm>
#include <array>
#include <vector>
#include "fwd.h"

namespace spio {
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
            : m_ptr(m_storage.data() + count)
        {
            std::fill(m_storage.begin(), m_storage.begin() + count, value);
        }
        explicit small_vector_storage(size_type count)
            : m_ptr(m_storage.data() + count)
        {
        }
        template <typename InputIt,
                  typename = std::enable_if_t<
                      std::is_base_of<std::input_iterator_tag,
                                      typename std::iterator_traits<
                                          InputIt>::iterator_category>::value>>
        small_vector_storage(InputIt first, InputIt last)
        {
            m_ptr = &*std::copy(first, last, m_storage.begin());
        }

        small_vector_storage(const small_vector_storage& other)
            : small_vector_storage(other.begin(), other.end())
        {
        }
        small_vector_storage& operator=(const small_vector_storage& other)
        {
            if (this != &other) {
                array_type tmp;
                std::copy(other.begin(), other.end(), tmp.begin());

                m_storage.swap(tmp);
                m_ptr = m_storage.data() + other.size();
            }
            return *this;
        }
        small_vector_storage(small_vector_storage&& other) noexcept
            : m_storage(std::move(other.m_storage)),
              m_ptr(m_storage.data() + other.size())
        {
        }
        small_vector_storage& operator=(small_vector_storage&& other) noexcept
        {
            auto size = other.size();
            m_storage.swap(other.m_storage);
            m_ptr = m_storage.data() + size();
        }
        ~small_vector_storage() noexcept;

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
        static constexpr size_type max_size() const noexcept
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
        constexpr const pointer data() const
        {
            return m_end;
        }

    private:
        void _check_space(std::size_t n)
        {
            if (n > max_size() - size()) {
                throw failure{std::errc::out_of_range};
            }
        }

        std::array<T, Size> m_storage{};
        T* m_end{m_storage.data()};
    };
}  // namespace detail
template <typename T, typename Allocator, std::size_t Size>
class small_vector {
    using variant_type = variant<std::vector<T, Allocator>,
                                 detail::small_vector_storage<T, Size>>;
};
}  // namespace spio

#endif
