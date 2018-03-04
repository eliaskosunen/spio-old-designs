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

#include "buffered_device.h"
#include "config.h"
#include "formatter.h"
#include "locale.h"
#include "stream_base.h"
#include "stream_iterator.impl.h"
#include "util.h"

namespace spio {
// default arguments defined in stream_iterator.h
template <typename CharT, typename Formatter, typename Buffer, typename Traits>
class basic_outstream : public virtual basic_ios_base<CharT>,
                        public detail::basic_output_stream<CharT> {
public:
    using char_type = CharT;
    using formatter_type = Formatter;
    using traits_type = Traits;
    using buffer_type = Buffer;

    template <typename... Args>
    basic_outstream& print(const char_type* f, const Args&... a)
    {
        m_fmt.format_to(outstream_iterator<char_type, char_type>(*this), f,
                        a...);
        return *this;
    }

protected:
    basic_outstream() = default;
    basic_outstream(std::unique_ptr<buffer_type> b) : m_buffer(std::move(b)) {}

    std::unique_ptr<buffer_type> m_buffer{nullptr};
    formatter_type m_fmt{};
};

using outstream = basic_outstream<char>;
using woutstream = basic_outstream<wchar_t>;

// default arguments defined in stream_iterator.h
template <typename CharT, typename Traits>
class basic_instream : public virtual basic_ios_base<CharT>,
                       public detail::basic_input_stream<CharT> {
public:
    using char_type = CharT;
    using traits_type = Traits;

protected:
    basic_instream() = default;
};

using instream = basic_instream<char>;
using winstream = basic_instream<wchar_t>;

template <typename CharT,
          typename Formatter = basic_default_formatter<CharT>,
          typename Buffer = basic_default_device_buffer<CharT>,
          typename Traits = std::char_traits<CharT>>
class basic_iostream
    : public basic_instream<CharT, Traits>,
      public basic_outstream<CharT, Formatter, Buffer, Traits> {
    using in_base = basic_instream<CharT, Traits>;
    using out_base = basic_outstream<CharT, Formatter, Buffer, Traits>;

public:
    using char_type = CharT;
    using traits_type = Traits;
    using formatter_type = Formatter;
    using buffer_type = Buffer;

protected:
    basic_iostream() = default;
    basic_iostream(std::unique_ptr<buffer_type> b) : out_base(std::move(b)) {}
};

using iostream = basic_iostream<char>;
using wiostream = basic_iostream<wchar_t>;

namespace detail {
    template <typename Device, typename = void, typename... Args>
    struct device_stream_base;

    template <typename Device, typename... Args>
    struct device_stream_base<
        Device,
        std::enable_if_t<has_category<Device, input>::value &&
                         has_category<Device, output>::value>,
        Args...> : public basic_iostream<typename Device::char_type, Args...> {
    };

    template <typename Device, typename... Args>
    struct device_stream_base<
        Device,
        std::enable_if_t<!has_category<Device, input>::value &&
                         has_category<Device, output>::value>,
        Args...> : public basic_outstream<typename Device::char_type, Args...> {
        virtual streamsize read(span<typename Device::char_type>) = 0;
    };

    template <typename Device, typename... Args>
    struct device_stream_base<
        Device,
        std::enable_if_t<has_category<Device, input>::value &&
                         !has_category<Device, output>::value>,
        Args...> : public basic_instream<typename Device::char_type, Args...> {
        virtual streamsize write(span<const typename Device::char_type>) = 0;
    };

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

    template <typename Device>
    using is_closable_device = has_category<Device, closable_tag>;
    template <typename Device>
    using is_seekable_device = has_category<Device, detail::random_access>;
    template <typename Device>
    using is_sink = has_category<Device, output>;
    template <typename Device>
    using is_source = has_category<Device, input>;
    template <typename Device>
    using is_flushable_device = has_category<Device, flushable_tag>;
    template <typename Device>
    using is_localizable_device = has_category<Device, localizable_tag>;

    template <typename Device, typename = void>
    struct isopen_stream_impl {
        static bool is_open(const Device&)
        {
            throw failure(invalid_operation,
                          "Device doesn't have member function `is_open`");
        }
    };
    template <typename Device>
    struct isopen_stream_impl<
        Device,
        std::enable_if_t<is_openable_device<Device>::value>> {
        static bool is_open(const Device& d)
        {
            return d.is_open();
        }
    };

    template <typename Device, typename = void>
    struct closable_stream_impl {
        static void close(Device&)
        {
            throw failure(invalid_operation, "Device doesn't model Closable");
        }
    };
    template <typename Device>
    struct closable_stream_impl<
        Device,
        std::enable_if_t<is_closable_device<Device>::value>> {
        static void close(Device& d)
        {
            d.close();
        }
    };

    template <typename Device, typename = void>
    struct seekable_stream_impl {
        static streamsize seek(Device&, streamoff, seekdir, int)
        {
            throw failure(invalid_operation, "Device doesn't model Seekable");
        }
    };
    template <typename Device>
    struct seekable_stream_impl<
        Device,
        std::enable_if_t<is_seekable_device<Device>::value>> {
        static streamsize seek(Device& d, streamoff off, seekdir dir, int which)
        {
            return d.seek(off, dir, which);
        }
    };

    template <typename Device, typename = void>
    struct sink_stream_impl {
        static streamsize write(Device&, span<const typename Device::char_type>)
        {
            throw failure(invalid_operation, "Device doesn't model Sink");
        }
    };
    template <typename Device>
    struct sink_stream_impl<Device, std::enable_if_t<is_sink<Device>::value>> {
        static streamsize write(Device& d,
                                span<const typename Device::char_type> s)
        {
            return d.write(s);
        }
    };

    template <typename Device, typename = void>
    struct source_stream_impl {
        static streamsize read(Device&, span<typename Device::char_type>)
        {
            throw failure(invalid_operation, "Device doesn't model Source");
        }
    };
    template <typename Device>
    struct source_stream_impl<Device,
                              std::enable_if_t<is_source<Device>::value>> {
        static streamsize read(Device& d, span<typename Device::char_type> s)
        {
            return d.read(s);
        }
    };

    template <typename Device, typename = void>
    struct flushable_stream_impl {
        static void flush(Device&)
        {
            throw failure(invalid_operation, "Device doesn't model Flushable");
        }
    };
    template <typename Device>
    struct flushable_stream_impl<
        Device,
        std::enable_if_t<is_flushable_device<Device>::value>> {
        static void flush(Device& d)
        {
            d.flush();
        }
    };

    template <typename Device, typename = void>
    struct localizable_stream_impl {
#if SPIO_USE_LOCALE
        static void imbue(Device&, const std::locale&)
        {
            throw failure(invalid_operation,
                          "Device doesn't model Localizable");
        }
        static const std::locale& get_locale(const Device&)
        {
            throw failure(invalid_operation,
                          "Device doesn't model Localizable");
        }
#endif
    };
    template <typename Device>
    struct localizable_stream_impl<
        Device,
        std::enable_if_t<is_localizable_device<Device>::value>> {
#if SPIO_USE_LOCALE
        static void imbue(Device& d, const std::locale& l)
        {
            d.imbue(l);
        }
        static const std::locale& get_locale(const Device& d)
        {
            return d.get_locale();
        }
#endif
    };
}  // namespace detail

template <
    typename Device,
    typename Formatter = basic_default_formatter<typename Device::char_type>,
    typename Buffer = basic_default_device_buffer<typename Device::char_type>,
    typename Traits = std::char_traits<typename Device::char_type>>
class stream : public detail::
                   device_stream_base<Device, void, Formatter, Buffer, Traits> {
    using base =
        detail::device_stream_base<Device, void, Formatter, Buffer, Traits>;

    static_assert(has_category<Device, device_tag>::value,
                  "stream requires Device to model the concept Device");

public:
    using device_type = Device;
    using char_type = typename device_type::char_type;
    using category = typename device_type::category;

    template <bool Dependent = true,
              typename = std::enable_if_t<
                  std::is_default_constructible<device_type>::value>>
    stream() : base{}, m_dev{}
    {
    }
    stream(device_type d) : base{}, m_dev(std::move(d)) {}
    template <typename... Args>
    stream(device_type d, Args&&... a)
        : base(std::forward<Args>(a)...), m_dev(std::move(d))
    {
    }

    template <typename... Args>
    auto open(Args&&... args) -> std::enable_if_t<
        detail::is_openable_device<device_type, void, Args...>::value,
        void>
    {
        m_dev.open(std::forward<Args>(args)...);
    }

    bool is_open() const override
    {
        return detail::isopen_stream_impl<device_type>::is_open(m_dev);
    }
    void close() override
    {
        detail::closable_stream_impl<device_type>::close(m_dev);
    }

    streamsize seek(streamoff off,
                    seekdir dir,
                    int which = openmode::in | openmode::out) override
    {
        return detail::seekable_stream_impl<device_type>::seek(m_dev, off, dir,
                                                               which);
    }

    void flush() override
    {
        detail::flushable_stream_impl<device_type>::flush(m_dev);
    }

#if SPIO_USE_LOCALE
    void imbue(const std::locale& l) override
    {
        detail::localizable_stream_impl<device_type>::imbue(m_dev, l);
    }
    const std::locale& get_locale() const override
    {
        return detail::localizable_stream_impl<device_type>::get_locale(m_dev);
    }
#endif

    streamsize read(span<char_type> s) override
    {
        return detail::source_stream_impl<device_type>::read(m_dev, s);
    }
    streamsize write(span<const char_type> s) override
    {
        return detail::sink_stream_impl<device_type>::write(m_dev, s);
    }

protected:
    device_type m_dev;
};
}  // namespace spio

#endif
