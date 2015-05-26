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

#ifndef __ferrum__sqlite_database_column__
#define __ferrum__sqlite_database_column__

#include <cstdint>
#include <iostream>
#include <string>
#include <utility>

namespace fe {

    class sqlite_expression {
    public:
        sqlite_expression() : _expression() {
        }

        sqlite_expression(const char *expression) : _expression(expression) {
        }

        sqlite_expression(const std::string &expression) : _expression(expression) {
        }

        sqlite_expression(std::string &&expression) : _expression(std::move(expression)) {
        }

        operator std::string() const & {
            return _expression;
        }

        operator std::string() && {
            return std::move(_expression);
        }

        template <class Expression>
        sqlite_expression operator&&(Expression &&other) const & {
            return "(" + _expression + ") AND (" + static_cast<std::string>(std::forward<Expression>(other)) + ")";
        }

        template <class Expression>
        sqlite_expression operator&&(Expression &&other) && {
            return "(" + std::move(_expression) + ") AND (" +
                   static_cast<std::string>(std::forward<Expression>(other)) + ")";
        }

        template <class Expression>
        sqlite_expression operator||(Expression &&other) const & {
            return "(" + _expression + ") OR (" + static_cast<std::string>(std::forward<Expression>(other)) + ")";
        }

        template <class Expression>
        sqlite_expression operator||(Expression &&other) && {
            return "(" + std::move(_expression) + ") OR (" + static_cast<std::string>(std::forward<Expression>(other)) +
                   ")";
        }

        sqlite_expression operator!() const & {
            return "NOT (" + _expression + ")";
        }
        sqlite_expression operator!() && {
            return "NOT (" + std::move(_expression) + ")";
        }

        friend std::ostream &operator<<(std::ostream &os, const sqlite_expression &expression) {
            os << expression._expression;
            return os;
        }

        friend std::ostream &operator<<(std::ostream &os, sqlite_expression &&expression) {
            os << std::move(expression._expression);
            return os;
        }

    private:
        std::string _expression;
    };

    template <class T>
    struct sqlite_database_column {
        static std::string name() {
            return T::column_name();
        }

        sqlite_expression operator==(int value) const {
            return "'" + name() + "' == " + std::to_string(value);
        }
        sqlite_expression operator==(std::int64_t value) const {
            return "'" + name() + "' == " + std::to_string(value);
        }
        sqlite_expression operator==(double value) const {
            return "'" + name() + "' == " + std::to_string(value);
        }
        sqlite_expression operator==(const char *value) const {
            return "'" + name() + "' == '" + value + "'";
        }
        sqlite_expression operator==(const std::string &value) const {
            return "'" + name() + "' == '" + value + "'";
        }

        sqlite_expression operator!=(int value) const {
            return "'" + name() + "' != " + std::to_string(value);
        }
        sqlite_expression operator!=(std::int64_t value) const {
            return "'" + name() + "' != " + std::to_string(value);
        }
        sqlite_expression operator!=(double value) const {
            return "'" + name() + "' != " + std::to_string(value);
        }
        sqlite_expression operator!=(const char *value) const {
            return "'" + name() + "' != '" + value + "'";
        }
        sqlite_expression operator!=(const std::string &value) const {
            return "'" + name() + "' != '" + value + "'";
        }

        sqlite_expression operator>(int value) const {
            return "'" + name() + "' > " + std::to_string(value);
        }
        sqlite_expression operator>(std::int64_t value) const {
            return "'" + name() + "' > " + std::to_string(value);
        }
        sqlite_expression operator>(double value) const {
            return "'" + name() + "' > " + std::to_string(value);
        }
        sqlite_expression operator>(const char *value) const {
            return "'" + name() + "' > '" + value + "'";
        }
        sqlite_expression operator>(const std::string &value) const {
            return "'" + name() + "' > '" + value + "'";
        }

        sqlite_expression operator>=(int value) const {
            return "'" + name() + "' >= " + std::to_string(value);
        }
        sqlite_expression operator>=(std::int64_t value) const {
            return "'" + name() + "' >= " + std::to_string(value);
        }
        sqlite_expression operator>=(double value) const {
            return "'" + name() + "' >= " + std::to_string(value);
        }
        sqlite_expression operator>=(const char *value) const {
            return "'" + name() + "' >= '" + value + "'";
        }
        sqlite_expression operator>=(const std::string &value) const {
            return "'" + name() + "' >= '" + value + "'";
        }

        sqlite_expression operator<(int value) const {
            return "'" + name() + "' < " + std::to_string(value);
        }
        sqlite_expression operator<(std::int64_t value) const {
            return "'" + name() + "' < " + std::to_string(value);
        }
        sqlite_expression operator<(double value) const {
            return "'" + name() + "' < " + std::to_string(value);
        }
        sqlite_expression operator<(const char *value) const {
            return "'" + name() + "' < '" + value + "'";
        }
        sqlite_expression operator<(const std::string &value) const {
            return "'" + name() + "' < '" + value + "'";
        }

        sqlite_expression operator<=(int value) const {
            return "'" + name() + "' <= " + std::to_string(value);
        }
        sqlite_expression operator<=(std::int64_t value) const {
            return "'" + name() + "' <= " + std::to_string(value);
        }
        sqlite_expression operator<=(double value) const {
            return "'" + name() + "' <= " + std::to_string(value);
        }
        sqlite_expression operator<=(const char *value) const {
            return "'" + name() + "' <= '" + value + "'";
        }
        sqlite_expression operator<=(const std::string &value) const {
            return "'" + name() + "' <= '" + value + "'";
        }
    };
}

#endif /* defined(__ferrum__sqlite_database_column__) */
