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

#ifndef SPIO_STREAM_REF_H
#define SPIO_STREAM_REF_H

#include "fwd.h"
#include "stream.h"

namespace spio {
template <typename CharT,
          typename Formatter,
          typename Scanner,
          typename SinkBuffer,
          typename SourceBuffer,
          typename Traits>
class basic_stream_ref {
    struct stream {
        stream_base* base;
        detail::basic_input_stream_base<CharT>* source;
        detail::basic_output_stream_base<CharT>* sink;
        detail::seekable_stream_base* seek;
        detail::closable_stream_base* close;
        detail::flushable_stream_base* flush;
        detail::localizable_stream_base* locale;
    };

public:
    template <typename Category>
    using basic_stream_type = basic_stream<CharT,
                                           Category,
                                           Formatter,
                                           Scanner,
                                           SinkBuffer,
                                           SourceBuffer,
                                           Traits>;

    template <typename Category>
    basic_stream_ref(basic_stream_type<Category>& s) : m_stream(_init_stream(s))
    {
    }

    stream& operator*()
    {
        return m_stream;
    }
    const stream& operator*() const
    {
        return m_stream;
    }

    stream* operator->()
    {
        return &m_stream;
    }
    const stream* operator->() const
    {
        return &m_stream;
    }

    stream& get()
    {
        return operator*();
    }
    const stream& get() const
    {
        return operator*();
    }

private:
    template <typename Category, typename = void>
    struct _init_source;

    template <typename Category>
    struct _init_source<
        Category,
        std::enable_if_t<!is_category<Category, input>::value>> {
        static auto get(basic_stream_type<Category>&)
        {
            return nullptr;
        }
    };
    template <typename Category>
    struct _init_source<Category,
                        std::enable_if_t<is_category<Category, input>::value>> {
        static detail::basic_input_stream_base<CharT>* get(
            basic_stream_type<Category>& s)
        {
            return std::addressof(s);
        }
    };

    template <typename Category, typename = void>
    struct _init_sink;

    template <typename Category>
    struct _init_sink<Category,
                      std::enable_if_t<!is_category<Category, output>::value>> {
        static auto get(basic_stream_type<Category>&)
        {
            return nullptr;
        }
    };
    template <typename Category>
    struct _init_sink<Category,
                      std::enable_if_t<is_category<Category, output>::value>> {
        static detail::basic_output_stream_base<CharT>* get(
            basic_stream_type<Category>& s)
        {
            return std::addressof(s);
        }
    };

    template <typename Category, typename = void>
    struct _init_seek;

    template <typename Category>
    struct _init_seek<
        Category,
        std::enable_if_t<
            !is_category<Category, detail::random_access>::value>> {
        static auto get(basic_stream_type<Category>&)
        {
            return nullptr;
        }
    };
    template <typename Category>
    struct _init_seek<
        Category,
        std::enable_if_t<is_category<Category, detail::random_access>::value>> {
        static detail::seekable_stream_base* get(basic_stream_type<Category>& s)
        {
            return std::addressof(s);
        }
    };

    template <typename Category, typename = void>
    struct _init_close;

    template <typename Category>
    struct _init_close<
        Category,
        std::enable_if_t<!is_category<Category, closable_tag>::value>> {
        static auto get(basic_stream_type<Category>&)
        {
            return nullptr;
        }
    };
    template <typename Category>
    struct _init_close<
        Category,
        std::enable_if_t<is_category<Category, closable_tag>::value>> {
        static detail::closable_stream_base* get(basic_stream_type<Category>& s)
        {
            return std::addressof(s);
        }
    };

    template <typename Category, typename = void>
    struct _init_flush;

    template <typename Category>
    struct _init_flush<
        Category,
        std::enable_if_t<!is_category<Category, flushable_tag>::value>> {
        static auto get(basic_stream_type<Category>&)
        {
            return nullptr;
        }
    };
    template <typename Category>
    struct _init_flush<
        Category,
        std::enable_if_t<is_category<Category, flushable_tag>::value>> {
        static detail::flushable_stream_base* get(
            basic_stream_type<Category>& s)
        {
            return std::addressof(s);
        }
    };

    template <typename Category, typename = void>
    struct _init_locale;

    template <typename Category>
    struct _init_locale<
        Category,
        std::enable_if_t<!is_category<Category, localizable_tag>::value>> {
        static auto get(basic_stream_type<Category>&)
        {
            return nullptr;
        }
    };
    template <typename Category>
    struct _init_locale<
        Category,
        std::enable_if_t<is_category<Category, localizable_tag>::value>> {
        static detail::localizable_stream_base* get(
            basic_stream_type<Category>& s)
        {
            return std::addressof(s);
        }
    };

    template <typename Category>
    static stream _init_stream(basic_stream_type<Category>& s)
    {
        return stream{std::addressof(s),
                      _init_source<Category>::get(s),
                      _init_sink<Category>::get(s),
                      _init_seek<Category>::get(s),
                      _init_close<Category>::get(s),
                      _init_flush<Category>::get(s),
                      _init_locale<Category>::get(s)};
    }

    stream m_stream;
};
}  // namespace spio

#endif
