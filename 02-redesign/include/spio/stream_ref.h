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

#include "error.h"
#include "fwd.h"
#include "stream.h"

namespace spio {
namespace detail {
    template <typename CharT,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits>
    struct erased_stream_storage_base {
        virtual void read(span<CharT>) = 0;
        virtual void write(span<const CharT>) = 0;
        virtual streampos seek(streamoff, seekdir, int) = 0;
        virtual void close() = 0;
        virtual void flush() = 0;
#if SPIO_USE_LOCALE
        virtual void imbue(const std::locale&) = 0;
        virtual const std::locale& get_locale() const = 0;
#endif

        virtual Formatter& get_formatter() = 0;
        virtual Scanner& get_scanner() = 0;

        virtual SinkBuffer& get_sink_buffer() = 0;
        virtual SourceBuffer& get_source_buffer() = 0;

        virtual ~erased_stream_storage_base() = default;
    };

    template <typename Category, typename = void>
    struct do_read {
        template <typename Stream>
        [[noreturn]] static void read(Stream&, span<typename Stream::char_type>)
        {
            SPIO_UNREACHABLE;
        }
        template <typename Stream>
        [[noreturn]] static typename Stream::scanner_type& get_scanner(Stream&)
        {
            SPIO_UNREACHABLE;
        }
    };
    template <typename Category>
    struct do_read<Category,
                   std::enable_if_t<is_category<Category, input>::value>> {
        template <typename Stream>
        static void read(Stream& s, span<typename Stream::char_type> data)
        {
            s.read(data);
        }
        template <typename Stream>
        static typename Stream::scanner_type& get_scanner(Stream& s)
        {
            return s.get_scanner();
        }
    };

    template <typename Category, typename = void>
    struct do_write {
        template <typename Stream>
        [[noreturn]] static void write(Stream&,
                                       span<const typename Stream::char_type>)
        {
            SPIO_UNREACHABLE;
        }
        template <typename Stream>
        [[noreturn]] static typename Stream::formatter_type& get_formatter(
            Stream&)
        {
            SPIO_UNREACHABLE;
        }
    };
    template <typename Category>
    struct do_write<Category,
                    std::enable_if_t<is_category<Category, output>::value>> {
        template <typename Stream>
        static void write(Stream& s,
                          span<const typename Stream::char_type> data)
        {
            s.write(data);
        }
        template <typename Stream>
        static typename Stream::formatter_type& get_formatter(Stream& s)
        {
            return s.get_formatter();
        }
    };

    template <typename Category, typename = void>
    struct do_seek {
        template <typename Stream>
        [[noreturn]] static streampos seek(Stream&, streamoff, seekdir, int)
        {
            SPIO_UNREACHABLE;
        }
    };
    template <typename Category>
    struct do_seek<
        Category,
        std::enable_if_t<is_category<Category, detail::random_access>::value>> {
        template <typename Stream>
        static auto seek(Stream& s, streamoff off, seekdir dir, int which)
        {
            return s.seek(off, dir, which);
        }
    };

    template <typename Category, typename = void>
    struct do_close {
        template <typename Stream>
        [[noreturn]] static void close(Stream&)
        {
            SPIO_UNREACHABLE;
        }
    };
    template <typename Category>
    struct do_close<
        Category,
        std::enable_if_t<is_category<Category, closable_tag>::value>> {
        template <typename Stream>
        static void close(Stream& s)
        {
            s.close();
        }
    };

    template <typename Category, typename = void>
    struct do_flush {
        template <typename Stream>
        [[noreturn]] static void flush(Stream&)
        {
            SPIO_UNREACHABLE;
        }
    };
    template <typename Category>
    struct do_flush<
        Category,
        std::enable_if_t<is_category<Category, flushable_tag>::value>> {
        template <typename Stream>
        static void flush(Stream& s)
        {
            s.flush();
        }
    };

#if SPIO_USE_LOCALE
    template <typename Category, typename = void>
    struct do_locale {
        template <typename Stream>
        [[noreturn]] static void imbue(Stream&, const std::locale&)
        {
            SPIO_UNREACHABLE;
        }

        template <typename Stream>
        [[noreturn]] static const std::locale& get_locale(const Stream&)
        {
            SPIO_UNREACHABLE;
        }
    };
    template <typename Category>
    struct do_locale<
        Category,
        std::enable_if_t<is_category<Category, localizable_tag>::value>> {
        template <typename Stream>
        static void imbue(Stream& s, const std::locale& l)
        {
            s.imbue(l);
        }

        template <typename Stream>
        static const std::locale& get_locale(const Stream& s)
        {
            return s.get_locale();
        }
    };
#endif

    template <typename Category, typename = void>
    struct do_sourcebuffer {
        template <typename Stream>
        [[noreturn]] static typename Stream::source_buffer_type&
        get_source_buffer(Stream&)
        {
            SPIO_UNREACHABLE;
        }
    };
    template <typename Category>
    struct do_sourcebuffer<
        Category,
        std::enable_if_t<is_category<Category, input>::value &&
                         !is_category<Category, nobuffer_tag>::value>> {
        template <typename Stream>
        static typename Stream::source_buffer_type& get_source_buffer(Stream& s)
        {
            return s.get_source_buffer();
        }
    };
    template <typename Category, typename = void>
    struct do_sinkbuffer {
        template <typename Stream>
        [[noreturn]] static typename Stream::sink_buffer_type& get_sink_buffer(
            Stream&)
        {
            SPIO_UNREACHABLE;
        }
    };
    template <typename Category>
    struct do_sinkbuffer<
        Category,
        std::enable_if_t<is_category<Category, output>::value &&
                         !is_category<Category, nobuffer_tag>::value>> {
        template <typename Stream>
        static typename Stream::sink_buffer_type& get_sink_buffer(Stream& s)
        {
            return s.get_sink_buffer();
        }
    };

    template <typename Stream>
    class erased_stream_storage
        : public erased_stream_storage_base<typename Stream::char_type,
                                            typename Stream::formatter_type,
                                            typename Stream::scanner_type,
                                            typename Stream::sink_buffer_type,
                                            typename Stream::source_buffer_type,
                                            typename Stream::traits> {
    public:
        using stream_type = Stream;
        using char_type = typename stream_type::char_type;
        using category = typename stream_type::category;

        erased_stream_storage(stream_type& s) : m_stream(std::addressof(s)) {}

        void read(span<char_type> s) override
        {
            return do_read<category>::read(*m_stream, s);
        }
        void write(span<const char_type> s) override
        {
            return do_write<category>::write(*m_stream, s);
        }

        streampos seek(streamoff off, seekdir dir, int which) override
        {
            return do_seek<category>::seek(*m_stream, off, dir, which);
        }

        void close() override
        {
            do_close<category>::close(*m_stream);
        }
        void flush() override
        {
            do_flush<category>::flush(*m_stream);
        }

#if SPIO_USE_LOCALE
        void imbue(const std::locale& l) override
        {
            do_locale<category>::imbue(*m_stream, l);
        }
        const std::locale& get_locale() const override
        {
            return do_locale<category>::get_locale(*m_stream);
        }
#endif

        stream_type& get_stream()
        {
            return *m_stream;
        }
        const stream_type& get_stream() const
        {
            return *m_stream;
        }

        typename Stream::formatter_type& get_formatter() override
        {
            return do_write<category>::get_formatter(*m_stream);
        }
        typename Stream::scanner_type& get_scanner() override
        {
            return do_read<category>::get_scanner(*m_stream);
        }

        typename Stream::source_buffer_type& get_source_buffer() override
        {
            return do_sourcebuffer<category>::get_source_buffer(*m_stream);
        }
        typename Stream::sink_buffer_type& get_sink_buffer() override
        {
            return do_sinkbuffer<category>::get_sink_buffer(*m_stream);
        }

    private:
        stream_type* m_stream;
    };

    template <typename T, std::size_t N, typename... Args>
    struct count_tags_impl;
    template <typename T, std::size_t N, typename Tag, typename... Ts>
    struct count_tags_impl<T, N, Tag, Ts...> {
        template <typename U = T>
        static constexpr auto value()
            -> std::enable_if_t<std::is_base_of<Tag, U>::value, std::size_t>
        {
            return count_tags_impl<T, N + 1, Ts...>::value();
        }
        template <typename U = T>
        static constexpr auto value()
            -> std::enable_if_t<!std::is_base_of<Tag, U>::value, std::size_t>
        {
            return count_tags_impl<T, N, Ts...>::value();
        }
    };
    template <typename T, std::size_t N>
    struct count_tags_impl<T, N> {
        static constexpr std::size_t value()
        {
            static_assert(N != 100, "foo");
            return N;
        }
    };
    template <typename T, typename... Tags>
    struct count_tags
        : std::integral_constant<std::size_t,
                                 count_tags_impl<T, 0, Tags...>::value()> {
    };

    template <typename T>
    struct count_type_tags
        : std::integral_constant<std::size_t,
                                 count_tags<T, input, output>::value> {
    };
    template <typename T>
    struct count_mode_tags
        : std::integral_constant<std::size_t,
                                 count_tags<T,
                                            detail::random_access,
                                            asynchronized_tag,
                                            closable_tag,
                                            flushable_tag,
                                            localizable_tag,
                                            revertible_tag>::value> {
    };

    template <typename Source, typename Dest, typename = void>
    struct is_allowed_category_conversion : std::false_type {
    };
    template <typename Source, typename Dest>
    struct is_allowed_category_conversion<
        Source,
        Dest,
        std::enable_if_t<
            (count_type_tags<Source>::value >= count_type_tags<Dest>::value) &&
            (count_mode_tags<Source>::value >= count_mode_tags<Dest>::value)>>
        : std::true_type {
    };

    template <typename CharT,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits>
    class basic_erased_stream {
        template <typename Device>
        using stream_type = basic_stream<Device,
                                         Formatter,
                                         Scanner,
                                         SinkBuffer,
                                         SourceBuffer,
                                         Traits>;

        template <typename T>
        static void _delete(void* ptr)
        {
            return static_cast<T*>(ptr)->~T();
        }

    public:
        using base_type = detail::erased_stream_storage_base<CharT,
                                                             Formatter,
                                                             Scanner,
                                                             SinkBuffer,
                                                             SourceBuffer,
                                                             Traits>;
        using storage = typename std::aligned_storage<4 * sizeof(void*),
                                                      alignof(void*)>::type;
        using pointer = std::unique_ptr<base_type, void (*)(void*)>;

        basic_erased_stream() = default;
        basic_erased_stream(pointer p) : m_ptr(std::move(p)) {}
        template <typename Device,
                  typename = std::enable_if_t<std::is_same<
                      CharT,
                      typename stream_type<Device>::char_type>::value>,
                  typename T = erased_stream_storage<stream_type<Device>>>
        basic_erased_stream(stream_type<Device>& s)
            : m_ptr(new (&m_data) T(s), &_delete<T>)
        {
        }

        auto& get()
        {
            SPIO_ASSERT(valid(), "erased_stream::get: invalid stream");
            return get_pointer();
        }
        const auto& get() const
        {
            SPIO_ASSERT(valid(), "erased_stream::get: invalid stream");
            return get_pointer();
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
            return get_pointer();
        }
        const auto* operator-> () const
        {
            return get_pointer();
        }

        bool valid() const
        {
            return m_ptr.operator bool();
        }
        operator bool() const
        {
            return valid();
        }

    private:
        storage m_data{{0}};
        pointer m_ptr{nullptr};

        base_type* get_pointer()
        {
            return m_ptr.get();
        }
        const base_type* get_pointer() const
        {
            return m_ptr.get();
        }
    };
}  // namespace detail

template <typename CharT,
          typename Category,
          typename Formatter,
          typename Scanner,
          typename SinkBuffer,
          typename SourceBuffer,
          typename Traits>
class basic_stream_ref {
    using erased_stream = detail::basic_erased_stream<CharT,
                                                      Formatter,
                                                      Scanner,
                                                      SinkBuffer,
                                                      SourceBuffer,
                                                      Traits>;

public:
    using char_type = CharT;

    template <
        typename Stream,
        typename = std::enable_if_t<
            detail::is_allowed_category_conversion<typename Stream::category,
                                                   Category>::value>>
    basic_stream_ref(Stream& s) : m_stream(s)
    {
    }

    template <typename C,
              typename Ret = basic_stream_ref<CharT,
                                              C,
                                              Formatter,
                                              Scanner,
                                              SinkBuffer,
                                              SourceBuffer,
                                              Traits>>
    auto as() -> std::enable_if_t<
        detail::is_allowed_category_conversion<Category, C>::value,
        Ret>
    {
        return Ret(*this);
    }

    template <typename C = Category>
    auto read(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value, basic_stream_ref&>
    {
        m_stream->read(s);
        return *this;
    }
    template <typename C = Category>
    auto write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value, basic_stream_ref&>
    {
        m_stream->write(s);
        return *this;
    }
    template <typename C = Category>
    auto seek(streamoff off,
              seekdir dir,
              int which = openmode::in | openmode::out)
        -> std::enable_if_t<is_category<C, detail::random_access>::value,
                            streampos>
    {
        return m_stream->seek(off, dir, which);
    }

    template <typename C = Category>
    auto close() -> std::enable_if_t<is_category<C, closable_tag>::value,
                                     basic_stream_ref&>
    {
        m_stream->close();
        return *this;
    }
    template <typename C = Category>
    auto flush() -> std::enable_if_t<is_category<C, flushable_tag>::value,
                                     basic_stream_ref&>
    {
        m_stream->flush();
        return *this;
    }

#if SPIO_USE_LOCALE
    template <typename C = Category>
    auto imbue(const std::locale& l)
        -> std::enable_if_t<is_category<C, localizable_tag>::value,
                            basic_stream_ref&>
    {
        m_stream->imbue(l);
        return *this;
    }
    template <typename C = Category>
    auto get_locale() const
        -> std::enable_if_t<is_category<C, localizable_tag>::value,
                            basic_stream_ref&>
    {
        m_stream->get_locale();
        return *this;
    }
#endif

    template <typename C = Category, typename... Args>
    auto print(const char_type* f, const Args&... a)
        -> std::enable_if_t<is_category<C, output>::value, basic_stream_ref&>
    {
        m_stream->get_formatter()(
            outstream_iterator<char_type, char_type>(*this), f, a...);
        return *this;
    }

    template <typename C = Category, typename... Args>
    auto scan(const char_type* f, Args&... a)
        -> std::enable_if_t<is_category<C, input>::value, basic_stream_ref&>
    {
        m_stream->get_scanner()(instream_iterator<char_type, char_type>(*this),
                                f, a...);
        return *this;
    }

    template <typename C = Category>
    auto get_source_buffer()
        -> std::enable_if_t<is_category<C, input>::value &&
                                !is_category<C, nobuffer_tag>::value,
                            SourceBuffer&>
    {
        return m_stream->get_source_buffer();
    }
    template <typename C = Category>
    auto get_sink_buffer()
        -> std::enable_if_t<is_category<C, output>::value &&
                                !is_category<C, nobuffer_tag>::value,
                            SourceBuffer&>
    {
        return m_stream->get_sink_buffer();
    }

private:
    erased_stream m_stream;
};
}  // namespace spio

#endif
