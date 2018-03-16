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

#include "formatter.h"
#include "locale.h"
#include "scanner.h"
#include "stream_base.h"
#include "stream_buffer.h"
#include "stream_iterator.h"
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

    template <typename CharT,
              typename Category,
              typename Formatter,
              typename Buffer>
    struct sink_members_base {
        sink_members_base() = default;
        sink_members_base(std::unique_ptr<Buffer> b) : m_buf(std::move(b)) {}

        Buffer* get_sink_buffer()
        {
            return m_buf.get();
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
        template <typename C = Category>
        static auto _init_buf()
            -> std::enable_if_t<is_category<nobuffer_tag, C>::value,
                                std::nullptr_t>
        {
            return nullptr;
        }
        template <typename C = Category>
        static auto _init_buf()
            -> std::enable_if_t<!is_category<nobuffer_tag, C>::value,
                                std::unique_ptr<Buffer>>
        {
            return std::make_unique<Buffer>();
        }

        Formatter m_fmt{};
        std::unique_ptr<Buffer> m_buf{_init_buf()};
    };
    template <typename CharT,
              typename Category,
              typename Formatter,
              typename Buffer,
              typename = void>
    struct sink_members {
        Formatter& get_fmt() = delete;
        Buffer* get_sink_buffer() = delete;
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
        : sink_members_base<CharT, Category, Formatter, Buffer> {
        using sink_members_base<CharT, Category, Formatter, Buffer>::
            sink_members_base;
    };

    template <typename CharT,
              typename Category,
              typename Scanner,
              typename Buffer>
    struct source_members_base {
        source_members_base() = default;
        source_members_base(std::unique_ptr<Buffer> b) : m_buf(std::move(b)) {}

        Buffer* get_source_buffer()
        {
            return m_buf.get();
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
        template <typename C = Category>
        static auto _init_buf()
            -> std::enable_if_t<is_category<nobuffer_tag, C>::value,
                                std::nullptr_t>
        {
            return nullptr;
        }
        template <typename C = Category>
        static auto _init_buf()
            -> std::enable_if_t<!is_category<nobuffer_tag, C>::value,
                                std::unique_ptr<Buffer>>
        {
            return std::make_unique<Buffer>();
        }

        Scanner m_scan{};
        std::unique_ptr<Buffer> m_buf{_init_buf()};
    };
    template <typename CharT,
              typename Category,
              typename Scanner,
              typename Buffer,
              typename = void>
    struct source_members {
        Scanner& get_scanner() = delete;
        Buffer* get_source_buffer() = delete;
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
        : source_members_base<CharT, Category, Scanner, Buffer> {
        using source_members_base<CharT, Category, Scanner, Buffer>::
            source_members_base;
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

    ~basic_stream()
    {
        _flush_destruct();
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
        basic_stream&>
    {
        try {
            _check_error();
            m_dev.open(std::forward<Args>(a)...);
        }
        catch (const failure& f) {
            _handle_exception(f);
        }
        return *this;
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
        try {
            _check_error();
            if (eof()) {
                throw failure(end_of_file);
            }
            auto ret = _buffered_read(s);
            if (ret == -1) {
                setstate(iostate::eof);
            }
            else if (ret != s.size()) {
                throw failure(unknown_io_error);
            }
        }
        catch (const failure& f) {
            _handle_exception(f);
        }
        return *this;
    }
    template <typename C = category>
    auto write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value, basic_stream&>
    {
        try {
            _check_error();
            auto ret = _buffered_write(s);
            if (ret != s.size()) {
                throw failure(unknown_io_error);
            }
        }
        catch (const failure& f) {
            _handle_exception(f);
        }
        return *this;
    }

    template <typename C = category>
    auto seek(streamoff off,
              seekdir dir,
              int which = openmode::in | openmode::out)
        -> std::enable_if_t<is_category<C, detail::random_access>::value,
                            streampos>
    {
        try {
            _check_error();
            return m_dev.seek(off, dir, which);
        }
        catch (const failure& f) {
            _handle_exception(f);
        }
        return -1;
    }

    template <typename C = category>
    auto close()
        -> std::enable_if_t<is_category<C, closable_tag>::value, basic_stream&>
    {
        try {
            _check_error();
            m_dev.close();
        }
        catch (const failure& f) {
            _handle_exception(f);
        }
        return *this;
    }

    template <typename C = category>
    auto flush() -> std::enable_if_t<is_category<C, flushable_tag>::value ||
                                         !is_category<C, nobuffer_tag>::value,
                                     basic_stream&>
    {
        try {
            _check_error();
            _flush_buffer();
            _flush_device();
        }
        catch (const failure& f) {
            _handle_exception(f);
        }
        return *this;
    }
    template <typename C = category>
    auto flush_buffer()
        -> std::enable_if_t<!is_category<C, nobuffer_tag>::value, basic_stream&>
    {
        try {
            _check_error();
            auto b = get_sink_buffer().get_flushable_data();
            auto n = m_dev.write(b);
            get_sink_buffer().flag_flushed(n);
        }
        catch (const failure& f) {
            _handle_exception(f);
        }
        return *this;
    }

#if SPIO_USE_LOCALE
    template <typename C = category>
    auto imbue(const std::locale& l)
        -> std::enable_if_t<is_category<C, localizable_tag>::value,
                            basic_stream&>
    {
        try {
            _check_error();
            m_dev.imbue(l);
        }
        catch (const failure& f) {
            _handle_exception(f);
        }
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

    template <typename C = category, typename... Args>
    auto print(const char_type* f, const Args&... a)
        -> std::enable_if_t<is_category<C, output>::value, basic_stream&>;

    template <typename C = category, typename... Args>
    auto scan(const char_type* f, Args&... a)
        -> std::enable_if_t<is_category<C, input>::value, basic_stream&>;

    template <typename C = category>
    auto get_formatter()
        -> std::enable_if_t<is_category<C, output>::value, formatter_type&>
    {
        return m_sink->get_fmt();
    }
    template <typename C = category>
    auto get_scanner()
        -> std::enable_if_t<is_category<C, input>::value, scanner_type&>
    {
        return m_source->get_scanner();
    }

    template <typename C = category>
    auto get_sink_buffer()
        -> std::enable_if_t<is_category<C, output>::value &&
                                !is_category<C, nobuffer_tag>::value,
                            sink_buffer_type&>
    {
        return *m_sink->get_sink_buffer();
    }
    template <typename C = category>
    auto get_source_buffer()
        -> std::enable_if_t<is_category<C, input>::value &&
                                !is_category<C, nobuffer_tag>::value,
                            source_buffer_type&>
    {
        return *m_source->get_source_buffer();
    }

protected:
    basic_stream(device_type d,
                 sink_members_ptr sink,
                 source_members_ptr source)
        : m_dev(std::move(d)),
          m_sink(std::move(sink)),
          m_source(std::move(source))
    {
    }

private:
    void _check_error() const
    {
        if (fail()) {
            throw failure(invalid_operation, "fail flag is set");
        }
    }
    void _handle_exception(const failure& f)
    {
        setstate(iostate::fail);
        set_error(f.code());
        if ((exceptions() & iostate::fail) != 0) {
            throw f;
        }
    }

    template <typename C = category>
    auto _flush_buffer()
        -> std::enable_if_t<is_category<C, nobuffer_tag>::value, void>
    {
    }
    template <typename C = category>
    auto _flush_buffer()
        -> std::enable_if_t<!is_category<C, nobuffer_tag>::value, void>
    {
        flush_buffer();
    }

    template <typename C = category>
    auto _flush_device()
        -> std::enable_if_t<is_category<C, flushable_tag>::value, void>
    {
        m_dev.flush();
    }
    template <typename C = category>
    auto _flush_device()
        -> std::enable_if_t<!is_category<C, flushable_tag>::value, void>
    {
    }

    template <typename C = category>
    auto _flush_destruct()
        -> std::enable_if_t<!(is_category<C, flushable_tag>::value ||
                              !is_category<C, nobuffer_tag>::value),
                            void>
    {
    }
    template <typename C = category>
    auto _flush_destruct()
        -> std::enable_if_t<is_category<C, flushable_tag>::value ||
                                !is_category<C, nobuffer_tag>::value,
                            void>
    {
        flush();
    }

    template <typename C = category>
    auto _buffered_read(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value &&
                                is_category<C, nobuffer_tag>::value,
                            streamsize>
    {
        auto r = m_dev.read(s);
        if (r == -1) {
            return -1;
        }
        return r;
    }
    template <typename C = category>
    auto _buffered_read(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value &&
                                !is_category<C, nobuffer_tag>::value,
                            streamsize>
    {
        auto n = static_cast<streamsize>(
            std::min(get_source_buffer().size(), s.size_us()));
        get_source_buffer().read(s.first(n));
        auto r = m_dev.read(s);
        if (r == -1) {
            get_source_buffer().push(s.first(n));
            return -1;
        }
        return n + r;
    }

    template <typename C = category>
    auto _buffered_write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value &&
                                is_category<C, nobuffer_tag>::value,
                            streamsize>
    {
        return m_dev.write(s);
    }
    template <typename C = category>
    auto _buffered_write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value &&
                                !is_category<C, nobuffer_tag>::value,
                            streamsize>
    {
        if (!get_sink_buffer().is_writable_mode()) {
            return m_dev.write(s);
        }
        else {
            return get_sink_buffer().write(s, [&](span<const char_type> data) {
                return m_dev.write(data);
            });
        }
    }

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
};  // namespace spio
}  // namespace spio

#include "stream.impl.h"

#endif
