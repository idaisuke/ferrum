// The MIT License (MIT)
//
// Copyright (c) 2015 Daisuke Itabashi (https://github.com/idaisuke/ferrum)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __ferrum__json_value__
#define __ferrum__json_value__

#include <stdexcept>
#include <utility>
#include <experimental/optional>
#include "picojson.h"
#include "sqlite_database_column.h"

class entity {
public:
    template <class T>
    bool has() const {
        return _data.find(T::column_name()) != _data.end();
    }

    template <class T>
    std::experimental::optional<typename T::value_type> get() const {
        auto it = _data.find(T::column_name());
        if (it == _data.end()) {
            throw std::logic_error("logic_error");
        }

        if (it->second.template is<picojson::null>()) {
            return std::experimental::nullopt;
        }

        using json_type = decltype(get_json_type(std::declval<typename T::value_type>()));
        if (!it->second.template is<json_type>()) {
            throw std::logic_error("logic_error");
        }

        return static_cast<typename T::value_type>(it->second.template get<json_type>());
    }

    template <class T, class ValueTyoe>
    void set(ValueTyoe &&value) {
        using json_type = decltype(get_json_type(std::declval<typename T::value_type>()));
        _data.emplace(T::column_name(), picojson::value(static_cast<json_type>(std::forward<ValueTyoe>(value))));
    }

    template <class T>
    void set(const std::experimental::nullopt_t &) {
        _data.emplace(T::column_name(), picojson::value());
    }

    operator picojson::value() const & {
        return picojson::value(_data);
    }

    operator picojson::value() && {
        return picojson::value(std::move(_data));
    }

private:
    double get_json_type(int) const;
    double get_json_type(double) const;
    bool get_json_type(bool) const;
    std::string get_json_type(std::string) const;
    std::string get_json_type(const char *) const;

    picojson::object _data;
};

class Chara : public entity {
public:
    static constexpr char *table_name() {
        return "chara";
    }

    struct Id : he::sqlite_database_column<id> {
        static constexpr const char *column_name() {
            return "id";
        }

        using value_type = int;
    };

    struct Name : he::sqlite_database_column<name> {
        static constexpr const char *column_name() {
            return "name";
        }

        using value_type = std::string;
    };
};

#endif /* defined(__ferrum__json_value__) */
