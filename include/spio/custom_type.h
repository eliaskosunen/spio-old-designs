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

#ifndef SPIO_CUSTOM_TYPE_H
#define SPIO_CUSTOM_TYPE_H

#include "config.h"
#include "fmt.h"
#include "reader_options.h"
#include "writer_options.h"

namespace io {
template <typename T>
struct disable_read {
    template <typename Reader>
    static bool read(Reader& p, T& val, reader_options<T> opt) = delete;
};

template <typename T>
struct disable_write {
    template <typename Writer>
    static bool write(Writer& w, const T& val, writer_options<T> opt) = delete;
};

template <typename T>
struct default_read {
    template <typename Reader>
    static std::enable_if_t<is_reader<Reader>::value, bool>
    read(Reader& p, T& val, reader_options<T> opt)
    {
        SPIO_UNUSED(opt);
        return p.read(val);
    }
};

template <typename T>
struct default_write {
    template <typename Writer>
    static std::enable_if_t<is_writer<Writer>::value, bool>
    write(Writer& w, const T& val, writer_options<T> opt)
    {
        SPIO_UNUSED(opt);
#if SPIO_USE_FMT
        const auto str = fmt::format("{}", val);
        return w.write(
            make_span(str.c_str(), static_cast<extent_t>(str.size())));
#else
        return w.write(val);
#endif
    }
};

template <typename T>
struct custom_type : disable_read<T>, default_write<T> {
};

template <typename Container,
          typename = void_t<decltype(std::declval<Container>().begin()),
                            decltype(std::declval<Container>().end()),
                            decltype(std::declval<Container>().size()),
                            decltype(std::declval<Container>().empty()),
                            decltype(std::declval<Container>().resize(
                                std::declval<typename Container::size_type>())),
                            decltype(std::declval<Container>().erase(
                                std::declval<typename Container::iterator>(),
                                std::declval<typename Container::iterator>()))>>
struct growable_read {
    using type = Container;

    template <typename Reader>
    static std::enable_if_t<is_reader<Reader>::value, bool>
    read(Reader& p, Container& val, reader_options<Container> opt)
    {
        SPIO_UNUSED(opt);
        if (val.empty()) {
            val.resize(15);
        }

        {
            typename Reader::char_type ch{};
            while (p.get(ch)) {
                if (!is_space(ch, opt.spaces)) {
                    p.push(ch);
                    break;
                }
            }
            if (p.eof()) {
                return false;
            }
        }

        if (p.is_overreadable()) {
            auto s = make_span(val);
            reader_options<span<typename Reader::char_type>> o = {opt.spaces,
                                                                  false};
            while (true) {
                if (!p.read(s, o)) {
                    return false;
                }
                {
                    auto it = val.end() - 1;
                    for (; it != val.begin(); --it) {
                        if (*it != '\0') {
                            break;
                        }
                    }
                    val.erase(it + 1, val.end());
                }
                typename Reader::char_type ch;
                if (!p.get(ch)) {
                    return false;
                }
                if (!is_space(ch, opt.spaces)) {
                    p.push(ch);
                    val.resize(val.size() + 64);
                    s = make_span(val.end() - 64, val.end());
                }
                else {
                    break;
                }
            }
            return true;
        }
        else {
            auto it = val.begin();
            while (true) {
                typename Reader::char_type ch;
                auto ret = p.get(ch) == true;
                if (!is_space(ch, opt.spaces)) {
                    if (it == val.end()) {
                        auto s = val.size();
                        val.resize(s + 64);
                        it = val.begin() +
                             static_cast<typename type::difference_type>(s);
                    }
                    *it = ch;
                }
                else {
                    p.push(ch);
                    val.erase(it + 1, val.end());
                    return ret;
                }
                if (!ret) {
                    val.erase(it + 1, val.end());
                    return ret;
                }
                ++it;
            }
        }
    }
};

#if SPIO_USE_STL
template <typename CharT, typename Allocator>
struct custom_type<std::basic_string<CharT, Allocator>>
    : public growable_read<std::basic_string<CharT, Allocator>> {
    using type = std::basic_string<CharT, Allocator>;

    template <typename Writer>
    static std::enable_if_t<is_writer<Writer>::value, bool>
    write(Writer& p, const type& val, writer_options<type> opt)
    {
        SPIO_UNUSED(opt);
        return p.write(
            make_span(val.c_str(), static_cast<extent_t>(val.size())));
    }
};
#endif
}  // namespace io

#endif  // SPIO_CUSTOM_TYPE_H
