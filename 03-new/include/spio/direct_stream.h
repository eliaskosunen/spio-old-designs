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

#ifndef SPIO_DIRECT_STREAM_H
#define SPIO_DIRECT_STREAM_H

#include "fwd.h"

#include "stream.h"

namespace spio {
namespace detail {
}

template <typename Device,
          typename Formatter,
          typename Scanner,
          typename SinkBuffer,
          typename SourceBuffer,
          typename Traits>
class basic_stream<Device,
                   Formatter,
                   Scanner,
                   SinkBuffer,
                   SourceBuffer,
                   Traits,
                   std::enable_if_t<has_category<Device, direct_tag>::value>>
    : public detail::basic_stream_base<Device,
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
    using pos_type = typename traits::pos_type;

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
        auto opt = scan_options<char_type>{can_overread(base::get_device())};
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
        if (is_category<C, input>::value && is_category<C, output>::value &&
            (which & openmode::in) != 0 && (which & openmode::out) != 0 &&
            m_input != m_output) {
            return -1;
        }

        auto _seek = [&](pos_type& a, pos_type max, seekdir d,
                         typename traits::off_type n) {
            if (d == seekdir::beg) {
                if (max < n || n < 0) {
                    throw failure{make_error_code(std::errc::invalid_argument),
                                  "basic_stream<direct_tag>::seek: offset is "
                                  "out of range"};
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
                        throw failure{
                            make_error_code(std::errc::invalid_argument),
                            "basic_stream<direct_tag>::seek: offset is "
                            "out of range"};
                    }
                    a -= n;
                    return a;
                }
                if (a < n) {
                    throw failure{make_error_code(std::errc::invalid_argument),
                                  "basic_stream<direct_tag>::seek: offset is "
                                  "out of range"};
                }
                a += n;
                return a;
            }

            if (max < -n || n > 0) {
                throw failure{make_error_code(std::errc::invalid_argument),
                              "basic_stream<direct_tag>::seek: offset is "
                              "out of range"};
            }
            a = max + off;
            return a;
        };
        pos_type ret(-1);
        if ((which & openmode::in) != 0 && is_category<C, input>::value) {
            ret = _seek(m_input, _input_range_size(), dir, off);
        }
        if ((which & openmode::out) != 0 && is_category<C, output>::value) {
            auto tmp = _seek(m_output, _output_range_size(), dir, off);
            assert(ret == -1 || tmp == ret);
            ret = tmp;
        }
        return ret;
    }

    template <typename C = category>
    auto unchecked_read(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value, streamsize>
    {
        auto _read_nocheck = [&](span<char_type> data) {
            auto res = base::get_device().input().subspan(m_input);
            std::copy(res.begin(), res.begin() + data.size(), data.begin());
            m_input += data.size();
            return data.size();
        };

        auto bufsiz =
            static_cast<std::ptrdiff_t>(base::get_source_buffer().size());
        if (bufsiz >= s.size()) {
            return _read_nocheck(s);
        }

        auto r = _read_nocheck(s.last(s.size() - bufsiz));
        if (r == -1) {
            return -1;
        }
        base::get_source_buffer().read(s.first(bufsiz));
        return bufsiz + r;
    }
    template <typename C = category>
    auto unchecked_write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value &&
                                is_category<C, no_output_buffer_tag>::value,
                            streamsize>
    {
        prepare_direct();
        auto res = base::get_device().output().subspan(m_output);
        std::copy(s.begin(), s.begin() + s.size(), res.begin());
        m_output += s.size();
        return s.size();
    }
    template <typename C = category>
    auto unchecked_write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value &&
                                !is_category<C, no_output_buffer_tag>::value,
                            streamsize>
    {
        prepare_direct();
        auto _write_nocheck = [&](span<const char_type> data) {
            auto res = base::get_device().output().subspan(m_output);
            std::copy(data.begin(), data.begin() + data.size(), res.begin());
            m_output += data.size();
            return data.size();
        };
        if (!base::get_sink_buffer().is_writable_mode()) {
            return _write_nocheck(s);
        }
        else {
            return base::get_sink_buffer().write(
                s, [&](span<const char_type> data) {
                    return _write_nocheck(data);
                });
        }
    }

    template <typename C = category>
    auto input_range()
        -> std::enable_if_t<is_category<C, input>::value, span<const char_type>>
    {
        return base::get_device().input();
    }
    template <typename C = category>
    auto output_range()
        -> std::enable_if_t<is_category<C, output>::value, span<char_type>>
    {
        return base::get_device().output();
    }

    void prepare_direct()
    {
        base::_handle_tied();
    }

    template <typename C = category, typename... Args>
    auto print(const char_type* f, const Args&... a)
        -> std::enable_if_t<is_category<C, output>::value, basic_stream&>
    {
        detail::print<char_type>(base::get_formatter(),
                                 [&](auto s) { write(s); }, f, a...);
        return *this;
    }

    template <typename C = category, typename... Args>
    auto scan(const char_type* f, Args&... a)
        -> std::enable_if_t<is_category<C, input>::value, basic_stream&>
    {
        detail::scan<char_type>(base::get_scanner(), *this,
                                can_overread(base::get_device()), f, a...);
        return *this;
    }

private:
    template <typename C = category>
    auto _input_range_size()
        -> std::enable_if_t<is_category<C, input>::value, streamsize>
    {
        return input_range().size();
    }
    template <typename C = category>
    auto _input_range_size()
        -> std::enable_if_t<!is_category<C, input>::value, streamsize>
    {
        return -1;
    }

    template <typename C = category>
    auto _output_range_size()
        -> std::enable_if_t<is_category<C, output>::value, streamsize>
    {
        return output_range().size();
    }
    template <typename C = category>
    auto _output_range_size()
        -> std::enable_if_t<!is_category<C, output>::value, streamsize>
    {
        return -1;
    }

    template <typename C = category>
    auto _read(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value, streamsize>
    {
        auto res = base::get_device().input().size() > m_input
                       ? base::get_device().input().subspan(m_input)
                       : span<char_type>{};
        if (res.size() >= s.size()) {
            std::copy(res.begin(), res.begin() + s.size(), s.begin());
            m_input += s.size();
            return s.size();
        }
        else {
            return -1;
        }
    }
    template <typename C = category>
    auto _buffered_read(span<char_type> s)
        -> std::enable_if_t<is_category<C, input>::value, streamsize>
    {
        base::_handle_tied();
        auto bufsiz =
            static_cast<std::ptrdiff_t>(base::get_source_buffer().size());
        if (bufsiz >= s.size()) {
            return _read(s);
        }

        auto r = _read(s.last(s.size() - bufsiz));
        if (r == -1) {
            return -1;
        }
        base::get_source_buffer().read(s.first(bufsiz));
        return bufsiz + r;
    }

    template <typename C = category>
    auto _write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, input>::value, streamsize>
    {
        auto res = base::get_device().output().subspan(m_output);
        if (res.size() <= s.size()) {
            std::copy(s.begin(), s.begin() + s.size(), res.begin());
            m_output += s.size();
            return s.size();
        }
        else {
            throw failure{out_of_range};
        }
    }
    template <typename C = category>
    auto _buffered_write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value &&
                                is_category<C, no_output_buffer_tag>::value,
                            streamsize>
    {
        base::_handle_tied();
        return _write(s);
    }
    template <typename C = category>
    auto _buffered_write(span<const char_type> s)
        -> std::enable_if_t<is_category<C, output>::value &&
                                !is_category<C, no_output_buffer_tag>::value,
                            streamsize>
    {
        base::_handle_tied();
        if (!base::get_sink_buffer().is_writable_mode()) {
            return _write(s);
        }
        else {
            return base::get_sink_buffer().write(
                s, [&](span<const char_type> data) { return _write(data); });
        }
    }

    pos_type m_input{0};
    pos_type m_output{0};
};
}  // namespace spio

#endif
