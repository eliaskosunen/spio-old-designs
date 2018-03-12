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

#ifndef SPIO_STREAM_H
#define SPIO_STREAM_H

#include "fwd.h"

#include "buffered_device.h"
#include "formatter.h"
#include "locale.h"
#include "scanner.h"
#include "stream_base.h"
#include "stream_iterator.impl.h"
#include "util.h"

namespace spio {
namespace detail {
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

    template <typename CharT>
    struct basic_input_stream_base {
        virtual streamsize read(span<CharT>) = 0;
        virtual ~basic_input_stream_base() = default;
    };
    template <typename CharT, typename Category, typename = void>
    struct basic_input_stream {
    };
    template <typename CharT, typename Category>
    struct basic_input_stream<
        CharT,
        Category,
        std::enable_if_t<is_category<Category, input>::value>>
        : basic_input_stream_base<CharT> {
    };

    template <typename CharT>
    struct basic_output_stream_base {
        virtual streamsize write(span<const CharT>) = 0;
        virtual ~basic_output_stream_base() = default;
    };
    template <typename CharT, typename Category, typename = void>
    struct basic_output_stream {
    };
    template <typename CharT, typename Category>
    struct basic_output_stream<
        CharT,
        Category,
        std::enable_if_t<is_category<Category, output>::value>>
        : basic_output_stream_base<CharT> {
    };

    struct seekable_stream_base {
        virtual streampos seek(streamoff, seekdir, int) = 0;
        virtual ~seekable_stream_base() = default;
    };
    template <typename Category, typename = void>
    struct seekable_stream {
    };
    template <typename Category>
    struct seekable_stream<
        Category,
        std::enable_if_t<is_category<Category, detail::random_access>::value>>
        : seekable_stream_base {
    };

    struct closable_stream_base {
        virtual void close() = 0;
        virtual ~closable_stream_base() = default;
    };
    template <typename Category, typename = void>
    struct closable_stream {
    };
    template <typename Category>
    struct closable_stream<
        Category,
        std::enable_if_t<is_category<Category, closable_tag>::value>>
        : closable_stream_base {
    };

    struct flushable_stream_base {
        virtual void flush() = 0;
        virtual ~flushable_stream_base() = default;
    };
    template <typename Category, typename = void>
    struct flushable_stream {
    };
    template <typename Category>
    struct flushable_stream<
        Category,
        std::enable_if_t<is_category<Category, flushable_tag>::value>>
        : flushable_stream_base {
    };

    struct localizable_stream_base {
#if SPIO_USE_LOCALE
        virtual void imbue(const std::locale&) = 0;
        virtual const std::locale& get_locale() const = 0;
#endif
        virtual ~localizable_stream_base() = default;
    };
    template <typename Category, typename = void>
    struct localizable_stream {
    };
    template <typename Category>
    struct localizable_stream<
        Category,
        std::enable_if_t<is_category<Category, localizable_tag>::value>>
        : localizable_stream_base {
    };

    template <typename CharT,
              typename Category,
              typename Formatter,
              typename Buffer,
              typename = void>
    struct sink_members {
        Formatter get_fmt() = delete;
        Buffer get_sink_buffer() = delete;
        bool has_sink_buffer() const = delete;
    };
    template <typename CharT,
              typename Category,
              typename Formatter,
              typename Buffer>
    struct sink_members<
        CharT,
        Category,
        Formatter,
        Buffer,
        std::enable_if_t<is_category<Category, output>::value>> {
        Buffer& get_sink_buffer()
        {
            return *m_buf;
        }
        bool has_sink_buffer() const
        {
            return m_buf;
        }

        Formatter& get_fmt()
        {
            return m_fmt;
        }

    private:
        Formatter m_fmt{};
        std::unique_ptr<Buffer> m_buf;
    };

    template <typename CharT,
              typename Category,
              typename Scanner,
              typename Buffer,
              typename = void>
    struct source_members {
        Scanner get_scanner() = delete;
        Buffer get_source_buffer() = delete;
        bool has_source_buffer() = delete;
    };
    template <typename CharT,
              typename Category,
              typename Scanner,
              typename Buffer>
    struct source_members<
        CharT,
        Category,
        Scanner,
        Buffer,
        std::enable_if_t<is_category<Category, input>::value>> {
        Buffer& get_source_buffer()
        {
            return *m_buf;
        }
        bool has_source_buffer() const
        {
            return m_buf;
        }

        Scanner& get_scanner()
        {
            return m_scan;
        }

    private:
        Scanner m_scan{};
        std::unique_ptr<Buffer> m_buf;
    };
}  // namespace detail

template <typename CharT,
          typename Category,
          typename Formatter,
          typename Scanner,
          typename SinkBuffer,
          typename SourceBuffer,
          typename Traits>
class basic_stream : public stream_base,
                     public detail::basic_input_stream<CharT, Category>,
                     public detail::basic_output_stream<CharT, Category>,
                     public detail::seekable_stream<Category>,
                     public detail::closable_stream<Category>,
                     public detail::flushable_stream<Category>,
                     public detail::localizable_stream<Category> {
    using sink_members =
        detail::sink_members<CharT, Category, Formatter, SinkBuffer>;
    using source_members =
        detail::source_members<CharT, Category, Scanner, SourceBuffer>;

    using sink_members_ptr = std::unique_ptr<sink_members>;
    using source_members_ptr = std::unique_ptr<source_members>;

public:
    using char_type = CharT;
    using formatter_type = Formatter;
    using scanner_type = Scanner;
    using sink_buffer_type = SinkBuffer;
    using source_buffer_type = SourceBuffer;
    using traits = Traits;

    template <typename C = Category, typename... Args>
    auto print(const char_type* f, const Args&... a)
        -> std::enable_if_t<is_category<C, output>::value, basic_stream&>;

    template <typename C = Category, typename... Args>
    auto scan(const char_type* f, Args&... a)
        -> std::enable_if_t<is_category<C, input>::value, basic_stream&>;

    /* template <typename DestCategory> */
    /* auto with_category() -> std::enable_if_t< */
    /*     detail::is_allowed_category_conversion<Category,
     * DestCategory>::value, */
    /*     basic_stream<CharT, */
    /*                  DestCategory, */
    /*                  Formatter, */
    /*                  Scanner, */
    /*                  SinkBuffer, */
    /*                  SourceBuffer, */
    /*                  Traits>> */
    /* { */
    /*     return basic_stream(static_cast<stream_base&>(*this), m_sink,
     * m_source); */
    /* } */

protected:
    basic_stream() = default;

    struct base_construct_tag {
    };

    template <typename... Args>
    basic_stream(base_construct_tag, Args&&... args)
        : stream_base(std::forward<Args>(args)...)
    {
    }

private:
    template <typename C = Category>
    static auto _init_sink_members()
        -> std::enable_if_t<is_category<C, output>::value, sink_members_ptr>
    {
        return std::make_unique<sink_members>();
    }
    template <typename C = Category>
    static auto _init_sink_members()
        -> std::enable_if_t<!is_category<C, output>::value, sink_members_ptr>
    {
        return nullptr;
    }

    template <typename C = Category>
    static auto _init_source_members()
        -> std::enable_if_t<is_category<C, input>::value, source_members_ptr>
    {
        return std::make_unique<source_members>();
    }
    template <typename C = Category>
    static auto _init_source_members()
        -> std::enable_if_t<!is_category<C, input>::value, source_members_ptr>
    {
        return nullptr;
    }

    basic_stream(stream_base base,
                 sink_members_ptr sink,
                 source_members_ptr source)
        : stream_base(std::move(base)),
          m_sink(std::move(sink)),
          m_source(std::move(source))
    {
    }

    sink_members_ptr m_sink{_init_sink_members()};
    source_members_ptr m_source{_init_source_members()};
};

namespace detail {
    template <typename Device, typename = void, typename... Args>
    struct is_openable_device : std::false_type {
    };
    template <typename Device, typename... Args>
    struct is_openable_device<
        Device,
        void_t<decltype(std::declval<Device>().open(std::declval<Args>()...),
                        void())>,
        Args...> : std::true_type {
    };

#if 0
    template <typename CharT, typename Category, typename = void>
    struct basic_device_input_stream {
        virtual streamsize read(span<CharT>) = 0;
        virtual ~basic_device_input_stream() = default;
    };
    template <typename CharT, typename Category>
    struct basic_device_input_stream<
        CharT,
        Category,
        std::enable_if_t<is_category<Category, input>::value>> {
    };

    template <typename CharT, typename Category, typename = void>
    struct basic_device_output_stream {
        virtual streamsize write(span<const CharT>) = 0;
        virtual ~basic_device_output_stream() = default;
    };
    template <typename CharT, typename Category>
    struct basic_device_output_stream<
        CharT,
        Category,
        std::enable_if_t<is_category<Category, output>::value>> {
    };

    template <typename Category, typename = void>
    struct device_seekable_stream {
        virtual streampos seek(streamoff, seekdir, int) = 0;
        virtual ~device_seekable_stream() = default;
    };
    template <typename Category>
    struct device_seekable_stream<
        Category,
        std::enable_if_t<is_category<Category, detail::random_access>::value>> {
    };

    template <typename Category, typename = void>
    struct device_closable_stream {
        virtual void close() = 0;
        virtual ~device_closable_stream() = default;
    };
    template <typename Category>
    struct device_closable_stream<
        Category,
        std::enable_if_t<is_category<Category, closable_tag>::value>> {
    };

    template <typename Category, typename = void>
    struct device_flushable_stream {
        virtual void flush() = 0;
        virtual ~device_flushable_stream() = default;
    };
    template <typename Category>
    struct device_flushable_stream<
        Category,
        std::enable_if_t<is_category<Category, flushable_tag>::value>> {
    };

    template <typename Category, typename = void>
    struct device_localizable_stream {
#if SPIO_USE_LOCALE
        virtual void imbue(const std::locale&) = 0;
        virtual const std::locale& get_locale() const = 0;
#endif
        virtual ~device_localizable_stream() = default;
    };
    template <typename Category>
    struct device_localizable_stream<
        Category,
        std::enable_if_t<is_category<Category, localizable_tag>::value>> {
    };
#endif

    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits>
    struct basic_device_stream_base
        : public basic_stream<typename Device::char_type,
                              typename Device::category,
                              Formatter,
                              Scanner,
                              SinkBuffer,
                              SourceBuffer,
                              Traits> {
        virtual Device& get_device() = 0;
        virtual const Device& get_device() const = 0;
    };

    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits,
              typename = void>
    struct basic_device_input_stream {
    };
    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits>
    struct basic_device_input_stream<
        Device,
        Formatter,
        Scanner,
        SinkBuffer,
        SourceBuffer,
        Traits,
        std::enable_if_t<has_category<Device, input>::value>>
        : public virtual basic_device_stream_base<Device,
                                                  Formatter,
                                                  Scanner,
                                                  SinkBuffer,
                                                  SourceBuffer,
                                                  Traits> {
        streamsize read(span<typename Device::char_type> s) override
        {
            return basic_device_stream_base<Device, Formatter, Scanner,
                                            SinkBuffer, SourceBuffer,
                                            Traits>::get_device()
                .read(s);
        }
    };

    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits,
              typename = void>
    struct basic_device_output_stream {
    };
    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits>
    struct basic_device_output_stream<
        Device,
        Formatter,
        Scanner,
        SinkBuffer,
        SourceBuffer,
        Traits,
        std::enable_if_t<has_category<Device, output>::value>>
        : public virtual basic_device_stream_base<Device,
                                                  Formatter,
                                                  Scanner,
                                                  SinkBuffer,
                                                  SourceBuffer,
                                                  Traits> {
        streamsize write(span<const typename Device::char_type> s) override
        {
            return basic_device_stream_base<Device, Formatter, Scanner,
                                            SinkBuffer, SourceBuffer,
                                            Traits>::get_device()
                .write(s);
        }
    };

    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits,
              typename = void>
    struct device_seekable_stream {
    };
    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits>
    struct device_seekable_stream<
        Device,
        Formatter,
        Scanner,
        SinkBuffer,
        SourceBuffer,
        Traits,
        std::enable_if_t<has_category<Device, detail::random_access>::value>>
        : public virtual basic_device_stream_base<Device,
                                                  Formatter,
                                                  Scanner,
                                                  SinkBuffer,
                                                  SourceBuffer,
                                                  Traits> {
        streamoff seek(streamoff off, seekdir dir, int which) override
        {
            return basic_device_stream_base<Device, Formatter, Scanner,
                                            SinkBuffer, SourceBuffer,
                                            Traits>::get_device()
                .seek(off, dir, which);
        }
    };

    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits,
              typename = void>
    struct device_closable_stream {
    };
    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits>
    struct device_closable_stream<
        Device,
        Formatter,
        Scanner,
        SinkBuffer,
        SourceBuffer,
        Traits,
        std::enable_if_t<has_category<Device, closable_tag>::value>>
        : public virtual basic_device_stream_base<Device,
                                                  Formatter,
                                                  Scanner,
                                                  SinkBuffer,
                                                  SourceBuffer,
                                                  Traits> {
        void close() override
        {
            basic_device_stream_base<Device, Formatter, Scanner, SinkBuffer,
                                     SourceBuffer, Traits>::get_device()
                .close();
        }
    };

    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits,
              typename = void>
    struct device_flushable_stream {
    };
    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits>
    struct device_flushable_stream<
        Device,
        Formatter,
        Scanner,
        SinkBuffer,
        SourceBuffer,
        Traits,
        std::enable_if_t<has_category<Device, flushable_tag>::value>>
        : public virtual basic_device_stream_base<Device,
                                                  Formatter,
                                                  Scanner,
                                                  SinkBuffer,
                                                  SourceBuffer,
                                                  Traits> {
        void flush() override
        {
            basic_device_stream_base<Device, Formatter, Scanner, SinkBuffer,
                                     SourceBuffer, Traits>::get_device()
                .flush();
        }
    };

    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits,
              typename = void>
    struct device_localizable_stream {
    };
    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits>
    struct device_localizable_stream<
        Device,
        Formatter,
        Scanner,
        SinkBuffer,
        SourceBuffer,
        Traits,
        std::enable_if_t<has_category<Device, localizable_tag>::value>>
        : public virtual basic_device_stream_base<Device,
                                                  Formatter,
                                                  Scanner,
                                                  SinkBuffer,
                                                  SourceBuffer,
                                                  Traits> {
#if SPIO_USE_LOCALE
        void imbue(const std::locale& l) override
        {
            basic_device_stream_base<Device, Formatter, Scanner, SinkBuffer,
                                     SourceBuffer, Traits>::get_device()
                .imbue(l);
        }
        const std::locale& get_locale() override
        {
            return basic_device_stream_base<Device, Formatter, Scanner,
                                            SinkBuffer, SourceBuffer,
                                            Traits>::get_device()
                .get_locale();
        }
#endif
    };

    template <typename... Args>
    struct device_stream_base_helper
        : public basic_device_input_stream<Args...>,
          public basic_device_output_stream<Args...>,
          public device_seekable_stream<Args...>,
          public device_closable_stream<Args...>,
          public device_flushable_stream<Args...>,
          public device_localizable_stream<Args...> {
    };
}  // namespace detail

template <typename Device,
          typename Formatter,
          typename Scanner,
          typename SinkBuffer,
          typename SourceBuffer,
          typename Traits>
class basic_device_stream
    : public detail::device_stream_base_helper<Device,
                                               Formatter,
                                               Scanner,
                                               SinkBuffer,
                                               SourceBuffer,
                                               Traits> {
    using category = typename Device::category;

public:
    using device_type = Device;
    using char_type = typename Device::char_type;

    basic_device_stream() = default;
    template <typename... Args>
    basic_device_stream(Args&&... a) : m_dev(std::forward<Args>(a)...)
    {
    }
    basic_device_stream(device_type d) : m_dev(std::move(d)) {}

    template <typename... Args>
    auto open(Args&&... args)
        -> std::enable_if_t<detail::is_openable_device<Device, Args...>::value>
    {
        m_dev.open(std::forward<Args>(args)...);
    }

    device_type& get_device() override
    {
        return m_dev;
    }
    const device_type& get_device() const override
    {
        return m_dev;
    }

private:
    device_type m_dev{};
};
}  // namespace spio

#include "stream.impl.h"

#endif
