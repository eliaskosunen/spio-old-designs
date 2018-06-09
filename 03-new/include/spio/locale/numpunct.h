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

#ifndef SPIO_LOCALE_NUMPUNCT_H
#define SPIO_LOCALE_NUMPUNCT_H

#include "locale/facet.h"

namespace spio {
template <typename Char>
class numpunct : public detail::facet {
public:
    using char_type = Char;

    char_type decimal_point() const
    {
        return do_decimal_point();
    }
    char_type thousands_sep() const
    {
        return do_thousands_sep();
    }
    std::string grouping() const
    {
        return do_grouping();
    }
    std::basic_string<char_type> truename() const
    {
        return do_truename();
    }
    std::basic_string<char_type> falsename() const
    {
        return do_falsename();
    }

protected:
    virtual char_type do_decimal_point() const = 0;
    virtual char_type do_thousands_sep() const = 0;
    virtual std::string do_grouping() const = 0;
    virtual std::basic_string<char_type> do_truename() const = 0;
    virtual std::basic_string<char_type> do_falsename() const = 0;
};

template <>
class numpunct<char> : public detail::facet {
public:
    using char_type = char;

    char_type decimal_point() const
    {
        return '.';
    }
    char_type thousands_sep() const
    {
        return ',';
    }
    std::string grouping() const
    {
        return "";
    }
    std::basic_string<char_type> truename() const
    {
        return "true";
    }
    std::basic_string<char_type> falsename() const
    {
        return "false";
    }
};

template <>
class numpunct<wchar_t> : public detail::facet {
public:
    using char_type = wchar_t;

    char_type decimal_point() const
    {
        return L'.';
    }
    char_type thousands_sep() const
    {
        return L',';
    }
    std::string grouping() const
    {
        return "";
    }
    std::basic_string<char_type> truename() const
    {
        return L"true";
    }
    std::basic_string<char_type> falsename() const
    {
        return L"false";
    }
};
}  // namespace spio

#endif  // SPIO_LOCALE_NUMPUNCT_H
