// The MIT License (MIT)
//
// Copyright (c) 2015 Daisuke Itabashi (itabashi.d@gmail.com)
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

#ifndef __ferrum__sqlite_database__
#define __ferrum__sqlite_database__

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include "sqlite3.h"

namespace fe {

    class sqlite_database;
    class sqlite_iterator;
    class sqlite_query;

#pragma mark -
#pragma mark sqlite_blob

    /**
     *  expression of type blob of sqlite.
     */
    struct sqlite_blob {
        const void *data;
        int size;
    };

#pragma mark -
#pragma mark sqlite_exception

    /**
     *  exception thrown when sqlite error occurred.
     */
    class sqlite_exception : public std::runtime_error {
    public:
        sqlite_exception(const char *what);
        sqlite_exception(const std::string &what);
    };

#pragma mark -
#pragma mark sqlite_statement

    /**
     *  pre-compiled sql statement.
     */
    class sqlite_statement {
    public:
        /**
         *  destructor.
         *  finalize this statement if not finalized.
         */
        ~sqlite_statement() noexcept;

        /**
         *  move constructor.
         */
        sqlite_statement(sqlite_statement &&other) noexcept;

        /**
         *  move assignment operator.
         */
        sqlite_statement &operator=(sqlite_statement &&other) noexcept;

        /**
         *  finalize this statement.
         */
        void finalize();

        /**
         *  bind values to placeholder such as ?.
         */
        template <class... ArgType>
        void bind_values(ArgType &&... bind_args);

        /**
         *  bind a value to placeholder such as ? or ?NNN (NNN represents an integer literal).
         *
         *  @param index zero-based index or index equivalent to NNN
         *  @param value value to bind to placeholder
         */
        void bind(int index, int value);

        /**
         *  @copydoc sqlite_statement::bind(int,int)
         */
        void bind(int index, std::int64_t value);

        /**
         *  @copydoc sqlite_statement::bind(int,int)
         */
        void bind(int index, double value);

        /**
         *  @copydoc sqlite_statement::bind(int,int)
         */
        void bind(int index, const std::string &value);

        /**
         *  @copydoc sqlite_statement::bind(int,int)
         */
        void bind(int index, const char *value);

        /**
         *  @copydoc sqlite_statement::bind(int,int)
         */
        void bind(int index, sqlite_blob value);

        /**
         *  bind a value to placeholder such as :VVV or @VVV or $VVV (VVV represents an alphanumeric identifier).
         *
         *  @param parameter_name parameter name equivalent to VVV
         *  @param value          value to bind to placeholder
         */
        void bind(const std::string &parameter_name, int value);

        /**
         *  @copydoc sqlite_statement::bind(const std::string &,int)
         */
        void bind(const std::string &parameter_name, std::int64_t value);

        /**
         *  @copydoc sqlite_statement::bind(const std::string &,int)
         */
        void bind(const std::string &parameter_name, double value);

        /**
         *  @copydoc sqlite_statement::bind(const std::string &,int)
         */
        void bind(const std::string &parameter_name, const std::string &value);

        /**
         *  @copydoc sqlite_statement::bind(const std::string &,int)
         */
        void bind(const std::string &parameter_name, const char *value);

        /**
         *  @copydoc sqlite_statement::bind(const std::string &,int)
         */
        void bind(const std::string &parameter_name, sqlite_blob value);

        /**
         *  clear all binding values.
         */
        void clear_bindings();

    private:
        sqlite_statement(sqlite_database *db, const std::string &sql);
        sqlite_statement(const sqlite_statement &) = delete;
        sqlite_statement &operator=(const sqlite_statement &) = delete;

        sqlite3_stmt **handle();
        void exec();
        void reset();

        int bind_values_internal(int);

        template <class... ArgType>
        int bind_values_internal(int index, int first_arg, ArgType &&... bind_args);

        template <class... ArgType>
        int bind_values_internal(int index, std::int64_t first_arg, ArgType &&... bind_args);

        template <class... ArgType>
        int bind_values_internal(int index, double first_arg, ArgType &&... bind_args);

        template <class... ArgType>
        int bind_values_internal(int index, std::string first_arg, ArgType &&... bind_args);

        template <class... ArgType>
        int bind_values_internal(int index, const char *first_arg, ArgType &&... bind_args);

        template <class... ArgType>
        int bind_values_internal(int index, sqlite_blob first_arg, ArgType &&... bind_args);

        sqlite_database *_db;
        sqlite3_stmt *_stmt;

        friend class sqlite_database;
        friend class sqlite_query;
    };

#pragma mark -
#pragma mark sqlite_cursor

    /**
     *  cursor that fetch data from row.
     */
    class sqlite_cursor {
    public:
        /**
         *  fetch data from current row.
         *
         *  @tparam T int, std::int64_t, double, std::string, const char * or sqlite_blob
         *
         *  @param column_index zero-based index
         */
        template <class T>
        T get(int column_index) const;

        template <class T>
        T get(const std::string &column_name) const;

        /**
         *  get column name.
         *
         *  @param column_index zero-based index
         */
        std::string get_column_name(int column_index) const;

        int get_column_index(const std::string &column_name) const;

        int get_column_count() const;

    private:
        sqlite_cursor(sqlite3_stmt **stmt);

        sqlite3_stmt **_stmt;

        friend class sqlite_iterator;
    };

#pragma mark -
#pragma mark sqlite_iterator

    class sqlite_iterator : public std::iterator<std::forward_iterator_tag, sqlite_cursor> {
    public:
        sqlite_iterator(sqlite_iterator &&other) noexcept;

        sqlite_iterator &operator=(sqlite_iterator &&other) noexcept;

        sqlite_iterator &operator++();

        sqlite_cursor &operator*();

        sqlite_cursor *operator->();

        bool operator==(const sqlite_iterator &other) const;

        bool operator!=(const sqlite_iterator &other) const;

    private:
        sqlite_iterator();
        sqlite_iterator(sqlite_database *db, sqlite3_stmt **stmt);

        sqlite_iterator(const sqlite_iterator &) = delete;
        sqlite_iterator &operator=(const sqlite_iterator &) = delete;

        sqlite_database *_db;
        sqlite3_stmt **_stmt;
        sqlite_cursor _cursor;
        std::uint64_t _rowIndex;
        int _state;

        friend class sqlite_query;
    };

#pragma mark -
#pragma mark sqlite_query

    class sqlite_query {
    public:
        sqlite_iterator begin();
        sqlite_iterator end();

    private:
        sqlite_query(sqlite_database *db, sqlite_statement &&statement);

        sqlite_database *_db;
        sqlite_statement _statement;

        friend class sqlite_database;
    };

#pragma mark -
#pragma mark sqlite_transaction_mode

    /**
     *  @sa https://www.sqlite.org/lang_transaction.html
     */
    enum class sqlite_transaction_mode {
        deferred,
        immediate,
        exclusive,
    };

#pragma mark -
#pragma mark sqlite_transaction

    /**
     *  RAII class for transaction.
     */
    class sqlite_transaction {
    public:
        /**
         *  destructor.
         *  rollback transaction if not committed.
         *  do nothing if committed.
         */
        ~sqlite_transaction() noexcept;

        /**
         *  move constructor.
         */
        sqlite_transaction(sqlite_transaction &&other) noexcept;

        /**
         *  move assignment operator.
         */
        sqlite_transaction &operator=(sqlite_transaction &&other) noexcept;

        /**
         *  commit transaction.
         */
        void commit();

    private:
        /**
         *  constructor.
         *  begin transaction.
         */
        sqlite_transaction(sqlite_database *db, sqlite_transaction_mode mode);

        sqlite_transaction(const sqlite_transaction &) = delete;
        sqlite_transaction &operator=(const sqlite_transaction &) = delete;

        sqlite_database *_db;
        bool _in_transaction;

        friend class sqlite_database;
    };

#pragma mark -
#pragma mark sqlite_listener

    struct sqlite_listener {
        std::function<void(sqlite_database *db, int old_version, int new_version)> on_upgrade;
        std::function<void(sqlite_database *db, int old_version, int new_version)> on_downgrade;
    };

#pragma mark -
#pragma mark sqlite_database

    /**
     *  sqlite database wrapper for C++.
     */
    class sqlite_database {
    public:
        /**
         *  constructor.
         *  create in-memory database.
         */
        sqlite_database();

        /**
         *  constructor.
         *
         *  @param path path of database file
         */
        sqlite_database(const std::string &path);

        /**
         *  destructor.
         *  close database if not closed.
         */
        ~sqlite_database() noexcept;

        /**
         *  move constructor.
         */
        sqlite_database(sqlite_database &&other) noexcept;

        /**
         *  move assignment operator.
         */
        sqlite_database &operator=(sqlite_database &&other) noexcept;

        /**
         *  open database.
         */
        void open();
        void open(const std::string &key);

        /**
         *  close database.
         */
        void close();

        /**
         *  execute sql statement that does not return data (such as select).
         */
        void exec_sql(const std::string &sql);

        /**
         *  execute sql statement that does not return data (such as select).
         *
         *  @param sql       sql statement
         *  @param bind_args values to bind to placeholder such as ?
         */
        template <class... ArgType>
        void exec_sql(const std::string &sql, ArgType &&... bind_args);

        /**
         *  execute pre-compiled sql statement.
         */
        void exec(sqlite_statement &statement);

        template <class... ArgType>
        void exec(sqlite_statement &statement, ArgType &&... bind_args);

        sqlite_query query(const std::string &sql);

        template <class... ArgType>
        sqlite_query query(const std::string &sql, ArgType &&... bind_args);

        sqlite_statement prepare_statement(const std::string &sql);

        /**
         *  begin transaction.
         *
         *  @param mode transaction mode
         */
        void begin_transaction(sqlite_transaction_mode mode = sqlite_transaction_mode::deferred);

        /**
         *  commit transaction.
         */
        void commit_transaction();

        /**
         *  rollback transaction.
         */
        void rollback_transaction();

        /**
         *  return true if database is open, and false otherwise.
         */
        bool is_open() const;

        sqlite_transaction create_transaction(sqlite_transaction_mode mode = sqlite_transaction_mode::deferred);

        const sqlite_listener &get_listener() const;

        void set_listener(const sqlite_listener &listener);

        void set_listener(sqlite_listener &&listener);

        /**
         *  get user version of database.
         *  default value is 0.
         */
        int get_version();

        /**
         *  update user version of database.
         *
         *  @param version new version that must be > 0
         *  @param mode
         */
        void update_version(int version, sqlite_transaction_mode mode = sqlite_transaction_mode::deferred);

        int get_busy_waiting_interval_ms() const;

        void set_busy_waiting_interval_ms(int ms);

        /**
         *  return file path of database or ":memory:" if in-memory database.
         */
        const std::string &get_path() const;

    private:
        sqlite_database(const sqlite_database &) = delete;
        sqlite_database &operator=(const sqlite_database &) = delete;

        int open_internal() noexcept;

        int close_internal() noexcept;

        sqlite3 *handle() const;

        std::string _path;
        sqlite3 *_db = nullptr;
        sqlite_listener _listener;
        int _busy_waiting_interval_ms = 100;

        friend class sqlite_statement;
        friend class sqlite_iterator;
    };

#pragma mark -
#pragma mark sqlite_exception

    inline sqlite_exception::sqlite_exception(const char *what) : std::runtime_error(what) {
    }
    inline sqlite_exception::sqlite_exception(const std::string &what) : std::runtime_error(what) {
    }

#pragma mark -
#pragma mark sqlite_statement

    inline sqlite_statement::~sqlite_statement() noexcept {
        sqlite3_finalize(_stmt);
    }

    inline sqlite_statement::sqlite_statement(sqlite_statement &&other) noexcept : _db(other._db), _stmt(other._stmt) {
        other._db = nullptr;
        other._stmt = nullptr;
    }

    inline sqlite_statement &sqlite_statement::operator=(sqlite_statement &&other) noexcept {
        _db = other._db;
        _stmt = other._stmt;

        other._db = nullptr;
        other._stmt = nullptr;

        return *this;
    }

    inline void sqlite_statement::finalize() {
        auto rc = sqlite3_finalize(_stmt);
        if (rc != SQLITE_OK) {
            throw sqlite_exception("failed to finalize statement, result code = " + std::to_string(rc));
        }
        _stmt = nullptr;
    }

    template <class... ArgType>
    void sqlite_statement::bind_values(ArgType &&... bind_args) {
        auto rc = bind_values_internal(1, std::forward<ArgType>(bind_args)...);
        if (rc != SQLITE_OK) {
            throw sqlite_exception("failed to bind arguments, result code = " + std::to_string(rc));
        }
    }

    inline void sqlite_statement::bind(int index, int value) {
        auto rc = sqlite3_bind_int(_stmt, index + 1, value);
        if (rc != SQLITE_OK) {
            throw sqlite_exception("failed to bind int, result code = " + std::to_string(rc));
        }
    }

    inline void sqlite_statement::bind(int index, std::int64_t value) {
        auto rc = sqlite3_bind_int64(_stmt, index + 1, value);
        if (rc != SQLITE_OK) {
            throw sqlite_exception("failed to bind std::int64_t, result code = " + std::to_string(rc));
        }
    }

    inline void sqlite_statement::bind(int index, double value) {
        auto rc = sqlite3_bind_double(_stmt, index + 1, value);
        if (rc != SQLITE_OK) {
            throw sqlite_exception("failed to bind double, result code = " + std::to_string(rc));
        }
    }

    inline void sqlite_statement::bind(int index, const std::string &value) {
        auto rc =
            sqlite3_bind_text(_stmt, index + 1, value.c_str(), static_cast<int>(value.length()), SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            throw sqlite_exception("failed to bind std::string, result code = " + std::to_string(rc));
        }
    }

    inline void sqlite_statement::bind(int index, const char *value) {
        auto rc = sqlite3_bind_text(_stmt, index + 1, value, static_cast<int>(std::strlen(value)), SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            throw sqlite_exception("failed to bind const char *, result code = " + std::to_string(rc));
        }
    }

    inline void sqlite_statement::bind(int index, sqlite_blob value) {
        auto rc = sqlite3_bind_blob(_stmt, index + 1, value.data, value.size, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            throw sqlite_exception("failed to bind const char *, result code = " + std::to_string(rc));
        }
    }

    inline void sqlite_statement::bind(const std::string &parameter_name, int value) {
        auto index = sqlite3_bind_parameter_index(_stmt, parameter_name.c_str());
        if (index == 0) {
            throw sqlite_exception("no matching parameter named '" + parameter_name + "' is found.");
        }

        bind(index - 1, value);
    }

    inline void sqlite_statement::bind(const std::string &parameter_name, std::int64_t value) {
        auto index = sqlite3_bind_parameter_index(_stmt, parameter_name.c_str());
        if (index == 0) {
            throw sqlite_exception("no matching parameter named '" + parameter_name + "' is found.");
        }

        bind(index - 1, value);
    }

    inline void sqlite_statement::bind(const std::string &parameter_name, double value) {
        auto index = sqlite3_bind_parameter_index(_stmt, parameter_name.c_str());
        if (index == 0) {
            throw sqlite_exception("no matching parameter named '" + parameter_name + "' is found.");
        }

        bind(index - 1, value);
    }

    inline void sqlite_statement::bind(const std::string &parameter_name, const std::string &value) {
        auto index = sqlite3_bind_parameter_index(_stmt, parameter_name.c_str());
        if (index == 0) {
            throw sqlite_exception("no matching parameter named '" + parameter_name + "' is found.");
        }

        bind(index - 1, value);
    }

    inline void sqlite_statement::bind(const std::string &parameter_name, const char *value) {
        auto index = sqlite3_bind_parameter_index(_stmt, parameter_name.c_str());
        if (index == 0) {
            throw sqlite_exception("no matching parameter named '" + parameter_name + "' is found.");
        }

        bind(index - 1, value);
    }

    inline void sqlite_statement::bind(const std::string &parameter_name, sqlite_blob value) {
        auto index = sqlite3_bind_parameter_index(_stmt, parameter_name.c_str());
        if (index == 0) {
            throw sqlite_exception("no matching parameter named '" + parameter_name + "' is found.");
        }

        bind(index - 1, value);
    }

    inline void sqlite_statement::clear_bindings() {
        auto rc = sqlite3_clear_bindings(_stmt);
        if (rc != SQLITE_OK) {
            throw sqlite_exception("failed to clear bindings, result code = " + std::to_string(rc));
        }
    }

    inline sqlite_statement::sqlite_statement(sqlite_database *db, const std::string &sql) : _db(db) {
        auto rc = sqlite3_prepare_v2(db->handle(), sql.c_str(), static_cast<int>(sql.length()), &_stmt, nullptr);
        if (rc != SQLITE_OK) {
            throw sqlite_exception("failed to prepare statement, sql = \"" + sql + "\", result code = " +
                                   std::to_string(rc));
        }
    }

    inline sqlite3_stmt **sqlite_statement::handle() {
        return &_stmt;
    }

    inline void sqlite_statement::exec() {
        auto rc = sqlite3_step(_stmt);

        while (rc == SQLITE_BUSY) {
            std::this_thread::sleep_for(std::chrono::milliseconds(_db->get_busy_waiting_interval_ms()));
            rc = sqlite3_step(_stmt);
        }

        if (rc == SQLITE_ROW) {
            throw sqlite_exception("this method must not be any sql statement that returns data.");
        }

        if (rc != SQLITE_DONE) {
            throw sqlite_exception("failed to exec statement, result code = " + std::to_string(rc));
        }
    }

    inline void sqlite_statement::reset() {
        auto rc = sqlite3_reset(_stmt);
        if (rc != SQLITE_OK) {
            throw sqlite_exception("failed to reset statement, result code = " + std::to_string(rc));
        }
    }

    int sqlite_statement::bind_values_internal(int) {
        return SQLITE_OK;
    }

    template <class... ArgType>
    int sqlite_statement::bind_values_internal(int index, int first_arg, ArgType &&... bind_args) {
        auto rc = sqlite3_bind_int(_stmt, index, first_arg);
        if (rc != SQLITE_OK) {
            return rc;
        }
        return bind_values_internal(index + 1, std::forward<ArgType>(bind_args)...);
    }

    template <class... ArgType>
    int sqlite_statement::bind_values_internal(int index, std::int64_t first_arg, ArgType &&... bind_args) {
        auto rc = sqlite3_bind_int64(_stmt, index, first_arg);
        if (rc != SQLITE_OK) {
            return rc;
        }
        return bind_values_internal(index + 1, std::forward<ArgType>(bind_args)...);
    }

    template <class... ArgType>
    int sqlite_statement::bind_values_internal(int index, double first_arg, ArgType &&... bind_args) {
        auto rc = sqlite3_bind_double(_stmt, index, first_arg);
        if (rc != SQLITE_OK) {
            return rc;
        }
        return bind_values_internal(index + 1, std::forward<ArgType>(bind_args)...);
    }

    template <class... ArgType>
    int sqlite_statement::bind_values_internal(int index, std::string first_arg, ArgType &&... bind_args) {
        auto rc =
            sqlite3_bind_text(_stmt, index, first_arg.c_str(), static_cast<int>(first_arg.length()), SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            return rc;
        }
        return bind_values_internal(index + 1, std::forward<ArgType>(bind_args)...);
    }

    template <class... ArgType>
    int sqlite_statement::bind_values_internal(int index, const char *first_arg, ArgType &&... bind_args) {
        auto rc =
            sqlite3_bind_text(_stmt, index, first_arg, static_cast<int>(std::strlen(first_arg)), SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            return rc;
        }
        return bind_values_internal(index + 1, std::forward<ArgType>(bind_args)...);
    }

    template <class... ArgType>
    int sqlite_statement::bind_values_internal(int index, sqlite_blob first_arg, ArgType &&... bind_args) {
        auto rc = sqlite3_bind_blob(_stmt, index, first_arg.data, first_arg.size, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            return rc;
        }
        return bind_values_internal(index + 1, std::forward<ArgType>(bind_args)...);
    }

#pragma mark -
#pragma mark sqlite_cursor

    template <>
    inline int sqlite_cursor::get(int column_index) const {
        return sqlite3_column_int(*_stmt, column_index);
    }

    template <>
    inline std::int64_t sqlite_cursor::get(int column_index) const {
        return sqlite3_column_int64(*_stmt, column_index);
    }

    template <>
    inline double sqlite_cursor::get(int column_index) const {
        return sqlite3_column_double(*_stmt, column_index);
    }

    template <>
    inline const unsigned char *sqlite_cursor::get(int column_index) const {
        return sqlite3_column_text(*_stmt, column_index);
    }

    template <>
    inline const char *sqlite_cursor::get(int column_index) const {
        return reinterpret_cast<const char *>(sqlite3_column_text(*_stmt, column_index));
    }

    template <>
    inline std::string sqlite_cursor::get(int column_index) const {
        return reinterpret_cast<const char *>(sqlite3_column_text(*_stmt, column_index));
    }

    template <>
    inline const void *sqlite_cursor::get(int column_index) const {
        return sqlite3_column_blob(*_stmt, column_index);
    }

    template <class T>
    T sqlite_cursor::get(const std::string &column_name) const {
        auto index = get_column_index(column_name);
        if (index == -1) {
            throw sqlite_exception("column named '" + column_name + "' does not exist.");
        }
        return get<T>(index);
    }

    inline std::string sqlite_cursor::get_column_name(int column_index) const {
        return sqlite3_column_name(*_stmt, column_index);
    }

    inline int sqlite_cursor::get_column_index(const std::string &column_name) const {
        for (int i = 0, n = sqlite3_column_count(*_stmt); i < n; ++i) {
            if (0 == std::strcmp(sqlite3_column_name(*_stmt, i), column_name.c_str())) {
                return i;
            }
        }
        return -1;
    }

    inline int sqlite_cursor::get_column_count() const {
        return sqlite3_column_count(*_stmt);
    }

    inline sqlite_cursor::sqlite_cursor(sqlite3_stmt **stmt) : _stmt(stmt) {
    }

#pragma mark -
#pragma mark sqlite_iterator

    inline sqlite_iterator::sqlite_iterator(sqlite_iterator &&other) noexcept : _db(other._db),
                                                                                _stmt(other._stmt),
                                                                                _cursor(other._stmt),
                                                                                _rowIndex(other._rowIndex),
                                                                                _state(other._state) {
        other._db = nullptr;
        other._stmt = nullptr;
        other._rowIndex = 0;
        other._state = SQLITE_DONE;
    }

    inline sqlite_iterator &sqlite_iterator::operator=(sqlite_iterator &&other) noexcept {
        _db = other._db;
        _stmt = other._stmt;
        _rowIndex = other._rowIndex;
        _state = other._state;

        other._db = nullptr;
        other._stmt = nullptr;
        other._rowIndex = 0;
        other._state = SQLITE_DONE;
        return *this;
    }

    inline sqlite_iterator &sqlite_iterator::operator++() {
        _state = sqlite3_step(*_stmt);
        while (_state == SQLITE_BUSY) {
            std::this_thread::sleep_for(std::chrono::milliseconds(_db->get_busy_waiting_interval_ms()));
            _state = sqlite3_step(*_stmt);
        }
        _rowIndex++;
        return *this;
    }

    inline sqlite_cursor &sqlite_iterator::operator*() {
        return _cursor;
    }

    inline bool sqlite_iterator::operator==(const sqlite_iterator &other) const {
        if (_state == other._state) {
            if (_state == SQLITE_DONE) {
                return true;
            } else {
                return _rowIndex == other._rowIndex;
            }
        }
        return false;
    }

    inline bool sqlite_iterator::operator!=(const sqlite_iterator &other) const {
        return !operator==(other);
    }

    inline sqlite_iterator::sqlite_iterator()
        : _db(nullptr), _stmt(nullptr), _cursor(_stmt), _rowIndex(0), _state(SQLITE_DONE) {
    }

    inline sqlite_iterator::sqlite_iterator(sqlite_database *db, sqlite3_stmt **stmt)
        : _db(db), _stmt(stmt), _cursor(_stmt), _rowIndex(0), _state(sqlite3_step(*stmt)) {
    }

#pragma mark -
#pragma mark sqlite_query

    inline sqlite_iterator sqlite_query::begin() {
        _statement.reset();
        return sqlite_iterator(_db, _statement.handle());
    }

    inline sqlite_iterator sqlite_query::end() {
        return sqlite_iterator();
    }

    inline sqlite_query::sqlite_query(sqlite_database *db, sqlite_statement &&statement)
        : _db(db), _statement(std::move(statement)) {
    }

#pragma mark -
#pragma mark sqlite_transaction

    inline sqlite_transaction::~sqlite_transaction() noexcept {
        if (_in_transaction && _db) {
            try {
                _db->rollback_transaction();
            } catch (...) {
                // ignore
            }
        }
    }

    inline sqlite_transaction::sqlite_transaction(sqlite_transaction &&other) noexcept
        : _db(other._db),
          _in_transaction(other._in_transaction) {
        other._db = nullptr;
        other._in_transaction = false;
    }

    inline sqlite_transaction &sqlite_transaction::operator=(sqlite_transaction &&other) noexcept {
        _db = other._db;
        _in_transaction = other._in_transaction;

        other._db = nullptr;
        other._in_transaction = false;

        return *this;
    }

    inline void sqlite_transaction::commit() {
        _db->commit_transaction();
        _in_transaction = false;
    }

    inline sqlite_transaction::sqlite_transaction(sqlite_database *db, sqlite_transaction_mode mode) : _db(db) {
        _db->begin_transaction(mode);
        _in_transaction = true;
    }

#pragma mark -
#pragma mark sqlite_database

    inline sqlite_database::sqlite_database() : _path(":memory:") {
    }

    inline sqlite_database::sqlite_database(const std::string &path) : _path(path) {
    }

    inline sqlite_database::~sqlite_database() noexcept {
        close_internal();
    }

    inline sqlite_database::sqlite_database(sqlite_database &&other) noexcept : _db(other._db),
                                                                                _path(std::move(other._path)) {
        other._db = nullptr;
        other._path = "";
    }

    inline sqlite_database &sqlite_database::operator=(sqlite_database &&other) noexcept {
        _db = other._db;
        _path = std::move(other._path);

        other._db = nullptr;
        other._path = "";

        return *this;
    }

    inline void sqlite_database::open() {
        auto rc = open_internal();
        if (rc != SQLITE_OK) {
            throw sqlite_exception("failed to open db, result code = " + std::to_string(rc));
        }
    }

    inline void sqlite_database::open(const std::string &key) {
        open();

#ifdef SQLITE_HAS_CODEC
        sqlite3_key(_db, key.c_str(), key.length());
#endif
    }

    inline void sqlite_database::close() {
        auto rc = close_internal();
        if (rc != SQLITE_OK) {
            throw sqlite_exception("failed to close db, result code = " + std::to_string(rc));
        }
    }

    inline void sqlite_database::exec_sql(const std::string &sql) {
        char *error = nullptr;
        if (SQLITE_OK != sqlite3_exec(_db, sql.c_str(), nullptr, nullptr, &error)) {
            throw sqlite_exception("failed to exec sql, sql = \"" + sql + "\", reason = \"" + error + "\"");
        }
    }

    template <class... ArgType>
    void sqlite_database::exec_sql(const std::string &sql, ArgType &&... bind_args) {
        sqlite_statement statement(this, sql);
        statement.bind_values(std::forward<ArgType>(bind_args)...);
        statement.exec();
        statement.finalize();
    }

    inline void sqlite_database::exec(sqlite_statement &statement) {
        statement.reset();
        statement.exec();
    }

    template <class... ArgType>
    void sqlite_database::exec(sqlite_statement &statement, ArgType &&... bind_args) {
        statement.reset();
        statement.clear_bindings();
        statement.bind_values(std::forward<ArgType>(bind_args)...);
        statement.exec();
    }

    inline sqlite_query sqlite_database::query(const std::string &sql) {
        return sqlite_query(this, sqlite_statement(this, sql));
    }

    template <class... ArgType>
    sqlite_query sqlite_database::query(const std::string &sql, ArgType &&... bind_args) {
        sqlite_statement statement(this, sql);
        statement.bind_values(std::forward<ArgType>(bind_args)...);
        return sqlite_query(this, std::move(statement));
    }

    inline sqlite_statement sqlite_database::prepare_statement(const std::string &sql) {
        return sqlite_statement(this, sql);
    }

    inline void sqlite_database::begin_transaction(sqlite_transaction_mode mode) {
        char *error = nullptr;
        const char *sql;
        switch (mode) {
            case sqlite_transaction_mode::deferred:
                sql = "BEGIN DEFERRED;";
                break;

            case sqlite_transaction_mode::immediate:
                sql = "BEGIN IMMEDIATE;";
                break;

            case sqlite_transaction_mode::exclusive:
                sql = "BEGIN EXCLUSIVE;";
                break;

            default:
                throw sqlite_exception("invalid sqlite transaction mode, = " + std::to_string(static_cast<int>(mode)));
        }

        if (SQLITE_OK != sqlite3_exec(_db, sql, nullptr, nullptr, &error)) {
            throw sqlite_exception(std::string("failed to begin transaction, reason = \"") + error + "\"");
        }
    }

    inline void sqlite_database::commit_transaction() {
        char *error = nullptr;
        if (SQLITE_OK != sqlite3_exec(_db, "COMMIT;", nullptr, nullptr, &error)) {
            throw sqlite_exception(std::string("failed to commit transaction, reason = \"") + error + "\"");
        }
    }

    inline void sqlite_database::rollback_transaction() {
        char *error = nullptr;
        if (SQLITE_OK != sqlite3_exec(_db, "ROLLBACK;", nullptr, nullptr, &error)) {
            throw sqlite_exception(std::string("failed to rollback transaction, reason = \"") + error + "\"");
        }
    }

    inline bool sqlite_database::is_open() const {
        return _db;
    }

    inline sqlite_transaction sqlite_database::create_transaction(sqlite_transaction_mode mode) {
        return sqlite_transaction(this, mode);
    }

    inline const sqlite_listener &sqlite_database::get_listener() const {
        return _listener;
    }

    void sqlite_database::set_listener(const sqlite_listener &listener) {
        _listener = listener;
    }

    void sqlite_database::set_listener(sqlite_listener &&listener) {
        _listener = std::move(listener);
    }

    inline int sqlite_database::get_version() {
        sqlite_query query(this, sqlite_statement(this, "PRAGMA user_version;"));

        int result = 0;
        for (auto &&cursor : query) {
            result = cursor.get<int>(0);
        }

        return result;
    }

    inline void sqlite_database::update_version(int version, sqlite_transaction_mode mode) {
        auto old_version = get_version();
        if (old_version == version) {
            return;
        }

        auto transaction = create_transaction(mode);

        if (old_version < version) {
            if (_listener.on_upgrade) {
                _listener.on_upgrade(this, old_version, version);
            }
        } else if (old_version > version) {
            if (_listener.on_downgrade) {
                _listener.on_downgrade(this, old_version, version);
            }
        }

        exec_sql("PRAGMA user_version = " + std::to_string(version) + ";");

        transaction.commit();
    }

    int sqlite_database::get_busy_waiting_interval_ms() const {
        return _busy_waiting_interval_ms;
    }

    void sqlite_database::set_busy_waiting_interval_ms(int ms) {
        _busy_waiting_interval_ms = ms;
    }

    inline const std::string &sqlite_database::get_path() const {
        return _path;
    }

    inline int sqlite_database::open_internal() noexcept {
        auto rc = sqlite3_open(_path.c_str(), &_db);
        if (rc != SQLITE_OK) {
            sqlite3_close(_db);
            _db = nullptr;
            return rc;
        }

        return rc;
    }

    inline int sqlite_database::close_internal() noexcept {
        if (!_db) {
            return SQLITE_OK;
        }

        sqlite3_stmt *stmt = nullptr;
        while ((stmt = sqlite3_next_stmt(_db, stmt)) != nullptr) {
            auto rc = sqlite3_finalize(stmt);
            if (rc != SQLITE_OK) {
                return rc;
            }
        }

        auto rc = sqlite3_close(_db);
        if (rc == SQLITE_OK) {
            _db = nullptr;
        }
        return rc;
    }

    inline sqlite3 *sqlite_database::handle() const {
        return _db;
    }
}

#endif /* defined(__ferrum__sqlite_database__) */
