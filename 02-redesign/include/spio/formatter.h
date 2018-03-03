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

#ifndef SPIO_FORMATTER_H
#define SPIO_FORMATTER_H

#include "codeconv.h"
#include "config.h"
#include "fmt.h"
#include "locale.h"
#include "util.h"

namespace spio {
#if 0
template <typename CharT, std::size_t N>
struct fmt_arg_list {
    using type = typename detail::apply_fmt_supported_types<
        basic_arg_list,
        CharT,
        std::integral_constant<std::size_t, N>>::type;
};

template <typename CharT>
class basic_fmt_transformer {
public:
    template <std::size_t N>
    using arg_list = typename fmt_arg_list<CharT, N>::type;

    template <typename N>
    static auto transform(typename arg_list<N::value>::storage_type&& a)
    {
        return apply_n<N::value>(callable{}, std::move(a));
    }

private:
    struct callable {
        template <typename... T>
        auto operator()(T&... args) const
        {
            auto fn = [&](auto arg) {
                return *arg.template get<decltype(arg)>();
            };
            return fmt::make_args<typename fmt::buffer_context<CharT>::type>(
                args->visit(fn)...);
        }
    };
};
#endif

template <typename CharT>
class basic_fmt_formatter;

template <>
class basic_fmt_formatter<char> {
public:
    using char_type = char;
    using result = std::string;

#if 0
    using transformer = basic_fmt_transformer<char>;
    template <std::size_t N>
    using arg_list = typename fmt_arg_list<char, N>::type;
#endif

    template <typename T>
    result operator()(const T& a) const
    {
        return fmt::to_string(a);
    }

    template <typename... Args>
    result format(const char* f, const Args&... a) const
    {
        return fmt::format(f, a...);
    }
#if 0
    template <typename... Args>
    result format(const char* f, const Args&... a) const
    {
        constexpr auto n = sizeof...(Args);
        return vformat<n>(f, make_args<arg_list<n>>(a...));
    }
    template <std::size_t N>
    result vformat(const char* f, arg_list<N> a) const
    {
        return fmt::vformat(f, a.template transform<transformer>());
    }

    template <typename... Args>
    void format_to(span<char> s, const char* f, const Args&... a) const
    {
        constexpr auto n = sizeof...(Args);
        vformat_to<n>(s, f, make_args<arg_list<n>>(a...));
    }
    template <std::size_t N>
    void vformat_to(span<char> s, const char* f, arg_list<N> a) const
    {
        fmt::vformat_to(s.begin(), f, a.template transform<transformer>());
    }
#endif
};

template <>
class basic_fmt_formatter<wchar_t> {
public:
    using char_type = wchar_t;
    using result = std::wstring;

#if 0
    using transformer = basic_fmt_transformer<wchar_t>;
    template <std::size_t N>
    using arg_list = typename fmt_arg_list<wchar_t, N>::type;
#endif

    template <typename T>
    result operator()(const T& a) const
    {
        return fmt::to_wstring(a);
    }

    template <typename... Args>
    result format(const wchar_t* f, const Args&... a) const
    {
        return fmt::format(f, a...);
    }
#if 0
    template <typename... Args>
    result format(const wchar_t* f, const Args&... a) const
    {
        constexpr auto n = sizeof...(Args);
        return vformat<n>(f, make_args<arg_list<n>>(a...));
    }
    template <std::size_t N>
    result vformat(const wchar_t* f, arg_list<N> a) const
    {
        return fmt::vformat(f, a.template transform<transformer>());
    }

    template <typename... Args>
    void format_to(span<wchar_t> s, const wchar_t* f, const Args&... a) const
    {
        constexpr auto n = sizeof...(Args);
        vformat_to<n>(s, f, make_args<arg_list<n>>(a...));
    }
    template <std::size_t N>
    void vformat_to(span<wchar_t> s, const wchar_t* f, arg_list<N> a) const
    {
        fmt::vformat_to(s.begin(), f, a.template transform<transformer>());
    }
#endif
};

template <typename CharT>
class basic_fmt_formatter {
public:
    using char_type = CharT;
    using result = std::basic_string<char_type>;

#if 0
    using transformer = basic_fmt_transformer<char>;
    template <std::size_t N>
    using arg_list = typename fmt_arg_list<char, N>::type;
#endif

    template <typename T>
    result operator()(const T& a)
    {
        return m_conv(m_fmt(a));
    }
    result operator()(const char_type* s)
    {
        return m_conv(m_fmt(m_conv.reverse(s)));
    }

    template <typename... Args>
    result format(const char_type* f, const Args&... a)
    {
        auto fmt = m_conv.reverse(f);
        return m_conv(m_fmt.format(fmt.c_str(), a...));
    }
#if 0
    template <std::size_t N>
    result vformat(const char_type* f, arg_list<N> args) = delete;
#endif

private:
    codeconv<char, char_type> m_conv;
    basic_fmt_formatter<char> m_fmt;
};

template <typename CharT>
using basic_default_formatter = basic_fmt_formatter<CharT>;
}  // namespace spio

#endif
