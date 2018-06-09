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

#ifndef SPIO_LOCALE_LOCALE_H
#define SPIO_LOCALE_LOCALE_H

#include <cstring>
#include <string>
#include "depend/span.h"
#include "locale/ctype.h"
#include "locale/numpunct.h"

namespace spio {
class locale {
public:
    struct language {
        enum _code { CLASSIC, CUSTOM };

        language(_code c, std::string n = std::string{}) : code(c), name(n) {}

        _code code;
        std::string name;

        std::string str() const
        {
            if (code == CUSTOM) {
                return name;
            }
            return "C";
        }
    };
    struct country {
        enum _code { DEFAULT, CUSTOM };

        country(_code c = DEFAULT, std::string n = std::string{})
            : code(c), name(n)
        {
        }

        _code code;
        std::string name;

        std::string str() const
        {
            if (code == CUSTOM) {
                return name;
            }
            return {};
        }
    };
    struct encoding {
        enum _code { DEFAULT, CUSTOM };

        encoding(_code c = DEFAULT, std::string n = std::string{})
            : code(c), name(n)
        {
        }

        _code code;
        std::string name;

        std::string str() const
        {
            if (code == CUSTOM) {
                return name;
            }
            return {};
        }
    };
    struct other {
        other(std::string d = std::string{}) : data(std::move(d)) {}

        std::string data{};

        std::string str() const
        {
            return data;
        }
    };

    struct id {
        language lang;
        country c;
        encoding enc;
        other o;
    };

    class builder {
    public:
        builder(locale::language l) : m_id{{l}, {}, {}, {}} {}

        builder& language(locale::language l)
        {
            m_id.lang = l;
            return *this;
        }
        builder& country(locale::country c)
        {
            m_id.c = c;
            return *this;
        }
        builder& encoding(locale::encoding e)
        {
            m_id.enc = e;
            return *this;
        }
        builder& other(locale::other o)
        {
            m_id.o = o;
            return *this;
        }

        static id classic()
        {
            return builder(language::CLASSIC).get();
        }

        id get()
        {
            return m_id;
        }

    private:
        id m_id;
    };

    static builder build(language l)
    {
        return builder(l);
    }

    locale(id i) : m_id(i) {}
    locale(const char* name) : locale(_parse(name)) {}

    static locale classic()
    {
        return locale(builder::classic());
    }

    std::string name()
    {
        auto lang = std::string{m_id.lang.str()};

        auto c = m_id.c.str();
        if (c.size() > 0) {
            lang.push_back('_');
            lang.append(c);
        }

        auto enc = m_id.enc.str();
        if (enc.size() > 0) {
            lang.push_back('.');
            lang.append(enc);
        }

        auto o = m_id.o.str();
        if (o.size() > 0) {
            lang.push_back('@');
            lang.append(o);
        }

        return lang;
    }

private:
    id _parse(const char* name)
    {
        if (!name || std::strlen(name) == 0) {
            return builder::classic();
        }

        std::array<char, 4> lang;
        for (auto& ch : lang) {
            if (!(*name) || *name == '_' || *name == '.' || *name == '@') {
                ch = '\0';
                break;
            }
            ch = *name;
            ++name;
        }

        builder b([&]() {
            if (std::strcmp(lang.data(), "C") == 0) {
                return language{language::CLASSIC};
            }
            return language{language::CUSTOM, lang.data()};
        }());

        if (!(*name)) {
            return b.get();
        }

        if (*name == '_') {
            ++name;  // Skip '_'
            std::array<char, 4> c;
            for (auto& ch : c) {
                if (!(*name) || *name == '.' || *name == '@') {
                    ch = '\0';
                    break;
                }
                ch = *name;
                ++name;
            }

            b.country({country::CUSTOM, c.data()});
        }
        if (!(*name)) {
            return b.get();
        }

        if (*name == '.') {
            ++name;  // Skip '.'
            std::string enc;
            while (name && *name != '@') {
                enc.push_back(*name);
                ++name;
            }
            b.encoding({encoding::CUSTOM, enc});
        }
        if (!(*name)) {
            return b.get();
        }

        ++name;  // Skip '@'
        std::string o;
        while (*name) {
            o.push_back(*name);
            ++name;
        }
        b.other(o);
        return b.get();
    }
    id m_id;
};
}  // namespace spio

#endif  // SPIO_LOCALE_LOCALE_H
