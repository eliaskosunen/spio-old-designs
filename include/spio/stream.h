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

    template <typename T, typename Allocator = std::allocator<T>>
    using sink_buffer_type = basic_sink_buffer<std::vector<T, Allocator>>;

    template <typename CharT, typename Category, typename Traits>
    struct basic_sink_members_base {
        basic_sink_members_base() = default;
        basic_sink_members_base(std::unique_ptr<sink_buffer_type<CharT>> b)
            : m_buf(std::move(b))
        {
        }

        sink_buffer_type<CharT>* get_sink_buffer()
        {
            return m_buf.get();
        }
        bool has_sink_buffer() const
        {
            return m_buf;
        }

        basic_formatter<CharT>& get_fmt()
        {
            return m_fmt;
        }

        auto& get_pos()
        {
            return m_pos;
        }
        auto& get_pos() const
        {
            return m_pos;
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
                                std::unique_ptr<sink_buffer_type<CharT>>>
        {
            return std::make_unique<sink_buffer_type<CharT>>();
        }

        basic_formatter<CharT> m_fmt{};
        std::unique_ptr<sink_buffer_type<CharT>> m_buf{_init_buf()};
        typename Traits::pos_type m_pos{0};
    };
    template <typename CharT,
              typename Category,
              typename Traits,
              typename = void>
    struct basic_sink_members {
        basic_formatter<CharT>& get_fmt() = delete;
        sink_buffer_type<CharT>* get_sink_buffer() = delete;
        bool has_sink_buffer() const = delete;
        typename Traits::pos_type& get_pos() = delete;
        const typename Traits::pos_type& get_pos() const = delete;
    };
    template <typename CharT, typename Category, typename Traits>
    struct basic_sink_members<
        CharT,
        Category,
        Traits,
        std::enable_if_t<is_category<Category, output>::value>>
        : basic_sink_members_base<CharT, Category, Traits> {
        using basic_sink_members_base<CharT, Category, Traits>::
            basic_sink_members_base;
    };

    template <typename CharT, typename Category, typename Traits>
    struct basic_source_members_base {
        basic_source_members_base() = default;
        basic_source_members_base(std::unique_ptr<source_buffer_type<CharT>> b)
            : m_buf(std::move(b))
        {
        }

        source_buffer_type<CharT>* get_source_buffer()
        {
            return m_buf.get();
        }
        bool has_source_buffer() const
        {
            return m_buf;
        }

        basic_scanner<CharT>& get_scanner()
        {
            return m_scan;
        }

        auto& get_pos()
        {
            return m_pos;
        }
        auto& get_pos() const
        {
            return m_pos;
        }

    private:
        basic_scanner<CharT> m_scan{};
        std::unique_ptr<source_buffer_type<CharT>> m_buf{
            std::make_unique<source_buffer_type<CharT>>()};
        typename Traits::pos_type m_pos{0};
    };
    template <typename CharT,
              typename Category,
              typename Traits,
              typename = void>
    struct basic_source_members {
        basic_scanner<CharT>& get_scanner() = delete;
        source_buffer_type<CharT>* get_source_buffer() = delete;
        bool has_source_buffer() = delete;
        typename Traits::pos_type& get_pos() = delete;
        const typename Traits::pos_type& get_pos() const = delete;
    };
    template <typename CharT, typename Category, typename Traits>
    struct basic_source_members<
        CharT,
        Category,
        Traits,
        std::enable_if_t<is_category<Category, input>::value>>
        : basic_source_members_base<CharT, Category, Traits> {
        using basic_source_members_base<CharT, Category, Traits>::
            basic_source_members_base;
    };

    template <typename CharT, typename Write, typename... Args>
    void print(basic_formatter<CharT>& f,
               Write&& w,
               const CharT* fmt,
               const Args&... a)
    {
        using context = typename fmt::buffer_context<CharT>::type;
        auto str = f(fmt, fmt::basic_format_args<context>(
                              fmt::make_format_args<context>(a...)));
        w(make_span(str));
    }

    template <typename CharT, typename Stream, typename... Args>
    void scan(basic_scanner<CharT>& s,
              Stream& stream,
              bool overread,
              const CharT* fmt,
              Args&... a)
    {
        auto ref = basic_stream_ref<CharT, input>(stream);
        s(ref, fmt, overread, a...);
    }
    template <typename CharT, typename... Args>
    void scan(basic_scanner<CharT>& s,
              basic_stream_ref<CharT, input>& ref,
              bool overread,
              const CharT* fmt,
              Args&... a)
    {
        s(ref, fmt, overread, a...);
    }
}  // namespace detail

template <typename Device, typename Traits>
class basic_stream : public stream_base {
    using sink_members = detail::basic_sink_members<typename Device::char_type,
                                                    typename Device::category,
                                                    Traits>;
    using source_members =
        detail::basic_source_members<typename Device::char_type,
                                     typename Device::category,
                                     Traits>;

    using sink_members_ptr = std::unique_ptr<sink_members>;
    using source_members_ptr = std::unique_ptr<source_members>;

public:
    class instream_sentry {
    public:
        instream_sentry(basic_stream& s, bool noskipws = false) : m_stream(s)
        {
            if (!s.good()) {
                s.set_error(iostate::fail,
                            failure{invalid_operation, "Stream is not good"});
                return;
            }

            if (s.tie()) {
                s.tie()->flush();
            }

            if (!noskipws) {
                if (!s.get_scanner().skip_ws(s)) {
                    s.setstate(iostate::eof);
                    s.set_error(
                        iostate::fail,
                        failure{end_of_file, "EOF while skipping whitespace"});
                    return;
                }
            }

            m_success = s.good();
        }

        instream_sentry(const instream_sentry&) = delete;
        instream_sentry& operator=(const instream_sentry&) = delete;
        instream_sentry(instream_sentry&&) = delete;
        instream_sentry& operator=(instream_sentry&&) = delete;
        ~instream_sentry() = default;

        explicit operator bool() const
        {
            return m_success;
        }

    private:
        basic_stream& m_stream;
        bool m_success{false};
    };
    class outstream_sentry {
    public:
        outstream_sentry(basic_stream& s) : m_stream(s)
        {
            if (!s.good()) {
                s.set_error(iostate::fail,
                            failure{invalid_operation, "Stream is not good"});
                return;
            }

            if (s.tie()) {
                s.tie()->flush();
            }

            m_success = s.good();
        }

        outstream_sentry(const outstream_sentry&) = delete;
        outstream_sentry& operator=(const outstream_sentry&) = delete;
        outstream_sentry(outstream_sentry&&) = delete;
        outstream_sentry& operator=(outstream_sentry&&) = delete;
        ~outstream_sentry() = default;

        explicit operator bool() const
        {
            return m_success;
        }

    private:
        basic_stream& m_stream;
        bool m_success{false};
    };

    friend class instream_sentry;
    friend class outstream_sentry;

    using device_type = Device;
    using char_type = typename device_type::char_type;
    using category = typename device_type::category;
    using formatter_type = basic_formatter<char_type>;
    using scanner_type = basic_scanner<char_type>;
    using sink_buffer_type = sink_buffer_type<char_type>;
    using source_buffer_type = source_buffer_type<char_type>;
    using traits = Traits;
    using int_type = typename traits::int_type;
    using pos_type = typename traits::pos_type;
    using tied_type = basic_stream_ref<char_type, make_category<output>>;

    basic_stream() = default;
    basic_stream(device_type d) : m_dev(std::move(d)) {}
    basic_stream(device_type d,
                 std::unique_ptr<sink_buffer_type> sinkbuf,
                 std::unique_ptr<source_buffer_type> sourcebuf)
        : m_dev(std::move(d)),
          m_sink(_init_sink_members(std::move(sinkbuf))),
          m_source(_init_source_members(std::move(sourcebuf)))
    {
    }

    ~basic_stream()
    {
        _flush_destruct();
    }

    template <typename C = category>
    auto read(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value, basic_stream&>
    {
        try {
            instream_sentry sen(*this, true);
            if (!sen) {
                set_error(iostate::bad, failure{sentry_error});
            }

            auto ret = _buffered_read(s);
            if (ret == -1) {
                setstate(iostate::eof);
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
            outstream_sentry sen(*this);
            if (!sen) {
                set_error(iostate::bad, failure{sentry_error});
            }
            auto ret = _buffered_write(s);
            if (ret != s.size()) {
                set_error(iostate::bad, failure{unknown_io_error});
            }
        }
        catch (const failure& f) {
            _handle_exception(f);
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
        try {
            instream_sentry sen(*this);
            if (!sen) {
                set_error(iostate::bad, failure{sentry_error});
            }
            auto opt = scan_options<char_type>{can_overread(get_device())};
            streamsize i = 1;
            for (auto& ch : s) {
                auto tmp = get();
                if (fail()) {
                    break;
                }
                if (opt.is_space(tmp)) {
                    putback(tmp);
                    break;
                }
                if (eof()) {
                    break;
                }
                ch = tmp;
                ++i;
            }
            return i;
        }
        catch (const failure& f) {
            _handle_exception(f);
        }
        return -1;
    }
    template <typename C = category>
    auto readsome(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value, streamsize>
    {
        try {
            instream_sentry sen(*this);
            if (!sen) {
                set_error(iostate::bad, failure{sentry_error});
            }

            auto n = std::min(
                static_cast<std::ptrdiff_t>(get_source_buffer().size()),
                s.size());
            get_source_buffer().read(s.first(n));
            return n;
        }
        catch (const failure& f) {
            _handle_exception(f);
            return 0;
        }
    }

    template <typename C = category>
    auto getline(span<char_type> s, char_type delim = char_type{'\n'})
        -> std::enable_if_t<is_category<C, input>::value, streamsize>
    {
        try {
            instream_sentry sen(*this);
            if (!sen) {
                set_error(iostate::bad, failure{sentry_error});
            }

            auto it = s.begin();
            bool at_end = false;
            while (true) {
                auto ch = get();
                if (eof()) {
                    setstate(iostate::eof);
                    return std::distance(s.begin(), it);
                }
                if (Traits::eq(ch, delim)) {
                    return std::distance(s.begin(), it);
                }
                if (it == s.end()) {
                    if (!at_end) {
                        at_end = true;
                        continue;
                    }
                    set_error(iostate::fail, failure{out_of_range});
                    return std::distance(s.begin(), it);
                }
                *it = ch;
                ++it;
            }
            if (it == s.end()) {
                if (!eof()) {
                    auto ch = get();
                    if (eof() || Traits::eq(ch, delim)) {
                        return std::distance(s.begin(), it);
                    }
                    putback(ch);
                }
                set_error(iostate::fail, failure{out_of_range});
            }
        }
        catch (const failure& f) {
            _handle_exception(f);
        }
        return -1;
    }

    template <typename C = category>
    auto putback(span<const char_type> s)
        -> std::enable_if_t<is_category<C, input>::value, basic_stream&>
    {
        clear_eof();
        get_source_buffer().push(s);
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
                eof() || !*this) {
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
        -> std::enable_if_t<is_category<C, detail::random_access>::value &&
                                !is_category<C, direct_tag>::value,
                            typename Traits::pos_type>
    {
        try {
            return get_device().seek(off, dir, which);
        }
        catch (const failure& f) {
            _handle_exception(f);
        }
        return static_cast<typename Traits::off_type>(-1);
    }
    template <typename C = category>
    auto seek(typename Traits::off_type off,
              seekdir dir,
              int which = openmode::in | openmode::out)
        -> std::enable_if_t<is_category<C, detail::random_access>::value &&
                                is_category<C, direct_tag>::value,
                            typename Traits::pos_type>
    {
        if (is_category<C, input>::value && is_category<C, output>::value &&
            (which & openmode::in) != 0 && (which & openmode::out) != 0 &&
            get_input_pos() != get_output_pos()) {
            return -1;
        }

        auto _seek = [&](pos_type& a, pos_type max, seekdir d,
                         typename traits::off_type n) {
            if (d == seekdir::beg) {
                if (max < n || n < 0) {
                    set_error(
                        iostate::fail,
                        failure{make_error_code(std::errc::invalid_argument),
                                "basic_stream<direct_tag>::seek: offset is "
                                "out of range"});
                }
                a = n;
                return pos_type(n);
            }
            if (d == seekdir::cur) {
                if (n == 0) {
                    return a;
                }
                if (n < 0) {
                    if (a < -n) {
                        set_error(
                            iostate::fail,
                            failure{
                                make_error_code(std::errc::invalid_argument),
                                "basic_stream<direct_tag>::seek: offset is "
                                "out of range"});
                    }
                    a -= n;
                    return a;
                }
                if (a < n) {
                    set_error(
                        iostate::fail,
                        failure{make_error_code(std::errc::invalid_argument),
                                "basic_stream<direct_tag>::seek: offset is "
                                "out of range"});
                }
                a += n;
                return a;
            }

            if (max < -n || n > 0) {
                set_error(iostate::fail,
                          failure{make_error_code(std::errc::invalid_argument),
                                  "basic_stream<direct_tag>::seek: offset is "
                                  "out of range"});
            }
            a = max + off;
            return a;
        };
        pos_type ret(-1);
        if ((which & openmode::in) != 0 && is_category<C, input>::value) {
            ret = _seek(get_input_pos(), _input_range_size(), dir, off);
        }
        if ((which & openmode::out) != 0 && is_category<C, output>::value) {
            auto tmp = _seek(get_output_pos(), _output_range_size(), dir, off);
            assert(ret == std::streamoff(-1) || tmp == ret);
            ret = tmp;
        }
        return ret;
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
    auto close()
        -> std::enable_if_t<is_category<C, closable_tag>::value, basic_stream&>
    {
        try {
            m_dev.close();
        }
        catch (const failure& f) {
            _handle_exception(f);
        }
        return *this;
    }

    template <typename C = category>
    auto flush()
        -> std::enable_if_t<is_category<C, output>::value &&
                                !is_category<C, no_output_buffer_tag>::value,
                            basic_stream&>
    {
        try {
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
                                    basic_stream&>
    {
        try {
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
                                !is_category<C, no_output_buffer_tag>::value,
                            sink_buffer_type&>
    {
        return *m_sink->get_sink_buffer();
    }
    template <typename C = category>
    auto get_source_buffer()
        -> std::enable_if_t<is_category<C, input>::value, source_buffer_type&>
    {
        return *m_source->get_source_buffer();
    }

    template <typename C = category>
    auto unchecked_read(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value &&
                                is_category<C, direct_tag>::value,
                            streamsize>
    {
        auto _read_nocheck = [&](span<char_type> data) {
            auto res = get_device().input().subspan(get_input_pos());
            std::copy(res.begin(), res.begin() + data.size(), data.begin());
            get_input_pos() += data.size();
            return data.size();
        };

        auto bufsiz = static_cast<std::ptrdiff_t>(get_source_buffer().size());
        if (bufsiz >= s.size()) {
            return _read_nocheck(s);
        }

        auto r = _read_nocheck(s.last(s.size() - bufsiz));
        if (r == -1) {
            return -1;
        }
        get_source_buffer().read(s.first(bufsiz));
        return bufsiz + r;
    }
    template <typename C = category>
    auto unchecked_write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value &&
                                is_category<C, no_output_buffer_tag>::value &&
                                is_category<C, direct_tag>::value,
                            streamsize>
    {
        prepare_direct();
        auto res = get_device().output().subspan(get_output_pos());
        std::copy(s.begin(), s.begin() + s.size(), res.begin());
        get_output_pos() += s.size();
        return s.size();
    }
    template <typename C = category>
    auto unchecked_write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value &&
                                !is_category<C, no_output_buffer_tag>::value &&
                                is_category<C, direct_tag>::value,
                            streamsize>
    {
        prepare_direct();
        auto _write_nocheck = [&](span<const char_type> data) {
            auto res = get_device().output().subspan(get_output_pos());
            std::copy(data.begin(), data.begin() + data.size(), res.begin());
            get_output_pos() += data.size();
            return data.size();
        };
        if (!get_sink_buffer().is_writable_mode()) {
            return _write_nocheck(s);
        }
        else {
            return get_sink_buffer().write(s, [&](span<const char_type> data) {
                return _write_nocheck(data);
            });
        }
    }

    template <typename C = category>
    auto input_range()
        -> std::enable_if_t<is_category<C, input>::value &&
                                is_category<C, direct_tag>::value,
                            span<const char_type>>
    {
        return get_device().input();
    }
    template <typename C = category>
    auto output_range()
        -> std::enable_if_t<is_category<C, output>::value &&
                                is_category<C, direct_tag>::value,
                            span<char_type>>
    {
        return get_device().output();
    }

    template <typename C = category>
    auto prepare_direct()
        -> std::enable_if_t<is_category<C, direct_tag>::value, span<char_type>>
    {
        instream_sentry sen(*this);
        if (!sen) {
            set_error(iostate::bad, failure{sentry_error});
        }
    }

protected:
    void _handle_exception(const failure& f)
    {
        set_error(iostate::fail, f);
    }

    template <typename C = category>
    auto _flush_destruct_flush()
        -> std::enable_if_t<is_category<C, output>::value &&
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
    static auto _init_sink_members(std::unique_ptr<sink_buffer_type> buf =
                                       std::make_unique<sink_buffer_type>())
        -> std::enable_if_t<is_category<C, output>::value, sink_members_ptr>
    {
        return std::make_unique<sink_members>(std::move(buf));
    }
    template <typename C = category>
    static auto _init_sink_members(
        std::unique_ptr<sink_buffer_type> buf = nullptr)
        -> std::enable_if_t<!is_category<C, output>::value, sink_members_ptr>
    {
        SPIO_UNUSED(buf);
        return nullptr;
    }

    template <typename C = category>
    static auto _init_source_members(std::unique_ptr<source_buffer_type> buf =
                                         std::make_unique<source_buffer_type>())
        -> std::enable_if_t<is_category<C, input>::value, source_members_ptr>
    {
        return std::make_unique<source_members>(std::move(buf));
    }
    template <typename C = category>
    static auto _init_source_members(
        std::unique_ptr<source_buffer_type> buf = nullptr)
        -> std::enable_if_t<!is_category<C, input>::value, source_members_ptr>
    {
        SPIO_UNUSED(buf);
        return nullptr;
    }

    template <typename C = category>
    auto _input_range_size()
        -> std::enable_if_t<is_category<C, input>::value &&
                                is_category<C, direct_tag>::value,
                            streamsize>
    {
        return input_range().size();
    }
    template <typename C = category>
    auto _input_range_size()
        -> std::enable_if_t<!is_category<C, input>::value &&
                                is_category<C, direct_tag>::value,
                            streamsize>
    {
        return -1;
    }

    template <typename C = category>
    auto _output_range_size()
        -> std::enable_if_t<is_category<C, output>::value &&
                                is_category<C, direct_tag>::value,
                            streamsize>
    {
        return output_range().size();
    }
    template <typename C = category>
    auto _output_range_size()
        -> std::enable_if_t<!is_category<C, output>::value &&
                                is_category<C, direct_tag>::value,
                            streamsize>
    {
        return -1;
    }

    template <typename C = category>
    auto _direct_read(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value &&
                                is_category<C, direct_tag>::value,
                            streamsize>
    {
        auto res = get_device().input().size() > get_input_pos()
                       ? get_device().input().subspan(get_input_pos())
                       : span<char_type>{};
        if (res.size() >= s.size()) {
            std::copy(res.begin(), res.begin() + s.size(), s.begin());
            get_input_pos() += s.size();
            return s.size();
        }
        else {
            return -1;
        }
    }
    template <typename C = category>
    auto _buffered_read(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value &&
                                !is_category<C, direct_tag>::value,
                            streamsize>
    {
        auto bufsiz = static_cast<std::ptrdiff_t>(get_source_buffer().size());
        if (bufsiz >= s.size()) {
            get_source_buffer().read(s);
            return s.size();
        }
        auto r = get_device().read(s.last(s.size() - bufsiz));
        if (r == -1) {
            return -1;
        }
        get_source_buffer().read(s.first(bufsiz));
        return bufsiz + r;
    }
    template <typename C = category>
    auto _buffered_read(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value &&
                                is_category<C, direct_tag>::value,
                            streamsize>
    {
        auto bufsiz = static_cast<std::ptrdiff_t>(get_source_buffer().size());
        if (bufsiz >= s.size()) {
            get_source_buffer().read(s);
            return s.size();
        }

        auto r = _direct_read(s.last(s.size() - bufsiz));
        if (r == -1) {
            return -1;
        }
        get_source_buffer().read(s.first(bufsiz));
        return bufsiz + r;
    }

    template <typename C = category>
    auto _direct_write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, input>::value &&
                                is_category<C, direct_tag>::value,
                            streamsize>
    {
        auto res = get_device().output().subspan(get_output_pos());
        if (res.size() <= s.size()) {
            std::copy(s.begin(), s.begin() + s.size(), res.begin());
            get_output_pos() += s.size();
            return s.size();
        }
        else {
            set_error(iostate::fail, failure{out_of_range});
        }
    }
    template <typename C = category>
    auto _buffered_write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value &&
                                is_category<C, no_output_buffer_tag>::value &&
                                !is_category<C, direct_tag>::value,
                            streamsize>
    {
        return get_device().write(s);
    }
    template <typename C = category>
    auto _buffered_write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value &&
                                !is_category<C, no_output_buffer_tag>::value &&
                                !is_category<C, direct_tag>::value,
                            streamsize>
    {
        if (!get_sink_buffer().is_writable_mode()) {
            return get_device().write(s);
        }
        else {
            return get_sink_buffer().write(s, [&](span<const char_type> data) {
                return get_device().write(data);
            });
        }
    }
    template <typename C = category>
    auto _buffered_write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value &&
                                is_category<C, no_output_buffer_tag>::value &&
                                is_category<C, direct_tag>::value,
                            streamsize>
    {
        return _direct_write(s);
    }
    template <typename C = category>
    auto _buffered_write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value &&
                                !is_category<C, no_output_buffer_tag>::value &&
                                is_category<C, direct_tag>::value,
                            streamsize>
    {
        if (!get_sink_buffer().is_writable_mode()) {
            return _direct_write(s);
        }
        else {
            return get_sink_buffer().write(s, [&](span<const char_type> data) {
                return _direct_write(data);
            });
        }
    }

    template <typename C = category>
    auto get_input_pos()
        -> std::enable_if_t<is_category<C, input>::value &&
                                is_category<C, direct_tag>::value,
                            pos_type&>
    {
        return m_source->get_pos();
    }
    template <typename C = category>
    auto get_input_pos()
        -> std::enable_if_t<!is_category<C, input>::value &&
                                is_category<C, direct_tag>::value,
                            pos_type&>
    {
        static pos_type n{0};
        return n;
    }
    template <typename C = category>
    auto get_output_pos()
        -> std::enable_if_t<is_category<C, output>::value &&
                                is_category<C, direct_tag>::value,
                            pos_type&>
    {
        return m_sink->get_pos();
    }
    template <typename C = category>
    auto get_output_pos()
        -> std::enable_if_t<!is_category<C, output>::value &&
                                is_category<C, direct_tag>::value,
                            pos_type&>
    {
        static pos_type n{0};
        return n;
    }

private:
    device_type m_dev{};
    tied_type* m_tied{nullptr};
    sink_members_ptr m_sink{_init_sink_members()};
    source_members_ptr m_source{_init_source_members()};
};

template <typename Stream,
          typename Container,
          typename CharT = typename Stream::char_type,
          typename Traits = typename Stream::traits>
auto getline(Stream& in, Container& out, CharT delim) -> Stream&
{
    typename Stream::instream_sentry sen(in);
    if (!sen) {
        in.set_error(iostate::bad, failure{sentry_error});
    }

    out.erase();
    // Maximum size of SSO on MSVC
    // GCC and clang have it bigger
    out.resize(15);

    auto it = out.begin();
    while (true) {
        auto ch = in.get();
        if (in.eof()) {
            out.erase(it, out.end());
            out.shrink_to_fit();
            return in;
        }
        if (Traits::eq(ch, delim)) {
            out.erase(it, out.end());
            out.shrink_to_fit();
            return in;
        }
        if (it == out.end()) {
            auto s = out.size();
            auto newsize = s + std::max(64 - s, s);
            if (newsize > out.max_size()) {
                if (s + 1 >= out.max_size()) {
                    in.set_error(iostate::fail, failure{out_of_range});
                    out.erase(it, out.end());
                    out.shrink_to_fit();
                    return in;
                }
                else {
                    newsize = out.max_size();
                }
            }
            out.resize(newsize);
            it = out.begin() +
                 static_cast<typename Container::difference_type>(s);
        }
        *it = ch;
        ++it;
    }
}

template <typename Stream,
          typename Container,
          typename CharT = typename Stream::char_type,
          typename Traits = typename Stream::traits>
auto getline(Stream& in, Container& out) -> Stream&
{
    return getline(in, out, CharT('\n'));
}
}  // namespace spio

#include "stream.impl.h"

#endif
