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

    template <typename Device, typename = void>
    struct is_isopenable_device : std::false_type {
    };
    template <typename Device>
    struct is_isopenable_device<
        Device,
        void_t<decltype(std::declval<Device>().is_open(), void())>>
        : std::true_type {
    };

    template <typename CharT, typename Formatter, typename Buffer>
    struct sink_members_base {
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
              typename Formatter,
              typename Buffer,
              typename = void>
    struct sink_members {
        Formatter& get_fmt() = delete;
        Buffer& get_sink_buffer() = delete;
        bool has_sink_buffer() const = delete;
    };
    template <typename CharT,
              typename Category,
              typename Formatter,
              typename Buffer>
    struct sink_members<CharT,
                        Category,
                        Formatter,
                        Buffer,
                        std::enable_if_t<is_category<Category, output>::value>>
        : sink_members_base<CharT, Formatter, Buffer> {
    };

    template <typename CharT, typename Scanner, typename Buffer>
    struct source_members_base {
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
    template <typename CharT,
              typename Category,
              typename Scanner,
              typename Buffer,
              typename = void>
    struct source_members {
        Scanner& get_scanner() = delete;
        Buffer& get_source_buffer() = delete;
        bool has_source_buffer() = delete;
    };
    template <typename CharT,
              typename Category,
              typename Scanner,
              typename Buffer>
    struct source_members<CharT,
                          Category,
                          Scanner,
                          Buffer,
                          std::enable_if_t<is_category<Category, input>::value>>
        : source_members_base<CharT, Scanner, Buffer> {
    };
}  // namespace detail

template <typename Device,
          typename Formatter,
          typename Scanner,
          typename SinkBuffer,
          typename SourceBuffer,
          typename Traits>
class basic_stream : stream_base {
    using sink_members = detail::sink_members<typename Device::char_type,
                                              typename Device::category,
                                              Formatter,
                                              SinkBuffer>;
    using source_members = detail::source_members<typename Device::char_type,
                                                  typename Device::category,
                                                  Scanner,
                                                  SourceBuffer>;

    using sink_members_ptr = std::unique_ptr<sink_members>;
    using source_members_ptr = std::unique_ptr<source_members>;

public:
    using device_type = Device;
    using char_type = typename device_type::char_type;
    using category = typename device_type::category;
    using formatter_type = Formatter;
    using scanner_type = Scanner;
    using sink_buffer_type = SinkBuffer;
    using source_buffer_type = SourceBuffer;
    using traits = Traits;

    basic_stream() = default;
    basic_stream(device_type d) : m_dev(std::move(d)) {}
    template <typename... Args>
    basic_stream(Args&&... a) : m_dev(std::forward<Args>(a)...)
    {
    }

    device_type& get_device()
    {
        return m_dev;
    }
    const device_type& get_device() const
    {
        return m_dev;
    }

    template <typename... Args>
    auto open(Args&&... a) -> std::enable_if_t<
        detail::is_openable_device<device_type, Args...>::value,
        void>
    {
        return m_dev.open(std::forward<Args>(a)...);
    }
    template <typename D = device_type>
    auto is_open() const
        -> std::enable_if_t<detail::is_isopenable_device<D>::value, bool>
    {
        return m_dev.is_open();
    }

    template <typename C = category>
    auto read(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value, basic_stream&>
    {
        m_dev.read(s);
        return *this;
    }
    template <typename C = category>
    auto write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value, basic_stream&>
    {
        m_dev.write(s);
        return *this;
    }

    template <typename C = category>
    auto seek(streamoff off,
              seekdir dir,
              int which = openmode::in | openmode::out)
        -> std::enable_if_t<is_category<C, detail::random_access>::value,
                            streampos>
    {
        return m_dev.seek(off, dir, which);
    }

    template <typename C = category>
    auto close()
        -> std::enable_if_t<is_category<C, closable_tag>::value, basic_stream&>
    {
        m_dev.close();
        return *this;
    }
    template <typename C = category>
    auto flush()
        -> std::enable_if_t<is_category<C, flushable_tag>::value, basic_stream&>
    {
        m_dev.flush();
        return *this;
    }

#if SPIO_USE_LOCALE
    template <typename C = category>
    auto imbue(const std::locale& l)
        -> std::enable_if_t<is_category<C, localizable_tag>::value,
                            basic_stream&>
    {
        m_dev.imbue(l);
        return *this;
    }
    template <typename C = category>
    auto get_locale()
        -> std::enable_if_t<is_category<C, localizable_tag>::value,
                            const std::locale&>
    {
        return m_dev.get_locale();
    }
#endif

    template <typename C = category>
    auto get_formatter()
        -> std::enable_if_t<is_category<C, output>::value, formatter_type&> {
            return m_sink->get_fmt();
        }
    template <typename C = category>
    auto get_scanner()
        -> std::enable_if_t<is_category<C, input>::value, scanner_type&> {
            return m_source->get_scanner();
        }

    template <typename C = category, typename... Args>
    auto print(const char_type* f, const Args&... a)
        -> std::enable_if_t<is_category<C, output>::value, basic_stream&>;

    template <typename C = category, typename... Args>
    auto scan(const char_type* f, Args&... a)
        -> std::enable_if_t<is_category<C, input>::value, basic_stream&>;

private:
    template <typename C = category>
    static auto _init_sink_members()
        -> std::enable_if_t<is_category<C, output>::value, sink_members_ptr>
    {
        return std::make_unique<sink_members>();
    }
    template <typename C = category>
    static auto _init_sink_members()
        -> std::enable_if_t<!is_category<C, output>::value, sink_members_ptr>
    {
        return nullptr;
    }

    template <typename C = category>
    static auto _init_source_members()
        -> std::enable_if_t<is_category<C, input>::value, source_members_ptr>
    {
        return std::make_unique<source_members>();
    }
    template <typename C = category>
    static auto _init_source_members()
        -> std::enable_if_t<!is_category<C, input>::value, source_members_ptr>
    {
        return nullptr;
    }

    device_type m_dev{};
    sink_members_ptr m_sink{_init_sink_members()};
    source_members_ptr m_source{_init_source_members()};
};
}  // namespace spio

#include "stream.impl.h"

#endif
