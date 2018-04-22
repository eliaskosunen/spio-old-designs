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
        void_t<decltype(std::declval<Device>().open(std::declval<Args>()...))>,
        Args...> : std::true_type {
    };

    template <typename CharT,
              typename Category,
              typename Formatter,
              typename Buffer>
    struct basic_sink_members_base {
        basic_sink_members_base() = default;
        basic_sink_members_base(std::unique_ptr<Buffer> b) : m_buf(std::move(b))
        {
        }

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
            -> std::enable_if_t<is_category<no_output_buffer_tag, C>::value,
                                std::nullptr_t>
        {
            return nullptr;
        }
        template <typename C = Category>
        static auto _init_buf()
            -> std::enable_if_t<!is_category<no_output_buffer_tag, C>::value,
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
    struct basic_sink_members {
        Formatter& get_fmt() = delete;
        Buffer* get_sink_buffer() = delete;
        bool has_sink_buffer() const = delete;
    };
    template <typename CharT,
              typename Category,
              typename Formatter,
              typename Buffer>
    struct basic_sink_members<
        CharT,
        Category,
        Formatter,
        Buffer,
        std::enable_if_t<is_category<Category, output>::value>>
        : basic_sink_members_base<CharT, Category, Formatter, Buffer> {
        using basic_sink_members_base<CharT, Category, Formatter, Buffer>::
            basic_sink_members_base;
    };

    template <typename CharT,
              typename Category,
              typename Scanner,
              typename Buffer>
    struct basic_source_members_base {
        basic_source_members_base() = default;
        basic_source_members_base(std::unique_ptr<Buffer> b)
            : m_buf(std::move(b))
        {
        }

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
        Scanner m_scan{};
        std::unique_ptr<Buffer> m_buf{std::make_unique<Buffer>()};
    };
    template <typename CharT,
              typename Category,
              typename Scanner,
              typename Buffer,
              typename = void>
    struct basic_source_members {
        Scanner& get_scanner() = delete;
        Buffer* get_source_buffer() = delete;
        bool has_source_buffer() = delete;
    };
    template <typename CharT,
              typename Category,
              typename Scanner,
              typename Buffer>
    struct basic_source_members<
        CharT,
        Category,
        Scanner,
        Buffer,
        std::enable_if_t<is_category<Category, input>::value>>
        : basic_source_members_base<CharT, Category, Scanner, Buffer> {
        using basic_source_members_base<CharT, Category, Scanner, Buffer>::
            basic_source_members_base;
    };

    template <typename Device,
              typename Formatter,
              typename Scanner,
              typename SinkBuffer,
              typename SourceBuffer,
              typename Traits>
    class basic_stream_base : public stream_base {
        using sink_members = basic_sink_members<typename Device::char_type,
                                                typename Device::category,
                                                Formatter,
                                                SinkBuffer>;
        using source_members = basic_source_members<typename Device::char_type,
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
        using int_type = typename traits::int_type;
        using tied_type = basic_stream_ref<char_type, make_category<output>>;

        basic_stream_base() = default;
        basic_stream_base(device_type d) : m_dev(std::move(d)) {}
        basic_stream_base(device_type d,
                          std::unique_ptr<SinkBuffer> sinkbuf,
                          std::unique_ptr<SourceBuffer> sourcebuf)
            : m_dev(std::move(d)),
              m_sink(_init_sink_members(std::move(sinkbuf))),
              m_source(_init_source_members(std::move(sourcebuf)))
        {
        }

        ~basic_stream_base()
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
        auto open(Args&&... a)
            -> std::enable_if_t<is_openable_device<device_type, Args...>::value,
                                basic_stream_base&>
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
        template <typename C = category>
        auto is_open() const
            -> std::enable_if_t<is_category<C, closable_tag>::value, bool>
        {
            return m_dev.is_open();
        }

        template <typename C = category>
        auto close() -> std::enable_if_t<is_category<C, closable_tag>::value,
                                         basic_stream_base&>
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
        auto flush() -> std::enable_if_t<
            is_category<C, output>::value &&
                !is_category<C, no_output_buffer_tag>::value,
            basic_stream_base&>
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

        template <typename C = category>
        auto sync() -> std::enable_if_t<is_category<C, output>::value &&
                                            is_category<C, syncable_tag>::value,
                                        basic_stream_base&>
        {
            try {
                _check_error();
                m_dev.sync();
            }
            catch (const failure& f) {
                _handle_exception(f);
            }
            return *this;
        }

        tied_type* tie() const;
        tied_type* tie(tied_type* s);

        template <typename C = category, typename... Args>
        auto print(const char_type* f, const Args&... a)
            -> std::enable_if_t<is_category<C, output>::value,
                                basic_stream_base&>;

        template <typename C = category, typename... Args>
        auto scan(const char_type* f, Args&... a)
            -> std::enable_if_t<is_category<C, input>::value,
                                basic_stream_base&>;

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
        auto get_sink_buffer() -> std::enable_if_t<
            is_category<C, output>::value &&
                !is_category<C, no_output_buffer_tag>::value,
            sink_buffer_type&>
        {
            return *m_sink->get_sink_buffer();
        }
        template <typename C = category>
        auto get_source_buffer()
            -> std::enable_if_t<is_category<C, input>::value,
                                source_buffer_type&>
        {
            return *m_source->get_source_buffer();
        }

    protected:
        void _check_error()
        {
            if (eof() || bad()) {
                setstate(iostate::fail);
            }
            if (fail()) {
                throw failure(invalid_operation, "Failbit is set");
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
        auto _flush_destruct_flush() -> std::enable_if_t<
            is_category<C, output>::value &&
                !is_category<C, no_output_buffer_tag>::value,
            void>
        {
            try {
                flush();
            }
            catch (...) {
            }
        }
        template <typename C = category>
        auto _flush_destruct_flush()
            -> std::enable_if_t<!(is_category<C, output>::value &&
                                  !is_category<C, no_output_buffer_tag>::value),
                                void>
        {
        }

        template <typename C = category>
        auto _flush_destruct_sync()
            -> std::enable_if_t<is_category<C, output>::value &&
                                    is_category<C, syncable_tag>::value,
                                void>
        {
            try {
                sync();
            }
            catch (...) {
            }
        }
        template <typename C = category>
        auto _flush_destruct_sync()
            -> std::enable_if_t<!(is_category<C, output>::value &&
                                  is_category<C, syncable_tag>::value),
                                void>
        {
        }

        void _flush_destruct()
        {
            _flush_destruct_flush();
            _flush_destruct_sync();
        }

        void _handle_tied();

        template <typename C = category>
        static auto _init_sink_members(
            std::unique_ptr<SinkBuffer> buf = std::make_unique<SinkBuffer>())
            -> std::enable_if_t<is_category<C, output>::value, sink_members_ptr>
        {
            return std::make_unique<sink_members>(std::move(buf));
        }
        template <typename C = category>
        static auto _init_sink_members(
            std::unique_ptr<SinkBuffer> buf = nullptr)
            -> std::enable_if_t<!is_category<C, output>::value,
                                sink_members_ptr>
        {
            SPIO_UNUSED(buf);
            return nullptr;
        }

        template <typename C = category>
        static auto _init_source_members(std::unique_ptr<SourceBuffer> buf =
                                             std::make_unique<SourceBuffer>())
            -> std::enable_if_t<is_category<C, input>::value,
                                source_members_ptr>
        {
            return std::make_unique<source_members>(std::move(buf));
        }
        template <typename C = category>
        static auto _init_source_members(
            std::unique_ptr<SourceBuffer> buf = nullptr)
            -> std::enable_if_t<!is_category<C, input>::value,
                                source_members_ptr>
        {
            SPIO_UNUSED(buf);
            return nullptr;
        }

    private:
        device_type m_dev{};
        tied_type* m_tied{nullptr};
        sink_members_ptr m_sink{_init_sink_members()};
        source_members_ptr m_source{_init_source_members()};
    };
}  // namespace detail

template <typename Device,
          typename Formatter,
          typename Scanner,
          typename SinkBuffer,
          typename SourceBuffer,
          typename Traits,
          typename Enable>
class basic_stream : public detail::basic_stream_base<Device,
                                                      Formatter,
                                                      Scanner,
                                                      SinkBuffer,
                                                      SourceBuffer,
                                                      Traits> {
    using base = detail::basic_stream_base<Device,
                                           Formatter,
                                           Scanner,
                                           SinkBuffer,
                                           SourceBuffer,
                                           Traits>;

public:
    using device_type = typename base::device_type;
    using char_type = typename base::char_type;
    using category = typename base::category;
    using formatter_type = typename base::formatter_type;
    using scanner_type = typename base::scanner_type;
    using sink_buffer_type = typename base::sink_buffer_type;
    using source_buffer_type = typename base::source_buffer_type;
    using traits = typename base::traits;
    using int_type = typename base::int_type;

    using tied_type = typename base::tied_type;

    basic_stream() = default;
    basic_stream(device_type d) : base(std::move(d)) {}
    basic_stream(device_type d,
                 std::unique_ptr<SinkBuffer> sinkbuf,
                 std::unique_ptr<SourceBuffer> sourcebuf)
        : base(std::move(d), std::move(sinkbuf), std::move(sourcebuf))
    {
    }

    template <typename C = category>
    auto read(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value, basic_stream&>
    {
        try {
            base::_check_error();
            if (base::eof()) {
                throw failure(end_of_file);
            }
            auto ret = _buffered_read(s);
            if (ret == -1) {
                base::setstate(iostate::eof);
            }
        }
        catch (const failure& f) {
            base::_handle_exception(f);
        }
        return *this;
    }
    template <typename C = category>
    auto write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value, basic_stream&>
    {
        try {
            base::_check_error();
            auto ret = _buffered_write(s);
            if (ret != s.size()) {
                throw failure(unknown_io_error);
            }
        }
        catch (const failure& f) {
            base::_handle_exception(f);
        }
        return *this;
    }

    template <typename C = category>
    auto get() -> std::enable_if_t<is_category<C, input>::value, char_type>
    {
        char_type ch;
        read(make_span(&ch, 1));
        return ch;
    }
    template <typename C = category>
    auto readword(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value, streamsize>
    {
        auto opt = scan_options<char_type>{can_overread(*this)};
        streamsize i = 1;
        for (auto& ch : s) {
            auto tmp = get();
            if (base::fail()) {
                break;
            }
            if (opt.is_space(tmp)) {
                putback(tmp);
                break;
            }
            if (base::eof()) {
                break;
            }
            ch = tmp;
            ++i;
        }
        return i;
    }
    template <typename C = category>
    auto readsome(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value, streamsize>
    {
        try {
            base::_check_error();
            auto n = std::min(
                static_cast<std::ptrdiff_t>(base::get_source_buffer().size()),
                s.size());
            base::get_source_buffer().read(s.first(n));
            return n;
        }
        catch (const failure& f) {
            base::_handle_exception(f);
            return 0;
        }
    }
    template <typename C = category>
    auto getline(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value, basic_stream&>
    {
        return getline(s, char_type('\n'));
    }
    template <typename C = category>
    auto getline(span<char_type> s, char_type delim)
        -> std::enable_if_t<is_category<C, input>::value, basic_stream&>
    {
        auto it = s.begin();
        while (it != s.end() - 1) {
            auto ch = get();
            if (base::fail() || base::eof()) {
                break;
            }
            if (Traits::eq(ch, delim)) {
                break;
            }
            *it = ch;
            ++it;
        }
        *it = char_type();
        return *this;
    }
    template <typename C = category>
    auto putback(span<const char_type> s)
        -> std::enable_if_t<is_category<C, input>::value, basic_stream&>
    {
        base::clear_eof();
        base::get_source_buffer().push(s);
        return *this;
    }
    template <typename C = category>
    auto putback(char_type s)
        -> std::enable_if_t<is_category<C, input>::value, basic_stream&>
    {
        return putback(make_span(&s, 1));
    }

    template <typename C = category>
    auto put(char_type ch)
        -> std::enable_if_t<is_category<C, output>::value, basic_stream&>
    {
        return write(make_span(&ch, 1));
    }

    template <typename C = category>
    auto ignore(std::size_t count = 1, int_type delim = Traits::eof())
        -> std::enable_if_t<is_category<C, input>::value, basic_stream&>
    {
        for (std::size_t i = 0; i < count; ++i) {
            if (Traits::eq_int_type(Traits::to_int_type(get()), delim) ||
                base::eof() || !*this) {
                break;
            }
        }
        return *this;
    }

    template <typename C = category>
    auto nl() -> std::enable_if_t<is_category<C, output>::value, basic_stream&>
    {
        return put(static_cast<char_type>('\n'));
    }

    template <typename C = category>
    auto seek(typename Traits::off_type off,
              seekdir dir,
              int which = openmode::in | openmode::out)
        -> std::enable_if_t<is_category<C, detail::random_access>::value,
                            typename Traits::pos_type>
    {
        try {
            base::_check_error();
            return base::get_device().seek(off, dir, which);
        }
        catch (const failure& f) {
            base::_handle_exception(f);
        }
        return static_cast<typename Traits::off_type>(-1);
    }

    template <typename C = category, typename... Args>
    auto print(const char_type* f, const Args&... a)
        -> std::enable_if_t<is_category<C, output>::value, basic_stream&>;

    template <typename C = category, typename... Args>
    auto scan(const char_type* f, Args&... a)
        -> std::enable_if_t<is_category<C, input>::value, basic_stream&>;

private:
    template <typename C = category>
    auto _buffered_read(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value, streamsize>
    {
        base::_handle_tied();
        auto bufsiz =
            static_cast<std::ptrdiff_t>(base::get_source_buffer().size());
        if (bufsiz >= s.size()) {
            base::get_source_buffer().read(s);
            return s.size();
        }
        auto r = base::get_device().read(s.last(s.size() - bufsiz));
        if (r == -1) {
            return -1;
        }
        base::get_source_buffer().read(s.first(bufsiz));
        return bufsiz + r;
    }

    template <typename C = category>
    auto _buffered_write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value &&
                                is_category<C, no_output_buffer_tag>::value,
                            streamsize>
    {
        base::_handle_tied();
        return base::get_device().write(s);
    }
    template <typename C = category>
    auto _buffered_write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value &&
                                !is_category<C, no_output_buffer_tag>::value,
                            streamsize>
    {
        base::_handle_tied();
        if (!base::get_sink_buffer().is_writable_mode()) {
            return base::get_device().write(s);
        }
        else {
            return base::get_sink_buffer().write(
                s, [&](span<const char_type> data) {
                    return base::get_device().write(data);
                });
        }
    }
};
}  // namespace spio

#include "stream.impl.h"

#endif
